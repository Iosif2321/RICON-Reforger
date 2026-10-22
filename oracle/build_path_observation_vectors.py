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
PROFILE = {"SEA": {"tempC": 20, "salinityPpt": 35}, "FRESH": {"tempC": 15, "salinityPpt": 0.2}}
COMPLETE = {f: "COMPLETE" for f in o.FAMILIES}


def air(x, y=2.0, z=0.0):
    return {"position": [x, y, z], "medium": "AIR", "depthM": 0, "waterClass": "NONE", "waterBodyKey": ""}


def wet(x, depth, cls, body="", y=-1.0, z=0.0):
    return {"position": [x, y, z], "medium": "WATER", "depthM": depth, "waterClass": cls, "waterBodyKey": body}


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

CASES = [
    ("O01", "Воздух–воздух без сегментов", BASE, PROFILE),
    ("O02", "Смешанные семейства, составной объект, перекрытие разных объектов", MIXED, PROFILE),
    ("O03", "Перекрытие сегментов одного объекта", mutate(MIXED, lambda b: b["segments"][2].update(startM=126)), PROFILE),
    ("O04", "Нарушен порядок сегментов", mutate(MIXED, lambda b: b["segments"].reverse()), PROFILE),
    ("O05", "Сегмент за концом пути", mutate(BASE, lambda b: b["segments"].append(seg("STRUCTURE", 990, 1000.5, "x"))), PROFILE),
    ("O06", "Пустой сегмент start=end", mutate(BASE, lambda b: b["segments"].append(seg("STRUCTURE", 10, 10, "x"))), PROFILE),
    ("O07", "UNKNOWN с materialId", mutate(BASE, lambda b: b["segments"].append(seg("STRUCTURE", 1, 2, "x", "UNKNOWN", "brick"))), PROFILE),
    ("O08", "PROFILE без materialId", mutate(BASE, lambda b: b["segments"].append(seg("STRUCTURE", 1, 2, "x", "PROFILE", ""))), PROFILE),
    ("O09", "Рельеф не бывает сегментом", mutate(BASE, lambda b: b["segments"].append(seg("TERRAIN", 1, 2, "hill"))), PROFILE),
    ("O10", "Покрытие без семейства", mutate(BASE, lambda b: b["coverage"].pop("VEHICLE")), PROFILE),
    ("O11", "Нулевая длина пути", obs(air(5), air(5)), PROFILE),
    ("O12", "Глубина у антенны в воздухе", mutate(BASE, lambda b: b["tx"].update(depthM=1)), PROFILE),
    ("O13", "Отрицательная глубина", obs(wet(0, -0.5, "SEA"), air(1000)), PROFILE),
    ("O14", "Один водоём, два класса воды", obs(wet(0, 1, "SEA", "lake.1"), wet(50, 1, "FRESH", "lake.1")), PROFILE),
    ("O15", "Недопустимые символы ключа", mutate(BASE, lambda b: b["segments"].append(seg("STRUCTURE", 1, 2, 'x"); injected('))), PROFILE),
    ("O16", "Лишнее поле наблюдения", mutate(BASE, lambda b: b.update(extra=1)), PROFILE),
    ("O17", "Нулевая частота", mutate(BASE, lambda b: b.update(frequencyMHz=0)), PROFILE),
    ("O18", "Нулевая версия среды", mutate(BASE, lambda b: b.update(environmentRevision=0)), PROFILE),
    ("O19", "bool вместо числа", mutate(BASE, lambda b: b["tx"]["position"].__setitem__(0, True)), PROFILE),
    ("O20", "Координата вне binary32", mutate(BASE, lambda b: b["rx"]["position"].__setitem__(0, 1e39)), PROFILE),
    ("W01", "Морская вода, антенна на 0.5 м", obs(wet(0, 0.5, "SEA", "sea"), air(1000)), PROFILE),
    ("W02", "Пресная вода, антенна на 2 м", obs(air(0), wet(1000, 2.0, "FRESH", "lake.1")), PROFILE),
    ("W03", "Оба конца в разных водоёмах", obs(wet(0, 1.0, "SEA", "sea"), wet(800, 1.5, "FRESH", "lake.1")), PROFILE),
    ("W04", "Один водоём рядом: прямой подводный путь", obs(wet(0, 3.0, "FRESH", "lake.1"), wet(5, 3.0, "FRESH", "lake.1")), PROFILE),
    ("W05", "Один водоём далеко: выход по вертикали", obs(wet(0, 3.0, "FRESH", "lake.1"), wet(100, 3.0, "FRESH", "lake.1")), PROFILE),
    ("W06", "Оба в воде, водоём не опознан", obs(wet(0, 3.0, "FRESH"), wet(5, 3.0, "FRESH")), PROFILE),
    ("W07", "Неизвестный класс воды", obs(wet(0, 1.0, "UNKNOWN", "pond"), air(1000)), PROFILE),
    ("W08", "Класса нет в профиле", obs(wet(0, 1.0, "FRESH", "lake.1"), air(1000)), {"SEA": PROFILE["SEA"]}),
    ("W09", "Покрытие воды неполное", obs(wet(0, 1.0, "SEA", "sea"), air(1000), coverage=dict(COMPLETE, WATER="PARTIAL")), PROFILE),
    ("W10", "Среда конца неизвестна", obs(dict(air(0), medium="UNKNOWN"), air(1000)), PROFILE),
    ("W11", "Температура выше области", obs(wet(0, 1.0, "SEA", "sea"), air(1000)), {"SEA": {"tempC": 41, "salinityPpt": 35}}),
    ("W12", "Солёность выше области", obs(wet(0, 1.0, "SEA", "sea"), air(1000)), {"SEA": {"tempC": 20, "salinityPpt": 40.5}}),
    ("W13", "Границы области T=−4, S=40 включены", obs(wet(0, 1.0, "SEA", "sea"), air(1000)), {"SEA": {"tempC": -4, "salinityPpt": 40}}),
    ("W14", "Антенна у самой поверхности воды", obs(wet(0, 0.0, "SEA", "sea"), air(1000)), PROFILE),
    ("W15", "Неверная структура профиля", obs(wet(0, 1.0, "SEA", "sea"), air(1000)), {"SEA": {"tempC": 20}}),
    ("W16", "Частота выше 1000 ГГц", obs(wet(0, 1.0, "SEA", "sea"), air(1000), f=1.5e6), PROFILE),
    ("W17", "Морская вода 30 МГц", obs(wet(0, 0.5, "SEA", "sea"), air(1000), f=30.0), PROFILE),
    ("W18", "Наблюдение неверно — слой тоже отказывает", mutate(obs(wet(0, 1, "SEA", "sea"), air(1000)), lambda b: b.update(extra=1)), PROFILE),
    ("W19", "Оба в воздухе, но покрытие воды неполное", obs(air(0), air(1000), coverage=dict(COMPLETE, WATER="NOT_OBSERVED")), PROFILE),
    ("W20", "Морская вода 450 МГц, 10 м", obs(wet(0, 10.0, "SEA", "sea"), air(1000), f=450.0), PROFILE),
    ("W21", "Переполнение binary32 по выходу", obs(wet(0, 3.0e38, "SEA", "sea"), air(1000)), PROFILE),
    ("W22", "Пресная вода 2 ГГц, 0.3 м", obs(wet(0, 0.3, "FRESH", "lake.2"), air(1000), f=2000.0), PROFILE),
]


def build() -> bytes:
    cases = []
    for cid, title, observation, profile in CASES:
        case = {"id": cid, "title": title, "input": {"observation": observation, "profile": profile}}
        case["expect"] = o.evaluate(case)
        cases.append(case)
    doc = {"schema": "ricon.oracle.path_observation", "version": 1,
           "source": "docs/req/REQ-path-observation.md v0.1; ITU-R P.527-6 §5.1.2 eq. (14)-(27), (4)",
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
    print(f"PATH_OBSERVATION_VECTORS_WRITTEN: {len(CASES)}")
    return 0


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    raise SystemExit(main())
