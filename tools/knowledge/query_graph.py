"""Поиск узлов, прямых связей и проблем; файлы проекта не исполняются и не меняются."""
import argparse
import json
import sys
from pathlib import Path


def select(graph, query):
    term = query.casefold()
    nodes = [n for n in graph['nodes'] if term in json.dumps(n, ensure_ascii=False).casefold()]
    problems = [p for p in graph['problems'] if term in json.dumps(p, ensure_ascii=False).casefold()]
    ids = {n['id'] for n in nodes}
    ids.update(i for p in problems for i in p.get('node_ids', []))
    edges = [e for e in graph['edges'] if e['from'] in ids or e['to'] in ids]
    related = ids | {i for e in edges for i in (e['from'], e['to'])}
    return {'nodes': [n for n in graph['nodes'] if n['id'] in related], 'edges': edges,
            'problems': [p for p in graph['problems'] if p in problems or set(p.get('node_ids', [])) & ids]}


def main():
    sys.stdout.reconfigure(encoding='utf-8')
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('query', nargs='?', default='P_')
    parser.add_argument('--graph', type=Path)
    parser.add_argument('--json', action='store_true', help='Вывести найденный подграф в JSON')
    args = parser.parse_args()
    path = args.graph or Path(__file__).resolve().parents[2] / 'docs/analysis/RICON_KNOWLEDGE_GRAPH.json'
    try:
        graph = json.loads(path.read_text(encoding='utf-8-sig'))
        result = select(graph, args.query)
    except (OSError, ValueError, KeyError) as exc:
        parser.exit(2, f'Ошибка чтения графа: {exc}\n')
    if args.json:
        print(json.dumps(result, indent=2, ensure_ascii=False))
        return
    if not any(result.values()):
        print('Совпадений нет. Попробуйте ID узла, название класса или имя файла.')
        return
    for problem in result['problems']:
        print(f"\nПРОБЛЕМА {problem['id']}: {problem['title']}")
        print('  Влияние:', problem.get('impact', 'не указано'))
        print('  Следующий факт:', problem.get('next_fact', 'не указан'))
    for node in result['nodes']:
        print(f"\n{node['id']} [{node['status']}] {node['title']}")
        print(' ', node['summary'])
        for source in node.get('sources', []):
            suffix = f":{source['line']}" if source.get('line') else ''
            print(f"  Источник: {source['root']}:{source['path']}{suffix}")
    print('\nПРЯМЫЕ СВЯЗИ')
    for edge in result['edges']:
        print(f"  {edge['from']} —{edge['relation']}→ {edge['to']} [{edge['status']}]: {edge['summary']}")
    print('\nГраф служит навигацией. Перед изменением откройте owning source и проверьте текущие доказательства.')


if __name__ == '__main__':
    main()
