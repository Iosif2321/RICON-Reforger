#!/usr/bin/env python3
"""claude: independent check of a CT10.1 Workbench log against the frozen corpus (REQ-channel-descriptor v0.1).

The probe compares descriptor/bytes strictly inside Enforce; this checker re-reads what the log prints: op, code and
sizeBytes per case, match/immutable flags, host-only absence, frame/flip counts, the source SHA, the summary,
exactly one done: PASS, no SCRIPT (E) and no tagged FAIL. Usage: python -I ct10-1-log-check-claude.py <log> <json>"""
import hashlib
import json
import re
import sys

FROZEN = "95208dd198cee4680763e6813cb66374832ef241ea1b5590e20799d834f2aa27"
HOST_ONLY = {"D17", "D18", "D19", "D20", "D21", "D31", "D47", "D54"}
log_path, corpus_path = sys.argv[1], sys.argv[2]
raw = open(corpus_path, "rb").read()
errors = []
if hashlib.sha256(raw).hexdigest() != FROZEN:
    errors.append("corpus SHA")
cases = {c["id"]: c for c in json.loads(raw)["cases"]}
lines = open(log_path, encoding="utf-8", errors="replace").read().splitlines()
if any(re.search(r"SCRIPT\s*\(E\)", l) for l in lines):
    errors.append("SCRIPT (E)")
if [l for l in lines if re.search(r"\[RICON_[^\]]+\].*(?<![A-Za-z])FAIL(?![A-Za-z])", l)]:
    errors.append("tagged FAIL")
if sum("[RICON_DESCRIPTOR] done: PASS" in l for l in lines) != 1:
    errors.append("done: PASS count")
if not any(f"[RICON_DESCRIPTOR] source={FROZEN}" in l for l in lines):
    errors.append("source line")
pat = re.compile(r"\[RICON_DESCRIPTOR\] (D\d\d) op=(\w+) code=(\w+) size=(-?\d+) match=(\w+) immutable=(\w+)")
seen = {}
for l in lines:
    m = pat.search(l)
    if m:
        if m.group(1) in seen:
            errors.append(f"dup {m.group(1)}")
        seen[m.group(1)] = m.groups()[1:]
for cid, c in cases.items():
    if cid in HOST_ONLY:
        if cid in seen:
            errors.append(f"{cid} host-only printed")
        continue
    got = seen.get(cid)
    if not got:
        errors.append(f"{cid} missing")
        continue
    op, code, size, match, immutable = got
    want = c["expect"]
    if op != c["input"]["op"] or code != want["code"] or int(size) != want.get("sizeBytes", 0):
        errors.append(f"{cid} {op}/{code}/{size} != {c['input']['op']}/{want['code']}/{want.get('sizeBytes', 0)}")
    if match != "1" or immutable != "1":
        errors.append(f"{cid} match/immutable")
ok_frames = sum(1 for cid, c in cases.items() if cid not in HOST_ONLY and c["expect"]["code"] == "OK" and "bytes" in c["expect"])
if not any(f"[RICON_DESCRIPTOR_FRAMES] frames={ok_frames} flips={4 * ok_frames}" in l for l in lines):
    errors.append("frames/flips")
if not any("[RICON_DESCRIPTOR] cases=46 match=46 immutable=46 controls=8 extras=3 hostOnly=8" in l for l in lines):
    errors.append("summary")
print("\n".join(errors) if errors else f"CT10_1_LOG_CHECK_PASS: 46 native, host-only 8 absent, frames {ok_frames}, SCRIPT(E) 0")
sys.exit(1 if errors else 0)
