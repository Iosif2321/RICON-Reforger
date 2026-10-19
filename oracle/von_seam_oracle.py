#!/usr/bin/env python3
"""Host-эталон заготовки VoN seam по docs/req/REQ-von-seam.md (stdlib). Не production-код.

Seam stateless: `apply` проверяет accepted LinkSnapshot и binding, `expire` — запоздалое истечение lease против
текущего binding. Любой валидный вход даёт `BLOCKED_NO_ACTUATOR` (§3): это не доставка голоса. Раннер сверяет
решения с `oracle/vectors/von_seam_v1.json`, где ожидания выписаны вручную по тексту REQ.
"""

from __future__ import annotations

import argparse
import copy
import json
import sys
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parent))  # own directory only: reuse the snapshot domain (§8.3)
from snapshot_oracle import decode, is_valid_snapshot  # noqa: E402

INT_MAX = 2147483647

# §3: closed decision enum with its integer values.
DECISIONS = {
    "NULL_INPUT": 0, "INVALID_SNAPSHOT": 1, "DECISION_NOT_ACCEPTED": 2, "INVALID_BINDING": 3,
    "IDENTITY_MISMATCH": 4, "INVALID_EXPIRY": 5, "BLOCKED_NO_ACTUATOR": 6,
}
# Upstream receiver decisions (REQ-core §8.3, RICON_ESnapshotDecision); only three allow application (§2.3, §5.3).
UPSTREAM = ("ACCEPT_FIRST", "ACCEPT_NEWER", "STALE", "INVALID_SNAPSHOT", "INVALID_IDENTITY", "ACCEPT_RESYNC",
            "INVALID_RESYNC")
ACCEPTED = frozenset(("ACCEPT_FIRST", "ACCEPT_NEWER", "ACCEPT_RESYNC"))
BINDING_KEYS = ("sessionEpoch", "linkId", "talkerPlayerId", "listenerPlayerId", "talkerDeviceId", "listenerDeviceId",
                "talkerLeaseId", "talkerTransceiverIndex", "listenerTransceiverIndex")
IDENTITY_KEYS = ("sessionEpoch", "linkId", "talkerPlayerId", "listenerPlayerId")  # §5.5


def is_int(value: Any) -> bool:
    return isinstance(value, int) and not isinstance(value, bool)


def is_positive(value: Any) -> bool:
    """Positive int32: 1..int.MAX."""
    return is_int(value) and 1 <= value <= INT_MAX


def is_epoch(value: Any) -> bool:
    return is_int(value) and 1 <= value <= INT_MAX - 1


def is_index(value: Any) -> bool:
    """Explicit non-negative transceiver index; the sentinel -1 is invalid (§4)."""
    return is_int(value) and 0 <= value <= INT_MAX


def snapshot_ok(snapshot: Any) -> bool:
    """§5.2: the §8.3 domain plus positive, distinct player IDs (seam rule, stronger than REQ-core §8.3)."""
    if not is_valid_snapshot(snapshot):
        return False
    talker, listener = snapshot.get("talkerPlayerId"), snapshot.get("listenerPlayerId")
    return is_positive(talker) and is_positive(listener) and talker != listener


def binding_ok(binding: Any) -> bool:
    """§4 field domains."""
    if not isinstance(binding, dict) or any(key not in binding for key in BINDING_KEYS):
        return False
    b = binding
    return (is_epoch(b["sessionEpoch"]) and is_positive(b["linkId"])
            and is_positive(b["talkerPlayerId"]) and is_positive(b["listenerPlayerId"])
            and b["talkerPlayerId"] != b["listenerPlayerId"]
            and is_positive(b["talkerDeviceId"]) and is_positive(b["listenerDeviceId"])
            and b["talkerDeviceId"] != b["listenerDeviceId"]
            and is_positive(b["talkerLeaseId"])
            and is_index(b["talkerTransceiverIndex"]) and is_index(b["listenerTransceiverIndex"]))


def apply(snapshot: Any, binding: Any, upstream: Any) -> str:
    """§5, global order. An unknown upstream value fails closed as DECISION_NOT_ACCEPTED (§7)."""
    if snapshot is None or binding is None:
        return "NULL_INPUT"
    if not snapshot_ok(snapshot):
        return "INVALID_SNAPSHOT"
    if upstream not in ACCEPTED:
        return "DECISION_NOT_ACCEPTED"
    if not binding_ok(binding):
        return "INVALID_BINDING"
    if any(binding[key] != snapshot[key] for key in IDENTITY_KEYS):
        return "IDENTITY_MISMATCH"
    return "BLOCKED_NO_ACTUATOR"


