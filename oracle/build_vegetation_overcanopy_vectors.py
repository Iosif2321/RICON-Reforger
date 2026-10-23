#!/usr/bin/env python3
"""TOOL_ONLY (claude): build oracle/vectors/vegetation_overcanopy_v1.json; expectations come from
vegetation_overcanopy_oracle.py. Catalog reuses the TEST_ONLY P4 v1 catalog. Usage: [--check]"""
from __future__ import annotations

import copy
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import build_vegetation_vectors as b  # noqa: E402
import vegetation_overcanopy_oracle as o  # noqa: E402

OUT = Path(__file__).resolve().parent / "vectors" / "vegetation_overcanopy_v1.json"
FIG2 = "forest.temperate_fig2"
DEFAULT = {"heightM": None, "provenance": "STANDARD_DEFAULT"}
CANOPY = {FIG2: DEFAULT, "forest.mixed_spb": {"heightM": 16.0, "provenance": "MEASURED"},
          "forest.tropical": {"heightM": 25.0, "provenance": "ASSUMED_CONFIG"}}


def case(length, f, segments, tx_h=2.0, rx_h=2.0, pol="V", canopy=None, catalog=None, coverage=None):
    c = b.case(length, f, segments, pol=pol, tx_h=tx_h, rx_h=rx_h, catalog=catalog, coverage=coverage)
    c["canopy"] = copy.deepcopy(CANOPY if canopy is None else canopy)
    return c


def mutate(base, fn):
    x = copy.deepcopy(base)
    fn(x)
    return x


def veg(start, stop, mat=FIG2, key="trees.1", knowledge="RULE"):
    return b.veg(start, stop, mat, key, knowledge)


