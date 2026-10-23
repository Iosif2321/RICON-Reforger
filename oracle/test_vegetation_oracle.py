"""P4 oracle tests (claude): source-anchored model checks, frequency-scenario matrix and corpus integrity (v0.4)."""
import copy
import json
import math
from pathlib import Path
import unittest

import build_vegetation_vectors as b
import vegetation_oracle as v

VECTORS = Path(__file__).resolve().parent / "vectors" / "vegetation_v1.json"
MED_OK = v.typed_profile(b.profile("TEMPERATE", b.FIG2, "STANDARD_FIGURE"))
TROP = v.typed_profile(b.TROPICAL)
SPB = v.typed_profile(b.TEMPERATE)

# REQ VG10a: which models are applicable per band and scenario (polarization V, heights 3/3 m).
# Temperate profile: dense dry in-leaf, gamma from P.833 Fig. 2, Am of P.833 St. Petersburg; tropical: Fig. 2 gamma, no Am.
MATRIX = {
    30.0: "M5 OPEN M5 OPEN M2 | M5 OPEN M4+M5 M4 M2",
    60.0: "M5 OPEN M5 OPEN M2 | M5 OPEN M4+M5 M4 M2",
    88.0: "M5 OPEN M5 OPEN M2 | M5 OPEN M4+M5 M4 M2",
    150.0: "M1+M5 M1 M5 OPEN M2 | M5 OPEN M4+M5 M4 M2",
    230.0: "M3+M1+M5 M1 M3+M5 OPEN M3+M2 | M5 OPEN M4+M5 M4 M2",
    466.475: "M3+M1+M5 M1 M3+M5 OPEN M3+M2 | M5 OPEN M5 OPEN M2",
    1500.0: "M3+M1+M5 M1 M3+M5 OPEN M3 | M5 OPEN M5 OPEN OPEN",
    3000.0: "M3+M5 OPEN M3+M5 OPEN M3 | M5 OPEN M5 OPEN OPEN",
    30000.0: "M3+M5 OPEN M3+M5 OPEN M3 | M5 OPEN M5 OPEN OPEN",
}
CELLS = (("S1", 100.0), ("S1", 600.0), ("S2", 300.0), ("S2", 800.0), ("S3", 20.0))
# VG10a: closed cells (primary inside its source domain, S1/S2 only).
CLOSED = ({(f, "T", "S1", 100.0) for f in (150.0, 230.0, 466.475, 1500.0, 3000.0, 30000.0)}
          | {(f, "T", "S1", 600.0) for f in (150.0, 230.0, 466.475, 1500.0)}
          | {(f, "T", "S2", 300.0) for f in (230.0, 466.475, 1500.0, 3000.0, 30000.0)}
          | {(f, "R", "S2", 300.0) for f in (30.0, 60.0, 88.0, 150.0, 230.0)}
          | {(f, "R", "S2", 800.0) for f in (30.0, 60.0, 88.0, 150.0, 230.0)})


def applicable(prof, f, scenario, d):
    ms = {"M1": lambda: v.m1(prof, f, d, "V"), "M2": lambda: v.m2(prof, f, d, "V"), "M3": lambda: v.m3(prof, f, d),
          "M4": lambda: v.m4(prof, f, d, "V", 3.0, 3.0), "M5": lambda: v.m5(prof, f, d, "V")}
    return "+".join(m for m in v.SCENARIOS[scenario] if ms[m]() is not None) or "OPEN"


