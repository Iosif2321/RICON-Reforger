#!/usr/bin/env python3
"""The committed Enforce VoN seam fixture must be exactly what the generator makes from the current vectors, and
the generator must refuse vector values that could inject code or have no typed form."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

import gen_von_seam_fixture as gen

REPO = Path(__file__).resolve().parents[2]
VECTORS = REPO / "oracle" / "vectors" / "von_seam_v1.json"
FIXTURE = REPO / "addons" / "RICON_CoreTests" / "Scripts" / "Game" / "RICON_Tests" / "RICON_VoNSeamFixtures.c"


def first_apply(data: dict) -> dict:
    return next(step for case in data["cases"] for step in case["calls"]
                if step["op"] == "apply" and step["snapshot"] is not None and step["binding"] is not None)


class GeneratedVoNSeamFixtureTest(unittest.TestCase):
    def test_fixture_matches_vectors(self) -> None:
        self.assertEqual(FIXTURE.read_text(encoding="utf-8"), gen.generate(VECTORS),
                         "regenerate: python -I tools/RICON_TestRunner/gen_von_seam_fixture.py "
                         "oracle/vectors/von_seam_v1.json <fixture>")

    def test_every_call_is_checked(self) -> None:
        text = FIXTURE.read_text(encoding="utf-8").split("protected static void Control1")[0]
        calls = sum(len(c["calls"]) for c in json.loads(VECTORS.read_text(encoding="utf-8"))["cases"])
        self.assertEqual(text.count("t.Apply(") + text.count("t.Expire("), calls)
        self.assertEqual(text.count("ADAPTER_MAPPED"), 1)

    def _rejects(self, mutate) -> None:
        data = json.loads(VECTORS.read_text(encoding="utf-8"))
        mutate(data)
        with self.assertRaises(gen.VectorError):
            gen.generate_bytes(json.dumps(data).encode("utf-8"))

    def test_code_injection_is_refused(self) -> None:
        payload = 'OK); Print("x"); //'
        self._rejects(lambda d: d.update(source="docs/req/REQ-von-seam.md@949f7ca\nWorkbench.Exit(0);"))
        self._rejects(lambda d: d["cases"][0].update(id='A01"); Print("x'))
        self._rejects(lambda d: d["cases"][0]["calls"][0].update(expect=payload))
        self._rejects(lambda d: first_apply(d).update(upstreamDecision=payload))
        self._rejects(lambda d: first_apply(d)["snapshot"].update(state="CLEAN); //"))
        self._rejects(lambda d: first_apply(d)["snapshot"].update(quality="1); //"))
        self._rejects(lambda d: first_apply(d)["snapshot"].update(quality=1e-30))
        self._rejects(lambda d: first_apply(d)["binding"].update(linkId="10); //"))

    def test_untypeable_records_are_refused(self) -> None:
        self._rejects(lambda d: first_apply(d)["snapshot"].update(extra=1))
        self._rejects(lambda d: first_apply(d)["binding"].update(extra=1))
        self._rejects(lambda d: first_apply(d)["binding"].update(linkId=True))
        self._rejects(lambda d: first_apply(d)["binding"].update(linkId=2 ** 31))
        self._rejects(lambda d: first_apply(d).update(upstreamDecision=[]))
        self._rejects(lambda d: d["cases"][0].update(extra=1))


if __name__ == "__main__":
    unittest.main()
