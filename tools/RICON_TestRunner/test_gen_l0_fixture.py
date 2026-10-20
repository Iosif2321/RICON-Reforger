"""Граница доверия фиксированного L0-корпуса и воспроизводимость генерации."""
import contextlib
import io
from pathlib import Path
import tempfile
import unittest

import gen_l0_fixture as g

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "oracle/vectors/link_eval_v1.json"
FIXTURE = ROOT / "addons/RICON_CoreTests/Scripts/Game/RICON_Tests/RICON_L0Fixtures.c"


class GeneratorTests(unittest.TestCase):
    def test_committed_fixture_is_exact(self):
        result = g.render(SOURCE.read_bytes())
        self.assertEqual(FIXTURE.read_text(encoding="utf-8"), result)
        self.assertEqual(65, result.count("protected static void Case"))
        self.assertEqual(30, result.count("protected static RICON_LinkQuery Query"))
        self.assertIn("quietPositive == 57 && rejected == 8", result)
        self.assertIn("RICON_L0Run.Special(1)", result)

    def test_changed_or_untrusted_input_is_rejected(self):
        raw = SOURCE.read_bytes()
        for bad in (b"{}", raw + b"\n", raw.replace(b'"lossDb": 72.447783', b'"lossDb": 0'),
                    raw.replace(b'"FSPL-01"', b'"injected\\\"; exit;"')):
            with self.subTest(size=len(bad)), self.assertRaises(ValueError):
                g.render(bad)

    def test_rejection_preserves_existing_output(self):
        with tempfile.TemporaryDirectory(prefix="ricon-l0-") as tmp:
            source = Path(tmp) / "bad.json"
            output = Path(tmp) / "fixture.c"
            source.write_text("{}", encoding="utf-8")
            output.write_bytes(b"existing")
            with contextlib.redirect_stdout(io.StringIO()):
                result = g.main([str(source), str(output)])
            self.assertEqual(1, result)
            self.assertEqual(b"existing", output.read_bytes())


if __name__ == "__main__":
    unittest.main()
