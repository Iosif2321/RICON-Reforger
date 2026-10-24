"""claude: positive and negative controls for live-endpoint-log-check-claude.py (B0a, codex 0a039a2, claude #4577).
Synthetic logs in the journal format of RICON_LiveEndpointProbe: launcher lines, one or two observation cycles (START,
BINDING when the binding changes, SAMPLE with the bench PlayerState and members, RADIO_STATE of the reader, DONE or
ABORTED, the cycle's dump). The default cycle has no local player on samples 1-2, a controller without a character on 3
and the AN/PRC-68 player from 4 on. The values are test values for the parser.
Usage: python -I live-endpoint-log-check-controls-claude.py <repo> <outdir>"""
import json
import subprocess
import sys

repo, out = sys.argv[1], sys.argv[2]
checker = repo + "/docs/evidence/receipts/live-endpoint-log-check-claude.py"
LAUNCH = ["[RICON_LIVEB] state=ENDPOINT_ONLY bench=0 saved=0", "[RICON_LIVEB] state=PLAY_REQUESTED isGameModeNow=0",
          "[RICON_LIVEB] watch t=1000ms isGameMode=1 fps=60"]
PREFAB = "{26A9756790131354}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Rifleman.et"
RADIO = "{73950FBA2D7DB5C5}Prefabs/Items/Equipment/Radios/Radio_ANPRC68.et"


def state(n, b):
    """(pc, controlled, playerId) of sample n."""
    if b.get("never"):
        return 0, 0, -1
    if n <= 2:
        return 0, 0, -1
    if n == 3:
        return 1, 0, 1
    return 1, 1, b.get("pid", 1)


def player_state(pc, controlled, pid):
    text = "isServer=1,isClient=0,players=1,pc=" + str(pc)
    if not pc:
        return text
    text += f",playerId={pid},vonController=1,vonEntries=2,controlled={controlled}"
    if controlled:
        text += f",controlledPrefab={PREFAB},entityVoN=1,gadgetManager=1"
    return text


def cycle(ordinal, b):
    """Rows (kind, player, tr, value, text) of one cycle."""
    samples = b.get("samples", 30)
    start = f"cycle={ordinal},limit=30,isServer=1,isClient=0" + b.get("start_rpl", ",rplMode=1") + b.get("start_extra", "")
    rows = [("ENDPOINT_START", -1, -1, 0, start)]
    rev, prev, ms = 0, None, 1003
    for n in range(1, samples + 1):
        pc, controlled, pid = state(n, b)
        cur = (pc, controlled, pid)
        if (prev is None or cur != prev) and b.get("skip_binding_at") != n:
            rev += 2 if b.get("rev_skip_at") == n else 1
            rows.append(("ENDPOINT_BINDING", pid, -1, n,
                         f"cycle={ordinal},revision={rev},initial={int(prev is None)},pcChanged={int(prev is None or pc != prev[0])},"
                         f"entityChanged={int(prev is None or controlled != prev[1])},idChanged={int(prev is None or pid != prev[2])},"
                         f"pc={pc},controlled={controlled},previousPlayerId={-1 if prev is None else prev[2]}"))
        members = (b.get("members_at", {}).get(n) or b.get("members", "1")) if pc else "NONE"
        shown_rev = rev - 1 if b.get("stale_rev_at") == n else rev
        rows.append(("ENDPOINT_SAMPLE", pid, -1, n,
                     f"cycle={ordinal},elapsedMs={ms},bindingRevision={shown_rev},{player_state(pc, controlled, pid)},"
                     f"members={members}" + b.get("sample_rpl", ",rplMode=1") + (b.get("sample_extra", "") if n == 5 else "")))
        if controlled and (prev is None or prev[1] == 0):
            for t, f in ((0, 32000), (1, 48000)):
                extra = b.get("radio_extra", "")
                rows.append(("RADIO_STATE", pid if not b.get("radio_player_wrong") else 7, t, f,
                             f"binding=LOCAL_ONLY,obs={t},slot=0,prefab={RADIO},min=32000,max=68000,res=200,powered=1,muted=0{extra}"))
        ms += b.get("step_ms", 1004)
        prev = cur
    end_kind = b.get("end_kind", "ENDPOINT_DONE" if samples == 30 else "ENDPOINT_ABORTED")
    reason = b.get("reason", "LIMIT" if end_kind == "ENDPOINT_DONE" else "GAME_END")
    rows.append((end_kind, prev[2] if prev else -1, -1, samples, f"cycle={ordinal},samples={samples},reason={reason}"))
    rows += [(k, p, tr, v, t) for k, p, tr, v, t in b.get("extra_rows", [])]
    return rows


