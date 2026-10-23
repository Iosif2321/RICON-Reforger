#!/usr/bin/env python3
"""P4.2 oracle (claude): over-canopy terminal correction for terminals inside vegetation (REQ-vegetation-overcanopy v0.1).

ORACLE level: pure Python, binary64 over binary32-typed inputs (P2 PO03). Model M6: Rec. ITU-R P.2108-1 §3.1, height gain
terminal correction, clutter type "Urban/trees/forest", eq. (1), (2a), (2c)-(2e), (2g), 0.03-3 GHz. The correction is
added to a basic transmission loss computed to/from the representative clutter height R; it is a separate geometric
candidate and is never added to the through-vegetation losses of REQ-vegetation v0.4.
Usage: python vegetation_overcanopy_oracle.py --check vectors/vegetation_overcanopy_v1.json"""
from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import path_observation_oracle as po  # noqa: E402
import vegetation_oracle as v  # noqa: E402

CODES = ("INVALID_OBSERVATION", "INVALID_INPUT", "VEGETATION_COVERAGE_INCOMPLETE", "VEGETATION_UNKNOWN",
         "NO_VEGETATION", "VEGETATION_MIXED_PROFILE", "NO_TERMINAL_IN_VEGETATION", "OUT_OF_MODEL_DOMAIN",
         "NUMERIC_FAILURE", "OK")
R_DEFAULT_M = 15.0         # P.2108-1 Table 3, Urban/trees/forest, where local information is not available
WS_M = 27.0                # P.2108-1 Table 2 default; P.2108 gives no woodland-specific value (ASSUMED for forest)
F_MIN_MHZ, F_MAX_MHZ = 30.0, 3000.0
CANOPY_PROVENANCE = ("MEASURED", "ASSUMED_CONFIG", "STANDARD_DEFAULT")


def j_nu(nu):
    """P.2108-1 eq. (1); J = 0 for nu <= -0.78."""
    if nu <= -0.78:
        return 0.0
    return 6.9 + 20.0 * math.log10(math.sqrt((nu - 0.1) ** 2 + 1.0) + nu - 0.1)


def terminal_correction(f_mhz, h, r):
    """P.2108-1 §3.1.2, eq. (2a), (2c)-(2e), (2g) for trees/forest. Returns the record of intermediate values."""
    if h >= r:
        return {"hdifM": 0.0, "thetaDeg": 0.0, "nu": 0.0, "jDb": 0.0, "ahDb": 0.0}
    hdif = r - h
    theta = math.degrees(math.atan(hdif / WS_M))
    nu = 0.342 * math.sqrt(f_mhz / 1000.0) * math.sqrt(hdif * theta)
    j = j_nu(nu)
    return {"hdifM": hdif, "thetaDeg": theta, "nu": nu, "jDb": j, "ahDb": j - 6.03}


def typed_canopy(canopy, catalog):
    """canopy: materialId -> {heightM: binary32 > 0 or null, provenance}. Keys must be non-negligible catalog entries."""
    if type(canopy) is not dict:
        return None
    out = {}
    for key, entry in canopy.items():
        if type(key) is not str or key not in catalog or catalog[key]["negligible"]:
            return None
        if type(entry) is not dict or set(entry) != {"heightM", "provenance"} or entry["provenance"] not in CANOPY_PROVENANCE:
            return None
        if entry["heightM"] is None:
            if entry["provenance"] != "STANDARD_DEFAULT":
                return None
            out[key] = (R_DEFAULT_M, "STANDARD_DEFAULT")
            continue
        h = po.typed(entry["heightM"])
        if h is None or h <= 0 or entry["provenance"] == "STANDARD_DEFAULT":
            return None
        out[key] = (h, entry["provenance"])
    return out


