#!/usr/bin/env python3
"""Host-эталон порядка LinkSnapshot по REQ-core §8 (stdlib). Не production-код.

Проверяет файл `oracle/vectors/snapshot_v1.json`: сравнение снимков, трассы получателя (identity, resync, TTL)
и выдачу seq у authority (граница int.MAX, ротация epoch).
"""

from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path
from typing import Any

STATES = ("LOST", "WEAK", "DEGRADED", "CLEAN")
SPECIAL_FLOATS = {"NaN": math.nan, "Infinity": math.inf, "-Infinity": -math.inf}


def decode(value: Any) -> Any:
    if isinstance(value, str) and value in SPECIAL_FLOATS:
        return SPECIAL_FLOATS[value]
    if isinstance(value, list):
        return [decode(item) for item in value]
    if isinstance(value, dict):
        return {key: decode(item) for key, item in value.items()}
    return value


def is_int(value: Any) -> bool:
    return isinstance(value, int) and not isinstance(value, bool)


def is_valid_snapshot(s: Any) -> bool:
    """Домены §8.3: проверяются до identity и seq."""
    if not isinstance(s, dict):
        return False
    for key in ("schemaVersion", "linkId", "linkSeq", "ttlMs", "routeId", "routeNodeCount"):
        if not is_int(s.get(key)) or s[key] <= 0:
            return False
    if not is_int(s.get("sessionEpoch")) or s["sessionEpoch"] == 0:
        return False
    quality = s.get("quality")
    if isinstance(quality, bool) or not isinstance(quality, (int, float)) or not math.isfinite(quality):
        return False
    if not 0.0 <= quality <= 1.0:
        return False
    if s.get("state") not in STATES:
        return False
    if s["state"] == "LOST" and quality != 0.0:
        return False
    return True


def compare(current: Any, incoming: Any) -> str:
    """Решение §8.3 для пары (текущий слот, входящий снимок)."""
    if not is_valid_snapshot(incoming):
        return "INVALID_SNAPSHOT"
    if current is None:
        return "ACCEPT_FIRST"
    for key in ("schemaVersion", "sessionEpoch", "linkId"):
        if incoming[key] != current[key]:
            return "INVALID_IDENTITY"
    return "ACCEPT_NEWER" if incoming["linkSeq"] > current["linkSeq"] else "STALE"


class Receiver:
    """Owner-scoped получатель: identity задаёт только full resync; слоты по linkId."""

    def __init__(self, supported_schema: int):
        self.supported_schema = supported_schema
        self.epoch: int | None = None
        self.slots: dict[int, tuple[dict[str, Any], int]] = {}

    def full_resync(self, now_ms: int, epoch: int, schema: int, snapshots: list[Any]) -> dict[str, Any]:
        self.epoch = epoch if schema == self.supported_schema else None
        self.slots = {}
        for s in snapshots:
            if is_valid_snapshot(s) and s["sessionEpoch"] == epoch and s["schemaVersion"] == schema:
                self.slots[s["linkId"]] = (s, now_ms)
        return {"slots": self.slot_view()}

    def receive(self, now_ms: int, s: Any) -> dict[str, Any]:
        if not is_valid_snapshot(s):
            return self.result("INVALID_SNAPSHOT", ["DIAGNOSTIC"])
        if self.epoch is None or s["schemaVersion"] != self.supported_schema:
            return self.result("INVALID_IDENTITY", ["REQUEST_RESYNC"])
        if s["sessionEpoch"] != self.epoch:
            self.slots = {}  # сброс state старого epoch (§8.3)
            return self.result("INVALID_IDENTITY", ["REQUEST_RESYNC"])
        current = self.slots.get(s["linkId"])
        decision = compare(current[0] if current else None, s)
        if decision in ("ACCEPT_FIRST", "ACCEPT_NEWER"):
            self.slots[s["linkId"]] = (s, now_ms)
            return self.result(decision, [])
        if decision == "STALE":
            return self.result(decision, [])
        return self.result(decision, ["REQUEST_RESYNC"])

    def visible(self, now_ms: int, link_id: int) -> dict[str, Any]:
        slot = self.slots.get(link_id)
        if slot is None:
            return {"visible": False, "effects": []}
        snapshot, received_at = slot
        elapsed = now_ms - received_at
        if elapsed < 0:
            return {"visible": False, "effects": ["DIAGNOSTIC"]}
        return {"visible": elapsed < snapshot["ttlMs"], "effects": []}

    def result(self, decision: str, effects: list[str]) -> dict[str, Any]:
        return {"decision": decision, "effects": effects, "slots": self.slot_view()}

    def slot_view(self) -> dict[str, int]:
        return {str(link_id): slot[0]["linkSeq"] for link_id, slot in sorted(self.slots.items())}


