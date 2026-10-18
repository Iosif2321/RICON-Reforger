"""Проверки host-эталона LinkSnapshot (REQ-core §8), только stdlib."""

from __future__ import annotations

import copy
import json
import unittest
from pathlib import Path

import snapshot_oracle

VECTOR_PATH = Path(__file__).with_name("vectors") / "snapshot_v1.json"


class SnapshotOracleTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.document = json.loads(VECTOR_PATH.read_text(encoding="utf-8"))

    def test_all_vectors_pass(self) -> None:
        self.assertEqual([], snapshot_oracle.check(VECTOR_PATH))

    def test_kinds_are_covered(self) -> None:
        kinds = {vector["kind"] for vector in self.document["vectors"]}
        self.assertEqual({"compare", "receiver_trace", "allocator_trace"}, kinds)

    def test_every_decision_is_exercised(self) -> None:
        decisions = {v["expected"]["decision"] for v in self.document["vectors"] if v["kind"] == "compare"}
        self.assertEqual({"ACCEPT_FIRST", "ACCEPT_NEWER", "STALE", "INVALID_SNAPSHOT", "INVALID_IDENTITY"}, decisions)

    def test_wrong_expectation_is_detected(self) -> None:
        vector = copy.deepcopy(next(v for v in self.document["vectors"] if v["id"] == "CMP-03"))
        vector["expected"]["decision"] = "ACCEPT_NEWER"
        self.assertTrue(snapshot_oracle.run_vector(vector, self.document))

    def test_allocator_never_issues_int_max(self) -> None:
        allocator = snapshot_oracle.Allocator(self.document["intMax"], 7, {"10": self.document["intMax"] - 2})
        issued = [allocator.next(10)["linkSeq"] for _ in range(3)]
        self.assertNotIn(self.document["intMax"], issued)
        self.assertEqual([self.document["intMax"] - 1, 1, 2], issued)


if __name__ == "__main__":
    unittest.main()
