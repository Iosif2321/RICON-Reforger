#!/usr/bin/env python3
"""Tests of the cellular oracle: A3 by the text of TS 36.331 §5.5.4.4, handover geometry anchors, interference sum,
closed input domain and the committed scenario vectors."""

from __future__ import annotations

import copy
import json
import math
import random
import unittest
from pathlib import Path

import cellular_oracle as co
import link_physics_oracle as lpo

VECTORS = Path(__file__).resolve().parent / "vectors" / "cellular_scenarios_v1.json"
GROUND = {"permittivity": 15.0, "conductivitySm": 0.005, "polarization": "VERTICAL"}
FLAT = {"earthRadiusKm": 6371.0, "kFactor": 4 / 3, "profileStepKm": 0.1, "hill": None}
A3 = {"hysDb": 0.0, "offDb": 0.0, "ofnDb": 0.0, "ofpDb": 0.0, "timeToTriggerMs": 0, "pingPongWindowS": 5.0}


def road(channels=(1, 1, 1), **over) -> dict:
    sc = {"frequencyMHz": 900.0, "propagation": "P526_GENERAL", "ground": dict(GROUND), "terrain": dict(FLAT),
          "noiseEnvironment": "rural", "radio": {"bandwidthHz": 200000.0, "rxNoiseFigureDb": 7.0, "sinrRequiredDb": 0.0},
          "cells": [{"id": f"{n}1", "xM": x, "heightM": 30.0, "txPowerDbm": 43.0, "gainDbi": 15.0, "channel": ch}
                    for n, x, ch in zip("ABC", (0.0, 5000.0, 10000.0), channels)],
          "mobile": {"startXM": -500.0, "speedMps": 15.0, "heightM": 1.5, "durationS": 700.0},
          "handover": dict(A3), "timeStepMs": 40, "measurementErrorDb": 0.0, "measurementErrorSeed": 1}
    for key, value in over.items():
        if key in A3:
            sc["handover"][key] = value
        else:
            sc[key] = value
    return sc


class Shared:
    table = None

    @classmethod
    def get(cls):
        if cls.table is None:
            cls.table = co.build_table(road())
        return cls.table


class A3Test(unittest.TestCase):
    def test_entering_condition_is_strict(self) -> None:
        cfg = dict(A3, hysDb=3.0, offDb=1.0)
        self.assertFalse(co.a3_entering(-80.0, -84.0, cfg))         # −80 − 3 = −83 is not > −84 + 1 = −83
        self.assertTrue(co.a3_entering(-79.999, -84.0, cfg))
        self.assertTrue(co.a3_entering(-80.0, -84.0, cfg, ocn=0.5))  # cell individual offset of the neighbour
        self.assertFalse(co.a3_entering(-80.0, -84.0, cfg, ocp=0.5))

    def test_ttt_values_are_the_standard_set(self) -> None:
        self.assertEqual(co.TTT_MS[0], 0)
        self.assertEqual(co.TTT_MS[-1], 5120)
        self.assertEqual(len(co.TTT_MS), 16)

    def test_ttt_window_and_reset(self) -> None:
        """agora #2308: the trigger needs ttt_steps + 1 consecutive samples with the condition; one false sample
        resets the window of that neighbour only."""
        def run(sequence, ttt_steps=3):
            timers, fired = [0, 0, 0], []
            for k, meas in enumerate(sequence):
                hit = co.a3_step(timers, meas, 0, A3, ttt_steps)
                if hit is not None:
                    fired.append((k, hit))
            return fired

        good, bad = [-90.0, -85.0, -100.0], [-90.0, -95.0, -100.0]
        self.assertEqual(run([good] * 4), [(3, 1)])                       # 4 samples = TTT of 3 steps
        self.assertEqual(run([good] * 3), [])
        self.assertEqual(run([good, good, good, bad, good, good, good, good]), [(7, 1)])  # reset at sample 3
        self.assertEqual(run([good] * 4, ttt_steps=0)[:1], [(0, 1)])      # TTT 0: the first true sample

    def test_ttt_per_neighbour_and_target_change(self) -> None:
        """Each neighbour has its own window: a target that keeps changing never fills a window."""
        timers = [0, 0, 0]
        n1, n2 = [-90.0, -85.0, -95.0], [-90.0, -95.0, -85.0]
        hits = [co.a3_step(timers, m, 0, A3, 2) for m in (n1, n2, n1, n2, n1, n2)]
        self.assertEqual(hits, [None] * 6)
        both = [-90.0, -85.0, -84.0]
        timers = [0, 0, 0]
        hits = [co.a3_step(timers, both, 0, A3, 2) for _ in range(3)]
        self.assertEqual(hits, [None, None, 2])                           # the strongest of the triggered ones

    def test_power_sum(self) -> None:
        self.assertAlmostEqual(co.power_sum_dbm([-90.0, -90.0]), -90.0 + 10 * math.log10(2), places=12)
        self.assertAlmostEqual(co.power_sum_dbm([-90.0]), -90.0, places=12)
        self.assertAlmostEqual(co.power_sum_dbm([-60.0, -200.0]), -60.0, places=9)


