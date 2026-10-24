#!/usr/bin/env python3
"""claude: check and summary of a B0a log (Game-side endpoint observer RICON_LiveEndpointProbe under
-riconEndpointProbe=1, codex 0a039a2, schema docs/evidence/E-live-endpoint-bootstrap-codex.md; claude #4563, #4574, #4577).
Modes:
  off     - the gate is not given: not one [RICON_LIVE] line (stream or dump), no [RICON_LIVEB] lifecycle/heartbeat line of
            the old bench, the launcher's "state=ENDPOINT_ONLY" line present, and the launcher's watch line shows the game
            mode entered (isGameMode=1), so that silence is not a run that never reached Play; no SCRIPT (E).
            Verdict ENDPOINT_GATE_OFF_CLEAN.
  on      - exactly one observation cycle; restart - exactly two (Play -> Edit -> Play in one process).
For every cycle: the stream from ENDPOINT_START (seq 0) to ENDPOINT_DONE / ENDPOINT_ABORTED with consecutive seq; START
fields cycle, limit=30, isServer, isClient, rplMode (= RplSession.Mode() as an integer, required in START and in every
SAMPLE, codex #4579; the number is recorded, never mapped to a name by guess); per sample n = 1.. an optional ENDPOINT_BINDING (value n), then
ENDPOINT_SAMPLE (value n), then RADIO_STATE / RADIO_GONE of the radio reader; elapsedMs >= 1000 for the first sample and at
least 999 ms after the previous one; each BINDING raises revision by one, the first one has initial=1 and
previousPlayerId=-1, a BINDING is required whenever pc, controlled or playerId differ from the previous sample, and its
pc/controlled/player equal its sample; SAMPLE.bindingRevision is the latest revision; the SAMPLE field set is that of the
bench PlayerState (as in A1-P0) plus cycle, elapsedMs, bindingRevision, members (numeric IDs joined by "|", NONE or UNKNOWN);
RADIO_STATE has binding=LOCAL_ONLY, obs, slot, prefab, min, max, res, powered, muted, the sample's player and tr >= 0;
RADIO_GONE has binding, obs; the end is DONE (value 30, samples=30, reason=LIMIT) or ABORTED (value = samples,
reason GAME_END / WORLD_CLEANUP / WORLD_CHANGED / GAME_START_RESET); then one dump BEGIN count=N dropped=0 nextSeq=N that
repeats exactly the cycle's stream, and dump=END. No other kind (no VoN, BENCH, PLAYER, QM, SCREEN records).
Cycles are numbered 1, 2, ... in stream order. Findings are descriptive: the process role, samples with a local player,
the playerIds and members seen, binding revisions, radios observed, and LOCAL_ROLE_READY when the last 5 samples have
pc=1, controlled=1, one stable revision and the own playerId among members in each of them (LOCAL_ROLE_NOT_READY otherwise; a missing
player early in a cycle is not a log error). The end of an observation is not a proven role readiness (codex schema).
Restart (codex #4579, -riconEndpointRestart=1: the launcher waits for >= 3 samples, requests Edit, waits for the edit mode
and requests the second Play): cycle 1 must end ABORTED after at least 3 samples with GAME_END, WORLD_CLEANUP or
WORLD_CHANGED, cycle 2 must end DONE, and the launcher lines are exactly restart=EDIT_REQUESTED,
restart=SECOND_PLAY_REQUESTED and restart=SECOND_PLAY_ENTERED (no REFUSED); cycle 2 may carry cycle=2 (same game object) or
cycle=1 (game object recreated), recorded as cycleField; a cycle 1 that reached DONE did not exercise the cleanup
(ENDPOINT_RESTART_NOT_EXERCISED, not a pass).
Verdicts: ENDPOINT_GATE_OFF_CLEAN / ENDPOINT_CYCLE_COMPLETE / ENDPOINT_CYCLE_ABORTED / ENDPOINT_RESTART_COMPLETE /
ENDPOINT_RESTART_NOT_EXERCISED / LOG_INVALID.
Usage: python -I live-endpoint-log-check-claude.py <console.log> off|on|restart"""
import hashlib
import json
import re
import sys

