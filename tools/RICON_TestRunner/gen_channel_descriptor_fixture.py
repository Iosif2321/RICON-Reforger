#!/usr/bin/env python3
"""TOOL_ONLY: копирует замороженный корпус CT10.1 (channel_descriptor_v1.json) в Enforce; ожидания не вычисляет."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from gen_channel_fading_fixture import int_literal, key_literal
from gen_radio_device_fixture import need, value_literal

SOURCE_SHA = '95208dd198cee4680763e6813cb66374832ef241ea1b5590e20799d834f2aa27'
CASES = 54
# Ошибки JSON-типа и структуры недостижимы в типизированном Enforce: их проверяет host.
HOST_ONLY = {'D17': 'bool вместо бюджета', 'D18': 'дробный бюджет', 'D19': 'bool вместо веса', 'D20': 'лишнее поле epoch',
             'D21': 'пропущенное поле', 'D31': 'bool вместо байта', 'D47': 'bool вместо якоря',
             'D54': 'bool и 999 в списке байтов'}
CODES = ('INVALID_LIMITS', 'INVALID_DESCRIPTOR', 'INVALID_FRAME', 'UNSUPPORTED_VERSION', 'UNSUPPORTED_MODEL',
         'RESOURCE_LIMIT', 'OUT_OF_MODEL_DOMAIN', 'INVALID_ANCHOR', 'OK')
INTS = (('profileVersion', 'm_iProfileVersion'), ('environmentRevision', 'm_iEnvironmentRevision'),
        ('anchorMs', 'm_iAnchorMs'), ('validFromMs', 'm_iValidFromMs'), ('validUntilMs', 'm_iValidUntilMs'))
BOOLS = (('staticChannel', 'm_bStaticChannel'), ('coverageComplete', 'm_bCoverageComplete'),
         ('resourceExhausted', 'm_bResourceExhausted'))
FLOATS = (('frequencyMHz', 'm_fFrequencyMHz'), ('bandwidthHz', 'm_fBandwidthHz'))
FIELDS = {'modelId', 'profileId', 'a', 'b', 'components'} | {k for k, _ in INTS + BOOLS + FLOATS}


def descriptor_lines(name, d):
    """Descriptor в переменную name; null → неинициализированная ссылка."""
    if d is None:
        return [f'\t\tRICON_FadingDescriptor {name};']
    need(type(d) is dict and set(d) == FIELDS, f'Неизвестные поля {name}')
    lines = [f'\t\tRICON_FadingDescriptor {name} = new RICON_FadingDescriptor();',
             f'\t\t{name}.m_sModelId = {key_literal(d["modelId"])};', f'\t\t{name}.m_sProfileId = {key_literal(d["profileId"])};']
    for end, field in (('a', 'm_A'), ('b', 'm_B')):
        e = d[end]
        need(type(e) is dict and set(e) == {'deviceId', 'transceiverIndex'}, 'Неизвестные поля конца')
        lines.append(f'\t\t{name}.{field} = t.End({int_literal(e["deviceId"])}, {int_literal(e["transceiverIndex"])});')
    lines += [f'\t\t{name}.{f} = {int_literal(d[k])};' for k, f in INTS]
    lines += [f'\t\t{name}.{f} = {value_literal(d[k], "bool")};' for k, f in BOOLS]
    lines += [f'\t\t{name}.{f} = {value_literal(d[k], "float")};' for k, f in FLOATS]
    need(type(d['components']) is list, 'Ожидался массив составляющих')
    for c in d['components']:
        need(type(c) is dict and set(c) == {'componentId', 'powerWeight', 'phaseQ', 'dopplerQ'}, 'Неизвестные поля составляющей')
        lines.append(f'\t\tt.Add({name}, {key_literal(c["componentId"])}, {value_literal(c["powerWeight"], "float")}, '
                     f'{int_literal(c["phaseQ"])}, {int_literal(c["dopplerQ"])});')
    return lines


def bytes_lines(name, data):
    if data is None:
        return [f'\t\tarray<int> {name};']
    need(type(data) is list and all(type(b) is int and -2**31 <= b < 2**31 for b in data), 'Ожидался список int')
    if all(0 <= b <= 255 for b in data):
        # Длинный литерал массива превышает лимит инструкций функции Enforce; hex разбирает стенд.
        return [f'\t\tarray<int> {name} = t.Hex("{bytes(data).hex()}");']
    return [f'\t\tarray<int> {name} = {{' + ', '.join(str(b) for b in data) + '};']


def case_lines(index, case):
    need(type(case['id']) is str and re.fullmatch('D[0-9]{2}', case['id']) is not None, 'Некорректный id')
    need(set(case) == {'id', 'title', 'input', 'expect'}, 'Неизвестные поля случая')
    inp, exp = case['input'], case['expect']
    need(type(exp) is dict and exp.get('code') in CODES, 'Неизвестное ожидание')
    if exp['code'] != 'OK':
        need(set(exp) == {'code'}, 'Отказ содержит только код')
    op = inp.get('op')
    lines = [f'\tprotected static void Case{index}(RICON_ChannelDescriptorRun t)', '\t{']
    if op == 'ENCODE':
        need(set(inp) == {'op', 'descriptor', 'maxBytes'}, 'Неизвестные поля Encode')
        lines += descriptor_lines('d', inp['descriptor'])
        call = f't.CheckEncode({json.dumps(case["id"])}, d, {int_literal(inp["maxBytes"])}, expected);'
    elif op == 'DECODE':
        need(set(inp) == {'op', 'bytes', 'maxBytes'}, 'Неизвестные поля Decode')
        lines += bytes_lines('bytes', inp['bytes'])
        call = f't.CheckDecode({json.dumps(case["id"])}, bytes, {int_literal(inp["maxBytes"])}, expected);'
    elif op == 'REBASE':
        need(set(inp) == {'op', 'descriptor', 'anchorMs'}, 'Неизвестные поля Rebase')
        lines += descriptor_lines('d', inp['descriptor'])
        call = f't.CheckRebase({json.dumps(case["id"])}, d, {int_literal(inp["anchorMs"])}, expected);'
    else:
        raise ValueError('Неизвестная операция')
    lines += ['\t\tRICON_DescriptorResult expected = new RICON_DescriptorResult();',
              f'\t\texpected.m_eCode = RICON_EDescriptorCode.{exp["code"]};']
    if exp['code'] == 'OK':
        want = {'code', 'descriptor', 'bytes', 'sizeBytes'} if op != 'REBASE' else {'code', 'descriptor'}
        need(set(exp) == want, 'Неизвестные поля OK')
        lines += descriptor_lines('expectedDescriptor', exp['descriptor']) + ['\t\texpected.m_Descriptor = expectedDescriptor;']
        if op != 'REBASE':
            need(exp['sizeBytes'] == len(exp['bytes']), 'sizeBytes не равен длине')
            lines += bytes_lines('expectedBytes', exp['bytes'])
            lines += ['\t\texpected.m_aBytes = expectedBytes;', f'\t\texpected.m_iSizeBytes = {exp["sizeBytes"]};']
    lines += [f'\t\t{call}', '\t}', '']
    return lines


def render(raw):
    need(hashlib.sha256(raw).hexdigest() == SOURCE_SHA, 'SHA корпуса изменён: требуется взаимное ревью')
    doc = json.loads(raw)
    need(doc['schema'] == 'ricon-channel-descriptor-v1' and doc['version'] == 1 and len(doc['cases']) == CASES,
         'Неверная ревизия')
    need(sorted(doc['hostOnly']) == sorted(HOST_ONLY), 'hostOnly корпуса не совпал с генератором')
    golden = bytes.fromhex(doc['goldenHex'])
    native, host = [], []
    for c in doc['cases']:
        if c['id'] in HOST_ONLY:
            host.append(c['id'])
        else:
            native.append(c)
    need(sorted(host) == sorted(HOST_ONLY), 'Список host-only не совпал с корпусом')
    lines = ['//! Сгенерировано gen_channel_descriptor_fixture.py; ожидания только копируются; TEST_ONLY.',
             f'//! Host-only (недостижимы в типизированном Enforce): {", ".join(f"{k} — {v}" for k, v in HOST_ONLY.items())}.',
             'class RICON_ChannelDescriptorFixtures', '{', f'\tstatic const string SOURCE_SHA = "{SOURCE_SHA}";',
             f'\tstatic const int CASES = {len(native)};', f'\tstatic const int HOST_ONLY = {len(host)};', '',
             '\t//! Корневой goldenHex корпуса: вручную размеченный кадр CD07.',
             '\tstatic array<int> Golden()', '\t{', '\t\tarray<int> golden = {' + ', '.join(str(b) for b in golden) + '};',
             '\t\treturn golden;', '\t}', '']
    for i, c in enumerate(native):
        lines += case_lines(i, c)
    lines += ['\tstatic void Run(RICON_ChannelDescriptorRun t)', '\t{'] + [f'\t\tCase{i}(t);' for i in range(len(native))]
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
        print(f'CHANNEL_DESCRIPTOR_GENERATOR_FAIL: {error}')
        return 1
    print(f'CHANNEL_DESCRIPTOR_GENERATOR_PASS: {CASES - len(HOST_ONLY)} native, {len(HOST_ONLY)} host-only')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
