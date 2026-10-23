#!/usr/bin/env python3
"""P4.2 TOOL_ONLY: копирует frozen JSON в Enforce без пересчёта ожиданий."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from gen_vegetation_fixture import need, text, value_literal, observation_lines, profile_lines

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'oracle/vectors/vegetation_overcanopy_v1.json'
TARGET = ROOT / 'addons/RICON_CoreTests/Scripts/Game/RICON_Tests/RICON_OvercanopyFixtures.c'
SHA = '00d7da2115f5d27445ae40502ef6fa3541e027e85a4275b7ba80264664a99cf2'
CODES = ('INVALID_OBSERVATION', 'INVALID_INPUT', 'VEGETATION_COVERAGE_INCOMPLETE', 'VEGETATION_UNKNOWN',
         'NO_VEGETATION', 'VEGETATION_MIXED_PROFILE', 'NO_TERMINAL_IN_VEGETATION', 'OUT_OF_MODEL_DOMAIN', 'NUMERIC_FAILURE', 'OK')
STRINGS = {'end': 'End', 'materialId': 'MaterialId', 'canopyProvenance': 'CanopyProvenance'}
FLOATS = {'intervalStartM': 'IntervalStartM', 'intervalEndM': 'IntervalEndM', 'antennaHeightM': 'AntennaHeightM',
          'canopyHeightM': 'CanopyHeightM', 'hdifM': 'HdifM', 'thetaDeg': 'ThetaDeg', 'nu': 'Nu', 'jDb': 'JDb', 'ahDb': 'AhDb'}


def input_lines(inp, profiles):
    """Общий типизированный вход P4.2/P4.3; только перенос полей frozen JSON."""
    need(set(inp) == {'observation', 'catalog', 'polarization', 'txHeightM', 'rxHeightM', 'canopy'}, 'Неверный вход')
    lines = observation_lines('observation', inp['observation'])
    lines += ['\t\tRICON_OvercanopyInput input = new RICON_OvercanopyInput();', '\t\tinput.m_Observation = observation;',
              f'\t\tinput.m_sPolarization = {text(inp["polarization"])};']
    for key, field in (('txHeightM', 'TxHeight'), ('rxHeightM', 'RxHeight')):
        if inp[key] is not None:
            lines += [f'\t\tinput.m_bHas{field} = true;', f'\t\tinput.m_f{field}M = {value_literal(inp[key], "float")};']
    for material, p in inp['catalog'].items():
        lines.append(f'\t\tinput.m_aCatalog.Insert(Profile{profiles[json.dumps([material, p], sort_keys=True)]}());')
    for i, (material, entry) in enumerate(inp['canopy'].items()):
        need(set(entry) == {'heightM', 'provenance'}, 'Неверная запись крон')
        name = f'canopy{i}'
        lines += [f'\t\tRICON_CanopyEntry {name} = new RICON_CanopyEntry();',
                  f'\t\t{name}.m_sMaterialId = {text(material)};', f'\t\t{name}.m_sProvenance = {text(entry["provenance"])};']
        if entry['heightM'] is not None:
            lines += [f'\t\t{name}.m_bHasHeight = true;', f'\t\t{name}.m_fHeightM = {value_literal(entry["heightM"], "float")};']
        lines.append(f'\t\tinput.m_aCanopy.Insert({name});')
    return lines


def case_lines(index, c, profiles):
    need(set(c) == {'id', 'title', 'input', 'expect'} and re.fullmatch(r'O[0-9]{2}', c['id']), 'Неверный случай')
    inp, exp = c['input'], c['expect']
    need(exp['code'] in CODES, 'Неверный код')
    need(set(exp) == ({'code', 'model', 'status', 'budgetMode', 'integrationClosed', 'totalAhDb', 'terminals'}
                     if exp['code'] == 'OK' else {'code'}), 'Неверные поля результата')
    lines = [f'\tprotected static void Case{index}(RICON_OvercanopyRun t)', '\t{'] + input_lines(inp, profiles)
    lines += ['\t\tRICON_OvercanopyResult expected = new RICON_OvercanopyResult();',
              f'\t\texpected.m_eCode = RICON_EOvercanopyCode.{exp["code"]};']
    if exp['code'] == 'OK':
        for key, field in (('model', 'Model'), ('status', 'Status'), ('budgetMode', 'BudgetMode')):
            lines.append(f'\t\texpected.m_s{field} = {text(exp[key])};')
        lines += [f'\t\texpected.m_bIntegrationClosed = {value_literal(exp["integrationClosed"], "bool")};',
                  f'\t\texpected.m_fTotalAhDb = {value_literal(exp["totalAhDb"], "float")};']
        for i, item in enumerate(exp['terminals']):
            need(set(item) == set(STRINGS) | set(FLOATS), 'Неверные поля терминала')
            name = f'terminal{i}'
            lines.append(f'\t\tRICON_OvercanopyTerminal {name} = new RICON_OvercanopyTerminal();')
            lines += [f'\t\t{name}.m_s{field} = {text(item[key])};' for key, field in STRINGS.items()]
            lines += [f'\t\t{name}.m_f{field} = {value_literal(item[key], "float")};' for key, field in FLOATS.items()]
            lines.append(f'\t\texpected.m_aTerminals.Insert({name});')
    return lines + [f'\t\tt.CheckCase({text(c["id"])}, input, expected);', '\t}', '']


def build(raw):
    need(hashlib.sha256(raw).hexdigest() == SHA, 'SHA frozen корпуса изменён')
    doc = json.loads(raw)
    cs = doc['cases']
    need(len(cs) == 31 and [c['id'] for c in cs] == [f'O{i:02}' for i in range(1, 32)], 'Неверный набор случаев')
    need(doc['hostOnly'] == ['O28'] and cs[27]['expect'] == {'code': 'INVALID_INPUT'}, 'Неверный host-only')
    native = [c for c in cs if c['id'] != 'O28']
    profiles, indices = [], {}
    for c in native:
        for material, p in c['input']['catalog'].items():
            key = json.dumps([material, p], sort_keys=True)
            if key not in indices:
                indices[key] = len(profiles)
                profiles.append((material, p))
    lines = ['//! Сгенерировано gen_overcanopy_fixture.py: только копирование frozen P4.2, TEST_ONLY.',
             '//! O28 host-only: лишнее поле корня JSON невыразимо типизированным DTO.',
             'class RICON_OvercanopyFixtures', '{', f'\tstatic const string SOURCE_SHA = "{SHA}";',
             '\tstatic const int CASES = 30;', '\tstatic const int HOST_ONLY = 1;', '']
    for i, (material, p) in enumerate(profiles):
        lines += profile_lines(i, material, p)
    for i, c in enumerate(native):
        lines += case_lines(i, c, indices)
    lines += ['\tstatic void Run(RICON_OvercanopyRun t)', '\t{'] + [f'\t\tCase{i}(t);' for i in range(len(native))]
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
        print(f'P42_GENERATOR_FAIL: {error}')
        return 1
    print('P42_GENERATOR_PASS: 30 native, 1 host-only')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
