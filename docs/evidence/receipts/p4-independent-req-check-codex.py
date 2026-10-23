#!/usr/bin/env python3
"""Независимый пересчёт P4 по REQ v0.3 и первоисточникам, без импорта эталона автора."""
from __future__ import annotations

import argparse
import copy
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import sys


P2_PATH = Path(__file__).with_name("p2-independent-req-check-codex.py")
sys.dont_write_bytecode = True
SPEC = importlib.util.spec_from_file_location("codex_p2_check", P2_PATH)
P2 = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(P2)
SHA = "d130f2f085cc333f2e80ff6b61e06ab1442880d82758b68db648cf3185777560"
PROVENANCE = ("MEASURED_TABLE", "STANDARD_FIGURE", "ASSUMED_CONFIG")
JB_FREQ = (25, 50, 100, 250, 400)
# ECAC ESD-TR-81-101, табл. 13: alpha, A, B; расстояние в статутных милях.
JB = {
    "V": ((0, 0, .00212), (0, 0, .00106), (.045, .615, .000529),
          (.050, .759, .000443), (.055, 1.02, .000523)),
    "H": ((0, 0, .00424), (0, 0, .00424), (.020, .472, .00551),
          (.025, .774, .000588), (.035, 1.11, .000598)),
}


def profile(value):
    if type(value) is not dict or type(value.get("negligible")) is not bool:
        raise ValueError("Неверный профиль")
    if value["negligible"]:
        P2.fields(value, "negligible")
        return value.copy()
    P2.fields(value, "negligible climate leafState density moisture gammaTables gammaProvenance am")
    for key, choices in (
        ("climate", ("TEMPERATE", "TROPICAL", "UNSPECIFIED")),
        ("leafState", ("IN_LEAF", "LEAFLESS", "UNKNOWN")),
        ("density", ("DENSE", "SPARSE", "UNKNOWN")),
        ("moisture", ("DRY", "WET", "UNKNOWN")),
        ("gammaProvenance", PROVENANCE),
    ):
        if value[key] not in choices:
            raise ValueError("Неверное поле профиля")
    tables = value["gammaTables"]
    if (type(tables) is not dict or not tables or set(tables) - {"V", "H", "ANY"}
            or ("ANY" in tables and len(tables) != 1)):
        raise ValueError("Неверный набор таблиц")
    typed_tables = {}
    for pol, points in tables.items():
        if type(points) is not list or not 2 <= len(points) <= 32:
            raise ValueError("Неверная длина таблицы")
        typed_points, previous = [], 0
        for point in points:
            if type(point) is not list or len(point) != 2:
                raise ValueError("Неверная точка таблицы")
            frequency, gamma = map(P2.number, point)
            if not frequency > previous or not gamma > 0:
                raise ValueError("Неверный домен таблицы")
            typed_points.append((frequency, gamma))
            previous = frequency
        typed_tables[pol] = typed_points
    am = value["am"]
    if am is not None:
        P2.fields(am, "a1 alpha minMHz maxMHz provenance")
        am = dict(am, **{key: P2.number(am[key]) for key in ("a1", "alpha", "minMHz", "maxMHz")})
        if not (am["a1"] > 0 and 0 < am["minMHz"] <= am["maxMHz"] and am["provenance"] in PROVENANCE):
            raise ValueError("Неверный Am")
    return dict(value, gammaTables=typed_tables, am=am)


def gamma_at(p, frequency, pol):
    points = p["gammaTables"].get(pol, p["gammaTables"].get("ANY"))
    if points:
        for f, gamma in points:
            if f == frequency:
                return gamma, "TABLE_POINT"
        for (f0, g0), (f1, g1) in zip(points, points[1:]):
            if f0 < frequency < f1:
                fraction = math.log(frequency / f0) / math.log(f1 / f0)
                return math.exp(math.log(g0) + fraction * math.log(g1 / g0)), "INTERPOLATED"
    return None, "NONE"


