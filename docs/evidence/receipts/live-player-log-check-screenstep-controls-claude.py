"""claude: positive and negative controls for the A1-S2 'screenstep' checks of live-player-log-check-claude.py (claude
#4488, codex #4489/#4491/#4492). Synthetic playerscreenstep logs in the journal format, modelled on the real A1-S1c run
(the player's AN/PRC-68, transceivers at 32000 and 48000 raw, entry 1 not active): 30 samples with 2 entries each, the
S1 screen records on 5, 6, 7, 8, 10, 14, 17, 20, 30 with WIDGET records on 7, 8, 10, 14, 17, the step plan CHOOSE(5)
STEP(9) READ(10, 14) RESTORE(15) FINAL(20) on entry 1 (48000 -> 48200 -> 48000) and PLAYER_READER PRE(7) POST(10)
BACK(17). The values are test values for the parser.
Usage: python -I live-player-log-check-screenstep-controls-claude.py <repo> <outdir>"""
import json
import subprocess
import sys

repo, out = sys.argv[1], sys.argv[2]
checker = repo + "/docs/evidence/receipts/live-player-log-check-claude.py"
START = ("level=NATIVE_ROUNDTRIP,prefabs=4,isServer=1,isClient=0,from=EOnInit,activeBefore=1,radioManager=0,"
         "expectedManager=-1,setter=component,setterGiven=0,playerController=0,factionManager=1,mode=playerscreenstep,"
         "modeGiven=1")
STATE = ("isServer=1,isClient=0,players=1,pc=1,playerId=1,vonController=1,vonEntries=2,controlled=1,controlledPrefab="
         "{26A9756790131354}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Rifleman.et,entityVoN=1,gadgetManager=1")
RADIO = "{73950FBA2D7DB5C5}Prefabs/Items/Equipment/Radios/Radio_ANPRC68.et"
DONE = "level=PLAYER_SCREEN_STEP,not=PIXELS|LOCALIZATION|INPUT|AUDIO|REPLICATION"
OPEN_VIA = ",hadControl=0,controls=1,openAction=VONMenu,closeOnRelease=0,performOnClose=0,closeOnPerform=1"
RAW0, TARGET = 48000, 48200
STEPPED = range(10, 16)  # entry snapshots taken after STEP (9) and before RESTORE (15)


def text(raw):
    return f"{raw / 1000:.1f}%20#AR-VON_FrequencyUnits_MHz"


def label(raw):
    return f"{raw / 1000:.1f}%20MHz"


def raw1(n):
    return TARGET if n in STEPPED else RAW0


def entry(idx, raw, active=None, dtext=None):
    active = 1 - idx if active is None else active
    return (f"idx={idx},entry=1,cls=SCR_VONEntryRadio,method=1,active={active},transmit={active},usable=1,"
            f"dtext={text(raw) if dtext is None else dtext},radioEntry=1,trNumber={idx + 1},entryFreq={raw},emuted=0,"
            f"gadget=1,longRange=0,transceiver=1,raw={raw},min=32000,max=68000,res=200,muted=0,radioPrefab={RADIO},"
            f"editorRadio=0,trIndex={idx},inventorySlot=0,gadgetOwnerMatch=1")


def entries_ok(n):
    return [entry(0, 32000), entry(1, raw1(n))]


def widget(idx, ftext):
    return (f"op=WIDGET,idx={idx},component=1,root=1,rootVis=1,f=1,fText={ftext},fVis=1,fX=713,fY={269 + 356 * idx},"
            f"fW=71,fH=23,c=1,cText=Error,cVis=1,cX=700,cY={240 + 356 * idx},cW=40,cH=20")


def menu(op, opened=1, active=0):
    return f"op={op},opened={opened},hasDisplay=1,inputs=1,selectedId=0,activeIdx={active}"


def screens(fmt=text, shown=raw1):
    s = {5: [menu("BEFORE", opened=0).replace("hasDisplay=1,inputs=1", "hasDisplay=0,inputs=0")],
         6: [menu("OPEN") + OPEN_VIA]}
    for n in (7, 8, 10, 14, 17):
        s[n] = [menu("MENU"), widget(0, fmt(32000)), widget(1, fmt(shown(n)))]
    s[20] = [menu("MENU")]
    s[30] = [menu("MENU")]
    return s


def state(raw):
    return f"bound=1,raw={raw},entryFreq={raw},muted=0,emuted=0,dtext={text(raw)}"


def acts():
    return {5: [f"op=CHOOSE,idx=1,trNumber=2,min=32000,max=68000,res=200,raw0={RAW0},muted0=0,target={TARGET},"
                f"modifier=1,slot=0,{state(RAW0)}"],
            9: [f"op=STEP,idx=1,phase=t0,sinceStepMs=0,target={TARGET},{state(TARGET)}"],
            10: [f"op=READ,idx=1,phase=t1,sinceStepMs=1000,target={TARGET},{state(TARGET)}"],
            14: [f"op=READ,idx=1,phase=t5,sinceStepMs=5000,target={TARGET},{state(TARGET)}"],
            15: [f"op=RESTORE,idx=1,raw0={RAW0},{state(RAW0)}"],
            20: [f"op=FINAL,idx=1,raw0={RAW0},muted0=0,{state(RAW0)}"]}