class ModelTest(unittest.TestCase):
    def test_med_branches_meet_at_14_m(self):
        """ECAC eq. 6a/6b: the two branches agree at d = 14 m within 0.05 dB (6.28 vs 6.30 dB at 1 GHz)."""
        for f in (230.0, 1000.0, 9000.0):
            lo = 0.45 * (f / 1000) ** 0.284 * 14
            hi = 1.33 * (f / 1000) ** 0.284 * 14 ** 0.588
            self.assertLess(abs(lo - hi), 0.05 * (f / 1000) ** 0.284)
            self.assertEqual(v.m3(MED_OK, f, 14.0), hi)

    def test_med_domain_and_conditions(self):
        self.assertIsNone(v.m3(MED_OK, 229.9, 50))
        self.assertIsNone(v.m3(MED_OK, 95000.1, 50))
        self.assertIsNone(v.m3(MED_OK, 1000, 400.1))
        self.assertIsNotNone(v.m3(MED_OK, 95000.0, 400.0))
        for key, other in (("climate", "TROPICAL"), ("climate", "UNSPECIFIED"), ("leaf", "LEAFLESS"),
                           ("leaf", "UNKNOWN"), ("density", "SPARSE"), ("moisture", "WET"), ("moisture", "UNKNOWN")):
            self.assertIsNone(v.m3(dict(MED_OK, **{key: other}), 1000, 50), key + other)

    def test_p833_eq2_matches_table1(self):
        """P.833-10 TABLE 1 (St. Petersburg mixed forest) against eq. (2) with A1 = 1.37, alpha = 0.42: within 15 %."""
        for f, am in ((105.9, 9.4), (466.475, 18.0), (949.0, 26.5), (1852.2, 29.0), (2117.5, 34.1)):
            self.assertAlmostEqual(1.0, 1.37 * f ** 0.42 / am, delta=0.15, msg=f)

    def test_p833_eq1_limits(self):
        g, src = v.gamma_at(SPB, v.po.f32(466.475), "V")
        self.assertEqual((0.12, "TABLE_POINT"), (round(g, 6), src))
        self.assertAlmostEqual(0.01 * g, v.m1(SPB, 466.475, 0.01, "V"), delta=1e-6)
        am = SPB["am"]["a1"] * 466.475 ** SPB["am"]["alpha"]
        self.assertAlmostEqual(am, v.m1(SPB, 466.475, 1.0e6, "V"), places=6)
        self.assertIsNone(v.m1(MED_OK, 466.475, 50, "V"))  # no Am block -> no eq. (1)

    def test_fig2_reading_is_straight_in_log_log(self):
        """P.833 Fig. 2 end points: H is proportional to f over 30 MHz-30 GHz, V meets H at 1 GHz."""
        prof = v.typed_profile(b.profile("UNSPECIFIED", b.FIG2, "STANDARD_FIGURE"))
        for f in (30.0, 60.0, 150.0, 1000.0, 3000.0, 30000.0):
            self.assertAlmostEqual(0.0002 * f, v.gamma_at(prof, f, "H")[0], delta=1e-9 + 1e-6 * f)
        self.assertEqual(v.gamma_at(prof, 1000.0, "V"), v.gamma_at(prof, 1000.0, "H"))
        self.assertEqual((None, "NONE"), v.gamma_at(prof, 29.9, "V"))

    def test_two_low_vhf_gamma_sources_agree_on_v_not_on_h(self):
        """REQ VG08: Fig. 2 and Rice-71 agree within 10 % for V at 30-60 MHz; for H at 60 MHz they differ ~1.7x."""
        fig2 = v.typed_profile(b.profile("UNSPECIFIED", b.FIG2, "STANDARD_FIGURE"))
        rice = v.typed_profile(b.profile("UNSPECIFIED", b.RICE71, "STANDARD_FIGURE"))
        for f in (30.0, 60.0):
            self.assertAlmostEqual(1.0, v.gamma_at(rice, f, "V")[0] / v.gamma_at(fig2, f, "V")[0], delta=0.1)
        self.assertGreater(v.gamma_at(rice, 60.0, "H")[0] / v.gamma_at(fig2, 60.0, "H")[0], 1.5)

    def test_jansky_bailey_table_frequency_is_exact(self):
        alpha, a, bb = v.jb_constants("V", 250.0)
        self.assertEqual((0.050, 0.759, 0.000443), (alpha, a, bb))
        dm = 500 / v.MILE_M
        lb = 36.57 + 20 * math.log10(250) - 20 * math.log10(a * math.exp(-1609 * alpha * dm) / dm + bb / dm ** 2)
        self.assertAlmostEqual(lb - (36.57 + 20 * math.log10(250) + 20 * math.log10(dm)),
                               v.m4(TROP, 250.0, 500.0, "V", 3, 3), places=9)

    def test_jansky_bailey_40logd_beyond_160_m(self):
        """ECAC §5.1: beyond about 160 m the basic loss grows as 40·lg d, i.e. the excess grows ~6 dB per doubling."""
        for pol in "VH":
            step = v.m4(TROP, 250.0, 1600.0, pol, 3, 3) - v.m4(TROP, 250.0, 800.0, pol, 3, 3)
            self.assertAlmostEqual(20 * math.log10(2), step, delta=1.0, msg=pol)

    def test_negative_jansky_bailey_excess_is_excluded(self):
        self.assertIsNone(v.m4(TROP, 100.0, 10.0, "H", 3, 3))
        self.assertIsNotNone(v.m4(TROP, 100.0, 10.0, "V", 3, 3))

    def test_lin_gamma_depth_ceiling(self):
        self.assertEqual(400.0 * v.po.f32(0.02), v.m5(MED_OK, 30.0, 400.0, "V"))
        self.assertIsNone(v.m5(MED_OK, 30.0, 400.1, "V"))

    def test_frequency_scenario_matrix(self):
        for f, want in MATRIX.items():
            got = " ".join(applicable(MED_OK_AM, f, s, d) for s, d in CELLS) + " | " + \
                  " ".join(applicable(TROP_FIG2, f, s, d) for s, d in CELLS)
            self.assertEqual(want, got, f)

    def test_only_source_domain_primary_closes_s1_s2(self):
        """VG10a: a cell is closed only when its primary is M1/M3/M4 in S1/S2; M5-only, M2 and S3 cells stay open."""
        closed = set()
        for f in MATRIX:
            for name, prof in (("T", MED_OK_AM), ("R", TROP_FIG2)):
                for s, d in CELLS:
                    first = applicable(prof, f, s, d).split("+")[0]
                    if v.STATUS.get(first) == "SOURCE_DOMAIN" and s in ("S1", "S2"):
                        closed.add((f, name, s, d))
        self.assertEqual(CLOSED, closed)

    def test_statuses(self):
        self.assertEqual({"M1": "SOURCE_DOMAIN", "M2": "UNCAPPED", "M3": "SOURCE_DOMAIN", "M4": "SOURCE_DOMAIN",
                          "M5": "ASSUMED_EXTENSION"}, v.STATUS)


