#!/usr/bin/env python3
"""Эталон S3 по REQ-pair-service v0.2: отбор пар, сборка S2 и бюджет свободного пространства. Без сети и звука."""
from __future__ import annotations

import argparse
import copy
import json
import math
from pathlib import Path
import random
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))  # own directory only
import radio_device_oracle as rd  # noqa: E402
import ricon_oracle as ro  # noqa: E402

INT_MAX = 2**31 - 1
STATES = ("NONE", "CONNECTED_UNREGISTERED", "REGISTERED_UNBOUND", "BOUND_ACTIVE", "BOUND_INACTIVE", "DISCONNECTED")
KINDS = ("CARRIED", "WORLD")
LIMIT_KEYS = ("maxBindings", "maxListeners", "maxPlayers", "maxDevices", "maxEndpoints")
ARRAYS = (("bindings", "maxBindings"), ("listeners", "maxListeners"), ("players", "maxPlayers"),
          ("devices", "maxDevices"), ("endpoints", "maxEndpoints"))
CALL_CODES = ("OK", "NULL_INPUT", "INVALID_LIMITS", "LIMIT_EXCEEDED", "INVALID_ELEMENT", "DUPLICATE")
PAIR_CODES = ("BINDING_INACTIVE", "TX_PLAYER_INVALID", "TX_DEVICE_INVALID", "SELF_PAIR", "RX_PLAYER_INVALID",
              "RX_DEVICE_INVALID", "STATE_MISMATCH", "DEVICE_QUERY", "EVALUATED")
BINDING = {"leaseId": (1, INT_MAX), "sessionEpoch": (1, INT_MAX), "txPlayerId": (1, INT_MAX),
           "txGeneration": (1, INT_MAX), "txDeviceId": (1, INT_MAX - 1), "txTransceiverIndex": (0, INT_MAX - 1),
           "networkTypeId": (1, INT_MAX), "channelId": (1, INT_MAX), "issuedAtMs": (0, INT_MAX),
           "ttlMs": (1, INT_MAX)}
LISTENER = {"playerId": (1, INT_MAX), "generation": (1, INT_MAX), "sessionEpoch": (1, INT_MAX),
            "deviceId": (1, INT_MAX - 1), "transceiverIndex": (0, INT_MAX - 1)}
PLAYER = {"playerId": (1, INT_MAX), "generation": (1, INT_MAX)}
FIXTURE = {"a": -15.0, "b": 15.0, "qClean": 0.8, "qDegraded": 0.5}  # TEST_ONLY linear_test, как в link_eval_v1
TOLERANCE = {"sinrDb": 0.05, "intelligibility": 0.002}


def int_in(value, low, high):
    return type(value) is int and low <= value <= high


def fields_ok(value, domains, extra=()):
    return type(value) is dict and set(value) == set(domains) | set(extra) \
        and all(int_in(value[k], lo, hi) for k, (lo, hi) in domains.items())


def device_ok(d):
    """Повторяет RICON_LifecycleDevice.IsValid (RICON_Lifecycle.c:172–199)."""
    if type(d) is not dict or set(d) != {"deviceId", "kind", "ownerPlayerId", "powered", "accessPlayerIds", "allowed"}:
        return False
    if not int_in(d["deviceId"], 1, INT_MAX) or d["kind"] not in KINDS or not int_in(d["ownerPlayerId"], 0, INT_MAX) \
            or type(d["powered"]) is not bool or type(d["accessPlayerIds"]) is not list or type(d["allowed"]) is not list:
        return False
    previous = 0
    for player in d["accessPlayerIds"]:
        if not int_in(player, 1, INT_MAX) or player <= previous:
            return False
        previous = player
    previous = (0, 0)
    for pair in d["allowed"]:
        if type(pair) is not list or len(pair) != 2 or not all(int_in(v, 1, INT_MAX) for v in pair):
            return False
        if tuple(pair) <= previous:
            return False
        previous = tuple(pair)
    return True


