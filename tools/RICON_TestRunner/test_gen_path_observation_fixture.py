"""Проверка привязки native fixture P2 к замороженному корпусу."""
import hashlib
import json
import struct
import unittest
from unittest.mock import patch

import gen_path_observation_fixture as generator


class PathObservationFixtureTest(unittest.TestCase):
    def setUp(self):
        self.raw = generator.SOURCE.read_bytes()

    def test_committed_fixture_matches_frozen_source(self):
        self.assertEqual(generator.build(self.raw).encode("utf-8"), generator.TARGET.read_bytes())

    def test_changed_expectation_is_rejected(self):
        corpus = json.loads(self.raw)
        corpus["cases"][0]["expect"]["water"]["lossDb"] = 999
        with self.assertRaisesRegex(ValueError, "SHA"):
            generator.build(json.dumps(corpus).encode())

    def test_line_endings_are_part_of_freeze(self):
        self.assertNotIn(b"\r", self.raw)
        with self.assertRaisesRegex(ValueError, "SHA"):
            generator.build(self.raw.replace(b"\n", b"\r\n"))

    def test_duplicate_case_is_rejected_even_with_matching_digest(self):
        corpus = json.loads(self.raw)
        corpus["cases"][1] = corpus["cases"][0]
        raw = json.dumps(corpus).encode()
        with patch.object(generator, "SOURCE_SHA", hashlib.sha256(raw).hexdigest()):
            with self.assertRaisesRegex(ValueError, "состав"):
                generator.build(raw)

    def test_host_only_cases_are_explicit(self):
        source = generator.build(self.raw)
        self.assertIn("CASES = 47;", source)
        self.assertIn("HOST_ONLY = 4;", source)
        for case_id in ("O16", "O19", "W17", "W21"):
            self.assertIn(f"// {case_id}: host-only", source)
            self.assertNotIn(f't.CheckCase("{case_id}"', source)

    def test_nonzero_expected_loss_is_copied_as_a_literal(self):
        case = next(x for x in json.loads(self.raw)["cases"] if x["id"] == "W06")
        literal = generator.value_literal(case["expect"]["water"]["lossDb"], "float")
        self.assertIn(f"expected.m_fLossDb = {literal};", "\n".join(generator.case_lines(0, case)))

    def test_small_float_literals_preserve_binary32(self):
        for value in (1e-17, 1e-18, 1e-35, -1e-35, 1.234567890123e-35, 2**-149):
            with self.subTest(value=value):
                text = generator.value_literal(value, "float")
                self.assertNotIn("e", text.lower())
                self.assertEqual(struct.pack("!f", float(text)), struct.pack("!f", value))


if __name__ == "__main__":
    unittest.main()
