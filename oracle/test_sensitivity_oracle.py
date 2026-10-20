#!/usr/bin/env python3
"""Tests of the sensitivity oracle: P.527-6 soil model against the standard's own figures (read by eye) and
physical constants, step sweep limits, and the committed vector file against the link scenario matrix."""

from __future__ import annotations

import json
import math
import unittest

import link_physics_oracle as lpo
import sensitivity_oracle as so

DATA = json.loads(so.VECTORS.read_text(encoding="utf-8"))
MATRIX = {s["id"]: s for s in json.loads(so.MATRIX.read_text(encoding="utf-8"))["scenarios"]}


class SoilModelTest(unittest.TestCase):
    def test_matches_p527_figures(self) -> None:
        """Fig. 14-16 of P.527-6 read at 0.1, 1 and 10 GHz; tolerance 2 % of the figure's full scale."""
        for soil, mv, f, re_read, im_read in so.FIGURE_READINGS:
            re, im = so.soil_permittivity(f, 23.0, soil, mv)
            tol = so.READING_TOLERANCE[mv]
            self.assertLessEqual(abs(re - re_read), tol, (soil, mv, f, re))
            self.assertLessEqual(abs(im - im_read), tol, (soil, mv, f, im))

    def test_pure_water_static_permittivity(self) -> None:
        """Static permittivity of pure water at 25 °C is 78.30, measured below 100 kHz, better than 0.1 %
        (Malmberg and Maryott 1956, J. Res. NBS 56(1), DOI 10.6028/jres.056.001; codex #2685), outside P.527."""
        self.assertAlmostEqual(so.pure_water(1.0, 25.0)[0], 78.30, delta=0.08)

    def test_refuses_complex_continuation(self) -> None:
        """Silty clay at low moisture: εfw' < 0 and the fractional power in (59) would turn complex (codex #2666)."""
        for t in (0.0, 23.0, 40.0):
            for f in (0.03, 0.05, 0.088, 0.1, 0.4):
                with self.assertRaisesRegex(ValueError, "SOIL_OUTSIDE_REAL_MODEL"):
                    so.soil_permittivity(f, t, "silty_clay", 0.1)
        with self.assertRaisesRegex(ValueError, "SOIL_OUTSIDE_REAL_MODEL"):
            so.ground_p527(100.0, "silty_clay", 0.1)

    def test_real_or_refused_on_grid(self) -> None:
        for soil in so.SOILS:
            for f in (0.03, 0.05, 0.088, 0.1, 0.4, 1.0):
                for mv in (0.05, 0.1, 0.3, 0.5):
                    for t in (0.0, 23.0, 40.0):
                        try:
                            re, im = so.soil_permittivity(f, t, soil, mv)
                        except ValueError as e:
                            self.assertEqual(str(e), "SOIL_OUTSIDE_REAL_MODEL")
                            continue
                        self.assertIs(type(re), float)
                        self.assertIs(type(im), float)
                        self.assertTrue(math.isfinite(re) and math.isfinite(im) and re > 1.0 and im >= 0.0)

    def test_upper_scope_bound(self) -> None:
        """P.527-6 covers frequencies up to 1 000 GHz: 1000 is accepted, above it refused (codex #2716)."""
        so.soil_permittivity(1000.0, 23.0, "loam", 0.3)
        for f in (1000.001, 2000.0):
            with self.assertRaisesRegex(ValueError, "SOIL_ABOVE_SCOPE"):
                so.soil_permittivity(f, 23.0, "loam", 0.3)
        with self.assertRaisesRegex(ValueError, "SOIL_ABOVE_SCOPE"):
            so.ground_p527(2_000_000.0, "loam", 0.3)

    def test_rejects_bad_inputs(self) -> None:
        for args in ((0.1, 23.0, "peat", 0.3), (0.0, 23.0, "loam", 0.3), (0.1, 23.0, "loam", 0.0),
                     (0.1, 23.0, "loam", 1.0), (math.nan, 23.0, "loam", 0.3), (0.1, 23.0, "loam", True)):
            with self.assertRaises(ValueError, msg=args):
                so.soil_permittivity(*args)

    def test_conductivity_factor_is_two_pi_eps0(self) -> None:
        """P.527-6 eq (3a) factor 0.05563 = 2π·ε0 per GHz (ε0 = 8.8541878128e-12 F/m)."""
        self.assertAlmostEqual(2 * math.pi * 8.8541878128e-12 * 1e9, 0.05563, delta=5e-6)

    def test_wetter_soil_has_higher_permittivity_and_conductivity(self) -> None:
        for f in (30.0, 50.0, 88.0, 400.0):
            dry, wet = so.ground_p527(f, "silty_loam", 0.07), so.ground_p527(f, "silty_loam", 0.30)
            self.assertGreater(wet["permittivity"], dry["permittivity"])
            self.assertGreater(wet["conductivitySm"], dry["conductivitySm"])

    def test_domain_marks_extrapolation_below_figures(self) -> None:
        for f, dom in ((30.0, "BELOW_FIGURE_RANGE"), (88.0, "BELOW_FIGURE_RANGE"), (100.0, "IN_FIGURE_RANGE"),
                       (400.0, "IN_FIGURE_RANGE")):
            self.assertEqual(so.ground_p527(f, "loam", 0.2)["domain"], dom)

    def test_grounds_pass_scenario_validation(self) -> None:
        sc = MATRIX["S-30MHz-1W-h1.5-flat"]["input"]
        for g in DATA["groundsByFrequency"]:
            ground = so.as_scenario_ground(dict(g, polarization="VERTICAL"))
            self.assertIsNone(lpo.validate_scenario(dict(sc, ground=ground)), g)


