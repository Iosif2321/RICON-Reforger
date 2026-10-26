"""Проверка наблюдения назначений B0d; исходные пути и личные данные не выводятся."""
import argparse
from collections import Counter
from datetime import datetime
import hashlib
import json
from pathlib import Path
import re


def fields(text):
    pairs = [part.split("=", 1) for part in text.split(",")]
    result = dict(pairs)
    assert len(result) == len(pairs), "Повтор поля"
    return result


def check_log(path, mode, limit, crc):
    data = path.read_bytes()
    text = data.decode("utf-8-sig")
    assert not re.search(r"SCRIPT\s*\(E\)", text), "Ошибка скрипта"
    assert f"RICON_LiveEndpointProbe.c' CRC: {crc}" in text, "Иной наблюдатель"
    lines = [s for s in text.splitlines() if "[RICON_LIVE]" in s]
    tagged = [s[s.index("[RICON_LIVE]"):].rstrip() for s in lines]
    starts = [i for i, s in enumerate(tagged) if "dump=BEGIN" in s]
    assert len(starts) == 1, "Нужен один полный цикл"
    n = starts[0]
    assert tagged[n] == f"[RICON_LIVE] dump=BEGIN count={n} dropped=0 nextSeq={n}"
    assert tagged[n + 1:-1] == tagged[:n] and tagged[-1] == "[RICON_LIVE] dump=END"
    pattern = re.compile(r"\[RICON_LIVE\] seq=(\d+) t=(-?\d+) side=probe kind=(\w+) player=(-?\d+) device=-1 tr=(-?\d+) value=(-?\d+) text=(\S+)")
    events = [pattern.fullmatch(s) for s in tagged[:n]]
    assert all(events) and [int(e[1]) for e in events] == list(range(n))
    assert events[0][3] == "ENDPOINT_START" and events[-1][3] == "ENDPOINT_DONE"
    assert fields(events[0][7]) == dict(cycle="1", limit=str(limit), isServer=str(int(mode == 2)), isClient=str(int(mode == 1)), rplMode=str(mode))
    assert events[-1][7] == f"cycle=1,samples={limit},reason=LIMIT"
    rows = []
    for i, event in enumerate(events):
        if event[3] != "ENDPOINT_SAMPLE":
            continue
        f = fields(event[7])
        assert int(event[6]) == len(rows) + 1 and f["cycle"] == "1"
        assert f["rplMode"] == str(mode)
        assert f["isServer"] == str(int(mode == 2)) and f["isClient"] == str(int(mode == 1))
        assert f["gameMode"] in ("0", "1")
        assert f["gameModeRespawn"] in (("UNKNOWN",) if f["gameMode"] == "0" else ("0", "1"))
        assert int(f["spawnPoints"]) >= 0
        for key in ("factionManager", "loadoutManager"):
            assert f[key] in ("0", "1")
        for key in ("respawnEnabled", "spawnLogic"):
            assert f[key] in ("0", "1", "UNKNOWN")
        if f["spawnLogic"] in ("0", "UNKNOWN"):
            assert f["spawnLogicClass"] == ("NONE" if f["spawnLogic"] == "0" else "UNKNOWN")
        else:
            assert f["spawnLogicClass"] not in ("", "NONE", "UNKNOWN")
        members = set() if f["members"] == "NONE" else set(f["members"].split("|"))
        assert len(members) == int(f["players"])
        states = {}
        for offset, player in enumerate(("1", "2"), 1):
            item = events[i + offset]
            assert item[3] == "ENDPOINT_PLAYER" and item[4] == player and item[6] == event[6]
            state = fields(item[7])
            assert state["cycle"] == "1" and state["manager"] == "1"
            for key in ("connected", "pc", "controlled", "pcControlled", "sameEntity", "respawnObservable"):
                assert state[key] in ("0", "1"), key
            observable = mode == 2 or int(event[4]) > 0 and event[4] == player
            assert state["respawnObservable"] == str(int(observable))
            assert state["respawn"] in (("0", "1") if observable else ("UNKNOWN",))
            if state["pcControlled"] == "1":
                assert state["pc"] == "1"
            if state["sameEntity"] == "1":
                assert state["controlled"] == state["pcControlled"] == "1"
            if player == event[4]:
                assert state["pc"] == f["pc"] and state["pcControlled"] == f["controlled"]
            states[player] = state
        wall = datetime.strptime(lines[i].split()[0], "%H:%M:%S.%f")
        mismatches = [player for player, state in states.items() if state["connected"] != str(int(player in members))]
        rows.append(dict(index=int(event[6]), local=event[4], members=members, wall=wall, sample=f, states=states, connectedMismatch=mismatches))
    assert len(rows) == limit
    assert sum(e[3] == "ENDPOINT_PLAYER" for e in events) == 2 * limit
    return text, rows, dict(logSha256=hashlib.sha256(data).hexdigest(), events=n, samples=limit,
                           connectedMismatch=[dict(sample=r["index"], players=r["connectedMismatch"]) for r in rows if r["connectedMismatch"]])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host-log", type=Path, required=True)
    parser.add_argument("--client-log", type=Path, required=True)
    parser.add_argument("--probe-crc", required=True)
    parser.add_argument("--out", type=Path)
    args = parser.parse_args()
    assert re.fullmatch(r"[0-9a-f]{8}", args.probe_crc)
    host_text, host, host_report = check_log(args.host_log, 2, 120, args.probe_crc)
    _, client, client_report = check_log(args.client_log, 1, 30, args.probe_crc)
    # Нужна устойчивая непрерывная конечная серия, а не пять случайных совпадений.
    tails = []
    for rows, local in ((host, "1"), (client, "2")):
        tail = []
        for row in rows:
            if row["local"] == local and row["members"] == {"1", "2"}:
                tail.append(row)
            else:
                tail = []
        assert len(tail) >= 5
        tails.append(tail)
    start = max(tail[0]["wall"] for tail in tails)
    end = min(tail[-1]["wall"] for tail in tails)
    for tail, report in zip(tails, (host_report, client_report)):
        common = [r for r in tail if start <= r["wall"] <= end]
        assert len(common) >= 5
        assert not any(r["connectedMismatch"] for r in common), "Расхождение подключения в устойчивом общем интервале"
        report["commonSamples"] = len(common)
        for player in ("1", "2"):
            report[f"player{player}States"] = dict(Counter(json.dumps(r["states"][player], sort_keys=True) for r in common))
        for key in ("gameModeRespawn", "spawnPoints", "respawnEnabled", "spawnLogic", "spawnLogicClass", "factionManager", "loadoutManager"):
            report[key] = dict(Counter(r["sample"][key] for r in common))
    assert "inspect=OBSERVATION_END samples=120 complete=1 isGameMode=1" in host_text
    times = []
    for marker in ("EDIT_REQUESTED", "EDIT_STABLE"):
        matches = re.findall(r"^(\d\d:\d\d:\d\d\.\d+).*\[RICON_LIVEB\] inspect=" + marker + r"\b", host_text, re.M)
        assert len(matches) == 1
        times.append(datetime.strptime(matches[0], "%H:%M:%S.%f"))
    assert (times[1] - times[0]).total_seconds() >= 5
    report = dict(result="PLAYER_ASSIGNMENT_OBSERVED", host=host_report, client=client_report,
                  probeCRC=args.probe_crc, streamEqualsDump=True, dropped=0,
                  overlapSeconds=(end - start).total_seconds(),
                  checkerSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  limits="Наблюдаются доступные назначения. Наличие всех сущностей мира, звук, UI и очистка процессов этим checker не проверяются.")
    output = json.dumps(report, ensure_ascii=False, indent=2) + "\n"
    if args.out:
        args.out.write_text(output, encoding="utf-8")
    else:
        print(output, end="")


if __name__ == "__main__":
    main()
