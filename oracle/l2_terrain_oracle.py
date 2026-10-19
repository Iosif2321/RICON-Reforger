#!/usr/bin/env python3
"""Host-эталон L2: дифракция на рельефе по docs/req/REQ-L2-terrain.md (ITU-R P.526-16 §4.5), stdlib. Не production.

`l2(inp)` возвращает закрытый код (REQ §3) и при OK — L, Lba, Lbs, Lsph, ветки Bullington и случай §3.2.
Раннер сверяет результат с `oracle/vectors/l2_terrain_v1.json`; там у каждого ожидания указано происхождение
(STD_INVARIANT / HAND_DERIVED / CROSS_CHECKED, REQ §7).
"""

from __future__ import annotations

import argparse
import json
import math
import re
import sys
from pathlib import Path
from typing import Any

SPEED_OF_LIGHT_MPS = 299792458.0  # exact by SI (REQ-LE-02)
MIN_FREQUENCY_MHZ = 30.0           # REQ §2
MAX_FREQUENCY_MHZ = 100000.0       # REQ §2 domain bound (float32 safety, not from the standard)
MAX_PATH_KM = 500.0
MIN_TERRAIN_M, MAX_TERRAIN_M = -1000.0, 10000.0
MAX_ANTENNA_M = 10000.0
J_THRESHOLD = -0.78                # eq (31), strict
STEP_SLACK = 1.0 + 1e-6            # REQ §2: rounding of an evenly spaced profile is not a step violation
CODES = ("INVALID_CONFIG", "OUT_OF_DOMAIN_FREQUENCY", "INVALID_PROFILE", "PROFILE_STEP_TOO_LARGE",
         "INVALID_ANTENNA_HEIGHT", "NUMERIC_FAILURE", "OK")
BRANCHES = ("LOS", "TRANS")
SPH_CASES = ("BEYOND", "CLEAR", "AH_NEGATIVE", "INTERPOLATED")
POLARIZATIONS = ("VERTICAL", "HORIZONTAL")
CONFIG_KEYS = ("earthRadiusKm", "kFactor", "permittivity", "conductivitySm", "polarization", "maxProfileStepKm",
               "maxProfilePoints")
ACCURACY = ("MET", "NOT_MET", "NOT_EVALUATED")


class NumericFailure(Exception):
    pass


def finite(*values: float) -> None:
    """REQ §3 code 6: any non-finite intermediate value is a NUMERIC_FAILURE."""
    for value in values:
        if not math.isfinite(value):
            raise NumericFailure(value)


def is_number(value: Any) -> bool:
    """A finite JSON number. A huge JSON integer is not a number here (float conversion would overflow)."""
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        return False
    try:
        return math.isfinite(float(value))
    except OverflowError:
        return False


# REQ §2: closed, float32-safe domains of every config field: (low, low inclusive, high).
CONFIG_DOMAIN = {
    "earthRadiusKm": (1000.0, True, 100000.0),
    "kFactor": (0.1, True, 100.0),
    "permittivity": (1.0, False, 1000.0),
    "conductivitySm": (0.0, True, 1000.0),
    "maxProfileStepKm": (0.002, True, 10.0),  # >= 2 m keeps the provider's actual step >= 1 m (REQ §6)
}
MAX_PROFILE_POINTS_CAP = 100000
MIN_SPACING_KM = 0.001              # REQ §2: 1 m between profile points
MIN_ANTENNA_M = 0.01


def in_domain(value: Any, low: float, low_inclusive: bool, high: float) -> bool:
    return is_number(value) and (value >= low if low_inclusive else value > low) and value <= high


# ---------------------------------------------------------------- §4.5.1 Bullington (eq. 49-57)

def knife_edge_j(nu: float) -> float:
    """eq (31) for nu > -0.78 (strict), zero otherwise (eq 52/56)."""
    if not nu > J_THRESHOLD:
        return 0.0
    return 6.9 + 20.0 * math.log10(math.sqrt((nu - 0.1) ** 2 + 1.0) + nu - 0.1)