LINE = re.compile(r"\[RICON_LIVE\] seq=(\d+) t=(-?\d+) side=(\w+) kind=(\w+) player=(-?\d+) device=(-?\d+) "
                  r"tr=(-?\d+) value=(-?\d+) text=(\S*)$")
INT = re.compile(r"-?\d+")
BIT = ("0", "1")
LIMIT = 30
ABORT_REASONS = {"GAME_END", "WORLD_CLEANUP", "WORLD_CHANGED", "GAME_START_RESET"}
ALLOWED = {"ENDPOINT_START", "ENDPOINT_BINDING", "ENDPOINT_SAMPLE", "ENDPOINT_DONE", "ENDPOINT_ABORTED", "RADIO_STATE",
           "RADIO_GONE"}
MEMBERS = re.compile(r"NONE|UNKNOWN|\d+(\|\d+)*")
READY_TAIL = 5


def fields(text):
    out, dup = {}, False
    for part in text.split(","):
        if "=" in part:
            k, v = part.split("=", 1)
            dup = dup or k in out
            out[k] = v
        elif part:
            dup = True
    return out, dup


def player_state_errors(f):
    """The bench PlayerState field set (A1-P0 rules) inside an ENDPOINT_SAMPLE."""
    errs = []
    want = {"isServer", "isClient", "players", "pc"}
    if f.get("isServer") not in BIT or f.get("isClient") not in BIT or f.get("pc") not in BIT:
        errs.append("role/pc not 0|1")
    if not INT.fullmatch(f.get("players", "")):
        errs.append("players not an integer")
    if f.get("pc") == "1":
        want |= {"playerId", "vonController", "controlled"}
        if not INT.fullmatch(f.get("playerId", "")) or f.get("vonController") not in BIT or f.get("controlled") not in BIT:
            errs.append("player fields")
        if f.get("vonController") == "1":
            want.add("vonEntries")
            if not INT.fullmatch(f.get("vonEntries", "")):
                errs.append("vonEntries")
        if f.get("controlled") == "1":
            want |= {"controlledPrefab", "entityVoN", "gadgetManager"}
            if f.get("entityVoN") not in BIT or f.get("gadgetManager") not in BIT:
                errs.append("controlled fields")
    return errs, want


