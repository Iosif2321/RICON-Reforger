#!/usr/bin/env python3
"""TOOL_ONLY (claude): build oracle/vectors/path_observation_v1.json from the case list below.
Expectations are computed by path_observation_oracle.py; positions, depths and the water profile are TEST_ONLY.
Usage: python build_path_observation_vectors.py [--check]"""
from __future__ import annotations

import copy
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import path_observation_oracle as o  # noqa: E402

OUT = Path(__file__).resolve().parent / "vectors" / "path_observation_v1.json"
PROFILE = {"tempC": 15, "salinityPpt": 0.5}      # one averaged water profile per world (#3415), TEST_ONLY
COMPLETE = {f: "COMPLETE" for f in o.FAMILIES}


def air(x, y=2.0, z=0.0):
    return {"position": [x, y, z], "medium": "AIR", "depthM": 0}


def wet(x, depth, y=2.0, z=0.0):
    return {"position": [x, y, z], "medium": "WATER", "depthM": depth}


def seg(family, start, end, key, knowledge="UNKNOWN", material="", openings=False):
    return {"family": family, "startM": start, "endM": end, "entityKey": key, "knowledge": knowledge,
            "materialId": material, "openings": openings}


def obs(tx, rx, f=150.0, segments=(), coverage=None, rev=1):
    return {"frequencyMHz": f, "environmentRevision": rev, "tx": tx, "rx": rx,
            "coverage": dict(coverage or COMPLETE), "segments": list(segments)}


def mutate(base, fn):
    b = copy.deepcopy(base)
    fn(b)
    return b


BASE = obs(air(0), air(1000))
MIXED = obs(air(0), air(1000), segments=[
    seg("VEGETATION", 100, 130, "tree.17", "RULE", "rule.tree_broadleaf"),
    seg("STRUCTURE", 120, 128, "house.3", "PROFILE", "brick_wall_02", True),
    seg("STRUCTURE", 140, 148, "house.3", "PROFILE", "brick_wall_02"),
    seg("VEHICLE", 500, 504, "veh.ural.9", "HINT_ONLY"),
    seg("STRUCTURE", 700, 712, "mod.bunker.1"),
])
SAME_START = obs(air(0), air(1000), segments=[
    seg("WATER", 300, 310, "z.pond"), seg("STRUCTURE", 300, 305, "a.wall"), seg("STRUCTURE", 300, 302, "b.wall"),
    seg("VEHICLE", 300, 301, "a.car")])
POND = obs(air(0), air(1000), segments=[seg("WATER", 400, 412.5, "pond.4")])

