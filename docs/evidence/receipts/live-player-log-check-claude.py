#!/usr/bin/env python3
"""claude: check and summary of an A1-P0 log (RICON_LiveRadioBenchComponent with -riconBenchMode=player, claude #4360,
codex #4365): read-only observation of the local player in Workbench Play.
Checks: one BENCH_START with mode=player, modeGiven=1; no radio or control action at all (no BENCH_SPAWN, _READ, _SKIP,
_SET_FREQUENCY, _MUTE, _ENTRY in the stream); exactly 30 PLAYER_SAMPLE with values 1..30 in stream order, the first
at elapsedMs >= 1000 and each next at least 1 s of game time after the previous one (no catch-up burst after a stall,
codex #4372); every sample has a consistent field set (pc=0 -> no player fields; pc=1 -> playerId,
vonController (+ vonEntries), controlled; controlled=1 -> controlledPrefab decoded by the strict shared codec, entityVoN,
gadgetManager); BENCH_DONE last with value 30 and level=PLAYER_OBSERVATION; no SCRIPT (E). Only the stream counts (the
dump between dump=BEGIN/END repeats it). The finding is descriptive: LOCAL_PLAYER_ABSENT (pc=0 in every sample),
LOCAL_PLAYER_PRESENT (pc=1 in every sample), LOCAL_PLAYER_CHANGED (otherwise, with the first sample of each state).
A1-P1 (claude #4385, codex #4384), second argument "entries": after each sample with a VON controller exactly vonEntries
PLAYER_ENTRY records (value = sample, idx 0..N-1) with a consistent field set per entry kind (radio entries add the
transceiver number, entry frequency and mute, range, and the transceiver raw/min/max/res/mute and radio prefab); the
player's own menu text is compared with the raw getter value descriptively (TEXT_MATCH / ROUNDED / MISMATCH / UNPARSED).
Without the argument a log with PLAYER_ENTRY is an error (an A1-P0 log has none).
A1-P2 (claude #4399, codex #4388/#4390), second argument "menu" (implies entries, mode playermenu): the PLAYER_ACT
plan CHOOSE(5) STEP(6) READ t1(7) READ t5(11) RESTORE(12) MUTE(14) UNMUTE(15) FINAL(20), or a prefix of it closed by
one STOP with nothing after it; values checked against the chosen raw0/target/muted0 (menu text x 1000 = raw,
ToggleMuteEntry returns the new state), and the entries after FINAL equal those before CHOOSE. Verdicts
PLAYER_MENU_RUNTIME_PASS / OBSERVED_MISMATCH / STOPPED; BENCH_DONE level=PLAYER_MENU_RUNTIME with stopped=0|1.
A1-S1 (claude #4431, codex #4425/#4427/#4430/#4432), second argument "screen" (implies entries, mode playerscreen):
SCREEN records BEFORE(5), one OPEN or ALREADY_OPEN(6), MENU(7..12, 20, 30) with one WIDGET record per entry after each
MENU of 7..12; PLAYER_SCREEN_WIDGET_PASS when the menu stays open, the active entry and all radio entries never change,
and every entry's FrequencyText TextWidget is visible in the hierarchy with a non-zero size and shows exactly the entry's
display text. A live widget is not a proof of pixels or localization.
A1-S2 (claude #4488, codex #4489/#4491/#4492), second argument "screenstep" (implies entries, mode playerscreenstep):
the S1 screen records on BEFORE(5), OPEN(6), MENU(7, 8, 10, 14, 17, 20, 30) with WIDGET records after the MENU of 7, 8,
10, 14, 17; the PLAYER_ACT plan CHOOSE(5) STEP(9) READ t1(10) READ t5(14) RESTORE(15) FINAL(20) without mute,
checked like A1-P2 (target entry reads target on samples 10..15); the widget number x 1000 equals the target entry's
raw (raw0 on 7, 8, 17, target on 10, 14) and every other entry never changes; PLAYER_READER PRE(7) POST(10) BACK(17)
of the production RICON_BaseRadioObservationReader.TryRead: ok, authority, deviceId 1, the chosen transceiver index,
and rawBefore = tuning = rawAfter = raw0 / target / raw0. Verdicts PLAYER_SCREEN_STEP_PASS / _LABEL_UNVERIFIED /
OBSERVED_MISMATCH / STOPPED / NOT_READY / LOG_INVALID; BENCH_DONE level=PLAYER_SCREEN_STEP.
Usage: python -I live-player-log-check-claude.py <console.log> [entries|menu|screen|screenstep]"""
import hashlib
import importlib.util
import json
import pathlib
import re
import sys

sys.dont_write_bytecode = True  # no __pycache__ in the receipts folder
_spec = importlib.util.spec_from_file_location(
    "live_text_codec_claude", pathlib.Path(__file__).resolve().parent / "live_text_codec_claude.py")
codec = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(codec)

LINE = re.compile(r"\[RICON_LIVE\] seq=(\d+) t=(-?\d+) side=(\w+) kind=(\w+) player=(-?\d+) device=(-?\d+) "
                  r"tr=(-?\d+) value=(-?\d+) text=(\S+)$")
FORBIDDEN = {"BENCH_SPAWN", "BENCH_READ", "BENCH_SKIP", "BENCH_SET_FREQUENCY", "BENCH_MUTE", "BENCH_ENTRY"}
SAMPLES = 30
BIT = ("0", "1")
INT = re.compile(r"-?\d+")


def fields(text):
    out = {}
    for part in text.split(","):
        if "=" in part:
            k, v = part.split("=", 1)
            out[k] = v
    return out


