#!/usr/bin/env python3
"""TOOL_ONLY (claude): build oracle/vectors/vegetation_path_v1.json; expectations come from vegetation_path_oracle.py.
Base budgets (FSPL, L2) are TEST_ONLY synthetic numbers standing for the outputs of the existing L0/L2 evaluators."""
from __future__ import annotations

import copy
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import build_vegetation_overcanopy_vectors as bo  # noqa: E402
import build_vegetation_vectors as b  # noqa: E402
import vegetation_path_oracle as p  # noqa: E402

OUT = Path(__file__).resolve().parent / "vectors" / "vegetation_path_v1.json"
FIG2 = "forest.temperate_fig2"
SPB = "forest.mixed_spb"
TROP = "forest.tropical"


def budget(fspl=90.0, l2=5.0, fspl_over=90.0, l2_over=0.0):
    return {"fsplDb": fspl, "l2Db": l2, "fsplOverDb": fspl_over, "l2OverDb": l2_over}


def case(length, f, segments, bud=None, **kw):
    c = bo.case(length, f, segments, **kw)
    c["budget"] = budget() if bud is None else bud
    return c


def veg(start, stop, mat=FIG2, key="trees.1", knowledge="RULE"):
    return b.veg(start, stop, mat, key, knowledge)


def mutate(base, fn):
    x = copy.deepcopy(base)
    fn(x)
    return x


