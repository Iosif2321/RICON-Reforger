"""HOST-only ordered ACTIVE/END protocol; no change to raw temporal/v1 receivers."""
from __future__ import annotations

import copy

import temporal_snapshot_oracle as ts

MAX = ts.MAX
ENVELOPE_FIELDS = "protocolVersion kind ownerPlayerId sessionEpoch deliverySeq linkIdHighWater items"
AUTHORITY_FIELDS = "ownerPlayerId sessionEpoch deliverySeq kind linkIdHighWater activeExpected endedTargets"
END_FIELDS = "sessionEpoch linkId linkSeq talkerPlayerId listenerPlayerId binding reason"
END_REASONS = frozenset((
    "TTL_EXPIRED", "CLIENT_STOP", "DISCONNECT", "RECONNECT", "ENTITY_CHANGED",
    "ENTITY_INACTIVE", "ACCESS_LOST", "POWER_LOST", "DEVICE_REMOVED",
    "CATALOG_INVALIDATED", "SESSION_ROTATED", "ADMIN_REVOKED", "TICK_ANOMALY", "LINK_REMOVED",
))


def _header(frame):
    return (ts.exact(frame, ENVELOPE_FIELDS)
            and ts.integer(frame["protocolVersion"], 1, 1)
            and type(frame["kind"]) is str and frame["kind"] in ("FULL", "DELTA")
            and ts.integer(frame["ownerPlayerId"], 1)
            and ts.integer(frame["sessionEpoch"], 1, MAX - 1)
            and ts.integer(frame["deliverySeq"], 1, MAX - 1)
            and ts.integer(frame["linkIdHighWater"]))


def _expected_valid(expected):
    if (not ts.exact(expected, ts.EXPECTED_FIELDS) or not ts.binding_valid(expected["binding"])
            or not ts.exact(expected["channel"], "tx rx frequencyMHz bandwidthHz profileId profileVersion environmentRevision")
            or not all(ts.integer(expected[k], 1) for k in ("linkId", "talkerPlayerId", "listenerPlayerId"))
            or not ts.integer(expected["sessionEpoch"], 1, MAX - 1)
            or expected["talkerPlayerId"] == expected["listenerPlayerId"]
            or type(expected["mode"]) is not str or expected["mode"] not in ("SERVER", "RECEIVER")):
        return False
    try:
        ts.cf.key(expected["timebaseId"])
        channel = expected["channel"]
        ts.cf.key(channel["profileId"])
        ts.cf.integer(channel["profileVersion"], 1)
        ts.cf.integer(channel["environmentRevision"], 1)
        ts.cf.number(channel["frequencyMHz"], positive=True)
        ts.cf.number(channel["bandwidthHz"], positive=True)
        tx, rx = ts.cf.endpoint(channel["tx"]), ts.cf.endpoint(channel["rx"])
        binding = expected["binding"]
        return (tx != rx
                and tx == (binding["txDeviceId"], binding["txTransceiverIndex"])
                and rx == (binding["rxDeviceId"], binding["rxTransceiverIndex"]))
    except (TypeError, ValueError, OverflowError):
        return False


def _maps_valid(authority):
    for name, valid in (("activeExpected", _expected_valid), ("endedTargets", _end_valid)):
        records = authority[name]
        if type(records) is not dict or len(records) > 65536:
            return False
        for link, value in records.items():
            if (not ts.integer(link, 1) or not valid(value) or value["linkId"] != link
                    or value["sessionEpoch"] != authority["sessionEpoch"]
                    or value["listenerPlayerId"] != authority["ownerPlayerId"]):
                return False
    return True


def _end_valid(end):
    return (ts.exact(end, END_FIELDS)
            and all(ts.integer(end[k], 1, MAX - 1) for k in ("sessionEpoch", "linkSeq"))
            and all(ts.integer(end[k], 1) for k in ("linkId", "talkerPlayerId", "listenerPlayerId"))
            and end["talkerPlayerId"] != end["listenerPlayerId"]
            and ts.binding_valid(end["binding"])
            and type(end["reason"]) is str and end["reason"] in END_REASONS)


