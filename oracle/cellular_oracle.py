#!/usr/bin/env python3
"""Host-эталон сотовой связи (спринт 22, фокус владельца agora #2020 п.2): SINR и хендовер. stdlib, не production.

- Мощность от каждой соты — `link_physics_oracle` (FSPL или P.526-16 §4.5 на ровном профиле, как L2 в RICON).
  Своей физики распространения здесь нет.
- SINR = S / (N + ΣI). Помехи — линейная сумма мощностей других сот на том же канале (REQ-LE-65). Шум —
  kT0B + NF и шум среды P.372-17; вне его области — опорный фон (REQ-LE-63).
- Хендовер — событие A3 по 3GPP TS 36.331 V19.4.0 §5.5.4.4 (`docs/evidence/E-web2-standards-cellular.md:129–146`):
  вход `Mn + Ofn + Ocn − Hys > Mp + Ofp + Ocp + Off`, условие держится TimeToTrigger.
  По стандарту A3 запускает только отчёт об измерениях; решение о хендовере принимает сеть, и стандарт его не
  задаёт. Здесь **допущение игровой модели**: хендовер выполняется сразу при срабатывании A3 (TEST_ONLY).
  Поэтому условие выхода из A3 в этой модели не используется.
- Измерение M — полная принимаемая мощность в дБм (упрощение: не RSRP на ресурсный элемент; при одинаковых
  полосе и мощности сот постоянный сдвиг сокращается в сравнении A3).
- Ping-pong (в загруженных текстах 3GPP термина нет, `E-web2:150`): определение этой модели — хендовер A→B, за которым
  следует B→A не позже `pingPongWindowS`.
- Флуктуации измерений: необязательная гауссова ошибка с seed (`random.Random`) — TEST_ONLY, не модель замираний.
"""

from __future__ import annotations

import argparse
import json
import math
import random
import re
import sys
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parent))  # own directory only
import link_physics_oracle as lpo  # noqa: E402

TTT_MS = (0, 40, 64, 80, 100, 128, 160, 256, 320, 480, 512, 640, 1024, 1280, 2560, 5120)  # TS 36.331 TimeToTrigger
PROPAGATION = ("FSPL", "P526_GENERAL")
SOURCE = "oracle/cellular_oracle.py"
STANDARDS = ["3GPP TS 36.331 V19.4.0 §5.5.4.4 (event A3, TimeToTrigger)", "Rec. ITU-R P.526-16 §4.5",
             "Rec. ITU-R P.372-17 eq (5), (17)"]


def a3_entering(mn: float, mp: float, cfg: dict[str, Any], ocn: float = 0.0, ocp: float = 0.0) -> bool:
    """TS 36.331 §5.5.4.4 entering condition, strict: Mn + Ofn + Ocn − Hys > Mp + Ofp + Ocp + Off."""
    return mn + cfg["ofnDb"] + ocn - cfg["hysDb"] > mp + cfg["ofpDb"] + ocp + cfg["offDb"]


def power_sum_dbm(powers_dbm: list[float]) -> float:
    """REQ-LE-65: powers add in linear scale (max-shifted for stability)."""
    top = max(powers_dbm)
    return top + 10.0 * math.log10(sum(10.0 ** ((p - top) / 10.0) for p in powers_dbm))


class LossTable:
    """Loss on a 1 m grid with linear interpolation: the trajectory needs tens of thousands of distances, the L2
    profile computation is slow. The interpolation error is checked by a test."""

    def __init__(self, propagation: str, f_mhz: float, h_bs: float, h_ms: float, ground: dict[str, Any],
                 terrain: dict[str, Any], max_m: int):
        self.step = 1.0
        self.values = []
        for i in range(max_m + 2):
            d = max(float(i), 1.0)
            if propagation == "FSPL":
                self.values.append(lpo.fspl_db(math.hypot(d, h_bs - h_ms), f_mhz))
            else:
                self.values.append(lpo.p526_general_db(d, h_bs, h_ms, f_mhz, ground, terrain))

    def __call__(self, d_m: float) -> float:
        d = max(d_m, 1.0)
        i = int(d)
        frac = d - i
        return self.values[i] * (1.0 - frac) + self.values[i + 1] * frac


