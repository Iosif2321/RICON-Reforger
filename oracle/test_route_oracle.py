"""Проверки host-эталона маршрута (REQ-core §9), только stdlib."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

import route_oracle

VECTOR_PATH = Path(__file__).with_name("vectors") / "route_v1.json"
REASONS = {"OK", "INVALID_CONTEXT", "EMPTY_ROUTE", "TOO_MANY_HOPS", "NULL_LEG", "INVALID_ENDPOINT",
           "INVALID_BUDGET", "WRONG_START", "DISCONTINUITY", "CYCLE", "WRONG_DESTINATION"}


class RouteOracleTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.document = json.loads(VECTOR_PATH.read_text(encoding="utf-8"))

    def test_all_vectors_pass(self) -> None:
        self.assertEqual([], route_oracle.check(VECTOR_PATH))

    def test_every_reason_is_exercised(self) -> None:
        reasons = {vector["expected"]["reason"] for vector in self.document["vectors"]}
        self.assertEqual(REASONS, reasons)

    def test_invalid_route_is_lost_with_zero_quality(self) -> None:
        for vector in self.document["vectors"]:
            if vector["expected"]["reason"] != "OK":
                with self.subTest(vector=vector["id"]):
                    self.assertEqual("LOST", vector["expected"]["state"])
                    self.assertEqual(0.0, vector["expected"]["quality"])

    def test_order_cases_have_two_violations(self) -> None:
        order = [vector for vector in self.document["vectors"] if "order" in vector["tags"]]
        self.assertGreaterEqual(len(order), 8)


if __name__ == "__main__":
    unittest.main()
