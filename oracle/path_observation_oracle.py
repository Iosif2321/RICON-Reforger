#!/usr/bin/env python3
"""P2 oracle (claude): path-observation contract check and the water layer `water_p527` (REQ-path-observation v0.1).

ORACLE level: pure Python, binary64, no world/engine calls. Sea/fresh water permittivity follows
ITU-R P.527-6 §5.1.2 eq. (14)-(27); the water profile (T, S per class) is TEST_ONLY.
Usage: python path_observation_oracle.py --check vectors/path_observation_v1.json"""
from __future__ import annotations

import argparse
import cmath
import json
import math
import re
import sys
from pathlib import Path

MAX = 2147483647
C = 299_792_458.0
DB_PER_NEPER = 20.0 * math.log10(math.e)
FAMILIES = ("TERRAIN", "WATER", "STRUCTURE", "VEGETATION", "VEHICLE")
SEGMENT_FAMILIES = ("WATER", "STRUCTURE", "VEGETATION", "VEHICLE")
COVERAGE = ("COMPLETE", "PARTIAL", "NOT_OBSERVED")
KNOWLEDGE = ("PROFILE", "RULE", "HINT_ONLY", "UNKNOWN")
MEDIA = ("AIR", "WATER", "UNKNOWN")
WATER_CLASSES = ("SEA", "FRESH", "UNKNOWN")
KEY = re.compile(r"[A-Za-z0-9_.:\-]{1,64}")
MAX_SEGMENTS = 1024
T_MIN, T_MAX, S_MIN, S_MAX = -4.0, 40.0, 0.0, 40.0   # P.527-6 §5.1
F_MAX_GHZ = 1000.0                                  # P.527-6 Annex 1 scope
WATER_CODES = ("OK", "NO_SUBMERGED_ENDPOINT", "INVALID_OBSERVATION", "INVALID_PROFILE", "WATER_COVERAGE_INCOMPLETE",
               "WATER_CLASS_UNKNOWN", "OUT_OF_MODEL_DOMAIN", "NUMERIC_FAILURE")


F32_MAX = 3.4028234663852886e38


def is_num(v) -> bool:
    """Finite and within the binary32 range (REQ PO03)."""
    return type(v) in (int, float) and math.isfinite(v) and abs(v) <= F32_MAX


def is_int(v, lo, hi) -> bool:
    return type(v) is int and lo <= v <= hi


# ---------------------------------------------------------------- P.527-6 §5.1 water permittivity

def pure_water_parameters(t_c: float) -> tuple[float, float, float, float, float]:
    th = 300.0 / (t_c + 273.15) - 1.0                      # (11)
    es = 77.66 + 103.3 * th                                # (8)
    e1 = 0.0671 * es                                       # (9)
    einf = 3.52 - 7.52 * th                                # (10)
    f1 = 20.20 - 146.4 * th + 316.0 * th * th              # (12) GHz
    return es, e1, einf, f1, 39.8 * f1                     # (13)


def sea_conductivity(t_c: float, s: float) -> float:
    """σsw (S/m), eq. (22)-(27)."""
    sigma35 = (2.903602 + 8.607e-2 * t_c + 4.738817e-4 * t_c ** 2
               - 2.991e-6 * t_c ** 3 + 4.3047e-9 * t_c ** 4)                                   # (23)
    r15 = s * (37.5109 + 5.45216 * s + 1.4409e-2 * s * s) / (1004.75 + 182.283 * s + s * s)    # (24)
    a0 = (6.9431 + 3.2841 * s - 9.9486e-2 * s * s) / (84.850 + 69.024 * s + s * s)              # (26)
    a1 = 49.843 - 0.2276 * s + 0.198e-2 * s * s                                               # (27)
    rt15 = 1.0 + a0 * (t_c - 15.0) / (a1 + t_c)                                                # (25)
    return sigma35 * r15 * rt15                                                                # (22)


