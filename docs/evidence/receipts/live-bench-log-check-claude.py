#!/usr/bin/env python3
"""claude: check and summary of a phase A0 bench log (RICON_LiveRadioBenchComponent in Play, LIVE-order §4).

Reads the streamed [RICON_LIVE] lines with kind BENCH_* (side=probe) and checks: one BENCH_START with prefabs=N; N
BENCH_SPAWN; per spawned radio the reads spawn, t0, t1 (actualDeltaMs >= 1000), t5 (actualDeltaMs >= 5000) in order;
for transceiver 0 either BENCH_SET_FREQUENCY or BENCH_SKIP; one BENCH_MUTE per radio; exactly one BENCH_DONE with value
6 after everything and no BENCH_ABORTED; no SCRIPT (E) (engine errors elsewhere in the log are listed, not hidden).
Then it summarises per radio the raw values and compares raw min/max with the passport band of the catalog
(oracle/data/radio_model_catalog_v1.json, matched by exact prefab path): ratio raw/MHz is reported for the two unit
hypotheses (kHz: ratio 1000, MHz: ratio 1). This is an offline comparison at level NATIVE_ROUNDTRIP + PRIMARY_MANUAL,
not an observation of the UI and not a proof of what the player hears.
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


def passport_band(catalog, prefab):
    """Exact prefab path match ($ArmaReforger:<path without GUID>) → passport frequency ranges (MHz)."""
    path = "$ArmaReforger:" + re.sub(r"^\{[0-9A-F]{16}\}", "", prefab)
    for binding in catalog["bindings"]:
        if binding["prefab"] == path:
            model = catalog["models"][binding["modelId"]]
            return binding["modelId"], [f["value"] for f in model["facts"] if f["quantity"] == "frequencyMHz"]
    return None, []


def main(argv):
    raw = open(argv[0], "rb").read()
    lines = raw.decode("utf-8", errors="replace").splitlines()
    catalog = json.load(open(argv[1], encoding="utf-8"))
    errors, notes = [], []
    script_errors = [l for l in lines if re.search(r"SCRIPT\s*\(E\)", l)]
    if script_errors:
        errors.append(f"SCRIPT (E): {len(script_errors)}")
    events = []
    seen_seq = set()
    for l in lines:
        m = LINE.search(l)
        if m and m.group(4).startswith("BENCH_") and m.group(1) not in seen_seq:
            seen_seq.add(m.group(1))  # the stream prints each record once; Dump repeats them — keep the first
            events.append({"seq": int(m.group(1)), "kind": m.group(4), "tr": int(m.group(7)), "value": int(m.group(8)),
                           "text": m.group(9), "f": fields(m.group(9))})
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
            notes.append(f"device {device}: spawn failed")
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
        r0 = reads.get("spawn")
        if r0:
            model, bands = passport_band(catalog, prefab)
            lo, hi = int(r0["f"]["min"]), int(r0["f"]["max"])
            row.update(model=model, rawMin=lo, rawMax=hi, rawRes=int(r0["f"]["res"]), rawFrequency=r0["value"],
                       passportMHz=bands)
            if bands:
                pmin, pmax = min(b[0] for b in bands), max(b[1] for b in bands)
                row["ratioMinRawToPassportMHz"] = lo / pmin
                row["ratioMaxRawToPassportMHz"] = hi / pmax
        if step:
            row["step"] = step[0]["kind"]
            row["stepText"] = step[0]["text"]
            row["target"] = step[0]["value"] if step[0]["kind"] == "BENCH_SET_FREQUENCY" else None
        for p in ("t0", "t1", "t5"):
            if p in reads:
                row[f"raw_{p}"] = reads[p]["value"]
        if mute:
            row["mute"] = mute[0]["text"]
        summary.append(row)
    print(json.dumps({"logSha256": hashlib.sha256(raw).hexdigest(), "errors": errors, "notes": notes,
                      "radios": summary}, ensure_ascii=False, indent=1))
    print("LIVE_BENCH_LOG_CHECK_" + ("PASS" if not errors else "FAIL"))
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
