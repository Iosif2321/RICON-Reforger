#!/usr/bin/env python3
"""P4.3 TOOL_ONLY: перенос frozen JSON в Enforce без вычисления ожиданий."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

sys.path.insert(0,str(Path(__file__).resolve().parent))
from gen_overcanopy_fixture import input_lines, profile_lines, need, text, value_literal

ROOT=Path(__file__).resolve().parents[2]
SOURCE=ROOT/'oracle/vectors/vegetation_path_v1.json'
TARGET=ROOT/'addons/RICON_CoreTests/Scripts/Game/RICON_Tests/RICON_VegetationPathFixtures.c'
SHA='d9e3b7e7d01c8149b090e3e7642917e542f994632f18b5f54aaa12e6ca0bab51'
CODES={'INVALID_INPUT','INVALID_OBSERVATION','VEGETATION_COVERAGE_INCOMPLETE','VEGETATION_UNKNOWN',
       'VEGETATION_MIXED_PROFILE','NUMERIC_FAILURE','NO_VEGETATION','UNRESOLVED','OK'}


def branch_lines(name, branch, through):
    fields={'present','reason'}
    if branch['present']:
        fields|={'baseDb','totalDb','closed','vegetationDb','budgetMode'} if through else {'baseDb','totalDb','closed','canopyDb'}
    need(set(branch)==fields,'Неверные поля ветви')
    lines=[f'\t\tRICON_VegetationPathBranch {name} = new RICON_VegetationPathBranch();',
           f'\t\t{name}.m_bPresent = {value_literal(branch["present"],"bool")};']
    if branch['reason'] is not None:
        lines.append(f'\t\t{name}.m_sReason = {text(branch["reason"])};')
    if branch['present']:
        for key,field in (('baseDb','BaseDb'),('vegetationDb' if through else 'canopyDb','AddedDb'),('totalDb','TotalDb')):
            lines.append(f'\t\t{name}.m_f{field} = {value_literal(branch[key],"float")};')
        lines.append(f'\t\t{name}.m_bClosed = {value_literal(branch["closed"],"bool")};')
        if through:
            lines.append(f'\t\t{name}.m_sBudgetMode = {text(branch["budgetMode"])};')
    return lines


def case_lines(index,c,profiles):
    need(set(c)=={'id','title','input','expect'} and re.fullmatch(r'P[0-9]{2}',c['id']),'Неверный случай')
    inp,exp=c['input'],c['expect']
    need(set(inp)=={'observation','catalog','polarization','txHeightM','rxHeightM','canopy','budget'},'Неверный вход')
    need(exp['code'] in CODES,'Неизвестный код')
    need(set(inp['budget'])=={'fsplDb','l2Db','fsplOverDb','l2OverDb'},'Неверные поля бюджета')
    fields={'code'}
    if exp['code']=='OK':fields|={'selected','totalLossDb','status','through','over'}
    if exp['code']=='NO_VEGETATION':fields.add('totalLossDb')
    if exp['code']=='UNRESOLVED':
        fields.add('reason')
        if exp['reason']=='NO_BRANCH':fields|={'through','over'}
    need(set(exp)==fields,'Неверные поля результата')
    lines=[f'\tprotected static void Case{index}(RICON_VegetationPathRun t)','\t{']
    lines+=input_lines({k:v for k,v in inp.items() if k!='budget'},profiles)
    lines+=['\t\tRICON_VegetationPathInput query = new RICON_VegetationPathInput();','\t\tquery.m_Input = input;',
            '\t\tquery.m_Budget = new RICON_VegetationPathBudget();']
    for key,field in (('fsplDb','FsplDb'),('fsplOverDb','FsplOverDb')):
        lines.append(f'\t\tquery.m_Budget.m_f{field} = {value_literal(inp["budget"][key],"float")};')
    for key,field in (('l2Db','L2'),('l2OverDb','L2Over')):
        if inp['budget'][key] is not None:
            lines += [f'\t\tquery.m_Budget.m_bHas{field} = true;',f'\t\tquery.m_Budget.m_f{field}Db = {value_literal(inp["budget"][key],"float")};']
    lines+=['\t\tRICON_VegetationPathResult expected = new RICON_VegetationPathResult();',
            f'\t\texpected.m_eCode = RICON_EVegetationPathCode.{exp["code"]};']
    if 'reason' in exp:lines.append(f'\t\texpected.m_sReason = {text(exp["reason"])};')
    if 'totalLossDb' in exp:
        lines += ['\t\texpected.m_bHasLoss = true;',f'\t\texpected.m_fTotalLossDb = {value_literal(exp["totalLossDb"],"float")};']
    for key,field in (('selected','Selected'),('status','Status')):
        if key in exp:lines.append(f'\t\texpected.m_s{field} = {text(exp[key])};')
    for key,field in (('through','Through'),('over','Over')):
        if key in exp:lines+=branch_lines(key,exp[key],key=='through')+[f'\t\texpected.m_{field} = {key};']
    return lines+[f'\t\tt.CheckCase({text(c["id"])}, query, expected);','\t}','']


def build(raw):
    need(hashlib.sha256(raw).hexdigest()==SHA,'SHA frozen корпуса изменён')
    doc=json.loads(raw);cases=doc['cases']
    need(len(cases)==31 and [c['id'] for c in cases]==[f'P{i:02}' for i in range(1,32)],'Неверный набор случаев')
    need(doc['hostOnly']==['P18','P27'],'Неверный host-only')
    native=[c for c in cases if c['id'] not in doc['hostOnly']]
    profiles,indices=[],{}
    for c in native:
        for material,p in c['input']['catalog'].items():
            k=json.dumps([material,p],sort_keys=True)
            if k not in indices:
                indices[k]=len(profiles);profiles.append((material,p))
    lines=['//! Сгенерировано gen_vegetation_path_fixture.py: только копирование frozen P4.3, TEST_ONLY.',
           '//! P18/P27 host-only: лишние поля JSON невыразимы типизированным DTO.',
           'class RICON_VegetationPathFixtures','{',f'\tstatic const string SOURCE_SHA = "{SHA}";',
           '\tstatic const int CASES = 29;','\tstatic const int HOST_ONLY = 2;','']
    for i,(material,p) in enumerate(profiles):lines+=profile_lines(i,material,p)
    for i,c in enumerate(native):lines+=case_lines(i,c,indices)
    lines+=['\tstatic void Run(RICON_VegetationPathRun t)','\t{']+[f'\t\tCase{i}(t);' for i in range(len(native))]
    return '\n'.join(lines+['\t}','}',''])


def main(argv=None):
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check',action='store_true')
    parser.add_argument('--source',type=Path,default=SOURCE)
    parser.add_argument('--target',type=Path,default=TARGET)
    args=parser.parse_args(argv)
    try:
        output=build(args.source.read_bytes())
        if args.check:need(args.target.read_text(encoding='utf-8')==output,'Фикстура отличается')
        else:args.target.write_text(output,encoding='utf-8',newline='\n')
    except (OSError,KeyError,TypeError,ValueError) as e:
        print(f'P43_GENERATOR_FAIL: {e}');return 1
    print('P43_GENERATOR_PASS: 29 native, 2 host-only');return 0


if __name__=='__main__':raise SystemExit(main())
