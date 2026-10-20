#!/usr/bin/env python3
"""TOOL_ONLY: копирует принятые 57 векторов L0 в Enforce; формулы не вычисляет.

python -B -I tools/RICON_TestRunner/gen_l0_fixture.py <link_eval_v1.json> <RICON_L0Fixtures.c> [--check]
"""
from __future__ import annotations

import argparse
import copy
import hashlib
import json
from pathlib import Path
import sys

# Режим -I убирает папку скрипта; разрешаем импорт только соседнего инструмента.
sys.path.insert(0, str(Path(__file__).resolve().parent))
from gen_l2_fixture import literal

# shortcut: принимается только этот корпус; новую ревизию и её SHA добавлять после отдельного ревью.
SOURCE_SHA = "e7e1c7ef4026b03a7b7e666215ae01d8dcb34d94dbd716375dcd58b453e86a78"
SPECIALS = {"NaN": 0, "Infinity": 1, "-Infinity": -1}
QUERY_FIELDS = {
    "frequencyMHz": "m_fFrequencyMHz", "bandwidthHz": "m_fBandwidthHz", "txPowerDbm": "m_fTxPowerDbm",
    "txGainDbi": "m_fTxGainDbi", "rxGainDbi": "m_fRxGainDbi", "rxNoiseFigureDb": "m_fRxNoiseFigureDb",
    "hasRxSensitivity": "m_bHasRxSensitivity", "rxSensitivityDbm": "m_fRxSensitivityDbm",
    "hasRequiredSinr": "m_bHasRequiredSinr", "requiredSinrDb": "m_fRequiredSinrDb",
    "hasInterference": "m_bHasInterference", "interferenceDbm": "m_fInterferenceDbm",
    "noiseEnvironmentId": "m_sNoiseEnvironmentId", "bandId": "m_sBandId", "modelId": "m_sModelId",
    "intelligibilityModelId": "m_sIntelligibilityModelId",
}
BUDGET_FIELDS = {
    "distanceM": "m_fDistanceM", "totalLossDb": "m_fTotalLossDb", "rxPowerDbm": "m_fRxPowerDbm",
    "thermalNoiseDbm": "m_fThermalNoiseDbm", "noiseDbm": "m_fNoiseDbm",
    "noisePlusInterferenceDbm": "m_fNoisePlusInterferenceDbm", "sinrDb": "m_fSinrDb",
    "sensitivityDbm": "m_fSensitivityDbm", "noiseModelInDomain": "m_bNoiseModelInDomain",
    "intelligibility": "m_fIntelligibility", "valid": "m_bValid",
}


def value(item: object) -> str:
    if isinstance(item, bool):
        return str(item).lower()
    if isinstance(item, str):
        if item in SPECIALS:
            return f"RICON_L0Run.Special({SPECIALS[item]})"
        return json.dumps(item)
    return literal(item)


def query_method(vector: dict, index: int) -> list[str]:
    inp = vector["input"]
    lines = [f"\tprotected static RICON_LinkQuery Query{index}()", "\t{", "\t\tRICON_LinkQuery q = new RICON_LinkQuery();"]
    for name, member in (("txPos", "m_vTxPos"), ("rxPos", "m_vRxPos")):
        lines.append(f"\t\tq.{member} = Vector({', '.join(value(x) for x in inp[name])});")
    for name, member in QUERY_FIELDS.items():
        lines.append(f"\t\tq.{member} = {value(inp[name])};")
    return lines + ["\t\treturn q;", "\t}", ""]


