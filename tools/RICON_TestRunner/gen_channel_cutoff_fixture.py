#!/usr/bin/env python3
"""TOOL_ONLY: копирует замороженный корпус P1 (channel_cutoff_v1.json) в Enforce; ожидания не вычисляет."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from gen_radio_device_fixture import need, value_literal

SOURCE_SHA = 'b4ff545d34de7859aae3d71678689a727c835c57f46d429df1a722d492c654d8'
CASES = 27
# Ошибки JSON-типа и лишние поля недостижимы в типизированном Enforce: их проверяют host unit-тесты эталона.
HOST_ONLY = {'C23': 'bool вместо времени', 'C24': 'bool вместо мощности', 'C26': 'лишнее поле JSON'}
DECISIONS = ('UNRESOLVED', 'CONTINUE', 'BOUNDED')
REASONS = ('INVALID_INPUT', 'RESOURCE_EXHAUSTED', 'INCOMPLETE_COVERAGE', 'STALE_BOUND', 'EMPTY_PATH_SET',
           'MISSING_PATH_BOUND', 'NUMERIC_UNCERTIFIED', 'NUMERIC_FAILURE', 'CUTOFF_DISABLED', 'BELOW_TOLERANCE',
           'ABOVE_TOLERANCE')
INTS = (('nowMs', 'm_iNowMs'), ('validFromMs', 'm_iValidFromMs'), ('validUntilMs', 'm_iValidUntilMs'),
        ('environmentRevision', 'm_iEnvironmentRevision'), ('boundRevision', 'm_iBoundRevision'))
BOOLS = (('coverageComplete', 'm_bCoverageComplete'), ('resourceExhausted', 'm_bResourceExhausted'))
FIELDS = {k for k, _ in INTS} | {k for k, _ in BOOLS} | {'toleranceDbm', 'numericalGuardDb', 'paths'}


def optional_float(lines, target, has_field, value_field, value):
    """null → явный флаг false (REQ-LE-16), иначе флаг и литерал binary32."""
    if value is None:
        lines.append(f'\t\t{target}.{has_field} = false;')
        return
    lines += [f'\t\t{target}.{has_field} = true;', f'\t\t{target}.{value_field} = {value_literal(value, "float")};']


def case_lines(index, case):
    need(type(case['id']) is str and re.fullmatch('C[0-9]{2}', case['id']) is not None, 'Некорректный id')
    need(set(case) == {'id', 'title', 'input', 'expect'}, 'Неизвестные поля случая')
    inp, exp = case['input'], case['expect']
    need(type(inp) is dict and set(inp) == FIELDS, 'Неизвестные входы')
    need(type(exp) is dict and set(exp) == {'decision', 'reason', 'usefulIsZero', 'upperDbm'}, 'Неизвестное ожидание')
    need(exp['decision'] in DECISIONS and exp['reason'] in REASONS and type(exp['usefulIsZero']) is bool,
         'Неизвестное решение')
    lines = [f'\tprotected static void Case{index}(RICON_ChannelCutoffRun t)', '\t{',
             '\t\tRICON_CutoffInput input = new RICON_CutoffInput();']
    lines += [f'\t\tinput.{f} = {value_literal(inp[k], "int")};' for k, f in INTS]
    lines += [f'\t\tinput.{f} = {value_literal(inp[k], "bool")};' for k, f in BOOLS]
    optional_float(lines, 'input', 'm_bHasTolerance', 'm_fToleranceDbm', inp['toleranceDbm'])
    optional_float(lines, 'input', 'm_bHasNumericalGuard', 'm_fNumericalGuardDb', inp['numericalGuardDb'])
    need(type(inp['paths']) is list, 'Ожидался массив путей')
    for i, p in enumerate(inp['paths']):
        need(type(p) is dict and set(p) == {'pathId', 'upperDbm'}, 'Неизвестные поля пути')
        name = f'path{i}'
        lines += [f'\t\tRICON_CutoffPathBound {name} = new RICON_CutoffPathBound();',
                  f'\t\t{name}.m_sPathId = {value_literal(p["pathId"], "string")};']
        optional_float(lines, name, 'm_bHasUpper', 'm_fUpperDbm', p['upperDbm'])
        lines.append(f'\t\tinput.m_aPaths.Insert({name});')
    lines += ['\t\tRICON_CutoffResult expected = new RICON_CutoffResult();',
              f'\t\texpected.m_eDecision = RICON_ECutoffDecision.{exp["decision"]};',
              f'\t\texpected.m_eReason = RICON_ECutoffReason.{exp["reason"]};',
              f'\t\texpected.m_bUsefulIsZero = {value_literal(exp["usefulIsZero"], "bool")};']
    optional_float(lines, 'expected', 'm_bHasUpper', 'm_fUpperDbm', exp['upperDbm'])
    lines += [f'\t\tt.CheckCase({json.dumps(case["id"])}, input, expected);', '\t}', '']
    return lines


def render(raw):
    need(hashlib.sha256(raw).hexdigest() == SOURCE_SHA, 'SHA корпуса изменён: требуется взаимное ревью')
    d = json.loads(raw)
    need(d['schema'] == 'ricon.oracle.channel_cutoff' and d['version'] == 1 and len(d['cases']) == CASES,
         'Неверная ревизия')
    native, host = [], []
    for c in d['cases']:
        if c['id'] in HOST_ONLY:
            need(c['expect']['decision'] == 'UNRESOLVED' and c['expect']['reason'] == 'INVALID_INPUT',
                 'Host-only допустим только для INVALID_INPUT')
            host.append(c['id'])
        else:
            native.append(c)
    need(sorted(host) == sorted(HOST_ONLY), 'Список host-only не совпал с корпусом')
    lines = ['//! Сгенерировано gen_channel_cutoff_fixture.py; ожидания только копируются; TEST_ONLY.',
             f'//! Host-only (ошибки JSON-типа, недостижимы в Enforce): {", ".join(f"{k} — {v}" for k, v in HOST_ONLY.items())}.',
             'class RICON_ChannelCutoffFixtures', '{', f'\tstatic const string SOURCE_SHA = "{SOURCE_SHA}";',
             f'\tstatic const int CASES = {len(native)};', f'\tstatic const int HOST_ONLY = {len(host)};', '']
    for i, c in enumerate(native):
        lines += case_lines(i, c)
    lines += ['\tstatic void Run(RICON_ChannelCutoffRun t)', '\t{'] + [f'\t\tCase{i}(t);' for i in range(len(native))]
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
        print(f'CHANNEL_CUTOFF_GENERATOR_FAIL: {error}')
        return 1
    print(f'CHANNEL_CUTOFF_GENERATOR_PASS: {CASES - len(HOST_ONLY)} native, {len(HOST_ONLY)} host-only')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
