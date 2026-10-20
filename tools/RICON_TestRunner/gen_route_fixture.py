#!/usr/bin/env python3
"""TOOL_ONLY: copies the accepted route corpus (oracle/vectors/route_v1.json, 48 vectors) into Enforce; computes nothing.

python -B -I tools/RICON_TestRunner/gen_route_fixture.py <route_v1.json> <RICON_RouteFixtures.c> [--check]

Adapter mappings (values a typed API cannot carry as they are), all counted and marked in the output:
- ADAPTER_MAPPED: a route object without the "legs" key is a route with no legs; an unknown state name is enum -1.
- TEST_SUBCLASS: a null leg is inserted through RICON_TestRoute, because the public AddLeg refuses null.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))  # -I removes the script folder; only the sibling tool
from gen_l2_fixture import literal  # noqa: E402

SOURCE_SHA = "efccccb536a362cc9b2346a73138b7fc3444f159be19ad24ac172a18e25f43fd"  # new revision -> new review
STATES = ("LOST", "WEAK", "DEGRADED", "CLEAN")
REASONS = ("OK", "INVALID_CONTEXT", "EMPTY_ROUTE", "TOO_MANY_HOPS", "NULL_LEG", "INVALID_ENDPOINT", "INVALID_BUDGET",
           "WRONG_START", "DISCONTINUITY", "CYCLE", "WRONG_DESTINATION")
CONTEXT_KEYS = {"sessionEpoch", "serverTimeMs", "maxHops", "correlationId", "hasAccessPolicy"}
ID = re.compile(r"R-[0-9]{2}")
INT32 = (-2 ** 31, 2 ** 31 - 1)


class VectorError(Exception):
    pass


def need(condition: bool, what: str) -> None:
    if not condition:
        raise VectorError(what)


def int32(value: object, what: str) -> int:
    need(isinstance(value, int) and not isinstance(value, bool) and INT32[0] <= value <= INT32[1], what)
    return value  # type: ignore[return-value]


def number(value: object, what: str) -> str:
    if value == "NaN":
        return "RICON_L2Run.NaN()"
    need(isinstance(value, (int, float)) and not isinstance(value, bool) and abs(value) <= 1e6, what)
    return literal(float(value))


def case(vector: dict, index: int, counts: dict) -> list[str]:
    need(set(vector) == {"id", "kind", "req", "input", "expected", "tags"} and vector["kind"] == "route", "vector keys")
    vid = vector["id"]
    need(isinstance(vid, str) and ID.fullmatch(vid) is not None, "id")
    inp, exp = vector["input"], vector["expected"]
    need(isinstance(inp, dict) and set(inp) == {"context", "route"}, f"{vid}: input keys")
    need(isinstance(exp, dict) and set(exp) == {"reason", "quality", "state"}, f"{vid}: expected keys")
    need(exp["reason"] in REASONS and exp["state"] in STATES, f"{vid}: expected names")
    ctx = inp["context"]
    need(isinstance(ctx, dict) and set(ctx) == CONTEXT_KEYS and isinstance(ctx["hasAccessPolicy"], bool), f"{vid}: context")
    lines = [f"\tprotected static void Case{index}(RICON_RouteRun t)", "\t{"]
    lines.append("\t\tRICON_AccessPolicy policy = " + ("new RICON_TestAllowAllAccessPolicy();" if ctx["hasAccessPolicy"]
                                                       else "null;"))
    args = ", ".join(str(int32(ctx[k], f"{vid}: {k}")) for k in ("sessionEpoch", "serverTimeMs", "maxHops", "correlationId"))
    lines.append(f"\t\tRICON_RoutingContext context = RICON_RouteRun.Context({args}, policy);")
    route = inp["route"]
    if route is None:
        lines.append("\t\tRICON_TestRoute route = null;")
    else:
        need(isinstance(route, dict) and set(route) in ({"talkerEndpointId", "listenerEndpointId", "legs"},
                                                         {"talkerEndpointId", "listenerEndpointId"}), f"{vid}: route keys")
        lines.append(f"\t\tRICON_TestRoute route = new RICON_TestRoute({int32(route['talkerEndpointId'], 'talker')}, "
                     f"{int32(route['listenerEndpointId'], 'listener')});")
        if "legs" not in route:
            lines.append("\t\t// ADAPTER_MAPPED: no \"legs\" key -> a route with no legs")
            counts["adapter"] += 1
        legs = route.get("legs", [])
        need(isinstance(legs, list) and len(legs) <= 16, f"{vid}: legs")
        for n, leg in enumerate(legs):
            if leg is None:
                lines.append("\t\troute.InsertNullLeg(); // TEST_SUBCLASS: the public AddLeg refuses null")
                counts["subclass"] += 1
                continue
            need(isinstance(leg, dict) and set(leg) == {"fromEndpointId", "toEndpointId", "budget"}, f"{vid}: leg keys")
            budget = leg["budget"]
            if budget is None:
                lines.append(f"\t\tRICON_LinkBudget b{n} = null;")
            else:
                need(isinstance(budget, dict) and set(budget) == {"valid", "intelligibility", "state"}
                     and isinstance(budget["valid"], bool), f"{vid}: budget")
                if budget["state"] in STATES:
                    state = f"RICON_ELinkState.{budget['state']}"
                else:
                    need(isinstance(budget["state"], str) and re.fullmatch(r"[A-Z]{1,16}", budget["state"]) is not None,
                         f"{vid}: state")
                    state = "-1"
                    lines.append(f"\t\t// ADAPTER_MAPPED: unknown state name {budget['state']} -> enum -1")
                    counts["adapter"] += 1
                lines.append(f"\t\tRICON_LinkBudget b{n} = RICON_RouteRun.Budget({str(budget['valid']).lower()}, "
                             f"{number(budget['intelligibility'], f'{vid}: q')}, {state});")
                if budget["intelligibility"] == "NaN":  # the special value must be real before Evaluate (#2505)
                    lines.append(f'\t\tt.RequireNaN("{vid}", b{n}.m_fIntelligibility);')
            lines.append(f"\t\tRICON_RouteLeg l{n} = new RICON_RouteLeg({int32(leg['fromEndpointId'], 'from')}, "
                         f"{int32(leg['toEndpointId'], 'to')}, b{n});")
            lines.append(f'\t\tt.m_bOk = RICON_TestSupport.Expect(route.AddLeg(l{n}), "{vid}", "AddLeg") && t.m_bOk;')
    lines.append(f'\t\tt.Check("{vid}", route, context, RICON_ERouteReason.{exp["reason"]}, '
                 f'{number(exp["quality"], f"{vid}: expected quality")}, RICON_ELinkState.{exp["state"]});')
    return lines + ["\t}", ""]


def render(raw: bytes) -> tuple[str, dict]:
    if hashlib.sha256(raw).hexdigest() != SOURCE_SHA:
        raise VectorError("route_v1.json is not the accepted revision (SHA-256 differs)")
    data = json.loads(raw.decode("utf-8"))
    need(data.get("schema") == "ricon.oracle.route" and data.get("version") == 1, "schema")
    need(data.get("tolerances") == {"ratio": 1e-06}, "tolerance")
    vectors = data["vectors"]
    need(isinstance(vectors, list) and len(vectors) == 48 and len({v.get("id") for v in vectors}) == 48, "48 unique")
    counts = {"adapter": 0, "subclass": 0}
    body: list[str] = []
    for i, vector in enumerate(vectors):
        body += case(vector, i, counts)
    head = ["//! GENERATED by tools/RICON_TestRunner/gen_route_fixture.py from oracle/vectors/route_v1.json, do not edit.",
            f"//! source sha256 {SOURCE_SHA}; adapter-mapped {counts['adapter']}, test-subclass {counts['subclass']}.",
            "class RICON_RouteFixtures", "{",
            f'\tstatic const string SOURCE_SHA = "{SOURCE_SHA}";',
            f"\tstatic const int ADAPTER_MAPPED = {counts['adapter']};",
            f"\tstatic const int TEST_SUBCLASS = {counts['subclass']};", ""]
    run = ["\tstatic void Run(RICON_RouteRun t)", "\t{"] + [f"\t\tCase{i}(t);" for i in range(len(vectors))] + ["\t}"]
    return "\n".join(head + body + run + ["}", ""]), counts


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args(argv)
    try:
        text, counts = render(args.source.read_bytes())
    except (VectorError, ValueError, KeyError, TypeError) as error:
        print(f"ROUTE_GENERATOR_FAIL: {error}")
        return 2
    if args.check:
        same = args.destination.read_text(encoding="utf-8") == text
        print("ROUTE_GENERATOR_PASS" if same else "ROUTE_GENERATOR_FAIL: fixture differs from the corpus")
        return 0 if same else 1
    args.destination.write_text(text, encoding="utf-8", newline="\n")
    print(f"wrote 48 cases, adapter-mapped {counts['adapter']}, test-subclass {counts['subclass']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
