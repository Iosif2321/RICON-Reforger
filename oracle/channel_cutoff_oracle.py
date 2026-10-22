#!/usr/bin/env python3
"""Эталон P1 по REQ-channel-cutoff; оценивает границы, не моделирует речь и мир."""
from __future__ import annotations

import argparse
import copy
import json
import math
from pathlib import Path

from radio_device_oracle import f32

MAX = 2147483647
FIELDS = {"nowMs", "validFromMs", "validUntilMs", "environmentRevision", "boundRevision",
          "coverageComplete", "resourceExhausted", "toleranceDbm", "numericalGuardDb", "paths"}


def integer(value, minimum=0):
    return type(value) is int and minimum <= value <= MAX


def optional_float(value):
    return None if value is None else f32(value)


def evaluate(raw):
    """Полный preflight до любого разрешения отсечки; результат не ссылается на вход."""
    def result(decision, reason, upper=None):
        return {"decision": decision, "reason": reason,
                "usefulIsZero": decision == "BOUNDED", "upperDbm": upper}

    invalid = result("UNRESOLVED", "INVALID_INPUT")
    if type(raw) is not dict or set(raw) != FIELDS:
        return invalid
    data = copy.deepcopy(raw)
    try:
        data["toleranceDbm"] = optional_float(data["toleranceDbm"])
        data["numericalGuardDb"] = optional_float(data["numericalGuardDb"])
        if type(data["paths"]) is not list or len(data["paths"]) > 4096:
            return invalid
        for path in data["paths"]:
            if type(path) is not dict or set(path) != {"pathId", "upperDbm"}:
                return invalid
            path["upperDbm"] = optional_float(path["upperDbm"])
    except (ValueError, TypeError, OverflowError):
        return invalid
    if not all(integer(data[k]) for k in ("nowMs", "validFromMs", "validUntilMs")) \
            or not data["validFromMs"] < data["validUntilMs"] \
            or not all(integer(data[k], 1) for k in ("environmentRevision", "boundRevision")) \
            or not all(type(data[k]) is bool for k in ("coverageComplete", "resourceExhausted")):
        return invalid
    numbers = [data["toleranceDbm"], data["numericalGuardDb"]] + [p["upperDbm"] for p in data["paths"]]
    if any(v is not None and not math.isfinite(v) for v in numbers) \
            or (data["numericalGuardDb"] is not None and data["numericalGuardDb"] < 0):
        return invalid
    ids = [p["pathId"] for p in data["paths"]]
    if any(type(v) is not str or not v for v in ids) or len(set(ids)) != len(ids):
        return invalid
    if data["resourceExhausted"]:
        return result("UNRESOLVED", "RESOURCE_EXHAUSTED")
    if not data["coverageComplete"]:
        return result("UNRESOLVED", "INCOMPLETE_COVERAGE")
    if data["environmentRevision"] != data["boundRevision"] \
            or not data["validFromMs"] <= data["nowMs"] < data["validUntilMs"]:
        return result("UNRESOLVED", "STALE_BOUND")
    if not data["paths"]:
        return result("BOUNDED", "EMPTY_PATH_SET")
    powers = [p["upperDbm"] for p in data["paths"]]
    if None in powers:
        return result("CONTINUE", "MISSING_PATH_BOUND")
    if data["numericalGuardDb"] is None:
        return result("CONTINUE", "NUMERIC_UNCERTIFIED")
    peak = max(powers)
    upper = peak + 20 * math.log10(math.fsum(10 ** ((v - peak) / 20) for v in powers)) \
        + data["numericalGuardDb"]
    if not math.isfinite(upper) or not math.isfinite(f32(upper)):
        return result("UNRESOLVED", "NUMERIC_FAILURE")
    if data["toleranceDbm"] is None:
        return result("CONTINUE", "CUTOFF_DISABLED", upper)
    if upper < data["toleranceDbm"]:
        return result("BOUNDED", "BELOW_TOLERANCE", upper)
    return result("CONTINUE", "ABOVE_TOLERANCE", upper)


def baseline():
    return {"nowMs": 100, "validFromMs": 0, "validUntilMs": 200,
            "environmentRevision": 1, "boundRevision": 1, "coverageComplete": True,
            "resourceExhausted": False, "toleranceDbm": -120, "numericalGuardDb": 0,
            "paths": [{"pathId": "direct", "upperDbm": -150}]}


