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

    def test_codex_independent_anchors(self) -> None:
        """agora #2110: codex's separate math/cmath computation from P.526-16 eq (100)/(101), not this code."""
        brewster = {"permittivity": 4.0, "conductivitySm": 0.0, "polarization": "VERTICAL"}
        r = o.two_ray_flat(6.0, 1.5, 1.5, 50.0, brewster)
        self.assertAlmostEqual(abs(r["reflection"]), 0.0, places=12)
        self.assertAlmostEqual(r["lossDb"], 21.9902083163, places=8)
        dry_h = dict(DRY, polarization="HORIZONTAL")
        r = o.two_ray_flat(1000.0, 1.5, 1.5, 50.0, dry_h)
        self.assertAlmostEqual(r["reflection"].real, -0.998407526290, places=11)
        self.assertAlmostEqual(r["reflection"].imag, 0.000101872121, places=11)
        self.assertAlmostEqual(r["pathDifferenceM"], 0.00449998987505, places=13)
        self.assertAlmostEqual(o.fspl_db(1000.0, 50.0), 66.4271833086, places=8)
        self.assertAlmostEqual(r["lossDb"], 112.323938356, places=7)
        self.assertAlmostEqual(o.two_ray_flat(1000.0, 1.5, 1.5, 400.0, dry_h)["lossDb"], 112.953024005, places=7)
        self.assertAlmostEqual(o.fspl_db(100.0, 400.0), 64.4889830484, places=8)
        self.assertAlmostEqual(o.two_ray_flat(100.0, 10.0, 10.0, 400.0, dry_h)["lossDb"], 59.8888003288, places=7)

    def test_path_difference_without_cancellation(self) -> None:
        """agora #2138: r2 − r1 loses digits on long paths; the oracle uses 4·h1·h2/(r1 + r2)."""
        r = o.two_ray_flat(500000.0, 0.01, 0.01, 30.0, DRY)
        self.assertAlmostEqual(r["pathDifferenceM"] / 4e-10, 1.0, places=9)

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