class Allocator:
    """Authority: seq по линку, никогда не выдаёт int.MAX; на границе — новый epoch и seq 1 у всех линков."""

    def __init__(self, int_max: int, epoch: int, seq_by_link: dict[str, int]):
        self.int_max = int_max
        self.epoch = epoch
        self.seq = {int(key): value for key, value in seq_by_link.items()}

    def next(self, link_id: int) -> dict[str, Any]:
        candidate = self.seq.get(link_id, 0) + 1
        if candidate < self.int_max:
            self.seq[link_id] = candidate
            return {"linkId": link_id, "linkSeq": candidate, "rotated": False}
        old = self.epoch
        self.epoch = old + 1 if old + 1 != 0 else old + 2
        self.seq = {link_id: 1}
        return {"linkId": link_id, "linkSeq": 1, "rotated": True, "fullResync": True, "_oldEpoch": old}


def run_vector(vector: dict[str, Any], document: dict[str, Any]) -> list[str]:
    kind = vector["kind"]
    data = decode(vector["input"])
    expected = vector["expected"]
    if kind == "compare":
        actual = compare(data["current"], data["incoming"])
        return [] if actual == expected["decision"] else [f"decision {actual} != {expected['decision']}"]
    if kind == "receiver_trace":
        receiver = Receiver(document["supportedSchemaVersion"])
        errors = []
        for index, (event, want) in enumerate(zip(data["events"], expected["steps"], strict=True)):
            if event["event"] == "full_resync":
                got = receiver.full_resync(event["nowMs"], event["sessionEpoch"], event["schemaVersion"], event["snapshots"])
            elif event["event"] == "receive":
                got = receiver.receive(event["nowMs"], event["snapshot"])
            elif event["event"] == "visible":
                got = receiver.visible(event["nowMs"], event["linkId"])
            else:
                return [f"step {index}: unknown event {event['event']!r}"]
            for key, value in want.items():
                if got.get(key) != value:
                    errors.append(f"step {index}: {key} {got.get(key)!r} != {value!r}")
        return errors
    if kind == "allocator_trace":
        state = data["state"]
        allocator = Allocator(document["intMax"], state["sessionEpoch"], state["seqByLink"])
        errors = []
        epoch = allocator.epoch
        for index, (link_id, want) in enumerate(zip(data["requests"], expected["steps"], strict=True)):
            got = allocator.next(link_id)
            if got.get("rotated"):
                if allocator.epoch in (0, got["_oldEpoch"]):
                    errors.append(f"step {index}: epoch not rotated to a new non-zero value")
            elif allocator.epoch != epoch:
                errors.append(f"step {index}: epoch changed without rotation")
            epoch = allocator.epoch
            for key, value in want.items():
                if got.get(key) != value:
                    errors.append(f"step {index}: {key} {got.get(key)!r} != {value!r}")
        return errors
    return [f"unknown kind {kind!r}"]


def check(path: Path) -> list[str]:
    document = json.loads(path.read_text(encoding="utf-8"))
    if document.get("schema") != "ricon.oracle.snapshot" or document.get("version") != 1:
        return ["schema: unsupported schema/version"]
    ids = [vector["id"] for vector in document["vectors"]]
    if len(ids) != len(set(ids)):
        return ["schema: duplicate vector ids"]
    failures = []
    for vector in document["vectors"]:
        failures.extend(f"{vector['id']}: {error}" for error in run_vector(vector, document))
    return failures


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", type=Path, required=True, metavar="VECTORS_JSON")
    args = parser.parse_args(argv)
    failures = check(args.check)
    for failure in failures:
        print(f"SNAPSHOT_ORACLE_FAIL: {failure}", file=sys.stderr)
    if failures:
        return 1
    count = len(json.loads(args.check.read_text(encoding="utf-8"))["vectors"])
    print(f"SNAPSHOT_ORACLE_PASS: {count} vectors")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
