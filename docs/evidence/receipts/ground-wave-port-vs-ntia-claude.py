"""claude, TOOL_ONLY: Python port oracle/ground_wave_oracle.lfmf versus the unmodified NTIA LFMF v1.1 build.

The build is the scratchpad executable of receipts/ground_wave_ntia_build_claude.bat + ground_wave_ntia_driver_claude.cpp
(sha256 e3dc0246de37be9d44833fb72cf133ebf2ed157b4aba679ee73035e34ef590f8), which prints 9 decimals; it is not in the repo.
Part 1: a wide grid (f 0.01-30 MHz, d 1 m-10000 km, 7 height pairs, 5 grounds, H and V, Ns edges, validation edges,
the 95 published NTIA examples). Part 2: every OK case of oracle/vectors/ground_wave_v1.json at P = 1000 W.
Pass: identical rc and method, |dB difference| <= 2e-9 for A_btl, E and P_rx; NTIA published values within 0.05 dB.
Usage: python -B ground-wave-port-vs-ntia-claude.py <repo> <exe> <LFMF_Examples.csv> <out.json>"""
import csv
import json
import math
import subprocess
import sys
import time

repo, exe, examples, out = sys.argv[1:5]
sys.path.insert(0, repo + "/oracle")
import ground_wave_oracle as g  # noqa: E402

F = (0.01, 0.1, 0.5, 1.0, 2.0, 5.0, 10.0, 15.0, 20.0, 22.5, 25.0, 27.5, 30.0)
D = (0.001, 0.01, 0.1, 0.5, 1.0, 2.0, 5.0, 10.0, 20.0, 30.0, 50.0, 100.0, 300.0, 1000.0, 3000.0, 10000.0)
H = ((0.0, 0.0), (0.0, 2.0), (1.0, 1.0), (2.0, 2.0), (10.0, 2.0), (50.0, 50.0), (50.0, 0.0))
GROUND = ((15.0, 0.005), (70.0, 5.0), (80.0, 0.01), (4.0, 0.001), (30.0, 0.02))
cases = []
for f in F:
    dt = 80 / math.cbrt(f)
    for d in D + (dt * (1 - 1e-9), dt * (1 + 1e-9)):
        if not 0.001 <= d <= 10000:
            continue
        for h1, h2 in H:
            for eps, sig in GROUND:
                for pol in (0, 1):
                    cases.append((h1, h2, f, 1.0, 301.0, d, eps, sig, pol))
for ns in (250.0, 400.0):
    for f in (20.0, 30.0):
        for d in (1.0, 30.0, 300.0):
            cases.append((2.0, 2.0, f, 1.0, ns, d, 15.0, 0.005, 1))
with open(examples, newline="") as fh:
    ex = [r for r in csv.DictReader(fh)]
for r in ex:
    cases.append((float(r["h_tx__meter"]), float(r["h_rx__meter"]), float(r["f__mhz"]), float(r["P_tx__watt"]),
                  float(r["N_s"]), float(r["d__km"]), float(r["epsilon"]), float(r["sigma"]), int(r["pol"])))
# validation edges (rc only)
for c in ((50.0001, 1, 20, 1, 301, 1, 15, .005, 1), (-1e-9, 1, 20, 1, 301, 1, 15, .005, 1), (1, 50.0001, 20, 1, 301, 1, 15, .005, 1),
          (1, 1, 30.0000001, 1, 301, 1, 15, .005, 1), (1, 1, .0099999, 1, 301, 1, 15, .005, 1), (1, 1, 20, 0, 301, 1, 15, .005, 1),
          (1, 1, 20, 1, 249.999, 1, 15, .005, 1), (1, 1, 20, 1, 400.001, 1, 15, .005, 1), (1, 1, 20, 1, 301, .0009999, 15, .005, 1),
          (1, 1, 20, 1, 301, 10000.001, 15, .005, 1), (1, 1, 20, 1, 301, 1, .9999, .005, 1), (1, 1, 20, 1, 301, 1, 15, 0, 1),
          (1, 1, 20, 1, 301, 1, 15, .005, 2), (50, 50, 30, 1, 301, .001, 1, 1e-9, 0), (0, 0, .01, 1, 250, 10000, 1, 1e-9, 1)):
    cases.append(tuple(float(v) for v in c[:8]) + (int(c[8]),))

