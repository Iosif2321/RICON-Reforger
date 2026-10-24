"""claude: positive and negative controls for the A1-P2 'menu' checks of live-player-log-check-claude.py (claude #4399,
codex #4388/#4390/#4402). Synthetic playermenu logs in the journal format, modelled on the real A1-P1 entries (the
player's AN/PRC-68: transceiver 1 at 32000 raw active for transmission, transceiver 2 at 48000 not active): 30 samples
with 2 entries each and the planned PLAYER_ACT sequence on the NON-active entry idx 1 (48000 -> 48200 -> 48000, mute
0 -> 1 -> 0). A STOP aborts the run at its sample (BENCH_ABORTED). The values are test values for the parser.
Usage: python -I live-player-log-check-menu-controls-claude.py <repo> <outdir>"""
import json
import subprocess
import sys

repo, out = sys.argv[1], sys.argv[2]
checker = repo + "/docs/evidence/receipts/live-player-log-check-claude.py"
START = ("level=NATIVE_ROUNDTRIP,prefabs=4,isServer=1,isClient=0,from=EOnInit,activeBefore=1,radioManager=0,"
         "expectedManager=-1,setter=component,setterGiven=0,playerController=0,factionManager=1,mode=playermenu,modeGiven=1")
STATE = ("isServer=1,isClient=0,players=1,pc=1,playerId=1,vonController=1,vonEntries=2,controlled=1,controlledPrefab="
         "{26A9756790131354}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Rifleman.et,entityVoN=1,gadgetManager=1")
RADIO = "{73950FBA2D7DB5C5}Prefabs/Items/Equipment/Radios/Radio_ANPRC68.et"
DONE = "level=PLAYER_MENU_RUNTIME,not=SCREEN|SPAWN|CONTROL|AUDIO|REPLICATION"


def text(raw):
    return f"{raw / 1000:.1f}%20#AR-VON_FrequencyUnits_MHz"


def entry(idx, raw, muted, active):
    return (f"idx={idx},entry=1,cls=SCR_VONEntryRadio,method=1,active={active},transmit={active},usable=1,dtext={text(raw)},"
            f"radioEntry=1,trNumber={idx + 1},entryFreq={raw},emuted={muted},gadget=1,longRange=0,transceiver=1,raw={raw},"
            f"min=32000,max=68000,res=200,muted={muted},radioPrefab={RADIO},editorRadio=0,trIndex={idx},inventorySlot=0,"
            f"gadgetOwnerMatch=1")


def state(raw, muted):
    return f"bound=1,raw={raw},entryFreq={raw},muted={muted},emuted={muted},dtext={text(raw)}"


def plan():
    return {
        5: [("CHOOSE", "trNumber=2,min=32000,max=68000,res=200,raw0=48000,muted0=0,target=48200,modifier=1,slot=0,"
             + state(48000, 0))],
        6: [("STEP", "phase=t0,sinceStepMs=0,target=48200," + state(48200, 0))],
        7: [("READ", "phase=t1,sinceStepMs=1004,target=48200," + state(48200, 0))],
        11: [("READ", "phase=t5,sinceStepMs=5021,target=48200," + state(48200, 0))],
        12: [("RESTORE", "raw0=48000," + state(48000, 0))],
        14: [("MUTE", "ret=1,muted0=0," + state(48000, 1))],
        15: [("UNMUTE", "ret=0,muted0=0," + state(48000, 0))],
        20: [("FINAL", "raw0=48000,muted0=0," + state(48000, 0))],
    }


def default_entries(n):
    # entries are recorded before the act of the same sample: 48200 on samples 7..12, muted on sample 15
    return [entry(0, 32000, 0, 1), entry(1, 48200 if 7 <= n <= 12 else 48000, 1 if n == 15 else 0, 0)]


