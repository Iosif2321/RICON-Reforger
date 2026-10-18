#!/usr/bin/env python3
"""Host-эталон проверки маршрута по REQ-core §9 (stdlib). Не production-код.

Проверяет `oracle/vectors/route_v1.json`: контекст, структуру маршрута в глобальном порядке причин,
min(Q) и худшее состояние по ногам.
"""

from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path
from typing import Any

STATES = ("LOST", "WEAK", "DEGRADED", "CLEAN")  # stable ordering: LOST worst
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


def context_valid(c: Any) -> bool:
    """§9.2: positive epoch, non-negative time, positive maxHops and correlation id, AccessPolicy present."""
    return (isinstance(c, dict)
            and is_int(c.get("sessionEpoch")) and c["sessionEpoch"] > 0
            and is_int(c.get("serverTimeMs")) and c["serverTimeMs"] >= 0
            and is_int(c.get("maxHops")) and c["maxHops"] > 0
            and is_int(c.get("correlationId")) and c["correlationId"] > 0
            and c.get("hasAccessPolicy") is True)


def budget_valid(budget: Any) -> bool:
    if not isinstance(budget, dict) or budget.get("valid") is not True:
        return False
    q = budget.get("intelligibility")
    if isinstance(q, bool) or not isinstance(q, (int, float)) or not math.isfinite(q) or not 0.0 <= q <= 1.0:
        return False
    if budget.get("state") not in STATES:
        return False
    return budget["state"] != "LOST" or q == 0.0  # LOST ⇒ Q = 0 (REQ-LE-72)


def endpoint_valid(leg: dict[str, Any]) -> bool:
    f, t = leg.get("fromEndpointId"), leg.get("toEndpointId")
    return is_int(f) and is_int(t) and f > 0 and t > 0 and f != t


def validate(context: Any, route: dict[str, Any]) -> str:
    """Первая причина в глобальном порядке §9.1."""
    if not context_valid(context):
        return "INVALID_CONTEXT"
    if not isinstance(route, dict):
        return "EMPTY_ROUTE"
    legs = route.get("legs") or []
    if not legs:
        return "EMPTY_ROUTE"
    if len(legs) > context["maxHops"]:
        return "TOO_MANY_HOPS"
    if any(leg is None for leg in legs):
        return "NULL_LEG"
    talker, listener = route.get("talkerEndpointId"), route.get("listenerEndpointId")
    if not (is_int(talker) and talker > 0 and is_int(listener) and listener > 0):
        return "INVALID_ENDPOINT"
    if not all(endpoint_valid(leg) for leg in legs):
        return "INVALID_ENDPOINT"
    if not all(budget_valid(leg.get("budget")) for leg in legs):
        return "INVALID_BUDGET"
    if legs[0]["fromEndpointId"] != talker:
        return "WRONG_START"
    seen = {talker}
    previous_to = talker
    for leg in legs:  # one ordered pass: continuity, then cycle, per leg
        if leg["fromEndpointId"] != previous_to:
            return "DISCONTINUITY"
        if leg["toEndpointId"] in seen:
            return "CYCLE"
        seen.add(leg["toEndpointId"])
        previous_to = leg["toEndpointId"]
    if legs[-1]["toEndpointId"] != listener:
        return "WRONG_DESTINATION"
    return "OK"


def evaluate(context: Any, route: dict[str, Any]) -> dict[str, Any]:
    reason = validate(context, route)
    if reason != "OK":
        return {"reason": reason, "quality": 0.0, "state": "LOST"}
    legs = route["legs"]
    quality = min(leg["budget"]["intelligibility"] for leg in legs)
    state = min((leg["budget"]["state"] for leg in legs), key=STATES.index)
    return {"reason": "OK", "quality": float(quality), "state": state}


def check(path: Path) -> list[str]:
    document = json.loads(path.read_text(encoding="utf-8"))
    if document.get("schema") != "ricon.oracle.route" or document.get("version") != 1:
        return ["schema: unsupported schema/version"]
    ids = [vector["id"] for vector in document["vectors"]]
    if len(ids) != len(set(ids)):
        return ["schema: duplicate vector ids"]
    tolerance = document["tolerances"]["ratio"]
    failures = []
    for vector in document["vectors"]:
        data = decode(vector["input"])
        got = evaluate(data["context"], data["route"])
        want = vector["expected"]
        if got["reason"] != want["reason"] or got["state"] != want["state"] \
                or abs(got["quality"] - want["quality"]) > tolerance:
            failures.append(f"{vector['id']}: got {got}, want {want}")
    return failures


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", type=Path, required=True, metavar="VECTORS_JSON")
    args = parser.parse_args(argv)
    failures = check(args.check)
    for failure in failures:
        print(f"ROUTE_ORACLE_FAIL: {failure}", file=sys.stderr)
    if failures:
        return 1
    count = len(json.loads(args.check.read_text(encoding="utf-8"))["vectors"])
    print(f"ROUTE_ORACLE_PASS: {count} vectors")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