def reader(phase, raw, **kw):
    """One PLAYER_READER record; keyword values replace fields, None removes a field."""
    f = {"phase": phase, "idx": "1", "bound": "1", "authority": "1", "deviceId": "1", "trIndex": "1", "ok": "1",
         "obsDevice": "1", "obsTr": "1", "tuning": str(raw), "powered": "1", "rawBefore": str(raw), "rawAfter": str(raw)}
    f.update(kw)
    return ",".join(f"{k}={v}" for k, v in f.items() if v is not None)


def readers():
    return {7: [reader("PRE", RAW0)], 10: [reader("POST", TARGET)], 17: [reader("BACK", RAW0)]}


def build(scr=None, act=None, rdr=None, entries=entries_ok, start=START, last=30, end=("BENCH_DONE", 30, DONE)):
    scr = screens() if scr is None else scr
    act = acts() if act is None else act
    rdr = readers() if rdr is None else rdr
    rows = [("BENCH_START", -1, start)]
    for n in range(1, last + 1):
        rows.append(("PLAYER_SAMPLE", n, f"elapsedMs={1000 * n + 7}," + STATE))
        rows += [("PLAYER_ENTRY", n, e) for e in entries(n)]
        rows += [("SCREEN", n, x) for x in scr.get(n, [])]
        rows += [("PLAYER_ACT", n, x) for x in act.get(n, [])]
        rows += [("PLAYER_READER", n, x) for x in rdr.get(n, [])]
    rows.append(end)
    return "\n".join(f"12:00:00.000 SCRIPT : [RICON_LIVE] seq={i} t={i} side=probe kind={k} player=-1 device=-1 tr=-1 "
                     f"value={v} text={t}" for i, (k, v, t) in enumerate(rows))


def upto(d, n, extra=None):
    """Records of samples before n (a STOP or NOT_READY on n), plus the records of n itself if given."""
    r = {k: v for k, v in d.items() if k < n}
    if extra is not None:
        r[n] = extra
    return r


def with_(d, n, records):
    return {**d, n: records}


def aborted(n, not_ready, stopped):
    return ("BENCH_ABORTED", n, f"level=PLAYER_SCREEN_STEP,notReady={not_ready},stopped={stopped},samples={n}")


