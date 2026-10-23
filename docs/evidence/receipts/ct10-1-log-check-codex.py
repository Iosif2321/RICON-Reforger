#!/usr/bin/env python3
"""Независимая сверка CT10.1 native-лога с frozen JSON; oracle и генератор не импортируются."""
import argparse
import hashlib
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[3]
SHA = '95208dd198cee4680763e6813cb66374832ef241ea1b5590e20799d834f2aa27'
HOST = {'D17', 'D18', 'D19', 'D20', 'D21', 'D31', 'D47', 'D54'}
SUMMARY = 'cases=46 match=46 immutable=46 controls=8 extras=3 hostOnly=8'


def need(condition, why):
    if not condition:
        raise ValueError(why)


def expected_cases():
    raw = (ROOT / 'oracle/vectors/channel_descriptor_v1.json').read_bytes()
    need(hashlib.sha256(raw).hexdigest() == SHA, 'Корпус изменён')
    doc = json.loads(raw)
    need(set(doc['hostOnly']) == HOST and len(doc['cases']) == 54, 'Неверный набор случаев')
    return {c['id']: c for c in doc['cases'] if c['id'] not in HOST}


def check(text, expected):
    need(not re.search(r'SCRIPT\s*\(E\)|\[RICON[^\n]*\bFAIL\b', text), 'Ошибка/FAIL в логе')
    lines = [s.split('[RICON_DESCRIPTOR] ', 1)[1].strip() for s in text.splitlines() if '[RICON_DESCRIPTOR] ' in s]
    for prefix, value in (('source=', 'source=' + SHA), ('cases=', SUMMARY), ('done:', 'done: PASS')):
        need([s for s in lines if s.startswith(prefix)] == [value], 'Неверный заголовок/итог ' + prefix)
    need(len(lines) == len(expected) + 3, 'Лишние/пропущенные строки')
    seen = set()
    for line in lines:
        if not re.match(r'D\d', line):
            continue
        key, rest = line.split(' ', 1)
        need(key in expected and key not in seen, 'Неизвестный, повторный или host-only случай')
        f = dict(re.findall(r'(\w+)=(\S+)', rest))
        need(set(f) == {'op', 'code', 'size', 'match', 'immutable'}, key + ': неполные поля')
        case = expected[key]
        need(f['op'] == case['input']['op'] and f['code'] == case['expect']['code'], key + ': операция/код')
        need(int(f['size']) == case['expect'].get('sizeBytes', 0), key + ': размер')
        need(f['match'] == f['immutable'] == '1', key + ': совпадение/неизменность')
        seen.add(key)
    need(seen == set(expected), 'Пропущены случаи')
    frames = sum('bytes' in c['expect'] for c in expected.values())
    rows = [s.split('[RICON_DESCRIPTOR_FRAMES] ', 1)[1].strip() for s in text.splitlines() if '[RICON_DESCRIPTOR_FRAMES] ' in s]
    need(rows == [f'frames={frames} flips={4*frames}'], 'Неверное число кадров/порч')
    return dict(cases=len(seen), frames=frames, byteFlipControls=4*frames, nativeControls=8, nativeExtras=3, hostOnly=8,
                descriptorAndBytes='Полные значения строго проверены native comparator; внешне сверены коды/операции/размеры')


def self_test(expected):
    lines = ['[RICON_DESCRIPTOR] source=' + SHA]
    for key, c in expected.items():
        lines.append(f"[RICON_DESCRIPTOR] {key} op={c['input']['op']} code={c['expect']['code']} "
                     f"size={c['expect'].get('sizeBytes',0)} match=1 immutable=1")
    frames = sum('bytes' in c['expect'] for c in expected.values())
    lines += [f'[RICON_DESCRIPTOR_FRAMES] frames={frames} flips={4*frames}',
              '[RICON_DESCRIPTOR] ' + SUMMARY, '[RICON_DESCRIPTOR] done: PASS']
    good = '\n'.join(lines)
    check(good, expected)
    row = next(s for s in lines if '[RICON_DESCRIPTOR] D01 ' in s)
    mutants = [good.replace(SHA, '0'*64), good.replace(row, ''), good + '\n' + row,
               good.replace('done: PASS', 'done: FAIL'), good.replace('done: PASS', ''),
               good + '\n[RICON_DESCRIPTOR] done: PASS', good.replace('D01 ', 'D17 '),
               good.replace('controls=8', 'controls=7'), good.replace('flips=48', 'flips=47'),
               good + '\nSCRIPT (E): намеренная ошибка']
    for field in ('op', 'code', 'size', 'match', 'immutable'):
        mutants.append(good.replace(row, re.sub(rf'\b{field}=\S+', field + '=99999', row)))
    for i, mutant in enumerate(mutants):
        try:
            check(mutant, expected)
        except (ValueError, KeyError):
            continue
        raise AssertionError('Не обнаружена порча ' + str(i))
    return len(mutants)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    group = parser.add_mutually_exclusive_group()
    group.add_argument('--log', type=Path)
    group.add_argument('--receipt', type=Path)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    expected = expected_cases()
    result = dict(validatorSelfTest='PASS', negativeControls=self_test(expected))
    path = args.log
    if args.receipt:
        receipt = json.loads(args.receipt.read_text(encoding='utf-8-sig'))
        need(receipt['exitCode'] == 0 and not receipt['timedOut'] and receipt['sourceStable'] is True
             and receipt['worldStarted'] is False, 'Процесс/исходники/мир')
        for source in receipt['sources']:
            need(hashlib.sha256((ROOT / source['path']).read_bytes()).hexdigest() == source['sha256'],
                 'Исходник изменён после запуска: ' + source['path'])
        path = Path(receipt['logPath'])
        need(hashlib.sha256(path.read_bytes()).hexdigest() == receipt['logSha256'], 'Лог изменён после запуска')
    if path:
        raw = path.read_bytes()
        result.update(check(raw.decode('utf-8', errors='replace'), expected))
        result.update(logSha256=hashlib.sha256(raw).hexdigest(), processReceiptChecked=bool(args.receipt), status='NATIVE_LOG_PASS')
    if args.report:
        args.report.write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps(result, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
