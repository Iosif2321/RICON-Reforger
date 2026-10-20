#!/usr/bin/env python3
"""Tests of the link physics oracle against closed-form anchors that do not use the oracle's own formulas
(limits, symmetries, exact constants) and against the committed scenario vectors."""

from __future__ import annotations

import copy
import json
import math
import unittest
from pathlib import Path

import l2_terrain_oracle as l2o
import link_physics_oracle as o

VECTORS = Path(__file__).resolve().parent / "vectors" / "link_scenarios_v1.json"
DRY = {"permittivity": 15.0, "conductivitySm": 0.005, "polarization": "VERTICAL"}
METAL_H = {"permittivity": 1.5, "conductivitySm": 1e9, "polarization": "HORIZONTAL"}  # limiting conductor
METAL_V = dict(METAL_H, polarization="VERTICAL")


class FreeSpaceAndFresnelTest(unittest.TestCase):
    def test_fspl_constant_and_slope(self) -> None:
        # 20 lg(4π·10⁹/c) for 1 km, 1 MHz; the familiar rounded value is 32.45 dB
        self.assertAlmostEqual(o.fspl_db(1000.0, 1.0), 32.44778322, places=7)
        self.assertAlmostEqual(o.fspl_db(2000.0, 50.0) - o.fspl_db(1000.0, 50.0), 20 * math.log10(2), places=12)
        self.assertAlmostEqual(o.fspl_db(1000.0, 100.0) - o.fspl_db(1000.0, 10.0), 20.0, places=12)

    def test_fresnel_radius_matches_practical_form(self) -> None:
        """P.526-16 eq (3) writes sqrt(c) as 550; the exact factor is sqrt(299792.458) ≈ 547.53 (0.45 % apart)."""
        d1, d2, f = 3.0, 7.0, 88.0
        practical = 550.0 * math.sqrt(d1 * d2 / ((d1 + d2) * f))
        exact = o.fresnel_radius_m(1, d1 * 1000.0, d2 * 1000.0, f)
        self.assertAlmostEqual(practical / exact, 550.0 / math.sqrt(299792.458), places=12)
        self.assertAlmostEqual(o.fresnel_radius_m(4, 5000.0, 5000.0, f), 2 * o.fresnel_radius_m(1, 5000.0, 5000.0, f))


class ReflectionTest(unittest.TestCase):
    def test_normal_incidence_lossless(self) -> None:
        n = math.sqrt(9.0)
        rh = o.reflection_coefficient(math.pi / 2, 100.0, 9.0, 0.0, "HORIZONTAL")
        rv = o.reflection_coefficient(math.pi / 2, 100.0, 9.0, 0.0, "VERTICAL")
        self.assertAlmostEqual(rh, (1 - n) / (1 + n), places=12)
        self.assertAlmostEqual(rv, -rh, places=12)

    def test_grazing_limit_is_minus_one(self) -> None:
        for pol in o.POLARIZATIONS:
            self.assertAlmostEqual(o.reflection_coefficient(1e-9, 50.0, 15.0, 0.005, pol), -1.0, places=6)

    def test_brewster_angle_vertical(self) -> None:
        eps = 15.0
        psi_b = math.atan(1.0 / math.sqrt(eps))
        self.assertAlmostEqual(abs(o.reflection_coefficient(psi_b, 50.0, eps, 0.0, "VERTICAL")), 0.0, places=12)
        self.assertGreater(abs(o.reflection_coefficient(psi_b, 50.0, eps, 0.0, "HORIZONTAL")), 0.3)

    def test_good_conductor(self) -> None:
        """R_H → −1; R_V → +1 only while sqrt|η|·sin ψ ≫ 1 (at grazing it still turns to −1)."""
        psi = 0.01
        self.assertAlmostEqual(o.reflection_coefficient(psi, 50.0, 1.5, 1e9, "HORIZONTAL"), -1.0, places=4)
        self.assertAlmostEqual(o.reflection_coefficient(psi, 50.0, 1.5, 1e9, "VERTICAL"), 1.0, places=3)
        self.assertAlmostEqual(o.reflection_coefficient(1e-12, 50.0, 1.5, 1e9, "VERTICAL"), -1.0, places=4)

    def test_lossy_ground_is_passive(self) -> None:
        for pol in o.POLARIZATIONS:
            for psi in (1e-4, 0.01, 0.1, 0.5, 1.2):
                for f in (30.0, 88.0, 400.0):
                    self.assertLessEqual(abs(o.reflection_coefficient(psi, f, 15.0, 0.005, pol)), 1.0)


