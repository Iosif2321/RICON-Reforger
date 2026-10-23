#!/usr/bin/env python3
"""CT10.1: RCFD/1 и перенос якоря CF, stdlib; не Rpl/RPC, не часы и не доставка голоса."""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import struct

import channel_fading_oracle as cf

MAGIC = b"RCFD"
VERSION = 1
MAX_BYTES = 5111
MAX_SUPPORTED_BYTES = 5066


def normal(descriptor):
    """CF preflight без самостоятельной копии его правил; служебный query берётся из descriptor."""
    if type(descriptor) is not dict:
        raise ValueError("Нужен descriptor")
    query = dict(tx=descriptor["a"], rx=descriptor["b"], subUs=0, nowMs=descriptor["validFromMs"])
    for key in ("profileId", "profileVersion", "frequencyMHz", "bandwidthHz", "environmentRevision"):
        query[key] = descriptor[key]
    typed, _ = cf.preflight(dict(descriptor=descriptor, query=query))
    for key in ("a", "b"):
        device, index = typed[key]
        typed[key] = dict(deviceId=device, transceiverIndex=index)
    for component in typed["components"]:
        if component["powerWeight"] == 0:
            component["powerWeight"] = 0.0
    return typed


def limits(value):
    return type(value) is int and 1 <= value <= 65535


def frame(descriptor):
    """Внутренний writer принимает только нормальную форму CF, чисел не пересчитывает."""
    out = bytearray(MAGIC + bytes((VERSION, 0, 0)))

    def key(value):
        encoded = value.encode("ascii")
        out.append(len(encoded))
        out.extend(encoded)

    def integer(value):
        out.extend(struct.pack(">i", value))

    def number(value):
        out.extend(struct.pack(">f", value))

    key(descriptor["modelId"])
    key(descriptor["profileId"])
    integer(descriptor["profileVersion"])
    for endpoint in (descriptor["a"], descriptor["b"]):
        integer(endpoint["deviceId"])
        integer(endpoint["transceiverIndex"])
    number(descriptor["frequencyMHz"])
    number(descriptor["bandwidthHz"])
    for name in ("environmentRevision", "anchorMs", "validFromMs", "validUntilMs"):
        integer(descriptor[name])
    flags = sum(int(descriptor[name]) << bit for bit, name in enumerate(("staticChannel", "coverageComplete", "resourceExhausted")))
    out.extend((flags, len(descriptor["components"])))
    for component in descriptor["components"]:
        key(component["componentId"])
        number(component["powerWeight"])
        integer(component["phaseQ"])
        integer(component["dopplerQ"])
    struct.pack_into(">H", out, 5, len(out))
    return bytes(out)


def encode(descriptor, max_bytes):
    if not limits(max_bytes):
        return dict(code="INVALID_LIMITS")
    try:
        typed = normal(descriptor)
    except (KeyError, TypeError, ValueError, OverflowError):
        return dict(code="INVALID_DESCRIPTOR")
    if typed["modelId"] != cf.MODEL:
        return dict(code="UNSUPPORTED_MODEL")
    raw = frame(typed)
    if len(raw) > max_bytes:
        return dict(code="RESOURCE_LIMIT")
    return dict(code="OK", descriptor=typed, bytes=list(raw), sizeBytes=len(raw))


