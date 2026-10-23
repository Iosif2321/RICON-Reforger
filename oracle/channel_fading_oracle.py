#!/usr/bin/env python3
"""P1b.1 DRAFT: REQ-channel-fading, чистый коэффициент среды, без мира/голоса/часов."""
from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
import re

from radio_device_oracle import f32, int32

M = 1 << 30
QUARTER = M // 4
MODEL = "FINITE_MULTIPATH_V1"
MAX = (1 << 31) - 1
KEY = re.compile(r"[A-Za-z0-9_.:-]{1,64}\Z")
D_FIELDS = "modelId profileId profileVersion a b frequencyMHz bandwidthHz environmentRevision anchorMs validFromMs validUntilMs staticChannel coverageComplete resourceExhausted components"
Q_FIELDS = "tx rx frequencyMHz bandwidthHz profileId profileVersion environmentRevision nowMs subUs"


def fields(value, names):
    if type(value) is not dict or set(value) != set(names.split()):
        raise ValueError("Неверные поля")


def integer(value, minimum=0, maximum=MAX):
    if not int32(value) or not minimum <= value <= maximum:
        raise ValueError("Неверный int32")
    return value


def key(value):
    if type(value) is not str or not KEY.fullmatch(value):
        raise ValueError("Неверный ключ")
    return value


def number(value, minimum=0, maximum=math.inf, positive=False):
    if type(value) not in (int, float):
        raise ValueError("Требуется число")
    typed = f32(value)
    if not math.isfinite(typed) or not minimum <= typed <= maximum or (positive and typed == 0):
        raise ValueError("Неверный binary32")
    return typed


def endpoint(value):
    fields(value, "deviceId transceiverIndex")
    return integer(value["deviceId"], 1), integer(value["transceiverIndex"])


def preflight(raw):
    fields(raw, "descriptor query")
    d, q = raw["descriptor"], raw["query"]
    fields(d, D_FIELDS)
    fields(q, Q_FIELDS)
    typed_d, typed_q = dict(d), dict(q)
    for obj in (typed_d, typed_q):
        obj["profileId"] = key(obj["profileId"])
        obj["profileVersion"] = integer(obj["profileVersion"], 1)
        obj["environmentRevision"] = integer(obj["environmentRevision"], 1)
        obj["frequencyMHz"] = number(obj["frequencyMHz"], positive=True)
        obj["bandwidthHz"] = number(obj["bandwidthHz"], positive=True)
    typed_d["modelId"] = key(d["modelId"])
    typed_d["a"], typed_d["b"] = endpoint(d["a"]), endpoint(d["b"])
    typed_q["tx"], typed_q["rx"] = endpoint(q["tx"]), endpoint(q["rx"])
    if typed_d["a"] >= typed_d["b"] or typed_q["tx"] == typed_q["rx"]:
        raise ValueError("Неверная физическая пара")
    for name in ("anchorMs", "validFromMs", "validUntilMs"):
        typed_d[name] = integer(d[name])
    if not d["anchorMs"] <= d["validFromMs"] < d["validUntilMs"]:
        raise ValueError("Неверный интервал")
    typed_q["nowMs"] = integer(q["nowMs"])
    typed_q["subUs"] = integer(q["subUs"], 0, 999)
    for name in ("staticChannel", "coverageComplete", "resourceExhausted"):
        if type(d[name]) is not bool:
            raise ValueError("Требуется bool")
    if type(d["components"]) is not list or len(d["components"]) > 64:
        raise ValueError("Неверный набор компонентов")
    components, previous = [], None
    for component in d["components"]:
        fields(component, "componentId powerWeight phaseQ dopplerQ")
        cid = key(component["componentId"])
        if previous is not None and cid <= previous:
            raise ValueError("Неверный порядок/дубликат")
        previous = cid
        weight = number(component["powerWeight"], maximum=1)
        phase = integer(component["phaseQ"], 0, M - 1)
        doppler = integer(component["dopplerQ"], -(1 << 31))
        if d["staticChannel"] and doppler != 0:
            raise ValueError("Доплер при статичной среде")
        components.append(dict(componentId=cid, powerWeight=weight, phaseQ=phase, dopplerQ=doppler))
    typed_d["components"] = components
    return typed_d, typed_q


def phase_at(component, delta_ms, sub_us):
    """Точная целочисленная арифметика, независимо от будущего native MulMod."""
    return (component["phaseQ"] + component["dopplerQ"] * (delta_ms * 1000 + sub_us)) % M


def horner(z, coefficients):
    result = f32(coefficients[-1])
    for coefficient in reversed(coefficients[:-1]):
        result = f32(f32(coefficient) + f32(z * result))
    return result