def endpoint_key(e):
    if type(e) is not dict or set(e) != {"observation", "profile"} or e["observation"] is None or e["profile"] is None:
        return None
    obs = e["observation"]
    if type(obs) is not dict or not int_in(obs.get("deviceId"), 1, INT_MAX - 1) \
            or not int_in(obs.get("transceiverIndex"), 0, INT_MAX - 1):
        return None
    return obs["deviceId"], obs["transceiverIndex"]


def preflight(inp):
    """REQ-PS-04: код вызова. Ни S2, ни evaluator здесь не вызываются."""
    if any(inp.get(k) is None for k, _ in ARRAYS) or inp.get("context") is None or not inp.get("hasEvaluator") \
            or inp.get("limits") is None:
        return "NULL_INPUT"
    limits = inp["limits"]
    if type(limits) is not dict or set(limits) != set(LIMIT_KEYS) | {"maxPairs"} \
            or not all(int_in(limits[k], 1, 4096) for k in LIMIT_KEYS) or not int_in(limits["maxPairs"], 1, 65536):
        return "INVALID_LIMITS"
    if any(len(inp[k]) > limits[m] for k, m in ARRAYS) \
            or len(inp["bindings"]) * len(inp["listeners"]) > limits["maxPairs"]:
        return "LIMIT_EXCEEDED"
    if not int_in(inp.get("nowMs"), 0, INT_MAX) or not int_in(inp.get("sessionEpoch"), 1, INT_MAX):
        return "INVALID_ELEMENT"
    if not all(fields_ok(b, BINDING, ("active",)) and type(b["active"]) is bool for b in inp["bindings"]):
        return "INVALID_ELEMENT"
    if not all(fields_ok(x, LISTENER) for x in inp["listeners"]):
        return "INVALID_ELEMENT"
    if not all(fields_ok(p, PLAYER, ("state",)) and p["state"] in STATES for p in inp["players"]):
        return "INVALID_ELEMENT"
    if not all(device_ok(d) for d in inp["devices"]) or any(endpoint_key(e) is None for e in inp["endpoints"]):
        return "INVALID_ELEMENT"
    keys = ([b["leaseId"] for b in inp["bindings"]], [b["txDeviceId"] for b in inp["bindings"]],
            [p["playerId"] for p in inp["players"]], [d["deviceId"] for d in inp["devices"]],
            [endpoint_key(e) for e in inp["endpoints"]],
            [(x["playerId"], x["deviceId"], x["transceiverIndex"]) for x in inp["listeners"]])
    if any(len(k) != len(set(k)) for k in keys):
        return "DUPLICATE"
    return "OK"


def empty_pair(b, x, code):
    return {"leaseId": b["leaseId"], "txDeviceId": b["txDeviceId"], "rxPlayerId": x["playerId"],
            "rxDeviceId": x["deviceId"], "rxTransceiverIndex": x["transceiverIndex"], "pairCode": code,
            "hasDeviceCode": False, "deviceCode": "NULL_INPUT", "hasBudget": False, "valid": False,
            "reason": "NUMERIC_FAILURE", "state": "LOST", "intelligibility": 0.0, "sinrDb": 0.0}