def bullington(ds: list[float], hs: list[float], hts: float, hrs: float, ce: float, lam: float) -> tuple[float, str]:
    """Bullington loss Lb (eq 57) and the branch for one profile (REQ §4)."""
    d = ds[-1]
    inner = range(1, len(ds) - 1)  # profile indices i = 2..n-1 in the standard's numbering

    def bulge(i: int) -> float:
        return hs[i] + 500.0 * ce * ds[i] * (d - ds[i])

    s_tim = max((bulge(i) - hts) / ds[i] for i in inner)                                     # (49)
    s_tr = (hrs - hts) / d                                                                    # (50)

    def los() -> float:
        nu_max = max((bulge(i) - (hts * (d - ds[i]) + hrs * ds[i]) / d)
                     * math.sqrt(0.002 * d / (lam * ds[i] * (d - ds[i]))) for i in inner)    # (51)
        return knife_edge_j(nu_max)                                                          # (52)

    if s_tim < s_tr:
        branch, l_uc = "LOS", los()
    else:
        s_rim = max((bulge(i) - hrs) / (d - ds[i]) for i in inner)                            # (53)
        if s_tim + s_rim == 0:
            branch, l_uc = "LOS", los()  # REQ §4 step 4: exact grazing, d_b undefined
        else:
            d_b = (hrs - hts + s_rim * d) / (s_tim + s_rim)                                   # (54)
            nu_b = ((hts + s_tim * d_b - (hts * (d - d_b) + hrs * d_b) / d)
                    * math.sqrt(0.002 * d / (lam * d_b * (d - d_b))))                        # (55)
            branch, l_uc = "TRANS", knife_edge_j(nu_b)                                       # (56)
    l_b = l_uc + (1.0 - math.exp(-l_uc / 6.0)) * (10.0 + 0.02 * d)                           # (57)
    finite(l_b)
    return l_b, branch


# ---------------------------------------------------------------- §3.1.1 and §3.2 (eq. 11a-25)

def surface_k(a_km: float, f_mhz: float, cfg: dict[str, Any]) -> float:
    eps, sigma = cfg["permittivity"], cfg["conductivitySm"]
    k_h = 0.36 * (a_km * f_mhz) ** (-1.0 / 3.0) * ((eps - 1.0) ** 2 + (18000.0 * sigma / f_mhz) ** 2) ** -0.25  # (11a)
    if cfg["polarization"] == "HORIZONTAL":
        return k_h
    return k_h * (eps ** 2 + (18000.0 * sigma / f_mhz) ** 2) ** 0.5                                         # (12a)


def field_relative_db(d_km: float, h1: float, h2: float, a_km: float, f_mhz: float,
                      cfg: dict[str, Any]) -> tuple[float, str]:
    """20 log(E/E0) = F(X) + G(Y1) + G(Y2) (eq 13) in practical units; a_km is a_e or a_em (REQ §5.3).
    Also returns the eq (19) first-term accuracy flag, which never changes the loss."""
    k = surface_k(a_km, f_mhz, cfg)
    beta = (1.0 + 1.6 * k ** 2 + 0.67 * k ** 4) / (1.0 + 4.5 * k ** 2 + 1.53 * k ** 4)                       # (16)
    x = 2.188 * beta * f_mhz ** (1.0 / 3.0) * a_km ** (-2.0 / 3.0) * d_km                                    # (14a)
    f_x = 11.0 + 10.0 * math.log10(x) - 17.6 * x if x >= 1.6 else -20.0 * math.log10(x) - 5.6488 * x ** 1.425  # (17)
    g_floor = 2.0 + 20.0 * math.log10(k)

    def g(h: float) -> float:
        y = 9.575e-3 * beta * f_mhz ** (2.0 / 3.0) * a_km ** (-1.0 / 3.0) * h                                 # (15a)
        b = beta * y                                                                                            # (18b)
        value = (17.6 * (b - 1.1) ** 0.5 - 5.0 * math.log10(b - 1.1) - 8.0) if b > 2 \
            else 20.0 * math.log10(b + 0.1 * b ** 3)                                                            # (18)
        return max(value, g_floor)

    def delta(h: float) -> float:
        by = beta * (9.575e-3 * beta * f_mhz ** (2.0 / 3.0) * a_km ** (-1.0 / 3.0) * h)                       # beta*Y
        d0 = 0.5 * (1.0 + math.tanh((0.5 * math.log10(by) - 0.255) / 0.3))                                     # (19c)
        dinf = 0.5 * (1.0 + math.tanh((0.5 * math.log10(by) + 0.255) / 0.25))                                  # (19d)
        return math.sqrt(by) * (d0 + 1.779 * (1.0 - beta) * (dinf - d0))                                       # (19b)

    x_lim = 1.096 - 1.280 * (1.0 - beta)                                                                       # (19a)
    accuracy = "MET" if x - delta(h1) - delta(h2) > x_lim else "NOT_MET"                                       # (19)
    result = f_x + g(h1) + g(h2)
    finite(result)
    return result, accuracy


