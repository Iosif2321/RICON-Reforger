"""claude: blind recomputation of the P1 corpus from REQ-channel-cutoff v0.1 only (oracle source/tests not read).
Usage: python -I check_cutoff.py <channel_cutoff_v1.json>"""
import json
import math
import struct
import sys

MAX = 2147483647
F32_MAX = struct.unpack("<f", b"\xff\xff\x7f\x7f")[0]
FIELDS = {"nowMs", "validFromMs", "validUntilMs", "environmentRevision", "boundRevision", "coverageComplete",
          "resourceExhausted", "toleranceDbm", "numericalGuardDb", "paths"}


def f32(x):
    try:
        return struct.unpack("<f", struct.pack("<f", x))[0]
    except OverflowError:
        return math.copysign(math.inf, x)


def f32num(v):
    """Finite binary32 value, or None when v is not a finite JSON number (strings, bool, NaN/Inf, overflow)."""
    if type(v) not in (int, float):
        return None
    x = f32(float(v))
    return x if math.isfinite(x) else None


def is_int(v, lo, hi):
    return type(v) is int and lo <= v <= hi


def result(decision, reason, upper=None):
    return {"decision": decision, "reason": reason, "usefulIsZero": decision == "BOUNDED", "upperDbm": upper}


def assess(inp):
    bad = result("UNRESOLVED", "INVALID_INPUT")
    if not isinstance(inp, dict) or set(inp) != FIELDS:
        return bad
    if not (is_int(inp["nowMs"], 0, MAX) and is_int(inp["validFromMs"], 0, MAX) and is_int(inp["validUntilMs"], 0, MAX)
            and inp["validFromMs"] < inp["validUntilMs"]
            and is_int(inp["environmentRevision"], 1, MAX) and is_int(inp["boundRevision"], 1, MAX)
            and type(inp["coverageComplete"]) is bool and type(inp["resourceExhausted"]) is bool):
        return bad
    tol = inp["toleranceDbm"]
    if tol is not None:
        tol = f32num(tol)
        if tol is None:
            return bad
    guard = inp["numericalGuardDb"]
    if guard is not None:
        guard = f32num(guard)
        if guard is None or guard < 0:
            return bad
    paths = inp["paths"]
    if not isinstance(paths, list) or len(paths) > 4096:
        return bad
    ids, uppers = set(), []
    for p in paths:
        if not isinstance(p, dict) or set(p) != {"pathId", "upperDbm"}:
            return bad
        pid = p["pathId"]
        if type(pid) is not str or not pid or pid in ids:
            return bad
        ids.add(pid)
        u = p["upperDbm"]
        if u is not None:
            u = f32num(u)
            if u is None:
                return bad
        uppers.append(u)
    if inp["resourceExhausted"]:
        return result("UNRESOLVED", "RESOURCE_EXHAUSTED")
    if not inp["coverageComplete"]:
        return result("UNRESOLVED", "INCOMPLETE_COVERAGE")
    if inp["environmentRevision"] != inp["boundRevision"] or not (inp["validFromMs"] <= inp["nowMs"] < inp["validUntilMs"]):
        return result("UNRESOLVED", "STALE_BOUND")
    if not uppers:
        return result("BOUNDED", "EMPTY_PATH_SET")
    if any(u is None for u in uppers):
        return result("CONTINUE", "MISSING_PATH_BOUND")
    if guard is None:
        return result("CONTINUE", "NUMERIC_UNCERTIFIED")
    try:
        m = max(uppers)
        U = m + 20.0 * math.log10(sum(10.0 ** ((u - m) / 20.0) for u in uppers)) + guard
    except (OverflowError, ValueError):
        return result("UNRESOLVED", "NUMERIC_FAILURE")
    if not math.isfinite(U) or abs(U) > F32_MAX:
        return result("UNRESOLVED", "NUMERIC_FAILURE")
    if tol is None:
        return result("CONTINUE", "CUTOFF_DISABLED", U)
    if U < tol:
        return result("BOUNDED", "BELOW_TOLERANCE", U)
    return result("CONTINUE", "ABOVE_TOLERANCE", U)


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    d = json.load(open(sys.argv[1], encoding="utf-8"))
    bad = 0
    for c in d["cases"]:
        before = json.dumps(c["input"], sort_keys=True)
        got, exp = assess(c["input"]), c["expect"]
        diff = [k for k in ("decision", "reason", "usefulIsZero") if got[k] != exp[k]]
        gu, eu = got["upperDbm"], exp["upperDbm"]
        if (gu is None) != (eu is None) or (gu is not None and abs(gu - eu) > 1e-6):
            diff.append(f"upperDbm {gu} != {eu}")
        if json.dumps(c["input"], sort_keys=True) != before:
            diff.append("input mutated")
        if diff:
            bad += 1
            print("DIFF", c["id"], diff, got)
    n = len(d["cases"])
    print(f"P1_BLIND_CHECK {'PASS' if not bad else 'FAIL'}: {n - bad}/{n}")
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
