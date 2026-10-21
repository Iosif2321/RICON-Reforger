"""Проверки границ генератора S2: закреплённый корпус, копирование ожиданий, закрытые литералы."""
import contextlib
import copy
import io
import json
from pathlib import Path
import tempfile
import unittest

import gen_radio_device_fixture as g

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "oracle/vectors/radio_device_v1.json"
OUTPUT = ROOT / "addons/RICON_CoreTests/Scripts/Game/RICON_Tests/RICON_RadioDeviceFixtures.c"


def first_case():
    return copy.deepcopy(json.loads(SOURCE.read_bytes())["cases"][0])


class RadioDeviceGeneratorTests(unittest.TestCase):
    def test_committed_fixture_and_all_cases(self):
        output = g.render(SOURCE.read_bytes())
        self.assertEqual(OUTPUT.read_text(encoding="utf-8"), output)
        self.assertEqual(g.CASES, output.count("t.CheckCase("))

    def test_modified_corpus_never_overwrites_destination(self):
        with tempfile.TemporaryDirectory(prefix="ricon-radio-gen-") as tmp:
            source, destination = Path(tmp) / "bad.json", Path(tmp) / "fixture.c"
            source.write_bytes(SOURCE.read_bytes() + b"\n")
            destination.write_bytes(b"existing")
            with contextlib.redirect_stdout(io.StringIO()):
                result = g.main([str(source), str(destination)])
            self.assertEqual(1, result)
            self.assertEqual(b"existing", destination.read_bytes())

    def test_expectations_are_copied_without_recomputation(self):
        case = first_case()
        case["expect"]["query"]["bandwidthHz"] = 777.5
        case["expect"]["query"]["txPowerDbm"] = 1.25
        result = "\n".join(g.case_lines(0, case))
        self.assertIn("expected.m_fBandwidthHz = 777.5;", result)
        self.assertIn("expected.m_fTxPowerDbm = 1.25;", result)
        self.assertIn("RICON_ERadioQueryCode.READY, expected);", result)

    def test_null_inputs_and_rejections_have_no_query(self):
        case = first_case()
        case["input"]["context"] = None
        case["expect"] = {"code": "NULL_INPUT", "query": None}
        result = "\n".join(g.case_lines(0, case))
        self.assertIn("RICON_RadioQueryContext context;", result)
        self.assertNotIn("context = new", result)
        self.assertNotIn("expected = new", result)
        case["expect"] = {"code": "NULL_INPUT", "query": first_case()["expect"]["query"]}
        with self.assertRaises(ValueError):
            g.case_lines(0, case)

    def test_injection_unknown_fields_and_types_are_rejected(self):
        for kind in ("id", "key", "field", "bool", "int", "code"):
            case = first_case()
            if kind == "id":
                case["id"] = 'R01"); injected();'
            elif kind == "key":
                case["input"]["txObservation"]["networkKey"] = 'a"; injected(); "'
            elif kind == "field":
                case["input"]["txProfile"]["extra"] = 1
            elif kind == "bool":
                case["input"]["txObservation"]["powered"] = 1
            elif kind == "int":
                case["input"]["txObservation"]["deviceId"] = 2**31
            else:
                case["expect"]["code"] = "DELIVERED"
            with self.assertRaises(ValueError):
                g.case_lines(0, case)

    def test_float_literals_follow_binary32_overflow(self):
        self.assertEqual("RICON_L0Run.Special(0)", g.number("NaN"))
        self.assertEqual("RICON_L0Run.Special(1)", g.number("+Inf"))
        self.assertEqual("RICON_L0Run.Special(-1)", g.number("-Inf"))
        self.assertEqual("RICON_L0Run.Special(1)", g.number(1e40))
        self.assertEqual("RICON_L0Run.Special(-1)", g.number(-1e40))
        self.assertEqual("50.00000001", g.number(50.00000001))
        for value in ("injected()", True, float("inf"), float("nan")):
            with self.assertRaises(ValueError):
                g.number(value)


if __name__ == "__main__":
    unittest.main()
