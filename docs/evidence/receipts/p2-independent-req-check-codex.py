#!/usr/bin/env python3
"""Независимый пересчёт P2 по REQ и P.527-6; эталон партнёра не импортируется."""
from __future__ import annotations

import cmath
import copy
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import struct


FAMILIES = ("TERRAIN", "WATER", "STRUCTURE", "VEGETATION", "VEHICLE")
KEY = re.compile(r"[A-Za-z0-9_.:-]{1,64}\Z")
FLOAT_MAX = struct.unpack("!f", bytes.fromhex("7f7fffff"))[0]


def number(value):
    if type(value) not in (int, float):
        raise ValueError("Требуется число, исключая bool")
    try:
        typed = struct.unpack("!f", struct.pack("!f", value))[0]
    except (OverflowError, struct.error):
        raise ValueError("Переполнение binary32") from None
    if not math.isfinite(typed):
        raise ValueError("Неконечное число")
    return typed


def fields(value, names):
    if type(value) is not dict or set(value) != set(names.split()):
        raise ValueError("Неверные поля")


def endpoint(value):
    fields(value, "position medium depthM")
    if type(value["position"]) is not list or len(value["position"]) != 3:
        raise ValueError("Неверная позиция")
    position = [number(x) for x in value["position"]]
    depth = number(value["depthM"])
    medium = value["medium"]
    if medium not in ("AIR", "WATER", "UNKNOWN") or depth < 0 or (medium != "WATER" and depth != 0):
        raise ValueError("Неверная среда или глубина")
    return {"position": position, "medium": medium, "depthM": depth}


def distance32(tx, rx):
    delta = [number(b - a) for a, b in zip(tx, rx)]
    scale = max(map(abs, delta))
    if scale == 0:
        raise ValueError("Совпадающие антенны")
    x, y, z = [number(x / scale) for x in delta]
    squares = [number(x * x), number(y * y), number(z * z)]
    square_sum = number(number(squares[0] + squares[1]) + squares[2])
    return number(scale * number(math.sqrt(square_sum)))


def observation(value):
    """PO03–PO07: общие ограничения; водная согласованность проверяется отдельно."""
    fields(value, "frequencyMHz environmentRevision tx rx coverage segments")
    revision = value["environmentRevision"]
    if type(revision) is not int or not 1 <= revision <= 2147483647:
        raise ValueError("Неверная ревизия int32")
    frequency = number(value["frequencyMHz"])
    if frequency <= 0:
        raise ValueError("Неверная частота")
    tx, rx = endpoint(value["tx"]), endpoint(value["rx"])
    length = distance32(tx["position"], rx["position"])
    if not 0 < length <= FLOAT_MAX:
        raise ValueError("Неверная длина")
    coverage = value["coverage"]
    fields(coverage, " ".join(FAMILIES))
    if any(x not in ("COMPLETE", "PARTIAL", "NOT_OBSERVED") for x in coverage.values()):
        raise ValueError("Неверное покрытие")
    if type(value["segments"]) is not list or len(value["segments"]) > 1024:
        raise ValueError("Неверный список отрезков")
    result, last_end, previous = [], {}, None
    for item in value["segments"]:
        fields(item, "family startM endM entityKey knowledge materialId openings")
        family, key = item["family"], item["entityKey"]
        if family not in FAMILIES[1:] or type(key) is not str or not KEY.fullmatch(key):
            raise ValueError("Неверное семейство или ключ")
        start, end = number(item["startM"]), number(item["endM"])
        if not 0 <= start < end <= length:
            raise ValueError("Неверные границы отрезка")
        order = (start, FAMILIES.index(family), key)
        if previous is not None and order < previous:
            raise ValueError("Неверный порядок отрезков")
        if start < last_end.get(key, -1):
            raise ValueError("Перекрытие одного объекта")
        previous, last_end[key] = order, end
        knowledge, material = item["knowledge"], item["materialId"]
        if knowledge not in ("PROFILE", "RULE", "HINT_ONLY", "UNKNOWN") or type(material) is not str:
            raise ValueError("Неверное знание материала")
        if knowledge in ("PROFILE", "RULE"):
            if not KEY.fullmatch(material):
                raise ValueError("Отсутствует RF-профиль")
        elif material != "":
            raise ValueError("Профиль без подтверждённого знания")
        if type(item["openings"]) is not bool:
            raise ValueError("Неверный признак проёма")
        result.append(dict(item, startM=start, endM=end))
    intervals = water_union(result)
    if coverage["WATER"] == "COMPLETE":
        for end, touches in ((tx, bool(intervals and intervals[0][0] == 0)),
                             (rx, bool(intervals and intervals[-1][1] == length))):
            if end["medium"] != "UNKNOWN" and (end["medium"] == "WATER") != touches:
                raise ValueError("Вода у антенны противоречит полному наблюдению пути")
    return dict(value, frequencyMHz=frequency, tx=tx, rx=rx, segments=result), length


