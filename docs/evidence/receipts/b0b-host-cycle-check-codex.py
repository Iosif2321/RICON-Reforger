"""Независимая проверка фиксированного журнала listen-хоста; клиент и звук не проверяются."""
from datetime import datetime
import hashlib
import json
from pathlib import Path
import re
from urllib.parse import unquote

folder = Path(__file__).resolve().parent
run = "ricon-b0b-serverplay-claude-20261025-175912"
path = Path.home() / "Documents/My Games/ArmaReforgerWorkbench/logs" / run / "console.log"
data = path.read_bytes()
digest = "ea359c6abc1e53513a87187b8929a1905f1018246d138325181edd902ba026c2"
assert hashlib.sha256(data).hexdigest() == digest
text = data.decode("utf-8-sig")
assert not re.search(r"SCRIPT\s*\(E\)", text)
for name, crc in {
    "RICON_LiveEndpointGame": "45b337e3",
    "RICON_LiveEndpointProbe": "e655d126",
    "RICON_LiveBenchLauncherPlugin": "124aa0bd",
    "RICON_LivePlayModeObserverPlugin": "350473c2",
}.items():
    assert f"{name}.c' CRC: {crc}" in text

tagged = [s[s.index("[RICON_LIVE]"):].rstrip() for s in text.splitlines() if "[RICON_LIVE]" in s]
begin = tagged.index("[RICON_LIVE] dump=BEGIN count=35 dropped=0 nextSeq=35")
stream = tagged[:begin]
assert len(stream) == 35
assert tagged[begin + 1:-1] == stream
assert tagged[-1] == "[RICON_LIVE] dump=END"
pattern = re.compile(r"\[RICON_LIVE\] seq=(\d+) t=(\d+) side=probe kind=(\w+) player=(-?\d+) device=-1 tr=(-?\d+) value=(-?\d+) text=(\S+)")
events = [pattern.fullmatch(s) for s in stream]
assert all(events)
assert [int(e[1]) for e in events] == list(range(35))
assert [int(e[2]) for e in events] == sorted(int(e[2]) for e in events)
assert [e[3] for e in events] == ["ENDPOINT_START", "ENDPOINT_BINDING", "ENDPOINT_SAMPLE", "RADIO_STATE", "RADIO_STATE"] + ["ENDPOINT_SAMPLE"] * 29 + ["ENDPOINT_DONE"]
assert events[0][7] == "cycle=1,limit=30,isServer=1,isClient=0,rplMode=2"
assert events[1][7] == "cycle=1,revision=1,initial=1,pcChanged=1,entityChanged=1,idChanged=1,pc=1,controlled=1,previousPlayerId=-1"
assert events[-1][6] == "30" and events[-1][7] == "cycle=1,samples=30,reason=LIMIT"


def fields(value):
    pairs = [part.split("=", 1) for part in value.split(",")]
    result = dict(pairs)
    assert len(result) == len(pairs)
    return result


elapsed = []
stable = []
radios = []
for event in events[1:]:
    assert event[4] == "1"
    if event[3] == "ENDPOINT_SAMPLE":
        f = fields(event[7])
        assert event[5] == "-1" and int(event[6]) == len(elapsed) + 1
        for key in ("cycle", "bindingRevision", "isServer", "players", "pc", "playerId", "vonController", "controlled", "entityVoN", "gadgetManager", "members"):
            assert f[key] == "1", (key, f)
        assert f["isClient"] == "0" and f["rplMode"] == f["vonEntries"] == "2"
        ms = int(f.pop("elapsedMs"))
        assert ms >= 1000 if not elapsed else ms - elapsed[-1] >= 999
        elapsed.append(ms)
        stable.append(f)
    elif event[3] == "RADIO_STATE":
        f = fields(event[7])
        index = len(radios)
        assert event[5] == str(index) and int(event[6]) == (32000, 48000)[index]
        assert f == dict(binding="LOCAL_ONLY", obs=str(index), slot="0",
                         prefab="{73950FBA2D7DB5C5}Prefabs/Items/Equipment/Radios/Radio_ANPRC68.et",
                         min="32000", max="68000", res="200", powered="1", muted="0")
        radios.append(int(event[6]))
assert len(elapsed) == 30 and len(radios) == 2 and all(f == stable[0] for f in stable)

marks = ["WAIT_PLUGIN", "PLUGIN_SIGNAL_ACCEPTED", "PLUGIN_SEEN", "PLUGIN_PLAY_REQUESTED", "WAIT_PLAY", "PLAY_ENTERED", "OBSERVATION_END", "EDIT_REQUESTED", "EDIT_STABLE"]
times = {}
positions = []
for mark in marks:
    hits = list(re.finditer(r"^(\d\d:\d\d:\d\d\.\d+).*\[RICON_LIVEB\] inspect=" + mark + r"\b.*$", text, re.M))
    assert len(hits) == 1, mark
    positions.append(hits[0].start())
    times[mark] = datetime.strptime(hits[0][1], "%H:%M:%S.%f")
assert positions == sorted(positions)
edit_seconds = (times["EDIT_STABLE"] - times["EDIT_REQUESTED"]).total_seconds()
assert edit_seconds >= 5
assert "inspect=OBSERVATION_END samples=30 complete=1 isGameMode=1" in text
assert "inspect=EDIT_STABLE checks=51" in text and "inspect=REFUSED" not in text
net = re.findall(r"\[RICON_NET\] cycle=1 rplMode=2 rplName=Listen listenPresent=1", text)
assert len(net) == 1
mode = re.findall(r"\[RICON_WBMODE\] event=START mode=(\S+) world=(\S+) fromCamera=1", text)
assert len(mode) == 1 and unquote(mode[0][0]) == "Server localhost"
assert "Autotest_GameMode_Plain.ent" in mode[0][1]
assert text.count("[RICON_WBMODE] event=END") == 1
assert "Starting RPL server, listening on address 0.0.0.0:2001" in text

report = dict(run=run, logSha256=digest, result="HOST_CYCLE_CONFIRMED",
              streamEvents=35, samples=30, streamEqualsDump=True, dropped=0,
              rplMode=2, rplName="Listen", workbenchMode="Server localhost",
              playerId=1, members=[1], bindingRevision=1, localRadioValues=radios,
              firstElapsedMs=elapsed[0], lastElapsedMs=elapsed[-1], editStableAfterSeconds=edit_seconds,
              engineReportedBind=dict(addressClass="ANY", port=2001),
              limits="Проверен журнал одного хоста. UDP ОС, UI-откат и завершение процессов проверяются по квитанции оператора отдельно. Второго клиента, принятого соединения и слышимости нет.",
              replayRequirements="Нужен исходный внешний console.log указанного запуска.",
              checkerSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
(folder / "b0b-host-cycle-review-codex.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
print("B0b подтверждён: 35 событий, 30 срезов, Listen, поток равен dump, устойчивый возврат в edit.")
