#!/usr/bin/env python3
"""claude: independent check of a P3.1 Workbench log against the frozen corpus (REQ-object-rf-profile v0.1).

Re-reads [RICON_OBJPROFILE] lines: code, knowledge, materialId, source, matchedKey per case (INVALID_INPUT prints
defaults UNKNOWN/""/NONE/""), match/immutable flags, host-only absence, source SHA, summary, one done: PASS,
no SCRIPT (E), no tagged FAIL. Usage: python -I p3-1-log-check-claude.py <console.log> <object_rf_profile_v1.json>"""
import hashlib
import json
import re
import sys

FROZEN = "b0b93be3238b49c223c859ac14e22f045ae708ce03f1cae2ed731eaf0531e4c5"
HOST_ONLY = {"R31", "R32", "R33", "R34", "R35", "R36", "R41"}
log_path, corpus_path = sys.argv[1], sys.argv[2]
raw = open(corpus_path, "rb").read()
errors = [] if hashlib.sha256(raw).hexdigest() == FROZEN else ["corpus SHA"]
cases = {c["id"]: c["expect"] for c in json.loads(raw)["cases"]}
lines = open(log_path, encoding="utf-8", errors="replace").read().splitlines()
if any(re.search(r"SCRIPT\s*\(E\)", l) for l in lines):
    errors.append("SCRIPT (E)")
if [l for l in lines if re.search(r"\[RICON_[^\]]+\].*(?<![A-Za-z])FAIL(?![A-Za-z])", l)]:
    errors.append("tagged FAIL")
if sum("[RICON_OBJPROFILE] done: PASS" in l for l in lines) != 1:
    errors.append("done: PASS count")
if not any(f"[RICON_OBJPROFILE] source={FROZEN}" in l for l in lines):
    errors.append("source line")
pat = re.compile(r"\[RICON_OBJPROFILE\] (R\d\d) code=(\w+) knowledge=(\w+) material=(\S*) source=(\w+) matched=(\S*) "
                 r"match=(\w+) immutable=(\w+)$")
seen = {}
for l in lines:
    m = pat.search(l)
    if m:
        if m.group(1) in seen:
            errors.append(f"dup {m.group(1)}")
        seen[m.group(1)] = m.groups()[1:]
for cid, want in cases.items():
    if cid in HOST_ONLY:
        if cid in seen:
            errors.append(f"{cid} host-only printed")
        continue
    got = seen.get(cid)
    if not got:
        errors.append(f"{cid} missing")
        continue
    full = dict(want) if want["code"] != "INVALID_INPUT" else {"code": "INVALID_INPUT", "knowledge": "UNKNOWN",
                                                                 "materialId": "", "source": "NONE", "matchedKey": ""}
    exp = (full["code"], full["knowledge"], full["materialId"], full["source"], full["matchedKey"], "1", "1")
    if got != exp:
        errors.append(f"{cid} {got} != {exp}")
if not any("[RICON_OBJPROFILE] cases=40 match=40 immutable=40 controls=6 extras=2 hostOnly=7" in l for l in lines):
    errors.append("summary")
print("\n".join(errors) if errors else "P3_1_LOG_CHECK_PASS: 40 native, host-only 7 absent, SCRIPT(E) 0")
sys.exit(1 if errors else 0)
