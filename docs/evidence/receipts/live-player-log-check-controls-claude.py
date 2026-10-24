"""claude: positive and negative controls for live-player-log-check-claude.py (A1-P0, codex #4365). Synthetic logs in
the journal line format (no real A1-P0 log exists yet): a player-mode BENCH_START, 30 PLAYER_SAMPLE, BENCH_DONE.
The sample contents here are test values for the parser, not claims about Workbench Play.
Usage: python -I live-player-log-check-controls-claude.py <repo> <outdir>"""
import json
import subprocess
import sys

repo, out = sys.argv[1], sys.argv[2]
checker = repo + "/docs/evidence/receipts/live-player-log-check-claude.py"
START = ("level=NATIVE_ROUNDTRIP,prefabs=4,isServer=1,isClient=0,from=EOnInit,activeBefore=1,radioManager=0,"
         "expectedManager=-1,setter=component,setterGiven=0,playerController=0,factionManager=1,mode=player,modeGiven=1")
ABSENT = "isServer=1,isClient=0,players=0,pc=0"
PRESENT = ("isServer=1,isClient=0,players=1,pc=1,playerId=1,vonController=1,vonEntries=2,controlled=1,"
           "controlledPrefab={ABCDEF0123456789}Prefabs/Characters/Test%20Unit.et,entityVoN=1,gadgetManager=1")


def line(seq, kind, value, text):
    return (f"12:00:00.000 SCRIPT : [RICON_LIVE] seq={seq} t={seq * 1000} side=probe kind={kind} player=-1 device=-1 "
            f"tr=-1 value={value} text={text}")


def build(sample=lambda n: ABSENT, start=START, done="level=PLAYER_OBSERVATION,not=SPAWN|SET|CONTROL|RADIO|AUDIO",
          values=None, extra=None, ms=lambda n: 1000 * n + 5, done_value=30):
    rows = [("BENCH_START", -1, start)]
    for n in (values or range(1, 31)):
        rows.append(("PLAYER_SAMPLE", n, f"elapsedMs={ms(n)}," + sample(n)))
    if extra:
        rows.insert(extra[0], extra[1])
    rows.append(("BENCH_DONE", done_value, done))
    body = [line(i, k, v, t) for i, (k, v, t) in enumerate(rows)]
    body += ["12:00:01.000 SCRIPT : [RICON_LIVE] dump=BEGIN count=1 dropped=0 nextSeq=1", body[0][body[0].index("12"):],
             "12:00:01.000 SCRIPT : [RICON_LIVE] dump=END"]
    return "\n".join(body)


cases = [
    ("absent_ok", build(), 0, "PLAYER_OBSERVATION_COMPLETE", "LOCAL_PLAYER_ABSENT"),
    ("present_ok", build(lambda n: PRESENT), 0, "PLAYER_OBSERVATION_COMPLETE", "LOCAL_PLAYER_PRESENT"),
    ("late_player", build(lambda n: ABSENT if n < 11 else PRESENT), 0, "PLAYER_OBSERVATION_COMPLETE", "LOCAL_PLAYER_CHANGED"),
    ("spawn_present", build(extra=(3, ("BENCH_SPAWN", 1, "bench=0,prefab=x,actualPrefab=x"))), 1, "LOG_INVALID", "LOCAL_PLAYER_ABSENT"),
    ("mute_present", build(extra=(5, ("BENCH_MUTE", -1, "bench=0,before=0"))), 1, "LOG_INVALID", "LOCAL_PLAYER_ABSENT"),
    ("samples_29", build(values=range(1, 30), done_value=29), 1, "LOG_INVALID", "LOCAL_PLAYER_ABSENT"),
    ("samples_out_of_order", build(values=[1, 3, 2] + list(range(4, 31))), 1, "LOG_INVALID", "LOCAL_PLAYER_ABSENT"),
    ("done_wrong_level", build(done="level=NATIVE_ROUNDTRIP"), 1, "LOG_INVALID", "LOCAL_PLAYER_ABSENT"),
    ("mode_missing", build(start=START.replace(",mode=player,modeGiven=1", "")), 1, "LOG_INVALID", "LOCAL_PLAYER_ABSENT"),
    ("pc0_with_player_fields", build(lambda n: ABSENT + ",playerId=1"), 1, "LOG_INVALID", "LOCAL_PLAYER_ABSENT"),
    ("pc1_missing_controlled", build(lambda n: PRESENT.split(",controlled=")[0]), 1, "LOG_INVALID", "LOCAL_PLAYER_PRESENT"),
    ("prefab_bad_escape", build(lambda n: PRESENT.replace("Test%20Unit", "Test%2Unit")), 1, "LOG_INVALID", "LOCAL_PLAYER_PRESENT"),
    ("elapsed_too_small", build(ms=lambda n: 1000 * n - 50), 1, "LOG_INVALID", "LOCAL_PLAYER_ABSENT"),
    ("burst_after_stall", build(ms=lambda n: 1005 if n == 1 else (10005 + n if n <= 10 else 1000 * n + 5)),
     1, "LOG_INVALID", "LOCAL_PLAYER_ABSENT"),
    ("player_id_change", build(lambda n: PRESENT if n < 16 else PRESENT.replace("playerId=1", "playerId=2")),
     0, "PLAYER_OBSERVATION_COMPLETE", "LOCAL_PLAYER_CHANGED"),
    ("player_manager_absent", build(lambda n: ABSENT.replace("players=0", "players=-1")),
     0, "PLAYER_OBSERVATION_COMPLETE", "LOCAL_PLAYER_ABSENT"),
    ("not_ready", "\n".join([line(0, "BENCH_START", -1, START.replace("mode=player", "mode=INVALID")),
                             line(1, "BENCH_NOT_READY", -1, "reason=MODE_PARAM,mode=INVALID,modeGiven=1"),
                             line(2, "BENCH_ABORTED", 0, "level=NATIVE_ROUNDTRIP,phase=0")]), 1, "NOT_READY", None),
    ("script_error", build() + "\n12:00:02.000 SCRIPT (E): NULL pointer to instance", 1, "LOG_INVALID", "LOCAL_PLAYER_ABSENT"),
]
passed = 0
for name, text, rc_expected, overall_expected, finding_expected in cases:
    path = f"{out}/{name}.log"
    open(path, "w", encoding="utf-8").write(text)
    run = subprocess.run([sys.executable, "-X", "utf8", "-I", checker, path], capture_output=True, text=True, encoding="utf-8")
    report = json.loads(run.stdout[:run.stdout.rindex("}") + 1])
    ok = run.returncode == rc_expected and report["overall"] == overall_expected and report["finding"] == finding_expected
    passed += ok
    print(f"{'PASS' if ok else 'FAIL'} {name}: rc={run.returncode} overall={report['overall']} finding={report['finding']} "
          f"errors={report['errors'][:2]}")
print(f"PLAYER_PARSER_CONTROLS {passed}/{len(cases)}")
sys.exit(0 if passed == len(cases) else 1)