def _same_binding(left, right):
    return all(left[k] == right[k] for k in
               ("sessionEpoch", "linkId", "talkerPlayerId", "listenerPlayerId", "binding"))


class OrderedReceiver:
    """Caller supplies independent current-authority receipts, not self-authenticating packet fields."""

    def __init__(self, owner_player_id, limits=ts.DEFAULT_LIMITS):
        self._owner = owner_player_id if ts.integer(owner_player_id, 1) else 0
        self._limits = copy.deepcopy(limits) if ts.limits_valid(limits) else None
        self._configuration = ("INVALID_LIMITS" if not ts.limits_valid(limits)
                               else "INVALID_INPUT" if not self._owner else "OK")
        self._epoch = None
        self._delivery_seq = 0
        self._checkpoint = 0
        self._seen = 0
        self._observed_epoch = None
        self._observed_seq = 0
        self._observed_high = 0
        self._needs_full = True
        self._active = {}
        self._closed = {}

    def state(self):
        return copy.deepcopy(dict(ownerPlayerId=self._owner, sessionEpoch=self._epoch,
                                  deliverySeq=self._delivery_seq, checkpointHighWater=self._checkpoint,
                                  seenHighWater=self._seen, needsFull=self._needs_full,
                                  observedEpoch=self._observed_epoch, observedDeliverySeq=self._observed_seq,
                                  observedHighWater=self._observed_high,
                                  active=self._active, closed=self._closed))

    def read(self, link):
        row = self._active.get(link) if ts.integer(link, 1) else None
        return copy.deepcopy(row["snapshot"]) if row else None

    def visible(self, now_ms, link):
        if self._needs_full or not ts.integer(now_ms) or not ts.integer(link, 1):
            return False
        row = self._active.get(link)
        return bool(row and 0 <= now_ms - row["receivedAtMs"] < row["snapshot"]["ttlMs"])

    def _result(self, code):
        return dict(code=code, needsFull=self._needs_full, deliverySeq=self._delivery_seq)

    def _reject(self, code, desync=True):
        if desync and code != "INVALID_AUTHORITY":
            self._needs_full = True
        return self._result(code)

    def _observe(self, frame):
        epoch = frame["sessionEpoch"]
        if self._observed_epoch != epoch:
            self._observed_epoch, self._observed_seq, self._observed_high = epoch, 0, 0
        self._observed_seq = max(self._observed_seq, frame["deliverySeq"])
        self._observed_high = max(self._observed_high, frame["linkIdHighWater"])
        if self._epoch == epoch:
            self._seen = max(self._seen, frame["linkIdHighWater"])

    def _collect(self, frame, authority):
        items = frame["items"]
        if type(items) is not list:
            return "INVALID_INPUT", []
        if len(items) > self._limits["maxSnapshots"]:
            return "BATCH_LIMIT", []
        rows, seen, ended_ids = [], set(), set()
        for item in items:
            if ts.exact(item, "kind snapshot") and item["kind"] == "ACTIVE":
                raw = item["snapshot"]
                link = raw.get("linkId") if type(raw) is dict else None
                expected = authority["activeExpected"].get(link) if ts.integer(link, 1) else None
                checked = ts.validate(raw, expected, self._limits)
                if checked["code"] != "OK":
                    return checked["code"], []
                record = checked["snapshot"]
                kind = "ACTIVE"
            elif ts.exact(item, "kind end") and item["kind"] == "END":
                record = item["end"]
                if frame["kind"] == "FULL" or not _end_valid(record):
                    return "INVALID_END", []
                link = record["linkId"]
                receipt = authority["endedTargets"].get(link)
                if not _end_valid(receipt) or record != receipt:
                    return "INVALID_AUTHORITY", []
                record = copy.deepcopy(record)
                ended_ids.add(link)
                kind = "END"
            else:
                return "INVALID_INPUT", []
            link = record["linkId"]
            if record["sessionEpoch"] != frame["sessionEpoch"] or record["listenerPlayerId"] != self._owner:
                return "BINDING_MISMATCH", []
            if link > frame["linkIdHighWater"]:
                return "HIGH_WATER_ROLLBACK", []
            if link in seen:
                return "DUPLICATE_LINK", []
            seen.add(link)
            rows.append((kind, record))
        if set(authority["endedTargets"]) != ended_ids:
            return "INVALID_AUTHORITY", []
        return "OK", rows

    def _authorize_items(self, frame, authority):
        """Check every END grant before observing a transaction, before ACTIVE payload domains."""
        seen, ended_ids = set(), set()
        for item in frame["items"]:
            if ts.exact(item, "kind snapshot") and item["kind"] == "ACTIVE":
                raw = item["snapshot"]
                if not ts.exact(raw, ts.SNAPSHOT_FIELDS) or not ts.integer(raw["linkId"], 1):
                    return "INVALID_SNAPSHOT"
                link = raw["linkId"]
            elif ts.exact(item, "kind end") and item["kind"] == "END":
                end = item["end"]
                if frame["kind"] == "FULL" or not _end_valid(end):
                    return "INVALID_END"
                link = end["linkId"]
                receipt = authority["endedTargets"].get(link)
                if not _end_valid(receipt) or end != receipt:
                    return "INVALID_AUTHORITY"
                ended_ids.add(link)
            else:
                return "INVALID_INPUT"
            if link in seen:
                return "DUPLICATE_LINK"
            seen.add(link)
        return "OK" if set(authority["endedTargets"]) == ended_ids else "INVALID_AUTHORITY"

    def _active_row(self, snapshot, old, now_ms, expected):
        if old:
            previous = old["snapshot"]
            if not _same_binding(previous, snapshot):
                return "BINDING_MISMATCH", None
            if snapshot["linkSeq"] < previous["linkSeq"]:
                return "LINK_SEQUENCE_ROLLBACK", None
            if snapshot["linkSeq"] == previous["linkSeq"]:
                if snapshot != previous:
                    return "LINK_SEQUENCE_CONFLICT", None
                return "OK", copy.deepcopy(old)
        return "OK", dict(snapshot=copy.deepcopy(snapshot), receivedAtMs=now_ms,
                          expected=copy.deepcopy(expected))

    def _postflight(self, active, closed, expected):
        if set(active) != set(expected):
            return "COMPLETENESS_MISMATCH"
        if len(active) + len(closed) > self._limits["maxSnapshots"]:
            return "RESOURCE_LIMIT"
        checked = ts.preflight([row["snapshot"] for row in active.values()], expected, self._limits)
        if checked["code"] != "OK":
            return checked["code"]
        for link, row in active.items():
            row["expected"] = copy.deepcopy(expected[link])
        return "OK"

    def apply(self, now_ms, envelope, authority):
        if self._configuration != "OK":
            return self._reject(self._configuration, False)
        if not _header(envelope):
            return self._reject("INVALID_INPUT", False)
        if (envelope["ownerPlayerId"] != self._owner or type(authority) is not dict
                or not ts.integer(authority.get("ownerPlayerId"), 1)
                or not ts.integer(authority.get("sessionEpoch"), 1, MAX - 1)
                or authority["ownerPlayerId"] != self._owner
                or authority["sessionEpoch"] != envelope["sessionEpoch"]):
            return self._reject("INVALID_AUTHORITY", False)
        same_epoch = self._epoch == envelope["sessionEpoch"]
        if same_epoch and envelope["deliverySeq"] <= self._delivery_seq:
            return self._result("STALE")
        observed_scope = self._observed_epoch == envelope["sessionEpoch"]
        if observed_scope and envelope["deliverySeq"] <= self._observed_seq:
            return self._result("STALE")
        if (not ts.exact(authority, AUTHORITY_FIELDS)
                or not ts.integer(authority["deliverySeq"], 1, MAX - 1)
                or not ts.integer(authority["linkIdHighWater"])
                or type(authority["kind"]) is not str
                or any(authority[k] != envelope[k] for k in
                       ("ownerPlayerId", "sessionEpoch", "deliverySeq", "kind", "linkIdHighWater"))):
            return self._reject("INVALID_AUTHORITY", False)
        # Validate raw inputs first; only normalized/candidate/retained values are copied.
        frame, receipt = envelope, authority
        if not ts.integer(now_ms):
            return self._reject("INVALID_INPUT")
        if type(frame["items"]) is not list:
            return self._reject("INVALID_INPUT")
        if len(frame["items"]) > self._limits["maxSnapshots"]:
            return self._reject("BATCH_LIMIT")
        if not _maps_valid(receipt):
            return self._reject("INVALID_AUTHORITY")
        code = self._authorize_items(frame, receipt)
        if code != "OK":
            return self._reject(code)
        required_high = self._seen if same_epoch else 0
        if observed_scope:
            required_high = max(required_high, self._observed_high)
        # Authenticated control evidence survives a rejected gap/cap transaction without committing payload.
        self._observe(frame)
        if frame["kind"] == "DELTA":
            if not same_epoch or self._needs_full:
                return self._reject("NEED_FULL")
            if self._delivery_seq >= MAX - 1 or frame["deliverySeq"] != self._delivery_seq + 1:
                return self._reject("GAP")
        if frame["linkIdHighWater"] < max(required_high, self._checkpoint if same_epoch else 0):
            return self._reject("HIGH_WATER_ROLLBACK")
        code, rows = self._collect(frame, receipt)
        if code != "OK":
            return self._reject(code)
        full = frame["kind"] == "FULL"
        active = {} if full else copy.deepcopy(self._active)
        closed = {} if full else copy.deepcopy(self._closed)
        prior_active = self._active if same_epoch else {}
        prior_closed = self._closed if same_epoch else {}
        barrier = self._checkpoint if same_epoch else 0
        for kind, record in rows:
            link = record["linkId"]
            if kind == "ACTIVE":
                old = prior_active.get(link)
                if link in prior_closed or (not old and link <= barrier):
                    return self._reject("CLOSED_ID")
                code, row = self._active_row(record, old, now_ms, receipt["activeExpected"][link])
                if code != "OK":
                    return self._reject(code)
                active[link] = row
                continue
            old_active, old_end = active.get(link), closed.get(link)
            if old_active:
                previous = old_active["snapshot"]
                if not _same_binding(previous, record):
                    return self._reject("BINDING_MISMATCH")
                if record["linkSeq"] < previous["linkSeq"]:
                    return self._reject("LINK_SEQUENCE_ROLLBACK")
                if record["linkSeq"] == previous["linkSeq"]:
                    return self._reject("LINK_SEQUENCE_CONFLICT")
                del active[link]
            elif old_end:
                if not _same_binding(old_end, record):
                    return self._reject("BINDING_MISMATCH")
                if record["linkSeq"] < old_end["linkSeq"]:
                    return self._reject("LINK_SEQUENCE_ROLLBACK")
                if record["linkSeq"] == old_end["linkSeq"]:
                    if record != old_end:
                        return self._reject("LINK_SEQUENCE_CONFLICT")
                    continue
            elif link <= barrier:
                continue  # Checkpoint already closes this ID; receipt cannot turn it into ACTIVE.
            closed[link] = copy.deepcopy(record)
        code = self._postflight(active, closed, receipt["activeExpected"])
        if code != "OK":
            return self._reject(code)
        self._active, self._closed = active, closed
        self._epoch = frame["sessionEpoch"]
        self._delivery_seq = frame["deliverySeq"]
        self._seen = self._observed_high
        if full:
            self._checkpoint = frame["linkIdHighWater"]
        self._needs_full = False
        return self._result("ACCEPT_FULL" if full else "ACCEPT_DELTA")
