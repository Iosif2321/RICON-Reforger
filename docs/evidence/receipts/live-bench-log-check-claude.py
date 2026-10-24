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
Usage: python -I live-bench-log-check-claude.py <console.log> <radio_model_catalog_v1.json>"""
import hashlib
import json
import re
import sys

LINE = re.compile(r"\[RICON_LIVE\] seq=(\d+) t=(-?\d+) side=(\w+) kind=(\w+) player=(-?\d+) device=(-?\d+) "
                  r"tr=(-?\d+) value=(-?\d+) text=(\S+)$")


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
    done = [e for e in events if e["kind"] == "BENCH_DONE"]
    if "BENCH_ABORTED" in kinds:
        errors.append("BENCH_ABORTED present")
    if len(done) != 1 or done[0]["value"] != 6 or events[-1]["kind"] != "BENCH_DONE":
        errors.append("BENCH_DONE must be the single last bench event with phase 6")
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
        if row.get("step") != "BENCH_SET_FREQUENCY" or not mute:
            row["verdict"] = "INCOMPLETE"
        elif frequency_ok and mute_ok:
            row["verdict"] = "NATIVE_ROUNDTRIP_PASS"
        else:
            row["verdict"] = "OBSERVED_MISMATCH"
        row["frequencyRoundTrip"] = frequency_ok
        row["muteToggleAndRestore"] = mute_ok
        summary.append(row)
    verdicts = [r.get("verdict", "INCOMPLETE") for r in summary]
    if not summary:
        overall = "NOT_EXECUTED"
    elif all(v == "NATIVE_ROUNDTRIP_PASS" for v in verdicts):
        overall = "NATIVE_ROUNDTRIP_PASS"
    elif any(v == "OBSERVED_MISMATCH" for v in verdicts):
        overall = "OBSERVED_MISMATCH"
    else:
        overall = "INCOMPLETE"
    print(json.dumps({"logSha256": hashlib.sha256(raw).hexdigest(), "errors": errors, "notes": notes,
                      "overall": overall, "radios": summary}, ensure_ascii=False, indent=1))
    print("LIVE_BENCH_LOG_CHECK_" + ("PASS" if not errors else "FAIL"))
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
