"""P3.1: происхождение назначения, отсутствие догадок, глобальная валидация и конечное наследование."""
import copy
import json
from pathlib import Path
import unittest

import object_rf_profile_oracle as op
import build_object_rf_profile_vectors as vectors


class ObjectRfProfileTest(unittest.TestCase):
    def test_component_and_surface_priority(self):
        value = op.baseline()
        self.assertEqual(dict(code='RESOLVED', knowledge='PROFILE', materialId='rf.metal', source='COMPONENT', matchedKey='rf.metal'), op.evaluate(value))
        value['object']['componentProfileId']=''
        self.assertEqual('MATERIAL', op.evaluate(value)['source'])
        self.assertEqual('rf.wood', op.evaluate(value)['materialId'])

    def test_unknown_assignment_never_falls_back(self):
        value = op.baseline()
        value['object']['componentProfileId']='absent'
        result = op.evaluate(value)
        self.assertEqual(dict(code='UNRESOLVED_PROFILE',knowledge='UNKNOWN',materialId='',source='COMPONENT',matchedKey='absent'),result)

    def test_prefab_beats_class_and_nearest_parent_wins(self):
        value = op.baseline()
        value['object'].update(componentProfileId='', surfaceProfileId='')
        self.assertEqual(('PREFAB','base.shed','rf.wood'), tuple(op.evaluate(value)[k] for k in ('source','matchedKey','materialId')))
        value['prefabRules']['mod.shed']='rf.glass'
        self.assertEqual('rf.glass',op.evaluate(value)['materialId'])

    def test_hint_never_becomes_material(self):
        value = op.baseline()
        value['object'].update(componentProfileId='',surfaceProfileId='')
        value.update(prefabRules={},classRules={})
        self.assertEqual(('HINT_ONLY',''),tuple(op.evaluate(value)[k] for k in ('knowledge','materialId')))
        value['object']['ballisticHint']=''
        self.assertEqual('UNKNOWN',op.evaluate(value)['knowledge'])

    def test_unreachable_bad_graph_is_not_masked(self):
        value = op.baseline()
        value['classParents'].update(a='b',b='a')
        self.assertEqual({'code':'INVALID_INPUT'},op.evaluate(value))
        value['classParents'].update(b='absent')
        self.assertEqual({'code':'INVALID_INPUT'},op.evaluate(value))

    def test_long_graph_does_not_use_python_recursion(self):
        value = op.baseline()
        value['object'].update(componentProfileId='',surfaceProfileId='',prefabId='p1499',classId='')
        value.update(prefabParents={'p0':None,**{f'p{i}':f'p{i-1}' for i in range(1,1500)}},classParents={},
                     prefabRules={'p0':'rf.wood'},classRules={})
        self.assertEqual(('RESOLVED','p0'),tuple(op.evaluate(value)[k] for k in ('code','matchedKey')))

    def test_map_order_and_profile_order_do_not_change_choice(self):
        value = op.baseline()
        value['object'].update(componentProfileId='',surfaceProfileId='')
        before = op.evaluate(value)
        for k in ('prefabParents','classParents','prefabRules','classRules'):
            value[k]=dict(reversed(list(value[k].items())))
        value['profiles'].reverse()
        self.assertEqual(before,op.evaluate(value))

    def test_corpus_immutability_and_all_codes(self):
        codes=set()
        for case in vectors.build()['cases']:
            original=copy.deepcopy(case['input'])
            a=op.evaluate(case['input'])
            self.assertEqual(case['expect'],a,case['id'])
            b=op.evaluate(case['input'])
            self.assertIsNot(a,b)
            a['code']='corrupt'
            self.assertEqual(case['expect'],b)
            self.assertEqual(original,case['input'])
            codes.add(b['code'])
        self.assertEqual({'RESOLVED','UNCLASSIFIED','UNRESOLVED_PROFILE','INVALID_INPUT'},codes)

    def test_vectors_are_reproducible(self):
        path = Path(vectors.__file__).with_name('vectors')/'object_rf_profile_v1.json'
        self.assertEqual(vectors.canonical().encode('utf-8'),path.read_bytes())
        self.assertEqual(47,len(json.loads(path.read_bytes())['cases']))


if __name__=='__main__':
    unittest.main()