def evaluate(inp):
    """Возвращает код вызова, пары по ключу REQ-PS-08 и счётчики вызовов S2 и evaluator (REQ-PS-09)."""
    code = preflight(inp)
    result = {"callCode": code, "pairs": [], "s2Calls": 0, "evaluatorCalls": 0}
    if code != "OK":
        return result
    now, epoch = inp["nowMs"], inp["sessionEpoch"]
    players = {p["playerId"]: p for p in inp["players"]}
    devices = {d["deviceId"]: d for d in inp["devices"]}
    endpoints = {endpoint_key(e): e for e in inp["endpoints"]}
    bindings = sorted(inp["bindings"], key=lambda b: b["leaseId"])
    listeners = sorted(inp["listeners"], key=lambda x: (x["deviceId"], x["transceiverIndex"], x["playerId"]))
    for b in bindings:
        for x in listeners:
            pair = empty_pair(b, x, None)
            tx_player, rx_player = players.get(b["txPlayerId"]), players.get(x["playerId"])
            tx_device, rx_device = devices.get(b["txDeviceId"]), devices.get(x["deviceId"])
            tx_end = endpoints.get((b["txDeviceId"], b["txTransceiverIndex"]))
            rx_end = endpoints.get((x["deviceId"], x["transceiverIndex"]))
            channel = [b["networkTypeId"], b["channelId"]]

            def access(device, player_id):
                return device["ownerPlayerId"] == player_id or player_id in device["accessPlayerIds"]

            if not b["active"] or b["sessionEpoch"] != epoch or now < b["issuedAtMs"] \
                    or now - b["issuedAtMs"] >= b["ttlMs"]:
                pair["pairCode"] = "BINDING_INACTIVE"
            elif tx_player is None or tx_player["state"] != "BOUND_ACTIVE" \
                    or tx_player["generation"] != b["txGeneration"]:
                pair["pairCode"] = "TX_PLAYER_INVALID"
            elif tx_device is None or tx_end is None or not access(tx_device, b["txPlayerId"]) \
                    or channel not in tx_device["allowed"]:
                pair["pairCode"] = "TX_DEVICE_INVALID"
            elif x["deviceId"] == b["txDeviceId"]:
                pair["pairCode"] = "SELF_PAIR"
            elif x["sessionEpoch"] != epoch or rx_player is None or rx_player["state"] != "BOUND_ACTIVE" \
                    or rx_player["generation"] != x["generation"]:
                pair["pairCode"] = "RX_PLAYER_INVALID"
            elif rx_device is None or rx_end is None or not access(rx_device, x["playerId"]) \
                    or channel not in rx_device["allowed"]:
                pair["pairCode"] = "RX_DEVICE_INVALID"
            elif tx_device["powered"] != tx_end["observation"].get("powered") \
                    or rx_device["powered"] != rx_end["observation"].get("powered"):
                pair["pairCode"] = "STATE_MISMATCH"
            else:
                result["s2Calls"] += 1
                built = rd.evaluate({"txObservation": tx_end["observation"], "txProfile": tx_end["profile"],
                                     "rxObservation": rx_end["observation"], "rxProfile": rx_end["profile"],
                                     "context": inp["context"]})
                pair["hasDeviceCode"], pair["deviceCode"] = True, built["code"]
                if built["code"] != "READY":
                    pair["pairCode"] = "DEVICE_QUERY"
                else:
                    result["evaluatorCalls"] += 1
                    budget = ro.evaluate_budget(built["query"], FIXTURE)
                    pair.update({"pairCode": "EVALUATED", "hasBudget": True, "valid": budget["valid"],
                                 "reason": budget["reason"], "state": budget["state"],
                                 "intelligibility": budget["intelligibility"], "sinrDb": budget["sinrDb"]})
            result["pairs"].append(pair)
    return result


# ---------------------------------------------------------------- корпус

def profile(device_id, index, tuning, freq, tx_power, gain):
    """Явные TEST_ONLY параметры; не характеристики ванильных раций."""
    return {"profileId": f"test_{device_id}_{index}", "deviceId": device_id, "transceiverIndex": index,
            "provenance": "TEST_ONLY", "expectedTuningValue": tuning, "frequencyMHz": freq, "rxBandwidthHz": 12500.0,
            "txPowerDbm": tx_power, "txGainDbi": gain, "rxGainDbi": gain, "rxNoiseFigureDb": 8.0,
            "hasRxSensitivity": False, "rxSensitivityDbm": 0.0, "hasRequiredSinr": True, "requiredSinrDb": 10.0}


def endpoint(device_id, index, x, tuning=50000, freq=50.0, tx_power=37.0, gain=2.0, key="test_net", powered=True):
    obs = {"deviceId": device_id, "transceiverIndex": index, "powered": powered, "tuningValue": tuning,
           "networkKey": key, "antennaPos": [x, 2.0, 0.0]}
    return {"observation": obs, "profile": profile(device_id, index, tuning, freq, tx_power, gain)}


