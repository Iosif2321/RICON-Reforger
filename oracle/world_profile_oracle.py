#!/usr/bin/env python3
"""Host-эталон поставщика профиля из мира по docs/req/REQ-L2-world-profile.md (оболочка S1). stdlib. Не production.

Поставщик строит профиль по правилу REQ-L2 §6 из абстрактного источника высот (REQ-WP-01…06). Здесь источник —
синтетическая формула (REQ §4, TEST_ONLY), которую так же можно вычислить в Enforce; вода — NOT_MODELLED. Мира и
движка эталон не касается: смысл `GetSurfaceY` остаётся открытым вопросом (REQ §6).

`model_losses` повторяет `RICON_TerrainDiffractionModel.AppendLosses` (`RICON_TerrainDiffractionModel.c:792`),
сам расчёт дифракции — `l2_terrain_oracle.l2`, свободное пространство — `ricon_oracle.free_space_loss`.
`provider_case` вызывает поставщика напрямую на границах входа (REQ §5, `level = "provider"`).

Режимы: `--check FILE` пересчитывает векторы и сравнивает; `--write FILE` записывает их заново.
"""

from __future__ import annotations

import argparse
import copy
import json
import math
import sys
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parent))  # own directory only
import l2_terrain_oracle as l2o  # noqa: E402
import ricon_oracle as ro  # noqa: E402

OK = "OK"
UNAVAILABLE = "TERRAIN_PROFILE_UNAVAILABLE"
NOT_CALLED = "NOT_CALLED"
L2_TO_REASON = {  # REQ-L2 §6 mapping
    "INVALID_CONFIG": "CATALOG_VIOLATION",
    "OUT_OF_DOMAIN_FREQUENCY": "OUT_OF_MODEL_DOMAIN",
    "INVALID_PROFILE": UNAVAILABLE,
    "PROFILE_STEP_TOO_LARGE": UNAVAILABLE,
    "INVALID_ANTENNA_HEIGHT": UNAVAILABLE,
    "NUMERIC_FAILURE": "NUMERIC_FAILURE",
    "OK": OK,
}
SOURCE_KEYS = {"base", "slopeX", "slopeZ", "hills", "failAtCall", "nanAtCall"}
NON_FINITE = {"NaN": math.nan, "+Inf": math.inf, "-Inf": -math.inf}  # REQ §5: JSON spelling of non-finite numbers
TOLERANCE_ABS = 1e-9  # host recompute is deterministic float64; any larger difference is a change of the oracle
ENFORCE_TOLERANCE = {"distanceKmRel": 1e-6, "heightM": 0.01, "lossDb": 0.05}  # REQ §5


class SyntheticSource:
    """REQ §4: analytic height, failure by call number (REQ-WP-01 contract); counts every call (REQ-WP-05)."""

    def __init__(self, spec: dict[str, Any]):
        unknown = set(spec) - SOURCE_KEYS
        if unknown:
            raise ValueError(f"unknown source keys {sorted(unknown)}")
        self.spec = spec
        self.calls = 0

    def height(self, x: float, z: float) -> float:
        s = self.spec
        h = s.get("base", 0.0) + s.get("slopeX", 0.0) * x + s.get("slopeZ", 0.0) * z
        for hill in s.get("hills", []):
            r2 = (x - hill["x"]) ** 2 + (z - hill["z"]) ** 2
            h += hill["height"] * math.exp(-r2 / (2.0 * hill["sigma"] ** 2))
        return h

    def try_get_height(self, x: float, z: float) -> tuple[bool, float]:
        call = self.calls
        self.calls += 1
        if self.spec.get("failAtCall") == call:
            return False, 0.0
        if self.spec.get("nanAtCall") == call:
            return True, math.nan
        return True, self.height(x, z)


def num(value: Any) -> float:
    return NON_FINITE[value] if isinstance(value, str) else float(value)


