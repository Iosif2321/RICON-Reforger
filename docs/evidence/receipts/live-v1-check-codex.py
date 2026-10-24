"""Независимая сверка фиксированного V1 с ранее принятым bench9; код парсера Claude не используется."""
import hashlib
import json
from pathlib import Path
import re
import subprocess

root = Path(__file__).resolve().parents[3]
folder = root/'docs/evidence/receipts'


def blob(commit, path):
    return subprocess.run(['git', 'show', commit+':'+path], cwd=root, check=True, capture_output=True).stdout


baseline = json.loads(blob('07d8224', 'docs/evidence/receipts/live-bench9-review-codex.json'))
before = Path(baseline['logPath']).read_bytes()
assert hashlib.sha256(before).hexdigest() == baseline['logSha256']
before_lines = [s[s.index('[RICON_LIVE]'):].rstrip() for s in before.decode('utf-8').splitlines() if '[RICON_LIVE]' in s]
assert len(before_lines) == 94
assert before_lines[46] == '[RICON_LIVE] dump=BEGIN count=46 dropped=0 nextSeq=46'
assert before_lines[:46] == before_lines[47:93] and before_lines[-1] == '[RICON_LIVE] dump=END'

# Сравниваем принятый исторический результат целиком, меняя только время и новую запись наличия менеджера.
times = {0: 0, 1004: 1001, 2017: 2004, 3023: 3010, 7019: 7005, 9018: 9007, 11018: 11007}
expected = []
for i, line in enumerate(before_lines[:46]):
    match = re.fullmatch(r'\[RICON_LIVE\] seq=(\d+) t=(\d+) (.*)', line)
    assert match and int(match[1]) == i
    body = match[3].replace('actualDeltaMs=1006,', 'actualDeltaMs=1005,').replace('actualDeltaMs=5002,', 'actualDeltaMs=5001,')
    if i == 0:
        body += ',radioManager=1,expectedManager=1'
    expected.append(f'[RICON_LIVE] seq={i+(i>0)} t={times[int(match[2])]} {body}')
    if i == 0:
        expected.append('[RICON_LIVE] seq=1 t=1001 side=probe kind=BENCH_MANAGER player=-1 device=-1 tr=-1 '
                        'value=1 text=where=preSpawn,radioManager=1,expected=1')
assert len(expected) == 47
assert 1005 >= 1000 and 5001 >= 5000
assert abs((times[3023]-times[2017])-1005) <= 1 and times[7019]-times[2017] == 5001
transcript = expected + ['[RICON_LIVE] dump=BEGIN count=47 dropped=0 nextSeq=47'] + expected + ['[RICON_LIVE] dump=END']


def valid(raw):
    got = [s[s.index('[RICON_LIVE]'):].rstrip() for s in raw.splitlines() if '[RICON_LIVE]' in s]
    return got == transcript and not re.search(r'SCRIPT\s*\(E\)', raw)


synthetic = '\n'.join(transcript)
assert valid(synthetic) and valid(synthetic+'\nENGINE : noise')
mutations = [
    synthetic.replace(expected[1]+'\n', '', 1),
    synthetic.replace('value=1 text=where=preSpawn', 'value=0 text=where=preSpawn', 1),
    synthetic.replace('where=preSpawn,radioManager=1,expected=1', 'where=preSpawn,radioManager=1,expected=0', 1),
    synthetic.replace('activeBefore=1,radioManager=1', 'activeBefore=1,radioManager=0', 1),
    synthetic.replace(expected[2], expected[2]+'\n'+expected[2], 1),
    synthetic.replace('actualDeltaMs=5001,', 'actualDeltaMs=4999,', 1),
    synthetic.replace('value=32000 text=bench=0,phase=t5', 'value=32200 text=bench=0,phase=t5', 1),
    synthetic.replace('after2=0', 'after2=1', 1),
    synthetic.replace('dump=BEGIN count=47', 'dump=BEGIN count=46', 1),
    synthetic.replace('seq=34 t=', 'seq=35 t=', 1),
    synthetic+'\nSCRIPT (E): error',
]
assert all(s != synthetic and not valid(s) for s in mutations)
path = Path(baseline['logPath']).parents[1]/'ricon-v1-claude-20261024-143158/console.log'
raw = path.read_bytes()
sha = hashlib.sha256(raw).hexdigest()
assert sha == '557d268b1996c5d55998a1687b1827947ecb20011c688ce30be5c54021e9d169'
text = raw.decode('utf-8', errors='replace')
assert valid(text)
assert "World doesn't contain RadioManagerEntity to support any BaseRadioComponent." not in text
assert 'setup existing entities=21 gameModes=1 radioManagers=0 noInstance=0 gameModeParam=0: radioManagerParam=1:1' in text
assert 'setup created gameMode=0 gameModeOk=0 radioManager=1 via=class radioManagerOk=1' in text

