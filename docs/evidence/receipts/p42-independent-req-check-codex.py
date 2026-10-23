#!/usr/bin/env python3
"""Независимая проверка P4.2 по REQ/корпусу и P.2108-1; эталон автора не читается и не импортируется."""
from __future__ import annotations

import argparse
import copy
import hashlib
import importlib.util
import json
import math
from pathlib import Path


SPEC = importlib.util.spec_from_file_location("codex_p4", Path(__file__).with_name("p4-independent-req-check-codex.py"))
P4 = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(P4)
P2 = P4.P2
SHA = "00d7da2115f5d27445ae40502ef6fa3541e027e85a4275b7ba80264664a99cf2"


def j_nu(nu):
    # asinh(x) = ln(x + sqrt(x*x+1)): та же формула (1), другой численный путь.
    return 0.0 if nu <= -.78 else 6.9 + 20 / math.log(10) * math.asinh(nu - .1)


def terminal_loss(frequency, height, canopy):
    if height >= canopy:
        return dict(hdifM=0.0, thetaDeg=0.0, nu=0.0, jDb=0.0, ahDb=0.0)
    hdif = canopy - height
    theta = math.degrees(math.atan(hdif / 27))
    product = hdif * theta
    if not math.isfinite(product) or product > P2.FLOAT_MAX:
        raise ArithmeticError("Произведение вне binary32")
    nu = .342 * math.sqrt(frequency / 1000) * math.sqrt(product)
    j = j_nu(nu)
    result = dict(hdifM=hdif, thetaDeg=theta, nu=nu, jDb=j, ahDb=j - 6.03)
    P4.check_finite(result)
    return result


def evaluate(value):
    try:
        observation, length = P2.observation(value["observation"])
    except (KeyError, TypeError, ValueError, OverflowError):
        return {"code": "INVALID_OBSERVATION"}
    try:
        P2.fields(value, "observation catalog polarization txHeightM rxHeightM canopy")
        catalog = value["catalog"]
        if type(catalog) is not dict or any(type(k) is not str or not P2.KEY.fullmatch(k) for k in catalog):
            raise ValueError("Неверный каталог")
        catalog = {key: P4.profile(p) for key, p in catalog.items()}
        if value["polarization"] not in ("V", "H"):
            raise ValueError("Неверная поляризация")
        heights = [None if value[k] is None else P2.number(value[k]) for k in ("txHeightM", "rxHeightM")]
        if any(h is not None and h < 0 for h in heights):
            raise ValueError("Отрицательная высота антенны")
        if type(value["canopy"]) is not dict:
            raise ValueError("Неверный каталог крон")
        canopy = {}
        for material, entry in value["canopy"].items():
            if material not in catalog or catalog[material]["negligible"]:
                raise ValueError("Крона без подходящего материала")
            P2.fields(entry, "heightM provenance")
            provenance = entry["provenance"]
            if provenance == "STANDARD_DEFAULT":
                if entry["heightM"] is not None:
                    raise ValueError("Явная высота для стандартного значения")
                height = 15.0
            elif provenance in ("MEASURED", "ASSUMED_CONFIG"):
                height = P2.number(entry["heightM"])
                if height <= 0:
                    raise ValueError("Неположительная высота крон")
            else:
                raise ValueError("Неизвестное происхождение высоты")
            canopy[material] = (height, provenance)
    except (KeyError, TypeError, ValueError, OverflowError):
        return {"code": "INVALID_INPUT"}
    if observation["coverage"]["VEGETATION"] != "COMPLETE":
        return {"code": "VEGETATION_COVERAGE_INCOMPLETE"}
    segments = [s for s in observation["segments"] if s["family"] == "VEGETATION"]
    if any(s["knowledge"] not in ("PROFILE", "RULE") or s["materialId"] not in catalog for s in segments):
        return {"code": "VEGETATION_UNKNOWN"}
    segments = [s for s in segments if not catalog[s["materialId"]]["negligible"]]
    if not segments:
        return {"code": "NO_VEGETATION"}
    intervals = []
    for s in segments:
        if intervals and s["startM"] <= intervals[-1][1]:
            intervals[-1][1] = max(intervals[-1][1], s["endM"])
            intervals[-1][2].add(s["materialId"])
        else:
            intervals.append([s["startM"], s["endM"], {s["materialId"]}])
    if any(len(materials) != 1 for _, _, materials in intervals):
        return {"code": "VEGETATION_MIXED_PROFILE"}
    terminal_intervals = []
    if intervals[0][0] == 0:
        terminal_intervals.append(("TX", heights[0], intervals[0]))
    if intervals[-1][1] == length:
        terminal_intervals.append(("RX", heights[1], intervals[-1]))
    if not terminal_intervals:
        return {"code": "NO_TERMINAL_IN_VEGETATION"}
    if any(next(iter(i[2])) not in canopy for _, _, i in terminal_intervals):
        return {"code": "INVALID_INPUT"}
    frequency = observation["frequencyMHz"]
    if not 30 <= frequency <= 3000 or any(h is None for _, h, _ in terminal_intervals):
        return {"code": "OUT_OF_MODEL_DOMAIN"}
    terminals = []
    try:
        for end, antenna, (start, stop, materials) in terminal_intervals:
            material = next(iter(materials))
            height, provenance = canopy[material]
            terminals.append(dict(end=end, intervalStartM=start, intervalEndM=stop, materialId=material,
                                  antennaHeightM=antenna, canopyHeightM=height, canopyProvenance=provenance,
                                  **terminal_loss(frequency, antenna, height)))
        total = sum(t["ahDb"] for t in terminals)
        P4.check_finite(total)
    except (ArithmeticError, ValueError):
        return {"code": "NUMERIC_FAILURE"}
    return dict(code="OK", model="M6", status="SOURCE_DOMAIN", budgetMode="BASE_AT_CANOPY_HEIGHT",
                integrationClosed=False, totalAhDb=total, terminals=terminals)


