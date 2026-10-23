#!/usr/bin/env python3
"""TOOL_ONLY (claude): build oracle/vectors/vegetation_v1.json; expectations come from vegetation_oracle.py.
Catalog entries are TEST_ONLY (P.833-10 TABLE 1 and Fig. 2, ECAC Fig. A-1; illustrative tropical gamma). Usage: [--check]"""
from __future__ import annotations

import copy
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import path_observation_oracle as po  # noqa: E402
import vegetation_oracle as v  # noqa: E402

OUT = Path(__file__).resolve().parent / "vectors" / "vegetation_v1.json"
TABLE1 = [[105.9, 0.04], [466.475, 0.12], [949.0, 0.17], [1852.2, 0.30], [2117.5, 0.34]]
# P.833-10 Fig. 2 read at its end points (straight lines in log-log): STANDARD_FIGURE, claude, see REQ VG08.
FIG2 = {"V": [[30.0, 0.02], [1000.0, 0.2], [30000.0, 6.0]], "H": [[30.0, 0.006], [1000.0, 0.2], [30000.0, 6.0]]}
# ECAC ESD-TR-81-101 Fig. A-1 (Rice 1971) read by claude, +-0.005 dB/m: STANDARD_FIGURE.
RICE71 = {"V": [[10.0, 0.02], [30.0, 0.02], [50.0, 0.026], [70.0, 0.036], [100.0, 0.06], [200.0, 0.117],
                [500.0, 0.225], [1000.0, 0.33]],
          "H": [[10.0, 0.02], [70.0, 0.02], [100.0, 0.032], [200.0, 0.07], [500.0, 0.185], [1000.0, 0.32]]}


def profile(climate, gamma, provenance, am=None, leaf="IN_LEAF", density="DENSE", moisture="DRY"):
    return {"negligible": False, "climate": climate, "leafState": leaf, "density": density, "moisture": moisture,
            "gammaTables": gamma, "gammaProvenance": provenance, "am": am}


AM_SPB = {"a1": 1.37, "alpha": 0.42, "minMHz": 105.9, "maxMHz": 2117.5, "provenance": "MEASURED_TABLE"}
AM_RIO = {"a1": 0.18, "alpha": 0.752, "minMHz": 900.0, "maxMHz": 1800.0, "provenance": "MEASURED_TABLE"}
TEMPERATE = profile("TEMPERATE", {"ANY": TABLE1}, "MEASURED_TABLE", AM_SPB)
TROPICAL = profile("TROPICAL", {"ANY": [[25.0, 0.02], [400.0, 0.1]]}, "ASSUMED_CONFIG", AM_RIO, moisture="WET")
CATALOG = {"forest.mixed_spb": TEMPERATE, "forest.mixed_spb_leafless": dict(TEMPERATE, leafState="LEAFLESS"),
           "forest.mixed_spb_sparse": dict(TEMPERATE, density="SPARSE"), "forest.tropical": TROPICAL,
           "forest.temperate_fig2": profile("TEMPERATE", FIG2, "STANDARD_FIGURE"),
           "woodland.p833_fig2": profile("UNSPECIFIED", FIG2, "STANDARD_FIGURE", leaf="UNKNOWN", density="UNKNOWN",
                                         moisture="UNKNOWN"),
           "woodland.rice71": profile("UNSPECIFIED", RICE71, "STANDARD_FIGURE", leaf="UNKNOWN", density="UNKNOWN",
                                      moisture="UNKNOWN"),
           "grass": {"negligible": True}, "bush.small": {"negligible": True}}
COMPLETE = {f: "COMPLETE" for f in po.FAMILIES}


def end(x):
    return {"position": [x, 2.0, 0.0], "medium": "AIR", "depthM": 0}


def veg(start, stop, mat="forest.mixed_spb", key="trees.1", knowledge="RULE"):
    return {"family": "VEGETATION", "startM": start, "endM": stop, "entityKey": key, "knowledge": knowledge,
            "materialId": mat if knowledge in ("PROFILE", "RULE") else "", "openings": False}


