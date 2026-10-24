"""Независимая проверка 30 наблюдений локального игрока, без парсера Claude."""
import hashlib
import json
from pathlib import Path
import re

folder = Path(__file__).resolve().parent
log = Path(r'C:\Users\Ded\Documents\My Games\ArmaReforgerWorkbench\logs\ricon-a1p0-claude-20261024-195517\console.log')
raw = log.read_bytes()
sha = hashlib.sha256(raw).hexdigest()
assert sha == '9f3ad5d0e1092c13f37e24ecb1fb09d588437337badcb69c5245a923b92f4530'
text = raw.decode('utf-8', errors='replace')
assert not re.search(r'SCRIPT\s*\(E\)', text)
tagged = [s[s.index('[RICON_LIVE]'):].rstrip() for s in text.splitlines() if '[RICON_LIVE]' in s]
assert len(tagged) == 66
assert tagged[32] == '[RICON_LIVE] dump=BEGIN count=32 dropped=0 nextSeq=32'
assert tagged[:32] == tagged[33:65] and tagged[-1] == '[RICON_LIVE] dump=END'
line = re.compile(r'\[RICON_LIVE\] seq=(\d+) t=(-?\d+) side=probe kind=(\w+) player=-1 device=-1 tr=-1 value=(-?\d+) text=(\S+)$')
events = [line.fullmatch(s) for s in tagged[:32]]
assert all(events) and [int(m[1]) for m in events] == list(range(32))
assert [m[3] for m in events] == ['BENCH_START']+['PLAYER_SAMPLE']*30+['BENCH_DONE']
start = dict(p.split('=', 1) for p in events[0][5].split(','))
assert start['mode'] == 'player' and start['modeGiven'] == '1' and start['playerController'] == '0'
assert events[-1][4] == '30' and events[-1][5] == 'level=PLAYER_OBSERVATION,not=SPAWN|SET|CONTROL|RADIO|AUDIO'
state = 'isServer=1,isClient=0,players=1,pc=1,playerId=1,vonController=1,vonEntries=2,controlled=1,controlledPrefab={26A9756790131354}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Rifleman.et,entityVoN=1,gadgetManager=1'
times = []
for n, event in enumerate(events[1:-1], 1):
    assert int(event[4]) == n
    elapsed, actual_state = event[5].split(',', 1)
    assert actual_state == state
    assert elapsed.startswith('elapsedMs=')
    ms = int(elapsed.split('=', 1)[1])
    assert abs(ms-int(event[2])) <= 1
    assert ms >= 1000 if not times else ms-times[-1] >= 999
    times.append(ms)
assert times[0] == 1013 and times[-1] == 30193
report = {
    'status': 'INDEPENDENT_PLAYER_OBSERVATION_COMPLETE',
    'scope': 'Наблюдение локального контроллера и персонажа; без управления, спавна и радио-актуаторов прибора',
    'logPath': str(log), 'logSha256': sha, 'streamEvents': 32, 'snapshotEvents': 32,
    'samples': 30, 'firstSampleMs': times[0], 'lastSampleMs': times[-1],
    'minIntervalMs': min(b-a for a,b in zip(times,times[1:])),
    'maxIntervalMs': max(b-a for a,b in zip(times,times[1:])),
    'controllerAtStart': False, 'controllerInEverySample': True, 'playerId': 1,
    'controlledPrefab': '{26A9756790131354}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Rifleman.et',
    'players': 1, 'vonController': True, 'vonEntries': 2, 'entityVoN': True, 'gadgetManager': True,
    'isServer': True, 'isClient': False, 'radioActionsInProbeStream': 0,
    'limits': 'Это не доказательство слышимости, сетевой репликации или экранного меню; события между срезами не покрыты',
    'checkerSha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
}
(folder/'live-player-p0-review-codex.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('PLAYER_P0_CONFIRMED samples=30 stream=32 snapshot=32 local_controller=1 controlled=1 radio_actions=0')
