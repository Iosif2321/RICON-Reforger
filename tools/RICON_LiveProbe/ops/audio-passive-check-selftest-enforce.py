"""Synthetic audio-passive checker tests; no game, Core Audio, VoN or personal logs."""
from pathlib import Path
import json
import runpy
import subprocess
import sys
import tempfile


CHECKER = Path(__file__).with_name("audio-passive-check-enforce.py")
checker = runpy.run_path(str(CHECKER))
parse_log, review = checker["parse_log"], checker["review"]


def fixture(player, server=False, kinds=("NOTE", "NOTE"), dropped=0, count=None, next_seq=None):
    lines = []

    def add(payload):
        lines.append(f"18:00:{len(lines):02d}.000 SCRIPT : {payload}")

    if server:
        add("[RICON_AUDIO] state=SERVER_BINDING subscriptions=2")
    add(f"[RICON_AUDIO] state=CLIENT_BINDING player={player} attached=1 subscriptions=2")
    add("[RICON_AUDIO] state=RUNNING passive=1 binding=UNKNOWN")
    for seq, kind in enumerate(kinds):
        add(f"[RICON_AUDIO] seq={seq} t={seq * 10} side=client kind={kind} player={player} "
            f"device=-1 tr=0 value={seq} text=synthetic")
    if next_seq is None:
        next_seq = len(kinds)
    if count is None:
        count = next_seq if dropped == 0 else next_seq + 1 - dropped
    add(f"[RICON_AUDIO] snapshot count={count} dropped={dropped} nextSeq={next_seq}")
    add("[RICON_AUDIO] state=STOPPED reason=GAME_END subscriptionsRemaining=0")
    return "\n".join(lines) + "\n"


def replace_once(text, old, new):
    assert text.count(old) == 1
    return text.replace(old, new)


def duplicate_line(text, marker):
    line = next(line for line in text.splitlines() if marker in line)
    return text + line + "\n"


def move_after(text, moving_marker, anchor_marker):
    lines = text.splitlines()
    moving = next(line for line in lines if moving_marker in line)
    anchor = next(line for line in lines if anchor_marker in line)
    lines.remove(moving)
    lines.insert(lines.index(anchor) + 1, moving)
    return "\n".join(lines) + "\n"


def move_before(text, moving_marker, anchor_marker):
    lines = text.splitlines()
    moving = next(line for line in lines if moving_marker in line)
    anchor = next(line for line in lines if anchor_marker in line)
    lines.remove(moving)
    lines.insert(lines.index(anchor), moving)
    return "\n".join(lines) + "\n"


def expect_rejected(folder, host_text, client_text, label):
    host_path, client_path = folder / "bad-host.log", folder / "bad-client.log"
    host_path.write_text(host_text, encoding="utf-8")
    client_path.write_text(client_text, encoding="utf-8")
    try:
        review(host_path, client_path)
    except AssertionError:
        return
    raise AssertionError(f"Not rejected: {label}")


