"""Границы генератора CT10.1: закреплённый SHA, host-only, null-входы, goldenHex и отказ без перезаписи."""
import contextlib
import copy
import io
import json
from pathlib import Path
import tempfile
import unittest

import gen_channel_descriptor_fixture as g

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'oracle/vectors/channel_descriptor_v1.json'
OUTPUT = ROOT / 'addons/RICON_CoreTests/Scripts/Game/RICON_Tests/RICON_ChannelDescriptorFixtures.c'


def case(cid):
    for c in json.loads(SOURCE.read_bytes())['cases']:
        if c['id'] == cid:
            return copy.deepcopy(c)
    raise KeyError(cid)


def lines(c):
    return '\n'.join(g.case_lines(0, c))


class ChannelDescriptorGeneratorTest(unittest.TestCase):
    def test_fixture_and_case_count(self):
        output = g.render(SOURCE.read_bytes())
        self.assertEqual(output, OUTPUT.read_text(encoding='utf-8'))
        self.assertEqual(46, output.count('t.Check'))
        self.assertIn('static const int HOST_ONLY = 8;', output)
        for cid in g.HOST_ONLY:
            self.assertNotIn(f'"{cid}"', output)
        golden = bytes.fromhex(json.loads(SOURCE.read_bytes())['goldenHex'])
        self.assertIn('array<int> golden = {' + ', '.join(str(b) for b in golden) + '};', output)

    def test_modified_source_does_not_overwrite(self):
        with tempfile.TemporaryDirectory(prefix='ricon-descriptor-generator-') as directory:
            src, dst = Path(directory) / 'source.json', Path(directory) / 'fixture.c'
            src.write_bytes(SOURCE.read_bytes().replace(b'\n', b'\r\n'))
            dst.write_bytes(b'existing')
            with contextlib.redirect_stdout(io.StringIO()):
                code = g.main([str(src), str(dst)])
            self.assertEqual(1, code)
            self.assertEqual(b'existing', dst.read_bytes())

    def test_null_inputs_and_failures(self):
        self.assertIn('RICON_FadingDescriptor d;', lines(case('D51')))
        self.assertIn('array<int> bytes;', lines(case('D52')))
        out = lines(case('D10'))
        self.assertIn('RICON_EDescriptorCode.INVALID_DESCRIPTOR', out)
        self.assertNotIn('expectedDescriptor', out)

    def test_ok_copies_descriptor_bytes_and_size(self):
        c = case('D01')
        out = lines(c)
        self.assertIn('expected.m_Descriptor = expectedDescriptor;', out)
        self.assertIn(f'expected.m_iSizeBytes = {c["expect"]["sizeBytes"]};', out)
        rebase = lines(case('D42'))
        self.assertIn('t.CheckRebase("D42", d, 1200, expected);', rebase)
        self.assertNotIn('expectedBytes', rebase)
        self.assertIn('(-2147483647 - 1)', lines(case('D48')))

    def test_typed_errors_are_refused(self):
        for mode in ('op', 'code', 'size', 'extra'):
            c = case('D01')
            if mode == 'op':
                c['input']['op'] = 'SEND'
            elif mode == 'code':
                c['expect']['code'] = 'LOST'
            elif mode == 'size':
                c['expect']['sizeBytes'] += 1
            else:
                c['expect']['extra'] = 1
            with self.assertRaises(ValueError, msg=mode):
                g.case_lines(0, c)


if __name__ == '__main__':
    unittest.main()
