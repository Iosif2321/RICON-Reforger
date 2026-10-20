"""Проверка фиксированного корпуса, ветви вне области шума и безопасного отказа до записи."""
import contextlib
import io
import json
from pathlib import Path
import tempfile
import unittest

import gen_p526_matrix as g

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "oracle/vectors/link_scenarios_v1.json"
OUTPUT = ROOT / "tools/RICON_TestRunner/Scripts/WorkbenchGame/RICON_P526MatrixPoints.c"


class GeneratorTests(unittest.TestCase):
    def test_committed_points_are_exact(self):
        result = g.render(SOURCE.read_bytes())
        self.assertEqual(OUTPUT.read_text(encoding="utf-8"), result)
        self.assertEqual(1008, result.count("t.CheckPoint("))
        self.assertEqual(48, result.count("protected static void Scenario"))

    def test_expectations_and_fallback_are_copied(self):
        data = json.loads(SOURCE.read_bytes())
        total = fallback = 0
        for scenario in data["scenarios"]:
            produced = g.rows(scenario)
            for point, offset in zip(scenario["expect"]["models"]["P526_GENERAL"]["samples"], range(0, len(produced), 3)):
                none = point["noise"]["none"]
                for env_index, env in enumerate(scenario["input"]["noiseEnvironments"]):
                    row = produced[offset + env_index]
                    self.assertEqual(row[7:9], (point["lossDb"], point["rxPowerDbm"]))
                    self.assertEqual(row[11], point["aboveSensitivity"])
                    if scenario["expect"]["noise"][env] is None:
                        self.assertEqual(row[9], none["snrDb"])
                        self.assertEqual(row[10:], (False, point["aboveSensitivity"], False, False))
                        fallback += 1
                    else:
                        self.assertEqual(row[9], point["noise"][env]["snrDb"])
                        self.assertTrue(row[10] and row[13])
                    total += 1
        self.assertEqual((1008, 168), (total, fallback))

    def test_changed_input_is_rejected_without_overwriting(self):
        raw = SOURCE.read_bytes()
        for bad in (b"{}", raw + b"\n", raw.replace(b'"revision": 3', b'"revision": 4')):
            with self.assertRaises(ValueError):
                g.render(bad)
        with tempfile.TemporaryDirectory(prefix="ricon-p526-") as tmp:
            source, output = Path(tmp) / "bad.json", Path(tmp) / "points.c"
            source.write_bytes(b"{}")
            output.write_bytes(b"existing")
            with contextlib.redirect_stdout(io.StringIO()):
                result = g.main([str(source), str(output)])
            self.assertEqual(1, result)
            self.assertEqual(b"existing", output.read_bytes())


if __name__ == "__main__":
    unittest.main()