def jansky_bailey(frequency, depth, pol):
    values = JB[pol]
    if frequency in JB_FREQ:
        alpha, a, b = values[JB_FREQ.index(frequency)]
    else:
        index = next(i for i in range(4) if JB_FREQ[i] < frequency < JB_FREQ[i + 1])
        t = math.log(frequency / JB_FREQ[index]) / math.log(JB_FREQ[index + 1] / JB_FREQ[index])
        alpha, a, b = (x + (y - x) * t for x, y in zip(values[index], values[index + 1]))
    miles = depth / 1609.344
    # После вычитания FSPL частота сокращается; log-sum-exp не теряет малое слагаемое.
    terms = [math.log(b / miles)]
    if a:
        terms.append(math.log(a) - 1609 * alpha * miles)
    peak = max(terms)
    return -20 / math.log(10) * (peak + math.log(sum(math.exp(v - peak) for v in terms)))


def interval_result(start, end, material, length, frequency, pol, heights, p):
    depth = end - start
    touches = int(start == 0) + int(end == length)
    scenario = ("S3+" if depth > 400 else "S3") if touches == 0 else ("S1" if touches == 1 else "S2")
    gamma, gamma_source = gamma_at(p, frequency, pol)
    candidates = []
    if scenario == "S2" and p["climate"] == "TROPICAL" and 25 <= frequency <= 400 and 8 <= depth <= 1600:
        if all(h is not None and 2 <= h <= 7 for h in heights):
            aj = jansky_bailey(frequency, depth, pol)
            if aj >= 0:  # Текущее правило REQ; физическая трактовка оспорена в Agora #3551.
                candidates.append({"model": "M4", "lossDb": aj})
    med_conditions = (p["climate"], p["leafState"], p["density"], p["moisture"])
    if scenario != "S3+" and 230 <= frequency <= 95000 and depth <= 400 and med_conditions == ("TEMPERATE", "IN_LEAF", "DENSE", "DRY"):
        med = (0.45 * depth if depth < 14 else 1.33 * depth ** .588) * (frequency / 1000) ** .284
        candidates.append({"model": "M3", "lossDb": med})
    am = p["am"]
    if scenario == "S1" and gamma is not None and am is not None and am["minMHz"] <= frequency <= am["maxMHz"]:
        maximum = am["a1"] * frequency ** am["alpha"]
        attenuation = maximum * -math.expm1(-depth * gamma / maximum)
        candidates.append({"model": "M1", "lossDb": attenuation})
    if scenario == "S3" and frequency <= 1000 and gamma is not None:
        candidates.append({"model": "M2", "lossDb": depth * gamma})
    if scenario in ("S1", "S2") and depth <= 400 and gamma is not None:
        candidates.append({"model": "M5", "lossDb": depth * gamma})
    if not candidates:
        return {"code": "NO_MODEL_FOR_SCENARIO", "interval": {"startM": start, "endM": end, "scenario": scenario}}
    model, loss = candidates[0]["model"], candidates[0]["lossDb"]
    losses = [c["lossDb"] for c in candidates]
    return {
        "startM": start, "endM": end, "materialId": material, "scenario": scenario,
        "primaryModel": model, "primaryLossDb": loss,
        "depthTrend": {"M1": "SATURATING", "M2": "LINEAR", "M3": "SUBLINEAR", "M4": "EMPIRICAL", "M5": "LINEAR"}[model],
        "excessBaseline": "FREE_SPACE" if model == "M4" else "OTHER_MECHANISMS",
        "candidates": candidates, "spreadMinDb": min(losses), "spreadMaxDb": max(losses),
        "exdDiagnosticDb": .26 * (frequency / 1000) ** .77 * depth,
        "gammaDbPerM": gamma, "gammaSource": gamma_source,
        "gammaProvenance": p["gammaProvenance"] if gamma is not None else None,
        "amProvenance": am["provenance"] if am is not None else None, "capApplied": False,
    }


def check_finite(value):
    if type(value) is dict:
        for item in value.values():
            check_finite(item)
    elif type(value) is list:
        for item in value:
            check_finite(item)
    elif type(value) in (float, int):
        if not math.isfinite(value) or not 0 <= value <= P2.FLOAT_MAX:
            raise ArithmeticError("Результат вне неотрицательного binary32")


