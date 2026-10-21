#!/usr/bin/env python3
"""Эталон формирования снимков S3b по REQ-link-publication. Только stdlib и принятые контракты S3."""
from __future__ import annotations

import argparse
import copy
import json
import math
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
import pair_service_oracle as ps

MAX = 2**31 - 1
STATES = ("LOST", "WEAK", "DEGRADED", "CLEAN")
REASONS = ("OK", "INVALID_COORDINATE", "INVALID_FREQUENCY", "INVALID_BANDWIDTH", "INVALID_GAIN",
           "INVALID_NOISE_FIGURE", "INVALID_SENSITIVITY", "INVALID_REQUIRED_SINR", "INVALID_INTERFERENCE",
           "CATALOG_VIOLATION", "UNKNOWN_BAND", "UNKNOWN_MODEL", "UNKNOWN_NOISE_ENVIRONMENT",
           "OUT_OF_MODEL_DOMAIN", "NUMERIC_FAILURE", "MODEL_CONTRACT_VIOLATION", "TERRAIN_PROFILE_UNAVAILABLE")
DEVICE_CODES = ("NULL_INPUT", "INVALID_OBSERVATION", "INVALID_PROFILE", "INVALID_CONTEXT", "PROFILE_MISMATCH",
                "POWER_OFF", "TUNING_MISMATCH", "CHANNEL_MISMATCH", "KEY_MISMATCH", "READY")
CALL_CODES = ("NULL_INPUT", "INVALID_LIMITS", "LIMIT_EXCEEDED", "INVALID_ELEMENT", "INVALID_REGISTRY",
              "DUPLICATE", "UPSTREAM_REFUSED", "IDENTITY_MISMATCH", "CLOCK_ROLLBACK", "ACTIVE_LIMIT",
              "ID_EXHAUSTED", "NEED_FULL_RESYNC", "OK")
KEY = ("leaseId", "rxDeviceId", "rxTransceiverIndex", "rxPlayerId")
TX = ("txPlayerId", "txDeviceId", "txTransceiverIndex")
PAIR_DOMAIN = {"leaseId": (1, MAX), "txDeviceId": (1, MAX-1), "rxPlayerId": (1, MAX),
               "rxDeviceId": (1, MAX-1), "rxTransceiverIndex": (0, MAX-1)}
ENTRY_DOMAIN = {k: v for k, v in PAIR_DOMAIN.items() if k != "txDeviceId"} | {
    "txPlayerId": (1, MAX), "txDeviceId": (1, MAX-1), "txTransceiverIndex": (0, MAX-1),
    "linkId": (1, MAX), "linkSeq": (1, MAX-1), "ttlMs": (1, MAX), "publishedAtMs": (0, MAX)}


def finite(value):
    return type(value) in (int, float) and math.isfinite(value) and abs(value) <= 3.4028234663852886e38


def key(row):
    return tuple(row[k] for k in KEY)


def decode(value):
    if isinstance(value, dict):
        return {k: decode(v) for k, v in value.items()}
    if isinstance(value, list):
        return [decode(v) for v in value]
    if value == "NaN":
        return math.nan
    if value == "Infinity":
        return math.inf
    if value == "-Infinity":
        return -math.inf
    return value


def encode(value):
    if isinstance(value, dict):
        return {k: encode(v) for k, v in value.items()}
    if isinstance(value, list):
        return [encode(v) for v in value]
    if type(value) is float and not math.isfinite(value):
        return "NaN" if math.isnan(value) else "Infinity" if value > 0 else "-Infinity"
    return value


