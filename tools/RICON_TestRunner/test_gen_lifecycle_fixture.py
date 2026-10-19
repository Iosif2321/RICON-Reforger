#!/usr/bin/env python3
"""The committed Enforce lifecycle fixture must be exactly what the generator makes from the current vectors."""

from __future__ import annotations

import unittest
from pathlib import Path

import gen_lifecycle_fixture as gen

REPO = Path(__file__).resolve().parents[2]
VECTORS = REPO / "oracle" / "vectors" / "lifecycle_v1.json"
FIXTURE = REPO / "addons" / "RICON_CoreTests" / "Scripts" / "Game" / "RICON_Tests" / "RICON_LifecycleFixtures.c"


class GeneratedFixtureTest(unittest.TestCase):
    def test_fixture_matches_vectors(self) -> None:
        expected = gen.generate(VECTORS)
        self.assertEqual(FIXTURE.read_text(encoding="utf-8"), expected,
                         "regenerate: python -I tools/RICON_TestRunner/gen_lifecycle_fixture.py "
                         "oracle/vectors/lifecycle_v1.json <fixture>")

    def test_every_step_is_checked(self) -> None:
        text = FIXTURE.read_text(encoding="utf-8")
        traces = text.split("protected static void Control1")[0]
        self.assertEqual(traces.count("t.Check("), 208)
        self.assertEqual(traces.count("ADAPTER_MAPPED"), 3)


if __name__ == "__main__":
    unittest.main()
