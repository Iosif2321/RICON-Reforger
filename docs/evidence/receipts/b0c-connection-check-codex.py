"""Проверка двух фиксированных raw-журналов B0c; путь клиента не сохраняется в отчёте."""
import argparse
from datetime import datetime
import hashlib
import json
from pathlib import Path
import re

parser = argparse.ArgumentParser()
parser.add_argument("--client-log", required=True, type=Path)
args = parser.parse_args()
folder = Path(__file__).resolve().parent
run = "ricon-b0c-serverplay-claude-20261026-160416"
host_path = Path.home() / "Documents/My Games/ArmaReforgerWorkbench/logs" / run / "console.log"


def check_log(path, digest, limit, event_count, mode):
    data = path.read_bytes()
    assert hashlib.sha256(data).hexdigest() == digest
    text = data.decode("utf-8-sig")
    assert not re.search(r"SCRIPT\s*\(E\)", text)
    assert "RICON_LiveEndpointProbe.c' CRC: 0b2d7a64" in text
    assert "RICON_LiveEndpointGame.c' CRC: 45b337e3" in text
    tagged = [s[s.index("[RICON_LIVE]"):].rstrip() for s in text.splitlines() if "[RICON_LIVE]" in s]
    begin = tagged.index(f"[RICON_LIVE] dump=BEGIN count={event_count} dropped=0 nextSeq={event_count}")
    stream = tagged[:begin]
    assert len(stream) == event_count and tagged[begin + 1:-1] == stream
    assert tagged[-1] == "[RICON_LIVE] dump=END"
    pattern = re.compile(r"\[RICON_LIVE\] seq=(\d+) t=(-?\d+) side=probe kind=(\w+) player=(-?\d+) device=-1 tr=(-?\d+) value=(-?\d+) text=(\S+)")
    events = [pattern.fullmatch(s) for s in stream]
    assert all(events) and [int(e[1]) for e in events] == list(range(event_count))
    assert events[0][3] == "ENDPOINT_START" and events[-1][3] == "ENDPOINT_DONE"
    assert f"limit={limit},isServer={int(mode == 2)},isClient={int(mode == 1)},rplMode={mode}" in events[0][7]
    assert events[-1][7] == f"cycle=1,samples={limit},reason=LIMIT"
    assert "kind=ENDPOINT_ABORTED" not in text
    rows = []
    live_lines = [s for s in text.splitlines() if "[RICON_LIVE]" in s][:event_count]
    for event, line in zip(events, live_lines):
        if event[3] != "ENDPOINT_SAMPLE":
            continue
        pairs = [p.split("=", 1) for p in event[7].split(",")]
        fields = dict(pairs)
        assert len(fields) == len(pairs)
        assert int(event[6]) == len(rows) + 1
        assert fields["rplMode"] == str(mode)
        assert fields["isServer"] == str(int(mode == 2)) and fields["isClient"] == str(int(mode == 1))
        members = {int(v) for v in fields["members"].split("|")}
        assert len(members) == int(fields["players"])
        ms = int(fields["elapsedMs"])
        assert ms >= 1000 if not rows else ms - rows[-1]["elapsedMs"] >= 999
        wall = datetime.strptime(line.split()[0], "%H:%M:%S.%f")
        rows.append(dict(index=int(event[6]), playerId=int(event[4]), members=members, fields=fields, wall=wall, elapsedMs=ms))
    assert len(rows) == limit
    assert "same identity" not in text
    return text, events, rows


host_sha = "7d8515d788a7f12ba5ce87575377e9fa723d5581366a7f0aa1d8ce860c9e05fc"
client_sha = "69616027b7324d8fe2e6a8072b04ea01b8029d200ee6ab42fb7860f642783a59"
host_text, host_events, host = check_log(host_path, host_sha, 120, 125, 2)
client_text, client_events, client = check_log(args.client_log, client_sha, 30, 34, 1)
assert all(r["playerId"] == 1 and r["fields"]["pc"] == r["fields"]["controlled"] == "1" for r in host)
assert all(r["members"] == {1} for r in host[:8])
assert all(r["members"] == {1, 2} for r in host[8:])
assert all(r["playerId"] == -1 and r["fields"]["pc"] == "0" and r["members"] == {1} for r in client[:2])
assert all(r["playerId"] == 2 and r["members"] == {1, 2} and r["fields"]["pc"] == "1" for r in client[2:])
assert all(r["fields"]["controlled"] == "0" and r["fields"]["vonEntries"] == "0" for r in client[2:])
assert sum(e[3] == "RADIO_STATE" for e in host_events) == 2
assert sum(e[3] == "RADIO_STATE" for e in client_events) == 0
assert sum(e[3] == "ENDPOINT_BINDING" for e in client_events) == 2

# Сопоставляется время строк одного ПК; игровые часы t/elapsedMs разных миров не приравниваются.
overlap_start = max(host[8]["wall"], client[2]["wall"])
overlap_end = min(host[-1]["wall"], client[-1]["wall"])
host_overlap = sum(overlap_start <= r["wall"] <= overlap_end for r in host[8:])
client_overlap = sum(overlap_start <= r["wall"] <= overlap_end for r in client[2:])
assert host_overlap >= 5 and client_overlap >= 5
assert "Creating player: PlayerId=2, Name=RICON_B0c_peer1" in host_text
assert "inspect=OBSERVATION_END samples=120 complete=1 isGameMode=1" in host_text
times = {}
for marker in ("EDIT_REQUESTED", "EDIT_STABLE"):
    hits = re.findall(r"^(\d\d:\d\d:\d\d\.\d+).*\[RICON_LIVEB\] inspect=" + marker + r"\b", host_text, re.M)
    assert len(hits) == 1
    times[marker] = datetime.strptime(hits[0], "%H:%M:%S.%f")
edit_seconds = (times["EDIT_STABLE"] - times["EDIT_REQUESTED"]).total_seconds()
assert edit_seconds >= 5

report = dict(run=run, result="CONNECTION_CONFIRMED",
              host=dict(logSha256=host_sha, events=125, samples=120, rplMode=2, playerId=1, twoMemberTail=112),
              client=dict(logSha256=client_sha, events=34, samples=30, rplMode=1, playerId=2, twoMemberTail=28,
                          controlledEntity=False, vonEntries=0, radioStateEvents=0),
              streamEqualsDump=True, dropped=0, scriptErrors=0,
              overlap=dict(seconds=(overlap_end - overlap_start).total_seconds(), hostSamples=host_overlap, clientSamples=client_overlap),
              editStableAfterSeconds=edit_seconds,
              limits="Подтверждено соединение двух участников. У клиента controlled=0 и vonEntries=0; голос и A→B не проверены. UI-восстановление и завершение процессов сверяются по квитанции оператора отдельно.",
              replayRequirements="Нужны исходный журнал хоста и переданный через --client-log исходный журнал клиента.",
              checkerSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
(folder / "b0c-connection-review-codex.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
print(f"B0c подтверждён: хост 120, клиент 30 срезов; в общем интервале {host_overlap}/{client_overlap} срезов с двумя участниками.")
