"""Проверка копирования чисел и отказа до записи при изменённом корпусе."""
import contextlib
import io
import json
from pathlib import Path
import tempfile
import unittest

import gen_sensitivity_points as g

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "oracle/vectors/l2_sensitivity_anchors_v1.json"
OUTPUT = ROOT / "tools/RICON_TestRunner/Scripts/WorkbenchGame/RICON_SensitivityPoints.c"


class GeneratorTests(unittest.TestCase):
    def test_committed_points_are_exact(self):
        result = g.render(SOURCE.read_bytes())
        self.assertEqual(OUTPUT.read_text(encoding="utf-8"), result)
        self.assertEqual(16, result.count("t.CheckPoint("))
        self.assertEqual(16, result.count("protected static void Anchor"))

    def test_expectations_and_diagnostics_are_copied(self):
        for i, case in enumerate(json.loads(SOURCE.read_bytes())["cases"]):
            result = "\n".join(g.case_lines(i, case))
            e, inp = case["expect"], case["input"]
            for field, value in (("m_fLossDb", e["L"]), ("m_fBullingtonActualDb", e["Lba"]),
                                 ("m_fBullingtonSmoothDb", e["Lbs"]), ("m_fSphericalDb", e["Lsph"])):
                self.assertIn(f"expected.{field} = {g.literal(value)};", result)
            for field, enum, value in (("m_eBranchActual", "RICON_EL2Branch", e["branchActual"]),
                                       ("m_eBranchSmooth", "RICON_EL2Branch", e["branchSmooth"]),
                                       ("m_eSphericalCase", "RICON_EL2SphericalCase", e["sphCase"]),
                                       ("m_eFirstTermAccuracy", "RICON_EFirstTermAccuracy", e["firstTermAccuracy"])):
                self.assertIn(f"expected.{field} = {enum}.{value};", result)
            call = next(line.strip() for line in result.splitlines() if "t.CheckPoint(" in line)
            args = call[len("t.CheckPoint("):-2].split(", ")
            self.assertEqual(json.loads(args[0]), case["id"])
            self.assertEqual(list(map(float, args[1:4])), [inp["pathKm"] * 1000, inp["frequencyMHz"], inp["htg"]])
            if inp["hill"]:
                self.assertEqual(list(map(float, args[4:7])), [inp["hill"][k] for k in
                                                              ("centerKm", "halfWidthKm", "heightM")])
            else:
                self.assertEqual(float(args[6]), 0.0)
                self.assertGreater(float(args[5]), 0.0)
            self.assertEqual(args[7:9], ["config", "expected"])
            self.assertEqual(int(args[9]), e["profilePointCount"])
            self.assertEqual(float(args[10]), e["peakTerrainM"])
            cfg = inp["config"]
            cfg_call = next(line for line in result.splitlines() if "RICON_L2Run.Config(" in line)
            cfg_args = cfg_call.split("RICON_L2Run.Config(")[1][:-2].split(", ")
            self.assertEqual(list(map(float, cfg_args[:4])), [cfg[k] for k in
                                                             ("earthRadiusKm", "kFactor", "permittivity", "conductivitySm")])
            self.assertEqual(cfg_args[4], "RICON_ETerrainPolarization." + cfg["polarization"])
            self.assertEqual(float(cfg_args[5]), cfg["maxProfileStepKm"])
            self.assertEqual(int(cfg_args[6]), cfg["maxProfilePoints"])

    def test_unsupported_symmetry_or_result_is_rejected(self):
        case = json.loads(SOURCE.read_bytes())["cases"][0]
        case["input"]["hrg"] += 1
        with self.assertRaises(ValueError):
            g.case_lines(0, case)
        case["input"]["hrg"] = case["input"]["htg"]
        case["expect"]["code"] = "INVALID_PROFILE"
        with self.assertRaises(ValueError):
            g.case_lines(0, case)

    def test_changed_input_is_rejected_without_overwriting(self):
        raw = SOURCE.read_bytes()
        for bad in (b"{}", raw + b"\n", raw.replace(b'"toleranceDb": 0.05', b'"toleranceDb": 0.5')):
            with self.assertRaises(ValueError):
                g.render(bad)
        with tempfile.TemporaryDirectory(prefix="ricon-sens-") as tmp:
            source, output = Path(tmp) / "bad.json", Path(tmp) / "points.c"
            source.write_bytes(b"{}")
            output.write_bytes(b"existing")
            with contextlib.redirect_stdout(io.StringIO()):
                result = g.main([str(source), str(output)])
            self.assertEqual(1, result)
            self.assertEqual(b"existing", output.read_bytes())


if __name__ == "__main__":
    unittest.main()
