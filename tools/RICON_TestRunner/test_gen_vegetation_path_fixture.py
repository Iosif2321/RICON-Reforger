"""P4.3: frozen-вход, отсутствие пересчёта ожиданий, отказ без перезаписи."""
import contextlib
import copy
import io
import json
from pathlib import Path
import tempfile
import unittest

import gen_vegetation_path_fixture as g


class VegetationPathGeneratorTest(unittest.TestCase):
    def test_frozen_fixture_and_host_only(self):
        output=g.build(g.SOURCE.read_bytes())
        self.assertEqual(output,g.TARGET.read_text(encoding='utf-8'))
        self.assertEqual(29,output.count('t.CheckCase('))
        self.assertNotIn('t.CheckCase("P18"',output)
        self.assertNotIn('t.CheckCase("P27"',output)
        self.assertIn('t.CheckCase("P31"',output)

    def test_copies_expected_fields(self):
        c=copy.deepcopy(json.loads(g.SOURCE.read_bytes())['cases'][0])
        indices={json.dumps([key,p],sort_keys=True):i for i,(key,p) in enumerate(c['input']['catalog'].items())}
        c['expect']['totalLossDb']=123.25
        c['expect']['through']['totalDb']=456.5
        output='\n'.join(g.case_lines(0,c,indices))
        self.assertIn('expected.m_fTotalLossDb = 123.25;',output)
        self.assertIn('through.m_fTotalDb = 456.5;',output)
        c['expect']['over']['extra']=1
        with self.assertRaises(ValueError):g.case_lines(0,c,indices)

    def test_refuses_source_drift_without_overwrite(self):
        with tempfile.TemporaryDirectory(prefix='ricon-p43-generator-') as directory:
            source,target=Path(directory)/'source.json',Path(directory)/'fixture.c'
            source.write_bytes(g.SOURCE.read_bytes()+b' ');target.write_text('existing',encoding='utf-8')
            with contextlib.redirect_stdout(io.StringIO()):r=g.main(['--source',str(source),'--target',str(target)])
            self.assertEqual(1,r);self.assertEqual('existing',target.read_text(encoding='utf-8'))

    def test_missing_budget_is_not_a_zero(self):
        c=json.loads(g.SOURCE.read_bytes())['cases'][6]
        indices={json.dumps([key,p],sort_keys=True):i for i,(key,p) in enumerate(c['input']['catalog'].items())}
        output='\n'.join(g.case_lines(0,c,indices))
        self.assertNotIn('query.m_Budget.m_bHasL2 = true;',output)
        self.assertIn('through.m_sReason = "BASE_BUDGET_MISSING";',output)


if __name__=='__main__':unittest.main()
