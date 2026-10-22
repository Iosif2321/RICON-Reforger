"""claude: blind recomputation of the S3b corpus from REQ-link-publication v0.2 only (no oracle source read).
Usage: python -I check_s3b.py <link_publication_v1.json>"""
import copy
import json
import math
import struct
import sys

MAX = 2147483647
CALL = ["NULL_INPUT", "INVALID_LIMITS", "LIMIT_EXCEEDED", "INVALID_ELEMENT", "INVALID_REGISTRY", "DUPLICATE",
        "UPSTREAM_REFUSED", "IDENTITY_MISMATCH", "CLOCK_ROLLBACK", "ACTIVE_LIMIT", "ID_EXHAUSTED", "NEED_FULL_RESYNC", "OK"]
S3_CALL = ["NULL_INPUT", "INVALID_LIMITS", "LIMIT_EXCEEDED", "INVALID_ELEMENT", "DUPLICATE", "OK"]
PAIR = {"BINDING_INACTIVE": 1, "TX_PLAYER_INVALID": 2, "TX_DEVICE_INVALID": 3, "SELF_PAIR": 4, "RX_PLAYER_INVALID": 5,
        "RX_DEVICE_INVALID": 6, "STATE_MISMATCH": 7, "DEVICE_QUERY": 8, "EVALUATED": 9}
DEVICE = ["NULL_INPUT", "INVALID_OBSERVATION", "INVALID_PROFILE", "INVALID_CONTEXT", "PROFILE_MISMATCH", "POWER_OFF",
          "TUNING_MISMATCH", "CHANNEL_MISMATCH", "KEY_MISMATCH", "READY"]
REASON = ["OK", "INVALID_COORDINATE", "INVALID_FREQUENCY", "INVALID_BANDWIDTH", "INVALID_GAIN", "INVALID_NOISE_FIGURE",
          "INVALID_SENSITIVITY", "INVALID_REQUIRED_SINR", "INVALID_INTERFERENCE", "CATALOG_VIOLATION", "UNKNOWN_BAND",
          "UNKNOWN_MODEL", "UNKNOWN_NOISE_ENVIRONMENT", "OUT_OF_MODEL_DOMAIN", "NUMERIC_FAILURE",
          "MODEL_CONTRACT_VIOLATION", "TERRAIN_PROFILE_UNAVAILABLE"]
STATE = ["LOST", "WEAK", "DEGRADED", "CLEAN"]


class Fail(Exception):
    def __init__(self, code):
        self.code = code


def is_int(v, lo, hi):
    return type(v) is int and lo <= v <= hi


def f32(x):
    try:
        return struct.unpack("<f", struct.pack("<f", x))[0]
    except OverflowError:
        return math.copysign(math.inf, x)


def num(v):
    """JSON number -> float32 (REQ-LP §1); special values are strings; other types are not numbers."""
    if type(v) in (int, float):
        return f32(float(v))
    if v in ("NaN", "Infinity", "-Infinity"):
        return float(v)
    return None


def finite(v):
    x = num(v)
    return x is not None and math.isfinite(x)


def key_of(o):
    return (o["leaseId"], o["rxDeviceId"], o["rxTransceiverIndex"], o["rxPlayerId"])


class Source:
    def __init__(self, spec):
        self.epoch = spec["epoch"]
        self.seq = {int(k): v for k, v in spec.get("sequences", {}).items()}
        self.failAt = spec.get("failAt", 0)
        self.rotateAt = spec.get("rotateAt", 0)
        self.invalidAt = spec.get("invalidAt", 0)
        self.invalidSeq = spec.get("invalidSeq", MAX)
        self.rotatedEpoch = spec.get("rotatedEpoch")
        self.calls = 0
        self.reads = 0

    def get_epoch(self):
        self.reads += 1
        return self.epoch

    def next(self, link_id):
        self.calls += 1
        if self.calls == self.failAt:
            return False, 0, False
        cur = self.seq.get(link_id, 0)
        rotated = False
        if self.calls == self.rotateAt or cur >= MAX - 1:
            self.seq.clear()
            if self.rotatedEpoch is not None:
                self.epoch = self.rotatedEpoch
            else:
                self.epoch = self.epoch + 1 if self.epoch < MAX - 1 else 1
            cur = 0
            rotated = True
        s = cur + 1
        if self.calls == self.invalidAt:
            s = self.invalidSeq
        self.seq[link_id] = s
        return True, s, rotated

    def state(self):
        return {"epoch": self.epoch, "sequences": {str(k): v for k, v in sorted(self.seq.items())}}


