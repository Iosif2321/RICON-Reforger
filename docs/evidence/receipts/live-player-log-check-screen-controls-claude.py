"""claude: positive and negative controls for the A1-S1 'screen' checks of live-player-log-check-claude.py (claude #4431,
codex #4425/#4427/#4430/#4432). Synthetic playerscreen logs in the journal format, modelled on the real A1-P1 entries
(the player's AN/PRC-68, transceivers at 32000 and 48000 raw): 30 samples with 2 entries each, SCREEN BEFORE/OPEN/MENU
records and one WIDGET record per entry after each MENU of samples 7..12. The values are test values for the parser.
Usage: python -I live-player-log-check-screen-controls-claude.py <repo> <outdir>"""
import json
import subprocess
import sys

repo, out = sys.argv[1], sys.argv[2]
checker = repo + "/docs/evidence/receipts/live-player-log-check-claude.py"
START = ("level=NATIVE_ROUNDTRIP,prefabs=4,isServer=1,isClient=0,from=EOnInit,activeBefore=1,radioManager=0,"
         "expectedManager=-1,setter=component,setterGiven=0,playerController=0,factionManager=1,mode=playerscreen,modeGiven=1")
STATE = ("isServer=1,isClient=0,players=1,pc=1,playerId=1,vonController=1,vonEntries=2,controlled=1,controlledPrefab="
         "{26A9756790131354}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Rifleman.et,entityVoN=1,gadgetManager=1")
RADIO = "{73950FBA2D7DB5C5}Prefabs/Items/Equipment/Radios/Radio_ANPRC68.et"
DONE = "level=PLAYER_SCREEN_WIDGET,not=PIXELS|LOCALIZATION|INPUT|AUDIO|REPLICATION"
RAWS = (32000, 48000)


def text(raw):
    return f"{raw / 1000:.1f}%20#AR-VON_FrequencyUnits_MHz"


def entry(idx, raw):
    return (f"idx={idx},entry=1,cls=SCR_VONEntryRadio,method=1,active={1 - idx},transmit={1 - idx},usable=1,dtext={text(raw)},"
            f"radioEntry=1,trNumber={idx + 1},entryFreq={raw},emuted=0,gadget=1,longRange=0,transceiver=1,raw={raw},"
            f"min=32000,max=68000,res=200,muted=0,radioPrefab={RADIO},editorRadio=0,trIndex={idx},inventorySlot=0,"
            f"gadgetOwnerMatch=1")


def widget(idx, ftext=None, fvis=1, fw=86, fh=24):
    ftext = text(RAWS[idx]) if ftext is None else ftext
    return (f"op=WIDGET,idx={idx},component=1,root=1,rootVis=1,f=1,fText={ftext},fVis={fvis},fX={600 + 200 * idx},fY=400,"
            f"fW={fw},fH={fh},c=1,cText=,cVis=1,cX={600 + 200 * idx},cY=430,cW=0,cH=24")


def menu(op, opened=1, active=0, selected=0):
    return f"op={op},opened={opened},hasDisplay=1,selectedId={selected},activeIdx={active}"


def screens(before_open=0):
    s = {5: [menu("BEFORE", opened=before_open, selected=-1)],
         6: [menu("ALREADY_OPEN" if before_open else "OPEN")]}
    for n in range(7, 13):
        s[n] = [menu("MENU"), widget(0), widget(1)]
    s[20] = [menu("MENU")]
    s[30] = [menu("MENU")]
    return s


def build(scr=None, entries=lambda n: [entry(0, RAWS[0]), entry(1, RAWS[1])], start=START, last=30,
          end=("BENCH_DONE", 30, DONE)):
    scr = screens() if scr is None else scr
    rows = [("BENCH_START", -1, start)]
    for n in range(1, last + 1):
        rows.append(("PLAYER_SAMPLE", n, f"elapsedMs={1000 * n + 7}," + STATE))
        rows += [("PLAYER_ENTRY", n, e) for e in entries(n)]
        rows += [("SCREEN", n, x) for x in scr.get(n, [])]
    rows.append(end)
    return "\n".join(f"12:00:00.000 SCRIPT : [RICON_LIVE] seq={i} t={i} side=probe kind={k} player=-1 device=-1 tr=-1 "
                     f"value={v} text={t}" for i, (k, v, t) in enumerate(rows))


def change(n, records):
    s = screens()
    s[n] = records
    return s


S = ["screen"]
cases = [
    ("screen_ok", build(), S, 0, "PLAYER_SCREEN_WIDGET_PASS"),
    ("already_open", build(screens(before_open=1)), S, 0, "PLAYER_SCREEN_WIDGET_PASS"),
    ("widget_localized", build(change(9, [menu("MENU"), widget(0, ftext="32.0%20MHz"), widget(1, ftext="48.0%20MHz")])),
     S, 0, "PLAYER_SCREEN_WIDGET_LABEL_UNVERIFIED"),
    ("menu_no_display", build(change(9, [menu("MENU").replace("hasDisplay=1", "hasDisplay=0"), widget(0), widget(1)])),
     S, 0, "OBSERVED_MISMATCH"),
    ("root_without_component", build(change(9, [menu("MENU"), widget(0).replace("component=1", "component=0"), widget(1)])),
     S, 1, "LOG_INVALID"),
    ("widget_invisible", build(change(9, [menu("MENU"), widget(0, fvis=0), widget(1)])), S, 0, "OBSERVED_MISMATCH"),
    ("widget_zero_size", build(change(10, [menu("MENU"), widget(0), widget(1, fw=0)])), S, 0, "OBSERVED_MISMATCH"),
    ("widget_other_number", build(change(8, [menu("MENU"), widget(0, ftext=text(32200)), widget(1)])), S, 0, "OBSERVED_MISMATCH"),
    ("entries_change", build(entries=lambda n: [entry(0, 32200 if n == 15 else 32000), entry(1, 48000)]),
     S, 0, "OBSERVED_MISMATCH"),
    ("active_changed", build(change(8, [menu("MENU", active=1), widget(0), widget(1)])), S, 0, "OBSERVED_MISMATCH"),
    ("menu_closed", build(change(10, [menu("MENU", opened=0), widget(0), widget(1)])), S, 0, "OBSERVED_MISMATCH"),
    ("sequence_missing_menu", build(change(11, [widget(0), widget(1)])), S, 1, "LOG_INVALID"),
    ("widget_field_missing", build(change(7, [menu("MENU"), widget(0).replace(",fH=24", ""), widget(1)])), S, 1, "LOG_INVALID"),
    ("widget_idx_missing", build(change(8, [menu("MENU"), widget(0)])), S, 1, "LOG_INVALID"),
    ("open_when_open", build({**screens(), 5: [menu("BEFORE", opened=1, selected=-1)]}), S, 1, "LOG_INVALID"),
    ("bad_escape", build(change(7, [menu("MENU"), widget(0, ftext="32.0%2"), widget(1)])), S, 1, "LOG_INVALID"),
    ("not_ready", build({5: ["op=NOT_READY,von=1,menu=0"]}, last=5,
                        end=("BENCH_ABORTED", 5, "level=PLAYER_SCREEN_WIDGET,notReady=1,samples=5")), S, 0, "NOT_READY"),
    ("screen_with_entries_arg", build(), ["entries"], 1, "LOG_INVALID"),
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
print(f"SCREEN_PARSER_CONTROLS {passed}/{len(cases)}")
sys.exit(0 if passed == len(cases) else 1)