def baseline():
    """2 передачи × 3 слушателя. L2 говорит слабо (−40 дБм) и не дотягивается до дальней рации 4."""
    return {
        "nowMs": 10000, "sessionEpoch": 3,
        "limits": {"maxBindings": 8, "maxListeners": 8, "maxPlayers": 8, "maxDevices": 8, "maxEndpoints": 8,
                   "maxPairs": 64},
        "bindings": [
            {"leaseId": 101, "sessionEpoch": 3, "txPlayerId": 11, "txGeneration": 5, "txDeviceId": 1,
             "txTransceiverIndex": 0, "networkTypeId": 1, "channelId": 1, "active": True, "issuedAtMs": 9000,
             "ttlMs": 5000},
            {"leaseId": 102, "sessionEpoch": 3, "txPlayerId": 12, "txGeneration": 6, "txDeviceId": 2,
             "txTransceiverIndex": 0, "networkTypeId": 1, "channelId": 1, "active": True, "issuedAtMs": 9500,
             "ttlMs": 5000}],
        "listeners": [
            {"playerId": 12, "generation": 6, "sessionEpoch": 3, "deviceId": 2, "transceiverIndex": 0},
            {"playerId": 13, "generation": 7, "sessionEpoch": 3, "deviceId": 3, "transceiverIndex": 0},
            {"playerId": 14, "generation": 8, "sessionEpoch": 3, "deviceId": 4, "transceiverIndex": 0}],
        "players": [{"playerId": p, "generation": g, "state": "BOUND_ACTIVE"}
                    for p, g in ((11, 5), (12, 6), (13, 7), (14, 8))],
        "devices": [{"deviceId": d, "kind": "CARRIED", "ownerPlayerId": o, "powered": True, "accessPlayerIds": [],
                     "allowed": [[1, 1], [1, 2]]} for d, o in ((1, 11), (2, 12), (3, 13), (4, 14))],
        "endpoints": [endpoint(1, 0, 0.0), endpoint(2, 0, 1000.0, tx_power=-40.0), endpoint(3, 0, 3000.0),
                      endpoint(3, 1, 3000.0, tuning=60000, freq=60.0), endpoint(4, 0, 20000.0)],
        "context": {"bandId": "test_vhf", "modelId": "free_space", "noiseEnvironmentId": "none",
                    "intelligibilityModelId": "linear_test", "hasInterference": False, "interferenceDbm": 0.0},
        "hasEvaluator": True,
    }


