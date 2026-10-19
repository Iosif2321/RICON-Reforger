#!/usr/bin/env python3
"""The committed Enforce lifecycle fixture must be exactly what the generator makes from the current vectors."""

from __future__ import annotations

import json
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

    def _rejects(self, mutate) -> None:
        data = json.loads(VECTORS.read_text(encoding="utf-8"))
        mutate(data)
        with self.assertRaises(gen.VectorError):
            gen.generate_bytes(json.dumps(data).encode("utf-8"))

    def test_code_injection_is_refused(self) -> None:
        payload = 'OK); Print("x"); //'
        self._rejects(lambda d: d["traces"][0]["steps"][1]["expect"].update(resultCode=payload))
        self._rejects(lambda d: d["traces"][0].update(id='T01"); Print("x'))
        self._rejects(lambda d: d["traces"][0]["steps"][1]["event"].update(playerId="1); Print(2"))
        self._rejects(lambda d: d["traces"][0]["steps"][0]["event"]["catalog"][0].update(kind="WORLD; //"))
        self._rejects(lambda d: d["traces"][0]["steps"][1]["expect"]["effects"][0].update(toState=payload))
        self._rejects(lambda d: d["traces"][0]["steps"][1]["event"].update(extra=1))
        self._rejects(lambda d: d["traces"][0]["steps"][1]["event"].update(playerId=True))
        self._rejects(lambda d: d["traces"][0]["steps"][1]["event"].update(playerId=2 ** 31))
        self._rejects(lambda d: d.update(source="docs/req/REQ-lifecycle.md@9e6ce55\nWorkbench.Exit(0);"))
        self._rejects(lambda d: d.update(source="x"))

    def test_untypeable_catalog_is_refused(self) -> None:
        self._rejects(lambda d: d["traces"][0]["steps"][0]["event"]["catalog"][0]["allowed"].update({"5": []}))
        self._rejects(lambda d: d["traces"][0]["steps"][0]["event"]["catalog"][0]["allowed"].update({"05": [1]}))
        self._rejects(lambda d: d["traces"][0]["steps"][0]["event"]["catalog"][0]["allowed"]
                      .update({"9999999999": [1]}))

    def test_every_step_is_checked(self) -> None:
        text = FIXTURE.read_text(encoding="utf-8")
        traces = text.split("protected static void Control1")[0]
        steps = sum(len(t["steps"]) for t in json.loads(VECTORS.read_text(encoding="utf-8"))["traces"])
        self.assertEqual(traces.count("t.Check("), steps)
        self.assertEqual(traces.count("ADAPTER_MAPPED"), 3)


if __name__ == "__main__":
    unittest.main()
