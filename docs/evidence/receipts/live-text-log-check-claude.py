#!/usr/bin/env python3
"""claude: external check of the RICON_LiveTextProbePlugin log (codex #4319): the %XX forms printed by the native
RICON_LiveText.Escape are decoded here and compared byte by byte with the original strings of the probe, so the
round trip is checked rather than assumed. Also: one summary line checks=14 passed=14, one "done: PASS", no mismatch,
no SCRIPT (E). The native side cannot see its own output; this script is the only lossless check.
Usage: python -I live-text-log-check-claude.py <console.log>"""
import hashlib
import json
import re
import sys

# Original strings of RICON_LiveTextProbePlugin.RunCommandline as UTF-8 bytes (source file is UTF-8).
CASES = {
    "plain": b"abc_09.#-",
    "empty": b"",
    "space": b"a b",
    "percent": b"%",
    "comma": b",",
    "equals": b"=",
    "controls": b"\t\n",
    "tilde_del_bell": b"~\x7f\x07",
    "menu_like": b"32.2 #AR-VON_FrequencyUnits_MHz",
    "cyrillic": "МГц".encode("utf-8"),
}
CASE = re.compile(r"\[RICON_LIVET\] case=(\w+) length=(-?\d+) wide=(-?\d+) escaped=(\S*)$")


def decode(text):
    """%XX -> one byte, %uXXXX -> UTF-8 of that code point; any other '%' or a forbidden plain byte is an error."""
    out, i = bytearray(), 0
    while i < len(text):
        c = text[i]
        if c == "%":
            if text[i + 1:i + 2] == "u":
                out += chr(int(text[i + 2:i + 6], 16)).encode("utf-8")
                i += 6
            else:
                out.append(int(text[i + 1:i + 3], 16))
                i += 3
        else:
            code = ord(c)
            if not (33 <= code <= 126) or c in "%,=":
                raise ValueError(f"unescaped byte {code} in {text!r}")
            out.append(code)
            i += 1
    return bytes(out)


def main(path):
    raw = open(path, "rb").read()
    lines = raw.decode("utf-8", errors="replace").splitlines()
    errors, rows = [], {}
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
            back = decode(row["escaped"])
        except (ValueError, IndexError) as exc:
            errors.append(f"case {name}: {exc}")
            continue
        row["roundTrip"] = back == original
        row["nativeLengthIsBytes"] = row["length"] == len(original)
        if not row["roundTrip"]:
            errors.append(f"case {name}: decoded {back!r} != {original!r}")
        if row["wide"] != 0:
            errors.append(f"case {name}: wide={row['wide']}")
    if set(rows) - set(CASES):
        errors.append(f"unexpected cases {sorted(set(rows) - set(CASES))}")
    text = "\n".join(lines)
    if text.count("[RICON_LIVET] checks=14 passed=14") != 1:
        errors.append("summary checks=14 passed=14 not found exactly once")
    if text.count("[RICON_LIVET] done: PASS") != 1 or "[RICON_LIVET] done: FAIL" in text:
        errors.append("done marker")
    if "[RICON_LIVET] mismatch" in text:
        errors.append("native mismatch present")
    print(json.dumps({"logSha256": hashlib.sha256(raw).hexdigest(), "errors": errors, "cases": rows},
                     ensure_ascii=False, indent=1))
    print("LIVE_TEXT_LOG_CHECK_" + ("PASS" if not errors else "FAIL"))
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1]))
