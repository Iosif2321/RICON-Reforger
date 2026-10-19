#!/usr/bin/env python3
"""TOOL_ONLY: generate the Enforce VoN seam parity fixture from the host oracle vectors (stdlib only).

Usage: python -I gen_von_seam_fixture.py <oracle/vectors/von_seam_v1.json> <out RICON_VoNSeamFixtures.c>

Every oracle call becomes one `t.Apply(...)` / `t.Expire(...)` on RICON_VoNSeamRun with the expected decision copied
from the vectors; the generator computes nothing. Values a typed API cannot carry are mapped the way a future
adapter must map them and counted as ADAPTER_MAPPED:
  - a link state name outside LOST/WEAK/DEGRADED/CLEAN -> enum value -1 (outside the enum, so INVALID_SNAPSHOT)
Special floats ("NaN", "Infinity", "-Infinity") are built at run time and checked as non-finite before use.
Snapshots and bindings are held in local variables: Enforce releases a temporary passed straight as an argument
during the call (first Workbench run: "binding changed by the call", then NULL_INPUT everywhere).
Trust boundary: validate() admits only closed records of int32 numbers, plain decimal floats and names from closed
sets; anything else raises VectorError before code is written. Three negative controls must fail.
"""

from __future__ import annotations

import copy
import hashlib
import json
import re
import sys
from pathlib import Path

INT_MIN, INT_MAX = -2147483648, 2147483647
SOURCE = re.compile(r"docs/req/REQ-von-seam\.md@[0-9a-f]{7,40}")
CASE_ID = re.compile(r"[A-Za-z0-9_-]{1,64}")
CT_ID = re.compile(r"CT-VON-S-[0-9]{3}")
STATE_NAME = re.compile(r"[A-Z_]{1,32}")
DECIMAL = re.compile(r"-?[0-9]{1,3}\.[0-9]{1,6}")
DECISIONS = frozenset(("NULL_INPUT", "INVALID_SNAPSHOT", "DECISION_NOT_ACCEPTED", "INVALID_BINDING",
                       "IDENTITY_MISMATCH", "INVALID_EXPIRY", "BLOCKED_NO_ACTUATOR"))
UPSTREAM = frozenset(("ACCEPT_FIRST", "ACCEPT_NEWER", "STALE", "INVALID_SNAPSHOT", "INVALID_IDENTITY",
                      "ACCEPT_RESYNC", "INVALID_RESYNC"))
STATES = frozenset(("LOST", "WEAK", "DEGRADED", "CLEAN"))
SPECIAL = {"NaN": ("NaN()", True), "Infinity": ("Infinity()", False), "-Infinity": ("-Infinity()", False)}
SNAPSHOT_FIELDS = ("schemaVersion", "sessionEpoch", "linkId", "linkSeq", "ttlMs", "routeId", "routeNodeCount",
                   "quality", "state", "talkerPlayerId", "listenerPlayerId")
BINDING_FIELDS = ("sessionEpoch", "linkId", "talkerPlayerId", "listenerPlayerId", "talkerDeviceId",
                  "listenerDeviceId", "talkerLeaseId", "talkerTransceiverIndex", "listenerTransceiverIndex")
CASE_KEYS = {"id", "ct", "req", "calls", "note"}
APPLY_KEYS = {"op", "snapshot", "binding", "upstreamDecision", "expect"}
EXPIRE_KEYS = {"op", "sessionEpoch", "playerId", "deviceId", "leaseId", "binding", "expect"}
RUN = "RICON_VoNSeamRun"


class VectorError(ValueError):
    pass


def require(condition: bool, where: str) -> None:
    if not condition:
        raise VectorError(f"unsupported vector value at {where}")


def is_int32(value: object) -> bool:
    return isinstance(value, int) and not isinstance(value, bool) and INT_MIN <= value <= INT_MAX


def check_snapshot(snapshot: object, where: str) -> None:
    if snapshot is None:
        return
    require(isinstance(snapshot, dict) and set(snapshot) == set(SNAPSHOT_FIELDS), where)
    for key in SNAPSHOT_FIELDS:
        value = snapshot[key]
        if key == "quality":
            require(value in SPECIAL or is_int32(value)
                    or (isinstance(value, float) and DECIMAL.fullmatch(repr(value)) is not None), f"{where}.quality")
        elif key == "state":
            require(isinstance(value, str) and STATE_NAME.fullmatch(value) is not None, f"{where}.state")
        else:
            require(is_int32(value), f"{where}.{key}")


def check_binding(binding: object, where: str) -> None:
    if binding is None:
        return
    require(isinstance(binding, dict) and set(binding) == set(BINDING_FIELDS)
            and all(is_int32(binding[key]) for key in BINDING_FIELDS), where)


