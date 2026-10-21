#!/usr/bin/env python3
"""TOOL_ONLY: копирует согласованный корпус S2 (radio_device_v1.json rev2) в Enforce; ожидания не вычисляет."""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from gen_l2_fixture import literal

# shortcut: генератор закреплён за корпусом, согласованным claude #3139; новую ревизию сначала сверить независимо.
SOURCE_SHA = "1806e7377cc0a68c7fc647ff19a3482841cbba4f5820fd22fbc24b8f8846d960"
CASES = 68
CODES = ("NULL_INPUT", "INVALID_OBSERVATION", "INVALID_PROFILE", "INVALID_CONTEXT", "PROFILE_MISMATCH",
         "POWER_OFF", "TUNING_MISMATCH", "CHANNEL_MISMATCH", "KEY_MISMATCH", "READY")
OBSERVATION = (("deviceId", "m_iDeviceId", "int"), ("transceiverIndex", "m_iTransceiverIndex", "int"),
               ("powered", "m_bPowered", "bool"), ("tuningValue", "m_iTuningValue", "int"),
               ("networkKey", "m_sNetworkKey", "string"), ("antennaPos", "m_vAntennaPos", "vector"))
PROFILE = (("profileId", "m_sProfileId", "string"), ("deviceId", "m_iDeviceId", "int"),
           ("transceiverIndex", "m_iTransceiverIndex", "int"), ("provenance", "m_sProvenance", "string"),
           ("expectedTuningValue", "m_iExpectedTuningValue", "int"), ("frequencyMHz", "m_fFrequencyMHz", "float"),
           ("rxBandwidthHz", "m_fRxBandwidthHz", "float"), ("txPowerDbm", "m_fTxPowerDbm", "float"),
           ("txGainDbi", "m_fTxGainDbi", "float"), ("rxGainDbi", "m_fRxGainDbi", "float"),
           ("rxNoiseFigureDb", "m_fRxNoiseFigureDb", "float"), ("hasRxSensitivity", "m_bHasRxSensitivity", "bool"),
           ("rxSensitivityDbm", "m_fRxSensitivityDbm", "float"), ("hasRequiredSinr", "m_bHasRequiredSinr", "bool"),
           ("requiredSinrDb", "m_fRequiredSinrDb", "float"))
CONTEXT = (("bandId", "m_sBandId", "string"), ("modelId", "m_sModelId", "string"),
           ("noiseEnvironmentId", "m_sNoiseEnvironmentId", "string"),
           ("intelligibilityModelId", "m_sIntelligibilityModelId", "string"),
           ("hasInterference", "m_bHasInterference", "bool"), ("interferenceDbm", "m_fInterferenceDbm", "float"))
QUERY = (("txPos", "m_vTxPos", "vector"), ("rxPos", "m_vRxPos", "vector"), ("frequencyMHz", "m_fFrequencyMHz", "float"),
         ("bandwidthHz", "m_fBandwidthHz", "float"), ("txPowerDbm", "m_fTxPowerDbm", "float"),
         ("txGainDbi", "m_fTxGainDbi", "float"), ("rxGainDbi", "m_fRxGainDbi", "float"),
         ("rxNoiseFigureDb", "m_fRxNoiseFigureDb", "float"), ("hasRxSensitivity", "m_bHasRxSensitivity", "bool"),
         ("rxSensitivityDbm", "m_fRxSensitivityDbm", "float"), ("hasRequiredSinr", "m_bHasRequiredSinr", "bool"),
         ("requiredSinrDb", "m_fRequiredSinrDb", "float"), ("hasInterference", "m_bHasInterference", "bool"),
         ("interferenceDbm", "m_fInterferenceDbm", "float"), ("bandId", "m_sBandId", "string"),
         ("modelId", "m_sModelId", "string"), ("noiseEnvironmentId", "m_sNoiseEnvironmentId", "string"),
         ("intelligibilityModelId", "m_sIntelligibilityModelId", "string"))
OBJECTS = (("txObservation", "RICON_RadioObservation", OBSERVATION), ("txProfile", "RICON_RadioRfProfile", PROFILE),
           ("rxObservation", "RICON_RadioObservation", OBSERVATION), ("rxProfile", "RICON_RadioRfProfile", PROFILE),
           ("context", "RICON_RadioQueryContext", CONTEXT))
SPECIAL = {"NaN": 0, "+Inf": 1, "-Inf": -1}