class KnifeEdgeTest(unittest.TestCase):
    def test_fresnel_table_values(self) -> None:
        """Abramowitz & Stegun Table 7.7 values, 10 digits."""
        for x, c, s in ((1.0, 0.7798934004, 0.4382591474), (2.0, 0.4882534061, 0.3434156784),
                        (3.0, 0.6057207893, 0.4963129990), (5.0, 0.5636311887, 0.4991913820)):
            got = o.fresnel_cs(x)
            self.assertAlmostEqual(got[0], c, places=9)
            self.assertAlmostEqual(got[1], s, places=9)
            self.assertEqual(o.fresnel_cs(-x), (-got[0], -got[1]))
        c, s = o.fresnel_cs(1e4)
        self.assertAlmostEqual(c, 0.5, places=4)
        self.assertAlmostEqual(s, 0.5, places=4)

    def test_fresnel_methods_join(self) -> None:
        """Series up to 2, continued fraction beyond: the step at the join equals the slope dC/dx = cos(πx²/2)."""
        dx = 1e-9
        c1, s1 = o.fresnel_cs(2.0)
        c2, s2 = o.fresnel_cs(2.0 + dx)
        self.assertAlmostEqual((c2 - c1) / dx, math.cos(2 * math.pi), places=4)
        self.assertAlmostEqual((s2 - s1) / dx, math.sin(2 * math.pi), places=4)

    def test_knife_edge_field(self) -> None:
        self.assertAlmostEqual(abs(o.knife_edge_field(0.0)), 0.5, places=12)                # grazing: −6.02 dB
        self.assertAlmostEqual(abs(o.knife_edge_field(-60.0)), 1.0, places=2)               # clear path
        for nu in (-0.7, 0.0, 0.8, 2.4, 7.0):
            self.assertAlmostEqual(-20 * math.log10(abs(o.knife_edge_field(nu))), o.knife_edge_j30(nu), places=10)

    def test_eq31_approximates_eq27(self) -> None:
        """Eq (31), used by L2, stays within 0.124 dB of the exact eq (30) on ν ∈ [−0.7, 10]."""
        worst = max(abs(o.knife_edge_j30(nu) - l2o.knife_edge_j(nu)) for nu in (-0.7 + i * 0.01 for i in range(1071)))
        self.assertLess(worst, 0.124)
        self.assertGreater(worst, 0.1)

    def test_no_obstacle_limit_is_not_two_ray(self) -> None:
        """agora #2203: the documented limitation of the RESEARCH model. A vanishing 1 cm edge should leave the
        two-ray result. With 200 m antennas it does, within 1 dB; with 3 m antennas every path grazes the edge
        (ν ≈ 0) and the loss moves by 7.5 dB, so the no-obstacle limit fails exactly where D-002 is asked."""
        for h, low, high in ((200.0, 0.0, 1.0), (3.0, 7.0, 8.0)):
            four = o.knife_edge_4ray(4000.0, h, h, 50.0, DRY, 2000.0, 0.01)
            diff = abs(four["lossDb"] - o.two_ray_flat(4000.0, h, h, 50.0, DRY)["lossDb"])
            self.assertTrue(low <= diff <= high, (h, diff))
        # codex #2214: an edge of zero height midway, 1.5 m antennas, vertical polarization: 12.03 dB off at 1 km
        four = o.knife_edge_4ray(1000.0, 1.5, 1.5, 50.0, DRY, 500.0, 0.0)
        self.assertAlmostEqual(four["lossDb"] - o.two_ray_flat(1000.0, 1.5, 1.5, 50.0, DRY)["lossDb"], 12.026583,
                               places=5)

    def test_no_ground_is_one_knife_edge(self) -> None:
        r = o.knife_edge_4ray(4000.0, 3.0, 3.0, 50.0, None, 2000.0, 60.0)
        self.assertAlmostEqual(r["excessDb"], o.knife_edge_j30(r["nu"][0]), places=10)
        nu = 57.0 * math.sqrt(2.0 * 4000.0 / (o.wavelength_m(50.0) * 2000.0 * 2000.0))     # eq (26), clearance 57 m
        self.assertAlmostEqual(r["nu"][0], nu, places=12)

    def test_obstacle_gain_characterisation(self) -> None:
        """Characterisation for D-002 (agora #2198, #2203), not a conclusion: 50 MHz, 3 m antennas, 60 m hill at
        2 km, receiver at 4 km. The RESEARCH 4-ray model gives 3.09 dB less loss than flat two-ray; §4.5 gives
        13.98 dB less than its own flat result. The 4-ray model has no verified source, so D-002 stays OPEN."""
        flat = {"earthRadiusKm": 6371.0, "kFactor": 4 / 3, "profileStepKm": 0.1, "hill": None}
        hill = dict(flat, hill={"centerKm": 2.0, "heightM": 60.0, "halfWidthKm": 0.5})
        ref_gain = (o.two_ray_flat(4000.0, 3.0, 3.0, 50.0, DRY)["lossDb"]
                    - o.knife_edge_4ray(4000.0, 3.0, 3.0, 50.0, DRY, 2000.0, 60.0)["lossDb"])
        p526_gain = o.p526_general_db(4000.0, 3.0, 3.0, 50.0, DRY, flat) - o.p526_general_db(4000.0, 3.0, 3.0, 50.0,
                                                                                              DRY, hill)
        self.assertAlmostEqual(ref_gain, 3.0903, places=3)
        self.assertAlmostEqual(p526_gain, 13.9777, places=3)
        self.assertGreater(p526_gain - ref_gain, 8.0)


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

    def test_budget(self) -> None:
        radio = {"txPowerDbm": 37.0, "txGainDbi": 2.0, "rxGainDbi": 1.0, "bandwidthHz": 25000.0,
                 "rxNoiseFigureDb": 10.0, "snrRequiredDb": 10.0}
        b = o.budget(150.0, radio, -120.0)
        self.assertAlmostEqual(b["rxPowerDbm"], -110.0)
        self.assertAlmostEqual(b["snrDb"], 10.0)
        self.assertTrue(b["meetsSnr"])

    def test_sensitivity_and_snr_are_separate(self) -> None:
        """codex #2132 item 2: Prx −105 dBm in city noise at 30 MHz is above the RICON-style sensitivity
        (−109.995787 dBm) but its actual SNR is −10.898 dB."""
        radio = {"txPowerDbm": 30.0, "txGainDbi": 0.0, "rxGainDbi": 0.0, "bandwidthHz": 25000.0,
                 "rxNoiseFigureDb": 10.0, "snrRequiredDb": 10.0}
        self.assertAlmostEqual(o.sensitivity_dbm(radio), -109.995787, places=5)
        noise = o.system_noise_dbm(25000.0, 10.0, "city", 30.0)["noiseDbm"]
        self.assertAlmostEqual(noise, -94.101973, places=5)
        b = o.budget(135.0, radio, noise)
        self.assertTrue(b["aboveSensitivity"])
        self.assertFalse(b["meetsSnr"])
        self.assertAlmostEqual(b["snrDb"], -10.898027, places=5)


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

    def test_fspl_uses_real_end_altitudes(self) -> None:
        """codex #2132 item 3 (30 MHz, 2 km): with the receiver on a 600 m hill top, FSPL follows the real altitude difference."""
        hill = {"centerKm": 2.0, "heightM": 600.0, "halfWidthKm": 0.5}
        self.assertAlmostEqual(o.slant_fspl_db(2000.0, 1.5, 1.5, 30.0, None), 68.010808, places=5)
        self.assertAlmostEqual(o.slant_fspl_db(2000.0, 1.5, 1.5, 30.0, hill), 68.385073, places=5)
        sc = scenario(hill=hill, models=["FSPL"])
        sc["frequencyMHz"] = 30.0
        self.assertAlmostEqual(o.loss_function("FSPL", sc)(2000.0), 68.385073, places=5)

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


