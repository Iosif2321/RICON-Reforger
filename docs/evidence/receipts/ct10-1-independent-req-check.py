#!/usr/bin/env python3
"""CT10.1 blind checker (claude): REQ-channel-descriptor v0.1 CD02-CD07 re-implemented from the REQ text only.

Inputs: the REQ and oracle/vectors/channel_descriptor_v1.json; the author oracle/builder/tests are NOT read.
CF typing and CF06 phase arithmetic are reused from claude's own P1b blind checker (p1b-independent-req-check.py).
Usage: python -I ct10-1-independent-req-check.py <corpus> [--json out]"""
from __future__ import annotations

import copy
import hashlib
import importlib.util
import json
import math
import struct
import sys
from pathlib import Path

spec = importlib.util.spec_from_file_location("p1b", Path(__file__).with_name("p1b-independent-req-check.py"))
p1b = importlib.util.module_from_spec(spec)
spec.loader.exec_module(p1b)

EXPECTED_SHA = "2f44d6a9292750ce7ebbcc7c9920ad026aee15b3ba20ac1a5be1dc0417f2c904"
HOST_ONLY = {"D17", "D18", "D19", "D20", "D21", "D31", "D47", "D54"}
CASES = 54
MODEL = "FINITE_MULTIPATH_V1"
MAGIC = b"RCFD"
STRUCT_MAX = 5111
M, Q = p1b.M, p1b.Q


class Bad(Exception):
    pass


# ---------------------------------------------------------------- CF descriptor typing (CD02, CD03)


def typed_descriptor(d):
    """CF03/CF04 descriptor only; returns a normal-form dict (binary32 values, zero weights as +0) or raises Bad."""
    try:
        p1b.exact(d, p1b.DESC_FIELDS)
        D = {"modelId": p1b.key(d["modelId"]), "profileId": p1b.key(d["profileId"]),
             "profileVersion": p1b.int32(d["profileVersion"], lo=1), "a": p1b.endpoint(d["a"]), "b": p1b.endpoint(d["b"]),
             "frequencyMHz": p1b.real32(d["frequencyMHz"]), "bandwidthHz": p1b.real32(d["bandwidthHz"]),
             "environmentRevision": p1b.int32(d["environmentRevision"], lo=1), "anchorMs": p1b.int32(d["anchorMs"], lo=0),
             "validFromMs": p1b.int32(d["validFromMs"], lo=0), "validUntilMs": p1b.int32(d["validUntilMs"], lo=0),
             "staticChannel": p1b.boolean(d["staticChannel"]), "coverageComplete": p1b.boolean(d["coverageComplete"]),
             "resourceExhausted": p1b.boolean(d["resourceExhausted"])}
        p1b.need(D["a"] < D["b"] and D["frequencyMHz"] > 0 and D["bandwidthHz"] > 0)
        p1b.need(D["anchorMs"] <= D["validFromMs"] < D["validUntilMs"])
        comps = d["components"]
        p1b.need(type(comps) is list and len(comps) <= 64)
        out = []
        for c in comps:
            p1b.exact(c, {"componentId", "powerWeight", "phaseQ", "dopplerQ"})
            w = p1b.real32(c["powerWeight"])
            p1b.need(0.0 <= w <= 1.0)
            out.append({"componentId": p1b.key(c["componentId"]), "powerWeight": 0.0 if w == 0 else w,
                        "phaseQ": p1b.int32(c["phaseQ"], lo=0, hi=M - 1), "dopplerQ": p1b.int32(c["dopplerQ"])})
        ids = [c["componentId"].encode("ascii") for c in out]
        p1b.need(all(x < y for x, y in zip(ids, ids[1:])))
        if D["staticChannel"]:
            p1b.need(all(c["dopplerQ"] == 0 for c in out))
    except p1b.Invalid:
        raise Bad
    D["a"] = {"deviceId": D["a"][0], "transceiverIndex": D["a"][1]}
    D["b"] = {"deviceId": D["b"][0], "transceiverIndex": D["b"][1]}
    D["components"] = out
    return D


# ---------------------------------------------------------------- RCFD/1 (CD07)


def key_bytes(k):
    raw = k.encode("ascii")
    return bytes([len(raw)]) + raw


def encode_bytes(D):
    body = key_bytes(D["modelId"]) + key_bytes(D["profileId"]) + struct.pack(">i", D["profileVersion"])
    body += struct.pack(">iiii", D["a"]["deviceId"], D["a"]["transceiverIndex"], D["b"]["deviceId"], D["b"]["transceiverIndex"])
    body += struct.pack(">ff", D["frequencyMHz"], D["bandwidthHz"]) + struct.pack(">i", D["environmentRevision"])
    body += struct.pack(">iii", D["anchorMs"], D["validFromMs"], D["validUntilMs"])
    flags = (1 if D["staticChannel"] else 0) | (2 if D["coverageComplete"] else 0) | (4 if D["resourceExhausted"] else 0)
    body += bytes([flags, len(D["components"])])
    for c in D["components"]:
        body += key_bytes(c["componentId"]) + struct.pack(">fii", c["powerWeight"], c["phaseQ"], c["dopplerQ"])
    total = 7 + len(body)
    return MAGIC + bytes([1]) + struct.pack(">H", total) + body


