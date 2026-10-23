"""P3.2a: черновой host-эталон потерь пластины, REQ-material-slab; без мира и голоса."""
from __future__ import annotations

import argparse
import cmath
import hashlib
import json
import math
from pathlib import Path
import re

from channel_fading_oracle import fields, number
from path_observation_oracle import C, F32_MAX, f32

MIN_COSINE = f32(.05)
PROVENANCE = ("SOURCE_DOMAIN", "ASSUMED_CONFIG", "TEST_ONLY")
ROOT = Path(__file__).resolve().parents[1]
CODES = ("INVALID_INPUT", "INVALID_PROFILE", "OUT_OF_MODEL_DOMAIN", "NUMERIC_DOMAIN_LIMIT", "NUMERIC_FAILURE", "OK")


def slab(frequency, thickness, cosine, polarization, epsilon, sigma):
    """MS05–07; входы уже проверены и округлены. Результат не содержит фазы канала."""
    if thickness == 0 or (epsilon == 1 and sigma == 0):
        return {"code": "OK", "lossDb": 0.0}
    e = complex(epsilon, -17.98 * sigma / (frequency / 1000))
    p = cmath.sqrt(e - 1 + cosine * cosine)
    q = (2 * math.pi * frequency * 1e6 / C) * thickness * p
    x, y = -q.imag, q.real
    a, b = (cosine if polarization == "TE" else e * cosine), p
    u, v = a / (a + b), b / (a + b)
    interface, reflection = 4 * u * v, u - v
    if x >= 12:
        denominator = 1
    else:
        decay = math.exp(-2 * x)
        remainder = complex(-math.expm1(-2 * x) + 2 * decay * math.sin(y) ** 2,
                            decay * math.sin(2 * y))
        denominator = interface + reflection * reflection * remainder
    loss = (20 / math.log(10)) * (x + math.log(abs(denominator)) - math.log(abs(interface)))
    if not all(math.isfinite(t) and abs(t) <= F32_MAX for t in (p.real, p.imag, x, y, loss)) or loss < -.001:
        return {"code": "NUMERIC_FAILURE"}
    return {"code": "OK", "lossDb": max(0.0, loss)}


def evaluate(raw):
    try:
        fields(raw, "frequencyMHz thicknessM cosIncidence polarization material")
        frequency = number(raw["frequencyMHz"], positive=True)
        thickness = number(raw["thicknessM"])
        cosine = number(raw["cosIncidence"], maximum=1)
        polarization = raw["polarization"]
        if type(polarization) is not str or polarization not in ("TE", "TM"):
            raise ValueError("Неизвестная поляризация")
    except (ValueError, TypeError, OverflowError):
        return {"code": "INVALID_INPUT"}
    try:
        material = raw["material"]
        fields(material, "epsilonReal conductivitySm minMHz maxMHz provenance")
        epsilon = number(material["epsilonReal"], minimum=1)
        sigma = number(material["conductivitySm"])
        lo = number(material["minMHz"], positive=True)
        hi = number(material["maxMHz"], positive=True)
        provenance = material["provenance"]
        if lo > hi or type(provenance) is not str or provenance not in PROVENANCE:
            raise ValueError("Неверная область или происхождение")
    except (ValueError, TypeError, OverflowError):
        return {"code": "INVALID_PROFILE"}
    if not 1 <= frequency <= 450000 or not lo <= frequency <= hi:
        return {"code": "OUT_OF_MODEL_DOMAIN"}
    if f32(frequency * thickness) > 500 or cosine < MIN_COSINE or epsilon > 100 or sigma > 1e8:
        return {"code": "NUMERIC_DOMAIN_LIMIT"}
    try:
        result = slab(frequency, thickness, cosine, polarization, epsilon, sigma)
    except (ArithmeticError, ValueError):
        return {"code": "NUMERIC_FAILURE"}
    if result["code"] == "OK":
        result["provenance"] = provenance
    return result


def baseline():
    return {"frequencyMHz": 100.0, "thicknessM": .2, "cosIncidence": 1.0, "polarization": "TE",
            "material": {"epsilonReal": 4.0, "conductivitySm": 0.0, "minMHz": 1.0,
                         "maxMHz": 450000.0, "provenance": "TEST_ONLY"}}


def check(path):
    data = json.loads(path.read_text(encoding="utf-8"))
    fields(data, "schema version revision source requirementsLfSha256 note cases")
    req_sha = hashlib.sha256((ROOT / "docs/req/REQ-material-slab.md").read_bytes().replace(b"\r\n", b"\n")).hexdigest()
    if (data["schema"] != "ricon.oracle.material_slab" or type(data["version"]) is not int or data["version"] != 1
            or type(data["revision"]) is not int or not 1 <= data["revision"] <= 2147483647
            or data["source"] != "docs/req/REQ-material-slab.md" or data["requirementsLfSha256"] != req_sha
            or type(data["cases"]) is not list or not 1 <= len(data["cases"]) <= 256):
        raise ValueError("Неверная схема корпуса")
    errors = []
    ids = set()
    for case in data["cases"]:
        fields(case, "id note native provenance input expect")
        expected = case["expect"]
        if (type(case["id"]) is not str or re.fullmatch(r"MS[0-9]{3}", case["id"]) is None
                or type(case["native"]) is not bool or case["provenance"] not in ("HAND_DERIVED", "ORACLE_DERIVED")
                or type(expected) is not dict or type(expected.get("code")) is not str or expected["code"] not in CODES):
            raise ValueError("Неверный случай корпуса")
        fields(expected, "code lossDb provenance" if expected["code"] == "OK" else "code")
        if expected["code"] == "OK" and (type(expected["lossDb"]) not in (int, float)
                or not math.isfinite(expected["lossDb"]) or expected["lossDb"] < 0 or expected["provenance"] not in PROVENANCE):
            raise ValueError("Неверное ожидаемое значение")
        if case["id"] in ids:
            errors.append(case["id"] + ": повтор id")
        ids.add(case["id"])
        actual = evaluate(case["input"])
        if set(actual) != set(expected):
            errors.append(case["id"] + ": поля результата")
            continue
        for key, value in expected.items():
            if key == "lossDb":
                same = math.isclose(actual[key], value, rel_tol=1e-12, abs_tol=1e-8)
            else:
                same = actual[key] == value
            if not same:
                errors.append(case["id"] + ": " + key)
    for error in errors:
        print(error)
    print(f"{'PASS' if not errors else 'FAIL'}: {len(data['cases'])} случаев P3.2a")
    return int(bool(errors))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Проверка host-корпуса однослойной пластины")
    parser.add_argument("--check", type=Path, required=True)
    raise SystemExit(check(parser.parse_args().check))
