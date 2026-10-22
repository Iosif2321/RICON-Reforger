"""P1b.1: аналитика и свойства канала; физическая сверка не использует нормативный полином."""
import cmath
import copy
import math
import random
import unittest

import channel_fading_oracle as oracle
from build_channel_fading_vectors import burst_input, component


def physical_sum(data):
    """Независимая binary64 сумма: f32 входы, но не f32 полином/нормировщик эталона."""
    d, q = data["descriptor"], data["query"]
    weights = [oracle.f32(c["powerWeight"]) for c in d["components"]]
    total = math.fsum(weights)
    delta = (q["nowMs"] - d["anchorMs"]) * 1000 + q["subUs"]
    return sum(math.sqrt(w / total) * cmath.exp(2j * math.pi * ((c["phaseQ"] + c["dopplerQ"] * delta) % oracle.M) / oracle.M)
               for c, w in zip(d["components"], weights))


class ChannelFadingTest(unittest.TestCase):
    def test_single_component_and_cardinals(self):
        for i, expected in enumerate(((1, 0), (0, 1), (-1, 0), (0, -1))):
            data = oracle.baseline()
            data["descriptor"]["components"][0]["phaseQ"] = i * oracle.QUARTER
            result = oracle.evaluate(data)
            self.assertEqual((result["real"], result["imag"]), expected)
            self.assertEqual(result["gainDb"], 0)

    def test_analytic_interference_and_recovery(self):
        data = oracle.baseline()
        data["descriptor"]["components"] = [component("a"), component("b")]
        self.assertAlmostEqual(oracle.evaluate(data)["gainDb"], 10 * math.log10(2), places=5)
        data["descriptor"]["components"][1].update(phaseQ=oracle.M // 2)
        self.assertEqual(oracle.evaluate(data)["code"], "ZERO_AT_SAMPLE")
        data["descriptor"]["staticChannel"] = False
        data["descriptor"]["components"][1]["dopplerQ"] = 1074
        data["query"]["nowMs"] += 250
        self.assertEqual(oracle.evaluate(data)["code"], "OK")

    def test_near_antiphase_one_quantum_is_not_erased(self):
        data = oracle.baseline()
        data["descriptor"]["components"] = [component("a"), component("b", phase=oracle.M // 2 + 1)]
        result = oracle.evaluate(data)
        self.assertEqual(result["code"], "OK")
        self.assertNotEqual(result["imag"], 0)
        self.assertAlmostEqual(result["gainDb"], 20 * math.log10(abs(physical_sum(data))), delta=0.001)

    def test_weak_component_survives_strong_cancellation(self):
        data = oracle.baseline()
        data["descriptor"]["components"] = [component("a"), component("b", phase=oracle.M // 2), component("c", weight=1e-24)]
        result = oracle.evaluate(data)
        self.assertEqual(result["code"], "OK")
        self.assertTrue(-245 < result["gainDb"] < -240)

    def test_static_channel_and_call_order(self):
        data = oracle.baseline()
        expected = oracle.evaluate(data)
        for time in (4999, 1000, 2500, 1001, 4900, 1000):
            data["query"]["nowMs"] = time
            self.assertEqual(oracle.evaluate(data), expected)
        data["descriptor"]["components"][0]["dopplerQ"] = 1
        self.assertEqual(oracle.evaluate(data)["code"], "INVALID_INPUT")

    def test_reciprocal_pair_and_independent_outputs(self):
        data = burst_input()
        saved = copy.deepcopy(data)
        expected = oracle.evaluate(data)
        result = oracle.evaluate(data)
        result["phasesQ"][0] += 1
        self.assertEqual(data, saved)
        self.assertEqual(oracle.evaluate(data), expected)
        data["query"]["tx"], data["query"]["rx"] = data["query"]["rx"], data["query"]["tx"]
        self.assertEqual(oracle.evaluate(data), expected)

    def test_clock_and_signed_phase_use_exact_integer_math(self):
        c = component("a", phase=oracle.M - 1, doppler=-oracle.QUARTER + 3)
        for time in (0, 1, 2147483646, 2147483647):
            for sub in (0, 1, 999):
                self.assertEqual(oracle.phase_at(c, time, sub), (c["phaseQ"] + c["dopplerQ"] * (1000 * time + sub)) % oracle.M)

    def test_native_modular_formula_against_big_integer(self):
        mask, modulus = (1 << 15) - 1, oracle.M
        rng = random.Random(20261023)
        for a, b in [(modulus - 1, modulus - 1), (0, modulus - 1)] + [(rng.randrange(modulus), rng.randrange(modulus)) for _ in range(10000)]:
            low = (a & mask) * (b & mask)
            cross_sum = (a & mask) * (b >> 15) + (a >> 15) * (b & mask)
            total = low + ((cross_sum & mask) << 15)
            self.assertLessEqual(max(cross_sum, total), oracle.MAX)
            self.assertEqual(total & (modulus - 1), a * b % modulus)

    def test_polynomial_against_independent_trigonometry(self):
        rng = random.Random(31)
        phases = [0, 1, oracle.M - 1, oracle.QUARTER - 1, oracle.QUARTER, oracle.QUARTER + 1]
        phases += [rng.randrange(oracle.M) for _ in range(10000)]
        for phase in phases:
            actual = oracle.phasor(phase)
            reference = cmath.exp(2j * math.pi * phase / oracle.M)
            self.assertLess(max(abs(actual[0] - reference.real), abs(actual[1] - reference.imag)), 3e-7)

    def test_complete_sum_against_physical_complex_reference(self):
        rng = random.Random(10)
        data = oracle.baseline()
        for _ in range(128):
            data["descriptor"]["components"] = [component(f"p{i:02}", weight=oracle.f32(rng.random()), phase=rng.randrange(oracle.M)) for i in range(16)]
            result = oracle.evaluate(data)
            physical = physical_sum(data)
            self.assertEqual(result["code"], "OK")
            self.assertLess(abs(complex(result["real"], result["imag"]) - physical), 4e-6)

    def test_short_irregular_windows_between_slow_queries(self):
        data = burst_input()
        hits = []
        for time in range(0, 30001, 10):
            data["query"]["nowMs"] = time
            result = oracle.evaluate(data)
            # TEST_ONLY: среднее −124 dBm, порог условного приёмника −118.5 dBm; не настройка игры.
            hits.append(result["code"] == "OK" and result["gainDb"] > 5.5)
        self.assertTrue(1 < sum(hits) < len(hits) * 0.15)
        starts = [i for i, hit in enumerate(hits) if hit and (i == 0 or not hits[i - 1])]
        self.assertGreater(len(starts), 3)
        self.assertGreater(len({b - a for a, b in zip(starts, starts[1:])}), 1)
        self.assertTrue(any(hits[i] and not hits[i // 100 * 100] and not hits[min((i // 100 + 1) * 100, len(hits) - 1)]
                            for i in range(len(hits))))

    def test_zero_weights_and_no_forced_good_interval(self):
        data = oracle.baseline()
        data["descriptor"]["components"][0]["powerWeight"] = 0
        self.assertEqual(oracle.evaluate(data)["code"], "EMPTY_MEDIUM")
        data = oracle.baseline()
        for time in range(1000, 5000, 100):
            data["query"]["nowMs"] = time
            self.assertLess(-150 + oracle.evaluate(data)["gainDb"], -120)

    def test_validation_priority_and_profiles(self):
        data = oracle.baseline()
        data["descriptor"]["resourceExhausted"] = True
        data["descriptor"]["components"].append(component("z", weight=-1))
        self.assertEqual(oracle.evaluate(data)["code"], "INVALID_INPUT")
        data = oracle.baseline()
        data["descriptor"]["profileVersion"] += 1
        self.assertEqual(oracle.evaluate(data)["code"], "CONTEXT_MISMATCH")
        data["descriptor"]["profileVersion"] -= 1
        data["query"]["nowMs"] = 5000
        self.assertEqual(oracle.evaluate(data)["code"], "STALE_DESCRIPTOR")

    def test_component_count_boundary_and_subnormal_weight(self):
        data = oracle.baseline()
        data["descriptor"]["components"] = [component(f"p{i:02}") for i in range(64)]
        self.assertAlmostEqual(oracle.evaluate(data)["gainDb"], 10 * math.log10(64), places=6)
        data["descriptor"]["components"].append(component("p64"))
        self.assertEqual(oracle.evaluate(data)["code"], "INVALID_INPUT")
        data["descriptor"]["components"] = [component("small", weight=2**-149)]
        self.assertEqual(oracle.evaluate(data)["gainDb"], 0)

    def test_conditional_rayleigh_and_rician_reference_moments(self):
        # TEST_ONLY ансамбль независимых равномерных фаз; не утверждение о конкретной карте/рации.
        rng = random.Random(20261023)
        data = oracle.baseline()
        for k_factor in (0, 4):
            powers = []
            diffuse_count = 31
            weights = [k_factor / (1 + k_factor)] + [1 / ((1 + k_factor) * diffuse_count)] * diffuse_count
            for _ in range(2048):
                data["descriptor"]["components"] = [component(f"p{i:02}", weight=w, phase=rng.randrange(oracle.M))
                                                     for i, w in enumerate(weights)]
                result = oracle.evaluate(data)
                self.assertEqual(result["code"], "OK")
                powers.append(result["real"]**2 + result["imag"]**2)
            mean = math.fsum(powers) / len(powers)
            variance = math.fsum((p - mean)**2 for p in powers) / len(powers)
            # Теоретические моменты нормированного Rayleigh/Rice и точная finite-N поправка дисперсии.
            classical_variance = (1 + 2 * k_factor) / (1 + k_factor)**2
            finite_variance = classical_variance - 1 / (diffuse_count * (1 + k_factor)**2)
            self.assertAlmostEqual(mean, 1, delta=0.08)
            self.assertAlmostEqual(variance, classical_variance, delta=0.13)
            self.assertAlmostEqual(variance, finite_variance, delta=0.13)
            if k_factor == 0:
                for threshold in (0.1, 0.5, 1, 2, 4):
                    empirical = sum(p <= threshold for p in powers) / len(powers)
                    self.assertAlmostEqual(empirical, 1 - math.exp(-threshold), delta=0.05)


if __name__ == "__main__":
    unittest.main()
