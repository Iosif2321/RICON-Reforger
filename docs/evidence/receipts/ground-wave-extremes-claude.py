"""claude, TOOL_ONLY: failure behaviour of the Python port versus the unmodified NTIA LFMF v1.1 build on extreme but
finite ground parameters (codex #4088). Each command runs in its own exe process, so a C++ exception (process abort)
is seen as such instead of truncating a batch. The port must give NumericFailure exactly where the build aborts or
returns non-finite values, and the same A_btl (|Δ| ≤ 2e-9 dB) where the build succeeds.
Usage: python -B ground-wave-extremes-claude.py <repo> <ntia exe> <out.json>"""
import json
import math
import subprocess
import sys

repo, exe, out = sys.argv[1:4]
sys.path.insert(0, repo + "/oracle")
import ground_wave_oracle as g  # noqa: E402

GROUNDS = ((15, 1e308), (1e308, 1), (15, 1e300), (1e300, 1), (15, 1e200), (1e200, 1), (15, 1e100), (1e100, 1),
           (15, 1e50), (1e50, 1), (15, 1e20), (1e20, 1), (15, 1e9), (1e9, 0.005), (1e6, 1e6), (1, 1e-300),
           (1, 5e-324), (1e308, 1e308))
cases = [(h, h, f, 1000.0, 301.0, d, float(eps), float(sig), 1)
         for f in (0.01, 1.0, 25.0, 30.0) for d in (0.001, 1.0, 100.0, 10000.0)
         for eps, sig in GROUNDS for h in (0.0, 50.0)]


def parse(text):
    """MSVC printf: inf, -inf, nan, -nan(ind) and the like are non-finite."""
    try:
        return float(text)
    except ValueError:
        return math.nan


def native(case):
    p = subprocess.run([exe], input="LFMF " + " ".join(repr(v) for v in case) + "\n", capture_output=True, text=True)
    line = p.stdout.strip()
    if p.returncode != 0 or not line:
        return {"status": "ABORT", "returncode": p.returncode}
    kv = dict(t.split("=") for t in line.split()[1:])
    values = [parse(kv[k]) for k in ("A_btl", "E", "P_rx")]
    if int(kv["rc"]) != 0:
        return {"status": "RC", "rc": int(kv["rc"])}
    if not all(math.isfinite(v) for v in values):
        return {"status": "NONFINITE"}
    return {"status": "OK", "A": values[0], "method": int(kv["method"])}


rows, mismatches = [], []
for case in cases:
    n = native(case)
    try:
        rc, a, _e, _p, m = g.lfmf(*case)
        port = {"status": "OK", "A": a, "method": m}
    except (g.NumericFailure, ArithmeticError, ValueError) as error:
        port = {"status": "FAILURE", "error": type(error).__name__}
    if n["status"] == "OK":
        same = port["status"] == "OK" and port["method"] == n["method"] and abs(port["A"] - n["A"]) <= 2e-9
    else:
        same = port["status"] == "FAILURE"
    rows.append({"case": case, "native": n, "port": port, "same": same})
    if not same:
        mismatches.append(rows[-1])
summary = {"cases": len(cases), "mismatches": len(mismatches),
           "native": {s: sum(r["native"]["status"] == s for r in rows) for s in ("OK", "NONFINITE", "ABORT", "RC")},
           "port_failures": sum(r["port"]["status"] == "FAILURE" for r in rows)}
json.dump({"summary": summary, "mismatches": mismatches, "rows": rows}, open(out, "w"), indent=1)
print(json.dumps(summary))
for m in mismatches[:10]:
    print(m)