CASES = [
    ("P01", "S1 466 МГц, MED закрыт, P.2108 закрыт — обе ветви, CLOSED", case(1000, 466.475, [veg(0, 50, SPB)])),
    ("P02", "Цель: S2 глубокий умеренный лес 800 м, 60 МГц — сквозь нет, BOUNDED_BY_OVER",
     case(800, 60.0, [veg(0, 800)], rx_h=3.0)),
    ("P03", "S1 60 МГц 100 м — сквозь только LIN-γ, поверх закрыт", case(1000, 60.0, [veg(0, 100)])),
    ("P04", "S2 тропики J&B 200 м — сквозь FSPL + Aj без L2, обе ветви закрыты",
     case(200, 60.0, [veg(0, 200, TROP)], tx_h=3.0, rx_h=3.0)),
    ("P05", "Антенна ровно на высоте крон: A_h = 0, сравнение двух полных бюджетов",
     case(1000, 466.475, [veg(0, 50, SPB)], tx_h=16.0, bud=budget(90.0, 5.0, 100.0, 6.0))),
    ("P06", "Поверх дешевле — выбирается OVER", case(1000, 466.475, [veg(0, 300, SPB)], bud=budget(90.0, 5.0, 90.0, 0.0))),
    ("P07", "Нет L2 для реальных высот — сквозь отсутствует", case(1000, 466.475, [veg(0, 50, SPB)], bud=budget(l2=None))),
    ("P08", "Нет L2 для опорных высот — поверх отсутствует", case(1000, 466.475, [veg(0, 50, SPB)], bud=budget(l2_over=None))),
    ("P09", "Глубокий лес без L2 опорных высот — ветвей нет", case(800, 60.0, [veg(0, 800)], bud=budget(l2_over=None))),
    ("P10", "Только крона посреди пути — поверх требует геометрии", case(1000, 466.475, [veg(400, 420, SPB)])),
    ("P11", "S1 и крона посреди пути — поверх не применим", case(1000, 466.475, [veg(0, 50, SPB), veg(400, 420, SPB, "trees.2")])),
    ("P12", "Нет растительности — L0 + L2", case(1000, 60.0, [])),
    ("P13", "Нет растительности и нет L2", case(1000, 60.0, [], bud=budget(l2=None))),
    ("P14", "HF 20 МГц Rice-71 — поверх вне области P.2108", case(1000, 20.0, [veg(0, 100, "woodland.rice71")],
     canopy=dict(bo.CANOPY, **{"woodland.rice71": bo.DEFAULT}))),
    ("P15", "Нет высоты TX — поверх вне области, сквозь MED закрыт", case(1000, 466.475, [veg(0, 50, SPB)], tx_h=None)),
    ("P16", "FSPL = 0 недопустим", case(1000, 60.0, [veg(0, 100)], bud=budget(fspl=0.0))),
    ("P17", "Отрицательный L2", case(1000, 60.0, [veg(0, 100)], bud=budget(l2=-1.0))),
    ("P18", "Лишнее поле бюджета", case(1000, 60.0, [veg(0, 100)], bud=dict(budget(), extra=1.0))),
    ("P19", "Неизвестная растительность", case(1000, 60.0, [veg(0, 100, knowledge="UNKNOWN")])),
    ("P20", "Покрытие неполное", case(1000, 60.0, [veg(0, 100)], coverage=dict(b.COMPLETE, VEGETATION="PARTIAL"))),
    ("P21", "Смешение пород", case(1000, 60.0, [veg(0, 100, FIG2, "a.t"), veg(50, 150, SPB, "b.t")])),
    ("P22", "Наблюдение неверно", mutate(case(1000, 60.0, []), lambda x: x["observation"].update(environmentRevision=0))),
    ("P23", "Неверная запись кроны", case(1000, 60.0, [veg(0, 100)],
     canopy=dict(bo.CANOPY, **{FIG2: {"heightM": -1.0, "provenance": "MEASURED"}}))),
    ("P24", "Переполнение суммы бюджета", case(1000, 466.475, [veg(0, 50, SPB)], bud=budget(3.0e38, 3.0e38, 3.0e38, 3.0e38))),
    ("P25", "Антенна выше крон — A_h = 0, поверх = база опорной геометрии", case(1000, 466.475, [veg(0, 50, SPB)], tx_h=20.0)),
    ("P26", "Только трава", case(1000, 60.0, [veg(0, 100, "grass")])),
    ("P27", "Лишнее поле корня", mutate(case(1000, 60.0, [veg(0, 100)]), lambda x: x.update(extra=1))),
    # Точное равенство: сквозь = 90 + 64·f32(0.02) (LIN-γ, 30 МГц), поверх = 90 + тот же binary32 при A_h = 0 (h ≥ R).
    ("P28", "Точное равенство полных потерь — выбирается THROUGH",
     case(1000, 30.0, [veg(0, 64)], tx_h=20.0, bud=budget(90.0, 0.0, 90.0, 64 * 0.019999999552965164))),
    ("P29", "Нет растительности, сумма бюджета переполняется", case(1000, 60.0, [], bud=budget(3.0e38, 3.0e38))),
    ("P30", "Близкое равенство codex #3692: разница в binary64 есть, в binary32 нет — THROUGH",
     case(1000, 30.0, [veg(0, 64)], tx_h=20.0, bud=budget(90.0, 0.0, 91.279998779296875, 0.0))),
    ("P31", "За полосой равенства: поверх дешевле на 0.002 дБ — OVER",
     case(1000, 30.0, [veg(0, 64)], tx_h=20.0, bud=budget(90.0, 0.0, 91.278, 0.0))),
]


def build() -> bytes:
    cases = []
    for cid, title, inp in CASES:
        c = {"id": cid, "title": title, "input": inp}
        c["expect"] = p.evaluate(c)
        cases.append(c)
    doc = {"schema": "ricon.oracle.vegetation_path", "version": 1,
           "source": "docs/req/REQ-vegetation-path.md v0.3; REQ-vegetation v0.4; REQ-vegetation-overcanopy v0.1",
           "note": "DRAFT; каталог, кроны и базовые бюджеты TEST_ONLY.", "hostOnly": ["P18", "P27"]}
    head = json.dumps(doc, ensure_ascii=False)[:-1]
    body = ",\n".join(json.dumps(c, ensure_ascii=False) for c in cases)
    return (head + ', "cases": [\n' + body + "\n]}\n").encode("utf-8")


def main() -> int:
    data = build()
    if "--check" in sys.argv[1:]:
        ok = OUT.read_bytes() == data
        print("VEGETATION_PATH_VECTORS_" + ("PASS" if ok else "DIFFER"))
        return 0 if ok else 1
    OUT.write_bytes(data)
    print(f"VEGETATION_PATH_VECTORS_WRITTEN: {len(CASES)}")
    return 0


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    raise SystemExit(main())
