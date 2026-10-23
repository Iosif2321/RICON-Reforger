"""Границы генератора P1b: закреплённый SHA, явный host-only список, литералы INT_MIN/ключей и отказ без перезаписи."""
import contextlib
import copy
import io
import json
from pathlib import Path
import tempfile
import unittest

import gen_channel_fading_fixture as g

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'oracle/vectors/channel_fading_v1.json'
OUTPUT = ROOT / 'addons/RICON_CoreTests/Scripts/Game/RICON_Tests/RICON_ChannelFadingFixtures.c'


def case(cid='F05'):
    for c in json.loads(SOURCE.read_bytes())['cases']:
        if c['id'] == cid:
            return copy.deepcopy(c)
    raise KeyError(cid)


def lines(c):
    return '\n'.join(g.case_lines(0, c))


class ChannelFadingGeneratorTest(unittest.TestCase):
    def test_fixture_and_case_count(self):
        output = g.render(SOURCE.read_bytes())
        self.assertEqual(output, OUTPUT.read_text(encoding='utf-8'))
        self.assertEqual(65, output.count('t.CheckCase('))
        self.assertIn('static const int HOST_ONLY = 6;', output)
        for cid in g.HOST_ONLY:
            self.assertNotIn(f't.CheckCase("{cid}"', output)
        self.assertIn('t.CheckCase("F34"', output)  # субнормальный вес — обязательный native

    def test_modified_source_does_not_overwrite(self):
        with tempfile.TemporaryDirectory(prefix='ricon-fading-generator-') as directory:
            src, dst = Path(directory) / 'source.json', Path(directory) / 'fixture.c'
            src.write_bytes(SOURCE.read_bytes().replace(b'\n', b'\r\n'))
            dst.write_bytes(b'existing')
            with contextlib.redirect_stdout(io.StringIO()):
                code = g.main([str(src), str(dst)])
            self.assertEqual(1, code)
            self.assertEqual(b'existing', dst.read_bytes())

    def test_null_coefficient_and_gain_stay_flags(self):
        out = lines(case('F12'))
        self.assertNotIn('m_bHasCoefficient', out)
        self.assertNotIn('m_bHasGain', out)
        zero = lines(case('F04'))
        self.assertIn('expected.m_bHasCoefficient = true;', zero)
        self.assertNotIn('m_bHasGain', zero)
        self.assertIn('expected.m_aPhasesQ.Insert(536870912);', zero)

    def test_int_min_and_awkward_keys(self):
        self.assertIn('(-2147483647 - 1)', lines(case('F24')))
        self.assertNotIn('-2147483648', lines(case('F24')))
        self.assertIn('"bad/key"', lines(case('F32')))
        self.assertIn('t.Add(d, "", ', lines(case('F63')))
        self.assertIn('"' + 'a' * 65 + '"', lines(case('F64')))

    def test_expectations_are_copied(self):
        c = case()
        c['expect']['gainDb'] = -77.5
        c['expect']['phasesQ'] = [3, 4]
        out = lines(c)
        self.assertIn('expected.m_fGainDb = -77.5;', out)
        self.assertIn('expected.m_aPhasesQ.Insert(3);', out)

    def test_typed_errors_are_refused_outside_host_only(self):
        for mode in ('bool', 'field', 'code', 'id', 'quote', 'half'):
            c = case()
            if mode == 'bool':
                c['input']['query']['nowMs'] = True
            elif mode == 'field':
                c['input']['descriptor']['extra'] = 1
            elif mode == 'code':
                c['expect']['code'] = 'LOST'
            elif mode == 'id':
                c['id'] = 'F01"); injected();'
            elif mode == 'quote':
                c['input']['descriptor']['components'][0]['componentId'] = 'a"); injected();//'
            else:
                c['expect']['imag'] = None
            with self.assertRaises(ValueError, msg=mode):
                g.case_lines(0, c)

    def test_host_only_must_be_invalid_input(self):
        d = json.loads(SOURCE.read_bytes())
        self.assertEqual(sorted(g.HOST_ONLY), sorted(d['hostOnly']))
        for c in d['cases']:
            if c['id'] in g.HOST_ONLY:
                self.assertEqual('INVALID_INPUT', c['expect']['code'])


if __name__ == '__main__':
    unittest.main()