def simulate(sc: dict[str, Any], table: LossTable | None = None) -> dict[str, Any]:
    cells, ms, radio, ho = sc["cells"], sc["mobile"], sc["radio"], sc["handover"]
    if table is None:
        table = build_table(sc)
    noise = lpo.system_noise_dbm(radio["bandwidthHz"], radio["rxNoiseFigureDb"], sc["noiseEnvironment"],
                                 sc["frequencyMHz"])
    n_dbm = noise["noiseDbm"] if noise["inDomain"] else \
        lpo.thermal_noise_dbm(radio["bandwidthHz"]) + radio["rxNoiseFigureDb"]               # REQ-LE-63 fallback
    rng = random.Random(sc["measurementErrorSeed"])
    dt = sc["timeStepMs"] / 1000.0
    steps = int(round(ms["durationS"] / dt))
    ttt_steps = ho["timeToTriggerMs"] // sc["timeStepMs"]
    serving, timers = None, [0] * len(cells)
    events: list[dict[str, Any]] = []
    good = 0
    sinr_min = math.inf
    for k in range(steps + 1):
        t = k * dt
        x = ms["startXM"] + ms["speedMps"] * t
        rx = [c["txPowerDbm"] + c["gainDbi"] - table(abs(x - c["xM"])) for c in cells]
        meas = [p + (rng.gauss(0.0, sc["measurementErrorDb"]) if sc["measurementErrorDb"] > 0 else 0.0) for p in rx]
        if serving is None:
            serving = max(range(len(cells)), key=lambda i: meas[i])
        else:
            best = None
            for i in range(len(cells)):
                if i == serving:
                    continue
                if a3_entering(meas[i], meas[serving], ho):
                    timers[i] += 1
                else:
                    timers[i] = 0
                if timers[i] > ttt_steps and (best is None or meas[i] > meas[best]):  # held through the whole TTT
                    best = i
            if best is not None:
                events.append({"timeS": round(t, 6), "xM": x, "from": cells[serving]["id"], "to": cells[best]["id"]})
                serving, timers = best, [0] * len(cells)
        same = [rx[i] for i in range(len(cells)) if i != serving and cells[i]["channel"] == cells[serving]["channel"]]
        total = power_sum_dbm([n_dbm] + same)
        sinr = rx[serving] - total
        sinr_min = min(sinr_min, sinr)
        good += sinr >= radio["sinrRequiredDb"]
    pp = sum(1 for a, b in zip(events, events[1:])
             if a["from"] == b["to"] and a["to"] == b["from"] and b["timeS"] - a["timeS"] <= ho["pingPongWindowS"])
    return {"handovers": events, "handoverCount": len(events), "pingPongCount": pp,
            "fractionSinrOk": good / (steps + 1), "minSinrDb": sinr_min, "noiseDbm": n_dbm,
            "noiseModelInDomain": noise["inDomain"]}


def build_table(sc: dict[str, Any]) -> LossTable:
    ms = sc["mobile"]
    span = max(abs(ms["startXM"] + s * ms["speedMps"] * ms["durationS"] - c["xM"]) for c in sc["cells"] for s in (0, 1))
    return LossTable(sc["propagation"], sc["frequencyMHz"], sc["cells"][0]["heightM"], ms["heightM"], sc["ground"],
                     sc["terrain"], int(math.ceil(span)) + 1)


# ---------------------------------------------------------------------- closed domain and vectors

def _num(v: Any, lo: float, hi: float) -> bool:
    return lpo._num(v, lo, hi)


def validate_scenario(sc: Any) -> str | None:
    keys = {"frequencyMHz", "propagation", "ground", "terrain", "noiseEnvironment", "radio", "cells", "mobile",
            "handover", "timeStepMs", "measurementErrorDb", "measurementErrorSeed"}
    if not lpo._keys(sc, keys):
        return "keys"
    if sc["propagation"] not in PROPAGATION or not _num(sc["frequencyMHz"], 30.0, 6000.0):
        return "propagation/frequency"
    if sc["noiseEnvironment"] not in lpo.NOISE_ENVIRONMENTS:
        return "noiseEnvironment"
    probe = {"frequencyMHz": sc["frequencyMHz"], "txHeightM": 10.0, "rxHeightM": 10.0, "ground": sc["ground"],
             "terrain": sc["terrain"], "radio": {"txPowerDbm": 0.0, "txGainDbi": 0.0, "rxGainDbi": 0.0,
                                                  "bandwidthHz": 1.0, "rxNoiseFigureDb": 0.0, "snrRequiredDb": 0.0},
             "noiseEnvironments": ["none"], "models": ["FSPL"], "grids": {"FSPL": {"minM": 1.0, "maxM": 2.0, "ratio": 2.0}},
             "sampleDistancesM": [1.0]}
    if lpo.validate_scenario(probe) is not None or sc["terrain"]["hill"] is not None:
        return "ground/terrain (flat only)"
    r = sc["radio"]
    if not lpo._keys(r, {"bandwidthHz", "rxNoiseFigureDb", "sinrRequiredDb"}) or not _num(r["bandwidthHz"], 1.0, 1e8) \
            or not _num(r["rxNoiseFigureDb"], 0.0, 40.0) or not _num(r["sinrRequiredDb"], -30.0, 60.0):
        return "radio"
    cells = sc["cells"]
    if not isinstance(cells, list) or not 2 <= len(cells) <= 16:
        return "cells"
    ids = set()
    for c in cells:
        if not lpo._keys(c, {"id", "xM", "heightM", "txPowerDbm", "gainDbi", "channel"}) \
                or not isinstance(c["id"], str) or not re.fullmatch(r"[A-Z][0-9]{1,2}", c["id"]) or c["id"] in ids \
                or not _num(c["xM"], -100000.0, 100000.0) or c["heightM"] != cells[0]["heightM"] \
                or not _num(c["heightM"], 1.0, 300.0) or not _num(c["txPowerDbm"], -30.0, 70.0) \
                or not _num(c["gainDbi"], -10.0, 30.0) or isinstance(c["channel"], bool) or not isinstance(c["channel"], int) \
                or not 0 <= c["channel"] <= 1000:
            return "cell"
        ids.add(c["id"])
    m = sc["mobile"]
    if not lpo._keys(m, {"startXM", "speedMps", "heightM", "durationS"}) or not _num(m["startXM"], -100000.0, 100000.0) \
            or not _num(m["speedMps"], 0.0, 100.0) or not _num(m["heightM"], 0.5, 50.0) or not _num(m["durationS"], 0.04, 3600.0):
        return "mobile"
    h = sc["handover"]
    if not lpo._keys(h, {"hysDb", "offDb", "ofnDb", "ofpDb", "timeToTriggerMs", "pingPongWindowS"}) \
            or not _num(h["hysDb"], 0.0, 15.0) or not _num(h["offDb"], -15.0, 15.0) or not _num(h["ofnDb"], -24.0, 24.0) \
            or not _num(h["ofpDb"], -24.0, 24.0) or h["timeToTriggerMs"] not in TTT_MS \
            or isinstance(h["timeToTriggerMs"], bool) or not _num(h["pingPongWindowS"], 0.0, 600.0):
        return "handover"
    if sc["timeStepMs"] not in (20, 40) or isinstance(sc["timeStepMs"], bool) or h["timeToTriggerMs"] % sc["timeStepMs"]:
        return "timeStepMs (TTT must be a whole number of steps)"
    if not _num(sc["measurementErrorDb"], 0.0, 20.0) or isinstance(sc["measurementErrorSeed"], bool) \
            or not isinstance(sc["measurementErrorSeed"], int) or not 0 <= sc["measurementErrorSeed"] < 2 ** 31:
        return "measurement error"
    return None