def case_method(vector: dict, index: int, query_index: int) -> list[str]:
    kind, inp, exp = vector["kind"], vector["input"], vector["expected"]
    body: list[str] = []
    if kind == "fspl":
        body.append(f't.CheckNumber("lossDb", RICON_FreeSpaceModel.LossDb({value(inp["distanceM"])}, '
                    f'{value(inp["frequencyMHz"])}), {value(exp["lossDb"])}, 0.01);')
    elif kind == "thermal_noise":
        body.append(f't.CheckNumber("thermalNoiseDbm", RICON_WirelessLinkEvaluator.ThermalNoiseDbm('
                    f'{value(inp["bandwidthHz"])}), {value(exp["thermalNoiseDbm"])}, 0.01);')
    elif kind == "system_noise":
        args = [inp[k] for k in ("bandwidthHz", "rxNoiseFigureDb", "noiseEnvironmentId", "frequencyMHz")]
        args += [exp[k] for k in ("faDb", "noiseDbm", "noiseModelInDomain")]
        body.append(f"t.CheckSystemNoise({', '.join(value(x) for x in args)});")
    elif kind == "power_sum":
        powers = ", ".join(value(x) for x in inp["powersDbm"])
        body += [f"array<float> powers = {{{powers}}};", f"array<float> before = {{{powers}}};", "float actual;",
                 't.Check(RICON_Num.TryPowerSumDbm(powers, actual), "сумма определена");',
                 f't.CheckNumber("sumDbm", actual, {value(exp["sumDbm"])}, 0.01);',
                 't.Check(powers.Count() == before.Count(), "размер входа не изменён");',
                 "for (int i = 0; i < before.Count() && i < powers.Count(); i++)",
                 '\tt.Check(powers[i] == before[i], "мощности не изменены");']
    elif kind == "intelligibility_linear_test":
        body += [f'RICON_LinearTestIntelligibility quality = new RICON_LinearTestIntelligibility('
                 f'{value(inp["params"]["a"])}, {value(inp["params"]["b"])}, 0.8, 0.5);',
                 't.Check(quality.IsConfigValid(), "TEST_ONLY конфиг");',
                 f't.CheckNumber("intelligibility", quality.Intelligibility({value(inp["sinrDb"])}), '
                 f'{value(exp["intelligibility"])}, 0.000001);']
    elif kind in ("budget", "invalid"):
        body += [f"RICON_LinkQuery q = Query{query_index}();", f"RICON_LinkQuery before = Query{query_index}();"]
        for name, member in (("txPos", "m_vTxPos"), ("rxPos", "m_vRxPos")):
            for axis, coordinate in enumerate(inp[name]):
                if isinstance(coordinate, str) and coordinate in SPECIALS:
                    body.append(f"t.RequireSpecial(q.{member}[{axis}], {SPECIALS[coordinate]});")
        for name, member in QUERY_FIELDS.items():
            if isinstance(inp[name], str) and inp[name] in SPECIALS:
                body.append(f"t.RequireSpecial(q.{member}, {SPECIALS[inp[name]]});")
        body += ["RICON_LinkBudget actual = t.Evaluate(q, before);", "RICON_LinkBudget expected = new RICON_LinkBudget();"]
        for name, member in BUDGET_FIELDS.items():
            if name in exp:
                body.append(f"expected.{member} = {value(exp[name])};")
        body += [f'expected.m_eReason = RICON_EReason.{exp["reason"]};',
                 f'expected.m_eState = RICON_ELinkState.{exp["state"]};']
        for layer in exp.get("layers", []):
            body.append(f'expected.m_aLayers.Insert(new RICON_LossLayer({value(layer["layerId"])}, '
                        f'{value(layer["lossDb"])}, RICON_FreeSpaceModel.MODEL_ID));')
        body.append("t.CheckBudget(actual, expected);")
    else:
        raise ValueError(f"Неподдерживаемый kind: {kind}")
    return [f'\t//! {vector["id"]}', f"\tprotected static void Case{index}(RICON_L0Run t)", "\t{"] + [
        "\t\t" + line for line in body] + ["\t}", ""]


def controls(vectors: list[dict]) -> list[tuple[str, dict, int]]:
    result = []
    # Все повреждения проходят тот же CheckBudget, что обычные векторы.
    for label, source, field, replacement in (
        ("loss", "BUD-01", "totalLossDb", 75.989608),
        ("distance", "BUD-01", "distanceM", 1000.002),
        ("state", "BUD-02", "state", "CLEAN"),
        ("domain", "BUD-06", "noiseModelInDomain", True),
        ("layer_id", "BUD-01", "layers", [{"layerId": "wrong_layer", "lossDb": 75.969608}]),
        ("layer_count", "BUD-01", "layers", []),
        ("reason", "INV-01", "reason", "INVALID_GAIN"),
        ("nonfinite_expected", "BUD-01", "totalLossDb", "NaN"),
    ):
        index = next(i for i, v in enumerate(vectors) if v["id"] == source)
        broken = copy.deepcopy(vectors[index])
        broken["id"] = "X_" + label
        broken["expected"][field] = replacement
        result.append((label, broken, index))
    return result


