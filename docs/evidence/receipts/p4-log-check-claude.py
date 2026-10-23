#!/usr/bin/env python3
"""claude: independent check of a P4 Workbench log against the frozen corpus (REQ-vegetation v0.4).

Reads only what the probe prints ([RICON_VEG], [RICON_VEG_OPEN], [RICON_VEG_INTERVAL], [RICON_VEG_CANDIDATE]) and
compares codes, NO_MODEL intervals, primary models/statuses/closure, candidate order/models/statuses and every printed
loss with the native tolerance max(1e-3 dB, 1e-5*|x|). Also: per-case match/immutable flags, V59 absent (host-only),
exactly one done: PASS, no SCRIPT (E), no tagged FAIL, the summary line.
Usage: python -I p4-log-check-claude.py <console.log> <vegetation_v1.json>"""
import hashlib
import json
import re
import sys

FROZEN = "576fed29dd79f83ec045fabb10ded6a52103a33a53935d2ee270299668657ca9"
HOST_ONLY = {"V59"}
log_path, corpus_path = sys.argv[1], sys.argv[2]
raw = open(corpus_path, "rb").read()
errors = []
if hashlib.sha256(raw).hexdigest() != FROZEN:
    errors.append("corpus SHA")
cases = {c["id"]: c["expect"] for c in json.loads(raw)["cases"]}
lines = open(log_path, encoding="utf-8", errors="replace").read().splitlines()
if any(re.search(r"SCRIPT\s*\(E\)", l) for l in lines):
    errors.append("SCRIPT (E)")
tagged = [l for l in lines if re.search(r"\[RICON_[^\]]+\].*(?<![A-Za-z])FAIL(?![A-Za-z])", l)]
if tagged:
    errors.append(f"tagged FAIL: {tagged[:3]}")
if sum("done: PASS" in l and "[RICON_VEG" in l for l in lines) != 1:
    errors.append("done: PASS count")


def close(got, want):
    return abs(float(got) - want) <= max(1e-3, 1e-5 * abs(want))


head = re.compile(r"\[RICON_VEG\] id=(V\d\d) code=(\w+) hasLoss=(\w+) loss=(\S+) intervals=(\d+) closed=(\w+) match=(\w+) immutable=(\w+)")
opened = re.compile(r"\[RICON_VEG_OPEN\] id=(V\d\d) start=(\S+) end=(\S+) scenario=(\S+)")
inter = re.compile(r"\[RICON_VEG_INTERVAL\] id=(V\d\d) index=(\d+) start=(\S+) end=(\S+) primary=(\w+) loss=(\S+) status=(\w+) closed=(\w+)")
cand = re.compile(r"\[RICON_VEG_CANDIDATE\] id=(V\d\d) interval=(\d+) index=(\d+) model=(\w+) loss=(\S+) status=(\w+)")
seen, opens, rows, cands = {}, {}, {}, {}
for l in lines:
    for pat, store, key in ((head, seen, lambda g: g[0]), (opened, opens, lambda g: g[0]),
                            (inter, rows, lambda g: (g[0], int(g[1]))), (cand, cands, lambda g: (g[0], int(g[1]), int(g[2])))):
        m = pat.search(l)
        if m:
            k = key(m.groups())
            if k in store:
                errors.append(f"dup {k}")
            store[k] = m.groups()
numbers = 0
for cid, want in cases.items():
    if cid in HOST_ONLY:
        if cid in seen:
            errors.append(f"{cid} host-only printed")
        continue
    got = seen.get(cid)
    if not got:
        errors.append(f"{cid} missing")
        continue
    _, code, has, loss, nint, closed, match, immutable = got
    if code != want["code"] or match != "1" or immutable != "1":
        errors.append(f"{cid} code/match/immutable {code}/{match}/{immutable}")
    if (has == "1") != ("lossDb" in want):
        errors.append(f"{cid} hasLoss")
    if "lossDb" in want:
        numbers += 1
        if not close(loss, want["lossDb"]):
            errors.append(f"{cid} loss {loss} != {want['lossDb']}")
    if want["code"] == "NO_MODEL_FOR_SCENARIO":
        o = opens.get(cid)
        w = want["interval"]
        if not o or not close(o[1], w["startM"]) or not close(o[2], w["endM"]) or o[3] != w["scenario"]:
            errors.append(f"{cid} open interval {o} != {w}")
    elif cid in opens:
        errors.append(f"{cid} unexpected open interval")
    want_rows = want.get("intervals", [])
    if int(nint) != len(want_rows):
        errors.append(f"{cid} interval count")
    if want["code"] == "OK" and (closed == "1") != want["allScenariosClosed"]:
        errors.append(f"{cid} allScenariosClosed")
    for i, w in enumerate(want_rows):
        r = rows.get((cid, i))
        if not r:
            errors.append(f"{cid}[{i}] missing")
            continue
        _, _, start, end, primary, ploss, status, rclosed = r
        numbers += 3
        if not (close(start, w["startM"]) and close(end, w["endM"]) and close(ploss, w["primaryLossDb"])):
            errors.append(f"{cid}[{i}] numbers")
        if primary != w["primaryModel"] or status != w["primaryStatus"] or (rclosed == "1") != w["scenarioClosed"]:
            errors.append(f"{cid}[{i}] primary/status/closed")
        for j, wc in enumerate(w["candidates"]):
            c = cands.get((cid, i, j))
            numbers += 1
            if not c or c[3] != wc["model"] or c[5] != wc["status"] or not close(c[4], wc["lossDb"]):
                errors.append(f"{cid}[{i}].c{j} {c} != {wc}")
        if (cid, i, len(w["candidates"])) in cands:
            errors.append(f"{cid}[{i}] extra candidate")
summary = [l for l in lines if re.search(r"\[RICON_VEG\] cases=58 match=58 immutable=58 controls=13 extras=4", l)]
if not summary:
    errors.append("summary")
print("\n".join(errors) if errors else
      f"P4_LOG_CHECK_PASS: 58 native, V59 absent, {numbers} printed numbers, SCRIPT(E) 0")
sys.exit(1 if errors else 0)
