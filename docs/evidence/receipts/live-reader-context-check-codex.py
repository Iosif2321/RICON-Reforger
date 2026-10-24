"""Проверка воспроизведения и исправления контекста игрока, без импорта кода reader."""
import hashlib
import json
from pathlib import Path
import re
import subprocess

root = Path(__file__).resolve().parents[3]
folder = root/'docs/evidence/receipts'
names = [
    'live-reader-context-red-codex-20261024-172456-415.json',
    'live-reader-context-red-codex-20261024-173049-395.json',
    'live-reader-context-green-codex-20261024-173348-580.json',
]
expected = [(False, 18, 4, 14), (True, 19, 3, 14), (True, 22, 0, 16)]
reports, events, crcs = [], [], []
for name, (matched, passed, mismatches, count) in zip(names, expected):
    receipt = json.loads((folder/name).read_text(encoding='utf-8'))
    raw = Path(receipt['logPath']).read_bytes()
    assert hashlib.sha256(raw).hexdigest() == receipt['logSha256']
    text = raw.decode('utf-8', errors='replace')
    assert receipt['expectationMatched'] == matched and receipt['exitCode'] == 0
    assert receipt['ownPidGone'] and not receipt['timedOut'] and receipt['sourceStable']
    assert receipt['sources'] == receipt['sourcesAfter'] and receipt['scriptErrors'] == 0
    assert not re.search(r'SCRIPT\s*\(E\)', text)
    summary = f'[RICON_LIVER] checks=22 passed={passed} count={count}'
    assert text.count(summary) == 1 and text.count('[RICON_LIVER] mismatch:') == mismatches
    marker = 'PASS' if passed == 22 else 'FAIL'
    assert text.count('[RICON_LIVER] done: '+marker) == 1
    tagged = [s[s.index('[RICON_LIVE]'):].rstrip() for s in text.splitlines() if '[RICON_LIVE]' in s]
    assert len(tagged) == count+2
    assert tagged[0] == f'[RICON_LIVE] dump=BEGIN count={count} dropped=0 nextSeq={count}'
    assert tagged[-1] == '[RICON_LIVE] dump=END'
    rows = tagged[1:-1]
    assert all(line.startswith(f'[RICON_LIVE] seq={i} t=-1 side=client ') for i, line in enumerate(rows))
    events.append(rows)
    all_crc = dict(re.findall(r"Script file: '(Scripts/[^']+)' CRC: ([0-9a-f]+)", text))
    crc = {}
    for source in receipt['sources']:
        blob = subprocess.run(['git', 'show', receipt['head']+':'+source['path']], cwd=root,
                              check=True, capture_output=True).stdout
        assert hashlib.sha256(blob).hexdigest() == source['sha256'], source['path']
        if source['path'].endswith('.c'):
            relative = source['path'].removeprefix('tools/RICON_LiveProbe/')
            crc[relative] = all_crc[relative]
    assert len(crc) == 12 and len(receipt['sources']) == 13
    crcs.append(crc)
    reports.append({'receipt': name, 'head': receipt['head'], 'logSha256': receipt['logSha256'],
                    'passed': passed, 'checks': 22, 'mismatches': mismatches, 'events': count,
                    'exitCode': 0, 'marker': marker, 'sourcesChecked': 13})

# Исправление ошибочного obs4 в фикстуре не меняло поведение старого reader.
assert events[0] == events[1]
red, green = events[1:]
assert red[:11] == green[:11]
assert 'seq=11 ' in red[11] and 'player=-1 ' in red[11] and 'obs=5,' in red[11]
expected_green = red[:12] + [
    red[11].replace('seq=11 ', 'seq=12 ', 1).replace('player=-1 ', 'player=7 ', 1),
    red[11].replace('seq=11 ', 'seq=13 ', 1).replace('player=-1 ', 'player=8 ', 1),
    red[12].replace('seq=12 ', 'seq=14 ', 1),
    red[13].replace('seq=13 ', 'seq=15 ', 1).replace('player=-1 ', 'player=8 ', 1),
]
assert green == expected_green
changed_crc = {key for key in crcs[1] if crcs[1][key] != crcs[2][key]}
assert changed_crc == {
    'Scripts/Game/RICON_Live/RICON_LiveRadioReader.c',
    'Scripts/WorkbenchGame/RICON_LiveRadioReaderProbePlugin.c',
}
report = {
    'status': 'NATIVE_SYNTHETIC_CONTEXT_FIX_CONFIRMED',
    'scope': 'ScriptEditor, синтетические Managed-ключи; без мира, игрока, настоящих раций и Play',
    'runs': reports,
    'initialAttempt': 'Ожидание obs4 в новой фикстуре было ошибочным: этот номер уже использован. Исправлено на obs5 до чистого RED.',
    'red': 'Три отказа: уточнение игрока, смена игрока, GONE с новым вместо последнего наблюдавшегося ID',
    'green': 'Те же 22 критерия прошли; прежние 16 сохранены, все 16 событий сверены',
    'lastObservationPlayers': [-1, 7, 8, 8, 8],
    'observationIdPreserved': 5,
    'processExitIsNotTestVerdict': True,
    'greenEngineCrc': crcs[2],
    'checkerSha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
}
(folder/'live-reader-context-review-codex.json').write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
print('READER_CONTEXT_CONFIRMED RED=19/22 GREEN=22/22 events=16 sources=13 CRC=12')
