#!/usr/bin/env python3
"""claude: external check of the RICON_LiveTextProbePlugin log (codex #4319, #4339): the %XX forms printed by the native
RICON_LiveText.Escape are decoded here with the strict shared codec (live_text_codec_claude.py, loaded by explicit path)
and compared byte by byte with the original strings of the probe, so the round trip is checked rather than assumed.
Also: the codec self-test, one summary line checks=15 passed=15, one "done: PASS", no mismatch, no SCRIPT (E). The native
side cannot see its own output; this script is the only lossless check.
Usage: python -I live-text-log-check-claude.py <console.log>"""
import hashlib
import importlib.util
import json
import pathlib
import re
import sys

_spec = importlib.util.spec_from_file_location(
    "live_text_codec_claude", pathlib.Path(__file__).resolve().parent / "live_text_codec_claude.py")
codec = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(codec)

# Original strings of RICON_LiveTextProbePlugin.RunCommandline (source file is UTF-8).
CASES = {
    "plain": "abc_09.#-",
    "empty": "",
    "space": "a b",
    "percent": "%",
    "comma": ",",
    "equals": "=",
    "controls": "\t\n",
    "tilde_del_bell": "~\x7f\x07",
    "menu_like": "32.2 #AR-VON_FrequencyUnits_MHz",
    "cyrillic": "МГц",
    "emoji_4byte": "\U0001F600",
}
CASE = re.compile(r"\[RICON_LIVET\] case=(\w+) length=(-?\d+) wide=(-?\d+) escaped=(\S*)$")


def main(path):
    raw = open(path, "rb").read()
    lines = raw.decode("utf-8", errors="replace").splitlines()
    errors, rows = [], {}
    codec_cases = codec.self_test()
    for form, ok in codec_cases:
        if not ok:
            errors.append(f"codec self-test failed on {form!r}")
    if [l for l in lines if re.search(r"SCRIPT\s*\(E\)", l)]:
        errors.append("SCRIPT (E) present")
    for l in lines:
        m = CASE.search(l)
        if m:
            if m.group(1) in rows:
                errors.append(f"case {m.group(1)} repeated")
            rows[m.group(1)] = {"length": int(m.group(2)), "wide": int(m.group(3)), "escaped": m.group(4)}
    for name, original in CASES.items():
        row = rows.get(name)
        if row is None:
            errors.append(f"case {name} missing")
            continue
        try:
            back = codec.unescape(row["escaped"])
        except ValueError as exc:
            errors.append(f"case {name}: {exc}")
            continue
        row["roundTrip"] = back == original
        row["nativeLengthIsBytes"] = row["length"] == len(original.encode("utf-8"))
        if not row["roundTrip"]:
            errors.append(f"case {name}: decoded {back!r} != {original!r}")
        if not row["nativeLengthIsBytes"]:
            errors.append(f"case {name}: native length {row['length']} != {len(original.encode('utf-8'))} bytes")
        if row["wide"] != 0:
            errors.append(f"case {name}: wide={row['wide']}")
    if set(rows) - set(CASES):
        errors.append(f"unexpected cases {sorted(set(rows) - set(CASES))}")
    text = "\n".join(lines)
    if text.count("[RICON_LIVET] checks=15 passed=15") != 1:
        errors.append("summary checks=15 passed=15 not found exactly once")
    if text.count("[RICON_LIVET] done: PASS") != 1 or "[RICON_LIVET] done: FAIL" in text:
        errors.append("done marker")
    if "[RICON_LIVET] mismatch" in text:
        errors.append("native mismatch present")
    print(json.dumps({"logSha256": hashlib.sha256(raw).hexdigest(), "errors": errors,
                      "codecSelfTest": f"{sum(ok for _, ok in codec_cases)}/{len(codec_cases)}", "cases": rows},
                     ensure_ascii=False, indent=1))
    print("LIVE_TEXT_LOG_CHECK_" + ("PASS" if not errors else "FAIL"))
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1]))