def sample_errors(f):
    """Field-set consistency of one PLAYER_SAMPLE; returns (errors, decoded prefab or None)."""
    errs, prefab = [], None
    base = {"elapsedMs", "isServer", "isClient", "players", "pc"}
    want = set(base)
    if f.get("isServer") not in BIT or f.get("isClient") not in BIT or f.get("pc") not in BIT:
        errs.append("role/pc not 0|1")
    if not INT.fullmatch(f.get("players", "")) or not INT.fullmatch(f.get("elapsedMs", "")):
        errs.append("players/elapsedMs not an integer")
    if f.get("pc") == "1":
        want |= {"playerId", "vonController", "controlled"}
        if not INT.fullmatch(f.get("playerId", "")) or f.get("vonController") not in BIT or f.get("controlled") not in BIT:
            errs.append("player fields")
        if f.get("vonController") == "1":
            want.add("vonEntries")
            if not INT.fullmatch(f.get("vonEntries", "")):
                errs.append("vonEntries not an integer")
        if f.get("controlled") == "1":
            want |= {"controlledPrefab", "entityVoN", "gadgetManager"}
            if f.get("entityVoN") not in BIT or f.get("gadgetManager") not in BIT:
                errs.append("controlled fields")
            try:
                prefab = codec.unescape(f.get("controlledPrefab", ""))
            except ValueError as exc:
                errs.append(f"controlledPrefab: {exc}")
    if set(f) != want:
        errs.append(f"field set {sorted(set(f) ^ want)}")
    return errs, prefab


ENTRY_FULL = {"idx", "entry", "cls", "method", "active", "transmit", "usable", "dtext", "radioEntry"}
MENU_TEXT = re.compile(r"^(-?\d+(?:\.\d+)?) #AR-VON_FrequencyUnits_MHz$")


def entry_errors(f):
    """A1-P1 field-set consistency of one PLAYER_ENTRY; returns (errors, summary dict)."""
    errs, out = [], {"idx": f.get("idx")}
    if f.get("entry") == "0":
        if set(f) != {"idx", "entry"}:
            errs.append(f"null entry field set {sorted(set(f))}")
        return errs, out
    want = set(ENTRY_FULL)
    for k in ("active", "transmit", "usable", "radioEntry"):
        if f.get(k) not in BIT:
            errs.append(f"{k} not 0|1")
    if not INT.fullmatch(f.get("method", "")) or not re.fullmatch(r"\w+", f.get("cls", "")):
        errs.append("method/cls")
    try:
        out["text"] = codec.unescape(f.get("dtext", ""))
    except ValueError as exc:
        errs.append(f"dtext: {exc}")
    out.update({k: f.get(k) for k in ("cls", "method", "active", "transmit", "usable", "radioEntry")})
    if f.get("radioEntry") == "1":
        want |= {"trNumber", "entryFreq", "emuted", "gadget", "transceiver"}
        # codex #4390: every bool of a radio entry only 0|1 (longRange/muted when present)
        for k in ("emuted", "gadget", "transceiver") + (("longRange",) if "longRange" in f else ()) \
                + (("muted",) if "muted" in f else ()):
            if f.get(k) not in BIT:
                errs.append(f"{k} not 0|1")
        if f.get("gadget") == "1":
            want.add("longRange")
        if f.get("transceiver") == "1":
            want |= {"raw", "min", "max", "res", "muted", "radioPrefab", "trIndex", "inventorySlot"}
            # editorRadio (BaseRadioComponent.IsEditorRadio, codex #4402) appears from A1-P2 on; the A1-P1 log has none
            if "editorRadio" in f:
                want.add("editorRadio")
                if f["editorRadio"] not in BIT:
                    errs.append("editorRadio not 0|1")
                out["editorRadio"] = f["editorRadio"]
            if f.get("gadget") == "1":
                want.add("gadgetOwnerMatch")
                if f.get("gadgetOwnerMatch") not in BIT:
                    errs.append("gadgetOwnerMatch not 0|1")
            try:
                out["radioPrefab"] = codec.unescape(f.get("radioPrefab", ""))
            except ValueError as exc:
                errs.append(f"radioPrefab: {exc}")
            for k in ("raw", "min", "max", "res", "trIndex", "inventorySlot"):
                if not INT.fullmatch(f.get(k, "")):
                    errs.append(f"{k} not an integer")
            # codex #4388: entry <-> transceiver <-> radio in the controlled character's inventory (descriptive)
            out.update({k: f.get(k) for k in ("trIndex", "inventorySlot", "gadgetOwnerMatch")})
            if INT.fullmatch(f.get("trIndex", "")) and INT.fullmatch(f.get("inventorySlot", "")) \
                    and INT.fullmatch(f.get("trNumber", "")):
                # codex #4390: MAPPED needs a gadget whose owner is the radio; without a gadget only the
                # transceiver <-> inventory part is known (PARTIAL), never an implied owner match
                linked = int(f["trIndex"]) == int(f["trNumber"]) - 1 and int(f["inventorySlot"]) >= 0 \
                    and f.get("editorRadio") != "1"
                out["mapping"] = ("NOT_MAPPED" if not linked or f.get("gadgetOwnerMatch") == "0" else
                                  "MAPPED" if f.get("gadgetOwnerMatch") == "1" else "PARTIAL")
        for k in ("trNumber", "entryFreq"):
            if not INT.fullmatch(f.get(k, "")):
                errs.append(f"{k} not an integer")
        out.update({k: f.get(k) for k in ("trNumber", "entryFreq", "emuted", "longRange", "raw", "res", "muted")})
        # descriptive Q-F point of the player's own menu entry: text number x 1000 against the raw getter value
        m = MENU_TEXT.match(out.get("text", ""))
        if f.get("transceiver") == "1" and INT.fullmatch(f.get("raw", "")):
            raw = int(f["raw"])
            out["menuText"] = ("TEXT_UNPARSED" if not m else "TEXT_ROUNDED" if raw % 100 else
                               "TEXT_MATCH" if round(float(m.group(1)) * 1000) == raw else "TEXT_MISMATCH")
    if set(f) != want:
        errs.append(f"field set {sorted(set(f) ^ want)}")
    return errs, out


ACT_PLAN = [(5, "CHOOSE"), (6, "STEP"), (7, "READ"), (11, "READ"), (12, "RESTORE"), (14, "MUTE"), (15, "UNMUTE"),
            (20, "FINAL")]
