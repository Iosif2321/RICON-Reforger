#!/usr/bin/env python3
"""The committed Enforce route fixture must be exactly what the generator makes from the accepted corpus, and the
generator must refuse any other corpus bytes or values without a typed form."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

import gen_route_fixture as gen

REPO = Path(__file__).resolve().parents[2]
VECTORS = REPO / "oracle" / "vectors" / "route_v1.json"
FIXTURE = REPO / "addons" / "RICON_CoreTests" / "Scripts" / "Game" / "RICON_Tests" / "RICON_RouteFixtures.c"


class GeneratedRouteFixtureTest(unittest.TestCase):
    def test_fixture_matches_corpus(self) -> None:
        text, counts = gen.render(VECTORS.read_bytes())
        self.assertEqual(FIXTURE.read_text(encoding="utf-8"), text)
        self.assertEqual(counts, {"adapter": 2, "subclass": 3})

    def test_every_vector_is_checked_once(self) -> None:
        text = FIXTURE.read_text(encoding="utf-8")
        ids = [v["id"] for v in json.loads(VECTORS.read_text(encoding="utf-8"))["vectors"]]
        for vid in ids:
            self.assertEqual(text.count(f't.Check("{vid}",'), 1, vid)
        self.assertEqual(text.count("t.Check("), 48)
        self.assertEqual(text.count("// ADAPTER_MAPPED"), 2)
        self.assertEqual(text.count("route.InsertNullLeg();"), 3)

    def test_other_bytes_are_refused(self) -> None:
        raw = VECTORS.read_bytes()
        for spoiled in (raw + b"\n", raw.replace(b'"quality": 0.9', b'"quality": 0.8', 1), raw.replace(b"\n", b"\r\n")):
            with self.assertRaises(gen.VectorError):
                gen.render(spoiled)

    def test_case_validation(self) -> None:
        """The SHA gate already refuses changed corpora; the per-vector checks are tested directly."""
        good = json.loads(VECTORS.read_text(encoding="utf-8"))["vectors"][0]

        def spoil(path, value):
            v = json.loads(json.dumps(good))
            node = v
            for key in path[:-1]:
                node = node[key]
            node[path[-1]] = value
            return v

        for path, value in ((("id",), 'R-01"); //'), (("input", "context", "sessionEpoch"), 2 ** 31),
                            (("input", "context", "maxHops"), True), (("input", "context", "extra"), 1),
                            (("expected", "reason"), "OK); //"), (("expected", "quality"), "1"),
                            (("input", "route", "legs"), [{"fromEndpointId": 1, "toEndpointId": 2,
                                                           "budget": {"valid": 1, "intelligibility": 0.9,
                                                                      "state": "CLEAN"}}]),
                            (("input", "route", "legs"), [{"fromEndpointId": 1, "toEndpointId": 2,
                                                           "budget": {"valid": True, "intelligibility": 0.9,
                                                                      "state": "X); //"}}])):
            with self.assertRaises(gen.VectorError, msg=str(path)):
                gen.case(spoil(path, value), 0, {"adapter": 0, "subclass": 0})


if __name__ == "__main__":
    unittest.main()