class HandoverGeometryTest(unittest.TestCase):
    def test_loss_table_interpolation(self) -> None:
        table, rng, worst = Shared.get(), random.Random(7), 0.0
        for _ in range(100):
            d = rng.uniform(1.0, 10000.0)
            worst = max(worst, abs(table(d) - lpo.p526_general_db(d, 30.0, 1.5, 900.0, GROUND, FLAT)))
        self.assertLess(worst, 0.005)

    def test_symmetric_midpoint(self) -> None:
        """Equal cells, no error, Hys 0, TTT 0: the first handover is the first sample past the midpoint."""
        r = co.simulate(road(), Shared.get())
        self.assertEqual([(e["from"], e["to"]) for e in r["handovers"]], [("A1", "B1"), ("B1", "C1")])
        step = 15.0 * 0.04
        self.assertTrue(2500.0 < r["handovers"][0]["xM"] <= 2500.0 + step)
        self.assertTrue(7500.0 < r["handovers"][1]["xM"] <= 7500.0 + step)
        self.assertEqual(r["pingPongCount"], 0)

    def test_time_to_trigger_delays_by_v_ttt(self) -> None:
        base = co.simulate(road(), Shared.get())["handovers"][0]["xM"]
        for ttt in (320, 1280):
            late = co.simulate(road(timeToTriggerMs=ttt), Shared.get())["handovers"][0]["xM"]
            self.assertAlmostEqual(late - base, 15.0 * ttt / 1000.0, delta=15.0 * 0.04 + 1e-9)

    def test_hysteresis_position(self) -> None:
        """With Hys the handover is the first sample where M_B − M_A > Hys (no offsets)."""
        table = Shared.get()
        r = co.simulate(road(hysDb=3.0), table)
        x = r["handovers"][0]["xM"]

        def margin(xm: float) -> float:
            return table(abs(xm - 0.0)) - table(abs(xm - 5000.0))  # loss_A − loss_B = M_B − M_A

        self.assertGreater(margin(x), 3.0)
        self.assertLessEqual(margin(x - 15.0 * 0.04), 3.0)
        self.assertGreater(x, 2500.0)

    def test_interference(self) -> None:
        """Reuse 1: near the midpoint the neighbour is as strong as the server, SINR just below 0 dB.
        Reuse 3 (three channels): no co-channel interference, SINR = SNR."""
        one = co.simulate(road(), Shared.get())
        three = co.simulate(road(channels=(1, 2, 3)), Shared.get())
        self.assertTrue(-1.0 < one["minSinrDb"] <= 0.0)
        self.assertGreater(three["minSinrDb"], one["minSinrDb"] + 10.0)
        self.assertEqual(three["fractionSinrOk"], 1.0)

    def test_noise_out_of_domain_at_900_mhz(self) -> None:
        """D-005 again: P.372 man-made noise is defined to 250 MHz; at 900 MHz the reference kT0B + NF is used."""
        r = co.simulate(road(durationS=0.04), Shared.get())
        self.assertFalse(r["noiseModelInDomain"])
        self.assertAlmostEqual(r["noiseDbm"], lpo.thermal_noise_dbm(200000.0) + 7.0, places=12)

    def test_ping_pong_and_its_suppression(self) -> None:
        """Characterisation with a fixed seed: 4 dB i.i.d. measurement error every 40 ms. Without Hys and TTT almost
        every handover is a ping-pong; Hys 3 dB with TTT 320 ms leaves the two geometric handovers."""
        noisy = co.simulate(road(measurementErrorDb=4.0), Shared.get())
        self.assertGreater(noisy["pingPongCount"], 1000)
        calm = co.simulate(road(measurementErrorDb=4.0, hysDb=3.0, timeToTriggerMs=320), Shared.get())
        self.assertEqual((calm["handoverCount"], calm["pingPongCount"]), (2, 0))

    def test_long_ttt_costs_sinr(self) -> None:
        """A late handover keeps the mobile on the weaker cell: the time with SINR ≥ 0 dB falls."""
        f = [co.simulate(road(timeToTriggerMs=t, hysDb=3.0), Shared.get())["fractionSinrOk"] for t in (0, 320, 1280)]
        self.assertGreaterEqual(f[0], f[1])
        self.assertGreater(f[1], f[2])


