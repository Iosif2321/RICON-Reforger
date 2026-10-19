#!/usr/bin/env python3
"""Self-test of the lifecycle oracle: vectors pass, format is checked and any tampering is detected."""

from __future__ import annotations

import copy
import json
import unittest
from pathlib import Path

import lifecycle_oracle as lc

VECTORS = Path(__file__).parent / "vectors" / "lifecycle_v1.json"


def load() -> dict:
    return json.loads(VECTORS.read_text(encoding="utf-8"))


def failures(data: dict) -> list[str]:
    return [failure for trace in data["traces"] for failure in lc.run_trace(trace)]


class LifecycleOracleTest(unittest.TestCase):
    def test_vectors_pass(self) -> None:
        data = load()
        self.assertEqual(lc.check_vector_file(data), [])
        self.assertEqual(failures(data), [])

    def test_every_acceptance_test_is_covered(self) -> None:
        covered = {ct for trace in load()["traces"] for ct in trace["ct"]}
        self.assertEqual(covered, {f"CT-LC-{n:03d}" for n in range(1, 11)})

    def test_source_names_the_req_revision(self) -> None:
        self.assertRegex(load()["source"], r"^docs/req/REQ-lifecycle\.md@[0-9a-f]{7,}$")

    def test_changed_result_code_is_detected(self) -> None:
        data = load()
        data["traces"][0]["steps"][2]["expect"]["resultCode"] = "OK"
        self.assertEqual(len(failures(data)), 1)

    def test_swapped_effect_order_is_detected(self) -> None:
        data = load()
        for trace in data["traces"]:
            for step in trace["steps"]:
                effects = step["expect"].get("effects", [])
                if len(effects) >= 2:
                    effects[0], effects[1] = effects[1], effects[0]
                    self.assertEqual(len(failures({"traces": [trace]})), 1, trace["id"])
                    return
        self.fail("no step with two effects")

    def test_missing_effect_is_detected(self) -> None:
        data = load()
        target = next(trace for trace in data["traces"] if trace["id"].startswith("T04"))
        disconnect = next(step for step in target["steps"] if step["event"]["type"] == "DISCONNECTED")
        disconnect["expect"]["effects"] = [e for e in disconnect["expect"]["effects"]
                                           if e["type"] != "VON_EXPIRY_INTENDED"]
        self.assertEqual(len(failures({"traces": [target]})), 1)

    def test_format_errors_are_reported(self) -> None:
        data = load()
        broken = copy.deepcopy(data)
        broken["traces"][0]["steps"][0]["expect"]["resultCode"] = "MAYBE"
        broken["traces"][1]["id"] = broken["traces"][0]["id"]
        problems = lc.check_vector_file(broken)
        self.assertTrue(any("resultCode" in p for p in problems))
        self.assertTrue(any("trace id" in p for p in problems))
        self.assertEqual(lc.check_vector_file({"schema": "x", "version": 1}), ["schema/version"])

    def test_effect_and_code_names_are_closed(self) -> None:
        for trace in load()["traces"]:
            for step in trace["steps"]:
                expect = step["expect"]
                self.assertIn(expect["resultCode"], lc.RESULT_CODES)
                if "cachedResultCode" in expect:
                    self.assertEqual(expect["resultCode"], "DUPLICATE")
                    self.assertIn(expect["cachedResultCode"], lc.RESULT_CODES)
                for effect in expect.get("effects", []):
                    self.assertIn(effect["type"], lc.EFFECT_TYPES)


if __name__ == "__main__":
    unittest.main()
