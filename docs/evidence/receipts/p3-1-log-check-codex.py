#!/usr/bin/env python3
"""Независимая сверка всех скалярных полей P3.1 native-лога с frozen JSON."""
import argparse
import hashlib
import json
from pathlib import Path
import re

ROOT=Path(__file__).resolve().parents[3]
SHA='b0b93be3238b49c223c859ac14e22f045ae708ce03f1cae2ed731eaf0531e4c5'
HOST={'R31','R32','R33','R34','R35','R36','R41'}
SUMMARY='cases=40 match=40 immutable=40 controls=6 extras=2 hostOnly=7'


def need(c,why):
    if not c:raise ValueError(why)


def expected_cases():
    raw=(ROOT/'oracle/vectors/object_rf_profile_v1.json').read_bytes()
    need(hashlib.sha256(raw).hexdigest()==SHA,'Изменён корпус')
    doc=json.loads(raw);need(len(doc['cases'])==47 and set(doc['hostOnly'])==HOST,'Неверный набор случаев')
    return {c['id']:c['expect'] for c in doc['cases'] if c['id'] not in HOST}


def full(e):
    return e if e['code']!='INVALID_INPUT' else dict(code='INVALID_INPUT',knowledge='UNKNOWN',materialId='',source='NONE',matchedKey='')


def check(text,expected):
    need(not re.search(r'SCRIPT\s*\(E\)|\[RICON[^\n]*\bFAIL\b',text),'Ошибка/FAIL в логе')
    lines=[s.split('[RICON_OBJPROFILE] ',1)[1].strip() for s in text.splitlines() if '[RICON_OBJPROFILE] ' in s]
    for prefix,value in (('source=','source='+SHA),('cases=',SUMMARY),('done:','done: PASS')):
        need([s for s in lines if s.startswith(prefix)]==[value],'Неверный заголовок/итог '+prefix)
    need(len(lines)==len(expected)+3,'Лишние/пропущенные строки')
    seen=set()
    for line in lines:
        if not re.match(r'R\d',line):continue
        key,rest=line.split(' ',1)
        need(key in expected and key not in seen,'Неизвестный/повторный/host-only случай')
        f=dict(re.findall(r'(\w+)=(\S*)',rest));e=full(expected[key])
        want=dict(code=e['code'],knowledge=e['knowledge'],material=e['materialId'],source=e['source'],matched=e['matchedKey'],match='1',immutable='1')
        need(f==want,key+': несовпадение полного результата')
        seen.add(key)
    need(seen==set(expected),'Пропущены случаи')
    return dict(cases=len(seen),strictScalarFieldsPerCase=5,nativeControls=6,nativeExtras=2,hostOnly=7)


def self_test(expected):
    lines=['[RICON_OBJPROFILE] source='+SHA]
    for key,e0 in expected.items():
        e=full(e0)
        lines.append(f"[RICON_OBJPROFILE] {key} code={e['code']} knowledge={e['knowledge']} material={e['materialId']} source={e['source']} matched={e['matchedKey']} match=1 immutable=1")
    lines+=['[RICON_OBJPROFILE] '+SUMMARY,'[RICON_OBJPROFILE] done: PASS'];good='\n'.join(lines)
    check(good,expected)
    row=next(s for s in lines if '[RICON_OBJPROFILE] R01 ' in s)
    mutants=[good.replace(SHA,'0'*64),good.replace(row,''),good+'\n'+row,good.replace('R01 ','R31 '),
             good.replace('controls=6','controls=5'),good.replace('done: PASS','done: FAIL'),good+'\nSCRIPT (E): намеренная ошибка']
    for field in ('code','knowledge','material','source','matched','match','immutable'):
        mutants.append(good.replace(row,re.sub(rf'\b{field}=\S*',field+'=BAD',row)))
    for i,m in enumerate(mutants):
        try:check(m,expected)
        except (ValueError,KeyError):continue
        raise AssertionError('Не обнаружена порча '+str(i))
    return len(mutants)


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--log',type=Path);p.add_argument('--report',type=Path);a=p.parse_args()
    expected=expected_cases();r=dict(validatorSelfTest='PASS',negativeControls=self_test(expected))
    if a.log:
        raw=a.log.read_bytes();r.update(check(raw.decode('utf-8',errors='replace'),expected))
        r.update(logSha256=hashlib.sha256(raw).hexdigest(),status='NATIVE_LOG_PASS',processReceiptChecked=False)
    if a.report:a.report.write_text(json.dumps(r,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(r,ensure_ascii=False,indent=2))


if __name__=='__main__':main()