def scenario(hill=None, models=None) -> dict:
    models = models or ["FSPL", "TWO_RAY_FLAT", "P526_GENERAL"]
    return {"frequencyMHz": 50.0, "txHeightM": 1.5, "rxHeightM": 1.5, "ground": dict(DRY),
            "terrain": {"earthRadiusKm": 6371.0, "kFactor": 4 / 3, "profileStepKm": 0.1, "hill": hill},
            "radio": {"txPowerDbm": 30.0, "txGainDbi": 0.0, "rxGainDbi": 0.0, "bandwidthHz": 25000.0,
                      "rxNoiseFigureDb": 10.0, "snrRequiredDb": 10.0},
            "noiseEnvironments": ["none"], "models": models,
            "grids": {m: {"minM": 100.0, "maxM": 1000.0, "ratio": 1.5} for m in models},
            "sampleDistancesM": [500.0]}


class InputDomainTest(unittest.TestCase):
    """agora #2132 item 1: a scenario outside the closed domain must never be evaluated silently."""

    def test_valid(self) -> None:
        self.assertIsNone(o.validate_scenario(scenario()))

    def test_rejections(self) -> None:
        def bad(mutate) -> dict:
            sc = scenario()
            mutate(sc)
            return sc

        cases = [
            lambda s: s["models"].__setitem__(0, "TYPO"),
            lambda s: s["ground"].update(polarization="TYPO"),
            lambda s: s.update(models=[]),
            lambda s: s.update(models=["FSPL", "FSPL"]),
            lambda s: s.update(noiseEnvironments=["quiet_rural"]),
            lambda s: s.update(extra=1),
            lambda s: s["radio"].update(extra=1),
            lambda s: s["radio"].update(txPowerDbm=True),
            lambda s: s["radio"].update(txPowerDbm=float("nan")),
            lambda s: s.update(frequencyMHz=10 ** 400),
            lambda s: s.update(frequencyMHz=20.0),
            lambda s: s["grids"]["FSPL"].update(ratio=1.0),
            lambda s: s["grids"]["FSPL"].update(maxM=50.0),
            lambda s: s["grids"].pop("FSPL"),
            lambda s: s["terrain"].update(hill={"centerKm": 1.0, "heightM": 10.0, "halfWidthKm": 0.1}),  # with two-ray
            lambda s: s["terrain"].update(kFactor=0.0),
            lambda s: s.update(sampleDistancesM=[]),
            lambda s: s.update(models=["KNIFE_EDGE_4RAY"], grids={"KNIFE_EDGE_4RAY": {"minM": 100.0, "maxM": 1000.0,
                                                                                      "ratio": 1.5}}),  # not a model
        ]
        for mutate in cases:
            sc = bad(mutate)
            self.assertIsNotNone(o.validate_scenario(sc))
            with self.assertRaises(ValueError):
                o.evaluate_scenario(sc)

    def test_unknown_names_raise(self) -> None:
        with self.assertRaises(ValueError):
            o.reflection_coefficient(0.1, 50.0, 15.0, 0.005, "TYPO")
        with self.assertRaises(ValueError):
            o.loss_function("TYPO", scenario())

    def test_bool_and_non_finite_never_match(self) -> None:
        inf, nan = float("inf"), float("nan")
        for a, b in ((True, 1), (False, 0), (1, True), (None, 0), (1.0, inf), (1.0, -inf), (inf, inf), (nan, nan),
                     (1.0, nan)):
            errors: list = []
            o._close(a, b, 1e-9, "x", errors)
            self.assertNotEqual(errors, [])


