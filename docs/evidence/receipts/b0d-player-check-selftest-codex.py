"""Отказы checker B0d на повреждённых данных; без игры и внешних журналов."""
from pathlib import Path
import runpy
import tempfile

check_log = runpy.run_path(str(Path(__file__).with_name("b0d-player-check-codex.py")))["check_log"]


def fixture(mode=1):
    local = 2 if mode == 1 else 1
    source = [
        ("ENDPOINT_START", -1, 0, f"cycle=1,limit=1,isServer={int(mode == 2)},isClient={int(mode == 1)},rplMode={mode}"),
        ("ENDPOINT_SAMPLE", local, 1, f"cycle=1,isServer={int(mode == 2)},isClient={int(mode == 1)},rplMode={mode},players=2,members=1|2,pc=1,controlled=0,gameMode=1,gameModeRespawn=0,spawnPoints=0,respawnEnabled=UNKNOWN,spawnLogic=UNKNOWN,spawnLogicClass=UNKNOWN,factionManager=1,loadoutManager=1"),
        ("ENDPOINT_PLAYER", 1, 1, "cycle=1,manager=1,connected=1,pc=1,controlled=0,pcControlled=0,sameEntity=0,respawnObservable=" + ("0,respawn=UNKNOWN" if mode == 1 else "1,respawn=0")),
        ("ENDPOINT_PLAYER", 2, 1, "cycle=1,manager=1,connected=1,pc=1,controlled=0,pcControlled=0,sameEntity=0,respawnObservable=1,respawn=0"),
        ("ENDPOINT_DONE", local, 1, "cycle=1,samples=1,reason=LIMIT"),
    ]
    events = [f"18:00:01.000 SCRIPT : [RICON_LIVE] seq={i} t=1000 side=probe kind={kind} player={player} device=-1 tr=-1 value={value} text={text}"
              for i, (kind, player, value, text) in enumerate(source)]
    return "\n".join(["RICON_LiveEndpointProbe.c' CRC: abcdef01", *events,
                      "[RICON_LIVE] dump=BEGIN count=5 dropped=0 nextSeq=5", *events, "[RICON_LIVE] dump=END"])


with tempfile.TemporaryDirectory(prefix="ricon-b0d-check-") as folder:
    path = Path(folder) / "console.log"
    for mode in (1, 2):
        path.write_text(fixture(mode), encoding="utf-8")
        _, rows, report = check_log(path, mode, 1, "abcdef01")
        assert len(rows) == 1 and report["events"] == 5
    path.write_text(fixture().replace("connected=1", "connected=0"), encoding="utf-8")
    _, _, report = check_log(path, 1, 1, "abcdef01")
    assert report["connectedMismatch"] == [dict(sample=1, players=["1", "2"])]
    corruptions = [
        ("seq=2", "seq=9"),
        ("dropped=0", "dropped=1"),
        ("kind=ENDPOINT_PLAYER player=2", "kind=ENDPOINT_PLAYER player=1"),
        ("sameEntity=0", "sameEntity=1"),
        ("respawnObservable=0,respawn=UNKNOWN", "respawnObservable=0,respawn=0"),
        ("respawnObservable=0,respawn=UNKNOWN", "respawnObservable=1,respawn=0"),
        ("spawnPoints=0", "spawnPoints=-1"),
        ("pcControlled=0", "pcControlled=1"),
        ("manager=1", "manager=0"),
        ("cycle=1,manager=1", "cycle=1,cycle=1,manager=1"),
        ("CRC: abcdef01", "CRC: abcdef02"),
    ]
    for old, new in corruptions:
        path.write_text(fixture().replace(old, new), encoding="utf-8")
        try:
            check_log(path, 1, 1, "abcdef01")
        except AssertionError:
            continue
        raise AssertionError(f"Не отклонено повреждение: {old}")
    # Отдельно портим только stream: dump остаётся исходным.
    path.write_text(fixture().replace("respawn=UNKNOWN", "respawn=0", 1), encoding="utf-8")
    try:
        check_log(path, 1, 1, "abcdef01")
    except AssertionError:
        pass
    else:
        raise AssertionError("Не обнаружено расхождение stream/dump")
print("B0d checker: 2 корректных журнала, диагностика переходного расхождения и 12 повреждений проверены.")
