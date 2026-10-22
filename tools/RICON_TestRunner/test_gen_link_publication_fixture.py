"""Границы генератора S3b: закрытые типы, копирование ожиданий, null-формы и отказ без перезаписи."""
import contextlib
import copy
import io
import json
from pathlib import Path
import tempfile
import unittest

import gen_link_publication_fixture as g

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'oracle/vectors/link_publication_v1.json'
OUTPUT = ROOT / 'addons/RICON_CoreTests/Scripts/Game/RICON_Tests/RICON_LinkPublicationFixtures.c'


def case(cid='P01'):
    for c in json.loads(SOURCE.read_bytes())['cases']:
        if c['id'] == cid:
            return copy.deepcopy(c)
    raise KeyError(cid)


def lines(c):
    return '\n'.join(g.case_lines(0, c))


class LinkPublicationGeneratorTest(unittest.TestCase):
    def test_fixture_and_case_count(self):
        output = g.render(SOURCE.read_bytes())
        self.assertEqual(output, OUTPUT.read_text(encoding='utf-8'))
        self.assertEqual(106, output.count('t.CheckCase('))

    def test_modified_source_does_not_overwrite(self):
        with tempfile.TemporaryDirectory(prefix='ricon-linkpub-generator-') as directory:
            src, dst = Path(directory) / 'source.json', Path(directory) / 'fixture.c'
            src.write_bytes(SOURCE.read_bytes() + b'\n')
            dst.write_bytes(b'existing')
            with contextlib.redirect_stdout(io.StringIO()):
                code = g.main([str(src), str(dst)])
            self.assertEqual(1, code)
            self.assertEqual(b'existing', dst.read_bytes())

    def test_expectations_are_copied(self):
        c = case()
        c['expect']['sequenceCalls'] = 77
        c['expect']['snapshots'][0]['linkSeq'] = 41
        c['expect']['snapshots'][0]['state'] = 'WEAK'
        c['expect']['sourceState']['sequences'] = {'1': 9}
        out = lines(c)
        self.assertIn('expected.m_iSequenceCalls = 77;', out)
        self.assertIn('snapshot0.m_iLinkSeq = 41;', out)
        self.assertIn('snapshot0.m_eState = RICON_ELinkState.WEAK;', out)
        self.assertIn('expectedSource.SetSequence(1, 9);', out)

    def test_null_forms_are_distinct(self):
        c = case()
        c['input']['bindings'] = [None]
        c['input']['pairResult']['pairs'] = None
        c['registry']['entries'] = None
        out = lines(c)
        self.assertIn('RICON_TransmissionBinding inputBindings0;', out)
        self.assertNotIn('inputBindings0 = new', out)
        self.assertIn('pr.m_aPairs = null;', out)
        self.assertIn('registry.m_aEntries = null;', out)
        c = case()
        c['input'] = None
        c['source'] = None
        c['expect']['sourceState'] = None
        out = lines(c)
        self.assertNotIn('input = new', out)
        self.assertNotIn('source = new', out)
        self.assertNotIn('expectedSource = new', out)

    def test_unknown_enum_only_for_device_code(self):
        c = case()
        c['input']['pairResult']['pairs'][0]['deviceCode'] = 'UNKNOWN'
        self.assertIn('prPairs0.m_eDeviceCode = -1;', lines(c))
        for field in ('pairCode', 'state', 'reason'):
            c = case()
            c['input']['pairResult']['pairs'][0][field] = 'UNKNOWN'
            with self.assertRaises(ValueError):
                g.case_lines(0, c)

    def test_special_numbers(self):
        c = case()
        c['input']['qualityDelta'] = 'NaN'
        c['input']['pairResult']['pairs'][0]['sinrDb'] = 'Infinity'
        out = lines(c)
        self.assertIn('input.m_fQualityDelta = RICON_L0Run.Special(0);', out)
        self.assertIn('prPairs0.m_fSinrDb = RICON_L0Run.Special(1);', out)
        c['input']['qualityDelta'] = 'Inf'
        with self.assertRaises(ValueError):
            g.case_lines(0, c)

    def test_injection_and_unknown_fields(self):
        for mode in ('id', 'field', 'int', 'bool', 'source', 'seqkey'):
            c = case()
            if mode == 'id':
                c['id'] = 'P01"); injected();'
            elif mode == 'field':
                c['input']['bindings'][0]['extra'] = 1
            elif mode == 'int':
                c['input']['nowMs'] = 2 ** 31
            elif mode == 'bool':
                c['input']['bindings'][0]['active'] = 1
            elif mode == 'source':
                c['source']['unexpected'] = 1
            else:
                c['source']['sequences'] = {'1); injected(': 1}
            with self.assertRaises(ValueError):
                g.case_lines(0, c)

    def test_expected_source_has_no_injections(self):
        c = case()
        c['expect']['sourceState']['failAt'] = 1
        with self.assertRaises(ValueError):
            g.case_lines(0, c)


if __name__ == '__main__':
    unittest.main()
