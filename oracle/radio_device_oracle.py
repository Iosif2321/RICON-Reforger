#!/usr/bin/env python3
"""Эталон S2 по REQ-radio-device-adapter. Только сборка запроса, без движка и доставки голоса."""
from __future__ import annotations

import argparse
import copy
import json
import math
from pathlib import Path
import struct

INPUTS = ("txObservation", "txProfile", "rxObservation", "rxProfile", "context")
FLOATS = ("frequencyMHz", "rxBandwidthHz", "txPowerDbm", "txGainDbi", "rxGainDbi",
          "rxNoiseFigureDb", "rxSensitivityDbm", "requiredSinrDb")
IDS = ("bandId", "modelId", "noiseEnvironmentId", "intelligibilityModelId")
CODES = ("NULL_INPUT", "INVALID_OBSERVATION", "INVALID_PROFILE", "INVALID_CONTEXT", "PROFILE_MISMATCH",
         "POWER_OFF", "TUNING_MISMATCH", "CHANNEL_MISMATCH", "KEY_MISMATCH", "READY")
SPECIAL = {"NaN": math.nan, "+Inf": math.inf, "-Inf": -math.inf}
INT_MAX = 2**31 - 1


def f32(value):
    """Только числовые поля: тип float Enforce уже округлён до binary32 при вызове."""
    if type(value) is str and value in SPECIAL:
        return SPECIAL[value]
    if type(value) not in (int, float):
        raise ValueError("ожидалось число или специальный float")
    try:
        return struct.unpack("!f", struct.pack("!f", float(value)))[0]
    except OverflowError:
        return -math.inf if value < 0 else math.inf


def int32(value):
    return type(value) is int and -2**31 <= value <= INT_MAX


def identity(value, minimum):
    return int32(value) and minimum <= value < INT_MAX


def nonempty(value):
    return type(value) is str and bool(value)


def observation(value):
    if type(value) is not dict or not identity(value.get("deviceId"), 1) \
            or not identity(value.get("transceiverIndex"), 0) or type(value.get("powered")) is not bool \
            or not int32(value.get("tuningValue")) or type(value.get("networkKey")) is not str:
        return None
    pos = value.get("antennaPos")
    if type(pos) is not list or len(pos) != 3:
        return None
    try:
        pos = [f32(component) for component in pos]
    except ValueError:
        return None
    if not all(math.isfinite(component) for component in pos):
        return None
    return {**value, "antennaPos": pos}


def profile(value):
    if type(value) is not dict or not nonempty(value.get("profileId")) \
            or not identity(value.get("deviceId"), 1) or not identity(value.get("transceiverIndex"), 0) \
            or value.get("provenance") not in ("TEST_ONLY", "ASSUMED") \
            or not int32(value.get("expectedTuningValue")):
        return None
    if type(value.get("hasRxSensitivity")) is not bool or type(value.get("hasRequiredSinr")) is not bool \
            or value["hasRxSensitivity"] == value["hasRequiredSinr"]:
        return None
    try:
        result = {**value, **{key: f32(value.get(key)) for key in FLOATS}}
    except ValueError:
        return None
    active = FLOATS[:6] + (("rxSensitivityDbm",) if value["hasRxSensitivity"] else ("requiredSinrDb",))
    if not all(math.isfinite(result[key]) for key in active):
        return None
    if result["frequencyMHz"] <= 0 or result["rxBandwidthHz"] <= 0 or result["rxNoiseFigureDb"] < 0:
        return None
    return result


def context(value):
    if type(value) is not dict or not all(nonempty(value.get(key)) for key in IDS) \
            or type(value.get("hasInterference")) is not bool:
        return None
    try:
        interference = f32(value.get("interferenceDbm"))
    except ValueError:
        return None
    if value["hasInterference"] and not math.isfinite(interference):
        return None
    return {**value, "interferenceDbm": interference}