T = ["screenstep"]
stale_text = text(RAW0)
cases = [
    ("step_ok", build(), T, 0, "PLAYER_SCREEN_STEP_PASS"),
    # the expected real outcome: the widget shows the localized label, a key-free text without a verified translation
    ("step_localized", build(screens(fmt=label)), T, 0, "PLAYER_SCREEN_STEP_LABEL_UNVERIFIED"),
    # the widget equals the entry's own (stale) text, but the number on the screen is not the stepped raw value
    ("widget_number_stale", build(with_(screens(), 10, [menu("MENU"), widget(0, text(32000)), widget(1, stale_text)]),
                                  entries=lambda n: [entry(0, 32000), entry(1, raw1(n), dtext=stale_text if n == 10 else None)]),
     T, 0, "OBSERVED_MISMATCH"),
    ("widget_not_stepped", build(screens(shown=lambda n: RAW0)), T, 0, "OBSERVED_MISMATCH"),
    # codex #4505: a non-finite number on the screen gives a report, not an exception
    ("widget_infinity", build(with_(screens(fmt=label), 10, [menu("MENU"), widget(0, label(32000)), widget(1, "1e309%20MHz")])),
     T, 0, "OBSERVED_MISMATCH"),
    ("widget_nan", build(with_(screens(fmt=label), 14, [menu("MENU"), widget(0, label(32000)), widget(1, "nan%20MHz")])),
     T, 0, "OBSERVED_MISMATCH"),
    ("entry_text_overflow", build(with_(screens(), 17, [menu("MENU"), widget(0, text(32000)), widget(1, "9" * 400 + "%20#AR-VON_FrequencyUnits_MHz")]),
                                  entries=lambda n: [entry(0, 32000), entry(1, raw1(n), dtext="9" * 400 + "%20#AR-VON_FrequencyUnits_MHz" if n == 17 else None)]),
     T, 0, "OBSERVED_MISMATCH"),
    ("entries_not_stepped", build(screens(shown=lambda n: RAW0), entries=lambda n: [entry(0, 32000), entry(1, RAW0)]),
     T, 0, "OBSERVED_MISMATCH"),
    ("not_restored", build(screens(shown=lambda n: TARGET if n >= 10 else RAW0),
                           entries=lambda n: [entry(0, 32000), entry(1, TARGET if n >= 10 else RAW0)]), T, 0, "OBSERVED_MISMATCH"),
    ("other_entry_changes", build(entries=lambda n: [entry(0, 32200 if n == 16 else 32000), entry(1, raw1(n))]),
     T, 0, "OBSERVED_MISMATCH"),
    ("target_active_changes", build(entries=lambda n: [entry(0, 32000), entry(1, raw1(n), active=1 if n == 12 else None)]),
     T, 0, "OBSERVED_MISMATCH"),
    ("menu_closed_after_step", build(with_(screens(), 10, [menu("MENU", opened=0), widget(0, text(32000)),
                                                           widget(1, text(TARGET))])), T, 0, "OBSERVED_MISMATCH"),
    ("reader_tuning_differs", build(rdr=with_(readers(), 10, [reader("POST", TARGET, tuning=str(RAW0))])), T, 0, "OBSERVED_MISMATCH"),
    ("reader_raw_moved", build(rdr=with_(readers(), 7, [reader("PRE", RAW0, rawAfter="48200")])), T, 0, "OBSERVED_MISMATCH"),
    ("reader_not_ok", build(rdr=with_(readers(), 17, [reader("BACK", RAW0, ok="0", obsDevice=None, obsTr=None, tuning=None,
                                                             powered=None)])), T, 0, "OBSERVED_MISMATCH"),
    ("reader_no_authority", build(rdr=with_(readers(), 7, [reader("PRE", RAW0, authority="0")])), T, 0, "OBSERVED_MISMATCH"),
    ("reader_wrong_transceiver", build(rdr=with_(readers(), 10, [reader("POST", TARGET, trIndex="0", obsTr="0")])),
     T, 0, "OBSERVED_MISMATCH"),
    ("reader_unbound", build(rdr=with_(readers(), 17, ["phase=BACK,idx=1,bound=0"])), T, 0, "OBSERVED_MISMATCH"),
    ("reader_missing", build(rdr=upto(readers(), 17)), T, 1, "LOG_INVALID"),
    # the network key and the position never appear in the log
    ("reader_key_logged", build(rdr=with_(readers(), 7, [reader("PRE", RAW0) + ",key=abc"])), T, 1, "LOG_INVALID"),
    ("reader_pos_logged", build(rdr=with_(readers(), 10, [reader("POST", TARGET) + ",pos=1|2|3"])), T, 1, "LOG_INVALID"),
    ("reader_unbound_extra", build(rdr=with_(readers(), 17, ["phase=BACK,idx=1,bound=0,ok=1"])), T, 1, "LOG_INVALID"),
    ("stop_step_mismatch", build(upto(screens(), 9), upto(acts(), 9, [f"op=STOP,idx=1,reason=STEP_MISMATCH,{state(48400)}"]),
                                 upto(readers(), 9), last=9, end=aborted(9, 0, 1)), T, 0, "STOPPED"),
    ("stop_screen_lost", build(upto(screens(), 15), upto(acts(), 15, [f"op=STOP,idx=1,reason=SCREEN_LOST,{state(TARGET)}"]),
                               upto(readers(), 15), last=15, end=aborted(15, 0, 1)), T, 0, "STOPPED"),
    ("not_ready_at_12", build(upto(screens(), 12, ["op=NOT_READY,von=1,vonMenu=1,menu=1,same=0"]), upto(acts(), 12),
                              upto(readers(), 12), last=12, end=aborted(12, 1, 0)), T, 0, "NOT_READY"),
    ("abort_flags_wrong", build(upto(screens(), 9), upto(acts(), 9, [f"op=STOP,idx=1,reason=STEP_MISMATCH,{state(48400)}"]),
                                upto(readers(), 9), last=9, end=aborted(9, 1, 0)), T, 1, "LOG_INVALID"),
    ("stop_without_abort", build(upto(screens(), 9), upto(acts(), 9, [f"op=STOP,idx=1,reason=STEP_MISMATCH,{state(48400)}"]),
                                 upto(readers(), 9), last=9, end=("BENCH_DONE", 9, DONE)), T, 1, "LOG_INVALID"),
    ("read_too_early", build(act=with_(acts(), 10, [f"op=READ,idx=1,phase=t1,sinceStepMs=900,target={TARGET},{state(TARGET)}"])),
     T, 1, "LOG_INVALID"),
    ("mute_in_step", build(act=with_(acts(), 16, [f"op=MUTE,idx=1,ret=1,muted0=0,{state(RAW0)}"])), T, 1, "LOG_INVALID"),
    ("extra_menu_at_9", build(with_(screens(), 9, [menu("MENU"), widget(0, text(32000)), widget(1, text(RAW0))])),
     T, 1, "LOG_INVALID"),
    ("step_log_as_screen", build(), ["screen"], 1, "LOG_INVALID"),
    ("screen_log_as_step", build(act={}, rdr={}, start=START.replace("mode=playerscreenstep", "mode=playerscreen"),
                                 entries=lambda n: [entry(0, 32000), entry(1, RAW0)], scr=screens(shown=lambda n: RAW0),
                                 end=("BENCH_DONE", 30, DONE.replace("PLAYER_SCREEN_STEP", "PLAYER_SCREEN_WIDGET"))),
     T, 1, "LOG_INVALID"),
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
print(f"SCREENSTEP_PARSER_CONTROLS {passed}/{len(cases)}")
sys.exit(0 if passed == len(cases) else 1)
