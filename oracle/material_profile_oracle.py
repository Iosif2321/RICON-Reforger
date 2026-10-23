"""P3.2b: частотный профиль по P.2040-4 (57–58), без распознавания объектов."""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import re

from channel_fading_oracle import fields, key, number
from material_slab_oracle import PROVENANCE, CODES
from path_observation_oracle import f32

ROOT = Path(__file__).resolve().parents[1]
REQ = ROOT / "docs/req/REQ-material-profile.md"
CATALOG = ROOT / "oracle/data/material_profiles_p2040_v1.json"
SOURCE = "https://www.itu.int/rec/R-REC-P.2040-4-202509-I/en"


def evaluate(raw):
    try:
        fields(raw, "frequencyMHz profile")
        frequency = number(raw["frequencyMHz"], positive=True)
    except (ValueError, TypeError, OverflowError):
        return {"code": "INVALID_INPUT"}
    try:
        profile = raw["profile"]
        fields(profile, "a b c d minMHz maxMHz maxInclusive provenance")
        a = number(profile["a"], positive=True)
        b = number(profile["b"], minimum=-math.inf)
        c = number(profile["c"])
        d = number(profile["d"], minimum=-math.inf)
        lo = number(profile["minMHz"], positive=True)
        hi = number(profile["maxMHz"], positive=True)
        inclusive, provenance = profile["maxInclusive"], profile["provenance"]
        if (type(inclusive) is not bool or lo > hi or (lo == hi and not inclusive)
                or type(provenance) is not str or provenance not in PROVENANCE):
            raise ValueError("Неверная полоса или происхождение")
    except (ValueError, TypeError, OverflowError):
        return {"code": "INVALID_PROFILE"}
    if not 1 <= frequency <= 450000 or frequency < lo or frequency > hi or (frequency == hi and not inclusive):
        return {"code": "OUT_OF_MODEL_DOMAIN"}
    try:
        log_frequency = math.log(frequency / 1000)
        epsilon = a if b == 0 else f32(math.exp(math.log(a) + b * log_frequency))
        sigma = c if c == 0 or d == 0 else f32(math.exp(math.log(c) + d * log_frequency))
    except (ArithmeticError, ValueError):
        return {"code": "NUMERIC_FAILURE"}
    if not math.isfinite(epsilon) or not math.isfinite(sigma) or epsilon == 0 or (c > 0 and sigma == 0):
        return {"code": "NUMERIC_FAILURE"}
    if not 1 <= epsilon <= 100 or sigma > 1e8:
        return {"code": "NUMERIC_DOMAIN_LIMIT"}
    return {"code": "OK", "material": {"epsilonReal": epsilon, "conductivitySm": sigma,
            "minMHz": frequency, "maxMHz": frequency, "provenance": provenance}}


def unique_pairs(pairs):
    result = {}
    for name, value in pairs:
        if name in result:
            raise ValueError("Повтор ключа JSON: " + name)
        result[name] = value
    return result


def read_json(path):
    return json.loads(path.read_text(encoding="utf-8"), object_pairs_hook=unique_pairs)


def lf_sha(path):
    return hashlib.sha256(path.read_bytes().replace(b"\r\n", b"\n")).hexdigest()


def load_catalog(path=CATALOG):
    data = read_json(path)
    fields(data, "schema version source note profiles")
    if (data["schema"] != "ricon.material_profiles.p2040" or type(data["version"]) is not int
            or data["version"] != 1 or data["source"] != SOURCE or type(data["note"]) is not str
            or type(data["profiles"]) is not dict or len(data["profiles"]) != 38):
        raise ValueError("Неверная схема каталога")
    rows = set()
    for name, entry in data["profiles"].items():
        key(name)
        fields(entry, "labelRu sourceRow profile")
        row, profile = entry["sourceRow"], entry["profile"]
        if (type(row) is not int or row < 1 or row in rows or type(entry["labelRu"]) is not str
                or not entry["labelRu"].strip() or type(profile) is not dict):
            raise ValueError("Неверная строка каталога")
        result = evaluate({"frequencyMHz": profile.get("minMHz"), "profile": profile})
        if result["code"] != "OK" or profile["provenance"] != "SOURCE_DOMAIN":
            raise ValueError("Неверный профиль каталога: " + name)
        rows.add(row)
    if rows != set(range(1, 39)):
        raise ValueError("Неполная нумерация Table 3")
    return data["profiles"]


def check(path):
    data = read_json(path)
    fields(data, "schema version revision source requirementsLfSha256 catalogLfSha256 note cases")
    if (data["schema"] != "ricon.oracle.material_profile" or type(data["version"]) is not int or data["version"] != 1
            or type(data["revision"]) is not int or not 1 <= data["revision"] < 2147483647
            or data["source"] != "docs/req/REQ-material-profile.md" or data["requirementsLfSha256"] != lf_sha(REQ)
            or data["catalogLfSha256"] != lf_sha(CATALOG) or type(data["note"]) is not str
            or type(data["cases"]) is not list or not 1 <= len(data["cases"]) <= 512):
        raise ValueError("Неверная схема корпуса")
    load_catalog()
    ids, errors = set(), []
    for case in data["cases"]:
        fields(case, "id note native provenance input expect")
        expected = case["expect"]
        if (type(case["id"]) is not str or re.fullmatch(r"MP[0-9]{3}", case["id"]) is None or case["id"] in ids
                or type(case["note"]) is not str or type(case["native"]) is not bool
                or case["provenance"] not in ("HAND_DERIVED", "ORACLE_DERIVED", "CROSS_CHECKED")
                or type(expected) is not dict or expected.get("code") not in CODES):
            raise ValueError("Неверный случай корпуса")
        ids.add(case["id"])
        fields(expected, "code material" if expected["code"] == "OK" else "code")
        actual = evaluate(case["input"])
        if actual.get("code") != expected["code"]:
            errors.append(case["id"] + ": code")
        elif expected["code"] == "OK":
            material = expected["material"]
            fields(material, "epsilonReal conductivitySm minMHz maxMHz provenance")
            for name, value in material.items():
                if name == "provenance":
                    same = type(value) is str and value in PROVENANCE and value == actual["material"][name]
                else:
                    same = (type(value) in (int, float) and math.isfinite(value) and value >= 0
                            and math.isclose(value, actual["material"][name], rel_tol=1e-12, abs_tol=0))
                if not same:
                    errors.append(case["id"] + ": " + name)
    for error in errors:
        print(error)
    print(f"{'PASS' if not errors else 'FAIL'}: {len(data['cases'])} случаев P3.2b")
    return int(bool(errors))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Проверка корпуса частотных профилей материала")
    parser.add_argument("--check", type=Path, required=True)
    raise SystemExit(check(parser.parse_args().check))
