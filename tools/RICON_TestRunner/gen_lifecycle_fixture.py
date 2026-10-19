#!/usr/bin/env python3
"""TOOL_ONLY: generate the Enforce lifecycle parity fixture from the host oracle vectors (stdlib only).

Usage: python -I gen_lifecycle_fixture.py <oracle/vectors/lifecycle_v1.json> <out RICON_LifecycleFixtures.c>

Every trace step becomes one typed call on RICON_LifecycleStateMachine plus its expectation, written verbatim from
the vector file; the generator computes nothing. Wire-level absences that a typed API cannot express are mapped
the way a future adapter must map them and are counted as ADAPTER_MAPPED:
  - missing nowMs                 -> nowMs = -1 (outside 0..INT_MAX, so MALFORMED)
  - INIT config with missing keys -> config = null (MALFORMED)
Trust boundary: before any code is written, validate() admits only closed records whose values are int32 numbers
or names from closed enum sets (trace ids match [A-Za-z0-9_-], source is REQ-lifecycle.md@<hex>); anything else raises VectorError, so a changed
vector file cannot inject Enforce code. A catalog network with an empty channel list is refused, because the
typed flat (network, channel) pairs cannot express it.
Three negative controls replay a vector with a deliberately broken expectation and must be reported as failed,
so a vacuous comparison cannot pass.
"""

from __future__ import annotations

import copy
import hashlib
import json
import re
import sys
from pathlib import Path

CONFIG_KEYS = (
    "maxPlayers", "maxActiveLeases", "maxTerminalLeases", "terminalLeaseTtlMs", "maxTombstones", "tombstoneTtlMs",
    "maxResponseCachePerPlayer", "maxRateEntries", "rateWindowMs", "rateMaxRequests",
)
INT_MIN, INT_MAX = -2147483648, 2147483647
# Closed name sets: the only strings that may reach generated code (enum member names of RICON_Lifecycle.c).
RESULT_CODES = frozenset((
    "MALFORMED", "TICK_ANOMALY", "NOT_READY", "INVALID_SESSION", "STALE_CONNECTION", "STALE_GENERATION",
    "STALE_ENTITY", "DUPLICATE", "REQUEST_CONFLICT", "STALE_REQUEST", "LIMIT_REACHED", "RATE_LIMITED",
    "FORGED_OWNER", "INACTIVE_ENTITY", "UNKNOWN_DEVICE", "NOT_OWNER", "POWER_OFF", "INVALID_CATALOG",
    "DEVICE_BUSY", "UNKNOWN_LEASE", "STALE_LEASE", "OK"))
PLAYER_STATES = frozenset(("NONE", "CONNECTED_UNREGISTERED", "REGISTERED_UNBOUND", "BOUND_ACTIVE", "BOUND_INACTIVE",
                           "DISCONNECTED"))
LEASE_STATES = frozenset(("ACTIVE", "EXPIRED", "REVOKED"))
TERMINAL_REASONS = frozenset((
    "TTL_EXPIRED", "CLIENT_STOP", "DISCONNECT", "RECONNECT", "ENTITY_CHANGED", "ENTITY_INACTIVE", "ACCESS_LOST",
    "POWER_LOST", "DEVICE_REMOVED", "CATALOG_INVALIDATED", "SESSION_ROTATED", "ADMIN_REVOKED", "TICK_ANOMALY"))
