"""Границы генератора S3: закрытые типы, копирование ожиданий и отказ без перезаписи."""
import contextlib
import copy
import io
import json
from pathlib import Path
import tempfile
import unittest

import gen_pair_service_fixture as g

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT/'oracle/vectors/pair_service_v1.json'
OUTPUT = ROOT/'addons/RICON_CoreTests/Scripts/Game/RICON_Tests/RICON_PairServiceFixtures.c'


def sample():
    return copy.deepcopy(json.loads(SOURCE.read_bytes())['cases'][0])


class PairServiceGeneratorTest(unittest.TestCase):
    def test_fixture_and_case_count(self):
        output=g.render(SOURCE.read_bytes())
        self.assertEqual(output,OUTPUT.read_text(encoding='utf-8'))
        self.assertEqual(72,output.count('t.CheckCase('))

    def test_modified_source_does_not_overwrite(self):
        with tempfile.TemporaryDirectory(prefix='ricon-pair-generator-') as directory:
            src,dst=Path(directory)/'source.json',Path(directory)/'fixture.c'
            src.write_bytes(SOURCE.read_bytes()+b'\n'); dst.write_bytes(b'existing')
            with contextlib.redirect_stdout(io.StringIO()):
                code=g.main([str(src),str(dst)])
            self.assertEqual(1,code)
            self.assertEqual(b'existing',dst.read_bytes())

    def test_expectations_are_copied(self):
        case=sample()
        case['expect']['s2Calls']=77
        case['expect']['pairs'][0]['sinrDb']=123.25
        case['expect']['pairs'][0]['pairCode']='DEVICE_QUERY'
        output='\n'.join(g.case_lines(0,case))
        self.assertIn('expected.m_iS2Calls = 77;',output)
        self.assertIn('pair0.m_fSinrDb = 123.25;',output)
        self.assertIn('pair0.m_ePairCode = RICON_EPairCode.DEVICE_QUERY;',output)

    def test_has_evaluator_is_a_bool(self):
        for value in (1,0,'true',None):
            case=sample(); case['input']['hasEvaluator']=value
            with self.assertRaises(ValueError):
                g.case_lines(0,case)

    def test_injection_and_unknown_fields(self):
        for mode in ('id','key','field','int','enum'):
            case=sample()
            if mode=='id': case['id']='P01"); injected();'
            elif mode=='key': case['input']['endpoints'][0]['observation']['networkKey']='x"; injected();'
            elif mode=='field': case['input']['bindings'][0]['extra']=1
            elif mode=='int': case['input']['nowMs']=2**31
            else: case['expect']['pairs'][0]['pairCode']='DELIVERED'
            with self.assertRaises(ValueError):
                g.case_lines(0,case)

    def test_null_arrays_parts_and_objects(self):
        case=sample(); case['input']['bindings']=None
        case['input']['listeners']=[None]
        case['input']['endpoints'][0]['profile']=None
        output='\n'.join(g.case_lines(0,case))
        self.assertIn('input.m_aBindings = null;',output)
        self.assertIn('RICON_PairListener listeners0;',output)
        self.assertNotIn('listeners0 = new',output)
        self.assertIn('RICON_RadioRfProfile endpoints0profile;',output)
        self.assertNotIn('endpoints0profile = new',output)

    def test_invalid_state_has_one_explicit_mapping(self):
        case=sample(); case['input']['players'][0]['state']='ALIVE'
        self.assertIn('players0.m_eState = -1;','\n'.join(g.case_lines(0,case)))
        case['input']['players'][0]['state']='injected()'
        with self.assertRaises(ValueError):
            g.case_lines(0,case)

    def test_numeric_special_and_nonfinite_expectation(self):
        case=sample(); case['input']['endpoints'][0]['observation']['antennaPos'][0]='NaN'
        self.assertIn('RICON_L0Run.Special(0)','\n'.join(g.case_lines(0,case)))
        case['expect']['pairs'][0]['sinrDb']=float('inf')
        with self.assertRaises(ValueError):
            g.case_lines(0,case)


if __name__=='__main__':
    unittest.main()
