"""Проверка схемы, owning references и дрейфа графа; без записи и runtime действий."""
import argparse
import hashlib
import json
from pathlib import Path
import sys


def validate(graph, roots):
    errors=[]
    nodes,edges=graph['nodes'],graph['edges']
    ids={n['id']for n in nodes}
    if len(ids)!=len(nodes):errors.append('Повторяются ID узлов')
    if len({e['id']for e in edges})!=len(edges):errors.append('Повторяются ID связей')
    if len({p['id']for p in graph['problems']})!=len(graph['problems']):errors.append('Повторяются ID проблем')
    if len({r['id']for r in graph['requirements']})!=len(graph['requirements']):errors.append('Повторяются ID требований')
    if len({r['id']for r in graph['invariants']})!=len(graph['invariants']):errors.append('Повторяются ID инвариантов')
    for e in edges:
        if e['from']not in ids or e['to']not in ids:errors.append('Неизвестный endpoint: '+e['id'])
    for p in graph['problems']:
        if not set(p['node_ids'])<=ids:errors.append('Неизвестный узел проблемы: '+p['id'])
    for r in graph['requirements']:
        if not r['related_nodes'] or not set(r['related_nodes'])<=ids:errors.append('Требование без корректных связей: '+r['id'])
    required={f'req.plan.{i:02}'for i in range(1,22)}-{'req.plan.11'}|{'req.plan.11A','req.plan.11B'}
    if {r['id']for r in graph['requirements']}!=required:errors.append('Не сохранены все 21 раздел и отдельные A/B')
    refs=[]
    for x in nodes+edges:refs.extend(x.get('sources',[]))
    refs.extend(r['source']for r in graph['requirements'])
    refs.extend(r['source']for r in graph['invariants'])
    cache={}
    for s in refs:
        if s['root']not in roots:errors.append('Неизвестный root: '+s['root']);continue
        root=roots[s['root']].resolve();p=(root/s['path']).resolve()
        if not p.is_relative_to(root):errors.append('Выход из root: '+s['path']);continue
        if not p.is_file():errors.append('Отсутствует источник: '+str(p));continue
        if p not in cache:
            raw=p.read_bytes();cache[p]=(hashlib.sha256(raw).hexdigest(),len(raw.decode('utf-8-sig').splitlines()))
        sha,lines=cache[p]
        if type(s.get('line')) is not int or not 1<=s['line']<=lines:errors.append('Некорректная строка: '+s['path'])
        if s.get('sha256')and s['sha256']!=sha:errors.append('SOURCE_DRIFT: '+s['path'])
    text=json.dumps(graph,ensure_ascii=False)
    if '\ufffd'in text or '???'in text:errors.append('Потеря кодировки в графе')
    for n in nodes:
        if n['status']not in graph['legend']:errors.append('Статус без определения: '+n['status'])
    return errors,{'nodes':len(nodes),'edges':len(edges),'problems':len(graph['problems']),
        'requirements':len(graph['requirements']),'references':len(refs),'source_files':len(cache)}


def main():
    sys.stdout.reconfigure(encoding='utf-8')
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--graph',type=Path)
    parser.add_argument('--repo',type=Path,help='Явный canonical root для проверки staged графа')
    parser.add_argument('--evidence',type=Path)
    parser.add_argument('--baseline',type=Path,help='Публикационный receipt с protected source hashes')
    args=parser.parse_args()
    path=args.graph or Path(__file__).resolve().parents[2]/'docs/analysis/RICON_KNOWLEDGE_GRAPH.json'
    try:
        g=json.loads(path.read_text(encoding='utf-8-sig'))
        roots={k:(path.parent/v).resolve()for k,v in g['roots'].items()}
        if args.repo:roots['repo']=args.repo.resolve()
        if args.evidence:roots['evidence']=args.evidence.resolve()
        errors,counts=validate(g,roots)
        if args.baseline:
            baseline=json.loads(args.baseline.read_text(encoding='utf-8-sig'))
            for pin in baseline['protected_sources']:
                p=(roots['repo']/pin['path']).resolve()
                if not p.is_relative_to(roots['repo'])or not p.is_file()or hashlib.sha256(p.read_bytes()).hexdigest()!=pin['sha256']:
                    errors.append('Изменён protected source: '+pin['path'])
        print(json.dumps({'status':'GRAPH_VALIDATED_SOURCE_SCOPE_ONLY'if not errors else'GRAPH_VALIDATION_FAILED',
                          **counts,'errors':errors,'runtime_qualified':False},ensure_ascii=False))
        raise SystemExit(bool(errors))
    except (OSError,ValueError,KeyError)as exc:parser.exit(2,f'Ошибка валидации: {exc}\n')


if __name__=='__main__':main()