MED_OK_AM = v.typed_profile(b.profile("TEMPERATE", b.FIG2, "STANDARD_FIGURE", b.AM_SPB))
TROP_FIG2 = v.typed_profile(b.profile("TROPICAL", b.FIG2, "STANDARD_FIGURE", moisture="WET"))


class CorpusTest(unittest.TestCase):
    def setUp(self):
        self.cases = json.loads(VECTORS.read_bytes())["cases"]
        self.rows = [r for c in self.cases for r in c["expect"].get("intervals", [])]

    def test_vectors_are_built_from_case_list(self):
        self.assertEqual(VECTORS.read_bytes(), b.build())
        self.assertNotIn(b"\r\n", VECTORS.read_bytes())

    def test_every_code_scenario_and_model_is_covered(self):
        self.assertEqual(set(v.CODES), {c["expect"]["code"] for c in self.cases})
        self.assertEqual({"S1", "S2", "S3"}, {r["scenario"] for r in self.rows})
        self.assertEqual({"M1", "M2", "M3", "M4", "M5"}, {r["primaryModel"] for r in self.rows})
        self.assertEqual({"TABLE_POINT", "INTERPOLATED", "NONE"}, {r["gammaSource"] for r in self.rows})
        open_scenarios = {c["expect"]["interval"]["scenario"] for c in self.cases
                          if c["expect"]["code"] == "NO_MODEL_FOR_SCENARIO"}
        self.assertEqual({"S1", "S2", "S3+"}, open_scenarios)
        low_vhf = {c["id"] for c in self.cases if c["input"]["observation"]["frequencyMHz"] in (30.0, 60.0)}
        self.assertGreaterEqual(len(low_vhf), 7)
        ok = [c["expect"] for c in self.cases if c["expect"]["code"] == "OK"]
        self.assertTrue(any(e["allScenariosClosed"] for e in ok) and not all(e["allScenariosClosed"] for e in ok))
        by_id = {c["id"]: c["expect"] for c in self.cases}
        self.assertEqual("S3+", by_id["V55"]["interval"]["scenario"])  # NO_MODEL outranks NUMERIC of V55's first interval

    def test_exd_is_never_a_candidate_and_cap_is_never_applied(self):
        for r in self.rows:
            self.assertNotIn("EXD", {c["model"] for c in r["candidates"]})
            self.assertFalse(r["capApplied"])
            self.assertEqual(r["primaryModel"], r["candidates"][0]["model"])
            self.assertEqual(r["excessBaseline"] == "FREE_SPACE", r["primaryModel"] == "M4")
            self.assertEqual(r["scenarioClosed"], r["primaryStatus"] == "SOURCE_DOMAIN" and r["scenario"] != "S3")
            self.assertEqual([v.STATUS[c["model"]] for c in r["candidates"]], [c["status"] for c in r["candidates"]])

    def test_check_detects_corruption(self):
        self.assertEqual(0, v.check(VECTORS))
        for cid, mutate in (("V03", lambda e: e["intervals"][0].update(primaryModel="M1")),
                            ("V03", lambda e: e["intervals"][0].update(primaryLossDb=e["intervals"][0]["primaryLossDb"] + 1e-3)),
                            ("V03", lambda e: e["intervals"][0]["candidates"].pop()),
                            ("V38", lambda e: e["intervals"][0].update(gammaSource="TABLE_POINT")),
                            ("V41", lambda e: e["interval"].update(scenario="S3+"))):
            corrupted = copy.deepcopy(self.cases)
            mutate(next(c for c in corrupted if c["id"] == cid)["expect"])
            errors = []
            for c in corrupted:
                v.po.close(v.evaluate(c), c["expect"], c["id"], errors, 1e-9)
            self.assertTrue(errors, cid)

    def test_input_is_not_mutated(self):
        inp = b.case(1000, 466.475, [b.veg(0, 50)])
        before = json.dumps(inp, sort_keys=True)
        v.vegetation_layer(inp)
        self.assertEqual(before, json.dumps(inp, sort_keys=True))


if __name__ == "__main__":
    unittest.main()
