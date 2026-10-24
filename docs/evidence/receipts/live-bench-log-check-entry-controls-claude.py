"""claude: positive and negative controls for the A1-E entry checks of live-bench-log-check-claude.py (codex #4315,
#4319). Synthetic entry-mode logs are derived from the real V2 console.log (setter transceiver, NATIVE_ROUNDTRIP_PASS):
BENCH_START gets setter=entry and the observation fields, one BENCH_ENTRY per radio is inserted after the spawn reads,
the step and the tr0 reads get the entry fields with a menu text built here as raw/1000 with one decimal, the mute line
gets the entry path and BENCH_DONE the MENU_ENTRY_RUNTIME level; seq is renumbered. The menu text here is a test value
for the parser, not a claim about the vanilla formatter output.
Usage: python -I live-bench-log-check-entry-controls-claude.py <repo> <V2 log> <outdir>"""
import json
import re
import subprocess
import sys

repo, log_v2, out = sys.argv[1], sys.argv[2], sys.argv[3]
parser = repo + "/docs/evidence/receipts/live-bench-log-check-claude.py"
catalog = repo + "/oracle/data/radio_model_catalog_v1.json"
LINE = re.compile(r"^(.*\[RICON_LIVE\] seq=)(\d+)( t=\S+ side=probe kind=(\w+) player=\S+ device=\S+ tr=(-?\d+) "
                  r"value=(-?\d+) text=)(\S+)$")


def stream(path):
    lines = open(path, encoding="utf-8", errors="replace").read().splitlines()
    end = next(i for i, l in enumerate(lines) if "kind=BENCH_DONE" in l)
    return [l for l in lines[:end + 1] if "[RICON_LIVE] seq=" in l and "side=probe" in l]


def esc(text):
    return "".join(c if 33 <= ord(c) <= 126 and c not in "%,=" else "%%%02X" % ord(c) for c in text)


def menu(raw):
    return esc(f"{raw / 1000:.1f} #AR-VON_FrequencyUnits_MHz")


def entry_fields(raw, emuted=0, text=None):
    return f"entryFreq={raw},usable=1,emuted={emuted},dtext={menu(raw) if text is None else text}"


MUTE = ("path=entry,before=0,ebefore=0,requested=1,ret=1,after=1,eafter=1,restoreTo=0,ret2=0,after2=0,eafter2=0,"
        "dret=0,dafter=1,edafter=1,dret2=0,dafter2=0,edafter2=0")


def build(mut=None):
    """Entry-mode log; mut(kind, device, phase, record) may return a changed record (dict) or None to drop it."""
    rows, devices_raw = [], {}
    for l in stream(log_v2):
        m = LINE.match(l)
        prefix, kind, tr, value, text = m.group(1), m.group(4), int(m.group(5)), int(m.group(6)), m.group(7)
        middle = m.group(3)
        f = dict(p.split("=", 1) for p in text.split(",") if "=" in p)
        device = f.get("bench")
        rows.append({"prefix": prefix, "middle": middle, "kind": kind, "tr": tr, "value": value, "text": text,
                     "device": device, "phase": f.get("phase")})
        if kind == "BENCH_READ" and tr == 0 and f.get("phase") == "spawn":
            devices_raw[device] = value
    result = []
    last_spawn_read = max(i for i, r in enumerate(rows) if r["kind"] == "BENCH_READ" and r["phase"] == "spawn")
    for i, r in enumerate(rows):
        if r["kind"] == "BENCH_START":
            r["text"] = r["text"].replace("setter=transceiver", "setter=entry") + ",playerController=0,factionManager=0"
        elif r["kind"] == "BENCH_SET_FREQUENCY":
            r["text"] = r["text"].replace(",setter=transceiver", f",modifier=1,{entry_fields(r['value'])},setter=entry")
        elif r["kind"] == "BENCH_READ" and r["tr"] == 0 and r["phase"] in ("t0", "t1", "t5"):
            r["text"] = r["text"] + "," + entry_fields(r["value"])
        elif r["kind"] == "BENCH_MUTE":
            r["text"] = f"bench={r['device']}," + MUTE
        elif r["kind"] == "BENCH_DONE":
            r["text"] = "level=MENU_ENTRY_RUNTIME,not=SCREEN|PLAYER_VON|KNOWN_CHANNEL|RX_TX|AUDIBLE|REPLICATION|PER_PAIR"
        result.append(r)
        if i == last_spawn_read:
            for d in sorted(devices_raw):
                raw = devices_raw[d]
                result.append({"prefix": r["prefix"], "kind": "BENCH_ENTRY", "tr": 0, "value": raw, "device": d,
                               "phase": "init", "middle": f" t=0 side=probe kind=BENCH_ENTRY player=-1 device=-1 tr=0 "
                               f"value={raw} text=",
                               "text": f"bench={d},phase=init,rawBefore={raw},raw={raw},{entry_fields(raw)}"})
    if mut:
        changed = []
        for r in result:
            r2 = mut(r)
            if r2 is not None:
                changed.append(r2)
        result = changed
    lines = []
    for seq, r in enumerate(result):
        middle = re.sub(r"value=-?\d+ text=$", f"value={r['value']} text=", r["middle"])
        lines.append(f"{r['prefix']}{seq}{middle}{r['text']}")
    return "\n".join(lines)


