#!/usr/bin/env python3
"""CT-VON-S-010: the VoN seam implementation file references no engine VoN/radio, Rpl, world/entity or file API
(REQ-von-seam §2.5 and §7). A static scan of the source; it is not runtime evidence."""

from __future__ import annotations

import re
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
SEAM = REPO / "addons" / "RICON_Core" / "Scripts" / "Game" / "RICON" / "Network" / "RICON_NetworkContracts.c"

# §7 names plus the families they stand for: engine VoN/radio classes, replication, game/world/entity lookup, files.
FORBIDDEN = re.compile(r"\b(VoNComponent|SCR_VoNComponent|BaseTransceiver|RadioTransceiver|RelayTransceiver"
                       r"|BaseRadioComponent|SCR_RadioComponent|Rpl\w*|GetGame|GetWorld|BaseWorld|IEntity|EntityID"
                       r"|FindEntityByID|FileIO|FileHandle|ScriptCallQueue|CallLater)\b")


def strip_comments(text: str) -> str:
    """Comments may name the forbidden APIs (they explain the boundary); code may not.

    A small lexer, not regexes: "//" or "/*" inside a string literal must not hide the code after it. String
    literals are kept, so a forbidden name used by string (reflection) is still reported. An unterminated block
    comment hides nothing silently: the rest is kept and scanned."""
    out, i, n = [], 0, len(text)
    while i < n:
        c = text[i]
        if c == '"':
            j = i + 1
            while j < n and text[j] != '"' and text[j] != "\n":
                j += 2 if text[j] == "\\" else 1
            out.append(text[i:j + 1])
            i = j + 1
        elif text.startswith("//", i):
            j = text.find("\n", i)
            i = n if j < 0 else j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            if j < 0:
                out.append(text[i:])
                break
            out.append(" ")
            i = j + 2
        else:
            out.append(c)
            i += 1
    return "".join(out)


def scan(text: str) -> list[str]:
    return sorted(set(FORBIDDEN.findall(strip_comments(text))))


class VoNSeamDependencyScanTest(unittest.TestCase):
    def test_seam_file_has_no_engine_dependency(self) -> None:
        self.assertEqual(scan(SEAM.read_text(encoding="utf-8")), [])

    def test_scan_catches_a_planted_dependency(self) -> None:
        text = SEAM.read_text(encoding="utf-8")
        for planted in ("\tBaseTransceiver m_Tx;\n", "\tvoid F() { GetGame().GetWorld(); }\n",
                        "\tvoid G() { FileIO.OpenFile(\"x\", FileMode.READ); }\n", "\t[RplRpc(RplChannel.Reliable)]\n"):
            self.assertNotEqual(scan(text + planted), [], planted)

    def test_comments_do_not_count(self) -> None:
        self.assertEqual(scan("// no BaseTransceiver here\n/* GetGame() */\nclass A {}\n"), [])

    def test_comment_markers_inside_strings_hide_nothing(self) -> None:
        self.assertEqual(scan('string s = "//"; GetGame();'), ["GetGame"])
        self.assertEqual(scan('string s = "/*"; GetGame(); string e = "*/";'), ["GetGame"])
        self.assertEqual(scan('string s = "a\\"//"; FileIO.OpenFile();'), ["FileIO"])
        self.assertEqual(scan('typename t = "BaseTransceiver".ToType();'), ["BaseTransceiver"])
        self.assertEqual(scan("/* unterminated GetWorld();"), ["GetWorld"])


if __name__ == "__main__":
    unittest.main()
