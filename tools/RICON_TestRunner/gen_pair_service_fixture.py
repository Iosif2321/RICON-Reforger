#!/usr/bin/env python3
"""TOOL_ONLY: копирует замороженный корпус S3 в Enforce; ожидания не вычисляет."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from gen_radio_device_fixture import object_lines, value_literal, need, OBSERVATION, PROFILE, CONTEXT

SOURCE_SHA = 'fcc9cefa1b8bb46374b67883c239ffc8c6c00343da49a6bf008cf679cac178e1'
CASES = 72
CALL_CODES = ('NULL_INPUT', 'INVALID_LIMITS', 'LIMIT_EXCEEDED', 'INVALID_ELEMENT', 'DUPLICATE', 'OK')
PAIR_CODES = ('BINDING_INACTIVE', 'TX_PLAYER_INVALID', 'TX_DEVICE_INVALID', 'SELF_PAIR', 'RX_PLAYER_INVALID',
              'RX_DEVICE_INVALID', 'STATE_MISMATCH', 'DEVICE_QUERY', 'EVALUATED')
STATES = ('NONE', 'CONNECTED_UNREGISTERED', 'REGISTERED_UNBOUND', 'BOUND_ACTIVE', 'BOUND_INACTIVE', 'DISCONNECTED')
REASONS = ('NUMERIC_FAILURE', 'OK', 'UNKNOWN_MODEL', 'UNKNOWN_NOISE_ENVIRONMENT')
BINDING = tuple((k, 'm_i'+v, 'int') for k,v in (
    ('leaseId','LeaseId'), ('sessionEpoch','SessionEpoch'), ('txPlayerId','TxPlayerId'), ('txGeneration','TxGeneration'),
    ('txDeviceId','TxDeviceId'), ('txTransceiverIndex','TxTransceiverIndex'), ('networkTypeId','NetworkTypeId'),
    ('channelId','ChannelId'), ('issuedAtMs','IssuedAtMs'), ('ttlMs','TtlMs'))) + (('active','m_bActive','bool'),)
LISTENER = tuple((k,'m_i'+v,'int') for k,v in (('playerId','PlayerId'), ('generation','Generation'),
    ('sessionEpoch','SessionEpoch'), ('deviceId','DeviceId'), ('transceiverIndex','TransceiverIndex')))
LIMITS = tuple((k,'m_i'+k[0].upper()+k[1:],'int') for k in (
    'maxBindings','maxListeners','maxPlayers','maxDevices','maxEndpoints','maxPairs'))
PAIR = tuple((k,'m_i'+v,'int') for k,v in (('leaseId','LeaseId'), ('txDeviceId','TxDeviceId'),
    ('rxPlayerId','RxPlayerId'), ('rxDeviceId','RxDeviceId'), ('rxTransceiverIndex','RxTransceiverIndex'))) + (
    ('hasDeviceCode','m_bHasDeviceCode','bool'), ('hasBudget','m_bHasBudget','bool'), ('valid','m_bValid','bool'),
    ('intelligibility','m_fIntelligibility','float'), ('sinrDb','m_fSinrDb','float'))


def enum(value, allowed, cls):
    need(type(value) is str and value in allowed, 'Неизвестное имя enum')
    return cls+'.'+value


def row(name, cls, fields, value):
    return object_lines(name, cls, fields, value)


def case_lines(index, case):
    need(type(case['id']) is str and re.fullmatch('[PCBSE][0-9]{2}', case['id']) is not None, 'Некорректный id')
    inp, exp = case['input'], case['expect']
    need(set(inp) == {'nowMs','sessionEpoch','limits','bindings','listeners','players','devices','endpoints','context','hasEvaluator'},
         'Неизвестные входы')
    lines = [f'\tprotected static void Case{index}(RICON_PairServiceRun t)', '\t{',
             '\t\tRICON_PairServiceInput input = new RICON_PairServiceInput();']
    for key, field in (('nowMs','m_iNowMs'), ('sessionEpoch','m_iSessionEpoch')):
        lines += [f'\t\tinput.{field} = {value_literal(inp[key], "int")};']
    lines += row('limits','RICON_PairServiceLimits',LIMITS,inp['limits'])
    lines += ['\t\tinput.m_Limits = limits;']
    lines += row('context','RICON_RadioQueryContext',CONTEXT,inp['context'])
    lines += ['\t\tinput.m_Context = context;']
    for key, cls, fields in (('bindings','RICON_TransmissionBinding',BINDING), ('listeners','RICON_PairListener',LISTENER)):
        array = 'input.m_a'+key[0].upper()+key[1:]
        if inp[key] is None:
            lines += [f'\t\t{array} = null;']; continue
        need(type(inp[key]) is list, 'Ожидался массив')
        for i, obj in enumerate(inp[key]):
            name = key+str(i)
            lines += row(name,cls,fields,obj) + [f'\t\t{array}.Insert({name});']
    for key in ('players','devices','endpoints'):
        array = 'input.m_a'+key[0].upper()+key[1:]
        if inp[key] is None:
            lines += [f'\t\t{array} = null;']; continue
        need(type(inp[key]) is list, 'Ожидался массив')
        for i, obj in enumerate(inp[key]):
            name=key+str(i)
            cls={'players':'RICON_PairPlayer','devices':'RICON_LifecycleDevice','endpoints':'RICON_PairEndpoint'}[key]
            lines += [f'\t\t{cls} {name};']
            if obj is None:
                lines += [f'\t\t{array}.Insert({name});']; continue
            lines += [f'\t\t{name} = new {cls}();']
            if key == 'players':
                need(set(obj)=={'playerId','generation','state'}, 'Неизвестные поля игрока')
                for k,f in (('playerId','m_iPlayerId'), ('generation','m_iGeneration')):
                    lines += [f'\t\t{name}.{f} = {value_literal(obj[k], "int")};']
                # ALIVE вне enum: типизированный стенд представляет его невалидным значением -1 (C18).
                state='-1' if obj['state']=='ALIVE' else enum(obj['state'],STATES,'RICON_EPlayerLifecycleState')
                lines += [f'\t\t{name}.m_eState = {state};']
            elif key == 'devices':
                need(set(obj)=={'deviceId','kind','ownerPlayerId','powered','accessPlayerIds','allowed'}, 'Неизвестные поля рации')
                for k,f,kind in (('deviceId','m_iDeviceId','int'), ('ownerPlayerId','m_iOwnerPlayerId','int'), ('powered','m_bPowered','bool')):
                    lines += [f'\t\t{name}.{f} = {value_literal(obj[k],kind)};']
                lines += [f'\t\t{name}.m_eKind = {enum(obj["kind"],("CARRIED","WORLD"),"RICON_EDeviceKind")};']
                need(type(obj['accessPlayerIds']) is list and type(obj['allowed']) is list, 'Ожидались массивы политики')
                for pid in obj['accessPlayerIds']:
                    lines += [f'\t\t{name}.m_aAccessPlayerIds.Insert({value_literal(pid,"int")});']
                for pair in obj['allowed']:
                    need(type(pair) is list and len(pair)==2, 'Ожидалась пара сети и канала')
                    lines += [f'\t\t{name}.m_aAllowed.Insert(new RICON_LifecycleCatalogPair({value_literal(pair[0],"int")}, {value_literal(pair[1],"int")}));']
            else:
                need(set(obj)=={'observation','profile'}, 'Неизвестные поля endpoint')
                for k,c,fields in (('observation','RICON_RadioObservation',OBSERVATION), ('profile','RICON_RadioRfProfile',PROFILE)):
                    part=name+k
                    lines += row(part,c,fields,obj[k])
                    field='m_Observation' if k=='observation' else 'm_Profile'
                    lines += [f'\t\t{name}.{field} = {part};']
            lines += [f'\t\t{array}.Insert({name});']
    need(set(exp)=={'callCode','pairs','s2Calls','evaluatorCalls'} and type(exp['pairs']) is list, 'Неизвестное ожидание')
    lines += ['\t\tRICON_PairServiceResult expected = new RICON_PairServiceResult();',
              f'\t\texpected.m_eCallCode = {enum(exp["callCode"],CALL_CODES,"RICON_EPairServiceCode")};']
    for k,f in (('s2Calls','m_iS2Calls'), ('evaluatorCalls','m_iEvaluatorCalls')):
        lines += [f'\t\texpected.{f} = {value_literal(exp[k],"int")};']
    for i,p in enumerate(exp['pairs']):
        need(set(p)=={k for k,_,_ in PAIR}|{'pairCode','deviceCode','reason','state'}, 'Неизвестные поля пары')
        name='pair'+str(i)
        partial={k:p[k] for k,_,_ in PAIR}
        lines += row(name,'RICON_PairResult',PAIR,partial)
        for k,f,allowed,cls in (('pairCode','m_ePairCode',PAIR_CODES,'RICON_EPairCode'),
                               ('deviceCode','m_eDeviceCode',('NULL_INPUT','INVALID_OBSERVATION','INVALID_PROFILE','INVALID_CONTEXT',
                                'PROFILE_MISMATCH','POWER_OFF','TUNING_MISMATCH','CHANNEL_MISMATCH','KEY_MISMATCH','READY'),'RICON_ERadioQueryCode'),
                               ('reason','m_eReason',REASONS,'RICON_EReason'),
                               ('state','m_eState',('LOST','WEAK','DEGRADED','CLEAN'),'RICON_ELinkState')):
            lines += [f'\t\t{name}.{f} = {enum(p[k],allowed,cls)};']
        lines += [f'\t\texpected.m_aPairs.Insert({name});']
    evaluator=value_literal(inp['hasEvaluator'],'bool')
    lines += [f'\t\tt.CheckCase({json.dumps(case["id"])}, input, {evaluator}, expected);', '\t}', '']
    return lines


def render(raw):
    need(hashlib.sha256(raw).hexdigest()==SOURCE_SHA, 'SHA корпуса изменён: требуется взаимное ревью')
    d=json.loads(raw)
    need(d['schema']=='ricon.pair_service' and d['version']==1 and d['revision']==1 and len(d['cases'])==CASES, 'Неверная ревизия')
    lines=['//! Сгенерировано gen_pair_service_fixture.py; ожидания только копируются; TEST_ONLY.',
           'class RICON_PairServiceFixtures','{',f'\tstatic const string SOURCE_SHA = "{SOURCE_SHA}";',f'\tstatic const int CASES = {CASES};','']
    for i,c in enumerate(d['cases']):
        lines += case_lines(i,c)
    lines += ['\tstatic void Run(RICON_PairServiceRun t)','\t{'] + [f'\t\tCase{i}(t);' for i in range(CASES)]
    return '\n'.join(lines+['\t}','}',''])


def main(argv=None):
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('source',type=Path); p.add_argument('destination',type=Path); p.add_argument('--check',action='store_true')
    args=p.parse_args(argv)
    try:
        output=render(args.source.read_bytes())
        if args.check:
            need(args.destination.read_text(encoding='utf-8')==output, 'Фикстура отличается')
        else:
            args.destination.write_text(output,encoding='utf-8',newline='\n')
    except (OSError,ValueError,KeyError,TypeError) as error:
        print(f'PAIR_SERVICE_GENERATOR_FAIL: {error}'); return 1
    print(f'PAIR_SERVICE_GENERATOR_PASS: {CASES}'); return 0


if __name__=='__main__':
    raise SystemExit(main())
