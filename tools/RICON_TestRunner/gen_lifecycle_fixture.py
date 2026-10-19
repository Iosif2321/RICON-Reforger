#!/usr/bin/env python3
"""TOOL_ONLY: generate the Enforce lifecycle parity fixture from the host oracle vectors (stdlib only).

Usage: python -I gen_lifecycle_fixture.py <oracle/vectors/lifecycle_v1.json> <out RICON_LifecycleFixtures.c>

Every trace step becomes one typed call on RICON_LifecycleStateMachine plus its expectation, written verbatim from
the vector file; the generator computes nothing. Wire-level absences that a typed API cannot express are mapped
the way a future adapter must map them and are counted as ADAPTER_MAPPED:
  - missing nowMs                 -> nowMs = -1 (outside 0..INT_MAX, so MALFORMED)
  - INIT config with missing keys -> config = null (MALFORMED)
Three negative controls replay a vector with a deliberately broken expectation and must be reported as failed,
so a vacuous comparison cannot pass.
"""

from __future__ import annotations

import copy
import hashlib
import json
import sys
from pathlib import Path

CONFIG_KEYS = (
    "maxPlayers", "maxActiveLeases", "maxTerminalLeases", "terminalLeaseTtlMs", "maxTombstones", "tombstoneTtlMs",
    "maxResponseCachePerPlayer", "maxRateEntries", "rateWindowMs", "rateMaxRequests",
)
RUN = "RICON_LifecycleTraceRun"
RESULT = "RICON_ELifecycleResult."
STATE = "RICON_EPlayerLifecycleState."
LEASE_STATE = "RICON_ETransmitLeaseState."
REASON = "RICON_ELeaseTerminalReason."


class Generator:
    def __init__(self) -> None:
        self.catalogs: dict[str, str] = {}
        self.adapter_mapped = 0

    def now(self, event: dict) -> str:
        if "nowMs" not in event:
            self.adapter_mapped += 1
            return "-1 /* ADAPTER_MAPPED: nowMs missing */"
        return str(event["nowMs"])

    def catalog(self, catalog: list) -> str:
        key = json.dumps(catalog, sort_keys=True)
        if key not in self.catalogs:
            self.catalogs[key] = f"Catalog{len(self.catalogs) + 1}"
        return self.catalogs[key] + "()"

    def catalog_function(self, name: str, catalog: list) -> list[str]:
        lines = [f"\tprotected static array<ref RICON_LifecycleDevice> {name}",
                 "\t{",
                 "\t\tarray<ref RICON_LifecycleDevice> catalog = {};",
                 "\t\tRICON_LifecycleDevice device;"]
        for entry in catalog:
            powered = "true" if entry["powered"] else "false"
            lines.append(f"\t\tdevice = {RUN}.Device({entry['deviceId']}, RICON_EDeviceKind.{entry['kind']}, "
                         f"{entry.get('ownerPlayerId', 0)}, {powered});")
            for player_id in entry["accessPlayerIds"]:
                lines.append(f"\t\tdevice.m_aAccessPlayerIds.Insert({player_id});")
            for network, channels in entry["allowed"].items():
                for channel in channels:
                    lines.append(f"\t\tdevice.m_aAllowed.Insert(new RICON_LifecycleCatalogPair({int(network)}, "
                                 f"{channel}));")
            lines.append("\t\tcatalog.Insert(device);")
        lines += ["\t\treturn catalog;", "\t}"]
        return lines

    def call(self, event: dict) -> str:
        kind = event["type"]
        e = event
        if kind == "INIT":
            config = e["config"]
            if isinstance(config, dict) and all(k in config for k in CONFIG_KEYS):
                config_code = f"{RUN}.Config({', '.join(str(config[k]) for k in CONFIG_KEYS)})"
            else:
                self.adapter_mapped += 1
                config_code = "null /* ADAPTER_MAPPED: config key missing */"
            return f"m.Initialize({e['sessionEpoch']}, {self.now(e)}, {config_code}, {self.catalog(e['catalog'])})"
        if kind == "CONNECTED":
            return f"m.Connected({self.now(e)}, {e['playerId']}, {e['connectionId']})"
        if kind == "REGISTERED":
            return f"m.Registered({self.now(e)}, {e['playerId']}, {e['generation']})"
        if kind == "ENTITY_BOUND":
            return f"m.EntityBound({self.now(e)}, {e['playerId']}, {e['generation']}, {e['entityId']})"
        if kind == "ENTITY_CHANGED":
            return (f"m.EntityChanged({self.now(e)}, {e['playerId']}, {e['generation']}, {e['fromEntityId']}, "
                    f"{e['toEntityId']})")
        if kind == "ENTITY_UNBOUND":
            return (f"m.EntityUnbound({self.now(e)}, {e['playerId']}, {e['generation']}, {e['entityId']}, "
                    f"RICON_EEntityUnbindReason.{e['reason']})")
        if kind == "DISCONNECTED":
            return f"m.Disconnected({self.now(e)}, {e['playerId']}, {e['generation']}, {e['connectionId']})"
        if kind == "TICK":
            return f"m.Tick({self.now(e)})"
        if kind == "ROTATE_EPOCH":
            return f"m.RotateEpoch({self.now(e)}, {e['sessionEpoch']})"
        common = (f"{self.now(e)}, {e['schemaVersion']}, {e['correlationId']}, {e['authenticatedPlayerId']}, "
                  f"{e['payloadPlayerId']}, {e['sessionEpoch']}, {e['generation']}, {e['requestSeq']}")
        if kind == "PTT_START":
            return (f"m.ProcessPtt({RUN}.Start({common}, {e['deviceId']}, {e['networkTypeId']}, {e['channelId']}, "
                    f"{e['ttlMs']}))")
        if kind == "PTT_STOP":
            return f"m.ProcessPtt({RUN}.Stop({common}, {e['leaseId']}))"
        raise ValueError(f"unknown event type {kind!r}")