def expire(epoch: Any, player_id: Any, device_id: Any, lease_id: Any, binding: Any) -> str:
    """§6: the quartet (epoch of issue, player, device, lease) must equal the talker side of the current binding."""
    if binding is None:
        return "NULL_INPUT"
    if not is_epoch(epoch) or not all(is_positive(v) for v in (player_id, device_id, lease_id)):
        return "INVALID_EXPIRY"
    if not binding_ok(binding):
        return "INVALID_BINDING"
    if (epoch, player_id, device_id, lease_id) != (binding["sessionEpoch"], binding["talkerPlayerId"],
                                                   binding["talkerDeviceId"], binding["talkerLeaseId"]):
        return "IDENTITY_MISMATCH"
    return "BLOCKED_NO_ACTUATOR"


def call(step: dict[str, Any]) -> str:
    """One vector call. Inputs are deep-copied and compared afterwards: §7 forbids any mutation."""
    args = decode(copy.deepcopy(step))
    before = copy.deepcopy(args)
    if args["op"] == "apply":
        result = apply(args["snapshot"], args["binding"], args["upstreamDecision"])
    else:
        result = expire(args["sessionEpoch"], args["playerId"], args["deviceId"], args["leaseId"], args["binding"])
    if args != before:
        raise AssertionError("seam mutated its input")
    return result


# ---------------------------------------------------------------------- runner

APPLY_KEYS = {"op", "snapshot", "binding", "upstreamDecision", "expect"}
EXPIRE_KEYS = {"op", "sessionEpoch", "playerId", "deviceId", "leaseId", "binding", "expect"}


def check_vector_file(data: Any) -> list[str]:
    """Format checks of von_seam_v1.json; returns problems (empty = valid)."""
    if not isinstance(data, dict) or data.get("schema") != "ricon.oracle.von_seam" or data.get("version") != 1:
        return ["schema/version"]
    problems = []
    if not str(data.get("source", "")).startswith("docs/req/REQ-von-seam.md@"):
        problems.append("source")
    cases = data.get("cases")
    if not isinstance(cases, list) or not cases:
        return problems + ["cases: non-empty list required"]
    seen = set()
    for case in cases:
        if not isinstance(case, dict) or not isinstance(case.get("calls"), list) or not case["calls"] \
                or not isinstance(case.get("ct"), list) or not case["ct"]:
            problems.append("case structure")
            continue
        cid = case.get("id")
        if not isinstance(cid, str) or cid in seen:
            problems.append(f"case id {cid!r}")
        seen.add(cid)
        for step in case["calls"]:
            keys = APPLY_KEYS if isinstance(step, dict) and step.get("op") == "apply" else EXPIRE_KEYS
            if not isinstance(step, dict) or step.get("op") not in ("apply", "expire") or set(step) != keys:
                problems.append(f"{cid}: call structure")
            elif step["expect"] not in DECISIONS:
                problems.append(f"{cid}: expect {step['expect']!r}")
    return problems


def run_case(case: dict[str, Any]) -> list[str]:
    failures = []
    for index, step in enumerate(case["calls"]):
        actual = call(step)
        if actual != step["expect"]:
            failures.append(f"{case['id']} call {index} ({step['op']}): expected {step['expect']}, actual {actual}")
    return failures


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", required=True, metavar="VECTORS_JSON")
    args = parser.parse_args(argv)
    data = json.loads(Path(args.check).read_text(encoding="utf-8"))
    problems = check_vector_file(data)
    if problems:
        print("FORMAT_FAIL:", *problems, sep="\n  ")
        return 1
    failures = [failure for case in data["cases"] for failure in run_case(case)]
    for failure in failures:
        print("FAIL", failure)
    calls = sum(len(case["calls"]) for case in data["cases"])
    if failures:
        print(f"VON_SEAM_ORACLE_FAIL: {len(failures)} of {calls} calls")
        return 1
    print(f"VON_SEAM_ORACLE_PASS: {len(data['cases'])} cases, {calls} calls")
    return 0


if __name__ == "__main__":
    sys.exit(main())
