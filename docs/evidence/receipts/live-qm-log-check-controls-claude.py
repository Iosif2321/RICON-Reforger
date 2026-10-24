"""claude: positive and negative controls for live-qm-log-check-claude.py (A0-M, claude #4526, codex #4524/#4529).
Synthetic radiomute logs in the journal format: four own radios (two with two transceivers, one with one, one more with
two), the QM plan 0..9 for each, BENCH_DONE and the journal dump. A small radio model applies the stand's writes; the
behaviour switches stand for possible engine answers (mute reset by retune or by power, a delayed reset, a write that
does not apply). The values are test values for the parser.
Usage: python -I live-qm-log-check-controls-claude.py <repo> <outdir>"""
import json
import subprocess
import sys

repo, out = sys.argv[1], sys.argv[2]
checker = repo + "/docs/evidence/receipts/live-qm-log-check-claude.py"
START = ("level=NATIVE_ROUNDTRIP,prefabs=4,isServer=1,isClient=0,from=EOnInit,activeBefore=1,radioManager=0,"
         "expectedManager=-1,setter=component,setterGiven=0,playerController=1,factionManager=1,mode=radiomute,modeGiven=1")
DONE = "level=QM_LOCAL,not=AUDIBLE|RX_TX|REPLICATION|CLIENT|PER_PAIR|PLAYER"
RADIOS = [dict(trs=2, f0=32000, m0=0, f1=48000, m1=0), dict(trs=2, f0=30000, m0=0, f1=40000, m1=0),
          dict(trs=1, f0=36000, m0=0, f1=-1, m1=-1), dict(trs=2, f0=42000, m0=1, f1=46000, m1=0)]
PHASES = {3: "afterRetune", 5: "powerToggled", 7: "powerBack"}
OPS = ["BASE", "MUTE", "RETUNE", "READ", "POWER", "READ", "POWER_BACK", "READ", "RESTORE", "FINAL"]


def st(s, pre=False):
    p = "pre" if pre else ""
    return ",".join(f"{p}{k}={s[k]}" for k in ("radio", "trs", "powered", "f0", "m0", "f1", "m1"))


def set_frequency_effect(s, b):
    """Possible engine answers of SetFrequency on the mute flag of the same transceiver."""
    if b.get("retune_resets_mute"):
        s["m0"] = 0
    if b.get("set_frequency_toggles_mute"):
        s["m0"] = 1 - s["m0"]


def simulate(cfg, b):
    """QM records (step, text) of one radio; b: behaviour switches."""
    s = dict(radio=1, powered=1, **cfg)
    base = dict(s)
    target = s["f0"] + 200
    recs = [(0, f"op=BASE,{st(s)},min=30000,max=87975,res=200,target={target}")]
    if b.get("no_room"):
        return [(0, f"op=STOP,reason=NO_ROOM_FOR_STEP,{st(s)},min=30000,max=30000,res=200")]
    for n in range(1, 10):
        if b.get("lost_at") == n:
            recs.append((n, f"op=STOP,reason=OBJECT_LOST,{st(s)}"))
            return recs
        op = OPS[n]
        pre = dict(s)
        if op == "MUTE":
            if not b.get("mute_fails"):
                s["m0"] = 1 - base["m0"]
            if b.get("mute_hits_control") and s["trs"] == 2:
                s["m1"] = 1 - s["m1"]
            recs.append((n, f"op=MUTE,want={1 - base['m0']},ret={s['m0']},{st(pre, True)},{st(s)}"))
        elif op == "RETUNE":
            if not b.get("retune_fails"):
                s["f0"] = target
            set_frequency_effect(s, b)
            recs.append((n, f"op=RETUNE,want={target},{st(pre, True)},{st(s)}"))
        elif op == "POWER":
            if not b.get("power_fails"):
                s["powered"] = 0
            if b.get("power_resets_mute"):
                s["m0"] = 0
            if b.get("power_hits_control") and s["trs"] == 2:
                s["m1"] = 1
            recs.append((n, f"op=POWER,want=0,{st(pre, True)},{st(s)}"))
        elif op == "POWER_BACK":
            if not b.get("power_back_fails"):
                s["powered"] = 1
            # the engine may give the control its state back with the power; the stand never writes transceiver 1
            if b.get("power_hits_control") and not b.get("control_stays_changed"):
                s["m1"] = base["m1"]
            recs.append((n, f"op=POWER_BACK,want=1,{st(pre, True)},{st(s)}"))
        elif op == "RESTORE":
            # the same order as the stand: power, then frequency by the fresh getter, then mute by the fresh getter
            flags, ret = {}, -1
            flags["setPower"] = int(s["powered"] != base["powered"])
            if flags["setPower"]:
                s["powered"] = base["powered"]
                if b.get("power_on_resets_mute"):
                    s["m0"] = 0
            flags["restoreFreqBefore"] = s["f0"]
            flags["setFreq"] = int(s["f0"] != base["f0"])
            if flags["setFreq"]:
                s["f0"] = base["f0"]
                set_frequency_effect(s, b)
            flags["restoreMuteBefore"] = s["m0"]
            flags["setMute"] = int(s["m0"] != base["m0"])
            if flags["setMute"]:
                if not b.get("restore_mute_fails"):
                    s["m0"] = base["m0"]
                ret = s["m0"]
            if b.get("restore_flag_lie"):
                flags["setMute"] = 1 - flags["setMute"]
            text = ",".join(f"{k}={v}" for k, v in flags.items())
            recs.append((n, f"op=RESTORE,{text},ret={ret},{st(pre, True)},{st(s)}"))
        elif op == "READ":
            if b.get("delayed_reset") and n == 5:
                s["m0"] = 0
            recs.append((n, f"op=READ,phase={PHASES[n]},{st(s)}"))
        else:
            recs.append((n, f"op=FINAL,{st(s)}"))
    return recs