def size_formula(D):
    return 55 + len(D["modelId"]) + len(D["profileId"]) + sum(13 + len(c["componentId"]) for c in D["components"])


def limits_ok(m):
    return type(m) is int and 1 <= m <= 65535


def ok_result(D, raw):
    assert len(raw) == size_formula(D)
    return {"code": "OK", "descriptor": D, "bytes": list(raw), "sizeBytes": len(raw)}


def encode(inp):
    if not limits_ok(inp.get("maxBytes")):
        return {"code": "INVALID_LIMITS"}
    try:
        D = typed_descriptor(inp.get("descriptor"))
    except Bad:
        return {"code": "INVALID_DESCRIPTOR"}
    if D["modelId"] != MODEL:
        return {"code": "UNSUPPORTED_MODEL"}
    raw = encode_bytes(D)
    if len(raw) > inp["maxBytes"]:
        return {"code": "RESOURCE_LIMIT"}
    return ok_result(D, raw)


class Reader:
    def __init__(self, raw):
        self.raw, self.at = raw, 7

    def take(self, n):
        if self.at + n > len(self.raw):
            raise Bad
        chunk = self.raw[self.at:self.at + n]
        self.at += n
        return chunk

    def key(self):
        n = self.take(1)[0]
        if not 1 <= n <= 64:
            raise Bad
        chunk = self.take(n)
        if any(b > 127 for b in chunk):
            raise Bad
        return chunk.decode("ascii")

    def i32(self):
        return struct.unpack(">i", self.take(4))[0]

    def f32(self):
        return struct.unpack(">f", self.take(4))[0]


def decode(inp):
    if not limits_ok(inp.get("maxBytes")):
        return {"code": "INVALID_LIMITS"}
    data = inp.get("bytes")
    if type(data) is not list:
        return {"code": "INVALID_FRAME"}
    if len(data) > inp["maxBytes"]:
        return {"code": "RESOURCE_LIMIT"}
    if not 7 <= len(data) <= STRUCT_MAX or not all(type(b) is int and 0 <= b <= 255 for b in data):
        return {"code": "INVALID_FRAME"}
    raw = bytes(data)
    if raw[:4] != MAGIC or struct.unpack(">H", raw[5:7])[0] != len(raw):
        return {"code": "INVALID_FRAME"}
    if raw[4] != 1:
        return {"code": "UNSUPPORTED_VERSION"}
    try:
        r = Reader(raw)
        d = {"modelId": r.key(), "profileId": r.key(), "profileVersion": r.i32()}
        d["a"] = {"deviceId": r.i32(), "transceiverIndex": r.i32()}
        d["b"] = {"deviceId": r.i32(), "transceiverIndex": r.i32()}
        d["frequencyMHz"], d["bandwidthHz"] = r.f32(), r.f32()
        d["environmentRevision"] = r.i32()
        d["anchorMs"], d["validFromMs"], d["validUntilMs"] = r.i32(), r.i32(), r.i32()
        flags = r.take(1)[0]
        if flags & ~7:
            raise Bad
        d["staticChannel"], d["coverageComplete"], d["resourceExhausted"] = bool(flags & 1), bool(flags & 2), bool(flags & 4)
        n = r.take(1)[0]
        if n > 64:
            raise Bad
        d["components"] = []
        for _ in range(n):
            d["components"].append({"componentId": r.key(), "powerWeight": r.f32(), "phaseQ": r.i32(), "dopplerQ": r.i32()})
        if r.at != len(raw):
            raise Bad
        for c in d["components"]:
            if not math.isfinite(c["powerWeight"]):
                raise Bad
        if not (math.isfinite(d["frequencyMHz"]) and math.isfinite(d["bandwidthHz"])):
            raise Bad
        D = typed_descriptor(d)
    except Bad:
        return {"code": "INVALID_FRAME"}
    if D["modelId"] != MODEL:
        return {"code": "UNSUPPORTED_MODEL"}
    again = encode_bytes(D)
    if again != raw:
        return {"code": "INVALID_FRAME"}
    return ok_result(D, again)


def rebase(inp):
    try:
        D = typed_descriptor(inp.get("descriptor"))
    except Bad:
        return {"code": "INVALID_DESCRIPTOR"}
    if D["modelId"] != MODEL:
        return {"code": "UNSUPPORTED_MODEL"}
    if any(abs(c["dopplerQ"]) > Q for c in D["components"]):
        return {"code": "OUT_OF_MODEL_DOMAIN"}
    t = inp.get("anchorMs")
    if type(t) is not int or not p1b.INT_MIN <= t <= p1b.INT_MAX or not D["validFromMs"] <= t < D["validUntilMs"]:
        return {"code": "INVALID_ANCHOR"}
    N = copy.deepcopy(D)
    for c in N["components"]:
        c["phaseQ"] = p1b.phase_at(c["phaseQ"], c["dopplerQ"], t, D["anchorMs"], 0)
    N["anchorMs"] = N["validFromMs"] = t
    return {"code": "OK", "descriptor": N}