class StepSweepTest(unittest.TestCase):
    def test_reference_step_has_zero_deviation(self) -> None:
        g = dict(so.BASELINE_GROUND, polarization="VERTICAL")
        r = so.step_sensitivity(10.0, 2.0, 0.1, 60.0, 50.0, 3.0, g, [0.005], 0.005, phases=4)
        self.assertEqual(r["rows"][0]["worstDeviationDb"], 0.0)

    def test_empty_sweep_is_refused(self) -> None:
        """phases = 0 or -1 used to return worstDeviationDb = 0 without any computation (codex #2673)."""
        g = dict(so.BASELINE_GROUND, polarization="VERTICAL")
        for phases in (0, -1, True, 2.5):
            with self.assertRaises(ValueError, msg=phases):
                so.step_sensitivity(5.0, 2.0, 0.5, 60.0, 88.0, 1.5, g, [0.1], 0.002, phases=phases)
        with self.assertRaises(ValueError):
            so.step_sensitivity(5.0, 2.0, 0.5, 60.0, 88.0, 1.5, g, [], 0.002, phases=8)

    def test_reference_converged(self) -> None:
        for b in DATA["stepLoss"]["blocks"]:
            self.assertLess(b["convergenceDeviationDb"], 0.25, b)

    def test_fine_step_on_wide_hill_is_close(self) -> None:
        for b in DATA["stepLoss"]["blocks"]:
            if b["halfWidthKm"] == 0.5:
                self.assertLess(b["rows"][0]["worstDeviationDb"], 0.1, b)

    def test_coarse_step_loses_a_narrow_hill(self) -> None:
        """Step 1 km over a 60 m wide hill: at the worst phase no profile point is near the vertex, so the
        deviation approaches the whole hill effect."""
        for b in DATA["stepLoss"]["blocks"]:
            if b["halfWidthKm"] == 0.03:
                self.assertGreater(b["rows"][-1]["worstDeviationDb"], 0.8 * abs(b["hillMinusFlatDb"]), b)


