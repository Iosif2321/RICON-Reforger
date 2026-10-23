#!/usr/bin/env python3
"""Независимая сверка всех видимых полей native P4.3 с frozen JSON, без oracle/генератора."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re

ROOT=Path(__file__).resolve().parents[3]
SHA='d9e3b7e7d01c8149b090e3e7642917e542f994632f18b5f54aaa12e6ca0bab51'
SUMMARY='cases=29 match=29 immutable=29 controls=23 extras=3 hostOnly=2'


def need(c,why):
    if not c:raise ValueError(why)


def expected_cases():
    raw=(ROOT/'oracle/vectors/vegetation_path_v1.json').read_bytes()
    need(hashlib.sha256(raw).hexdigest()==SHA,'Изменён корпус')
    doc=json.loads(raw)
    need(len(doc['cases'])==31 and doc['hostOnly']==['P18','P27'],'Неверный набор случаев')
    return {c['id']:c['expect'] for c in doc['cases'] if c['id'] not in doc['hostOnly']}


def check(text,expected):
    need(not re.search(r'SCRIPT\s*\(E\)|\[RICON[^\n]*\bFAIL\b',text),'Ошибка/FAIL в логе')
    lines=[s.split('[RICON_VPATH] ',1)[1].strip() for s in text.splitlines() if '[RICON_VPATH] ' in s]
    for prefix,value in (('source=','source='+SHA),('cases=',SUMMARY),('done:','done: PASS')):
        need([s for s in lines if s.startswith(prefix)]==[value],'Неверный заголовок/итог '+prefix)
    need(len(lines)==len(expected)+3,'Неверное число строк')
    seen,branches=set(),set()
    numbers=0
    maximum=0.0

    def number(actual,want,name):
        nonlocal numbers,maximum
        got=float(actual);error=abs(got-want)
        need(math.isfinite(got) and error<=max(.001,1e-5*abs(want))+5.1e-10,name+': неверное число')
        numbers+=1;maximum=max(maximum,error)

    for line in lines:
        if not line.startswith('id='):continue
        f=dict(re.findall(r'(\w+)=(\S+)',line));key=f['id']
        need(key in expected and key not in seen,'Неизвестный/повторный/host-only случай')
        need(set(f)=={'id','code','hasLoss','loss','selected','status','reason','match','immutable'},'Неверные поля результата')
        e=expected[key]
        want=dict(code=e['code'],hasLoss=str(int('totalLossDb' in e)),selected=e.get('selected','NONE'),
                  status=e.get('status','NONE'),reason=e.get('reason','NONE'),match='1',immutable='1')
        need(all(f[k]==v for k,v in want.items()),key+': неверный код/статус/причина/флаг')
        number(f['loss'],e.get('totalLossDb',0),key+'.loss');seen.add(key)
    need(seen==set(expected),'Пропущены случаи')
    wanted={(key,name) for key,e in expected.items() for name in ('through','over') if name in e}
    for line in text.splitlines():
        if '[RICON_VPATH_BRANCH] ' not in line:continue
        f=dict(re.findall(r'(\w+)=(\S+)',line.split('[RICON_VPATH_BRANCH] ',1)[1]))
        need(set(f)=={'id','branch','present','reason','base','added','total','closed','mode'},'Неверные поля ветви')
        pair=f['id'],f['branch'].lower()
        need(pair in wanted and pair not in branches,'Неизвестная/повторная ветвь')
        e=expected[pair[0]][pair[1]]
        need(f['present']==str(int(e['present'])) and f['closed']==str(int(e.get('closed',False))),'Неверные флаги ветви')
        need(f['reason']==(e['reason'] or 'NONE') and f['mode']==e.get('budgetMode','NONE'),'Неверная причина/mode')
        for name,want in (('base',e.get('baseDb',0)),('added',e.get('vegetationDb',e.get('canopyDb',0))),('total',e.get('totalDb',0))):
            number(f[name],want,pair[0]+'.'+pair[1]+'.'+name)
        branches.add(pair)
    need(branches==wanted,'Пропущены ветви')
    return dict(cases=len(seen),branches=len(branches),printedNumericFields=numbers,maxObservedAbsoluteError=maximum,
                nativeControls=23,nativeExtras=3,hostOnly=2)


def self_test(expected):
    lines=['[RICON_VPATH] source='+SHA]
    for key,e in expected.items():
        lines.append(f"[RICON_VPATH] id={key} code={e['code']} hasLoss={int('totalLossDb' in e)} loss={e.get('totalLossDb',0):.9f} "
                     f"selected={e.get('selected','NONE')} status={e.get('status','NONE')} reason={e.get('reason','NONE')} match=1 immutable=1")
        for name in ('through','over'):
            if name not in e:continue
            b=e[name]
            lines.append(f"[RICON_VPATH_BRANCH] id={key} branch={name.upper()} present={int(b['present'])} reason={b['reason'] or 'NONE'} "
                         f"base={b.get('baseDb',0):.9f} added={b.get('vegetationDb',b.get('canopyDb',0)):.9f} total={b.get('totalDb',0):.9f} "
                         f"closed={int(b.get('closed',False))} mode={b.get('budgetMode','NONE')}")
    lines+=['[RICON_VPATH] '+SUMMARY,'[RICON_VPATH] done: PASS'];good='\n'.join(lines)
    check(good,expected)
    root=next(s for s in lines if '[RICON_VPATH] id=P01 ' in s)
    branch=next(s for s in lines if '[RICON_VPATH_BRANCH] id=P01 ' in s)
    mutants=[good.replace(SHA,'0'*64),good.replace(root,''),good+'\n'+root,good.replace(branch,''),good+'\n'+branch,
             good.replace('done: PASS','done: FAIL'),good+'\nSCRIPT (E): намеренная ошибка',good.replace('controls=23','controls=22'),
             good.replace('id=P01','id=P18')]
    for line in (root,branch):
        for field in re.findall(r'(\w+)=',line):
            mutants.append(good.replace(line,re.sub(rf'\b{field}=\S+',field+'=99999',line)))
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