def pair_valid(p):
    if not ps.fields_ok(p, PAIR_DOMAIN, ("pairCode", "hasDeviceCode", "deviceCode", "hasBudget", "valid",
                                        "reason", "state", "intelligibility", "sinrDb")):
        return False
    if p["pairCode"] not in ps.PAIR_CODES or p["deviceCode"] not in DEVICE_CODES or p["reason"] not in REASONS \
            or p["state"] not in STATES or not all(type(p[x]) is bool for x in ("hasDeviceCode", "hasBudget", "valid")) \
            or not finite(p["intelligibility"]) or not 0 <= p["intelligibility"] <= 1 or not finite(p["sinrDb"]):
        return False
    budget = p["pairCode"] == "EVALUATED"
    device = p["pairCode"] in ("DEVICE_QUERY", "EVALUATED")
    if p["hasBudget"] != budget or p["hasDeviceCode"] != device:
        return False
    if (budget and p["deviceCode"] != "READY") or (p["pairCode"] == "DEVICE_QUERY" and p["deviceCode"] == "READY") \
            or (not device and p["deviceCode"] != "NULL_INPUT"):
        return False
    if not budget:
        return (p["valid"], p["reason"], p["state"], p["intelligibility"], p["sinrDb"]) == \
               (False, "NUMERIC_FAILURE", "LOST", 0, 0)
    if p["valid"] != (p["reason"] == "OK") or (p["state"] == "LOST" and p["intelligibility"] != 0):
        return False
    return p["valid"] or (p["state"], p["intelligibility"], p["sinrDb"]) == ("LOST", 0, 0)


def registry_valid(r):
    if type(r) is not dict or set(r) != {"sessionEpoch", "highWaterLinkId", "lastTickMs", "needsFullResync", "entries"} \
            or not ps.int_in(r["sessionEpoch"], 1, MAX-1) or not ps.int_in(r["highWaterLinkId"], 0, MAX) \
            or not ps.int_in(r["lastTickMs"], -1, MAX) or type(r["needsFullResync"]) is not bool:
        return False
    for e in r["entries"]:
        if not ps.fields_ok(e, ENTRY_DOMAIN, ("state", "quality")) or e["state"] not in STATES \
                or not finite(e["quality"]) or not 0 <= e["quality"] <= 1 \
                or (e["state"] == "LOST" and e["quality"] != 0) \
                or e["linkId"] > r["highWaterLinkId"] or e["publishedAtMs"] > r["lastTickMs"]:
            return False
    return r["lastTickMs"] != -1 or (not r["entries"] and r["needsFullResync"])


def classify(inp, reg):
    """REQ-LP-05: локальный план, без id/seq и без изменения входов."""
    bindings = {b["leaseId"]: b for b in inp["bindings"]}
    pairs = {key(p): p for p in inp["pairResult"]["pairs"]}
    old = {key(e): e for e in reg["entries"]} if reg["sessionEpoch"] == inp["sessionEpoch"] else {}
    plan = []
    for k in sorted(pairs.keys() | old.keys()):
        p, e = pairs.get(k), old.get(k)
        valid = p is not None and p["pairCode"] == "EVALUATED" and p["valid"]
        terminal = p is None or p["pairCode"] in ps.PAIR_CODES[:6]
        if e is None and not valid:
            continue
        target = {x: v for x, v in zip(KEY, k)} | {x: bindings[k[0]][x] for x in TX} if e is None else copy.deepcopy(e)
        target["state"] = p["state"] if valid else "LOST"
        target["quality"] = p["intelligibility"] if valid else 0.0
        due = e is None or terminal or reg["needsFullResync"] or target["state"] != e["state"] \
            or abs(target["quality"] - e["quality"]) >= inp["qualityDelta"] or inp["ttlMs"] != e["ttlMs"] \
            or inp["nowMs"] - e["publishedAtMs"] >= inp["ttlMs"] // 2 + inp["ttlMs"] % 2
        plan.append((target, e, terminal, due))
    return plan


