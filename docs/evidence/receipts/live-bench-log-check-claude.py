#!/usr/bin/env python3
"""claude: check and summary of a phase A0 bench log (RICON_LiveRadioBenchComponent in Play, LIVE-order §4).

Reads the streamed [RICON_LIVE] lines with kind BENCH_* (side=probe) and checks: one BENCH_START with prefabs=N; N
BENCH_SPAWN; per spawned radio the reads spawn, t0, t1 (actualDeltaMs >= 1000), t5 (actualDeltaMs >= 5000) in order;
for transceiver 0 either BENCH_SET_FREQUENCY or BENCH_SKIP; one BENCH_MUTE per radio; exactly one BENCH_DONE with value
6 after everything and no BENCH_ABORTED; no SCRIPT (E) (engine errors elsewhere in the log are listed, not hidden).
Each BENCH_SPAWN must report actualPrefab equal to the requested ResourceName. Then it summarises per radio the raw
values next to the catalog (oracle/data/radio_model_catalog_v1.json) entry found ONLY through the confirmed
ResourceName map below: passport bands (MHz) and the old reportedGame raw ranges. The ratios raw/passport are
diagnostic numbers only — game ranges differ from the prototypes (codex #4115), so no kHz/MHz verdict is drawn here;
Q-F against the UI stays open. Level NATIVE_ROUNDTRIP + PRIMARY_MANUAL, not an observation of the UI or of audio.
A0b: the RadioManagerEntity records (BENCH_START radioManager, preSpawn BENCH_MANAGER, BENCH_NOT_READY) are checked
against an optional expectation 0|1 ("-" = none).
V2: the frequency-step method (BENCH_START setter/setterGiven, setter of every BENCH_SET_FREQUENCY) is checked against an
optional expectation component|transceiver|entry; a log before V2 has no setter field and used the component wrapper.
A1-E (setter entry): per radio one BENCH_ENTRY (own vanilla SCR_VONEntryRadio of transceiver 0), the entry fields of
the step and of the tr0 reads (entryFreq, usable, emuted, dtext = %XX-escaped GetDisplayText), and the entry mute path.
Q-F at level MENU_ENTRY_RUNTIME: the number of the decoded menu text x 1000 must equal the raw getter value at every
point whose raw is a multiple of 100 (TEXT_MATCH); other raw values are TEXT_ROUNDED without a verdict (codex #4315).
Usage: python -I live-bench-log-check-claude.py <console.log> <radio_model_catalog_v1.json> [expectManager 0|1|-]
       [expectSetter component|transceiver|entry]"""
import hashlib
import json
import re
import sys

LINE = re.compile(r"\[RICON_LIVE\] seq=(\d+) t=(-?\d+) side=(\w+) kind=(\w+) player=(-?\d+) device=(-?\d+) "
                  r"tr=(-?\d+) value=(-?\d+) text=(\S+)$")


SETTERS = ("component", "transceiver", "entry")
MENU_TEXT = re.compile(r"^(-?\d+(?:\.\d+)?) #AR-VON_FrequencyUnits_MHz$")


def unescape(text):
    """Inverse of RICON_LiveText.Escape: %XX -> byte, %uXXXX -> UTF-8 of the code point."""
    out, i = bytearray(), 0
    while i < len(text):
        if text[i] == "%":
            if text[i + 1:i + 2] == "u":
                out += chr(int(text[i + 2:i + 6], 16)).encode("utf-8")
                i += 6
            else:
                out.append(int(text[i + 1:i + 3], 16))
                i += 3
        else:
            out.append(ord(text[i]))
            i += 1
    return out.decode("utf-8", errors="replace")


def menu_point(f, raw):
    """Q-F point of one record: decoded menu text, its number and the comparison with the raw getter value."""
    if "dtext" not in f:
        return None
    text = unescape(f["dtext"])
    m = MENU_TEXT.match(text)
    point = {"raw": raw, "text": text, "entryFreq": int(f.get("entryFreq", "-1"))}
    if not m:
        point["status"] = "TEXT_UNPARSED"
    elif raw % 100 != 0:
        point["status"] = "TEXT_ROUNDED"
    else:
        point["status"] = "TEXT_MATCH" if round(float(m.group(1)) * 1000) == raw else "TEXT_MISMATCH"
    return point


def fields(text):
    out = {}
    for part in text.split(","):
        if "=" in part:
            k, v = part.split("=", 1)
            out[k] = v
    return out