def compare(actual, expected, path="result"):
    if type(expected) is dict:
        assert type(actual) is dict and set(actual) == set(expected), f"{path}: различаются поля"
        for key in expected:
            compare(actual[key], expected[key], f"{path}.{key}")
    elif type(expected) is list:
        assert type(actual) is list and len(actual) == len(expected), f"{path}: различаются списки"
        for i, (a, b) in enumerate(zip(actual, expected)):
            compare(a, b, f"{path}[{i}]")
    elif type(expected) in (int, float):
        assert type(actual) in (int, float) and math.isclose(actual, expected, rel_tol=1e-9, abs_tol=1e-9), f"{path}: {actual!r} != {expected!r}"
    else:
        assert type(actual) is type(expected) and actual == expected, f"{path}: {actual!r} != {expected!r}"


def evaluate(value):
    try:
        observation, length = P2.observation(value["observation"])
    except (KeyError, ValueError, TypeError, OverflowError):
        return {"code": "INVALID_OBSERVATION"}
    try:
        P2.fields(value, "observation catalog polarization txHeightM rxHeightM")
        catalog = value["catalog"]
        if type(catalog) is not dict or any(type(k) is not str or not P2.KEY.fullmatch(k) for k in catalog):
            raise ValueError("Неверный каталог")
        catalog = {key: profile(p) for key, p in catalog.items()}
        pol = value["polarization"]
        if pol not in ("V", "H"):
            raise ValueError("Неверная поляризация")
        heights = [None if value[k] is None else P2.number(value[k]) for k in ("txHeightM", "rxHeightM")]
        if any(h is not None and h < 0 for h in heights):
            raise ValueError("Отрицательная высота")
    except (KeyError, ValueError, TypeError, OverflowError):
        return {"code": "INVALID_INPUT"}
    if observation["coverage"]["VEGETATION"] != "COMPLETE":
        return {"code": "VEGETATION_COVERAGE_INCOMPLETE"}
    segments = [s for s in observation["segments"] if s["family"] == "VEGETATION"]
    if any(s["knowledge"] not in ("PROFILE", "RULE") or s["materialId"] not in catalog for s in segments):
        return {"code": "VEGETATION_UNKNOWN"}
    segments = [s for s in segments if not catalog[s["materialId"]]["negligible"]]
    if not segments:
        return {"code": "NO_VEGETATION", "lossDb": 0.0}
    intervals = []
    for s in segments:
        if intervals and s["startM"] <= intervals[-1][1]:
            intervals[-1][1] = max(intervals[-1][1], s["endM"])
            intervals[-1][2].add(s["materialId"])
        else:
            intervals.append([s["startM"], s["endM"], {s["materialId"]}])
    if any(len(materials) != 1 for _, _, materials in intervals):
        return {"code": "VEGETATION_MIXED_PROFILE"}
    results, numeric_failure = [], False
    for start, end, materials in intervals:
        material = next(iter(materials))
        try:
            result = interval_result(start, end, material, length, observation["frequencyMHz"], pol, heights, catalog[material])
            if result.get("code") == "NO_MODEL_FOR_SCENARIO":
                return result
            check_finite(result)
            results.append(result)
        except (ArithmeticError, ValueError):
            numeric_failure = True
    if numeric_failure:
        return {"code": "NUMERIC_FAILURE"}
    total = sum(i["primaryLossDb"] for i in results)
    if not math.isfinite(total) or total > P2.FLOAT_MAX:
        return {"code": "NUMERIC_FAILURE"}
    return {"code": "OK", "lossDb": total, "intervals": results}


def frequency_matrix(base):
    # Ожидаемые списки переписаны из VG10a, не получены из функции выбора.
    temperate = {
        30: ("M5", "", "M5", "", "M2"),
        60: ("M5", "", "M5", "", "M2"),
        88: ("M5", "", "M5", "", "M2"),
        150: ("M1 M5", "M1", "M5", "", "M2"),
        230: ("M3 M1 M5", "M1", "M3 M5", "", "M3 M2"),
        466: ("M3 M1 M5", "M1", "M3 M5", "", "M3 M2"),
        1500: ("M3 M1 M5", "M1", "M3 M5", "", "M3"),
        3000: ("M3 M5", "", "M3 M5", "", "M3"),
        30000: ("M3 M5", "", "M3 M5", "", "M3"),
    }
    tropical = {
        f: ("M5", "", "M4 M5" if f <= 400 else "M5", "M4" if f <= 400 else "", "M2" if f <= 1000 else "")
        for f in temperate
    }
    settings = ((0, 100, 1000), (0, 600, 1000), (0, 300, 300), (0, 800, 800), (400, 420, 1000))
    count = 0
    for climate, expected in (("TEMPERATE", temperate), ("TROPICAL", tropical)):
        raw_profile = copy.deepcopy(base["catalog"]["forest.temperate_fig2"])
        raw_profile["climate"] = climate
        if climate == "TEMPERATE":
            raw_profile["am"] = copy.deepcopy(base["catalog"]["forest.mixed_spb"]["am"])
        p = profile(raw_profile)
        for frequency, cells in expected.items():
            for (start, end, length), models in zip(settings, cells):
                result = interval_result(start, end, "test.matrix", length, frequency, "V", (3, 3), p)
                got = [c["model"] for c in result.get("candidates", [])]
                assert got == models.split(), (climate, frequency, start, end, got, models)
                if not models:
                    assert result["code"] == "NO_MODEL_FOR_SCENARIO"
                count += 1
    return count


