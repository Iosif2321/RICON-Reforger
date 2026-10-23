#!/usr/bin/env python3
"""P1b blind checker (claude): REQ-channel-fading v0.1 CF03-CF09 re-implemented from the REQ text only.

Inputs: the REQ and oracle/vectors/channel_fading_v1.json. The author oracle, builder and tests are NOT read.
binary32 arithmetic via struct (every +,*,/,sqrt rounded separately; double-then-float rounding is innocuous
for these operations because 53 >= 2*24+2). Usage: python p1b-independent-req-check.py <corpus> [--json out]"""
from __future__ import annotations

import copy
import hashlib
import json
import math
import re
import struct
import sys

M = 1 << 30
Q = M // 4
INT_MIN, INT_MAX = -(1 << 31), (1 << 31) - 1
F32_MAX = struct.unpack("<f", b"\xff\xff\x7f\x7f")[0]
F32_OVERFLOW = 2.0 ** 128 - 2.0 ** 103  # halfway between FLT_MAX and 2^128: ties-to-even goes to Inf
KEY = re.compile(r"[A-Za-z0-9_.:-]{1,64}")
EXPECTED_SHA = "365e94e9c9a2386caae0a8b7e171471d84e615fabf4c14882347fb660ca46321"
HOST_ONLY = {"F27", "F28", "F29", "F30"}
CODES = ("INVALID_INPUT", "UNSUPPORTED_MODEL", "RESOURCE_EXHAUSTED", "INCOMPLETE_COVERAGE", "CONTEXT_MISMATCH",
         "STALE_DESCRIPTOR", "OUT_OF_MODEL_DOMAIN", "EMPTY_MEDIUM", "NUMERIC_FAILURE", "ZERO_AT_SAMPLE", "OK")
DESC_FIELDS = {"modelId", "profileId", "profileVersion", "a", "b", "frequencyMHz", "bandwidthHz",
               "environmentRevision", "anchorMs", "validFromMs", "validUntilMs", "staticChannel",
               "coverageComplete", "resourceExhausted", "components"}
QUERY_FIELDS = {"tx", "rx", "frequencyMHz", "bandwidthHz", "profileId", "profileVersion", "environmentRevision",
                "nowMs", "subUs"}


class Invalid(Exception):
    pass


def f32(x: float) -> float:
    if math.isnan(x):
        return x
    if abs(x) >= F32_OVERFLOW:
        return math.copysign(math.inf, x)
    return struct.unpack("<f", struct.pack("<f", x))[0]


def add(a, b): return f32(a + b)
def mul(a, b): return f32(a * b)
def div(a, b): return f32(a / b)
def sqrt(a): return f32(math.sqrt(a))


# ---------------------------------------------------------------- CF03-CF05 typing


def need(cond):
    if not cond:
        raise Invalid


def int32(v, lo=INT_MIN, hi=INT_MAX):
    need(type(v) is int and INT_MIN <= v <= INT_MAX and lo <= v <= hi)
    return v


def real32(v):
    need(type(v) in (int, float))
    t = f32(float(v))
    need(math.isfinite(t))
    return t


def key(v):
    need(type(v) is str and KEY.fullmatch(v) is not None)
    return v


def boolean(v):
    need(type(v) is bool)
    return v


def exact(obj, fields):
    need(type(obj) is dict and set(obj) == fields)


def endpoint(v):
    exact(v, {"deviceId", "transceiverIndex"})
    return int32(v["deviceId"], lo=1), int32(v["transceiverIndex"], lo=0)


