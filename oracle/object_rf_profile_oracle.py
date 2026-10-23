#!/usr/bin/env python3
"""P3.1: чистое определение RF-профиля по REQ-object-rf-profile, без мира и RF-математики."""
import argparse
import json
from pathlib import Path

from channel_fading_oracle import fields, key


def forest(value):
    if type(value) is not dict:
        raise ValueError("Требуется граф родителей")
    for node, parent in value.items():
        key(node)
        if parent is not None and key(parent) not in value:
            raise ValueError("Отсутствует родитель")
    done = set()
    for node in value:
        path, visiting = [], set()
        while node is not None and node not in done:
            if node in visiting:
                raise ValueError("Цикл наследования")
            path.append(node)
            visiting.add(node)
            node = value[node]
        done.update(path)
    return value


def rules(value, parents, profiles):
    if type(value) is not dict:
        raise ValueError("Требуются правила")
    for node, profile in value.items():
        if key(node) not in parents or key(profile) not in profiles:
            raise ValueError("Правило ссылается на отсутствующий узел или профиль")
    return value


def result(code, knowledge, material, source, matched):
    return dict(code=code, knowledge=knowledge, materialId=material, source=source, matchedKey=matched)


def evaluate(value):
    try:
        fields(value, "object profiles prefabParents classParents prefabRules classRules")
        obj = value["object"]
        fields(obj, "componentProfileId surfaceProfileId prefabId classId ballisticHint")
        if type(value["profiles"]) is not list:
            raise ValueError("Требуется список профилей")
        profiles = {key(p) for p in value["profiles"]}
        if len(profiles) != len(value["profiles"]):
            raise ValueError("Повторный профиль")
        prefab, classes = forest(value["prefabParents"]), forest(value["classParents"])
        prefab_rules = rules(value["prefabRules"], prefab, profiles)
        class_rules = rules(value["classRules"], classes, profiles)
        for item in obj.values():
            if type(item) is not str:
                raise ValueError("Требуется строка")
            if item:
                key(item)
        for name, parents in (("prefabId", prefab), ("classId", classes)):
            if obj[name] and obj[name] not in parents:
                raise ValueError("Неизвестный конкретный узел")
    except (ValueError, TypeError, KeyError):
        return {"code": "INVALID_INPUT"}
    for name, source in (("componentProfileId", "COMPONENT"), ("surfaceProfileId", "MATERIAL")):
        profile = obj[name]
        if profile:
            if profile in profiles:
                return result("RESOLVED", "PROFILE", profile, source, profile)
            return result("UNRESOLVED_PROFILE", "UNKNOWN", "", source, profile)
    for name, parents, mapping, source in (("prefabId", prefab, prefab_rules, "PREFAB"),
                                           ("classId", classes, class_rules, "CLASS")):
        node = obj[name] or None
        while node is not None:
            if node in mapping:
                return result("RESOLVED", "RULE", mapping[node], source, node)
            node = parents[node]
    hint = obj["ballisticHint"]
    return result("UNCLASSIFIED", "HINT_ONLY" if hint else "UNKNOWN", "", "HINT" if hint else "NONE", hint)


def baseline():
    return dict(object=dict(componentProfileId="rf.metal", surfaceProfileId="rf.wood", prefabId="mod.shed",
                            classId="ModShed", ballisticHint="ballistic.metal"),
                profiles=["rf.wood", "rf.metal", "rf.concrete", "rf.glass"],
                prefabParents={"mod.shed": "base.shed", "base.shed": "base.entity", "base.entity": None},
                classParents={"ModShed": "Building", "Building": "Entity", "Entity": None},
                prefabRules={"base.shed": "rf.wood", "base.entity": "rf.metal"}, classRules={"Building": "rf.concrete"})


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", type=Path, required=True)
    args = parser.parse_args()
    cases = json.loads(args.check.read_bytes())["cases"]
    for case in cases:
        actual = evaluate(case["input"])
        if actual != case["expect"]:
            raise SystemExit(f"OBJECT_RF_PROFILE_FAIL {case['id']}: {actual}")
    print(f"OBJECT_RF_PROFILE_PASS: {len(cases)}")


if __name__ == "__main__":
    main()