def check_binding(b):
    if b is None:
        raise Fail("INVALID_ELEMENT")
    ok = (is_int(b.get("leaseId"), 1, MAX) and is_int(b.get("sessionEpoch"), 1, MAX)
          and is_int(b.get("txPlayerId"), 1, MAX) and is_int(b.get("txGeneration"), 1, MAX)
          and is_int(b.get("txDeviceId"), 1, MAX - 1) and is_int(b.get("txTransceiverIndex"), 0, MAX - 1)
          and is_int(b.get("networkTypeId"), 1, MAX) and is_int(b.get("channelId"), 1, MAX)
          and type(b.get("active")) is bool and is_int(b.get("issuedAtMs"), 0, MAX) and is_int(b.get("ttlMs"), 1, MAX))
    if not ok:
        raise Fail("INVALID_ELEMENT")


def check_pair(p):
    if p is None:
        raise Fail("INVALID_ELEMENT")
    ok = (is_int(p.get("leaseId"), 1, MAX) and is_int(p.get("txDeviceId"), 1, MAX - 1)
          and is_int(p.get("rxPlayerId"), 1, MAX) and is_int(p.get("rxDeviceId"), 1, MAX - 1)
          and is_int(p.get("rxTransceiverIndex"), 0, MAX - 1)
          and p.get("pairCode") in PAIR and type(p.get("hasDeviceCode")) is bool and type(p.get("hasBudget")) is bool
          and type(p.get("valid")) is bool and p.get("deviceCode") in DEVICE and p.get("reason") in REASON
          and p.get("state") in STATE and finite(p.get("sinrDb")) and finite(p.get("intelligibility")))
    if not ok:
        raise Fail("INVALID_ELEMENT")
    q = num(p["intelligibility"])
    sinr = num(p["sinrDb"])
    code = p["pairCode"]
    if not (0.0 <= q <= 1.0):
        raise Fail("INVALID_ELEMENT")
    if p["hasBudget"] != (code == "EVALUATED"):
        raise Fail("INVALID_ELEMENT")
    if p["hasDeviceCode"] != (code in ("DEVICE_QUERY", "EVALUATED")):
        raise Fail("INVALID_ELEMENT")
    dev = p["deviceCode"]
    if code == "EVALUATED" and dev != "READY":
        raise Fail("INVALID_ELEMENT")
    if code == "DEVICE_QUERY" and dev == "READY":
        raise Fail("INVALID_ELEMENT")
    if code not in ("EVALUATED", "DEVICE_QUERY") and dev != "NULL_INPUT":
        raise Fail("INVALID_ELEMENT")
    if not p["hasBudget"]:
        if not (p["valid"] is False and p["reason"] == "NUMERIC_FAILURE" and p["state"] == "LOST" and q == 0 and sinr == 0):
            raise Fail("INVALID_ELEMENT")
    else:
        if p["valid"] != (p["reason"] == "OK"):
            raise Fail("INVALID_ELEMENT")
        if not p["valid"] and not (p["state"] == "LOST" and q == 0 and sinr == 0):
            raise Fail("INVALID_ELEMENT")
    if p["state"] == "LOST" and q != 0:
        raise Fail("INVALID_ELEMENT")


