#!/usr/bin/env python3
"""claude: independent check of a P3.2a Workbench log against the frozen corpus (REQ-material-slab v0.2, rev2).

Re-reads [RICON_SLAB] lines: code, loss (within max(0.05 dB, 2e-6|L|)), provenance (NONE on refusal), match and
immutable flags per native case in corpus order, host-only absence, source LF-SHA, three [RICON_SLAB_TETM] ok lines,
the summary counts, one done: PASS, no SCRIPT (E), no tagged FAIL.
Usage: python -I p3-2a-log-check-claude.py <console.log> <material_slab_v1.json>"""
import hashlib
import json
import re
import sys

CORPUS_LF_SHA = "2f5a4f10f5e1e472ea9eb253bc07230c806ad908002d59ab4db19b90cce1efbd"
log_path, corpus_path = sys.argv[1], sys.argv[2]
raw = open(corpus_path, "rb").read().replace(bytes([13, 10]), bytes([10]))
errors = [] if hashlib.sha256(raw).hexdigest() == CORPUS_LF_SHA else ["corpus LF-SHA"]
doc = json.loads(raw)
native = [c for c in doc["cases"] if c["native"]]
host_only = {c["id"] for c in doc["cases"] if not c["native"]}
log_bytes = open(log_path, "rb").read()
lines = log_bytes.decode("utf-8", errors="replace").splitlines()
if any(re.search(r"SCRIPT\s*\(E\)", l) for l in lines):
    errors.append("SCRIPT (E)")
if [l for l in lines if re.search(r"\[RICON_[^\]]+\].*(?<![A-Za-z])FAIL(?![A-Za-z])", l)]:
    errors.append("tagged FAIL")
if sum("[RICON_SLAB] done: PASS" in l for l in lines) != 1:
    errors.append("done: PASS count")
if not any(f"[RICON_SLAB] source={CORPUS_LF_SHA}" in l for l in lines):
    errors.append("source line")
pat = re.compile(r"\[RICON_SLAB\] id=(MS\d{3}) code=(\w+) loss=(\S+) provenance=(\w+) match=(\w+) immutable=(\w+)$")
seen, order = {}, []
for l in lines:
    m = pat.search(l)
    if m:
        if m.group(1) in seen:
            errors.append(f"dup {m.group(1)}")
        seen[m.group(1)] = m.groups()[1:]
        order.append(m.group(1))
if order != [c["id"] for c in native]:
    errors.append("case order")
worst = 0.0
for case in doc["cases"]:
    cid, want = case["id"], case["expect"]
    if cid in host_only:
        if cid in seen:
            errors.append(f"{cid} host-only printed")
        continue
    got = seen.get(cid)
    if not got:
        errors.append(f"{cid} missing")
        continue
    code, loss, provenance, match, immutable = got
    if code != want["code"] or match != "1" or immutable != "1":
        errors.append(f"{cid} {got} != {want}")
        continue
    if want["code"] != "OK":
        if provenance != "NONE":
            errors.append(f"{cid} provenance printed on refusal")
        continue
    try:
        actual = float(loss)
    except ValueError:
        errors.append(f"{cid} loss not numeric: {loss}")
        continue
    expected = want["lossDb"]
    deviation = abs(actual - expected)
    worst = max(worst, deviation)
    if provenance != want["provenance"] or deviation > max(0.05, 2e-6 * abs(expected)):
        errors.append(f"{cid} loss {actual} vs {expected} (dev {deviation:.3g}) provenance {provenance}")
tetm = [l for l in lines if "[RICON_SLAB_TETM]" in l]
if len(tetm) != 3 or any(not l.rstrip().endswith("ok=1") for l in tetm):
    errors.append("TETM lines")
summary = re.compile(r"\[RICON_SLAB\] cases=71 match=71 immutable=71 controls=5 extras=3 hostOnly=9 maxDeviationDb=(\S+)")
printed = [summary.search(l).group(1) for l in lines if summary.search(l)]
if len(printed) != 1:
    errors.append("summary")
print("\n".join(errors) if errors else f"P3_2A_LOG_CHECK_PASS: 71 native, host-only 9 absent, SCRIPT(E) 0, "
      f"worst |dev| {worst:.3g} dB (printed maxDeviationDb={printed[0] if printed else '?'}), "
      f"log sha256 {hashlib.sha256(log_bytes).hexdigest()}")
sys.exit(1 if errors else 0)
