"""Независимая проверка действий P2 и восстановления всех реальных entry игрока."""
import hashlib
import json
from pathlib import Path
import re
from urllib.parse import unquote_to_bytes

folder = Path(__file__).resolve().parent
path = Path(r'C:\Users\Ded\Documents\My Games\ArmaReforgerWorkbench\logs\ricon-a1p2-claude-20261024-203425\console.log')
raw = path.read_bytes()
sha = hashlib.sha256(raw).hexdigest()
assert sha == '99f291867e001543353b8aa7ce3e5a336f7084105f10501fefd8f3fecbce4450'
text = raw.decode('utf-8', errors='replace')
assert not re.search(r'SCRIPT\s*\(E\)', text)
tagged = [s[s.index('[RICON_LIVE]'):].rstrip() for s in text.splitlines() if '[RICON_LIVE]' in s]
assert len(tagged) == 202
assert tagged[100] == '[RICON_LIVE] dump=BEGIN count=100 dropped=0 nextSeq=100'
assert tagged[:100] == tagged[101:201] and tagged[-1] == '[RICON_LIVE] dump=END'
pat = re.compile(r'\[RICON_LIVE\] seq=(\d+) t=(-?\d+) side=probe kind=(\w+) player=-1 device=-1 tr=-1 value=(-?\d+) text=(\S+)$')
events = [pat.fullmatch(s) for s in tagged[:100]]
assert all(events) and [int(e[1]) for e in events] == list(range(100))
plan = {5:'CHOOSE',6:'STEP',7:'READ',11:'READ',12:'RESTORE',14:'MUTE',15:'UNMUTE',20:'FINAL'}
kinds = ['BENCH_START']
for n in range(1,31):
    kinds += ['PLAYER_SAMPLE','PLAYER_ENTRY','PLAYER_ENTRY']
    if n in plan: kinds.append('PLAYER_ACT')
kinds += ['BENCH_DONE']
assert [e[3] for e in events] == kinds
prefab = '{73950FBA2D7DB5C5}Prefabs/Items/Equipment/Radios/Radio_ANPRC68.et'
state = 'isServer=1,isClient=0,players=1,pc=1,playerId=1,vonController=1,vonEntries=2,controlled=1,controlledPrefab={26A9756790131354}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Rifleman.et,entityVoN=1,gadgetManager=1'
samples, entries, acts = [], {}, []
for e in events:
    f = dict(p.split('=',1) for p in e[5].split(','))
    n = int(e[4])
    if e[3] == 'PLAYER_SAMPLE':
        elapsed, actual = e[5].split(',',1)
        assert actual == state and n == len(samples)+1
        ms = int(elapsed.split('=',1)[1])
        assert abs(ms-int(e[2])) <= 1 and (ms >= 1000 if not samples else ms-samples[-1] >= 999)
        samples.append(ms)
    elif e[3] == 'PLAYER_ENTRY':
        i = int(f['idx'])
        frequency = 32000 if i == 0 else (48200 if 7 <= n <= 12 else 48000)
        muted = int(i == 1 and n == 15)
        expected = dict(idx=str(i),entry='1',cls='SCR_VONEntryRadio',method='1',active=str(1-i),
                        transmit=str(1-i),usable='1',dtext=f'{frequency/1000:.1f}%20#AR-VON_FrequencyUnits_MHz',
                        radioEntry='1',trNumber=str(i+1),entryFreq=str(frequency),emuted=str(muted),gadget='1',
                        longRange='0',transceiver='1',raw=str(frequency),min='32000',max='68000',res='200',
                        muted=str(muted),radioPrefab=prefab,editorRadio='0',trIndex=str(i),inventorySlot='0',gadgetOwnerMatch='1')
        assert i in (0,1) and f == expected
        entries.setdefault(n,[]).append(f)
    elif e[3] == 'PLAYER_ACT':
        op = plan[n]
        frequency = 48200 if op in ('STEP','READ') else 48000
        muted = int(op == 'MUTE')
        expected = dict(op=op,idx='1',bound='1',raw=str(frequency),entryFreq=str(frequency),
                        muted=str(muted),emuted=str(muted),dtext=f'{frequency/1000:.1f}%20#AR-VON_FrequencyUnits_MHz')
        if op == 'CHOOSE':
            expected.update(trNumber='2',min='32000',max='68000',res='200',raw0='48000',muted0='0',target='48200',modifier='1',slot='0')
        elif op in ('STEP','READ'):
            phase, delay = {6:('t0',0),7:('t1',1004),11:('t5',5021)}[n]
            expected.update(phase=phase,sinceStepMs=str(delay),target='48200')
        elif op == 'RESTORE': expected.update(raw0='48000')
        elif op in ('MUTE','UNMUTE'): expected.update(ret=str(muted),muted0='0')
        elif op == 'FINAL': expected.update(raw0='48000',muted0='0')
        assert f == expected
        assert round(float(unquote_to_bytes(f['dtext']).decode().split(' ',1)[0])*1000) == frequency
        acts.append({'sample':n,'op':op,'gameMs':int(e[2]),'raw':frequency,'muted':muted})
assert len(samples) == 30 and len(acts) == 8 and all([int(f['idx']) for f in v] == [0,1] for v in entries.values())
assert entries[4] == entries[21] == entries[30]
assert events[-1][5] == 'level=PLAYER_MENU_RUNTIME,not=SCREEN|SPAWN|CONTROL|AUDIO|REPLICATION'
report = {
    'status':'INDEPENDENT_PLAYER_MENU_RUNTIME_PASS','scope':'Управление через реальные radio-entry тестового персонажа; без экранного меню, голоса и репликации',
    'logPath':str(path),'logSha256':sha,'streamEvents':100,'snapshotEvents':100,'samples':30,'entriesChecked':60,
    'acts':acts,'selectedEntry':1,'transceiverIndex':1,'radioPrefab':prefab,
    'frequencySequence':[48000,48200,48000],'muteSequence':[0,1,0],'activeEntryUnchanged':True,
    'restoredEntriesEqual':True,'firstSampleMs':samples[0],'lastSampleMs':samples[-1],
    'timingBasis':'Времена относятся к наблюдаемому фиксированному прогону, не к прогнозу задержек',
    'limits':'Не доказывает слышимость, общее ownership bool-API, сетевую синхронизацию и избирательное A→B',
    'checkerSha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
}
(folder/'live-player-p2-review-codex.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('PLAYER_P2_CONFIRMED samples=30 entries=60 acts=8 stream=100 snapshot=100 restored=1 active_unchanged=1')
