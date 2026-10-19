#!/usr/bin/env python3
"""The committed Enforce L2 fixture must be exactly what the generator makes from the current vectors, and the
generator must refuse vector values that could inject code or have no typed form."""

from __future__ import annotations

import json
import re
import unittest
from pathlib import Path

import gen_l2_fixture as gen

REPO = Path(__file__).resolve().parents[2]
VECTORS = REPO / "oracle" / "vectors" / "l2_terrain_v1.json"
FIXTURE = REPO / "addons" / "RICON_CoreTests" / "Scripts" / "Game" / "RICON_Tests" / "RICON_L2Fixtures.c"


def first_ok(data: dict) -> dict:
    return next(c for c in data["cases"] if c["expect"]["code"] == "OK")


class GeneratedL2FixtureTest(unittest.TestCase):
    def test_fixture_matches_vectors(self) -> None:
        self.assertEqual(FIXTURE.read_text(encoding="utf-8"), gen.generate(VECTORS),
                         "regenerate: python -I tools/RICON_TestRunner/gen_l2_fixture.py "
                         "oracle/vectors/l2_terrain_v1.json <fixture>")

    def test_every_case_is_checked(self) -> None:
        text = FIXTURE.read_text(encoding="utf-8").split("protected static void Control1")[0]
        cases = json.loads(VECTORS.read_text(encoding="utf-8"))["cases"]
        self.assertEqual(text.count("t.CheckOk(") + text.count("t.CheckCode("), len(cases))
        self.assertEqual(text.count("ADAPTER_MAPPED"), 3)
        body = text.split("class RICON_L2Fixtures", 1)[1]  # the header carries a hex sha256
        self.assertIsNone(re.search(r"(?<![0-9A-Za-z_])[0-9]+(\.[0-9]+)?[eE][+-]?[0-9]+", body))  # plain decimals

    def _rejects(self, mutate) -> None:
        data = json.loads(VECTORS.read_text(encoding="utf-8"))
        mutate(data)
        with self.assertRaises(gen.VectorError):
            gen.generate_bytes(json.dumps(data).encode("utf-8"))

    def test_code_injection_is_refused(self) -> None:
        payload = 'OK); Print("x"); //'
        self._rejects(lambda d: d.update(source="docs/req/REQ-L2-terrain.md@c4f3b0a\nWorkbench.Exit(0);"))
        self._rejects(lambda d: d["cases"][0].update(id='E01"); Print("x'))
        self._rejects(lambda d: d["cases"][0]["expect"].update(code=payload))
        self._rejects(lambda d: first_ok(d)["expect"].update(sphCase=payload))
        self._rejects(lambda d: first_ok(d)["input"]["config"].update(polarization="VERTICAL); //"))
        self._rejects(lambda d: first_ok(d)["input"].update(htg="2); //"))
        self._rejects(lambda d: first_ok(d)["input"]["profile"].__setitem__(0, [0.0, "1); //"]))

    def test_untypeable_values_are_refused(self) -> None:
        self._rejects(lambda d: first_ok(d)["input"].update(htg=10 ** 400))
        self._rejects(lambda d: first_ok(d)["input"].update(htg=True))
        self._rejects(lambda d: first_ok(d)["input"]["config"].update(maxProfilePoints=1.5))
        self._rejects(lambda d: first_ok(d)["input"]["config"].update(extra=1))
        self._rejects(lambda d: first_ok(d)["input"].update(extra=1))
        self._rejects(lambda d: first_ok(d)["expect"].update(L=None))
        self._rejects(lambda d: d.update(enforceToleranceDb=1.0))


if __name__ == "__main__":
    unittest.main()