ACT_STATE = {"bound", "raw", "entryFreq", "muted", "emuted", "dtext"}
# A1-P2: phases of STEP/READ, samples whose entry snapshot shows the target, sample whose snapshot shows the mute
MENU_SPEC = {"plan": ACT_PLAN, "phases": {6: "t0", 7: "t1", 11: "t5"}, "target": range(7, 13), "mute": 15}
# A1-S2 (codex #4489): the same step on the open menu, no mute; entries are recorded before the act of their sample,
# so the snapshot of 9 still shows raw0 and that of 15 still shows the target
STEP_SPEC = {"plan": [(5, "CHOOSE"), (9, "STEP"), (10, "READ"), (14, "READ"), (15, "RESTORE"), (20, "FINAL")],
             "phases": {9: "t0", 10: "t1", 14: "t5"}, "target": range(10, 16), "mute": None}


def menu_status(f, raw):
    try:
        m = MENU_TEXT.match(codec.unescape(f.get("dtext", "")))
    except ValueError:
        return "TEXT_UNDECODABLE"
    if not m:
        return "TEXT_UNPARSED"
    if raw % 100:
        return "TEXT_ROUNDED"
    return "TEXT_MATCH" if round(float(m.group(1)) * 1000) == raw else "TEXT_MISMATCH"


def check_acts(acts, errors, by_sample, spec=MENU_SPEC, cut=None):
    """A1-P2 (claude #4399, codex #4388/#4390/#4402/#4414): the planned PLAYER_ACT sequence, or a prefix of it closed by
    one STOP after which nothing follows. Bound to the plan: phases STEP 6 = t0, READ 7 = t1, READ 11 = t5; one idx on
    the whole chain; CHOOSE has res > 0, raw0 and target in [min, max], target = raw0 + modifier x res != raw0, and refers
    to an existing non-active MAPPED radio entry of its own sample (same idx, trNumber, slot, raw0, mute0, min/max/res).
    The PLAYER_ENTRY snapshots must agree with the acts: the target entry reads target on samples 7..12 and raw0
    otherwise, is muted only on sample 15 (recorded between MUTE and UNMUTE), and the other entries never change.
    spec gives the plan, phases, target samples and mute sample (MENU_SPEC for A1-P2, STEP_SPEC for A1-S2). cut: the
    sample of a SCREEN NOT_READY abort (A1-S2); the acts then are exactly the planned ones before it, without STOP.
    Returns (verdict, rows, chosen). Log-format problems go to errors; value disagreements make OBSERVED_MISMATCH."""
    rows, mismatches, plan = [], [], spec["plan"]
    stop = next((i for i, a in enumerate(acts) if a["f"].get("op") == "STOP"), None)
    if stop is not None and stop != len(acts) - 1:
        errors.append("PLAYER_ACT after STOP")
    if stop is not None and cut is not None:
        errors.append("PLAYER_ACT STOP and SCREEN NOT_READY in one run")
    planned = acts if stop is None else acts[:stop]
    if [(a["value"], a["f"].get("op")) for a in planned] != plan[:len(planned)]:
        errors.append(f"PLAYER_ACT sequence {[(a['value'], a['f'].get('op')) for a in planned]} is not a prefix of the plan")
    if cut is not None:
        if len(planned) != len([p for p in plan if p[0] < cut]):
            errors.append(f"PLAYER_ACT before NOT_READY at {cut} is not the planned prefix")
    elif stop is None and len(planned) != len(plan):
        errors.append("PLAYER_ACT plan incomplete without STOP")
    phases = spec["phases"]
    for a in planned:
        if a["value"] in phases and a["f"].get("phase") != phases[a["value"]]:
            errors.append(f"PLAYER_ACT {a['value']} {a['f'].get('op')}: phase {a['f'].get('phase')} != {phases[a['value']]}")
    idxs = {a["f"].get("idx") for a in acts}
    if len(idxs) != 1:
        errors.append(f"PLAYER_ACT idx not one value: {sorted(map(str, idxs))}")
    choose = planned[0]["f"] if planned and planned[0]["f"].get("op") == "CHOOSE" else {}
    try:
        raw0, target, mod = int(choose["raw0"]), int(choose["target"]), int(choose["modifier"])
        muted0, idx = choose["muted0"], choose["idx"]
        res, lo, hi = int(choose["res"]), int(choose["min"]), int(choose["max"])
        tr_number, slot = choose["trNumber"], choose["slot"]
    except (KeyError, ValueError):
        if planned:
            errors.append("CHOOSE fields")
        return ("STOPPED" if stop is not None else "NOT_READY" if cut is not None else "INCOMPLETE"), rows, {}
    if mod not in (1, -1) or res <= 0 or target != raw0 + mod * res or target == raw0 or muted0 not in BIT \
            or not (lo <= raw0 <= hi and lo <= target <= hi):
        errors.append("CHOOSE target/modifier/res/range/muted0")
    # CHOOSE refers to an existing non-active MAPPED radio entry of its own sample
    chosen = next((e for e in by_sample.get(planned[0]["value"], []) if e.get("idx") == idx), None)
    if not chosen or chosen.get("cls") != "SCR_VONEntryRadio" or chosen.get("active") != "0" \
            or chosen.get("transmit") != "0" or chosen.get("mapping") != "MAPPED" or chosen.get("trNumber") != tr_number \
            or chosen.get("inventorySlot") != slot or chosen.get("raw") != str(raw0) or chosen.get("muted") != muted0 \
            or chosen.get("res") != str(res):
        errors.append(f"CHOOSE idx {idx} does not match a non-active MAPPED radio entry of sample {planned[0]['value']}")
    for a in acts:
        f, op = a["f"], a["f"].get("op")
        if op == "STOP":
            rows.append({"n": a["value"], "op": op, "reason": f.get("reason")})
            continue
        if not ACT_STATE <= set(f) or f.get("bound") != "1" or not INT.fullmatch(f.get("raw", "")) \
                or not INT.fullmatch(f.get("entryFreq", "")) or f.get("muted") not in BIT or f.get("emuted") not in BIT:
            errors.append(f"PLAYER_ACT {a['value']} {op}: state fields")
            continue
        raw, efreq, muted = int(f["raw"]), int(f["entryFreq"]), f["muted"]
        want_raw = target if op in ("STEP", "READ") else raw0
        want_muted = muted0 if op not in ("MUTE",) else str(1 - int(muted0))
        text = menu_status(f, raw)
        ok = raw == want_raw and efreq == want_raw and muted == want_muted and f["emuted"] == muted \
            and text in ("TEXT_MATCH", "TEXT_ROUNDED")
        if op in ("MUTE", "UNMUTE"):
            ok = ok and f.get("ret") == muted  # ToggleMuteEntry returns the new state (SCR_VONEntryRadio.c:240-244)
        if op in ("STEP", "READ"):
            # codex #4402: t0/t1/t5 relative to the actual step call
            need = {"t0": 0, "t1": 1000, "t5": 5000}.get(f.get("phase"))
            since = int(f["sinceStepMs"]) if INT.fullmatch(f.get("sinceStepMs", "")) else None
            if need is None or since is None or since < need or (need == 0 and since != 0):
                errors.append(f"PLAYER_ACT {a['value']} {op}: phase={f.get('phase')} sinceStepMs={f.get('sinceStepMs')}")
        if text == "TEXT_UNDECODABLE":
            errors.append(f"PLAYER_ACT {a['value']} {op}: dtext undecodable")
        if not ok:
            mismatches.append(a["value"])
        rows.append({"n": a["value"], "op": op, "phase": f.get("phase"), "raw": raw, "entryFreq": efreq,
                     "muted": muted, "emuted": f["emuted"], "ret": f.get("ret"), "text": text, "ok": ok})
    # codex #4414: the entry snapshots during the experiment agree with the acts (target entry) and nothing else moves
    last = acts[stop]["value"] if stop is not None else cut
    before = {e.get("idx"): e for e in by_sample.get(4, [])}
    for n in sorted(by_sample):
        if last is not None and n > last:
            break
        for e in by_sample[n]:
            if e.get("idx") == idx:
                want_raw = target if n in spec["target"] else raw0
                want_muted = str(1 - int(muted0)) if n == spec["mute"] else muted0
                if e.get("raw") != str(want_raw) or e.get("entryFreq") != str(want_raw) or e.get("muted") != want_muted:
                    mismatches.append(f"entry@{n}")
            elif before.get(e.get("idx")) is not None and json.dumps(e, sort_keys=True) != \
                    json.dumps(before[e.get("idx")], sort_keys=True):
                mismatches.append(f"other@{n}")
    chosen_out = {"idx": idx, "raw0": raw0, "target": target, "muted0": muted0, "trNumber": tr_number}
    if stop is not None:
        return "STOPPED", rows, chosen_out
    if cut is not None:
        return "NOT_READY", rows, chosen_out
    return ("OBSERVED_MISMATCH" if mismatches else "PLAYER_MENU_RUNTIME_PASS"), rows, chosen_out