PROBES = Path(__file__).resolve().parent / "vectors" / "link_probe_points_v1.json"


class ProbePointsTest(unittest.TestCase):
    """Golden budgets for the Enforce probe of codex (agora #2201, #2206)."""

    def setUp(self) -> None:
        self.data = json.loads(PROBES.read_text(encoding="utf-8"))

    def test_file_passes(self) -> None:
        self.assertEqual(o.check_probe_file(self.data), [])
        self.assertEqual([p["id"] for p in self.data["points"]], [f"P0{i}" for i in range(1, 8)])

    def test_against_codex_l0_oracle(self) -> None:
        """The free-space and noise part of every point equals codex's independent L0 oracle (ricon_oracle.py,
        REQ-LE); the terrain layer only shifts Prx and SINR by terrainDb."""
        import ricon_oracle as ro
        fixture = {"a": -15.0, "b": 15.0, "qClean": 0.8, "qDegraded": 0.5}
        for point in self.data["points"]:
            p, e = point["input"], point["expect"]
            hill = p["terrain"]["hill"] if p["layers"] == "P526_GENERAL" else None
            z1 = o.hill_height_m(0.0, hill) + p["txHeightM"]
            z2 = o.hill_height_m(p["distanceM"] / 1000.0, hill) + p["rxHeightM"]
            r = p["radio"]
            query = {"txPos": [0.0, z1, 0.0], "rxPos": [p["distanceM"], z2, 0.0], "frequencyMHz": p["frequencyMHz"],
                     "bandwidthHz": r["bandwidthHz"], "txPowerDbm": r["txPowerDbm"], "txGainDbi": r["txGainDbi"],
                     "rxGainDbi": r["rxGainDbi"], "rxNoiseFigureDb": r["rxNoiseFigureDb"], "hasRxSensitivity": False,
                     "rxSensitivityDbm": 0.0, "hasRequiredSinr": True, "requiredSinrDb": r["snrRequiredDb"],
                     "hasInterference": False, "interferenceDbm": 0.0, "noiseEnvironmentId": p["noiseEnvironment"],
                     "bandId": "test_vhf", "modelId": "free_space", "intelligibilityModelId": "linear_test"}
            b = ro.evaluate_budget(query, fixture)
            shift = e["terrainDb"] or 0.0
            self.assertAlmostEqual(b["layers"][0]["lossDb"], e["fsplDb"], places=9, msg=point["id"])
            self.assertAlmostEqual(b["noiseDbm"], e["noiseDbm"], places=9, msg=point["id"])
            self.assertEqual(b["noiseModelInDomain"], e["noiseModelInDomain"], point["id"])
            self.assertAlmostEqual(b["sensitivityDbm"], e["sensitivityDbm"], places=9, msg=point["id"])
            self.assertAlmostEqual(b["rxPowerDbm"] - shift, e["rxPowerDbm"], places=9, msg=point["id"])
            if shift == 0.0:
                self.assertEqual(b["state"], e["state"])
                self.assertAlmostEqual(b["intelligibility"], e["intelligibility"], places=9)

    def test_d004_and_d005_points(self) -> None:
        by = {p["id"]: p["expect"] for p in self.data["points"]}
        self.assertEqual(by["P07"]["state"], "WEAK")                      # D-004: SINR −10.9 dB is not LOST
        self.assertAlmostEqual(by["P07"]["sinrDb"], -10.898027, places=5)
        self.assertFalse(by["P06"]["noiseModelInDomain"])                 # D-005: 400 MHz rural → f_a = 1
        self.assertAlmostEqual(by["P06"]["noiseDbm"], by["P06"]["thermalNoiseDbm"] + 10.0, places=9)

    def test_corruption_is_caught(self) -> None:
        bad = copy.deepcopy(self.data)
        bad["points"][0]["expect"]["state"] = "WEAK"
        self.assertNotEqual(o.check_probe_file(bad), [])
        bad = copy.deepcopy(self.data)
        bad["points"][1]["expect"]["sinrDb"] += 1e-6
        self.assertNotEqual(o.check_probe_file(bad), [])
        bad = copy.deepcopy(self.data)
        bad["points"][0]["input"]["layers"] = "TWO_RAY_FLAT"
        self.assertNotEqual(o.check_probe_file(bad), [])
        bad = copy.deepcopy(self.data)
        bad["fixture"]["a"] = -20.0
        self.assertEqual(o.check_probe_file(bad), ["envelope"])


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
        self.assertEqual(self.data["revision"], 3)

    def test_samples_carry_the_budget(self) -> None:
        for sc in self.data["scenarios"]:
            for entry in sc["expect"]["models"].values():
                for point in entry["samples"]:
                    self.assertEqual(set(point) - {"curvaturePhaseErrorRad"},
                                     {"distanceM", "lossDb", "rxPowerDbm", "aboveSensitivity", "noise"})
                    self.assertEqual(set(point["noise"]), set(sc["input"]["noiseEnvironments"]))

    def test_corruption_is_caught(self) -> None:
        bad = copy.deepcopy(self.data)
        sc = next(s for s in bad["scenarios"] if "TWO_RAY_FLAT" in s["expect"]["models"])
        sc["expect"]["models"]["TWO_RAY_FLAT"]["samples"][0]["lossDb"] += 0.01
        self.assertNotEqual(o.check_vector_file(bad, self.evaluated), [])
        bad = copy.deepcopy(self.data)
        bad["scenarios"][0]["expect"]["models"]["FSPL"]["samples"][0]["lossDb"] = float("inf")
        self.assertNotEqual(o.check_vector_file(bad, self.evaluated), [])
        bad = copy.deepcopy(self.data)
        bad["scenarios"][0]["expect"]["models"]["FSPL"]["meetsSnr"]["none"]["status"] = "SAMPLED_ESTIMATE"
        self.assertNotEqual(o.check_vector_file(bad, self.evaluated), [])
        bad = copy.deepcopy(self.data)
        bad["scenarios"][0]["expect"]["models"]["FSPL"]["aboveSensitivity"]["intervalsM"] = []
        self.assertNotEqual(o.check_vector_file(bad, self.evaluated), [])

    def test_envelope_is_closed(self) -> None:
        for key, value in (("relTolerance", 1.0), ("scenarios", []), ("speechQuality", "CALIBRATED"),
                           ("snrState", "TEST_ONLY"), ("provenance", "CROSS_CHECKED"), ("extra", 1),
                           ("version", True), ("version", 1.0), ("revision", 0), ("revision", True),
                           ("source", None), ("standards", ["Rec. ITU-R P.526-15"]), ("note", None),
                           ("note", "")):  # agora #2162
            bad = copy.deepcopy(self.data)
            bad[key] = value
            self.assertEqual(o.check_vector_file(bad, self.evaluated), ["envelope"], key)
        bad = copy.deepcopy(self.data)
        bad["scenarios"].append(copy.deepcopy(bad["scenarios"][0]))
        self.assertEqual(o.check_vector_file(bad, self.evaluated), ["scenario ids"])
        bad = copy.deepcopy(self.data)
        bad["scenarios"][0]["input"]["models"] = ["TYPO"]
        self.assertNotEqual(o.check_vector_file(bad, self.evaluated), [])

    def test_labels(self) -> None:
        self.assertEqual(self.data["speechQuality"], "NOT_CALIBRATED")
        for sc in self.data["scenarios"]:
            for entry in sc["expect"]["models"].values():
                ranges = [entry["aboveSensitivity"]] + list(entry["meetsSnr"].values())
                for r in ranges:
                    if r.get("outerLimitM") is not None:
                        self.assertEqual(r["status"], "SAMPLED_ESTIMATE")
                # with no external noise both criteria coincide: noise = kT0B + NF
                self.assertEqual(entry["aboveSensitivity"], entry["meetsSnr"]["none"])


if __name__ == "__main__":
    unittest.main()
