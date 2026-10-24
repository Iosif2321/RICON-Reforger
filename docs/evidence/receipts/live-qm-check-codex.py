"""Независимая проверка фиксированного A0-M: локальные флаги четырёх собственных раций, без вывода о звуке."""
import hashlib
import json
from pathlib import Path
import re

folder = Path(__file__).resolve().parent
path = Path(r'C:\Users\Ded\Documents\My Games\ArmaReforgerWorkbench\logs\ricon-a0m-claude-20261024-224002\console.log')
raw = path.read_bytes()
sha = hashlib.sha256(raw).hexdigest()
assert sha == '126cdde02f1723baaefccc017aa38f64fcc7210b3d04b38c9f7e74600b840a63'
text = raw.decode('utf-8-sig')
assert not re.search(r'SCRIPT\s*\(E\)', text)
assert "RICON_LiveRadioBenchComponent.c' CRC: 37beadf1" in text
tagged = [s[s.index('[RICON_LIVE]'):].rstrip() for s in text.splitlines() if '[RICON_LIVE]' in s]
assert len(tagged) == 96 and tagged[47] == '[RICON_LIVE] dump=BEGIN count=47 dropped=0 nextSeq=47'
assert tagged[:47] == tagged[48:95] and tagged[-1] == '[RICON_LIVE] dump=END'
pat = re.compile(r'\[RICON_LIVE\] seq=(\d+) t=(-?\d+) side=probe kind=(\w+) player=-1 device=-1 tr=-1 value=(-?\d+) text=(\S+)$')
events = [pat.fullmatch(s) for s in tagged[:47]]
assert all(events) and [int(e[1]) for e in events] == list(range(47))
assert [e[3] for e in events] == ['BENCH_START', 'BENCH_MANAGER'] + ['BENCH_SPAWN'] * 4 + ['QM'] * 40 + ['BENCH_DONE']
assert 'isServer=1,isClient=0' in events[0][5] and 'mode=radiomute,modeGiven=1' in events[0][5]
assert events[1][5] == 'where=preSpawn,radioManager=0,expected=-1'
names = ('ANPRC68', 'ANPRC77', 'R107M', 'RF10')
frequencies = (32000, 48000, 48000, 42000)
control = (48000, 42000, 42000, 39000)
ops = ('BASE', 'MUTE', 'RETUNE', 'READ', 'POWER', 'READ', 'POWER_BACK', 'READ', 'RESTORE', 'FINAL')
def state(device, step):
    return dict(radio='1', trs='2', powered=str(int(step not in (4, 5))),
                f0=str(frequencies[device] + (200 if 2 <= step <= 7 else 0)), m0=str(int(1 <= step <= 7)),
                f1=str(control[device]), m1='0')
reports = []
for device, name in enumerate(names):
    spawn = dict(p.split('=', 1) for p in events[device + 2][5].split(','))
    assert events[device + 2][4] == '1' and spawn['bench'] == str(device)
    assert spawn['prefab'] == spawn['actualPrefab'] and ('Radio_' + name) in spawn['prefab']
    times, rows = [], []
    for step, op in enumerate(ops):
        e = events[6 + step * 4 + device]
        pairs = [p.split('=', 1) for p in e[5].split(',')]
        f = dict(pairs)
        assert len(f) == len(pairs) and int(e[4]) == step
        ms = int(f['elapsedMs'])
        assert abs(ms - int(e[2])) <= 1 and (ms >= 1000 if not times else ms - times[-1] >= 999)
        expected = dict(bench=str(device), op=op, elapsedMs=str(ms), **state(device, step))
        if step == 0:
            expected.update({k: f[k] for k in ('min', 'max', 'res', 'target')})
            assert int(f['res']) == 200 and int(f['target']) == frequencies[device] + 200
            assert int(f['min']) <= frequencies[device] < int(f['target']) <= int(f['max'])
        if step in (1, 2, 4, 6, 8):
            expected.update({'pre' + k: v for k, v in state(device, step - 1).items()})
        if step == 1:
            expected.update(want='1', ret='0')
        elif step == 2:
            expected.update(want=str(frequencies[device] + 200))
        elif step in (4, 6):
            expected.update(want='0' if step == 4 else '1')
        elif step == 8:
            expected.update(setPower='0', restoreFreqBefore=str(frequencies[device] + 200), setFreq='1',
                            restoreMuteBefore='1', setMute='1', ret='0')
        elif step in (3, 5, 7):
            expected.update(phase={3: 'afterRetune', 5: 'powerToggled', 7: 'powerBack'}[step])
        assert f == expected, (device, step, f, expected)
        times.append(ms)
        rows.append({'step': step, 'op': op, 'gameMs': ms, **state(device, step)})
    assert state(device, 0) == state(device, 9)
    reports.append(dict(model=name, prefab=spawn['prefab'], baselineFrequency=frequencies[device],
                        controlFrequency=control[device], mutedAfterSet=True, mutePersistsAfterRetune=True,
                        mutePersistsWhilePowerOff=True, mutePersistsAfterPowerBack=True,
                        controlUnchanged=True, observedStateDrift=False, restored=True, setterReturns=[0, 0], snapshots=rows))
assert events[-1][4] == '9' and events[-1][5] == 'level=QM_LOCAL,not=AUDIBLE|RX_TX|REPLICATION|CLIENT|PER_PAIR|PLAYER'
report = dict(status='INDEPENDENT_QM_LOCAL_OBSERVED', logPath=str(path), logSha256=sha,
              streamEvents=47, snapshotEvents=47, qmRecords=40, nativeBenchCRC='37beadf1', radioManagerPresent=False,
              radios=reports, limits='Наблюдены только флаги компонентов в одном authority-процессе без RadioManager. Слышимость, RX/TX, сетевые права/репликация и точечное A→B не проверены. Bool SetMuteState не считается кодом успеха.',
              replayRequirements='Для повтора нужен исходный внешний console.log по записанному пути.',
              checkerSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
(folder / 'live-qm-review-codex.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
print('QM_CONFIRMED radios=4 records=40 stream=47 snapshot=47 restored=4 control_unchanged=4')
