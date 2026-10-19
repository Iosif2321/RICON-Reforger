#!/usr/bin/env python3
"""Host-эталон жизненного цикла RICON по docs/req/REQ-lifecycle.md (stdlib). Не production-код.

Модель принимает упорядоченный trace событий (§1.2) и для каждого события возвращает result record:
`resultCode`, упорядоченный список `effects` (§1.1) и поля ответа PTT. Раннер сверяет их с ожиданиями из
`oracle/vectors/lifecycle_v1.json`, выписанными вручную по тексту REQ.

Tie-breakers закреплены в REQ; README (раздел «Жизненный цикл») индексирует соответствие эталона.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any

INT_MAX = 2147483647

STATES = ("CONNECTED_UNREGISTERED", "REGISTERED_UNBOUND", "BOUND_ACTIVE", "BOUND_INACTIVE", "DISCONNECTED")
RESULT_CODES = (
    "MALFORMED", "TICK_ANOMALY", "NOT_READY", "INVALID_SESSION", "STALE_CONNECTION", "STALE_GENERATION",
    "STALE_ENTITY", "DUPLICATE", "REQUEST_CONFLICT", "STALE_REQUEST", "LIMIT_REACHED", "RATE_LIMITED",
    "FORGED_OWNER", "INACTIVE_ENTITY", "UNKNOWN_DEVICE", "NOT_OWNER", "POWER_OFF", "INVALID_CATALOG",
    "DEVICE_BUSY", "UNKNOWN_LEASE", "STALE_LEASE", "OK",
)
EFFECT_TYPES = (
    "PLAYER_STATE_CHANGED", "ENTITY_BOUND", "ENTITY_UNBOUND", "LEASE_GRANTED", "LEASE_TERMINATED",
    "VON_EXPIRY_INTENDED", "SUBSCRIPTIONS_CLEARED", "CARRIED_DEVICES_DETACHED", "TOMBSTONE_WRITTEN",
    "FULL_RESYNC_REQUIRED",
)
EVENT_TYPES = (
    "INIT", "CONNECTED", "REGISTERED", "ENTITY_BOUND", "ENTITY_CHANGED", "ENTITY_UNBOUND", "DISCONNECTED",
    "PTT_START", "PTT_STOP", "TICK", "ROTATE_EPOCH",
)
CONFIG_KEYS = (
    "maxPlayers", "maxActiveLeases", "maxTerminalLeases", "terminalLeaseTtlMs", "maxTombstones", "tombstoneTtlMs",
    "maxResponseCachePerPlayer", "maxRateEntries", "rateWindowMs", "rateMaxRequests",
)
UNBIND_REASONS = ("KILLED", "DELETED", "CONTROL_LOST")
METHOD_IDS = {"PTT_START": 1, "PTT_STOP": 2}
# Client payload fields in schema order (§1.2); they form the canonical fingerprint (§5.2.1) together with methodId.
PAYLOAD_FIELDS = {
    "PTT_START": ("schemaVersion", "correlationId", "payloadPlayerId", "sessionEpoch", "generation", "requestSeq",
                  "deviceId", "networkTypeId", "channelId"),
    "PTT_STOP": ("schemaVersion", "correlationId", "payloadPlayerId", "sessionEpoch", "generation", "requestSeq",
                 "leaseId"),
}


def is_int(value: Any) -> bool:
    return isinstance(value, int) and not isinstance(value, bool)


def is_id(value: Any) -> bool:
    """Mandatory int32 ID: 1..INT_MAX (§1.2)."""
    return is_int(value) and 1 <= value <= INT_MAX


def is_time(value: Any) -> bool:
    return is_int(value) and 0 <= value <= INT_MAX


class Lifecycle:
    """Server authority state of REQ-lifecycle v0.3 (host model)."""

    def __init__(self) -> None:
        self.ready = False
        self.init_record: Any = None
        self.epoch = 0
        self.config: dict[str, int] = {}
        self.catalog: dict[int, dict[str, Any]] = {}
        self.last_now = 0
        self.latched = False
        self.gen_alloc = 0
        self.lease_alloc = 0
        self.players: dict[int, dict[str, Any]] = {}
        self.tombstones: list[dict[str, Any]] = []
        self.leases: dict[int, dict[str, Any]] = {}
        self.device_active: dict[int, int] = {}
        self.rate: dict[tuple[int, int], dict[str, int]] = {}

    # ------------------------------------------------------------------ helpers

    def _terminate(self, lease_id: int, state: str, reason: str, now: int, effects: list[dict[str, Any]]) -> None:
        """Active lease -> terminal, exactly once, as the adjacent pair LEASE_TERMINATED + VON_EXPIRY_INTENDED (§5.1)."""
        lease = self.leases[lease_id]
        lease["state"], lease["reason"], lease["terminalAtMs"] = state, reason, now
        del self.device_active[lease["deviceId"]]
        effects.append({"type": "LEASE_TERMINATED", "playerId": lease["playerId"], "generation": lease["generation"],
                        "deviceId": lease["deviceId"], "leaseId": lease_id, "leaseState": state,
                        "terminalReason": reason})
        # Epoch of issue: _rotate terminates leases before it switches self.epoch (REQ v0.4 §1.1, §11).
        effects.append({"type": "VON_EXPIRY_INTENDED", "sessionEpoch": self.epoch, "playerId": lease["playerId"],
                        "deviceId": lease["deviceId"], "leaseId": lease_id})
        # A cached start response must not keep reporting ACTIVE (§5.2.1).
        player = self.players.get(lease["playerId"])
        if player is not None and player["generation"] == lease["generation"]:
            for entry in player["cache"].values():
                if entry["response"].get("leaseId") == lease_id:
                    entry["response"]["leaseState"] = state
                    entry["response"]["terminalReason"] = reason
        self._evict_terminal_leases()

    def _evict_terminal_leases(self) -> None:
        """Bounded terminal records: evict the oldest by (terminalAtMs, leaseId) beyond maxTerminalLeases (§9)."""
        terminal = sorted((lease["terminalAtMs"], lid) for lid, lease in self.leases.items()
                          if lease["state"] != "ACTIVE")
        while len(terminal) > self.config["maxTerminalLeases"]:
            _, lid = terminal.pop(0)
            del self.leases[lid]

    def _active_lease_ids(self) -> list[int]:
        return sorted(lid for lid, lease in self.leases.items() if lease["state"] == "ACTIVE")

    def _active_leases_of(self, player_id: int) -> list[int]:
        return sorted(lid for lid, lease in self.leases.items()
                      if lease["state"] == "ACTIVE" and lease["playerId"] == player_id)

    def _latest_tombstone(self, player_id: int) -> dict[str, Any] | None:
        own = [t for t in self.tombstones if t["playerId"] == player_id]
        return max(own, key=lambda t: t["generation"]) if own else None

    def _observe_and_sweep(self, now: int, effects: list[dict[str, Any]]) -> None:
        """§1.1 step 3 and §9: update time, expire leases by leaseId, then TTL cleanup."""
        self.last_now = now
        due = [lid for lid in sorted(self.leases) if self.leases[lid]["state"] == "ACTIVE"
               and now - self.leases[lid]["issuedAtMs"] >= self.leases[lid]["ttlMs"]]
        for lid in due:  # snapshot: eviction inside _terminate removes only terminal records
            self._terminate(lid, "EXPIRED", "TTL_EXPIRED", now, effects)
        self.tombstones = [t for t in self.tombstones if now < t["expiresAtMs"]]
        ttl = self.config["terminalLeaseTtlMs"]
        for lid in [lid for lid, lease in self.leases.items()
                    if lease["state"] != "ACTIVE" and now - lease["terminalAtMs"] >= ttl]:
            del self.leases[lid]

    def _cleanup_block(self, player_id: int, reason: str, now: int, effects: list[dict[str, Any]]) -> None:
        """Disconnect / live reconnect cleanup in the fixed order of §4.3 (steps 1-6)."""
        player = self.players[player_id]
        gen = player["generation"]
        for lid in self._active_leases_of(player_id):
            self._terminate(lid, "REVOKED", reason, now, effects)
        effects.append({"type": "SUBSCRIPTIONS_CLEARED", "playerId": player_id, "generation": gen})
        if player["entityId"] is not None:
            effects.append({"type": "ENTITY_UNBOUND", "playerId": player_id, "generation": gen,
                            "entityId": player["entityId"]})
        effects.append({"type": "CARRIED_DEVICES_DETACHED", "playerId": player_id, "generation": gen})
        effects.append({"type": "PLAYER_STATE_CHANGED", "playerId": player_id, "oldGeneration": gen,
                        "newGeneration": gen, "fromState": player["state"], "toState": "DISCONNECTED"})
        if len(self.tombstones) >= self.config["maxTombstones"]:
            oldest = min(self.tombstones, key=lambda t: (t["expiresAtMs"], t["playerId"], t["generation"]))
            self.tombstones.remove(oldest)
        tomb = {"playerId": player_id, "generation": gen, "connectionId": player["connectionId"],
                "lastRequestSeq": player["lastRequestSeq"],
                "expiresAtMs": min(now + self.config["tombstoneTtlMs"], INT_MAX)}  # int32-saturated, see README L11
        self.tombstones.append(tomb)
        effects.append({"type": "TOMBSTONE_WRITTEN", "playerId": player_id, "generation": gen,
                        "connectionId": tomb["connectionId"], "expiresAtMs": tomb["expiresAtMs"]})
        del self.players[player_id]
        for key in [key for key in self.rate if key[0] == player_id]:
            del self.rate[key]

    # ------------------------------------------------------------------ INIT

    @staticmethod
    def _valid_catalog(catalog: Any) -> dict[int, dict[str, Any]] | None:
        if not isinstance(catalog, list):
            return None
        result: dict[int, dict[str, Any]] = {}
        ids = []
        for entry in catalog:
            if not isinstance(entry, dict) or not is_id(entry.get("deviceId")):
                return None
            if entry.get("kind") not in ("CARRIED", "WORLD") or not isinstance(entry.get("powered"), bool):
                return None
            owner = entry.get("ownerPlayerId")
            if owner is not None and not is_id(owner):
                return None
            access = entry.get("accessPlayerIds")
            if not isinstance(access, list) or not all(is_id(a) for a in access) or access != sorted(set(access)):
                return None
            allowed = entry.get("allowed")
            if not isinstance(allowed, dict):
                return None
            parsed: dict[int, list[int]] = {}
            keys = list(allowed)
            for key in keys:
                if not re.fullmatch(r"[1-9][0-9]{0,9}", key) or not is_id(int(key)):
                    return None
                channels = allowed[key]
                # A present network key needs at least one channel; an empty list is MALFORMED (REQ catalog v1).
                if not isinstance(channels, list) or not channels or not all(is_id(c) for c in channels) \
                        or channels != sorted(set(channels)):
                    return None
                parsed[int(key)] = channels
            if [int(k) for k in keys] != sorted(parsed):
                return None
            ids.append(entry["deviceId"])
            result[entry["deviceId"]] = {"kind": entry["kind"], "ownerPlayerId": owner, "access": access,
                                         "powered": entry["powered"], "allowed": parsed}
        if ids != sorted(set(ids)):
            return None
        return result

    def _init(self, ev: dict[str, Any]) -> dict[str, Any]:
        record = {"sessionEpoch": ev.get("sessionEpoch"), "config": ev.get("config"), "catalog": ev.get("catalog")}
        config = ev.get("config")
        catalog = self._valid_catalog(ev.get("catalog"))
        if not is_time(ev.get("nowMs")) or not is_id(ev.get("sessionEpoch")) or catalog is None \
                or not isinstance(config, dict) or set(config) != set(CONFIG_KEYS) \
                or not all(is_int(config[k]) and -INT_MAX - 1 <= config[k] <= INT_MAX for k in CONFIG_KEYS):
            return {"resultCode": "MALFORMED", "effects": []}
        if not all(config[k] > 0 for k in CONFIG_KEYS):
            return {"resultCode": "NOT_READY", "effects": []}  # well-formed but invalid config (§1.1)
        self.ready, self.init_record = True, json.dumps(record, sort_keys=True)
        self.epoch, self.config, self.catalog = ev["sessionEpoch"], dict(config), catalog
        self.last_now = ev["nowMs"]
        return {"resultCode": "OK", "effects": []}

    # ------------------------------------------------------------------ dispatch

    def apply(self, ev: dict[str, Any]) -> dict[str, Any]:
        if not isinstance(ev, dict):
            return {"resultCode": "MALFORMED" if self.ready else "NOT_READY", "effects": []}
        kind = ev.get("type")
        if not self.ready:
            if kind == "INIT":
                return self._init(ev)
            return {"resultCode": "NOT_READY", "effects": []}  # §1.1 clock step 1: no observe
        now = ev.get("nowMs")
        if not is_time(now):
            return {"resultCode": "MALFORMED", "effects": []}  # step 2
        effects: list[dict[str, Any]] = []
        if kind == "ROTATE_EPOCH" and self.latched:
            return self._rotate(ev, now, effects)  # step 5: no anomaly-check / sweep
        if self.latched:
            return {"resultCode": "TICK_ANOMALY", "effects": []}
        if now < self.last_now:
            self.latched = True  # step 4
            for lid in self._active_lease_ids():
                self._terminate(lid, "REVOKED", "TICK_ANOMALY", now, effects)
            return {"resultCode": "TICK_ANOMALY", "effects": effects}
        self._observe_and_sweep(now, effects)
        handler = {
            "INIT": self._init_again, "CONNECTED": self._connected, "REGISTERED": self._registered,
            "ENTITY_BOUND": self._entity_bound, "ENTITY_CHANGED": self._entity_changed,
            "ENTITY_UNBOUND": self._entity_unbound, "DISCONNECTED": self._disconnected,
            "PTT_START": self._ptt, "PTT_STOP": self._ptt, "TICK": lambda e, n, f: {"resultCode": "OK"},
            "ROTATE_EPOCH": self._rotate,
        }.get(kind)
        if handler is None:
            result = {"resultCode": "MALFORMED"}
        else:
            result = handler(ev, now, effects)
        result["effects"] = effects
        return result

    def _init_again(self, ev: dict[str, Any], now: int, effects: list) -> dict[str, Any]:
        record = {"sessionEpoch": ev.get("sessionEpoch"), "config": ev.get("config"), "catalog": ev.get("catalog")}
        if json.dumps(record, sort_keys=True) == self.init_record:
            return {"resultCode": "DUPLICATE"}
        return {"resultCode": "REQUEST_CONFLICT"}

    # ------------------------------------------------------------------ callbacks

    def _connected(self, ev: dict[str, Any], now: int, effects: list) -> dict[str, Any]:
        pid, conn = ev.get("playerId"), ev.get("connectionId")
        if not is_id(pid) or not is_id(conn):
            return {"resultCode": "MALFORMED"}
        live = self.players.get(pid)
        if live is not None and live["connectionId"] == conn:
            return {"resultCode": "DUPLICATE"}
        if live is None and len(self.players) >= self.config["maxPlayers"]:
            return {"resultCode": "LIMIT_REACHED"}
        if self.gen_alloc >= INT_MAX:
            return {"resultCode": "LIMIT_REACHED"}  # generation range exhausted: ROTATE_EPOCH required
        from_state, old_gen = "NONE", 0
        if live is not None:
            old_gen = live["generation"]
            self._cleanup_block(pid, "RECONNECT", now, effects)
            from_state = "DISCONNECTED"
        self.gen_alloc += 1
        gen = self.gen_alloc
        self.players[pid] = {"generation": gen, "connectionId": conn, "state": "CONNECTED_UNREGISTERED",
                             "entityId": None, "lastRequestSeq": 0, "cache": {}}
        effects.append({"type": "PLAYER_STATE_CHANGED", "playerId": pid, "oldGeneration": old_gen,
                        "newGeneration": gen, "fromState": from_state, "toState": "CONNECTED_UNREGISTERED"})
        effects.append({"type": "FULL_RESYNC_REQUIRED", "playerId": pid, "sessionEpoch": self.epoch,
                        "generation": gen})
        return {"resultCode": "OK"}

    def _resolve_callback(self, pid: int, gen: int) -> tuple[dict[str, Any] | None, str | None]:
        """Common gate of REGISTERED / ENTITY_* (§4.3): no live state -> tombstone rule, else NOT_READY."""
        live = self.players.get(pid)
        if live is None:
            tomb = self._latest_tombstone(pid)
            if tomb is not None and gen <= tomb["generation"]:
                return None, "STALE_GENERATION"
            return None, "NOT_READY"
        return live, None

    def _registered(self, ev: dict[str, Any], now: int, effects: list) -> dict[str, Any]:
        pid, gen = ev.get("playerId"), ev.get("generation")
        if not is_id(pid) or not is_id(gen):
            return {"resultCode": "MALFORMED"}
        live, code = self._resolve_callback(pid, gen)
        if live is None:
            return {"resultCode": code}
        if gen != live["generation"]:
            return {"resultCode": "STALE_GENERATION"}
        if live["state"] != "CONNECTED_UNREGISTERED":
            return {"resultCode": "DUPLICATE"}
        live["state"] = "REGISTERED_UNBOUND"
        effects.append({"type": "PLAYER_STATE_CHANGED", "playerId": pid, "oldGeneration": gen, "newGeneration": gen,
                        "fromState": "CONNECTED_UNREGISTERED", "toState": "REGISTERED_UNBOUND"})
        return {"resultCode": "OK"}

    def _entity_bound(self, ev: dict[str, Any], now: int, effects: list) -> dict[str, Any]:
        pid, gen, entity = ev.get("playerId"), ev.get("generation"), ev.get("entityId")
        if not is_id(pid) or not is_id(gen) or not is_id(entity):
            return {"resultCode": "MALFORMED"}
        live, code = self._resolve_callback(pid, gen)
        if live is None:
            return {"resultCode": code}
        # Priority 3 NOT_READY precedes STALE_GENERATION (6), see README decision L7.
        if live["state"] == "CONNECTED_UNREGISTERED":
            return {"resultCode": "NOT_READY"}
        if gen != live["generation"]:
            return {"resultCode": "STALE_GENERATION"}
        if live["state"] == "BOUND_ACTIVE":
            return {"resultCode": "STALE_ENTITY" if live["entityId"] != entity else "DUPLICATE"}
        from_state = live["state"]
        live["state"], live["entityId"] = "BOUND_ACTIVE", entity
        effects.append({"type": "ENTITY_BOUND", "playerId": pid, "generation": gen, "entityId": entity})
        effects.append({"type": "PLAYER_STATE_CHANGED", "playerId": pid, "oldGeneration": gen, "newGeneration": gen,
                        "fromState": from_state, "toState": "BOUND_ACTIVE"})
        return {"resultCode": "OK"}

    def _entity_changed(self, ev: dict[str, Any], now: int, effects: list) -> dict[str, Any]:
        pid, gen = ev.get("playerId"), ev.get("generation")
        src, dst = ev.get("fromEntityId"), ev.get("toEntityId")
        if not is_id(pid) or not is_id(gen) or not is_id(src) or not is_id(dst):
            return {"resultCode": "MALFORMED"}
        live, code = self._resolve_callback(pid, gen)
        if live is None:
            return {"resultCode": code}
        if gen != live["generation"]:
            return {"resultCode": "STALE_GENERATION"}
        if live["state"] != "BOUND_ACTIVE" or live["entityId"] != src:
            return {"resultCode": "STALE_ENTITY"}
        if src == dst:
            return {"resultCode": "DUPLICATE"}
        for lid in self._active_leases_of(pid):
            self._terminate(lid, "REVOKED", "ENTITY_CHANGED", now, effects)
        effects.append({"type": "ENTITY_UNBOUND", "playerId": pid, "generation": gen, "entityId": src})
        effects.append({"type": "CARRIED_DEVICES_DETACHED", "playerId": pid, "generation": gen})
        effects.append({"type": "ENTITY_BOUND", "playerId": pid, "generation": gen, "entityId": dst})
        live["entityId"] = dst
        return {"resultCode": "OK"}

    def _entity_unbound(self, ev: dict[str, Any], now: int, effects: list) -> dict[str, Any]:
        pid, gen, entity = ev.get("playerId"), ev.get("generation"), ev.get("entityId")
        if not is_id(pid) or not is_id(gen) or not is_id(entity) or ev.get("reason") not in UNBIND_REASONS:
            return {"resultCode": "MALFORMED"}
        live, code = self._resolve_callback(pid, gen)
        if live is None:
            return {"resultCode": code}
        if gen != live["generation"]:
            return {"resultCode": "STALE_GENERATION"}
        if live["entityId"] is None or live["entityId"] != entity:
            return {"resultCode": "STALE_ENTITY"}
        for lid in self._active_leases_of(pid):
            self._terminate(lid, "REVOKED", "ENTITY_INACTIVE", now, effects)
        effects.append({"type": "ENTITY_UNBOUND", "playerId": pid, "generation": gen, "entityId": entity})
        effects.append({"type": "CARRIED_DEVICES_DETACHED", "playerId": pid, "generation": gen})
        effects.append({"type": "PLAYER_STATE_CHANGED", "playerId": pid, "oldGeneration": gen, "newGeneration": gen,
                        "fromState": live["state"], "toState": "BOUND_INACTIVE"})
        live["state"], live["entityId"] = "BOUND_INACTIVE", None
        return {"resultCode": "OK"}

    def _disconnected(self, ev: dict[str, Any], now: int, effects: list) -> dict[str, Any]:
        pid, gen, conn = ev.get("playerId"), ev.get("generation"), ev.get("connectionId")
        if not is_id(pid) or not is_id(gen) or not is_id(conn):
            return {"resultCode": "MALFORMED"}
        live = self.players.get(pid)
        if live is None:
            tomb = self._latest_tombstone(pid)
            if tomb is not None and tomb["generation"] == gen and tomb["connectionId"] == conn:
                return {"resultCode": "DUPLICATE"}
            if tomb is not None and gen <= tomb["generation"]:
                return {"resultCode": "STALE_GENERATION"}
            return {"resultCode": "NOT_READY"}
        if conn != live["connectionId"]:
            return {"resultCode": "STALE_CONNECTION"}
        if gen != live["generation"]:
            return {"resultCode": "STALE_GENERATION"}
        self._cleanup_block(pid, "DISCONNECT", now, effects)
        return {"resultCode": "OK"}

    # ------------------------------------------------------------------ RPC

    def _ptt(self, ev: dict[str, Any], now: int, effects: list) -> dict[str, Any]:
        kind = ev["type"]
        fields = PAYLOAD_FIELDS[kind]
        auth = ev.get("authenticatedPlayerId")
        if ev.get("schemaVersion") != 1 or not is_id(auth) or not all(
                is_id(ev.get(name)) for name in fields if name != "schemaVersion"):
            return {"resultCode": "MALFORMED"}
        if kind == "PTT_START" and not is_id(ev.get("ttlMs")):
            return {"resultCode": "MALFORMED"}
        gen, seq = ev["generation"], ev["requestSeq"]
        live = self.players.get(auth)
        if live is None:
            tomb = self._latest_tombstone(auth)
            if tomb is None or gen > tomb["generation"]:
                return {"resultCode": "NOT_READY"}
            if ev["sessionEpoch"] != self.epoch:
                return {"resultCode": "INVALID_SESSION"}
            return {"resultCode": "STALE_GENERATION"}
        if ev["sessionEpoch"] != self.epoch:
            return {"resultCode": "INVALID_SESSION"}
        if gen != live["generation"]:
            return {"resultCode": "STALE_GENERATION"}
        fingerprint = [METHOD_IDS[kind]] + [ev[name] for name in fields]
        cached = live["cache"].get(seq)
        if cached is not None:
            if cached["fingerprint"] == fingerprint:
                result = {"resultCode": "DUPLICATE", "cachedResultCode": cached["response"]["resultCode"]}
                result.update({k: v for k, v in cached["response"].items() if k != "resultCode"})
                return result
            return {"resultCode": "REQUEST_CONFLICT"}
        if seq <= live["lastRequestSeq"]:
            return {"resultCode": "STALE_REQUEST"}
        live["lastRequestSeq"] = seq  # the request is now used (§5.2.1)
        response = self._ptt_checks(ev, kind, auth, live, now, effects)
        cache = live["cache"]
        if len(cache) >= self.config["maxResponseCachePerPlayer"]:
            del cache[min(cache)]
        cache[seq] = {"fingerprint": fingerprint, "response": dict(response)}
        return response

    def _ptt_checks(self, ev: dict[str, Any], kind: str, auth: int, live: dict[str, Any], now: int,
                    effects: list) -> dict[str, Any]:
        """Priorities 11..22 for a first-seen request of the current session/generation."""
        method = METHOD_IDS[kind]
        key = (auth, method)
        if key not in self.rate and len(self.rate) >= self.config["maxRateEntries"]:
            return {"resultCode": "LIMIT_REACHED"}
        if kind == "PTT_START":
            active = sum(1 for lease in self.leases.values() if lease["state"] == "ACTIVE")
            if active >= self.config["maxActiveLeases"] or self.lease_alloc >= INT_MAX:
                return {"resultCode": "LIMIT_REACHED"}
        entry = self.rate.setdefault(key, {"windowStartMs": now, "count": 0})
        if now - entry["windowStartMs"] >= self.config["rateWindowMs"]:
            entry["windowStartMs"], entry["count"] = now, 0
        if entry["count"] >= self.config["rateMaxRequests"]:
            return {"resultCode": "RATE_LIMITED"}
        entry["count"] += 1
        if ev["payloadPlayerId"] != auth:
            return {"resultCode": "FORGED_OWNER"}
        if kind == "PTT_START":
            return self._start(ev, auth, live, now, effects)
        return self._stop(ev, auth, live, now, effects)

    def _start(self, ev: dict[str, Any], auth: int, live: dict[str, Any], now: int, effects: list) -> dict[str, Any]:
        if live["state"] != "BOUND_ACTIVE":
            return {"resultCode": "INACTIVE_ENTITY"}
        device = self.catalog.get(ev["deviceId"])
        if device is None:
            return {"resultCode": "UNKNOWN_DEVICE"}
        if device["ownerPlayerId"] != auth and auth not in device["access"]:
            return {"resultCode": "NOT_OWNER"}
        if not device["powered"]:
            return {"resultCode": "POWER_OFF"}
        if ev["channelId"] not in device["allowed"].get(ev["networkTypeId"], []):
            return {"resultCode": "INVALID_CATALOG"}
        if ev["deviceId"] in self.device_active:
            return {"resultCode": "DEVICE_BUSY"}
        self.lease_alloc += 1
        lid = self.lease_alloc
        self.leases[lid] = {"playerId": auth, "generation": live["generation"], "deviceId": ev["deviceId"],
                            "issuedAtMs": now, "ttlMs": ev["ttlMs"], "state": "ACTIVE", "reason": None,
                            "terminalAtMs": None}
        self.device_active[ev["deviceId"]] = lid
        effects.append({"type": "LEASE_GRANTED", "playerId": auth, "generation": live["generation"],
                        "deviceId": ev["deviceId"], "leaseId": lid, "issuedAtMs": now, "ttlMs": ev["ttlMs"]})
        return {"resultCode": "OK", "leaseId": lid, "leaseState": "ACTIVE"}

    def _stop(self, ev: dict[str, Any], auth: int, live: dict[str, Any], now: int, effects: list) -> dict[str, Any]:
        lid = ev["leaseId"]
        lease = self.leases.get(lid)
        if lease is None:
            return {"resultCode": "UNKNOWN_LEASE"}
        if lease["playerId"] != auth or lease["generation"] != live["generation"]:
            return {"resultCode": "STALE_LEASE"}
        if lease["state"] == "ACTIVE":
            self._terminate(lid, "REVOKED", "CLIENT_STOP", now, effects)
        return {"resultCode": "OK", "leaseId": lid, "leaseState": lease["state"], "terminalReason": lease["reason"]}

    # ------------------------------------------------------------------ epoch

    def _rotate(self, ev: dict[str, Any], now: int, effects: list) -> dict[str, Any]:
        epoch = ev.get("sessionEpoch")
        if not is_id(epoch) or epoch <= self.epoch:
            return {"resultCode": "MALFORMED", "effects": effects}
        for lid in self._active_lease_ids():
            self._terminate(lid, "REVOKED", "SESSION_ROTATED", now, effects)
        self.gen_alloc = 0
        for pid in sorted(self.players):
            player = self.players[pid]
            old = player["generation"]
            effects.append({"type": "SUBSCRIPTIONS_CLEARED", "playerId": pid, "generation": old})
            if player["entityId"] is not None:
                effects.append({"type": "ENTITY_UNBOUND", "playerId": pid, "generation": old,
                                "entityId": player["entityId"]})
            effects.append({"type": "CARRIED_DEVICES_DETACHED", "playerId": pid, "generation": old})
            self.gen_alloc += 1
            effects.append({"type": "PLAYER_STATE_CHANGED", "playerId": pid, "oldGeneration": old,
                            "newGeneration": self.gen_alloc, "fromState": player["state"],
                            "toState": "CONNECTED_UNREGISTERED"})
            effects.append({"type": "FULL_RESYNC_REQUIRED", "playerId": pid, "sessionEpoch": epoch,
                            "generation": self.gen_alloc})
            player.update({"generation": self.gen_alloc, "state": "CONNECTED_UNREGISTERED", "entityId": None,
                           "lastRequestSeq": 0, "cache": {}})
        self.tombstones, self.leases, self.device_active, self.rate = [], {}, {}, {}
        self.lease_alloc, self.epoch, self.last_now, self.latched = 0, epoch, now, False
        return {"resultCode": "OK", "effects": effects}


# ---------------------------------------------------------------------- runner

def check_vector_file(data: Any) -> list[str]:
    """Format checks of lifecycle_v1.json; returns problems (empty = valid)."""
    problems = []
    if not isinstance(data, dict) or data.get("schema") != "ricon.oracle.lifecycle" or data.get("version") != 1:
        return ["schema/version"]
    if not str(data.get("source", "")).startswith("docs/req/REQ-lifecycle.md@"):
        problems.append("source")
    traces = data.get("traces")
    if not isinstance(traces, list) or not traces:
        return problems + ["traces: non-empty list required"]
    seen = set()
    for trace in traces:
        if not isinstance(trace, dict) or not isinstance(trace.get("steps"), list) or not all(
                isinstance(step, dict) and isinstance(step.get("event"), dict) and isinstance(step.get("expect"), dict)
                and isinstance(step["expect"].get("effects", []), list) for step in trace["steps"]):
            problems.append("trace structure")
            continue
        tid = trace.get("id")
        if not isinstance(tid, str) or tid in seen:
            problems.append(f"trace id {tid!r}")
        seen.add(tid)
        if not trace.get("ct") or not trace.get("steps"):
            problems.append(f"{tid}: ct/steps")
        for step in trace.get("steps", []):
            expect = step.get("expect", {})
            if step.get("event", {}).get("type") not in EVENT_TYPES:
                problems.append(f"{tid}: event type")
            if expect.get("resultCode") not in RESULT_CODES:
                problems.append(f"{tid}: resultCode {expect.get('resultCode')!r}")
            for effect in expect.get("effects", []):
                if not isinstance(effect, dict) or effect.get("type") not in EFFECT_TYPES:
                    problems.append(f"{tid}: effect {effect.get('type') if isinstance(effect, dict) else effect!r}")
    return problems


def run_trace(trace: dict[str, Any]) -> list[str]:
    model = Lifecycle()
    failures = []
    for index, step in enumerate(trace["steps"]):
        actual = model.apply(step["event"])
        expect = dict(step["expect"])
        expect.setdefault("effects", [])
        if actual != expect:
            failures.append(f"{trace['id']} step {index} ({step['event'].get('type')}):\n"
                            f"  expected {json.dumps(expect, sort_keys=True)}\n"
                            f"  actual   {json.dumps(actual, sort_keys=True)}")
            break
    return failures


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", required=True, metavar="VECTORS_JSON")
    args = parser.parse_args(argv)
    data = json.loads(Path(args.check).read_text(encoding="utf-8"))
    problems = check_vector_file(data)
    if problems:
        print("FORMAT_FAIL:", *problems, sep="\n  ")
        return 1
    failures = [failure for trace in data["traces"] for failure in run_trace(trace)]
    for failure in failures:
        print("FAIL", failure)
    steps = sum(len(trace["steps"]) for trace in data["traces"])
    if failures:
        print(f"LIFECYCLE_ORACLE_FAIL: {len(failures)} of {len(data['traces'])} traces")
        return 1
    print(f"LIFECYCLE_ORACLE_PASS: {len(data['traces'])} traces, {steps} steps")
    return 0


if __name__ == "__main__":
    sys.exit(main())
