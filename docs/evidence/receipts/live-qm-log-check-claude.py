#!/usr/bin/env python3
"""claude: check and summary of an A0-M log (RICON_LiveRadioBenchComponent with -riconBenchMode=radiomute, claude #4526,
order codex #4524): local Q-M on the bench's own isolated radios.
Checks: one BENCH_START with mode=radiomute, modeGiven=1; none of the old radio phases or player records in the stream
(BENCH_READ, _SET_FREQUENCY, _MUTE, _ENTRY, _SKIP, PLAYER_*, SCREEN); per own radio (bench=d) the QM plan 0 BASE,
1 MUTE, 2 RETUNE, 3 READ, 4 POWER, 5 READ, 6 POWER_BACK, 7 READ, 8 RESTORE, 9 FINAL, or a prefix of it closed by one STOP
with nothing after it; each step of all radios before any record of the next step, and every next step of a radio at
least 1 s of game time (999 ms) after the previous one; exact field sets (state radio, trs, powered, f0, m0, f1, m1;
pre* before every write; BASE min/max/res/target; MUTE want/ret; RETUNE/POWER/POWER_BACK want; RESTORE
setPower/restoreFreqBefore/setFreq/restoreMuteBefore/setMute/ret; READ phase); BENCH_DONE last with value 9 and level=QM_LOCAL (BENCH_ABORTED otherwise);
no SCRIPT (E). Only the stream counts (the dump between dump=BEGIN/END repeats it).
Agreement (OBSERVED_MISMATCH otherwise): MUTE gives m0 = want; RETUNE gives f0 = want = target; POWER and POWER_BACK
give powered = want; the control transceiver 1 is unchanged by MUTE, RETUNE and the READ after it (the stand never
writes it); RESTORE sets exactly what differs from BASE - power by its pre state, frequency by the getter read after
the power call, mute by the getter read after the frequency call (codex #4541: each call can change the next one) - and
FINAL equals BASE in every field.
Findings are descriptive per radio (whether the mute flag of transceiver 0 PERSISTS or CHANGED after the retune, while the
radio is off, after power back; frequency after power back; transceiver 1 during the power steps; any change between two
records without a write is listed as stateChangedSinceObservation - a delayed engine effect, not a proven foreign writer,
codex #4529). The journal dump must report dropped=0 and repeat exactly the stream. IsMuted is the native transceiver
flag, not audibility or RX/TX.
Verdicts QM_LOCAL_OBSERVED / OBSERVED_MISMATCH / STOPPED / LOG_INVALID.
Usage: python -I live-qm-log-check-claude.py <console.log>"""
import hashlib
import json
import re
import sys

LINE = re.compile(r"\[RICON_LIVE\] seq=(\d+) t=(-?\d+) side=(\w+) kind=(\w+) player=(-?\d+) device=(-?\d+) "
                  r"tr=(-?\d+) value=(-?\d+) text=(\S+)$")
FORBIDDEN = {"BENCH_READ", "BENCH_SET_FREQUENCY", "BENCH_MUTE", "BENCH_ENTRY", "BENCH_SKIP", "PLAYER_SAMPLE",
             "PLAYER_ENTRY", "PLAYER_ACT", "PLAYER_READER", "SCREEN"}
PLAN = ["BASE", "MUTE", "RETUNE", "READ", "POWER", "READ", "POWER_BACK", "READ", "RESTORE", "FINAL"]
READ_PHASES = {3: "afterRetune", 5: "powerToggled", 7: "powerBack"}
STATE = ("radio", "trs", "powered", "f0", "m0", "f1", "m1")
WRITES = {"MUTE", "RETUNE", "POWER", "POWER_BACK", "RESTORE"}
EXTRA = {"BASE": {"min", "max", "res", "target"}, "MUTE": {"want", "ret"}, "RETUNE": {"want"}, "POWER": {"want"},
         "POWER_BACK": {"want"}, "RESTORE": {"setPower", "restoreFreqBefore", "setFreq", "restoreMuteBefore", "setMute", "ret"},
         "READ": {"phase"}, "FINAL": set(),
         "STOP": {"reason"}}
INT = re.compile(r"-?\d+")
BIT = ("0", "1")


def fields(text):
    out, dup = {}, False
    for part in text.split(","):
        if "=" in part:
            k, v = part.split("=", 1)
            dup = dup or k in out
            out[k] = v
    return out, dup