def build(b=None, per=None, start=START, end=("BENCH_DONE", 9, DONE), step_ms=1003, extra=(), dump_drop=0,
          dump_skip=False, mutate=None, error_line=False, order_swap=False):
    b = b or {}
    per = per or {}
    rows = [("BENCH_START", -1, -1, start), ("BENCH_MANAGER", 0, -1, "where=preSpawn,radioManager=0,expected=-1")]
    for i in range(len(RADIOS)):
        rows.append(("BENCH_SPAWN", 1, -1, f"bench={i},prefab=P{i},actualPrefab=P{i}"))
    sims = [simulate(RADIOS[i], {**b, **per.get(i, {})}) for i in range(len(RADIOS))]
    qm = []
    for n in range(10):
        for i, recs in enumerate(sims):
            for step, text in recs:
                if step == n:
                    ms = 2000 + step_ms * n
                    qm.append(("QM", n, -1, f"bench={i},{text},elapsedMs={ms}"))
    if order_swap:
        qm[4], qm[3] = qm[3], qm[4]
        a = next(i for i, r in enumerate(qm) if r[1] == 1)
        b2 = next(i for i, r in enumerate(qm) if r[1] == 2)
        qm[a], qm[b2] = qm[b2], qm[a]
    if mutate:
        qm = mutate(qm)
    rows += qm + list(extra) + [(end[0], end[1], -1, end[2])]
    lines = [f"12:00:00.000 SCRIPT : [RICON_LIVE] seq={i} t={i} side=probe kind={k} player=-1 device=-1 tr={tr} "
             f"value={v} text={t}" for i, (k, v, tr, t) in enumerate(rows)]
    dump = [l[l.index("[RICON_LIVE]"):] for l in lines]
    if dump_skip:
        dump = dump[:-1]
    text = "\n".join(lines) + f"\n12:00:01.000 SCRIPT : [RICON_LIVE] dump=BEGIN count={len(rows)} dropped={dump_drop} " \
        f"nextSeq={len(rows)}\n" + "\n".join("12:00:01.000 SCRIPT : " + l for l in dump) + \
        "\n12:00:01.000 SCRIPT : [RICON_LIVE] dump=END\n"
    if error_line:
        text += "12:00:02.000 SCRIPT    (E): something\n"
    return text


def drop_step(n, bench):
    return lambda qm: [r for r in qm if not (r[1] == n and r[3].startswith(f"bench={bench},"))]


def edit(n, bench, old, new):
    return lambda qm: [(k, v, tr, t.replace(old, new) if v == n and t.startswith(f"bench={bench},") else t)
                       for k, v, tr, t in qm]