ROOT_KEYS = {"schema", "version", "revision", "source", "standards", "provenance", "note", "assumptions",
             "relTolerance", "scenarios"}
ASSUMPTIONS = ["handover executes when A3 has held for TimeToTrigger (the standard only triggers a report)",
               "measurement = total received power in dBm, not RSRP",
               "ping-pong = A->B then B->A within pingPongWindowS (no 3GPP definition)",
               "measurement error: seeded Gaussian, TEST_ONLY, not a fading model",
               "loss interpolated linearly on a 1 m grid"]


def check_vector_file(data: Any) -> list[str]:
    if not lpo._keys(data, ROOT_KEYS) or data["schema"] != "ricon.oracle.cellular_scenarios" \
            or type(data["version"]) is not int or data["version"] != 1 or type(data["revision"]) is not int \
            or data["revision"] < 1 or data["source"] != SOURCE or data["standards"] != STANDARDS \
            or data["provenance"] != "ORACLE_SNAPSHOT" or not isinstance(data["note"], str) or not data["note"] \
            or data["assumptions"] != ASSUMPTIONS or data["relTolerance"] != 1e-9 \
            or not isinstance(data["scenarios"], list) or not data["scenarios"]:
        return ["envelope"]
    ids = [s.get("id") if isinstance(s, dict) else None for s in data["scenarios"]]
    if len(set(map(str, ids))) != len(ids) or not all(isinstance(i, str) and lpo.SCENARIO_ID.fullmatch(i) for i in ids):
        return ["scenario ids"]
    errors: list[str] = []
    tables: dict[str, LossTable] = {}
    for s in data["scenarios"]:
        if not lpo._keys(s, {"id", "input", "expect"}):
            errors.append(f"{s['id']}: keys")
            continue
        reason = validate_scenario(s["input"])
        if reason is not None:
            errors.append(f"{s['id']}: input {reason}")
            continue
        key = table_key(s["input"])
        if key not in tables:
            tables[key] = build_table(s["input"])
        lpo._close(simulate(s["input"], tables[key]), s["expect"], data["relTolerance"], s["id"], errors)
    return errors


def table_key(sc: dict[str, Any]) -> str:
    """Scenarios that share geometry share one loss table (the span is included, so a longer drive rebuilds it)."""
    ms = sc["mobile"]
    span = max(abs(ms["startXM"] + s * ms["speedMps"] * ms["durationS"] - c["xM"]) for c in sc["cells"] for s in (0, 1))
    return json.dumps([sc["propagation"], sc["frequencyMHz"], sc["cells"][0]["heightM"], ms["heightM"], sc["ground"],
                       sc["terrain"], int(math.ceil(span)) + 1], sort_keys=True)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", type=Path, required=True)
    args = parser.parse_args(argv)
    errors = check_vector_file(json.loads(args.check.read_text(encoding="utf-8")))
    for e in errors[:50]:
        print(e)
    print("CELLULAR_ORACLE_PASS" if not errors else f"CELLULAR_ORACLE_FAIL ({len(errors)})")
    return 0 if not errors else 1


if __name__ == "__main__":
    sys.exit(main())