def config_valid(cfg: Any) -> bool:
    """REQ-L2 §2 config domain, same predicate as the L2 validator's INVALID_CONFIG branch."""
    return isinstance(cfg, dict) and l2o.validate({"config": cfg, "frequencyMHz": l2o.MIN_FREQUENCY_MHZ}) \
        != "INVALID_CONFIG"


def horizontal_distance_m(tx: list[float], rx: list[float]) -> float:
    return math.sqrt((rx[0] - tx[0]) ** 2 + (rx[2] - tx[2]) ** 2)


def sample_point(tx: list[float], rx: list[float], i: int, n: int) -> tuple[float, float]:
    """REQ-WP-03 step 5: ends are exactly the antenna positions, inner points by t = i/(n-1)."""
    if i == 0:
        return tx[0], tx[2]
    if i == n - 1:
        return rx[0], rx[2]
    t = i / (n - 1)
    return tx[0] + (rx[0] - tx[0]) * t, tx[2] + (rx[2] - tx[2]) * t


def get_profile(tx: list[float] | None, rx: list[float] | None, cfg: Any,
                source: SyntheticSource | None) -> dict[str, Any]:
    """REQ-WP-03 in order; every refusal is TERRAIN_PROFILE_UNAVAILABLE with an empty result.
    `tx is None` stands for a null query."""
    refused = {"reason": UNAVAILABLE}
    if tx is None or rx is None or not config_valid(cfg):                      # step 1
        return refused
    if source is None:                                                       # step 2
        return refused
    if not all(math.isfinite(v) for v in (*tx, *rx)):                        # step 3
        return refused
    code, ds = l2o.provider_sampling(horizontal_distance_m(tx, rx), cfg)       # step 4
    if code != "OK":
        return refused
    n = len(ds)
    heights = []
    for i in range(n):                                                       # step 5
        ok, h = source.try_get_height(*sample_point(tx, rx, i, n))
        if not ok or not math.isfinite(h):                                   # REQ-WP-04
            return refused
        heights.append(h)
    htg, hrg = tx[1] - heights[0], rx[1] - heights[-1]                         # step 7
    if not (math.isfinite(htg) and math.isfinite(hrg)):
        return refused
    return {"reason": OK, "profile": [[d, h] for d, h in zip(ds, heights)], "htg": htg, "hrg": hrg}  # step 6


def make_source(spec: dict[str, Any] | None) -> SyntheticSource | None:
    return SyntheticSource(spec) if spec is not None else None


def provider_case(inp: dict[str, Any]) -> dict[str, Any]:
    source = make_source(inp["source"])
    tx = None if inp["nullQuery"] else [num(v) for v in inp["txPos"]]
    rx = None if inp["nullQuery"] else [num(v) for v in inp["rxPos"]]
    out = get_profile(tx, rx, inp["config"], source)
    result: dict[str, Any] = {"providerReason": out["reason"], "heightCalls": source.calls if source else 0}
    if out["reason"] == OK:
        result.update(n=len(out["profile"]), profile=out["profile"], htg=out["htg"], hrg=out["hrg"])
    return result