# Confirmed ResourceName → catalog prefab key, only from the metadata log
# ricon-liveradio2-claude-20261024-062851-RICON_LiveResourceProbePlugin (sha256 4769705f…, codex #4115):
# an unknown GUID is never matched by stripping it from the path.
CONFIRMED = {
    "{73950FBA2D7DB5C5}Prefabs/Items/Equipment/Radios/Radio_ANPRC68.et": "$ArmaReforger:Prefabs/Items/Equipment/Radios/Radio_ANPRC68.et",
    "{9B6B61BB3FE3DFB0}Prefabs/Items/Equipment/Radios/Radio_ANPRC77.et": "$ArmaReforger:Prefabs/Items/Equipment/Radios/Radio_ANPRC77.et",
    "{54C68E438DD34265}Prefabs/Items/Equipment/Radios/Radio_R107M.et": "$ArmaReforger:Prefabs/Items/Equipment/Radios/Radio_R107M.et",
    "{73EFD7591F4134D3}Prefabs/Items/Equipment/Radios/Radio_RF10/Radio_RF10.et": "$ArmaReforger:Prefabs/Items/Equipment/Radios/Radio_RF10/Radio_RF10.et",
}


def catalog_entry(catalog, prefab):
    """Confirmed ResourceName → (model id, passport frequency ranges in MHz, reportedGame raw ranges)."""
    path = CONFIRMED.get(prefab)
    if path is None:
        return None, [], []
    for binding in catalog["bindings"]:
        if binding["prefab"] == path:
            model = catalog["models"][binding["modelId"]]
            bands = [f["value"] for f in model["facts"] if f["quantity"] == "frequencyMHz"]
            game = [(t["minRaw"], t["maxRaw"]) for t in binding["reportedGame"]["transceivers"]]
            return binding["modelId"], bands, game
    return None, [], []