def water_constants(frequency_mhz, temperature, salinity):
    """P.527-6 (8)–(27), независимое вычисление через комплексную сумму Дебая."""
    f, t, s = frequency_mhz / 1000, temperature, salinity
    theta = 300 / (t + 273.15) - 1
    es = 77.66 + 103.3 * theta
    e1, einf = 0.0671 * es, 3.52 - 7.52 * theta
    f1 = 20.20 - 146.4 * theta + 316 * theta ** 2
    f2 = 39.8 * f1
    ess = es * math.exp(-3.33330e-3 * s + 4.74868e-6 * s ** 2)
    f1s = f1 * (1 + s * (2.3232e-3 - 7.9208e-5 * t + 3.6764e-6 * t ** 2
                         + 3.5594e-7 * t ** 3 + 8.9795e-9 * t ** 4))
    e1s = e1 * math.exp(-6.28908e-3 * s + 1.76032e-4 * s ** 2 - 9.22144e-5 * t * s)
    f2s = f2 * (1 + s * (-1.99723e-2 + 1.81176e-4 * t))
    einfs = einf * (1 + s * (-2.04265e-3 + 1.57883e-4 * t))
    sigma35 = (2.903602 + 8.607e-2 * t + 4.738817e-4 * t ** 2
               - 2.991e-6 * t ** 3 + 4.3047e-9 * t ** 4)
    r15 = s * (37.5109 + 5.45216 * s + 1.4409e-2 * s ** 2) / (1004.75 + 182.283 * s + s ** 2)
    a0 = (6.9431 + 3.2841 * s - 9.9486e-2 * s ** 2) / (84.850 + 69.024 * s + s ** 2)
    a1 = 49.843 - 0.2276 * s + 0.198e-2 * s ** 2
    sigma = sigma35 * r15 * (1 + a0 * (t - 15) / (a1 + t))
    epsilon = einfs + (ess - e1s) / (1 + 1j * f / f1s) + (e1s - einfs) / (1 + 1j * f / f2s)
    epsilon -= 1j * 18 * sigma / f
    n = cmath.sqrt(epsilon)
    alpha = 20 / math.log(10) * (2 * math.pi * frequency_mhz * 1e6 / 299792458) * abs(n.imag)
    # Та же 1-|Γ|² без вычитания близких чисел при большом |n|.
    transmission = 4 * n.real / abs(1 + n) ** 2
    interface = -10 * math.log10(transmission)
    return epsilon, sigma, alpha, interface


def water_union(segments):
    merged = []
    for item in segments:
        if item["family"] != "WATER":
            continue
        start, end = item["startM"], item["endM"]
        if merged and start <= merged[-1][1]:
            merged[-1][1] = max(merged[-1][1], end)
        else:
            merged.append([start, end])
    return merged