def cases():
    rows = []

    def add(title, change):
        inp = baseline()
        change(inp)
        rows.append({"id": f"C{len(rows)+1:02}", "title": title, "input": inp, "expect": evaluate(inp)})

    add("Вклад ниже явного допуска", lambda q: None)
    add("Равенство не отсекается", lambda q: q["paths"][0].update(upperDbm=-120))
    add("Последующее усиление уже включено в границу", lambda q: q["paths"][0].update(upperDbm=-90))
    add("Два слабых когерентных пути вместе значимы", lambda q: q.update(paths=[
        {"pathId": "a", "upperDbm": -125}, {"pathId": "b", "upperDbm": -125}]))
    add("Тридцать слабых путей проверяются суммарно", lambda q: q.update(paths=[
        {"pathId": str(i), "upperDbm": -140} for i in range(30)]))
    add("Стохастический хвост без строгой границы", lambda q: q["paths"][0].update(upperDbm=None))
    add("Неизвестный материал", lambda q: q.update(coverageComplete=False))
    add("Исчерпан ресурс наблюдений", lambda q: q.update(resourceExhausted=True))
    add("Смена среды отменяет границу", lambda q: q.update(environmentRevision=2))
    add("Начало интервала включено", lambda q: q.update(nowMs=0))
    add("Конец интервала исключён", lambda q: q.update(nowMs=200))
    add("Время до интервала", lambda q: q.update(validFromMs=101))
    add("Подтверждённый пустой набор", lambda q: q.update(paths=[]))
    add("Пустой набор без покрытия не доказывает ноль", lambda q: q.update(paths=[], coverageComplete=False))
    add("Отсечка выключена без допуска", lambda q: q.update(toleranceDbm=None))
    add("Нет сертификации численной ошибки", lambda q: q.update(numericalGuardDb=None))
    add("Запас исключает пограничную отсечку", lambda q: q.update(numericalGuardDb=31))
    add("Большой динамический диапазон", lambda q: q.update(paths=[
        {"pathId": "a", "upperDbm": -400}, {"pathId": "b", "upperDbm": 400}]))
    add("NaN в последней границе запрещает ранний успех", lambda q: q["paths"].append({"pathId": "x", "upperDbm": "NaN"}))
    add("Inf не является границей", lambda q: q["paths"][0].update(upperDbm="+Inf"))
    add("Отрицательный численный запас", lambda q: q.update(numericalGuardDb=-1))
    add("Повтор идентичности пути", lambda q: q["paths"].append(copy.deepcopy(q["paths"][0])))
    add("bool не время", lambda q: q.update(nowMs=True))
    add("bool не мощность", lambda q: q["paths"][0].update(upperDbm=True))
    add("Пустой интервал", lambda q: q.update(validUntilMs=0))
    add("Неизвестное поле", lambda q: q.update(receiverThresholdDbm=-100))
    add("Верхняя граница численного диапазона", lambda q: q.update(numericalGuardDb=3.4028234663852886e38,
        paths=[{"pathId": "a", "upperDbm": 3.4028234663852886e38}]))
    return rows


def check(data):
    if type(data) is not dict or set(data) != {"schema", "version", "note", "cases"} \
            or data["schema"] != "ricon.oracle.channel_cutoff" or type(data["version"]) is not int \
            or data["version"] != 1 or type(data["note"]) is not str or not data["note"] \
            or type(data["cases"]) is not list or not data["cases"]:
        return ["envelope"]
    errors, ids = [], set()
    for row in data["cases"]:
        if type(row) is not dict or set(row) != {"id", "title", "input", "expect"} \
                or type(row["id"]) is not str or not row["id"] or row["id"] in ids \
                or type(row["title"]) is not str or not row["title"]:
            errors.append("case")
            continue
        ids.add(row["id"])
        actual, expected = evaluate(row["input"]), row["expect"]
        if type(expected) is not dict or set(expected) != set(actual):
            errors.append(row["id"])
            continue
        for key in actual:
            a, b = actual[key], expected[key]
            if key == "upperDbm" and a is not None and type(b) in (int, float):
                valid = math.isfinite(b) and abs(a-b) <= 1e-6
            else:
                valid = type(a) is type(b) and a == b
            if not valid:
                errors.append(f"{row['id']}.{key}")
    return errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", type=Path)
    parser.add_argument("--write", type=Path)
    args = parser.parse_args()
    if args.write:
        data = {"schema": "ricon.oracle.channel_cutoff", "version": 1,
                "note": "DRAFT; все уровни мощности и допуски TEST_ONLY; не модель речи или мира.", "cases": cases()}
        args.write.write_text(json.dumps(data, ensure_ascii=False, indent=2, allow_nan=False)+"\n", encoding="utf-8", newline="\n")
    if args.check:
        data = json.loads(args.check.read_text(encoding="utf-8"))
        errors = check(data)
        for error in errors:
            print(error)
        print(f"CHANNEL_CUTOFF_ORACLE_{'FAIL' if errors else 'PASS'} ({len(data.get('cases', []))})")
        return bool(errors)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
