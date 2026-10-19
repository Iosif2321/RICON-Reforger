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
    """Comments may name the forbidden APIs (they explain the boundary); code may not."""
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    return re.sub(r"//[^\n]*", " ", text)


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


if __name__ == "__main__":
    unittest.main()