DEVICE_KINDS = frozenset(("CARRIED", "WORLD"))
UNBIND_REASONS = frozenset(("KILLED", "DELETED", "CONTROL_LOST"))
EVENT_FIELDS = {
    "INIT": {"nowMs", "sessionEpoch", "config", "catalog"},
    "CONNECTED": {"nowMs", "playerId", "connectionId"},
    "REGISTERED": {"nowMs", "playerId", "generation"},
    "ENTITY_BOUND": {"nowMs", "playerId", "generation", "entityId"},
    "ENTITY_CHANGED": {"nowMs", "playerId", "generation", "fromEntityId", "toEntityId"},
    "ENTITY_UNBOUND": {"nowMs", "playerId", "generation", "entityId", "reason"},
    "DISCONNECTED": {"nowMs", "playerId", "generation", "connectionId"},
    "TICK": {"nowMs"},
    "ROTATE_EPOCH": {"nowMs", "sessionEpoch"},
    "PTT_START": {"nowMs", "schemaVersion", "correlationId", "authenticatedPlayerId", "payloadPlayerId",
                  "sessionEpoch", "generation", "requestSeq", "deviceId", "networkTypeId", "channelId", "ttlMs"},
    "PTT_STOP": {"nowMs", "schemaVersion", "correlationId", "authenticatedPlayerId", "payloadPlayerId",
                 "sessionEpoch", "generation", "requestSeq", "leaseId"},
}
EFFECT_FIELDS = {
    "PLAYER_STATE_CHANGED": {"playerId", "oldGeneration", "newGeneration", "fromState", "toState"},
    "FULL_RESYNC_REQUIRED": {"playerId", "sessionEpoch", "generation"},
    "ENTITY_BOUND": {"playerId", "generation", "entityId"},
    "ENTITY_UNBOUND": {"playerId", "generation", "entityId"},
    "LEASE_GRANTED": {"playerId", "generation", "deviceId", "leaseId", "issuedAtMs", "ttlMs"},
    "LEASE_TERMINATED": {"playerId", "generation", "deviceId", "leaseId", "leaseState", "terminalReason"},
    "VON_EXPIRY_INTENDED": {"sessionEpoch", "playerId", "deviceId", "leaseId"},
    "SUBSCRIPTIONS_CLEARED": {"playerId", "generation"},
    "CARRIED_DEVICES_DETACHED": {"playerId", "generation"},
    "TOMBSTONE_WRITTEN": {"playerId", "generation", "connectionId", "expiresAtMs"},
}
NAME_FIELDS = {"fromState": PLAYER_STATES, "toState": PLAYER_STATES, "leaseState": LEASE_STATES,
               "terminalReason": TERMINAL_REASONS, "reason": UNBIND_REASONS, "resultCode": RESULT_CODES,
               "cachedResultCode": RESULT_CODES}
TRACE_ID = re.compile(r"[A-Za-z0-9_-]{1,64}")
CT_ID = re.compile(r"CT-LC-[0-9]{3}")
NETWORK_KEY = re.compile(r"[1-9][0-9]{0,9}")
SOURCE = re.compile(r"docs/req/REQ-lifecycle\.md@[0-9a-f]{7,40}")

RUN = "RICON_LifecycleTraceRun"
RESULT = "RICON_ELifecycleResult."
STATE = "RICON_EPlayerLifecycleState."
LEASE_STATE = "RICON_ETransmitLeaseState."
REASON = "RICON_ELeaseTerminalReason."


class VectorError(ValueError):
    pass


def require(condition: bool, where: str) -> None:
    if not condition:
        raise VectorError(f"unsupported vector value at {where}")


def check_int(value: object, where: str) -> None:
    require(isinstance(value, int) and not isinstance(value, bool) and INT_MIN <= value <= INT_MAX, where)


def check_record(record: object, fields: set[str], where: str, required: set[str] | None = None) -> None:
    """Closed record: known keys only; names from closed sets, everything else int32."""
    require(isinstance(record, dict) and set(record) <= fields, where)
    require(set(record) >= (fields if required is None else required), where)
    for key, value in record.items():
        if key in NAME_FIELDS:
            require(value in NAME_FIELDS[key], f"{where}.{key}")
        else:
            check_int(value, f"{where}.{key}")