CASES = [
    ("O01", "S1: TX в умеренном лесу, 60 МГц, h = 2 м, R по умолчанию 15 м", case(2000, 60.0, [veg(0, 800)])),
    ("O02", "S2: глубокий лес 2 км, 60 МГц, обе антенны 2/3 м — цель P4.2", case(2000, 60.0, [veg(0, 2000)], rx_h=3.0)),
    ("O03", "S1 у RX: R = 16 м по измерению (P.833 табл. 1 лес)", case(1000, 150.0, [veg(600, 1000, "forest.mixed_spb")], rx_h=1.5)),
    ("O04", "Граница области 30 МГц", case(1000, 30.0, [veg(0, 500)])),
    ("O05", "Граница области 3000 МГц", case(1000, 3000.0, [veg(0, 500)])),
    ("O06", "29.9 МГц вне области P.2108", case(1000, 29.9, [veg(0, 500)])),
    ("O07", "3001 МГц вне области P.2108", case(1000, 3001.0, [veg(0, 500)])),
    ("O08", "Антенна на высоте R — поправка 0", case(1000, 60.0, [veg(0, 500)], tx_h=15.0)),
    ("O09", "Антенна выше крон — поправка 0", case(1000, 60.0, [veg(0, 500)], tx_h=20.0)),
    ("O10", "Нет высоты у антенны в лесу", case(1000, 60.0, [veg(0, 500)], tx_h=None)),
    ("O11", "Нет высоты у антенны вне леса — не нужна", case(1000, 60.0, [veg(0, 500)], rx_h=None)),
    ("O12", "Только крона посреди пути — терминалов в лесу нет", case(1000, 60.0, [veg(400, 420)])),
    ("O13", "Нет растительности", case(1000, 60.0, [])),
    ("O14", "Только трава", case(1000, 60.0, [veg(0, 100, "grass")])),
    ("O15", "Неизвестная растительность у TX", case(1000, 60.0, [veg(0, 100, knowledge="UNKNOWN")])),
    ("O16", "Покрытие растительности неполное", case(1000, 60.0, [veg(0, 100)], coverage=dict(b.COMPLETE, VEGETATION="PARTIAL"))),
    ("O17", "Смешение пород у TX", case(1000, 60.0, [veg(0, 100, FIG2, "a.tree"), veg(50, 150, "forest.mixed_spb", "b.tree")])),
    ("O18", "Нет записи кроны для породы у терминала", case(1000, 60.0, [veg(0, 100, "woodland.rice71")])),
    ("O19", "Отрицательная высота крон", case(1000, 60.0, [veg(0, 100)], canopy=dict(CANOPY, **{FIG2: {"heightM": -1.0, "provenance": "MEASURED"}}))),
    ("O20", "Неизвестная provenance кроны", case(1000, 60.0, [veg(0, 100)], canopy=dict(CANOPY, **{FIG2: {"heightM": 15.0, "provenance": "GUESS"}}))),
    ("O21", "STANDARD_DEFAULT с явной высотой", case(1000, 60.0, [veg(0, 100)], canopy=dict(CANOPY, **{FIG2: {"heightM": 15.0, "provenance": "STANDARD_DEFAULT"}}))),
    ("O22", "Запись кроны для травы", case(1000, 60.0, [veg(0, 100)], canopy=dict(CANOPY, grass=DEFAULT))),
    ("O23", "Запись кроны для отсутствующей породы", case(1000, 60.0, [veg(0, 100)], canopy=dict(CANOPY, **{"forest.none": DEFAULT}))),
    ("O24", "H-поляризация — P.2108 от поляризации не зависит", case(2000, 60.0, [veg(0, 800)], pol="H")),
    ("O25", "466 МГц, S1, R = 25 м, тропики", case(1000, 466.475, [veg(0, 300, "forest.tropical")], tx_h=3.0)),
    ("O26", "Огромная высота крон — переполнение binary32", case(1000, 60.0, [veg(0, 100)], canopy=dict(CANOPY, **{FIG2: {"heightM": 3.0e38, "provenance": "MEASURED"}}))),
    ("O27", "Наблюдение неверно", mutate(case(1000, 60.0, []), lambda x: x["observation"].update(environmentRevision=0))),
    ("O28", "Лишнее поле входа", mutate(case(1000, 60.0, [veg(0, 100)]), lambda x: x.update(extra=1))),
    ("O29", "Отрицательная высота антенны", case(1000, 60.0, [veg(0, 100)], tx_h=-1.0)),
    ("O30", "Два интервала у обоих концов, между ними поле", case(3000, 60.0, [veg(0, 500), veg(2500, 3000, key="trees.2")], rx_h=4.0)),
    ("O31", "Почти у крон: h = 14.9 м, ν мало", case(1000, 60.0, [veg(0, 500)], tx_h=14.9)),
]


def build() -> bytes:
    cases = []
    for cid, title, inp in CASES:
        c = {"id": cid, "title": title, "input": inp}
        c["expect"] = o.evaluate(c)
        cases.append(c)
    doc = {"schema": "ricon.oracle.vegetation_overcanopy", "version": 1,
           "source": "docs/req/REQ-vegetation-overcanopy.md v0.1; Rec. ITU-R P.2108-1 §3.1",
           "note": "DRAFT; каталог и кроны TEST_ONLY; не наблюдения мира.", "hostOnly": ["O28"]}
    head = json.dumps(doc, ensure_ascii=False)[:-1]
    body = ",\n".join(json.dumps(c, ensure_ascii=False) for c in cases)
    return (head + ', "cases": [\n' + body + "\n]}\n").encode("utf-8")


def main() -> int:
    data = build()
    if "--check" in sys.argv[1:]:
        ok = OUT.read_bytes() == data
        print("OVERCANOPY_VECTORS_" + ("PASS" if ok else "DIFFER"))
        return 0 if ok else 1
    OUT.write_bytes(data)
    print(f"OVERCANOPY_VECTORS_WRITTEN: {len(CASES)}")
    return 0


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    raise SystemExit(main())