def state_keys(f, prefix):
    """State keys present for this record: radio=0 -> only radio, else all of STATE."""
    if f.get(prefix + "radio") == "0":
        return {prefix + "radio"}
    return {prefix + k for k in STATE}


def state(f, prefix=""):
    if f.get(prefix + "radio") != "1":
        return None
    try:
        return {k: int(f[prefix + k]) for k in STATE[1:]}
    except (KeyError, ValueError):
        return None


def check_radio(d, recs, errors, mismatches):
    """Plan, fields and agreement of one radio's QM records; returns the finding row."""
    ops = [(r["value"], r["f"].get("op")) for r in recs]
    stop = next((i for i, (_, op) in enumerate(ops) if op == "STOP"), None)
    if stop is not None and stop != len(ops) - 1:
        errors.append(f"bench {d}: QM after STOP")
    planned = ops if stop is None else ops[:stop]
    if planned != list(enumerate(PLAN))[:len(planned)]:
        errors.append(f"bench {d}: QM sequence {ops} is not a prefix of the plan")
    if stop is not None and ops[stop][0] != len(planned):
        errors.append(f"bench {d}: STOP at step {ops[stop][0]} after {len(planned)} planned steps")
    if stop is None and len(planned) != len(PLAN):
        errors.append(f"bench {d}: QM plan incomplete without STOP")
    last_ms, prev, base, target = None, None, None, None
    row = {"bench": d, "stopped": stop is not None, "stateChangedSinceObservation": []}
    for r in recs:
        f, op, n = r["f"], r["f"].get("op"), r["value"]
        want = {"op", "bench", "elapsedMs"} | EXTRA.get(op, set()) | state_keys(f, "")
        if op == "STOP" and f.get("reason") == "NO_ROOM_FOR_STEP":
            want |= {"min", "max", "res"}  # the range that left no room is logged with the refusal
        if op in WRITES:
            want |= state_keys(f, "pre")
        if set(f) != want:
            errors.append(f"bench {d} step {n} {op}: field set {sorted(set(f) ^ want)}")
            continue
        if not INT.fullmatch(f["elapsedMs"]):
            errors.append(f"bench {d} step {n}: elapsedMs")
            continue
        ms = int(f["elapsedMs"])
        if last_ms is not None and ms - last_ms < 999:
            errors.append(f"bench {d} step {n}: {ms} ms after {last_ms}")
        last_ms = ms
        if op == "STOP":
            row["stopReason"] = f["reason"]
            break
        post = state(f)
        pre = state(f, "pre") if op in WRITES else None
        if post is None or (op in WRITES and pre is None) or any(post[k] not in (0, 1) for k in ("powered",)) \
                or post["m0"] not in (0, 1) or post["m1"] not in (-1, 0, 1):
            errors.append(f"bench {d} step {n} {op}: state values")
            continue
        if op == "READ" and f["phase"] != READ_PHASES.get(n):
            errors.append(f"bench {d} step {n}: READ phase {f['phase']}")
        if op == "BASE":
            if not all(INT.fullmatch(f[k]) for k in ("min", "max", "res", "target")):
                errors.append(f"bench {d}: BASE range fields")
                continue
            base, target = post, int(f["target"])
            res = int(f["res"])
            if res <= 0 or target not in (post["f0"] + res, post["f0"] - res) or \
                    not int(f["min"]) <= target <= int(f["max"]):
                errors.append(f"bench {d}: BASE target {target} is not one step inside the range")
            row.update({"base": base, "target": target})
            prev = post
            continue
        if base is None:
            break
        # a change since the previous record that the stand did not make: before a write (pre) or at a read; listed
        if op in WRITES and pre != prev:
            row["stateChangedSinceObservation"].append({"step": n, "before": prev, "seen": pre})
        elif op in ("READ", "FINAL") and post != prev:
            row["stateChangedSinceObservation"].append({"step": n, "before": prev, "seen": post})
        if op == "MUTE":
            if f["want"] != str(1 - base["m0"]) or f["ret"] not in BIT:
                errors.append(f"bench {d}: MUTE want/ret")
            if post["m0"] != 1 - base["m0"]:
                mismatches.append(f"bench {d}: MUTE not applied")
            if (post["f1"], post["m1"]) != (pre["f1"], pre["m1"]) or post["f0"] != pre["f0"]:
                mismatches.append(f"bench {d}: MUTE changed the control or the frequency")
            row["muteAfterSet"] = post["m0"]
            row["muteRet"] = f["ret"]
        elif op == "RETUNE":
            if f["want"] != str(target):
                errors.append(f"bench {d}: RETUNE want")
            if post["f0"] != target:
                mismatches.append(f"bench {d}: RETUNE not applied")
            if (post["f1"], post["m1"]) != (pre["f1"], pre["m1"]):
                mismatches.append(f"bench {d}: RETUNE changed the control")
            row["muteAfterRetune"] = "PERSISTS" if post["m0"] == pre["m0"] else "CHANGED"
        elif op == "READ":
            if n == 3:
                if (post["f1"], post["m1"]) != (prev["f1"], prev["m1"]):
                    mismatches.append(f"bench {d}: control changed after the retune")
                row["muteAfterRetuneT1"] = "PERSISTS" if post["m0"] == 1 - base["m0"] else "CHANGED"
                row["freqAfterRetuneT1"] = post["f0"]
            elif n == 5:
                row["muteWhilePowerToggledT1"] = "PERSISTS" if post["m0"] == 1 - base["m0"] else "CHANGED"
                row["poweredWhileToggledT1"] = post["powered"]
            elif n == 7:
                row["muteAfterPowerBackT1"] = "PERSISTS" if post["m0"] == 1 - base["m0"] else "CHANGED"
                row["freqAfterPowerBackT1"] = "PERSISTS" if post["f0"] == target else "CHANGED"
                row["controlAfterPowerBackT1"] = "UNCHANGED" if (post["f1"], post["m1"]) == (base["f1"], base["m1"]) \
                    else "CHANGED"
        elif op in ("POWER", "POWER_BACK"):
            if f["want"] != str(1 - base["powered"] if op == "POWER" else base["powered"]):
                errors.append(f"bench {d}: {op} want")
            if post["powered"] != int(f["want"]):
                mismatches.append(f"bench {d}: {op} not applied")
            key = "Off" if op == "POWER" else "Back"
            row[f"muteAtPower{key}"] = "PERSISTS" if post["m0"] == pre["m0"] else "CHANGED"
            row[f"freqAtPower{key}"] = "PERSISTS" if post["f0"] == pre["f0"] else "CHANGED"
            row[f"controlAtPower{key}"] = "UNCHANGED" if (post["f1"], post["m1"]) == (pre["f1"], pre["m1"]) else "CHANGED"
        elif op == "RESTORE":
            flags = {k: f[k] for k in ("setPower", "setFreq", "setMute")}
            if not INT.fullmatch(f["restoreFreqBefore"]) or f["restoreMuteBefore"] not in BIT:
                errors.append(f"bench {d}: RESTORE before-values")
                continue
            # codex #4541: the decision for each call is made on the fresh getter after the previous call
            need = {"setPower": str(int(pre["powered"] != base["powered"])),
                    "setFreq": str(int(int(f["restoreFreqBefore"]) != base["f0"])),
                    "setMute": str(int(int(f["restoreMuteBefore"]) != base["m0"]))}
            if need["setPower"] == "0" and f["restoreFreqBefore"] != str(pre["f0"]):
                errors.append(f"bench {d}: RESTORE frequency moved without a power call")
            if need["setPower"] == "0" and need["setFreq"] == "0" and f["restoreMuteBefore"] != str(pre["m0"]):
                errors.append(f"bench {d}: RESTORE mute moved without a power or frequency call")
            ret_ok = f["ret"] == "-1" if need["setMute"] == "0" else f["ret"] in BIT
            if flags != need or not ret_ok:
                errors.append(f"bench {d}: RESTORE flags {flags} != {need} or ret {f['ret']}")
            if (post["powered"], post["f0"], post["m0"]) != (base["powered"], base["f0"], base["m0"]):
                mismatches.append(f"bench {d}: RESTORE did not reach BASE")
            row["restoreActions"] = flags
        elif op == "FINAL":
            if post != base:
                mismatches.append(f"bench {d}: FINAL differs from BASE")
            row["restored"] = post == base
        prev = post
    return row