def check_registry(r):
    if not (is_int(r.get("sessionEpoch"), 1, MAX - 1) and is_int(r.get("highWaterLinkId"), 0, MAX)
            and type(r.get("lastTickMs")) is int and (r["lastTickMs"] == -1 or is_int(r["lastTickMs"], 0, MAX))
            and type(r.get("needsFullResync")) is bool):
        raise Fail("INVALID_REGISTRY")
    hw, lt = r["highWaterLinkId"], r["lastTickMs"]
    if lt == -1 and (r["entries"] or not r["needsFullResync"]):
        raise Fail("INVALID_REGISTRY")
    for e in r["entries"]:
        if e is None:
            raise Fail("INVALID_REGISTRY")
        ok = (is_int(e.get("leaseId"), 1, MAX) and is_int(e.get("rxDeviceId"), 1, MAX - 1)
              and is_int(e.get("rxTransceiverIndex"), 0, MAX - 1) and is_int(e.get("rxPlayerId"), 1, MAX)
              and is_int(e.get("txPlayerId"), 1, MAX) and is_int(e.get("txDeviceId"), 1, MAX - 1)
              and is_int(e.get("txTransceiverIndex"), 0, MAX - 1) and is_int(e.get("linkId"), 1, hw)
              and is_int(e.get("linkSeq"), 1, MAX - 1) and e.get("state") in STATE and finite(e.get("quality"))
              and is_int(e.get("ttlMs"), 1, MAX) and type(e.get("publishedAtMs")) is int
              and 0 <= e["publishedAtMs"] <= lt)
        if not ok:
            raise Fail("INVALID_REGISTRY")
        q = num(e["quality"])
        if not (0.0 <= q <= 1.0) or (e["state"] == "LOST" and q != 0):
            raise Fail("INVALID_REGISTRY")


def preflight(inp, reg, src):
    if inp is None or reg is None or src is None:
        raise Fail("NULL_INPUT")
    lim, binds, pr = inp.get("limits"), inp.get("bindings"), inp.get("pairResult")
    if lim is None or binds is None or pr is None or pr.get("pairs") is None or reg.get("entries") is None:
        raise Fail("NULL_INPUT")
    pairs, entries = pr["pairs"], reg["entries"]
    if not (is_int(lim.get("maxBindings"), 1, 4096) and is_int(lim.get("maxPairs"), 1, 65536)
            and is_int(lim.get("maxActiveLinks"), 1, 65536)):
        raise Fail("INVALID_LIMITS")
    if len(binds) > lim["maxBindings"] or len(pairs) > lim["maxPairs"] or len(entries) > lim["maxActiveLinks"]:
        raise Fail("LIMIT_EXCEEDED")
    qd = num(inp.get("qualityDelta"))
    if not (is_int(inp.get("nowMs"), 0, MAX) and is_int(inp.get("sessionEpoch"), 1, MAX - 1)
            and is_int(inp.get("ttlMs"), 1, MAX) and qd is not None and math.isfinite(qd) and 0 < qd <= 1):
        raise Fail("INVALID_ELEMENT")
    if pr.get("callCode") not in S3_CALL or not is_int(pr.get("s2Calls"), 0, MAX) \
            or not is_int(pr.get("evaluatorCalls"), 0, MAX):
        raise Fail("INVALID_ELEMENT")
    for b in binds:
        check_binding(b)
    for p in pairs:
        check_pair(p)
    check_registry(reg)
    if len({b["leaseId"] for b in binds}) != len(binds) or len({b["txDeviceId"] for b in binds}) != len(binds):
        raise Fail("DUPLICATE")
    if len({key_of(p) for p in pairs}) != len(pairs):
        raise Fail("DUPLICATE")
    if len({key_of(e) for e in entries}) != len(entries) or len({e["linkId"] for e in entries}) != len(entries):
        raise Fail("DUPLICATE")
    if pr["callCode"] != "OK":
        raise Fail("UPSTREAM_REFUSED")
    epoch = inp["sessionEpoch"]
    same = reg["sessionEpoch"] == epoch
    bind = {b["leaseId"]: b for b in binds}
    old = {key_of(e): e for e in entries} if same else {}
    for p in pairs:
        b = bind.get(p["leaseId"])
        if b is None or b["txDeviceId"] != p["txDeviceId"]:
            raise Fail("IDENTITY_MISMATCH")
        if p["pairCode"] != "BINDING_INACTIVE" and b["sessionEpoch"] != epoch:
            raise Fail("IDENTITY_MISMATCH")
        e = old.get(key_of(p))
        if e is not None and (e["txPlayerId"] != b["txPlayerId"] or e["txDeviceId"] != b["txDeviceId"]
                              or e["txTransceiverIndex"] != b["txTransceiverIndex"]):
            raise Fail("IDENTITY_MISMATCH")
    if same and reg["lastTickMs"] >= 0 and inp["nowMs"] < reg["lastTickMs"]:
        raise Fail("CLOCK_ROLLBACK")
    plan = select(inp, reg, old, bind)
    kept = sum(1 for x in plan if x["kind"] in ("keep", "new"))
    if kept > lim["maxActiveLinks"]:
        raise Fail("ACTIVE_LIMIT")
    hw = reg["highWaterLinkId"] if same else 0
    if sum(1 for x in plan if x["kind"] == "new") > MAX - hw:
        raise Fail("ID_EXHAUSTED")
    return plan, qd