def model_losses(inp: dict[str, Any]) -> dict[str, Any]:
    """RICON_TerrainDiffractionModel.AppendLosses with the world provider (REQ-L2 §6 + REQ-WP)."""
    tx, rx, f, cfg = inp["txPos"], inp["rxPos"], inp["frequencyMHz"], inp["config"]
    source = make_source(inp["source"])
    out: dict[str, Any] = {"providerReason": NOT_CALLED}

    def finish(result: dict[str, Any]) -> dict[str, Any]:
        result["heightCalls"] = source.calls if source else 0
        return result

    if not config_valid(cfg):                     # IsConfigValid (the provider object always exists here)
        return finish({**out, "modelReason": "CATALOG_VIOLATION"})
    if not (math.isfinite(f) and l2o.MIN_FREQUENCY_MHZ <= f <= l2o.MAX_FREQUENCY_MHZ):
        return finish({**out, "modelReason": "OUT_OF_MODEL_DOMAIN"})
    fspl = ro.free_space_loss(math.dist(tx, rx), f)["lossDb"]  # 3D distance, REQ-LE-40
    if horizontal_distance_m(tx, rx) < 2.0:
        return finish({**out, "modelReason": OK, "layers": {"fspl": fspl, "terrain_diffraction": 0.0}})
    prof = get_profile(tx, rx, cfg, source)
    out["providerReason"] = prof["reason"]
    if prof["reason"] != OK:
        return finish({**out, "modelReason": prof["reason"]})
    out.update(n=len(prof["profile"]), profile=prof["profile"], htg=prof["htg"], hrg=prof["hrg"])
    res = l2o.l2({"profile": prof["profile"], "htg": prof["htg"], "hrg": prof["hrg"], "frequencyMHz": f,
                  "config": cfg})
    out["l2Code"] = res["code"]
    reason = L2_TO_REASON[res["code"]]
    if reason != OK:
        return finish({**out, "modelReason": reason})
    return finish({**out, "modelReason": OK, "layers": {"fspl": fspl, "terrain_diffraction": res["L"]}})


# ---------------------------------------------------------------- cases (REQ §5)

CFG = {"earthRadiusKm": 6371.0, "kFactor": 4.0 / 3.0, "permittivity": 15.0, "conductivitySm": 0.001,
       "polarization": "VERTICAL", "maxProfileStepKm": 0.1, "maxProfilePoints": 1000}  # TEST_ONLY (REQ-L2 §2)
DEFAULT = object()


def case(cid: str, ct: str, note: str, tx: list[float], rx: list[float], source: dict[str, Any] | None,
         f: float = 50.0, cfg: dict[str, Any] | None = None) -> dict[str, Any]:
    return {"id": cid, "ct": ct, "level": "model", "note": note,
            "input": {"txPos": tx, "rxPos": rx, "frequencyMHz": f, "config": copy.deepcopy(cfg or CFG),
                      "source": source}}


def pcase(cid: str, note: str, tx: list[Any], rx: list[Any], source: Any = DEFAULT, cfg: Any = DEFAULT,
          null_query: bool = False) -> dict[str, Any]:
    return {"id": cid, "ct": "CT-WP-006", "level": "provider", "note": note,
            "input": {"nullQuery": null_query, "txPos": tx, "rxPos": rx,
                      "config": copy.deepcopy(CFG) if cfg is DEFAULT else cfg,
                      "source": {"base": 0.0} if source is DEFAULT else source}}