def self_test(cases):
    base = next(c["input"] for c in cases if c["id"] == "V03")
    actual = evaluate(base)
    assert actual["code"] == "OK" and actual["intervals"][0]["primaryModel"] == "M3"
    controls = []
    for key, replacement in (("code", "NO_VEGETATION"), ("lossDb", 0), ("intervals", [])):
        changed = copy.deepcopy(actual)
        changed[key] = replacement
        controls.append(changed)
    for key, replacement in (("primaryModel", "M5"), ("capApplied", True), ("capApplied", 0), ("excessBaseline", "FREE_SPACE"),
                             ("gammaSource", "NONE"), ("gammaProvenance", "ASSUMED_CONFIG"),
                             ("spreadMaxDb", 0), ("candidates", []), ("startM", 1), ("amProvenance", None)):
        changed = copy.deepcopy(actual)
        changed["intervals"][0][key] = replacement
        controls.append(changed)
    caught = 0
    for changed in controls:
        try:
            compare(changed, actual)
        except AssertionError:
            caught += 1
    assert caught == len(controls)
    assert math.isclose(jansky_bailey(25, 8, "V"), 20 * math.log10((8 / 1609.344) / .00212), abs_tol=1e-12)
    assert jansky_bailey(100, 10, "H") < 0
    deep = copy.deepcopy(base)
    deep["observation"]["frequencyMHz"] = 60
    deep["observation"]["segments"][0]["endM"] = 401
    deep["observation"]["segments"][0]["materialId"] = "woodland.p833_fig2"
    assert evaluate(deep)["code"] == "NO_MODEL_FOR_SCENARIO"
    return caught, frequency_matrix(base)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("corpus", type=Path)
    parser.add_argument("--receipt", type=Path)
    args = parser.parse_args()
    raw = args.corpus.read_bytes()
    sha = hashlib.sha256(raw).hexdigest()
    if sha != SHA:
        raise SystemExit("SHA корпуса изменён; требуется новое согласование")
    cases = json.loads(raw)["cases"]
    assert len(cases) == 54 and [c["id"] for c in cases] == [f"V{i:02}" for i in range(1, 55)]
    controls, matrix_cells = self_test(cases)
    errors, models, codes = [], set(), set()
    for case in cases:
        original = copy.deepcopy(case["input"])
        actual = evaluate(case["input"])
        codes.add(actual["code"])
        models.update(c["model"] for i in actual.get("intervals", []) for c in i["candidates"])
        try:
            compare(actual, case["expect"])
            assert case["input"] == original, "Вход изменён"
        except AssertionError as exc:
            errors.append({"id": case["id"], "error": str(exc)})
    report = {"checker": "codex: REQ v0.3, JSON и первоисточники; без эталона автора", "corpusSha256": sha,
              "cases": len(cases), "matched": len(cases) - len(errors), "errors": errors,
              "negativeControls": controls, "frequencyMatrixCells": matrix_cells,
              "models": sorted(models), "codes": sorted(codes),
              "physicalAcceptance": False, "freeze": "PENDING_REVIEW"}
    if args.receipt:
        args.receipt.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, ensure_ascii=False, indent=2))
    print("P4_INDEPENDENT_CHECK_PASS" if not errors else "P4_INDEPENDENT_CHECK_FAIL")
    return bool(errors)


if __name__ == "__main__":
    raise SystemExit(main())
