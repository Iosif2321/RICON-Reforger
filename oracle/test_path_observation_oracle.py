"""P2 oracle tests (claude): P.527-6 figure checks, physical identities, observation rules and corpus integrity."""
import copy
import json
import math
from pathlib import Path
import unittest

import build_path_observation_vectors as b
import path_observation_oracle as o

VECTORS = Path(__file__).resolve().parent / "vectors" / "path_observation_v1.json"


class WaterModelTest(unittest.TestCase):
    def test_matches_p527_fig2(self):
        """Fig. 2 (T = 20 °C), read by eye from the local STD_COPY; tolerance 2 units of the 0–100 scale."""
        reads = {(0.1, 0): (80.0, 0.5), (1, 0): (80.0, 4.5), (1, 35): (71.5, 90.0), (17, 0): (42.0, 37.5),
                 (5.5, 35): (67.0, 32.0), (100, 0): (7.5, 12.5), (100, 35): (7.5, 14.5)}
        for (f, s), (real, imag) in reads.items():
            e1, e2 = o.water_permittivity(f, 20, s)
            self.assertAlmostEqual(real, e1, delta=2.0, msg=(f, s))
            self.assertAlmostEqual(imag, e2, delta=2.0, msg=(f, s))

    def test_matches_p527_fig1_penetration_depth(self):
        """Fig. 1 (20 °C, S 35 for sea water), log scale read by eye; tolerance 15 %."""
        for f, s, depth in ((0.01, 35, 0.072), (0.1, 35, 0.023), (1, 35, 0.010), (0.1, 0, 20.0), (1, 0, 0.2)):
            e = o.water_permittivity(f, 20, s)
            self.assertAlmostEqual(1.0, o.penetration_depth_m(f * 1000, *e) / depth, delta=0.15, msg=(f, s))

    def test_sea_conductivity_reference(self):
        self.assertAlmostEqual(4.79, o.sea_conductivity(20, 35), delta=0.01)
        self.assertAlmostEqual(1.0, o.sea_conductivity(15, 35) / o.sea_conductivity(15, 35), places=12)
        self.assertEqual(0.0, o.sea_conductivity(20, 0))

    def test_pure_water_is_special_case(self):
        """Eq. (14)-(21) with S = 0 reduce to eq. (5)-(7)."""
        es, e1, einf, f1, f2 = o.pure_water_parameters(20)
        f = 3.0
        real = (es - e1) / (1 + (f / f1) ** 2) + (e1 - einf) / (1 + (f / f2) ** 2) + einf
        imag = (f / f1) * (es - e1) / (1 + (f / f1) ** 2) + (f / f2) * (e1 - einf) / (1 + (f / f2) ** 2)
        self.assertEqual((real, imag), o.water_permittivity(f, 20, 0))

    def test_attenuation_equals_penetration_depth(self):
        for f_mhz, t, s in ((30, 10, 35), (150, 15, 0.2), (2000, 25, 5)):
            e = o.water_permittivity(f_mhz / 1000, t, s)
            alpha = o.attenuation_db_per_m(f_mhz, o.refractive_index(*e))
            self.assertAlmostEqual(alpha, o.DB_PER_NEPER / o.penetration_depth_m(f_mhz, *e), delta=1e-9 * alpha)

    def test_interface_identities(self):
        self.assertEqual(0.0, o.interface_loss_db(complex(1.0, 0.0)))
        n = o.refractive_index(*o.water_permittivity(0.15, 20, 35))
        self.assertLessEqual(n.imag, 0.0)
        self.assertGreater(o.interface_loss_db(n), 0.0)


class ObservationTest(unittest.TestCase):
    def test_touching_segments_of_one_entity_are_allowed(self):
        base = b.obs(b.air(0), b.air(100), segments=[b.seg("STRUCTURE", 10, 20, "h"), b.seg("STRUCTURE", 20, 30, "h")])
        self.assertEqual("VALID", o.check_observation(base))

    def test_segment_limit(self):
        many = b.obs(b.air(0), b.air(5000), segments=[b.seg("STRUCTURE", i, i + 0.5, f"s{i}") for i in range(1025)])
        self.assertEqual("INVALID_OBSERVATION", o.check_observation(many))
        many["segments"].pop()
        self.assertEqual("VALID", o.check_observation(many))

    def test_input_is_not_mutated(self):
        case = {"input": {"observation": b.MIXED, "profile": b.PROFILE}}
        before = json.dumps(case, sort_keys=True)
        o.evaluate(case)
        self.assertEqual(before, json.dumps(case, sort_keys=True))

    def test_water_order_coverage_before_no_water(self):
        observation = b.obs(b.air(0), b.air(10), coverage=dict(b.COMPLETE, WATER="PARTIAL"))
        self.assertEqual("WATER_COVERAGE_INCOMPLETE", o.water_layer(observation, b.PROFILE)["code"])

    def test_binary32_typing_precedes_checks(self):
        case = next(c for c in b.CASES if c[0] == "O24")
        self.assertEqual("INVALID_OBSERVATION", o.check_observation(case[2]))
        untyped = copy.deepcopy(case[2])
        untyped["segments"][0].update(startM=16777200, endM=16777217)
        self.assertEqual("VALID", o.check_observation(untyped))

    def test_vertical_exit_error_bound(self):
        """WL05: Snell bound 1/cos(asin(1/|n|)) - 1 is small for water and the cut-off is |n| = 3."""
        n = o.refractive_index(*o.water_permittivity(0.15, 15, 0.5))
        self.assertLess(1 / math.cos(math.asin(1 / abs(n))) - 1, 0.01)
        self.assertAlmostEqual(0.0607, 1 / math.cos(math.asin(1 / 3.0)) - 1, places=4)


class CorpusTest(unittest.TestCase):
    def test_vectors_are_built_from_case_list(self):
        self.assertEqual(VECTORS.read_bytes(), b.build())
        self.assertNotIn(b"\r\n", VECTORS.read_bytes())

    def test_check_passes_and_detects_corruption(self):
        self.assertEqual(0, o.check(VECTORS))
        d = json.loads(VECTORS.read_bytes())
        for mutate in (lambda c: c["expect"]["water"].update(lossDb=c["expect"]["water"]["lossDb"] + 1e-3),
                       lambda c: c["expect"]["water"].update(crossings=9),
                       lambda c: c["expect"].update(observationCode="INVALID_OBSERVATION")):
            corrupted = copy.deepcopy(d)
            target = next(c for c in corrupted["cases"] if c["id"] == "W02")
            mutate(target)
            errors = []
            for c in corrupted["cases"]:
                o.close(o.evaluate(c), c["expect"], c["id"], errors, 1e-9)
            self.assertTrue(errors)

    def test_every_water_code_is_covered(self):
        codes = {c["expect"]["water"]["code"] for c in json.loads(VECTORS.read_bytes())["cases"]}
        self.assertEqual(set(o.WATER_CODES), codes)


if __name__ == "__main__":
    unittest.main()
