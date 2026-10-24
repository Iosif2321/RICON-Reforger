"""Независимая сверка фиксированного V2 с принятым V1; код парсера Claude не используется."""
import hashlib
import json
from pathlib import Path
import re
import subprocess

root = Path(__file__).resolve().parents[3]
folder = root/'docs/evidence/receipts'


def blob(commit, path):
    return subprocess.run(['git', 'show', commit+':'+path], cwd=root, check=True, capture_output=True).stdout


baseline = json.loads(blob('a863915', 'docs/evidence/receipts/live-v1-review-codex.json'))
before = Path(baseline['logPath']).read_bytes()
assert hashlib.sha256(before).hexdigest() == baseline['logSha256']
previous = [s[s.index('[RICON_LIVE]'):].rstrip() for s in before.decode('utf-8').splitlines() if '[RICON_LIVE]' in s]
assert len(previous) == 96 and previous[47] == '[RICON_LIVE] dump=BEGIN count=47 dropped=0 nextSeq=47'
assert previous[:47] == previous[48:95] and previous[-1] == '[RICON_LIVE] dump=END'

# В историческом V2 изменены метод, чтения tr0 после шага и времена. Все остальные поля обязаны совпасть с V1.
times = {0: 0, 1001: 1007, 2004: 2013, 3010: 3013, 7005: 7019, 9007: 9014, 11007: 11018}
expected = []
updated_reads = 0
for i, line in enumerate(previous[:47]):
    match = re.fullmatch(r'\[RICON_LIVE\] seq=(\d+) t=(\d+) (.*)', line)
    assert match and int(match[1]) == i
    body = match[3].replace('actualDeltaMs=1005,', 'actualDeltaMs=1000,').replace('actualDeltaMs=5001,', 'actualDeltaMs=5006,')
    if 'kind=BENCH_START ' in body:
        body += ',setter=transceiver,setterGiven=1'
    elif 'kind=BENCH_SET_FREQUENCY ' in body:
        body += ',setter=transceiver'
    elif 'kind=BENCH_READ ' in body and ' tr=0 ' in body and re.search(r'phase=t(?:0|1|5),', body):
        target = re.search(r'targetTr0=(\d+)', body)[1]
        body = re.sub(r' value=-?\d+ ', f' value={target} ', body, count=1)
        updated_reads += 1
    expected.append(f'[RICON_LIVE] seq={i} t={times[int(match[2])]} {body}')
assert updated_reads == 12
transcript = expected + ['[RICON_LIVE] dump=BEGIN count=47 dropped=0 nextSeq=47'] + expected + ['[RICON_LIVE] dump=END']


def valid(raw):
    got = [s[s.index('[RICON_LIVE]'):].rstrip() for s in raw.splitlines() if '[RICON_LIVE]' in s]
    return got == transcript and not re.search(r'SCRIPT\s*\(E\)', raw)


