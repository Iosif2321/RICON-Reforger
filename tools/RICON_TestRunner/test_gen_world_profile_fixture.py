"""Проверки границ генератора: корпус, копирование ожиданий и отказ до записи."""
import contextlib
import copy
import io
import json
from pathlib import Path
import tempfile
import unittest

import gen_world_profile_fixture as g

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "oracle/vectors/world_profile_v1.json"
OUTPUT = ROOT / "addons/RICON_CoreTests/Scripts/Game/RICON_Tests/RICON_WorldProfileFixtures.c"


class WorldProfileGeneratorTests(unittest.TestCase):
    def test_committed_fixture_and_all_cases(self):
        output = g.render(SOURCE.read_bytes())
        self.assertEqual(OUTPUT.read_text(encoding="utf-8"), output)
        self.assertEqual(g.CASES, output.count("t.CheckCase("))

    def test_modified_corpus_never_overwrites_destination(self):
        with tempfile.TemporaryDirectory(prefix="ricon-world-gen-") as tmp:
            source, destination = Path(tmp) / "bad.json", Path(tmp) / "fixture.c"
            source.write_bytes(SOURCE.read_bytes() + b"\n")
            destination.write_bytes(b"existing")
            with contextlib.redirect_stdout(io.StringIO()):
                result = g.main([str(source), str(destination)])
            self.assertEqual(1, result)
            self.assertEqual(b"existing", destination.read_bytes())

    def test_expectations_are_copied_without_recomputation(self):
        case = copy.deepcopy(json.loads(SOURCE.read_bytes())["cases"][0])
        case["expect"]["profile"][0][1] = 123.456
        case["expect"]["htg"] = 2.75
        result = "\n".join(g.case_lines(0, case))
        self.assertIn("expected.m_Profile.AddPoint(0.0, 123.456);", result)
        self.assertIn("expected.m_fTxAgl = 2.75;", result)
        self.assertIn("source.m_fBase = 100.0;", result)

    def test_injection_unknown_source_and_wrong_count_are_rejected(self):
        base = json.loads(SOURCE.read_bytes())["cases"][0]
        for kind in ("id", "source", "count"):
            case = copy.deepcopy(base)
            if kind == "id":
                case["id"] = 'W01"); injected();'
            elif kind == "source":
                case["input"]["source"]["water"] = 0
            else:
                case["expect"]["n"] += 1
            with self.assertRaises(ValueError):
                g.case_lines(0, case)

    def test_special_values_are_closed_and_keep_their_class(self):
        self.assertEqual("RICON_L0Run.Special(0)", g.number("NaN"))
        self.assertEqual("RICON_L0Run.Special(1)", g.number("+Inf"))
        self.assertEqual("RICON_L0Run.Special(-1)", g.number("-Inf"))
        for value in ("injected()", True, 10**500, float("inf")):
            with self.assertRaises((ValueError, OverflowError)):
                g.number(value)


if __name__ == "__main__":
    unittest.main()