def typed(inp):
    exact(inp, {"descriptor", "query"})
    d, q = inp["descriptor"], inp["query"]
    exact(d, DESC_FIELDS)
    exact(q, QUERY_FIELDS)
    D = {"modelId": key(d["modelId"]), "profileId": key(d["profileId"]),
         "profileVersion": int32(d["profileVersion"], lo=1), "a": endpoint(d["a"]), "b": endpoint(d["b"]),
         "frequencyMHz": real32(d["frequencyMHz"]), "bandwidthHz": real32(d["bandwidthHz"]),
         "environmentRevision": int32(d["environmentRevision"], lo=1), "anchorMs": int32(d["anchorMs"], lo=0),
         "validFromMs": int32(d["validFromMs"], lo=0), "validUntilMs": int32(d["validUntilMs"], lo=0),
         "staticChannel": boolean(d["staticChannel"]), "coverageComplete": boolean(d["coverageComplete"]),
         "resourceExhausted": boolean(d["resourceExhausted"])}
    need(D["a"] < D["b"])
    need(D["frequencyMHz"] > 0 and D["bandwidthHz"] > 0)
    need(D["anchorMs"] <= D["validFromMs"] < D["validUntilMs"])
    comps = d["components"]
    need(type(comps) is list and len(comps) <= 64)
    out = []
    for c in comps:
        exact(c, {"componentId", "powerWeight", "phaseQ", "dopplerQ"})
        w = real32(c["powerWeight"])
        need(0.0 <= w <= 1.0)
        out.append((key(c["componentId"]), w, int32(c["phaseQ"], lo=0, hi=M - 1), int32(c["dopplerQ"])))
    ids = [c[0].encode("ascii") for c in out]
    need(all(x < y for x, y in zip(ids, ids[1:])))
    if D["staticChannel"]:
        need(all(c[3] == 0 for c in out))
    D["components"] = out
    Qy = {"tx": endpoint(q["tx"]), "rx": endpoint(q["rx"]), "frequencyMHz": real32(q["frequencyMHz"]),
          "bandwidthHz": real32(q["bandwidthHz"]), "profileId": key(q["profileId"]),
          "profileVersion": int32(q["profileVersion"], lo=1), "environmentRevision": int32(q["environmentRevision"], lo=1),
          "nowMs": int32(q["nowMs"], lo=0), "subUs": int32(q["subUs"], lo=0, hi=999)}
    need(Qy["tx"] != Qy["rx"])
    need(Qy["frequencyMHz"] > 0 and Qy["bandwidthHz"] > 0)
    return D, Qy


# ---------------------------------------------------------------- CF06 phase


def mulmod_split(x, y):
    """CF06 two 15-bit halves; asserts every intermediate stays inside int32."""
    h, mask = 1 << 15, (1 << 15) - 1
    lo = (x & mask) * (y & mask)
    p1, p2 = (x & mask) * (y >> 15), (x >> 15) * (y & mask)
    for v in (lo, p1, p2, p1 + p2):
        assert v <= INT_MAX, v
    cross = (p1 + p2) & mask
    s = lo + (cross << 15)
    assert s <= INT_MAX, s
    assert h == 1 << 15
    return s & (M - 1)


def phase_at(phase0, doppler, now, anchor, sub):
    dms = now - anchor
    tq = (((dms % M) * 1000 % M) + sub) % M
    exact_phase = (phase0 + doppler * tq) % M
    # Enforce path: positive MulMod, then sign.
    p = mulmod_split(abs(doppler) % M, tq)
    native = (phase0 + p) % M if doppler >= 0 else (phase0 - p) % M
    assert native == exact_phase, (phase0, doppler, tq)
    return exact_phase


# ---------------------------------------------------------------- CF07 complex sum

C2PI = f32(2.0 * math.pi)
SIN_C = [f32(c) for c in (-1 / 6, 1 / 120, -1 / 5040, 1 / 362880, -1 / 39916800)]
COS_C = [f32(c) for c in (-1 / 2, 1 / 24, -1 / 720, 1 / 40320, -1 / 3628800, 1 / 479001600)]


def horner(z, coeffs):
    t = coeffs[-1]
    for c in reversed(coeffs[:-1]):
        t = add(c, mul(z, t))
    return add(1.0, mul(z, t))


