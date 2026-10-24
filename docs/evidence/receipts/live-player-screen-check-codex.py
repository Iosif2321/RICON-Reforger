"""Независимая проверка фиксированных S1b/S1c; пиксели просмотрены отдельно, не распознаны этим скриптом."""
import hashlib
import json
from pathlib import Path
import re
import struct
from urllib.parse import unquote_to_bytes

folder = Path(__file__).resolve().parent
logs = Path(r'C:\Users\Ded\Documents\My Games\ArmaReforgerWorkbench\logs')
runs = [
    ('ricon-a1s1b-claude-20261024-214154', '5bc5cc0cf75732afa40456832f4c4f7747cb41e9da10eb6190399b0fa504fe9b', 5, 18),
    ('ricon-a1s1c-claude-20261024-214617', '67403433d320c7ce3fe20451cbc079a2a1126a2bb4ac841b7720acaf074fff9c', 30, 114),
]
pattern = re.compile(r'\[RICON_LIVE\] seq=(\d+) t=(-?\d+) side=probe kind=(\w+) player=-1 device=-1 tr=-1 value=(-?\d+) text=(\S+)$')
reports = []
for name, expected_sha, count, size in runs:
    path = logs / name / 'console.log'
    raw = path.read_bytes()
    assert hashlib.sha256(raw).hexdigest() == expected_sha
    text = raw.decode('utf-8-sig')
    assert not re.search(r'SCRIPT\s*\(E\)', text)
    config_error = "Unknown keyword/data 'm_VONMenu'"
    assert text.count(config_error) == int(count == 5)
    tagged = [s[s.index('[RICON_LIVE]'):].rstrip() for s in text.splitlines() if '[RICON_LIVE]' in s]
    assert len(tagged) == 2 * size + 2
    assert tagged[size] == f'[RICON_LIVE] dump=BEGIN count={size} dropped=0 nextSeq={size}'
    assert tagged[:size] == tagged[size + 1:-1] and tagged[-1] == '[RICON_LIVE] dump=END'
    events = [pattern.fullmatch(s) for s in tagged[:size]]
    assert all(events) and [int(e[1]) for e in events] == list(range(size))
    expected_kinds = ['BENCH_START']
    for n in range(1, count + 1):
        expected_kinds += ['PLAYER_SAMPLE', 'PLAYER_ENTRY', 'PLAYER_ENTRY']
        expected_kinds += ['SCREEN'] * (1 if count == 5 and n == 5 else
                                      3 if count == 30 and 7 <= n <= 12 else
                                      1 if count == 30 and n in (5, 6, 20, 30) else 0)
    expected_kinds += ['BENCH_ABORTED' if count == 5 else 'BENCH_DONE']
    assert [e[3] for e in events] == expected_kinds
    samples, entries, screens = [], {}, []
    for e in events:
        pairs = [p.split('=', 1) for p in e[5].split(',')]
        fields = dict(pairs)
        assert len(fields) == len(pairs)
        n = int(e[4])
        if e[3] == 'PLAYER_SAMPLE':
            assert n == len(samples) + 1
            assert all(fields[k] == '1' for k in ('pc', 'playerId', 'players', 'vonController', 'controlled', 'entityVoN', 'gadgetManager', 'isServer'))
            assert fields['isClient'] == '0' and fields['vonEntries'] == '2'
            ms = int(fields['elapsedMs'])
            assert abs(ms - int(e[2])) <= 1 and (ms >= 1000 if not samples else ms - samples[-1] >= 999)
            samples.append(ms)
        elif e[3] == 'PLAYER_ENTRY':
            i = int(fields['idx'])
            assert i in (0, 1)
            frequency = (32000, 48000)[i]
            assert fields['raw'] == fields['entryFreq'] == str(frequency)
            assert fields['muted'] == fields['emuted'] == '0'
            assert fields['active'] == fields['transmit'] == str(1 - i)
            assert fields['trNumber'] == str(i + 1) and fields['trIndex'] == str(i)
            assert fields['gadgetOwnerMatch'] == '1' and fields['inventorySlot'] == fields['editorRadio'] == '0'
            assert fields['cls'] == 'SCR_VONEntryRadio' and fields['min'] == '32000' and fields['max'] == '68000' and fields['res'] == '200'
            assert fields['dtext'] == f'{frequency / 1000:.1f}%20#AR-VON_FrequencyUnits_MHz'
            entries.setdefault(n, []).append(fields)
        elif e[3] == 'SCREEN':
            screens.append((n, fields))
    assert len(samples) == count and list(entries) == list(range(1, count + 1))
    assert [e['idx'] for e in entries[1]] == ['0', '1']
    assert all(v == entries[1] for v in entries.values())
    if count == 5:
        assert screens == [(5, dict(op='NOT_READY', von='1', menu='0', same='0'))]
        assert events[-1][5] == 'level=PLAYER_SCREEN_WIDGET,notReady=1,samples=5'
        reports.append(dict(run=name, logSha256=expected_sha, status='CONFIRMED_NOT_READY', events=size,
                            samples=count, unchangedEntryRows=count * 2, configError=config_error, openInvoked=False))
        continue
    before = dict(op='BEFORE', opened='0', hasDisplay='0', inputs='0', selectedId='0', activeIdx='0')
    menu = dict(op='MENU', opened='1', hasDisplay='1', inputs='1', selectedId='0', activeIdx='0')
    opening = dict(menu, op='OPEN', hadControl='0', controls='1', openAction='VONMenu', closeOnRelease='0', performOnClose='0', closeOnPerform='1')
    assert screens[:2] == [(5, before), (6, opening)]
    assert [(n, f) for n, f in screens if f['op'] == 'MENU'] == [(n, menu) for n in [7, 8, 9, 10, 11, 12, 20, 30]]
    widgets = [(n, f) for n, f in screens if f['op'] == 'WIDGET']
    assert [(n, f['idx']) for n, f in widgets] == [(n, str(i)) for n in range(7, 13) for i in (0, 1)]
    for n, f in widgets:
        i = int(f['idx'])
        assert all(f[k] == '1' for k in ('component', 'root', 'rootVis', 'f', 'fVis', 'c', 'cVis'))
        assert not re.search(r'%(?![0-9A-F]{2})', f['fText'])
        assert unquote_to_bytes(f['fText']).decode('utf-8') == ('32.0 MHz', '48.0 MHz')[i]
        assert (int(f['fX']), int(f['fY']), int(f['fW']), int(f['fH'])) == (713, (269, 625)[i], 71, 23)
        assert f['cText'] == 'Error'
    assert events[-1][5] == 'level=PLAYER_SCREEN_WIDGET,not=PIXELS|LOCALIZATION|INPUT|AUDIO|REPLICATION'
    reports.append(dict(run=name, logSha256=expected_sha, status='CONTROLLER_AND_FREQUENCY_WIDGETS_CONFIRMED',
                        events=size, samples=count, unchangedEntryRows=count * 2, widgetRows=len(widgets),
                        firstSampleMs=samples[0], lastSampleMs=samples[-1], controllerConfig=opening,
                        rawToWidget=[{'raw': 32000, 'text': '32.0 MHz'}, {'raw': 48000, 'text': '48.0 MHz'}],
                        channelWidgetText='Error', channelTextIsNotPixelEvidence=True))

