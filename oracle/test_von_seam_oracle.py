#!/usr/bin/env python3
"""Self-test of the VoN seam oracle: vectors pass, format is checked, tampering and priority errors are detected."""

from __future__ import annotations

import copy
import itertools
import json
import unittest
from pathlib import Path

import von_seam_oracle as vs

VECTORS = Path(__file__).parent / "vectors" / "von_seam_v1.json"


def load() -> dict:
    return json.loads(VECTORS.read_text(encoding="utf-8"))


def failures(data: dict) -> list[str]:
    return [failure for case in data["cases"] for failure in vs.run_case(case)]


def first_call(data: dict, op: str) -> dict:
    return next(step for case in data["cases"] for step in case["calls"] if step["op"] == op)


class VonSeamOracleTest(unittest.TestCase):
    def test_vectors_pass(self) -> None:
        data = load()
        self.assertEqual(vs.check_vector_file(data), [])
        self.assertEqual(failures(data), [])

    def test_every_vector_acceptance_test_is_covered(self) -> None:
        covered = {ct for case in load()["cases"] for ct in case["ct"]}
        self.assertEqual(covered, {f"CT-VON-S-{n:03d}" for n in range(1, 10)})  # 010 is a code scan

    def test_every_decision_is_expected_somewhere(self) -> None:
        expected = {step["expect"] for case in load()["cases"] for step in case["calls"]}
        self.assertEqual(expected, set(vs.DECISIONS))

    def test_source_names_the_req_revision(self) -> None:
        self.assertRegex(load()["source"], r"^docs/req/REQ-von-seam\.md@[0-9a-f]{7,}$")

    def test_changed_expectation_is_detected(self) -> None:
        data = load()
        first_call(data, "apply")["expect"] = "BLOCKED_NO_ACTUATOR" if first_call(data, "apply")["expect"] \
            != "BLOCKED_NO_ACTUATOR" else "NULL_INPUT"
        self.assertEqual(len(failures(data)), 1)

    def test_format_errors_are_reported(self) -> None:
        data = load()
        broken = copy.deepcopy(data)
        broken["cases"][0]["calls"][0]["expect"] = "OK"  # §3 has no OK on purpose
        broken["cases"][1]["id"] = broken["cases"][0]["id"]
        broken["cases"][2]["calls"][0]["extra"] = 1
        problems = vs.check_vector_file(broken)
        self.assertTrue(any("expect" in p for p in problems))
        self.assertTrue(any("case id" in p for p in problems))
        self.assertTrue(any("call structure" in p for p in problems))
        self.assertEqual(vs.check_vector_file({"schema": "x", "version": 1}), ["schema/version"])
        header = {"schema": "ricon.oracle.von_seam", "version": 1, "source": "docs/req/REQ-von-seam.md@2bf70a7"}
        self.assertIn("cases: non-empty list required", vs.check_vector_file(dict(header, cases=[])))
        self.assertIn("case structure", vs.check_vector_file(dict(header, cases=[{"id": "x", "calls": []}])))

    def test_mutating_seam_is_caught(self) -> None:
        original = vs.apply

        def mutating(snapshot, binding, upstream):
            binding["talkerLeaseId"] = 2
            return original(snapshot, binding, upstream)

        vs.apply = mutating
        try:
            with self.assertRaises(AssertionError):
                vs.call(first_call(load(), "apply"))
        finally:
            vs.apply = original

    def test_hostile_input_is_rejected_without_crash(self) -> None:
        data = load()
        valid = next(step for case in data["cases"] for step in case["calls"]
                     if step["op"] == "apply" and step["expect"] == "BLOCKED_NO_ACTUATOR")
        snap, bind = vs.decode(valid["snapshot"]), valid["binding"]
        self.assertEqual(vs.apply([1, 2], bind, "ACCEPT_NEWER"), "INVALID_SNAPSHOT")
        self.assertEqual(vs.apply(snap, "binding", "ACCEPT_NEWER"), "INVALID_BINDING")
        self.assertEqual(vs.apply(dict(snap, talkerPlayerId=True), bind, "ACCEPT_NEWER"), "INVALID_SNAPSHOT")
        self.assertEqual(vs.apply(dict(snap, talkerPlayerId=1.0), bind, "ACCEPT_NEWER"), "INVALID_SNAPSHOT")
        self.assertEqual(vs.apply(snap, dict(bind, talkerLeaseId=True), "ACCEPT_NEWER"), "INVALID_BINDING")
        self.assertEqual(vs.apply(snap, dict(bind, talkerLeaseId=2 ** 31), "ACCEPT_NEWER"), "INVALID_BINDING")
        missing = dict(bind)
        del missing["listenerTransceiverIndex"]
        self.assertEqual(vs.apply(snap, missing, "ACCEPT_NEWER"), "INVALID_BINDING")
        self.assertEqual(vs.apply(snap, bind, None), "DECISION_NOT_ACCEPTED")
        self.assertEqual(vs.expire("7", 1, 11, 1, bind), "INVALID_EXPIRY")
        self.assertEqual(vs.expire(7, 1, 11, None, bind), "INVALID_EXPIRY")

    def test_untypeable_records_fail_closed(self) -> None:
        """codex #1172: extra keys and unhashable values have no typed Enforce form and must not pass or crash."""
        data = load()
        valid = next(step for case in data["cases"] for step in case["calls"]
                     if step["op"] == "apply" and step["expect"] == "BLOCKED_NO_ACTUATOR")
        snap, bind = vs.decode(valid["snapshot"]), valid["binding"]
        self.assertEqual(vs.apply(dict(snap, extra=1), bind, "ACCEPT_NEWER"), "INVALID_SNAPSHOT")
        self.assertEqual(vs.apply(snap, dict(bind, extra=1), "ACCEPT_NEWER"), "INVALID_BINDING")
        self.assertEqual(vs.expire(7, 1, 11, 1, dict(bind, extra=1)), "INVALID_BINDING")
        for upstream in ([], {}, 1.5, ["ACCEPT_NEWER"]):
            self.assertEqual(vs.apply(snap, bind, upstream), "DECISION_NOT_ACCEPTED", upstream)
        broken = copy.deepcopy(data)
        broken["cases"][0]["calls"][0]["expect"] = []
        broken["cases"][1]["calls"][0]["expect"] = {"x": 1}
        self.assertEqual(sum("expect" in p for p in vs.check_vector_file(broken)), 2)

    def test_vector_envelope_is_strict(self) -> None:
        data = load()
        for source in (data["source"] + "\nWorkbench.Exit(0);", data["source"] + " ", "docs/req/REQ-von-seam.md@xyz",
                       "x" + data["source"], None):
            self.assertIn("source", vs.check_vector_file(dict(data, source=source)), repr(source))
        mutations = [
            lambda c: c.update(extra=1),
            lambda c: c.update(ct=[]),
            lambda c: c.update(ct=["CT-LC-001"]),
            lambda c: c.update(req=[""]),
            lambda c: c.update(note=None),
            lambda c: c.pop("req"),
        ]
        for mutate in mutations:
            broken = copy.deepcopy(data)
            mutate(broken["cases"][0])
            self.assertIn("case structure", vs.check_vector_file(broken))
        broken = copy.deepcopy(data)
        broken["cases"][0]["id"] = "A01\nWorkbench.Exit(0);"
        self.assertTrue(any("case id" in p for p in vs.check_vector_file(broken)))

    def test_apply_priority_holds_for_every_combination(self) -> None:
        """§5 order is global: with any subset of failures the first one in REQ order wins."""
        data = load()
        valid = next(step for case in data["cases"] for step in case["calls"]
                     if step["op"] == "apply" and step["expect"] == "BLOCKED_NO_ACTUATOR")
        snap, bind = vs.decode(valid["snapshot"]), valid["binding"]
        order = ["NULL_INPUT", "INVALID_SNAPSHOT", "DECISION_NOT_ACCEPTED", "INVALID_BINDING", "IDENTITY_MISMATCH"]
        for flags in itertools.product((False, True), repeat=len(order)):
            null, bad_snap, bad_up, bad_bind, mismatch = flags
            s = dict(snap, linkSeq=0) if bad_snap else dict(snap)
            b = dict(bind, talkerTransceiverIndex=-1) if bad_bind else dict(bind)
            if mismatch:
                b["linkId"] = s["linkId"] + 1
            up = "STALE" if bad_up else "ACCEPT_NEWER"
            expected = next((name for name, flag in zip(order, flags) if flag), "BLOCKED_NO_ACTUATOR")
            actual = vs.apply(None if null else s, b, up)
            self.assertEqual(actual, expected, flags)

    def test_expire_priority_holds_for_every_combination(self) -> None:
        data = load()
        bind = next(step for case in data["cases"] for step in case["calls"]
                    if step["op"] == "expire" and step["expect"] == "BLOCKED_NO_ACTUATOR")["binding"]
        order = ["NULL_INPUT", "INVALID_EXPIRY", "INVALID_BINDING", "IDENTITY_MISMATCH"]
        for flags in itertools.product((False, True), repeat=len(order)):
            null, bad_ids, bad_bind, mismatch = flags
            b = dict(bind, listenerDeviceId=bind["talkerDeviceId"]) if bad_bind else dict(bind)
            lease = 0 if bad_ids else b["talkerLeaseId"]
            player = b["listenerPlayerId"] if mismatch else b["talkerPlayerId"]  # valid ID of the wrong side
            expected = next((name for name, flag in zip(order, flags) if flag), "BLOCKED_NO_ACTUATOR")
            actual = vs.expire(b["sessionEpoch"], player, b["talkerDeviceId"], lease, None if null else b)
            self.assertEqual(actual, expected, flags)

    def test_decision_names_and_values_are_closed(self) -> None:
        self.assertEqual(sorted(vs.DECISIONS.values()), list(range(7)))
        self.assertNotIn("OK", vs.DECISIONS)
        for case in load()["cases"]:
            for step in case["calls"]:
                self.assertIn(step["expect"], vs.DECISIONS)


if __name__ == "__main__":
    unittest.main()
