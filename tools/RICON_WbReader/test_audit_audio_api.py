"""Проверяет извлечение методов и отказ неполной API-инвентаризации."""
from pathlib import Path
import tempfile
import unittest

import audit_audio_api as a


class AudioApiTests(unittest.TestCase):
    def test_method_line_and_html_entities(self):
        page = '\n<tr><td class="memItemRight"><a href="base#x">GetContainer</a> (<a>int</a> index=0)</td></tr>'
        self.assertEqual(a.methods(page), [{"name": "GetContainer", "line": 2, "signature": "GetContainer ( int index=0)"}])
        self.assertEqual(a.methods('<td class="memItemRight"><a>Attribute</a></td>'), [])

    def test_missing_or_incomplete_page_fails_closed(self):
        with tempfile.TemporaryDirectory(prefix="ricon-audio-api-") as tmp:
            root = Path(tmp)
            with self.assertRaises(OSError):
                a.audit(root)
            (root / "interfaceAudioEditor.html").write_text("<html>no methods</html>", encoding="utf-8")
            with self.assertRaises(ValueError):
                a.audit(root)


if __name__ == "__main__":
    unittest.main()
