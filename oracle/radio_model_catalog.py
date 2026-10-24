"""Частичный каталог паспортов: точный offline-поиск, без сборки профиля S2."""
import argparse
import copy
import json
import math
from pathlib import Path
import re


DEFAULT_PATH = Path(__file__).parent / "data" / "radio_model_catalog_v1.json"
PREFIX = "$ArmaReforger:Prefabs/Items/Equipment/Radios/"
QUANTITIES = {"frequencyMHz", "channelSpacingKHz", "presetChannelSpacingKHz",
              "tuningReadoutKHz", "txPowerW", "rxSensitivityUv", "supplyVoltageV", "antennaLengthM"}
RELATIONS = {"range", "exact", "specified", "nominal", "minimum", "greater_than", "maximum", "less_than"}


def need(condition, message):
    if not condition:
        raise ValueError(message)


def fields(value, names):
    need(isinstance(value, dict) and set(value) == set(names.split()), "Неверный набор полей")


def text(value):
    return isinstance(value, str) and bool(value.strip())


def positive(value):
    try:
        return type(value) in (int, float) and math.isfinite(value) and value > 0
    except OverflowError:
        return False


def prefab(value):
    return (text(value) and value.startswith(PREFIX) and value.endswith(".et")
            and "\\" not in value and ".." not in value.split("/"))


def validate_catalog(catalog):
    fields(catalog, "schemaVersion gameBuild sources models bindings basePrefabs")
    need(type(catalog["schemaVersion"]) is int and catalog["schemaVersion"] == 1, "Неизвестная схема")
    need(text(catalog["gameBuild"]), "Нет версии игры")
    sources, models = catalog["sources"], catalog["models"]
    need(isinstance(sources, dict) and sources and isinstance(models, dict) and models, "Пустые справочники")
    for source_id, source in sources.items():
        need(text(source_id), "Пустой ID источника")
        fields(source, "kind title uri sha256")
        need(source["kind"] in ("PRIMARY_MANUAL", "WORKBENCH_STATIC", "REPO_REPORT"), "Неверный вид источника")
        need(text(source["title"]) and text(source["uri"]), "Нет описания источника")
        need(isinstance(source["sha256"], str) and re.fullmatch(r"[0-9a-f]{64}", source["sha256"]), "Неверный SHA")
    for model_id, model in models.items():
        need(text(model_id), "Пустой ID модели")
        fields(model, "name variant facts unknowns")
        need(text(model["name"]) and text(model["variant"]), "Нет имени или варианта")
        need(isinstance(model["unknowns"], list) and model["unknowns"]
             and all(text(item) for item in model["unknowns"]), "Не описаны неизвестные параметры")
        need(isinstance(model["facts"], list), "Факты должны быть списком")
        for fact in model["facts"]:
            fields(fact, "quantity value relation sourceId locator conditions")
            need(isinstance(fact["quantity"], str) and fact["quantity"] in QUANTITIES, "Неизвестная величина")
            need(isinstance(fact["relation"], str) and fact["relation"] in RELATIONS, "Неверная семантика числа")
            source_id = fact["sourceId"]
            need(isinstance(source_id, str) and source_id in sources
                 and sources[source_id]["kind"] == "PRIMARY_MANUAL", "Нет первичного источника факта")
            need(text(fact["locator"]) and text(fact["conditions"]), "Нет страницы или условий")
            value = fact["value"]
            if fact["relation"] == "range":
                need(isinstance(value, list) and len(value) == 2 and all(positive(v) for v in value)
                     and value[0] <= value[1], "Неверные границы")
            else:
                need(positive(value), "Неверное число")
    bases = catalog["basePrefabs"]
    need(isinstance(bases, list) and all(prefab(p) for p in bases) and len(set(bases)) == len(bases), "Неверные базы")
    bindings = catalog["bindings"]
    need(isinstance(bindings, list) and bindings, "Нет предметов")
    seen = set(bases)
    for binding in bindings:
        fields(binding, "prefab modelId variant nameKey nameFrom nameSource identitySource reportedGame")
        path = binding["prefab"]
        need(prefab(path) and path not in seen, "Повторный или неверный prefab")
        seen.add(path)
        need(isinstance(binding["modelId"], str) and binding["modelId"] in models, "Неизвестная модель")
        need(text(binding["variant"]) and text(binding["nameKey"]) and prefab(binding["nameFrom"]), "Нет идентичности")
        for key in ("nameSource", "identitySource"):
            source_id = binding[key]
            need(isinstance(source_id, str) and source_id in sources
                 and sources[source_id]["kind"] == "WORKBENCH_STATIC", "Нет игрового источника")
        game = binding["reportedGame"]
        fields(game, "sourceId tuningUnit transceivers")
        need(isinstance(game["sourceId"], str) and game["sourceId"] in sources
             and sources[game["sourceId"]]["kind"] == "REPO_REPORT", "Нет отчёта о настройках")
        need(game["tuningUnit"] == "UNVERIFIED" and isinstance(game["transceivers"], list), "Неверный снимок настроек")
        indices = set()
        for tr in game["transceivers"]:
            fields(tr, "index minRaw maxRaw stepRaw basis")
            need(type(tr["index"]) is int and tr["index"] >= 0 and tr["index"] not in indices, "Неверный индекс")
            indices.add(tr["index"])
            need(text(tr["basis"]), "Нет основания настроек")
            for key in ("minRaw", "maxRaw", "stepRaw"):
                need(tr[key] is None or (type(tr[key]) is int and 0 < tr[key] < 2**31), "Неверное сырое число")
            need((tr["minRaw"] is None) == (tr["maxRaw"] is None), "Неполный диапазон")
            need(tr["minRaw"] is None or tr["minRaw"] <= tr["maxRaw"], "Перевёрнутый диапазон")
    return catalog


def unique_object(pairs):
    result = {}
    for key, value in pairs:
        need(key not in result, "Повторный JSON-ключ: " + key)
        result[key] = value
    return result


def load_catalog(path=DEFAULT_PATH):
    with Path(path).open(encoding="utf-8-sig") as stream:
        data = json.load(stream, object_pairs_hook=unique_object,
                         parse_constant=lambda value: need(False, "Недопустимая константа: " + value))
    return validate_catalog(data)


def lookup(catalog, prefab_path, game_build):
    validate_catalog(catalog)
    result = {"code": "INVALID_INPUT", "readyForS2": False}
    if not text(prefab_path) or not text(game_build):
        return result
    if game_build != catalog["gameBuild"]:
        result["code"] = "BUILD_MISMATCH"
    elif prefab_path in catalog["basePrefabs"]:
        result["code"] = "BASE_PREFAB"
    else:
        result["code"] = "UNKNOWN_PREFAB"
        for binding in catalog["bindings"]:
            if binding["prefab"] == prefab_path:
                model = catalog["models"][binding["modelId"]]
                result.update(code="PASSPORT_PARTIAL" if model["facts"] else "IDENTITY_ONLY",
                              binding=copy.deepcopy(binding), model=copy.deepcopy(model))
                break
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--catalog", type=Path, default=DEFAULT_PATH)
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--prefab")
    parser.add_argument("--build")
    args = parser.parse_args()
    catalog = load_catalog(args.catalog)
    if args.check:
        print("RADIO_MODEL_CATALOG_PASS models=%d bindings=%d" % (len(catalog["models"]), len(catalog["bindings"])))
    elif args.prefab and args.build:
        print(json.dumps(lookup(catalog, args.prefab, args.build), ensure_ascii=False, allow_nan=False, indent=2))
    else:
        parser.error("Укажите --check либо --prefab и --build")
