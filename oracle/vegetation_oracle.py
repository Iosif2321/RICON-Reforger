#!/usr/bin/env python3
"""P4 oracle (claude): vegetation layer as a model ensemble (REQ-vegetation v0.2).

ORACLE level: pure Python, binary64 over binary32-typed inputs (P2 PO03). Models: P.833-10 eq. (1), (7);
DoD ECAC ESD-TR-81-101 MED eq. (6a)/(6b) and Jansky & Bailey eq. (11) with TABLE 13; EXD eq. (35) diagnostic only.
The catalog is ASSUMED_CONFIG, TEST_ONLY here. Usage: python vegetation_oracle.py --check vectors/vegetation_v1.json"""
from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import path_observation_oracle as po  # noqa: E402

MILE_M = 1609.344
CLIMATES = ("TEMPERATE", "TROPICAL")
# ECAC ESD-TR-81-101 TABLE 13: f MHz -> (alpha, A, B) for V and H polarization.
JB_TABLE = {
    "V": ((25, 0.0, 0.0, 0.00212), (50, 0.0, 0.0, 0.00106), (100, 0.045, 0.615, 0.000529),
          (250, 0.050, 0.759, 0.000443), (400, 0.055, 1.02, 0.000523)),
    "H": ((25, 0.0, 0.0, 0.00424), (50, 0.0, 0.0, 0.00424), (100, 0.020, 0.472, 0.00551),
          (250, 0.025, 0.774, 0.000588), (400, 0.035, 1.11, 0.000598)),
}
CODES = ("INVALID_OBSERVATION", "INVALID_INPUT", "VEGETATION_COVERAGE_INCOMPLETE", "VEGETATION_UNKNOWN",
         "NO_VEGETATION", "VEGETATION_MIXED_PROFILE", "NO_MODEL_FOR_SCENARIO", "NUMERIC_FAILURE", "OK")
SCENARIOS = {"S1": ("M3", "M1"), "S2": ("M4", "M3"), "S3": ("M3", "M2")}


# ---------------------------------------------------------------- catalog and inputs (VG08, VG09)

def typed_profile(p):
    if type(p) is not dict or type(p.get("negligible")) is not bool:
        return None
    if p["negligible"]:
        return {"negligible": True} if set(p) == {"negligible"} else None
    if set(p) != {"negligible", "climate", "gammaTable", "amA1", "amAlpha", "amMinMHz", "amMaxMHz"}:
        return None
    if p["climate"] not in CLIMATES or type(p["gammaTable"]) is not list or not 2 <= len(p["gammaTable"]) <= 32:
        return None
    table = []
    for row in p["gammaTable"]:
        if type(row) is not list or len(row) != 2:
            return None
        f, g = po.typed(row[0]), po.typed(row[1])
        if f is None or g is None or f <= 0 or g <= 0 or (table and f <= table[-1][0]):
            return None
        table.append((f, g))
    a1, alpha, lo, hi = (po.typed(p[k]) for k in ("amA1", "amAlpha", "amMinMHz", "amMaxMHz"))
    if None in (a1, alpha, lo, hi) or a1 <= 0 or not 0 < lo < hi:
        return None
    return {"negligible": False, "climate": p["climate"], "gamma": table, "a1": a1, "alpha": alpha, "lo": lo, "hi": hi}


def typed_inputs(inp):
    """VG09 + catalog: (catalog, polarization, txH, rxH) or None when INVALID_INPUT."""
    if type(inp) is not dict or set(inp) != {"observation", "catalog", "polarization", "txHeightM", "rxHeightM"}:
        return None
    cat = inp["catalog"]
    if type(cat) is not dict or not all(type(k) is str and po.KEY.fullmatch(k) for k in cat):
        return None
    catalog = {}
    for k, v in cat.items():
        t = typed_profile(v)
        if t is None:
            return None
        catalog[k] = t
    if inp["polarization"] not in ("V", "H"):
        return None
    heights = []
    for key in ("txHeightM", "rxHeightM"):
        v = inp[key]
        if v is None:
            heights.append(None)
            continue
        h = po.typed(v)
        if h is None or h < 0:
            return None
        heights.append(h)
    return catalog, inp["polarization"], heights[0], heights[1]