source_commit = subprocess.run(['git', 'rev-parse', '81147b1'], cwd=root, check=True, capture_output=True, text=True).stdout.strip()
previous = json.loads(blob('07d8224', 'docs/evidence/receipts/live-bench-yesonce-codex-20261024-110120.json'))
sources = {s['path']: s['sha256'] for s in previous['sources']}
bench = 'tools/RICON_LiveProbe/Scripts/Game/RICON_Live/RICON_LiveRadioBenchComponent.c'
launcher = 'tools/RICON_LiveProbe/Scripts/WorkbenchGame/RICON_LiveBenchLauncherPlugin.c'
sources[bench] = 'fd6efd73ef3abeb66d7fdae760be4a5ea5a465486c2e01284947e77d7f9e4ce0'
sources[launcher] = '7daa97b011629bf00caec2ae2ad25e183408730c35d5b8b9309b93a525e76816'
for name, digest in sources.items():
    assert hashlib.sha256(blob(source_commit, name)).hexdigest() == digest, name
crc = dict(baseline['matchingEngineCrc'])
crc.update({bench: '92706d6b', launcher: '77c77ee8'})
logged_crc = dict(re.findall(r"Script file: '(Scripts/[^']+)' CRC: ([0-9a-f]+)", text))
assert len(crc) == 12 and len(sources) == 13
assert all(logged_crc.get(name.removeprefix('tools/RICON_LiveProbe/')) == value for name, value in crc.items())
report = {
    'status': 'INDEPENDENT_V1_OBSERVATION_CONFIRMED', 'functionalVerdict': 'OBSERVED_MISMATCH',
    'scope': 'Серверный Play, только native getter/setter; единица частоты, звук, RX/TX, репликация и A→B не проверены',
    'logPath': str(path), 'logSha256': sha, 'sourceCommit': source_commit, 'sources': sources, 'engineCrc': crc,
    'streamEvents': 47, 'snapshotEvents': 47, 'allEventFieldsChecked': True,
    'baselineCommit': '07d8224', 'baselineLogSha256': baseline['logSha256'],
    'comparison': 'Исторический контраст с bench9, диагностика изменилась; строгий парный опыт не выполнен',
    'managerAtStart': 1, 'managerBeforeSpawn': 1, 'managerWarningPresent': False,
    'actualDelayMs': {'t1': 1005, 't5': 5001}, 'radios': baseline['radios'],
    'conclusion': 'В проверенной конфигурации наличия RadioManagerEntity недостаточно для наблюдаемой смены частоты через BaseRadioComponent.SetTransceiverFrequency',
    'positiveNoiseControl': True, 'negativeControls': len(mutations),
    'uiEvidence': 'Нажатия UI этот проверяющий не наблюдает; квитанция оператора хранится отдельно',
    'checkerSha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
}
(folder/'live-v1-review-codex.json').write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
print('V1_REVIEW_CONFIRMED stream=47 snapshot=47 controls=11 sources=13 CRC=12 manager=1 result=OBSERVED_MISMATCH')