def effect_line(effect: dict) -> str:
    t, f = effect["type"], effect
    if t == "PLAYER_STATE_CHANGED":
        return (f"t.Psc({f['playerId']}, {f['oldGeneration']}, {f['newGeneration']}, {STATE}{f['fromState']}, "
                f"{STATE}{f['toState']});")
    if t == "FULL_RESYNC_REQUIRED":
        return f"t.Resync({f['playerId']}, {f['sessionEpoch']}, {f['generation']});"
    if t == "ENTITY_BOUND":
        return f"t.Bound({f['playerId']}, {f['generation']}, {f['entityId']});"
    if t == "ENTITY_UNBOUND":
        return f"t.Unbound({f['playerId']}, {f['generation']}, {f['entityId']});"
    if t == "LEASE_GRANTED":
        return (f"t.Granted({f['playerId']}, {f['generation']}, {f['deviceId']}, {f['leaseId']}, {f['issuedAtMs']}, "
                f"{f['ttlMs']});")
    if t == "LEASE_TERMINATED":
        return (f"t.Terminated({f['playerId']}, {f['generation']}, {f['deviceId']}, {f['leaseId']}, "
                f"{LEASE_STATE}{f['leaseState']}, {REASON}{f['terminalReason']});")
    if t == "VON_EXPIRY_INTENDED":
        return f"t.Von({f['playerId']}, {f['deviceId']}, {f['leaseId']});"
    if t == "SUBSCRIPTIONS_CLEARED":
        return f"t.Subscriptions({f['playerId']}, {f['generation']});"
    if t == "CARRIED_DEVICES_DETACHED":
        return f"t.Detached({f['playerId']}, {f['generation']});"
    if t == "TOMBSTONE_WRITTEN":
        return f"t.Tombstone({f['playerId']}, {f['generation']}, {f['connectionId']}, {f['expiresAtMs']});"
    raise ValueError(f"unknown effect type {t!r}")


def check_line(expect: dict) -> str:
    has_cached = "cachedResultCode" in expect
    has_lease = "leaseId" in expect
    cached = expect.get("cachedResultCode", "OK")
    lease_id = expect.get("leaseId", 0)
    lease_state = expect.get("leaseState") or "NONE"
    reason = expect.get("terminalReason") or "NONE"
    return (f"t.Check(r, {RESULT}{expect['resultCode']}, {str(has_cached).lower()}, {RESULT}{cached}, "
            f"{str(has_lease).lower()}, {lease_id}, {LEASE_STATE}{lease_state}, {REASON}{reason});")


def trace_function(gen: Generator, name: str, trace: dict) -> list[str]:
    lines = [f"\t//! {trace['id']} ({', '.join(trace['ct'])}), {len(trace['steps'])} steps",
             f"\tprotected static void {name}({RUN} t)",
             "\t{",
             "\t\tRICON_LifecycleStateMachine m = t.Machine();",
             "\t\tRICON_LifecycleResult r;"]
    for index, step in enumerate(trace["steps"]):
        lines.append(f"\t\t// step {index}: {step['event']['type']}")
        lines.append(f"\t\tr = {gen.call(step['event'])};")
        for effect in step["expect"].get("effects", []):
            lines.append("\t\t" + effect_line(effect))
        lines.append("\t\t" + check_line(step["expect"]))
    lines.append("\t}")
    return lines


