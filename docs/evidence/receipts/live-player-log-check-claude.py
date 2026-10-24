#!/usr/bin/env python3
"""claude: check and summary of an A1-P0 log (RICON_LiveRadioBenchComponent with -riconBenchMode=player, claude #4360,
codex #4365): read-only observation of the local player in Workbench Play.
Checks: one BENCH_START with mode=player, modeGiven=1; no radio or control action at all (no BENCH_SPAWN, _READ, _SKIP,
_SET_FREQUENCY, _MUTE, _ENTRY in the stream); exactly 30 PLAYER_SAMPLE with values 1..30 in stream order, the first
at elapsedMs >= 1000 and each next at least 1 s of game time after the previous one (no catch-up burst after a stall,
codex #4372); every sample has a consistent field set (pc=0 -> no player fields; pc=1 -> playerId,
vonController (+ vonEntries), controlled; controlled=1 -> controlledPrefab decoded by the strict shared codec, entityVoN,
gadgetManager); BENCH_DONE last with value 30 and level=PLAYER_OBSERVATION; no SCRIPT (E). Only the stream counts (the
dump between dump=BEGIN/END repeats it). The finding is descriptive: LOCAL_PLAYER_ABSENT (pc=0 in every sample),
LOCAL_PLAYER_PRESENT (pc=1 in every sample), LOCAL_PLAYER_CHANGED (otherwise, with the first sample of each state).
Usage: python -I live-player-log-check-claude.py <console.log>"""
import hashlib
import importlib.util
import json
import pathlib
import re
import sys

sys.dont_write_bytecode = True  # no __pycache__ in the receipts folder
_spec = importlib.util.spec_from_file_location(
    "live_text_codec_claude", pathlib.Path(__file__).resolve().parent / "live_text_codec_claude.py")
codec = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(codec)

LINE = re.compile(r"\[RICON_LIVE\] seq=(\d+) t=(-?\d+) side=(\w+) kind=(\w+) player=(-?\d+) device=(-?\d+) "
                  r"tr=(-?\d+) value=(-?\d+) text=(\S+)$")
FORBIDDEN = {"BENCH_SPAWN", "BENCH_READ", "BENCH_SKIP", "BENCH_SET_FREQUENCY", "BENCH_MUTE", "BENCH_ENTRY"}
SAMPLES = 30
BIT = ("0", "1")
INT = re.compile(r"-?\d+")


def fields(text):
    out = {}
    for part in text.split(","):
        if "=" in part:
            k, v = part.split("=", 1)
            out[k] = v
    return out


def sample_errors(f):
    """Field-set consistency of one PLAYER_SAMPLE; returns (errors, decoded prefab or None)."""
    errs, prefab = [], None
    base = {"elapsedMs", "isServer", "isClient", "players", "pc"}
    want = set(base)
    if f.get("isServer") not in BIT or f.get("isClient") not in BIT or f.get("pc") not in BIT:
        errs.append("role/pc not 0|1")
    if not INT.fullmatch(f.get("players", "")) or not INT.fullmatch(f.get("elapsedMs", "")):
        errs.append("players/elapsedMs not an integer")
    if f.get("pc") == "1":
        want |= {"playerId", "vonController", "controlled"}
        if not INT.fullmatch(f.get("playerId", "")) or f.get("vonController") not in BIT or f.get("controlled") not in BIT:
            errs.append("player fields")
        if f.get("vonController") == "1":
            want.add("vonEntries")
            if not INT.fullmatch(f.get("vonEntries", "")):
                errs.append("vonEntries not an integer")
        if f.get("controlled") == "1":
            want |= {"controlledPrefab", "entityVoN", "gadgetManager"}
            if f.get("entityVoN") not in BIT or f.get("gadgetManager") not in BIT:
                errs.append("controlled fields")
            try:
                prefab = codec.unescape(f.get("controlledPrefab", ""))
            except ValueError as exc:
                errs.append(f"controlledPrefab: {exc}")
    if set(f) != want:
        errs.append(f"field set {sorted(set(f) ^ want)}")
    return errs, prefab


