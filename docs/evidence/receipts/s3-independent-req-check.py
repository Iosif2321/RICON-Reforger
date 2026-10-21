"""Независимая сверка S3 из REQ v0.2; реализация Claude не импортируется."""
from pathlib import Path
import copy
import hashlib
import json
import math
import sys

ROOT = Path.cwd()
sys.path.insert(0, str(ROOT / 'oracle'))
import radio_device_oracle as s2
import ricon_oracle as physics

MAX = 2**31 - 1
ARRAYS = ('bindings', 'listeners', 'players', 'devices', 'endpoints')
CAPS = dict(zip(ARRAYS, ('maxBindings', 'maxListeners', 'maxPlayers', 'maxDevices', 'maxEndpoints')))
STATES = ('NONE', 'CONNECTED_UNREGISTERED', 'REGISTERED_UNBOUND', 'BOUND_ACTIVE', 'BOUND_INACTIVE', 'DISCONNECTED')

def integer(x, low=1, high=MAX):
    return type(x) is int and low <= x <= high

def valid_device(x):
    if type(x) is not dict or not integer(x.get('deviceId')) or x.get('kind') not in ('CARRIED', 'WORLD'):
        return False
    if not integer(x.get('ownerPlayerId'), 0) or type(x.get('powered')) is not bool:
        return False
    access, allowed = x.get('accessPlayerIds'), x.get('allowed')
    if type(access) is not list or type(allowed) is not list:
        return False
    if not all(integer(v) for v in access) or access != sorted(set(access)):
        return False
    if not all(type(v) is list and len(v) == 2 and all(integer(n) for n in v) for v in allowed):
        return False
    return [tuple(v) for v in allowed] == sorted(set(tuple(v) for v in allowed))

def valid_fields(row, fields):
    return type(row) is dict and all(integer(row.get(key), *bounds) for key, bounds in fields.items())

