"""claude: strict decoder of the RICON_LiveText %XX form (codex #4339), shared by live-text-log-check-claude.py and
live-bench-log-check-claude.py. Both load this file by its explicit path (importlib), so python -I stays usable.
Rules, mirroring RICON_LiveText.Escape: a plain byte is printable ASCII 33..126 except '%', ',' and '='; every other
byte is '%' followed by exactly two UPPERCASE hex digits; the bytes must be valid UTF-8. '%ZZ' (the native refusal
marker for a code outside 0..255) and any other violation raise ValueError: nothing is guessed or replaced."""
HEX = "0123456789ABCDEF"


def unescape(text):
    out, i = bytearray(), 0
    while i < len(text):
        c = text[i]
        if c == "%":
            pair = text[i + 1:i + 3]
            if len(pair) != 2 or pair[0] not in HEX or pair[1] not in HEX:
                raise ValueError(f"bad escape {text[i:i + 3]!r} at {i}")
            out.append(int(pair, 16))
            i += 3
        else:
            code = ord(c)
            if not (33 <= code <= 126) or c in ",=":
                raise ValueError(f"unescaped character {code} at {i}")
            out.append(code)
            i += 1
    return bytes(out).decode("utf-8", errors="strict")


def self_test():
    """Codec cases (codex #4339): accepted canonical forms and rejected malformed ones."""
    ok = [("", ""), ("a%20b", "a b"), ("%25%2C%3D", "%,="), ("%D0%9C", "М"), ("%F0%9F%98%80", "\U0001F600")]
    bad = ["%A", "%a0", "%G0", "%ZZ", "a b", "a,b", "a=b", "%FF", "%D0", "%C0%AF", "\t"]
    results = []
    for form, expected in ok:
        try:
            results.append((form, unescape(form) == expected))
        except ValueError:
            results.append((form, False))
    for form in bad:
        try:
            unescape(form)
            results.append((form, False))
        except (ValueError, UnicodeDecodeError):
            results.append((form, True))
    return results
