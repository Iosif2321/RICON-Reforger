#!/usr/bin/env python3
"""TOOL_ONLY: копирует frozen P2 в Enforce, не вычисляя ожидаемые результаты."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from gen_radio_device_fixture import need, value_literal

SOURCE_SHA = "7a12ed92587318108059a213ace1d3f4286ad9a66bf86e8eb63dd4cdc7eea032"
HOST_ONLY = {"O16": "лишнее поле JSON", "O19": "bool вместо числа", "W17": "лишнее поле JSON",
             "W21": "пропущенное поле профиля JSON"}
FAMILIES = ("TERRAIN", "WATER", "STRUCTURE", "VEGETATION", "VEHICLE")
COVERAGE = ("NOT_OBSERVED", "PARTIAL", "COMPLETE")
MEDIUM = ("UNKNOWN", "AIR", "WATER")
KNOWLEDGE = ("UNKNOWN", "HINT_ONLY", "RULE", "PROFILE")
CODES = ("INVALID_OBSERVATION", "INVALID_PROFILE", "WATER_COVERAGE_INCOMPLETE", "NO_WATER",
         "OUT_OF_MODEL_DOMAIN", "NUMERIC_FAILURE", "OK")
ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "oracle/vectors/path_observation_v1.json"
TARGET = ROOT / "addons/RICON_CoreTests/Scripts/Game/RICON_Tests/RICON_PathObservationFixtures.c"


def enum(value, names):
    need(type(value) is str, "Ожидалось имя enum")
    return str(names.index(value)) if value in names else "-1"


def text(value):
    need(type(value) is str and value.isascii() and all(ord(c) >= 32 for c in value), "Неподдерживаемая строка корпуса")
    return json.dumps(value)


def endpoint(lines, name, value):
    need(type(value) is dict and set(value) == {"position", "medium", "depthM"}, "Неверные поля конца")
    lines += [f"\t\t{name} = new RICON_PathEndpoint();",
              f"\t\t{name}.m_vPosition = {value_literal(value['position'], 'vector')};",
              f"\t\t{name}.m_eMedium = {enum(value['medium'], MEDIUM)};",
              f"\t\t{name}.m_fDepthM = {value_literal(value['depthM'], 'float')};"]


def observation_lines(name, obs):
    """Общее наблюдение P2 для генераторов водного и растительного слоёв; только копирование."""
    need(re.fullmatch(r'[A-Za-z_][A-Za-z0-9_]*', name), "Неверное имя переменной")
    need(set(obs) == {"frequencyMHz", "environmentRevision", "tx", "rx", "coverage", "segments"}, "Неверные поля наблюдения")
    lines = [f"\t\tRICON_PathObservation {name} = new RICON_PathObservation();",
             f"\t\t{name}.m_fFrequencyMHz = {value_literal(obs['frequencyMHz'], 'float')};",
             f"\t\t{name}.m_iEnvironmentRevision = {value_literal(obs['environmentRevision'], 'int')};"]
    endpoint(lines, f"{name}.m_Tx", obs["tx"])
    endpoint(lines, f"{name}.m_Rx", obs["rx"])
    need(set(obs["coverage"]) <= set(FAMILIES), "Неизвестное семейство покрытия")
    for family in FAMILIES:
        if family in obs["coverage"]:
            lines.append(f"\t\t{name}.m_aCoverage.Insert({enum(obs['coverage'][family], COVERAGE)});")
    for i, segment in enumerate(obs["segments"]):
        need(set(segment) == {"family", "startM", "endM", "entityKey", "knowledge", "materialId", "openings"}, "Неверные поля сегмента")
        seg = f"segment{i}"
        lines += [f"\t\tRICON_PathSegment {seg} = new RICON_PathSegment();",
                  f"\t\t{seg}.m_eFamily = {enum(segment['family'], FAMILIES)};",
                  f"\t\t{seg}.m_fStartM = {value_literal(segment['startM'], 'float')};",
                  f"\t\t{seg}.m_fEndM = {value_literal(segment['endM'], 'float')};",
                  f"\t\t{seg}.m_sEntityKey = {text(segment['entityKey'])};",
                  f"\t\t{seg}.m_eKnowledge = {enum(segment['knowledge'], KNOWLEDGE)};",
                  f"\t\t{seg}.m_sMaterialId = {text(segment['materialId'])};",
                  f"\t\t{seg}.m_bOpenings = {value_literal(segment['openings'], 'bool')};",
                  f"\t\t{name}.m_aSegments.Insert({seg});"]
    return lines


def case_lines(index, case):
    need(set(case) == {"id", "title", "input", "expect"}, "Неверные поля случая")
    inp, expected = case["input"], case["expect"]
    need(set(inp) == {"observation", "profile"}, "Неверные поля входа")
    obs, profile = inp["observation"], inp["profile"]
    need(set(profile) == {"tempC", "salinityPpt"}, "Неверные поля профиля")
    need(set(expected) == {"observationCode", "water"}, "Неверные поля ожидания")
    water = expected["water"]
    need(water["code"] in CODES, "Неверный код воды")
    need(expected["observationCode"] == ("INVALID_OBSERVATION" if water["code"] == "INVALID_OBSERVATION" else "VALID"),
         "Несогласованные ожидаемые коды")
    lines = [f"\tprotected static void Case{index}(RICON_PathObservationRun t)", "\t{"] + observation_lines("input", obs)
    lines += ["\t\tRICON_WaterProfile profile = new RICON_WaterProfile();",
              f"\t\tprofile.m_fTempC = {value_literal(profile['tempC'], 'float')};",
              f"\t\tprofile.m_fSalinityPpt = {value_literal(profile['salinityPpt'], 'float')};",
              "\t\tRICON_WaterLayerResult expected = new RICON_WaterLayerResult();",
              f"\t\texpected.m_eCode = RICON_EWaterCode.{water['code']};",
              f"\t\texpected.m_bHasLoss = {str('lossDb' in water).lower()};"]
    if "lossDb" in water:
        lines.append(f"\t\texpected.m_fLossDb = {value_literal(water['lossDb'], 'float')};")
    if water["code"] == "OK":
        need(set(water) == {"code", "lossDb", "alphaDbPerM", "interfaceDb", "waterLengthM", "crossings", "intervals"},
             "Неверные поля OK")
        for key, field in (("alphaDbPerM", "AlphaDbPerM"), ("interfaceDb", "InterfaceDb"), ("waterLengthM", "WaterLengthM")):
            lines.append(f"\t\texpected.m_f{field} = {value_literal(water[key], 'float')};")
        lines.append(f"\t\texpected.m_iCrossings = {value_literal(water['crossings'], 'int')};")
        for i, (start, end) in enumerate(water["intervals"]):
            lines += [f"\t\tRICON_WaterInterval interval{i} = new RICON_WaterInterval();",
                      f"\t\tinterval{i}.m_fStartM = {value_literal(start, 'float')};",
                      f"\t\tinterval{i}.m_fEndM = {value_literal(end, 'float')};",
                      f"\t\texpected.m_aIntervals.Insert(interval{i});"]
    lines += [f"\t\tt.CheckCase({text(case['id'])}, input, profile, expected);", "\t}", ""]
    return lines


def build(raw):
    need(hashlib.sha256(raw).hexdigest() == SOURCE_SHA, "SHA frozen корпуса не совпал")
    corpus = json.loads(raw)
    cases = corpus["cases"]
    ids = [case["id"] for case in cases]
    need(len(cases) == 51 and len(set(ids)) == 51 and all(re.fullmatch(r"[OW][0-9]{2}", x) for x in ids), "Неверный состав корпуса")
    need(set(HOST_ONLY) <= set(ids), "Нет host-only случаев")
    native = [case for case in cases if case["id"] not in HOST_ONLY]
    lines = ["//! TOOL_ONLY: frozen P2, ожидания скопированы из независимо проверенного JSON.",
             "class RICON_PathObservationFixtures", "{", f'\tstatic const string SOURCE_SHA = "{SOURCE_SHA}";',
             f"\tstatic const int CASES = {len(native)};", f"\tstatic const int HOST_ONLY = {len(HOST_ONLY)};", ""]
    lines += [f"\t// {cid}: host-only — {why}." for cid, why in HOST_ONLY.items()]
    lines += ["\tstatic void Run(RICON_PathObservationRun t)", "\t{"]
    lines += [f"\t\tCase{i}(t);" for i in range(len(native))]
    lines += ["\t}", ""]
    for i, case in enumerate(native):
        lines += case_lines(i, case)
    lines += ["}", ""]
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    generated = build(SOURCE.read_bytes())
    if args.check:
        need(TARGET.read_bytes() == generated.encode("utf-8"), "Fixture отличается от frozen корпуса")
    else:
        TARGET.write_text(generated, encoding="utf-8", newline="\n")
    print(f"P2 fixture: {51 - len(HOST_ONLY)} native, {len(HOST_ONLY)} host-only; SHA {SOURCE_SHA}; PASS")


if __name__ == "__main__":
    main()