def build_cases():
    cases = []

    def add(cid, note, edit=None, ct=()):
        data = baseline()
        if edit:
            edit(data)
        cases.append({"id": cid, "note": note, "ct": list(ct), "input": data})

    def at(path, value):
        def edit(data):
            node = data
            for key in path[:-1]:
                node = node[key]
            node[path[-1]] = value
        return edit

    def both(*edits):
        def edit(data):
            for e in edits:
                e(data)
        return edit

    add("P01", "базовая сцена 2×3: самопара, дальний LOST и четыре CLEAN", ct=("CT-PS-001", "CT-PS-005", "CT-PS-010"))

    def shuffle(data):
        rng = random.Random(7)
        for key, _ in ARRAYS:
            rng.shuffle(data[key])
    add("P02", "перестановка всех входных массивов не меняет выход", shuffle, ("CT-PS-001",))
    # Отказ всего вызова, §2.
    for i, key in enumerate(("bindings", "listeners", "players", "devices", "endpoints", "context", "limits")):
        add(f"C0{i + 1}", f"отсутствует {key}", at([key], None), ("CT-PS-002",))
    add("C08", "нет evaluator", at(["hasEvaluator"], False), ("CT-PS-002",))
    add("C09", "лимит 0", at(["limits", "maxPlayers"], 0), ("CT-PS-002",))
    add("C10", "лимит 4097", at(["limits", "maxEndpoints"], 4097), ("CT-PS-002", "CT-PS-012"))
    add("C11", "maxPairs 65537", at(["limits", "maxPairs"], 65537), ("CT-PS-002", "CT-PS-012"))
    add("C12", "длина массива больше лимита", at(["limits", "maxEndpoints"], 4), ("CT-PS-002",))
    add("C13", "bindings × listeners = 6 > maxPairs 5", at(["limits", "maxPairs"], 5), ("CT-PS-002",))
    add("C14", "nowMs < 0", at(["nowMs"], -1), ("CT-PS-002",))
    add("C15", "epoch сервера 0", at(["sessionEpoch"], 0), ("CT-PS-002",))
    add("C16", "null-элемент среди слушателей", lambda d: d["listeners"].append(None), ("CT-PS-002",))
    add("C17", "ttlMs 0", at(["bindings", 0, "ttlMs"], 0), ("CT-PS-002",))
    add("C18", "неизвестное состояние игрока", at(["players", 0, "state"], "ALIVE"), ("CT-PS-002",))
    add("C19", "невалидная рация lifecycle: доступ не по возрастанию",
        at(["devices", 0, "accessPlayerIds"], [13, 12]), ("CT-PS-002",))
    add("C20", "у трансивера нет профиля", at(["endpoints", 0, "profile"], None), ("CT-PS-002",))
    add("C21", "ключ трансивера вне домена S2", at(["endpoints", 0, "observation", "deviceId"], INT_MAX),
        ("CT-PS-002",))
    add("C22", "повтор leaseId", at(["bindings", 1, "leaseId"], 101), ("CT-PS-002",))
    add("C23", "повтор txDeviceId", at(["bindings", 1, "txDeviceId"], 1), ("CT-PS-002",))
    add("C24", "повтор игрока", at(["players", 1, "playerId"], 11), ("CT-PS-002",))
    add("C25", "повтор рации", at(["devices", 1, "deviceId"], 1), ("CT-PS-002",))
    add("C26", "повтор трансивера", lambda d: d["endpoints"].append(endpoint(4, 0, 5.0)), ("CT-PS-002",))
    add("C27", "повтор слушателя", lambda d: d["listeners"].append(dict(d["listeners"][0])), ("CT-PS-002",))
    add("C28", "несколько нарушений: лимит раньше дубля и домена",
        both(at(["limits", "maxPairs"], 5), at(["bindings", 1, "leaseId"], 101), at(["nowMs"], -1)), ("CT-PS-002",))
    add("C29", "несколько нарушений: домен раньше дубля", both(at(["nowMs"], -1), at(["bindings", 1, "leaseId"], 101)),
        ("CT-PS-002",))
    add("C30", "leaseId = int.MAX и лимиты 4096/65536 принимаются",
        both(at(["bindings", 1, "leaseId"], INT_MAX),
             lambda d: d["limits"].update({k: 4096 for k in LIMIT_KEYS} | {"maxPairs": 65536})), ("CT-PS-012",))
    # Коды пары, §3.
    add("B01", "lease не ACTIVE", at(["bindings", 0, "active"], False), ("CT-PS-003",))
    add("B02", "lease из прошлой epoch", at(["bindings", 0, "sessionEpoch"], 2), ("CT-PS-003",))
    add("B03", "nowMs = issuedAtMs − 1: передача из будущего", at(["nowMs"], 8999), ("CT-PS-003", "CT-PS-004"))
    add("B04", "nowMs = issuedAtMs + ttlMs − 1: ещё активна", at(["nowMs"], 13999), ("CT-PS-004",))
    add("B05", "nowMs = issuedAtMs + ttlMs: истекла (L1), L2 ещё жива", at(["nowMs"], 14000), ("CT-PS-004",))
    add("B06", "issuedAtMs и ttlMs у int.MAX без переполнения",
        both(at(["nowMs"], INT_MAX), at(["bindings", 0, "issuedAtMs"], INT_MAX - 10),
             at(["bindings", 0, "ttlMs"], INT_MAX)), ("CT-PS-004",))
    add("B07", "говорящего нет среди игроков", lambda d: d["players"].pop(0), ("CT-PS-003",))
    add("B08", "говорящий BOUND_INACTIVE", at(["players", 0, "state"], "BOUND_INACTIVE"), ("CT-PS-003",))
    add("B09", "чужая generation говорящего", at(["bindings", 0, "txGeneration"], 4), ("CT-PS-003",))
    add("B10", "рации отправителя нет", lambda d: d["devices"].pop(0), ("CT-PS-003",))
    add("B11", "трансивера отправителя нет", at(["bindings", 0, "txTransceiverIndex"], 1), ("CT-PS-003",))
    add("B12", "говорящий без доступа к рации", at(["devices", 0, "ownerPlayerId"], 99), ("CT-PS-003",))
    add("B13", "доступ по списку, а не по владению",
        both(at(["devices", 0, "ownerPlayerId"], 0), at(["devices", 0, "accessPlayerIds"], [11])), ("CT-PS-003",))
    add("B14", "канал не разрешён рации отправителя", at(["devices", 0, "allowed"], [[1, 2]]), ("CT-PS-003",))
    add("B15", "слушатель из прошлой epoch", at(["listeners", 1, "sessionEpoch"], 2), ("CT-PS-003", "CT-PS-007"))
    add("B16", "устаревший слушатель: другая generation", at(["listeners", 1, "generation"], 6),
        ("CT-PS-003", "CT-PS-007"))
    add("B17", "слушателя нет среди игроков", lambda d: d["players"].pop(2), ("CT-PS-003",))
    add("B18", "слушатель REGISTERED_UNBOUND", at(["players", 2, "state"], "REGISTERED_UNBOUND"), ("CT-PS-003",))
    add("B19", "рации слушателя нет", lambda d: d["devices"].pop(2), ("CT-PS-003",))
    add("B20", "трансивера слушателя нет", at(["listeners", 2, "transceiverIndex"], 1), ("CT-PS-003",))
    add("B21", "слушатель без доступа к рации", at(["devices", 2, "ownerPlayerId"], 99), ("CT-PS-003",))
    add("B22", "канал не разрешён рации слушателя", at(["devices", 3, "allowed"], [[1, 2]]), ("CT-PS-003",))
    add("B23", "питание lifecycle ≠ наблюдению (TX)", at(["devices", 0, "powered"], False), ("CT-PS-003",))
    add("B24", "питание lifecycle ≠ наблюдению (RX)", at(["endpoints", 2, "observation", "powered"], False),
        ("CT-PS-003",))
    add("B25", "несколько причин: неактивный lease раньше всего",
        both(at(["bindings", 0, "active"], False), at(["players", 0, "state"], "BOUND_INACTIVE"),
             at(["devices", 1, "allowed"], [[1, 2]])), ("CT-PS-003",))
    add("B26", "несколько причин: TX раньше RX",
        both(at(["bindings", 0, "txGeneration"], 4), at(["listeners", 1, "generation"], 6)), ("CT-PS-003",))
    add("B27", "самопара раньше проверок слушателя: устаревший слушатель рации 2 у L2 даёт SELF_PAIR",
        at(["listeners", 0, "sessionEpoch"], 2), ("CT-PS-003", "CT-PS-005"))
    # S2 после фильтров службы.
    add("S01", "оба выключены: POWER_OFF в S2",
        both(at(["devices", 2, "powered"], False), at(["endpoints", 2, "observation", "powered"], False)),
        ("CT-PS-008",))
    add("S02", "настройка трансивера не совпала: TUNING_MISMATCH",
        at(["endpoints", 2, "observation", "tuningValue"], 49999), ("CT-PS-008",))
    add("S03", "второй трансивер рации 3 на 60 МГц: CHANNEL_MISMATCH",
        at(["listeners", 1, "transceiverIndex"], 1), ("CT-PS-006", "CT-PS-008"))
    add("S04", "ключ сети другой: KEY_MISMATCH", at(["endpoints", 2, "observation", "networkKey"], "other"),
        ("CT-PS-008",))
    add("S05", "профиль чужого трансивера: PROFILE_MISMATCH", at(["endpoints", 2, "profile", "transceiverIndex"], 1),
        ("CT-PS-008",))
    add("S06", "нечисловая позиция антенны: INVALID_OBSERVATION",
        at(["endpoints", 2, "observation", "antennaPos"], ["NaN", 2.0, 0.0]), ("CT-PS-008",))
    add("S07", "невалидный профиль: INVALID_PROFILE", at(["endpoints", 2, "profile", "rxBandwidthHz"], 0.0),
        ("CT-PS-008",))
    add("S08", "пустой bandId: INVALID_CONTEXT у всех пар S2", at(["context", "bandId"], ""), ("CT-PS-008",))

    def two_listeners(data):
        data["listeners"].append({"playerId": 13, "generation": 7, "sessionEpoch": 3, "deviceId": 3,
                                  "transceiverIndex": 1})
        data["endpoints"][3] = endpoint(3, 1, 3000.0)
    add("S09", "два слушателя на двух трансиверах одной рации", two_listeners, ("CT-PS-006",))
    # Бюджет.
    add("E01", "неизвестная модель: EVALUATED с невалидным бюджетом", at(["context", "modelId"], "nope"),
        ("CT-PS-009",))
    add("E02", "неизвестная среда шума: EVALUATED с невалидным бюджетом",
        at(["context", "noiseEnvironmentId"], "mars"), ("CT-PS-009",))

    def swap(data):
        data["endpoints"][1] = endpoint(2, 0, 1000.0, tx_power=37.0, gain=6.0)
    add("E03", "асимметрия: усиление 6 дБи у рации 2 меняет бюджет в обе стороны по-разному", swap, ("CT-PS-010",))
    add("E04", "помеха из контекста снижает SINR всех пар",
        both(at(["context", "hasInterference"], True), at(["context", "interferenceDbm"], -58.0)), ("CT-PS-009",))
    for c in cases:
        c["expect"] = evaluate(copy.deepcopy(c["input"]))
    return cases