def main(argv):
    raw = open(argv[0], "rb").read()
    lines = raw.decode("utf-8", errors="replace").splitlines()
    catalog = json.load(open(argv[1], encoding="utf-8"))
    errors, notes = [], []
    script_errors = [l for l in lines if re.search(r"SCRIPT\s*\(E\)", l)]
    if script_errors:
        errors.append(f"SCRIPT (E): {len(script_errors)}")
    # Only the stream counts: lines inside dump=BEGIN … dump=END are the snapshot and are skipped, so a missing stream
    # line can never be "restored" from the snapshot (codex #4113). A repeated seq in the stream is an error.
    events = []
    seen_seq = set()
    in_dump = False
    for l in lines:
        if "[RICON_LIVE] dump=BEGIN" in l:
            in_dump = True
            continue
        if "[RICON_LIVE] dump=END" in l:
            in_dump = False
            continue
        m = LINE.search(l)
        if in_dump or not m or not m.group(4).startswith("BENCH_"):
            continue
        if m.group(1) in seen_seq:
            errors.append(f"stream seq {m.group(1)} repeated")
        seen_seq.add(m.group(1))
        events.append({"seq": int(m.group(1)), "kind": m.group(4), "tr": int(m.group(7)), "value": int(m.group(8)),
                       "text": m.group(9), "f": fields(m.group(9))})
    if not events:
        errors.append("no BENCH stream lines: bench did not start (readiness, not a GetFrequency/mute result)")
    kinds = [e["kind"] for e in events]
    starts = [e for e in events if e["kind"] == "BENCH_START"]
    if len(starts) != 1:
        errors.append(f"BENCH_START count {len(starts)}")
        n = 0
    else:
        n = int(starts[0]["f"].get("prefabs", "0"))
    spawns = [e for e in events if e["kind"] == "BENCH_SPAWN"]
    if len(spawns) != n:
        errors.append(f"BENCH_SPAWN {len(spawns)} != {n}")
    # A0b (codex #4233): RadioManagerEntity presence at BENCH_START and before the spawn. With an expectation (third
    # argument 0|1) exactly one preSpawn BENCH_MANAGER must precede the first BENCH_SPAWN and match it; BENCH_NOT_READY
    # means the bench was not ready (no radio result). Logs before A0b have no manager record: noted, not an error.
    expect = None
    if len(argv) > 2 and argv[2] != "-":
        if argv[2] in ("0", "1"):
            expect = int(argv[2])
        else:
            errors.append(f"expectManager must be 0 or 1, got {argv[2]!r}")
    managers = [e for e in events if e["kind"] == "BENCH_MANAGER"]
    not_ready = [e for e in events if e["kind"] == "BENCH_NOT_READY"]
    start_f = starts[0]["f"] if len(starts) == 1 else {}
    manager = {"expected": expect, "start": start_f.get("radioManager"), "startExpected": start_f.get("expectedManager"),
               "preSpawn": [m["value"] for m in managers]}
    for e in not_ready:
        errors.append("BENCH_NOT_READY: " + e["text"])
    # codex #4237: every manager field must agree, not only value; the early BENCH_START radioManager may be 0 or 1
    for m in managers:
        f = m["f"]
        if m["value"] not in (0, 1) or f.get("radioManager") != str(m["value"]) or f.get("where") != "preSpawn":
            errors.append(f"manager record inconsistent: value={m['value']} text={m['text']}")
    # codex #4238: with an expectation the early BENCH_START radioManager is mandatory (0|1); without one, checked if present
    if (expect is not None or "radioManager" in start_f) and start_f.get("radioManager") not in ("0", "1"):
        errors.append(f"BENCH_START radioManager={start_f.get('radioManager')}")
    if expect is not None:
        # order by position in the stream, not by seq value
        pos_start = next((i for i, e in enumerate(events) if e["kind"] == "BENCH_START"), None)
        pos_spawn = next((i for i, e in enumerate(events) if e["kind"] == "BENCH_SPAWN"), None)
        if start_f.get("expectedManager") != str(expect):
            errors.append(f"BENCH_START expectedManager={start_f.get('expectedManager')} != {expect}")
        if len(managers) != 1 or managers[0]["value"] != expect or managers[0]["f"].get("expected") != str(expect):
            errors.append(f"manager records {[m['text'] for m in managers]} != one preSpawn with radioManager={expect}")
        elif pos_start is None or events.index(managers[0]) < pos_start:
            errors.append("manager record before BENCH_START")
        elif pos_spawn is not None and events.index(managers[0]) > pos_spawn:
            errors.append("manager record after the first BENCH_SPAWN")
    elif start_f.get("expectedManager") in ("0", "1"):
        errors.append(f"log expects radioManager={start_f['expectedManager']}: pass the expectation explicitly")
    elif not managers:
        notes.append("no BENCH_MANAGER record (log before A0b)")
    # V2 (codex #4262): BENCH_START setter/setterGiven and the setter of every BENCH_SET_FREQUENCY must agree; without
    # the parameter only the old wrapper is allowed. With an expectation (fourth argument) the parameter must have been
    # given and match; without one a log with setterGiven=1 is an error. Logs before V2 have no setter field.
    expect_setter = None
    if len(argv) > 3:
        if argv[3] in SETTERS:
            expect_setter = argv[3]
        else:
            errors.append(f"expectSetter must be one of {SETTERS}, got {argv[3]!r}")
    steps = [e for e in events if e["kind"] == "BENCH_SET_FREQUENCY"]
    setter = {"expected": expect_setter, "start": start_f.get("setter"), "startGiven": start_f.get("setterGiven"),
              "steps": [s["f"].get("setter") for s in steps]}
    if "setter" in start_f or "setterGiven" in start_f or expect_setter is not None:
        if start_f.get("setter") not in SETTERS or start_f.get("setterGiven") not in ("0", "1"):
            errors.append(f"BENCH_START setter={start_f.get('setter')} setterGiven={start_f.get('setterGiven')}")
        elif start_f.get("setterGiven") == "0" and start_f.get("setter") != "component":
            errors.append("BENCH_START setter without the parameter must be component")
        for s in steps:
            if s["f"].get("setter") != start_f.get("setter"):
                errors.append(f"device {s['f'].get('bench')}: step setter {s['f'].get('setter')} != "
                              f"BENCH_START setter {start_f.get('setter')}")
    elif any("setter" in s["f"] for s in steps):
        errors.append("BENCH_SET_FREQUENCY names a setter but BENCH_START does not")
    else:
        notes.append("no setter record (log before V2: BaseRadioComponent.SetTransceiverFrequency)")
    if expect_setter is not None:
        if start_f.get("setterGiven") != "1" or start_f.get("setter") != expect_setter:
            errors.append(f"BENCH_START setter={start_f.get('setter')} setterGiven={start_f.get('setterGiven')} "
                          f"!= given {expect_setter}")
    elif start_f.get("setterGiven") == "1":
        errors.append(f"log was run with setter={start_f.get('setter')}: pass the expectation explicitly")
    done =[e for e in events if e["kind"] == "BENCH_DONE"]
    if "BENCH_ABORTED" in kinds:
        errors.append("BENCH_ABORTED present")
    if len(done) != 1 or done[0]["value"] != 6 or events[-1]["kind"] != "BENCH_DONE":
        errors.append("BENCH_DONE must be the single last bench event with phase 6")
    entry_mode = start_f.get("setter") == "entry"
    level = "MENU_ENTRY_RUNTIME" if entry_mode else "NATIVE_ROUNDTRIP"
    if len(done) == 1 and done[0]["f"].get("level") != level:
        errors.append(f"BENCH_DONE level={done[0]['f'].get('level')} != {level}")
    observed = {"playerController": start_f.get("playerController"), "factionManager": start_f.get("factionManager")}
    if not entry_mode and any(e["kind"] == "BENCH_ENTRY" for e in events):
        errors.append("BENCH_ENTRY without setter=entry")
    summary = []
    for spawn in spawns:
        device = spawn["f"]["bench"]
        prefab = spawn["f"].get("prefab", "?")
        mine = [e for e in events if e["f"].get("bench") == device]
        reads = {}
        for e in mine:
            if e["kind"] == "BENCH_READ" and e["tr"] == 0:
                reads.setdefault(e["f"].get("phase"), e)
        row = {"device": device, "prefab": prefab, "spawned": spawn["value"] == 1}
        if spawn["value"] != 1:
            # codex #4113: a failed spawn is "no measurement", never a successful round-trip
            errors.append(f"device {device}: spawn failed, no measurement")
            summary.append(row)
            continue
        phases = [p for p in ("spawn", "t0", "t1", "t5") if p in reads]
        if phases != ["spawn", "t0", "t1", "t5"]:
            errors.append(f"device {device}: reads {phases}")
        else:
            order = [reads[p]["seq"] for p in phases]
            if order != sorted(order):
                errors.append(f"device {device}: read order")
            if int(reads["t1"]["f"]["actualDeltaMs"]) < 1000 or int(reads["t5"]["f"]["actualDeltaMs"]) < 5000:
                errors.append(f"device {device}: delays {reads['t1']['f']['actualDeltaMs']}/{reads['t5']['f']['actualDeltaMs']}")
        step = [e for e in mine if e["kind"] in ("BENCH_SET_FREQUENCY", "BENCH_SKIP")]
        mute = [e for e in mine if e["kind"] == "BENCH_MUTE"]
        entries = [e for e in mine if e["kind"] == "BENCH_ENTRY"]
        text_points = []
        entry_ok = True
        if entry_mode:
            # A1-E: one BENCH_ENTRY before the step; InitEntry's AdjustEntryModif(0) must not move the frequency
            if len(entries) != 1:
                errors.append(f"device {device}: entry records {len(entries)}")
                entry_ok = False
            else:
                ef = entries[0]["f"]
                row["entry"] = entries[0]["text"]
                if entries[0]["value"] < 0 or "reason" in ef:
                    entry_ok = False
                elif ef.get("usable") != "1" or ef.get("rawBefore") != ef.get("raw") or ef.get("entryFreq") != ef.get("raw"):
                    errors.append(f"device {device}: entry init {entries[0]['text']}")
                    entry_ok = False
                else:
                    text_points.append(("init", menu_point(ef, int(ef["raw"]))))
                if step and events.index(entries[0]) > events.index(step[0]):
                    errors.append(f"device {device}: BENCH_ENTRY after the step")
            if step and step[0]["kind"] == "BENCH_SET_FREQUENCY":
                sf = step[0]["f"]
                if sf.get("modifier") not in ("1", "-1") or "dtext" not in sf or "entryFreq" not in sf:
                    errors.append(f"device {device}: entry step fields {step[0]['text']}")
                elif int(sf["entryFreq"]) != step[0]["value"]:
                    errors.append(f"device {device}: entry frequency after the step {sf['entryFreq']} != target")
            for p in ("t0", "t1", "t5"):
                if p in reads:
                    point = menu_point(reads[p]["f"], reads[p]["value"])
                    if point is None:
                        errors.append(f"device {device}: read {p} without entry fields")
                    text_points.append((p, point))
            row["menuText"] = {p: q for p, q in text_points}
        if len(step) != 1:
            errors.append(f"device {device}: step records {len(step)}")
        if len(mute) != 1:
            errors.append(f"device {device}: mute records {len(mute)}")
        actual = spawn["f"].get("actualPrefab")
        row["actualPrefab"] = actual
        if actual != prefab:
            errors.append(f"device {device}: actual prefab {actual} != requested {prefab}")
        r0 = reads.get("spawn")
        if r0:
            model, bands, game = catalog_entry(catalog, prefab)
            lo, hi = int(r0["f"]["min"]), int(r0["f"]["max"])
            row.update(model=model, rawMin=lo, rawMax=hi, rawRes=int(r0["f"]["res"]), rawFrequency=r0["value"],
                       passportMHz=bands, reportedGameRaw=game)
            if bands:
                # Diagnostic only (codex #4115): game ranges differ from the prototypes, so this is not a unit verdict.
                pmin, pmax = min(b[0] for b in bands), max(b[1] for b in bands)
                row["diagRatioRawMinToPassportMinMHz"] = lo / pmin
                row["diagRatioRawMaxToPassportMaxMHz"] = hi / pmax
        if step:
            row["step"] = step[0]["kind"]
            row["stepText"] = step[0]["text"]
            row["target"] = step[0]["value"] if step[0]["kind"] == "BENCH_SET_FREQUENCY" else None
            row["setter"] = step[0]["f"].get("setter")
        for p in ("t0", "t1", "t5"):
            if p in reads:
                row[f"raw_{p}"] = reads[p]["value"]
        if mute:
            row["mute"] = mute[0]["text"]
        # Functional verdict (codex #4122): PASS only if the step was set, t5 reads the target and the mute
        # toggle and restore both took effect; SKIP or failed spawn → INCOMPLETE; any mismatch → OBSERVED_MISMATCH.
        frequency_ok = row.get("step") == "BENCH_SET_FREQUENCY" and row.get("raw_t5") == row.get("target")
        m = fields(mute[0]["text"]) if mute else {}
        mute_ok = bool(m) and m.get("after") == m.get("requested") and m.get("after2") == m.get("restoreTo")
        if entry_mode and m:
            # entry path: ToggleMuteEntry and the direct SetMuteState act on one state that entry.GetIsMuted reads
            if m.get("path") != "entry":
                errors.append(f"device {device}: mute path {m.get('path')} != entry")
            mute_ok = mute_ok and m.get("path") == "entry" and m.get("ebefore") == m.get("before") \
                and m.get("eafter") == m.get("after") and m.get("eafter2") == m.get("after2") \
                and m.get("dafter") == m.get("requested") and m.get("edafter") == m.get("dafter") \
                and m.get("dafter2") == m.get("restoreTo") and m.get("edafter2") == m.get("dafter2")
        elif m.get("path") == "entry":
            errors.append(f"device {device}: entry mute path without setter=entry")
        statuses = [q["status"] for _, q in text_points if q]
        text_ok = bool(statuses) and all(s == "TEXT_MATCH" for s in statuses)
        if entry_mode and any(s in ("TEXT_MISMATCH", "TEXT_UNPARSED") for s in statuses):
            row["textMismatch"] = True
        if row.get("step") != "BENCH_SET_FREQUENCY" or not mute or (entry_mode and not entry_ok):
            row["verdict"] = "INCOMPLETE"
        elif entry_mode and frequency_ok and mute_ok and text_ok:
            row["verdict"] = "MENU_ENTRY_RUNTIME_PASS"
        elif entry_mode and frequency_ok and mute_ok and not row.get("textMismatch"):
            row["verdict"] = "INCOMPLETE"
        elif not entry_mode and frequency_ok and mute_ok:
            row["verdict"] = "NATIVE_ROUNDTRIP_PASS"
        else:
            row["verdict"] = "OBSERVED_MISMATCH"
        if entry_mode:
            row["menuTextOk"] = text_ok
        row["frequencyRoundTrip"] = frequency_ok
        row["muteToggleAndRestore"] = mute_ok
        summary.append(row)
    verdicts = [r.get("verdict", "INCOMPLETE") for r in summary]
    if not_ready:
        overall = "NOT_READY"
    elif not summary:
        overall = "NOT_EXECUTED"
    elif all(v == "NATIVE_ROUNDTRIP_PASS" for v in verdicts):
        overall = "NATIVE_ROUNDTRIP_PASS"
    elif all(v == "MENU_ENTRY_RUNTIME_PASS" for v in verdicts):
        overall = "MENU_ENTRY_RUNTIME_PASS"
    elif any(v == "OBSERVED_MISMATCH" for v in verdicts):
        overall = "OBSERVED_MISMATCH"
    else:
        overall = "INCOMPLETE"
    print(json.dumps({"logSha256": hashlib.sha256(raw).hexdigest(), "errors": errors, "notes": notes,
                      "overall": overall, "level": level, "observed": observed, "radioManager": manager,
                      "setter": setter, "radios": summary}, ensure_ascii=False, indent=1))
    print("LIVE_BENCH_LOG_CHECK_" + ("PASS" if not errors else "FAIL"))
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