def select(inp, reg, old, bind):
    pairs = {key_of(p): p for p in inp["pairResult"]["pairs"]}
    plan = []
    for k in sorted(set(pairs) | set(old)):
        p, e = pairs.get(k), old.get(k)
        code = p["pairCode"] if p else None
        if code == "EVALUATED" and p["valid"]:
            target = (p["state"], 0.0 if p["state"] == "LOST" else num(p["intelligibility"]))
            plan.append({"kind": "keep" if e else "new", "key": k, "pair": p, "old": e, "target": target})
        elif code in ("EVALUATED", "STATE_MISMATCH", "DEVICE_QUERY"):
            if e:
                plan.append({"kind": "keep", "key": k, "pair": p, "old": e, "target": ("LOST", 0.0)})
        else:
            if e:
                plan.append({"kind": "term", "key": k, "pair": p, "old": e, "target": ("LOST", 0.0)})
    return plan


def publish(case):
    inp, reg, srcspec = case["input"], case["registry"], case["source"]
    src = Source(srcspec) if srcspec is not None else None
    try:
        plan, qd = preflight(inp, reg, srcspec)
    except Fail as f:
        return {"callCode": f.code, "snapshots": [], "registry": copy.deepcopy(reg), "sequenceCalls": 0,
                "epochReads": 0, "requiresFullResync": False, "sourceState": src.state() if src else None}
    epoch = inp["sessionEpoch"]

    def reset(new_epoch, hw_if_same):
        e = new_epoch if is_int(new_epoch, 1, MAX - 1) else epoch
        hw = hw_if_same if e == reg["sessionEpoch"] else 0
        return {"callCode": "NEED_FULL_RESYNC", "snapshots": [],
                "registry": {"sessionEpoch": e, "highWaterLinkId": hw, "lastTickMs": -1, "needsFullResync": True,
                             "entries": []},
                "sequenceCalls": src.calls, "epochReads": src.reads, "requiresFullResync": True,
                "sourceState": src.state()}

    e0 = src.get_epoch()
    if e0 != epoch or reg["sessionEpoch"] != epoch:
        return reset(e0, reg["highWaterLinkId"])
    hw = reg["highWaterLinkId"]
    for x in plan:
        x["linkId"] = x["old"]["linkId"] if x["old"] else None
        if x["kind"] == "new":
            hw += 1
            x["linkId"] = hw
    full = reg["needsFullResync"]
    now, ttl = inp["nowMs"], inp["ttlMs"]
    snaps, entries = [], []
    for x in plan:
        e = x["old"]
        if x["kind"] == "term" or x["kind"] == "new" or full:
            need = True
        else:
            st, q = x["target"]
            diff = f32(q - num(e["quality"]))
            half = e["ttlMs"] // 2 + e["ttlMs"] % 2
            need = (st != e["state"] or abs(diff) >= qd or ttl != e["ttlMs"] or now - e["publishedAtMs"] >= max(1, half))
        x["need"] = need
        if not need:
            continue
        ok, seq, rot = src.next(x["linkId"])
        er = src.get_epoch()
        if not ok or rot or not is_int(seq, 1, MAX - 1) or (e and seq <= e["linkSeq"]) or er != epoch:
            return reset(er, hw)
        x["seq"] = seq
    for x in plan:
        b = None
        if x["pair"] is not None:
            b = [bb for bb in inp["bindings"] if bb["leaseId"] == x["pair"]["leaseId"]][0]
        talker = b["txPlayerId"] if b else x["old"]["txPlayerId"]
        st, q = x["target"]
        k = x["key"]
        if x["need"]:
            snaps.append({"schemaVersion": 1, "sessionEpoch": epoch, "linkId": x["linkId"], "linkSeq": x["seq"],
                          "ttlMs": ttl, "routeId": x["linkId"], "routeNodeCount": 2, "talkerPlayerId": talker,
                          "listenerPlayerId": k[3], "state": st, "quality": q})
        if x["kind"] == "term":
            continue
        if x["need"]:
            ent = {"leaseId": k[0], "rxDeviceId": k[1], "rxTransceiverIndex": k[2], "rxPlayerId": k[3],
                   "txPlayerId": b["txPlayerId"], "txDeviceId": b["txDeviceId"],
                   "txTransceiverIndex": b["txTransceiverIndex"], "state": st, "quality": q,
                   "linkId": x["linkId"], "linkSeq": x["seq"], "ttlMs": ttl, "publishedAtMs": now}
        else:
            ent = copy.deepcopy(x["old"])
        entries.append(ent)
    return {"callCode": "OK", "snapshots": snaps,
            "registry": {"sessionEpoch": epoch, "highWaterLinkId": hw, "lastTickMs": now, "needsFullResync": False,
                         "entries": entries},
            "sequenceCalls": src.calls, "epochReads": src.reads, "requiresFullResync": full,
            "sourceState": src.state()}


