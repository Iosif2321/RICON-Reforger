"""P4 oracle tests (claude): source-anchored model checks, scenario map coverage and corpus integrity."""
import copy
import json
import math
from pathlib import Path
import unittest

import build_vegetation_vectors as b
import vegetation_oracle as v

VECTORS = Path(__file__).resolve().parent / "vectors" / "vegetation_v1.json"
TEMP = {"climate": "TEMPERATE"}
TROP = {"climate": "TROPICAL"}


class ModelTest(unittest.TestCase):
    def test_med_branches_meet_at_14_m(self):
        """ECAC eq. 6a/6b: the two branches agree at d = 14 m within 0.05 dB (6.28 vs 6.30 dB at 1 GHz)."""
        for f in (230.0, 1000.0, 9000.0):
            lo = 0.45 * (f / 1000) ** 0.284 * 14
            hi = 1.33 * (f / 1000) ** 0.284 * 14 ** 0.588
            self.assertLess(abs(lo - hi), 0.05 * (f / 1000) ** 0.284)
            self.assertEqual(v.m3(TEMP, f, 14.0), hi)

    def test_med_domain(self):
        self.assertIsNone(v.m3(TEMP, 229.9, 50))
        self.assertIsNone(v.m3(TEMP, 95000.1, 50))
        self.assertIsNone(v.m3(TEMP, 1000, 400.1))
        self.assertIsNone(v.m3(TROP, 1000, 50))
        self.assertIsNotNone(v.m3(TEMP, 95000.0, 400.0))

    def test_p833_eq2_matches_table1(self):
        """P.833-10 TABLE 1 (St. Petersburg mixed forest) against eq. (2) with A1 = 1.37, alpha = 0.42: within 15 %."""
        for f, am in ((105.9, 9.4), (466.475, 18.0), (949.0, 26.5), (1852.2, 29.0), (2117.5, 34.1)):
            self.assertAlmostEqual(1.0, 1.37 * f ** 0.42 / am, delta=0.15, msg=f)

    def test_p833_eq1_limits(self):
        prof = v.typed_profile(b.TEMPERATE)
        g = v.gamma_at(prof, 466.475)
        self.assertAlmostEqual(0.12, g, places=6)
        self.assertAlmostEqual(0.01 * g, v.m1(prof, 466.475, 0.01), delta=1e-6)
        am = prof["a1"] * 466.475 ** prof["alpha"]
        self.assertAlmostEqual(am, v.m1(prof, 466.475, 1.0e6), places=6)

    def test_jansky_bailey_table_frequency_is_exact(self):
        alpha, a, bb = v.jb_constants("V", 250.0)
        self.assertEqual((0.050, 0.759, 0.000443), (alpha, a, bb))
        dm = 500 / v.MILE_M
        lb = 36.57 + 20 * math.log10(250) - 20 * math.log10(a * math.exp(-1609 * alpha * dm) / dm + bb / dm ** 2)
        self.assertAlmostEqual(lb - (36.57 + 20 * math.log10(250) + 20 * math.log10(dm)),
                               v.m4(TROP, 250.0, 500.0, "V", 3, 3), places=9)

    def test_jansky_bailey_40logd_beyond_160_m(self):
        """ECAC §5.1: beyond about 160 m the basic loss grows as 40·lg d, i.e. the excess grows ~6 dB per doubling."""
        for pol in "VH":
            step = v.m4(TROP, 250.0, 1600.0, pol, 3, 3) - v.m4(TROP, 250.0, 800.0, pol, 3, 3)
            self.assertAlmostEqual(20 * math.log10(2), step, delta=1.0, msg=pol)

    def test_negative_jansky_bailey_excess_is_excluded(self):
        self.assertIsNone(v.m4(TROP, 100.0, 10.0, "H", 3, 3))
        self.assertIsNotNone(v.m4(TROP, 100.0, 10.0, "V", 3, 3))


class CorpusTest(unittest.TestCase):
    def setUp(self):
        self.cases = json.loads(VECTORS.read_bytes())["cases"]

    def test_vectors_are_built_from_case_list(self):
        self.assertEqual(VECTORS.read_bytes(), b.build())
        self.assertNotIn(b"\r\n", VECTORS.read_bytes())

    def test_every_code_scenario_and_model_is_covered(self):
        self.assertEqual(set(v.CODES), {c["expect"]["code"] for c in self.cases})
        rows = [r for c in self.cases for r in c["expect"].get("intervals", [])]
        self.assertEqual({"S1", "S2", "S3"}, {r["scenario"] for r in rows})
        self.assertEqual({"M1", "M2", "M3", "M4"}, {r["primaryModel"] for r in rows})
        self.assertTrue(any(c["id"] == "V18" and c["expect"]["code"] == "NO_MODEL_FOR_SCENARIO" for c in self.cases))

    def test_check_detects_corruption(self):
        self.assertEqual(0, v.check(VECTORS))
        for mutate in (lambda e: e["intervals"][0].update(primaryModel="M1"),
                       lambda e: e["intervals"][0].update(primaryLossDb=e["intervals"][0]["primaryLossDb"] + 1e-3),
                       lambda e: e["intervals"][0]["candidates"].pop()):
            corrupted = copy.deepcopy(self.cases)
            mutate(next(c for c in corrupted if c["id"] == "V03")["expect"])
            errors = []
            for c in corrupted:
                v.po.close(v.evaluate(c), c["expect"], c["id"], errors, 1e-9)
            self.assertTrue(errors)

    def test_input_is_not_mutated(self):
        inp = b.case(1000, 466.475, [b.veg(0, 50)])
        before = json.dumps(inp, sort_keys=True)
        v.vegetation_layer(inp)
        self.assertEqual(before, json.dumps(inp, sort_keys=True))


if __name__ == "__main__":
    unittest.main()