class Reader:
    def __init__(self, raw):
        self.raw, self.at = raw, 7

    def read(self, format):
        size = struct.calcsize(format)
        if self.at + size > len(self.raw):
            raise ValueError("Неполное тело")
        value = struct.unpack_from(format, self.raw, self.at)[0]
        self.at += size
        return value

    def key(self):
        length = self.read(">B")
        if not 1 <= length <= 64 or self.at + length > len(self.raw):
            raise ValueError("Неверная длина ключа")
        value = self.raw[self.at:self.at + length].decode("ascii")
        self.at += length
        return cf.key(value)

    def descriptor(self):
        d = dict(modelId=self.key(), profileId=self.key(), profileVersion=self.read(">i"))
        for name in ("a", "b"):
            d[name] = dict(deviceId=self.read(">i"), transceiverIndex=self.read(">i"))
        d["frequencyMHz"], d["bandwidthHz"] = self.read(">f"), self.read(">f")
        for name in ("environmentRevision", "anchorMs", "validFromMs", "validUntilMs"):
            d[name] = self.read(">i")
        flags, count = self.read(">B"), self.read(">B")
        if flags & ~7 or count > 64:
            raise ValueError("Неверные флаги/число составляющих")
        for bit, name in enumerate(("staticChannel", "coverageComplete", "resourceExhausted")):
            d[name] = bool(flags & (1 << bit))
        d["components"] = [dict(componentId=self.key(), powerWeight=self.read(">f"),
                                phaseQ=self.read(">i"), dopplerQ=self.read(">i")) for _ in range(count)]
        if self.at != len(self.raw):
            raise ValueError("Лишние байты")
        return normal(d)


def decode(values, max_bytes):
    if not limits(max_bytes):
        return dict(code="INVALID_LIMITS")
    if type(values) is not list:
        return dict(code="INVALID_FRAME")
    if len(values) > max_bytes:
        return dict(code="RESOURCE_LIMIT")
    if not 7 <= len(values) <= MAX_BYTES or any(type(x) is not int or not 0 <= x <= 255 for x in values):
        return dict(code="INVALID_FRAME")
    raw = bytes(values)
    if raw[:4] != MAGIC or struct.unpack_from(">H", raw, 5)[0] != len(raw):
        return dict(code="INVALID_FRAME")
    if raw[4] != VERSION:
        return dict(code="UNSUPPORTED_VERSION")
    try:
        typed = Reader(raw).descriptor()
    except (KeyError, TypeError, ValueError, OverflowError):
        return dict(code="INVALID_FRAME")
    if typed["modelId"] != cf.MODEL:
        return dict(code="UNSUPPORTED_MODEL")
    if frame(typed) != raw:
        return dict(code="INVALID_FRAME")
    return dict(code="OK", descriptor=typed, bytes=list(raw), sizeBytes=len(raw))


def rebase(descriptor, anchor_ms):
    try:
        typed = normal(descriptor)
    except (KeyError, TypeError, ValueError, OverflowError):
        return dict(code="INVALID_DESCRIPTOR")
    if typed["modelId"] != cf.MODEL:
        return dict(code="UNSUPPORTED_MODEL")
    if any(abs(c["dopplerQ"]) > cf.QUARTER for c in typed["components"]):
        return dict(code="OUT_OF_MODEL_DOMAIN")
    if type(anchor_ms) is not int or not typed["validFromMs"] <= anchor_ms < typed["validUntilMs"]:
        return dict(code="INVALID_ANCHOR")
    delta = anchor_ms - typed["anchorMs"]
    for component in typed["components"]:
        component["phaseQ"] = cf.phase_at(component, delta, 0)
    typed["anchorMs"] = typed["validFromMs"] = anchor_ms
    return dict(code="OK", descriptor=typed)


def evaluate(inp):
    op = inp["op"]
    if op == "ENCODE":
        cf.fields(inp, "op descriptor maxBytes")
        return encode(inp["descriptor"], inp["maxBytes"])
    if op == "DECODE":
        cf.fields(inp, "op bytes maxBytes")
        return decode(inp["bytes"], inp["maxBytes"])
    if op == "REBASE":
        cf.fields(inp, "op descriptor anchorMs")
        return rebase(inp["descriptor"], inp["anchorMs"])
    raise ValueError("Неизвестная операция корпуса")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", type=Path, required=True)
    args = parser.parse_args()
    cases = json.loads(args.check.read_text(encoding="utf-8"))["cases"]
    for case in cases:
        if evaluate(case["input"]) != case["expect"]:
            raise SystemExit("CHANNEL_DESCRIPTOR_FAIL: " + case["id"])
    print(f"CHANNEL_DESCRIPTOR_PASS: {len(cases)}")


if __name__ == "__main__":
    main()