def on(kind, device=None, phase=None, fn=None):
    def mut(r):
        if r["kind"] == kind and (device is None or r["device"] == device) and (phase is None or r["phase"] == phase):
            return fn(r)
        return r
    return mut


def set_text(old, new):
    def fn(r):
        r = dict(r)
        r["text"] = r["text"].replace(old, new)
        return r
    return fn


def entry_after_step_log():
    """BENCH_ENTRY of device 0 placed after its BENCH_SET_FREQUENCY."""
    text = build().split("\n")
    i = next(k for k, l in enumerate(text) if "kind=BENCH_ENTRY" in l and "text=bench=0," in l)
    j = next(k for k, l in enumerate(text) if "kind=BENCH_SET_FREQUENCY" in l and "text=bench=0," in l)
    line = text.pop(i)
    text.insert(j, line)
    return "\n".join(text)


def transceiver_with_entry_log():
    """The real V2 log with one BENCH_ENTRY line added (setter=transceiver)."""
    text = [l for l in stream(log_v2)]
    i = next(k for k, l in enumerate(text) if "kind=BENCH_SET_FREQUENCY" in l)
    text.insert(i, text[i].split("kind=")[0].replace("seq=", "seq=9") + "kind=BENCH_ENTRY player=-1 device=-1 tr=0 "
                "value=32000 text=bench=0,phase=init,rawBefore=32000,raw=32000," + entry_fields(32000))
    return "\n".join(text)


rounded = on("BENCH_ENTRY", "0", None, lambda r: dict(r, value=32050, text=r["text"].replace(
    "rawBefore=32000,raw=32000,entryFreq=32000", "rawBefore=32050,raw=32050,entryFreq=32050")))
