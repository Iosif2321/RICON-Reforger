#!/usr/bin/env python3
"""claude: independent check of a P4.3 Workbench log against the frozen corpus (REQ-vegetation-path v0.4).

Re-reads [RICON_VPATH] and [RICON_VPATH_BRANCH]: code, loss, selected, status, reason; per branch present, reason,
base, added (vegetationDb / canopyDb), total, closed, budget mode; numbers with max(1e-3, 1e-5*|x|); match/immutable;
P18/P27 absent; summary; one done: PASS; no SCRIPT (E); no tagged FAIL.
Usage: python -I p43-log-check-claude.py <console.log> <vegetation_path_v1.json>"""
import hashlib
import json
import re
import sys

FROZEN = "d9e3b7e7d01c8149b090e3e7642917e542f994632f18b5f54aaa12e6ca0bab51"
HOST_ONLY = {"P18", "P27"}
log_path, corpus_path = sys.argv[1], sys.argv[2]
raw = open(corpus_path, "rb").read()
errors = [] if hashlib.sha256(raw).hexdigest() == FROZEN else ["corpus SHA"]
cases = {c["id"]: c["expect"] for c in json.loads(raw)["cases"]}
lines = open(log_path, encoding="utf-8", errors="replace").read().splitlines()
if any(re.search(r"SCRIPT\s*\(E\)", l) for l in lines):
    errors.append("SCRIPT (E)")
if [l for l in lines if re.search(r"\[RICON_[^\]]+\].*(?<![A-Za-z])FAIL(?![A-Za-z])", l)]:
    errors.append("tagged FAIL")
if sum("[RICON_VPATH] done: PASS" in l for l in lines) != 1:
    errors.append("done: PASS count")
if not any(f"[RICON_VPATH] source={FROZEN}" in l for l in lines):
    errors.append("source line")


def close(got, want):
    return abs(float(got) - want) <= max(1e-3, 1e-5 * abs(want))


head = re.compile(r"\[RICON_VPATH\] id=(P\d\d) code=(\w+) hasLoss=(\d) loss=(\S+) selected=(\w+) status=(\w+) "
                  r"reason=(\w+) match=(\w+) immutable=(\w+)")
br = re.compile(r"\[RICON_VPATH_BRANCH\] id=(P\d\d) branch=(\w+) present=(\d) reason=(\w+) base=(\S+) added=(\S+) "
                r"total=(\S+) closed=(\d) mode=(\w+)")
heads, branches = {}, {}
for l in lines:
    m = head.search(l)
    if m:
        heads.setdefault(m.group(1), m.groups()[1:])
    m = br.search(l)
    if m:
        branches[(m.group(1), m.group(2))] = m.groups()[2:]
numbers = 0
for cid, want in cases.items():
    if cid in HOST_ONLY:
        if cid in heads:
            errors.append(f"{cid} host-only printed")
        continue
    h = heads.get(cid)
    if not h:
        errors.append(f"{cid} missing")
        continue
    code, has, loss, selected, status, reason, match, immutable = h
    if code != want["code"] or match != "1" or immutable != "1":
        errors.append(f"{cid} code/match/immutable {code}/{match}/{immutable}")
    has_loss = "totalLossDb" in want
    if (has == "1") != has_loss:
        errors.append(f"{cid} hasLoss")
    if has_loss:
        numbers += 1
        if not close(loss, want["totalLossDb"]):
            errors.append(f"{cid} loss {loss} != {want['totalLossDb']}")
    if (selected, status) != (want.get("selected", "NONE"), want.get("status", "NONE")):
        errors.append(f"{cid} selected/status {selected}/{status}")
    if reason != (want.get("reason") or "NONE"):
        errors.append(f"{cid} reason {reason}")
    for name, key, added in (("THROUGH", "through", "vegetationDb"), ("OVER", "over", "canopyDb")):
        if key not in want:
            if (cid, name) in branches:
                errors.append(f"{cid} unexpected branch {name}")
            continue
        w, g = want[key], branches.get((cid, name))
        if not g:
            errors.append(f"{cid} {name} missing")
            continue
        present, breason, base, add, total, closed, mode = g
        if (present == "1") != w["present"] or breason != (w["reason"] or "NONE"):
            errors.append(f"{cid} {name} present/reason")
        if w["present"]:
            numbers += 3
            if not (close(base, w["baseDb"]) and close(add, w[added]) and close(total, w["totalDb"])):
                errors.append(f"{cid} {name} numbers")
            if (closed == "1") != w["closed"] or mode != w.get("budgetMode", "NONE"):
                errors.append(f"{cid} {name} closed/mode")
if not any("[RICON_VPATH] cases=29 match=29 immutable=29 controls=23 extras=3 hostOnly=2" in l for l in lines):
    errors.append("summary")
print("\n".join(errors) if errors else f"P43_LOG_CHECK_PASS: 29 native, host-only 2 absent, {numbers} printed numbers, SCRIPT(E) 0")
sys.exit(1 if errors else 0)