def same(a, b, path=""):
    """Exact compare; quality fields within 1e-6 (float32 vs JSON decimal)."""
    if isinstance(a, dict) and isinstance(b, dict):
        if set(a) != set(b):
            return f"{path}: keys {sorted(set(a) ^ set(b))}"
        for k in a:
            r = same(a[k], b[k], f"{path}.{k}")
            if r:
                return r
        return None
    if isinstance(a, list) and isinstance(b, list):
        if len(a) != len(b):
            return f"{path}: len {len(a)} != {len(b)}"
        for i, (x, y) in enumerate(zip(a, b)):
            r = same(x, y, f"{path}[{i}]")
            if r:
                return r
        return None
    if path.endswith(".quality") and type(a) in (int, float) and type(b) in (int, float):
        return None if abs(a - b) <= 1e-6 else f"{path}: {a!r} != {b!r}"
    if type(a) is bool or type(b) is bool:
        return None if a is b else f"{path}: {a!r} != {b!r}"
    return None if a == b else f"{path}: {a!r} != {b!r}"


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    d = json.load(open(sys.argv[1], encoding="utf-8"))
    bad = 0
    for c in d["cases"]:
        got = publish(c)
        exp = c["expect"]
        r = same(got, exp, c["id"])
        if r:
            bad += 1
            print("DIFF", r)
    n = len(d["cases"])
    print(f"S3B_BLIND_CHECK {'PASS' if bad == 0 else 'FAIL'}: {n - bad}/{n}")
    sys.exit(1 if bad else 0)


main()