def spherical(d_km: float, h1: float, h2: float, lam: float, ae_km: float, f_mhz: float,
              cfg: dict[str, Any]) -> tuple[float, str, str]:
    """§3.2: Lsph = A (eq 25), the case and the eq (19) flag (REQ §5.3). Eq 21-24 in metres."""
    d, ae = d_km * 1000.0, ae_km * 1000.0
    d_los = math.sqrt(2.0 * ae) * (math.sqrt(h1) + math.sqrt(h2))                                           # (21)
    if d >= d_los:
        rel, accuracy = field_relative_db(d_km, h1, h2, ae_km, f_mhz, cfg)
        return -rel, "BEYOND", accuracy
    m = d ** 2 / (4.0 * ae * (h1 + h2))                                                                       # (22e)
    c = (h1 - h2) / (h1 + h2)                                                                                 # (22d)
    arg = max(-1.0, min(1.0, 1.5 * c * math.sqrt(3.0 * m / (m + 1.0) ** 3)))  # REQ §5.3 step 2 clamp
    b = 2.0 * math.sqrt((m + 1.0) / (3.0 * m)) * math.cos(math.pi / 3.0 + math.acos(arg) / 3.0)             # (22c)
    d1 = d / 2.0 * (1.0 + b)                                                                                  # (22a)
    d2 = d - d1                                                                                               # (22b)
    h = ((h1 - d1 ** 2 / (2.0 * ae)) * d2 + (h2 - d2 ** 2 / (2.0 * ae)) * d1) / d                             # (22)
    h_req = 0.552 * math.sqrt(d1 * d2 * lam / d)                                                              # (23)
    finite(h, h_req)
    if h > h_req:
        return 0.0, "CLEAR", "NOT_EVALUATED"
    a_em = 0.5 * (d / (math.sqrt(h1) + math.sqrt(h2))) ** 2                                                  # (24)
    rel, accuracy = field_relative_db(d_km, h1, h2, a_em / 1000.0, f_mhz, cfg)  # a_em metres -> km (REQ §5.3)
    a_h = -rel
    if a_h < 0:
        return 0.0, "AH_NEGATIVE", accuracy
    return (1.0 - h / h_req) * a_h, "INTERPOLATED", accuracy                                                            # (25)


# ---------------------------------------------------------------- §4.5.2 complete method (eq. 58-66)