def preflight(inp, reg, has_source):
    if inp is None or reg is None or not has_source or any(inp.get(k) is None for k in ("limits", "bindings", "pairResult")) \
            or inp["pairResult"].get("pairs") is None or reg.get("entries") is None:
        return "NULL_INPUT"
    limits = inp["limits"]
    if type(limits) is not dict or set(limits) != {"maxBindings", "maxPairs", "maxActiveLinks"} \
            or not ps.int_in(limits["maxBindings"], 1, 4096) \
            or not all(ps.int_in(limits[k], 1, 65536) for k in ("maxPairs", "maxActiveLinks")):
        return "INVALID_LIMITS"
    arrays = (inp["bindings"], inp["pairResult"]["pairs"], reg["entries"])
    if any(type(a) is not list for a in arrays):
        return "INVALID_ELEMENT"
    if any(len(a) > limits[k] for a, k in zip(arrays, ("maxBindings", "maxPairs", "maxActiveLinks"))):
        return "LIMIT_EXCEEDED"
    result = inp["pairResult"]
    if not ps.int_in(inp.get("nowMs"), 0, MAX) or not ps.int_in(inp.get("sessionEpoch"), 1, MAX-1) \
            or not ps.int_in(inp.get("ttlMs"), 1, MAX) or not finite(inp.get("qualityDelta")) \
            or not 0 < inp["qualityDelta"] <= 1 or result.get("callCode") not in ps.CALL_CODES \
            or not all(ps.int_in(result.get(k), 0, MAX) for k in ("s2Calls", "evaluatorCalls")) \
            or not all(ps.fields_ok(b, ps.BINDING, ("active",)) and type(b["active"]) is bool for b in arrays[0]) \
            or not all(pair_valid(p) for p in arrays[1]):
        return "INVALID_ELEMENT"
    if not registry_valid(reg):
        return "INVALID_REGISTRY"
    if any(len(values) != len(set(values)) for values in (
            [b["leaseId"] for b in arrays[0]], [b["txDeviceId"] for b in arrays[0]],
            [key(p) for p in arrays[1]], [key(e) for e in arrays[2]], [e["linkId"] for e in arrays[2]])):
        return "DUPLICATE"
    if result["callCode"] != "OK":
        return "UPSTREAM_REFUSED"
    bm = {b["leaseId"]: b for b in arrays[0]}
    for p in arrays[1]:
        b = bm.get(p["leaseId"])
        if b is None or b["txDeviceId"] != p["txDeviceId"]:
            return "IDENTITY_MISMATCH"
    if reg["sessionEpoch"] == inp["sessionEpoch"]:
        pm = {key(p): p for p in arrays[1]}
        for e in arrays[2]:
            if key(e) in pm and any(e[x] != bm[e["leaseId"]][x] for x in TX):
                return "IDENTITY_MISMATCH"
        if reg["lastTickMs"] >= 0 and inp["nowMs"] < reg["lastTickMs"]:
            return "CLOCK_ROLLBACK"
    plan = classify(inp, reg)
    if sum(not terminal for _, _, terminal, _ in plan) > limits["maxActiveLinks"]:
        return "ACTIVE_LIMIT"
    high = reg["highWaterLinkId"] if reg["sessionEpoch"] == inp["sessionEpoch"] else 0
    if sum(e is None for _, e, _, _ in plan) > MAX - high:
        return "ID_EXHAUSTED"
    return "OK"


class SequenceSource:
    """TEST_ONLY source: изменяется только fake; счётчики учитывают реальные вызовы методов."""
    def __init__(self, spec):
        self.spec = copy.deepcopy(spec)
        self.epoch = spec["epoch"]
        self.sequences = dict(spec.get("sequences", {}))
        self.calls = 0
        self.reads = 0

    def get_epoch(self):
        self.reads += 1
        return self.epoch

    def next(self, link_id):
        self.calls += 1
        if self.calls == self.spec.get("failAt", 0):
            return False, 0, False
        current = self.sequences.get(str(link_id), 0)
        rotated = self.calls == self.spec.get("rotateAt", 0) or current >= MAX-1
        if rotated:
            self.epoch = self.spec.get("rotatedEpoch", 1 if self.epoch >= MAX-1 else self.epoch + 1)
            self.sequences.clear()
            current = 0
        seq = current + 1
        if self.calls == self.spec.get("invalidAt", 0):
            seq = self.spec.get("invalidSeq", MAX)
        self.sequences[str(link_id)] = seq
        return True, seq, rotated

    def state(self):
        return {"epoch": self.epoch, "sequences": dict(sorted(self.sequences.items(), key=lambda x: int(x[0])))}


def empty_registry(epoch=7, high=0):
    return {"sessionEpoch": epoch, "highWaterLinkId": high, "lastTickMs": -1,
            "needsFullResync": True, "entries": []}


