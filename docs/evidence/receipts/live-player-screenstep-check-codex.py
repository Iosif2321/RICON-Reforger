"""Независимая проверка фиксированного A1-S2: журнал, Core.TryRead и файлы трёх просмотренных кадров."""
from datetime import datetime
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
from urllib.parse import unquote_to_bytes

folder = Path(__file__).resolve().parent
repo = folder.parents[2]
run = 'ricon-a1s2-claude-20261024-221205'
path = Path(r'C:\Users\Ded\Documents\My Games\ArmaReforgerWorkbench\logs') / run / 'console.log'
raw = path.read_bytes()
sha = hashlib.sha256(raw).hexdigest()
assert sha == '90440dc2e8310bad511f0d8a2cd7eff7bc1c5bf23a0f8ede813bbfe1152e39b8'
text = raw.decode('utf-8-sig')
assert not re.search(r'SCRIPT\s*\(E\)', text) and "Unknown keyword/data 'm_VONMenu'" not in text
for name, crc in [('RICON_LiveRadioBenchComponent.c', '6cbe1f6e'), ('RICON_LiveVONMenuAccess.c', 'dff4ab3a'),
                  ('RICON_RadioDeviceAdapter.c', '08a7f37f'), ('RICON_CommsManagerComponent.c', '034528a3')]:
    assert name + "' CRC: " + crc in text
tagged = [s[s.index('[RICON_LIVE]'):].rstrip() for s in text.splitlines() if '[RICON_LIVE]' in s]
assert len(tagged) == 242 and tagged[120] == '[RICON_LIVE] dump=BEGIN count=120 dropped=0 nextSeq=120'
assert tagged[:120] == tagged[121:241] and tagged[-1] == '[RICON_LIVE] dump=END'
pattern = re.compile(r'\[RICON_LIVE\] seq=(\d+) t=(-?\d+) side=probe kind=(\w+) player=-1 device=-1 tr=-1 value=(-?\d+) text=(\S+)$')
events = [pattern.fullmatch(s) for s in tagged[:120]]
assert all(events) and [int(e[1]) for e in events] == list(range(120))
act_plan = {5: 'CHOOSE', 9: 'STEP', 10: 'READ', 14: 'READ', 15: 'RESTORE', 20: 'FINAL'}
reader_plan = {7: ('PRE', 48000), 10: ('POST', 48200), 17: ('BACK', 48000)}
widget_samples = (7, 8, 10, 14, 17)
menu_samples = (5, 6, 7, 8, 10, 14, 17, 20, 30)
kinds = ['BENCH_START']
for n in range(1, 31):
    kinds += ['PLAYER_SAMPLE', 'PLAYER_ENTRY', 'PLAYER_ENTRY']
    if n in menu_samples:
        kinds += ['SCREEN'] * (3 if n in widget_samples else 1)
    if n in act_plan:
        kinds.append('PLAYER_ACT')
    if n in reader_plan:
        kinds.append('PLAYER_READER')
kinds.append('BENCH_DONE')
assert [e[3] for e in events] == kinds

samples, entries, acts, readers, widgets, menus = [], {}, [], [], [], []
for e in events:
    n = int(e[4])
    pairs = [p.split('=', 1) for p in e[5].split(',')]
    f = dict(pairs)
    assert len(pairs) == len(f)
    if e[3] == 'PLAYER_SAMPLE':
        assert n == len(samples) + 1 and f['playerId'] == f['pc'] == f['players'] == f['isServer'] == '1'
        assert f['isClient'] == '0' and f['vonEntries'] == '2'
        ms = int(f.pop('elapsedMs'))
        assert abs(ms - int(e[2])) <= 1 and (ms >= 1000 if not samples else ms - samples[-1]['ms'] >= 999)
        samples.append({'ms': ms, 'state': f})
    elif e[3] == 'PLAYER_ENTRY':
        i = int(f['idx'])
        assert i in (0, 1)
        freq = 32000 if i == 0 else 48200 if 10 <= n <= 15 else 48000
        assert f['raw'] == f['entryFreq'] == str(freq) and f['muted'] == f['emuted'] == '0'
        assert f['active'] == f['transmit'] == str(1 - i)
        assert f['trIndex'] == str(i) and f['trNumber'] == str(i + 1)
        assert f['gadgetOwnerMatch'] == '1' and f['inventorySlot'] == f['editorRadio'] == '0'
        assert f['dtext'] == f'{freq / 1000:.1f}%20#AR-VON_FrequencyUnits_MHz'
        entries.setdefault(n, []).append(f)
    elif e[3] == 'PLAYER_ACT':
        op = act_plan[n]
        freq = 48200 if op in ('STEP', 'READ') else 48000
        assert f['op'] == op and f['idx'] == f['bound'] == '1'
        assert f['raw'] == f['entryFreq'] == str(freq) and f['muted'] == f['emuted'] == '0'
        assert f['dtext'] == f'{freq / 1000:.1f}%20#AR-VON_FrequencyUnits_MHz'
        if op == 'CHOOSE':
            assert all(f[k] == v for k, v in dict(raw0='48000', target='48200', res='200', modifier='1', trNumber='2', slot='0').items())
        if op in ('STEP', 'READ'):
            phase, delay = {9: ('t0', 0), 10: ('t1', 1004), 14: ('t5', 5015)}[n]
            assert f['phase'] == phase and int(f['sinceStepMs']) == delay and f['target'] == '48200'
            assert int(e[2]) - 9037 == delay
        acts.append({'sample': n, 'gameMs': int(e[2]), **f})
    elif e[3] == 'PLAYER_READER':
        phase, freq = reader_plan[n]
        assert f == dict(phase=phase, idx='1', bound='1', authority='1', deviceId='1', trIndex='1', ok='1',
                         obsDevice='1', obsTr='1', tuning=str(freq), powered='1', rawBefore=str(freq), rawAfter=str(freq))
        readers.append({'sample': n, **f})
    elif e[3] == 'SCREEN':
        if f['op'] == 'WIDGET':
            i = int(f['idx'])
            freq = 32000 if i == 0 else 48200 if n in (10, 14) else 48000
            assert all(f[k] == '1' for k in ('component', 'root', 'rootVis', 'f', 'fVis'))
            assert unquote_to_bytes(f['fText']).decode('utf-8') == f'{freq / 1000:.1f} MHz'
            assert int(f['fW']) > 0 and int(f['fH']) > 0
            widgets.append({'sample': n, 'idx': i, 'text': unquote_to_bytes(f['fText']).decode('utf-8')})
        else:
            assert f['selectedId'] == f['activeIdx'] == '0'
            assert f['opened'] == f['hasDisplay'] == f['inputs'] == ('0' if n == 5 else '1')
            assert f['op'] == ('BEFORE' if n == 5 else 'OPEN' if n == 6 else 'MENU')
            if n == 6:
                assert all(f[k] == v for k, v in dict(openAction='VONMenu', controls='1', hadControl='0', closeOnRelease='0', performOnClose='0', closeOnPerform='1').items())
            menus.append(n)
