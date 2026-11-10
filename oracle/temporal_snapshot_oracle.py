#!/usr/bin/env python3
"""CT10 schema2 HOST candidate; not Enforce migration, transport or voice acceptance."""
from __future__ import annotations

import copy

import channel_descriptor_oracle as cd
import channel_fading_oracle as cf
import snapshot_oracle as legacy

MAX = legacy.INT_MAX
DEFAULT_LIMITS = dict(maxSnapshots=65536, maxDescriptorBytes=cd.MAX_BYTES,
                      maxTotalDescriptorBytes=MAX)
SNAPSHOT_FIELDS = ("schemaVersion sessionEpoch linkId linkSeq ttlMs routeId routeNodeCount "
                   "talkerPlayerId listenerPlayerId state quality mode timebaseId binding "
                   "descriptorVersion descriptor")
BINDING_FIELDS = ("leaseId txGeneration rxGeneration txDeviceId txTransceiverIndex "
                  "rxDeviceId rxTransceiverIndex networkTypeId channelId")
EXPECTED_FIELDS = ("sessionEpoch linkId talkerPlayerId listenerPlayerId mode timebaseId binding channel")
MAPPING_FIELDS = ("mappingId timebaseId localAnchorMs serverAnchorMs validFromLocalMs "
                  "validUntilLocalMs maxErrorUs")


def exact(value, fields):
    return type(value) is dict and set(value) == set(fields.split())


def integer(value, minimum=0, maximum=MAX):
    return type(value) is int and minimum <= value <= maximum


def limits_valid(value):
    return (exact(value, "maxSnapshots maxDescriptorBytes maxTotalDescriptorBytes")
            and integer(value["maxSnapshots"], 1, 65536)
            and integer(value["maxDescriptorBytes"], 1, 65535)
            and integer(value["maxTotalDescriptorBytes"], 1))


def binding_valid(value):
    return (exact(value, BINDING_FIELDS)
            and all(integer(value[k], 0 if k.endswith("Index") else 1) for k in value)
            and value["txDeviceId"] != value["rxDeviceId"])


def typed(snapshot, limits):
    """Shape/version/budget only. Physical expiry and clock qualification are view gates."""
    if not limits_valid(limits):
        return dict(code="INVALID_LIMITS")
    if type(snapshot) is not dict:
        return dict(code="INVALID_SNAPSHOT")
    if "descriptor" not in snapshot:
        return dict(code="INVALID_DESCRIPTOR")
    if not exact(snapshot, SNAPSHOT_FIELDS):
        return dict(code="INVALID_SNAPSHOT")
    s = snapshot
    if (not all(integer(s[k], 1) for k in ("schemaVersion", "linkId", "ttlMs", "routeId",
                                            "routeNodeCount", "talkerPlayerId", "listenerPlayerId",
                                            "descriptorVersion"))
            or not all(integer(s[k], 1, MAX-1) for k in ("sessionEpoch", "linkSeq"))
            or s["routeId"] != s["linkId"] or s["routeNodeCount"] != 2
            or s["talkerPlayerId"] == s["listenerPlayerId"]
            or type(s["state"]) is not str or s["state"] not in legacy.STATES
            or type(s["mode"]) is not str or not binding_valid(s["binding"])):
        return dict(code="INVALID_SNAPSHOT")
    try:
        cf.key(s["timebaseId"])
        quality = cf.number(s["quality"], maximum=1)
        if s["state"] == "LOST" and quality != 0:
            raise ValueError("LOST diagnostic must have Q0")
    except (TypeError, ValueError, OverflowError):
        return dict(code="INVALID_SNAPSHOT")
    try:
        descriptor = cd.normal(s["descriptor"])
    except (KeyError, TypeError, ValueError, OverflowError):
        return dict(code="INVALID_DESCRIPTOR")
    for bad, code in ((s["schemaVersion"] != 2, "UNSUPPORTED_SCHEMA"),
                      (s["descriptorVersion"] != 1, "UNSUPPORTED_DESCRIPTOR_VERSION"),
                      (descriptor["modelId"] != cf.MODEL, "UNSUPPORTED_MODEL"),
                      (s["mode"] not in ("SERVER", "RECEIVER"), "UNSUPPORTED_MODE")):
        if bad:
            return dict(code=code)
    encoded = cd.encode(descriptor, limits["maxDescriptorBytes"])
    if encoded["code"] != "OK":
        return dict(code=encoded["code"])
    if encoded["sizeBytes"] > limits["maxTotalDescriptorBytes"]:
        return dict(code="RESOURCE_LIMIT")
    normal = copy.deepcopy(s)
    normal.update(quality=quality, descriptor=encoded["descriptor"])
    return dict(code="OK", snapshot=normal, descriptorBytes=encoded["bytes"])