def phasor(q):
    """Нормативная f32-аппроксимация CF07; независимые sin/cos проверяют её отдельно."""
    quadrant = (q + M // 8) // QUARTER
    remainder = q - quadrant * QUARTER
    if remainder == 0:
        cosine, sine = 1.0, 0.0
    else:
        x = f32(f32(remainder / M) * f32(2 * math.pi))
        z = f32(x * x)
        sine = f32(x * horner(z, (1, -1/6, 1/120, -1/5040, 1/362880, -1/39916800)))
        cosine = horner(z, (1, -1/2, 1/24, -1/720, 1/40320, -1/3628800, 1/479001600))
    return ((cosine, sine), (-sine, cosine), (-cosine, -sine), (sine, -cosine))[quadrant % 4]


def envelope32(real, imag):
    scale = max(abs(real), abs(imag))
    if scale == 0:
        return 0.0
    x, y = f32(real / scale), f32(imag / scale)
    squares = f32(f32(x * x) + f32(y * y))
    return f32(scale * f32(math.sqrt(squares)))


def evaluate(raw):
    def result(code, real=None, imag=None, gain=None, phases=None):
        return dict(code=code, real=real, imag=imag, gainDb=gain, phasesQ=[] if phases is None else phases)

    try:
        d, q = preflight(raw)
    except (ValueError, TypeError, OverflowError):
        return result("INVALID_INPUT")
    if d["modelId"] != MODEL:
        return result("UNSUPPORTED_MODEL")
    if d["resourceExhausted"]:
        return result("RESOURCE_EXHAUSTED")
    if not d["coverageComplete"]:
        return result("INCOMPLETE_COVERAGE")
    pair = tuple(sorted((q["tx"], q["rx"])))
    if pair != (d["a"], d["b"]) or any(q[name] != d[name] for name in
            ("frequencyMHz", "bandwidthHz", "profileId", "profileVersion", "environmentRevision")):
        return result("CONTEXT_MISMATCH")
    if not d["validFromMs"] <= q["nowMs"] < d["validUntilMs"]:
        return result("STALE_DESCRIPTOR")
    if any(abs(c["dopplerQ"]) > QUARTER for c in d["components"]):
        return result("OUT_OF_MODEL_DOMAIN")
    total_weight = 0.0
    for c in d["components"]:
        total_weight = f32(total_weight + c["powerWeight"])
    if total_weight == 0:
        return result("EMPTY_MEDIUM", 0.0, 0.0)
    normalizer = f32(math.sqrt(total_weight))
    if not math.isfinite(normalizer) or normalizer <= 0:
        return result("NUMERIC_FAILURE")
    real, imag, phases = 0.0, 0.0, []
    for c in d["components"]:
        phase = phase_at(c, q["nowMs"] - d["anchorMs"], q["subUs"])
        phases.append(phase)
        amplitude = f32(f32(math.sqrt(c["powerWeight"])) / normalizer)
        cosine, sine = phasor(phase)
        real = f32(real + f32(amplitude * cosine))
        imag = f32(imag + f32(amplitude * sine))
    if not all(math.isfinite(x) for x in (real, imag)):
        return result("NUMERIC_FAILURE")
    if real == 0 and imag == 0:
        return result("ZERO_AT_SAMPLE", 0.0, 0.0, phases=phases)
    envelope = envelope32(real, imag)
    if not math.isfinite(envelope) or envelope <= 0:
        return result("NUMERIC_FAILURE")
    gain = 20 * math.log10(envelope)
    if not math.isfinite(f32(gain)):
        return result("NUMERIC_FAILURE")
    return result("OK", real, imag, gain, phases)


def baseline():
    a, b = dict(deviceId=10, transceiverIndex=0), dict(deviceId=20, transceiverIndex=0)
    d = dict(modelId=MODEL, profileId="TEST_ONLY", profileVersion=1, a=a, b=b, frequencyMHz=150.0,
             bandwidthHz=25000.0, environmentRevision=1, anchorMs=1000, validFromMs=1000, validUntilMs=5000,
             staticChannel=True, coverageComplete=True, resourceExhausted=False,
             components=[dict(componentId="direct", powerWeight=1.0, phaseQ=0, dopplerQ=0)])
    q = dict(tx=dict(a), rx=dict(b), frequencyMHz=150.0, bandwidthHz=25000.0, profileId="TEST_ONLY",
             profileVersion=1, environmentRevision=1, nowMs=1000, subUs=0)
    return dict(descriptor=d, query=q)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", type=Path, required=True)
    args = parser.parse_args()
    corpus = json.loads(args.check.read_text(encoding="utf-8"))
    failures = [case["id"] for case in corpus["cases"] if evaluate(case["input"]) != case["expect"]]
    print(f"CHANNEL_FADING_ORACLE_{'FAIL' if failures else 'PASS'}: {len(corpus['cases'])}, ошибки={failures}")
    raise SystemExit(bool(failures))


if __name__ == "__main__":
    main()
