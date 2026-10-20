#!/usr/bin/env python3
"""Host-эталон чувствительности расчёта связи к параметрам конфига (спринт 27, agora #2633, #2636). stdlib.

Две части, без новой физики распространения (всё считает `link_physics_oracle` / `l2_terrain_oracle`):
1. **Шаг профиля** `maxProfileStepKm` для §4.5 на холмах разной ширины. Вершина сдвигается относительно узлов
   сетки (перебор фаз в пределах шага), берётся худшее отклонение от опорного мелкого шага. Это измерение на
   заданных TEST_ONLY формах, **не** рекомендация шага для поставщика высот (codex #2636).
2. **Параметры грунта** по ITU-R P.527-6 §5.2 (модель почвы, ур. 57–70, с водой по ур. 6–13). Формулы сверены с
   рисунками 14–16 стандарта (тест). Annex 1 заявляет методы «до 1000 ГГц» без нижней границы, но рисунки для
   почвы начинаются со 100 МГц: ниже сверки со стандартом нет, такие точки помечаются `BELOW_FIGURE_RANGE`.
   Классические кривые Fig. 24 (из P.527-3) — отдельный набор, прочитан только на 30 МГц.
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
import link_physics_oracle as lpo  # noqa: E402

SOILS = {  # P.527-6 Table 2
    "sandy_loam": {"psand": 51.52, "pclay": 13.42, "psilt": 35.06, "rho_s": 2.66, "rho_b": 1.6006},
    "loam": {"psand": 41.96, "pclay": 8.53, "psilt": 49.51, "rho_s": 2.70, "rho_b": 1.5781},
    "silty_loam": {"psand": 30.63, "pclay": 13.48, "psilt": 55.89, "rho_s": 2.59, "rho_b": 1.5750},
    "silty_clay": {"psand": 5.02, "pclay": 47.38, "psilt": 47.60, "rho_s": 2.56, "rho_b": 1.4758},
}
FIGURE_MIN_GHZ = 0.1  # P.527-6 Fig. 14-16 start at 0.1 GHz
MAX_GHZ = 1000.0  # P.527-6 scope: up to 1 000 GHz; Fig. 14-16 end there


def pure_water(f_ghz: float, t_c: float) -> tuple[float, float, float, float, float]:
    th = 300.0 / (t_c + 273.15) - 1.0                     # (11)
    es = 77.66 + 103.3 * th                               # (8)
    e1 = 0.0671 * es                                      # (9)
    einf = 3.52 - 7.52 * th                               # (10)
    f1 = 20.20 - 146.4 * th + 316.0 * th * th             # (12) GHz
    return es, e1, einf, f1, 39.8 * f1                    # (13)


def soil_permittivity(f_ghz: float, t_c: float, soil: str, mv: float) -> tuple[float, float]:
    """P.527-6 §5.2: real and imaginary part of the complex relative permittivity of soil (eq 58-70).
    Raises ValueError("SOIL_OUTSIDE_REAL_MODEL") when a base of the fractional powers in (59)/(60) is negative:
    the formulas then continue into complex numbers (e.g. silty clay, mv 0.1, 0.1 GHz; codex #2666), which are
    not ground constants."""
    if soil not in SOILS:
        raise ValueError(f"soil {soil!r}")
    for name, v in (("f_ghz", f_ghz), ("t_c", t_c), ("mv", mv)):
        if isinstance(v, bool) or not isinstance(v, (int, float)) or not math.isfinite(v):
            raise ValueError(f"{name} {v!r}")
    if f_ghz <= 0.0 or not 0.0 < mv < 1.0 or t_c <= -273.15:
        raise ValueError("input out of range")
    if f_ghz > MAX_GHZ:  # above the scope of P.527-6 and the end of Fig. 14-16 (codex #2716)
        raise ValueError("SOIL_ABOVE_SCOPE")
    s = SOILS[soil]
    rho_s, rho_b, psand, pclay = s["rho_s"], s["rho_b"], s["psand"], s["pclay"]
    es, e1, einf, f1, f2 = pure_water(f_ghz, t_c)
    s1 = 0.0467 + 0.2204 * rho_b - 0.004111 * psand - 0.006614 * pclay      # (69)
    s2 = -1.645 + 1.939 * rho_b - 0.0225622 * psand + 0.01594 * pclay      # (70)
    seff1 = (f_ghz / 1.35) * (s1 - s2) / (1.0 + (f_ghz / 1.35) ** 2)         # (67)
    seff2 = s2 + (s1 - s2) / (1.0 + (f_ghz / 1.35) ** 2)                     # (68)
    k = (rho_s - rho_b) / (rho_s * mv)
    efw1 = (es - e1) / (1 + (f_ghz / f1) ** 2) + (e1 - einf) / (1 + (f_ghz / f2) ** 2) + einf \
        + 18.0 * seff1 / f_ghz * k                                                         # (65)
    efw2 = (f_ghz / f1) * (es - e1) / (1 + (f_ghz / f1) ** 2) \
        + (f_ghz / f2) * (e1 - einf) / (1 + (f_ghz / f2) ** 2) + 18.0 * seff2 / f_ghz * k  # (66)
    alpha = 0.65                                                                           # (64)
    esm = (1.01 + 0.44 * rho_s) ** 2 - 0.062                                               # (61)
    b1 = 1.2748 - 0.00519 * psand - 0.00152 * pclay                                        # (62)
    b2 = 1.33797 - 0.00603 * psand - 0.00166 * pclay                                       # (63)
    if efw1 < 0.0 or efw2 < 0.0:
        raise ValueError("SOIL_OUTSIDE_REAL_MODEL")
    base_re = 1 + rho_b / rho_s * (esm ** alpha - 1) + mv ** b1 * efw1 ** alpha - mv
    if base_re <= 0.0:
        raise ValueError("SOIL_OUTSIDE_REAL_MODEL")
    re = base_re ** (1 / alpha)                                                            # (59)
    im = (mv ** b2 * efw2 ** alpha) ** (1 / alpha)                                         # (60)
    if not (isinstance(re, float) and isinstance(im, float) and math.isfinite(re) and math.isfinite(im)):
        raise ValueError("SOIL_OUTSIDE_REAL_MODEL")
    return re, im


def ground_p527(f_mhz: float, soil: str, mv: float, t_c: float = 23.0,
                polarization: str = "VERTICAL") -> dict[str, Any]:
    """Ground for the oracles: εr and σ = 0.05563·f_GHz·εr'' (P.527-6 eq 3a)."""
    f_ghz = f_mhz / 1000.0
    re, im = soil_permittivity(f_ghz, t_c, soil, mv)
    return {"permittivity": re, "conductivitySm": 0.05563 * f_ghz * im, "polarization": polarization,
            "domain": "IN_FIGURE_RANGE" if f_ghz >= FIGURE_MIN_GHZ else "BELOW_FIGURE_RANGE"}


def as_scenario_ground(g: dict[str, Any]) -> dict[str, Any]:
    return {k: g[k] for k in ("permittivity", "conductivitySm", "polarization")}


def triangular_profile(ds: list[float], center_km: float, half_km: float, height_m: float) -> list[list[float]]:
    return [[x, max(0.0, height_m * (1.0 - abs(x - center_km) / half_km))] for x in ds]


def l2_loss(d_km: float, step_km: float, center_km: float, half_km: float, height_m: float, f_mhz: float,
            h_m: float, ground: dict[str, Any]) -> float:
    cfg = {"earthRadiusKm": 6371.0, "kFactor": 4.0 / 3.0, "permittivity": ground["permittivity"],
           "conductivitySm": ground["conductivitySm"], "polarization": ground["polarization"],
           "maxProfileStepKm": step_km, "maxProfilePoints": l2o.MAX_PROFILE_POINTS_CAP}
    status, ds = l2o.provider_sampling(d_km * 1000.0, cfg)
    if status != "OK":
        raise ValueError(status)
    r = l2o.l2({"profile": triangular_profile(ds, center_km, half_km, height_m), "htg": h_m, "hrg": h_m,
                "frequencyMHz": f_mhz, "config": cfg})
    if r["code"] != "OK":
        raise ValueError(r["code"])
    return r["L"]


def step_sensitivity(d_km: float, center_km: float, half_km: float, height_m: float, f_mhz: float, h_m: float,
                     ground: dict[str, Any], steps: list[float], reference_step: float,
                     phases: int = 8) -> dict[str, Any]:
    """Worst |L(step, phase) − L(reference)| over vertex positions center + k·step/phases (k = 0…phases−1).
    The reference uses the same vertex shift, so only the sampling differs. An empty sweep is refused rather
    than reported as zero deviation (codex #2673)."""
    if isinstance(phases, bool) or not isinstance(phases, int) or phases < 1:
        raise ValueError(f"phases {phases!r}")
    if not steps:
        raise ValueError("no steps")
    out = []
    for step in steps:
        worst, worst_phase = -1.0, -1
        for k in range(phases):
            c = center_km + k * step / phases
            ref = l2_loss(d_km, reference_step, c, half_km, height_m, f_mhz, h_m, ground)
            dev = abs(l2_loss(d_km, step, c, half_km, height_m, f_mhz, h_m, ground) - ref)
            if not math.isfinite(dev):
                raise ValueError(f"non-finite deviation at step {step}, phase {k}")
            if dev > worst:
                worst, worst_phase = dev, k
        out.append({"stepKm": step, "worstDeviationDb": worst, "worstPhase": worst_phase})
    return {"referenceStepKm": reference_step, "phases": phases, "rows": out}


# ---------------------------------------------------------------------- sweeps written to the vector file

VECTORS = Path(__file__).resolve().parent / "vectors" / "sensitivity_v1.json"
MATRIX = Path(__file__).resolve().parent / "vectors" / "link_scenarios_v1.json"
# readings of P.527-6 Fig. 14-16 by claude (by eye from the figure images; T = 23 °C): (soil, mv, f_GHz, ε', ε'')
FIGURE_READINGS = [
    ("silty_loam", 0.5, 0.1, 29.4, 7.8), ("silty_loam", 0.5, 1.0, 30.3, 3.1), ("silty_loam", 0.5, 10.0, 26.0, 9.7),
    ("silty_clay", 0.5, 0.1, 23.0, 2.0), ("silty_clay", 0.5, 1.0, 25.2, 3.9), ("silty_clay", 0.5, 10.0, 24.0, 9.2),
    ("silty_loam", 0.07, 0.1, 3.77, 1.8), ("silty_loam", 0.07, 1.0, 4.3, 0.5), ("silty_loam", 0.07, 10.0, 4.7, 0.4),
]
READING_TOLERANCE = {0.5: 0.7, 0.07: 0.2}  # 2 % of the figure's full scale (35 and 10)
BASELINE_GROUND = {"permittivity": 15.0, "conductivitySm": 0.005}  # matrix TEST_ONLY ground
GROUND_CASES = [  # (id, soil, mv); soil None = matrix baseline
    ("TEST_ONLY_15_0.005", None, None),
    ("P527_silty_loam_mv0.07", "silty_loam", 0.07),
    ("P527_silty_loam_mv0.30", "silty_loam", 0.30),
    ("P527_silty_clay_mv0.50", "silty_clay", 0.50),
]
# P.527-6 Fig. 24 (reproduced P.527-3 Fig. 1), read by claude: flat low-frequency levels, which at 30 MHz are
# still at the start of the knee for B and D; therefore applied to the 30 MHz scenarios only
CLASSIC_GROUNDS_30MHZ = [("FIG24_B_wet", 30.0, 0.01), ("FIG24_D_medium_dry", 15.0, 0.001),
                         ("FIG24_E_very_dry", 3.0, 0.0001)]
POLS = ("VERTICAL", "HORIZONTAL")
STEP_FREQS, STEP_HEIGHTS, STEP_HALVES = (30.0, 400.0), (1.5, 10.0), (0.5, 0.1, 0.03)
STEPS = [0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0]
REFERENCE_STEP, CONVERGENCE_STEP, PHASES = 0.002, 0.0025, 8  # REQ-L2 lower bound of maxProfileStepKm is 0.002
STEP_PATH_KM, STEP_CENTER_KM, STEP_HILL_M = 10.0, 2.0, 60.0
RANGE_STEPS = [0.01, 0.05, 0.1, 0.25, 0.5, 1.0]
RANGE_SCENARIOS = ["S-30MHz-1W-h1.5-hill", "S-30MHz-1W-h10-hill", "S-400MHz-1W-h1.5-hill", "S-400MHz-1W-h10-hill"]


def ground_case(case_id: str, soil: str | None, mv: float | None, f_mhz: float, pol: str) -> dict[str, Any]:
    if soil is None:
        return dict(BASELINE_GROUND, polarization=pol, domain="TEST_ONLY")
    return ground_p527(f_mhz, soil, mv, polarization=pol)


def range_entry(model_out: dict[str, Any]) -> dict[str, Any]:
    a, r = model_out["aboveSensitivity"], model_out["meetsSnr"]["rural"]
    return {"aboveSensitivity": {"status": a["status"], "outerLimitM": a["outerLimitM"]},
            "meetsSnrRural": {"status": r["status"], "outerLimitM": r.get("outerLimitM")},  # absent out of domain
            "applicableUpToM": model_out["applicableUpToM"],
            "lossDbAtSamples": [[p["distanceM"], p["lossDb"]] for p in model_out["samples"]]}


def ground_sweep_one(sc_in: dict[str, Any], case: tuple[str, str | None, float | None], pol: str) -> dict[str, Any]:
    return sweep_with_ground(sc_in, case[0], ground_case(*case, sc_in["frequencyMHz"], pol), pol)


def classic_sweep_one(sc_in: dict[str, Any], case: tuple[str, float, float], pol: str) -> dict[str, Any]:
    g = {"permittivity": case[1], "conductivitySm": case[2], "polarization": pol, "domain": "FIG24_READ_30MHZ"}
    return sweep_with_ground(sc_in, case[0], g, pol)


def sweep_with_ground(sc_in: dict[str, Any], case_id: str, g: dict[str, Any], pol: str) -> dict[str, Any]:
    sc = copy.deepcopy(sc_in)
    sc["ground"] = as_scenario_ground(g)
    sc["models"] = [m for m in sc["models"] if m != "FSPL"]
    sc["grids"] = {m: sc["grids"][m] for m in sc["models"]}
    sc["noiseEnvironments"] = ["rural"]
    out = lpo.evaluate_scenario(sc)
    return {"ground": case_id, "polarization": pol, "groundUsed": g,
            "models": {m: range_entry(v) for m, v in out["models"].items()}}


def step_range_one(sc_in: dict[str, Any], step_km: float) -> dict[str, Any]:
    sc = copy.deepcopy(sc_in)
    sc["terrain"]["profileStepKm"] = step_km
    sc["models"], sc["noiseEnvironments"] = ["P526_GENERAL"], ["rural"]
    sc["grids"] = {"P526_GENERAL": sc["grids"]["P526_GENERAL"]}
    return {"stepKm": step_km, **range_entry(lpo.evaluate_scenario(sc)["models"]["P526_GENERAL"])}


def matrix_scenarios() -> dict[str, dict[str, Any]]:
    data = json.loads(MATRIX.read_text(encoding="utf-8"))
    return {s["id"]: s["input"] for s in data["scenarios"]}


def step_block(f_mhz: float, h_m: float, half_km: float) -> dict[str, Any]:
    g = dict(BASELINE_GROUND, polarization="VERTICAL")
    r = step_sensitivity(STEP_PATH_KM, STEP_CENTER_KM, half_km, STEP_HILL_M, f_mhz, h_m, g,
                         STEPS + [CONVERGENCE_STEP], REFERENCE_STEP, PHASES)
    rows = r["rows"]
    hill = l2_loss(STEP_PATH_KM, REFERENCE_STEP, STEP_CENTER_KM, half_km, STEP_HILL_M, f_mhz, h_m, g)
    flat = l2_loss(STEP_PATH_KM, REFERENCE_STEP, STEP_CENTER_KM, half_km, 0.0, f_mhz, h_m, g)
    return {"frequencyMHz": f_mhz, "antennaHeightM": h_m, "halfWidthKm": half_km,
            "hillMinusFlatDb": hill - flat, "convergenceDeviationDb": rows[-1]["worstDeviationDb"],
            "rows": rows[:-1]}


def compute() -> dict[str, Any]:
    scenarios = matrix_scenarios()
    anchors = []
    for soil, mv, f, re_read, im_read in FIGURE_READINGS:
        re, im = soil_permittivity(f, 23.0, soil, mv)
        anchors.append({"soil": soil, "mv": mv, "frequencyGHz": f, "readRe": re_read, "readIm": im_read,
                        "calcRe": re, "calcIm": im, "tolerance": READING_TOLERANCE[mv]})
    grounds = []
    for f in sorted({sc["frequencyMHz"] for sc in scenarios.values()}):
        for case in GROUND_CASES[1:]:
            g = ground_p527(f, case[1], case[2])
            grounds.append({"ground": case[0], "frequencyMHz": f, "permittivity": g["permittivity"],
                            "conductivitySm": g["conductivitySm"], "domain": g["domain"]})
    sweep = [{"scenario": sid, "cases": [ground_sweep_one(sc, case, pol) for case in GROUND_CASES for pol in POLS]
              + ([classic_sweep_one(sc, case, pol) for case in CLASSIC_GROUNDS_30MHZ for pol in POLS]
                 if sc["frequencyMHz"] == 30.0 else [])}
             for sid, sc in scenarios.items()]
    steps = [step_block(f, h, half) for f in STEP_FREQS for h in STEP_HEIGHTS for half in STEP_HALVES]
    ranges = [{"scenario": sid, "hill": scenarios[sid]["terrain"]["hill"],
               "rows": [step_range_one(scenarios[sid], st) for st in RANGE_STEPS]} for sid in RANGE_SCENARIOS]
    return {"schema": "ricon.oracle.sensitivity", "version": 1, "revision": 1,
            "source": "oracle/sensitivity_oracle.py", "provenance": "ORACLE_SNAPSHOT",
            "standards": ["Rec. ITU-R P.527-6 (09/2021): eq (3a), (6)-(13), (57)-(70), Table 2, Fig. 14-16, Fig. 24",
                          "Rec. ITU-R P.526-16 (11/2025): §4.5 via l2_terrain_oracle"],
            "note": "Hills, antenna heights, radio parameters and the baseline ground are TEST_ONLY. P.527-6 soil "
                    "figures start at 0.1 GHz; Annex 1 states methods up to 1000 GHz without a lower bound, so points below 0.1 GHz "
                    "have no check against the standard's figures (BELOW_FIGURE_RANGE). Fig. 24 classic grounds are "
                    "read at their flat low-frequency levels and applied to the 30 MHz scenarios only. "
                    "Step results are measurements on synthetic triangular hills, not a step recommendation. "
                    "Ranges are SAMPLED_ESTIMATE on the matrix grids.",
            "relTolerance": 1e-9, "soilFigureAnchors": anchors, "groundsByFrequency": grounds,
            "groundSweep": sweep,
            "stepLoss": {"pathKm": STEP_PATH_KM, "centerKm": STEP_CENTER_KM, "hillHeightM": STEP_HILL_M,
                         "referenceStepKm": REFERENCE_STEP, "convergenceStepKm": CONVERGENCE_STEP,
                         "phases": PHASES, "blocks": steps},
            "stepRange": ranges}


def compare(a: Any, b: Any, tol: float, path: str, errors: list[str]) -> None:
    if isinstance(a, dict) and isinstance(b, dict):
        if set(a) != set(b):
            errors.append(f"{path}: keys {sorted(set(a) ^ set(b))}")
            return
        for k in a:
            compare(a[k], b[k], tol, f"{path}.{k}", errors)
    elif isinstance(a, list) and isinstance(b, list):
        if len(a) != len(b):
            errors.append(f"{path}: length {len(a)} != {len(b)}")
            return
        for i, (x, y) in enumerate(zip(a, b)):
            compare(x, y, tol, f"{path}[{i}]", errors)
    elif isinstance(a, (bool, str)) or a is None or b is None:
        if a != b:
            errors.append(f"{path}: {a!r} != {b!r}")
    elif isinstance(a, (int, float)) and isinstance(b, (int, float)) and not isinstance(b, bool):
        if not math.isclose(a, b, rel_tol=tol, abs_tol=1e-12):
            errors.append(f"{path}: {a!r} != {b!r}")
    else:
        errors.append(f"{path}: type {type(a).__name__} != {type(b).__name__}")


# ---------------------------------------------------------------------- R28 anchors for the Enforce probe

ANCHORS = Path(__file__).resolve().parent / "vectors" / "l2_sensitivity_anchors_v1.json"
ANCHOR_TOLERANCE_DB = 0.05  # as enforceToleranceDb of l2_terrain_v1 (float32 in Enforce)
# (id, f_MHz, h_m, ground, pol, half_km or None (flat), step_km, phase k of 8, note); path 10 km, vertex 2 km + k·step/8
ANCHOR_SPECS = [
    ("A01-p527-dry-30-V", 30.0, 1.5, ("P527", "silty_loam", 0.07), "VERTICAL", None, 0.1, 0, "P.527-6 dry soil, BELOW_FIGURE_RANGE"),
    ("A02-p527-dry-30-H", 30.0, 1.5, ("P527", "silty_loam", 0.07), "HORIZONTAL", None, 0.1, 0, "horizontal polarization"),
    ("A03-fig24-B-30-V", 30.0, 1.5, ("FIG24", 30.0, 0.01), "VERTICAL", None, 0.1, 0, "Fig. 24 wet ground"),
    ("A04-fig24-E-30-V", 30.0, 1.5, ("FIG24", 3.0, 0.0001), "VERTICAL", None, 0.1, 0, "Fig. 24 very dry ground, small eps"),
    ("A05-p527-clay-88-V", 88.0, 1.5, ("P527", "silty_clay", 0.5), "VERTICAL", None, 0.1, 0, "P.527-6 wet clay"),
    ("A06-p527-clay-88-H", 88.0, 1.5, ("P527", "silty_clay", 0.5), "HORIZONTAL", None, 0.1, 0, "wet clay, horizontal"),
    ("A07-p527-mv30-400-V", 400.0, 1.5, ("P527", "silty_loam", 0.30), "VERTICAL", None, 0.1, 0, "IN_FIGURE_RANGE"),
    ("A08-matrix-50-H", 50.0, 1.5, ("MATRIX",), "HORIZONTAL", None, 0.1, 0, "matrix ground, horizontal"),
    ("A09-hw30-s100-on-node", 30.0, 1.5, ("MATRIX",), "VERTICAL", 0.03, 0.1, 0, "vertex on a profile node"),
    ("A10-hw30-s100-missed", 30.0, 1.5, ("MATRIX",), "VERTICAL", 0.03, 0.1, 4, "vertex midway, nearest nodes 50 m away: hill missed"),
    ("A11-hw30-s50-on-node", 30.0, 1.5, ("MATRIX",), "VERTICAL", 0.03, 0.05, 0, "vertex on a node"),
    ("A12-hw30-s50-phase3", 30.0, 1.5, ("MATRIX",), "VERTICAL", 0.03, 0.05, 3, "nodes 18.75 m and 31.25 m from the vertex"),
    ("A13-hw100-s100-400-h10", 400.0, 10.0, ("MATRIX",), "VERTICAL", 0.1, 0.1, 4, "worst phase; here the hill adds loss"),
    ("A14-hw500-s100-worst", 30.0, 1.5, ("MATRIX",), "VERTICAL", 0.5, 0.1, 4, "worst phase of the matrix hill width"),
    ("A15-hw1900-s100-shape", 30.0, 1.5, ("MATRIX",), "VERTICAL", 1.9, 0.1, 0, "wide hill: shape changes the loss"),
    ("A16-hw30-s2-reference", 30.0, 1.5, ("MATRIX",), "VERTICAL", 0.03, 0.002, 0, "reference step, 5001 points"),
]


def anchor_ground(spec: tuple[Any, ...], f_mhz: float, pol: str) -> dict[str, Any]:
    if spec[0] == "P527":
        return as_scenario_ground(ground_p527(f_mhz, spec[1], spec[2], polarization=pol))
    if spec[0] == "FIG24":
        return {"permittivity": spec[1], "conductivitySm": spec[2], "polarization": pol}
    if spec == ("MATRIX",):
        return dict(BASELINE_GROUND, polarization=pol)
    raise ValueError(f"ground source {spec!r}")  # a typo must not fall back to the matrix ground (codex #2765)


def anchor_case(spec: tuple[Any, ...]) -> dict[str, Any]:
    cid, f, h, gspec, pol, half, step, phase, note = spec
    g = anchor_ground(gspec, f, pol)
    cfg = {"earthRadiusKm": 6371.0, "kFactor": 4.0 / 3.0, **{k: g[k] for k in ("permittivity", "conductivitySm",
                                                                              "polarization")},
           "maxProfileStepKm": step, "maxProfilePoints": l2o.MAX_PROFILE_POINTS_CAP}
    center = STEP_CENTER_KM + phase * step / PHASES
    status, ds = l2o.provider_sampling(STEP_PATH_KM * 1000.0, cfg)
    if status != "OK":
        raise ValueError(status)
    hill = {"centerKm": center, "halfWidthKm": half, "heightM": STEP_HILL_M} if half is not None else None
    profile = triangular_profile(ds, center, half, STEP_HILL_M) if hill else [[x, 0.0] for x in ds]
    r = l2o.l2({"profile": profile, "htg": h, "hrg": h, "frequencyMHz": f, "config": cfg})
    if r["code"] != "OK":
        raise ValueError(r["code"])
    expect = dict(r, profilePointCount=len(ds), peakTerrainM=max(p[1] for p in profile))
    return {"id": cid, "note": note, "groundSource": gspec[0],
            "input": {"pathKm": STEP_PATH_KM, "hill": hill, "htg": h, "hrg": h, "frequencyMHz": f, "config": cfg},
            "expect": expect}


def compute_anchors() -> dict[str, Any]:
    return {"schema": "ricon.oracle.l2_sensitivity_anchors", "version": 1, "revision": 1,
            "source": "oracle/sensitivity_oracle.py (l2_terrain_oracle.l2 on provider_sampling profiles)",
            "provenance": "ORACLE_SNAPSHOT",
            "note": "R28 anchors for an Enforce probe (agora #2706, #2737, #2758). Profile: provider_sampling of "
                    "pathKm at maxProfileStepKm, heights of the symmetric triangle hill (null = flat ground). Hills, "
                    "heights and the matrix ground are TEST_ONLY; P.527 grounds below 0.1 GHz are BELOW_FIGURE_RANGE.",
            "toleranceDb": ANCHOR_TOLERANCE_DB, "cases": [anchor_case(s) for s in ANCHOR_SPECS]}


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--write", action="store_true", help="regenerate the vector file instead of checking it")
    parser.add_argument("--anchors", action="store_true", help="work on the R28 anchor file only")
    args = parser.parse_args(argv)
    if args.anchors:
        anchors = compute_anchors()
        if args.write:
            ANCHORS.write_text(json.dumps(anchors, ensure_ascii=False, indent=1) + "\n", encoding="utf-8",
                               newline="\n")
            print(f"written {ANCHORS.name}")
            return 0
        errors = []
        compare(anchors, json.loads(ANCHORS.read_text(encoding="utf-8")), 1e-9, "$", errors)
        for e in errors[:50]:
            print(e)
        print("SENSITIVITY_ANCHORS_PASS" if not errors else f"SENSITIVITY_ANCHORS_FAIL ({len(errors)})")
        return 0 if not errors else 1
    data = compute()
    if args.write:
        VECTORS.write_text(json.dumps(data, ensure_ascii=False, indent=1) + "\n", encoding="utf-8", newline="\n")
        print(f"written {VECTORS.name}")
        return 0
    errors: list[str] = []
    compare(data, json.loads(VECTORS.read_text(encoding="utf-8")), data["relTolerance"], "$", errors)
    for e in errors[:50]:
        print(e)
    print("SENSITIVITY_ORACLE_PASS" if not errors else f"SENSITIVITY_ORACLE_FAIL ({len(errors)})")
    return 0 if not errors else 1


if __name__ == "__main__":
    sys.exit(main())
