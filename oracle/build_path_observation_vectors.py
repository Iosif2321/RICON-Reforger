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


def wet(x, depth, y=-1.0, z=0.0):
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
    ("O13", "Отрицательная глубина", obs(wet(0, -0.5), air(1000))),
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
    ("O22", "WATER-сегмент у самой антенны недопустим", mutate(BASE, lambda b: b["segments"].append(seg("WATER", 0, 3, "pond")))),
    ("O23", "Недопустимые символы ключа", mutate(BASE, lambda b: b["segments"].append(seg("STRUCTURE", 1, 2, 'x"); injected(')))),
    ("O24", "binary32 до проверки: 16777216 и 16777217 совпадают, сегмент пуст",
     obs(air(0, 2.0), air(20000000, 2.0), segments=[seg("STRUCTURE", 16777216, 16777217, "long")])),
    ("W01", "Антенна на 0.5 м под водой", obs(wet(0, 0.5), air(1000))),
    ("W02", "Приёмник на 2 м под водой", obs(air(0), wet(1000, 2.0))),
    ("W03", "Оба конца в воде", obs(wet(0, 1.0), wet(800, 1.5))),
    ("W04", "Водный отрезок посреди пути", POND),
    ("W05", "Погружённый конец и водный отрезок", mutate(POND, lambda b: b.update(tx=wet(0, 0.7)))),
    ("W06", "Антенна у самой поверхности воды", obs(wet(0, 0.0), air(1000))),
    ("W07", "Покрытие воды неполное", obs(wet(0, 1.0), air(1000), coverage=dict(COMPLETE, WATER="PARTIAL"))),
    ("W08", "Среда конца неизвестна", obs(dict(air(0), medium="UNKNOWN"), air(1000))),
    ("W09", "Оба в воздухе, но вода не наблюдалась", obs(air(0), air(1000), coverage=dict(COMPLETE, WATER="NOT_OBSERVED"))),
    ("W10", "Частота выше 1000 ГГц", obs(wet(0, 1.0), air(1000), f=1.5e6)),
    ("W11", "30 МГц", obs(wet(0, 0.5), air(1000), f=30.0)),
    ("W12", "2 ГГц, 0.3 м", obs(wet(0, 0.3), air(1000), f=2000.0)),
    ("W13", "450 МГц, 10 м", obs(wet(0, 10.0), air(1000), f=450.0)),
    ("W14", "Переполнение binary32 по выходу", obs(wet(0, 3.0e38), air(1000))),
    ("W15", "Наблюдение неверно — слой тоже отказывает", mutate(obs(wet(0, 1), air(1000)), lambda b: b.update(extra=1))),
]
PROFILE_CASES = [
    ("W16", "Температура выше области", obs(wet(0, 1.0), air(1000)), {"tempC": 41, "salinityPpt": 0.5}),
    ("W17", "Солёность выше области", obs(wet(0, 1.0), air(1000)), {"tempC": 15, "salinityPpt": 40.5}),
    ("W18", "Границы области T=−4, S=40 включены", obs(wet(0, 1.0), air(1000)), {"tempC": -4, "salinityPpt": 40}),
    ("W19", "Неверная структура профиля", obs(wet(0, 1.0), air(1000)), {"tempC": 15}),
    ("W20", "Профиль без воды на пути не нужен", BASE, {"tempC": 99, "salinityPpt": 0.5}),
    ("W21", "Солёный профиль: морская вода 35 ‰", obs(wet(0, 0.5), air(1000)), {"tempC": 20, "salinityPpt": 35}),
    ("W22", "Чистая вода S=0", obs(wet(0, 0.5), air(1000)), {"tempC": 20, "salinityPpt": 0}),
    ("W23", "|n| < 3: выход по вертикали вне области", obs(wet(0, 0.5), air(1000), f=900000.0), {"tempC": 20, "salinityPpt": 0}),
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
