#!/usr/bin/env python3
"""TOOL_ONLY: копирует frozen корпус P3.2a (material_slab_v1.json rev2) в Enforce; ожидания не вычисляет.
Числа переносятся битовыми образами binary32 (RICON_MaterialSlabRun.F): вход MS03 округляется к binary32 до любой
проверки, граничные случаи корпуса отличаются на один ulp, и разбор десятичных литералов в Enforce не нормативен."""
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

REQ_LF_SHA = '9eeeb8c59571dc75b747253d2f1655475319e5a4c6d7207cc65294a4fb8e5d0d'
CORPUS_LF_SHA = '2f5a4f10f5e1e472ea9eb253bc07230c806ad908002d59ab4db19b90cce1efbd'
CASES = 80
# JSON-only случаи корпуса (native: false): типы/структура JSON недостижимы в типизированном Enforce, а литералы
# около FLT_MAX небезопасны для ToString native (см. память об аварии VM).
HOST_ONLY = {'MS054': 'bool вместо frequencyMHz', 'MS055': 'bool вместо thicknessM', 'MS056': 'bool вместо cosIncidence',
             'MS058': 'лишнее поле', 'MS059': 'число строкой', 'MS068': 'переполнение binary32 (1e39)',
             'MS069': '3.4028236e38 округляется в Inf', 'MS070': 'FLT_MAX вне области: литерал небезопасен в native',
             'MS076': 'отрицательный ноль JSON'}
CODES = ('INVALID_INPUT', 'INVALID_PROFILE', 'OUT_OF_MODEL_DOMAIN', 'NUMERIC_DOMAIN_LIMIT', 'NUMERIC_FAILURE', 'OK')
POLARIZATIONS = ('TE', 'TM')
PROVENANCE = ('SOURCE_DOMAIN', 'ASSUMED_CONFIG', 'TEST_ONLY')
TOP = ('frequencyMHz', 'thicknessM', 'cosIncidence', 'polarization', 'material')
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


def enum_line(target, enum, allowed, value):
    need(type(value) is str, 'Ожидалась строка')
    if value in allowed:
        return f'\t\t{target} = {enum}.{value};'
    return f'\t\t{target} = RICON_MaterialSlabRun.BAD_ENUM; // {value!r}'


def case_lines(index, case):
    need(set(case) == {'id', 'note', 'native', 'provenance', 'input', 'expect'}, 'Неизвестные поля случая')
    need(re.fullmatch('MS0[0-9]{2}', case['id']) is not None, 'Некорректный id')
    inp, exp = case['input'], case['expect']
    need(type(inp) is dict and set(inp) == set(TOP), 'Неизвестные поля входа')
    lines = [f'\tprotected static void Case{index}(RICON_MaterialSlabRun t)', '\t{',
             '\t\tRICON_MaterialSlabInput input = t.NewInput();',
             float_line('input.m_fFrequencyMHz', inp['frequencyMHz']), float_line('input.m_fThicknessM', inp['thicknessM']),
             float_line('input.m_fCosIncidence', inp['cosIncidence']),
             enum_line('input.m_ePolarization', 'RICON_ESlabPolarization', POLARIZATIONS, inp['polarization'])]
    m = inp['material']
    if m is None:
        lines.append('\t\tinput.m_Material = null;')
    else:
        need(type(m) is dict and set(m) == {k for k, _ in MATERIAL} | {'provenance'}, 'Неизвестные поля профиля')
        lines += [float_line(f'input.m_Material.{field}', m[key]) for key, field in MATERIAL]
        lines.append(enum_line('input.m_Material.m_eProvenance', 'RICON_EMaterialProvenance', PROVENANCE, m['provenance']))
    need(type(exp) is dict and exp.get('code') in CODES, 'Неизвестное ожидание')
    lines += ['\t\tRICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();',
              f'\t\texpected.m_eCode = RICON_EMaterialSlabCode.{exp["code"]};']
    if exp['code'] == 'OK':
        need(set(exp) == {'code', 'lossDb', 'provenance'} and exp['provenance'] in PROVENANCE, 'Неверные поля OK')
        lines += [float_line('expected.m_fLossDb', exp['lossDb']),
                  f'\t\texpected.m_eProvenance = RICON_EMaterialProvenance.{exp["provenance"]};']
    else:
        need(set(exp) == {'code'}, 'Отказ содержит только код')
    lines += [f'\t\tt.CheckCase({json.dumps(case["id"])}, input, expected);', '\t}', '']
    return lines


def render(raw):
    normalized = raw.replace(b'\r\n', b'\n')
    need(hashlib.sha256(normalized).hexdigest() == CORPUS_LF_SHA, 'LF-SHA корпуса изменён: требуется взаимное ревью')
    doc = json.loads(normalized)
    need(doc['schema'] == 'ricon.oracle.material_slab' and doc['version'] == 1 and doc['revision'] == 2
         and doc['requirementsLfSha256'] == REQ_LF_SHA and len(doc['cases']) == CASES, 'Неверная ревизия')
    need([c['id'] for c in doc['cases']] == [f'MS{i:03}' for i in range(1, CASES + 1)], 'Неверный набор случаев')
    need(sorted(c['id'] for c in doc['cases'] if not c['native']) == sorted(HOST_ONLY), 'native-флаги не совпали с генератором')
    native = [c for c in doc['cases'] if c['native']]
    lines = ['//! Сгенерировано gen_material_slab_fixture.py; ожидания только копируются; TEST_ONLY.',
             f'//! Host-only (JSON-only в корпусе): {", ".join(f"{k} — {v}" for k, v in HOST_ONLY.items())}.',
             'class RICON_MaterialSlabFixtures', '{', f'\tstatic const string SOURCE_SHA = "{CORPUS_LF_SHA}";',
             f'\tstatic const int CASES = {len(native)};', f'\tstatic const int HOST_ONLY = {len(HOST_ONLY)};', '']
    for i, c in enumerate(native):
        lines += case_lines(i, c)
    lines += ['\tstatic void Run(RICON_MaterialSlabRun t)', '\t{'] + [f'\t\tCase{i}(t);' for i in range(len(native))]
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
        print(f'MATERIAL_SLAB_GENERATOR_FAIL: {error}')
        return 1
    print(f'MATERIAL_SLAB_GENERATOR_PASS: {CASES - len(HOST_ONLY)} native, {len(HOST_ONLY)} host-only')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
