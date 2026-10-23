#!/usr/bin/env python3
"""TOOL_ONLY: копирует frozen корпус P3.2b (material_profile_v1.json rev2) в Enforce; ожидания не вычисляет.
Числа переносятся битовыми образами binary32 (RICON_MaterialProfileRun.F), как в P3.2a: вход MP02 округляется к
binary32 до любой проверки, граничные случаи отличаются на один ulp. Host-only случаи — все с native=false в
корпусе (JSON-типы/структура недостижимы в типизированном Enforce); их причины берутся из note корпуса."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from gen_channel_fading_fixture import int_literal
from gen_radio_device_fixture import need

REQ_LF_SHA = '2385e37bcb24f8183eaee84e477bf6211a17f557c808d7a011998dbdfb9597dd'
CORPUS_LF_SHA = '36f854eaa5cc2b9e1dc10d8ee6e158f4db55f2b4ae6bee7ccfcc38042135ebc2'
CASES = 236
HOST_ONLY_COUNT = 13
CODES = ('INVALID_INPUT', 'INVALID_PROFILE', 'OUT_OF_MODEL_DOMAIN', 'NUMERIC_FAILURE', 'NUMERIC_DOMAIN_LIMIT', 'OK')
PROVENANCE = ('SOURCE_DOMAIN', 'ASSUMED_CONFIG', 'TEST_ONLY')
PROFILE = (('a', 'm_fA'), ('b', 'm_fB'), ('c', 'm_fC'), ('d', 'm_fD'), ('minMHz', 'm_fMinMHz'), ('maxMHz', 'm_fMaxMHz'))
MATERIAL = (('epsilonReal', 'm_fEpsilonReal'), ('conductivitySm', 'm_fConductivitySm'), ('minMHz', 'm_fMinMHz'),
            ('maxMHz', 'm_fMaxMHz'))


def bits(value):
    """Знаковый int32 с битовым образом binary32 ближайшего к value; bool, строки и неконечные отвергаются."""
    need(type(value) in (int, float), 'Ожидалось число')
    try:
        raw = struct.unpack('<I', struct.pack('<f', float(value)))[0]
    except OverflowError:
        raise ValueError('Число вне binary32') from None
    need(raw & 0x7F800000 != 0x7F800000, 'Неконечное binary32')
    return raw - 2 ** 32 if raw >= 2 ** 31 else raw


def float_line(target, value):
    return f'\t\t{target} = t.F({int_literal(bits(value))}); // {value!r}'


def provenance_line(target, value):
    need(type(value) is str, 'Ожидалась строка provenance')
    if value in PROVENANCE:
        return f'\t\t{target} = RICON_EMaterialProvenance.{value};'
    return f'\t\t{target} = RICON_MaterialProfileRun.BAD_ENUM; // {value!r}'


def case_lines(index, case):
    need(set(case) == {'id', 'note', 'native', 'provenance', 'input', 'expect'}, 'Неизвестные поля случая')
    need(re.fullmatch('MP[0-9]{3}', case['id']) is not None, 'Некорректный id')
    inp, exp = case['input'], case['expect']
    need(type(inp) is dict and set(inp) == {'frequencyMHz', 'profile'}, 'Неизвестные поля входа')
    lines = [f'\tprotected static void Case{index}(RICON_MaterialProfileRun t)', '\t{',
             '\t\tRICON_MaterialProfileInput input = t.NewInput();',
             float_line('input.m_fFrequencyMHz', inp['frequencyMHz'])]
    p = inp['profile']
    if p is None:
        lines.append('\t\tinput.m_Profile = null;')
    else:
        need(type(p) is dict and set(p) == {k for k, _ in PROFILE} | {'maxInclusive', 'provenance'}, 'Неизвестные поля профиля')
        need(type(p['maxInclusive']) is bool, 'maxInclusive должен быть bool в native-случае')
        lines += [float_line(f'input.m_Profile.{field}', p[key]) for key, field in PROFILE]
        lines.append(f'\t\tinput.m_Profile.m_bMaxInclusive = {str(p["maxInclusive"]).lower()};')
        lines.append(provenance_line('input.m_Profile.m_eProvenance', p['provenance']))
    need(type(exp) is dict and exp.get('code') in CODES, 'Неизвестное ожидание')
    lines.append(f'\t\tRICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.{exp["code"]});')
    if exp['code'] == 'OK':
        need(set(exp) == {'code', 'material'}, 'OK содержит ровно code и material')
        m = exp['material']
        need(type(m) is dict and set(m) == {k for k, _ in MATERIAL} | {'provenance'} and m['provenance'] in PROVENANCE,
             'Неверные поля material')
        lines += [float_line(f'expected.m_Material.{field}', m[key]) for key, field in MATERIAL]
        lines.append(f'\t\texpected.m_Material.m_eProvenance = RICON_EMaterialProvenance.{m["provenance"]};')
    else:
        need(set(exp) == {'code'}, 'Отказ содержит только код')
    lines += [f'\t\tt.CheckCase({json.dumps(case["id"])}, input, expected);', '\t}', '']
    return lines


def render(raw):
    need('FILL' not in CORPUS_LF_SHA and 'FILL' not in REQ_LF_SHA, 'Генератор не привязан к freeze-SHA')
    normalized = raw.replace(b'\r\n', b'\n')
    need(hashlib.sha256(normalized).hexdigest() == CORPUS_LF_SHA, 'LF-SHA корпуса изменён: требуется взаимное ревью')
    doc = json.loads(normalized)
    need(doc['schema'] == 'ricon.oracle.material_profile' and doc['version'] == 1 and doc['revision'] == 2
         and doc['requirementsLfSha256'] == REQ_LF_SHA and len(doc['cases']) == CASES, 'Неверная ревизия')
    need([c['id'] for c in doc['cases']] == [f'MP{i:03}' for i in range(1, CASES + 1)], 'Неверный набор случаев')
    host_only = {c['id']: c['note'] for c in doc['cases'] if not c['native']}
    need(len(host_only) == HOST_ONLY_COUNT, f'host-only {len(host_only)} != {HOST_ONLY_COUNT}')
    native = [c for c in doc['cases'] if c['native']]
    lines = ['//! Сгенерировано gen_material_profile_fixture.py; ожидания только копируются; TEST_ONLY.',
             f'//! Host-only (native=false в корпусе): {", ".join(f"{k} — {v}" for k, v in host_only.items())}.',
             'class RICON_MaterialProfileFixtures', '{', f'\tstatic const string SOURCE_SHA = "{CORPUS_LF_SHA}";',
             f'\tstatic const int CASES = {len(native)};', f'\tstatic const int HOST_ONLY = {len(host_only)};', '']
    for i, c in enumerate(native):
        lines += case_lines(i, c)
    lines += ['\tstatic void Run(RICON_MaterialProfileRun t)', '\t{'] + [f'\t\tCase{i}(t);' for i in range(len(native))]
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
        print(f'MATERIAL_PROFILE_GENERATOR_FAIL: {error}')
        return 1
    print(f'MATERIAL_PROFILE_GENERATOR_PASS: {CASES - HOST_ONLY_COUNT} native, {HOST_ONLY_COUNT} host-only')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