def evaluate(inputs):
    """Порядок §3. Возвращаемые записи не содержат ссылок на изменяемые входы."""
    def reject(code):
        return {"code": code, "query": None}

    if type(inputs) is not dict or any(inputs.get(key) is None for key in INPUTS):
        return reject("NULL_INPUT")
    tx, rx = observation(inputs["txObservation"]), observation(inputs["rxObservation"])
    if tx is None or rx is None:
        return reject("INVALID_OBSERVATION")
    tp, rp = profile(inputs["txProfile"]), profile(inputs["rxProfile"])
    if tp is None or rp is None:
        return reject("INVALID_PROFILE")
    ctx = context(inputs["context"])
    if ctx is None:
        return reject("INVALID_CONTEXT")
    for state, rf in ((tx, tp), (rx, rp)):
        if any(state[key] != rf[key] for key in ("deviceId", "transceiverIndex")):
            return reject("PROFILE_MISMATCH")
    if not tx["powered"] or not rx["powered"]:
        return reject("POWER_OFF")
    if tx["tuningValue"] != tp["expectedTuningValue"] or rx["tuningValue"] != rp["expectedTuningValue"]:
        return reject("TUNING_MISMATCH")
    if tp["frequencyMHz"] != rp["frequencyMHz"]:
        return reject("CHANNEL_MISMATCH")
    if tx["networkKey"] != rx["networkKey"]:
        return reject("KEY_MISMATCH")
    query = {"txPos": list(tx["antennaPos"]), "rxPos": list(rx["antennaPos"]),
             "frequencyMHz": tp["frequencyMHz"], "bandwidthHz": rp["rxBandwidthHz"],
             "txPowerDbm": tp["txPowerDbm"], "txGainDbi": tp["txGainDbi"], "rxGainDbi": rp["rxGainDbi"],
             "rxNoiseFigureDb": rp["rxNoiseFigureDb"], "hasRxSensitivity": rp["hasRxSensitivity"],
             "rxSensitivityDbm": rp["rxSensitivityDbm"] if rp["hasRxSensitivity"] else 0.0,
             "hasRequiredSinr": rp["hasRequiredSinr"],
             "requiredSinrDb": rp["requiredSinrDb"] if rp["hasRequiredSinr"] else 0.0,
             "hasInterference": ctx["hasInterference"],
             "interferenceDbm": ctx["interferenceDbm"] if ctx["hasInterference"] else 0.0,
             **{key: ctx[key] for key in IDS}}
    return {"code": "READY", "query": query}


def baseline():
    """Явные TEST_ONLY параметры двух разных устройств; не характеристики ванильных раций."""
    tp = {"profileId": "test_tx_1", "deviceId": 1, "transceiverIndex": 0, "provenance": "TEST_ONLY",
          "expectedTuningValue": 50000, "frequencyMHz": 50.0, "rxBandwidthHz": 25000.0,
          "txPowerDbm": 37.0, "txGainDbi": 2.0, "rxGainDbi": 7.0, "rxNoiseFigureDb": 12.0,
          "hasRxSensitivity": False, "rxSensitivityDbm": 0.0, "hasRequiredSinr": True, "requiredSinrDb": 14.0}
    rp = {**tp, "profileId": "test_rx_2", "deviceId": 2, "transceiverIndex": 1, "rxBandwidthHz": 12500.0,
          "txPowerDbm": 30.0, "txGainDbi": 5.0, "rxGainDbi": 3.0, "rxNoiseFigureDb": 8.0, "requiredSinrDb": 10.0}
    tx = {"deviceId": 1, "transceiverIndex": 0, "powered": True, "tuningValue": 50000,
          "networkKey": "test_net", "antennaPos": [0.0, 101.5, 0.0]}
    rx = {**tx, "deviceId": 2, "transceiverIndex": 1, "antennaPos": [2000.0, 101.5, 0.0]}
    ctx = {"bandId": "test_vhf", "modelId": "terrain_diffraction_p526_16", "noiseEnvironmentId": "none",
           "intelligibilityModelId": "linear_test", "hasInterference": False, "interferenceDbm": 0.0}
    return {"txObservation": tx, "txProfile": tp, "rxObservation": rx, "rxProfile": rp, "context": ctx}


def expected_baseline():
    """Ожидания записаны отдельно от evaluate: неправильная сторона RF-поля не обновит их автоматически."""
    return {"txPos": [0.0, 101.5, 0.0], "rxPos": [2000.0, 101.5, 0.0], "frequencyMHz": 50.0,
            "bandwidthHz": 12500.0, "txPowerDbm": 37.0, "txGainDbi": 2.0, "rxGainDbi": 3.0,
            "rxNoiseFigureDb": 8.0, "hasRxSensitivity": False, "rxSensitivityDbm": 0.0,
            "hasRequiredSinr": True, "requiredSinrDb": 10.0, "hasInterference": False, "interferenceDbm": 0.0,
            "bandId": "test_vhf", "modelId": "terrain_diffraction_p526_16", "noiseEnvironmentId": "none",
            "intelligibilityModelId": "linear_test"}


