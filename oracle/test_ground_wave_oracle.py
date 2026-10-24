"""Тесты эталона земной волны REQ-ground-wave v0.1 (порт NTIA/ITS LFMF v1.1)."""
import cmath
import copy
import json
import math
from pathlib import Path
import tempfile
import unittest
from unittest import mock

import build_ground_wave_vectors as builder
import ground_wave_oracle as g

VECTORS = Path(__file__).resolve().parent / "vectors" / "ground_wave_v1.json"


def airy_ai_series(z, derivative=False, terms=120):
    """Независимая проверка: ряд Маклорена Ai(z) = c1·f(z) − c2·g(z) (DLMF 9.4.1); только при |z| ≤ 4."""
    c1, c2 = 0.355028053887817239, 0.258819403792806798
    f = g_ = complex(0.0)
    tf, tg = complex(1.0), z
    df = dg = complex(0.0)
    for k in range(terms):
        f += tf
        g_ += tg
        if derivative:
            df += tf * 3 * k / z if k else 0
            dg += tg * (3 * k + 1) / z
        tf *= z ** 3 / ((3 * k + 2) * (3 * k + 3))
        tg *= z ** 3 / ((3 * k + 3) * (3 * k + 4))
    return c1 * df - c2 * dg if derivative else c1 * f - c2 * g_


def baseline():
    return builder.baseline()