SCREEN_PLAN = [(5, "BEFORE"), (6, "OPEN")] + [(n, "MENU") for n in range(7, 13)] + [(20, "MENU"), (30, "MENU")]
SCREEN_SPEC = {"plan": SCREEN_PLAN, "widgets": range(7, 13), "step": False}
# A1-S2 (codex #4492): PRE on 7 and 8 (STEP on 9), POST on 10 and 14, BACK on 17 (RESTORE on 15)
STEP_SCREEN_SPEC = {"plan": [(5, "BEFORE"), (6, "OPEN")] + [(n, "MENU") for n in (7, 8, 10, 14, 17, 20, 30)],
                    "widgets": (7, 8, 10, 14, 17), "step": True}
STEP_MOVING = ("raw", "entryFreq", "text", "menuText")  # fields of the stepped entry that follow its frequency
MENU_FIELDS = {"op", "opened", "hasDisplay", "inputs", "selectedId", "activeIdx"}


def widget_fields(f, prefix, errs):
    """One TextWidget block of a WIDGET record: present=0 -> nothing else; present=1 -> text, visibility, pos, size."""
    keys = {prefix}
    if f.get(prefix) not in BIT:
        errs.append(f"{prefix} not 0|1")
        return keys, None
    if f[prefix] == "0":
        return keys, None
    names = [prefix + s for s in ("Text", "Vis", "X", "Y", "W", "H")]
    keys |= set(names)
    if f.get(prefix + "Vis") not in BIT or not all(INT.fullmatch(f.get(n, "")) for n in names[2:]):
        errs.append(f"{prefix} widget fields")
        return keys, None
    try:
        text = codec.unescape(f.get(prefix + "Text", ""))
    except ValueError as exc:
        errs.append(f"{prefix}Text: {exc}")
        return keys, None
    return keys, {"text": text, "visible": f[prefix + "Vis"] == "1", "w": int(f[prefix + "W"]), "h": int(f[prefix + "H"]),
                  "x": int(f[prefix + "X"]), "y": int(f[prefix + "Y"])}


