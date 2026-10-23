#!/usr/bin/env python3
"""TOOL_ONLY: копирует замороженный корпус P3.1 (object_rf_profile_v1.json) в Enforce; ожидания не вычисляет."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from gen_channel_fading_fixture import key_literal
from gen_radio_device_fixture import need

SOURCE_SHA = 'b0b93be3238b49c223c859ac14e22f045ae708ce03f1cae2ed731eaf0531e4c5'
CASES = 47
# Ошибки JSON-типа и лишние поля недостижимы в типизированном Enforce: их проверяет host.
HOST_ONLY = {'R31': 'лишнее поле корня', 'R32': 'лишнее поле объекта', 'R33': 'bool вместо ID профиля',
             'R34': 'null вместо строки назначения', 'R35': 'bool вместо родителя', 'R36': 'число вместо назначения',
             'R41': 'число вместо родителя'}
CODES = ('INVALID_INPUT', 'UNRESOLVED_PROFILE', 'UNCLASSIFIED', 'RESOLVED')
KNOWLEDGE = ('UNKNOWN', 'HINT_ONLY', 'RULE', 'PROFILE')
SOURCES = ('NONE', 'COMPONENT', 'MATERIAL', 'PREFAB', 'CLASS', 'HINT')
OBJECT = (('componentProfileId', 'm_sComponentProfileId'), ('surfaceProfileId', 'm_sSurfaceProfileId'),
          ('prefabId', 'm_sPrefabId'), ('classId', 'm_sClassId'), ('ballisticHint', 'm_sBallisticHint'))
ROOT = {'object', 'profiles', 'prefabParents', 'classParents', 'prefabRules', 'classRules'}


def graph_lines(name, g):
    if g is None:
        return [f'\t\tinput.{name} = null;']
    need(type(g) is dict, 'Ожидался граф')
    lines = []
    for node, parent in g.items():
        need(parent is None or type(parent) is str, 'Родитель — строка или null')
        has = 'false' if parent is None else 'true'
        lines.append(f'\t\tt.AddNode(input.{name}, {key_literal(node)}, {has}, {key_literal(parent or "")});')
    return lines


def rule_lines(name, r):
    if r is None:
        return [f'\t\tinput.{name} = null;']
    need(type(r) is dict and all(type(v) is str for v in r.values()), 'Ожидались правила')
    return [f'\t\tt.AddRule(input.{name}, {key_literal(k)}, {key_literal(v)});' for k, v in r.items()]


def case_lines(index, case):
    need(type(case['id']) is str and re.fullmatch('R[0-9]{2}', case['id']) is not None, 'Некорректный id')
    need(set(case) == {'id', 'title', 'input', 'expect'}, 'Неизвестные поля случая')
    inp, exp = case['input'], case['expect']
    need(type(exp) is dict and exp.get('code') in CODES, 'Неизвестное ожидание')
    lines = [f'\tprotected static void Case{index}(RICON_ObjectRfProfileRun t)', '\t{']
    if inp is None:
        lines.append('\t\tRICON_ObjectProfileInput input;')
    else:
        need(type(inp) is dict and set(inp) == ROOT, 'Неизвестные поля корня')
        lines.append('\t\tRICON_ObjectProfileInput input = t.NewInput();')
        o = inp['object']
        if o is None:
            lines.append('\t\tinput.m_Object = null;')
        else:
            need(type(o) is dict and set(o) == {k for k, _ in OBJECT} and all(type(v) is str for v in o.values()),
                 'Неизвестные поля объекта')
            lines += [f'\t\tinput.m_Object.{f} = {key_literal(o[k])};' for k, f in OBJECT]
        if inp['profiles'] is None:
            lines.append('\t\tinput.m_aProfiles = null;')
        else:
            need(type(inp['profiles']) is list and all(type(p) is str for p in inp['profiles']), 'Ожидался список ID')
            lines += [f'\t\tinput.m_aProfiles.Insert({key_literal(p)});' for p in inp['profiles']]
        lines += graph_lines('m_aPrefabParents', inp['prefabParents']) + graph_lines('m_aClassParents', inp['classParents'])
        lines += rule_lines('m_aPrefabRules', inp['prefabRules']) + rule_lines('m_aClassRules', inp['classRules'])
    lines += ['\t\tRICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();',
              f'\t\texpected.m_eCode = RICON_EObjectProfileCode.{exp["code"]};']
    if exp['code'] == 'INVALID_INPUT':
        need(set(exp) == {'code'}, 'INVALID_INPUT содержит только код')
    else:
        need(set(exp) == {'code', 'knowledge', 'materialId', 'source', 'matchedKey'}, 'Неизвестные поля результата')
        need(exp['knowledge'] in KNOWLEDGE and exp['source'] in SOURCES, 'Неизвестные knowledge/source')
        lines += [f'\t\texpected.m_eKnowledge = RICON_EPathKnowledge.{exp["knowledge"]};',
                  f'\t\texpected.m_sMaterialId = {key_literal(exp["materialId"])};',
                  f'\t\texpected.m_eSource = RICON_EObjectProfileSource.{exp["source"]};',
                  f'\t\texpected.m_sMatchedKey = {key_literal(exp["matchedKey"])};']
    lines += [f'\t\tt.CheckCase({json.dumps(case["id"])}, input, expected);', '\t}', '']
    return lines


def render(raw):
    need(hashlib.sha256(raw).hexdigest() == SOURCE_SHA, 'SHA корпуса изменён: требуется взаимное ревью')
    doc = json.loads(raw)
    need(doc['schema'] == 'ricon-object-rf-profile-v1' and doc['version'] == 1 and len(doc['cases']) == CASES,
         'Неверная ревизия')
    need(sorted(doc['hostOnly']) == sorted(HOST_ONLY), 'hostOnly корпуса не совпал с генератором')
    native = [c for c in doc['cases'] if c['id'] not in HOST_ONLY]
    for c in doc['cases']:
        if c['id'] in HOST_ONLY:
            need(c['expect']['code'] == 'INVALID_INPUT', 'Host-only допустим только для INVALID_INPUT')
    lines = ['//! Сгенерировано gen_object_rf_profile_fixture.py; ожидания только копируются; TEST_ONLY.',
             f'//! Host-only (недостижимы в типизированном Enforce): {", ".join(f"{k} — {v}" for k, v in HOST_ONLY.items())}.',
             'class RICON_ObjectRfProfileFixtures', '{', f'\tstatic const string SOURCE_SHA = "{SOURCE_SHA}";',
             f'\tstatic const int CASES = {len(native)};', f'\tstatic const int HOST_ONLY = {len(HOST_ONLY)};', '']
    for i, c in enumerate(native):
        lines += case_lines(i, c)
    lines += ['\tstatic void Run(RICON_ObjectRfProfileRun t)', '\t{'] + [f'\t\tCase{i}(t);' for i in range(len(native))]
    return '\n'.join(lines + ['\t}', '}', ''])


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('source', type=Path)
    p.add_argument('destination', type=Path)
    p.add_argument('--check', action='store_true')
    args = p.parse_args(argv)
    try:
        output = render(args.source.read_bytes())
        if args.check:
            need(args.destination.read_text(encoding='utf-8') == output, 'Фикстура отличается')
        else:
            args.destination.write_text(output, encoding='utf-8', newline='\n')
    except (OSError, ValueError, KeyError, TypeError) as error:
        print(f'OBJECT_RF_PROFILE_GENERATOR_FAIL: {error}')
        return 1
    print(f'OBJECT_RF_PROFILE_GENERATOR_PASS: {CASES - len(HOST_ONLY)} native, {len(HOST_ONLY)} host-only')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