def build_cases() -> list[dict[str, Any]]:
    plane = {"base": 50.0, "slopeX": 0.01, "slopeZ": -0.005}
    g = SyntheticSource(plane).height
    tx3, rx3 = [100.0, g(100, 200) + 2.0, 200.0], [1300.0, g(1300, 1100) + 2.0, 1100.0]
    hill = {"base": 0.0, "hills": [{"x": 2500.0, "z": 0.0, "height": 80.0, "sigma": 300.0}]}
    map_src = {"base": 30.0, "slopeX": 0.002, "slopeZ": 0.001,
               "hills": [{"x": 6000.0, "z": 5500.0, "height": 120.0, "sigma": 800.0},
                         {"x": 9000.0, "z": 8000.0, "height": 60.0, "sigma": 400.0}]}
    gm = SyntheticSource(map_src).height
    small_cfg = dict(CFG, maxProfilePoints=20)
    t0, r0 = [0.0, 2.0, 0.0], [2000.0, 2.0, 0.0]
    cases = [
        case("W01", "CT-WP-001", "flat ground along x", [0.0, 101.5, 0.0], [2000.0, 101.5, 0.0], {"base": 100.0}),
        case("W02", "CT-WP-002", "flat ground along z", [0.0, 101.5, 0.0], [0.0, 101.5, 2000.0], {"base": 100.0}),
        case("W03", "CT-WP-002", "diagonal path over a tilted plane", tx3, rx3, plane),
        case("W04", "CT-WP-002", "W03 reversed: reversed profile", rx3, tx3, plane),
        case("W05", "CT-WP-003", "Gaussian hill 80 m between antennas", [0.0, 2.0, 0.0], [5000.0, 2.0, 0.0], hill),
        case("W06", "CT-WP-003", "flat path of the same length as W05", [0.0, 2.0, 0.0], [5000.0, 2.0, 0.0],
             {"base": 0.0}),
        case("W07", "CT-WP-004", "ground at -20 m is used as is (water NOT_MODELLED)", [0.0, -18.0, 0.0],
             [3000.0, -18.0, 0.0], {"base": -20.0}),
        case("W08", "CT-WP-005", "height source fails on the first call (transmitter end)", t0, r0,
             {"base": 0.0, "failAtCall": 0}),
        case("W09", "CT-WP-005", "height source fails mid-path (call 10)", t0, r0, {"base": 0.0, "failAtCall": 10}),
        case("W10", "CT-WP-005", "NaN on the last call (receiver end)", t0, r0, {"base": 0.0, "nanAtCall": 20}),
        case("W11", "CT-WP-005", "NaN mid-path (call 7)", t0, r0, {"base": 0.0, "nanAtCall": 7}),
        case("W12", "CT-WP-007", "n = 51 > maxProfilePoints = 20: refused before sampling", [0.0, 2.0, 0.0],
             [5000.0, 2.0, 0.0], {"base": 0.0}, cfg=small_cfg),
        case("W13", "CT-WP-007", "path 501 km > 500 km: refused before sampling", [0.0, 2.0, 0.0],
             [501000.0, 2.0, 0.0], {"base": 0.0}),
        case("W14", "CT-WP-008", "horizontal 1.41 m: terrain layer 0, FSPL by 3D distance", [0.0, 10.0, 0.0],
             [1.0, 30.0, 1.0], {"base": 0.0}),
        case("W15", "CT-WP-008", "horizontal 2.5 m: n = 3", [0.0, 2.0, 0.0], [2.5, 2.0, 0.0], {"base": 0.0}),
        case("W16", "CT-WP-008", "20 MHz outside the model domain: no sampling", t0, r0, {"base": 0.0}, f=20.0),
        case("W17", "CT-WP-009", "transmitter 1 m below ground: provider OK, L2 refuses the antenna",
             [0.0, -1.0, 0.0], r0, {"base": 0.0}),
        case("W18", "CT-WP-009", "terrain 10500 m above the L2 domain", [0.0, 10502.0, 0.0],
             [2000.0, 10502.0, 0.0], {"base": 10500.0}),
        case("W19", "CT-WP-010", "map-scale coordinates up to 12 km", [12000.0, gm(12000, 11000) + 2.0, 11000.0],
             [1000.0, gm(1000, 500) + 2.0, 500.0], map_src),
        case("W20", "CT-WP-001", "receiver 300 m above ground: FSPL by 3D distance", [0.0, 2.0, 0.0],
             [1000.0, 302.0, 0.0], {"base": 0.0}),
        case("W21", "CT-WP-003", "W05 hill at 400 MHz", [0.0, 2.0, 0.0], [5000.0, 2.0, 0.0], hill, f=400.0),
        case("W22", "CT-WP-006", "model with a provider that has no height source", t0, r0, None),
        pcase("P01", "control: valid provider call", t0, r0),
        pcase("P02", "null query", t0, r0, null_query=True),
        pcase("P03", "null config", t0, r0, cfg=None),
        pcase("P04", "invalid config (kFactor = 0)", t0, r0, cfg=dict(CFG, kFactor=0.0)),
        pcase("P05", "null height source", t0, r0, source=None),
    ]
    labels = ("x", "y", "z")
    k = 6
    for end, base in (("tx", t0), ("rx", r0)):
        for axis in range(3):
            for token in (("NaN", "-Inf") if axis == 1 else ("NaN", "+Inf")):
                pos: list[Any] = list(base)
                pos[axis] = token
                tx, rx = (pos, r0) if end == "tx" else (t0, pos)
                cases.append(pcase(f"P{k:02d}", f"{end}.{labels[axis]} = {token}", tx, rx))
                k += 1
    return cases


