"""Независимая сверка A1-E с принятым V2; без импорта парсера Claude."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
from urllib.parse import unquote_to_bytes

root = Path(__file__).resolve().parents[3]
folder = root/'docs/evidence/receipts'


def blob(commit, path):
    return subprocess.run(['git', 'show', commit+':'+path], cwd=root, check=True, capture_output=True).stdout


baseline = json.loads(blob('2254be0', 'docs/evidence/receipts/live-v2-review-codex.json'))
before = Path(baseline['logPath']).read_bytes()
assert hashlib.sha256(before).hexdigest() == baseline['logSha256']
lines = [s[s.index('[RICON_LIVE]'):].rstrip() for s in before.decode('utf-8').splitlines() if '[RICON_LIVE]' in s]
assert len(lines) == 96 and lines[:47] == lines[48:95]
times = {0: 0, 1007: 1005, 2013: 2011, 3013: 3011, 7019: 7018, 9014: 9013, 11018: 11013}
expected = []


def entry_fields(raw):
    return f'entryFreq={raw},usable=1,emuted=0,dtext={raw/1000:.1f}%20#AR-VON_FrequencyUnits_MHz'


for i, line in enumerate(lines[:47]):
    m = re.fullmatch(r'\[RICON_LIVE\] seq=(\d+) t=(\d+) (.*)', line)
    assert m and int(m[1]) == i
    body = m[3].replace('actualDeltaMs=5006,', 'actualDeltaMs=5007,')
    if 'kind=BENCH_START ' in body:
        body = body.replace('setter=transceiver', 'setter=entry') + ',playerController=0,factionManager=1'
    elif 'kind=BENCH_SET_FREQUENCY ' in body:
        target = int(re.search(r' value=(\d+) ', body)[1])
        body = body.replace(',setter=transceiver', ',modifier=1,'+entry_fields(target)+',setter=entry')
    elif 'kind=BENCH_READ ' in body and ' tr=0 ' in body and re.search(r'phase=t(?:0|1|5),', body):
        target = int(re.search(r' value=(\d+) ', body)[1])
        body += ','+entry_fields(target)
    elif 'kind=BENCH_MUTE ' in body:
        device = re.search(r'bench=(\d+),', body)[1]
        body = body.split(' text=', 1)[0] + f' text=bench={device},path=entry,before=0,ebefore=0,requested=1,ret=1,after=1,eafter=1,restoreTo=0,ret2=0,after2=0,eafter2=0,dret=0,dafter=1,edafter=1,dret2=0,dafter2=0,edafter2=0'
    elif 'kind=BENCH_DONE ' in body:
        body = body.split(' text=', 1)[0] + ' text=level=MENU_ENTRY_RUNTIME,not=SCREEN|PLAYER_VON|KNOWN_CHANNEL|RX_TX|AUDIBLE|REPLICATION|PER_PAIR'
    expected.append(f'[RICON_LIVE] seq={len(expected)} t={times[int(m[2])]} {body}')
    if i == 13:
        for device, radio in enumerate(baseline['radios']):
            raw = radio['initialTr0']
            expected.append(f'[RICON_LIVE] seq={len(expected)} t=1005 side=probe kind=BENCH_ENTRY player=-1 device=-1 tr=0 value={raw} text=bench={device},phase=init,rawBefore={raw},raw={raw},'+entry_fields(raw))
assert len(expected) == 51
transcript = expected + ['[RICON_LIVE] dump=BEGIN count=51 dropped=0 nextSeq=51'] + expected + ['[RICON_LIVE] dump=END']


def valid(raw):
    got = [s[s.index('[RICON_LIVE]'):].rstrip() for s in raw.splitlines() if '[RICON_LIVE]' in s]
    return got == transcript and not re.search(r'SCRIPT\s*\(E\)', raw)


synthetic = '\n'.join(transcript)
assert valid(synthetic) and valid(synthetic+'\nENGINE : noise')
mutations = [
    synthetic.replace(expected[14]+'\n', '', 1),
    synthetic.replace('value=32000 text=bench=0,phase=init', 'value=99999 text=bench=0,phase=init', 1),
    synthetic.replace('entryFreq=32200', 'entryFreq=32000', 1),
    synthetic.replace('dtext=32.2%20', 'dtext=32.0%20', 1),
    synthetic.replace('dtext=32.0%20', 'dtext=32.0%2', 1),
    synthetic.replace('phase=t5,actualDeltaMs=5007', 'phase=t5,actualDeltaMs=4999', 1),
    synthetic.replace('path=entry,before=0,ebefore=0,requested=1', 'path=entry,before=0,ebefore=0,requested=0', 1),
    synthetic.replace('restoreTo=0', 'restoreTo=1', 1),
    synthetic.replace('edafter=1', 'edafter=0', 1),
    synthetic.replace('emuted=0', 'emuted=1', 1),
    synthetic.replace('tr=1 value=48000', 'tr=1 value=48200', 1),
    synthetic.replace('dump=BEGIN count=51', 'dump=BEGIN count=50', 1),
    synthetic+'\nSCRIPT (E): error',
]
assert all(s != synthetic and not valid(s) for s in mutations)
path = Path(baseline['logPath']).parents[1]/'ricon-a1e-claude-20261024-192824/console.log'
raw = path.read_bytes()
sha = hashlib.sha256(raw).hexdigest()
assert sha == 'df685d35d9f4b073fa0e5d92abf55e27ed28b6c54d350b1226c9a4481d0a54e5'
text = raw.decode('utf-8', errors='replace')
assert valid(text)
points = []
for line in expected:
    if 'dtext=' not in line:
        continue
    fields = dict(p.split('=', 1) for p in line.split(' text=', 1)[1].split(','))
    display = unquote_to_bytes(fields['dtext']).decode('utf-8')
    number, label = display.split(' ', 1)
    assert label == '#AR-VON_FrequencyUnits_MHz'
    assert round(float(number)*1000) == int(fields['entryFreq'])
    points.append({'bench': int(fields['bench']), 'phase': fields.get('phase', 'step'),
                   'raw': int(fields['entryFreq']), 'text': display})
assert len(points) == 20
source_commit = subprocess.run(['git', 'rev-parse', 'beb1c65'], cwd=root, check=True, capture_output=True, text=True).stdout.strip()
paths = subprocess.run(['git', 'ls-tree', '-r', '--name-only', source_commit, '--', 'tools/RICON_LiveProbe'],
                       cwd=root, check=True, capture_output=True, text=True).stdout.splitlines()
sources = {p: hashlib.sha256(blob(source_commit, p)).hexdigest() for p in paths if p.endswith(('.c', '.gproj'))}
logged_crc = dict(re.findall(r"Script file: '(Scripts/[^']+)' CRC: ([0-9a-f]+)", text))
crc = {p: logged_crc[p.removeprefix('tools/RICON_LiveProbe/')] for p in sources if p.endswith('.c')}
assert len(sources) == 15 and len(crc) == 14
assert crc['tools/RICON_LiveProbe/Scripts/Game/RICON_Live/RICON_LiveRadioBenchComponent.c'] == '1493fae8'
compile_log = path.parents[1]/'ricon-livetext-claude-20261024-183233/console.log'
compile_raw = compile_log.read_bytes()
assert hashlib.sha256(compile_raw).hexdigest() == '94ea46dfebd860f0bf8b5fbdf697ee008eff781289c3c0f023566c6160d9dc3a'
compile_crc = dict(re.findall(r"Script file: '(Scripts/[^']+)' CRC: ([0-9a-f]+)", compile_raw.decode('utf-8', errors='replace')))
assert all(value == compile_crc[name.removeprefix('tools/RICON_LiveProbe/')] for name, value in crc.items())
report = {
    'status': 'INDEPENDENT_MENU_ENTRY_RUNTIME_PASS',
    'scope': 'Свои ванильные entry на четырёх рациях; без экранного меню игрока, GetKnownChannel, звука и репликации',
    'logPath': str(path), 'logSha256': sha, 'sourceCommit': source_commit, 'sources': sources, 'engineCrc': crc,
    'streamEvents': 51, 'snapshotEvents': 51, 'allEventFieldsChecked': True,
    'crcMatchedAcceptedCompile': 14,
    'baselineCommit': '2254be0', 'baselineLogSha256': baseline['logSha256'],
    'textPoints': points, 'tr1Unchanged': True, 'actualDelayMs': {'t0': 0, 't1': 1000, 't5': 5007},
    'muteEntryStates': [0, 1, 0], 'muteEntryReturns': [1, 0], 'muteDirectStates': [0, 1, 0],
    'muteDirectReturns': [0, 0], 'entryReadsBothMutePaths': True,
    'localControllerAtStart': False, 'factionManagerAtStart': True,
    'controllerLater': 'NOT_OBSERVED', 'channelTextOverwrite': 'RICON_BENCH',
    'positiveNoiseControl': True, 'negativeControls': len(mutations),
    'uiEvidence': 'Клик и настройки фиксирует оператор; этот checker проверяет сырой игровой журнал',
    'checkerSha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
}
(folder/'live-a1e-review-codex.json').write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
print('A1E_REVIEW_CONFIRMED stream=51 snapshot=51 text_points=20 controls=13 sources=15 CRC=14')