def evaluate(value, profile):
    try:
        observed, length = observation(value)
    except ValueError:
        return {"observationCode": "INVALID_OBSERVATION", "water": {"code": "INVALID_OBSERVATION"}}
    result = {"observationCode": "VALID"}

    def stop(code):
        return dict(result, water={"code": code})

    try:
        fields(profile, "tempC salinityPpt")
        temperature, salinity = number(profile["tempC"]), number(profile["salinityPpt"])
    except ValueError:
        return stop("INVALID_PROFILE")
    if observed["coverage"]["WATER"] != "COMPLETE" or any(observed[x]["medium"] == "UNKNOWN" for x in ("tx", "rx")):
        return stop("WATER_COVERAGE_INCOMPLETE")
    intervals = water_union(observed["segments"])
    if not intervals:
        return dict(result, water={"code": "NO_WATER", "lossDb": 0.0})
    if not -4 <= temperature <= 40 or not 0 <= salinity <= 40 or observed["frequencyMHz"] > 1e6:
        return stop("OUT_OF_MODEL_DOMAIN")
    try:
        epsilon, _, alpha, interface = water_constants(observed["frequencyMHz"], temperature, salinity)
        crossings = sum(int(a > 0) + int(b < length) for a, b in intervals)
        water_length = math.fsum(b - a for a, b in intervals)
        loss = alpha * water_length + crossings * interface
        numbers = (epsilon.real, -epsilon.imag, abs(epsilon), alpha, interface, water_length, loss)
        if any(not math.isfinite(x) or not 0 <= x <= FLOAT_MAX for x in numbers):
            return stop("NUMERIC_FAILURE")
    except (OverflowError, ValueError, ZeroDivisionError):
        return stop("NUMERIC_FAILURE")
    return dict(result, water={"code": "OK", "lossDb": loss, "alphaDbPerM": alpha, "interfaceDb": interface,
                               "waterLengthM": water_length, "crossings": crossings, "intervals": intervals})


def compare(actual, expected, path="result"):
    if type(expected) is dict:
        if type(actual) is not dict or set(actual) != set(expected):
            raise AssertionError(f"{path}: различаются поля {actual} / {expected}")
        for key in expected:
            compare(actual[key], expected[key], f"{path}.{key}")
    elif type(expected) is list:
        if type(actual) is not list or len(actual) != len(expected):
            raise AssertionError(f"{path}: различаются списки")
        for index, (a, b) in enumerate(zip(actual, expected)):
            compare(a, b, f"{path}[{index}]")
    elif type(expected) in (int, float):
        if type(actual) not in (int, float) or not math.isclose(actual, expected, rel_tol=1e-9, abs_tol=1e-9):
            raise AssertionError(f"{path}: {actual} != {expected}")
    elif actual != expected:
        raise AssertionError(f"{path}: {actual!r} != {expected!r}")