# ---------------------------------------------------------------- models (VG03-VG07)

def gamma_at(profile, f_mhz):
    table = profile["gamma"]
    if not table[0][0] <= f_mhz <= table[-1][0]:
        return None
    for (f0, g0), (f1, g1) in zip(table, table[1:]):
        if f0 <= f_mhz <= f1:
            t = (math.log(f_mhz) - math.log(f0)) / (math.log(f1) - math.log(f0))
            return math.exp(math.log(g0) + t * (math.log(g1) - math.log(g0)))
    return None


def m1(profile, f_mhz, d):
    """P.833-10 eq. (1) with Am of eq. (2)."""
    g = gamma_at(profile, f_mhz)
    if g is None or not profile["lo"] <= f_mhz <= profile["hi"]:
        return None
    am = profile["a1"] * f_mhz ** profile["alpha"]
    return am * (1.0 - math.exp(-d * g / am))


def m2(profile, f_mhz, d):
    """P.833-10 eq. (7), f <= 1 GHz, no cap."""
    g = gamma_at(profile, f_mhz)
    if g is None or f_mhz > 1000.0:
        return None
    return d * g


def m3(profile, f_mhz, d):
    """MED, ECAC eq. (36a)/(36b); temperate in-leaf forest, 230 MHz-95 GHz, 0 < d <= 400 m."""
    if profile["climate"] != "TEMPERATE" or not 230.0 <= f_mhz <= 95000.0 or not 0.0 < d <= 400.0:
        return None
    f = f_mhz / 1000.0
    return 1.33 * f ** 0.284 * d ** 0.588 if d >= 14.0 else 0.45 * f ** 0.284 * d


def jb_constants(pol, f_mhz):
    rows = JB_TABLE[pol]
    for (f0, *c0), (f1, *c1) in zip(rows, rows[1:]):
        if f0 <= f_mhz <= f1:
            t = (math.log10(f_mhz) - math.log10(f0)) / (math.log10(f1) - math.log10(f0))
            return [a + t * (b - a) for a, b in zip(c0, c1)]
    return None


def m4(profile, f_mhz, d, pol, tx_h, rx_h):
    """Jansky & Bailey, ECAC eq. (11) minus free space in the same units; tropical, both antennas inside."""
    if profile["climate"] != "TROPICAL" or not 25.0 <= f_mhz <= 400.0 or not 8.0 <= d <= 1600.0:
        return None
    if tx_h is None or rx_h is None or not (2.0 <= tx_h <= 7.0 and 2.0 <= rx_h <= 7.0):
        return None
    alpha, a, b = jb_constants(pol, f_mhz)
    dm = d / MILE_M
    lb = 36.57 + 20.0 * math.log10(f_mhz) - 20.0 * math.log10(a * math.exp(-1609.0 * alpha * dm) / dm + b / dm ** 2)
    excess = lb - (36.57 + 20.0 * math.log10(f_mhz) + 20.0 * math.log10(dm))
    # VG06: a negative excess (short H-polarized paths, 75-150 MHz) is a fit artefact; the candidate is excluded.
    return excess if excess >= 0 else None


def exd(f_mhz, d):
    return 0.26 * (f_mhz / 1000.0) ** 0.77 * d


# ---------------------------------------------------------------- layer (VG10-VG12)

