#!/usr/bin/env python3
"""Авторский корпус P3.1: TEST_ONLY, не независимая приёмка."""
import argparse
import copy
import hashlib
import json
from pathlib import Path

import object_rf_profile_oracle as op


def build():
    cases, host = [], []

    def add(title, change=None, host_only=False, raw=False):
        value = None if raw else op.baseline()
        if change:
            change(value)
        cid = f"R{len(cases)+1:02}"
        cases.append(dict(id=cid, title=title, input=value, expect=op.evaluate(copy.deepcopy(value))))
        if host_only:
            host.append(cid)

    def clear(v):
        v['object'].update(componentProfileId='', surfaceProfileId='')

    def rules_off(v):
        clear(v)
        v.update(prefabRules={}, classRules={})

    add('Компонент побеждает поверхность, prefab и класс')
    add('Поверхность при отсутствии назначения компонента', lambda v: v['object'].update(componentProfileId=''))
    add('Ближайший prefab побеждает базовый', lambda v: (clear(v), v['prefabRules'].update({'mod.shed': 'rf.glass'})))
    add('Проверенный prefab родителя модового объекта', clear)
    add('Следующий prefab предок', lambda v: (clear(v), v['prefabRules'].pop('base.shed')))
    add('Конкретный класс при отсутствии prefab rules', lambda v: (clear(v), v.update(prefabRules={}), v['classRules'].update({'ModShed': 'rf.glass'})))
    add('Базовый класс при отсутствии prefab rules', lambda v: (clear(v), v.update(prefabRules={})))
    add('Корневой класс', lambda v: (clear(v), v.update(prefabRules={}, classRules={'Entity': 'rf.wood'})))
    add('BallisticInfo остаётся только подсказкой', rules_off)
    add('Нет данных — UNKNOWN без материала', lambda v: (rules_off(v), v['object'].update(ballisticHint='')))
    add('Неизвестный компонент блокирует fallback', lambda v: v['object'].update(componentProfileId='rf.absent'))
    add('Неизвестная поверхность блокирует fallback', lambda v: v['object'].update(componentProfileId='', surfaceProfileId='rf.absent'))
    add('Неизвестное нижнее назначение не отменяет известный компонент', lambda v: v['object'].update(surfaceProfileId='rf.absent'))
    add('Порядок записи map не влияет на результат', lambda v: (clear(v), v.update(prefabParents=dict(reversed(list(v['prefabParents'].items()))), prefabRules=dict(reversed(list(v['prefabRules'].items()))))))
    add('Пустой реестр и отсутствие сведений', lambda v: (v.update(profiles=[], prefabParents={}, classParents={}, prefabRules={}, classRules={}), v['object'].update({k: '' for k in v['object']})))
    add('Отсутствие узлов при явном профиле допустимо', lambda v: (v.update(prefabParents={}, classParents={}, prefabRules={}, classRules={}), v['object'].update(prefabId='', classId='')))
    add('Prefix имени prefab не является правилом', lambda v: (rules_off(v), v['prefabParents'].update({'mod.shed.extra': 'base.entity'}), v['prefabRules'].update({'mod.shed': 'rf.glass'}), v['object'].update(prefabId='mod.shed.extra')))
    add('Регистр класса значим', lambda v: (rules_off(v), v['classParents'].update({'modshed': 'Entity'}), v['classRules'].update({'ModShed': 'rf.glass'}), v['object'].update(classId='modshed')))
    add('Максимальный ключ 64 символа', lambda v: (v['profiles'].append('a'*64), v['object'].update(componentProfileId='a'*64)))
    add('Повторный ID профиля', lambda v: v['profiles'].append(v['profiles'][0]))
    add('Пустой ID в реестре', lambda v: v['profiles'].append(''))
    add('Цикл на пути выбранного prefab', lambda v: v['prefabParents'].update({'base.entity': 'mod.shed'}))
    add('Неиспользуемый цикл prefab запрещён даже с компонентом', lambda v: v['prefabParents'].update({'unused.a': 'unused.b', 'unused.b': 'unused.a'}))
    add('Отсутствующий родитель неиспользуемого prefab', lambda v: v['prefabParents'].update({'unused': 'absent'}))
    add('Неиспользуемый цикл класса', lambda v: v['classParents'].update({'unused.a': 'unused.b', 'unused.b': 'unused.a'}))
    add('Конкретный prefab отсутствует в графе', lambda v: v['object'].update(prefabId='absent'))
    add('Конкретный класс отсутствует в графе', lambda v: v['object'].update(classId='absent'))
    add('Правило неизвестного prefab', lambda v: v['prefabRules'].update({'absent': 'rf.wood'}))
    add('Правило ссылается на неизвестный профиль', lambda v: v['classRules'].update({'Entity': 'absent'}))
    add('Неверный ключ неиспользуемого правила', lambda v: v['prefabRules'].update({'bad key': 'rf.wood'}))
    add('Лишнее поле корня JSON', lambda v: v.update(extra=1), host_only=True)
    add('Лишнее поле объекта JSON', lambda v: v['object'].update(extra=1), host_only=True)
    add('Bool вместо ID профиля', lambda v: v['profiles'].append(True), host_only=True)
    add('Null вместо строки назначения', lambda v: v['object'].update(componentProfileId=None), host_only=True)
    add('Bool вместо родителя', lambda v: v['prefabParents'].update({'unused': True}), host_only=True)
    add('Число вместо назначения поверхности', lambda v: v['object'].update(surfaceProfileId=42), host_only=True)
    add('Null реестр профилей', lambda v: v.update(profiles=None))
    add('Null объект', lambda v: v.update(object=None))
    add('Null граф prefab', lambda v: v.update(prefabParents=None))
    add('Null правила класса', lambda v: v.update(classRules=None))
    add('Число вместо родителя', lambda v: v['classParents'].update({'unused': 42}), host_only=True)
    add('Ключ длиннее 64 символов', lambda v: v['object'].update(componentProfileId='a'*65))
    add('Null вход', raw=True)
    add('Самоссылка класса', lambda v: v['classParents'].update({'Entity': 'Entity'}))
    add('Пустая строка не является null родителем', lambda v: v['classParents'].update({'Entity': ''}))
    add('Неверный ключ нижнего назначения не скрывается компонентом', lambda v: v['object'].update(surfaceProfileId='bad key'))
    add('Пустой реестр не превращает явный профиль в воздух', lambda v: v.update(profiles=[], prefabRules={}, classRules={}))
    return dict(schema='ricon-object-rf-profile-v1', version=1, requirement='REQ-object-rf-profile v0.1 DRAFT',
                note='TEST_ONLY. Нет движка, RF-параметров, геометрии, потерь и слышимости.', hostOnly=host, cases=cases)


def canonical():
    d = build()
    head = json.dumps({k:v for k,v in d.items() if k!='cases'}, ensure_ascii=False, indent=2)[:-2]
    return head + ',\n  "cases": [\n' + ',\n'.join('    '+json.dumps(c,ensure_ascii=False) for c in d['cases']) + '\n  ]\n}\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    target = Path(__file__).with_name('vectors')/'object_rf_profile_v1.json'
    raw = canonical().encode('utf-8')
    if args.check:
        if target.read_bytes()!=raw:
            raise SystemExit('OBJECT_RF_PROFILE_VECTORS_FAIL')
    else:
        target.write_bytes(raw)
    print(f"OBJECT_RF_PROFILE_VECTORS_PASS: {len(build()['cases'])}, SHA={hashlib.sha256(raw).hexdigest()}")


if __name__=='__main__':
    main()
