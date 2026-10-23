"""Границы P4 generator: freeze, host-only, отсутствие пересчёта ожиданий и отказ без перезаписи."""
import contextlib
import copy
import io
import json
from pathlib import Path
import struct
import tempfile
import unittest

import gen_vegetation_fixture as g


def case(cid):
    return next(c for c in json.loads(g.SOURCE.read_bytes())['cases'] if c['id'] == cid)


class VegetationGeneratorTest(unittest.TestCase):
    def test_frozen_fixture_and_host_only(self):
        output = g.build(g.SOURCE.read_bytes())
        self.assertEqual(output, g.TARGET.read_text(encoding='utf-8'))
        self.assertEqual(58, output.count('t.CheckCase('))
        self.assertNotIn('t.CheckCase("V59"', output)
        self.assertIn('t.CheckCase("V55"', output)

    def test_changed_source_does_not_overwrite(self):
        with tempfile.TemporaryDirectory(prefix='ricon-p4-generator-') as directory:
            source, target = Path(directory) / 'source.json', Path(directory) / 'fixture.c'
            source.write_bytes(g.SOURCE.read_bytes().replace(b'\n', b'\r\n'))
            target.write_text('existing', encoding='utf-8')
            with contextlib.redirect_stdout(io.StringIO()):
                result = g.main(['--source', str(source), '--target', str(target)])
            self.assertEqual(1, result)
            self.assertEqual('existing', target.read_text(encoding='utf-8'))

    def test_expectations_are_copied(self):
        c = copy.deepcopy(case('V03'))
        c['expect']['lossDb'] = 123.25
        indices = {json.dumps([key, p], sort_keys=True): i for i, (key, p) in enumerate(c['input']['catalog'].items())}
        output = '\n'.join(g.case_lines(0, c, indices))
        self.assertIn('expected.m_fLossDb = 123.25;', output)
        c['expect']['invented'] = 1
        with self.assertRaises(ValueError):
            g.case_lines(0, c, indices)

    def test_profile_structure_and_bool_number_are_refused(self):
        p = copy.deepcopy(case('V03')['input']['catalog']['forest.mixed_spb'])
        p['season'] = 'SUMMER'
        with self.assertRaises(ValueError):
            g.profile_lines(0, 'forest.test', p)
        del p['season']
        p['gammaTables']['ANY'][0][1] = True
        with self.assertRaises(ValueError):
            g.profile_lines(0, 'forest.test', p)

    def test_small_float_literal_preserves_binary32(self):
        literal = g.value_literal(1e-35, 'float')
        f32 = lambda v: struct.pack('<f', v)
        self.assertEqual(f32(1e-35), f32(float(literal)))
        self.assertNotEqual(0, float(literal))

    def test_uncovered_interval_is_not_a_loss(self):
        c = case('V55')
        indices = {json.dumps([key, p], sort_keys=True): i for i, (key, p) in enumerate(c['input']['catalog'].items())}
        output = '\n'.join(g.case_lines(0, c, indices))
        self.assertIn('expected.m_Uncovered = uncovered;', output)
        self.assertIn('uncovered.m_sScenario = "S3+";', output)
        self.assertNotIn('expected.m_bHasLoss = true;', output)
        self.assertNotIn('expected.m_aIntervals.Insert(', output)


if __name__ == '__main__':
    unittest.main()