def document():
    return {"schema": "ricon.pair_service", "version": 1, "revision": 1,
            "source": "docs/req/REQ-pair-service.md v0.2",
            "note": "TEST_ONLY. Бюджет — ricon_oracle.evaluate_budget (free_space), S2 — radio_device_oracle.evaluate.",
            "fixture": FIXTURE, "enforceTolerance": TOLERANCE, "cases": build_cases()}


def compare(actual, expected, path=""):
    if isinstance(expected, dict):
        if not isinstance(actual, dict) or set(actual) != set(expected):
            return [f"{path}: поля"]
        return [d for k in expected for d in compare(actual[k], expected[k], f"{path}.{k}")]
    if isinstance(expected, list):
        if not isinstance(actual, list) or len(actual) != len(expected):
            return [f"{path}: длина"]
        return [d for i, (a, e) in enumerate(zip(actual, expected)) for d in compare(a, e, f"{path}[{i}]")]
    if isinstance(expected, float) and not isinstance(expected, bool):
        return [] if isinstance(actual, float) and abs(actual - expected) <= 1e-9 else [f"{path}: {actual} != {expected}"]
    return [] if actual == expected and type(actual) is type(expected) else [f"{path}: {actual!r} != {expected!r}"]


def check(data):
    reference = document()
    errors = compare({k: v for k, v in data.items() if k != "cases"},
                     {k: v for k, v in reference.items() if k != "cases"}, "header")
    if len(data.get("cases", [])) != len(reference["cases"]):
        return errors + ["cases: длина"]
    for got, ref in zip(data["cases"], reference["cases"]):
        errors += compare(got, ref, got.get("id", "?"))
    return errors


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("vectors", type=Path)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--check", action="store_true")
    mode.add_argument("--write", action="store_true")
    args = parser.parse_args(argv)
    if args.write:
        with open(args.vectors, "w", encoding="utf-8", newline="\n") as handle:
            json.dump(document(), handle, ensure_ascii=False, indent=1, allow_nan=False)
            handle.write("\n")
        print(f"PAIR_SERVICE_ORACLE_WRITTEN: {len(document()['cases'])} cases")
        return 0
    errors = check(json.loads(args.vectors.read_text(encoding="utf-8")))
    if errors:
        print("PAIR_SERVICE_ORACLE_FAIL")
        for e in errors[:20]:
            print(e)
        return 1
    print(f"PAIR_SERVICE_ORACLE_PASS: {len(document()['cases'])} cases")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
