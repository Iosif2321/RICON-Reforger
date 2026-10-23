#!/usr/bin/env python3
"""P4.3: независимый пересчёт REQ/JSON; используются только собственные checker P4 и P4.2."""
import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path

SPEC = importlib.util.spec_from_file_location('codex_p42', Path(__file__).with_name('p42-independent-req-check-codex.py'))
P42 = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(P42)
P4, P2 = P42.P4, P42.P2
SHA = '1bffd636582e62b5b7ca77537b94828a13c06e2ff922aea64ef3f36054f51514'
COMMON = 'observation catalog polarization txHeightM rxHeightM'
ERRORS = {'INVALID_OBSERVATION','INVALID_INPUT','VEGETATION_COVERAGE_INCOMPLETE','VEGETATION_UNKNOWN','VEGETATION_MIXED_PROFILE'}


def plus(a, b):
    result = a + b
    P4.check_finite(result)
    return result


def mid_path(value):
    obs, length = P2.observation(value['observation'])
    merged = []
    for s in obs['segments']:
        if s['family'] != 'VEGETATION' or value['catalog'][s['materialId']]['negligible']:
            continue
        if merged and s['startM'] <= merged[-1][1]:
            merged[-1][1] = max(merged[-1][1], s['endM'])
        else:
            merged.append([s['startM'],s['endM']])
    return any(start>0 and stop<length for start,stop in merged)


def evaluate(value):
    try:
        P2.fields(value, COMMON+' canopy budget')
    except (ValueError,TypeError):
        return {'code':'INVALID_INPUT'}
    through_result = P4.evaluate({k:value[k] for k in COMMON.split()})
    over_result = P42.evaluate({k:value[k] for k in (COMMON+' canopy').split()})
    for result in (through_result,over_result):
        if result['code'] in ERRORS:
            return {'code':result['code']}
    try:
        P2.fields(value['budget'],'fsplDb l2Db fsplOverDb l2OverDb')
        budget={}
        for k,v in value['budget'].items():
            if k in ('l2Db','l2OverDb') and v is None:
                budget[k]=None
            else:
                budget[k]=P2.number(v)
                if budget[k]<0 or (k in ('fsplDb','fsplOverDb') and budget[k]==0):
                    raise ValueError('Неверный бюджет')
    except (ValueError,TypeError,OverflowError):
        return {'code':'INVALID_INPUT'}
    if any(r['code']=='NUMERIC_FAILURE' for r in (through_result,over_result)):
        return {'code':'NUMERIC_FAILURE'}
    try:
        if through_result['code']=='NO_VEGETATION':
            if budget['l2Db'] is None:
                return dict(code='UNRESOLVED',reason='BASE_BUDGET_MISSING')
            return dict(code='NO_VEGETATION',totalLossDb=plus(budget['fsplDb'],budget['l2Db']))
        through=dict(present=False,reason=through_result['code'])
        if through_result['code']=='OK':
            jb=any(i['excessBaseline']=='FREE_SPACE' for i in through_result['intervals'])
            if not jb and budget['l2Db'] is None:
                through=dict(present=False,reason='BASE_BUDGET_MISSING')
            else:
                base=budget['fsplDb'] if jb else plus(budget['fsplDb'],budget['l2Db'])
                through=dict(present=True,reason=None,baseDb=base,vegetationDb=through_result['lossDb'],
                             totalDb=plus(base,through_result['lossDb']),closed=through_result['allScenariosClosed'],
                             budgetMode='FREE_SPACE_PLUS_JB' if jb else 'BASE_PLUS_EXCESS')
        if mid_path(value):
            over=dict(present=False,reason='MID_PATH_VEGETATION')
        elif over_result['code']!='OK':
            over=dict(present=False,reason=over_result['code'])
        elif budget['l2OverDb'] is None:
            over=dict(present=False,reason='BASE_BUDGET_MISSING')
        else:
            base=plus(budget['fsplOverDb'],budget['l2OverDb'])
            over=dict(present=True,reason=None,baseDb=base,canopyDb=over_result['totalAhDb'],
                      totalDb=plus(base,over_result['totalAhDb']),closed=over_result['status']=='SOURCE_DOMAIN')
    except ArithmeticError:
        return {'code':'NUMERIC_FAILURE'}
    if not through['present'] and not over['present']:
        return dict(code='UNRESOLVED',reason='NO_BRANCH',through=through,over=over)
    if through['present'] and (not over['present'] or through['totalDb']<=over['totalDb']):
        selected='THROUGH'
        total=through['totalDb']
    else:
        selected='OVER'
        total=over['totalDb']
    if through.get('closed') and over.get('closed'):
        status='CLOSED'
    elif over.get('closed'):
        status='OVER_REFERENCE_ONLY'
    else:
        status='OPEN'
    return dict(code='OK',selected=selected,totalLossDb=total,status=status,through=through,over=over)


