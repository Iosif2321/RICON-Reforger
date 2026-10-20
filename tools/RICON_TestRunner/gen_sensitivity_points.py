#!/usr/bin/env python3
"""TOOL_ONLY: копирует согласованные 16 опорных точек L2 в Enforce, без расчёта ожиданий."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from gen_l2_fixture import literal

# shortcut: только проверенный R28 корпус; при новой ревизии сначала ревью данных и совместимости probe.
SOURCE_SHA = "8e80dabe4c9450aa76e4fed5b947072dd319b327f0838b829bbbc752fc136271"
NUMBERS = {"L": "m_fLossDb", "Lba": "m_fBullingtonActualDb", "Lbs": "m_fBullingtonSmoothDb",
           "Lsph": "m_fSphericalDb"}
ENUMS = {"branchActual": ("m_eBranchActual", "RICON_EL2Branch"),
         "branchSmooth": ("m_eBranchSmooth", "RICON_EL2Branch"),
         "sphCase": ("m_eSphericalCase", "RICON_EL2SphericalCase"),
         "firstTermAccuracy": ("m_eFirstTermAccuracy", "RICON_EFirstTermAccuracy")}


def case_lines(index: int, case: dict) -> list[str]:
    inp, expect = case["input"], case["expect"]
    if inp["htg"] != inp["hrg"] or expect["code"] != "OK":
        raise ValueError("Этот probe проверяет только согласованные симметричные OK-входы")
    cfg = inp["config"]
    config_args = [literal(cfg[k]) for k in ("earthRadiusKm", "kFactor", "permittivity", "conductivitySm")]
    config_args += ["RICON_ETerrainPolarization." + cfg["polarization"], literal(cfg["maxProfileStepKm"]),
                    str(cfg["maxProfilePoints"])]
    lines = [f'\t//! {case["id"]}',
             f"\tprotected static void Anchor{index}(RICON_SensitivityProbePlugin t)", "\t{",
             f"\t\tRICON_TerrainDiffractionConfig config = RICON_L2Run.Config({', '.join(config_args)});",
             "\t\tRICON_L2Result expected = new RICON_L2Result();"]
    lines += [f"\t\texpected.{field} = {literal(expect[key])};" for key, field in NUMBERS.items()]
    lines += [f"\t\texpected.{field} = {enum}.{expect[key]};" for key, (field, enum) in ENUMS.items()]
    # Плоский профиль передаётся нулевой высотой; ненулевая полуширина исключает деление на ноль.
    hill = inp["hill"] or {"centerKm": 0.0, "halfWidthKm": 1.0, "heightM": 0.0}
    args = [json.dumps(case["id"]), literal(inp["pathKm"] * 1000.0), literal(inp["frequencyMHz"]),
            literal(inp["htg"])] + [literal(hill[k]) for k in ("centerKm", "halfWidthKm", "heightM")]
    args += ["config", "expected", str(expect["profilePointCount"]), literal(expect["peakTerrainM"])]
    return lines + [f"\t\tt.CheckPoint({', '.join(args)});", "\t}", ""]


def render(raw: bytes) -> str:
    if hashlib.sha256(raw).hexdigest() != SOURCE_SHA:
        raise ValueError("Корпус отличается от согласованного SHA; требуется ревью")
    data = json.loads(raw)
    if len(data["cases"]) != 16 or data["toleranceDb"] != 0.05:
        raise ValueError("Счётчик или допуск принятого корпуса неверен")
    lines = ["//! Сгенерировано gen_sensitivity_points.py; TEST_ONLY, ожидания только копируются.",
             "class RICON_SensitivityPoints", "{", f'\tstatic const string SOURCE_SHA = "{SOURCE_SHA}";', ""]
    for index, case in enumerate(data["cases"]):
        lines += case_lines(index, case)
    lines += ["\tstatic void Run(RICON_SensitivityProbePlugin t)", "\t{"]
    lines += [f"\t\tAnchor{index}(t);" for index in range(16)]
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
            if args.destination.read_text(encoding="utf-8") != output:
                raise ValueError("Сохранённые точки отличаются от генератора")
        else:
            args.destination.write_text(output, encoding="utf-8", newline="\n")
    except (OSError, ValueError) as exc:
        print(f"SENSITIVITY_GENERATOR_FAIL: {exc}")
        return 1
    print("SENSITIVITY_GENERATOR_PASS: 16")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