def smooth_heights(ds: list[float], hs: list[float], hts: float, hrs: float) -> tuple[float, float]:
    """h_st, h_sr: least-squares surface (58-60), obstruction correction (61-62), clamp to the ground (63)."""
    d, n = ds[-1], len(ds)
    v1 = sum((ds[i] - ds[i - 1]) * (hs[i] + hs[i - 1]) for i in range(1, n))                                 # (58)
    v2 = sum((ds[i] - ds[i - 1]) * (hs[i] * (2 * ds[i] + ds[i - 1]) + hs[i - 1] * (ds[i] + 2 * ds[i - 1]))
             for i in range(1, n))                                                                            # (59)
    h_stip = (2.0 * v1 * d - v2) / d ** 2                                                                     # (60a)
    h_srip = (v2 - v1 * d) / d ** 2                                                                           # (60b)
    inner = range(1, n - 1)
    h_ob = [hs[i] - (hts * (d - ds[i]) + hrs * ds[i]) / d for i in inner]                                     # (61d)
    h_obs = max(h_ob)                                                                                         # (61a)
    if h_obs <= 0:
        h_stp, h_srp = h_stip, h_srip                                                                         # (62a,b)
    else:
        a_obt = max(h_ob[k] / ds[i] for k, i in enumerate(inner))                                             # (61b)
        a_obr = max(h_ob[k] / (d - ds[i]) for k, i in enumerate(inner))                                       # (61c)
        g_t, g_r = a_obt / (a_obt + a_obr), a_obr / (a_obt + a_obr)                                           # (62e,f)
        h_stp, h_srp = h_stip - h_obs * g_t, h_srip - h_obs * g_r                                             # (62c,d)
    return min(h_stp, hs[0]), min(h_srp, hs[-1])                                                              # (63)


def validate(inp: Any) -> str | None:
    """REQ §3 codes 1-5 in order; None when the input is admissible."""
    cfg = inp.get("config") if isinstance(inp, dict) else None
    if not isinstance(cfg, dict) or set(cfg) != set(CONFIG_KEYS) \
            or not all(in_domain(cfg[key], *domain) for key, domain in CONFIG_DOMAIN.items()) \
            or isinstance(cfg["maxProfilePoints"], bool) or not isinstance(cfg["maxProfilePoints"], int) \
            or not 3 <= cfg["maxProfilePoints"] <= MAX_PROFILE_POINTS_CAP \
            or not isinstance(cfg["polarization"], str) or cfg["polarization"] not in POLARIZATIONS:
        return "INVALID_CONFIG"
    if not is_number(inp.get("frequencyMHz")) or not MIN_FREQUENCY_MHZ <= inp["frequencyMHz"] <= MAX_FREQUENCY_MHZ:
        return "OUT_OF_DOMAIN_FREQUENCY"
    profile = inp.get("profile")
    if not isinstance(profile, list) or not 3 <= len(profile) <= cfg["maxProfilePoints"] or not all(
            isinstance(p, list) and len(p) == 2 and is_number(p[0]) and is_number(p[1]) for p in profile):
        return "INVALID_PROFILE"
    ds = [p[0] for p in profile]
    if ds[0] != 0 or any(b - a < MIN_SPACING_KM / STEP_SLACK for a, b in zip(ds, ds[1:])) or ds[-1] > MAX_PATH_KM \
            or not all(MIN_TERRAIN_M <= p[1] <= MAX_TERRAIN_M for p in profile):
        return "INVALID_PROFILE"
    if any(b - a > cfg["maxProfileStepKm"] * STEP_SLACK for a, b in zip(ds, ds[1:])):
        return "PROFILE_STEP_TOO_LARGE"
    if not all(in_domain(inp.get(k), MIN_ANTENNA_M, True, MAX_ANTENNA_M) for k in ("htg", "hrg")):
        return "INVALID_ANTENNA_HEIGHT"
    return None


def provider_sampling(horizontal_distance_m: float, cfg: dict[str, Any]) -> tuple[str, list[float]]:
    """REQ §6 reference for the profile provider: ("SKIP", []) when dKm < 0.002 (terrain layer = 0),
    ("UNAVAILABLE", []) when n > maxProfilePoints (decided before any allocation), else ("OK", d_i in km)."""
    d_km = horizontal_distance_m / 1000.0
    if d_km < 2 * MIN_SPACING_KM:
        return "SKIP", []
    step = cfg["maxProfileStepKm"]
    n = max(3, math.ceil(d_km / step) + 1)
    if n > cfg["maxProfilePoints"]:
        return "UNAVAILABLE", []
    return "OK", [d_km * i / (n - 1) for i in range(n)]


