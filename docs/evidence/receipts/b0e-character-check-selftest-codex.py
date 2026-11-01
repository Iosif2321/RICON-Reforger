"""Синтетические проверки B0e без игры, Workbench и личных журналов."""
from datetime import datetime, timedelta
import json
from pathlib import Path
import runpy
import subprocess
import sys
import tempfile


CHECKER = Path(__file__).with_name("b0e-character-check-codex.py")
checker = runpy.run_path(str(CHECKER))
check_log, check_host_completion, read_radios, review_pair = (
    checker[key] for key in ("check_log", "check_host_completion", "read_radios", "review_pair")
)
CRC = "abcdef01"
PREFAB = "{0123456789ABCDEF}Prefabs/Characters/Test.et"
RADIO = "{FEDCBA9876543210}Prefabs/Items/Radios/Test.et"


def fixture(mode, count=7, start=0, spawn=1, radio_at=1, gone_at=None):
    local = 1 if mode == 2 else 2
    events = []

    def add(kind, player, value, payload, sample=0, tr=-1):
        wall = (datetime(1900, 1, 1, 18) + timedelta(seconds=start + sample)).strftime("%H:%M:%S.%f")[:-3]
        events.append(f"{wall} SCRIPT : [RICON_LIVE] seq={len(events)} t={sample * 1000} side=probe "
                      f"kind={kind} player={player} device=-1 tr={tr} value={value} text={payload}")

    role = f"isServer={int(mode == 2)},isClient={int(mode == 1)},rplMode={mode}"
    add("ENDPOINT_START", -1, 0, f"cycle=1,limit={count},{role}")
    for i in range(1, count + 1):
        assigned = int(i >= spawn)
        own = 1 if mode == 2 else assigned
        sample = f"cycle=1,{role},players=2,members=1|2,pc=1,controlled={own},vonController=1"
        if own:
            sample += f",controlledPrefab={PREFAB},entityVoN=1,gadgetManager=1"
        sample += (",gameMode=1,gameModeRespawn=1,spawnPoints=1,respawnEnabled=1,spawnLogic=1,"
                   "spawnLogicClass=SCR_AutoSpawnLogic,factionManager=1,loadoutManager=1")
        add("ENDPOINT_SAMPLE", local, i, sample, i)
        for player in (1, 2):
            pc = int(player == 2 or mode == 2)
            controlled = assigned if player == 2 else int(mode == 2)
            observable = int(mode == 2 or player == local)
            add("ENDPOINT_PLAYER", player, i,
                f"cycle=1,manager=1,connected=1,pc={pc},controlled={controlled},pcControlled={controlled},"
                f"sameEntity={controlled},respawnObservable={observable},respawn={'1' if observable else 'UNKNOWN'}", i)
        if mode == 1 and i == radio_at:
            add("RADIO_STATE", local, 32000,
                f"binding=LOCAL_ONLY,obs=0,slot=0,prefab={RADIO},min=32000,max=68000,res=200,powered=0,muted=1", i, 0)
        if mode == 1 and i == gone_at:
            add("RADIO_GONE", local, -1, "binding=LOCAL_ONLY,obs=0", i, 0)
    add("ENDPOINT_DONE", local, count, f"cycle=1,samples={count},reason=LIMIT", count)
    lifecycle = []
    if mode == 2:
        base = datetime(1900, 1, 1, 18) + timedelta(seconds=start + count)
        lifecycle = [
            f"{base.strftime('%H:%M:%S.%f')[:-3]} SCRIPT : [RICON_LIVEB] inspect=OBSERVATION_END samples=120 complete=1 isGameMode=1",
            f"{(base + timedelta(seconds=1)).strftime('%H:%M:%S.%f')[:-3]} SCRIPT : [RICON_LIVEB] inspect=EDIT_REQUESTED",
            f"{(base + timedelta(seconds=7)).strftime('%H:%M:%S.%f')[:-3]} SCRIPT : [RICON_LIVEB] inspect=EDIT_STABLE",
        ]
    return "\n".join([f"RICON_LiveEndpointProbe.c' CRC: {CRC}", *events, *lifecycle,
                      f"[RICON_LIVE] dump=BEGIN count={len(events)} dropped=0 nextSeq={len(events)}",
                      *events, "[RICON_LIVE] dump=END"])


