"""Независимая сверка фиксированного опыта bench9; код проверяющего Claude не используется."""
import hashlib
import json
from pathlib import Path
import re
import subprocess

root = Path(__file__).resolve().parents[3]
folder = root / 'docs/evidence/receipts'
receipt = json.loads((folder/'live-bench-yesonce-codex-20261024-110120.json').read_text(encoding='utf-8-sig'))
own = Path(receipt['logPath'])
peer = own.parents[1]/'ricon-bench9-claude-20261024-112223/console.log'
prefabs = [
    '{73950FBA2D7DB5C5}Prefabs/Items/Equipment/Radios/Radio_ANPRC68.et',
    '{9B6B61BB3FE3DFB0}Prefabs/Items/Equipment/Radios/Radio_ANPRC77.et',
    '{54C68E438DD34265}Prefabs/Items/Equipment/Radios/Radio_R107M.et',
    '{73EFD7591F4134D3}Prefabs/Items/Equipment/Radios/Radio_RF10/Radio_RF10.et',
]
frequencies = [(32000, 48000), (48000, 42000), (48000, 42000), (42000, 39000)]
ranges = [((32000, 68000), (32000, 68000)), ((30000, 76000), (30000, 76000)),
          ((30000, 76000), (20000, 52000)), ((30000, 76000), (20000, 52000))]
expected = []


def add(t, kind, text, tr=-1, value=-1):
    expected.append(f'[RICON_LIVE] seq={len(expected)} t={t} side=probe kind={kind} '
                    f'player=-1 device=-1 tr={tr} value={value} text={text}')


def reads(i, phase, t, delta):
    for tr in (0, 1):
        low, high = ranges[i][tr]
        target = frequencies[i][0] + 200 if tr == 0 and phase != 'spawn' else -1
        add(t, 'BENCH_READ', f'bench={i},phase={phase},actualDeltaMs={delta},min={low},max={high},'
            f'res=200,powered=1,muted=0,targetTr0={target}', tr, frequencies[i][tr])


add(0, 'BENCH_START', 'level=NATIVE_ROUNDTRIP,prefabs=4,isServer=1,isClient=0,from=EOnInit,activeBefore=1')
for i, prefab in enumerate(prefabs):
    add(1004, 'BENCH_SPAWN', f'bench={i},prefab={prefab},actualPrefab={prefab}', value=1)
for i in range(4):
    reads(i, 'spawn', 1004, -1)
for i in range(4):
    low, high = ranges[i][0]
    add(2017, 'BENCH_SET_FREQUENCY', f'bench={i},current={frequencies[i][0]},min={low},max={high},res=200',
        0, frequencies[i][0]+200)
    reads(i, 't0', 2017, 0)
for phase, t in [('t1', 3023), ('t5', 7019)]:
    for i in range(4):
        reads(i, phase, t, t-2017)
for i in range(4):
    add(9018, 'BENCH_MUTE', f'bench={i},before=0,requested=1,ret=0,after=1,restoreTo=0,ret2=0,after2=0', 0)
add(11018, 'BENCH_DONE', 'level=NATIVE_ROUNDTRIP,not=UI_MHz|RX_TX|AUDIBLE|REPLICATION|PER_PAIR', value=6)
assert len(expected) == 46
transcript = expected + ['[RICON_LIVE] dump=BEGIN count=46 dropped=0 nextSeq=46'] + expected + ['[RICON_LIVE] dump=END']


def valid(raw):
    lines = [line[line.index('[RICON_LIVE]'):].rstrip() for line in raw.splitlines() if '[RICON_LIVE]' in line]
    return lines == transcript and not re.search(r'SCRIPT\s*\(E\)', raw)