def self_test(cases):
    base = copy.deepcopy(cases[0]["input"])
    actual = evaluate(base)
    controls = []
    for key, replacement in (("code", "LOST"), ("model", "M5"), ("status", "ASSUMED_EXTENSION"),
                             ("budgetMode", "ADD_TO_THROUGH"), ("integrationClosed", True),
                             ("integrationClosed", 0), ("totalAhDb", 0), ("terminals", [])):
        changed = copy.deepcopy(actual)
        changed[key] = replacement
        controls.append(changed)
    for key in actual["terminals"][0]:
        changed = copy.deepcopy(actual)
        changed["terminals"][0][key] = "BAD" if type(changed["terminals"][0][key]) is str else 999
        controls.append(changed)
    caught = 0
    for changed in controls:
        try:
            P4.compare(changed, actual)
        except AssertionError:
            caught += 1
    assert caught == len(controls)
    assert j_nu(-.78) == 0
    assert math.isclose(j_nu(0), 6.032852208563606, abs_tol=1e-12)
    properties = 0
    for frequency in (30, 60, 88, 150, 1000, 3000):
        losses = [terminal_loss(frequency, h, 15)["ahDb"] for h in (0, 2, 10, 14.9, 15, 20)]
        assert all(a >= b >= 0 for a, b in zip(losses, losses[1:]))
        properties += 5
    for height in (0, 2, 10, 14.9):
        losses = [terminal_loss(f, height, 15)["ahDb"] for f in (30, 60, 88, 150, 1000, 3000)]
        assert all(a <= b for a, b in zip(losses, losses[1:]))
        properties += 5
    # Приоритеты проверяются по входам, а не по ожидаемому коду существующего корпуса.
    priorities = [
        (lambda x: x.update(canopy={}), "INVALID_INPUT"),
        (lambda x: x.update(txHeightM=None), "OUT_OF_MODEL_DOMAIN"),
        (lambda x: x["canopy"]["forest.temperate_fig2"].update(heightM=3e38, provenance="MEASURED"), "NUMERIC_FAILURE"),
    ]
    for change, code in priorities:
        x = copy.deepcopy(base)
        change(x)
        assert evaluate(x) == {"code": code}
    x = copy.deepcopy(cases[1]["input"])
    x["canopy"]["forest.temperate_fig2"].update(heightM=3e38, provenance="MEASURED")
    x["rxHeightM"] = None
    assert evaluate(x) == {"code": "OUT_OF_MODEL_DOMAIN"}
    x["canopy"] = {}
    assert evaluate(x) == {"code": "INVALID_INPUT"}
    x = copy.deepcopy(base)
    x["observation"]["coverage"]["VEGETATION"] = "PARTIAL"
    x["observation"]["segments"][0].update(knowledge="UNKNOWN", materialId="")
    assert evaluate(x) == {"code": "VEGETATION_COVERAGE_INCOMPLETE"}
    x["polarization"] = "BAD"
    assert evaluate(x) == {"code": "INVALID_INPUT"}
    return caught, properties, 7


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("corpus", type=Path)
    parser.add_argument("--receipt", type=Path)
    args = parser.parse_args()
    raw = args.corpus.read_bytes()
    sha = hashlib.sha256(raw).hexdigest()
    assert sha == SHA, "Корпус изменён: требуется новое согласование"
    doc = json.loads(raw)
    cases = doc["cases"]
    assert len(cases) == 31 and [c["id"] for c in cases] == [f"O{i:02}" for i in range(1, 32)]
    assert doc["hostOnly"] == ["O28"]
    controls, properties, priorities = self_test(cases)
    errors, codes = [], set()
    for case in cases:
        before = copy.deepcopy(case["input"])
        actual = evaluate(case["input"])
        codes.add(actual["code"])
        try:
            P4.compare(actual, case["expect"])
            assert before == case["input"], "Вход изменён"
        except AssertionError as exc:
            errors.append(dict(id=case["id"], error=str(exc)))
    report = dict(checker="codex: только REQ v0.1, JSON и P.2108-1; без эталона автора",
                  corpusSha256=sha, cases=31, matched=31-len(errors), errors=errors, negativeControls=controls,
                  monotonicityChecks=properties, priorityChecks=priorities, codes=sorted(codes),
                  native=30, hostOnly=["O28"], integrationClosed=False, physicalAcceptance=False)
    if args.receipt:
        args.receipt.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps(report, ensure_ascii=False, indent=2))
    print("P42_INDEPENDENT_CHECK_PASS" if not errors else "P42_INDEPENDENT_CHECK_FAIL")
    return bool(errors)


if __name__ == "__main__":
    raise SystemExit(main())