def water_permittivity(f_ghz: float, t_c: float, s: float) -> tuple[float, float]:
    """(ε′, ε″) of saline water, eq. (14)-(21); S = 0 gives pure water, eq. (5)-(7)."""
    es, e1, einf, f1, f2 = pure_water_parameters(t_c)
    ess = es * math.exp(-3.33330e-3 * s + 4.74868e-6 * s * s)                                  # (17)
    f1s = f1 * (1.0 + s * (2.3232e-3 - 7.9208e-5 * t_c + 3.6764e-6 * t_c ** 2
                           + 3.5594e-7 * t_c ** 3 + 8.9795e-9 * t_c ** 4))                    # (18)
    e1s = e1 * math.exp(-6.28908e-3 * s + 1.76032e-4 * s * s - 9.22144e-5 * t_c * s)          # (19)
    f2s = f2 * (1.0 + s * (-1.99723e-2 + 1.81176e-4 * t_c))                                    # (20)
    einfs = einf * (1.0 + s * (-2.04265e-3 + 1.57883e-4 * t_c))                                # (21)
    r1, r2 = f_ghz / f1s, f_ghz / f2s
    real = (ess - e1s) / (1.0 + r1 * r1) + (e1s - einfs) / (1.0 + r2 * r2) + einfs           # (15)
    imag = r1 * (ess - e1s) / (1.0 + r1 * r1) + r2 * (e1s - einfs) / (1.0 + r2 * r2)         # (16)
    if s > 0.0:
        imag += 18.0 * sea_conductivity(t_c, s) / f_ghz
    return real, imag


def refractive_index(eps_real: float, eps_imag: float) -> complex:
    """Principal root of ε′ − jε″; Im n ≤ 0 for a lossy medium."""
    return cmath.sqrt(complex(eps_real, -eps_imag))


def attenuation_db_per_m(f_mhz: float, n: complex) -> float:
    """WL03: 20·lg(e)·(2π/λ)·|Im n|."""
    wavelength = C / (f_mhz * 1e6)
    return DB_PER_NEPER * (2.0 * math.pi / wavelength) * abs(n.imag)


def penetration_depth_m(f_mhz: float, eps_real: float, eps_imag: float) -> float:
    """P.527-6 eq. (4): δ = λ/(2π)·sqrt(2/(|ε| − ε′)); equals 20·lg(e)/αdB."""
    wavelength = C / (f_mhz * 1e6)
    return wavelength / (2.0 * math.pi) * math.sqrt(2.0 / (math.hypot(eps_real, eps_imag) - eps_real))


def interface_loss_db(n: complex) -> float:
    """WL04: normal incidence, Γ = (1−n)/(1+n), L = −10·lg(1 − |Γ|²)."""
    gamma = (1.0 - n) / (1.0 + n)
    return -10.0 * math.log10(1.0 - abs(gamma) ** 2)


# ---------------------------------------------------------------- PO03-PO07 observation check

def valid_endpoint(e) -> bool:
    if type(e) is not dict or set(e) != {"position", "medium", "depthM", "waterClass", "waterBodyKey"}:
        return False
    p = e["position"]
    if type(p) is not list or len(p) != 3 or not all(is_num(v) for v in p):
        return False
    if e["medium"] not in MEDIA or not is_num(e["depthM"]) or type(e["waterBodyKey"]) is not str:
        return False
    if e["medium"] == "WATER":
        if e["depthM"] < 0 or e["waterClass"] not in WATER_CLASSES:
            return False
        return e["waterBodyKey"] == "" or KEY.fullmatch(e["waterBodyKey"]) is not None
    return e["depthM"] == 0 and e["waterClass"] == "NONE" and e["waterBodyKey"] == ""


def distance(a, b) -> float:
    return math.dist(a["position"], b["position"])