def check_screen(screens, by_sample, errors, spec=SCREEN_SPEC, chosen=None, last=SAMPLES):
    """A1-S1 (claude #4431, codex #4425/#4427/#4430/#4432): BEFORE (5), one OPEN or ALREADY_OPEN (6), MENU on 7..12, 20,
    30, and after each MENU of 7..12 one WIDGET record per entry. Clean observation: the radio entries (frequency, mute,
    active, transmit, text) and the active entry never change over all samples. Widget evidence per entry and sample: the
    FrequencyText TextWidget exists, is visible in the hierarchy with a non-zero screen size, and its text equals the
    entry's own display text of the same sample. A1-S2 (spec STEP_SCREEN_SPEC, chosen from check_acts, last = the
    sample of a STOP): the plan up to last; the stepped entry may change only in STEP_MOVING (its values are checked by
    check_acts), and on every widget sample the number of its FrequencyText x 1000 equals raw0 before the step and after
    the return, target between them; other widgets show their own entry's raw. Returns (verdict, summary)."""
    menus = [s for s in screens if s["f"].get("op") != "WIDGET"]
    widgets = [s for s in screens if s["f"].get("op") == "WIDGET"]
    if any(s["f"].get("op") == "NOT_READY" for s in screens):
        return "NOT_READY", {}
    got = [(s["value"], s["f"].get("op")) for s in menus]
    want = [(n, "ALREADY_OPEN" if op == "OPEN" and len(menus) > 1 and menus[1]["f"].get("op") == "ALREADY_OPEN" else op)
            for n, op in spec["plan"] if n <= last]
    if got != want:
        errors.append(f"SCREEN sequence {got} != plan")
    for s in menus:
        f = s["f"]
        want = set(MENU_FIELDS)
        if f.get("op") == "OPEN":
            # codex #4463/#4465: OPEN goes through the menu's own config controller and logs its inputs config
            want |= {"hadControl", "controls"}
            if f.get("controls") == "1":
                want |= {"openAction", "closeOnRelease", "performOnClose", "closeOnPerform"}
            # codex #4467: the bench calls OnInputOpen only with the controller's inputs; OPEN with controls=0 is invalid
            if f.get("controls") != "1":
                errors.append(f"SCREEN {s['value']} OPEN without controller inputs config (controls={f.get('controls')})")
            for k in ("hadControl", "controls", "closeOnRelease", "performOnClose", "closeOnPerform"):
                if k in want and f.get(k) not in BIT:
                    errors.append(f"SCREEN {s['value']} OPEN: {k} not 0|1")
            if "openAction" in f:
                try:
                    codec.unescape(f["openAction"])
                except ValueError as exc:
                    errors.append(f"SCREEN {s['value']} OPEN: openAction {exc}")
        if set(f) != want or f.get("opened") not in BIT or f.get("hasDisplay") not in BIT or f.get("inputs") not in BIT \
                or not INT.fullmatch(f.get("selectedId", "")) or not INT.fullmatch(f.get("activeIdx", "")):
            errors.append(f"SCREEN {s['value']} {f.get('op')}: menu fields")
    before = next((s["f"] for s in menus if s["f"].get("op") == "BEFORE"), {})
    opening = next((s["f"] for s in menus if s["f"].get("op") in ("OPEN", "ALREADY_OPEN")), {})
    mismatches, rows = [], []
    if opening.get("op") == "OPEN" and before.get("opened") != "0":
        errors.append("OPEN although the menu was open before")
    if opening.get("op") == "ALREADY_OPEN" and before.get("opened") != "1":
        errors.append("ALREADY_OPEN although the menu was closed before")
    for s in menus:
        if s["value"] >= 6 and s["f"].get("opened") != "1":
            mismatches.append(f"menu closed at {s['value']}")
        # codex #4436: an opened menu must also have its display
        if s["value"] >= 6 and s["f"].get("hasDisplay") != "1":
            mismatches.append(f"menu without display at {s['value']}")
        # codex #4458: after the opening the menu must also have its controller inputs (BEFORE may lack them: lazy)
        if s["value"] >= 6 and s["f"].get("inputs") != "1":
            mismatches.append(f"menu without controller inputs at {s['value']}")
        if s["f"].get("activeIdx") != before.get("activeIdx"):
            mismatches.append(f"active entry changed at {s['value']}")
    step_idx = str(chosen.get("idx")) if spec["step"] and chosen else None
    shown = []
    for n in [m for m in spec["widgets"] if m <= last]:
        mine = [w for w in widgets if w["value"] == n]
        entries = by_sample.get(n, [])
        if [w["f"].get("idx") for w in mine] != [str(i) for i in range(len(entries))]:
            errors.append(f"sample {n}: WIDGET idx {[w['f'].get('idx') for w in mine]} != entries")
        for w in mine:
            f, errs = w["f"], []
            keys = {"op", "idx", "component", "root"}
            if f.get("component") not in BIT or f.get("root") not in BIT:
                errs.append("component/root not 0|1")
            elif f.get("component") == "0" and f.get("root") == "1":
                errs.append("root without component")
            fw = cw = None
            if f.get("root") == "1":
                keys.add("rootVis")
                if f.get("rootVis") not in BIT:
                    errs.append("rootVis not 0|1")
                k1, fw = widget_fields(f, "f", errs)
                k2, cw = widget_fields(f, "c", errs)
                keys |= k1 | k2
            if set(f) != keys:
                errs.append(f"field set {sorted(set(f) ^ keys)}")
            for x in errs:
                errors.append(f"sample {n} WIDGET {f.get('idx')}: {x}")
            entry = next((e for e in entries if e.get("idx") == f.get("idx")), {})
            # codex #4427, #4436: GetText may keep the localization key (WIDGET_KEY = exactly the entry text); a different
            # label with the same number is LABEL_UNVERIFIED (no verified translation), recorded as a fact, not a PASS
            etext = entry.get("text") or ""
            kind = None
            if fw is not None:
                if fw["text"] == etext:
                    kind = "WIDGET_KEY" if "#" in etext else "WIDGET_EQUAL"
                elif fw["text"].split(" ", 1)[0] == etext.split(" ", 1)[0] and "#" not in fw["text"] and " " in fw["text"]:
                    # codex #4436: same number, a different label without a verified translation of the key
                    kind = "LABEL_UNVERIFIED"
            ok = kind is not None and fw["visible"] and fw["w"] > 0 and fw["h"] > 0 and f.get("rootVis") == "1"
            if spec["step"]:
                # A1-S2: the number on the screen x 1000 is the raw value the entry should have on this sample
                if f.get("idx") == step_idx:
                    want_raw = chosen["target"] if n in STEP_SPEC["target"] else chosen["raw0"]
                else:
                    want_raw = int(entry["raw"]) if INT.fullmatch(entry.get("raw") or "") else None
                try:
                    shown_raw = round(float(fw["text"].split(" ", 1)[0]) * 1000) if fw is not None else None
                except ValueError:
                    shown_raw = None
                if shown_raw is None or shown_raw != want_raw:
                    ok = False
                if f.get("idx") == step_idx:
                    shown.append({"n": n, "text": fw["text"] if fw else None, "shownRaw": shown_raw, "wantRaw": want_raw})
            if not ok:
                mismatches.append(f"widget {f.get('idx')}@{n}")
            rows.append({"n": n, "idx": f.get("idx"), "component": f.get("component"), "root": f.get("root"),
                         "rootVisible": f.get("rootVis"), "frequency": fw, "channel": cw, "entryText": entry.get("text"),
                         "textKind": kind})
    # clean observation: the radio state seen through the entries never changes (A1-S2: except the stepped entry's
    # frequency fields, which check_acts compares with the plan)
    def still(rows_):
        return json.dumps([{k: v for k, v in e.items() if e.get("idx") != step_idx or k not in STEP_MOVING}
                           for e in rows_], sort_keys=True)
    first = still(by_sample.get(1, []))
    changed = [n for n in sorted(by_sample) if still(by_sample[n]) != first]
    if changed:
        mismatches.append(f"entries changed at {changed[:5]}")
    if opening.get("op") == "OPEN" and "openAction" in opening:
        try:
            opening = dict(opening, openAction=codec.unescape(opening["openAction"]))
        except ValueError:
            pass
    summary = {"before": before, "opening": opening, "widgets": rows[:len(by_sample.get(7, []))], "stepShown": shown,
               "entriesChangedAt": changed, "mismatches": mismatches[:10],
               "textKinds": sorted({str(r["textKind"]) for r in rows}),
               "selectionChanged": sorted({s["f"].get("selectedId") for s in menus}) if len({s["f"].get("selectedId") for s in menus}) > 1 else []}
    if mismatches:
        return "OBSERVED_MISMATCH", summary
    if any(r["textKind"] == "LABEL_UNVERIFIED" for r in rows):
        return "PLAYER_SCREEN_WIDGET_LABEL_UNVERIFIED", summary
    return "PLAYER_SCREEN_WIDGET_PASS", summary