def check_cycle(ordinal, events, dump, errors, allowed_cycles=None):
    """allowed_cycles: the cycle numbers ENDPOINT_START may carry (default: the ordinal). A restart may recreate the
    game object ("Workbench Reload Game", codex #4594), so its second cycle may count from 1 again."""
    allowed = allowed_cycles or [str(ordinal)]
    """One observation cycle: stream events (already split), its dump lines; returns the finding row."""
    tag = f"cycle {ordinal}"
    row = {"cycle": ordinal, "samples": 0, "end": None, "reason": None}
    if [int(e["seq"]) for e in events] != list(range(len(events))):
        errors.append(f"{tag}: seq not consecutive from 0")
    start = events[0]["f"] if events and events[0]["kind"] == "ENDPOINT_START" else {}
    want = {"cycle", "limit", "isServer", "isClient", "rplMode"}
    if set(start) != want or not INT.fullmatch(start.get("rplMode", "")) or start.get("cycle") not in allowed or start.get("limit") != str(LIMIT) \
            or start.get("isServer") not in BIT or start.get("isClient") not in BIT \
            or events[0]["value"] != 0 or events[0]["player"] != -1:
        errors.append(f"{tag}: ENDPOINT_START {start}")
    row["role"] = {k: start.get(k) for k in ("isServer", "isClient", "rplMode") if k in start}
    row["cycleField"] = start.get("cycle")
    ends = [i for i, e in enumerate(events) if e["kind"] in ("ENDPOINT_DONE", "ENDPOINT_ABORTED")]
    if ends != [len(events) - 1]:
        errors.append(f"{tag}: one ENDPOINT_DONE/ABORTED must close the cycle (at {ends})")
    sample, revision, prev, last_ms, players, members_seen, tail = 0, 0, None, None, set(), set(), []
    radios, binding_at, rpl_seen = {}, None, set()
    for e in events[1:]:
        k, f, v = e["kind"], e["f"], e["value"]
        if k not in ALLOWED:
            errors.append(f"{tag}: kind {k} not allowed")
            continue
        if k == "ENDPOINT_BINDING":
            if v != sample + 1:
                errors.append(f"{tag}: BINDING value {v} is not the next sample {sample + 1}")
            want = {"cycle", "revision", "initial", "pcChanged", "entityChanged", "idChanged", "pc", "controlled",
                    "previousPlayerId"}
            if set(f) != want or f.get("cycle") != start.get("cycle") or any(f.get(x) not in BIT for x in
                                                                     ("initial", "pcChanged", "entityChanged",
                                                                      "idChanged", "pc", "controlled")) \
                    or not INT.fullmatch(f.get("revision", "")) or not INT.fullmatch(f.get("previousPlayerId", "")):
                errors.append(f"{tag}: BINDING fields {sorted(f)}")
                continue
            if int(f["revision"]) != revision + 1:
                errors.append(f"{tag}: BINDING revision {f['revision']} after {revision}")
            first = revision == 0
            if f["initial"] != ("1" if first else "0") or (first and f["previousPlayerId"] != "-1"):
                errors.append(f"{tag}: BINDING initial/previousPlayerId")
            if prev is not None and f["previousPlayerId"] != str(prev["player"]):
                errors.append(f"{tag}: BINDING previousPlayerId {f['previousPlayerId']} != {prev['player']}")
            revision = int(f["revision"])
            binding_at = {"sample": v, "f": f, "player": e["player"]}
        elif k == "ENDPOINT_SAMPLE":
            if v != sample + 1:
                errors.append(f"{tag}: SAMPLE value {v} after {sample}")
            sample = v
            errs, want = player_state_errors(f)
            want |= {"cycle", "elapsedMs", "bindingRevision", "members"}
            want.add("rplMode")
            if not INT.fullmatch(f.get("rplMode", "")):
                errs.append("rplMode not an integer")
            if set(f) != want:
                errs.append(f"field set {sorted(set(f) ^ want)}")
            if f.get("cycle") != start.get("cycle") or not INT.fullmatch(f.get("elapsedMs", "")) \
                    or not MEMBERS.fullmatch(f.get("members", "")):
                errs.append("cycle/elapsedMs/members")
            for x in errs:
                errors.append(f"{tag} sample {v}: {x}")
            if errs:
                continue
            ms = int(f["elapsedMs"])
            if ms < 1000 or (last_ms is not None and ms - last_ms < 999):
                errors.append(f"{tag} sample {v}: elapsedMs {ms} after {last_ms}")
            last_ms = ms
            pid = int(f["playerId"]) if f["pc"] == "1" else -1
            if e["player"] != pid:
                errors.append(f"{tag} sample {v}: envelope player {e['player']} != playerId {pid}")
            cur = {"pc": f["pc"], "controlled": f.get("controlled", "0"), "player": pid}
            changed = prev is None or cur != {x: prev[x] for x in cur}
            bound = binding_at is not None and binding_at["sample"] == v
            if changed and not bound:
                errors.append(f"{tag} sample {v}: binding changed without ENDPOINT_BINDING")
            if bound:
                bf = binding_at["f"]
                if (bf["pc"], bf["controlled"], binding_at["player"]) != (cur["pc"], cur["controlled"], pid):
                    errors.append(f"{tag} sample {v}: BINDING disagrees with its sample")
            if f["bindingRevision"] != str(revision):
                errors.append(f"{tag} sample {v}: bindingRevision {f['bindingRevision']} != {revision}")
            if pid >= 0:
                players.add(pid)
            members_seen.add(f["members"])
            if "rplMode" in f:
                rpl_seen.add(f["rplMode"])
            tail.append({"n": v, "pc": cur["pc"], "controlled": cur["controlled"], "player": pid,
                         "revision": revision, "members": f["members"]})
            prev = cur
        elif k == "RADIO_STATE":
            want = {"binding", "obs", "slot", "prefab", "min", "max", "res", "powered", "muted"}
            if set(f) != want or f.get("binding") != "LOCAL_ONLY" or e["tr"] < 0 or prev is None \
                    or e["player"] != prev["player"] or f.get("powered") not in BIT or f.get("muted") not in BIT \
                    or not all(INT.fullmatch(f.get(x, "")) for x in ("obs", "slot", "min", "max", "res")):
                errors.append(f"{tag}: RADIO_STATE {e['line'][-120:]}")
                continue
            radios.setdefault(f["obs"], {"prefab": f["prefab"], "tr": e["tr"], "raw": []})["raw"].append(v)
        elif k == "RADIO_GONE":
            if set(f) != {"binding", "obs"} or f.get("binding") != "LOCAL_ONLY" or v != -1:
                errors.append(f"{tag}: RADIO_GONE {e['line'][-80:]}")
        elif k in ("ENDPOINT_DONE", "ENDPOINT_ABORTED"):
            if set(f) != {"cycle", "samples", "reason"} or f.get("cycle") != start.get("cycle") \
                    or f.get("samples") != str(sample) or v != sample:
                errors.append(f"{tag}: {k} fields {f} value {v} after {sample} samples")
            if k == "ENDPOINT_DONE" and (sample != LIMIT or f.get("reason") != "LIMIT"):
                errors.append(f"{tag}: ENDPOINT_DONE needs {LIMIT} samples and reason=LIMIT")
            if k == "ENDPOINT_ABORTED" and f.get("reason") not in ABORT_REASONS:
                errors.append(f"{tag}: ENDPOINT_ABORTED reason {f.get('reason')}")
            row.update({"end": k, "reason": f.get("reason")})
        elif k == "ENDPOINT_START":
            errors.append(f"{tag}: second ENDPOINT_START inside a cycle")
    if sample > LIMIT:
        errors.append(f"{tag}: {sample} samples > {LIMIT}")
    # the dump repeats exactly this cycle's stream, dropped=0
    head = dump[0] if dump else ""
    m = re.fullmatch(r"\[RICON_LIVE\] dump=BEGIN count=(\d+) dropped=(\d+) nextSeq=(\d+)", head)
    if not m or m.group(2) != "0" or int(m.group(1)) != len(events) or int(m.group(3)) != len(events) \
            or dump[1:] != [e["line"] for e in events]:
        errors.append(f"{tag}: dump {head!r} does not repeat the {len(events)} stream events with dropped=0")
    last = tail[-READY_TAIL:]
    ready = len(last) == READY_TAIL and all(t["pc"] == "1" and t["controlled"] == "1" for t in last) \
        and len({(t["player"], t["revision"]) for t in last}) == 1 \
        and all(str(t["player"]) in t["members"].split("|") for t in last)  # codex #4585: every sample of the window
    row.update({"samples": sample, "bindingRevisions": revision, "playerIds": sorted(players),
                "members": sorted(members_seen), "firstLocalPlayerSample": next((t["n"] for t in tail if t["pc"] == "1"),
                                                                              None),
                "firstControlledSample": next((t["n"] for t in tail if t["controlled"] == "1"), None),
                "radios": radios, "role": row["role"], "rplModesInSamples": sorted(rpl_seen),
                "localRole": "LOCAL_ROLE_READY" if ready else "LOCAL_ROLE_NOT_READY"})
    return row