def case(length, f, segments, pol="V", tx_h=None, rx_h=None, catalog=None, coverage=None):
    observation = {"frequencyMHz": f, "environmentRevision": 1, "tx": end(0), "rx": end(length),
                   "coverage": dict(coverage or COMPLETE), "segments": list(segments)}
    return {"observation": observation, "catalog": copy.deepcopy(catalog or CATALOG), "polarization": pol,
            "txHeightM": tx_h, "rxHeightM": rx_h}


def mutate(base, fn):
    b = copy.deepcopy(base)
    fn(b)
    return b


BAD_GAMMA = dict(TEMPERATE, gammaTables={"ANY": [[500.0, 0.1], [400.0, 0.2]]})
HUGE = dict(TROPICAL, gammaTables={"ANY": [[25.0, 3.0e38], [400.0, 3.0e38]]})
ANY_AND_V = dict(TEMPERATE, gammaTables={"ANY": TABLE1, "V": TABLE1})
BAD_AM = dict(TEMPERATE, am=dict(AM_SPB, provenance="GUESS"))
FIG2_MAT = "woodland.p833_fig2"

CASES = [
    ("V01", "Нет растительности на пути", case(1000, 466.475, [])),
    ("V02", "Только трава — по решению владельца без потерь", case(1000, 466.475, [veg(100, 140, "grass")])),
    ("V03", "S1: TX в умеренном лесу 50 м, 466 МГц — MED основная, P.833 ур. 1 сверка",
     case(1000, 466.475, [veg(0, 50)])),
    ("V04", "S1: 150 МГц — MED вне области, остаётся P.833 ур. 1", case(1000, 150.0, [veg(0, 50)])),
    ("V05", "S1: 600 м леса — MED вне области по глубине", case(1000, 949.0, [veg(0, 600)])),
    ("V06", "S1: 3 ГГц — P.833 табл. вне области, остаётся MED", case(1000, 3000.0, [veg(940, 1000)])),
    ("V07", "S1: 100 ГГц — ни одна модель не в области", case(1000, 100000.0, [veg(0, 30)])),
    ("V08", "S2: тропический лес, 200 м, 100 МГц V, высоты 3/4 м — Jansky & Bailey",
     case(200, 100.0, [veg(0, 200, "forest.tropical")], tx_h=3.0, rx_h=4.0)),
    ("V09", "S2: тропики без высот антенн — J&B неприменима, остаётся LIN-γ", case(200, 100.0, [veg(0, 200, "forest.tropical")])),
    ("V10", "S2: умеренный лес, 300 м, 466 МГц — MED", case(300, 466.475, [veg(0, 300)], tx_h=3.0, rx_h=3.0)),
    ("V11", "S2: H 100 МГц, 10 м — отрицательный избыток J&B исключён",
     case(10, 100.0, [veg(0, 10, "forest.tropical")], pol="H", tx_h=3.0, rx_h=3.0)),
    ("V12", "S2: тропики 1 ГГц — J&B вне области", case(200, 1000.0, [veg(0, 200, "forest.tropical")], tx_h=3.0, rx_h=3.0)),
    ("V13", "S2: J&B на частоте таблицы 250 МГц, граница 1600 м",
     case(1600, 250.0, [veg(0, 1600, "forest.tropical")], tx_h=2.0, rx_h=7.0)),
    ("V14", "S2: J&B 175 МГц — интерполяция констант по lg f", case(500, 175.0, [veg(0, 500, "forest.tropical")], pol="H", tx_h=5.0, rx_h=5.0)),
    ("V15", "S3: крона 20 м посреди пути, 466 МГц — MED основная, ур. 7 сверка, без ограничения",
     case(1000, 466.475, [veg(400, 420)])),
    ("V16", "S3: 1.5 ГГц — ур. 7 вне области", case(1000, 1500.0, [veg(400, 420)])),
    ("V17", "S3: 150 МГц — MED вне области, остаётся ур. 7", case(1000, 150.0, [veg(400, 420)])),
    ("V18", "S3+: 500 м леса посреди пути — нужен путь над кронами", case(2000, 466.475, [veg(500, 1000)])),
    ("V19", "MED: ветвь d < 14 м", case(1000, 466.475, [veg(400, 410)])),
    ("V20", "MED: граница d = 14 м", case(1000, 466.475, [veg(400, 414)])),
    ("V21", "UNKNOWN — неизвестная растительность", case(1000, 466.475, [veg(100, 120, knowledge="UNKNOWN")])),
    ("V22", "HINT_ONLY не даёт модели", case(1000, 466.475, [veg(100, 120, knowledge="HINT_ONLY")])),
    ("V23", "materialId вне каталога", case(1000, 466.475, [veg(100, 120, "forest.unknown_mod")])),
    ("V24", "Покрытие растительности неполное", case(1000, 466.475, [veg(100, 120)], coverage=dict(COMPLETE, VEGETATION="PARTIAL"))),
    ("V25", "Перекрытие разных пород — смешение не моделируется",
     case(1000, 466.475, [veg(100, 130, "forest.mixed_spb", "a.tree"), veg(120, 150, "forest.tropical", "b.tree")])),
    ("V26", "Перекрытие одной породы сливается", case(1000, 466.475, [veg(100, 130, key="a.tree"), veg(120, 150, key="b.tree")])),
    ("V27", "Два интервала: у конца и посреди пути — сумма основных", case(1000, 466.475, [veg(0, 40), veg(500, 520, key="trees.2")])),
    ("V28", "Каталог: частоты γ не возрастают", case(1000, 466.475, [veg(100, 120)], catalog=dict(CATALOG, **{"forest.mixed_spb": BAD_GAMMA}))),
    ("V29", "Неизвестная поляризация", case(1000, 466.475, [veg(100, 120)], pol="X")),
    ("V30", "Отрицательная высота антенны", case(1000, 466.475, [veg(100, 120)], tx_h=-1.0, rx_h=3.0)),
    ("V31", "S2: высота 8 м вне области J&B — остаётся LIN-γ", case(200, 100.0, [veg(0, 200, "forest.tropical")], tx_h=8.0, rx_h=3.0)),
    ("V32", "Трава под деревом отбрасывается до объединения",
     case(1000, 466.475, [veg(100, 140, "grass", "a.grass"), veg(110, 130, key="b.tree")])),
    ("V33", "Переполнение binary32", case(1000, 150.0, [veg(400, 410, "forest.tropical")], catalog=dict(CATALOG, **{"forest.tropical": HUGE}))),
    ("V34", "Наблюдение неверно", mutate(case(1000, 466.475, []), lambda b: b["observation"].update(environmentRevision=0))),
    ("V35", "S1 у RX: 949 МГц, 80 м — MED и ур. 1", case(1000, 949.0, [veg(920, 1000)])),
    ("V36", "S2: J&B 25 МГц (A = 0), 8 м", case(8, 25.0, [veg(0, 8, "forest.tropical")], tx_h=2.0, rx_h=2.0)),
    ("V37", "S1 low VHF: 30 МГц V, 100 м, P.833 рис. 2 — только LIN-γ, точка таблицы",
     case(1000, 30.0, [veg(0, 100, FIG2_MAT)])),
    ("V38", "S1 low VHF: 60 МГц H, 100 м, рис. 2 — LIN-γ, интерполяция", case(1000, 60.0, [veg(0, 100, FIG2_MAT)], pol="H")),
    ("V39", "S1 low VHF: 60 МГц V, 100 м, Rice-71 (ECAC рис. A-1) — второй источник γ",
     case(1000, 60.0, [veg(0, 100, "woodland.rice71")])),
    ("V40", "S2 умеренный лес low VHF: 60 МГц, 300 м — LIN-γ", case(300, 60.0, [veg(0, 300, "forest.temperate_fig2")], tx_h=3.0, rx_h=3.0)),
    ("V41", "S2 умеренный лес low VHF: 800 м — открыт срез латеральной волны",
     case(800, 60.0, [veg(0, 800, "forest.temperate_fig2")], tx_h=3.0, rx_h=3.0)),
    ("V42", "S1: безлистный лес 466 МГц — MED вне условий, основная P.833 ур. 1",
     case(1000, 466.475, [veg(0, 50, "forest.mixed_spb_leafless")])),
    ("V43", "S1: 150 МГц в листве — P.833 ур. 1 основная, LIN-γ сверка", case(1000, 150.0, [veg(0, 50)])),
    ("V44", "S3 low VHF: крона 20 м, 60 МГц — ур. 7", case(1000, 60.0, [veg(400, 420, FIG2_MAT)])),
    ("V45", "S1 150 МГц V, рис. 2", case(1000, 150.0, [veg(0, 100, FIG2_MAT)])),
    ("V46", "S1 150 МГц H, рис. 2 — таблица своей поляризации", case(1000, 150.0, [veg(0, 100, FIG2_MAT)], pol="H")),
    ("V47", "Каталог: ANY вместе с V", case(1000, 466.475, [veg(100, 120)], catalog=dict(CATALOG, **{"forest.mixed_spb": ANY_AND_V}))),
    ("V48", "Каталог: неизвестная provenance Am", case(1000, 466.475, [veg(100, 120)], catalog=dict(CATALOG, **{"forest.mixed_spb": BAD_AM}))),
    ("V49", "S1 30 ГГц: MED основная, LIN-γ сверка", case(1000, 30000.0, [veg(0, 30, "forest.temperate_fig2")])),
    ("V50", "S3 3 ГГц: только MED", case(1000, 3000.0, [veg(400, 420, "forest.temperate_fig2")])),
    ("V51", "S1 low VHF: 401 м — LIN-γ вне глубины, срез открыт", case(1000, 30.0, [veg(0, 401, FIG2_MAT)])),
    ("V52", "S2 тропики 60 МГц V, 200 м — J&B основная, LIN-γ сверка",
     case(200, 60.0, [veg(0, 200, "forest.tropical")], tx_h=3.0, rx_h=3.0)),
    ("V53", "S1: разреженный лес — MED вне условий", case(1000, 466.475, [veg(0, 50, "forest.mixed_spb_sparse")])),
    ("V54", "S1 29.9 МГц — ниже таблиц γ (HF), модели нет", case(1000, 29.9, [veg(0, 100, FIG2_MAT)])),
]