def over_canopy(inp) -> dict:
    obs = po.typed_observation(inp.get("observation") if type(inp) is dict else None)
    if obs is None:
        return {"code": "INVALID_OBSERVATION"}
    if type(inp) is not dict or set(inp) != {"observation", "catalog", "polarization", "txHeightM", "rxHeightM", "canopy"}:
        return {"code": "INVALID_INPUT"}
    base = {k: inp[k] for k in ("observation", "catalog", "polarization", "txHeightM", "rxHeightM")}
    typed = v.typed_inputs(base)
    if typed is None:
        return {"code": "INVALID_INPUT"}
    catalog, _pol, tx_h, rx_h = typed
    canopy = typed_canopy(inp["canopy"], catalog)
    if canopy is None:
        return {"code": "INVALID_INPUT"}
    if obs["coverage"]["VEGETATION"] != "COMPLETE":
        return {"code": "VEGETATION_COVERAGE_INCOMPLETE"}
    veg = [s for s in obs["segments"] if s["family"] == "VEGETATION"]
    if any(s["knowledge"] in ("UNKNOWN", "HINT_ONLY") or s["materialId"] not in catalog for s in veg):
        return {"code": "VEGETATION_UNKNOWN"}
    veg = [s for s in veg if not catalog[s["materialId"]]["negligible"]]
    if not veg:
        return {"code": "NO_VEGETATION"}
    merged: list[list] = []
    for s in sorted(veg, key=lambda s: (s["startM"], s["endM"])):
        if merged and s["startM"] <= merged[-1][1]:
            merged[-1][1] = max(merged[-1][1], s["endM"])
            merged[-1][2].add(s["materialId"])
        else:
            merged.append([s["startM"], s["endM"], {s["materialId"]}])
    if any(len(m[2]) > 1 for m in merged):
        return {"code": "VEGETATION_MIXED_PROFILE"}
    length = obs["lengthM"]
    ends = []
    for start, end, mats in merged:
        mat = next(iter(mats))
        if start == 0:
            ends.append(("TX", start, end, mat, tx_h))
        if end == length:
            ends.append(("RX", start, end, mat, rx_h))
    if not ends:
        return {"code": "NO_TERMINAL_IN_VEGETATION"}
    f_mhz = obs["frequencyMHz"]
    for _end, _s, _e, mat, h in ends:
        if mat not in canopy:
            return {"code": "INVALID_INPUT"}
    if not F_MIN_MHZ <= f_mhz <= F_MAX_MHZ or any(h is None for *_x, h in ends):
        return {"code": "OUT_OF_MODEL_DOMAIN"}
    terminals, total = [], 0.0
    try:
        for end_name, start, end, mat, h in ends:
            r, provenance = canopy[mat]
            rec = terminal_correction(f_mhz, h, r)
            terminals.append({"end": end_name, "intervalStartM": start, "intervalEndM": end, "materialId": mat,
                              "antennaHeightM": h, "canopyHeightM": r, "canopyProvenance": provenance, **rec})
            total += rec["ahDb"]
    except (ValueError, ZeroDivisionError, OverflowError):
        return {"code": "NUMERIC_FAILURE"}
    # VG-OC08: every normative intermediate, including the product hdif*theta under the root of eq. (2c), must be a
    # finite binary32 value; a binary64-only result is not accepted (native computes in binary32).
    values = [total] + [t[k] for t in terminals for k in ("hdifM", "thetaDeg", "nu", "jDb", "ahDb")]
    values += [t["hdifM"] * t["thetaDeg"] for t in terminals]
    if not all(math.isfinite(x) and abs(x) <= po.F32_MAX for x in values) or any(t["ahDb"] < -1e-6 for t in terminals):
        return {"code": "NUMERIC_FAILURE"}
    return {"code": "OK", "model": "M6", "status": "SOURCE_DOMAIN", "budgetMode": "BASE_AT_CANOPY_HEIGHT",
            "integrationClosed": False, "totalAhDb": total, "terminals": terminals}


def evaluate(case) -> dict:
    return over_canopy(case["input"])


def check(path: Path) -> int:
    d = json.loads(path.read_bytes())
    errors: list[str] = []
    for c in d["cases"]:
        po.close(evaluate(c), c["expect"], c["id"], errors, 1e-9)
    if errors:
        print("\n".join(errors[:20]))
        print(f"OVERCANOPY_ORACLE_FAIL: {len(errors)}")
        return 1
    print(f"OVERCANOPY_ORACLE_PASS: {len(d['cases'])} cases")
    return 0


def main(argv=None) -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--check", type=Path, required=True)
    return check(p.parse_args(argv).check)


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    raise SystemExit(main())