class DomainTest(unittest.TestCase):
    def test_valid(self) -> None:
        self.assertIsNone(co.validate_scenario(road()))

    def test_rejections(self) -> None:
        mutations = [
            lambda s: s["handover"].update(timeToTriggerMs=300),          # not in the 36.331 set
            lambda s: s.update(timeStepMs=40) or s["handover"].update(timeToTriggerMs=100),  # not whole steps
            lambda s: s["handover"].update(timeToTriggerMs=True),
            lambda s: s["terrain"].update(hill={"centerKm": 1.0, "heightM": 10.0, "halfWidthKm": 0.1}),
            lambda s: s["cells"][1].update(id="A1"),
            lambda s: s["cells"][1].update(channel=True),
            lambda s: s["cells"][1].update(heightM=40.0),                # one loss table needs equal heights
            lambda s: s.update(cells=s["cells"][:1]),
            lambda s: s.update(propagation="TWO_RAY_FLAT"),
            lambda s: s.update(extra=1),
            lambda s: s["mobile"].update(speedMps=float("nan")),
            lambda s: s.update(measurementErrorSeed=-1),
            lambda s: s.update(noiseEnvironment="quiet_rural"),
        ]
        for mutate in mutations:
            sc = road()
            mutate(sc)
            self.assertIsNotNone(co.validate_scenario(sc))


class VectorFileTest(unittest.TestCase):
    def setUp(self) -> None:
        self.data = json.loads(VECTORS.read_text(encoding="utf-8"))

    def test_vectors_pass(self) -> None:
        self.assertEqual(co.check_vector_file(self.data), [])
        self.assertEqual(len(self.data["scenarios"]), 48)

    def test_corruption_and_envelope(self) -> None:
        bad = copy.deepcopy(self.data)
        bad["scenarios"][0]["expect"]["handovers"][0]["xM"] += 0.01
        self.assertNotEqual(co.check_vector_file(bad), [])
        bad = copy.deepcopy(self.data)
        bad["scenarios"][0]["expect"]["pingPongCount"] = True
        self.assertNotEqual(co.check_vector_file(bad), [])
        for key, value in (("relTolerance", 1.0), ("assumptions", []), ("standards", ["TS 36.331"]),
                           ("version", True), ("scenarios", []), ("extra", 1)):
            bad = copy.deepcopy(self.data)
            bad[key] = value
            self.assertEqual(co.check_vector_file(bad), ["envelope"], key)


if __name__ == "__main__":
    unittest.main()