def context_matches(snapshot, expected):
    if not exact(expected, EXPECTED_FIELDS) or not binding_valid(expected["binding"]):
        return False
    if not exact(expected["channel"], "tx rx frequencyMHz bandwidthHz profileId profileVersion environmentRevision"):
        return False
    if (not all(integer(expected[k], 1) for k in
                ("linkId", "talkerPlayerId", "listenerPlayerId"))
            or not integer(expected["sessionEpoch"], 1, MAX-1)
            or type(expected["mode"]) is not str
            or expected["mode"] not in ("SERVER", "RECEIVER")):
        return False
    try:
        cf.key(expected["timebaseId"])
        query = dict(expected["channel"], nowMs=snapshot["descriptor"]["validFromMs"], subUs=0)
        d, q = cf.preflight(dict(descriptor=snapshot["descriptor"], query=query))
    except (KeyError, TypeError, ValueError, OverflowError):
        return False
    binding = expected["binding"]
    tx = (binding["txDeviceId"], binding["txTransceiverIndex"])
    rx = (binding["rxDeviceId"], binding["rxTransceiverIndex"])
    return (all(snapshot[k] == expected[k] for k in EXPECTED_FIELDS.split() if k != "channel")
            and (q["tx"], q["rx"]) == (tx, rx)
            and tuple(sorted((tx, rx))) == (d["a"], d["b"])
            and all(q[k] == d[k] for k in
                    ("frequencyMHz", "bandwidthHz", "profileId", "profileVersion", "environmentRevision")))


def validate(snapshot, expected, limits=DEFAULT_LIMITS):
    result = typed(snapshot, limits)
    if result["code"] == "OK" and not context_matches(result["snapshot"], expected):
        return dict(code="CONTEXT_MISMATCH")
    return result


def preflight(snapshots, expected_by_link, limits):
    if not limits_valid(limits):
        return dict(code="INVALID_LIMITS")
    if (type(snapshots) is not list or type(expected_by_link) is not dict
            or not all(integer(key, 1) for key in expected_by_link)):
        return dict(code="INVALID_INPUT")
    if len(snapshots) > limits["maxSnapshots"]:
        return dict(code="BATCH_LIMIT")
    rows, seen, total = [], set(), 0
    for snapshot in snapshots:
        link = snapshot.get("linkId") if type(snapshot) is dict else None
        expected = expected_by_link.get(link) if integer(link, 1) else None
        result = validate(snapshot, expected, limits)
        if result["code"] != "OK":
            return dict(code=result["code"])
        link_id = result["snapshot"]["linkId"]
        if link_id in seen:
            return dict(code="DUPLICATE_LINK")
        seen.add(link_id)
        total += len(result["descriptorBytes"])
        if total > limits["maxTotalDescriptorBytes"]:
            return dict(code="RESOURCE_LIMIT")
        rows.append(result)
    return dict(code="OK", rows=rows)