synthetic = '\n'.join(transcript)
assert valid(synthetic) and valid(synthetic+'\nENGINE : noise')
mutations = [
    synthetic.replace(expected[1]+'\n', '', 1),
    synthetic.replace('where=preSpawn,radioManager=1', 'where=preSpawn,radioManager=0', 1),
    synthetic.replace('setter=transceiver,setterGiven=1', 'setter=component,setterGiven=1', 1),
    synthetic.replace('res=200,setter=transceiver', 'res=200,setter=component', 1),
    synthetic.replace('value=32200 text=bench=0,phase=t5', 'value=32000 text=bench=0,phase=t5', 1),
    synthetic.replace('value=32200 text=bench=0,phase=t0', 'value=32000 text=bench=0,phase=t0', 1),
    synthetic.replace('value=48000 text=bench=0,phase=t5', 'value=48200 text=bench=0,phase=t5', 1),
    synthetic.replace('actualDeltaMs=5006,', 'actualDeltaMs=4999,', 1),
    synthetic.replace('after2=0', 'after2=1', 1),
    synthetic.replace(expected[2], expected[2]+'\n'+expected[2], 1),
    synthetic.replace('dump=BEGIN count=47', 'dump=BEGIN count=46', 1),
    synthetic+'\nSCRIPT (E): error',
]
assert all(s != synthetic and not valid(s) for s in mutations)
path = Path(baseline['logPath']).parents[1]/'ricon-v2-claude-20261024-145309/console.log'
raw = path.read_bytes()
sha = hashlib.sha256(raw).hexdigest()
assert sha == '7a9a109265c53d2ead595212e533803dacdf0ce0c8aaaf1dc48add860fd2ef05'
text = raw.decode('utf-8', errors='replace')
assert valid(text)
assert "World doesn't contain RadioManagerEntity to support any BaseRadioComponent." not in text
assert 'setup existing entities=21 gameModes=1 radioManagers=0 noInstance=0 gameModeParam=0: radioManagerParam=1:1' in text
assert 'setup created gameMode=0 gameModeOk=0 radioManager=1 via=class radioManagerOk=1' in text

source_commit = subprocess.run(['git', 'rev-parse', 'c8298bd'], cwd=root, check=True, capture_output=True, text=True).stdout.strip()
sources = dict(baseline['sources'])
bench = 'tools/RICON_LiveProbe/Scripts/Game/RICON_Live/RICON_LiveRadioBenchComponent.c'
sources[bench] = '876f8a395574e8db6e868b61d47e99b54ceca8100cff810c1a32b56dd748971e'
for name, digest in sources.items():
    assert hashlib.sha256(blob(source_commit, name)).hexdigest() == digest, name
crc = dict(baseline['engineCrc'])
crc[bench] = 'db3a17a4'
logged_crc = dict(re.findall(r"Script file: '(Scripts/[^']+)' CRC: ([0-9a-f]+)", text))
assert len(crc) == 12 and len(sources) == 13
assert all(logged_crc.get(name.removeprefix('tools/RICON_LiveProbe/')) == value for name, value in crc.items())
radios = baseline['radios']
for radio in radios:
    for field in ('t0Tr0', 't1Tr0', 't5Tr0'):
        radio[field] = radio['targetTr0']
    radio['tr1Unchanged'] = True
report = {
    'status': 'INDEPENDENT_V2_OBSERVATION_CONFIRMED', 'functionalVerdict': 'NATIVE_ROUNDTRIP_PASS',
    'scope': 'Серверный Play, только native getter/setter; единица частоты, звук, RX/TX, репликация и A→B не проверены',
    'logPath': str(path), 'logSha256': sha, 'sourceCommit': source_commit, 'sources': sources, 'engineCrc': crc,
    'streamEvents': 47, 'snapshotEvents': 47, 'allEventFieldsChecked': True,
    'baselineCommit': 'a863915', 'baselineLogSha256': baseline['logSha256'],
    'comparison': 'Исторический контраст с V1; разные процессы, причина различий методов не установлена',
    'setter': 'BaseTransceiver.SetFrequency', 'managerAtStart': 1, 'managerBeforeSpawn': 1,
    'actualDelayMs': {'t0': 0, 't1': 1000, 't5': 5006}, 'radios': radios,
    'conclusion': 'Прямой setter изменил tr0 всех четырёх раций на заданный шаг; чтения t0/t1/t5 совпали с целью, tr1 не изменились, mute переключился и восстановился',
    'positiveNoiseControl': True, 'negativeControls': len(mutations),
    'uiEvidence': 'Нажатия UI этот проверяющий не наблюдает; квитанция оператора хранится отдельно',
    'checkerSha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
}
(folder/'live-v2-review-codex.json').write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
print('V2_REVIEW_CONFIRMED stream=47 snapshot=47 controls=12 sources=13 CRC=12 result=NATIVE_ROUNDTRIP_PASS')