def l2(inp: Any) -> dict[str, Any]:
    code = validate(inp)
    if code is not None:
        return {"code": code}
    cfg, f = inp["config"], float(inp["frequencyMHz"])
    ds = [float(p[0]) for p in inp["profile"]]
    hs = [float(p[1]) for p in inp["profile"]]
    d = ds[-1]
    lam = SPEED_OF_LIGHT_MPS / (f * 1e6)
    ae = cfg["kFactor"] * cfg["earthRadiusKm"]
    ce = 1.0 / ae
    hts, hrs = hs[0] + inp["htg"], hs[-1] + inp["hrg"]
    try:
        lba, branch_actual = bullington(ds, hs, hts, hrs, ce, lam)                       # REQ §5.1
        h_st, h_sr = smooth_heights(ds, hs, hts, hrs)                                     # REQ §5.2
        hts_s, hrs_s = hts - h_st, hrs - h_sr                                             # (64)
        lbs, branch_smooth = bullington(ds, [0.0] * len(ds), hts_s, hrs_s, ce, lam)
        lsph, sph_case, accuracy = spherical(d, hts_s, hrs_s, lam, ae, f, cfg)           # (65), REQ §5.3
        loss = lba + max(lsph - lbs, 0.0)                                                 # (66)
        finite(loss, lsph, lbs)
    except (NumericFailure, ValueError, ZeroDivisionError, OverflowError):
        return {"code": "NUMERIC_FAILURE"}
    return {"code": "OK", "L": loss, "Lba": lba, "Lbs": lbs, "Lsph": lsph, "branchActual": branch_actual,
            "branchSmooth": branch_smooth, "sphCase": sph_case, "firstTermAccuracy": accuracy}


# ---------------------------------------------------------------------- runner

SOURCE = re.compile(r"docs/req/REQ-L2-terrain\.md@[0-9a-f]{7,40}")
CASE_KEYS = {"id", "ct", "provenance", "note", "input", "expect"}
PROVENANCE = ("STD_INVARIANT", "HAND_DERIVED", "CROSS_CHECKED")
NUMERIC_FIELDS = ("L", "Lba", "Lbs", "Lsph")
EXACT_FIELDS = ("branchActual", "branchSmooth", "sphCase", "firstTermAccuracy")


ROOT_KEYS = {"schema", "version", "revision", "source", "standard", "note", "toleranceDb", "enforceToleranceDb", "cases"}
STANDARD = ("Rec. ITU-R P.526-16 (11/2025), sha256 "
            "7647c3cb3873dc0389a7007f487728359551ed82468a5677b7cd09430875f23d")
INPUT_KEYS = {"profile", "htg", "hrg", "frequencyMHz", "config"}
CASE_ID = re.compile(r"[A-Za-z0-9_-]{1,64}")
SAFE_STRING = re.compile(r"[A-Z_]{1,32}")
SAFE_KEY = re.compile(r"[A-Za-z]{1,32}")
SAFE_MAGNITUDE = 1e12


def safe_json(value: Any, depth: int = 0) -> bool:
    """Input values a code generator may carry: null, plain numbers of bounded magnitude, enum-like strings and
    nested lists/objects with plain keys. Malformed-on-purpose inputs stay possible; unbounded ones do not."""
    if depth > 4:
        return False
    if value is None:
        return True
    if isinstance(value, bool):
        return False
    if isinstance(value, (int, float)):
        return is_number(value) and abs(value) <= SAFE_MAGNITUDE
    if isinstance(value, str):
        return SAFE_STRING.fullmatch(value) is not None
    if isinstance(value, list):
        return len(value) <= MAX_PROFILE_POINTS_CAP and all(safe_json(item, depth + 1) for item in value)
    if isinstance(value, dict):
        return all(isinstance(k, str) and SAFE_KEY.fullmatch(k) and safe_json(v, depth + 1) for k, v in value.items())
    return False