READER_PLAN = [(7, "PRE"), (10, "POST"), (17, "BACK")]
READER_BASE = {"phase", "idx", "bound"}
READER_BOUND = {"authority", "deviceId", "trIndex", "ok", "rawBefore", "rawAfter"}
READER_OBS = {"obsDevice", "obsTr", "tuning", "powered"}
READER_BITS = {"authority", "ok", "powered"}


def check_readers(readers, errors, chosen, stop_value):
    """A1-S2 (codex #4491): PLAYER_READER PRE(7) POST(10) BACK(17) of the production
    RICON_BaseRadioObservationReader.TryRead after the full target binding; after an abort only those before it (no act
    aborts on 7, 10 or 17). bound=0 carries nothing else; bound=1 adds authority, deviceId, trIndex, ok, rawBefore and
    rawAfter, and ok=1 the observation (device, transceiver index, tuning value, power). Match: bound, authority, ok,
    deviceId = obsDevice = 1, trIndex = obsTr = trNumber - 1 of CHOOSE, rawBefore = tuning = rawAfter = raw0 (PRE, BACK)
    or target (POST). The network key and the position are never logged. Returns (verdict, rows)."""
    want = [p for p in READER_PLAN if stop_value is None or p[0] < stop_value]
    got = [(r["value"], r["f"].get("phase")) for r in readers]
    if got != want:
        errors.append(f"PLAYER_READER {got} != plan {want}")
    rows, mismatches = [], []
    tr_index = int(chosen["trNumber"]) - 1 if INT.fullmatch(str(chosen.get("trNumber", ""))) else None
    for r in readers:
        f = r["f"]
        keys = set(READER_BASE)
        if f.get("bound") == "1":
            keys |= READER_BOUND
            if f.get("ok") == "1":
                keys |= READER_OBS
        if set(f) != keys or f.get("bound") not in BIT or f.get("idx") != str(chosen.get("idx")) \
                or any(f.get(k) not in BIT for k in READER_BITS & keys) \
                or not all(INT.fullmatch(f.get(k, "")) for k in keys - READER_BASE - READER_BITS):
            errors.append(f"PLAYER_READER {r['value']}: fields {sorted(set(f) ^ keys)}")
            continue
        wanted = chosen.get("target") if f.get("phase") == "POST" else chosen.get("raw0")
        match = f["bound"] == "1" and f.get("authority") == "1" and f.get("ok") == "1" and f.get("deviceId") == "1" \
            and f.get("obsDevice") == "1" and tr_index is not None and f.get("trIndex") == str(tr_index) \
            and f.get("obsTr") == str(tr_index) and f.get("rawBefore") == f.get("tuning") == f.get("rawAfter") == str(wanted)
        if not match:
            mismatches.append(r["value"])
        rows.append({"n": r["value"], **{k: f.get(k) for k in ("phase", "bound", "authority", "ok", "deviceId", "trIndex",
                                                               "obsDevice", "obsTr", "rawBefore", "tuning", "rawAfter",
                                                               "powered")}, "wantRaw": wanted, "match": match})
    return ("OBSERVED_MISMATCH" if mismatches else "READER_PASS"), rows


