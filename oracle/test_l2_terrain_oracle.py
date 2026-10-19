#!/usr/bin/env python3
"""Self-test of the L2 terrain oracle: vectors pass, invariants of REQ-L2 hold, exact branch boundaries are pinned,
tampering and hostile input are detected."""

from __future__ import annotations

import copy
import itertools
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

    def test_formula_invariants_on_vectors(self) -> None:
        """CT-L2-004/005 are relations, not standard-provided numbers (REQ §7): checked here, not via provenance."""
        cases = {c["id"]: c["expect"] for c in load()["cases"]}
        for cid in ("S01-smooth-5km-low", "S02-smooth-20km-beyond", "S03-smooth-20km-horizontal", "S04-smooth-uhf",
                    "S05-smooth-irregular-spacing", "S06-sea-ah-negative"):
            e = cases[cid]
            self.assertAlmostEqual(e["Lba"], e["Lbs"], places=9, msg=cid)
            self.assertAlmostEqual(e["L"], max(e["Lsph"], e["Lbs"]), places=9, msg=cid)
        self.assertEqual(cases["C01-clear-short-high"]["L"], 0.0)
        self.assertEqual(cases["C01-clear-short-high"]["sphCase"], "CLEAR")
        provenance = {c["id"]: c["provenance"] for c in load()["cases"]}
        self.assertEqual({k for k, v in provenance.items() if v == "CROSS_CHECKED"},
                         {"O01-single-peak", "O05-valley-terminals", "S06-sea-ah-negative"})  # codex #1330 only
        self.assertNotIn("STD_INVARIANT", provenance.values())  # numbers are ours (REQ §7)

    def test_domain_corners_never_fail_numerically(self) -> None:
        """REQ §2: bounds of every mathematical config field (radius, k, eps, sigma, both polarizations) x frequency x
        path x terrain x antenna extremes give OK and L >= 0 (float64). The capacity fields (maxProfileStepKm,
        maxProfilePoints) are fixed here and covered by test_provider_sampling_rule."""
        lows = {k: (lo if inc else 1.0 + 1e-6) for k, (lo, inc, hi) in o.CONFIG_DOMAIN.items()}
        highs = {k: hi for k, (lo, inc, hi) in o.CONFIG_DOMAIN.items()}
        count = 0
        for er, kf, eps, sig in itertools.product(*[(lows[k], highs[k]) for k in
                                                    ("earthRadiusKm", "kFactor", "permittivity", "conductivitySm")]):
            for pol in o.POLARIZATIONS:
                cfg = {"earthRadiusKm": er, "kFactor": kf, "permittivity": eps, "conductivitySm": sig,
                       "polarization": pol, "maxProfileStepKm": 10.0, "maxProfilePoints": 51}
                for f, d in itertools.product((30.0, 1e5), (0.002, 500.0)):
                    n = 3 if d < 1 else 51
                    for ground, mid in ((-1000.0, 10000.0), (10000.0, -1000.0)):
                        hs = [ground] + [mid] * (n - 2) + [ground]
                        for htg, hrg in ((0.01, 0.01), (0.01, 1e4), (1e4, 0.01), (1e4, 1e4)):
                            r = o.l2({"profile": [[d * i / (n - 1), hs[i]] for i in range(n)], "htg": htg, "hrg": hrg,
                                      "frequencyMHz": f, "config": cfg})
                            self.assertEqual(r["code"], "OK", (cfg, f, d, ground, htg, hrg))
                            self.assertGreaterEqual(r["L"], 0.0)
                            count += 1
        self.assertEqual(count, 16 * 2 * 4 * 2 * 4)  # 1024 corners

    def test_provider_sampling_rule(self) -> None:
        cfg = {"maxProfileStepKm": 0.002, "maxProfilePoints": 100000}
        self.assertEqual(o.provider_sampling(0.0, cfg), ("SKIP", []))
        self.assertEqual(o.provider_sampling(1.9999, cfg)[0], "SKIP")
        code, ds = o.provider_sampling(2.1, cfg)  # n = 3, actual step 1.05 m >= 1 m
        self.assertEqual((code, len(ds)), ("OK", 3))
        for got, want in zip(ds, (0.0, 0.00105, 0.0021)):
            self.assertAlmostEqual(got, want, places=15)
        rng = random.Random(5)
        for _ in range(2000):
            step = rng.choice([0.002, 0.0021, 0.01, 1.0, 10.0, rng.uniform(0.002, 10.0)])
            d_m = rng.choice([2.0, 2.1, step * 1000, rng.uniform(2.0, 500000.0)])
            code, ds = o.provider_sampling(d_m, {"maxProfileStepKm": step, "maxProfilePoints": 100000})
            if code == "UNAVAILABLE":  # e.g. 2 m step over 500 km: more points than the cap, refused up front
                self.assertGreater(math.ceil(d_m / 1000.0 / step) + 1, 100000)
                continue
            self.assertEqual(code, "OK")
            steps = [b - a for a, b in zip(ds, ds[1:])]
            self.assertGreaterEqual(min(steps), 0.001 * (1 - 1e-9))
            self.assertLessEqual(max(steps), step * (1 + 1e-9))
        self.assertEqual(o.provider_sampling(500000.0, {"maxProfileStepKm": 0.002, "maxProfilePoints": 1000}),
                         ("UNAVAILABLE", []))

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
                   "polarization": rng.choice(o.POLARIZATIONS), "maxProfileStepKm": 10.0, "maxProfilePoints": 51}
            d = rng.uniform(0.01, 500.0)
            n = max(3, math.ceil(d / 10.0) + 1, rng.choice([3, 5, 9]))  # spacing <= 10 km, >= 1 m
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

    def test_huge_json_numbers_never_crash(self) -> None:
        """codex #1328: a JSON integer beyond float range is not a number; finite absurd configs are rejected."""
        good = copy.deepcopy(ok_case(load(), "O01-single-peak")["input"])
        for key, value in (("earthRadiusKm", 10 ** 400), ("maxProfilePoints", 10 ** 400), ("kFactor", 1e308),
                           ("earthRadiusKm", 1e308)):
            bad = copy.deepcopy(good)
            bad["config"][key] = value
            self.assertEqual(o.l2(bad), {"code": "INVALID_CONFIG"}, key)
        for key in ("htg", "frequencyMHz"):
            bad = copy.deepcopy(good)
            bad[key] = 10 ** 400
            self.assertNotEqual(o.l2(bad)["code"], "OK", key)
        bad = copy.deepcopy(good)
        bad["profile"][1] = [0.1, 10 ** 400]
        self.assertEqual(o.l2(bad), {"code": "INVALID_PROFILE"})
        bad = copy.deepcopy(good)
        bad["htg"] = bad["hrg"] = 1e-300
        self.assertEqual(o.l2(bad), {"code": "INVALID_ANTENNA_HEIGHT"})

    def test_vector_envelope_is_closed(self) -> None:
        data = load()
        self.assertIn("root keys", o.check_vector_file(dict(data, extra=1)))
        self.assertIn("standard", o.check_vector_file(dict(data, standard="P.526-15")))
        self.assertIn("enforceToleranceDb", o.check_vector_file(dict(data, enforceToleranceDb=0.5)))
        for mutate in (lambda c: c.update(id="E01\nWorkbench.Exit(0);"), lambda c: c.update(note=None),
                       lambda c: c["input"].update(extra=1), lambda c: c["input"].update(htg=10 ** 400),
                       lambda c: c["input"]["config"].update(polarization="x); //"),
                       lambda c: c.update(provenance=["HAND_DERIVED"])):
            broken = copy.deepcopy(data)
            mutate(broken["cases"][0])
            self.assertTrue(any(p.startswith("case") for p in o.check_vector_file(broken)))


if __name__ == "__main__":
    unittest.main()