synthetic = '\n'.join(transcript)
assert valid(synthetic) and valid(synthetic+'\nENGINE : noise')
mutations = [
    synthetic.replace(expected[5]+'\n', '', 1),
    synthetic.replace(expected[5], expected[5]+'\n'+expected[5], 1),
    synthetic.replace('kind=BENCH_SET_FREQUENCY', 'kind=BENCH_SKIP', 1),
    synthetic.replace('actualDeltaMs=5002', 'actualDeltaMs=4999', 1),
    synthetic.replace('after2=0', 'after2=1', 1),
    synthetic.replace('seq=33 ', 'seq=34 ', 1),
    synthetic.replace(transcript[-1], ''),
    synthetic+'\nSCRIPT (E): error',
    synthetic.replace('prefab='+prefabs[0], 'prefab=UNKNOWN', 1),
]
assert all(not valid(raw) for raw in mutations)
raw = peer.read_bytes()
text = raw.decode('utf-8', errors='replace')
peer_sha = hashlib.sha256(raw).hexdigest()
assert peer_sha == '654b0be02743168d2b212b9b1fe0587a1d1c55c0147a351af7bff76745d5e124'
assert valid(text)
own_raw = own.read_bytes()
assert hashlib.sha256(own_raw).hexdigest() == receipt['logSha256']
assert receipt['sourceStable'] and receipt['sources'] == receipt['sourcesAfter']
assert receipt['reason'] == 'TIMEOUT' and receipt['uiAction']['yesClicks'] == 0
assert not receipt['uiAction']['authorizationConsumed'] and receipt['ownPidGone']
crcs = []
for log in (own_raw.decode('utf-8', errors='replace'), text):
    all_crc = dict(re.findall(r"Script file: '(Scripts/[^']+)' CRC: ([0-9a-f]+)", log))
    crcs.append({s['path']: all_crc[s['path'].removeprefix('tools/RICON_LiveProbe/')]
                 for s in receipt['sources'] if s['path'].endswith('.c')})
assert crcs[0] == crcs[1] and len(crcs[0]) == 12
for source in receipt['sources']:
    blob = subprocess.run(['git', 'show', receipt['head']+':'+source['path']], cwd=root,
                          check=True, capture_output=True).stdout
    assert hashlib.sha256(blob).hexdigest() == source['sha256'], source['path']
warning = "World doesn't contain RadioManagerEntity to support any BaseRadioComponent."
assert text.count(warning) == 1 and text.index(warning) < text.index(expected[1])
report = {
    'status': 'INDEPENDENT_BENCH9_OBSERVATION_CONFIRMED', 'functionalVerdict': 'OBSERVED_MISMATCH',
    'scope': 'Серверный Play: чтение/вызов native API; единица частоты, звук, RX/TX, репликация и A→B не проверены',
    'logPath': str(peer), 'logSha256': peer_sha, 'streamEvents': 46, 'snapshotEvents': 46,
    'allEventFieldsChecked': True, 'negativeControls': len(mutations), 'positiveNoiseControl': True,
    'sourceCommit': receipt['head'], 'sourcesChecked': len(receipt['sources']), 'matchingEngineCrc': crcs[0],
    'actualDelayMs': {'t1': 1006, 't5': 5002},
    'radios': [{'prefab': p, 'initialTr0': frequencies[i][0], 'targetTr0': frequencies[i][0]+200,
                't0Tr0': frequencies[i][0], 't1Tr0': frequencies[i][0], 't5Tr0': frequencies[i][0],
                'muteBeforeAfterRestored': [0, 1, 0], 'muteReturns': [0, 0]}
               for i, p in enumerate(prefabs)],
    'warning': warning, 'warningCausality': 'НЕ УСТАНОВЛЕНА',
    'permission': {'operator': 'claude', 'reportedYesCount': 1, 'remainingYesCount': 0,
                   'evidence': 'live-bench-wb-claude-20261024.txt; запись оператора, не независимая видеозапись'},
    'ownEarlierAttempt': {'receipt': 'live-bench-yesonce-codex-20261024-110120.json', 'yesClicks': 0,
                          'result': receipt['uiAction']['result']},
    'checkerSha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
}
(folder/'live-bench9-review-codex.json').write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
print('BENCH9_REVIEW_CONFIRMED stream=46 snapshot=46 controls=9 sources=13 CRC=12 result=OBSERVED_MISMATCH')
