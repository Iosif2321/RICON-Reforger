#!/usr/bin/env python3
"""TOOL_ONLY: копирует ожидания snapshot rev9; allocator не подменяется тестовой формулой."""
from __future__ import annotations

import argparse
import copy
import hashlib
import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from gen_l2_fixture import literal

# shortcut: принимается только этот корпус; новую ревизию и SHA добавлять после отдельного ревью.
SOURCE_SHA = "3440982cea0cce21ef857e09a1db3dbe77a5c85561771212dd9ea1199e93721c"
STATES = {"LOST", "WEAK", "DEGRADED", "CLEAN"}
FIELDS = ("schemaVersion", "sessionEpoch", "linkId", "linkSeq", "ttlMs", "routeId", "routeNodeCount", "quality")


def value(item):
    if isinstance(item, bool):
        return str(item).lower()
    if item == "NaN":
        return "RICON_L0Run.Special(0)"
    return literal(item)


def case_method(vector, index):
    body = []
    snapshot_index = 0

    def snapshot(data):
        nonlocal snapshot_index
        if data is None:
            return "null"
        name = f"s{snapshot_index}"
        snapshot_index += 1
        args = [value(data[k]) for k in FIELDS]
        # Единственное отображение нетипизированного BROKEN в недопустимый Enforce enum.
        args.append("RICON_ELinkState." + data["state"] if data["state"] in STATES else "-1")
        args += [value(data[k]) for k in ("talkerPlayerId", "listenerPlayerId")]
        body.append(f"RICON_LinkSnapshot {name} = RICON_VoNSeamRun.Snapshot({', '.join(args)});")
        if data["quality"] == "NaN":
            body.append(f"t.RequireNaN({name});")
        return name

    if vector["kind"] == "compare":
        current = snapshot(vector["input"]["current"])
        incoming = snapshot(vector["input"]["incoming"])
        body.append(f't.Compare({current}, {incoming}, RICON_ESnapshotDecision.{vector["expected"]["decision"]});')
    else:
        for step, (event, expected) in enumerate(zip(vector["input"]["events"], vector["expected"]["steps"], strict=True)):
            event_type = event["event"]
            if event_type == "visible":
                body.append(f't.Visible({event["nowMs"]}, {event["linkId"]}, {value(expected["visible"])}, '
                            f'{value("DIAGNOSTIC" in expected["effects"])});')
                continue
            ids = list(expected["slots"])
            body += [f"array<int> ids{step} = {{{', '.join(ids)}}};",
                     f"array<int> seq{step} = {{{', '.join(str(expected['slots'][key]) for key in ids)}}};"]
            tail = (f'RICON_ESnapshotDecision.{expected["decision"]}, '
                    f'{value("REQUEST_RESYNC" in expected["effects"])}, {value("DIAGNOSTIC" in expected["effects"])}, '
                    f'ids{step}, seq{step}')
            if event_type == "full_resync":
                names = [snapshot(s) for s in event["snapshots"]]
                body.append(f"array<ref RICON_LinkSnapshot> entries{step} = {{{', '.join(names)}}};")
                body.append(f't.Resync({event["nowMs"]}, {event["sessionEpoch"]}, {event["schemaVersion"]}, entries{step}, {tail});')
            elif event_type == "receive":
                name = snapshot(event["snapshot"])
                body.append(f't.Receive({event["nowMs"]}, {name}, {tail});')
            else:
                raise ValueError(f"Неизвестное событие {event_type}")
    return [f'\t//! {vector["id"]}', f"\tprotected static void Case{index}(RICON_SnapshotRun t)", "\t{"] + [
        "\t\t" + line for line in body] + ["\t}", ""]


def controls(vectors):
    specifications = [
        ("CMP-01", None, "decision", "STALE"),
        ("RX-01", 0, "effects", []),
        ("RX-08", 2, "effects", []),
        ("RX-09", 1, "visible", False),
        ("RX-03", 0, "slots", {"10": 5}),
        ("RX-02", 2, "slots", {"10": 5}),
        ("RX-10", 1, "effects", ["REQUEST_RESYNC"]),
    ]
    result = []
    for source, step, field, replacement in specifications:
        vector = copy.deepcopy(next(v for v in vectors if v["id"] == source))
        expected = vector["expected"] if step is None else vector["expected"]["steps"][step]
        expected[field] = replacement
        vector["id"] = f"X-{len(result) + 1}"
        result.append(vector)
    return result


def calls(vector):
    return 1 if vector["kind"] == "compare" else len(vector["input"]["events"])


def render(raw):
    if hashlib.sha256(raw).hexdigest() != SOURCE_SHA:
        raise ValueError("Корпус отличается от принятого SHA; требуется ревью")
    document = json.loads(raw)
    vectors = [v for v in document["vectors"] if v["kind"] != "allocator_trace"]
    if len(vectors) != 43 or sum(calls(v) for v in vectors) != 77:
        raise ValueError("Неверные счётчики принятого корпуса")
    negative = controls(vectors)
    lines = ["//! GENERATED TEST_ONLY: gen_snapshot_fixture.py, ожидания копируются из rev9.",
             "//! 3 allocator_trace — BLOCKED_NO_AUTHORITY_CONTEXT; BROKEN -> enum -1 (ADAPTER_MAPPED=1).",
             "class RICON_SnapshotFixtures", "{", f'\tstatic const string SOURCE_SHA = "{SOURCE_SHA}";', ""]
    for index, vector in enumerate(vectors + negative):
        lines += case_method(vector, index)
    lines += ["\tstatic bool RunAll()", "\t{", '\t\tPrint("[RICON_SNAPSHOT] source=" + SOURCE_SHA);',
              "\t\tint cases;", "\t\tint steps;", "\t\tint quietPositive;", "\t\tint rejected;", "\t\tRICON_SnapshotRun t;"]
    for index, vector in enumerate(vectors):
        lines += [f'\t\tt = new RICON_SnapshotRun("{vector["id"]}");', f"\t\tCase{index}(t);",
                  f"\t\tif (t.Passed({calls(vector)})) cases++;", "\t\tsteps += t.Calls();",
                  f'\t\tt = new RICON_SnapshotRun("{vector["id"]}", true);', f"\t\tCase{index}(t);",
                  f"\t\tif (t.Passed({calls(vector)})) quietPositive++;"]
    for index, vector in enumerate(negative, len(vectors)):
        lines += [f'\t\tt = new RICON_SnapshotRun("{vector["id"]}", true);', f"\t\tCase{index}(t);",
                  "\t\tif (t.Failed()) rejected++;"]
    lines += ['\t\tPrintFormat("[RICON_SNAPSHOT] cases=%1 steps=%2 quietPositive=%3 controls=%4 blockedAllocator=3 adapterMapped=1",',
              "\t\t\tcases, steps, quietPositive, rejected);",
              "\t\treturn cases == 43 && steps == 77 && quietPositive == 43 && rejected == 7;", "\t}", "}", ""]
    return "\n".join(lines)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args(argv)
    try:
        output = render(args.source.read_bytes())
        if args.check:
            if args.destination.read_text(encoding="utf-8") != output:
                raise ValueError("Сохранённая фикстура отличается от генератора")
        else:
            args.destination.write_text(output, encoding="utf-8", newline="\n")
    except (OSError, ValueError) as exc:
        print(f"SNAPSHOT_GENERATOR_FAIL: {exc}")
        return 1
    print("SNAPSHOT_GENERATOR_PASS: 43 случая, 77 шагов, 3 allocator заблокированы")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
