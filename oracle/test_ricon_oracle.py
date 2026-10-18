"""Проверки host-oracle только средствами стандартной библиотеки."""

from __future__ import annotations

import copy
import unittest
from pathlib import Path

import ricon_oracle


VECTOR_PATH = Path(__file__).with_name("vectors") / "link_eval_v1.json"


class OracleVectorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.document = ricon_oracle.load_vector_document(VECTOR_PATH)

    def test_all_committed_vectors(self) -> None:
        failures = ricon_oracle.check_document(self.document)
        self.assertEqual([], failures, "\n".join(failures))

    def test_vector_schema_is_versioned_and_unique(self) -> None:
        self.assertEqual("ricon.oracle.link_eval", self.document["schema"])
        self.assertEqual(1, self.document["version"])
        self.assertEqual(2, self.document["revision"])
        identifiers = [vector["id"] for vector in self.document["vectors"]]
        self.assertEqual(57, len(identifiers))
        self.assertEqual(len(identifiers), len(set(identifiers)))

    def test_linear_fixture_is_explicitly_test_only(self) -> None:
        fixture = self.document["fixtures"]["linear_test"]
        self.assertEqual("TEST_ONLY", fixture["status"])
        budget_vectors = [
            vector for vector in self.document["vectors"] if vector["kind"] == "budget"
        ]
        self.assertTrue(budget_vectors)
        self.assertTrue(
            all("test_only_intelligibility" in vector["tags"] for vector in budget_vectors)
        )

    def test_invalid_vectors_return_specific_finite_budgets(self) -> None:
        invalid_vectors = [
            vector for vector in self.document["vectors"] if vector["kind"] == "invalid"
        ]
        self.assertEqual(22, len(invalid_vectors))
        for vector in invalid_vectors:
            with self.subTest(vector=vector["id"]):
                actual = ricon_oracle.evaluate_vector(vector, self.document)
                self.assertFalse(actual["valid"])
                self.assertEqual(vector["expected"]["reason"], actual["reason"])
                self.assertEqual("LOST", actual["state"])
                self.assertEqual(0.0, actual["intelligibility"])
                self.assertTrue(ricon_oracle.all_numeric_values_finite(actual))

    def test_evaluation_is_deterministic(self) -> None:
        vector = next(
            vector for vector in self.document["vectors"] if vector["id"] == "BUD-02"
        )
        first = ricon_oracle.evaluate_vector(vector, self.document)
        second = ricon_oracle.evaluate_vector(vector, self.document)
        self.assertEqual(first, second)

    def test_schema_errors_fail_before_evaluation(self) -> None:
        malformed = copy.deepcopy(self.document)
        malformed["tolerances"]["db"] = 0
        failures = ricon_oracle.check_document(malformed)
        self.assertEqual(["schema: tolerance db недопустим"], failures)

    def test_tolerance_mapping_distinguishes_metres_from_dbm(self) -> None:
        tolerances = self.document["tolerances"]
        self.assertEqual(tolerances["m"], ricon_oracle.tolerance_for("distanceM", tolerances))
        self.assertEqual(tolerances["m"], ricon_oracle.tolerance_for("effectiveDistanceM", tolerances))
        self.assertEqual(tolerances["db"], ricon_oracle.tolerance_for("rxPowerDbm", tolerances))
        self.assertEqual(tolerances["db"], ricon_oracle.tolerance_for("sinrDb", tolerances))
        self.assertEqual(
            tolerances["ratio"], ricon_oracle.tolerance_for("intelligibility", tolerances)
        )

    def test_lost_is_valid_physics_not_invalid_input(self) -> None:
        vector = next(
            vector for vector in self.document["vectors"] if vector["id"] == "BUD-04"
        )
        actual = ricon_oracle.evaluate_vector(vector, self.document)
        self.assertTrue(actual["valid"])
        self.assertEqual("OK", actual["reason"])
        self.assertEqual("LOST", actual["state"])

    def test_boolean_presence_flags_are_not_truthy_strings(self) -> None:
        vector = copy.deepcopy(
            next(vector for vector in self.document["vectors"] if vector["id"] == "BUD-01")
        )
        vector["input"]["hasRxSensitivity"] = "false"
        actual = ricon_oracle.evaluate_vector(vector, self.document)
        self.assertFalse(actual["valid"])
        self.assertEqual("CATALOG_VIOLATION", actual["reason"])


if __name__ == "__main__":
    unittest.main()