class VectorFileTest(unittest.TestCase):
    def test_header(self) -> None:
        self.assertEqual(DATA["schema"], "ricon.oracle.sensitivity")
        self.assertEqual(len(DATA["groundSweep"]), len(MATRIX))

    def test_baseline_reproduces_matrix(self) -> None:
        """The TEST_ONLY baseline ground with vertical polarization is the matrix itself: outer limits must equal
        the committed link_scenarios_v1 expectations."""
        for entry in DATA["groundSweep"]:
            expect = MATRIX[entry["scenario"]]["expect"]["models"]
            base = next(c for c in entry["cases"] if c["ground"] == "TEST_ONLY_15_0.005"
                        and c["polarization"] == "VERTICAL")
            for model, got in base["models"].items():
                self.assertEqual(got["aboveSensitivity"]["outerLimitM"],
                                 expect[model]["aboveSensitivity"]["outerLimitM"], (entry["scenario"], model))
                rural = expect[model]["meetsSnr"]["rural"]
                self.assertEqual(got["meetsSnrRural"]["status"], rural["status"], (entry["scenario"], model))
                self.assertEqual(got["meetsSnrRural"]["outerLimitM"], rural.get("outerLimitM"),
                                 (entry["scenario"], model))

    def test_partial_recompute(self) -> None:
        sid = "S-88MHz-1W-h3-flat"
        entry = next(e for e in DATA["groundSweep"] if e["scenario"] == sid)
        case = so.GROUND_CASES[2]
        got = so.ground_sweep_one(MATRIX[sid]["input"], case, "HORIZONTAL")
        want = next(c for c in entry["cases"] if c["ground"] == case[0] and c["polarization"] == "HORIZONTAL")
        errors: list[str] = []
        so.compare(json.loads(json.dumps(got)), want, DATA["relTolerance"], sid, errors)
        self.assertEqual(errors, [])
        block = so.step_block(30.0, 1.5, 0.1)
        errors = []
        so.compare(json.loads(json.dumps(block)), DATA["stepLoss"]["blocks"][1], DATA["relTolerance"], "step", errors)
        self.assertEqual(errors, [])


class AnchorFileTest(unittest.TestCase):
    """R28 anchors (agora #2758) for the Enforce probe."""

    @classmethod
    def setUpClass(cls) -> None:
        cls.data = json.loads(so.ANCHORS.read_text(encoding="utf-8"))
        cls.cases = {c["id"]: c for c in cls.data["cases"]}

    def test_recompute(self) -> None:
        errors: list[str] = []
        so.compare(json.loads(json.dumps(so.compute_anchors())), self.data, 1e-9, "$", errors)
        self.assertEqual(errors, [])
        self.assertEqual(len(self.cases), 16)

    def test_geometry_by_hand(self) -> None:
        """Node count = ceil(10 km / step) + 1; peak heights from the triangle: 60·(1 − |x − c|/w)."""
        want = {"A09-hw30-s100-on-node": (101, 60.0), "A10-hw30-s100-missed": (101, 0.0),
                "A11-hw30-s50-on-node": (201, 60.0), "A12-hw30-s50-phase3": (201, 60.0 * (1 - 0.01875 / 0.03)),
                "A13-hw100-s100-400-h10": (101, 60.0 * (1 - 0.05 / 0.1)),
                "A14-hw500-s100-worst": (101, 60.0 * (1 - 0.05 / 0.5)), "A16-hw30-s2-reference": (5001, 60.0)}
        for cid, (n, peak) in want.items():
            e = self.cases[cid]["expect"]
            self.assertEqual(e["profilePointCount"], n, cid)
            self.assertAlmostEqual(e["peakTerrainM"], peak, places=9, msg=cid)

    def test_missed_hill_equals_flat_ground(self) -> None:
        c = self.cases["A10-hw30-s100-missed"]
        flat = so.l2_loss(10.0, 0.1, 2.0, 0.03, 0.0, 30.0, 1.5, dict(so.BASELINE_GROUND, polarization="VERTICAL"))
        self.assertAlmostEqual(c["expect"]["L"], flat, places=9)

    def test_on_node_close_to_reference(self) -> None:
        ref = self.cases["A16-hw30-s2-reference"]["expect"]["L"]
        for cid in ("A09-hw30-s100-on-node", "A11-hw30-s50-on-node"):
            self.assertLess(abs(self.cases[cid]["expect"]["L"] - ref), 0.05, cid)

    def test_flat_cases_have_flat_profile_and_vertical_beats_horizontal(self) -> None:
        for v, h in (("A01-p527-dry-30-V", "A02-p527-dry-30-H"), ("A05-p527-clay-88-V", "A06-p527-clay-88-H")):
            self.assertIsNone(self.cases[v]["input"]["hill"])
            self.assertEqual(self.cases[v]["expect"]["peakTerrainM"], 0.0)
            self.assertGreater(self.cases[h]["expect"]["L"], self.cases[v]["expect"]["L"])


if __name__ == "__main__":
    unittest.main()
