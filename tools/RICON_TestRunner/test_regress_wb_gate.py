"""Регрессия гейта WB: FAIL-маркер отвергается, NUMERIC_FAILURE как значение поля допускается."""
from pathlib import Path
import re
import unittest


class WorkbenchGateTest(unittest.TestCase):
    def setUp(self):
        self.source=Path(__file__).with_name('regress_wb.ps1').read_text(encoding='utf-8')
        found=re.search(r"^\$taggedFailPattern = '([^']+)'$",self.source,re.MULTILINE)
        self.assertIsNotNone(found)
        self.pattern=re.compile(found.group(1),re.IGNORECASE)

    def test_all_real_failure_markers_are_rejected(self):
        for line in ('SCRIPT : [RICON_TEST] FAIL case: expected mismatch',
                     'SCRIPT : [RICON_PAIR] done: FAIL',
                     'SCRIPT : [RICON_TEST] suite lifecycle: FAIL',
                     'SCRIPT : [RICON_CUSTOM] fail: assertion'):
            with self.subTest(line=line):
                self.assertIsNotNone(self.pattern.search(line))

    def test_diagnostic_failure_reason_is_not_a_failure_marker(self):
        for line in ('SCRIPT : [RICON_PAIR_BUDGET] valid=0 reason=NUMERIC_FAILURE state=LOST',
                     'SCRIPT : [RICON_PAIR] done: PASS',
                     'SCRIPT : [RICON_TEST] case FAILED_MODEL_NAME is test data',
                     'PLATFORM (E): Steamworks: RequestCurrentStats failed'):
            with self.subTest(line=line):
                self.assertIsNone(self.pattern.search(line))

    def test_runner_uses_the_checked_pattern_and_other_guards(self):
        self.assertIn('Select-String -Pattern $taggedFailPattern',self.source)
        for guard in ('$p.ExitCode -eq 0','$done -eq 1','$errors -eq 0','$fails -eq 0'):
            self.assertIn(guard,self.source)


if __name__=='__main__':
    unittest.main()