class TwoRayTest(unittest.TestCase):
    def test_plane_earth_asymptote(self) -> None:
        """R = −1 far beyond the breakpoint: L → 40 lg d − 20 lg(h1·h2), independent of frequency."""
        for f in (30.0, 400.0):
            loss = o.two_ray_flat(200000.0, 10.0, 10.0, f, METAL_H)["lossDb"]
            self.assertAlmostEqual(loss, 40 * math.log10(200000.0) - 20 * math.log10(100.0), delta=0.01)

    def test_conductor_vertical_doubles_field(self) -> None:
        """R = +1 and a path difference much smaller than λ: field doubles, 6.02 dB less than free space."""
        r = o.two_ray_flat(2000.0, 2.0, 2.0, 30.0, METAL_V)
        self.assertAlmostEqual(r["lossDb"], o.fspl_db(2000.0, 30.0) - 20 * math.log10(2), delta=0.01)

    def test_interference_null(self) -> None:
        """With R ≈ −1 and Δr = λ the two rays cancel up to the r1/r2 amplitude ratio."""
        h, f = 10.0, 400.0
        lam = o.wavelength_m(f)
        d = self._solve_path_difference(h, lam)
        r = o.two_ray_flat(d, h, h, f, METAL_H)
        self.assertAlmostEqual(r["pathDifferenceM"], lam, places=6)
        self.assertGreater(r["excessDb"], 30.0)

    @staticmethod
    def _solve_path_difference(h: float, lam: float) -> float:
        lo, hi = 1.0, 1e6
        for _ in range(200):
            mid = (lo + hi) / 2
            if math.hypot(mid, 2 * h) - mid > lam:
                lo = mid
            else:
                hi = mid
        return (lo + hi) / 2

    def test_grid_can_miss_a_null(self) -> None:
        """agora #2083: a sampled range is an estimate. A null between two grid nodes is deeper than either node."""
        h, f = 30.0, 400.0
        d_null = self._solve_path_difference(h, o.wavelength_m(f))
        at_null = o.two_ray_flat(d_null, h, h, f, METAL_H)["lossDb"]
        for ratio, depth in ((1.001, 10.0), (1.01, 30.0)):  # one cell of the two-ray grid and of the terrain grid
            nodes = (d_null / math.sqrt(ratio), d_null * math.sqrt(ratio))
            self.assertGreater(at_null - max(o.two_ray_flat(x, h, h, f, METAL_H)["lossDb"] for x in nodes), depth)

    def test_low_antennas_lose_far_more_than_free_space(self) -> None:
        """Over real ground with 1.5 m antennas the reflected ray cancels the direct one: at 3 km the two-ray loss
        exceeds FSPL by more than 20 dB at every band of the matrix. RICON models no ground reflection (agora #2095)."""
        for f in (30.0, 50.0, 88.0, 400.0):
            for pol in o.POLARIZATIONS:
                ground = dict(DRY, polarization=pol)
                self.assertGreater(o.two_ray_flat(3000.0, 1.5, 1.5, f, ground)["excessDb"], 20.0)

    def test_curvature_phase_error_grows_with_distance(self) -> None:
        ae = 6371.0 * 4 / 3
        near = o.curvature_phase_error_rad(1000.0, 10.0, 10.0, 88.0, ae)
        far = o.curvature_phase_error_rad(15000.0, 10.0, 10.0, 88.0, ae)
        self.assertLess(near, 1e-2)
        self.assertGreater(far, near)
        # at the horizon the heights above the tangent plane reach zero: the whole flat Δr is phase error
        d_los = o.radio_horizon_m(10.0, 10.0, ae)
        k = 2 * math.pi / o.wavelength_m(88.0)
        self.assertAlmostEqual(o.curvature_phase_error_rad(d_los * 0.99999, 10.0, 10.0, 88.0, ae),
                               k * 2 * 10.0 * 10.0 / (d_los * 0.99999), delta=1e-4)
        self.assertIsNone(o.curvature_phase_error_rad(d_los * 1.001, 10.0, 10.0, 88.0, ae))

    def test_radio_horizon(self) -> None:
        # 4/3 Earth: d_los ≈ 4.12·(√h1 + √h2) km, the textbook rounding of sqrt(2·8495 km)
        self.assertAlmostEqual(o.radio_horizon_m(10.0, 10.0, 6371.0 * 4 / 3) / 1000.0, 4.1224 * 2 * math.sqrt(10.0),
                               delta=0.01)


