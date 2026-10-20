"""SHA-граница snapshot, точное покрытие и защита существующего файла при отказе."""
import contextlib
import io
from pathlib import Path
import tempfile
import unittest

import gen_snapshot_fixture as g

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "oracle/vectors/snapshot_v1.json"
FIXTURE = ROOT / "addons/RICON_CoreTests/Scripts/Game/RICON_Tests/RICON_SnapshotFixtures.c"


class GeneratorTests(unittest.TestCase):
    def test_committed_fixture_and_coverage(self):
        rendered = g.render(SOURCE.read_bytes())
        self.assertEqual(FIXTURE.read_text(encoding="utf-8"), rendered)
        self.assertEqual(50, rendered.count("protected static void Case"))
        self.assertEqual(1, rendered.count("t.RequireNaN("))
        self.assertIn("cases == 43 && steps == 77 && quietPositive == 43 && rejected == 7", rendered)
        self.assertIn("BLOCKED_NO_AUTHORITY_CONTEXT", rendered)

    def test_untrusted_or_changed_bytes_rejected(self):
        raw = SOURCE.read_bytes()
        for bad in (b"{}", raw + b"\n", raw.replace(b'"ACCEPT_FIRST"', b'"STALE"'),
                    raw.replace(b'"CMP-01"', b'"injected\\\"; exit;"')):
            with self.subTest(size=len(bad)), self.assertRaises(ValueError):
                g.render(bad)

    def test_rejected_input_preserves_output(self):
        with tempfile.TemporaryDirectory(prefix="ricon-snapshot-") as tmp:
            source, output = Path(tmp) / "bad.json", Path(tmp) / "existing.c"
            source.write_text("{}", encoding="utf-8")
            output.write_bytes(b"existing")
            with contextlib.redirect_stdout(io.StringIO()):
                result = g.main([str(source), str(output)])
            self.assertEqual(1, result)
            self.assertEqual(b"existing", output.read_bytes())


if __name__ == "__main__":
    unittest.main()
