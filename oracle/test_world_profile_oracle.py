"""Тесты эталона поставщика профиля из мира (REQ-L2-world-profile v0.2). Запуск:
python -m unittest discover -s oracle -p "test_*.py"
"""

from __future__ import annotations

import copy
import json
import math
import unittest
from pathlib import Path

import l2_terrain_oracle as l2o
import world_profile_oracle as wp

VECTORS = Path(__file__).resolve().parent / "vectors" / "world_profile_v1.json"


def by_id() -> dict[str, dict]:
    return {c["id"]: c for c in wp.compute(wp.build_cases())}


class RecordingSource(wp.SyntheticSource):
    def __init__(self, spec):
        super().__init__(spec)
        self.points = []

    def try_get_height(self, x, z):
        self.points.append((x, z))
        return super().try_get_height(x, z)


class WorldProfileTest(unittest.TestCase):
    def test_vector_file_is_current(self):
        data = json.loads(VECTORS.read_text(encoding="utf-8"))
        self.assertEqual(wp.check(data), [])

    def test_check_detects_tampering(self):
        data = json.loads(VECTORS.read_text(encoding="utf-8"))
        bad = copy.deepcopy(data)
        bad["cases"][0]["expect"]["layers"]["terrain_diffraction"] += 1e-6
        self.assertTrue(wp.check(bad))
        bad = copy.deepcopy(data)
        bad["cases"][8]["expect"]["heightCalls"] += 1
        self.assertTrue(wp.check(bad))
        bad = copy.deepcopy(data)
        bad["enforceTolerance"]["lossDb"] = 0.5
        self.assertTrue(wp.check(bad))

    def test_flat_matches_direct_l2(self):
        e = by_id()["W01"]["expect"]
        self.assertTrue(all(h == 100.0 for _, h in e["profile"]))
        direct = l2o.l2({"profile": e["profile"], "htg": 1.5, "hrg": 1.5, "frequencyMHz": 50.0, "config": wp.CFG})
        self.assertEqual(direct["L"], e["layers"]["terrain_diffraction"])
        self.assertEqual(by_id()["W02"]["expect"]["layers"], e["layers"])

    def test_reverse_path_reverses_profile(self):
        cases = by_id()
        fwd, rev = cases["W03"]["expect"], cases["W04"]["expect"]
        self.assertEqual([d for d, _ in fwd["profile"]], [d for d, _ in rev["profile"]])
        for (_, a), (_, b) in zip(fwd["profile"], reversed(rev["profile"])):
            self.assertAlmostEqual(a, b, places=9)

    def test_endpoints_are_exact_antenna_positions(self):
        src = RecordingSource({"base": 0.0})
        tx, rx = [0.1, 2.0, -7.3], [812.7, 2.0, 593.9]
        out = wp.get_profile(tx, rx, wp.CFG, src)
        self.assertEqual(out["reason"], wp.OK)
        self.assertEqual(src.points[0], (tx[0], tx[2]))
        self.assertEqual(src.points[-1], (rx[0], rx[2]))
        self.assertEqual(len(src.points), len(out["profile"]))

    def test_success_costs_exactly_n_calls(self):
        for cid, c in by_id().items():
            e = c["expect"]
            if e["providerReason"] == wp.OK:
                self.assertEqual(e["heightCalls"], e["n"], cid)

    def test_ground_below_zero_is_used_as_is(self):
        e = by_id()["W07"]["expect"]
        self.assertTrue(all(h == -20.0 for _, h in e["profile"]))
        self.assertEqual((e["htg"], e["hrg"]), (2.0, 2.0))

    def test_refusals_before_sampling(self):
        cases = by_id()
        for cid in ("W12", "W13", "W16", "W22") + tuple(f"P{k:02d}" for k in range(2, 18)):
            self.assertEqual(cases[cid]["expect"]["heightCalls"], 0, cid)
            self.assertEqual(cases[cid]["expect"]["providerReason"],
                             wp.NOT_CALLED if cid == "W16" else wp.UNAVAILABLE, cid)
        self.assertEqual(cases["W14"]["expect"]["providerReason"], wp.NOT_CALLED)
        self.assertEqual(cases["P01"]["expect"]["providerReason"], wp.OK)

    def test_nonfinite_positions_cover_every_component(self):
        seen = set()
        for c in wp.build_cases():
            if c["level"] == "provider":
                for end in ("txPos", "rxPos"):
                    for axis, v in enumerate(c["input"][end]):
                        if isinstance(v, str):
                            seen.add((end, axis))
        self.assertEqual(seen, {(e, a) for e in ("txPos", "rxPos") for a in range(3)})

    def test_first_failure_stops_sampling(self):
        cases = by_id()
        self.assertEqual(cases["W08"]["expect"]["heightCalls"], 1)
        self.assertEqual(cases["W09"]["expect"]["heightCalls"], 11)
        self.assertEqual(cases["W10"]["expect"]["heightCalls"], 21)
        self.assertEqual(cases["W11"]["expect"]["heightCalls"], 8)
        for cid in ("W08", "W09", "W10", "W11"):
            self.assertEqual(cases[cid]["expect"]["modelReason"], wp.UNAVAILABLE)
            self.assertNotIn("profile", cases[cid]["expect"])

    def test_antenna_domain_is_checked_by_l2_only(self):
        e = by_id()["W17"]["expect"]
        self.assertEqual((e["providerReason"], e["l2Code"], e["modelReason"]),
                         (wp.OK, "INVALID_ANTENNA_HEIGHT", wp.UNAVAILABLE))
        self.assertEqual(e["htg"], -1.0)
        self.assertEqual(by_id()["W18"]["expect"]["l2Code"], "INVALID_PROFILE")

    def test_nonfinite_antenna_height_is_refused(self):
        src = wp.SyntheticSource({"base": -1e308})
        out = wp.get_profile([0.0, 1e308, 0.0], [1000.0, 2.0, 0.0], wp.CFG, src)
        self.assertEqual(out, {"reason": wp.UNAVAILABLE})
        self.assertEqual(src.calls, 11)

    def test_fspl_uses_3d_distance(self):
        cases = by_id()
        self.assertEqual(cases["W14"]["expect"]["layers"]["fspl"],
                         wp.ro.free_space_loss(math.dist([0, 10, 0], [1, 30, 1]), 50.0)["lossDb"])
        self.assertEqual(cases["W20"]["expect"]["layers"]["fspl"],
                         wp.ro.free_space_loss(math.hypot(1000, 300), 50.0)["lossDb"])

    def test_hill_compared_with_flat_is_recorded_not_assumed(self):
        """CT-WP-003: the vectors record W05 < W06 (D-002 effect); this pins the fact, it is not a requirement."""
        cases = by_id()
        hill = cases["W05"]["expect"]["layers"]["terrain_diffraction"]
        flat = cases["W06"]["expect"]["layers"]["terrain_diffraction"]
        self.assertLess(hill, flat)
        self.assertGreater(max(h for _, h in cases["W05"]["expect"]["profile"]), 79.0)

    def test_unknown_source_key_and_json_strictness(self):
        with self.assertRaises(ValueError):
            wp.SyntheticSource({"bse": 0.0})
        text = VECTORS.read_text(encoding="utf-8")
        self.assertNotIn("NaN,", text.replace('"NaN"', ""))
        self.assertNotIn("Infinity", text)


if __name__ == "__main__":
    unittest.main()