class NoiseTest(unittest.TestCase):
    def test_thermal(self) -> None:
        self.assertAlmostEqual(o.thermal_noise_dbm(1.0), -173.975187, places=5)
        # P.372-17 eq (6) rounds 10 lg(k·T0) to −204 dBW, i.e. 0.025 dB lower than the exact constant
        self.assertAlmostEqual(o.thermal_noise_dbm(1.0) - 30.0 - (-204.0), 0.0248128, places=6)

    def test_receiver_only(self) -> None:
        n = o.system_noise_dbm(25000.0, 10.0, "none", 400.0)
        self.assertAlmostEqual(n["noiseDbm"], o.thermal_noise_dbm(25000.0) + 10.0, places=12)

    def test_manmade_sum(self) -> None:
        n = o.system_noise_dbm(25000.0, 0.0, "rural", 100.0)
        fam = 67.2 - 27.7 * 2.0
        self.assertAlmostEqual(n["famDb"], fam, places=12)
        self.assertAlmostEqual(n["noiseDbm"], o.thermal_noise_dbm(25000.0) + fam, places=12)  # fr = 1 → f = fa

    def test_domain(self) -> None:
        self.assertTrue(o.system_noise_dbm(25000.0, 6.0, "city", 250.0)["inDomain"])
        self.assertFalse(o.system_noise_dbm(25000.0, 6.0, "city", 250.01)["inDomain"])
        self.assertIsNone(o.system_noise_dbm(25000.0, 6.0, "city", 400.0)["noiseDbm"])

    def test_budget_labels(self) -> None:
        radio = {"txPowerDbm": 37.0, "txGainDbi": 2.0, "rxGainDbi": 1.0, "snrRequiredDb": 10.0}
        b = o.budget(150.0, radio, -120.0)
        self.assertAlmostEqual(b["rxPowerDbm"], -110.0)
        self.assertAlmostEqual(b["snrDb"], 10.0)
        self.assertTrue(b["available"])
        self.assertEqual((b["snrState"], b["speechQuality"]), ("TEST_ONLY", "NOT_CALIBRATED"))