E = ["1", "entry"]
cases = [
    ("entry_ok", build(), E, 0, "MENU_ENTRY_RUNTIME_PASS"),
    # observations: the values disagree, the log itself is consistent (rc 0)
    ("text_mismatch_t5", build(on("BENCH_READ", "0", "t5", set_text("dtext=" + menu(32200), "dtext=" + menu(32000)))),
     E, 0, "OBSERVED_MISMATCH"),
    ("text_unparsed", build(on("BENCH_READ", "1", "t1", set_text("dtext=" + menu(48200), "dtext=abc"))),
     E, 0, "OBSERVED_MISMATCH"),
    ("step_wrong_text", build(on("BENCH_SET_FREQUENCY", "0", None, set_text("dtext=" + menu(32200), "dtext=garbage"))),
     E, 0, "OBSERVED_MISMATCH"),
    ("text_rounded_init", build(rounded), E, 0, "INCOMPLETE"),
    ("entry_no_gadget", build(on("BENCH_ENTRY", "3", None, lambda r: dict(r, value=-1, text="bench=3,phase=init,reason=NO_GADGET"))),
     E, 0, "INCOMPLETE"),
    ("entry_init_moved", build(on("BENCH_ENTRY", "1", None, set_text("rawBefore=48000", "rawBefore=47800"))),
     E, 0, "OBSERVED_MISMATCH"),
    ("init_unusable", build(on("BENCH_ENTRY", "2", None, set_text("usable=1", "usable=0"))), E, 0, "OBSERVED_MISMATCH"),
    ("step_entryfreq_wrong", build(on("BENCH_SET_FREQUENCY", "0", None, set_text("entryFreq=32200", "entryFreq=32000"))),
     E, 0, "OBSERVED_MISMATCH"),
    ("read_wrong_entryfreq_t5", build(on("BENCH_READ", "0", "t5", set_text("entryFreq=32200", "entryFreq=-1"))),
     E, 0, "OBSERVED_MISMATCH"),
    ("read_entryfreq_stale", build(on("BENCH_READ", "1", "t0", set_text("entryFreq=48200", "entryFreq=48000"))),
     E, 0, "OBSERVED_MISMATCH"),
    ("read_unusable_t5", build(on("BENCH_READ", "0", "t5", set_text("usable=1", "usable=0"))), E, 0, "OBSERVED_MISMATCH"),
    ("read_mute_diverges_t5", build(on("BENCH_READ", "0", "t5", set_text("emuted=0", "emuted=1"))),
     E, 0, "OBSERVED_MISMATCH"),
    ("read_emuted_diverges_t1", build(on("BENCH_READ", "3", "t1", set_text("emuted=0", "emuted=1"))),
     E, 0, "OBSERVED_MISMATCH"),
    ("mute_entry_diverges", build(on("BENCH_MUTE", "1", None, set_text("edafter=1", "edafter=0"))), E, 0, "OBSERVED_MISMATCH"),
    ("mute_toggle_failed", build(on("BENCH_MUTE", "2", None, set_text("after=1,eafter=1", "after=0,eafter=0"))),
     E, 0, "OBSERVED_MISMATCH"),
    ("mute_toggle_return_wrong", build(on("BENCH_MUTE", "2", None, set_text("ret=1,after=1", "ret=0,after=1"))),
     E, 0, "OBSERVED_MISMATCH"),
    ("mute_requested_equals_before", build(on("BENCH_MUTE", "0", None, set_text(
        "requested=1,ret=1,after=1,eafter=1", "requested=0,ret=0,after=0,eafter=0"))), E, 0, "OBSERVED_MISMATCH"),
    ("mute_restore_to_wrong", build(on("BENCH_MUTE", "1", None, set_text(
        "restoreTo=0,ret2=0,after2=0,eafter2=0", "restoreTo=1,ret2=1,after2=1,eafter2=1"))), E, 0, "OBSERVED_MISMATCH"),
    # log errors: never a PASS (rc 1)
    ("init_missing_text", build(on("BENCH_ENTRY", "0", None, set_text(",dtext=" + menu(32000), ""))), E, 1, "INCOMPLETE"),
    ("init_value_conflict", build(on("BENCH_ENTRY", "0", None, lambda r: dict(r, value=99999))), E, 1, "LOG_INVALID"),
    ("init_bad_escape", build(on("BENCH_ENTRY", "1", None, set_text("dtext=48.0%20", "dtext=48.0%2"))), E, 1, "INCOMPLETE"),
    ("step_without_dtext", build(on("BENCH_SET_FREQUENCY", "0", None, set_text(",dtext=" + menu(32200), ""))),
     E, 1, "INCOMPLETE"),
    ("step_lowercase_escape", build(on("BENCH_SET_FREQUENCY", "1", None, set_text("48.2%20", "48.2%2a"))), E, 1, "INCOMPLETE"),
    ("step_refusal_marker", build(on("BENCH_SET_FREQUENCY", "2", None, set_text("48.2%20", "48.2%ZZ"))), E, 1, "INCOMPLETE"),
    ("read_without_entry", build(on("BENCH_READ", "2", "t1", lambda r: dict(r, text=r["text"].split(",entryFreq=")[0]))),
     E, 1, "INCOMPLETE"),
    ("read_invalid_utf8", build(on("BENCH_READ", "3", "t5", set_text("dtext=42.2%20", "dtext=42.2%FF%20"))), E, 1, "INCOMPLETE"),
    ("entry_missing", build(lambda r: None if r["kind"] == "BENCH_ENTRY" and r["device"] == "2" else r), E, 1, "INCOMPLETE"),
    ("entry_after_step", entry_after_step_log(), E, 1, "LOG_INVALID"),
    ("mute_path_missing", build(on("BENCH_MUTE", "0", None, set_text("path=entry,", ""))), E, 1, "OBSERVED_MISMATCH"),
    ("done_level_wrong", build(on("BENCH_DONE", None, None, set_text("level=MENU_ENTRY_RUNTIME", "level=NATIVE_ROUNDTRIP"))),
     E, 1, "LOG_INVALID"),
    ("expect_transceiver", build(), ["1", "transceiver"], 1, "LOG_INVALID"),
    ("no_expectation", build(), ["1"], 1, "LOG_INVALID"),
    ("v2_expect_entry", "\n".join(stream(log_v2)), E, 1, "LOG_INVALID"),
    ("entry_in_transceiver_mode", transceiver_with_entry_log(), ["1", "transceiver"], 1, "LOG_INVALID"),
    ("v2_as_is", "\n".join(stream(log_v2)), ["1", "transceiver"], 0, "NATIVE_ROUNDTRIP_PASS"),
]
passed = 0
for name, text, args, rc_expected, overall_expected in cases:
    path = f"{out}/{name}.log"
    open(path, "w", encoding="utf-8").write(text)
    run = subprocess.run([sys.executable, "-X", "utf8", "-I", parser, path, catalog, *args],
                         capture_output=True, text=True, encoding="utf-8")
    report = json.loads(run.stdout[:run.stdout.rindex("}") + 1])
    ok = run.returncode == rc_expected and report["overall"] == overall_expected
    passed += ok
    print(f"{'PASS' if ok else 'FAIL'} {name}: rc={run.returncode} overall={report['overall']} errors={report['errors'][:2]}")
print(f"ENTRY_PARSER_CONTROLS {passed}/{len(cases)}")
sys.exit(0 if passed == len(cases) else 1)
