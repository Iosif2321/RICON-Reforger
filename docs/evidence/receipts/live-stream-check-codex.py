"""Независимая сверка фиксированного опыта журнала, без импорта проверяющего кода Claude."""
import hashlib
import json
from pathlib import Path
import re
import subprocess

root = Path(__file__).resolve().parents[3]
folder = root / 'docs/evidence/receipts'
receipt = json.loads((folder/'live-stream-native-run-codex-20261024-060136.json').read_text(encoding='utf-8'))
own = Path(receipt['logPath'])
peer = own.parents[1]/'ricon-liveprobe2-claude-20261024-055216-RICON_LiveStreamProbePlugin/console.log'
events = [f'[RICON_LIVE] seq={i} t={5000+i} side=client kind=TX_CAPTURE player=7 device=-1 tr=-1 value=48000 text=binding=UNKNOWN' for i in range(1000)]
expected = ['[RICON_LIVES] stream=BEGIN adds=1000 capacity=8'] + events + [
    '[RICON_LIVES] stream=END', '[RICON_LIVE] dump=BEGIN count=8 dropped=993 nextSeq=1000',
    '[RICON_LIVE] seq=0 t=5992 side=probe kind=GAP player=-1 device=-1 tr=-1 value=993 text=overflow'
] + events[993:] + ['[RICON_LIVE] dump=END', '[RICON_LIVES] checks=7 passed=7 count=8 dropped=993 nextSeq=1000', '[RICON_LIVES] done: PASS']


def valid(raw):
    got = [line[line.index('[RICON_'):].rstrip() for line in raw.splitlines() if '[RICON_' in line]
    return got == expected and not re.search(r'SCRIPT\s*\(E\)', raw)


assert len(expected) == 1014
synthetic = '\n'.join(expected)
assert valid(synthetic) and valid(synthetic.replace(events[500], events[500]+'\nENGINE : noise'))
mutations = [expected+[events[0]], expected[:-1], expected+[expected[-2]], expected[:500]+expected[501:],
             expected[:1005]+[expected[1005].replace('value=48000','value=123')]+expected[1006:],
             [line.replace('value=993 text=overflow','value=992 text=overflow') for line in expected]]
assert all(not valid('\n'.join(rows)) for rows in mutations)
assert not valid(synthetic+'\nSCRIPT (E): error')
expected_sources = {s['path'].removeprefix('tools/RICON_LiveProbe/') for s in receipt['sources'] if s['path'].endswith('.c')}
assert len(expected_sources) == 6
logs, crcs = [], []
for path in (own, peer):
    raw = path.read_bytes()
    text = raw.decode('utf-8', errors='replace')
    assert valid(text), str(path)
    all_crc = dict(re.findall(r"Script file: '(Scripts/[^']+)' CRC: ([0-9a-f]+)", text))
    crc = {name: all_crc[name] for name in expected_sources if name in all_crc}
    assert set(crc) == expected_sources, 'Нет ожидаемых CRC LiveProbe'
    crcs.append(crc)
    logs.append({'path':str(path), 'sha256':hashlib.sha256(raw).hexdigest(), 'taggedLines':1014, 'streamEvents':1000, 'snapshotEvents':8})
assert crcs[0] == crcs[1]
assert receipt['sources'] == receipt['sourcesAfter'] and receipt['sourceStable'] and receipt['exitCode'] == 0 and not receipt['timedOut']
for source in receipt['sources']:
    # Сверяется принятый срез, а не более поздняя работа партнёра в общем дереве.
    blob = subprocess.run(['git', 'show', receipt['head']+':'+source['path']], cwd=root,
                          check=True, capture_output=True).stdout
    assert hashlib.sha256(blob).hexdigest() == source['sha256'], source['path']
assert logs[0]['sha256'] == receipt['logSha256']
for name, summary, tag in [
    ('ricon-liveprobe2-claude-20261024-055226-RICON_LiveJournalProbePlugin', '[RICON_LIVEJ] checks=19 passed=19 count=8 dropped=13', 'RICON_LIVEJ'),
    ('ricon-liveprobe2-claude-20261024-055235-RICON_LiveObserverProbePlugin', '[RICON_LIVEO] checks=30 passed=30 count=6 ignored=2 now=-1', 'RICON_LIVEO')]:
    path = own.parents[1]/name/'console.log'
    raw = path.read_bytes()
    text = raw.decode('utf-8', errors='replace')
    assert text.count(summary) == 1 and text.count('['+tag+'] done: PASS') == 1 and not re.search(r'SCRIPT\s*\(E\)', text)
    logs.append({'path':str(path), 'sha256':hashlib.sha256(raw).hexdigest(), 'summary':summary})
report = {'status':'NATIVE_STREAM_LOG_PASS', 'scope':'1000 синтетических записей; не звук и не Play',
          'ownProcessReceipt':'live-stream-native-run-codex-20261024-060136.json', 'sourcesChecked':len(receipt['sources']),
          'sourceCommit':receipt['head'],
          'engineCrc':crcs[0], 'positiveNoiseControl':True, 'negativeControls':7, 'logs':logs,
          'checkerPath':str(Path(__file__).resolve()), 'checkerSha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
(folder/'live-stream-log-check-codex.json').write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
print('INDEPENDENT_STREAM_PASS own=1000 peer=1000 transcript=1014 sources=7 CRC=6 controls=7 journal=19 observer=30')