def cos_sin(qp):
    k = (qp + M // 8) // Q
    r = qp - k * Q
    if r == 0:
        c, s = 1.0, 0.0
    else:
        x = mul(f32(r / M), C2PI)
        z = mul(x, x)
        s = mul(x, horner(z, SIN_C))
        c = horner(z, COS_C)
    return ((c, s), (-s, c), (-c, -s), (s, -c))[k % 4]


def null(code):
    return {"code": code, "real": None, "imag": None, "gainDb": None, "phasesQ": []}


def evaluate(inp):
    try:
        D, Qy = typed(inp)
    except Invalid:
        return null("INVALID_INPUT")
    if D["modelId"] != "FINITE_MULTIPATH_V1":
        return null("UNSUPPORTED_MODEL")
    if D["resourceExhausted"]:
        return null("RESOURCE_EXHAUSTED")
    if not D["coverageComplete"]:
        return null("INCOMPLETE_COVERAGE")
    pair = tuple(sorted((Qy["tx"], Qy["rx"])))
    if (pair != (D["a"], D["b"]) or Qy["frequencyMHz"] != D["frequencyMHz"] or Qy["bandwidthHz"] != D["bandwidthHz"]
            or Qy["profileId"] != D["profileId"] or Qy["profileVersion"] != D["profileVersion"]
            or Qy["environmentRevision"] != D["environmentRevision"]):
        return null("CONTEXT_MISMATCH")
    if not D["validFromMs"] <= Qy["nowMs"] < D["validUntilMs"]:
        return null("STALE_DESCRIPTOR")
    comps = D["components"]
    if any(abs(c[3]) > Q for c in comps):
        return null("OUT_OF_MODEL_DOMAIN")
    w = 0.0
    for c in comps:
        w = add(w, c[1])
    if not comps or w == 0.0:
        return {"code": "EMPTY_MEDIUM", "real": 0.0, "imag": 0.0, "gainDb": None, "phasesQ": []}
    phases = [phase_at(c[2], c[3], Qy["nowMs"], D["anchorMs"], Qy["subUs"]) for c in comps]
    sw = sqrt(w)
    re_, im_ = 0.0, 0.0
    vals = [w, sw]
    for (cid, pw, _p, _d), qp in zip(comps, phases):
        a = div(sqrt(pw), sw)
        c, s = cos_sin(qp)
        re_ = add(re_, mul(a, c))
        im_ = add(im_, mul(a, s))
        vals += [a, c, s, re_, im_]
    if not all(math.isfinite(v) and abs(v) <= F32_MAX for v in vals):
        return null("NUMERIC_FAILURE")
    if re_ == 0.0 and im_ == 0.0:
        return {"code": "ZERO_AT_SAMPLE", "real": 0.0, "imag": 0.0, "gainDb": None, "phasesQ": phases}
    mag = f32(math.hypot(re_, im_))  # CF07 leaves the scaled-hypot algorithm open; gainDb carries 0.001 dB
    return {"code": "OK", "real": re_, "imag": im_, "gainDb": 20.0 * math.log10(mag), "phasesQ": phases}


# ---------------------------------------------------------------- compare


def compare(cid, got, want, errors):
    if set(want) != {"code", "real", "imag", "gainDb", "phasesQ"}:
        errors.append(f"{cid}: expect fields {sorted(want)}")
    for k in ("code", "phasesQ"):
        if got[k] != want.get(k):
            errors.append(f"{cid}: {k} {got[k]} != {want.get(k)}")
    for k in ("real", "imag"):
        g, w = got[k], want.get(k)
        if (g is None) != (w is None) or (g is not None and (f32(w) != w or g != w)):
            errors.append(f"{cid}: {k} {g!r} != {w!r}")
    g, w = got["gainDb"], want.get("gainDb")
    if (g is None) != (w is None) or (g is not None and abs(g - w) > 1e-3):
        errors.append(f"{cid}: gainDb {g!r} != {w!r}")


def run(raw: bytes):
    errors, report = [], {}
    sha = hashlib.sha256(raw).hexdigest()
    if sha != EXPECTED_SHA:
        errors.append(f"corpus SHA {sha}")
    doc = json.loads(raw)
    cases = doc["cases"]
    if set(doc.get("hostOnly", [])) != HOST_ONLY:
        errors.append(f"hostOnly {doc.get('hostOnly')}")
    ids = [c["id"] for c in cases]
    if len(ids) != 50 or len(set(ids)) != 50:
        errors.append("case ids")
    for c in cases:
        got = evaluate(copy.deepcopy(c["input"]))
        compare(c["id"], got, c["expect"], errors)
        report[c["id"]] = got["code"]
    return sha, cases, report, errors


def controls(cases):
    """Negative controls: each corruption of an input or expectation must be caught."""
    by = {c["id"]: c for c in cases}
    muts = [
        ("F05", "expect", lambda e: e["phasesQ"].__setitem__(1, e["phasesQ"][1] + 1)),
        ("F05", "expect", lambda e: e.update(real=f32(e["real"] + 6e-8))),
        ("F03", "expect", lambda e: e.update(gainDb=e["gainDb"] + 2e-3)),
        ("F04", "expect", lambda e: e.update(code="OK")),
        ("F40", "input", lambda i: i["descriptor"]["components"][2].update(phaseQ=345678902)),
        ("F40", "input", lambda i: i["descriptor"]["components"][3].update(powerWeight=0.5)),
        ("F41", "input", lambda i: i["query"].update(nowMs=381)),
        ("F41", "input", lambda i: i["query"].update(subUs=1)),
        ("F01", "input", lambda i: i["descriptor"].update(profileVersion=2, modelId="FINITE_MULTIPATH_V1")),
        ("F01", "input", lambda i: i["query"].update(environmentRevision=3)),
        ("F40", "input", lambda i: i["descriptor"]["components"].pop()),
        ("F09", "input", lambda i: i["descriptor"]["components"][0].update(dopplerQ=-268435452)),
    ]
    caught = 0
    for cid, part, fn in muts:
        c = copy.deepcopy(by[cid])
        fn(c[part])
        e = []
        compare(cid, evaluate(c["input"]), c["expect"], e)
        caught += bool(e)
    return caught, len(muts)


def extras():
    """Checks beyond the corpus: MulMod at its extremes, polynomial error, quarter symmetry, reciprocity."""
    out = {}
    worst = 0
    for x in (0, 1, Q, Q - 1, M - 1, (1 << 15) - 1, 1 << 15, M - (1 << 15)):
        for y in (0, 1, M - 1, M - 2, (1 << 15) - 1, 123456789, M // 2):
            assert mulmod_split(x, y) == x * y % M
            worst += 1
    out["mulmodPairs"] = worst
    err = 0.0
    step = 4093
    for qp in range(0, M, step * 61):
        c, s = cos_sin(qp)
        ang = 2 * math.pi * qp / M
        err = max(err, abs(c - math.cos(ang)), abs(s - math.sin(ang)))
    for qp in list(range(0, 64)) + [Q - 1, Q, Q + 1, M // 2 - 1, M // 2 + 1, 3 * Q, M - 1, M // 8, M // 8 - 1, 7 * M // 8]:
        c, s = cos_sin(qp)
        ang = 2 * math.pi * qp / M
        err = max(err, abs(c - math.cos(ang)), abs(s - math.sin(ang)))
    out["polyMaxAbsErr"] = err
    sym = all(cos_sin((qp + M // 2) % M) == tuple(-v for v in cos_sin(qp)) for qp in range(1, M, 7919 * 131))
    out["halfTurnAntisymmetric"] = sym
    return out


def main(argv):
    path = argv[1]
    raw = open(path, "rb").read()
    sha, cases, report, errors = run(raw)
    caught, total = controls(cases)
    ex = extras()
    covered = sorted({c for c in report.values()})
    missing = [c for c in CODES if c not in covered]
    native = [cid for cid in report if cid not in HOST_ONLY]
    summary = {"checker": "claude p1b-independent-req-check (REQ v0.1 + JSON only)", "corpusSha256": sha,
               "cases": len(report), "native": len(native), "hostOnly": sorted(HOST_ONLY), "errors": errors,
               "controlsCaught": f"{caught}/{total}", "codesCovered": covered, "codesNotInCorpus": missing,
               "extras": ex, "codes": report}
    print(json.dumps({k: v for k, v in summary.items() if k != "codes"}, ensure_ascii=False, indent=1))
    if "--json" in argv:
        with open(argv[argv.index("--json") + 1], "w", encoding="utf-8", newline="\n") as fh:
            fh.write(json.dumps(summary, ensure_ascii=False, indent=1) + "\n")
    ok = not errors and caught == total
    print("P1B_BLIND_CHECK_" + ("PASS" if ok else "FAIL"))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    raise SystemExit(main(sys.argv))
