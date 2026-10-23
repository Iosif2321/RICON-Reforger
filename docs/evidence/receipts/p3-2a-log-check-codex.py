#!/usr/bin/env python3
"""Независимая сверка P3.2a native-лога с frozen rev2; процесс проверяется отдельным receipt."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import struct

ROOT = Path(__file__).resolve().parents[3]
CORPUS_SHA = '2f5a4f10f5e1e472ea9eb253bc07230c806ad908002d59ab4db19b90cce1efbd'
REQ_SHA = '9eeeb8c59571dc75b747253d2f1655475319e5a4c6d7207cc65294a4fb8e5d0d'
HOST = {'MS054', 'MS055', 'MS056', 'MS058', 'MS059', 'MS068', 'MS069', 'MS070', 'MS076'}
PREFIX = '[RICON_SLAB] '
EXTRA = '[RICON_SLAB_TETM] '
SUMMARY = 'cases=71 match=71 immutable=71 controls=5 extras=3 hostOnly=9 maxDeviationDb='


def need(condition, why):
    if not condition:
        raise ValueError(why)


def lf_sha(raw):
    return hashlib.sha256(raw.replace(b'\r\n', b'\n')).hexdigest()


def f32(value):
    return struct.unpack('<f', struct.pack('<f', value))[0]


def tolerance(loss):
    return max(0.05, 2e-6 * abs(loss))


def number(text):
    value = float(text)
    need(math.isfinite(value), 'Неконечное число в логе')
    return value


def expected_cases():
    raw = (ROOT / 'oracle/vectors/material_slab_v1.json').read_bytes()
    need(lf_sha(raw) == CORPUS_SHA, 'Изменён frozen корпус')
    need(lf_sha((ROOT / 'docs/req/REQ-material-slab.md').read_bytes()) == REQ_SHA, 'Изменён frozen REQ')
    doc = json.loads(raw)
    need(doc['revision'] == 2 and doc['requirementsLfSha256'] == REQ_SHA, 'Неверная ревизия')
    need([c['id'] for c in doc['cases']] == [f'MS{i:03}' for i in range(1, 81)], 'Неверный набор случаев')
    need({c['id'] for c in doc['cases'] if not c['native']} == HOST, 'Неверный host-only набор')
    return {c['id']: c['expect'] for c in doc['cases'] if c['native']}


def check(text, expected):
    need(not re.search(r'SCRIPT\s*\(E\)|\[RICON[^\n]*\bFAIL\b', text), 'Ошибка/FAIL в логе')
    rows = []
    for line in text.splitlines():
        if '[RICON_SLAB' in line:
            marker = line[line.index('[RICON_SLAB'):].strip()
            need(marker.startswith((PREFIX, EXTRA)), 'Неизвестный маркер')
            rows.append(marker)
    need(len(rows) == len(expected) + 6, 'Лишние/пропущенные строки')
    need(rows[0] == PREFIX + 'source=' + CORPUS_SHA and rows[-1] == PREFIX + 'done: PASS', 'Неверный заголовок/итог')
    deviations, fractions = [], []
    for row, (key, exp) in zip(rows[1:72], expected.items()):
        match = re.fullmatch(re.escape(PREFIX) + r'id=(MS\d{3}) code=(\w+) loss=(\S+) provenance=(\w+) match=1 immutable=1', row)
        need(match is not None, key + ': неверные поля строки')
        ident, code, loss_text, provenance = match.groups()
        need(ident == key and code == exp['code'], key + ': неверный id/код/порядок')
        loss = number(loss_text)
        need(provenance == exp.get('provenance', 'NONE'), key + ': неверный provenance')
        if code == 'OK':
            delta = abs(loss - exp['lossDb'])
            need(loss >= 0 and delta <= tolerance(exp['lossDb']), key + ': потери вне допуска')
            fractions.append((delta / tolerance(exp['lossDb']), key))
            deviations.append(abs(f32(loss - f32(exp['lossDb']))))
    for row, values in zip(rows[72:75], ((5.3, 0.03, 0.2), (4, 1, 0.05), (100, 0.5, 0.1))):
        match = re.fullmatch(re.escape(EXTRA) + r'eps=(\S+) sigma=(\S+) d=(\S+) te=(\S+) tm=(\S+) ok=1', row)
        need(match is not None, 'Неверная строка TE=TM')
        eps, sigma, thickness, te, tm = map(number, match.groups())
        need(all(abs(a - f32(b)) <= 1e-8 for a, b in zip((eps, sigma, thickness), values)), 'Неверные входы TE=TM')
        need(te > 0 and tm > 0 and abs(te - tm) <= 0.001 + 1e-9, 'TE и TM расходятся')
    need(rows[-2].startswith(PREFIX + SUMMARY), 'Неверные счётчики')
    maximum = number(rows[-2][len(PREFIX + SUMMARY):])
    need(maximum >= 0 and abs(maximum - max(deviations)) <= 1e-6, 'Неверный maxDeviationDb')
    fraction, worst = max(fractions)
    return dict(cases=len(expected), nativeControls=5, nativeExtras=3, hostOnly=9,
                maxDeviationDb=maximum, worstToleranceFraction=fraction, worstCase=worst)


def self_test(expected):
    lines = [PREFIX + 'source=' + CORPUS_SHA]
    deviations = []
    for key, exp in expected.items():
        loss = f32(exp.get('lossDb', 0))
        lines.append(f"{PREFIX}id={key} code={exp['code']} loss={loss:.9f} provenance={exp.get('provenance', 'NONE')} match=1 immutable=1")
        if exp['code'] == 'OK':
            deviations.append(abs(f32(loss - f32(exp['lossDb']))))
    for eps, sigma, d in ((5.3, 0.03, 0.2), (4, 1, 0.05), (100, 0.5, 0.1)):
        lines.append(f'{EXTRA}eps={f32(eps):.9f} sigma={f32(sigma):.9f} d={f32(d):.9f} te=10 tm=10 ok=1')
    lines += [PREFIX + SUMMARY + '0', PREFIX + 'done: PASS']
    good = '\n'.join(lines)
    check(good, expected)
    row = lines[3]
    mutants = [good.replace(CORPUS_SHA, '0' * 64), good.replace(row, ''), good + '\n' + row,
               good.replace('id=MS003', 'id=MS054'), good.replace('controls=5', 'controls=4'),
               good.replace('done: PASS', 'done: FAIL'), good + '\nSCRIPT (E): намеренная ошибка',
               good + '\n[RICON_TEST] FAIL контроль', good + '\n[RICON_SLAB_X] неизвестное',
               good.replace('te=10 tm=10', 'te=10 tm=10.002'), good.replace('eps=100.', 'eps=99.'),
               good.replace('maxDeviationDb=0', 'maxDeviationDb=1'), good.replace('ok=1', 'ok=0')]
    for field, value in (('code', 'NUMERIC_FAILURE'), ('loss', 'nan'), ('loss', '-1'), ('loss', '2.1'),
                         ('provenance', 'ASSUMED_CONFIG'), ('match', '0'), ('immutable', '0')):
        mutants.append(good.replace(row, re.sub(rf'\b{field}=\S+', field + '=' + value, row)))
    for index, mutant in enumerate(mutants):
        try:
            check(mutant, expected)
        except (ValueError, KeyError):
            continue
        raise AssertionError('Не обнаружена порча ' + str(index))
    return len(mutants)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--log', type=Path)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    expected = expected_cases()
    report = dict(validatorSelfTest='PASS', negativeControls=self_test(expected))
    if args.log:
        raw = args.log.read_bytes()
        report.update(check(raw.decode('utf-8', errors='replace'), expected))
        report.update(logSha256=hashlib.sha256(raw).hexdigest(), status='NATIVE_LOG_PASS', processReceiptChecked=False)
    if args.report:
        args.report.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
