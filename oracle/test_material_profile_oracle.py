"""P3.2b: независимая формула, границы, неизменность и существующий slab."""
import copy
import contextlib
import io
import json
import math
from pathlib import Path
import random
import tempfile
import unittest
from unittest.mock import patch

import build_material_profile_vectors as builder
import material_profile_oracle as o
import material_slab_oracle as slab


class MaterialProfileTests(unittest.TestCase):
    def test_corpus_and_builder_check_do_not_write(self):
        self.assertEqual(0, o.check(builder.TARGET))
        before = builder.TARGET.read_bytes()
        with patch.object(Path, "write_text") as write:
            builder.build(check=True)
            original_read = Path.read_text
            def changed(path, *args, **kwargs):
                return "{}\n" if path == builder.TARGET else original_read(path, *args, **kwargs)
            with patch.object(Path, "read_text", changed):
                with self.assertRaises(SystemExit):
                    builder.build(check=True)
            write.assert_not_called()
        self.assertEqual(before, builder.TARGET.read_bytes())

    def test_direct_power_for_all_profiles(self):
        rng = random.Random(20404)
        for entry in o.load_catalog().values():
            p = entry["profile"]
            for _ in range(10):
                frequency = o.f32(rng.uniform(p["minMHz"], p["maxMHz"]))
                result = o.evaluate({"frequencyMHz": frequency, "profile": p})
                self.assertEqual("OK", result["code"])
                for field, coefficient, exponent in (("epsilonReal", "a", "b"), ("conductivitySm", "c", "d")):
                    expected = o.f32(o.f32(p[coefficient]) * (frequency / 1000) ** o.f32(p[exponent]))
                    self.assertEqual(expected, result["material"][field])

    def test_catalog_domains_and_boundary_selection(self):
        catalog = o.load_catalog()
        for frequency in (20, 30, 76):
            accepted = {name for name, entry in catalog.items()
                        if o.evaluate({"frequencyMHz": frequency, "profile": entry["profile"]})["code"] == "OK"}
            self.assertEqual({"p2040.air.01", "p2040.wood.01"}, accepted)
        self.assertEqual(1e7, catalog["p2040.metal.01"]["profile"]["c"])
        for frequency, expected in ((220000, "02a"), (300000, "02b"), (400000, "02c"), (450000, None)):
            selected = [suffix for suffix in ("02a", "02b", "02c")
                        if o.evaluate({"frequencyMHz": frequency,
                            "profile": catalog["p2040.floorboard." + suffix]["profile"]})["code"] == "OK"]
            self.assertEqual([] if expected is None else [expected], selected)
        for suffix in ("01", "02"):
            self.assertEqual("OUT_OF_MODEL_DOMAIN", o.evaluate({"frequencyMHz": 105000,
                "profile": catalog["p2040.concrete." + suffix]["profile"]})["code"])

    def test_profile_to_existing_slab_and_wrong_frequency(self):
        for name, frequency in (("wood.01", 30), ("concrete.01", 1000), ("metal.01", 1000)):
            profile = o.load_catalog()["p2040." + name]["profile"]
            result = o.evaluate({"frequencyMHz": frequency, "profile": profile})
            value = slab.baseline()
            value.update(frequencyMHz=frequency, material=result["material"], thicknessM=.01)
            computed = slab.evaluate(value)
            self.assertEqual("OK", computed["code"])
            self.assertGreater(computed["lossDb"], 0)
            self.assertEqual("SOURCE_DOMAIN", computed["provenance"])
            value["frequencyMHz"] += 1
            self.assertEqual("OUT_OF_MODEL_DOMAIN", slab.evaluate(value)["code"])

    def test_immutability_and_detached_results(self):
        for entry in o.load_catalog().values():
            value = {"frequencyMHz": entry["profile"]["minMHz"], "profile": entry["profile"]}
            before = copy.deepcopy(value)
            first, second = o.evaluate(value), o.evaluate(value)
            self.assertEqual(before, value)
            first["material"]["provenance"] = "TEST_ONLY"
            self.assertEqual("SOURCE_DOMAIN", second["material"]["provenance"])
            self.assertEqual(before, value)

    def test_numeric_limits_and_no_silent_underflow(self):
        value = builder.baseline()
        value["frequencyMHz"] = 1
        value["profile"].update(c=1e-30, d=100)
        self.assertEqual({"code": "NUMERIC_FAILURE"}, o.evaluate(value))
        value["profile"].update(c=0, d=3e38)
        self.assertEqual(0, o.evaluate(value)["material"]["conductivitySm"])
        value["frequencyMHz"] = 110000
        value["profile"].update(a=1e-38, b=19)
        self.assertEqual("OK", o.evaluate(value)["code"])
        self.assertGreater(o.evaluate(value)["material"]["epsilonReal"], 1)

    def test_bad_types_and_nonfinite_never_become_profiles(self):
        for bad in (True, None, "1", math.nan, math.inf, -math.inf, 10 ** 1000):
            value = builder.baseline()
            value["frequencyMHz"] = bad
            self.assertEqual({"code": "INVALID_INPUT"}, o.evaluate(value))
            for field in ("a", "b", "c", "d", "minMHz", "maxMHz"):
                value = builder.baseline()
                value["profile"][field] = bad
                self.assertEqual({"code": "INVALID_PROFILE"}, o.evaluate(value))

    def test_native_power_regressions_without_arbitrary_log_cutoffs(self):
        for coefficient, exponent, frequency, expected in ((9e-36, 10, 100, o.f32(1e-45)),
                                                          (1e38, 15, 1, o.f32(o.f32(1e38) * .001 ** 15))):
            value = builder.baseline()
            value["frequencyMHz"] = frequency
            value["profile"].update(c=coefficient, d=exponent)
            result = o.evaluate(value)
            self.assertEqual("OK", result["code"])
            self.assertEqual(expected, result["material"]["conductivitySm"])
        value = builder.baseline()
        value["profile"].update(c=3.4e38, d=1)
        self.assertEqual({"code": "NUMERIC_DOMAIN_LIMIT"}, o.evaluate(value))

    def test_catalog_rejects_corruption_even_in_unselected_row(self):
        original = o.read_json(o.CATALOG)
        with tempfile.TemporaryDirectory() as tmp:
            target = Path(tmp) / "catalog.json"
            for change in (lambda d: d["profiles"]["p2040.ground.wet"]["profile"].update(a=-1),
                           lambda d: d["profiles"]["p2040.ground.wet"].update(sourceRow=1),
                           lambda d: d.update(version=True)):
                data = copy.deepcopy(original)
                change(data)
                target.write_text(json.dumps(data), encoding="utf-8")
                with self.assertRaises(ValueError):
                    o.load_catalog(target)
            target.write_text('{"a": 1, "a": 2}', encoding="utf-8")
            with self.assertRaises(ValueError):
                o.read_json(target)

    def test_corpus_sha_and_expected_type_mutations_are_detected(self):
        original = o.read_json(builder.TARGET)
        with tempfile.TemporaryDirectory() as tmp:
            target = Path(tmp) / "corpus.json"
            data = copy.deepcopy(original)
            data["catalogLfSha256"] = "0" * 64
            target.write_text(json.dumps(data), encoding="utf-8")
            with self.assertRaises(ValueError):
                o.check(target)
            data = copy.deepcopy(original)
            data["cases"][0]["expect"]["material"]["epsilonReal"] = True
            target.write_text(json.dumps(data), encoding="utf-8")
            with contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(1, o.check(target))


if __name__ == "__main__":
    unittest.main()