class GroundWaveOracleTest(unittest.TestCase):
    def test_corpus_check_passes_without_writing(self):
        before = VECTORS.read_bytes()
        self.assertEqual(g.check(VECTORS), 0)
        self.assertEqual(VECTORS.read_bytes(), before)

    def test_builder_reproduces_corpus(self):
        text = json.dumps(builder.build(), ensure_ascii=False, indent=2, allow_nan=False) + "\n"
        self.assertEqual(VECTORS.read_text(encoding="utf-8"), text)

    def test_corrupted_corpus_rejected(self):
        data = json.loads(VECTORS.read_text(encoding="utf-8"))
        ok = next(i for i, c in enumerate(data["cases"]) if c["expect"]["code"] == "OK")
        refusal = next(i for i, c in enumerate(data["cases"]) if c["expect"]["code"] != "OK")
        mutations = [("basicLossDb", 2e-8), ("fieldDbuVm", -2e-8), ("method", None), ("provenance", None),
                     ("kr", 1e-9), ("code", None)]
        for field, delta in mutations:
            bad = copy.deepcopy(data)
            e = bad["cases"][ok]["expect"]
            if field == "method":
                e["method"] = "RESIDUE_SERIES" if e["method"] == "FLAT_EARTH_CURVE" else "FLAT_EARTH_CURVE"
            elif field == "provenance":
                e["provenance"] = "SOURCE_DOMAIN"
            elif field == "code":
                bad["cases"][refusal]["expect"]["code"] = "OK"
            elif field == "kr":
                e["kr"] *= 1 + delta
            else:
                e[field] += delta
            with tempfile.TemporaryDirectory() as tmp:
                path = Path(tmp) / "c.json"
                path.write_text(json.dumps(bad, ensure_ascii=False), encoding="utf-8")
                with mock.patch("builtins.print"):
                    self.assertEqual(g.check(path), 1, field)

    def test_ntia_example_corruption_rejected(self):
        data = json.loads(VECTORS.read_text(encoding="utf-8"))
        row = next(r for r in data["ntiaExamples"] if r["rc"] == 0)
        row["aBtlDb"] += 0.06
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "c.json"
            path.write_text(json.dumps(data, ensure_ascii=False), encoding="utf-8")
            with mock.patch("builtins.print"):
                self.assertEqual(g.check(path), 1)

    def test_req_sha_mismatch_rejected(self):
        data = json.loads(VECTORS.read_text(encoding="utf-8"))
        data["requirementsLfSha256"] = "0" * 64
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "c.json"
            path.write_text(json.dumps(data), encoding="utf-8")
            with self.assertRaises(ValueError):
                g.check(path)

    def test_duplicate_json_key_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "d.json"
            path.write_text('{"a": 1, "a": 2}', encoding="utf-8")
            with self.assertRaises(ValueError):
                g.read_json(path)

    def test_input_not_mutated_and_result_new(self):
        value = baseline()
        snapshot = copy.deepcopy(value)
        first, second = g.evaluate(value), g.evaluate(value)
        self.assertEqual(value, snapshot)
        self.assertEqual(first, second)
        self.assertIsNot(first, second)

    def test_heights_symmetric(self):
        a, b = baseline(), baseline()
        a["txHeightM"], a["rxHeightM"] = 10.0, 2.0
        b["txHeightM"], b["rxHeightM"] = 2.0, 10.0
        for d in (1.0, 50.0):
            a["distanceKm"] = b["distanceKm"] = d
            self.assertEqual(g.evaluate(a), g.evaluate(b))

    def test_invariant_constant(self):
        expected = (10 * math.log10(1000 * 10 ** 0.477) + 10 * math.log10(g.ETA * 4 * g.PI) + 240
                    - 20 * math.log10(g.C))
        self.assertAlmostEqual(g.LFMF_CONSTANT_DB, expected, delta=1e-12)
        self.assertGreater(abs(g.LFMF_CONSTANT_DB - 142.0), 0.01)  # NOTE 1 P.368 округляет до 142.0

    def test_unit_field_gives_free_space_loss(self):
        """GW06 (codex #4056): усиление опорного монополя сокращается — при нормированном поле 1 A = FSPL."""
        for f, d in ((30.0, 1.0), (20.0, 7.0), (0.5, 300.0)):
            with mock.patch.object(g, "flat_earth_curve_correction", return_value=1.0), \
                    mock.patch.object(g, "residue_series", return_value=1.0):
                rc, a, _e, _p, _m = g.lfmf(2, 2, f, 1000, 301, d, 15, 0.005, 1)
            self.assertEqual(rc, 0)
            self.assertAlmostEqual(a, 20 * math.log10(4 * g.PI * d * 1000 * f * 1e6 / g.C), delta=1e-12)

    def test_loss_independent_of_power(self):
        for p in (0.001, 1.0, 1000.0, 1e6):
            rc, a, e, _p, m = g.lfmf(2, 2, 25, p, 301, 20, 15, 0.005, 1)
            self.assertEqual(rc, 0)
            self.assertAlmostEqual(a, g.lfmf(2, 2, 25, 1.0, 301, 20, 15, 0.005, 1)[1], delta=1e-9)

    def test_wofz_against_closed_forms(self):
        self.assertEqual(g.wofz(0j), complex(1.0, 0.0))
        for y in (0.1, 0.5, 1.0, 3.0, 10.0):  # W(iy) = exp(y²)·erfc(y)
            self.assertAlmostEqual(g.wofz(complex(0, y)).real, math.exp(y * y) * math.erfc(y), delta=1e-13)
        for x in (0.3, 1.0, 2.5, 7.0):  # Re W(x) = exp(−x²)
            self.assertAlmostEqual(g.wofz(complex(x, 0)).real, math.exp(-x * x), delta=1e-15)

    def test_airy_w1_against_independent_series(self):
        """w1(z) = 2√π·e^{−iπ/6}·Ai(z·e^{−2πi/3}); w1'(z) = 2√π·e^{−5iπ/6}·Ai'(z·e^{−2πi/3}) (масштаб Wait)."""
        rot = cmath.exp(-2j * math.pi / 3)
        for z in (0.5 + 0.2j, -1 + 1j, 2 - 1.5j, -3 - 0.5j, 1 + 3j):
            arg = z * rot
            w = 2 * math.sqrt(math.pi) * cmath.exp(-1j * math.pi / 6) * airy_ai_series(arg)
            dw = 2 * math.sqrt(math.pi) * cmath.exp(-5j * math.pi / 6) * airy_ai_series(arg, derivative=True)
            self.assertLess(abs(g.airy_w1(z, False) - w) / abs(w), 1e-6, z)
            self.assertLess(abs(g.airy_w1(z, True) - dw) / abs(dw), 1e-6, z)

    def test_roots_satisfy_equation(self):
        for q in (0.05 - 0.3j, 2 - 2j, 30 - 10j):
            for i in (1, 2, 5, 11):
                t = g.wi_root(i, q)
                residual = g.airy_w1(t, True) - q * g.airy_w1(t, False)
                self.assertLess(abs(residual) / abs(q * g.airy_w1(t, False)), 1e-5, (q, i))

    def test_published_ntia_examples(self):
        data = json.loads(VECTORS.read_text(encoding="utf-8"))
        self.assertEqual(len(data["ntiaExamples"]), 95)
        for row in data["ntiaExamples"]:
            rc, a, e, p, m = g.lfmf(*[row[k] for k in ("hTxM", "hRxM", "fMHz", "pTxW", "nS", "dKm", "epsilon", "sigma", "pol")])
            self.assertEqual(rc, row["rc"])
            if rc == 0:
                self.assertEqual(m, row["method"])
                self.assertLessEqual(max(abs(a - row["aBtlDb"]), abs(e - row["eDbuVm"]), abs(p - row["pRxDbm"])), 0.05)

    def test_numeric_failure_paths(self):
        value = baseline()
        with mock.patch.object(g, "flat_earth_curve_correction", return_value=0.0):
            self.assertEqual(g.evaluate(value), {"code": "NUMERIC_FAILURE"})
        value["distanceKm"] = 100.0
        with mock.patch.object(g, "wi_root", side_effect=g.NumericFailure("нет сходимости")):
            self.assertEqual(g.evaluate(value), {"code": "NUMERIC_FAILURE"})
        with mock.patch.object(g, "residue_series", return_value=math.inf):
            self.assertEqual(g.evaluate(value), {"code": "NUMERIC_FAILURE"})

    def test_method_switches_at_threshold(self):
        value = baseline()
        d_test = 80 / math.cbrt(25.0)
        value["distanceKm"] = math.nextafter(d_test, 0.0)
        self.assertEqual(g.evaluate(value)["method"], "FLAT_EARTH_CURVE")
        value["distanceKm"] = d_test
        self.assertEqual(g.evaluate(value)["method"], "RESIDUE_SERIES")

    def test_domain_and_invalid_codes(self):
        cases = [({"polarization": "H"}, "OUT_OF_MODEL_DOMAIN"), ({"frequencyMHz": 30.5}, "OUT_OF_MODEL_DOMAIN"),
                 ({"frequencyMHz": 0.0}, "INVALID_INPUT"), ({"txHeightM": 51.0}, "OUT_OF_MODEL_DOMAIN"),
                 ({"frequencyMHz": float("nan")}, "INVALID_INPUT"), ({"distanceKm": float("inf")}, "INVALID_INPUT")]
        for change, code in cases:
            value = baseline()
            value.update(change)
            self.assertEqual(g.evaluate(value), {"code": code}, change)


if __name__ == "__main__":
    unittest.main()