def self_test(cases):
    actual=evaluate(cases[0]['input'])
    controls=[]
    for k,v in (('code','LOST'),('selected','OVER'),('totalLossDb',0),('status','OPEN')):
        mutant=copy.deepcopy(actual);mutant[k]=v;controls.append(mutant)
    for branch in ('through','over'):
        for k,v in actual[branch].items():
            mutant=copy.deepcopy(actual)
            mutant[branch][k]=not v if type(v) is bool else (999 if type(v) in (int,float) else 'BAD')
            controls.append(mutant)
    caught=0
    for mutant in controls:
        try:
            P4.compare(mutant,actual)
        except AssertionError:
            caught+=1
    assert caught==len(controls)
    assert actual['totalLossDb']==min(actual['through']['totalDb'],actual['over']['totalDb'])
    assert actual['totalLossDb']!=actual['through']['totalDb']+actual['over']['totalDb']
    return caught


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('corpus',type=Path)
    parser.add_argument('--receipt',type=Path)
    args=parser.parse_args()
    raw=args.corpus.read_bytes()
    sha=hashlib.sha256(raw).hexdigest()
    assert sha==SHA,'Корпус изменён'
    doc=json.loads(raw);cases=doc['cases']
    assert len(cases)==29 and [c['id'] for c in cases]==[f'P{i:02}' for i in range(1,30)]
    assert doc['hostOnly']==['P18','P27']
    controls=self_test(cases)
    errors=[]
    for c in cases:
        before=copy.deepcopy(c['input']);actual=evaluate(c['input'])
        try:
            P4.compare(actual,c['expect'])
            assert c['input']==before,'Вход изменён'
        except AssertionError as e:
            errors.append(dict(id=c['id'],error=str(e)))
    # Контрпример #3692: входы binary32, выбор double и native суммы различается.
    near=copy.deepcopy(cases[27]['input'])
    near['budget'].update(fsplOverDb=91.279998779296875,l2OverDb=0)
    e=evaluate(near)
    through_native=P2.number(e['through']['totalDb']);over_native=P2.number(e['over']['totalDb'])
    native_choice='THROUGH' if through_native<=over_native else 'OVER'
    assert e['selected']=='OVER' and native_choice=='THROUGH'
    report=dict(checker='codex: только REQ v0.2, JSON, собственные checker P4/P4.2',corpusSha256=sha,
                cases=29,matched=29-len(errors),errors=errors,negativeControls=controls,native=27,hostOnly=['P18','P27'],
                freezeApproved=False,pendingCounter=3692,
                nearTie=dict(referenceSelected=e['selected'],nativeSelected=native_choice,
                             referenceThroughDb=e['through']['totalDb'],referenceOverDb=e['over']['totalDb'],
                             nativeThroughDb=through_native,nativeOverDb=over_native))
    if args.receipt:
        args.receipt.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(report,ensure_ascii=False,indent=2))
    print('P43_INDEPENDENT_CHECK_PASS' if not errors else 'P43_INDEPENDENT_CHECK_FAIL')
    return bool(errors)


if __name__=='__main__':
    raise SystemExit(main())