def render(raw: bytes) -> str:
    if hashlib.sha256(raw).hexdigest() != SOURCE_SHA:
        raise ValueError("Вход отличается от принятого SHA; новая ревизия требует ревью")
    data = json.loads(raw)
    vectors = data["vectors"]
    if len(vectors) != 57:
        raise ValueError("Ожидались 57 векторов")
    lines = ["//! GENERATED TEST_ONLY: gen_l0_fixture.py; ожидания скопированы, формулы не вычисляются.",
             f"//! Источник SHA-256: {SOURCE_SHA}",
             "//! FSPL-01…04: effectiveDistanceM не экспортируется Core, сверяется только lossDb.",
             "class RICON_L0Fixtures", "{"]
    for index, vector in enumerate(vectors):
        if vector["kind"] in ("budget", "invalid"):
            lines += query_method(vector, index)
        lines += case_method(vector, index, index)
    negative = controls(vectors)
    for offset, (_, vector, query_index) in enumerate(negative, len(vectors)):
        lines += case_method(vector, offset, query_index)
    lines += ["\tstatic bool RunAll()", "\t{", "\t\tRICON_L0Run.s_fMaxDeviationDb = 0;",
              f'\t\tPrint("[RICON_L0] source={SOURCE_SHA}");', "\t\tbool ok = true;",
              "\t\tint cases = 0;", "\t\tint quietPositive = 0;", "\t\tint rejected = 0;", "\t\tRICON_L0Run t;"]
    for index, vector in enumerate(vectors):
        lines += [f'\t\tt = new RICON_L0Run("{vector["id"]}");', f"\t\tCase{index}(t);",
                  "\t\tif (t.IsPassed())", "\t\t\tcases++;", "\t\telse", "\t\t\tok = false;",
                  f'\t\tt = new RICON_L0Run("{vector["id"]}", true);', f"\t\tCase{index}(t);",
                  "\t\tif (t.IsPassed())", "\t\t\tquietPositive++;", "\t\telse", "\t\t\tok = false;"]
    for offset, (label, _, _) in enumerate(negative, len(vectors)):
        lines += [f'\t\tt = new RICON_L0Run("X_{label}", true);', f"\t\tCase{offset}(t);",
                  "\t\tif (t.IsFailed())", "\t\t\trejected++;", "\t\telse", "\t\t\tok = false;"]
    lines += ['\t\tt = new RICON_L0Run("X_nonfinite_actual", true);',
              '\t\tt.CheckNumber("finite", RICON_L0Run.Special(0), 0, 0.01);',
              "\t\tok = t.IsFailed() && ok;",
              '\t\tPrintFormat("[RICON_L0] cases=%1 quietPositive=%2 controls=%3 unobservableFields=4 maxDeviationDb=%4",',
              "\t\t\tcases, quietPositive, rejected, RICON_L0Run.s_fMaxDeviationDb.ToString(-1, 9));",
              "\t\treturn ok && cases == 57 && quietPositive == 57 && rejected == 8;", "\t}", "}", ""]
    return "\n".join(lines)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args(argv)
    try:
        result = render(args.source.read_bytes())
        if args.check:
            if args.destination.read_text(encoding="utf-8") != result:
                raise ValueError("Сохранённая фикстура отличается от генератора")
        else:
            args.destination.write_text(result, encoding="utf-8", newline="\n")
    except (OSError, ValueError) as exc:
        print(f"L0_GENERATOR_FAIL: {exc}")
        return 1
    print("L0_GENERATOR_PASS: 57 случаев, 4 недоступных поля effectiveDistanceM")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