def build(acts=None, entries=default_entries, start=START, move=None, last=30, end=None):
    acts = plan() if acts is None else acts
    rows = [("BENCH_START", -1, start)]
    for n in range(1, last + 1):
        rows.append(("PLAYER_SAMPLE", n, f"elapsedMs={1000 * n + 7}," + STATE))
        rows += [("PLAYER_ENTRY", n, e) for e in entries(n)]
        rows += [("PLAYER_ACT", n, f"op={op},idx=1,{body}") for op, body in acts.get(n, [])]
    rows.append(end or ("BENCH_DONE", 30, DONE))
    if move:
        rows = move(rows)
    return "\n".join(f"12:00:00.000 SCRIPT : [RICON_LIVE] seq={i} t={i} side=probe kind={k} player=-1 device=-1 tr=-1 "
                     f"value={v} text={t}" for i, (k, v, t) in enumerate(rows))


def with_acts(**changes):
    p = plan()
    for key, value in changes.items():
        n = int(key[1:])
        if value is None:
            p.pop(n, None)
        else:
            p[n] = value
    return p


def stop_at(n, reason, keep):
    p = {k: v for k, v in plan().items() if k < n and k <= keep}
    p[n] = [("STOP", f"reason={reason}," + state(48400, 0))]
    return p


ABORT = lambda n: ("BENCH_ABORTED", n, f"level=PLAYER_MENU_RUNTIME,stopped=1,samples={n}")


def move_act(rows):
    i = next(k for k, r in enumerate(rows) if r[0] == "PLAYER_ACT" and r[1] == 6)
    act = rows.pop(i)
    j = next(k for k, r in enumerate(rows) if r[0] == "PLAYER_SAMPLE" and r[1] == 8)
    rows.insert(j, act)
    return rows