def need(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def number(value: object) -> str:
    """Литерал float. Конечное в binary32 — исходная десятичная запись (округляет компилятор Enforce);
    переполнение binary32 — бесконечность того же знака, как в REQ §2."""
    if type(value) is str:
        need(value in SPECIAL, "Неизвестное специальное число")
        return f"RICON_L0Run.Special({SPECIAL[value]})"
    need(type(value) in (int, float) and math.isfinite(value), "Некорректное число")
    try:
        struct.pack("!f", float(value))
    except OverflowError:
        return f"RICON_L0Run.Special({-1 if value < 0 else 1})"
    return literal(value)


def text(value: object) -> str:
    need(type(value) is str and re.fullmatch(r"[A-Za-z0-9_.\- ]{0,64}", value) is not None, "Недопустимая строка")
    return json.dumps(value)


def value_literal(value: object, kind: str) -> str:
    if kind == "int":
        need(type(value) is int and -2**31 <= value < 2**31, "Ожидалось int32")
        return str(value)
    if kind == "bool":
        need(type(value) is bool, "Ожидалось bool")
        return str(value).lower()
    if kind == "string":
        return text(value)
    if kind == "float":
        return number(value)
    need(type(value) is list and len(value) == 3, "Ожидался вектор из трёх чисел")
    return "Vector(" + ", ".join(number(v) for v in value) + ")"


def object_lines(name: str, cls: str, fields: tuple, value: object) -> list[str]:
    lines = [f"\t\t{cls} {name};"]
    if value is None:
        return lines
    need(type(value) is dict and set(value) == {f[0] for f in fields}, f"Неизвестные поля {name}")
    lines.append(f"\t\t{name} = new {cls}();")
    lines += [f"\t\t{name}.{field} = {value_literal(value[key], kind)};" for key, field, kind in fields]
    return lines


def case_lines(index: int, case: dict) -> list[str]:
    cid, inp, exp = case["id"], case["input"], case["expect"]
    need(type(cid) is str and re.fullmatch(r"R[0-9]{2}", cid) is not None, "Некорректный id")
    need(set(inp) == {o[0] for o in OBJECTS}, "Неизвестные входы")
    need(set(exp) == {"code", "query"} and exp["code"] in CODES, "Неизвестное ожидание")
    need((exp["code"] == "READY") == (exp["query"] is not None), "Запрос только при READY")
    lines = [f"\tprotected static void Case{index}(RICON_RadioDeviceRun t)", "\t{"]
    for name, cls, fields in OBJECTS:
        lines += object_lines(name, cls, fields, inp[name])
    lines += object_lines("expected", "RICON_LinkQuery", QUERY, exp["query"])
    lines.append(f"\t\tt.CheckCase({json.dumps(cid)}, txObservation, txProfile, rxObservation, rxProfile, context, "
                 f"RICON_ERadioQueryCode.{exp['code']}, expected);")
    return lines + ["\t}", ""]


def render(raw: bytes) -> str:
    need(hashlib.sha256(raw).hexdigest() == SOURCE_SHA, "SHA корпуса изменён: требуется ревью")
    data = json.loads(raw)
    need(data["schema"] == "ricon.radio_device" and data["version"] == 1 and data["revision"] == 2
         and len(data["cases"]) == CASES, "Неподдержанная ревизия корпуса")
    lines = ["//! Сгенерировано gen_radio_device_fixture.py; TEST_ONLY; ожидания только копируются.",
             "class RICON_RadioDeviceFixtures", "{", f'\tstatic const string SOURCE_SHA = "{SOURCE_SHA}";',
             f"\tstatic const int CASES = {CASES};", ""]
    for i, case in enumerate(data["cases"]):
        lines += case_lines(i, case)
    lines += ["\tstatic void Run(RICON_RadioDeviceRun t)", "\t{"]
    lines += [f"\t\tCase{i}(t);" for i in range(CASES)]
    return "\n".join(lines + ["\t}", "}", ""])


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args(argv)
    try:
        output = render(args.source.read_bytes())
        if args.check:
            need(args.destination.read_text(encoding="utf-8") == output, "Фикстура отличается от генератора")
        else:
            args.destination.write_text(output, encoding="utf-8", newline="\n")
    except (OSError, ValueError, KeyError, TypeError) as exc:
        print(f"RADIO_DEVICE_GENERATOR_FAIL: {exc}")
        return 1
    print(f"RADIO_DEVICE_GENERATOR_PASS: {CASES}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