def main(path):
    raw = open(path, "rb").read()
    lines = raw.decode("utf-8", errors="replace").splitlines()
    errors = []
    if [l for l in lines if re.search(r"SCRIPT\s*\(E\)", l)]:
        errors.append("SCRIPT (E) present")
    events, in_dump, seen = [], False, set()
    for l in lines:
        if "[RICON_LIVE] dump=BEGIN" in l:
            in_dump = True
            continue
        if "[RICON_LIVE] dump=END" in l:
            in_dump = False
            continue
        m = LINE.search(l)
        if in_dump or not m:
            continue
        if m.group(1) in seen:
            errors.append(f"stream seq {m.group(1)} repeated")
        seen.add(m.group(1))
        events.append({"kind": m.group(4), "value": int(m.group(8)), "text": m.group(9), "f": fields(m.group(9))})
    starts = [e for e in events if e["kind"] == "BENCH_START"]
    start_f = starts[0]["f"] if len(starts) == 1 else {}
    if len(starts) != 1:
        errors.append(f"BENCH_START count {len(starts)}")
    elif start_f.get("mode") != "player" or start_f.get("modeGiven") != "1":
        errors.append(f"BENCH_START mode={start_f.get('mode')} modeGiven={start_f.get('modeGiven')}")
    not_ready = [e for e in events if e["kind"] == "BENCH_NOT_READY"]
    for e in not_ready:
        errors.append("BENCH_NOT_READY: " + e["text"])
    actions = sorted({e["kind"] for e in events if e["kind"] in FORBIDDEN})
    if actions:
        errors.append(f"not read-only: {actions}")
    samples = [e for e in events if e["kind"] == "PLAYER_SAMPLE"]
    if [s["value"] for s in samples] != list(range(1, SAMPLES + 1)):
        errors.append(f"PLAYER_SAMPLE values {[s['value'] for s in samples]} != 1..{SAMPLES}")
    rows, last_ms = [], -1
    for s in samples:
        errs, prefab = sample_errors(s["f"])
        for x in errs:
            errors.append(f"sample {s['value']}: {x}")
        ms = int(s["f"]["elapsedMs"]) if INT.fullmatch(s["f"].get("elapsedMs", "")) else -1
        # codex #4372, #4379: samples at least 1 s of game time apart; with b + 1 s <= a the truncated ms differ by at
        # least 1000, 999 leaves one millisecond for binary32 rounding of elapsed x 1000
        if ms < 1000 or (last_ms >= 0 and ms - last_ms < 999):
            errors.append(f"sample {s['value']}: elapsedMs {ms} after {last_ms}")
        last_ms = ms
        rows.append({"n": s["value"], "pc": s["f"].get("pc"), "playerId": s["f"].get("playerId"),
                     "controlled": s["f"].get("controlled"), "prefab": prefab, "players": s["f"].get("players"),
                     "vonController": s["f"].get("vonController"), "vonEntries": s["f"].get("vonEntries"),
                     "entityVoN": s["f"].get("entityVoN"), "gadgetManager": s["f"].get("gadgetManager"),
                     "isServer": s["f"].get("isServer"), "isClient": s["f"].get("isClient")})
    done = [e for e in events if e["kind"] == "BENCH_DONE"]
    if "BENCH_ABORTED" in {e["kind"] for e in events}:
        errors.append("BENCH_ABORTED present")
    if len(done) != 1 or done[0]["value"] != SAMPLES or done[0]["f"].get("level") != "PLAYER_OBSERVATION" \
            or not events or events[-1]["kind"] != "BENCH_DONE":
        errors.append("BENCH_DONE must be the single last event with value 30 and level=PLAYER_OBSERVATION")
    # codex #4372: a change of playerId (or of the player count / entries) with the same prefab is a state change too
    states = [tuple(r[k] for k in ("pc", "playerId", "controlled", "prefab", "players", "vonController", "vonEntries",
                                   "entityVoN", "gadgetManager", "isServer", "isClient")) for r in rows]
    changes = [rows[i]["n"] for i in range(len(rows)) if i == 0 or states[i] != states[i - 1]]
    if not rows:
        finding = None
    elif all(r["pc"] == "0" for r in rows):
        finding = "LOCAL_PLAYER_ABSENT"
    elif all(r["pc"] == "1" for r in rows) and len(changes) == 1:
        finding = "LOCAL_PLAYER_PRESENT"
    else:
        finding = "LOCAL_PLAYER_CHANGED"
    if not_ready:
        overall = "NOT_READY"
    elif errors:
        overall = "LOG_INVALID"
    else:
        overall = "PLAYER_OBSERVATION_COMPLETE"
    print(json.dumps({"logSha256": hashlib.sha256(raw).hexdigest(), "errors": errors, "overall": overall,
                      "finding": finding, "stateChangesAtSample": changes,
                      "start": {k: start_f.get(k) for k in ("isServer", "isClient", "playerController", "factionManager",
                                                            "mode", "modeGiven", "radioManager")},
                      "samples": rows}, ensure_ascii=False, indent=1))
    print("LIVE_PLAYER_LOG_CHECK_" + ("PASS" if not errors else "FAIL"))
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1]))
