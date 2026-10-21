"""Тесты эталона службы пар (REQ-pair-service v0.2). Запуск:
python -m unittest discover -s oracle -p "test_*.py"
"""

from __future__ import annotations

import copy
import json
import random
import unittest
from pathlib import Path

import pair_service_oracle as ps

VECTORS = Path(__file__).resolve().parent / "vectors" / "pair_service_v1.json"


def by_id():
    return {c["id"]: c for c in ps.build_cases()}


class PairServiceTest(unittest.TestCase):
    def test_vector_file_is_current(self):
        self.assertEqual(ps.check(json.loads(VECTORS.read_text(encoding="utf-8"))), [])

    def test_check_detects_tampering(self):
        data = json.loads(VECTORS.read_text(encoding="utf-8"))
        bad = copy.deepcopy(data)
        bad["cases"][0]["expect"]["pairs"][0]["sinrDb"] += 1e-6
        self.assertTrue(ps.check(bad))
        bad = copy.deepcopy(data)
        bad["cases"][0]["expect"]["s2Calls"] += 1
        self.assertTrue(ps.check(bad))
        bad = copy.deepcopy(data)
        bad["enforceTolerance"]["sinrDb"] = 0.5
        self.assertTrue(ps.check(bad))

    def test_every_call_and_pair_code_is_covered(self):
        cases = ps.build_cases()
        self.assertEqual({c["expect"]["callCode"] for c in cases}, set(ps.CALL_CODES))
        self.assertEqual({p["pairCode"] for c in cases for p in c["expect"]["pairs"]}, set(ps.PAIR_CODES))
        device_codes = {p["deviceCode"] for c in cases for p in c["expect"]["pairs"] if p["pairCode"] == "DEVICE_QUERY"}
        self.assertEqual(device_codes, {"INVALID_OBSERVATION", "INVALID_PROFILE", "INVALID_CONTEXT",
                                        "PROFILE_MISMATCH", "POWER_OFF", "TUNING_MISMATCH", "CHANNEL_MISMATCH",
                                        "KEY_MISMATCH"})

    def test_refusal_is_empty_and_calls_nothing(self):
        for c in ps.build_cases():
            e = c["expect"]
            if e["callCode"] != "OK":
                self.assertEqual((e["pairs"], e["s2Calls"], e["evaluatorCalls"]), ([], 0, 0), c["id"])

    def test_ok_has_full_matrix_in_key_order(self):
        for c in ps.build_cases():
            e, inp = c["expect"], c["input"]
            if e["callCode"] != "OK":
                continue
            self.assertEqual(len(e["pairs"]), len(inp["bindings"]) * len(inp["listeners"]), c["id"])
            keys = [(p["leaseId"], p["rxDeviceId"], p["rxTransceiverIndex"], p["rxPlayerId"]) for p in e["pairs"]]
            self.assertEqual(keys, sorted(keys), c["id"])

    def test_call_counters_match_pair_codes(self):
        for c in ps.build_cases():
            pairs = c["expect"]["pairs"]
            self.assertEqual(c["expect"]["s2Calls"], sum(p["hasDeviceCode"] for p in pairs), c["id"])
            self.assertEqual(c["expect"]["evaluatorCalls"], sum(p["hasBudget"] for p in pairs), c["id"])
            for p in pairs:
                self.assertEqual(p["hasBudget"], p["pairCode"] == "EVALUATED")
                self.assertEqual(p["hasDeviceCode"], p["pairCode"] in ("DEVICE_QUERY", "EVALUATED"))

    def test_any_permutation_gives_same_output(self):
        base = ps.baseline()
        expected = ps.evaluate(copy.deepcopy(base))
        rng = random.Random(2026)
        for _ in range(20):
            data = copy.deepcopy(base)
            for key, _ in ps.ARRAYS:
                rng.shuffle(data[key])
            self.assertEqual(ps.evaluate(data), expected)

    def test_inputs_are_not_mutated(self):
        for c in ps.build_cases():
            before = copy.deepcopy(c["input"])
            ps.evaluate(c["input"])
            self.assertEqual(c["input"], before, c["id"])

    def test_ttl_edges_and_int_max(self):
        cases = by_id()
        self.assertEqual(cases["B03"]["expect"]["pairs"][0]["pairCode"], "BINDING_INACTIVE")
        self.assertEqual(cases["B04"]["expect"]["pairs"][0]["pairCode"], "EVALUATED")
        self.assertEqual(cases["B05"]["expect"]["pairs"][0]["pairCode"], "BINDING_INACTIVE")
        self.assertEqual(cases["B05"]["expect"]["pairs"][4]["pairCode"], "EVALUATED")
        self.assertEqual(cases["B06"]["expect"]["pairs"][0]["pairCode"], "EVALUATED")
        self.assertEqual(cases["C30"]["expect"]["callCode"], "OK")
        self.assertIn(ps.INT_MAX, [p["leaseId"] for p in cases["C30"]["expect"]["pairs"]])

    def test_self_pair_precedes_listener_checks(self):
        pairs = by_id()["B27"]["expect"]["pairs"]
        self.assertEqual(pairs[0]["pairCode"], "RX_PLAYER_INVALID")
        self.assertEqual(pairs[3]["pairCode"], "SELF_PAIR")

    def test_two_transceivers_of_one_radio(self):
        pairs = by_id()["S09"]["expect"]["pairs"]
        radio3 = [p for p in pairs if p["leaseId"] == 101 and p["rxDeviceId"] == 3]
        self.assertEqual([p["rxTransceiverIndex"] for p in radio3], [0, 1])
        self.assertTrue(all(p["pairCode"] == "EVALUATED" for p in radio3))

    def test_asymmetry_follows_profiles(self):
        base = by_id()["P01"]["expect"]["pairs"]
        changed = by_id()["E03"]["expect"]["pairs"]
        # L2 (рация 2) теперь говорит 37 дБм и 6 дБи вместо −40 дБм и 2 дБи; приём на рации 2 от L1 растёт на 4 дБ.
        self.assertAlmostEqual(changed[0]["sinrDb"] - base[0]["sinrDb"], 4.0, places=4)
        self.assertAlmostEqual(changed[4]["sinrDb"] - base[4]["sinrDb"], 81.0, places=4)

    def test_invalid_budget_stays_evaluated(self):
        for cid, reason in (("E01", "UNKNOWN_MODEL"), ("E02", "UNKNOWN_NOISE_ENVIRONMENT")):
            evaluated = [p for p in by_id()[cid]["expect"]["pairs"] if p["pairCode"] == "EVALUATED"]
            self.assertEqual(len(evaluated), 5)
            self.assertTrue(all(not p["valid"] and p["reason"] == reason for p in evaluated))

    def test_states_are_far_from_thresholds(self):
        """Допуск Enforce 0.05 дБ не должен переключать состояние: запас по Q ≥ 0.01."""
        for c in ps.build_cases():
            for p in c["expect"]["pairs"]:
                if p["hasBudget"] and p["valid"] and 0 < p["intelligibility"] < 1:
                    self.assertGreater(min(abs(p["intelligibility"] - 0.8), abs(p["intelligibility"] - 0.5)), 0.01)

    def test_json_is_strict(self):
        text = VECTORS.read_text(encoding="utf-8")
        self.assertNotIn("Infinity", text)
        self.assertNotIn(": NaN", text)


if __name__ == "__main__":
    unittest.main()