def compute(cases: list[dict[str, Any]]) -> list[dict[str, Any]]:
    run = {"model": model_losses, "provider": provider_case}
    return [{**c, "expect": run[c["level"]](c["input"])} for c in cases]


def document() -> dict[str, Any]:
    return {"schema": "ricon.world_profile", "version": 1, "revision": 2,
            "source": "docs/req/REQ-L2-world-profile.md v0.2",
            "note": "TEST_ONLY synthetic height sources; not a claim about GetSurfaceY or any Reforger map",
            "toleranceAbs": TOLERANCE_ABS, "enforceTolerance": ENFORCE_TOLERANCE,
            "cases": compute(build_cases())}


def diff(expected: Any, actual: Any, path: str, tol: float, out: list[str]) -> None:
    if isinstance(expected, dict) and isinstance(actual, dict):
        for key in sorted(set(expected) | set(actual)):
            if key not in expected or key not in actual:
                out.append(f"{path}.{key}: missing on one side")
            else:
                diff(expected[key], actual[key], f"{path}.{key}", tol, out)
    elif isinstance(expected, list) and isinstance(actual, list):
        if len(expected) != len(actual):
            out.append(f"{path}: length {len(expected)} != {len(actual)}")
        for i, (e, a) in enumerate(zip(expected, actual)):
            diff(e, a, f"{path}[{i}]", tol, out)
    elif isinstance(expected, bool) or isinstance(actual, bool) or isinstance(expected, str):
        if expected != actual:
            out.append(f"{path}: {expected!r} != {actual!r}")
    elif isinstance(expected, (int, float)) and isinstance(actual, (int, float)):
        if not abs(expected - actual) <= tol:
            out.append(f"{path}: {expected!r} != {actual!r}")
    elif expected != actual:
        out.append(f"{path}: {expected!r} != {actual!r}")


def check(data: dict[str, Any]) -> list[str]:
    problems: list[str] = []
    fresh = document()
    diff({k: v for k, v in fresh.items() if k != "cases"}, {k: v for k, v in data.items() if k != "cases"},
         "header", 0.0, problems)
    ids = [c["id"] for c in data.get("cases", [])]
    if ids != [c["id"] for c in fresh["cases"]]:
        problems.append(f"case ids differ: {ids}")
        return problems
    for stored, recomputed in zip(data["cases"], fresh["cases"]):
        diff(stored, recomputed, stored["id"], TOLERANCE_ABS, problems)
    return problems


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--check", metavar="VECTORS_JSON")
    mode.add_argument("--write", metavar="VECTORS_JSON")
    args = parser.parse_args(argv)
    if args.write:
        Path(args.write).write_text(json.dumps(document(), ensure_ascii=False, indent=1, allow_nan=False) + "\n",
                                    encoding="utf-8", newline="\n")  # LF on every OS: blob SHA = file SHA
        print(f"written {args.write}")
        return 0
    data = json.loads(Path(args.check).read_text(encoding="utf-8"))
    problems = check(data)
    for p in problems:
        print("FAIL", p)
    if problems:
        print(f"WORLD_PROFILE_ORACLE_FAIL: {len(problems)} problems")
        return 1
    print(f"WORLD_PROFILE_ORACLE_PASS: {len(data['cases'])} cases")
    return 0


if __name__ == "__main__":
    sys.exit(main())
