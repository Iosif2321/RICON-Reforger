#!/usr/bin/env python3
"""P4 TOOL_ONLY: копирует frozen JSON в Enforce, не вычисляя формулы и ожидания."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from gen_path_observation_fixture import need, value_literal, text, observation_lines

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'oracle/vectors/vegetation_v1.json'
TARGET = ROOT / 'addons/RICON_CoreTests/Scripts/Game/RICON_Tests/RICON_VegetationFixtures.c'
SHA = '576fed29dd79f83ec045fabb10ded6a52103a33a53935d2ee270299668657ca9'
HOST_ONLY = {'V59'}
CODES = ('INVALID_OBSERVATION', 'INVALID_INPUT', 'VEGETATION_COVERAGE_INCOMPLETE', 'VEGETATION_UNKNOWN',
         'NO_VEGETATION', 'VEGETATION_MIXED_PROFILE', 'NO_MODEL_FOR_SCENARIO', 'NUMERIC_FAILURE', 'OK')
PROFILE_STRINGS = {'climate': 'Climate', 'leafState': 'LeafState', 'density': 'Density',
                   'moisture': 'Moisture', 'gammaProvenance': 'GammaProvenance'}
INTERVAL_STRINGS = {'materialId': 'MaterialId', 'scenario': 'Scenario', 'primaryModel': 'PrimaryModel',
                    'primaryStatus': 'PrimaryStatus', 'depthTrend': 'DepthTrend', 'excessBaseline': 'ExcessBaseline',
                    'gammaSource': 'GammaSource', 'gammaProvenance': 'GammaProvenance', 'amProvenance': 'AmProvenance'}
INTERVAL_FLOATS = {'startM': 'StartM', 'endM': 'EndM', 'primaryLossDb': 'PrimaryLossDb',
                   'spreadMinDb': 'SpreadMinDb', 'spreadMaxDb': 'SpreadMaxDb', 'exdDiagnosticDb': 'ExdDiagnosticDb'}


def profile_lines(index, material, p):
    need(type(p['negligible']) is bool, 'Ожидался bool negligible')
    fields = {'negligible'} if p['negligible'] else {'negligible', 'gammaTables', 'am'} | set(PROFILE_STRINGS)
    need(set(p) == fields, 'Невыразимые поля профиля')
    lines = [f'\tstatic RICON_VegetationProfile Profile{index}()', '\t{',
             '\t\tRICON_VegetationProfile p = new RICON_VegetationProfile();',
             f'\t\tp.m_sMaterialId = {text(material)};', f'\t\tp.m_bNegligible = {value_literal(p["negligible"], "bool")};']
    if not p['negligible']:
        lines += [f'\t\tp.m_s{field} = {text(p[key])};' for key, field in PROFILE_STRINGS.items()]
        need(type(p['gammaTables']) is dict, 'Неверные таблицы')
        for i, (pol, points) in enumerate(p['gammaTables'].items()):
            name = f'table{i}'
            lines += [f'\t\tRICON_VegetationTable {name} = new RICON_VegetationTable();',
                      f'\t\t{name}.m_sPolarization = {text(pol)};']
            for j, pair in enumerate(points):
                need(type(pair) is list and len(pair) == 2, 'Неверная точка')
                point = f'point{i}_{j}'
                lines += [f'\t\tRICON_VegetationPoint {point} = new RICON_VegetationPoint();',
                          f'\t\t{point}.m_fFrequencyMHz = {value_literal(pair[0], "float")};',
                          f'\t\t{point}.m_fGamma = {value_literal(pair[1], "float")};',
                          f'\t\t{name}.m_aPoints.Insert({point});']
            lines += [f'\t\tp.m_aTables.Insert({name});']
        if p['am'] is not None:
            am = p['am']
            need(set(am) == {'a1', 'alpha', 'minMHz', 'maxMHz', 'provenance'}, 'Неверный Am')
            lines += ['\t\tp.m_Am = new RICON_VegetationAm();']
            for key, field in (('a1', 'A1'), ('alpha', 'Alpha'), ('minMHz', 'MinMHz'), ('maxMHz', 'MaxMHz')):
                lines.append(f'\t\tp.m_Am.m_f{field} = {value_literal(am[key], "float")};')
            lines.append(f'\t\tp.m_Am.m_sProvenance = {text(am["provenance"])};')
    return lines + ['\t\treturn p;', '\t}', '']


def interval_lines(name, item, uncovered=False):
    fields = {'startM', 'endM', 'scenario'} if uncovered else set(INTERVAL_STRINGS) | set(INTERVAL_FLOATS) | {
        'scenarioClosed', 'capApplied', 'gammaDbPerM', 'candidates'}
    need(set(item) == fields, 'Неверные поля интервала')
    lines = [f'\t\tRICON_VegetationInterval {name} = new RICON_VegetationInterval();']
    for key, field in INTERVAL_STRINGS.items():
        if key in item and item[key] is not None:
            lines.append(f'\t\t{name}.m_s{field} = {text(item[key])};')
    for key, field in INTERVAL_FLOATS.items():
        if key in item:
            lines.append(f'\t\t{name}.m_f{field} = {value_literal(item[key], "float")};')
    if not uncovered:
        for key, field in (('scenarioClosed', 'ScenarioClosed'), ('capApplied', 'CapApplied')):
            lines.append(f'\t\t{name}.m_b{field} = {value_literal(item[key], "bool")};')
        if item['gammaDbPerM'] is not None:
            lines += [f'\t\t{name}.m_bHasGamma = true;',
                      f'\t\t{name}.m_fGamma = {value_literal(item["gammaDbPerM"], "float")};']
        for i, c in enumerate(item['candidates']):
            need(set(c) == {'model', 'lossDb', 'status'}, 'Неверный кандидат')
            candidate = f'{name}c{i}'
            lines += [f'\t\tRICON_VegetationCandidate {candidate} = new RICON_VegetationCandidate();',
                      f'\t\t{candidate}.m_sModel = {text(c["model"])};',
                      f'\t\t{candidate}.m_fLossDb = {value_literal(c["lossDb"], "float")};',
                      f'\t\t{candidate}.m_sStatus = {text(c["status"])};', f'\t\t{name}.m_aCandidates.Insert({candidate});']
    return lines


def case_lines(index, c, profile_indices):
    need(set(c) == {'id', 'title', 'input', 'expect'} and re.fullmatch(r'V[0-9]{2}', c['id']), 'Неверный случай')
    inp, exp = c['input'], c['expect']
    need(set(inp) == {'observation', 'catalog', 'polarization', 'txHeightM', 'rxHeightM'}, 'Неверный вход')
    need(exp['code'] in CODES, 'Неверный код')
    exp_fields = {'code'}
    if exp['code'] == 'OK':
        exp_fields |= {'lossDb', 'intervals', 'allScenariosClosed'}
    elif exp['code'] == 'NO_VEGETATION':
        exp_fields.add('lossDb')
    elif exp['code'] == 'NO_MODEL_FOR_SCENARIO':
        exp_fields.add('interval')
    need(set(exp) == exp_fields, 'Неверные поля результата')
    lines = [f'\tprotected static void Case{index}(RICON_VegetationRun t)', '\t{'] + observation_lines('observation', inp['observation'])
    lines += ['\t\tRICON_VegetationInput input = new RICON_VegetationInput();', '\t\tinput.m_Observation = observation;',
              f'\t\tinput.m_sPolarization = {text(inp["polarization"])};']
    for key, field in (('txHeightM', 'TxHeight'), ('rxHeightM', 'RxHeight')):
        if inp[key] is not None:
            lines += [f'\t\tinput.m_bHas{field} = true;', f'\t\tinput.m_f{field}M = {value_literal(inp[key], "float")};']
    for material, p in inp['catalog'].items():
        i = profile_indices[json.dumps([material, p], sort_keys=True)]
        lines.append(f'\t\tinput.m_aCatalog.Insert(Profile{i}());')
    lines += ['\t\tRICON_VegetationResult expected = new RICON_VegetationResult();',
              f'\t\texpected.m_eCode = RICON_EVegetationCode.{exp["code"]};']
    if 'lossDb' in exp:
        lines += ['\t\texpected.m_bHasLoss = true;', f'\t\texpected.m_fLossDb = {value_literal(exp["lossDb"], "float")};']
    if exp['code'] == 'OK':
        lines.append(f'\t\texpected.m_bAllScenariosClosed = {value_literal(exp["allScenariosClosed"], "bool")};')
        for j, item in enumerate(exp['intervals']):
            lines += interval_lines(f'interval{j}', item) + [f'\t\texpected.m_aIntervals.Insert(interval{j});']
    if 'interval' in exp:
        lines += interval_lines('uncovered', exp['interval'], uncovered=True) + ['\t\texpected.m_Uncovered = uncovered;']
    return lines + [f'\t\tt.CheckCase({text(c["id"])}, input, expected);', '\t}', '']


def build(raw):
    need(hashlib.sha256(raw).hexdigest() == SHA, 'SHA frozen корпуса изменён')
    doc = json.loads(raw)
    cs = doc['cases']
    need(len(cs) == 59 and [c['id'] for c in cs] == [f'V{i:02}' for i in range(1, 60)], 'Неверный набор случаев')
    need(next(c for c in cs if c['id'] == 'V59')['expect'] == {'code': 'INVALID_INPUT'}, 'Неверный host-only')
    native = [c for c in cs if c['id'] not in HOST_ONLY]
    profiles, profile_indices = [], {}
    for c in native:
        for material, p in c['input']['catalog'].items():
            key = json.dumps([material, p], sort_keys=True)
            if key not in profile_indices:
                profile_indices[key] = len(profiles)
                profiles.append((material, p))
    lines = ['//! Сгенерировано gen_vegetation_fixture.py: ожидания только копируются, TEST_ONLY.',
             '//! V59 host-only: лишнее JSON-поле season невыразимо типизированным DTO.',
             'class RICON_VegetationFixtures', '{', f'\tstatic const string SOURCE_SHA = "{SHA}";',
             '\tstatic const int CASES = 58;', '\tstatic const int HOST_ONLY = 1;', '']
    for i, (material, p) in enumerate(profiles):
        lines += profile_lines(i, material, p)
    for i, c in enumerate(native):
        lines += case_lines(i, c, profile_indices)
    lines += ['\tstatic void Run(RICON_VegetationRun t)', '\t{'] + [f'\t\tCase{i}(t);' for i in range(len(native))]
    return '\n'.join(lines + ['\t}', '}', ''])


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--source', type=Path, default=SOURCE)
    parser.add_argument('--target', type=Path, default=TARGET)
    args = parser.parse_args(argv)
    try:
        output = build(args.source.read_bytes())
        if args.check:
            need(args.target.read_text(encoding='utf-8') == output, 'Фикстура отличается')
        else:
            args.target.write_text(output, encoding='utf-8', newline='\n')
    except (OSError, KeyError, TypeError, ValueError) as error:
        print(f'P4_GENERATOR_FAIL: {error}')
        return 1
    print('P4_GENERATOR_PASS: 58 native, 1 host-only')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