def publish(inp, reg, source):
    calls_before, reads_before = (source.calls, source.reads) if source is not None else (0, 0)
    code = preflight(inp, reg, source is not None)
    out = {"callCode": code, "snapshots": [], "registry": copy.deepcopy(reg), "sequenceCalls": 0,
           "epochReads": 0, "requiresFullResync": False}
    if code != "OK":
        return out

    def reset(epoch, high):
        target_epoch = epoch if ps.int_in(epoch, 1, MAX-1) else inp["sessionEpoch"]
        out.update(callCode="NEED_FULL_RESYNC", snapshots=[], requiresFullResync=True,
                   sequenceCalls=source.calls-calls_before, epochReads=source.reads-reads_before,
                   registry=empty_registry(target_epoch, high if target_epoch == reg["sessionEpoch"] else 0))
        return out

    epoch = source.get_epoch()
    if epoch != inp["sessionEpoch"] or reg["sessionEpoch"] != inp["sessionEpoch"]:
        return reset(epoch, reg["highWaterLinkId"])
    plan = classify(inp, reg)
    high = reg["highWaterLinkId"]
    for target, old, _, _ in plan:
        if old is None:
            high += 1
            target["linkId"] = high
    entries = []
    for target, old, terminal, due in plan:
        if due:
            ok, seq, rotated = source.next(target["linkId"])
            epoch = source.get_epoch()
            if not ok or rotated or epoch != inp["sessionEpoch"] or not ps.int_in(seq, 1, MAX-1) \
                    or (old is not None and seq <= old["linkSeq"]):
                return reset(epoch, high)
            target.update(linkSeq=seq, ttlMs=inp["ttlMs"], publishedAtMs=inp["nowMs"])
            out["snapshots"].append({"schemaVersion": 1, "sessionEpoch": inp["sessionEpoch"],
                "linkId": target["linkId"], "linkSeq": seq, "ttlMs": inp["ttlMs"],
                "routeId": target["linkId"], "routeNodeCount": 2, "talkerPlayerId": target["txPlayerId"],
                "listenerPlayerId": target["rxPlayerId"], "state": target["state"], "quality": target["quality"]})
        if not terminal:
            entries.append(copy.deepcopy(target if due else old))
    out.update(registry={"sessionEpoch": inp["sessionEpoch"], "highWaterLinkId": high,
                        "lastTickMs": inp["nowMs"], "needsFullResync": False, "entries": entries},
               requiresFullResync=reg["needsFullResync"], sequenceCalls=source.calls-calls_before,
               epochReads=source.reads-reads_before)
    return out


def evaluate(inp, reg, spec):
    source = SequenceSource(spec) if spec is not None else None
    out = publish(decode(copy.deepcopy(inp)), decode(copy.deepcopy(reg)), source)
    out["sourceState"] = source.state() if source else None
    return encode(out)


def binding(lease=11, tx=101, player=1, index=0):
    return {"leaseId": lease, "sessionEpoch": 7, "txPlayerId": player, "txGeneration": 1,
            "txDeviceId": tx, "txTransceiverIndex": index, "networkTypeId": 1, "channelId": 1,
            "active": True, "issuedAtMs": 0, "ttlMs": 10000}


def pair(lease=11, tx=101, rx=202, player=2, index=0, code="EVALUATED", quality=0.75, state="CLEAN", valid=True):
    budget = code == "EVALUATED"
    return {"leaseId": lease, "txDeviceId": tx, "rxPlayerId": player, "rxDeviceId": rx,
            "rxTransceiverIndex": index, "pairCode": code, "hasDeviceCode": code in ("DEVICE_QUERY", "EVALUATED"),
            "deviceCode": "READY" if budget else "POWER_OFF" if code == "DEVICE_QUERY" else "NULL_INPUT",
            "hasBudget": budget, "valid": budget and valid, "reason": "OK" if budget and valid else "NUMERIC_FAILURE",
            "state": state if budget and valid else "LOST", "intelligibility": quality if budget and valid else 0.0,
            "sinrDb": 16.0 if budget and valid else 0.0}


def baseline():
    inp = {"nowMs": 100, "sessionEpoch": 7, "ttlMs": 1000, "qualityDelta": 0.125,
           "limits": {"maxBindings": 8, "maxPairs": 32, "maxActiveLinks": 16}, "bindings": [binding()],
           "pairResult": {"callCode": "OK", "s2Calls": 1, "evaluatorCalls": 1, "pairs": [pair()]}}
    return inp, empty_registry(), {"epoch": 7, "sequences": {}}