def build_cases():
    cases = []

    def add(note, edits=(), code="READY", query_edits=None):
        inputs = copy.deepcopy(baseline())
        for path, value in edits:
            target = inputs
            for key in path[:-1]:
                target = target[key]
            target[path[-1]] = copy.deepcopy(value)
        query = {**expected_baseline(), **(query_edits or {})} if code == "READY" else None
        cases.append({"id": f"R{len(cases)+1:02d}", "note": note, "input": inputs,
                      "expect": {"code": code, "query": query}})

    add("Разные TX/RX профили, прямая пара")
    b = baseline()
    add("Обратное направление", ((("txObservation",), b["rxObservation"]), (("rxObservation",), b["txObservation"]),
        (("txProfile",), b["rxProfile"]), (("rxProfile",), b["txProfile"])), query_edits={
            "txPos": [2000.0, 101.5, 0.0], "rxPos": [0.0, 101.5, 0.0], "bandwidthHz": 25000.0,
            "txPowerDbm": 30.0, "txGainDbi": 5.0, "rxGainDbi": 7.0, "rxNoiseFigureDb": 12.0, "requiredSinrDb": 14.0})
    add("Разные raw значения одной RF-частоты", ((("txObservation", "tuningValue"), 12345),
        (("txProfile", "expectedTuningValue"), 12345), (("rxObservation", "tuningValue"), 67890),
        (("rxProfile", "expectedTuningValue"), 67890)))
    add("RX sensitivity вместо required SINR", ((("rxProfile", "hasRxSensitivity"), True),
        (("rxProfile", "hasRequiredSinr"), False), (("rxProfile", "rxSensitivityDbm"), -111.0)),
        query_edits={"hasRxSensitivity": True, "hasRequiredSinr": False, "rxSensitivityDbm": -111.0, "requiredSinrDb": 0.0})
    add("Оба пустых ключа", ((("txObservation", "networkKey"), ""), (("rxObservation", "networkKey"), "")))
    add("ASSUMED объявлен явно", ((("txProfile", "provenance"), "ASSUMED"), (("rxProfile", "provenance"), "ASSUMED")))
    add("Неактивные поля не переносят NaN", ((("rxProfile", "rxSensitivityDbm"), "NaN"),
        (("context", "interferenceDbm"), "+Inf")))
    add("Частоты равны после binary32", ((("txProfile", "frequencyMHz"), 50.00000001),))
    add("Точка ног остаётся на грунте", ((("txObservation", "antennaPos", 1), 100.0),),
        query_edits={"txPos": [0.0, 100.0, 0.0]})
    add("Неиспользуемые стороны профилей", ((("txProfile", "rxBandwidthHz"), 5000.0),
        (("txProfile", "rxGainDbi"), 80.0), (("txProfile", "rxNoiseFigureDb"), 25.0),
        (("rxProfile", "txPowerDbm"), 60.0), (("rxProfile", "txGainDbi"), -4.0)))
    add("Активная помеха", ((("context", "hasInterference"), True), (("context", "interferenceDbm"), -85.0)),
        query_edits={"hasInterference": True, "interferenceDbm": -85.0})
    for key in INPUTS:
        add("Отсутствует " + key, (((key,), None),), "NULL_INPUT")
    for key, value in (("deviceId", 0), ("deviceId", INT_MAX), ("transceiverIndex", -1), ("transceiverIndex", INT_MAX)):
        add("Неверная identity " + key, ((("txObservation", key), value),), "INVALID_OBSERVATION")
    for i, (side, axis) in enumerate((side, axis) for side in ("txObservation", "rxObservation") for axis in range(3)):
        add("Нечисловая компонента " + side + str(axis), (((side, "antennaPos", axis), ("NaN", "+Inf", "-Inf")[i % 3]),),
            "INVALID_OBSERVATION")
    for path, value in ((("txProfile", "profileId"), ""), (("rxProfile", "provenance"), "VERIFIED"),
                        (("txProfile", "deviceId"), 0), (("rxProfile", "transceiverIndex"), -1)):
        add("Непригодный профиль " + path[1], ((path, value),), "INVALID_PROFILE")
    for side in ("txProfile", "rxProfile"):
        for i, key in enumerate(FLOATS[:6] + ("requiredSinrDb",)):
            add("Нечисловой профиль " + side + "." + key,
                (((side, key), ("NaN", "+Inf", "-Inf")[i % 3]),), "INVALID_PROFILE")
    add("Нечисловая sensitivity", ((("rxProfile", "hasRxSensitivity"), True),
        (("rxProfile", "hasRequiredSinr"), False), (("rxProfile", "rxSensitivityDbm"), "NaN")), "INVALID_PROFILE")
    for value in (False, True):
        add("Нарушен XOR порогов", ((("txProfile", "hasRxSensitivity"), value),
            (("txProfile", "hasRequiredSinr"), value)), "INVALID_PROFILE")
    add("Отрицательный NF", ((("rxProfile", "rxNoiseFigureDb"), -1.0),), "INVALID_PROFILE")
    add("Нулевая полоса", ((("rxProfile", "rxBandwidthHz"), 0.0),), "INVALID_PROFILE")
    add("Overflow float32", ((("txProfile", "txPowerDbm"), 1e40),), "INVALID_PROFILE")
    add("Underflow положительной частоты", ((("txProfile", "frequencyMHz"), 1e-50),), "INVALID_PROFILE")
    add("Пустой context ID", ((("context", "bandId"), ""),), "INVALID_CONTEXT")
    add("Нечисловая активная помеха", ((("context", "hasInterference"), True),
        (("context", "interferenceDbm"), "-Inf")), "INVALID_CONTEXT")
    add("Профиль другого device", ((("txProfile", "deviceId"), 9),), "PROFILE_MISMATCH")
    add("Профиль другого transceiver", ((("rxProfile", "transceiverIndex"), 2),), "PROFILE_MISMATCH")
    for side in ("txObservation", "rxObservation"):
        add("Выключен " + side, (((side, "powered"), False),), "POWER_OFF")
        add("Настройка не совпала " + side, (((side, "tuningValue"), 49999),), "TUNING_MISMATCH")
    add("RF-частоты не совпали", ((("rxProfile", "frequencyMHz"), 51.0),), "CHANNEL_MISMATCH")
    add("Ключи не совпали", ((("rxObservation", "networkKey"), "other_test_net"),), "KEY_MISMATCH")
    for note, edits, code in (
        ("Профиль раньше питания", ((("txProfile", "provenance"), ""), (("rxObservation", "powered"), False)), "INVALID_PROFILE"),
        ("Наблюдение раньше профиля", ((("txObservation", "deviceId"), 0), (("rxProfile", "profileId"), "")), "INVALID_OBSERVATION"),
        ("Контекст раньше mismatch", ((("context", "modelId"), ""), (("txProfile", "deviceId"), 9)), "INVALID_CONTEXT"),
        ("Питание раньше настройки", ((("txObservation", "powered"), False), (("rxObservation", "tuningValue"), 1)), "POWER_OFF"),
        ("Настройка раньше канала", ((("txObservation", "tuningValue"), 1), (("rxProfile", "frequencyMHz"), 51.0)), "TUNING_MISMATCH"),
        ("Канал раньше ключа", ((("rxProfile", "frequencyMHz"), 51.0), (("rxObservation", "networkKey"), "other")), "CHANNEL_MISMATCH")):
        add(note, edits, code)
    return cases