def evaluate(inp, fixture):
    def reject(code):
        return dict(callCode=code, pairs=[], s2Calls=0, evaluatorCalls=0)
    if any(inp.get(k) is None for k in (*ARRAYS, 'context', 'limits')) or inp.get('hasEvaluator') is not True:
        return reject('NULL_INPUT')
    limits = inp['limits']
    if type(limits) is not dict or any(not integer(limits.get(k), 1, 4096) for k in CAPS.values()) \
            or not integer(limits.get('maxPairs'), 1, 65536):
        return reject('INVALID_LIMITS')
    if any(type(inp[k]) is not list for k in ARRAYS):
        return reject('INVALID_ELEMENT')
    if any(len(inp[k]) > limits[CAPS[k]] for k in ARRAYS) \
            or len(inp['bindings']) * len(inp['listeners']) > limits['maxPairs']:
        return reject('LIMIT_EXCEEDED')
    if not integer(inp.get('nowMs'), 0) or not integer(inp.get('sessionEpoch')):
        return reject('INVALID_ELEMENT')
    bf = {k: (1, MAX) for k in ('leaseId', 'sessionEpoch', 'txPlayerId', 'txGeneration', 'networkTypeId', 'channelId', 'ttlMs')}
    bf.update(txDeviceId=(1, MAX-1), txTransceiverIndex=(0, MAX-1), issuedAtMs=(0, MAX))
    lf = {k: (1, MAX) for k in ('playerId', 'generation', 'sessionEpoch')}
    lf.update(deviceId=(1, MAX-1), transceiverIndex=(0, MAX-1))
    for b in inp['bindings']:
        if not valid_fields(b, bf) or type(b.get('active')) is not bool:
            return reject('INVALID_ELEMENT')
    for l in inp['listeners']:
        if not valid_fields(l, lf):
            return reject('INVALID_ELEMENT')
    for p in inp['players']:
        if not valid_fields(p, {'playerId': (1, MAX), 'generation': (1, MAX)}) or p.get('state') not in STATES:
            return reject('INVALID_ELEMENT')
    if not all(valid_device(d) for d in inp['devices']):
        return reject('INVALID_ELEMENT')
    for e in inp['endpoints']:
        if type(e) is not dict or type(e.get('observation')) is not dict or type(e.get('profile')) is not dict:
            return reject('INVALID_ELEMENT')
        if not valid_fields(e['observation'], {'deviceId': (1, MAX-1), 'transceiverIndex': (0, MAX-1)}):
            return reject('INVALID_ELEMENT')
    keys = [
        [b['leaseId'] for b in inp['bindings']],
        [b['txDeviceId'] for b in inp['bindings']],
        [p['playerId'] for p in inp['players']],
        [d['deviceId'] for d in inp['devices']],
        [(e['observation']['deviceId'], e['observation']['transceiverIndex']) for e in inp['endpoints']],
        [(l['playerId'], l['deviceId'], l['transceiverIndex']) for l in inp['listeners']],
    ]
    if any(len(v) != len(set(v)) for v in keys):
        return reject('DUPLICATE')
    players = {p['playerId']: p for p in inp['players']}
    devices = {d['deviceId']: d for d in inp['devices']}
    endpoints = {(e['observation']['deviceId'], e['observation']['transceiverIndex']): e for e in inp['endpoints']}
    result = reject('OK')
    def allowed(device, pid, binding):
        return device is not None and (device['ownerPlayerId'] == pid or pid in device['accessPlayerIds']) \
            and [binding['networkTypeId'], binding['channelId']] in device['allowed']
    for b in sorted(inp['bindings'], key=lambda x: x['leaseId']):
        for l in sorted(inp['listeners'], key=lambda x: (x['deviceId'], x['transceiverIndex'], x['playerId'])):
            row = dict(leaseId=b['leaseId'], txDeviceId=b['txDeviceId'], rxPlayerId=l['playerId'],
                       rxDeviceId=l['deviceId'], rxTransceiverIndex=l['transceiverIndex'],
                       pairCode='BINDING_INACTIVE', hasDeviceCode=False, deviceCode='NULL_INPUT',
                       hasBudget=False, valid=False, reason='NUMERIC_FAILURE', state='LOST', intelligibility=0.0, sinrDb=0.0)
            result['pairs'].append(row)
            txp, rxp = players.get(b['txPlayerId']), players.get(l['playerId'])
            txd, rxd = devices.get(b['txDeviceId']), devices.get(l['deviceId'])
            txe = endpoints.get((b['txDeviceId'], b['txTransceiverIndex']))
            rxe = endpoints.get((l['deviceId'], l['transceiverIndex']))
            if not b['active'] or b['sessionEpoch'] != inp['sessionEpoch'] or inp['nowMs'] < b['issuedAtMs'] \
                    or inp['nowMs'] - b['issuedAtMs'] >= b['ttlMs']:
                continue
            if txp is None or txp['state'] != 'BOUND_ACTIVE' or txp['generation'] != b['txGeneration']:
                row['pairCode'] = 'TX_PLAYER_INVALID'; continue
            if txe is None or not allowed(txd, b['txPlayerId'], b):
                row['pairCode'] = 'TX_DEVICE_INVALID'; continue
            if l['deviceId'] == b['txDeviceId']:
                row['pairCode'] = 'SELF_PAIR'; continue
            if l['sessionEpoch'] != inp['sessionEpoch'] or rxp is None or rxp['state'] != 'BOUND_ACTIVE' \
                    or rxp['generation'] != l['generation']:
                row['pairCode'] = 'RX_PLAYER_INVALID'; continue
            if rxe is None or not allowed(rxd, l['playerId'], b):
                row['pairCode'] = 'RX_DEVICE_INVALID'; continue
            if txd['powered'] != txe['observation']['powered'] or rxd['powered'] != rxe['observation']['powered']:
                row['pairCode'] = 'STATE_MISMATCH'; continue
            result['s2Calls'] += 1
            rd = s2.evaluate(dict(txObservation=txe['observation'], txProfile=txe['profile'],
                                  rxObservation=rxe['observation'], rxProfile=rxe['profile'], context=inp['context']))
            row.update(pairCode='DEVICE_QUERY', hasDeviceCode=True, deviceCode=rd['code'])
            if rd['code'] != 'READY':
                continue
            result['evaluatorCalls'] += 1
            budget = physics.evaluate_budget(rd['query'], fixture)
            row.update(pairCode='EVALUATED', hasBudget=True,
                       **{k: budget[k] for k in ('valid', 'reason', 'state', 'intelligibility', 'sinrDb')})
    return result

def differences(a, b, path=''):
    if isinstance(a, dict) and isinstance(b, dict):
        if a.keys() != b.keys():
            return [path + ': keys']
        return [v for k in a for v in differences(a[k], b[k], path+'.'+k)]
    if isinstance(a, list) and isinstance(b, list):
        if len(a) != len(b):
            return [path + ': length']
        return [v for i,(x,y) in enumerate(zip(a,b)) for v in differences(x,y,f'{path}[{i}]')]
    if type(a) is float or type(b) is float:
        if type(a) not in (int,float) or type(b) not in (int,float) or not math.isfinite(a) or not math.isfinite(b) or abs(a-b)>1e-9:
            return [f'{path}: {a!r} != {b!r}']
    elif type(a) is not type(b) or a != b:
        return [f'{path}: {a!r} != {b!r}']
    return []

if __name__ == '__main__':
    corpus = ROOT/'oracle/vectors/pair_service_v1.json'
    raw = corpus.read_bytes(); doc = json.loads(raw)
    errors=[]
    for case in doc['cases']:
        inputs=copy.deepcopy(case['input'])
        actual=evaluate(inputs,doc['fixture'])
        assert inputs==case['input'],case['id']
        errors += [(case['id'], e) for e in differences(actual,case['expect'])]
    receipt=dict(corpus_sha256=hashlib.sha256(raw).hexdigest(), checker_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                 cases=len(doc['cases']), differences=errors)
    Path(__file__).with_suffix('.json').write_text(json.dumps(receipt,indent=2),encoding='utf-8')
    print(json.dumps(receipt))
    raise SystemExit(bool(errors))