def validate(data: object) -> None:
    require(isinstance(data, dict) and data.get("schema") == "ricon.oracle.von_seam" and data.get("version") == 1,
            "schema/version")
    require(isinstance(data.get("source"), str) and SOURCE.fullmatch(data["source"]) is not None, "source")
    cases = data.get("cases")
    require(isinstance(cases, list) and len(cases) > 0, "cases")
    seen = set()
    for case in cases:
        require(isinstance(case, dict) and set(case) == CASE_KEYS, "case keys")
        cid = case["id"]
        require(isinstance(cid, str) and CASE_ID.fullmatch(cid) is not None and cid not in seen, "case id")
        seen.add(cid)
        require(isinstance(case["ct"], list) and len(case["ct"]) > 0
                and all(isinstance(ct, str) and CT_ID.fullmatch(ct) for ct in case["ct"]), f"{cid}.ct")
        require(isinstance(case["calls"], list) and len(case["calls"]) > 0, f"{cid}.calls")
        for index, step in enumerate(case["calls"]):
            at = f"{cid}[{index}]"
            require(isinstance(step, dict) and step.get("op") in ("apply", "expire"), at)
            require(isinstance(step.get("expect"), str) and step["expect"] in DECISIONS, f"{at}.expect")
            if step["op"] == "apply":
                require(set(step) == APPLY_KEYS, at)
                check_snapshot(step["snapshot"], f"{at}.snapshot")
                up = step["upstreamDecision"]
                require((isinstance(up, str) and up in UPSTREAM) or is_int32(up), f"{at}.upstreamDecision")
            else:
                require(set(step) == EXPIRE_KEYS, at)
                for key in ("sessionEpoch", "playerId", "deviceId", "leaseId"):
                    require(is_int32(step[key]), f"{at}.{key}")
            check_binding(step["binding"], f"{at}.binding")


class Generator:
    def __init__(self) -> None:
        self.adapter_mapped = 0
        self.local = 0

    def name(self, prefix: str) -> str:
        self.local += 1
        return f"{prefix}{self.local}"

    def snapshot(self, snapshot: dict | None, pre: list[str]) -> str:
        if snapshot is None:
            return "null"
        q = snapshot["quality"]
        if q in SPECIAL:
            factory, must_be_nan = SPECIAL[q]
            sign, factory = ("-", factory[1:]) if factory.startswith("-") else ("", factory)
            local = self.name("q")
            pre.append(f"float {local} = {sign}{RUN}.{factory};")
            pre.append(f"t.RequireNonFinite({local}, {str(must_be_nan).lower()});")
            quality = local
        else:
            quality = repr(float(q))
        state = snapshot["state"]
        if state in STATES:
            state_code = f"RICON_ELinkState.{state}"
        else:
            self.adapter_mapped += 1
            local = self.name("st")
            pre.append(f"RICON_ELinkState {local} = -1; // ADAPTER_MAPPED: unknown state name")
            state_code = local
        ints = [str(snapshot[k]) for k in SNAPSHOT_FIELDS if k not in ("quality", "state")]
        # field order of RICON_VoNSeamRun.Snapshot: 7 ints, quality, state, talker, listener
        local = self.name("s")
        pre.append(f"RICON_LinkSnapshot {local} = {RUN}.Snapshot({', '.join(ints[:7])}, {quality}, {state_code}, "
                   f"{ints[7]}, {ints[8]});")
        return local

    def binding(self, binding: dict | None, pre: list[str]) -> str:
        """A local strong reference: a temporary passed straight as an argument is released during the call."""
        if binding is None:
            return "null"
        local = self.name("b")
        pre.append(f"RICON_VoNDeviceBinding {local} = {RUN}.Binding({', '.join(str(binding[k]) for k in BINDING_FIELDS)});")
        return local

    def upstream(self, value: object, pre: list[str]) -> str:
        if isinstance(value, str):
            return f"RICON_ESnapshotDecision.{value}"
        local = self.name("up")
        pre.append(f"RICON_ESnapshotDecision {local} = {value}; // value outside the enum")
        return local

    def call(self, step: dict) -> list[str]:
        pre: list[str] = []
        expected = f"RICON_EVoNSeamDecision.{step['expect']}"
        if step["op"] == "apply":
            snapshot = self.snapshot(step["snapshot"], pre)
            upstream = self.upstream(step["upstreamDecision"], pre)
            line = f"t.Apply({snapshot}, {self.binding(step['binding'], pre)}, {upstream}, {expected});"
        else:
            ids = ", ".join(str(step[k]) for k in ("sessionEpoch", "playerId", "deviceId", "leaseId"))
            line = f"t.Expire({ids}, {self.binding(step['binding'], pre)}, {expected});"
        return pre + [line]


def case_function(gen: Generator, name: str, case: dict) -> list[str]:
    lines = [f"\t//! {case['id']} ({', '.join(case['ct'])}), {len(case['calls'])} calls",
             f"\tprotected static void {name}({RUN} t)",
             "\t{"]
    for index, step in enumerate(case["calls"]):
        lines.append(f"\t\t// call {index}: {step['op']}")
        lines += ["\t\t" + line for line in gen.call(step)]
    lines.append("\t}")
    return lines