def main(path, mode):
    raw = open(path, "rb").read()
    lines = raw.decode("utf-8", errors="replace").splitlines()
    errors = []
    if [l for l in lines if re.search(r"SCRIPT\s*\(E\)", l)]:
        errors.append("SCRIPT (E) present")
    live = [l[l.index("[RICON_LIVE]"):].rstrip() for l in lines if "[RICON_LIVE]" in l]
    endpoint_only = any("[RICON_LIVEB] state=ENDPOINT_ONLY" in l for l in lines)
    old_bench = [l for l in lines if re.search(r"\[RICON_LIVEB\] (lifecycle=|heartbeat|state=BENCH_CREATED)", l)]
    if not endpoint_only:
        errors.append("launcher line state=ENDPOINT_ONLY missing")
    if old_bench:
        errors.append(f"old bench lines present: {old_bench[0][-80:]}")
    rows = []
    if mode == "off":
        if not any(re.search(r"\[RICON_LIVEB\] watch t=\d+ms isGameMode=1", l) for l in lines):
            errors.append("no launcher watch line with isGameMode=1: Play not shown")
        if live:
            errors.append(f"{len(live)} [RICON_LIVE] lines with the gate off")
        verdict = "ENDPOINT_GATE_OFF_CLEAN"
    else:
        # split into cycles: stream events from ENDPOINT_START to the end record, then dump BEGIN ... END
        cycles, cur, dump, in_dump = [], None, None, False
        for l in live:
            if l.startswith("[RICON_LIVE] dump=BEGIN"):
                in_dump, dump = True, [l]
                continue
            if l == "[RICON_LIVE] dump=END":
                if cur is None or not in_dump:
                    errors.append("dump=END without a cycle")
                else:
                    cycles.append((cur, dump))
                cur, dump, in_dump = None, None, False
                continue
            if in_dump:
                dump.append(l)
                continue
            m = LINE.fullmatch(l)
            if not m:
                errors.append(f"unparsed line {l[:80]}")
                continue
            f, dup = fields(m.group(9))
            if dup:
                errors.append(f"duplicate or bare field in {l[:80]}")
            e = {"seq": m.group(1), "kind": m.group(4), "player": int(m.group(5)), "device": m.group(6),
                 "tr": int(m.group(7)), "value": int(m.group(8)), "f": f, "line": l}
            if m.group(3) != "probe" or e["device"] != "-1":
                errors.append(f"side/device in {l[:80]}")
            if e["kind"] == "ENDPOINT_START":
                if cur is not None:
                    errors.append("ENDPOINT_START before the previous cycle's dump")
                    cycles.append((cur, []))
                cur = []
            if cur is None:
                errors.append(f"event outside a cycle: {l[:80]}")
                continue
            cur.append(e)
        if cur is not None or in_dump:
            errors.append("last cycle has no dump=END")
            if cur:
                cycles.append((cur, dump or []))
        want = 2 if mode == "restart" else 1
        if len(cycles) != want:
            errors.append(f"{len(cycles)} cycles, expected {want}")
        rows = [check_cycle(i + 1, ev, du, errors, ["2", "1"] if mode == "restart" and i == 1 else None)
                for i, (ev, du) in enumerate(cycles)]
        ends = [r["end"] for r in rows]
        verdict = "ENDPOINT_CYCLE_COMPLETE" if ends and all(x == "ENDPOINT_DONE" for x in ends[-1:]) \
            else "ENDPOINT_CYCLE_ABORTED"
        if mode == "restart":
            marks = [m.group(1) for m in (re.search(r"\[RICON_LIVEB\] restart=(\w+)", l) for l in lines) if m]
            if marks != ["EDIT_REQUESTED", "SECOND_PLAY_REQUESTED", "SECOND_PLAY_ENTERED"]:
                errors.append(f"restart launcher lines {marks} != EDIT_REQUESTED, SECOND_PLAY_REQUESTED, SECOND_PLAY_ENTERED")
        if mode == "restart" and len(rows) == 2:
            first, second = rows
            if first["end"] == "ENDPOINT_DONE":
                first["note"] = "first cycle reached LIMIT before the restart: cleanup on Edit not exercised"
                verdict = "ENDPOINT_RESTART_NOT_EXERCISED"
            elif first["samples"] < 3 or first["reason"] not in ("GAME_END", "WORLD_CLEANUP", "WORLD_CHANGED"):
                errors.append(f"restart: cycle 1 ended {first['reason']} after {first['samples']} samples")
            elif second["end"] != "ENDPOINT_DONE":
                errors.append(f"restart: cycle 2 ended {second['end']} {second['reason']}")
            else:
                verdict = "ENDPOINT_RESTART_COMPLETE"
    overall = "LOG_INVALID" if errors else verdict
    print(json.dumps({"logSha256": hashlib.sha256(raw).hexdigest(), "mode": mode, "errors": errors,
                      "overall": overall, "liveLines": len(live), "endpointOnly": endpoint_only, "cycles": rows},
                     ensure_ascii=False, indent=1))
    print("LIVE_ENDPOINT_LOG_CHECK_" + ("PASS" if not errors else "FAIL"))
    return 1 if errors else 0


if __name__ == "__main__":
    if len(sys.argv) != 3 or sys.argv[2] not in ("off", "on", "restart"):
        print("usage: live-endpoint-log-check-claude.py <console.log> off|on|restart")
        sys.exit(2)
    sys.exit(main(sys.argv[1], sys.argv[2]))