def canonical(value):
    return json.dumps(value, ensure_ascii=False, sort_keys=True, allow_nan=False)


def document():
    cases = build_cases()
    for case in cases:
        actual = evaluate(case["input"])
        if canonical(actual) != canonical(case["expect"]):
            raise AssertionError(f"{case['id']}: эталон не совпал с отдельно заданным ожиданием: {actual}")
    return {"schema": "ricon.radio_device", "version": 1, "revision": 1,
            "source": "docs/req/REQ-radio-device-adapter.md v0.1",
            "floatSemantics": "IEEE754 binary32 на входе; только выбор полей",
            "note": "TEST_ONLY/ASSUMED. Native радио, актуальный authority binding и доставка голоса не проверены.", "cases": cases}


def check(data):
    try:
        return [] if canonical(data) == canonical(document()) else ["корпус отличается от нормативных входов/ожиданий"]
    except (TypeError, ValueError) as error:
        return [str(error)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--check", type=Path)
    group.add_argument("--write", type=Path)
    args = parser.parse_args()
    if args.write:
        args.write.write_text(json.dumps(document(), ensure_ascii=False, indent=1, allow_nan=False) + "\n",
                              encoding="utf-8", newline="\n")
        return 0
    try:
        data = json.loads(args.check.read_text(encoding="utf-8"))
        errors = check(data)
    except (OSError, ValueError) as error:
        errors = [str(error)]
    if errors:
        for error in errors:
            print("RADIO_DEVICE_ORACLE_FAIL: " + error)
        return 1
    print(f"RADIO_DEVICE_ORACLE_PASS: {len(data['cases'])} cases")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
