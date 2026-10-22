#!/usr/bin/env python3
"""TOOL_ONLY: копирует замороженный корпус S3b (link_publication_v1.json) в Enforce; ожидания не вычисляет."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from gen_radio_device_fixture import need, value_literal

SOURCE_SHA = 'a60ed14590222c103aceea9030fb80ee233eea432b5969b8ed7fe93ad5c10be3'
CASES = 106
CALL_CODES = ('NULL_INPUT', 'INVALID_LIMITS', 'LIMIT_EXCEEDED', 'INVALID_ELEMENT', 'INVALID_REGISTRY', 'DUPLICATE',
              'UPSTREAM_REFUSED', 'IDENTITY_MISMATCH', 'CLOCK_ROLLBACK', 'ACTIVE_LIMIT', 'ID_EXHAUSTED',
              'NEED_FULL_RESYNC', 'OK')
S3_CODES = ('NULL_INPUT', 'INVALID_LIMITS', 'LIMIT_EXCEEDED', 'INVALID_ELEMENT', 'DUPLICATE', 'OK')
PAIR_CODES = ('BINDING_INACTIVE', 'TX_PLAYER_INVALID', 'TX_DEVICE_INVALID', 'SELF_PAIR', 'RX_PLAYER_INVALID',
              'RX_DEVICE_INVALID', 'STATE_MISMATCH', 'DEVICE_QUERY', 'EVALUATED')
DEVICE_CODES = ('NULL_INPUT', 'INVALID_OBSERVATION', 'INVALID_PROFILE', 'INVALID_CONTEXT', 'PROFILE_MISMATCH',
                'POWER_OFF', 'TUNING_MISMATCH', 'CHANNEL_MISMATCH', 'KEY_MISMATCH', 'READY')
REASONS = ('OK', 'INVALID_COORDINATE', 'INVALID_FREQUENCY', 'INVALID_BANDWIDTH', 'INVALID_GAIN', 'INVALID_NOISE_FIGURE',
           'INVALID_SENSITIVITY', 'INVALID_REQUIRED_SINR', 'INVALID_INTERFERENCE', 'CATALOG_VIOLATION', 'UNKNOWN_BAND',
           'UNKNOWN_MODEL', 'UNKNOWN_NOISE_ENVIRONMENT', 'OUT_OF_MODEL_DOMAIN', 'NUMERIC_FAILURE',
           'MODEL_CONTRACT_VIOLATION', 'TERRAIN_PROFILE_UNAVAILABLE')
STATES = ('LOST', 'WEAK', 'DEGRADED', 'CLEAN')
# REQ-LP §6: строки JSON для специальных чисел; S2-помощник знает +Inf/-Inf/NaN.
SPECIAL_ALIASES = {'Infinity': '+Inf', '-Infinity': '-Inf'}


def ints(*names):
    return tuple((n, 'm_i' + n[0].upper() + n[1:], 'int') for n in names)


BINDING = ints('leaseId', 'sessionEpoch', 'txPlayerId', 'txGeneration', 'txDeviceId', 'txTransceiverIndex',
               'networkTypeId', 'channelId', 'issuedAtMs', 'ttlMs') + (('active', 'm_bActive', 'bool'),)
LIMITS = ints('maxBindings', 'maxPairs', 'maxActiveLinks')
PAIR = ints('leaseId', 'txDeviceId', 'rxPlayerId', 'rxDeviceId', 'rxTransceiverIndex') + (
    ('hasDeviceCode', 'm_bHasDeviceCode', 'bool'), ('hasBudget', 'm_bHasBudget', 'bool'), ('valid', 'm_bValid', 'bool'),
    ('intelligibility', 'm_fIntelligibility', 'float'), ('sinrDb', 'm_fSinrDb', 'float'))
ENTRY = ints('leaseId', 'rxDeviceId', 'rxTransceiverIndex', 'rxPlayerId', 'txPlayerId', 'txDeviceId',
             'txTransceiverIndex', 'linkId', 'linkSeq', 'ttlMs', 'publishedAtMs') + (('quality', 'm_fQuality', 'float'),)
SNAPSHOT = ints('schemaVersion', 'sessionEpoch', 'linkId', 'linkSeq', 'ttlMs', 'routeId', 'routeNodeCount',
                'talkerPlayerId', 'listenerPlayerId') + (('quality', 'm_fQuality', 'float'),)


def lit(value, kind):
    if kind == 'float' and type(value) is str:
        value = SPECIAL_ALIASES.get(value, value)
    return value_literal(value, kind)


def enum(value, allowed, cls, unknown=False):
    """Имя enum; неизвестное имя допустимо только там, где REQ кодирует его как −1 (C37)."""
    need(type(value) is str, 'Ожидалось имя enum')
    if value in allowed:
        return cls + '.' + value
    need(unknown and value == 'UNKNOWN', 'Неизвестное имя enum')
    return '-1'


def fields_lines(name, obj, fields, extra):
    need(type(obj) is dict and set(obj) == {f[0] for f in fields} | set(extra), f'Неизвестные поля {name}')
    return [f'\t\t{name}.{field} = {lit(obj[key], kind)};' for key, field, kind in fields]


def object_array(lines, owner, field, cls, values, render):
    """null-массив → null; null-элемент вставляется как null (REQ-LP-03 различает эти случаи)."""
    if values is None:
        lines.append(f'\t\t{owner}.{field} = null;')
        return
    need(type(values) is list, 'Ожидался массив')
    for i, obj in enumerate(values):
        name = f'{owner}{field[3:]}{i}'
        lines.append(f'\t\t{cls} {name};')
        if obj is not None:
            lines.append(f'\t\t{name} = new {cls}();')
            lines.extend(render(name, obj))
        lines.append(f'\t\t{owner}.{field}.Insert({name});')


def pair_lines(name, p):
    lines = fields_lines(name, p, PAIR, ('pairCode', 'deviceCode', 'reason', 'state'))
    lines += [f'\t\t{name}.m_ePairCode = {enum(p["pairCode"], PAIR_CODES, "RICON_EPairCode")};',
              f'\t\t{name}.m_eDeviceCode = {enum(p["deviceCode"], DEVICE_CODES, "RICON_ERadioQueryCode", True)};',
              f'\t\t{name}.m_eReason = {enum(p["reason"], REASONS, "RICON_EReason")};',
              f'\t\t{name}.m_eState = {enum(p["state"], STATES, "RICON_ELinkState")};']
    return lines


def entry_lines(name, e):
    return fields_lines(name, e, ENTRY, ('state',)) + [
        f'\t\t{name}.m_eState = {enum(e["state"], STATES, "RICON_ELinkState")};']


def registry_lines(lines, name, reg):
    lines.append(f'\t\tRICON_LinkRegistry {name};')
    if reg is None:
        return
    need(type(reg) is dict and set(reg) == {'sessionEpoch', 'highWaterLinkId', 'lastTickMs', 'needsFullResync',
                                            'entries'}, 'Неизвестные поля реестра')
    lines += [f'\t\t{name} = new RICON_LinkRegistry();',
              f'\t\t{name}.m_iSessionEpoch = {lit(reg["sessionEpoch"], "int")};',
              f'\t\t{name}.m_iHighWaterLinkId = {lit(reg["highWaterLinkId"], "int")};',
              f'\t\t{name}.m_iLastTickMs = {lit(reg["lastTickMs"], "int")};',
              f'\t\t{name}.m_bNeedsFullResync = {lit(reg["needsFullResync"], "bool")};']
    object_array(lines, name, 'm_aEntries', 'RICON_LinkRegistryEntry', reg['entries'], entry_lines)


def source_lines(lines, name, spec, scenario):
    lines.append(f'\t\tRICON_LinkFakeSource {name};')
    if spec is None:
        return
    allowed = {'epoch', 'sequences'} | ({'failAt', 'rotateAt', 'invalidAt', 'invalidSeq', 'rotatedEpoch'} if scenario else set())
    need(type(spec) is dict and {'epoch', 'sequences'} <= set(spec) <= allowed, 'Неизвестные поля source')
    lines.append(f'\t\t{name} = new RICON_LinkFakeSource({lit(spec["epoch"], "int")});')
    need(type(spec['sequences']) is dict, 'Ожидалась карта seq')
    for link, seq in sorted(spec['sequences'].items(), key=lambda kv: int(kv[0])):
        need(re.fullmatch('[0-9]{1,10}', link) is not None, 'Ключ seq — десятичный linkId')
        lines.append(f'\t\t{name}.SetSequence({lit(int(link), "int")}, {lit(seq, "int")});')
    for key, field in (('failAt', 'm_iFailAt'), ('rotateAt', 'm_iRotateAt'), ('invalidAt', 'm_iInvalidAt'),
                       ('invalidSeq', 'm_iInvalidSeq')):
        if key in spec:
            lines.append(f'\t\t{name}.{field} = {lit(spec[key], "int")};')
    if 'rotatedEpoch' in spec:
        lines.append(f'\t\t{name}.SetRotatedEpoch({lit(spec["rotatedEpoch"], "int")});')


def case_lines(index, case):
    need(type(case['id']) is str and re.fullmatch('[PQTBLCE][0-9]{2,3}', case['id']) is not None, 'Некорректный id')
    need(set(case) == {'id', 'title', 'input', 'registry', 'source', 'expect'}, 'Неизвестные поля случая')
    inp, exp = case['input'], case['expect']
    lines = [f'\tprotected static void Case{index}(RICON_LinkPublicationRun t)', '\t{',
             '\t\tRICON_LinkPublicationInput input;']
    if inp is not None:
        need(set(inp) == {'nowMs', 'sessionEpoch', 'ttlMs', 'qualityDelta', 'limits', 'bindings', 'pairResult'},
             'Неизвестные входы')
        lines.append('\t\tinput = new RICON_LinkPublicationInput();')
        for key, field in (('nowMs', 'm_iNowMs'), ('sessionEpoch', 'm_iSessionEpoch'), ('ttlMs', 'm_iTtlMs')):
            lines.append(f'\t\tinput.{field} = {lit(inp[key], "int")};')
        lines.append(f'\t\tinput.m_fQualityDelta = {lit(inp["qualityDelta"], "float")};')
        lines.append('\t\tRICON_LinkPublicationLimits limits;')
        if inp['limits'] is not None:
            lines.append('\t\tlimits = new RICON_LinkPublicationLimits();')
            lines += fields_lines('limits', inp['limits'], LIMITS, ())
        lines.append('\t\tinput.m_Limits = limits;')
        object_array(lines, 'input', 'm_aBindings', 'RICON_TransmissionBinding', inp['bindings'],
                     lambda n, b: fields_lines(n, b, BINDING, ()))
        pr = inp['pairResult']
        lines.append('\t\tRICON_PairServiceResult pr;')
        if pr is not None:
            need(set(pr) == {'callCode', 's2Calls', 'evaluatorCalls', 'pairs'}, 'Неизвестные поля pairResult')
            lines += ['\t\tpr = new RICON_PairServiceResult();',
                      f'\t\tpr.m_eCallCode = {enum(pr["callCode"], S3_CODES, "RICON_EPairServiceCode")};',
                      f'\t\tpr.m_iS2Calls = {lit(pr["s2Calls"], "int")};',
                      f'\t\tpr.m_iEvaluatorCalls = {lit(pr["evaluatorCalls"], "int")};']
            object_array(lines, 'pr', 'm_aPairs', 'RICON_PairResult', pr['pairs'], pair_lines)
        lines.append('\t\tinput.m_PairResult = pr;')
    registry_lines(lines, 'registry', case['registry'])
    source_lines(lines, 'source', case['source'], True)
    need(set(exp) == {'callCode', 'snapshots', 'registry', 'sequenceCalls', 'epochReads', 'requiresFullResync',
                      'sourceState'} and type(exp['snapshots']) is list, 'Неизвестное ожидание')
    lines += ['\t\tRICON_LinkPublicationResult expected = new RICON_LinkPublicationResult();',
              f'\t\texpected.m_eCallCode = {enum(exp["callCode"], CALL_CODES, "RICON_ELinkPublicationCode")};',
              f'\t\texpected.m_iSequenceCalls = {lit(exp["sequenceCalls"], "int")};',
              f'\t\texpected.m_iEpochReads = {lit(exp["epochReads"], "int")};',
              f'\t\texpected.m_bRequiresFullResync = {lit(exp["requiresFullResync"], "bool")};']
    for i, s in enumerate(exp['snapshots']):
        name = f'snapshot{i}'
        lines += [f'\t\tRICON_LinkSnapshot {name} = new RICON_LinkSnapshot();'] + fields_lines(name, s, SNAPSHOT, ('state',))
        lines += [f'\t\t{name}.m_eState = {enum(s["state"], STATES, "RICON_ELinkState")};',
                  f'\t\texpected.m_aSnapshots.Insert({name});']
    registry_lines(lines, 'expectedRegistry', exp['registry'])
    lines.append('\t\texpected.m_Registry = expectedRegistry;')
    source_lines(lines, 'expectedSource', exp['sourceState'], False)
    lines += [f'\t\tt.CheckCase({json.dumps(case["id"])}, input, registry, source, expected, expectedSource);', '\t}', '']
    return lines


def render(raw):
    need(hashlib.sha256(raw).hexdigest() == SOURCE_SHA, 'SHA корпуса изменён: требуется взаимное ревью')
    d = json.loads(raw)
    need(d['schema'] == 'ricon.link_publication' and d['version'] == 1 and d['revision'] == 1
         and len(d['cases']) == CASES, 'Неверная ревизия')
    lines = ['//! Сгенерировано gen_link_publication_fixture.py; ожидания только копируются; TEST_ONLY.',
             'class RICON_LinkPublicationFixtures', '{', f'\tstatic const string SOURCE_SHA = "{SOURCE_SHA}";',
             f'\tstatic const int CASES = {CASES};', '']
    for i, c in enumerate(d['cases']):
        lines += case_lines(i, c)
    lines += ['\tstatic void Run(RICON_LinkPublicationRun t)', '\t{'] + [f'\t\tCase{i}(t);' for i in range(CASES)]
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
        print(f'LINK_PUBLICATION_GENERATOR_FAIL: {error}')
        return 1
    print(f'LINK_PUBLICATION_GENERATOR_PASS: {CASES}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