cases = [
    ("qm_ok_persists", build(), 0, "QM_LOCAL_OBSERVED"),
    ("qm_retune_resets_mute", build({"retune_resets_mute": True}), 0, "QM_LOCAL_OBSERVED"),
    ("qm_power_resets_mute", build({"power_resets_mute": True}), 0, "QM_LOCAL_OBSERVED"),
    ("qm_delayed_reset", build({"delayed_reset": True}), 0, "QM_LOCAL_OBSERVED"),
    ("qm_power_hits_control", build({"power_hits_control": True}), 0, "QM_LOCAL_OBSERVED"),
    # codex #4541: a frequency call that flips the mute makes the stand's mute call necessary although pre.m0 = BASE
    ("restore_cascade_frequency", build({"set_frequency_toggles_mute": True}), 0, "QM_LOCAL_OBSERVED"),
    # codex #4541: a failed POWER_BACK, then the restore's own SetPower brings the mute back - no mute call follows
    ("restore_cascade_power", build(per={0: {"power_back_fails": True, "power_on_resets_mute": True}}), 0,
     "OBSERVED_MISMATCH"),
    ("restore_freq_moved_alone", build(mutate=lambda qm: [(k, v, tr, t.replace("restoreFreqBefore=32200", "restoreFreqBefore=32400"))
                                                          for k, v, tr, t in qm]), 1, "LOG_INVALID"),
    ("mute_not_applied", build(per={1: {"mute_fails": True}}), 0, "OBSERVED_MISMATCH"),
    ("retune_not_applied", build(per={0: {"retune_fails": True}}), 0, "OBSERVED_MISMATCH"),
    ("power_not_applied", build(per={3: {"power_fails": True}}), 0, "OBSERVED_MISMATCH"),
    ("mute_hits_control", build(per={0: {"mute_hits_control": True}}), 0, "OBSERVED_MISMATCH"),
    ("restore_mute_fails", build(per={2: {"restore_mute_fails": True, "retune_resets_mute": False}}), 0, "OBSERVED_MISMATCH"),
    ("control_not_restored", build(per={0: {"power_hits_control": True, "control_stays_changed": True}}), 0,
     "OBSERVED_MISMATCH"),
    ("stop_object_lost", build(per={1: {"lost_at": 4}}), 0, "STOPPED"),
    ("stop_no_room", build(per={2: {"no_room": True}}), 0, "STOPPED"),
    ("restore_flag_lie", build(per={0: {"restore_flag_lie": True}}), 1, "LOG_INVALID"),
    ("step_too_fast", build(step_ms=500), 1, "LOG_INVALID"),
    ("step_order", build(order_swap=True), 1, "LOG_INVALID"),
    ("missing_step", build(mutate=drop_step(5, 1)), 1, "LOG_INVALID"),
    ("read_phase_wrong", build(mutate=edit(3, 0, "phase=afterRetune", "phase=powerBack")), 1, "LOG_INVALID"),
    ("pre_missing", build(mutate=edit(1, 2, "pretrs=1,", "")), 1, "LOG_INVALID"),
    ("key_logged", build(mutate=edit(0, 0, "op=BASE,", "op=BASE,key=abc,")), 1, "LOG_INVALID"),
    ("base_target_far", build(mutate=edit(0, 3, "target=42200", "target=42400")), 1, "LOG_INVALID"),
    ("old_phase_record", build(extra=[("BENCH_READ", 32000, 0, "bench=0,phase=t0")]), 1, "LOG_INVALID"),
    ("dump_dropped", build(dump_drop=1), 1, "LOG_INVALID"),
    ("dump_short", build(dump_skip=True), 1, "LOG_INVALID"),
    ("mode_radio", build(start=START.replace("mode=radiomute", "mode=radio")), 1, "LOG_INVALID"),
    ("aborted", build(end=("BENCH_ABORTED", 4, "level=QM_LOCAL,step=4")), 1, "LOG_INVALID"),
    ("script_error", build(error_line=True), 1, "LOG_INVALID"),
]
passed = 0
for name, text_, rc_expected, overall_expected in cases:
    path = f"{out}/{name}.log"
    open(path, "w", encoding="utf-8").write(text_)
    run = subprocess.run([sys.executable, "-X", "utf8", "-I", checker, path], capture_output=True, text=True,
                         encoding="utf-8")
    try:
        report = json.loads(run.stdout[:run.stdout.rindex("}") + 1])
    except ValueError:
        report = {"overall": "NO_REPORT", "errors": [run.stderr[-200:]]}
    ok = run.returncode == rc_expected and report["overall"] == overall_expected
    passed += ok
    print(f"{'PASS' if ok else 'FAIL'} {name}: rc={run.returncode} overall={report['overall']} "
          f"errors={report['errors'][:2]} mism={report.get('mismatches', [])[:1]}")
print(f"QM_PARSER_CONTROLS {passed}/{len(cases)}")
sys.exit(0 if passed == len(cases) else 1)
