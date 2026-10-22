"""Границы генератора P1: закреплённый SHA, явный host-only список, флаги null и отказ без перезаписи."""
import contextlib
import copy
import io
import json
from pathlib import Path
import tempfile
import unittest

import gen_channel_cutoff_fixture as g

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'oracle/vectors/channel_cutoff_v1.json'
OUTPUT = ROOT / 'addons/RICON_CoreTests/Scripts/Game/RICON_Tests/RICON_ChannelCutoffFixtures.c'


def case(cid='C01'):
    for c in json.loads(SOURCE.read_bytes())['cases']:
        if c['id'] == cid:
            return copy.deepcopy(c)
    raise KeyError(cid)


def lines(c):
    return '\n'.join(g.case_lines(0, c))


class ChannelCutoffGeneratorTest(unittest.TestCase):
    def test_fixture_and_case_count(self):
        output = g.render(SOURCE.read_bytes())
        self.assertEqual(output, OUTPUT.read_text(encoding='utf-8'))
        self.assertEqual(24, output.count('t.CheckCase('))
        self.assertIn('static const int HOST_ONLY = 3;', output)
        for cid in g.HOST_ONLY:
            self.assertNotIn(f't.CheckCase("{cid}"', output)

    def test_modified_source_does_not_overwrite(self):
        with tempfile.TemporaryDirectory(prefix='ricon-cutoff-generator-') as directory:
            src, dst = Path(directory) / 'source.json', Path(directory) / 'fixture.c'
            src.write_bytes(SOURCE.read_bytes().replace(b'\n', b'\r\n'))
            dst.write_bytes(b'existing')
            with contextlib.redirect_stdout(io.StringIO()):
                code = g.main([str(src), str(dst)])
            self.assertEqual(1, code)
            self.assertEqual(b'existing', dst.read_bytes())

    def test_null_becomes_flag(self):
        c = case()
        c['input']['toleranceDbm'] = None
        c['input']['paths'][0]['upperDbm'] = None
        c['expect']['upperDbm'] = None
        out = lines(c)
        self.assertIn('input.m_bHasTolerance = false;', out)
        self.assertNotIn('input.m_fToleranceDbm', out)
        self.assertIn('path0.m_bHasUpper = false;', out)
        self.assertIn('expected.m_bHasUpper = false;', out)

    def test_expectations_are_copied(self):
        c = case()
        c['expect']['reason'] = 'ABOVE_TOLERANCE'
        c['expect']['upperDbm'] = -77.5
        out = lines(c)
        self.assertIn('expected.m_eReason = RICON_ECutoffReason.ABOVE_TOLERANCE;', out)
        self.assertIn('expected.m_fUpperDbm = -77.5;', out)

    def test_typed_errors_are_refused_outside_host_only(self):
        for mode in ('bool', 'field', 'reason', 'id'):
            c = case()
            if mode == 'bool':
                c['input']['nowMs'] = True
            elif mode == 'field':
                c['input']['extra'] = 1
            elif mode == 'reason':
                c['expect']['reason'] = 'DEMODULATOR'
            else:
                c['id'] = 'C01"); injected();'
            with self.assertRaises(ValueError):
                g.case_lines(0, c)

    def test_host_only_must_be_invalid_input(self):
        d = json.loads(SOURCE.read_bytes())
        for c in d['cases']:
            if c['id'] in g.HOST_ONLY:
                self.assertEqual(('UNRESOLVED', 'INVALID_INPUT'), (c['expect']['decision'], c['expect']['reason']))


if __name__ == '__main__':
    unittest.main()