def parse(folder, text, mode, count=7):
    path = folder / f"synthetic-{mode}.log"
    path.write_text(text, encoding="utf-8")
    checked, rows, _ = check_log(path, mode, count, CRC)
    read_radios(checked, rows)
    return rows


with tempfile.TemporaryDirectory(prefix="ricon-b0e-check-") as directory:
    folder = Path(directory)
    host_text, client_text = fixture(2), fixture(1)
    check_host_completion(host_text)
    lifecycle_failures = [
        host_text.replace("inspect=OBSERVATION_END", "inspect=OBSERVATION_MISSING"),
        host_text.replace("inspect=EDIT_REQUESTED", "inspect=EDIT_MISSING"),
        host_text.replace("18:00:14.000 SCRIPT : [RICON_LIVEB] inspect=EDIT_STABLE",
                          "18:00:09.000 SCRIPT : [RICON_LIVEB] inspect=EDIT_STABLE"),
    ]
    for broken in lifecycle_failures:
        try:
            check_host_completion(broken)
        except AssertionError:
            continue
        raise AssertionError("Не отклонено незавершённое окно хоста")
    host = parse(folder, host_text, 2)
    client = parse(folder, client_text, 1)
    report = review_pair(host, client)
    assert report["result"] == "PLAYER2_SPAWN_RADIO_OBSERVED"
    assert report["audio"] == report["pairSuppression"] == "NOT_TESTED"
    assert report["host"]["commonSamples"] == report["client"]["commonSamples"] == 7
    assert client[0]["states"]["1"]["pc"] == "0"
    assert all(row["radios"] == client[0]["radios"] for row in client)
    assert client[0]["radios"][0] == dict(obs=0, player="2", tr=0, raw=32000, prefab=RADIO, powered=0, muted=1)

    # Только пять общих срезов; единственная радиозапись была до начала общего интервала.
    shifted_host = parse(folder, fixture(2, start=2), 2)
    overlap = review_pair(shifted_host, client)
    assert overlap["host"]["commonSamples"] == overlap["client"]["commonSamples"] == 5
    assert overlap["client"]["firstSample"] == 3

    late = parse(folder, fixture(1, radio_at=3), 1)
    assert late[1]["radios"] == [] and late[2]["radios"]
    assert review_pair(host, late)["client"]["commonSamples"] == 5
    before_spawn = parse(folder, fixture(1, spawn=3, radio_at=3), 1)
    assert "controlledPrefab" not in before_spawn[0]["sample"]
    assert review_pair(host, before_spawn)["client"]["commonSamples"] == 5
    gone = parse(folder, fixture(1, gone_at=7), 1)
    assert gone[-2]["radios"] and gone[-1]["radios"] == []

    failures = [
        (fixture(2, spawn=99), fixture(1, spawn=99, radio_at=None), "старый B0d: controlled=0"),
        (fixture(2, start=3), client_text, "четыре общих среза"),
        (host_text, fixture(1, radio_at=4), "четыре среза после выдачи рации"),
        (host_text, fixture(1, radio_at=None), "нет рации"),
        (host_text, fixture(1, gone_at=7), "последний RADIO_GONE сбрасывает серию"),
        (host_text, fixture(1, spawn=4, radio_at=4), "четыре среза после появления"),
        (host_text.replace("pcControlled=1", "pcControlled=0"), client_text, "несогласованный pcControlled"),
        (host_text.replace("sameEntity=1", "sameEntity=0"), client_text, "нет sameEntity"),
        (host_text.replace("connected=1", "connected=0"), client_text, "connectedMismatch"),
        (host_text, client_text.replace("kind=ENDPOINT_SAMPLE player=2", "kind=ENDPOINT_SAMPLE player=1"), "неверный local ID"),
        (host_text, client_text.replace("entityVoN=1", "entityVoN=0"), "нет entityVoN"),
        (host_text, client_text.replace("gadgetManager=1", "gadgetManager=0"), "нет gadgetManager"),
        (host_text, client_text.replace("vonController=1", "vonController=0"), "нет vonController"),
        (host_text, client_text.replace(f"controlledPrefab={PREFAB}", "controlledPrefab="), "пустой prefab"),
        (host_text, client_text.replace(f",controlledPrefab={PREFAB}", ""), "нет prefab"),
        (host_text, client_text.replace("kind=RADIO_STATE player=2", "kind=RADIO_STATE player=1"), "чужая рация"),
        (host_text, client_text.replace("binding=LOCAL_ONLY", "binding=REMOTE"), "не локальная привязка"),
        (host_text, client_text.replace("tr=0 value=32000", "tr=-1 value=32000"), "нет трансивера"),
        (host_text, client_text.replace("powered=0", "powered=2"), "неверный powered"),
        (host_text, client_text.replace("muted=1", "muted=2"), "неверный muted"),
        (host_text, fixture(1, radio_at=None, gone_at=1), "исчезновение неизвестного obs"),
        (host_text, client_text.replace("seq=2", "seq=9"), "повреждённая последовательность"),
        (host_text, client_text.replace("dropped=0", "dropped=1"), "потери журнала"),
        (host_text, client_text.replace("powered=0", "powered=1", 1), "расхождение stream/dump"),
    ]
    for h, c, label in failures:
        try:
            review_pair(parse(folder, h, 2), parse(folder, c, 1))
        except AssertionError:
            continue
        raise AssertionError(f"Не отклонено: {label}")

    # Проверяем настоящий CLI с рабочими лимитами 120/30 и только синтетическими файлами.
    host_path, client_path, out_path = (folder / name for name in ("host.log", "client.log", "review.json"))
    private_line = "Имя=PRIVATE_TEST_NAME путь=C:\\Private\\Profile адрес=192.0.2.77\n"
    host_path.write_text(private_line + fixture(2, count=120), encoding="utf-8")
    client_path.write_text(fixture(1, count=30, start=90), encoding="utf-8")
    command = [sys.executable, "-X", "utf8", str(CHECKER), "--host-log", str(host_path), "--client-log", str(client_path), "--probe-crc", CRC]
    stdout = subprocess.run(command, check=True, capture_output=True, text=True, encoding="utf-8").stdout
    result = json.loads(stdout)
    assert result["host"]["samples"] == 120 and result["client"]["samples"] == 30
    assert result["client"]["commonSamples"] == 30
    assert result["hostLifecycle"] == "OBSERVATION_END_EDIT_STABLE"
    assert all(len(result[key]) == 64 for key in ("checkerSha256", "helperSha256"))
    assert str(folder) not in stdout
    assert all(value not in stdout for value in ("PRIVATE_TEST_NAME", "C:\\Private", "192.0.2.77"))
    written = subprocess.run(command + ["--out", str(out_path)], check=True, capture_output=True, text=True)
    assert written.stdout == "" and json.loads(out_path.read_text(encoding="utf-8")) == result
    original = host_path.read_bytes()
    overwrite = subprocess.run(command + ["--out", str(host_path)], capture_output=True)
    assert overwrite.returncode != 0 and host_path.read_bytes() == original
    hardlink = folder / "linked-output.json"
    hardlink.hardlink_to(host_path)
    overwrite = subprocess.run(command + ["--out", str(hardlink)], capture_output=True)
    assert overwrite.returncode != 0 and host_path.read_bytes() == original

print(f"B0e checker: синтетические положительные случаи, {len(failures)} отказа готовности, "
      f"{len(lifecycle_failures)} отказа завершения, CLI 120/30 и защита входа проверены. Игра и звук не проверялись.")
