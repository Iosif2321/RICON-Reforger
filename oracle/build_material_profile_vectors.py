"""Сборка P3.2b; --check проверяет корпус без записи, в том числе при несовпадении."""
import argparse
import copy
import json
import math

from build_material_slab_vectors import adjacent
import material_profile_oracle as o

TARGET = o.ROOT / "oracle/vectors/material_profile_v1.json"


def baseline():
    return {"frequencyMHz": 1000, "profile": {"a": 4, "b": 0, "c": .25, "d": 0,
            "minMHz": 1, "maxMHz": 450000, "maxInclusive": True, "provenance": "TEST_ONLY"}}


def cases():
    rows = []

    def add(note, value, expected=None, native=True):
        result = o.evaluate(value)
        if expected is not None:
            assert result == expected, (note, result, expected)
        rows.append({"id": f"MP{len(rows) + 1:03d}", "note": note, "native": native,
                     "provenance": "HAND_DERIVED" if expected is not None else "ORACLE_DERIVED",
                     "input": copy.deepcopy(value), "expect": result})

    def change(note, profile=None, frequency=1000, expected=None, native=True):
        value = baseline()
        value["frequencyMHz"] = frequency
        value["profile"].update(profile or {})
        add(note, value, {"code": expected} if expected else None, native)

    for name, entry in o.load_catalog().items():
        p = entry["profile"]
        for label, frequency in (("нижняя граница", p["minMHz"]),
                                 ("внутри полосы", math.sqrt(p["minMHz"] * p["maxMHz"])),
                                 ("верхняя граница", p["maxMHz"]),
                                 ("сразу ниже полосы", adjacent(p["minMHz"], -1)),
                                 ("сразу выше полосы", adjacent(p["maxMHz"], 1))):
            add(name + ": " + label, {"frequencyMHz": frequency, "profile": p})

    for status in o.PROVENANCE:
        value = baseline()
        value["profile"]["provenance"] = status
        add("Константы и происхождение " + status, value, {"code": "OK", "material": {
            "epsilonReal": 4, "conductivitySm": .25, "minMHz": 1000, "maxMHz": 1000, "provenance": status}})
    change("Единица MHz→GHz, положительные показатели", {"a": 1, "b": 1, "c": .125, "d": 2}, 2000)
    change("Отрицательные показатели", {"a": 4, "b": -1, "c": .25, "d": -2}, 2000)
    change("Воздух при огромном показателе проводимости", {"a": 1, "c": 0, "d": 3e38}, 2000)
    change("Показатели при fGHz=1", {"b": 3e38, "d": -3e38}, 1000)
    change("Точечный профиль", {"minMHz": 1000, "maxMHz": 1000})
    change("Пустая полуоткрытая полоса", {"minMHz": 1000, "maxMHz": 1000, "maxInclusive": False}, expected="INVALID_PROFILE")
    change("Глобальная нижняя область", frequency=.5, expected="OUT_OF_MODEL_DOMAIN")
    change("Глобальная верхняя область", {"maxMHz": 1e6}, 500000, "OUT_OF_MODEL_DOMAIN")
    change("Оба численных предела MS03", {"a": 100, "c": 1e8})
    change("ε меньше единицы", {"a": .5}, expected="NUMERIC_DOMAIN_LIMIT")
    change("ε больше 100", {"a": adjacent(100, 1)}, expected="NUMERIC_DOMAIN_LIMIT")
    change("σ больше 1e8", {"c": adjacent(1e8, 1)}, expected="NUMERIC_DOMAIN_LIMIT")
    change("Малая положительная σ без вычисления степени", {"c": adjacent(0, 1)})
    change("Потеря положительной σ при underflow", {"c": 1e-30, "d": 100}, 1, "NUMERIC_FAILURE")
    change("Переполнение σ", {"c": 1, "d": 100}, 100000, "NUMERIC_FAILURE")
    change("Потеря ε при underflow", {"a": 1, "b": 100}, 1, "NUMERIC_FAILURE")
    change("Переполнение ε", {"a": 1, "b": 100}, 100000, "NUMERIC_FAILURE")
    change("Конечное произведение при непредставимой отдельной степени", {"a": 1e-38, "b": 19}, 110000)
    change("Сначала представимость обоих, затем предел ε", {"a": .5, "d": 100}, 100000, "NUMERIC_FAILURE")
    change("Профиль до частотной области", {"a": 0}, .5, "INVALID_PROFILE")
    change("Частота до профиля", {"a": 0}, 0, "INVALID_INPUT")
    for name, bad in (("a", 0), ("c", -1), ("minMHz", 0), ("maxMHz", 0), ("provenance", "UNKNOWN")):
        change("Неверное значение " + name, {name: bad}, expected="INVALID_PROFILE")
    change("Переставленные границы", {"minMHz": 2000, "maxMHz": 1000}, expected="INVALID_PROFILE")
    value = baseline()
    value["profile"] = None
    add("Отсутствующий объект профиля", value, {"code": "INVALID_PROFILE"})
    for name, bad in (("a", True), ("b", "0"), ("c", None), ("d", 1e40), ("maxInclusive", 1)):
        change("Неверный JSON-тип/диапазон " + name, {name: bad}, expected="INVALID_PROFILE", native=False)
    for bad in (True, None, "1000", 1e40):
        change("Неверный JSON-вход частоты", frequency=bad, expected="INVALID_INPUT", native=False)
    value = baseline()
    value["profile"]["extra"] = 1
    add("Лишнее поле профиля", value, {"code": "INVALID_PROFILE"}, False)
    value = baseline()
    del value["profile"]["a"]
    add("Отсутствующее поле профиля", value, {"code": "INVALID_PROFILE"}, False)
    value = baseline()
    value["extra"] = 1
    add("Лишнее поле входа", value, {"code": "INVALID_INPUT"}, False)
    add("Отсутствующее поле входа", {"frequencyMHz": 1000}, {"code": "INVALID_INPUT"}, False)
    change("Вычисленная положительная субнормальная σ binary32", {"c": 1e-30, "d": 10}, 100)
    change("Положительная σ binary64 теряется только при округлении binary32", {"c": 1e-30, "d": 16}, 100,
           "NUMERIC_FAILURE")
    return rows


def build(check=False):
    data = {"schema": "ricon.oracle.material_profile", "version": 1, "revision": 2,
            "source": "docs/req/REQ-material-profile.md", "requirementsLfSha256": o.lf_sha(o.REQ),
            "catalogLfSha256": o.lf_sha(o.CATALOG),
            "note": "P3.2b: профили отдельных строк Table 3 и граничные случаи, без игры/геометрии.", "cases": cases()}
    text = json.dumps(data, ensure_ascii=False, indent=2, allow_nan=False) + "\n"
    if check:
        if TARGET.read_text(encoding="utf-8") != text:
            raise SystemExit("Корпус P3.2b отличается; --check не пишет файлы")
    else:
        TARGET.write_text(text, encoding="utf-8", newline="\n")
    print(f"PASS: {len(data['cases'])} случаев P3.2b; native={sum(c['native'] for c in data['cases'])}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Сборка/сверка корпуса частотных профилей")
    parser.add_argument("--check", action="store_true")
    build(parser.parse_args().check)