def check_observation(obs) -> str:
    """PO07: 'VALID' or 'INVALID_OBSERVATION'; all checks global."""
    if type(obs) is not dict or set(obs) != {"frequencyMHz", "environmentRevision", "tx", "rx", "coverage", "segments"}:
        return "INVALID_OBSERVATION"
    if not is_num(obs["frequencyMHz"]) or obs["frequencyMHz"] <= 0 or not is_int(obs["environmentRevision"], 1, MAX):
        return "INVALID_OBSERVATION"
    cov = obs["coverage"]
    if type(cov) is not dict or set(cov) != set(FAMILIES) or not all(cov[k] in COVERAGE for k in FAMILIES):
        return "INVALID_OBSERVATION"
    if not valid_endpoint(obs["tx"]) or not valid_endpoint(obs["rx"]):
        return "INVALID_OBSERVATION"
    tx, rx = obs["tx"], obs["rx"]
    length = distance(tx, rx)
    if not length > 0:
        return "INVALID_OBSERVATION"
    if (tx["medium"] == rx["medium"] == "WATER" and tx["waterBodyKey"] and tx["waterBodyKey"] == rx["waterBodyKey"]
            and tx["waterClass"] != rx["waterClass"]):
        return "INVALID_OBSERVATION"   # one identified water body cannot have two classes
    segs = obs["segments"]
    if type(segs) is not list or len(segs) > MAX_SEGMENTS:
        return "INVALID_OBSERVATION"
    previous = None
    by_entity: dict[str, list[tuple[float, float]]] = {}
    for s in segs:
        if type(s) is not dict or set(s) != {"family", "startM", "endM", "entityKey", "knowledge", "materialId",
                                             "openings"}:
            return "INVALID_OBSERVATION"
        if s["family"] not in SEGMENT_FAMILIES or s["knowledge"] not in KNOWLEDGE or type(s["openings"]) is not bool:
            return "INVALID_OBSERVATION"
        if not is_num(s["startM"]) or not is_num(s["endM"]) or not 0 <= s["startM"] < s["endM"] <= length:
            return "INVALID_OBSERVATION"
        if type(s["entityKey"]) is not str or KEY.fullmatch(s["entityKey"]) is None or type(s["materialId"]) is not str:
            return "INVALID_OBSERVATION"
        known = s["knowledge"] in ("PROFILE", "RULE")
        if known != (s["materialId"] != "") or (known and KEY.fullmatch(s["materialId"]) is None):
            return "INVALID_OBSERVATION"
        order = (s["startM"], SEGMENT_FAMILIES.index(s["family"]), s["entityKey"])
        if previous is not None and order < previous:
            return "INVALID_OBSERVATION"
        previous = order
        by_entity.setdefault(s["entityKey"], []).append((s["startM"], s["endM"]))
    for spans in by_entity.values():
        spans.sort()
        for (_, end), (start, _) in zip(spans, spans[1:]):
            if start < end:
                return "INVALID_OBSERVATION"
    return "VALID"


# ---------------------------------------------------------------- WL01-WL07 water layer

def valid_profile(profile) -> bool:
    if type(profile) is not dict or not set(profile) <= {"SEA", "FRESH"}:
        return False
    for value in profile.values():
        if type(value) is not dict or set(value) != {"tempC", "salinityPpt"}:
            return False
        if not is_num(value["tempC"]) or not is_num(value["salinityPpt"]):
            return False
    return True