def render(rows, seq0=0, drop=0, skip_dump=False):
    lines = [f"[RICON_LIVE] seq={seq0 + i} t={1000 + i} side=probe kind={k} player={p} device=-1 tr={tr} value={v} text={t}"
             for i, (k, p, tr, v, t) in enumerate(rows)]
    dump = lines[:-1] if skip_dump else lines
    body = [f"12:00:00.000 SCRIPT : {l}" for l in lines]
    body.append(f"12:00:30.000 SCRIPT : [RICON_LIVE] dump=BEGIN count={len(lines)} dropped={drop} nextSeq={seq0 + len(lines)}")
    body += [f"12:00:30.000 SCRIPT : {l}" for l in dump]
    body.append("12:00:30.000 SCRIPT : [RICON_LIVE] dump=END")
    return body


def build(cycles=None, launcher=True, extra_lines=(), seq_continue=False, drop=0, skip_dump=False, restart_marks=None):
    cycles = cycles if cycles is not None else [{}]
    body = [f"11:59:59.000 SCRIPT : {l}" for l in LAUNCH] if launcher else []
    # the launcher's restart lines go between the cycles (codex 15efc1a)
    marks = ["EDIT_REQUESTED", "EDIT_STABLE checks=51", "SECOND_PLAY_REQUESTED", "SECOND_PLAY_ENTERED"] if restart_marks is None and len(cycles) == 2 else (restart_marks or [])
    seq = 0
    for i, b in enumerate(cycles):
        rows = cycle(b.get("ordinal", i + 1), b)
        body += render(rows, seq if seq_continue else 0, drop, skip_dump)
        if i == 0:
            body += [f"12:00:31.000 SCRIPT : [RICON_LIVEB] restart={m}" for m in marks]
        seq += len(rows)
    body += list(extra_lines)
    return "\n".join(body) + "\n"


