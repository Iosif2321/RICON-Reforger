"""P4.3 oracle tests (claude): selection of the through/over full budget, closure statuses and corpus integrity."""
import copy
import json
from pathlib import Path
import unittest

import build_vegetation_path_vectors as bp
import vegetation_path_oracle as p

VECTORS = Path(__file__).resolve().parent / "vectors" / "vegetation_path_v1.json"


class CorpusTest(unittest.TestCase):
    def setUp(self):
        self.doc = json.loads(VECTORS.read_bytes())
        self.cases = {c["id"]: c for c in self.doc["cases"]}
        self.ok = [c["expect"] for c in self.doc["cases"] if c["expect"]["code"] == "OK"]

    def test_vectors_are_built_from_case_list(self):
        self.assertEqual(VECTORS.read_bytes(), bp.build())
        self.assertNotIn(b"\r\n", VECTORS.read_bytes())
        self.assertEqual(0, p.check(VECTORS))
        self.assertEqual(["P18", "P27"], self.doc["hostOnly"])

    def test_every_code_status_and_branch_is_covered(self):
        self.assertEqual(set(p.CODES), {c["expect"]["code"] for c in self.cases.values()})
        self.assertEqual({"CLOSED", "OVER_REFERENCE_ONLY", "OPEN"}, {e["status"] for e in self.ok})
        self.assertEqual({"THROUGH", "OVER"}, {e["selected"] for e in self.ok})
        reasons = {e[b]["reason"] for e in self.ok for b in ("through", "over") if not e[b]["present"]}
        self.assertEqual({"NO_MODEL_FOR_SCENARIO", "BASE_BUDGET_MISSING", "MID_PATH_VEGETATION", "OUT_OF_MODEL_DOMAIN"}, reasons)

    def test_total_is_the_selected_branch_never_a_sum(self):
        """VP06/VP06a: the result is the full budget of the branch selected by the band rule (THROUGH may exceed the
        minimum by at most TIE_DB); the over-canopy correction is never added to a through loss."""
        for e in self.ok:
            chosen = e["through"] if e["selected"] == "THROUGH" else e["over"]
            self.assertTrue(chosen["present"])
            self.assertEqual(chosen["totalDb"], e["totalLossDb"])
            present = [e[b]["totalDb"] for b in ("through", "over") if e[b]["present"]]
            self.assertLessEqual(e["totalLossDb"] - min(present), p.TIE_DB + 1e-9)
            if e["through"]["present"] and e["over"]["present"]:
                diff = p.po.f32(e["through"]["totalDb"] - e["over"]["totalDb"])
                self.assertEqual(e["selected"] == "THROUGH", diff <= p.po.f32(p.TIE_DB))
                self.assertGreater(abs(diff - p.TIE_DB), 0.0009)  # no corpus case sits on the band edge

    def test_closure_rule(self):
        for e in self.ok:
            t, o = e["through"], e["over"]
            both = t["present"] and o["present"] and t["closed"] and o["closed"]
            want = "CLOSED" if both else "OVER_REFERENCE_ONLY" if o["present"] and o["closed"] else "OPEN"
            self.assertEqual(want, e["status"])

    def test_target_cells(self):
        """Deep temperate forest at low VHF: through has no model, over is source-backed -> OVER_REFERENCE_ONLY, not CLOSED."""
        e = self.cases["P02"]["expect"]
        self.assertEqual(("OVER", "OVER_REFERENCE_ONLY", "NO_MODEL_FOR_SCENARIO"), (e["selected"], e["status"], e["through"]["reason"]))
        tie = self.cases["P28"]["expect"]
        self.assertEqual(tie["through"]["totalDb"], tie["over"]["totalDb"])
        self.assertEqual("THROUGH", tie["selected"])
        self.assertEqual("NUMERIC_FAILURE", self.cases["P29"]["expect"]["code"])
        self.assertEqual("THROUGH", self.cases["P30"]["expect"]["selected"])  # codex #3692 near-tie
        self.assertEqual("OVER", self.cases["P31"]["expect"]["selected"])
        jb = self.cases["P04"]["expect"]["through"]
        self.assertEqual("FREE_SPACE_PLUS_JB", jb["budgetMode"])
        self.assertEqual(self.cases["P04"]["input"]["budget"]["fsplDb"], jb["baseDb"])  # no L2 under J&B

    def test_check_detects_corruption(self):
        for cid, mutate in (("P01", lambda e: e.update(status="OVER_REFERENCE_ONLY")),
                            ("P02", lambda e: e.update(selected="THROUGH")),
                            ("P04", lambda e: e["through"].update(baseDb=e["through"]["baseDb"] + 5.0)),
                            ("P06", lambda e: e.update(totalLossDb=e["totalLossDb"] + 1e-3))):
            cases = copy.deepcopy(self.doc["cases"])
            mutate(next(c for c in cases if c["id"] == cid)["expect"])
            errors = []
            for c in cases:
                p.po.close(p.evaluate(c), c["expect"], c["id"], errors, 1e-9)
            self.assertTrue(errors, cid)

    def test_input_is_not_mutated(self):
        inp = bp.case(800, 60.0, [bp.veg(0, 800)], rx_h=3.0)
        before = json.dumps(inp, sort_keys=True)
        p.select(inp)
        self.assertEqual(before, json.dumps(inp, sort_keys=True))


if __name__ == "__main__":
    unittest.main()
