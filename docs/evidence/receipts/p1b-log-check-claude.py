#!/usr/bin/env python3
"""claude: independent check of a P1b Workbench log against the frozen corpus (REQ-channel-fading v0.2).

The probe already compares real/imag/phases strictly inside Enforce; this checker re-reads only what the log prints:
code, gain (printed with 9 digits, tolerance 1e-3 dB), phase count, per-case match/immutable flags, host-only absence,
the source SHA, the summary line, exactly one done: PASS, no SCRIPT (E) and no tagged FAIL.
Usage: python -I p1b-log-check-claude.py <console.log> <channel_fading_v1.json>"""
import hashlib
import json
import re
import sys

FROZEN = "cf5030f6a4ab5e03b5c02ef63aa4211a012b6a36a059ee2b11b519491a925f61"
HOST_ONLY = {"F27", "F28", "F29", "F30", "F57", "F70"}
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
if sum("[RICON_FADING] done: PASS" in l for l in lines) != 1:
    errors.append("done: PASS count")
if not any(f"[RICON_FADING] source={FROZEN}" in l for l in lines):
    errors.append("source line")
pat = re.compile(r"\[RICON_FADING\] (F\d\d) code=(\w+) real=(\S+) imag=(\S+) gain=(\S+) phases=(\d+) match=(\w+) immutable=(\w+)")
seen = {}
for l in lines:
    m = pat.search(l)
    if m:
        if m.group(1) in seen:
            errors.append(f"dup {m.group(1)}")
        seen[m.group(1)] = m.groups()[1:]
checked = 0
for cid, want in cases.items():
    if cid in HOST_ONLY:
        if cid in seen:
            errors.append(f"{cid} host-only printed")
        continue
    got = seen.get(cid)
    if got is None:
        errors.append(f"{cid} missing")
        continue
    code, real, imag, gain, nphases, match, immutable = got
    if code != want["code"]:
        errors.append(f"{cid} code {code} != {want['code']}")
    if (real == "null") != (want["real"] is None) or (imag == "null") != (want["imag"] is None):
        errors.append(f"{cid} coefficient presence")
    if want["real"] is not None and real != "null" and abs(float(real) - want["real"]) > 1e-8 * max(1.0, abs(want["real"])) + 5e-10:
        errors.append(f"{cid} real {real} != {want['real']}")
    if want["imag"] is not None and imag != "null" and abs(float(imag) - want["imag"]) > 1e-8 * max(1.0, abs(want["imag"])) + 5e-10:
        errors.append(f"{cid} imag {imag} != {want['imag']}")
    if (gain == "null") != (want["gainDb"] is None):
        errors.append(f"{cid} gain presence")
    elif want["gainDb"] is not None and abs(float(gain) - want["gainDb"]) > 1e-3:
        errors.append(f"{cid} gain {gain} != {want['gainDb']}")
    if int(nphases) != len(want["phasesQ"]):
        errors.append(f"{cid} phases count")
    if match != "1" or immutable != "1":
        errors.append(f"{cid} match/immutable {match}/{immutable}")
    checked += 1
if not any("[RICON_FADING] cases=65 match=65 immutable=65 controls=9 extras=3 hostOnly=6" in l for l in lines):
    errors.append("summary")
sub = [l for l in lines if "[RICON_FADING_FAULTS] subnormal" in l]
print("\n".join(errors) if errors else
      f"P1B_LOG_CHECK_PASS: {checked} native, host-only {len(HOST_ONLY)} absent, SCRIPT(E) 0; {sub[0].split('] ')[-1] if sub else 'no subnormal line'}")
sys.exit(1 if errors else 0)
