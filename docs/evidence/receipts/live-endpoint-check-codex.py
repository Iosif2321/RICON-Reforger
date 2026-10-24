"""Независимая проверка фиксированных gate-off/on и двух опытов повторного Play; не проверка сетевого клиента."""
from datetime import datetime
import hashlib
import json
from pathlib import Path
import re

folder = Path(__file__).resolve().parent
logs = Path(r'C:\Users\Ded\Documents\My Games\ArmaReforgerWorkbench\logs')
runs = [
    ('ricon-b0a-off-claude-20261024-234801', '2cb8130c8171e894a60a9002d66824b16cdc2dc587810b1c1937ecadc788f322', []),
    ('ricon-b0a-on-claude-20261024-235216', '3405ae560f096c6ee412a4a4029552ed59a17da7953f3b526ddb9ae907764007', [30]),
    ('ricon-b0a-restart-claude-20261024-235851', '8eb19eb768e58a8f0a84db496725e376228e901f3ceb8fba218cf013f00f4612', [3]),
    ('ricon-b0a-restart2-claude-20261025-001136', '8d45f3f4ec35d3f8b9dad64544381cbede1e060f422c982ebc11a4ca9f1d052e', [3, 30]),
]
pat = re.compile(r'\[RICON_LIVE\] seq=(\d+) t=(-?\d+) side=probe kind=(\w+) player=(-?\d+) device=-1 tr=(-?\d+) value=(-?\d+) text=(\S+)$')
reports = []
for name, digest, sample_counts in runs:
    path = logs / name / 'console.log'
    data = path.read_bytes()
    assert hashlib.sha256(data).hexdigest() == digest
    text = data.decode('utf-8-sig')
    assert not re.search(r'SCRIPT\s*\(E\)', text)
    assert "RICON_LiveEndpointGame.c' CRC: 45b337e3" in text and "RICON_LiveEndpointProbe.c' CRC: 0cb4c712" in text
    assert '[RICON_LIVEB] state=ENDPOINT_ONLY bench=0 saved=0' in text
    assert re.search(r'\[RICON_LIVEB\] watch .*isGameMode=1', text)
    assert '[RICON_LIVEB] lifecycle=' not in text and '[RICON_LIVEB] heartbeat' not in text
    tagged = [s[s.index('[RICON_LIVE]'):].rstrip() for s in text.splitlines() if '[RICON_LIVE]' in s]
    if not sample_counts:
        assert not tagged
        reports.append(dict(run=name, logSha256=digest, result='GATE_OFF_CONFIRMED', endpointEvents=0))
        continue
    groups, cursor = [], 0
    while cursor < len(tagged):
        stop = next(i for i in range(cursor, len(tagged)) if 'dump=BEGIN' in tagged[i])
        stream = tagged[cursor:stop]
        count = len(stream)
        assert tagged[stop] == f'[RICON_LIVE] dump=BEGIN count={count} dropped=0 nextSeq={count}'
        assert tagged[stop + 1:stop + 1 + count] == stream
        assert tagged[stop + 1 + count] == '[RICON_LIVE] dump=END'
        groups.append(stream)
        cursor = stop + count + 2
    assert len(groups) == len(sample_counts)
    cycles = []
    for ordinal, (stream, samples_expected) in enumerate(zip(groups, sample_counts), 1):
        events = [pat.fullmatch(s) for s in stream]
        assert all(events) and [int(e[1]) for e in events] == list(range(len(events)))
        expected_kinds = ['ENDPOINT_START', 'ENDPOINT_BINDING', 'ENDPOINT_SAMPLE', 'RADIO_STATE', 'RADIO_STATE']
        expected_kinds += ['ENDPOINT_SAMPLE'] * (samples_expected - 1)
        expected_kinds.append('ENDPOINT_DONE' if samples_expected == 30 else 'ENDPOINT_ABORTED')
        assert [e[3] for e in events] == expected_kinds
        assert events[0][7] == 'cycle=1,limit=30,isServer=1,isClient=0,rplMode=0'
        assert tuple(events[0][k] for k in (4, 5, 6)) == ('-1', '-1', '0')
        assert events[1][7] == 'cycle=1,revision=1,initial=1,pcChanged=1,entityChanged=1,idChanged=1,pc=1,controlled=1,previousPlayerId=-1'
        assert tuple(events[1][k] for k in (4, 5, 6)) == ('1', '-1', '1')
        elapsed, world_times, stable = [], [], []
        for e in events[1:]:
            pairs = [p.split('=', 1) for p in e[7].split(',')]
            f = dict(pairs)
            assert len(f) == len(pairs) and e[4] == '1'
            if e[3] == 'ENDPOINT_SAMPLE':
                n = int(e[6])
                assert n == len(elapsed) + 1 and e[5] == '-1'
                assert f['cycle'] == f['bindingRevision'] == f['playerId'] == f['members'] == '1'
                assert f['isServer'] == f['players'] == f['pc'] == f['controlled'] == '1'
                assert f['isClient'] == f['rplMode'] == '0' and f['vonEntries'] == '2'
                ms = int(f.pop('elapsedMs'))
                assert ms >= 1000 if not elapsed else ms - elapsed[-1] >= 999
                elapsed.append(ms)
                world_times.append(int(e[2]))
                stable.append(f)
            elif e[3] == 'RADIO_STATE':
                i = int(e[5])
                assert i in (0, 1) and e[6] == str((32000, 48000)[i])
                assert f == dict(binding='LOCAL_ONLY', obs=str(i), slot='0',
                                 prefab='{73950FBA2D7DB5C5}Prefabs/Items/Equipment/Radios/Radio_ANPRC68.et',
                                 min='32000', max='68000', res='200', powered='1', muted='0')
        assert len(elapsed) == samples_expected and all(f == stable[0] for f in stable)
        reason = 'LIMIT' if samples_expected == 30 else 'GAME_END'
        assert events[-1][6] == str(samples_expected)
        assert events[-1][7] == f'cycle=1,samples={samples_expected},reason={reason}'
        cycles.append(dict(observedOrdinal=ordinal, cycleField=1, events=len(events), samples=samples_expected,
                           bindingRevision=1, playerId=1, members=[1], rplMode=0, radioValues=[32000, 48000],
                           firstElapsedMs=elapsed[0], lastElapsedMs=elapsed[-1], firstWorldTimeMs=world_times[0],
                           lastWorldTimeMs=world_times[-1], endReason=reason, streamEqualsDump=True))
    marks = re.findall(r'\[RICON_LIVEB\] restart=(\w+)', text)
    if len(groups) == 2:
        assert marks == ['EDIT_REQUESTED', 'EDIT_STABLE', 'SECOND_PLAY_REQUESTED', 'SECOND_PLAY_ENTERED']
        times = {}
        for mark in marks:
            value = re.search(r'^([\d:.]+).*\[RICON_LIVEB\] restart=' + mark, text, re.M)[1]
            times[mark] = datetime.strptime(value, '%H:%M:%S.%f')
        assert (times['EDIT_STABLE'] - times['EDIT_REQUESTED']).total_seconds() >= 5
        assert times['EDIT_STABLE'] <= times['SECOND_PLAY_REQUESTED'] < times['SECOND_PLAY_ENTERED']
        result = 'RESTART_CLEANUP_AND_NEW_OBSERVATION_CONFIRMED'
    elif samples_expected == 3:
        assert marks == ['EDIT_REQUESTED', 'SECOND_PLAY_REQUESTED']
        assert 'SECOND_PLAY_ENTERED' not in text
        result = 'FIRST_RESTART_INCOMPLETE_SECOND_PLAY_NOT_OBSERVED'
    else:
        assert not marks
        result = 'GATE_ON_SINGLE_CYCLE_CONFIRMED'
    reports.append(dict(run=name, logSha256=digest, result=result, cycles=cycles, restartMarkers=marks))
report = dict(runs=reports, scope='Локальный read-only bootstrap в Workbench: gate, чтение роли и раций, ранний Stop и новый Play.',
              lifecycleFinding='В успешном повторе cycleField=1 в обоих Play; это локальный счётчик игрового объекта/состояния, не уникальный ID процесса. Seq, revision и radio obs начинаются заново.',
              clockFinding='World t и elapsedMs имеют разные начала; их равенство и синхронизация процессов не утверждаются.',
              limits='Отдельный Diag-клиент, listen-сервер, сетевое подключение, голос и точечное A→B не проверены. RplMode=0 сохранён как наблюдённое число.',
              replayRequirements='Нужны четыре исходных внешних console.log по фиксированным путям.',
              checkerSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
(folder / 'live-endpoint-review-codex.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
print('ENDPOINT_CONFIRMED off=clean on=30samples failed_restart=preserved restart=3+30 fresh_journals=1')