def water_layer(obs, profile) -> dict:
    if check_observation(obs) != "VALID":
        return {"code": "INVALID_OBSERVATION"}
    if not valid_profile(profile):
        return {"code": "INVALID_PROFILE"}
    ends = (("tx", obs["tx"]), ("rx", obs["rx"]))
    if obs["coverage"]["WATER"] != "COMPLETE" or any(e["medium"] == "UNKNOWN" for _, e in ends):
        return {"code": "WATER_COVERAGE_INCOMPLETE"}
    wet = [(name, e) for name, e in ends if e["medium"] == "WATER"]
    if not wet:
        return {"code": "NO_SUBMERGED_ENDPOINT", "lossDb": 0.0}
    if any(e["waterClass"] == "UNKNOWN" or e["waterClass"] not in profile for _, e in wet):
        return {"code": "WATER_CLASS_UNKNOWN"}
    f_mhz = obs["frequencyMHz"]
    for _, e in wet:
        p = profile[e["waterClass"]]
        if not (T_MIN <= p["tempC"] <= T_MAX and S_MIN <= p["salinityPpt"] <= S_MAX) or f_mhz / 1000.0 > F_MAX_GHZ:
            return {"code": "OUT_OF_MODEL_DOMAIN"}
    try:
        rows, vertical = [], 0.0
        for name, e in wet:
            p = profile[e["waterClass"]]
            eps = water_permittivity(f_mhz / 1000.0, p["tempC"], p["salinityPpt"])
            n = refractive_index(*eps)
            alpha = attenuation_db_per_m(f_mhz, n)
            lint = interface_loss_db(n)
            end_loss = e["depthM"] * alpha + lint
            x, y, z = e["position"]
            rows.append({"end": name, "alphaDbPerM": alpha, "interfaceDb": lint, "endLossDb": end_loss,
                         "effectivePoint": [x, y + e["depthM"], z]})
            vertical += end_loss
        loss, variant = vertical, "VERTICAL_EXIT"
        tx, rx = obs["tx"], obs["rx"]
        if len(wet) == 2 and tx["waterBodyKey"] and tx["waterBodyKey"] == rx["waterBodyKey"]:
            direct = distance(tx, rx) * rows[0]["alphaDbPerM"]
            if direct < vertical:
                loss, variant = direct, "DIRECT_UNDERWATER"
        values = [loss] + [v for r in rows for v in (r["alphaDbPerM"], r["interfaceDb"], r["endLossDb"])]
        points = [c for r in rows for c in r["effectivePoint"]]
        if not all(math.isfinite(v) and 0 <= v <= F32_MAX for v in values) or not all(abs(c) <= F32_MAX for c in points):
            return {"code": "NUMERIC_FAILURE"}
    except (ValueError, ZeroDivisionError, OverflowError):
        return {"code": "NUMERIC_FAILURE"}
    return {"code": "OK", "lossDb": loss, "variant": variant, "endpoints": rows}


def evaluate(case) -> dict:
    return {"observationCode": check_observation(case["input"]["observation"]),
            "water": water_layer(case["input"]["observation"], case["input"]["profile"])}


# ---------------------------------------------------------------- corpus check

def close(a, b, path, errors, tol):
    if isinstance(a, dict) and isinstance(b, dict):
        if set(a) != set(b):
            errors.append(f"{path}: keys {sorted(set(a) ^ set(b))}")
            return
        for k in a:
            close(a[k], b[k], f"{path}.{k}", errors, tol)
    elif isinstance(a, list) and isinstance(b, list):
        if len(a) != len(b):
            errors.append(f"{path}: len")
            return
        for i, (x, y) in enumerate(zip(a, b)):
            close(x, y, f"{path}[{i}]", errors, tol)
    elif type(a) in (int, float) and type(b) in (int, float) and type(a) is not bool and type(b) is not bool:
        if abs(a - b) > tol * max(1.0, abs(b)):
            errors.append(f"{path}: {a!r} != {b!r}")
    elif a != b:
        errors.append(f"{path}: {a!r} != {b!r}")


def check(path: Path) -> int:
    d = json.loads(path.read_bytes())
    errors: list[str] = []
    for c in d["cases"]:
        close(evaluate(c), c["expect"], c["id"], errors, 1e-9)
    if errors:
        print("\n".join(errors[:20]))
        print(f"PATH_OBSERVATION_ORACLE_FAIL: {len(errors)}")
        return 1
    print(f"PATH_OBSERVATION_ORACLE_PASS: {len(d['cases'])} cases")
    return 0


def main(argv=None) -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--check", type=Path, required=True)
    return check(p.parse_args(argv).check)


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    raise SystemExit(main())
