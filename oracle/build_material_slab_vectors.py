"""Сборка чернового корпуса P3.2a; после freeze регенерация требует новой ревизии."""
import copy
import hashlib
import json
import math
from pathlib import Path
import struct

import material_slab_oracle as o

ROOT = Path(__file__).resolve().parents[1]


def adjacent(value, step):
    bits = struct.unpack(">I", struct.pack(">f", value))[0]
    return struct.unpack(">f", struct.pack(">I", bits + step))[0]


def cases():
    rows = []

    def add(note, changes=None, material=None, native=True, expected=None, origin=None):
        value = o.baseline()
        if changes:
            value.update(changes)
        if material:
            value["material"].update(material)
        result = o.evaluate(value)
        if expected is not None:
            assert set(result) == set(expected)
            for key, item in expected.items():
                if key == "lossDb":
                    assert math.isclose(result[key], item, rel_tol=1e-12, abs_tol=1e-8), note
                else:
                    assert result[key] == item, note
            result = expected
        rows.append({"id": f"MS{len(rows) + 1:03d}", "note": note, "native": native,
                     "provenance": origin or ("HAND_DERIVED" if expected else "ORACLE_DERIVED"),
                     "input": copy.deepcopy(value), "expect": result})

    def exact(loss):
        return {"code": "OK", "lossDb": loss, "provenance": "TEST_ONLY"}

    add("Нулевая толщина", {"thicknessM": 0}, expected=exact(0))
    add("Воздух", material={"epsilonReal": 1}, expected=exact(0))
    add("Четверть волны в n=2", {"thicknessM": o.C / (100e6 * 8)}, expected=exact(-20 * math.log10(.8)))
    add("Половина волны в n=2", {"thicknessM": o.C / (100e6 * 4)}, expected=exact(0))
    add("Брюстер TM", {"cosIncidence": 1 / math.sqrt(5), "polarization": "TM"}, expected=exact(0))
    for cosine in (1, .5, math.cos(math.radians(85)), o.MIN_COSINE):
        for pol in ("TE", "TM"):
            add("Диэлектрик, угол и поляризация", {"cosIncidence": cosine, "polarization": pol})
    for frequency in (20, 30, 76, 1000, 3000, 100000, 450000):
        add("Проводящий слой, несколько диапазонов", {"frequencyMHz": frequency, "thicknessM": .0001},
            {"epsilonReal": 1, "conductivitySm": 1e7})
    add("Проводник: линейная амплитуда underflow, конечный loss", {"frequencyMHz": 1000, "thicknessM": .1},
        {"epsilonReal": 1, "conductivitySm": 1e7})
    add("Тонкий проводящий лист", {"frequencyMHz": 30, "thicknessM": 1e-12},
        {"epsilonReal": 1, "conductivitySm": 1e8})
    add("Минимальная положительная binary32 толщина", {"thicknessM": adjacent(0, 1)},
        {"conductivitySm": 1e8})
    p = complex(4, -17.98 / .1) ** .5
    center = o.f32(12 / (-p.imag * 2 * math.pi * 100e6 / o.C))
    for d in (adjacent(center, -1), adjacent(center, 1)):
        add("Непрерывность около границы сильного поглощения", {"thicknessM": d}, {"conductivitySm": 1})
    for status in o.PROVENANCE:
        add("Происхождение сохраняется", material={"provenance": status})
    for f in (adjacent(1, -1), 1, 450000, adjacent(450000, 1)):
        add("Глобальная область частоты", {"frequencyMHz": f, "thicknessM": .0001})
    for f in (adjacent(100, -1), 100, adjacent(100, 1)):
        add("Одночастотный профиль", {"frequencyMHz": f}, {"minMHz": 100, "maxMHz": 100})
    for cosine in (adjacent(.05, -1), .05, adjacent(.05, 1)):
        add("Граница угла", {"cosIncidence": cosine})
    for d in (adjacent(5, -1), 5, adjacent(5, 1)):
        add("Граница binary32 f*d", {"thicknessM": d})
    for eps in (100, adjacent(100, 1)):
        add("Граница epsilon", material={"epsilonReal": eps})
    for sigma in (1e8, adjacent(1e8, 1)):
        add("Граница conductivity", material={"conductivitySm": sigma})
    for change in ({"frequencyMHz": 0}, {"thicknessM": -1}, {"cosIncidence": 1.1}, {"polarization": "H"}):
        add("Неверный структурный вход", change)
    for material in ({"epsilonReal": .5}, {"conductivitySm": -1}, {"minMHz": 200, "maxMHz": 100},
                     {"provenance": "UNKNOWN"}):
        add("Неверный профиль", material=material)
    for field in ("frequencyMHz", "thicknessM", "cosIncidence"):
        add("JSON bool не float", {field: True}, native=False)
    add("Профиль null", {"material": None})
    add("Лишнее поле", {"extra": 0}, native=False)
    add("Число в виде строки", {"frequencyMHz": "100"}, native=False)
    # Встречная проверка независимых E01–E17, c4bce7d; публичные параметры остаются TEST_ONLY.
    extra = [
        ("E01: произведение округляется к 500", {"frequencyMHz": 3, "thicknessM": 166.6666717529297}, {}, True, exact(1.5068075346460894)),
        ("E02: произведение округляется вниз", {"frequencyMHz": 76, "thicknessM": 6.5789475440979}, {}, True, exact(1.5068076561545352)),
        ("E03: второй случай округления к 500", {"frequencyMHz": 7, "thicknessM": 71.42857360839844}, {}, True, exact(1.5068075346460836)),
        ("E04: приоритет INVALID_INPUT", {"frequencyMHz": 0, "material": None}, {}, True, {"code": "INVALID_INPUT"}),
        ("E05: приоритет INVALID_PROFILE", {"frequencyMHz": .5}, {"epsilonReal": .5}, True, {"code": "INVALID_PROFILE"}),
        ("E06: область частоты раньше численного предела", {"frequencyMHz": .5, "thicknessM": 2000}, {}, True, {"code": "OUT_OF_MODEL_DOMAIN"}),
        ("E07: область профиля раньше epsilon-limit", {}, {"minMHz": 200, "maxMHz": 300, "epsilonReal": 100.00000762939453}, True, {"code": "OUT_OF_MODEL_DOMAIN"}),
        ("E08: параллельное падение", {"cosIncidence": 0}, {}, True, {"code": "NUMERIC_DOMAIN_LIMIT"}),
        ("E09: переполнение binary32", {"frequencyMHz": 1e39}, {}, False, {"code": "INVALID_INPUT"}),
        ("E10: округление в Inf", {"frequencyMHz": 3.4028236e38}, {}, False, {"code": "INVALID_INPUT"}),
        ("E11: конечный FLT_MAX вне области", {"frequencyMHz": 3.4028235e38}, {}, False, {"code": "OUT_OF_MODEL_DOMAIN"}),
        ("E12: фаза около верхнего предела", {"thicknessM": 5}, {"epsilonReal": 100}, True, exact(13.190136521546096)),
        ("E13: сопоставимые интерфейс и поглощение", {"frequencyMHz": 30, "thicknessM": 7e-11}, {"epsilonReal": 1, "conductivitySm": 1e8}, True, exact(7.305697535723499)),
        ("E14: очень малое поглощение", {"frequencyMHz": 1, "thicknessM": 5e-13}, {"epsilonReal": 1, "conductivitySm": 1e8}, True, exact(.08144519095511062)),
        ("E15: TM при наклонном падении", {"frequencyMHz": 30, "thicknessM": 1e-12, "cosIncidence": .05, "polarization": "TM"}, {"epsilonReal": 1, "conductivitySm": 1e8}, True, exact(.008178971274354132)),
        ("E16: тонкий диэлектрик с потерями", {"thicknessM": 1e-6}, {"conductivitySm": 1}, True, exact(.001636410522737617)),
        ("E17: отрицательный ноль", {"thicknessM": -0.0}, {}, False, exact(0)),
    ]
    for note, changes, material, native, expected in extra:
        add(note, changes, material, native, expected, origin="CROSS_CHECKED")
    add("Узкий lossless-резонанс", {"thicknessM": 3.9173684120178223, "cosIncidence": .05},
        {"epsilonReal": 100}, expected=exact(3.010875843391583), origin="CROSS_CHECKED")
    for depth in (1e-11, 5e-13):
        impedance = 2 * math.pi * 17.98e9 / o.C
        expected = 20 * math.log10(1 + impedance * o.f32(1e8) * o.f32(depth) / (2 * o.f32(.05)))
        add("Тонкий TE-лист: контроль запрещённых вычитаний", {"frequencyMHz": 1, "thicknessM": depth, "cosIncidence": .05},
            {"epsilonReal": 1, "conductivitySm": 1e8}, expected=exact(expected))
    add("TM-лист на нижней границе частоты", {"frequencyMHz": 1, "thicknessM": 1e-12, "cosIncidence": .05, "polarization": "TM"},
        {"epsilonReal": 1, "conductivitySm": 1e8}, expected=exact(.008178971273966228), origin="CROSS_CHECKED")
    return rows


def build():
    req = ROOT / "docs/req/REQ-material-slab.md"
    result = {"schema": "ricon.oracle.material_slab", "version": 1, "revision": 2,
              "source": "docs/req/REQ-material-slab.md", "requirementsLfSha256": hashlib.sha256(req.read_bytes().replace(b"\r\n", b"\n")).hexdigest(),
              "note": "FROZEN rev2; параметры TEST_ONLY, кроме явно указанного provenance. Не полевые измерения.",
              "cases": cases()}
    path = ROOT / "oracle/vectors/material_slab_v1.json"
    path.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"Корпус: {len(result['cases'])}, native: {sum(c['native'] for c in result['cases'])}")


if __name__ == "__main__":
    build()