def vegetation_layer(inp) -> dict:
    obs = po.typed_observation(inp.get("observation") if type(inp) is dict else None)
    if obs is None:
        return {"code": "INVALID_OBSERVATION"}
    typed = typed_inputs(inp)
    if typed is None:
        return {"code": "INVALID_INPUT"}
    catalog, pol, tx_h, rx_h = typed
    if obs["coverage"]["VEGETATION"] != "COMPLETE":
        return {"code": "VEGETATION_COVERAGE_INCOMPLETE"}
    veg = [s for s in obs["segments"] if s["family"] == "VEGETATION"]
    if any(s["knowledge"] in ("UNKNOWN", "HINT_ONLY") or s["materialId"] not in catalog for s in veg):
        return {"code": "VEGETATION_UNKNOWN"}
    veg = [s for s in veg if not catalog[s["materialId"]]["negligible"]]
    if not veg:
        return {"code": "NO_VEGETATION", "lossDb": 0.0}
    merged: list[list] = []
    for s in sorted(veg, key=lambda s: (s["startM"], s["endM"])):
        if merged and s["startM"] <= merged[-1][1]:
            merged[-1][1] = max(merged[-1][1], s["endM"])
            merged[-1][2].add(s["materialId"])
        else:
            merged.append([s["startM"], s["endM"], {s["materialId"]}])
    if any(len(m[2]) > 1 for m in merged):
        return {"code": "VEGETATION_MIXED_PROFILE"}
    f_mhz, length = obs["frequencyMHz"], obs["lengthM"]
    rows, total = [], 0.0
    try:
        for start, end, mats in merged:
            mat = next(iter(mats))
            prof = catalog[mat]
            d = end - start
            at_tx, at_rx = start == 0, end == length
            if at_tx and at_rx:
                scenario = "S2"
            elif at_tx or at_rx:
                scenario = "S1"
            elif d <= 400.0:
                scenario = "S3"
            else:
                return {"code": "NO_MODEL_FOR_SCENARIO"}
            models = {"M1": lambda: m1(prof, f_mhz, d), "M2": lambda: m2(prof, f_mhz, d),
                      "M3": lambda: m3(prof, f_mhz, d), "M4": lambda: m4(prof, f_mhz, d, pol, tx_h, rx_h)}
            candidates = []
            for name in SCENARIOS[scenario]:
                v = models[name]()
                if v is not None:
                    candidates.append({"model": name, "lossDb": v})
            if not candidates:
                return {"code": "NO_MODEL_FOR_SCENARIO"}
            losses = [c["lossDb"] for c in candidates]
            row = {"startM": start, "endM": end, "materialId": mat, "scenario": scenario,
                   "primaryModel": candidates[0]["model"], "primaryLossDb": candidates[0]["lossDb"],
                   "candidates": candidates, "spreadMinDb": min(losses), "spreadMaxDb": max(losses),
                   "exdDb": exd(f_mhz, d), "capApplied": False}
            rows.append(row)
            total += row["primaryLossDb"]
        values = [total] + [v for r in rows for v in (r["primaryLossDb"], r["spreadMinDb"], r["spreadMaxDb"], r["exdDb"])]
        if not all(math.isfinite(v) and 0 <= v <= po.F32_MAX for v in values):
            return {"code": "NUMERIC_FAILURE"}
    except (ValueError, ZeroDivisionError, OverflowError):
        return {"code": "NUMERIC_FAILURE"}
    return {"code": "OK", "lossDb": total, "intervals": rows}


def evaluate(case) -> dict:
    return vegetation_layer(case["input"])


def check(path: Path) -> int:
    d = json.loads(path.read_bytes())
    errors: list[str] = []
    for c in d["cases"]:
        po.close(evaluate(c), c["expect"], c["id"], errors, 1e-9)
    if errors:
        print("\n".join(errors[:20]))
        print(f"VEGETATION_ORACLE_FAIL: {len(errors)}")
        return 1
    print(f"VEGETATION_ORACLE_PASS: {len(d['cases'])} cases")
    return 0


def main(argv=None) -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--check", type=Path, required=True)
    return check(p.parse_args(argv).check)


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    raise SystemExit(main())
