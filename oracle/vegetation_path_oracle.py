#!/usr/bin/env python3
"""P4.3 oracle (claude): choice between the through-vegetation and over-canopy full budgets (REQ-vegetation-path v0.2).

ORACLE level: binary64 over binary32-typed inputs. Calls the frozen P4 v1 layer (vegetation_oracle.vegetation_layer)
and the frozen P4.2 layer (vegetation_overcanopy_oracle.over_canopy) on the same input; the base budgets L0/L2 for the
real heights and for the reference heights max(h, R) come in as numbers (computed by the existing L0/L2 evaluators).
Usage: python vegetation_path_oracle.py --check vectors/vegetation_path_v1.json"""
from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import path_observation_oracle as po  # noqa: E402
import vegetation_oracle as v  # noqa: E402
import vegetation_overcanopy_oracle as o  # noqa: E402

CODES = ("INVALID_OBSERVATION", "INVALID_INPUT", "VEGETATION_COVERAGE_INCOMPLETE", "VEGETATION_UNKNOWN",
         "VEGETATION_MIXED_PROFILE", "NO_VEGETATION", "NUMERIC_FAILURE", "UNRESOLVED", "OK")
BUDGET_FIELDS = {"fsplDb", "l2Db", "fsplOverDb", "l2OverDb"}
PASS_THROUGH = ("INVALID_OBSERVATION", "INVALID_INPUT", "VEGETATION_COVERAGE_INCOMPLETE", "VEGETATION_UNKNOWN",
                "VEGETATION_MIXED_PROFILE")


def typed_budget(b):
    """fsplDb, fsplOverDb: binary32 > 0; l2Db, l2OverDb: binary32 >= 0 or null (base budget not computed)."""
    if type(b) is not dict or set(b) != BUDGET_FIELDS:
        return None
    out = {}
    for key in BUDGET_FIELDS:
        x = b[key]
        if x is None and key in ("l2Db", "l2OverDb"):
            out[key] = None
            continue
        t = po.typed(x)
        if t is None or (t <= 0 if key.startswith("fspl") else t < 0):
            return None
        out[key] = t
    return out


def mid_path_vegetation(inp):
    """True if a merged non-negligible vegetation interval touches neither end (S3/S3+): over-canopy needs geometry."""
    obs = po.typed_observation(inp["observation"])
    catalog = v.typed_inputs({k: inp[k] for k in ("observation", "catalog", "polarization", "txHeightM", "rxHeightM")})[0]
    veg = sorted((s for s in obs["segments"] if s["family"] == "VEGETATION" and not catalog[s["materialId"]]["negligible"]),
                 key=lambda s: (s["startM"], s["endM"]))
    merged = []
    for s in veg:
        if merged and s["startM"] <= merged[-1][1]:
            merged[-1][1] = max(merged[-1][1], s["endM"])
        else:
            merged.append([s["startM"], s["endM"]])
    return any(a != 0 and b != obs["lengthM"] for a, b in merged)