def evaluate(inp):
    op = inp.get("op")
    body = {k: v for k, v in inp.items() if k != "op"}
    if op == "ENCODE":
        if set(body) != {"descriptor", "maxBytes"}:
            return {"code": "INVALID_DESCRIPTOR"}
        return encode(body)
    if op == "DECODE":
        return decode(body)
    if op == "REBASE":
        return rebase(body)
    raise ValueError(op)


# ---------------------------------------------------------------- continuity (CD06) and compare


def continuity(D, N, times):
    """Old and new descriptors give identical phases and coefficient for every later admissible time."""
    def query(t, sub):
        return {"tx": dict(D["a"]), "rx": dict(D["b"]), "frequencyMHz": D["frequencyMHz"], "bandwidthHz": D["bandwidthHz"],
                "profileId": D["profileId"], "profileVersion": D["profileVersion"],
                "environmentRevision": D["environmentRevision"], "nowMs": t, "subUs": sub}
    n = 0
    for t in times:
        if not N["validFromMs"] <= t < N["validUntilMs"]:
            continue
        for sub in (0, 1, 999):
            a = p1b.evaluate({"descriptor": copy.deepcopy(D), "query": query(t, sub)})
            b = p1b.evaluate({"descriptor": copy.deepcopy(N), "query": query(t, sub)})
            assert a["code"] == b["code"] and a["phasesQ"] == b["phasesQ"] and a["real"] == b["real"] and a["imag"] == b["imag"], (t, sub)
            n += 1
    return n


def same(got, want):
    return json.dumps(got, sort_keys=True) == json.dumps(want, sort_keys=True)


def main(argv):
    raw = open(argv[1], "rb").read()
    errors, codes, cont = [], {}, 0
    sha = hashlib.sha256(raw).hexdigest()
    if sha != EXPECTED_SHA:
        errors.append(f"corpus SHA {sha}")
    doc = json.loads(raw)
    if set(doc.get("hostOnly", [])) != HOST_ONLY:
        errors.append(f"hostOnly {doc.get('hostOnly')}")
    if len(doc["cases"]) != CASES:
        errors.append("case count")
    for c in doc["cases"]:
        got = evaluate(copy.deepcopy(c["input"]))
        codes[c["id"]] = got["code"]
        if not same(got, c["expect"]):
            errors.append(f"{c['id']}: {json.dumps(got)[:160]} != {json.dumps(c['expect'])[:160]}")
        if c["input"]["op"] == "REBASE" and got["code"] == "OK":
            D = typed_descriptor(c["input"]["descriptor"])
            N = got["descriptor"]
            span = [N["validFromMs"], N["validFromMs"] + 1, N["validUntilMs"] - 1, (N["validFromMs"] + N["validUntilMs"]) // 2]
            cont += continuity(D, N, span)
        if got["code"] == "OK" and "bytes" in got:
            back = decode({"bytes": got["bytes"], "maxBytes": 65535})
            if not same(back, got):
                errors.append(f"{c['id']}: round trip")
    # Controls: corrupting one byte of every OK frame must not decode to the same descriptor.
    caught = total = 0
    for c in doc["cases"]:
        e = c["expect"]
        if e["code"] == "OK" and "bytes" in e:
            for pos in (0, 4, 6, len(e["bytes"]) - 1):
                bad = list(e["bytes"])
                bad[pos] ^= 0x01
                total += 1
                r = decode({"bytes": bad, "maxBytes": 65535})
                caught += r["code"] != "OK" or r["descriptor"] != e["descriptor"]
    big = {"modelId": "x" * 64, "profileId": "p" * 64, "components": [{"componentId": "%064d" % i} for i in range(64)]}
    sizes = {"structMax": size_formula(big), "modelMax": size_formula(dict(big, modelId=MODEL))}
    summary = {"checker": "claude ct10-1-independent-req-check (REQ v0.1 + JSON only)", "corpusSha256": sha,
               "cases": len(codes), "native": len(codes) - len(HOST_ONLY), "errors": errors,
               "continuityChecks": cont, "byteFlipControls": f"{caught}/{total}", "sizes": sizes,
               "codes": sorted(set(codes.values()))}
    print(json.dumps(summary, ensure_ascii=False, indent=1))
    if "--json" in argv:
        with open(argv[argv.index("--json") + 1], "w", encoding="utf-8", newline="\n") as fh:
            fh.write(json.dumps(dict(summary, perCase=codes), ensure_ascii=False, indent=1) + "\n")
    ok = not errors and caught == total and sizes == {"structMax": 5111, "modelMax": 5066}
    print("CT10_1_BLIND_CHECK_" + ("PASS" if ok else "FAIL"))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    raise SystemExit(main(sys.argv))