def check_catalog(catalog: object, where: str) -> None:
    require(isinstance(catalog, list), where)
    for index, entry in enumerate(catalog):
        at = f"{where}[{index}]"
        keys = {"deviceId", "kind", "ownerPlayerId", "accessPlayerIds", "powered", "allowed"}
        require(isinstance(entry, dict) and set(entry) <= keys and set(entry) >= keys - {"ownerPlayerId"}, at)
        check_int(entry["deviceId"], f"{at}.deviceId")
        if "ownerPlayerId" in entry:
            check_int(entry["ownerPlayerId"], f"{at}.ownerPlayerId")
        require(entry["kind"] in DEVICE_KINDS, f"{at}.kind")
        require(isinstance(entry["powered"], bool), f"{at}.powered")
        require(isinstance(entry["accessPlayerIds"], list), f"{at}.accessPlayerIds")
        for player_id in entry["accessPlayerIds"]:
            check_int(player_id, f"{at}.accessPlayerIds")
        require(isinstance(entry["allowed"], dict), f"{at}.allowed")
        for network, channels in entry["allowed"].items():
            require(isinstance(network, str) and NETWORK_KEY.fullmatch(network) is not None
                    and int(network) <= INT_MAX, f"{at}.allowed key")
            # A network with no channels has no typed form (flat pairs): refuse instead of dropping it silently.
            require(isinstance(channels, list) and len(channels) > 0, f"{at}.allowed[{network}]")
            for channel in channels:
                check_int(channel, f"{at}.allowed[{network}]")


def validate(data: object) -> None:
    """Trust boundary: every value that reaches generated Enforce code is an int32 or a closed enum name."""
    require(isinstance(data, dict) and data.get("schema") == "ricon.oracle.lifecycle" and data.get("version") == 1,
            "schema/version")
    require(isinstance(data.get("source"), str) and SOURCE.fullmatch(data["source"]) is not None, "source")
    traces = data.get("traces")
    require(isinstance(traces, list) and len(traces) > 0, "traces")
    seen = set()
    for trace in traces:
        require(isinstance(trace, dict) and isinstance(trace.get("id"), str)
                and TRACE_ID.fullmatch(trace["id"]) is not None and trace["id"] not in seen, "trace id")
        seen.add(trace["id"])
        tid = trace["id"]
        require(isinstance(trace.get("ct"), list) and len(trace["ct"]) > 0
                and all(isinstance(ct, str) and CT_ID.fullmatch(ct) for ct in trace["ct"]), f"{tid}.ct")
        require(isinstance(trace.get("steps"), list) and len(trace["steps"]) > 0, f"{tid}.steps")
        for index, step in enumerate(trace["steps"]):
            at = f"{tid}[{index}]"
            require(isinstance(step, dict) and isinstance(step.get("event"), dict)
                    and isinstance(step.get("expect"), dict), at)
            event = step["event"]
            kind = event.get("type")
            require(kind in EVENT_FIELDS, f"{at}.type")
            fields = EVENT_FIELDS[kind]
            body = {k: v for k, v in event.items() if k != "type"}
            if kind == "INIT":
                require(set(body) <= fields and set(body) >= fields - {"nowMs"}, f"{at} INIT fields")
                if "nowMs" in body:
                    check_int(body["nowMs"], f"{at}.nowMs")
                check_int(body["sessionEpoch"], f"{at}.sessionEpoch")
                require(isinstance(body["config"], dict) and set(body["config"]) <= set(CONFIG_KEYS), f"{at}.config")
                for key, value in body["config"].items():
                    check_int(value, f"{at}.config.{key}")
                check_catalog(body["catalog"], f"{at}.catalog")
            else:
                check_record(body, fields, at, fields - {"nowMs"})
            expect = step["expect"]
            effects = expect.get("effects", [])
            require(isinstance(effects, list), f"{at}.effects")
            check_record({k: v for k, v in expect.items() if k != "effects"},
                         {"resultCode", "cachedResultCode", "leaseId", "leaseState", "terminalReason"},
                         f"{at}.expect", {"resultCode"})
            for number, effect in enumerate(effects):
                require(isinstance(effect, dict) and effect.get("type") in EFFECT_FIELDS, f"{at}.effects[{number}]")
                check_record({k: v for k, v in effect.items() if k != "type"}, EFFECT_FIELDS[effect["type"]],
                             f"{at}.effects[{number}]")


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
        return f"t.Von({f['sessionEpoch']}, {f['playerId']}, {f['deviceId']}, {f['leaseId']});"
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
    return generate_bytes(vectors_path.read_bytes())


def generate_bytes(raw: bytes) -> str:
    data = json.loads(raw.decode("utf-8"))
    validate(data)
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