def select(inp) -> dict:
    if type(inp) is not dict or set(inp) != {"observation", "catalog", "polarization", "txHeightM", "rxHeightM",
                                             "canopy", "budget"}:
        return {"code": "INVALID_INPUT"}
    layer_in = {k: inp[k] for k in ("observation", "catalog", "polarization", "txHeightM", "rxHeightM", "canopy")}
    over = o.over_canopy(layer_in)
    through = v.vegetation_layer({k: layer_in[k] for k in ("observation", "catalog", "polarization", "txHeightM", "rxHeightM")})
    if through["code"] in PASS_THROUGH:
        return {"code": through["code"]}
    if over["code"] in PASS_THROUGH:
        return {"code": over["code"]}
    budget = typed_budget(inp["budget"])
    if budget is None:
        return {"code": "INVALID_INPUT"}
    if through["code"] == "NUMERIC_FAILURE" or over["code"] == "NUMERIC_FAILURE":
        return {"code": "NUMERIC_FAILURE"}
    if through["code"] == "NO_VEGETATION":
        if budget["l2Db"] is None:
            return {"code": "UNRESOLVED", "reason": "BASE_BUDGET_MISSING"}
        total = budget["fsplDb"] + budget["l2Db"]
        if not (math.isfinite(total) and total <= po.F32_MAX):
            return {"code": "NUMERIC_FAILURE"}
        return {"code": "NO_VEGETATION", "totalLossDb": total}
    # Through branch (REQ VP04).
    t_branch = {"present": False, "reason": None}
    if through["code"] == "NO_MODEL_FOR_SCENARIO":
        t_branch["reason"] = "NO_MODEL_FOR_SCENARIO"
    else:
        free_space_only = any(r["excessBaseline"] == "FREE_SPACE" for r in through["intervals"])
        if free_space_only:
            base = budget["fsplDb"]
        elif budget["l2Db"] is None:
            base = None
            t_branch["reason"] = "BASE_BUDGET_MISSING"
        else:
            base = budget["fsplDb"] + budget["l2Db"]
        if base is not None:
            t_branch = {"present": True, "reason": None, "baseDb": base, "vegetationDb": through["lossDb"],
                        "totalDb": base + through["lossDb"], "closed": through["allScenariosClosed"],
                        "budgetMode": "FREE_SPACE_PLUS_JB" if free_space_only else "BASE_PLUS_EXCESS"}
    # Over branch (REQ VP05).
    o_branch = {"present": False, "reason": None}
    if mid_path_vegetation(inp):
        o_branch["reason"] = "MID_PATH_VEGETATION"
    elif over["code"] != "OK":
        o_branch["reason"] = over["code"]
    elif budget["l2OverDb"] is None:
        o_branch["reason"] = "BASE_BUDGET_MISSING"
    else:
        base = budget["fsplOverDb"] + budget["l2OverDb"]
        o_branch = {"present": True, "reason": None, "baseDb": base, "canopyDb": over["totalAhDb"],
                    "totalDb": base + over["totalAhDb"], "closed": over["status"] == "SOURCE_DOMAIN"}
    values = [b[k] for b in (t_branch, o_branch) if b["present"] for k in ("baseDb", "totalDb")]
    if not all(math.isfinite(x) and 0 <= x <= po.F32_MAX for x in values):
        return {"code": "NUMERIC_FAILURE"}
    if not t_branch["present"] and not o_branch["present"]:
        return {"code": "UNRESOLVED", "reason": "NO_BRANCH", "through": t_branch, "over": o_branch}
    if t_branch["present"] and (not o_branch["present"] or t_branch["totalDb"] <= o_branch["totalDb"]):
        selected = "THROUGH"
    else:
        selected = "OVER"
    total = t_branch["totalDb"] if selected == "THROUGH" else o_branch["totalDb"]
    # VP06: closure. Both branches present and closed -> CLOSED. Only the over branch is source-backed ->
    # OVER_REFERENCE_ONLY: over.totalDb is an engineering median reference, not a bound of the result (codex #3685).
    if t_branch["present"] and o_branch["present"] and t_branch["closed"] and o_branch["closed"]:
        status = "CLOSED"
    elif o_branch["present"] and o_branch["closed"]:
        status = "OVER_REFERENCE_ONLY"
    else:
        status = "OPEN"
    return {"code": "OK", "selected": selected, "totalLossDb": total, "status": status,
            "through": t_branch, "over": o_branch}


def evaluate(case) -> dict:
    return select(case["input"])


def check(path: Path) -> int:
    d = json.loads(path.read_bytes())
    errors: list[str] = []
    for c in d["cases"]:
        po.close(evaluate(c), c["expect"], c["id"], errors, 1e-9)
    if errors:
        print("\n".join(errors[:20]))
        print(f"VEGETATION_PATH_ORACLE_FAIL: {len(errors)}")
        return 1
    print(f"VEGETATION_PATH_ORACLE_PASS: {len(d['cases'])} cases")
    return 0


def main(argv=None) -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--check", type=Path, required=True)
    return check(p.parse_args(argv).check)


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    raise SystemExit(main())