png = Path(r'C:\Users\Ded\AppData\Local\Temp\claude\C--Users-Ded-Documents-Projects-ARMA-3-Scripts-RICON-Reforger\583c292f-5bc1-43e3-9045-3fc153e17657\scratchpad\a1s1-window-214640.png')
image = png.read_bytes()
assert hashlib.sha256(image).hexdigest() == '4930f5697a71a687f5506354aba182f508b23688c0a595bc339f6ac64c410e67'
assert image[:8] == b'\x89PNG\r\n\x1a\n' and struct.unpack('>II', image[16:24]) == (1936, 1056)
report = dict(runs=reports, screenshotPath=str(png), screenshotSha256=hashlib.sha256(image).hexdigest(),
              manualVisualReview='Codex открыл этот PNG через view_image: видны CH1 32.0 MHz и CH2 48.0 MHz; Error на пикселях не виден. Скрипт сверяет файл и размер, а не распознаёт изображение.',
              limits='Два базовых значения подтверждены на экране. Шаг частоты на экране, полный InputManager, звук, RX/TX, репликация и избирательное A→B этим опытом не проверены.',
              checkerSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
(folder / 'live-player-screen-review-codex.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
print('SCREEN_REVIEW_CONFIRMED S1b=NOT_READY S1c=114events/30samples/60entries/12widgets PNG_SHA_MATCH')