def established():
    inp, reg, spec = baseline()
    out = evaluate(inp, reg, spec)
    inp["nowMs"] = 101
    return inp, out["registry"], {"epoch": 7, "sequences": {"1": 1}}


def build_cases():
    cases = []

    def add(ident, title, triple):
        inp, reg, source = copy.deepcopy(triple)
        cases.append({"id": ident, "title": title, "input": inp, "registry": reg, "source": source,
                      "expect": evaluate(inp, reg, source)})

    add("P01", "Первый снимок, полный resync", baseline())
    t = baseline(); t[0]["pairResult"]["pairs"] = []; add("P02", "Пустой первый такт устанавливает identity", t)
    add("P03", "Повтор без изменений, seq не запрашивается", established())
    t = baseline(); t[0]["pairResult"]["pairs"] += [pair(rx=203, player=3), pair(index=1)]
    add("P04", "Два owner и два индекса одной рации", t)
    t[0]["pairResult"]["pairs"].reverse(); add("P05", "Та же сцена с перестановкой", t)
    t = established(); t[0]["pairResult"]["pairs"][0]["intelligibility"] = 0.6875
    add("Q01", "Изменение Q меньше порога", t)
    t[0]["pairResult"]["pairs"][0]["intelligibility"] = 0.625; add("Q02", "Равенство порогу Q", t)
    t = established(); t[0]["pairResult"]["pairs"][0]["state"] = "DEGRADED"; add("Q03", "Смена state", t)
    t = established(); t[0]["ttlMs"] = 999; add("Q04", "Смена TTL", t)
    for ident, now in (("T01", 599), ("T02", 600)):
        t = established(); t[0]["nowMs"] = now; add(ident, "Граница половины TTL", t)
    for ident, now in (("T03", 600), ("T04", 601)):
        t = established(); t[0]["ttlMs"] = t[1]["entries"][0]["ttlMs"] = 1001
        t[0]["nowMs"] = now; add(ident, "Нечётный TTL округляется вверх", t)
    for ident, ttl, now, at in (("T05", 1, 101, 100), ("T06", MAX, MAX-1, MAX-2),
                                ("T07", MAX, 1073741824, 0)):
        t = established(); t[0]["ttlMs"] = t[1]["entries"][0]["ttlMs"] = ttl
        t[1]["entries"][0]["publishedAtMs"] = t[1]["lastTickMs"] = at
        t[0]["nowMs"] = now; add(ident, "TTL 1/MAX без сложения времени", t)
    for i, code in enumerate(ps.PAIR_CODES, 1):
        for old in (False, True):
            t = established() if old else baseline()
            t[0]["pairResult"]["pairs"] = [pair(code=code)]
            add(f"B{i:02}{int(old)}", "Код пары со старой связью/без неё: " + code, t)
    for ident, old in (("B101", True), ("B100", False)):
        t = established() if old else baseline(); t[0]["pairResult"]["pairs"] = [pair(valid=False)]
        add(ident, "Невалидный budget", t)
    t = baseline(); t[0]["pairResult"]["pairs"] = [pair(quality=0.0, state="LOST")]
    add("B11", "Валидный LOST бюджет создаёт связь", t)
    t = established(); t[0]["bindings"] = []; t[0]["pairResult"]["pairs"] = []
    add("L01", "Связь и binding пропали, завершающий LOST", t)
    t = established(); t[0]["bindings"] = [binding(lease=12)]; t[0]["pairResult"]["pairs"] = [pair(lease=12)]
    add("L02", "Новое нажатие PTT: старую связь завершить, новую создать", t)
    t = established(); t[1]["entries"][0].update(state="LOST", quality=0.0)
    t[0]["pairResult"]["pairs"] = []; add("L03", "Завершить даже уже LOST", t)
    t = established(); t[1]["needsFullResync"] = True; add("L04", "Принудительный полный снимок", t)
    t = baseline(); t[1]["highWaterLinkId"] = 50; add("L05", "Удалённые id не используются повторно", t)
    null_targets = (("input",), ("registry",), ("source",), ("input", "limits"),
                    ("input", "bindings"), ("input", "pairResult"), ("input", "pairResult", "pairs"),
                    ("registry", "entries"))
    for i, path in enumerate(null_targets, 1):
        t = baseline(); data = dict(zip(("input", "registry", "source"), t)); obj = data
        for k in path[:-1]: obj = obj[k]
        obj[path[-1]] = None
        add(f"C{i:02}", "Null граница " + ".".join(path), tuple(data.values()))
    for ident, field, value in (("C09", "maxBindings", 0), ("C10", "maxPairs", 65537),
                                ("C11", "maxActiveLinks", 0)):
        t = baseline(); t[0]["limits"][field] = value; add(ident, "Неверный лимит", t)
    t = baseline(); t[0]["limits"]["maxPairs"] = 1; t[0]["pairResult"]["pairs"] *= 2
    add("C12", "Размер проверяется раньше duplicate", t)
    for i, (field, value) in enumerate((("nowMs", -1), ("sessionEpoch", MAX), ("ttlMs", 0),
                                       ("qualityDelta", 0.0), ("qualityDelta", "NaN")), 13):
        t = baseline(); t[0][field] = value; add(f"C{i:02}", "Неверное поле input", t)
    t = baseline(); t[0]["bindings"][0]["active"] = 1; add("C18", "Bool строгий", t)
    t = baseline(); t[0]["pairResult"]["pairs"][0]["hasBudget"] = False; add("C19", "Budget flag несогласован", t)
    t = baseline(); t[0]["pairResult"]["pairs"][0]["intelligibility"] = "Infinity"; add("C20", "Нечисловой Q", t)
    t = established(); t[1]["entries"][0]["linkId"] = 2; add("C21", "Id больше highWater", t)
    t = established(); t[1]["entries"][0]["publishedAtMs"] = 102; add("C22", "Дата выдачи в будущем реестра", t)
    t = baseline(); t[0]["bindings"] *= 2; add("C23", "Duplicate binding", t)
    t = baseline(); t[0]["pairResult"]["pairs"] *= 2; add("C24", "Duplicate pair", t)
    t = established(); t[1]["entries"] *= 2; add("C25", "Duplicate registry", t)
    t = baseline(); t[0]["pairResult"]["callCode"] = "INVALID_ELEMENT"; add("C26", "S3 отказал целиком", t)
    t = baseline(); t[0]["bindings"] = []; add("C27", "Нет binding по lease", t)
    t = baseline(); t[0]["bindings"][0]["txDeviceId"] = 102; add("C28", "TX device не совпал", t)
    t = established(); t[0]["bindings"][0]["txPlayerId"] = 8; add("C29", "Подмена talker у старого ключа", t)
    t = established(); t[0]["nowMs"] = 99; add("C30", "Откат монотонного времени", t)
    t = baseline(); t[0]["limits"]["maxActiveLinks"] = 1; t[0]["pairResult"]["pairs"] += [pair(index=1)]
    add("C31", "Будущий active-limit", t)
    t = baseline(); t[1]["highWaterLinkId"] = MAX; add("C32", "Id исчерпаны", t)
    t = baseline(); t[0]["bindings"] = [None]; add("C33", "Null binding", t)
    t = baseline(); t[0]["pairResult"]["pairs"] = [None]; add("C34", "Null pair", t)
    t = established(); t[1]["entries"] = [None]; add("C35", "Null registry entry", t)
    t = baseline(); t[1]["needsFullResync"] = False; add("C36", "Начальный реестр без full resync недопустим", t)
    t = baseline(); t[0]["pairResult"]["pairs"][0]["deviceCode"] = "UNKNOWN"; add("C37", "Неизвестный код S2", t)
    t = established(); t[1]["entries"][0].update(state="LOST", quality=0.25)
    add("C38", "LOST реестра обязан иметь Q ноль", t)
    t = baseline(); t[0]["limits"]["maxBindings"] = 1; t[0]["bindings"] *= 2
    add("C39", "Размер bindings до duplicate", t)
    t = established(); t[0]["limits"]["maxActiveLinks"] = 1; t[1]["entries"] *= 2
    add("C40", "Размер registry до duplicate", t)
    t = established(); t[1]["entries"] += [copy.deepcopy(t[1]["entries"][0])]
    t[1]["entries"][1]["rxTransceiverIndex"] = 1; add("C41", "Повтор linkId у разных ключей", t)
    t = baseline(); t[0]["bindings"] += [binding(lease=12)]; add("C42", "Повтор txDevice у разных lease", t)
    t = baseline(); t[0]["pairResult"]["callCode"] = "INVALID_ELEMENT"
    t[0]["bindings"] *= 2; add("C43", "Duplicate важнее upstream refusal", t)
    t = baseline(); t[0]["limits"]["maxPairs"] = 0; t[0]["qualityDelta"] = "NaN"
    add("C44", "Invalid limits важнее invalid input", t)
    t = baseline(); t[0]["pairResult"]["pairs"][0]["sinrDb"] = 1e40
    add("C45", "Конечное Python число вне float32", t)
    t = baseline(); t[1]["highWaterLinkId"] = MAX-1; add("E01", "Последний linkId MAX допустим", t)
    t = established(); t[2]["epoch"] = 8; add("E02", "Источник уже сменил эпоху", t)
    t = established(); t[0]["sessionEpoch"] = t[2]["epoch"] = 8; t[0]["nowMs"] = 0
    add("E03", "Внешняя смена эпохи допускает новое время", t)
    t = baseline(); t[2]["epoch"] = 0; add("E04", "Неавторитетный/null adapter", t)
    t = baseline(); t[0]["pairResult"]["pairs"] += [pair(index=1)]; t[2]["rotateAt"] = 2
    add("E05", "Ротация после готового первого снимка: весь output пуст", t)
    t = baseline(); t[0]["pairResult"]["pairs"] += [pair(index=1)]; t[2]["failAt"] = 2
    add("E06", "Отказ второго seq сохраняет consumed highWater", t)
    for ident, seq in (("E07", 0), ("E08", MAX)):
        t = baseline(); t[2].update(invalidAt=1, invalidSeq=seq); add(ident, "Невалидный seq", t)
    t = established(); t[0]["nowMs"] = 600; t[2]["sequences"] = {}
    add("E09", "Seq не больше последнего выданного", t)
    t = established(); t[0]["nowMs"] = 600; t[2]["sequences"] = {"1": MAX-2}
    add("E10", "Последний seq MAX-1", t)
    t[2]["sequences"] = {"1": MAX-1}; add("E11", "Seq MAX вызвал настоящую ротацию fake", t)
    t = baseline(); t[0]["limits"] = {"maxBindings": 4096, "maxPairs": 65536, "maxActiveLinks": 65536}
    add("E12", "Верхние значения лимитов", t)
    t = baseline(); t[2].update(rotateAt=1, rotatedEpoch=0); add("E13", "Невалидная эпоха после rotation", t)
    t = baseline(); t[2].update(rotateAt=1, rotatedEpoch=7); add("E14", "rotated с прежней epoch всё равно отказ", t)
    t = established(); t[1]["needsFullResync"] = True; t[0]["pairResult"]["pairs"] = []
    add("E15", "Full-партия содержит завершающий, receiver проецирует по registry", t)
    return cases


def document():
    return {"schema": "ricon.link_publication", "version": 1, "revision": 1,
            "status": "DRAFT", "source": "docs/req/REQ-link-publication.md v0.1",
            "note": "TEST_ONLY: формирование данных, без сети/мира/звука. Независимый пересчёт и freeze ещё не выполнены.",
            "enforceTolerance": {"quality": 1e-6}, "cases": build_cases()}


def check(data):
    return ps.compare(data, document())


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("vectors", type=Path)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--check", action="store_true")
    mode.add_argument("--write", action="store_true")
    args = parser.parse_args(argv)
    if args.write:
        with args.vectors.open("w", encoding="utf-8", newline="\n") as handle:
            handle.write(json.dumps(document(), ensure_ascii=False, indent=1, allow_nan=False) + "\n")
        print(f"LINK_PUBLICATION_ORACLE_WRITTEN: {len(document()['cases'])} случаев")
        return 0
    errors = check(json.loads(args.vectors.read_text(encoding="utf-8")))
    if errors:
        print("LINK_PUBLICATION_ORACLE_FAIL")
        print("\n".join(errors[:20]))
        return 1
    print(f"LINK_PUBLICATION_ORACLE_PASS: {len(document()['cases'])} случаев")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
