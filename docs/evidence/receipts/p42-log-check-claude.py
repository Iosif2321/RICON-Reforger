#!/usr/bin/env python3
"""claude: independent check of a P4.2 Workbench log against the frozen corpus (REQ-vegetation-overcanopy v0.1).

Re-reads [RICON_CANOPY], [RICON_CANOPY_GEOMETRY], [RICON_CANOPY_M6]: codes, totals, terminal order/end/material/
provenance/interval/antenna/canopy and hdif/theta/nu/J/Ah with the native tolerance max(1e-3, 1e-5*|x|); match and
immutable flags; O28 absent; summary; one done: PASS; no SCRIPT (E); no tagged FAIL.
Usage: python -I p42-log-check-claude.py <console.log> <vegetation_overcanopy_v1.json>"""
import hashlib
import json
import re
import sys

FROZEN = "00d7da2115f5d27445ae40502ef6fa3541e027e85a4275b7ba80264664a99cf2"
HOST_ONLY = {"O28"}
log_path, corpus_path = sys.argv[1], sys.argv[2]
raw = open(corpus_path, "rb").read()
errors = []
if hashlib.sha256(raw).hexdigest() != FROZEN:
    errors.append("corpus SHA")
cases = {c["id"]: c["expect"] for c in json.loads(raw)["cases"]}
lines = open(log_path, encoding="utf-8", errors="replace").read().splitlines()
if any(re.search(r"SCRIPT\s*\(E\)", l) for l in lines):
    errors.append("SCRIPT (E)")
if [l for l in lines if re.search(r"\[RICON_[^\]]+\].*(?<![A-Za-z])FAIL(?![A-Za-z])", l)]:
    errors.append("tagged FAIL")
if sum("[RICON_CANOPY] done: PASS" in l for l in lines) != 1:
    errors.append("done: PASS count")
if not any(f"[RICON_CANOPY] source={FROZEN}" in l for l in lines):
    errors.append("source line")


def close(got, want):
    return abs(float(got) - want) <= max(1e-3, 1e-5 * abs(want))


head = re.compile(r"\[RICON_CANOPY\] id=(O\d\d) code=(\w+) total=(\S+) terminals=(\d+) match=(\w+) immutable=(\w+)")
geo = re.compile(r"\[RICON_CANOPY_GEOMETRY\] id=(O\d\d) index=(\d+) end=(\w+) material=(\S+) provenance=(\w+) "
                 r"start=(\S+) stop=(\S+) antenna=(\S+) canopy=(\S+)")
m6 = re.compile(r"\[RICON_CANOPY_M6\] id=(O\d\d) index=(\d+) hdif=(\S+) theta=(\S+) nu=(\S+) j=(\S+) ah=(\S+)")
heads, geos, m6s = {}, {}, {}
for l in lines:
    for pat, store, key in ((head, heads, lambda g: g[0]), (geo, geos, lambda g: (g[0], int(g[1]))),
                            (m6, m6s, lambda g: (g[0], int(g[1])))):
        m = pat.search(l)
        if m:
            k = key(m.groups())
            if k in store:
                errors.append(f"dup {k}")
            store[k] = m.groups()
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
    _, code, total, nterm, match, immutable = h
    if code != want["code"] or match != "1" or immutable != "1":
        errors.append(f"{cid} code/match/immutable {code}/{match}/{immutable}")
    terms = want.get("terminals", [])
    if int(nterm) != len(terms):
        errors.append(f"{cid} terminal count")
    if want["code"] == "OK":
        numbers += 1
        if not close(total, want["totalAhDb"]):
            errors.append(f"{cid} total {total} != {want['totalAhDb']}")
    for i, t in enumerate(terms):
        g, n = geos.get((cid, i)), m6s.get((cid, i))
        if not g or not n:
            errors.append(f"{cid}[{i}] missing lines")
            continue
        if (g[2], g[3], g[4]) != (t["end"], t["materialId"], t["canopyProvenance"]):
            errors.append(f"{cid}[{i}] end/material/provenance")
        for got, key in zip(g[5:], ("intervalStartM", "intervalEndM", "antennaHeightM", "canopyHeightM")):
            numbers += 1
            if not close(got, t[key]):
                errors.append(f"{cid}[{i}] {key} {got} != {t[key]}")
        for got, key in zip(n[2:], ("hdifM", "thetaDeg", "nu", "jDb", "ahDb")):
            numbers += 1
            if not close(got, t[key]):
                errors.append(f"{cid}[{i}] {key} {got} != {t[key]}")
    if (cid, len(terms)) in geos:
        errors.append(f"{cid} extra terminal")
summary = [l for l in lines if re.search(r"\[RICON_CANOPY\] cases=30 match=30 immutable=30 controls=20 extras=5 hostOnly=1", l)]
if not summary:
    errors.append("summary")
print("\n".join(errors) if errors else f"P42_LOG_CHECK_PASS: 30 native, O28 absent, {numbers} printed numbers, SCRIPT(E) 0")
sys.exit(1 if errors else 0)