assert len(samples) == 30 and all(s['state'] == samples[0]['state'] for s in samples)
assert list(entries) == list(range(1, 31)) and all([e['idx'] for e in rows] == ['0', '1'] for rows in entries.values())
for rows in entries.values():
    assert rows[0] == entries[1][0]
    assert {k: v for k, v in rows[1].items() if k not in ('raw', 'entryFreq', 'dtext')} == {k: v for k, v in entries[1][1].items() if k not in ('raw', 'entryFreq', 'dtext')}
assert entries[4] == entries[21] == entries[30]
assert len(acts) == 6 and len(readers) == 3 and menus == list(menu_samples)
assert [(w['sample'], w['idx']) for w in widgets] == [(n, i) for n in widget_samples for i in (0, 1)]
assert events[-1][5] == 'level=PLAYER_SCREEN_STEP,not=PIXELS|LOCALIZATION|INPUT|AUDIO|REPLICATION'

# Время захвата взято из неизменяемой квитанции оператора и сверяется со временем действий в сыром логе.
receipt_ref = 'a433072:docs/evidence/receipts/live-bench-wb-claude-20261024.txt'
receipt = subprocess.check_output(['git', 'show', receipt_ref], cwd=repo).decode('utf-8')
capture = re.findall(r'#\s+(PRE|POST|BACK)\s+marker ([\d:.]+), capture ([\d:.]+)-\.(\d{3}), 1936 x 1056, ok, sha256\s*\n#\s+([0-9a-f]{64})', receipt)
assert [c[0] for c in capture] == ['PRE', 'POST', 'BACK']
scratch = Path(r'C:\Users\Ded\AppData\Local\Temp\claude\C--Users-Ded-Documents-Projects-ARMA-3-Scripts-RICON-Reforger\583c292f-5bc1-43e3-9045-3fc153e17657\scratchpad')
def time(value):
    return datetime.strptime(value, '%H:%M:%S.%f')
step = time(re.search(r'^([\d:.]+).*kind=PLAYER_ACT .*value=9 text=op=STEP,', text, re.M)[1])
restore = time(re.search(r'^([\d:.]+).*kind=PLAYER_ACT .*value=15 text=op=RESTORE,', text, re.M)[1])
images = []
for phase, marker, start, end_ms, digest in capture:
    end = start.rsplit('.', 1)[0] + '.' + end_ms
    assert time(marker) <= time(start) <= time(end)
    if phase == 'PRE':
        assert time(end) < step
    elif phase == 'POST':
        assert step < time(start) <= time(end) < restore
    else:
        assert time(start) > restore
    png = scratch / f'a1s2-{run}-{phase}.png'
    assert png.is_file(), 'Для повторной проверки нужен исходный внешний PNG: ' + str(png)
    data = png.read_bytes()
    assert hashlib.sha256(data).hexdigest() == digest and data[:8] == b'\x89PNG\r\n\x1a\n'
    assert struct.unpack('>II', data[16:24]) == (1936, 1056)
    images.append(dict(phase=phase, path=str(png), sha256=digest, captureStart=start, captureEnd=end))
report = dict(status='INDEPENDENT_SCREEN_STEP_AND_CORE_READER_CONFIRMED', logPath=str(path), logSha256=sha,
              streamEvents=120, snapshotEvents=120, samples=30, entryRows=60, widgetRows=10, acts=acts, readers=readers,
              widgets=widgets, screenshotFiles=images, captureTimingSource=receipt_ref,
              visualReview='Codex отдельно просмотрел все три PNG через view_image: CH2 48.0/48.2/48.0 MHz, CH1 32.0 MHz во всех. Скрипт проверяет файлы и время, но не распознаёт пиксели.',
              unitScope='RUNTIME_OBSERVED: native raw в кГц для проверенной AN/PRC-68 в установленной 1.8.0.13; общий builder остаётся с явным RF-профилем.',
              limits='Не доказаны звук, RX/TX, сетевые права и репликация, физический ввод или точечное A→B. DeviceId=1 и позиция владельца — TEST_ONLY входы reader; ключ и координаты не журналируются.',
              replayRequirements='Нужны исходный лог, три внешних PNG и Git-коммит a433072. Отсутствие любого доказательства прерывает повтор.',
              checkerSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
(folder / 'live-player-screenstep-review-codex.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
print('SCREENSTEP_CONFIRMED events=120 entries=60 widgets=10 reader=48000/48200/48000 captures=3 restored=1')
