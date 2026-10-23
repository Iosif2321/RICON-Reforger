"""P4.2 oracle tests (claude): P.2108-1 §3.1 anchors, monotonicity, FM 24-18 polarization rule and corpus integrity."""
import copy
import json
import math
from pathlib import Path
import unittest

import build_vegetation_overcanopy_vectors as bo
import build_vegetation_vectors as b
import vegetation_oracle as v
import vegetation_overcanopy_oracle as o

VECTORS = Path(__file__).resolve().parent / "vectors" / "vegetation_overcanopy_v1.json"


class P2108Test(unittest.TestCase):
    def test_j_anchor_points(self):
        """P.2108-1 note to eq. (1): J(-0.78) ~ 0 and J = 0 below; J(0) - 6.03 ~ +0.002852 dB (rounded constants), so
        Ah does not reach exactly 0 as h -> R from below; the h >= R branch is exactly 0 (codex #3637)."""
        self.assertAlmostEqual(0.0, o.j_nu(-0.78), delta=0.05)
        self.assertEqual(0.0, o.j_nu(-0.9))
        self.assertAlmostEqual(6.03, o.j_nu(0.0), delta=0.005)
        self.assertAlmostEqual(0.0, o.terminal_correction(60.0, 14.9999, 15.0)["ahDb"], delta=0.01)
        self.assertAlmostEqual(0.002852208563606, o.j_nu(0.0) - 6.03, places=12)
        self.assertEqual({"hdifM": 0.0, "thetaDeg": 0.0, "nu": 0.0, "jDb": 0.0, "ahDb": 0.0},
                         o.terminal_correction(60.0, 15.0, 15.0))

    def test_worked_example_60_mhz(self):
        """f = 0.06 GHz, h = 2 m, R = 15 m, ws = 27 m, by hand from eq. (2c)-(2g)."""
        theta = math.degrees(math.atan(13 / 27))
        nu = 0.342 * math.sqrt(0.06) * math.sqrt(13 * theta)
        j = 6.9 + 20 * math.log10(math.sqrt((nu - 0.1) ** 2 + 1) + nu - 0.1)
        rec = o.terminal_correction(60.0, 2.0, 15.0)
        self.assertAlmostEqual(j - 6.03, rec["ahDb"], places=12)
        self.assertAlmostEqual(10.91, rec["ahDb"], delta=0.01)

    def test_monotonic_in_frequency_and_depth_below_canopy(self):
        prev = -1.0
        for f in (30, 60, 88, 150, 466, 1000, 3000):
            ah = o.terminal_correction(float(f), 2.0, 15.0)["ahDb"]
            self.assertGreater(ah, prev)
            prev = ah
        prev = -1.0
        for h in (14.0, 10.0, 6.0, 2.0, 0.5):
            ah = o.terminal_correction(60.0, h, 15.0)["ahDb"]
            self.assertGreater(ah, prev)
            prev = ah

    def test_ah_is_nonnegative_on_the_domain(self):
        for f in (30.0, 300.0, 3000.0):
            for h in (0.1, 5.0, 14.999, 15.0, 40.0):
                self.assertGreaterEqual(o.terminal_correction(f, h, 15.0)["ahDb"], 0.0)


class FieldManualTest(unittest.TestCase):
    """FM 24-18 (1987) §3 and §7-5: near dense forests horizontal polarization suffers lower losses than vertical.
    A qualitative military rule, checked as a property of the P4 v1 models and catalog data below 1 GHz."""

    def test_gamma_tables_v_not_below_h(self):
        for tables in (b.FIG2, b.RICE71):
            prof = v.typed_profile(b.profile("UNSPECIFIED", tables, "STANDARD_FIGURE"))
            for f in (30.0, 60.0, 100.0, 150.0, 466.0, 999.0):
                self.assertGreaterEqual(v.gamma_at(prof, f, "V")[0], v.gamma_at(prof, f, "H")[0], f)

    def test_jansky_bailey_v_above_h(self):
        trop = v.typed_profile(b.TROPICAL)
        for f in (50.0, 100.0, 150.0, 250.0, 400.0):
            for d in (100.0, 500.0, 1600.0):
                self.assertGreater(v.m4(trop, f, d, "V", 3, 3), v.m4(trop, f, d, "H", 3, 3), (f, d))


class CorpusTest(unittest.TestCase):
    def setUp(self):
        self.doc = json.loads(VECTORS.read_bytes())
        self.cases = {c["id"]: c for c in self.doc["cases"]}

    def test_vectors_are_built_from_case_list(self):
        self.assertEqual(VECTORS.read_bytes(), bo.build())
        self.assertNotIn(b"\r\n", VECTORS.read_bytes())
        self.assertEqual(0, o.check(VECTORS))

    def test_every_code_is_covered(self):
        self.assertEqual(set(o.CODES), {c["expect"]["code"] for c in self.cases.values()})
        self.assertEqual(["O28"], self.doc["hostOnly"])

    def test_polarization_independent_and_two_terminals_add(self):
        self.assertEqual(self.cases["O01"]["expect"], self.cases["O24"]["expect"])
        e = self.cases["O02"]["expect"]
        self.assertEqual(["TX", "RX"], [t["end"] for t in e["terminals"]])
        self.assertAlmostEqual(sum(t["ahDb"] for t in e["terminals"]), e["totalAhDb"], places=12)
        self.assertFalse(e["integrationClosed"])
        self.assertEqual("BASE_AT_CANOPY_HEIGHT", e["budgetMode"])

    def test_check_detects_corruption(self):
        for cid, mutate in (("O01", lambda e: e.update(totalAhDb=e["totalAhDb"] + 1e-3)),
                            ("O02", lambda e: e["terminals"].pop()),
                            ("O03", lambda e: e["terminals"][0].update(canopyProvenance="STANDARD_DEFAULT")),
                            ("O26", lambda e: e.update(code="OK"))):
            cases = copy.deepcopy(self.doc["cases"])
            mutate(next(c for c in cases if c["id"] == cid)["expect"])
            errors = []
            for c in cases:
                o.po.close(o.evaluate(c), c["expect"], c["id"], errors, 1e-9)
            self.assertTrue(errors, cid)

    def test_input_is_not_mutated(self):
        inp = bo.case(2000, 60.0, [bo.veg(0, 2000)])
        before = json.dumps(inp, sort_keys=True)
        o.over_canopy(inp)
        self.assertEqual(before, json.dumps(inp, sort_keys=True))


if __name__ == "__main__":
    unittest.main()