def controls(traces: list) -> list[tuple[str, dict]]:
    """Three broken copies of real traces; each must fail in Enforce."""
    result = []
    code = copy.deepcopy(traces[0])
    step = code["steps"][2]["expect"]
    step["resultCode"] = "STALE_GENERATION" if step["resultCode"] != "STALE_GENERATION" else "OK"
    result.append(("changed resultCode", code))
    for trace in traces:
        steps = [s for s in trace["steps"] if len(s["expect"].get("effects", [])) >= 2]
        if steps:
            swapped = copy.deepcopy(trace)
            target = next(s for s in swapped["steps"] if len(s["expect"].get("effects", [])) >= 2)
            effects = target["expect"]["effects"]
            effects[0], effects[1] = effects[1], effects[0]
            result.append(("swapped effect order", swapped))
            break
    for trace in traces:
        steps = [s for s in trace["steps"]
                 if any(e["type"] == "VON_EXPIRY_INTENDED" for e in s["expect"].get("effects", []))]
        if steps:
            dropped = copy.deepcopy(trace)
            target = next(s for s in dropped["steps"]
                          if any(e["type"] == "VON_EXPIRY_INTENDED" for e in s["expect"].get("effects", [])))
            target["expect"]["effects"] = [e for e in target["expect"]["effects"]
                                           if e["type"] != "VON_EXPIRY_INTENDED"]
            result.append(("dropped VON_EXPIRY_INTENDED", dropped))
            break
    if len(result) != 3:
        raise ValueError("vectors do not allow all three negative controls")
    return result


def generate(vectors_path: Path) -> str:
    raw = vectors_path.read_bytes()
    data = json.loads(raw.decode("utf-8"))
    if data.get("schema") != "ricon.oracle.lifecycle" or data.get("version") != 1:
        raise ValueError("unexpected vector schema")
    traces = data["traces"]
    gen = Generator()
    body: list[str] = []
    names = []
    for index, trace in enumerate(traces, 1):
        name = f"Trace{index:02d}"
        names.append((name, trace))
        body += trace_function(gen, name, trace) + [""]
    adapter_mapped = gen.adapter_mapped
    control_list = controls(traces)
    for index, (_, trace) in enumerate(control_list, 1):
        body += trace_function(gen, f"Control{index}", trace) + [""]
    total_steps = sum(len(t["steps"]) for t in traces)

    run_all = [
        "\t//! Replays every trace; true only when all traces pass and every negative control fails.",
        "\tstatic bool RunAll()",
        "\t{",
        "\t\tint tracesPassed = 0;",
        "\t\tint stepsPassed = 0;",
        f"\t\t{RUN} t;",
    ]
    for name, trace in names:
        run_all += [f"\t\tt = new {RUN}(\"{trace['id']}\", {len(trace['steps'])});",
                    f"\t\t{name}(t);",
                    "\t\tstepsPassed += t.GetPassedSteps();",
                    "\t\tif (t.IsPassed())",
                    "\t\t\ttracesPassed++;"]
    run_all += ["\t\tint controlsCaught = 0;"]
    for index, (what, trace) in enumerate(control_list, 1):
        run_all += [f"\t\tt = new {RUN}(\"control: {what}\", {len(trace['steps'])}, true);",
                    f"\t\tControl{index}(t);",
                    "\t\tif (t.IsFailed())",
                    "\t\t\tcontrolsCaught++;",
                    "\t\telse",
                    f"\t\t\tPrint(\"[RICON_TEST] FAIL negative control not detected: {what}\");"]
    run_all += [
        f"\t\tbool ok = tracesPassed == {len(traces)} && stepsPassed == {total_steps} "
        f"&& controlsCaught == {len(control_list)};",
        f"\t\tPrintFormat(\"[RICON_TEST] lifecycle parity: traces %1/{len(traces)}, steps %2/{total_steps}, "
        f"negative controls caught %3/{len(control_list)}, adapter-mapped steps {adapter_mapped}\", "
        "tracesPassed, stepsPassed, controlsCaught);",
        "\t\tif (ok)",
        "\t\t\tPrint(\"[RICON_TEST] LIFECYCLE_ENFORCE_PARITY_PASS\");",
        "\t\telse",
        "\t\t\tPrint(\"[RICON_TEST] LIFECYCLE_ENFORCE_PARITY_FAIL\");",
        "\t\treturn ok;",
        "\t}",
        "",
    ]

    catalog_code: list[str] = []
    for key, name in gen.catalogs.items():
        catalog_code += gen.catalog_function(name + "()", json.loads(key)) + [""]

    header = [
        "// GENERATED by tools/RICON_TestRunner/gen_lifecycle_fixture.py - do not edit by hand.",
        f"// Source: oracle/vectors/lifecycle_v1.json sha256 {hashlib.sha256(raw).hexdigest()}",
        f"// Vectors: {data['source']}, {len(traces)} traces, {total_steps} steps, "
        f"adapter-mapped steps {adapter_mapped}.",
        "// TEST_ONLY lifecycle parity fixture: RICON_LifecycleStateMachine against the host oracle expectations.",
        "",
        "class RICON_LifecycleFixtures",
        "{",
    ]
    return "\n".join(header + run_all + catalog_code + body).rstrip() + "\n}\n"


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