M = ["menu"]
cases = [
    ("menu_ok", build(), M, 0, "PLAYER_MENU_RUNTIME_PASS"),
    ("stop_before_restore", build(stop_at(12, "BEFORE_RESTORE", 11), last=12, end=ABORT(12)), M, 0, "STOPPED"),
    ("stop_no_candidate", build({5: [("STOP", "reason=NO_CANDIDATE,bound=0")]}, last=5, end=ABORT(5)), M, 0, "STOPPED"),
    ("stop_but_done", build(stop_at(12, "BEFORE_RESTORE", 11), last=12, end=("BENCH_DONE", 12, DONE)), M, 1, "LOG_INVALID"),
    ("stop_samples_continue", build(stop_at(12, "BEFORE_RESTORE", 11), last=13, end=ABORT(13)), M, 1, "LOG_INVALID"),
    ("abort_without_stop", build(last=30, end=("BENCH_ABORTED", 30, "level=PLAYER_MENU_RUNTIME,stopped=0,samples=30")),
     M, 1, "LOG_INVALID"),
    ("act_after_stop", build({**stop_at(12, "BEFORE_RESTORE", 11), 13: plan()[14]}, last=13, end=ABORT(12)),
     M, 1, "LOG_INVALID"),
    ("step_not_applied", build(with_acts(s6=[("STEP", "phase=t0,sinceStepMs=0,target=48200," + state(48000, 0))])),
     M, 0, "OBSERVED_MISMATCH"),
    ("restore_wrong", build(with_acts(s12=[("RESTORE", "raw0=48000," + state(48200, 0))])), M, 0, "OBSERVED_MISMATCH"),
    ("mute_return_wrong", build(with_acts(s14=[("MUTE", "ret=0,muted0=0," + state(48000, 1))])), M, 0, "OBSERVED_MISMATCH"),
    ("mute_not_toggled", build(with_acts(s14=[("MUTE", "ret=0,muted0=0," + state(48000, 0))])), M, 0, "OBSERVED_MISMATCH"),
    ("read_t5_text_wrong", build(with_acts(s11=[("READ", "phase=t5,sinceStepMs=5021,target=48200,"
                                                 + state(48200, 0).replace(text(48200), text(48000)))])),
     M, 0, "OBSERVED_MISMATCH"),
    ("read_t5_too_early", build(with_acts(s11=[("READ", "phase=t5,sinceStepMs=4000,target=48200," + state(48200, 0))])),
     M, 1, "LOG_INVALID"),
    ("step_since_missing", build(with_acts(s6=[("STEP", "phase=t0,target=48200," + state(48200, 0))])), M, 1, "LOG_INVALID"),
    ("sequence_missing_read", build(with_acts(s11=None)), M, 1, "LOG_INVALID"),
    ("plan_incomplete", build(with_acts(s20=None)), M, 1, "LOG_INVALID"),
    ("choose_target_wrong", build(with_acts(s5=[("CHOOSE", plan()[5][0][1].replace("target=48200", "target=48400"))])),
     M, 1, "LOG_INVALID"),
    ("not_restored_entries", build(entries=lambda n: [entry(0, 32000, 0, 1), entry(1, 48200 if 7 <= n <= 12 or n == 21
                                                                                else 48000, 1 if n == 15 else 0, 0)]),
     M, 0, "OBSERVED_MISMATCH"),
    ("act_not_after_sample", build(move=move_act), M, 1, "LOG_INVALID"),
    ("state_field_missing", build(with_acts(s7=[("READ", "phase=t1,sinceStepMs=1004,target=48200,bound=1,raw=48200,"
                                                 "muted=0,emuted=0,dtext=" + text(48200))])), M, 1, "LOG_INVALID"),
    ("bad_escape", build(with_acts(s7=[("READ", "phase=t1,sinceStepMs=1004,target=48200,"
                                        + state(48200, 0).replace("%20", "%2"))])), M, 1, "LOG_INVALID"),
    # codex #4414: bound to the plan and to the entry snapshots
    ("read_wrong_phase", build(with_acts(s7=[("READ", "phase=t0,sinceStepMs=0,target=48200," + state(48200, 0))],
                                         s11=[("READ", "phase=t0,sinceStepMs=0,target=48200," + state(48200, 0))])),
     M, 1, "LOG_INVALID"),
    ("different_target_idx", build(move=lambda rows: [(k, v, t.replace("op=STEP,idx=1", "op=STEP,idx=0")) for k, v, t in rows]),
     M, 1, "LOG_INVALID"),
    ("zero_step", build(with_acts(s5=[("CHOOSE", plan()[5][0][1].replace("res=200", "res=0").replace("target=48200", "target=48000"))])),
     M, 1, "LOG_INVALID"),
    ("choose_refers_active_entry", build(move=lambda rows: [(k, v, t.replace(",idx=1,", ",idx=0,") if k == "PLAYER_ACT" else t)
                                                            for k, v, t in rows]), M, 1, "LOG_INVALID"),
    ("entry_snapshot_disagrees", build(entries=lambda n: [entry(0, 32000, 0, 1), entry(1, 48200 if 7 <= n <= 12 and n != 9
                                                                                    else 48000, 1 if n == 15 else 0, 0)]),
     M, 0, "OBSERVED_MISMATCH"),
    ("other_entry_changes", build(entries=lambda n: [entry(0, 32200 if n == 13 else 32000, 0, 1),
                                                     entry(1, 48200 if 7 <= n <= 12 else 48000, 1 if n == 15 else 0, 0)]),
     M, 0, "OBSERVED_MISMATCH"),
    ("menu_log_with_entries_arg", build(), ["entries"], 1, "LOG_INVALID"),
    ("player_mode_with_menu_arg", build(start=START.replace("mode=playermenu", "mode=player")), M, 1, "LOG_INVALID"),
]
passed = 0
for name, text_, args, rc_expected, overall_expected in cases:
    path = f"{out}/{name}.log"
    open(path, "w", encoding="utf-8").write(text_)
    run = subprocess.run([sys.executable, "-X", "utf8", "-I", checker, path, *args], capture_output=True, text=True,
                         encoding="utf-8")
    report = json.loads(run.stdout[:run.stdout.rindex("}") + 1])
    ok = run.returncode == rc_expected and report["overall"] == overall_expected
    passed += ok
    print(f"{'PASS' if ok else 'FAIL'} {name}: rc={run.returncode} overall={report['overall']} errors={report['errors'][:2]}")
print(f"MENU_PARSER_CONTROLS {passed}/{len(cases)}")
sys.exit(0 if passed == len(cases) else 1)
