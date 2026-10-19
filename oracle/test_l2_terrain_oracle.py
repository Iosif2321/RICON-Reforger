#!/usr/bin/env python3
"""Self-test of the L2 terrain oracle: vectors pass, invariants of REQ-L2 hold, exact branch boundaries are pinned,
tampering and hostile input are detected."""

from __future__ import annotations

import copy
import json
import math
import random
import unittest
from pathlib import Path

import l2_terrain_oracle as o

VECTORS = Path(__file__).parent / "vectors" / "l2_terrain_v1.json"


def load() -> dict:
    return json.loads(VECTORS.read_text(encoding="utf-8"))


def ok_case(data: dict, cid: str) -> dict:
    return next(c for c in data["cases"] if c["id"] == cid)


class L2TerrainOracleTest(unittest.TestCase):
    def test_vectors_pass(self) -> None:
        data = load()
        self.assertEqual(o.check_vector_file(data), [])
        self.assertEqual([f for c in data["cases"] for f in o.run_case(c, data["toleranceDb"])], [])

    def test_acceptance_coverage(self) -> None:
        covered = {ct for c in load()["cases"] for ct in c["ct"]}
        # CT-002 and CT-009 partly live in the unit tests below (exact J values, polarization formulas).
        self.assertEqual(covered, {f"CT-L2-{n:03d}" for n in range(1, 11)})
        codes = {c["expect"]["code"] for c in load()["cases"]}
        self.assertEqual(codes, set(o.CODES) - {"NUMERIC_FAILURE"})  # defensive only inside the domain (REQ §2)
        cases = {c["expect"].get("sphCase") for c in load()["cases"]} - {None}
        self.assertEqual(cases, set(o.SPH_CASES))
        branches = {c["expect"].get("branchActual") for c in load()["cases"]} - {None}
        self.assertEqual(branches, set(o.BRANCHES))
        accuracy = {c["expect"].get("firstTermAccuracy") for c in load()["cases"]} - {None}
        self.assertEqual(accuracy, set(o.ACCURACY))  # eq (19) flag: MET, NOT_MET, NOT_EVALUATED all occur

    def test_tampering_is_detected(self) -> None:
        data = load()
        case = ok_case(data, "O01-single-peak")
        case["expect"]["Lba"] += 2e-6
        self.assertEqual(len(o.run_case(case, data["toleranceDb"])), 1)
        case = copy.deepcopy(ok_case(load(), "O02-hill-trans-horizon"))
        case["expect"]["branchActual"] = "LOS"
        self.assertEqual(len(o.run_case(case, data["toleranceDb"])), 1)

    def test_format_is_strict(self) -> None:
        data = load()
        self.assertEqual(o.check_vector_file({"schema": "x", "version": 1}), ["schema/version"])
        self.assertIn("source", o.check_vector_file(dict(data, source=data["source"] + "\nx")))
        self.assertIn("toleranceDb", o.check_vector_file(dict(data, toleranceDb=0.1)))
        broken = copy.deepcopy(data)
        broken["cases"][0]["extra"] = 1
        broken["cases"][1]["provenance"] = "GUESSED"
        broken["cases"][2]["ct"] = ["CT-LC-001"]
        self.assertEqual(len(o.check_vector_file(broken)), 3)
        broken = copy.deepcopy(data)
        ok_case(broken, "O01-single-peak")["expect"]["sphCase"] = "MAYBE"
        self.assertTrue(any("expect" in p for p in o.check_vector_file(broken)))

    def test_knife_edge_function(self) -> None:
        self.assertEqual(o.knife_edge_j(-0.78), 0.0)  # strict threshold
        self.assertGreater(o.knife_edge_j(-0.78 + 1e-9), -1e-6)
        self.assertAlmostEqual(o.knife_edge_j(0.0), 6.9 + 20 * math.log10(math.sqrt(1.01) - 0.1), places=12)
        self.assertAlmostEqual(o.knife_edge_j(0.0), 6.03, places=2)  # Fig. 9: about 6 dB at nu = 0
        values = [o.knife_edge_j(-0.77 + 0.01 * k) for k in range(500)]
        self.assertTrue(all(b > a for a, b in zip(values, values[1:])))

    def test_branch_boundaries_are_pinned(self) -> None:
        """Exact boundaries are host-only (REQ §6 margin rule): S_tim = S_tr is trans-horizon; exact grazing uses LoS."""
        lam, ce = o.SPEED_OF_LIGHT_MPS / 60e6, 0.0  # Ce = 0: flat Earth makes the geometry exact
        # obstacle tip exactly on the straight line between 10 m and 30 m terminals at mid path: S_tim == S_tr
        _, branch = o.bullington([0.0, 1.0, 2.0], [0.0, 20.0, 0.0], 10.0, 30.0, ce, lam)
        self.assertEqual(branch, "LOS")  # grazing: S_tim + S_rim == 0 -> LoS formula, nu = 0
        loss, _ = o.bullington([0.0, 1.0, 2.0], [0.0, 20.0, 0.0], 10.0, 30.0, ce, lam)
        self.assertAlmostEqual(loss, o.knife_edge_j(0.0) + (1 - math.exp(-o.knife_edge_j(0.0) / 6)) * 10.04, places=9)
        _, branch = o.bullington([0.0, 1.0, 2.0], [0.0, 20.0 + 1e-9, 0.0], 10.0, 30.0, ce, lam)
        self.assertEqual(branch, "TRANS")
        _, branch = o.bullington([0.0, 1.0, 2.0], [0.0, 20.0 - 1e-9, 0.0], 10.0, 30.0, ce, lam)
        self.assertEqual(branch, "LOS")

    def test_polarization_and_k(self) -> None:
        cfg = dict(load()["cases"][0]["input"]["config"])
        k_v = o.surface_k(8494.67, 50.0, dict(cfg, polarization="VERTICAL"))
        k_h = o.surface_k(8494.67, 50.0, dict(cfg, polarization="HORIZONTAL"))
        self.assertAlmostEqual(k_v / k_h, math.sqrt(15.0 ** 2 + (18000 * 0.001 / 50) ** 2), places=9)  # (12a)

    def test_l_is_never_negative_and_numeric_failure_is_unreachable_in_domain(self) -> None:
        rng = random.Random(7)
        for _ in range(3000):
            cfg = {"earthRadiusKm": 6371.0, "kFactor": rng.choice([0.5, 1.0, 4 / 3, 4.0]),
                   "permittivity": rng.choice([1.0001, 3.0, 15.0, 80.0]), "conductivitySm": rng.choice([0.0, 1e-3, 5.0]),
                   "polarization": rng.choice(o.POLARIZATIONS), "maxProfileStepKm": 500.0, "maxProfilePoints": 9}
            d = rng.uniform(0.01, 500.0)
            n = rng.choice([3, 5, 9])
            hs = [rng.uniform(-1000, 3000) for _ in range(n)]
            result = o.l2({"profile": [[d * i / (n - 1), hs[i]] for i in range(n)],
                           "htg": rng.uniform(0.01, 1e4), "hrg": rng.uniform(0.01, 1e4),
                           "frequencyMHz": rng.uniform(30, 1e5), "config": cfg})
            self.assertEqual(result["code"], "OK")
            self.assertGreaterEqual(result["L"], 0.0)

    def test_hostile_input_is_rejected_without_crash(self) -> None:
        good = copy.deepcopy(ok_case(load(), "O01-single-peak")["input"])
        for mutate, code in (
                (lambda i: i.update(config=None), "INVALID_CONFIG"),
                (lambda i: i["config"].update(kFactor=True), "INVALID_CONFIG"),
                (lambda i: i["config"].update(extra=1), "INVALID_CONFIG"),
                (lambda i: i["config"].update(maxProfilePoints=21.0), "INVALID_CONFIG"),
                (lambda i: i["config"].update(maxProfilePoints=3), "INVALID_PROFILE"),
                (lambda i: i.update(frequencyMHz="60"), "OUT_OF_DOMAIN_FREQUENCY"),
                (lambda i: i.update(frequencyMHz=float("inf")), "OUT_OF_DOMAIN_FREQUENCY"),
                (lambda i: i.update(profile="flat"), "INVALID_PROFILE"),
                (lambda i: i["profile"].__setitem__(1, [0.1]), "INVALID_PROFILE"),
                (lambda i: i["profile"].__setitem__(1, [0.1, float("nan")]), "INVALID_PROFILE"),
                (lambda i: i.update(htg=None), "INVALID_ANTENNA_HEIGHT")):
            bad = copy.deepcopy(good)
            mutate(bad)
            self.assertEqual(o.l2(bad), {"code": code}, code)
        self.assertEqual(o.l2("x"), {"code": "INVALID_CONFIG"})


if __name__ == "__main__":
    unittest.main()