cmds = "\n".join("LFMF " + " ".join(repr(v) for v in c) for c in cases) + "\n"
t0 = time.time()
res = subprocess.run([exe], input=cmds, capture_output=True, text=True, check=True)
t_exe = time.time() - t0
lines = res.stdout.strip().splitlines()
assert len(lines) == len(cases), (len(lines), len(cases))
worst = {"A_btl": 0.0, "E": 0.0, "P_rx": 0.0}
mismatch, failures, rc_counts, inf_exe = [], [], {}, 0
t0 = time.time()
for c, line in zip(cases, lines):
    kv = dict(t.split("=") for t in line.split()[1:])
    rc = int(kv["rc"])
    try:
        prc, a, e, p, m = g.lfmf(*c)
    except g.NumericFailure as err:
        failures.append({"case": c, "exe": line, "err": str(err)})
        continue
    rc_counts[rc] = rc_counts.get(rc, 0) + 1
    if prc != rc:
        mismatch.append({"case": c, "exe": line, "port_rc": prc})
        continue
    if rc != 0:
        continue
    ex_vals = {"A_btl": float(kv["A_btl"]), "E": float(kv["E"]), "P_rx": float(kv["P_rx"])}
    if not all(math.isfinite(v) for v in ex_vals.values()):
        inf_exe += 1
        mismatch.append({"case": c, "exe": line, "port": (a, e, p, m), "note": "exe non-finite"})
        continue
    if int(kv["method"]) != m:
        mismatch.append({"case": c, "exe": line, "port": (a, e, p, m), "note": "method"})
        continue
    for name, val in (("A_btl", a), ("E", e), ("P_rx", p)):
        dev = abs(val - ex_vals[name])
        worst[name] = max(worst[name], dev)
        if dev > 2e-9:
            mismatch.append({"case": c, "exe": line, "port": (a, e, p, m), "note": f"{name} dev {dev:.3g}"})
            break
t_port = time.time() - t0
# NTIA published examples (1 decimal) versus the port
ex_dev = 0.0
ex_bad = []
for r in ex:
    c = (float(r["h_tx__meter"]), float(r["h_rx__meter"]), float(r["f__mhz"]), float(r["P_tx__watt"]), float(r["N_s"]),
         float(r["d__km"]), float(r["epsilon"]), float(r["sigma"]), int(r["pol"]))
    prc, a, e, p, m = g.lfmf(*c)
    if prc != int(r["rtn"]) or (prc == 0 and int(r["method"]) != m):
        ex_bad.append((c, r))
        continue
    if prc == 0:
        d = max(abs(a - float(r["A_btl__db"])), abs(e - float(r["E_dBuVm"])), abs(p - float(r["P_rx__dbm"])))
        ex_dev = max(ex_dev, d)
        if d > 0.05:
            ex_bad.append((c, r, a, e, p))
# Part 2: corpus OK cases versus the build (P = 1000 W, V)
corpus = json.load(open(repo + "/oracle/vectors/ground_wave_v1.json", encoding="utf-8"))
ok_cases = [c for c in corpus["cases"] if c["expect"]["code"] == "OK"]
ccmds = []
for c in ok_cases:
    i = c["input"]
    args = (float(i["txHeightM"]), float(i["rxHeightM"]), float(i["frequencyMHz"]), 1000.0, float(i["surfaceRefractivityN"]),
            float(i["distanceKm"]), float(i["ground"]["epsilonReal"]), float(i["ground"]["conductivitySm"]), 1)
    ccmds.append("LFMF " + " ".join(repr(v) for v in args))
cres = subprocess.run([exe], input="\n".join(ccmds) + "\n", capture_output=True, text=True,
                      check=True).stdout.strip().splitlines()
assert len(cres) == len(ok_cases), (len(cres), len(ok_cases))
corpus_worst, corpus_bad = 0.0, []
methods = ("FLAT_EARTH_CURVE", "RESIDUE_SERIES")
for c, line in zip(ok_cases, cres):
    kv = dict(t.split("=") for t in line.split()[1:])
    e = c["expect"]
    dev = max(abs(float(kv["A_btl"]) - e["basicLossDb"]), abs(float(kv["E"]) - e["fieldDbuVm"]))
    corpus_worst = max(corpus_worst, dev)
    if int(kv["rc"]) != 0 or methods[int(kv["method"])] != e["method"] or dev > 2e-9:
        corpus_bad.append((c["id"], line))
summary = {"corpus_ok_cases": len(ok_cases), "corpus_bad": len(corpus_bad), "corpus_worst_abs_dev": corpus_worst,
           "cases": len(cases), "rc_counts": rc_counts, "worst_abs_dev": worst, "mismatches": len(mismatch),
           "numeric_failures": len(failures), "exe_non_finite": inf_exe, "examples": len(ex), "examples_bad": len(ex_bad),
           "examples_worst_dev": ex_dev, "t_exe_s": round(t_exe, 1), "t_port_s": round(t_port, 1)}
json.dump({"summary": summary, "corpus_bad": corpus_bad, "mismatch": mismatch[:50], "failures": failures[:50], "examples_bad": ex_bad[:20]},
          open(out, "w"), indent=1)
print(json.dumps(summary, indent=1))
