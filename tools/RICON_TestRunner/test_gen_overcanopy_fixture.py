"""P4.2: генератор переносит ожидания и отказывает при дрейфе freeze без перезаписи."""
import contextlib
import copy
import io
import json
from pathlib import Path
import tempfile
import unittest

import gen_overcanopy_fixture as g


class OvercanopyGeneratorTest(unittest.TestCase):
    def test_frozen_cases(self):
        output = g.build(g.SOURCE.read_bytes())
        self.assertEqual(output, g.TARGET.read_text(encoding='utf-8'))
        self.assertEqual(30, output.count('t.CheckCase('))
        self.assertNotIn('t.CheckCase("O28"', output)
        self.assertIn('t.CheckCase("O31"', output)

    def test_refuses_changed_freeze_without_overwriting(self):
        with tempfile.TemporaryDirectory(prefix='ricon-p42-generator-') as directory:
            source, target = Path(directory) / 'source.json', Path(directory) / 'fixture.c'
            source.write_bytes(g.SOURCE.read_bytes() + b' ')
            target.write_text('existing', encoding='utf-8')
            with contextlib.redirect_stdout(io.StringIO()):
                result = g.main(['--source', str(source), '--target', str(target)])
            self.assertEqual(1, result)
            self.assertEqual('existing', target.read_text(encoding='utf-8'))

    def test_copies_values_and_refuses_extra_fields(self):
        c = copy.deepcopy(json.loads(g.SOURCE.read_bytes())['cases'][0])
        indices = {json.dumps([key, p], sort_keys=True): i for i, (key, p) in enumerate(c['input']['catalog'].items())}
        c['expect']['totalAhDb'] = 123.25
        c['expect']['terminals'][0]['ahDb'] = 456.5
        output = '\n'.join(g.case_lines(0, c, indices))
        self.assertIn('expected.m_fTotalAhDb = 123.25;', output)
        self.assertIn('terminal0.m_fAhDb = 456.5;', output)
        c['expect']['terminals'][0]['invented'] = 1
        with self.assertRaises(ValueError):
            g.case_lines(0, c, indices)

    def test_refuses_bool_as_height(self):
        c = copy.deepcopy(json.loads(g.SOURCE.read_bytes())['cases'][0])
        indices = {json.dumps([key, p], sort_keys=True): i for i, (key, p) in enumerate(c['input']['catalog'].items())}
        c['input']['canopy']['forest.temperate_fig2']['heightM'] = True
        with self.assertRaises(ValueError):
            g.case_lines(0, c, indices)


if __name__ == '__main__':
    unittest.main()