def controls(cases: list) -> list[tuple[str, dict]]:
    """Three broken copies of real cases; each must fail in Enforce."""
    result = []
    first = copy.deepcopy(cases[0])
    first["calls"][0]["expect"] = "BLOCKED_NO_ACTUATOR" if first["calls"][0]["expect"] != "BLOCKED_NO_ACTUATOR" \
        else "NULL_INPUT"
    result.append(("changed apply decision", first))
    expire_case = next(c for c in cases if any(s["op"] == "expire" and s["expect"] == "IDENTITY_MISMATCH"
                                               for s in c["calls"]))
    broken = copy.deepcopy(expire_case)
    step = next(s for s in broken["calls"] if s["op"] == "expire" and s["expect"] == "IDENTITY_MISMATCH")
    step["expect"] = "BLOCKED_NO_ACTUATOR"
    result.append(("mismatched expiry accepted", broken))
    nan_case = next(c for c in cases if any(s["op"] == "apply" and s["snapshot"] is not None
                                            and s["snapshot"]["quality"] == "NaN" for s in c["calls"]))
    broken = copy.deepcopy(nan_case)
    step = next(s for s in broken["calls"] if s["op"] == "apply" and s["snapshot"] is not None
                and s["snapshot"]["quality"] == "NaN")
    step["expect"] = "BLOCKED_NO_ACTUATOR"
    result.append(("NaN quality accepted", broken))
    return result


def generate_bytes(raw: bytes) -> str:
    data = json.loads(raw.decode("utf-8"))
    validate(data)
    cases = data["cases"]
    gen = Generator()
    body: list[str] = []
    names = []
    for index, case in enumerate(cases, 1):
        name = f"Case{index:02d}"
        names.append((name, case))
        body += case_function(gen, name, case) + [""]
    adapter_mapped = gen.adapter_mapped
    control_list = controls(cases)
    for index, (_, case) in enumerate(control_list, 1):
        body += case_function(gen, f"Control{index}", case) + [""]
    total = sum(len(c["calls"]) for c in cases)

    run_all = [
        "\t//! Replays every case; true only when all cases pass and every negative control fails.",
        "\tstatic bool RunAll()",
        "\t{",
        "\t\tint casesPassed = 0;",
        "\t\tint callsPassed = 0;",
        f"\t\t{RUN} t;",
    ]
    for name, case in names:
        run_all += [f"\t\tt = new {RUN}(\"{case['id']}\", {len(case['calls'])});",
                    f"\t\t{name}(t);",
                    "\t\tcallsPassed += t.GetPassedCalls();",
                    "\t\tif (t.IsPassed())",
                    "\t\t\tcasesPassed++;"]
    run_all += ["\t\tint controlsCaught = 0;"]
    for index, (what, case) in enumerate(control_list, 1):
        run_all += [f"\t\tt = new {RUN}(\"control: {what}\", {len(case['calls'])}, true);",
                    f"\t\tControl{index}(t);",
                    "\t\tif (t.IsFailed())",
                    "\t\t\tcontrolsCaught++;",
                    "\t\telse",
                    f"\t\t\tPrint(\"[RICON_TEST] FAIL negative control not detected: {what}\");"]
    run_all += [
        f"\t\tbool ok = casesPassed == {len(cases)} && callsPassed == {total} && controlsCaught == {len(control_list)};",
        f"\t\tPrintFormat(\"[RICON_TEST] von seam parity: cases %1/{len(cases)}, calls %2/{total}, "
        f"negative controls caught %3/{len(control_list)}, adapter-mapped calls {adapter_mapped}\", "
        "casesPassed, callsPassed, controlsCaught);",
        "\t\tif (ok)",
        "\t\t\tPrint(\"[RICON_TEST] VON_SEAM_ENFORCE_PARITY_PASS\");",
        "\t\telse",
        "\t\t\tPrint(\"[RICON_TEST] VON_SEAM_ENFORCE_PARITY_FAIL\");",
        "\t\treturn ok;",
        "\t}",
        "",
    ]
    header = [
        "// GENERATED by tools/RICON_TestRunner/gen_von_seam_fixture.py - do not edit by hand.",
        f"// Source: oracle/vectors/von_seam_v1.json sha256 {hashlib.sha256(raw).hexdigest()}",
        f"// Vectors: {data['source']}, {len(cases)} cases, {total} calls, adapter-mapped calls {adapter_mapped}.",
        "// TEST_ONLY VoN seam parity fixture: RICON_VoNIntegration against the host oracle expectations.",
        "",
        "class RICON_VoNSeamFixtures",
        "{",
    ]
    return "\n".join(header + run_all + body).rstrip() + "\n}\n"


def generate(vectors_path: Path) -> str:
    return generate_bytes(vectors_path.read_bytes())


def main(argv: list[str]) -> int:
    if len(argv) != 3:
        print(__doc__)
        return 2
    text = generate(Path(argv[1]))
    with open(argv[2], "w", encoding="utf-8", newline="\n") as out:
        out.write(text)
    print(f"written {argv[2]}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