def check_vector_file(data: Any) -> list[str]:
    if not isinstance(data, dict) or data.get("schema") != "ricon.oracle.l2_terrain" or data.get("version") != 1:
        return ["schema/version"]
    problems = []
    if set(data) != ROOT_KEYS:
        problems.append("root keys")
    if isinstance(data.get("revision"), bool) or not isinstance(data.get("revision"), int) or data["revision"] < 1:
        problems.append("revision")
    if not isinstance(data.get("source"), str) or SOURCE.fullmatch(data["source"]) is None:
        problems.append("source")
    if data.get("standard") != STANDARD:
        problems.append("standard")
    if not isinstance(data.get("note"), str):
        problems.append("note")
    tol = data.get("toleranceDb")
    if not is_number(tol) or not 0 < tol <= 1e-3:
        problems.append("toleranceDb")
    if data.get("enforceToleranceDb") != 0.05:  # REQ §6
        problems.append("enforceToleranceDb")
    cases = data.get("cases")
    if not isinstance(cases, list) or not cases:
        return problems + ["cases: non-empty list required"]
    seen = set()
    for case in cases:
        if not isinstance(case, dict) or set(case) != CASE_KEYS or not isinstance(case["id"], str) \
                or CASE_ID.fullmatch(case["id"]) is None or case["id"] in seen \
                or not isinstance(case["note"], str) \
                or not isinstance(case["provenance"], str) or case["provenance"] not in PROVENANCE \
                or not isinstance(case["ct"], list) or not case["ct"] \
                or not all(isinstance(c, str) and re.fullmatch(r"CT-L2-[0-9]{3}", c) for c in case["ct"]) \
                or not isinstance(case["input"], dict) or set(case["input"]) != INPUT_KEYS \
                or not safe_json(case["input"]) \
                or not isinstance(case["expect"], dict) or not isinstance(case["expect"].get("code"), str) \
                or case["expect"]["code"] not in CODES:
            problems.append(f"case {case.get('id') if isinstance(case, dict) else '?'!r}")
            continue
        seen.add(case["id"])
        expect = case["expect"]
        keys = {"code"} | (set(NUMERIC_FIELDS) | set(EXACT_FIELDS) if expect["code"] == "OK" else set())
        if set(expect) != keys or (expect["code"] == "OK" and not (
                all(is_number(expect[k]) for k in NUMERIC_FIELDS)
                and all(isinstance(expect[k], str) for k in EXACT_FIELDS)
                and expect["branchActual"] in BRANCHES and expect["branchSmooth"] in BRANCHES
                and expect["sphCase"] in SPH_CASES and expect["firstTermAccuracy"] in ACCURACY)):
            problems.append(f"{case['id']}: expect")
    return problems


def run_case(case: dict[str, Any], tol: float) -> list[str]:
    actual = l2(case["input"])
    expect = case["expect"]
    if actual["code"] != expect["code"]:
        return [f"{case['id']}: code expected {expect['code']} actual {actual['code']}"]
    failures = []
    if expect["code"] == "OK":
        if actual["L"] < 0:
            failures.append(f"{case['id']}: L < 0")
        for key in NUMERIC_FIELDS:
            if abs(actual[key] - expect[key]) > tol:
                failures.append(f"{case['id']}: {key} expected {expect[key]!r} actual {actual[key]!r}")
        for key in EXACT_FIELDS:
            if actual[key] != expect[key]:
                failures.append(f"{case['id']}: {key} expected {expect[key]} actual {actual[key]}")
    return failures


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", required=True, metavar="VECTORS_JSON")
    args = parser.parse_args(argv)
    data = json.loads(Path(args.check).read_text(encoding="utf-8"))
    problems = check_vector_file(data)
    if problems:
        print("FORMAT_FAIL:", *problems, sep="\n  ")
        return 1
    failures = [f for case in data["cases"] for f in run_case(case, data["toleranceDb"])]
    for failure in failures:
        print("FAIL", failure)
    if failures:
        print(f"L2_TERRAIN_ORACLE_FAIL: {len(failures)} problems in {len(data['cases'])} cases")
        return 1
    print(f"L2_TERRAIN_ORACLE_PASS: {len(data['cases'])} cases")
    return 0


if __name__ == "__main__":
    sys.exit(main())
