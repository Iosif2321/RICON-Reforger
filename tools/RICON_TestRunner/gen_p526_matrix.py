#!/usr/bin/env python3
"""TOOL_ONLY: копирует P526 samples принятой матрицы в Workbench, без расчёта ожиданий."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from gen_l2_fixture import literal

# shortcut: только этот принятый корпус; новую ревизию и совместимость R21-helper проверять при обновлении SHA.
SOURCE_SHA = "c325463b82cbeeabfc2366a7165070bf43a7bd7a7cfc1d18421d8b0bb3da2e7f"


def rows(scenario: dict) -> list[tuple]:
    inp, expect = scenario["input"], scenario["expect"]
    hill = inp["terrain"]["hill"]
    result = []
    for point in expect["models"]["P526_GENERAL"]["samples"]:
        for env in inp["noiseEnvironments"]:
            domain = expect["noise"][env] is not None
            noise = point["noise"][env] if domain else point["noise"]["none"]
            result.append((f'{scenario["id"]}@{literal(point["distanceM"])}-{env}', inp["frequencyMHz"],
                           point["distanceM"], inp["txHeightM"], inp["radio"]["txPowerDbm"],
                           hill["heightM"] if hill else 0.0, env, point["lossDb"], point["rxPowerDbm"],
                           noise["snrDb"], domain, point["aboveSensitivity"],
                           noise["meetsSnr"] if domain else False, domain))
    return result


def value(item: object) -> str:
    if isinstance(item, bool):
        return str(item).lower()
    if isinstance(item, str):
        return json.dumps(item)
    return literal(item)


def render(raw: bytes) -> str:
    if hashlib.sha256(raw).hexdigest() != SOURCE_SHA:
        raise ValueError("Матрица отличается от принятого SHA; требуется отдельное ревью")
    data = json.loads(raw)
    scenarios = data["scenarios"]
    all_rows = [row for scenario in scenarios for row in rows(scenario)]
    if len(scenarios) != 48 or len(all_rows) != 1008 or sum(not row[-1] for row in all_rows) != 168:
        raise ValueError("Неверные счётчики принятого корпуса")
    lines = ["//! GENERATED TEST_ONLY: gen_p526_matrix.py; числа копируются из samples.",
             "//! В 168 строках SINR — none-reference для проверки REQ-fallback, не физика UHF.",
             "class RICON_P526MatrixPoints", "{", f'\tstatic const string SOURCE_SHA = "{SOURCE_SHA}";', ""]
    for index, scenario in enumerate(scenarios):
        lines += [f'\t//! {scenario["id"]}',
                  f"\tprotected static void Scenario{index}(RICON_P526MatrixProbePlugin t)", "\t{"]
        for row in rows(scenario):
            lines.append(f"\t\tt.CheckPoint({', '.join(value(x) for x in row)});")
        lines += ["\t}", ""]
    lines += ["\tstatic void Run(RICON_P526MatrixProbePlugin t)", "\t{"]
    lines += [f"\t\tScenario{index}(t);" for index in range(len(scenarios))]
    lines += ["\t}", "}", ""]
    return "\n".join(lines)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args(argv)
    try:
        output = render(args.source.read_bytes())
        if args.check:
            if args.destination.read_text(encoding="utf-8") != output:
                raise ValueError("Сохранённые точки отличаются от генератора")
        else:
            args.destination.write_text(output, encoding="utf-8", newline="\n")
    except (OSError, ValueError) as exc:
        print(f"P526_GENERATOR_FAIL: {exc}")
        return 1
    print("P526_GENERATOR_PASS: 1008, physicalSnr=840, fallback=168")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
