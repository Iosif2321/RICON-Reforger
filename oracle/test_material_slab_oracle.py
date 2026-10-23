"""P3.2a: физические инварианты и численные края, без геометрии игры."""
import cmath
import copy
import math
import random
import json
from pathlib import Path
import tempfile
import unittest

import material_slab_oracle as o


def inputs(**changes):
    value = o.baseline()
    value.update(changes)
    return value


def direct_transfer(value):
    """Неперегруппированная формула источника для умеренных чисел."""
    f, d, c = (o.f32(value[k]) for k in ("frequencyMHz", "thicknessM", "cosIncidence"))
    material = value["material"]
    e = complex(o.f32(material["epsilonReal"]), -17.98 * o.f32(material["conductivitySm"]) / (f / 1000))
    p = cmath.sqrt(e - 1 + c * c)
    a = c if value["polarization"] == "TE" else e * c
    r = (a - p) / (a + p)
    q = 2 * math.pi * f * 1e6 / o.C * d * p
    z = cmath.exp(-2j * q)
    t = (1 - r * r) * cmath.exp(-1j * q) / (1 - r * r * z)
    reflection = r * (1 - z) / (1 - r * r * z)
    return t, reflection


class SlabTests(unittest.TestCase):
    def test_identity_and_normal_polarizations(self):
        for angle in (1, .5, o.MIN_COSINE):
            for pol in ("TE", "TM"):
                v = inputs(cosIncidence=angle, polarization=pol, thicknessM=0)
                self.assertEqual(0, o.evaluate(v)["lossDb"])
                v["thicknessM"] = 1
                v["material"]["epsilonReal"] = 1
                self.assertEqual(0, o.evaluate(v)["lossDb"])
        v = inputs()
        a = o.evaluate(v)
        v["polarization"] = "TM"
        self.assertEqual(a, o.evaluate(v))

    def test_quarter_and_half_wave(self):
        for factor, expected in ((4, -20 * math.log10(.8)), (2, 0)):
            v = inputs(thicknessM=o.C / (100e6 * 2 * factor))
            self.assertAlmostEqual(expected, o.evaluate(v)["lossDb"], delta=1e-10)

    def test_brewster(self):
        v = inputs(cosIncidence=1 / math.sqrt(5), polarization="TM")
        self.assertAlmostEqual(0, o.evaluate(v)["lossDb"], delta=1e-10)
        v["polarization"] = "TE"
        self.assertGreater(o.evaluate(v)["lossDb"], .1)

    def test_passivity_energy_and_direct_formula(self):
        rng = random.Random(2040)
        checked = 0
        for _ in range(500):
            v = inputs(frequencyMHz=rng.uniform(20, 3000), thicknessM=rng.uniform(.00001, .3),
                       cosIncidence=rng.uniform(.05, 1), polarization=rng.choice(("TE", "TM")))
            v["material"]["epsilonReal"] = rng.uniform(1, 100)
            v["material"]["conductivitySm"] = rng.choice((0, rng.uniform(0, 1)))
            result = o.evaluate(v)
            if result["code"] == "NUMERIC_DOMAIN_LIMIT":
                continue
            self.assertEqual("OK", result["code"])
            checked += 1
            transfer, reflection = direct_transfer(v)
            self.assertLessEqual(abs(transfer) ** 2 + abs(reflection) ** 2, 1 + 1e-12)
            if v["material"]["conductivitySm"] == 0:
                self.assertAlmostEqual(1, abs(transfer) ** 2 + abs(reflection) ** 2, delta=1e-12)
            expected = -20 * math.log10(abs(transfer))
            self.assertAlmostEqual(expected, result["lossDb"], delta=1e-8)
        self.assertGreater(checked, 300)

    def test_thin_conductor(self):
        v = inputs(frequencyMHz=30, thicknessM=1e-12)
        v["material"].update(epsilonReal=1, conductivitySm=1e8)
        result = o.evaluate(v)
        impedance = 2 * math.pi * 17.98e9 / o.C
        sheet_loss = 20 * math.log10(1 + impedance * o.f32(1e8) * o.f32(1e-12) / 2)
        self.assertAlmostEqual(sheet_loss, result["lossDb"], delta=1e-8)
        self.assertGreater(result["lossDb"], 0)

    def test_opaque_logarithm_and_no_false_zero(self):
        v = inputs(frequencyMHz=1000, thicknessM=.1)
        v["material"].update(epsilonReal=1, conductivitySm=1e7)
        result = o.evaluate(v)
        self.assertEqual("OK", result["code"])
        self.assertGreater(result["lossDb"], 100000)
        self.assertTrue(math.isfinite(result["lossDb"]))

    def test_invalid_inputs_profiles_and_precedence(self):
        for field in ("frequencyMHz", "thicknessM", "cosIncidence"):
            for bad in (True, None, "1", math.inf, math.nan, 10 ** 1000):
                self.assertEqual("INVALID_INPUT", o.evaluate(inputs(**{field: bad}))["code"])
        for field in ("epsilonReal", "conductivitySm", "minMHz", "maxMHz"):
            for bad in (True, None, "1", math.inf, math.nan, 10 ** 1000):
                v = inputs()
                v["material"][field] = bad
                self.assertEqual("INVALID_PROFILE", o.evaluate(v)["code"])
        self.assertEqual("INVALID_INPUT", o.evaluate({})["code"])
        self.assertEqual("INVALID_PROFILE", o.evaluate(inputs(material=None))["code"])
        v = inputs(frequencyMHz=.3)
        v["material"]["minMHz"] = -1
        self.assertEqual("INVALID_PROFILE", o.evaluate(v)["code"])

    def test_domain_provenance_and_no_mutation(self):
        for status in o.PROVENANCE:
            v = inputs()
            v["material"]["provenance"] = status
            before = copy.deepcopy(v)
            self.assertEqual(status, o.evaluate(v)["provenance"])
            self.assertEqual(before, v)
        for f in (.03, .3, 450001):
            self.assertEqual("OUT_OF_MODEL_DOMAIN", o.evaluate(inputs(frequencyMHz=f))["code"])
        for change in ({"cosIncidence": 0}, {"thicknessM": 1001}, {"thicknessM": 100}):
            self.assertEqual("NUMERIC_DOMAIN_LIMIT", o.evaluate(inputs(**change))["code"])
        v = inputs()
        v["material"].update(minMHz=100, maxMHz=100)
        self.assertEqual("OK", o.evaluate(v)["code"])

    def test_opaque_boundary_continuity(self):
        p = cmath.sqrt(complex(4, -17.98 / .1))
        center = 12 / (-p.imag * 2 * math.pi * 100e6 / o.C)
        for scale in (.999999, 1, 1.000001):
            v = inputs(thicknessM=center * scale)
            v["material"]["conductivitySm"] = 1
            transfer, _ = direct_transfer(v)
            self.assertAlmostEqual(-20 * math.log10(abs(transfer)), o.evaluate(v)["lossDb"], delta=1e-8)

    def test_corpus_integrity(self):
        path = Path(__file__).with_name("vectors") / "material_slab_v1.json"
        self.assertEqual(0, o.check(path))
        source = json.loads(path.read_text(encoding="utf-8"))
        with tempfile.TemporaryDirectory(prefix="ricon-slab-check-") as directory:
            candidate = Path(directory) / "bad.json"
            for field, bad in (("version", True), ("requirementsLfSha256", "0" * 64)):
                value = copy.deepcopy(source)
                value[field] = bad
                candidate.write_text(json.dumps(value), encoding="utf-8")
                with self.assertRaises(ValueError):
                    o.check(candidate)
            source["cases"][0]["expect"]["lossDb"] = True
            candidate.write_text(json.dumps(source), encoding="utf-8")
            with self.assertRaises(ValueError):
                o.check(candidate)


if __name__ == "__main__":
    unittest.main()
