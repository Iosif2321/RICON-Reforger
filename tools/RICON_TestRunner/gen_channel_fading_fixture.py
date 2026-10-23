#!/usr/bin/env python3
"""TOOL_ONLY: копирует замороженный корпус P1b (channel_fading_v1.json) в Enforce; ожидания не вычисляет."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from gen_radio_device_fixture import need, value_literal

SOURCE_SHA = 'cf5030f6a4ab5e03b5c02ef63aa4211a012b6a36a059ee2b11b519491a925f61'
CASES = 71
# Ошибки JSON-типа, лишние/пропущенные поля и выход за int32 недостижимы в типизированном Enforce: их проверяет host.
HOST_ONLY = {'F27': 'bool вместо времени', 'F28': 'bool вместо веса', 'F29': 'лишнее поле JSON',
             'F30': 'пропущено поле JSON', 'F57': 'время вне int32', 'F70': '1000.0 не JSON-int'}
CODES = ('INVALID_INPUT', 'UNSUPPORTED_MODEL', 'RESOURCE_EXHAUSTED', 'INCOMPLETE_COVERAGE', 'CONTEXT_MISMATCH',
         'STALE_DESCRIPTOR', 'OUT_OF_MODEL_DOMAIN', 'EMPTY_MEDIUM', 'NUMERIC_FAILURE', 'ZERO_AT_SAMPLE', 'OK')
DESC_INTS = (('profileVersion', 'm_iProfileVersion'), ('environmentRevision', 'm_iEnvironmentRevision'),
             ('anchorMs', 'm_iAnchorMs'), ('validFromMs', 'm_iValidFromMs'), ('validUntilMs', 'm_iValidUntilMs'))
DESC_BOOLS = (('staticChannel', 'm_bStaticChannel'), ('coverageComplete', 'm_bCoverageComplete'),
              ('resourceExhausted', 'm_bResourceExhausted'))
DESC_FLOATS = (('frequencyMHz', 'm_fFrequencyMHz'), ('bandwidthHz', 'm_fBandwidthHz'))
DESC_FIELDS = {'modelId', 'profileId', 'a', 'b', 'components'} | {k for k, _ in DESC_INTS + DESC_BOOLS + DESC_FLOATS}
QUERY_INTS = (('profileVersion', 'm_iProfileVersion'), ('environmentRevision', 'm_iEnvironmentRevision'),
              ('nowMs', 'm_iNowMs'), ('subUs', 'm_iSubUs'))
QUERY_FIELDS = {'tx', 'rx', 'profileId', 'frequencyMHz', 'bandwidthHz'} | {k for k, _ in QUERY_INTS}


def int_literal(value):
    """INT_MIN нельзя записать литералом −2147483648: унарный минус применился бы к 2147483648."""
    text = value_literal(value, 'int')
    return '(-2147483647 - 1)' if value == -2**31 else text


def key_literal(value):
    """Ключ копируется как есть, включая недопустимые для REQ (пустой, 65 символов, '/'): их отвергает ядро."""
    need(type(value) is str and re.fullmatch(r'[ -!#-\[\]-~]{0,128}', value) is not None, 'Недопустимая строка')
    return json.dumps(value)


def endpoint_lines(name, value):
    need(type(value) is dict and set(value) == {'deviceId', 'transceiverIndex'}, f'Неизвестные поля {name}')
    return [f'\t\tRICON_FadingEndpoint {name} = new RICON_FadingEndpoint();',
            f'\t\t{name}.m_iDeviceId = {int_literal(value["deviceId"])};',
            f'\t\t{name}.m_iTransceiverIndex = {int_literal(value["transceiverIndex"])};']


def case_lines(index, case):
    need(type(case['id']) is str and re.fullmatch('F[0-9]{2}', case['id']) is not None, 'Некорректный id')
    need(set(case) == {'id', 'title', 'input', 'expect'}, 'Неизвестные поля случая')
    inp, exp = case['input'], case['expect']
    need(type(inp) is dict and set(inp) == {'descriptor', 'query'}, 'Неизвестные входы')
    d, q = inp['descriptor'], inp['query']
    need(type(d) is dict and set(d) == DESC_FIELDS and type(q) is dict and set(q) == QUERY_FIELDS, 'Неизвестные поля')
    need(type(exp) is dict and set(exp) == {'code', 'real', 'imag', 'gainDb', 'phasesQ'} and exp['code'] in CODES,
         'Неизвестное ожидание')
    need((exp['real'] is None) == (exp['imag'] is None), 'real и imag задаются вместе')
    lines = [f'\tprotected static void Case{index}(RICON_ChannelFadingRun t)', '\t{']
    lines += endpoint_lines('a', d['a']) + endpoint_lines('b', d['b'])
    lines += ['\t\tRICON_FadingDescriptor d = new RICON_FadingDescriptor();',
              f'\t\td.m_sModelId = {key_literal(d["modelId"])};', f'\t\td.m_sProfileId = {key_literal(d["profileId"])};',
              '\t\td.m_A = a;', '\t\td.m_B = b;']
    lines += [f'\t\td.{f} = {int_literal(d[k])};' for k, f in DESC_INTS]
    lines += [f'\t\td.{f} = {value_literal(d[k], "bool")};' for k, f in DESC_BOOLS]
    lines += [f'\t\td.{f} = {value_literal(d[k], "float")};' for k, f in DESC_FLOATS]
    need(type(d['components']) is list, 'Ожидался массив составляющих')
    for i, c in enumerate(d['components']):
        need(type(c) is dict and set(c) == {'componentId', 'powerWeight', 'phaseQ', 'dopplerQ'}, 'Неизвестные поля составляющей')
        lines.append(f'\t\tt.Add(d, {key_literal(c["componentId"])}, {value_literal(c["powerWeight"], "float")}, '
                     f'{int_literal(c["phaseQ"])}, {int_literal(c["dopplerQ"])});')
    lines += endpoint_lines('tx', q['tx']) + endpoint_lines('rx', q['rx'])
    lines += ['\t\tRICON_FadingQuery q = new RICON_FadingQuery();', '\t\tq.m_Tx = tx;', '\t\tq.m_Rx = rx;',
              f'\t\tq.m_sProfileId = {key_literal(q["profileId"])};',
              f'\t\tq.m_fFrequencyMHz = {value_literal(q["frequencyMHz"], "float")};',
              f'\t\tq.m_fBandwidthHz = {value_literal(q["bandwidthHz"], "float")};']
    lines += [f'\t\tq.{f} = {int_literal(q[k])};' for k, f in QUERY_INTS]
    lines += ['\t\tRICON_FadingResult expected = new RICON_FadingResult();',
              f'\t\texpected.m_eCode = RICON_EFadingCode.{exp["code"]};']
    if exp['real'] is not None:
        lines += ['\t\texpected.m_bHasCoefficient = true;',
                  f'\t\texpected.m_fReal = {value_literal(exp["real"], "float")};',
                  f'\t\texpected.m_fImag = {value_literal(exp["imag"], "float")};']
    if exp['gainDb'] is not None:
        lines += ['\t\texpected.m_bHasGain = true;', f'\t\texpected.m_fGainDb = {value_literal(exp["gainDb"], "float")};']
    need(type(exp['phasesQ']) is list, 'Ожидался массив фаз')
    lines += [f'\t\texpected.m_aPhasesQ.Insert({int_literal(p)});' for p in exp['phasesQ']]
    lines += [f'\t\tt.CheckCase({json.dumps(case["id"])}, d, q, expected);', '\t}', '']
    return lines


def render(raw):
    need(hashlib.sha256(raw).hexdigest() == SOURCE_SHA, 'SHA корпуса изменён: требуется взаимное ревью')
    doc = json.loads(raw)
    need(doc['schema'] == 'ricon-channel-fading-v1' and doc['version'] == 1 and len(doc['cases']) == CASES,
         'Неверная ревизия')
    need(sorted(doc['hostOnly']) == sorted(HOST_ONLY), 'hostOnly корпуса не совпал с генератором')
    native, host = [], []
    for c in doc['cases']:
        if c['id'] in HOST_ONLY:
            need(c['expect']['code'] == 'INVALID_INPUT', 'Host-only допустим только для INVALID_INPUT')
            host.append(c['id'])
        else:
            native.append(c)
    need(sorted(host) == sorted(HOST_ONLY), 'Список host-only не совпал с корпусом')
    lines = ['//! Сгенерировано gen_channel_fading_fixture.py; ожидания только копируются; TEST_ONLY.',
             f'//! Host-only (недостижимы в типизированном Enforce): {", ".join(f"{k} — {v}" for k, v in HOST_ONLY.items())}.',
             'class RICON_ChannelFadingFixtures', '{', f'\tstatic const string SOURCE_SHA = "{SOURCE_SHA}";',
             f'\tstatic const int CASES = {len(native)};', f'\tstatic const int HOST_ONLY = {len(host)};', '']
    for i, c in enumerate(native):
        lines += case_lines(i, c)
    lines += ['\tstatic void Run(RICON_ChannelFadingRun t)', '\t{'] + [f'\t\tCase{i}(t);' for i in range(len(native))]
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
        print(f'CHANNEL_FADING_GENERATOR_FAIL: {error}')
        return 1
    print(f'CHANNEL_FADING_GENERATOR_PASS: {CASES - len(HOST_ONLY)} native, {len(HOST_ONLY)} host-only')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
