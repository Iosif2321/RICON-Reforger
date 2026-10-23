#!/usr/bin/env python3
"""Внешняя сверка native-лога P4.2 с JSON; формулы и генератор не импортируются."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[3]
SHA = '00d7da2115f5d27445ae40502ef6fa3541e027e85a4275b7ba80264664a99cf2'
SUMMARY = 'cases=30 match=30 immutable=30 controls=20 extras=5 hostOnly=1'
GEOMETRY = {'start': 'intervalStartM', 'stop': 'intervalEndM', 'antenna': 'antennaHeightM', 'canopy': 'canopyHeightM'}
MATH = {'hdif': 'hdifM', 'theta': 'thetaDeg', 'nu': 'nu', 'j': 'jDb', 'ah': 'ahDb'}


def need(condition, why):
    if not condition:
        raise ValueError(why)


def expected_cases():
    raw = (ROOT / 'oracle/vectors/vegetation_overcanopy_v1.json').read_bytes()
    need(hashlib.sha256(raw).hexdigest() == SHA, 'Изменён замороженный корпус')
    doc = json.loads(raw)
    need(doc['hostOnly'] == ['O28'] and len(doc['cases']) == 31, 'Неверный набор случаев')
    return {c['id']: c['expect'] for c in doc['cases'] if c['id'] != 'O28'}


def check(text, expected):
    need(not re.search(r'SCRIPT\s*\(E\)|\[RICON[^\n]*\bFAIL\b', text), 'В логе ошибка или FAIL')
    rows, geometry, maths = {}, {}, {}
    main = [line.split('[RICON_CANOPY] ', 1)[1].strip() for line in text.splitlines() if '[RICON_CANOPY] ' in line]
    for prefix, want in (('source=', 'source=' + SHA), ('cases=', SUMMARY), ('done:', 'done: PASS')):
        need([s for s in main if s.startswith(prefix)] == [want], 'Неверный заголовок или итог: ' + prefix)
    for line in main:
        if not line.startswith('id='):
            continue
        f = dict(re.findall(r'(\w+)=(\S+)', line))
        key = f['id']
        need(key in expected and key not in rows, 'Повторный, неизвестный или host-only случай')
        need(set(f) == {'id', 'code', 'total', 'terminals', 'match', 'immutable'}, 'Неверные поля результата')
        exp = expected[key]
        need(f['code'] == exp['code'] and f['match'] == f['immutable'] == '1', key + ': неверный код/совпадение/неизменность')
        need(int(f['terminals']) == len(exp.get('terminals', [])), key + ': неверное число терминалов')
        rows[key] = f
    for tag, target, numeric in (('GEOMETRY', geometry, GEOMETRY), ('M6', maths, MATH)):
        for line in text.splitlines():
            marker = '[RICON_CANOPY_' + tag + '] '
            if marker not in line:
                continue
            f = dict(re.findall(r'(\w+)=(\S+)', line.split(marker, 1)[1]))
            need(set(f) == {'id', 'index'} | set(numeric) | ({'end', 'material', 'provenance'} if tag == 'GEOMETRY' else set()),
                 'Неверные поля терминала')
            key = f['id'], int(f['index'])
            need(key not in target and key[0] in expected, 'Повторный или неизвестный терминал')
            target[key] = f
    need(set(rows) == set(expected), 'Пропущены случаи')
    wanted = {(key, i) for key, exp in expected.items() for i in range(len(exp.get('terminals', [])))}
    need(set(geometry) == set(maths) == wanted, 'Пропущены/добавлены терминалы')
    numbers, max_error = 0, 0.0

    def number(actual, target, name):
        nonlocal numbers, max_error
        got = float(actual)
        error = abs(got - target)
        need(math.isfinite(got) and error <= max(.001, 1e-5 * abs(target)) + 5.1e-10, name + ': неверное число')
        numbers += 1
        max_error = max(max_error, error)

    for key, exp in expected.items():
        number(rows[key]['total'], exp.get('totalAhDb', 0), key + '.total')
        for i, terminal in enumerate(exp.get('terminals', [])):
            g, m = geometry[key, i], maths[key, i]
            need((g['end'], g['material'], g['provenance']) ==
                 (terminal['end'], terminal['materialId'], terminal['canopyProvenance']), key + ': неверная идентичность')
            for fields, record in ((GEOMETRY, g), (MATH, m)):
                for name, field in fields.items():
                    number(record[name], terminal[field], key + '.' + field)
    return dict(cases=len(rows), terminals=len(wanted), printedNumericFields=numbers, maxObservedAbsoluteError=max_error,
                nativeControls=20, nativeExtras=5, hostOnly=1)


def self_test(expected):
    lines = ['[RICON_CANOPY] source=' + SHA]
    for key, exp in expected.items():
        lines.append(f"[RICON_CANOPY] id={key} code={exp['code']} total={exp.get('totalAhDb',0):.9f} "
                     f"terminals={len(exp.get('terminals',[]))} match=1 immutable=1")
        for i, terminal in enumerate(exp.get('terminals', [])):
            for tag, fields in (('GEOMETRY', GEOMETRY), ('M6', MATH)):
                values = ' '.join(f'{k}={terminal[v]:.9f}' for k, v in fields.items())
                extra = (f" end={terminal['end']} material={terminal['materialId']} provenance={terminal['canopyProvenance']}"
                         if tag == 'GEOMETRY' else '')
                lines.append(f'[RICON_CANOPY_{tag}] id={key} index={i} {values}{extra}')
    lines += ['[RICON_CANOPY] ' + SUMMARY, '[RICON_CANOPY] done: PASS']
    good = '\n'.join(lines)
    check(good, expected)
    target = next(s for s in lines if '[RICON_CANOPY_M6] ' in s)
    geometry = next(s for s in lines if '[RICON_CANOPY_GEOMETRY] ' in s)
    root = next(s for s in lines if '[RICON_CANOPY] id=O01 ' in s)
    mutants = [good.replace(SHA, '0'*64), good.replace('done: PASS', 'done: FAIL'), good.replace(target, ''),
               good + '\n' + target, good + '\nSCRIPT (E): намеренная ошибка', good.replace(root, ''),
               good + '\n' + root, good.replace('controls=20', 'controls=19'), good.replace('id=O01', 'id=O28')]
    for line, fields in ((root, ('code', 'total', 'terminals', 'match', 'immutable')),
                         (geometry, ('end', 'material', 'provenance', *GEOMETRY)), (target, tuple(MATH))):
        for field in fields:
            mutants.append(good.replace(line, re.sub(rf'\b{field}=\S+', field + '=99999', line)))
    mutants.append(good.replace(target, re.sub(r'\bnu=\S+', 'nu=nan', target)))
    for i, mutant in enumerate(mutants):
        try:
            check(mutant, expected)
        except (ValueError, KeyError):
            continue
        raise AssertionError('Не обнаружено искажение ' + str(i))
    return len(mutants)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--log', type=Path)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    expected = expected_cases()
    result = dict(validatorSelfTest='PASS', negativeControls=self_test(expected))
    if args.log:
        raw = args.log.read_bytes()
        result.update(check(raw.decode('utf-8', errors='replace'), expected))
        result.update(logSha256=hashlib.sha256(raw).hexdigest(), status='NATIVE_LOG_PASS', processReceiptChecked=False)
    if args.report:
        args.report.write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps(result, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
