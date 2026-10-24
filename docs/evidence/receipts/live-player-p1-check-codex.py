"""Независимая проверка привязки реальных radio-entry в фиксированном P1."""
import hashlib
import json
from pathlib import Path
import re
from urllib.parse import unquote_to_bytes

folder = Path(__file__).resolve().parent
path = Path(r'C:\Users\Ded\Documents\My Games\ArmaReforgerWorkbench\logs\ricon-a1p1-claude-20261024-201240\console.log')
raw = path.read_bytes()
sha = hashlib.sha256(raw).hexdigest()
assert sha == 'f5b1428bd7ccac75591d5c27732965c692c01d4ae1a5ae4809f624772085f28f'
text = raw.decode('utf-8', errors='replace')
assert not re.search(r'SCRIPT\s*\(E\)', text)
tagged = [s[s.index('[RICON_LIVE]'):].rstrip() for s in text.splitlines() if '[RICON_LIVE]' in s]
assert len(tagged) == 186
assert tagged[92] == '[RICON_LIVE] dump=BEGIN count=92 dropped=0 nextSeq=92'
assert tagged[:92] == tagged[93:185] and tagged[-1] == '[RICON_LIVE] dump=END'
pattern = re.compile(r'\[RICON_LIVE\] seq=(\d+) t=(-?\d+) side=probe kind=(\w+) player=-1 device=-1 tr=-1 value=(-?\d+) text=(\S+)$')
events = [pattern.fullmatch(s) for s in tagged[:92]]
assert all(events) and [int(m[1]) for m in events] == list(range(92))
assert [m[3] for m in events] == ['BENCH_START']+['PLAYER_SAMPLE','PLAYER_ENTRY','PLAYER_ENTRY']*30+['BENCH_DONE']
prefab = '{73950FBA2D7DB5C5}Prefabs/Items/Equipment/Radios/Radio_ANPRC68.et'
state = 'isServer=1,isClient=0,players=1,pc=1,playerId=1,vonController=1,vonEntries=2,controlled=1,controlledPrefab={26A9756790131354}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Rifleman.et,entityVoN=1,gadgetManager=1'
times = []
observations = []
for n in range(1, 31):
    sample, *entries = events[1+(n-1)*3:1+n*3]
    assert int(sample[4]) == n
    elapsed, actual = sample[5].split(',', 1)
    assert actual == state
    ms = int(elapsed.split('=', 1)[1])
    assert abs(ms-int(sample[2])) <= 1
    assert ms >= 1000 if not times else ms-times[-1] >= 999
    times.append(ms)
    for i, event in enumerate(entries):
        assert int(event[4]) == n and abs(int(event[2])-int(sample[2])) <= 1
        f = dict(part.split('=', 1) for part in event[5].split(','))
        frequency = (32000,48000)[i]
        expected = dict(idx=str(i),entry='1',cls='SCR_VONEntryRadio',method='1',active=str(1-i),
                        transmit=str(1-i),usable='1',dtext=f'{frequency/1000:.1f}%20#AR-VON_FrequencyUnits_MHz',
                        radioEntry='1',trNumber=str(i+1),entryFreq=str(frequency),emuted='0',gadget='1',
                        longRange='0',transceiver='1',raw=str(frequency),min='32000',max='68000',res='200',
                        muted='0',radioPrefab=prefab,trIndex=str(i),inventorySlot='0',gadgetOwnerMatch='1')
        assert f == expected, (n, i)
        decoded = unquote_to_bytes(f['dtext']).decode('utf-8')
        assert round(float(decoded.split(' ', 1)[0])*1000) == frequency
        if n == 1:
            observations.append({'idx':i,'type':f['cls'],'method':1,'transceiverIndex':i,'entryNumber':i+1,
                                 'raw':frequency,'entryFreq':frequency,'text':decoded,'radioPrefab':prefab,
                                 'inventorySlot':0,'gadgetOwnerMatch':True,'active':i==0})
assert events[-1][4] == '30' and events[-1][5] == 'level=PLAYER_OBSERVATION,not=SPAWN|SET|CONTROL|RADIO|AUDIO'
report = {
    'status':'INDEPENDENT_PLAYER_RADIO_ENTRY_MAPPING_CONFIRMED',
    'scope':'Чтение реальных entry игрока; без изменения частоты, mute, передачи и экранного меню',
    'logPath':str(path),'logSha256':sha,'streamEvents':92,'snapshotEvents':92,'samples':30,'entriesChecked':60,
    'firstSampleMs':times[0],'lastSampleMs':times[-1],
    'minIntervalMs':min(b-a for a,b in zip(times,times[1:])),
    'maxIntervalMs':max(b-a for a,b in zip(times,times[1:])),
    'sameStateInAllSamples':True,'observations':observations,
    'directSpeechInEnumeratedList':False,
    'limits':'Отсутствие DIRECT в этом перечислении не означает отсутствия прямой речи в игре; виджет и звук не проверены',
    'checkerSha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
}
(folder/'live-player-p1-review-codex.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('PLAYER_P1_CONFIRMED samples=30 entries=60 stream=92 snapshot=92 mapped_radios=1 transceivers=2')