def main(path):
    raw = open(path, "rb").read()
    lines = raw.decode("utf-8", errors="replace").splitlines()
    errors, mismatches = [], []
    if [l for l in lines if re.search(r"SCRIPT\s*\(E\)", l)]:
        errors.append("SCRIPT (E) present")
    events, in_dump, seen, dumps, dumped = [], False, set(), [], []
    for l in lines:
        if "[RICON_LIVE] dump=BEGIN" in l:
            in_dump = True
            dumps.append(l[l.index("[RICON_LIVE]"):].strip())
            continue
        if "[RICON_LIVE] dump=END" in l:
            in_dump = False
            continue
        m = LINE.search(l)
        if in_dump and m:
            dumped.append(m.group(0))
        if in_dump or not m:
            continue
        if m.group(1) in seen:
            errors.append(f"stream seq {m.group(1)} repeated")
        seen.add(m.group(1))
        f, dup = fields(m.group(9))
        if dup:
            errors.append(f"seq {m.group(1)}: duplicate field")
        events.append({"kind": m.group(4), "value": int(m.group(8)), "tr": m.group(7), "f": f, "line": m.group(0)})
    # codex #4529: a dropped journal event can never become QM_LOCAL_OBSERVED; the dump repeats the stream exactly
    if len(dumps) != 1 or not re.fullmatch(r"\[RICON_LIVE\] dump=BEGIN count=(\d+) dropped=0 nextSeq=\1", dumps[0]) \
            or int(dumps[0].split("count=")[1].split()[0]) != len(events) or dumped != [e["line"] for e in events]:
        errors.append(f"journal dump {dumps[:1]} does not repeat the {len(events)} stream events with dropped=0")
    starts = [e for e in events if e["kind"] == "BENCH_START"]
    start_f = starts[0]["f"] if len(starts) == 1 else {}
    if len(starts) != 1:
        errors.append(f"BENCH_START count {len(starts)}")
    elif start_f.get("mode") != "radiomute" or start_f.get("modeGiven") != "1":
        errors.append(f"BENCH_START mode={start_f.get('mode')} modeGiven={start_f.get('modeGiven')}")
    for e in events:
        if e["kind"] == "BENCH_NOT_READY":
            errors.append("BENCH_NOT_READY present")
    actions = sorted({e["kind"] for e in events if e["kind"] in FORBIDDEN})
    if actions:
        errors.append(f"old radio phases or player records in a radiomute run: {actions}")
    spawns = [e for e in events if e["kind"] == "BENCH_SPAWN"]
    qm = [e for e in events if e["kind"] == "QM"]
    by_radio = {}
    for e in qm:
        d = e["f"].get("bench")
        if e["tr"] != "-1" or d is None or not INT.fullmatch(d):
            errors.append(f"QM record without bench/tr: {e['f']}")
            continue
        by_radio.setdefault(int(d), []).append(e)
    if sorted(by_radio) != list(range(len(spawns))):
        errors.append(f"QM radios {sorted(by_radio)} != spawned 0..{len(spawns) - 1}")
    # every step of all radios before any record of the next step
    steps = [e["value"] for e in qm]
    if steps != sorted(steps):
        errors.append("QM steps not in stream order")
    rows = [check_radio(d, by_radio[d], errors, mismatches) for d in sorted(by_radio)]
    done = [e for e in events if e["kind"] == "BENCH_DONE"]
    aborted = [e for e in events if e["kind"] == "BENCH_ABORTED"]
    if aborted or len(done) != 1 or done[0]["value"] != len(PLAN) - 1 or done[0]["f"].get("level") != "QM_LOCAL" \
            or not events or events[-1]["kind"] != "BENCH_DONE":
        errors.append("BENCH_DONE must be the single last event with value 9 and level=QM_LOCAL")
    stopped = any(r["stopped"] for r in rows)
    overall = ("LOG_INVALID" if errors else "STOPPED" if stopped else "OBSERVED_MISMATCH" if mismatches else
               "QM_LOCAL_OBSERVED")
    print(json.dumps({"logSha256": hashlib.sha256(raw).hexdigest(), "errors": errors, "mismatches": mismatches,
                      "overall": overall, "radios": rows,
                      "spawned": [e["f"].get("actualPrefab") for e in spawns],
                      "start": {k: start_f.get(k) for k in ("isServer", "isClient", "playerController", "mode",
                                                            "modeGiven", "radioManager")}},
                     ensure_ascii=False, indent=1))
    print("LIVE_QM_LOG_CHECK_" + ("PASS" if not errors else "FAIL"))
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1]))
