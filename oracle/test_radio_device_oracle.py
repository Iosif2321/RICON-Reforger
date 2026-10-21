"""Проверки границ, выбора разных RF-профилей и сохранности входов S2."""
import copy
import json
import math
from pathlib import Path
import unittest

import radio_device_oracle as rd
import ricon_oracle as ro
import world_profile_oracle as wp

VECTORS = Path(__file__).resolve().parent / "vectors/radio_device_v1.json"


def changed(path, value):
    inputs = rd.baseline()
    target = inputs
    for key in path[:-1]:
        target = target[key]
    target[path[-1]] = value
    return inputs


class RadioDeviceTest(unittest.TestCase):
    def test_corpus_and_all_codes(self):
        data = json.loads(VECTORS.read_text(encoding="utf-8"))
        self.assertEqual(rd.check(data), [])
        self.assertEqual({case["expect"]["code"] for case in data["cases"]}, set(rd.CODES))
        self.assertEqual(len({case["id"] for case in data["cases"]}), len(data["cases"]))

    def test_checker_rejects_changes_in_input_output_header_and_types(self):
        data = rd.document()
        for path, value in ((["revision"], data["revision"] + 1), (["cases", 0, "expect", "query", "txPowerDbm"], 30.0),
                            (["cases", 0, "input", "txObservation", "deviceId"], True)):
            bad = copy.deepcopy(data)
            target = bad
            for key in path[:-1]:
                target = target[key]
            target[path[-1]] = value
            self.assertTrue(rd.check(bad))
        self.assertTrue(rd.check({}))

    def test_selected_fields_and_reverse(self):
        self.assertEqual(rd.evaluate(rd.baseline())["query"], rd.expected_baseline())
        reverse = rd.build_cases()[1]
        actual = rd.evaluate(reverse["input"])
        self.assertEqual(actual, reverse["expect"])
        self.assertNotEqual(actual["query"]["txPowerDbm"], rd.expected_baseline()["txPowerDbm"])
        self.assertEqual(actual["query"]["bandwidthHz"], 25000.0)

    def test_every_active_numeric_field_rejects_nonfinite(self):
        for side in ("txProfile", "rxProfile"):
            for field in rd.FLOATS[:6] + ("requiredSinrDb",):
                for special in rd.SPECIAL:
                    with self.subTest(side=side, field=field, special=special):
                        self.assertEqual(rd.evaluate(changed([side, field], special))["code"], "INVALID_PROFILE")
            for special in rd.SPECIAL:
                data = changed([side, "rxSensitivityDbm"], special)
                data[side].update(hasRxSensitivity=True, hasRequiredSinr=False)
                self.assertEqual(rd.evaluate(data)["code"], "INVALID_PROFILE")
        for side in ("txObservation", "rxObservation"):
            for axis in range(3):
                for special in rd.SPECIAL:
                    self.assertEqual(rd.evaluate(changed([side, "antennaPos", axis], special))["code"],
                                     "INVALID_OBSERVATION")

    def test_inactive_fields_are_typed_but_not_required_finite(self):
        for special in rd.SPECIAL:
            data = changed(["rxProfile", "rxSensitivityDbm"], special)
            data["context"]["interferenceDbm"] = special
            output = rd.evaluate(data)
            self.assertEqual(output["code"], "READY")
            self.assertEqual(output["query"]["rxSensitivityDbm"], 0.0)
            self.assertEqual(output["query"]["interferenceDbm"], 0.0)
        self.assertEqual(rd.evaluate(changed(["rxProfile", "rxSensitivityDbm"], "missing"))["code"], "INVALID_PROFILE")

    def test_opaque_keys_and_ids_are_not_decoded_as_special_floats(self):
        data = rd.baseline()
        data["txObservation"]["networkKey"] = data["rxObservation"]["networkKey"] = "NaN"
        data["context"]["bandId"] = "NaN"
        data["txProfile"]["profileId"] = "NaN"
        output = rd.evaluate(data)
        self.assertEqual(output["code"], "READY")
        self.assertEqual(output["query"]["bandId"], "NaN")
        self.assertNotIn("networkKey", output["query"])

    def test_float32_comparison_overflow_and_underflow(self):
        self.assertEqual(rd.evaluate(changed(["txProfile", "frequencyMHz"], 50.00000001))["code"], "READY")
        self.assertEqual(rd.evaluate(changed(["txProfile", "frequencyMHz"], 50.001))["code"], "CHANNEL_MISMATCH")
        self.assertEqual(rd.evaluate(changed(["rxProfile", "txPowerDbm"], 1e40))["code"], "INVALID_PROFILE")
        self.assertEqual(rd.evaluate(changed(["txProfile", "frequencyMHz"], 1e-50))["code"], "INVALID_PROFILE")
        self.assertTrue(math.isinf(rd.f32(10**1000)))
        self.assertEqual(rd.f32(-1e-50), 0.0)
        self.assertLess(math.copysign(1.0, rd.f32(-1e-50)), 0)

    def test_bool_and_string_are_not_numeric(self):
        for field in ("deviceId", "transceiverIndex", "tuningValue"):
            self.assertEqual(rd.evaluate(changed(["txObservation", field], True))["code"], "INVALID_OBSERVATION")
        for field in rd.FLOATS + ("expectedTuningValue",):
            self.assertEqual(rd.evaluate(changed(["txProfile", field], True))["code"], "INVALID_PROFILE")
        self.assertEqual(rd.evaluate(changed(["context", "hasInterference"], 0))["code"], "INVALID_CONTEXT")
        self.assertEqual(rd.evaluate(changed(["rxProfile", "frequencyMHz"], "50"))["code"], "INVALID_PROFILE")

    def test_required_fields_have_no_defaults(self):
        for root in rd.INPUTS:
            data = rd.baseline()
            data[root] = None
            self.assertEqual(rd.evaluate(data), {"code": "NULL_INPUT", "query": None})
        for root, code in (("txObservation", "INVALID_OBSERVATION"), ("txProfile", "INVALID_PROFILE"),
                           ("context", "INVALID_CONTEXT")):
            for field in rd.baseline()[root]:
                data = rd.baseline()
                del data[root][field]
                self.assertEqual(rd.evaluate(data)["code"], code, (root, field))

    def test_int32_and_identity_boundaries(self):
        for value in (0, -1, rd.INT_MAX, 2**31, -(2**31)-1):
            self.assertEqual(rd.evaluate(changed(["txObservation", "deviceId"], value))["code"], "INVALID_OBSERVATION")
        data = rd.baseline()
        data["txObservation"]["tuningValue"] = data["txProfile"]["expectedTuningValue"] = -(2**31)
        self.assertEqual(rd.evaluate(data)["code"], "READY")
        self.assertEqual(rd.evaluate(changed(["txObservation", "tuningValue"], 2**31))["code"], "INVALID_OBSERVATION")

    def test_inputs_and_repeated_outputs_do_not_alias(self):
        data = rd.baseline()
        before = copy.deepcopy(data)
        first = rd.evaluate(data)
        second = rd.evaluate(data)
        first["query"]["txPos"][0] = 99.0
        first["query"]["txPowerDbm"] = -99.0
        self.assertEqual(data, before)
        self.assertEqual(second["query"], rd.expected_baseline())

    def test_same_endpoint_is_left_for_s3_pair_selection(self):
        data = rd.baseline()
        data["rxObservation"] = copy.deepcopy(data["txObservation"])
        data["rxProfile"] = copy.deepcopy(data["txProfile"])
        before = copy.deepcopy(data)
        result = rd.evaluate(data)
        self.assertEqual(result["code"], "READY")
        self.assertEqual(result["query"]["txPos"], result["query"]["rxPos"])
        self.assertEqual(result["query"]["bandwidthHz"], 25000.0)
        self.assertEqual(result["query"]["rxNoiseFigureDb"], 12.0)
        self.assertEqual(result["query"]["requiredSinrDb"], 14.0)
        self.assertEqual(data, before)

    def test_all_corpus_outputs_are_finite_and_inputs_unchanged(self):
        for case in rd.build_cases():
            before = copy.deepcopy(case["input"])
            actual = rd.evaluate(case["input"])
            self.assertEqual(case["input"], before)
            self.assertEqual(actual, case["expect"], case["id"])
            self.assertTrue(ro.all_numeric_values_finite(actual))

    def test_ready_passes_positions_to_existing_s1_without_height_fixup(self):
        base_s1 = wp.document()["cases"][0]["input"]
        for y, reason in ((101.5, "OK"), (100.0, "TERRAIN_PROFILE_UNAVAILABLE")):
            query = rd.evaluate(changed(["txObservation", "antennaPos", 1], y))["query"]
            inputs = {**base_s1, "txPos": query["txPos"], "rxPos": query["rxPos"],
                      "frequencyMHz": query["frequencyMHz"]}
            losses = wp.model_losses(inputs)
            self.assertEqual(losses["modelReason"], reason)
            self.assertEqual(query["txPos"][1], y)
            if reason == "OK":
                total_loss = sum(losses["layers"].values())
                self.assertAlmostEqual(total_loss, 108.1797261261363, places=8)
                rx_power = query["txPowerDbm"] + query["txGainDbi"] + query["rxGainDbi"] - total_loss
                noise = ro.thermal_noise(query["bandwidthHz"]) + query["rxNoiseFigureDb"]
                self.assertAlmostEqual(rx_power, -66.1797261261363, places=8)
                self.assertAlmostEqual(rx_power - noise, 58.82636093801125, places=8)


if __name__ == "__main__":
    unittest.main()