with tempfile.TemporaryDirectory(prefix="ricon-audio-passive-") as directory:
    folder = Path(directory)
    host_text = fixture(1, True)
    client_text = fixture(2)
    host_path, client_path = folder / "host.log", folder / "client.log"
    host_path.write_text(host_text, encoding="utf-8")
    client_path.write_text(client_text, encoding="utf-8")
    report = review(host_path, client_path)
    assert report["result"] == "PASSIVE_OBSERVER_READY"
    assert report["transmissionVerdict"] == report["audio"] == "NOT_TESTED"
    assert report["host"]["events"] == report["client"]["events"] == 2
    assert report["host"]["rxEvents"] == report["client"]["txCaptureEvents"] == 0

    # No callback events is still observer readiness, never a transmission verdict.
    empty_host, empty_client = fixture(1, True, ()), fixture(2, kinds=())
    (folder / "empty-host.log").write_text(empty_host, encoding="utf-8")
    (folder / "empty-client.log").write_text(empty_client, encoding="utf-8")
    empty = review(folder / "empty-host.log", folder / "empty-client.log")
    assert empty["host"]["events"] == empty["client"]["events"] == 0
    assert empty["transmissionVerdict"] == "NOT_TESTED"

    # RX/TX counts are reported but do not change the boundary.
    signal_host = fixture(1, True, ("TX_CAPTURE",))
    signal_client = fixture(2, kinds=("RX_EVENT",))
    (folder / "signal-host.log").write_text(signal_host, encoding="utf-8")
    (folder / "signal-client.log").write_text(signal_client, encoding="utf-8")
    signal = review(folder / "signal-host.log", folder / "signal-client.log")
    assert signal["host"]["txCaptureEvents"] == signal["client"]["rxEvents"] == 1
    assert signal["transmissionVerdict"] == signal["audio"] == "NOT_TESTED"

    # Full streamed sequence remains continuous even when the ring snapshot evicted entries.
    overflow_kinds = tuple("NOTE" for _ in range(257))
    overflow = fixture(1, True, overflow_kinds, dropped=2, count=256, next_seq=257)
    overflow_path = folder / "overflow.log"
    overflow_path.write_text(overflow, encoding="utf-8")
    assert parse_log(overflow_path, 1, True)["snapshotDropped"] == 2

    failures = [
        (replace_once(host_text, "state=RUNNING", "state=NOT_RUNNING"), client_text, "missing RUNNING"),
        (duplicate_line(host_text, "state=RUNNING"), client_text, "duplicate RUNNING"),
        (replace_once(host_text, "snapshot count=2", "snapshot_missing count=2"), client_text, "missing snapshot"),
        (duplicate_line(host_text, "snapshot count="), client_text, "duplicate snapshot"),
        (replace_once(host_text, "state=STOPPED", "state=NOT_STOPPED"), client_text, "missing STOPPED"),
        (duplicate_line(host_text, "state=STOPPED"), client_text, "duplicate STOPPED"),
        (replace_once(host_text, "subscriptionsRemaining=0", "subscriptionsRemaining=1"), client_text, "cleanup"),
        (replace_once(host_text, "SERVER_BINDING subscriptions=2", "SERVER_BINDING subscriptions=1"), client_text, "server subscriptions"),
        (duplicate_line(host_text, "SERVER_BINDING"), client_text, "duplicate server binding"),
        (move_after(host_text, "SERVER_BINDING", "state=RUNNING"), client_text, "server binding after RUNNING"),
        (host_text, replace_once(client_text, "player=2 attached=1", "player=1 attached=1"), "wrong local player"),
        (host_text, replace_once(client_text, "attached=1", "attached=0"), "detached client"),
        (host_text, replace_once(client_text, "subscriptions=2", "subscriptions=1"), "client subscriptions"),
        (host_text, replace_once(client_text, "seq=1", "seq=2"), "event seq gap"),
        (host_text, replace_once(client_text, "nextSeq=2", "nextSeq=3"), "nextSeq mismatch"),
        (host_text, replace_once(client_text, "count=2", "count=1"), "count mismatch"),
        (host_text, move_before(client_text, "seq=0 ", "state=RUNNING"), "event before RUNNING"),
        (host_text, move_after(client_text, "seq=1 ", "snapshot count="), "event after snapshot"),
        (host_text, fixture(2, kinds=tuple("NOTE" for _ in range(256)), dropped=2, count=255, next_seq=256), "overflow count not capacity"),
        (host_text, fixture(2, kinds=tuple("NOTE" for _ in range(257)), dropped=0, count=257, next_seq=257), "pre-overflow exceeds capacity"),
        (host_text + "18:01:00.000 SCRIPT : [RICON_AUDIO] seq=2 t=20 side=client kind=NOTE player=1 device=-1 tr=0 value=2 text=late\n",
         client_text, "event after stop"),
        ("SCRIPT (E): synthetic\n" + host_text, client_text, "script error"),
        (host_text + "18:01:00.000 SCRIPT : [RICON_AUDIO] unknown=1\n", client_text, "unknown audio line"),
    ]
    for bad_host, bad_client, label in failures:
        expect_rejected(folder, bad_host, bad_client, label)

    # Exercise the real CLI and output protection using only synthetic files.
    out_path = folder / "review.json"
    command = [sys.executable, "-X", "utf8", str(CHECKER), "--host-log", str(host_path),
               "--client-log", str(client_path)]
    stdout = subprocess.run(command, check=True, capture_output=True, text=True, encoding="utf-8").stdout
    cli = json.loads(stdout)
    assert cli["result"] == "PASSIVE_OBSERVER_READY" and len(cli["checkerSha256"]) == 64
    assert str(folder) not in stdout
    written = subprocess.run(command + ["--out", str(out_path)], check=True, capture_output=True, text=True)
    assert written.stdout == "" and json.loads(out_path.read_text(encoding="utf-8")) == cli
    original = host_path.read_bytes()
    overwrite = subprocess.run(command + ["--out", str(host_path)], capture_output=True)
    assert overwrite.returncode != 0 and host_path.read_bytes() == original
    hardlink = folder / "host-hardlink.json"
    hardlink.hardlink_to(host_path)
    overwrite = subprocess.run(command + ["--out", str(hardlink)], capture_output=True)
    assert overwrite.returncode != 0 and host_path.read_bytes() == original

print(f"audio-passive checker selftest: PASS ({len(failures)} synthetic rejections, CLI and input protection; game/audio not tested)")