def self_test():
    # Опорные значения получены до чтения корпуса/эталона, по STD_COPY P.527-6.
    epsilon, sigma, alpha, interface = water_constants(100, 20, 35)
    assert math.isclose(epsilon.real, 71.670471776, abs_tol=1e-9)
    assert math.isclose(-epsilon.imag, 862.748817785, abs_tol=1e-9)
    assert math.isclose(sigma, 4.791266067, abs_tol=1e-9)
    assert math.isclose(alpha, 362.729133241, abs_tol=1e-9)
    assert 0 < interface < 20
    _, sigma0, alpha0, _ = water_constants(100, 20, 0)
    assert sigma0 == 0 and math.isclose(alpha0, 0.448537638, abs_tol=1e-9)
    base = {"frequencyMHz": 100, "environmentRevision": 2147483647,
            "tx": {"position": [0, 0, 0], "medium": "AIR", "depthM": 0},
            "rx": {"position": [100, 0, 0], "medium": "AIR", "depthM": 0},
            "coverage": dict.fromkeys(FAMILIES, "COMPLETE"), "segments": []}
    assert observation(base)[1] == 100
    for change in ({"environmentRevision": True}, {"frequencyMHz": True}, {"frequencyMHz": 1e39},
                   {"frequencyMHz": float("nan")}, {"unknown": 1}):
        try:
            observation(dict(base, **change))
        except ValueError:
            pass
        else:
            raise AssertionError(f"Принят неверный вход {change}")
    segments = [{"family": "WATER", "startM": a, "endM": b} for a, b in ((0, 10), (5, 15), (15, 20), (30, 40))]
    saved = copy.deepcopy(segments)
    assert water_union(segments) == [[0, 20], [30, 40]]
    assert segments == saved
    assert distance32([0, 0, 0], [1, 2, 0]) == number(math.sqrt(5))
    assert distance32([0, 0, 0], [1, 1, 0]) == number(math.sqrt(2))
    profile = {"tempC": 20, "salinityPpt": 0}
    assert evaluate(base, profile)["water"] == {"code": "NO_WATER", "lossDb": 0.0}
    wet = copy.deepcopy(base)
    wet["tx"]["medium"] = wet["rx"]["medium"] = "WATER"
    wet["tx"]["depthM"], wet["rx"]["depthM"] = 30, 40
    wet["segments"] = [{"family": "WATER", "startM": 0, "endM": 100, "entityKey": "water",
                        "knowledge": "UNKNOWN", "materialId": "", "openings": False}]
    underwater = evaluate(wet, profile)["water"]
    assert underwater["crossings"] == 0 and underwater["waterLengthM"] == 100
    assert math.isclose(underwater["lossDb"], alpha0 * 100, abs_tol=1e-9)
    partial = copy.deepcopy(wet)
    partial["coverage"]["WATER"], partial["segments"] = "PARTIAL", []
    assert evaluate(partial, profile)["water"]["code"] == "WATER_COVERAGE_INCOMPLETE"
    extreme = dict(wet, frequencyMHz=1e-35)
    assert evaluate(extreme, dict(profile, salinityPpt=35))["water"]["code"] == "NUMERIC_FAILURE"
    unknown = copy.deepcopy(wet)
    unknown["tx"].update(medium="UNKNOWN", depthM=0)
    assert evaluate(unknown, profile)["water"]["code"] == "WATER_COVERAGE_INCOMPLETE"
    assert evaluate(base, {"tempC": 100, "salinityPpt": 0})["water"]["code"] == "NO_WATER"
    assert evaluate(wet, {"tempC": 100, "salinityPpt": 0})["water"]["code"] == "OUT_OF_MODEL_DOMAIN"
    # Мутант сравнения результата обязан быть замечен.
    try:
        compare({"lossDb": 3.1}, {"lossDb": 3.2})
    except AssertionError:
        pass
    else:
        raise AssertionError("Сравнение пропустило неверные потери")


def check_corpus(path):
    raw = path.read_bytes()
    corpus = json.loads(raw)
    passed, failures = [], []
    for case in corpus["cases"]:
        inp = case["input"]
        saved = copy.deepcopy(inp)
        try:
            actual = evaluate(inp["observation"], inp["profile"])
            compare(actual, case["expect"], case["id"])
            assert inp == saved, "Изменён вход"
        except (AssertionError, ValueError, TypeError) as error:
            failures.append({"id": case["id"], "error": str(error)})
        else:
            passed.append(case["id"])
    return {"sourceSha256": hashlib.sha256(raw).hexdigest(), "cases": len(corpus["cases"]),
            "passed": len(passed), "failures": failures, "partnerOracleRead": False}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("corpus", nargs="?", type=Path)
    args = parser.parse_args()
    self_test()
    if args.corpus:
        receipt = check_corpus(args.corpus)
        print(json.dumps(receipt, ensure_ascii=False, indent=2))
        raise SystemExit(bool(receipt["failures"]))
    print("P2 независимый checker: self-test PASS; корпус ещё не заморожен")
