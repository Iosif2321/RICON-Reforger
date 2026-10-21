"""Проверки S3b: известные значения, цепочки тактов и порча результатов."""
import copy
import json
import random
import unittest
from pathlib import Path

import link_publication_oracle as lp
import snapshot_oracle as so


class LinkPublicationTest(unittest.TestCase):
    def test_document_matches_and_detects_corruption(self):
        path = Path(__file__).with_name("vectors") / "link_publication_v1.json"
        data = json.loads(path.read_text(encoding="utf-8"))
        self.assertEqual(lp.check(data), [])
        data["cases"][0]["expect"]["snapshots"][0]["linkSeq"] += 1
        self.assertTrue(lp.check(data))

    def test_first_snapshot_exact_values(self):
        out = lp.evaluate(*lp.baseline())
        self.assertEqual(out["callCode"], "OK")
        self.assertEqual(out["snapshots"], [{"schemaVersion": 1, "sessionEpoch": 7, "linkId": 1,
            "linkSeq": 1, "ttlMs": 1000, "routeId": 1, "routeNodeCount": 2, "talkerPlayerId": 1,
            "listenerPlayerId": 2, "state": "CLEAN", "quality": 0.75}])
        self.assertEqual((out["sequenceCalls"], out["epochReads"], out["requiresFullResync"]), (1, 2, True))
        self.assertEqual(out["registry"]["highWaterLinkId"], 1)
        self.assertEqual(out["registry"]["lastTickMs"], 100)

    def test_all_codes_have_cases_and_preflight_never_touches_source(self):
        cases = lp.build_cases()
        self.assertEqual({c["expect"]["callCode"] for c in cases}, set(lp.CALL_CODES))
        self.assertEqual(len({c["id"] for c in cases}), len(cases))
        for c in cases:
            e = c["expect"]
            if e["callCode"] not in ("OK", "NEED_FULL_RESYNC"):
                self.assertEqual((e["snapshots"], e["sequenceCalls"], e["epochReads"]), ([], 0, 0), c["id"])
                self.assertEqual(e["registry"], c["registry"], c["id"])
            elif e["callCode"] == "OK":
                self.assertEqual(e["epochReads"], e["sequenceCalls"] + 1, c["id"])
                self.assertTrue(all(so.is_valid_snapshot(s) for s in e["snapshots"]), c["id"])

    def test_quality_changes_accumulate_from_last_published(self):
        inp, reg, spec = lp.established()
        for quality in (0.71875, 0.6875, 0.65625):
            inp["pairResult"]["pairs"][0]["intelligibility"] = quality
            out = lp.evaluate(inp, reg, spec)
            self.assertEqual(out["snapshots"], [])
            reg = out["registry"]
            self.assertEqual(reg["entries"][0]["quality"], 0.75)
            inp["nowMs"] += 1
        inp["pairResult"]["pairs"][0]["intelligibility"] = 0.625
        out = lp.evaluate(inp, reg, spec)
        self.assertEqual(out["snapshots"][0]["quality"], 0.625)
        self.assertEqual(out["snapshots"][0]["linkSeq"], 2)

    def test_terminal_then_new_ptt_never_reuses_id(self):
        inp, reg, spec = lp.established()
        inp["bindings"], inp["pairResult"]["pairs"] = [], []
        out = lp.evaluate(inp, reg, spec)
        self.assertEqual([(s["linkId"], s["linkSeq"], s["state"], s["quality"]) for s in out["snapshots"]],
                         [(1, 2, "LOST", 0.0)])
        self.assertEqual(out["registry"]["entries"], [])
        inp["nowMs"] += 1
        inp["bindings"], inp["pairResult"]["pairs"] = [lp.binding(lease=12)], [lp.pair(lease=12)]
        out = lp.evaluate(inp, out["registry"], out["sourceState"])
        self.assertEqual(out["snapshots"][0]["linkId"], 2)

    def test_invalid_budget_is_not_a_new_link(self):
        inp, reg, spec = lp.baseline()
        inp["pairResult"]["pairs"] = [lp.pair(valid=False)]
        out = lp.evaluate(inp, reg, spec)
        self.assertEqual((out["snapshots"], out["registry"]["highWaterLinkId"], out["sequenceCalls"]), ([], 0, 0))
        inp, reg, spec = lp.established()
        inp["pairResult"]["pairs"] = [lp.pair(valid=False)]
        out = lp.evaluate(inp, reg, spec)
        self.assertEqual(out["snapshots"][0]["state"], "LOST")
        self.assertEqual(len(out["registry"]["entries"]), 1)

    def test_odd_ttl_and_integer_overflow_boundaries(self):
        by_id = {c["id"]: c["expect"] for c in lp.build_cases()}
        self.assertEqual([by_id[k]["sequenceCalls"] for k in ("T01", "T02", "T03", "T04", "T05", "T06", "T07")],
                         [0, 1, 0, 1, 1, 0, 1])
        self.assertEqual(by_id["E01"]["snapshots"][0]["linkId"], lp.MAX)
        self.assertEqual(by_id["E10"]["snapshots"][0]["linkSeq"], lp.MAX-1)
        self.assertEqual(by_id["E11"]["callCode"], "NEED_FULL_RESYNC")

    def test_failure_drops_all_and_consumes_preallocated_ids(self):
        inp, reg, spec = lp.baseline()
        inp["pairResult"]["pairs"] += [lp.pair(index=1), lp.pair(index=2)]
        spec["failAt"] = 2
        out = lp.evaluate(inp, reg, spec)
        self.assertEqual((out["callCode"], out["snapshots"], out["sequenceCalls"], out["epochReads"]),
                         ("NEED_FULL_RESYNC", [], 2, 3))
        self.assertEqual(out["registry"], lp.empty_registry(7, 3))
        self.assertEqual(out["sourceState"]["sequences"], {"1": 1})
        out = lp.evaluate(inp, out["registry"], out["sourceState"])
        self.assertEqual([s["linkId"] for s in out["snapshots"]], [4, 5, 6])

    def test_rotation_drops_first_snapshot_and_restarts_new_epoch(self):
        inp, reg, spec = lp.baseline()
        inp["pairResult"]["pairs"] += [lp.pair(index=1)]
        spec["rotateAt"] = 2
        out = lp.evaluate(inp, reg, spec)
        self.assertEqual(out["snapshots"], [])
        self.assertEqual(out["registry"], lp.empty_registry(8))
        self.assertEqual(out["sourceState"], {"epoch": 8, "sequences": {"2": 1}})
        inp["sessionEpoch"] = 8
        out = lp.evaluate(inp, out["registry"], out["sourceState"])
        self.assertEqual(out["callCode"], "OK")
        self.assertTrue(out["requiresFullResync"])
        self.assertEqual({s["sessionEpoch"] for s in out["snapshots"]}, {8})

    def test_immutable_inputs_and_detached_outputs(self):
        inp, reg, spec = lp.established()
        inp["nowMs"] = 600
        before = copy.deepcopy((inp, reg, spec))
        source = lp.SequenceSource(spec)
        out = lp.publish(inp, reg, source)
        self.assertEqual((inp, reg, spec), before)
        out["snapshots"][0]["quality"] = 0.0
        self.assertEqual(out["registry"]["entries"][0]["quality"], 0.75)
        out["registry"]["entries"][0]["quality"] = 0.125
        self.assertEqual((inp, reg, spec), before)

    def test_source_counters_are_per_call_on_reused_fake(self):
        inp, reg, spec = lp.baseline()
        source = lp.SequenceSource(spec)
        first = lp.publish(inp, reg, source)
        inp["nowMs"] = 600
        second = lp.publish(inp, first["registry"], source)
        self.assertEqual((second["sequenceCalls"], second["epochReads"]), (1, 2))
        self.assertEqual((source.calls, source.reads), (2, 4))

    def test_order_and_full_resync_receiver_projection(self):
        inp, reg, spec = lp.established()
        reg["needsFullResync"] = True
        inp["bindings"] += [lp.binding(lease=12, tx=103, player=3)]
        inp["pairResult"]["pairs"] = [lp.pair(lease=12, tx=103, player=4, rx=204)]
        out = lp.evaluate(inp, reg, spec)
        self.assertEqual([s["linkId"] for s in out["snapshots"]], [1, 2])
        live_ids = {e["linkId"] for e in out["registry"]["entries"]}
        live = [s for s in out["snapshots"] if s["linkId"] in live_ids]
        rx = so.Receiver(1)
        rx.full_resync(1000, 7, 1, live)
        self.assertEqual(set(rx.slots), {2})
        rnd = random.Random(9)
        rnd.shuffle(inp["bindings"]); rnd.shuffle(inp["pairResult"]["pairs"])
        self.assertEqual(lp.evaluate(inp, reg, spec), out)

    def test_s3_frozen_result_connects_without_recalculation(self):
        path = Path(__file__).with_name("vectors") / "pair_service_v1.json"
        c = json.loads(path.read_text(encoding="utf-8"))["cases"][0]
        inp, reg, spec = lp.baseline()
        inp["bindings"] = copy.deepcopy(c["input"]["bindings"])
        inp["pairResult"] = copy.deepcopy(c["expect"])
        out = lp.evaluate(inp, reg, spec)
        self.assertEqual(out["callCode"], "OK")
        eligible = [p for p in c["expect"]["pairs"] if p["pairCode"] == "EVALUATED" and p["valid"]]
        self.assertEqual(len(out["snapshots"]), len(eligible))
        self.assertTrue(eligible)


if __name__ == "__main__":
    unittest.main()