CASES = [
    ("O01", "Воздух–воздух без сегментов", BASE),
    ("O02", "Смешанные семейства, составной объект, перекрытие разных объектов", MIXED),
    ("O03", "Перекрытие сегментов одного объекта", mutate(MIXED, lambda b: b["segments"][2].update(startM=126))),
    ("O04", "Нарушен порядок сегментов", mutate(MIXED, lambda b: b["segments"].reverse())),
    ("O05", "Сегмент за концом пути", mutate(BASE, lambda b: b["segments"].append(seg("STRUCTURE", 990, 1000.5, "x")))),
    ("O06", "Пустой сегмент start=end", mutate(BASE, lambda b: b["segments"].append(seg("STRUCTURE", 10, 10, "x")))),
    ("O07", "UNKNOWN с materialId", mutate(BASE, lambda b: b["segments"].append(seg("STRUCTURE", 1, 2, "x", "UNKNOWN", "brick")))),
    ("O08", "PROFILE без materialId", mutate(BASE, lambda b: b["segments"].append(seg("STRUCTURE", 1, 2, "x", "PROFILE", "")))),
    ("O09", "Рельеф не бывает сегментом", mutate(BASE, lambda b: b["segments"].append(seg("TERRAIN", 1, 2, "hill")))),
    ("O10", "Покрытие без семейства", mutate(BASE, lambda b: b["coverage"].pop("VEHICLE"))),
    ("O11", "Нулевая длина пути", obs(air(5), air(5))),
    ("O12", "Глубина у антенны в воздухе", mutate(BASE, lambda b: b["tx"].update(depthM=1))),
    ("O13", "Отрицательная глубина", obs(wet(0, -0.5), air(1000), segments=[seg("WATER", 0, 0.5, "sea")])),
    ("O14", "Одинаковый start: порядок семейств и побайтный ключ", SAME_START),
    ("O15", "Нарушен порядок семейств при одинаковом start",
     mutate(SAME_START, lambda b: b["segments"].insert(0, b["segments"].pop(3)))),
    ("O16", "Лишнее поле наблюдения", mutate(BASE, lambda b: b.update(extra=1))),
    ("O17", "Нулевая частота", mutate(BASE, lambda b: b.update(frequencyMHz=0))),
    ("O18", "Нулевая версия среды", mutate(BASE, lambda b: b.update(environmentRevision=0))),
    ("O19", "bool вместо числа", mutate(BASE, lambda b: b["tx"]["position"].__setitem__(0, True))),
    ("O20", "Координата вне binary32", mutate(BASE, lambda b: b["rx"]["position"].__setitem__(0, 1e39))),
    ("O21", "Касание сегментов одного объекта допустимо",
     obs(air(0), air(100), segments=[seg("STRUCTURE", 10, 20, "h"), seg("STRUCTURE", 20, 30, "h")])),
    ("O23", "Недопустимые символы ключа", mutate(BASE, lambda b: b["segments"].append(seg("STRUCTURE", 1, 2, 'x"); injected(')))),
    ("O24", "binary32 до проверки: 16777216 и 16777217 совпадают, сегмент пуст",
     obs(air(0, 2.0), air(20000000, 2.0), segments=[seg("STRUCTURE", 16777216, 16777217, "long")])),
    ("W01", "Антенна TX в воде, 0.5 м воды по пути", obs(wet(0, 0.5), air(1000), segments=[seg("WATER", 0, 0.5, "sea")])),
    ("W02", "Антенна RX в воде, 2 м воды", obs(air(0), wet(1000, 2.0), segments=[seg("WATER", 998, 1000, "lake.1")])),
    ("W03", "Оба конца в воде, вода у каждого конца", obs(wet(0, 1.0), wet(800, 1.5),
     segments=[seg("WATER", 0, 4, "sea"), seg("WATER", 790, 800, "lake.1")])),
    ("W04", "Водный отрезок посреди пути", POND),
    ("W05", "Прямая подводная связь [0, L]", obs(wet(0, 3.0), wet(5, 3.0), segments=[seg("WATER", 0, 5, "lake.1")])),
    ("W06", "Перекрытие разных водных объектов сливается", obs(air(0), air(1000),
     segments=[seg("WATER", 10, 20, "a.pond"), seg("WATER", 15, 25, "b.pond")])),
    ("W07", "Касание сегментов одного водного объекта сливается", obs(air(0), air(1000),
     segments=[seg("WATER", 10, 20, "a.pond"), seg("WATER", 20, 30, "a.pond")])),
    ("W08", "Касание разных водных объектов сливается у конца", obs(wet(0, 1.0), air(1000),
     segments=[seg("WATER", 0, 6, "a.sea"), seg("WATER", 6, 9, "b.bay")])),
    ("W09", "Конец в воде без водного интервала", obs(wet(0, 1.0), air(1000))),
    ("W10", "Водный интервал у конца, а конец в воздухе", obs(air(0), air(1000), segments=[seg("WATER", 0, 3, "pond")])),
    ("W11", "Покрытие воды неполное", obs(air(0), air(1000), coverage=dict(COMPLETE, WATER="PARTIAL"))),
    ("W12", "Среда конца неизвестна", obs(dict(air(0), medium="UNKNOWN"), air(1000))),
    ("W13", "Частота выше 1000 ГГц", mutate(POND, lambda b: b.update(frequencyMHz=1.5e6))),
    ("W14", "30 МГц", mutate(POND, lambda b: b.update(frequencyMHz=30.0))),
    ("W15", "2 ГГц", mutate(POND, lambda b: b.update(frequencyMHz=2000.0))),
    ("W16", "Переполнение binary32 по выходу", obs(air(0), air(3.0e38),
     segments=[seg("WATER", 0.5e37, 3.0e37, "ocean")])),
    ("W17", "Наблюдение неверно — слой тоже отказывает", mutate(POND, lambda b: b.update(extra=1))),
]
PROFILE_CASES = [
    ("W18", "Температура выше области", POND, {"tempC": 41, "salinityPpt": 0.5}),
    ("W19", "Солёность выше области", POND, {"tempC": 15, "salinityPpt": 40.5}),
    ("W20", "Границы области T=−4, S=40 включены", POND, {"tempC": -4, "salinityPpt": 40}),
    ("W21", "Неверная структура профиля", POND, {"tempC": 15}),
    ("W22", "Без воды на пути профиль не проверяется на область", BASE, {"tempC": 99, "salinityPpt": 0.5}),
    ("W23", "Солёный профиль 35 ‰", POND, {"tempC": 20, "salinityPpt": 35}),
    ("W24", "Чистая вода S=0", POND, {"tempC": 20, "salinityPpt": 0}),
]


def build() -> bytes:
    cases = []
    for cid, title, observation, *rest in [c + (PROFILE,) for c in CASES] + PROFILE_CASES:
        profile = rest[0]
        case = {"id": cid, "title": title, "input": {"observation": observation, "profile": profile}}
        case["expect"] = o.evaluate(case)
        cases.append(case)
    doc = {"schema": "ricon.oracle.path_observation", "version": 1,
           "source": "docs/req/REQ-path-observation.md v0.2; ITU-R P.527-6 §5.1.2 eq. (14)-(27), (4)",
           "note": "DRAFT; положения, глубины и профиль воды TEST_ONLY; не наблюдения мира.",
           "cases": cases}
    return (json.dumps(doc, ensure_ascii=False, indent=1) + "\n").encode("utf-8")


def main() -> int:
    data = build()
    if "--check" in sys.argv[1:]:
        ok = OUT.read_bytes() == data
        print("PATH_OBSERVATION_VECTORS_" + ("PASS" if ok else "DIFFER"))
        return 0 if ok else 1
    OUT.write_bytes(data)
    print(f"PATH_OBSERVATION_VECTORS_WRITTEN: {len(CASES) + len(PROFILE_CASES)}")
    return 0


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    raise SystemExit(main())
