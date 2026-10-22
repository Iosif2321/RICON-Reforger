"""Регрессии P1: редкие просветы нельзя терять из-за среднего или локальной отсечки."""
import copy
import json
import math
from pathlib import Path
import unittest

import channel_cutoff_oracle as cc


class ChannelCutoffTest(unittest.TestCase):
    def test_frozen_values_and_corruption(self):
        path = Path(__file__).with_name("vectors") / "channel_cutoff_v1.json"
        data = json.loads(path.read_text(encoding="utf-8"))
        self.assertEqual(cc.check(data), [])
        data["cases"][0]["expect"]["usefulIsZero"] = False
        self.assertTrue(cc.check(data))

    def test_coherent_sum_not_individual_pruning(self):
        q = cc.baseline()
        q["paths"] = [{"pathId": "a", "upperDbm": -125}, {"pathId": "b", "upperDbm": -125}]
        out = cc.evaluate(q)
        self.assertEqual(out["decision"], "CONTINUE")
        self.assertAlmostEqual(out["upperDbm"], -125 + 20 * math.log10(2))

    def test_invalidation_and_expiry_allow_recovery(self):
        q = cc.baseline()
        self.assertTrue(cc.evaluate(q)["usefulIsZero"])
        q["nowMs"] = 200
        self.assertEqual(cc.evaluate(q)["reason"], "STALE_BOUND")
        q.update(validFromMs=200, validUntilMs=400, environmentRevision=2, boundRevision=2)
        q["paths"][0]["upperDbm"] = -90
        self.assertFalse(cc.evaluate(q)["usefulIsZero"])

    def test_no_bound_is_not_zero_or_physical_loss(self):
        q = cc.baseline()
        q["paths"][0]["upperDbm"] = None
        self.assertEqual(cc.evaluate(q), {"decision": "CONTINUE", "reason": "MISSING_PATH_BOUND",
                                         "usefulIsZero": False, "upperDbm": None})

    def test_inputs_unchanged_and_result_independent(self):
        for row in cc.cases():
            before = copy.deepcopy(row["input"])
            out = cc.evaluate(row["input"])
            out["decision"] = "changed"
            self.assertEqual(before, row["input"], row["id"])

    def test_global_validation_and_precedence(self):
        q = cc.baseline()
        q.update(resourceExhausted=True, coverageComplete=False, boundRevision=2)
        self.assertEqual(cc.evaluate(q)["reason"], "RESOURCE_EXHAUSTED")
        q["paths"].append({"pathId": "bad", "upperDbm": "NaN"})
        self.assertEqual(cc.evaluate(q)["reason"], "INVALID_INPUT")

    def test_boundary_strict_and_optional_fields(self):
        q = cc.baseline()
        q["paths"][0]["upperDbm"] = q["toleranceDbm"]
        self.assertFalse(cc.evaluate(q)["usefulIsZero"])
        q["toleranceDbm"] = None
        self.assertEqual(cc.evaluate(q)["reason"], "CUTOFF_DISABLED")
        q["numericalGuardDb"] = None
        self.assertEqual(cc.evaluate(q)["reason"], "NUMERIC_UNCERTIFIED")

    def test_reordering_preserves_bound(self):
        q = cc.baseline()
        q["paths"] = [{"pathId": str(i), "upperDbm": -100-i*10} for i in range(30)]
        forward = cc.evaluate(q)
        q["paths"].reverse()
        self.assertEqual(cc.evaluate(q), forward)

    def test_path_limit(self):
        q = cc.baseline()
        q["paths"] = [{"pathId": str(i), "upperDbm": -150} for i in range(4097)]
        self.assertEqual(cc.evaluate(q)["reason"], "INVALID_INPUT")


if __name__ == "__main__":
    unittest.main()
