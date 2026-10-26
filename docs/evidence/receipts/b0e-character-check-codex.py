"""B0e: назначение персонажа 2 и доступность его радио/VoN; звук не проверяется."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import runpy


HELPER = Path(__file__).with_name("b0d-player-check-codex.py")
shared = runpy.run_path(str(HELPER))
check_log, fields = shared["check_log"], shared["fields"]
ASSIGNMENT = ("connected", "pc", "controlled", "pcControlled", "sameEntity")
COMPONENTS = ("entityVoN", "gadgetManager", "vonController")


def read_radios(text, rows):
    """Дополняет уже проверенные срезы состоянием после окончания каждого опроса."""
    active, retired = {}, set()
    current = None
    for line in text.splitlines():
        if "[RICON_LIVE]" not in line:
            continue
        tagged = line.split("[RICON_LIVE] ", 1)[1]
        if tagged.startswith("dump=BEGIN"):
            break
        header, payload = tagged.split(" text=", 1)
        event = dict(part.split("=", 1) for part in header.split())
        kind = event["kind"]
        if kind in ("ENDPOINT_SAMPLE", "ENDPOINT_DONE"):
            if current is not None:
                # RADIO_STATE/GONE идут после SAMPLE; снимок завершается только перед следующим опросом.
                current["radios"] = list(active.values())
            current = rows[int(event["value"]) - 1] if kind == "ENDPOINT_SAMPLE" else None
        elif kind in ("RADIO_STATE", "RADIO_GONE"):
            assert current is not None, "Радиособытие вне опроса"
            state = fields(payload)
            assert state.get("binding") == "LOCAL_ONLY", "Нет локальной привязки рации"
            obs, tr = int(state["obs"]), int(event["tr"])
            assert obs >= 0 and tr >= 0, "Неверное наблюдение или индекс трансивера"
            if kind == "RADIO_GONE":
                assert obs in active, "Исчезновение неизвестной рации"
                assert active[obs]["player"] == event["player"] and active[obs]["tr"] == tr
                del active[obs]
                retired.add(obs)
            else:
                assert event["player"] == current["local"], "Рация не принадлежит локальному игроку"
                assert current["sample"].get("controlled") == "1", "Рация без управляемой сущности"
                assert obs not in retired, "Повторное использование завершённого obs"
                assert obs not in active or active[obs]["tr"] == tr, "Изменился трансивер того же obs"
                assert state.get("powered") in ("0", "1") and state.get("muted") in ("0", "1")
                assert state.get("prefab") not in (None, ""), "Нет prefab рации"
                active[obs] = dict(obs=obs, player=event["player"], tr=tr, raw=int(event["value"]),
                                   prefab=state["prefab"], powered=int(state["powered"]), muted=int(state["muted"]))
    assert all("radios" in row for row in rows), "Не завершены радиоснимки"


def review_pair(host, client):
    """Ищет общую часть конечных непрерывных серий готовности двух процессов."""
    tails = []
    for rows, local in ((host, "1"), (client, "2")):
        assert all(a["wall"] < b["wall"] for a, b in zip(rows, rows[1:])), "Время срезов не возрастает"
        tail = []
        for row in rows:
            sample = row["sample"]
            ready = (row["local"] == local and row["members"] == {"1", "2"}
                     and not row["connectedMismatch"]
                     and all(row["states"]["2"].get(key) == "1" for key in ASSIGNMENT))
            if local == "2":
                ready = (ready and all(sample.get(key) == "1" for key in COMPONENTS)
                         and sample.get("controlledPrefab") not in (None, "", "NONE", "UNKNOWN")
                         and any(radio["player"] == "2" for radio in row["radios"]))
            tail = tail + [row] if ready else []
        assert len(tail) >= 5, "Нет пяти конечных последовательных срезов готовности"
        tails.append(tail)
    start = max(tail[0]["wall"] for tail in tails)
    end = min(tail[-1]["wall"] for tail in tails)
    report = dict(result="PLAYER2_SPAWN_RADIO_OBSERVED", audio="NOT_TESTED", pairSuppression="NOT_TESTED",
                  intervalStart=start.strftime("%H:%M:%S.%f")[:-3], intervalEnd=end.strftime("%H:%M:%S.%f")[:-3],
                  overlapSeconds=(end - start).total_seconds(), connectedMismatch=0,
                  limits="Наблюдаются назначение и локальные радио/VoN. Звук, PTT и соответствие VoN-entry трансиверу не проверены. obs локальны процессу.")
    for side, tail in zip(("host", "client"), tails):
        common = [row for row in tail if start <= row["wall"] <= end]
        assert len(common) >= 5, "В общем интервале меньше пяти срезов одной из сторон"
        report[side] = dict(commonSamples=len(common), firstSample=common[0]["index"], lastSample=common[-1]["index"],
                            player2={key: 1 for key in ASSIGNMENT},
                            localSamples=[dict(sample=row["index"],
                                               components={key: row["sample"].get(key) for key in COMPONENTS},
                                               controlledPrefab=row["sample"].get("controlledPrefab"),
                                               radios=row["radios"]) for row in common])
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host-log", type=Path, required=True)
    parser.add_argument("--client-log", type=Path, required=True)
    parser.add_argument("--probe-crc", required=True)
    parser.add_argument("--out", type=Path)
    args = parser.parse_args()
    assert re.fullmatch(r"[0-9a-f]{8}", args.probe_crc), "Неверный CRC"
    host_text, host, host_report = check_log(args.host_log, 2, 120, args.probe_crc)
    client_text, client, client_report = check_log(args.client_log, 1, 30, args.probe_crc)
    assert host_report["logSha256"] != client_report["logSha256"], "Нужны независимые журналы"
    read_radios(host_text, host)
    read_radios(client_text, client)
    report = review_pair(host, client)
    report["host"].update(host_report)
    report["client"].update(client_report)
    report.update(probeCRC=args.probe_crc, streamEqualsDump=True, dropped=0,
                  checkerSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  helperSha256=hashlib.sha256(HELPER.read_bytes()).hexdigest())
    output = json.dumps(report, ensure_ascii=False, indent=2) + "\n"
    if args.out:
        if args.out.exists():
            assert not any(args.out.samefile(path) for path in (args.host_log, args.client_log, Path(__file__), HELPER)), "Нельзя перезаписать вход или проверяющий скрипт"
        args.out.write_text(output, encoding="utf-8")
    else:
        print(output, end="")


if __name__ == "__main__":
    main()
