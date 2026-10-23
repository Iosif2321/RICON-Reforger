"""Границы генератора P3.1: закреплённый SHA, host-only, null-входы, корень графа и отказ без перезаписи."""
import contextlib
import copy
import io
import json
from pathlib import Path
import tempfile
import unittest

import gen_object_rf_profile_fixture as g

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'oracle/vectors/object_rf_profile_v1.json'
OUTPUT = ROOT / 'addons/RICON_CoreTests/Scripts/Game/RICON_Tests/RICON_ObjectRfProfileFixtures.c'


def case(cid):
    for c in json.loads(SOURCE.read_bytes())['cases']:
        if c['id'] == cid:
            return copy.deepcopy(c)
    raise KeyError(cid)


def lines(c):
    return '\n'.join(g.case_lines(0, c))


class ObjectRfProfileGeneratorTest(unittest.TestCase):
    def test_fixture_and_case_count(self):
        output = g.render(SOURCE.read_bytes())
        self.assertEqual(output, OUTPUT.read_text(encoding='utf-8'))
        self.assertEqual(40, output.count('t.CheckCase('))
        self.assertIn('static const int HOST_ONLY = 7;', output)
        for cid in g.HOST_ONLY:
            self.assertNotIn(f'"{cid}"', output)

    def test_modified_source_does_not_overwrite(self):
        with tempfile.TemporaryDirectory(prefix='ricon-objprofile-generator-') as directory:
            src, dst = Path(directory) / 'source.json', Path(directory) / 'fixture.c'
            src.write_bytes(SOURCE.read_bytes().replace(b'\n', b'\r\n'))
            dst.write_bytes(b'existing')
            with contextlib.redirect_stdout(io.StringIO()):
                code = g.main([str(src), str(dst)])
            self.assertEqual(1, code)
            self.assertEqual(b'existing', dst.read_bytes())

    def test_root_and_empty_parent_stay_distinct(self):
        """null-родитель → hasParent=false; пустая строка родителя (R45) → hasParent=true с пустым ID."""
        out = lines(case('R45'))
        self.assertIn(', true, "");', out)
        root = lines(case('R01'))
        self.assertIn(', false, "");', root)

    def test_null_inputs(self):
        self.assertIn('RICON_ObjectProfileInput input;', lines(case('R43')))
        self.assertIn('input.m_aProfiles = null;', lines(case('R37')))
        self.assertIn('input.m_Object = null;', lines(case('R38')))
        self.assertIn('input.m_aPrefabParents = null;', lines(case('R39')))
        self.assertIn('input.m_aClassRules = null;', lines(case('R40')))

    def test_expectations_and_refusals(self):
        out = lines(case('R01'))
        self.assertIn('expected.m_eSource = RICON_EObjectProfileSource.COMPONENT;', out)
        self.assertIn('expected.m_eKnowledge = RICON_EPathKnowledge.PROFILE;', out)
        for mode in ('code', 'source', 'extra'):
            c = case('R01')
            if mode == 'code':
                c['expect']['code'] = 'LOST'
            elif mode == 'source':
                c['expect']['source'] = 'NAME'
            else:
                c['input']['extra'] = 1
            with self.assertRaises(ValueError, msg=mode):
                g.case_lines(0, c)


if __name__ == '__main__':
    unittest.main()