def build() -> bytes:
    cases = []
    for cid, title, inp in CASES:
        c = {"id": cid, "title": title, "input": inp}
        c["expect"] = v.evaluate(c)
        cases.append(c)
    doc = {"schema": "ricon.oracle.vegetation", "version": 1,
           "source": "docs/req/REQ-vegetation.md v0.3; DoD ECAC ESD-TR-81-101; ITU-R P.833-10",
           "note": "DRAFT; каталог TEST_ONLY; не наблюдения мира.", "cases": cases}
    # One case per line: the full catalog repeats in every case, indented output would be ~350 KB.
    head = json.dumps({k: v for k, v in doc.items() if k != "cases"}, ensure_ascii=False)[:-1]
    body = ",\n".join(json.dumps(c, ensure_ascii=False) for c in doc["cases"])
    return (head + ', "cases": [\n' + body + "\n]}\n").encode("utf-8")


def main() -> int:
    data = build()
    if "--check" in sys.argv[1:]:
        ok = OUT.read_bytes() == data
        print("VEGETATION_VECTORS_" + ("PASS" if ok else "DIFFER"))
        return 0 if ok else 1
    OUT.write_bytes(data)
    print(f"VEGETATION_VECTORS_WRITTEN: {len(CASES)}")
    return 0


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    raise SystemExit(main())