class Publisher:
    """HOST active slots with caller-assigned IDs. Not S3b pair/lease lifecycle."""
    def __init__(self, expected_by_link, limits=DEFAULT_LIMITS):
        self.expected = copy.deepcopy(expected_by_link)
        self.limits = copy.deepcopy(limits)
        self.entries = {}

    def read(self, link_id):
        slot = self.entries.get(link_id) if integer(link_id, 1) else None
        return copy.deepcopy(slot[0]) if slot else None

    def publish(self, now_ms, candidates, next_seq, quality_delta=0.125, force=False):
        checked = preflight(candidates, self.expected, self.limits)
        def result(code, snapshots=None, calls=0):
            return dict(code=code, snapshots=[] if snapshots is None else snapshots, sequenceCalls=calls)
        if checked["code"] != "OK":
            return result(checked["code"])
        try:
            delta = cf.number(quality_delta, maximum=1, positive=True)
        except (TypeError, ValueError, OverflowError):
            return result("INVALID_INPUT")
        if not integer(now_ms) or type(force) is not bool or not callable(next_seq):
            return result("INVALID_INPUT")
        retained = {k: v[0] for k, v in self.entries.items()}
        retained.update({r["snapshot"]["linkId"]: r["snapshot"] for r in checked["rows"]})
        capacity = preflight(list(retained.values()), self.expected, self.limits)
        if capacity["code"] != "OK":
            return result(capacity["code"])
        due = []
        for row in checked["rows"]:
            s = row["snapshot"]
            old = self.entries.get(s["linkId"])
            if old is not None and now_ms < old[1]:
                return result("CLOCK_ROLLBACK")
            if old is None:
                due.append(s)
                continue
            previous, published_at = old
            # Wire ordering fields do not seed the physical process.
            same_context = all(s[k] == previous[k] for k in EXPECTED_FIELDS.split() if k != "channel")
            if (force or not same_context or s["state"] != previous["state"]
                    or s["ttlMs"] != previous["ttlMs"]
                    or cf.f32(abs(cf.f32(s["quality"] - previous["quality"]))) >= delta
                    or row["descriptorBytes"] != cd.encode(previous["descriptor"], self.limits["maxDescriptorBytes"])["bytes"]
                    or now_ms - published_at >= (s["ttlMs"] // 2 + s["ttlMs"] % 2)):
                due.append(s)
        pending, outputs, calls = copy.deepcopy(self.entries), [], 0
        for s in due:
            calls += 1
            try:
                seq = next_seq(s["linkId"])
            except Exception:
                return result("NEED_FULL_RESYNC", calls=calls)
            old = self.entries.get(s["linkId"])
            if not integer(seq, 1, MAX-1) or (old and seq <= old[0]["linkSeq"]):
                return result("NEED_FULL_RESYNC", calls=calls)
            emitted = copy.deepcopy(s)
            emitted["linkSeq"] = seq
            pending[s["linkId"]] = (copy.deepcopy(emitted), now_ms)
            outputs.append(emitted)
        self.entries = pending
        return result("OK", outputs, calls)


class Receiver:
    """Reuse v1 ordering/TTL only after explicit v2 admission; never downgrade payload."""
    def __init__(self, expected_by_link, limits=DEFAULT_LIMITS):
        self.expected = copy.deepcopy(expected_by_link)
        self.limits = copy.deepcopy(limits)
        self.legacy = legacy.Receiver(2)

    def read(self, link_id):
        slot = self.legacy.slots.get(link_id) if integer(link_id, 1) else None
        return copy.deepcopy(slot[0]) if slot else None

    def visible(self, now_ms, link_id):
        if not integer(now_ms) or not integer(link_id, 1):
            return dict(visible=False, effects=["DIAGNOSTIC"])
        return self.legacy.visible(now_ms, link_id)

    def full_resync(self, now_ms, epoch, schema, snapshots):
        checked = preflight(snapshots, self.expected, self.limits)
        if checked["code"] == "OK" and integer(schema, 1) and schema != 2:
            checked = dict(code="UNSUPPORTED_SCHEMA")
        if (not integer(now_ms) or not integer(epoch, 1, MAX-1)
                or not integer(schema, 1) or checked["code"] != "OK"):
            self.legacy.epoch, self.legacy.slots = None, {}
            return self.legacy.result("INVALID_RESYNC", ["REQUEST_RESYNC", "DIAGNOSTIC"]) | {
                "reason": checked["code"] if checked["code"] != "OK" else "INVALID_INPUT"}
        result = self.legacy.full_resync(now_ms, epoch, schema, [r["snapshot"] for r in checked["rows"]])
        if result["decision"] == "INVALID_RESYNC":
            result["reason"] = "INVALID_IDENTITY"
        return result

    def receive(self, now_ms, snapshot):
        checked = typed(snapshot, self.limits)
        if not integer(now_ms) or checked["code"] != "OK":
            reason = checked["code"] if checked["code"] != "OK" else "INVALID_INPUT"
            effects = ["REQUEST_RESYNC"] if reason.startswith("UNSUPPORTED_") else ["DIAGNOSTIC"]
            return self.legacy.result("INVALID_SNAPSHOT", effects) | dict(reason=reason)
        s = checked["snapshot"]
        if self.legacy.epoch is None or s["sessionEpoch"] != self.legacy.epoch:
            return self.legacy.receive(now_ms, s)
        if not context_matches(s, self.expected.get(s["linkId"])):
            return self.legacy.result("INVALID_SNAPSHOT", ["REQUEST_RESYNC"]) | dict(reason="CONTEXT_MISMATCH")
        old = self.legacy.slots.get(s["linkId"])
        if old is None or s["linkSeq"] > old[0]["linkSeq"]:
            retained = {k: v[0] for k, v in self.legacy.slots.items()}
            retained[s["linkId"]] = s
            capacity = preflight(list(retained.values()), self.expected, self.limits)
            if capacity["code"] != "OK":
                return self.legacy.result("INVALID_SNAPSHOT", ["REQUEST_RESYNC"]) | dict(reason=capacity["code"])
        return self.legacy.receive(now_ms, s)

    def view(self, now_ms, link_id, mapping, sub_us=0):
        visible = self.visible(now_ms, link_id)["visible"]
        def result(code, fading=None):
            out = dict(code=code, visible=visible)
            if fading is not None:
                out["result"] = fading
            return out
        if not visible:
            return result("PRESENTATION_EXPIRED")
        s = self.read(link_id)
        if not exact(mapping, MAPPING_FIELDS):
            return result("CLOCK_UNVERIFIED")
        try:
            cf.key(mapping["mappingId"])
            cf.key(mapping["timebaseId"])
        except (TypeError, ValueError):
            return result("CLOCK_UNVERIFIED")
        if (not all(integer(mapping[k]) for k in MAPPING_FIELDS.split()[2:])
                or not integer(sub_us, 0, 999)
                or not mapping["validFromLocalMs"] <= mapping["localAnchorMs"] < mapping["validUntilLocalMs"]
                or not mapping["validFromLocalMs"] <= now_ms < mapping["validUntilLocalMs"]
                or mapping["timebaseId"] != s["timebaseId"]):
            return result("CLOCK_UNVERIFIED")
        estimate = 1000 * (mapping["serverAnchorMs"] + now_ms - mapping["localAnchorMs"]) + sub_us
        lower, upper = estimate - mapping["maxErrorUs"], estimate + mapping["maxErrorUs"]
        if lower < 0 or upper > MAX * 1000 + 999:
            return result("CLOCK_OUT_OF_RANGE")
        d = s["descriptor"]
        if lower < d["validFromMs"] * 1000 or upper >= d["validUntilMs"] * 1000:
            return result("STALE_DESCRIPTOR")
        query = copy.deepcopy(self.expected[link_id]["channel"])
        query.update(nowMs=estimate // 1000, subUs=estimate % 1000)
        fading = cf.evaluate(dict(descriptor=d, query=query))
        return result(fading["code"], fading)
