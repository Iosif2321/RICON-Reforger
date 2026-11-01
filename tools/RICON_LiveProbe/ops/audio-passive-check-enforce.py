"""Validate passive two-role RICON_AUDIO journals; never claims PTT, delivery or sound."""
import argparse
import hashlib
import json
from pathlib import Path
import re


EVENT = re.compile(
    r"\[RICON_AUDIO\] seq=(\d+) t=(-?\d+) side=(\w+) kind=(\w+) "
    r"player=(-?\d+) device=(-?\d+) tr=(-?\d+) value=(-?\d+) text=(\S+)"
)
SNAPSHOT = re.compile(r"\[RICON_AUDIO\] snapshot count=(\d+) dropped=(\d+) nextSeq=(\d+)")
RUNNING = "[RICON_AUDIO] state=RUNNING passive=1 binding=UNKNOWN"
SERVER_BINDING = re.compile(r"\[RICON_AUDIO\] state=SERVER_BINDING subscriptions=(\d+)")
CLIENT_BINDING = re.compile(
    r"\[RICON_AUDIO\] state=CLIENT_BINDING player=(-?\d+) attached=([01]) subscriptions=(\d+)"
)
STOPPED = re.compile(r"\[RICON_AUDIO\] state=STOPPED reason=(\w+) subscriptionsRemaining=(\d+)")


def tagged(line):
    """Return the exact RICON_AUDIO payload from an engine log line."""
    at = line.find("[RICON_AUDIO]")
    return line[at:].rstrip() if at >= 0 else None


def parse_log(path, expected_player, require_server):
    data = path.read_bytes()
    text = data.decode("utf-8-sig")
    assert not re.search(r"SCRIPT\s*\(E\)", text), "SCRIPT (E) in log"
    rows = [(index, tagged(line)) for index, line in enumerate(text.splitlines()) if tagged(line) is not None]
    assert rows, "No RICON_AUDIO lines"

    events = []
    running = []
    snapshots = []
    server_bindings = []
    client_bindings = []
    stopped = []
    for line_index, payload in rows:
        match = EVENT.fullmatch(payload)
        if match:
            events.append(dict(line=line_index, seq=int(match[1]), t=int(match[2]), side=match[3],
                               kind=match[4], player=int(match[5]), device=int(match[6]),
                               tr=int(match[7]), value=int(match[8]), text=match[9]))
            continue
        if payload == RUNNING:
            running.append(line_index)
            continue
        recognized = False
        for pattern, destination in ((SNAPSHOT, snapshots), (SERVER_BINDING, server_bindings),
                                     (CLIENT_BINDING, client_bindings), (STOPPED, stopped)):
            match = pattern.fullmatch(payload)
            if match:
                destination.append((line_index, match))
                recognized = True
                break
        assert recognized, "Unknown RICON_AUDIO line"

    assert len(running) == 1, "Need exactly one RUNNING"
    assert len(snapshots) == 1, "Need exactly one snapshot"
    assert len(stopped) == 1, "Need exactly one STOPPED"
    snapshot_line, snapshot_match = snapshots[0]
    stopped_line, stopped_match = stopped[0]
    assert running[0] < snapshot_line < stopped_line, "Lifecycle order is not RUNNING -> snapshot -> STOPPED"
    assert int(stopped_match[2]) == 0, "Subscriptions remain at STOPPED"
    assert all(running[0] < event["line"] < snapshot_line for event in events), "Event outside RUNNING -> snapshot interval"
    assert not any(event["line"] > stopped_line for event in events), "Event after STOPPED"

    assert client_bindings, "No CLIENT_BINDING"
    binding_line, binding = client_bindings[-1]
    assert binding_line < snapshot_line, "Final CLIENT_BINDING is not before snapshot"
    assert int(binding[1]) == expected_player, "Wrong final local player"
    assert binding[2] == "1" and int(binding[3]) == 2, "Final client binding is not attached with two subscriptions"

    if require_server:
        assert len(server_bindings) == 1, "Host needs exactly one SERVER_BINDING"
        assert int(server_bindings[0][1][1]) == 2, "Host server binding needs two subscriptions"
        assert server_bindings[0][0] < running[0], "SERVER_BINDING must precede RUNNING"
    else:
        assert not server_bindings, "Client log must not claim SERVER_BINDING"

    count, dropped, next_seq = map(int, snapshot_match.groups())
    assert count >= 0 and dropped >= 0 and next_seq >= 0
    assert [event["seq"] for event in events] == list(range(next_seq)), "Audio event seq is not complete and continuous"
    if dropped == 0:
        assert count == next_seq and count <= 256, "Pre-overflow snapshot counters are inconsistent"
    else:
        assert (count == 256 and dropped >= 2 and next_seq > 256
                and count + dropped == next_seq + 1), "Overflowed ring counters are inconsistent"

    kinds = {}
    for event in events:
        kinds[event["kind"]] = kinds.get(event["kind"], 0) + 1
    return dict(
        logSha256=hashlib.sha256(data).hexdigest(),
        audioLines=len(rows), events=len(events), eventSeqFirst=(events[0]["seq"] if events else None),
        eventSeqLast=(events[-1]["seq"] if events else None), snapshotCount=count,
        snapshotDropped=dropped, nextSeq=next_seq, running=1,
        finalBinding=dict(player=expected_player, attached=1, subscriptions=2),
        serverBinding=(dict(subscriptions=2) if require_server else None),
        stopped=dict(reason=stopped_match[1], subscriptionsRemaining=0), kinds=kinds,
        rxEvents=kinds.get("RX_EVENT", 0), txCaptureEvents=kinds.get("TX_CAPTURE", 0),
    )


def review(host_path, client_path):
    host = parse_log(host_path, 1, True)
    client = parse_log(client_path, 2, False)
    assert host["logSha256"] != client["logSha256"], "Need two distinct full logs"
    return dict(
        result="PASSIVE_OBSERVER_READY",
        transmissionVerdict="NOT_TESTED",
        audio="NOT_TESTED",
        limits=("Lifecycle, local bindings, subscriptions, event-stream continuity and cleanup are observed. "
                "RX/TX counts are descriptive only and do not prove PTT, delivery, device/transceiver binding or sound."),
        host=host,
        client=client,
        checkerSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host-log", type=Path, required=True)
    parser.add_argument("--client-log", type=Path, required=True)
    parser.add_argument("--out", type=Path)
    args = parser.parse_args()
    report = review(args.host_log, args.client_log)
    output = json.dumps(report, ensure_ascii=False, indent=2) + "\n"
    if args.out:
        protected = (args.host_log, args.client_log, Path(__file__))
        if args.out.exists():
            assert not any(args.out.samefile(path) for path in protected), "Cannot overwrite an input or checker"
        args.out.write_text(output, encoding="utf-8")
    else:
        print(output, end="")


if __name__ == "__main__":
    main()
