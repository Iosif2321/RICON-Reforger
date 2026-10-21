#!/usr/bin/env python3
"""TOOL_ONLY: копирует проверенный корпус профиля мира в Enforce; ожидания не вычисляет."""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from gen_l2_fixture import literal

# shortcut: генератор закреплён за проверенным корпусом; новую ревизию сначала сверить независимо.
SOURCE_SHA = "56c6a148236325183af8117d838ed1a22e12316642d9946c3b3ab57be48b5b1e"
CASES = 39
CONFIG_KEYS = ("earthRadiusKm", "kFactor", "permittivity", "conductivitySm", "polarization",
               "maxProfileStepKm", "maxProfilePoints")
SOURCE_KEYS = {"base", "slopeX", "slopeZ", "hills", "failAtCall", "nanAtCall"}
REASONS = {"OK", "TERRAIN_PROFILE_UNAVAILABLE", "OUT_OF_MODEL_DOMAIN", "CATALOG_VIOLATION", "NUMERIC_FAILURE"}


def need(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def number(value: object) -> str:
    if isinstance(value, str):
        need(value in {"NaN", "+Inf", "-Inf"}, "Неизвестное специальное число")
        sign = {"NaN": 0, "+Inf": 1, "-Inf": -1}[value]
        return f"RICON_L0Run.Special({sign})"
    need(type(value) in (int, float) and abs(value) <= 1e12 and math.isfinite(value), "Некорректное число")
    return literal(value)


def case_lines(index: int, case: dict) -> list[str]:
    cid, level, inp, exp = case["id"], case["level"], case["input"], case["expect"]
    need(isinstance(cid, str) and re.fullmatch(r"[A-Za-z0-9_-]{1,64}", cid) is not None, "Некорректный id")
    need(level in {"model", "provider"}, "Неизвестный уровень")
    keys = {"txPos", "rxPos", "config", "source"} | ({"frequencyMHz"} if level == "model" else {"nullQuery"})
    need(set(inp) == keys, "Неизвестные поля входа")
    lines = [f"\tprotected static void Case{index}(RICON_WorldProfileRun t)", "\t{"]
    cfg = inp["config"]
    lines.append("\t\tRICON_TerrainDiffractionConfig config;")
    if cfg is not None:
        need(set(cfg) == set(CONFIG_KEYS) and cfg["polarization"] in {"VERTICAL", "HORIZONTAL"}, "Конфиг не поддержан")
        need(type(cfg["maxProfilePoints"]) is int, "Лимит должен быть целым")
        args = [number(cfg[k]) for k in CONFIG_KEYS[:4]]
        args += ["RICON_ETerrainPolarization." + cfg["polarization"], number(cfg["maxProfileStepKm"]),
                 str(cfg["maxProfilePoints"])]
        lines.append(f"\t\tconfig = RICON_L2Run.Config({', '.join(args)});")
    lines.append("\t\tRICON_LinkQuery query;")
    if not inp.get("nullQuery", False):
        lines.append("\t\tquery = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, "
                     + number(inp.get("frequencyMHz", 50.0)) + ");")
        for key, field in (("txPos", "m_vTxPos"), ("rxPos", "m_vRxPos")):
            need(isinstance(inp[key], list) and len(inp[key]) == 3, "Некорректная позиция")
            lines.append(f"\t\tquery.{field} = Vector({', '.join(number(v) for v in inp[key])});")
    lines.append("\t\tRICON_AnalyticHeightSource source;")
    src = inp["source"]
    if src is not None:
        need(set(src) <= SOURCE_KEYS, "Неизвестные поля источника")
        lines.append("\t\tsource = new RICON_AnalyticHeightSource();")
        for key, field in (("base", "m_fBase"), ("slopeX", "m_fSlopeX"), ("slopeZ", "m_fSlopeZ")):
            lines.append(f"\t\tsource.{field} = {number(src.get(key, 0.0))};")
        for key, field in (("failAtCall", "m_iFailAtCall"), ("nanAtCall", "m_iNanAtCall")):
            need(type(src.get(key, -1)) is int, "Номер обращения должен быть целым")
            lines.append(f"\t\tsource.{field} = {src.get(key, -1)};")
        for hill in src.get("hills", []):
            need(set(hill) == {"x", "z", "height", "sigma"} and hill["sigma"] > 0, "Некорректный холм")
            args = ", ".join(number(hill[k]) for k in ("x", "z", "height", "sigma"))
            lines.append(f"\t\tsource.m_aHills.Insert(new RICON_TestHeightHill({args}));")
    lines.append("\t\tRICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();")
    called = exp["providerReason"] != "NOT_CALLED"
    lines.append(f"\t\texpected.m_bProviderCalled = {str(called).lower()};")
    if called:
        need(exp["providerReason"] in REASONS, "Неизвестная причина поставщика")
        lines.append(f"\t\texpected.m_eProviderReason = RICON_EReason.{exp['providerReason']};")
    need(type(exp["heightCalls"]) is int and exp["heightCalls"] >= 0, "Некорректный счётчик")
    lines.append(f"\t\texpected.m_iHeightCalls = {exp['heightCalls']};")
    if level == "model":
        need(exp["modelReason"] in REASONS, "Неизвестная причина модели")
        lines += ["\t\texpected.m_bModel = true;", f"\t\texpected.m_eModelReason = RICON_EReason.{exp['modelReason']};"]
    if "profile" in exp:
        need(exp["n"] == len(exp["profile"]) and exp["n"] >= 3, "Число точек не совпадает с профилем")
        lines.append("\t\texpected.m_Profile = new RICON_TerrainProfile();")
        for distance, height in exp["profile"]:
            lines.append(f"\t\texpected.m_Profile.AddPoint({number(distance)}, {number(height)});")
        lines += [f"\t\texpected.m_fTxAgl = {number(exp['htg'])};", f"\t\texpected.m_fRxAgl = {number(exp['hrg'])};"]
    if "l2Code" in exp:
        need(exp["l2Code"] in {"OK", "INVALID_ANTENNA_HEIGHT", "INVALID_PROFILE"}, "Неизвестный код L2")
        lines += ["\t\texpected.m_bHasL2 = true;", f"\t\texpected.m_eL2Code = RICON_EL2ResultCode.{exp['l2Code']};"]
    if "layers" in exp:
        need(set(exp["layers"]) == {"fspl", "terrain_diffraction"}, "Неизвестные слои")
        for layer in ("fspl", "terrain_diffraction"):
            lines.append(f'\t\texpected.m_aLayers.Insert(new RICON_LossLayer("{layer}", {number(exp["layers"][layer])}, '
                         "RICON_TerrainDiffractionModel.MODEL_ID));")
    return lines + [f"\t\tt.CheckCase({json.dumps(cid)}, {str(level == 'model').lower()}, query, config, source, expected);",
                    "\t}", ""]


def render(raw: bytes) -> str:
    need(hashlib.sha256(raw).hexdigest() == SOURCE_SHA, "SHA корпуса изменён: требуется ревью")
    data = json.loads(raw)
    need(data["schema"] == "ricon.world_profile" and data["revision"] == 2 and len(data["cases"]) == CASES,
         "Неподдержанная ревизия корпуса")
    need(data["enforceTolerance"] == {"distanceKmRel": 1e-6, "heightM": 0.01, "lossDb": 0.05}, "Допуск изменён")
    lines = ["//! Сгенерировано gen_world_profile_fixture.py; TEST_ONLY; ожидания только копируются.",
             "class RICON_WorldProfileFixtures", "{", f'\tstatic const string SOURCE_SHA = "{SOURCE_SHA}";',
             f"\tstatic const int CASES = {CASES};", ""]
    for i, case in enumerate(data["cases"]):
        lines += case_lines(i, case)
    lines += ["\tstatic void Run(RICON_WorldProfileRun t)", "\t{"]
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
        print(f"WORLD_PROFILE_GENERATOR_FAIL: {exc}")
        return 1
    print(f"WORLD_PROFILE_GENERATOR_PASS: {CASES}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