class RangeTest(unittest.TestCase):
    def test_runs(self) -> None:
        pts = [(1, True), (2, True), (3, False), (4, True), (5, False), (6, True)]
        self.assertEqual(o.availability(pts), [[1, 2], [4, 4], [6, 6]])
        self.assertEqual(o.availability([(1, False)]), [])

    def test_grid(self) -> None:
        g = o.geometric_grid({"minM": 10.0, "maxM": 1000.0, "ratio": 10.0})
        self.assertEqual(len(g), 3)
        self.assertAlmostEqual(g[-1], 1000.0)

    def test_p526_flat_equals_l2(self) -> None:
        terrain = {"earthRadiusKm": 6371.0, "kFactor": 4 / 3, "profileStepKm": 0.1, "hill": None}
        loss = o.p526_general_db(20000.0, 10.0, 3.0, 88.0, DRY, terrain)
        cfg = {"earthRadiusKm": 6371.0, "kFactor": 4 / 3, "permittivity": 15.0, "conductivitySm": 0.005,
               "polarization": "VERTICAL", "maxProfileStepKm": 0.1, "maxProfilePoints": l2o.MAX_PROFILE_POINTS_CAP}
        r = l2o.l2({"profile": [[20.0 * i / 200, 0.0] for i in range(201)], "htg": 10.0, "hrg": 3.0,
                    "frequencyMHz": 88.0, "config": cfg})
        self.assertAlmostEqual(loss, o.fspl_db(math.hypot(20000.0, 7.0), 88.0) + r["L"], places=9)
        self.assertAlmostEqual(r["L"], max(r["Lsph"], r["Lbs"]), places=9)  # STD_INVARIANT of REQ-L2 §7

    def test_hill_can_lower_the_p526_loss(self) -> None:
        """Characterisation, not a physics claim: with 3 m antennas at 50 MHz a 60 m hill midway gives a LOWER §4.5
        loss than flat ground (L 27.16 dB against 41.14 dB). Eq (62c,d) lowers the smooth surface under the
        obstruction, the effective heights grow and Lsph falls from 41.14 to 9.66 dB; Lba rises only to 23.35 dB.
        Whether this matches the real effect known as obstacle gain is open (R21 discrepancy registry)."""
        flat = {"earthRadiusKm": 6371.0, "kFactor": 4 / 3, "profileStepKm": 0.1, "hill": None}
        hill = dict(flat, hill={"centerKm": 2.0, "heightM": 60.0, "halfWidthKm": 0.5})
        fspl = o.fspl_db(4000.0, 50.0)
        self.assertAlmostEqual(o.p526_general_db(4000.0, 3.0, 3.0, 50.0, DRY, flat) - fspl, 41.1409, places=3)
        self.assertAlmostEqual(o.p526_general_db(4000.0, 3.0, 3.0, 50.0, DRY, hill) - fspl, 27.1632, places=3)
        # with 10 m antennas the hill does add loss
        self.assertGreater(o.p526_general_db(4000.0, 10.0, 10.0, 50.0, DRY, hill),
                           o.p526_general_db(4000.0, 10.0, 10.0, 50.0, DRY, flat))


class VectorFileTest(unittest.TestCase):
    evaluated: dict = {}

    @classmethod
    def setUpClass(cls) -> None:
        data = json.loads(VECTORS.read_text(encoding="utf-8"))
        cls.evaluated = {sc["id"]: o.evaluate_scenario(sc["input"]) for sc in data["scenarios"]}

    def setUp(self) -> None:
        self.data = json.loads(VECTORS.read_text(encoding="utf-8"))

    def test_vectors_pass(self) -> None:
        self.assertEqual(o.check_vector_file(self.data, self.evaluated), [])
        self.assertEqual(len(self.data["scenarios"]), 48)

    def test_corruption_is_caught(self) -> None:
        bad = copy.deepcopy(self.data)
        sc = next(s for s in bad["scenarios"] if "TWO_RAY_FLAT" in s["expect"]["models"])
        sc["expect"]["models"]["TWO_RAY_FLAT"]["samples"][0]["lossDb"] += 0.01
        self.assertNotEqual(o.check_vector_file(bad, self.evaluated), [])
        bad = copy.deepcopy(self.data)
        bad["scenarios"][0]["expect"]["models"]["FSPL"]["range"]["none"]["status"] = "SAMPLED_ESTIMATE"
        self.assertNotEqual(o.check_vector_file(bad, self.evaluated), [])
        bad = copy.deepcopy(self.data)
        bad["relTolerance"] = 1.0
        self.assertEqual(o.check_vector_file(bad, self.evaluated), ["envelope"])

    def test_labels(self) -> None:
        self.assertEqual(self.data["speechQuality"], "NOT_CALIBRATED")
        for sc in self.data["scenarios"]:
            for entry in sc["expect"]["models"].values():
                for r in entry["range"].values():
                    if r.get("outerLimitM") is not None:
                        self.assertEqual(r["status"], "SAMPLED_ESTIMATE")


if __name__ == "__main__":
    unittest.main()