def main(path, expect_entries=False, expect_menu=False, expect_screen=False, expect_step=False):
    raw = open(path, "rb").read()
    lines = raw.decode("utf-8", errors="replace").splitlines()
    errors = []
    if [l for l in lines if re.search(r"SCRIPT\s*\(E\)", l)]:
        errors.append("SCRIPT (E) present")
    events, in_dump, seen = [], False, set()
    for l in lines:
        if "[RICON_LIVE] dump=BEGIN" in l:
            in_dump = True
            continue
        if "[RICON_LIVE] dump=END" in l:
            in_dump = False
            continue
        m = LINE.search(l)
        if in_dump or not m:
            continue
        if m.group(1) in seen:
            errors.append(f"stream seq {m.group(1)} repeated")
        seen.add(m.group(1))
        events.append({"kind": m.group(4), "value": int(m.group(8)), "text": m.group(9), "f": fields(m.group(9))})
    starts = [e for e in events if e["kind"] == "BENCH_START"]
    start_f = starts[0]["f"] if len(starts) == 1 else {}
    if len(starts) != 1:
        errors.append(f"BENCH_START count {len(starts)}")
    elif start_f.get("mode") != ("playermenu" if expect_menu else "playerscreenstep" if expect_step else
                                 "playerscreen" if expect_screen else "player") \
            or start_f.get("modeGiven") != "1":
        errors.append(f"BENCH_START mode={start_f.get('mode')} modeGiven={start_f.get('modeGiven')}")
    not_ready = [e for e in events if e["kind"] == "BENCH_NOT_READY"]
    for e in not_ready:
        errors.append("BENCH_NOT_READY: " + e["text"])
    actions = sorted({e["kind"] for e in events if e["kind"] in FORBIDDEN})
    if actions:
        errors.append(f"not read-only: {actions}")
    samples = [e for e in events if e["kind"] == "PLAYER_SAMPLE"]
    # codex #4402: in the menu mode a STOP aborts the run at its sample (BENCH_ABORTED, never a DONE)
    act_stop = next((e["value"] for e in events if e["kind"] == "PLAYER_ACT" and e["f"].get("op") == "STOP"), None) \
        if expect_menu or expect_step else None
    # A1-S1: a SCREEN op=NOT_READY aborts the run at its sample, like a STOP of the menu mode (A1-S2: either of them)
    screen_cut = next((e["value"] for e in events if e["kind"] == "SCREEN" and e["f"].get("op") == "NOT_READY"), None) \
        if expect_screen else None
    stop_value = min((v for v in (act_stop, screen_cut) if v is not None), default=None)
    last_sample = stop_value if stop_value is not None else SAMPLES
    if [s["value"] for s in samples] != list(range(1, last_sample + 1)):
        errors.append(f"PLAYER_SAMPLE values {[s['value'] for s in samples]} != 1..{last_sample}")
    rows, last_ms = [], -1
    for s in samples:
        errs, prefab = sample_errors(s["f"])
        for x in errs:
            errors.append(f"sample {s['value']}: {x}")
        ms = int(s["f"]["elapsedMs"]) if INT.fullmatch(s["f"].get("elapsedMs", "")) else -1
        # codex #4372, #4379: samples at least 1 s of game time apart; with b + 1 s <= a the truncated ms differ by at
        # least 1000, 999 leaves one millisecond for binary32 rounding of elapsed x 1000
        if ms < 1000 or (last_ms >= 0 and ms - last_ms < 999):
            errors.append(f"sample {s['value']}: elapsedMs {ms} after {last_ms}")
        last_ms = ms
        rows.append({"n": s["value"], "pc": s["f"].get("pc"), "playerId": s["f"].get("playerId"),
                     "controlled": s["f"].get("controlled"), "prefab": prefab, "players": s["f"].get("players"),
                     "vonController": s["f"].get("vonController"), "vonEntries": s["f"].get("vonEntries"),
                     "entityVoN": s["f"].get("entityVoN"), "gadgetManager": s["f"].get("gadgetManager"),
                     "isServer": s["f"].get("isServer"), "isClient": s["f"].get("isClient")})
    # A1-P1: PLAYER_ENTRY records follow their sample in the stream, value = sample number, idx 0..vonEntries-1;
    # samples without a VON controller have none. Without the expectation a log with entries is an error, and
    # vice versa (an A1-P0 log has none).
    entries, current, by_sample, acts, screens, readers = [], None, {}, [], [], []
    for e in events:
        if e["kind"] == "PLAYER_SAMPLE":
            current = e
            by_sample[e["value"]] = []
        elif e["kind"] == "PLAYER_READER":
            # A1-S2: a production-reader record follows its sample
            if current is None or e["value"] != current["value"]:
                errors.append(f"PLAYER_READER value {e['value']} not after its sample")
            readers.append(e)
        elif e["kind"] == "SCREEN":
            if current is None or e["value"] != current["value"]:
                errors.append(f"SCREEN value {e['value']} not after its sample")
            screens.append(e)
        elif e["kind"] == "PLAYER_ACT":
            # A1-P2: an action record follows its sample (after that sample's entries)
            if current is None or e["value"] != current["value"]:
                errors.append(f"PLAYER_ACT value {e['value']} not after its sample")
            acts.append(e)
        elif e["kind"] == "PLAYER_ENTRY":
            if current is None or e["value"] != current["value"]:
                errors.append(f"PLAYER_ENTRY value {e['value']} not after its sample")
                continue
            errs, out = entry_errors(e["f"])
            for x in errs:
                errors.append(f"sample {e['value']} entry {e['f'].get('idx')}: {x}")
            by_sample[e["value"]].append(out)
            entries.append(out)
    if entries and not expect_entries:
        errors.append("log has PLAYER_ENTRY: pass the expectation 'entries'")
    if expect_entries:
        for s in samples:
            n, f = s["value"], s["f"]
            want = int(f["vonEntries"]) if f.get("pc") == "1" and f.get("vonController") == "1" \
                and INT.fullmatch(f.get("vonEntries", "")) else 0
            got = [x.get("idx") for x in by_sample.get(n, [])]
            if got != [str(i) for i in range(want)]:
                errors.append(f"sample {n}: entries idx {got} != 0..{want - 1}")
        if not entries:
            errors.append("expected PLAYER_ENTRY records, none found")
    entry_states = [json.dumps(by_sample.get(r["n"], []), sort_keys=True) for r in rows]
    entry_changes = [rows[i]["n"] for i in range(len(rows)) if i == 0 or entry_states[i] != entry_states[i - 1]]
    if acts and not (expect_menu or expect_step):
        errors.append("log has PLAYER_ACT: pass the expectation 'menu' or 'screenstep'")
    menu_verdict, act_rows, chosen = (check_acts(acts, errors, by_sample) if expect_menu else
                                      check_acts(acts, errors, by_sample, STEP_SPEC, screen_cut) if expect_step else
                                      (None, [], {}))
    # A1-P2: the entry state after FINAL (sample 21) must equal the state before CHOOSE (sample 4): restored exactly
    restored = None
    if (expect_menu or expect_step) and menu_verdict == "PLAYER_MENU_RUNTIME_PASS":
        restored = json.dumps(by_sample.get(4), sort_keys=True) == json.dumps(by_sample.get(21), sort_keys=True)
        if not restored:
            menu_verdict = "OBSERVED_MISMATCH"
    if screens and not expect_screen:
        errors.append("log has SCREEN: pass the expectation 'screen'")
    screen_verdict, screen_summary = (
        check_screen(screens, by_sample, errors, STEP_SCREEN_SPEC, chosen, stop_value or SAMPLES) if expect_step else
        check_screen(screens, by_sample, errors) if expect_screen else (None, {}))
    if readers and not expect_step:
        errors.append("log has PLAYER_READER: pass the expectation 'screenstep'")
    reader_verdict, reader_rows = (check_readers(readers, errors, chosen, stop_value) if expect_step else (None, []))
    level = "PLAYER_MENU_RUNTIME" if expect_menu else "PLAYER_SCREEN_STEP" if expect_step else \
        "PLAYER_SCREEN_WIDGET" if expect_screen else "PLAYER_OBSERVATION"
    done = [e for e in events if e["kind"] == "BENCH_DONE"]
    aborted = [e for e in events if e["kind"] == "BENCH_ABORTED"]
    if stop_value is not None:
        if done or len(aborted) != 1 or events[-1]["kind"] != "BENCH_ABORTED" or aborted[0]["value"] != stop_value \
                or aborted[0]["f"].get("level") != level \
                or (expect_step and (aborted[0]["f"].get("notReady") != ("1" if screen_cut is not None else "0")
                                     or aborted[0]["f"].get("stopped") != ("1" if act_stop is not None else "0"))) \
                or (not expect_step and aborted[0]["f"].get("notReady" if expect_screen else "stopped") != "1"):
            errors.append(f"after STOP/NOT_READY the single last event must be BENCH_ABORTED value={stop_value} level={level}")
    else:
        if aborted:
            errors.append("BENCH_ABORTED present")
        if len(done) != 1 or done[0]["value"] != SAMPLES or done[0]["f"].get("level") != level \
                or not events or events[-1]["kind"] != "BENCH_DONE":
            errors.append(f"BENCH_DONE must be the single last event with value 30 and level={level}")
    # codex #4372: a change of playerId (or of the player count / entries) with the same prefab is a state change too
    states = [tuple(r[k] for k in ("pc", "playerId", "controlled", "prefab", "players", "vonController", "vonEntries",
                                   "entityVoN", "gadgetManager", "isServer", "isClient")) for r in rows]
    changes = [rows[i]["n"] for i in range(len(rows)) if i == 0 or states[i] != states[i - 1]]
    if not rows:
        finding = None
    elif all(r["pc"] == "0" for r in rows):
        finding = "LOCAL_PLAYER_ABSENT"
    elif all(r["pc"] == "1" for r in rows) and len(changes) == 1:
        finding = "LOCAL_PLAYER_PRESENT"
    else:
        finding = "LOCAL_PLAYER_CHANGED"
    if not_ready:
        overall = "NOT_READY"
    elif errors:
        overall = "LOG_INVALID"
    elif expect_menu:
        overall = menu_verdict
    elif expect_step:
        # A1-S2: an abort decides first, then any disagreement, then the label rule of A1-S1
        verdicts = (menu_verdict, screen_verdict, reader_verdict)
        overall = ("NOT_READY" if "NOT_READY" in verdicts else "STOPPED" if "STOPPED" in verdicts else
                   "OBSERVED_MISMATCH" if "OBSERVED_MISMATCH" in verdicts or menu_verdict != "PLAYER_MENU_RUNTIME_PASS" else
                   "PLAYER_SCREEN_STEP_LABEL_UNVERIFIED" if screen_verdict == "PLAYER_SCREEN_WIDGET_LABEL_UNVERIFIED" else
                   "PLAYER_SCREEN_STEP_PASS")
    elif expect_screen:
        overall = screen_verdict
    else:
        overall = "PLAYER_OBSERVATION_COMPLETE"
    print(json.dumps({"logSha256": hashlib.sha256(raw).hexdigest(), "errors": errors, "overall": overall,
                      "menuExpected": expect_menu, "acts": act_rows, "restoredEntriesEqual": restored,
                      "screenExpected": expect_screen, "screen": screen_summary,
                      "stepExpected": expect_step, "readers": reader_rows,
                      "finding": finding, "stateChangesAtSample": changes,
                      "entriesExpected": expect_entries, "entryStateChangesAtSample": entry_changes if entries else [],
                      "entriesFirstSample": by_sample.get(1, []), "entriesLastSample": by_sample.get(SAMPLES, []),
                      "start": {k: start_f.get(k) for k in ("isServer", "isClient", "playerController", "factionManager",
                                                            "mode", "modeGiven", "radioManager")},
                      "samples": rows}, ensure_ascii=False, indent=1))
    print("LIVE_PLAYER_LOG_CHECK_" + ("PASS" if not errors else "FAIL"))
    return 1 if errors else 0


if __name__ == "__main__":
    if len(sys.argv) > 2 and sys.argv[2] not in ("entries", "menu", "screen", "screenstep"):
        print(f"second argument must be 'entries', 'menu', 'screen' or 'screenstep', got {sys.argv[2]!r}")
        sys.exit(2)
    # 'menu' (A1-P2) and 'screen' (A1-S1) imply the entries of A1-P1
    arg = sys.argv[2] if len(sys.argv) > 2 else ""
    sys.exit(main(sys.argv[1], bool(arg), arg == "menu", arg in ("screen", "screenstep"), arg == "screenstep"))