OFF = ["off"]
ON = ["on"]
RE = ["restart"]
cases = [
    ("off_clean", build(cycles=[]), OFF, 0, "ENDPOINT_GATE_OFF_CLEAN"),
    ("off_with_live_line", build(cycles=[{"samples": 3}]), OFF, 1, "LOG_INVALID"),
    ("off_old_bench", build(cycles=[], extra_lines=["12:00:00.000 SCRIPT : [RICON_LIVEB] lifecycle=EOnInit world=1"]),
     OFF, 1, "LOG_INVALID"),
    ("off_without_endpoint_only", build(cycles=[], launcher=False), OFF, 1, "LOG_INVALID"),
    ("off_never_played", build(cycles=[]).replace("isGameMode=1", "isGameMode=0"), OFF, 1, "LOG_INVALID"),
    ("on_complete", build(), ON, 0, "ENDPOINT_CYCLE_COMPLETE", "LOCAL_ROLE_READY"),
    # codex #4579: rplMode is required in START and every SAMPLE
    ("on_no_rplmode", build(cycles=[{"start_rpl": "", "sample_rpl": ""}]), ON, 1, "LOG_INVALID"),
    ("on_rplmode_start_missing", build(cycles=[{"start_rpl": ""}]), ON, 1, "LOG_INVALID"),
    ("on_rplmode_sample_missing", build(cycles=[{"sample_rpl": ""}]), ON, 1, "LOG_INVALID"),
    ("on_rplmode_not_int", build(cycles=[{"sample_rpl": ",rplMode=Listen"}]), ON, 1, "LOG_INVALID"),
    ("on_never_player", build(cycles=[{"never": True}]), ON, 0, "ENDPOINT_CYCLE_COMPLETE", "LOCAL_ROLE_NOT_READY"),
    ("on_self_not_member", build(cycles=[{"members": "2"}]), ON, 0, "ENDPOINT_CYCLE_COMPLETE", "LOCAL_ROLE_NOT_READY"),
    # codex #4585: membership must hold on every sample of the readiness window, not only the last one
    ("on_members_ready_only_last", build(cycles=[{"members_at": {26: "UNKNOWN", 27: "UNKNOWN", 28: "UNKNOWN", 29: "UNKNOWN"}}]),
     ON, 0, "ENDPOINT_CYCLE_COMPLETE", "LOCAL_ROLE_NOT_READY"),
    ("on_aborted", build(cycles=[{"samples": 12}]), ON, 0, "ENDPOINT_CYCLE_ABORTED"),
    ("on_bad_reason", build(cycles=[{"samples": 12, "reason": "TIMEOUT"}]), ON, 1, "LOG_INVALID"),
    ("on_done_early", build(cycles=[{"samples": 29, "end_kind": "ENDPOINT_DONE", "reason": "LIMIT"}]), ON, 1, "LOG_INVALID"),
    ("on_missing_binding", build(cycles=[{"skip_binding_at": 4}]), ON, 1, "LOG_INVALID"),
    ("on_revision_skip", build(cycles=[{"rev_skip_at": 3}]), ON, 1, "LOG_INVALID"),
    ("on_stale_revision", build(cycles=[{"stale_rev_at": 4}]), ON, 1, "LOG_INVALID"),
    ("on_too_fast", build(cycles=[{"step_ms": 500}]), ON, 1, "LOG_INVALID"),
    ("on_dump_dropped", build(drop=1), ON, 1, "LOG_INVALID"),
    ("on_dump_short", build(skip_dump=True), ON, 1, "LOG_INVALID"),
    ("on_von_record", build(cycles=[{"extra_rows": [("TX_CAPTURE", 1, -1, 0, "q=1")]}]), ON, 1, "LOG_INVALID"),
    ("on_bench_record", build(cycles=[{"extra_rows": [("BENCH_START", -1, -1, -1, "mode=radio")]}]), ON, 1, "LOG_INVALID"),
    ("on_two_cycles", build(cycles=[{"samples": 5}, {}]), ON, 1, "LOG_INVALID"),
    ("on_start_extra_field", build(cycles=[{"start_extra": ",key=abc"}]), ON, 1, "LOG_INVALID"),
    ("on_sample_extra_field", build(cycles=[{"sample_extra": ",name=x"}]), ON, 1, "LOG_INVALID"),
    ("on_radio_extra_field", build(cycles=[{"radio_extra": ",key=abc"}]), ON, 1, "LOG_INVALID"),
    ("on_radio_player_wrong", build(cycles=[{"radio_player_wrong": True}]), ON, 1, "LOG_INVALID"),
    ("on_members_name", build(cycles=[{"members": "Ivan"}]), ON, 1, "LOG_INVALID"),
    ("on_script_error", build(extra_lines=["12:00:31.000 SCRIPT    (E): something"]), ON, 1, "LOG_INVALID"),
    ("restart_ok", build(cycles=[{"samples": 8}, {}]), RE, 0, "ENDPOINT_RESTART_COMPLETE"),
    ("restart_world_cleanup", build(cycles=[{"samples": 4, "reason": "WORLD_CLEANUP"}, {}]), RE, 0, "ENDPOINT_RESTART_COMPLETE"),
    ("restart_not_exercised", build(cycles=[{}, {}]), RE, 0, "ENDPOINT_RESTART_NOT_EXERCISED"),
    ("restart_too_early", build(cycles=[{"samples": 2}, {}]), RE, 1, "LOG_INVALID"),
    ("restart_reset_reason", build(cycles=[{"samples": 8, "reason": "GAME_START_RESET"}, {}]), RE, 1, "LOG_INVALID"),
    ("restart_second_aborted", build(cycles=[{"samples": 8}, {"samples": 10}]), RE, 1, "LOG_INVALID"),
    # codex #4594: a recreated game object may count the second cycle from 1 again; 3 is never valid
    ("restart_cycle_recreated", build(cycles=[{"samples": 8}, {"ordinal": 1}]), RE, 0, "ENDPOINT_RESTART_COMPLETE"),
    ("restart_cycle_number", build(cycles=[{"samples": 8}, {"ordinal": 3}]), RE, 1, "LOG_INVALID"),
    ("restart_no_entered", build(cycles=[{"samples": 8}, {}], restart_marks=["EDIT_REQUESTED", "EDIT_STABLE checks=51",
                                                                             "SECOND_PLAY_REQUESTED"]), RE, 1, "LOG_INVALID"),
    ("restart_no_stable", build(cycles=[{"samples": 8}, {}], restart_marks=["EDIT_REQUESTED", "SECOND_PLAY_REQUESTED",
                                                                            "SECOND_PLAY_ENTERED"]), RE, 1, "LOG_INVALID"),
    ("restart_seq_not_reset", build(cycles=[{"samples": 8}, {}], seq_continue=True), RE, 1, "LOG_INVALID"),
    ("restart_one_cycle", build(), RE, 1, "LOG_INVALID"),
    ("restart_no_marks", build(cycles=[{"samples": 8}, {}], restart_marks=[]), RE, 1, "LOG_INVALID"),
    ("restart_refused_mark", build(cycles=[{"samples": 8}, {}], restart_marks=["EDIT_REQUESTED", "REFUSED"]), RE, 1,
     "LOG_INVALID"),
]
passed = 0
for name, text_, args, rc_expected, overall_expected, *role_expected in cases:
    path = f"{out}/{name}.log"
    open(path, "w", encoding="utf-8").write(text_)
    run = subprocess.run([sys.executable, "-X", "utf8", "-I", checker, path, *args], capture_output=True, text=True,
                         encoding="utf-8")
    try:
        report = json.loads(run.stdout[:run.stdout.rindex("}") + 1])
    except ValueError:
        report = {"overall": "NO_REPORT", "errors": [run.stderr[-200:]], "cycles": []}
    roles = [c.get("localRole") for c in report.get("cycles", [])]
    ok = run.returncode == rc_expected and report["overall"] == overall_expected         and (not role_expected or roles[-1:] == role_expected)
    passed += ok
    print(f"{'PASS' if ok else 'FAIL'} {name}: rc={run.returncode} overall={report['overall']} role={roles} "
          f"errors={report['errors'][:2]}")
print(f"ENDPOINT_PARSER_CONTROLS {passed}/{len(cases)}")
sys.exit(0 if passed == len(cases) else 1)
