#!/usr/bin/env python3
"""claude: independent check of a RICON_LiveStreamProbePlugin Workbench log (stream mode of RICON_LiveJournal).

The whole tagged transcript is compared, not a sample (codex #4044): every line containing "[RICON_LIVE" (the
[RICON_LIVE] and [RICON_LIVES] tags) is cut from the tag to the end and must equal, in order and with nothing extra:
  [RICON_LIVES] stream=BEGIN adds=1000 capacity=8
  1000 stream lines [RICON_LIVE] seq=<n> t=<5000+n> side=client kind=TX_CAPTURE player=7 device=-1 tr=-1 value=48000
                    text=binding=UNKNOWN, n = 0..999
  [RICON_LIVES] stream=END
  [RICON_LIVE] dump=BEGIN count=8 dropped=993 nextSeq=1000
  [RICON_LIVE] seq=0 t=5992 side=probe kind=GAP player=-1 device=-1 tr=-1 value=993 text=overflow
  the stream lines of seq 993..999, complete
  [RICON_LIVE] dump=END
  [RICON_LIVES] checks=7 passed=7 count=8 dropped=993 nextSeq=1000
  [RICON_LIVES] done: PASS
Untagged lines (engine diagnostics) are ignored; separately the log must have no SCRIPT (E) and no tagged FAIL token.
Usage: python -I live-stream-log-check-claude.py <console.log>
       python -I live-stream-log-check-claude.py --self-check"""
import hashlib
import re
import sys

ADDS, KEPT, DROPPED = 1000, 7, 993
TAG = "[RICON_LIVE"


def stream_line(n):
    return (f"[RICON_LIVE] seq={n} t={5000 + n} side=client kind=TX_CAPTURE player=7 device=-1 tr=-1 value=48000 "
            f"text=binding=UNKNOWN")


def transcript():
    lines = ["[RICON_LIVES] stream=BEGIN adds=1000 capacity=8"]
    lines += [stream_line(n) for n in range(ADDS)]
    lines += ["[RICON_LIVES] stream=END",
              "[RICON_LIVE] dump=BEGIN count=8 dropped=993 nextSeq=1000",
              "[RICON_LIVE] seq=0 t=5992 side=probe kind=GAP player=-1 device=-1 tr=-1 value=993 text=overflow"]
    lines += [stream_line(n) for n in range(ADDS - KEPT, ADDS)]
    lines += ["[RICON_LIVE] dump=END",
              "[RICON_LIVES] checks=7 passed=7 count=8 dropped=993 nextSeq=1000",
              "[RICON_LIVES] done: PASS"]
    return lines


def tagged(lines):
    out = []
    for line in lines:
        i = line.find(TAG)
        if i >= 0:
            out.append(line[i:].rstrip())
    return out


def check(lines):
    errors = []
    if any(re.search(r"SCRIPT\s*\(E\)", l) for l in lines):
        errors.append("SCRIPT (E)")
    if any(re.search(r"\[RICON_[^\]]+\].*(?<![A-Za-z])FAIL(?![A-Za-z])", l) for l in lines):
        errors.append("tagged FAIL")
    got, want = tagged(lines), transcript()
    if got != want:
        first = next((i for i, (a, b) in enumerate(zip(got, want)) if a != b), min(len(got), len(want)))
        errors.append(f"tagged transcript differs at line {first}: got {got[first] if first < len(got) else None!r}, "
                      f"want {want[first] if first < len(want) else None!r} (got {len(got)}, want {len(want)} lines)")
    return errors


def synthetic():
    return [f"05:00:00.{i % 1000:03d}  SCRIPT       : {line}" for i, line in enumerate(transcript())]


def self_check():
    base = synthetic()
    if check(base):
        print("SELF_CHECK_BASE_FAIL", check(base))
        return 1
    k = next(i for i, l in enumerate(base) if l.endswith(stream_line(500)))
    snap = next(i for i, l in enumerate(base) if l.endswith(stream_line(994)) and i > ADDS)
    noise = "05:00:00.500  ENGINE       : FileSystem: diagnostic line without a RICON tag"
    positive = base[:k + 1] + [noise] + base[k + 1:]
    if check(positive):
        print("SELF_CHECK_FAIL: untagged engine line rejected", check(positive))
        return 1
    mutations = {
        "stream line missing": base[:k] + base[k + 1:],
        "stream line duplicated": base[:k] + [base[k]] + base[k:],
        "stream order swapped": base[:k] + [base[k + 1], base[k]] + base[k + 2:],
        "stream value changed": base[:k] + [base[k].replace("value=48000", "value=48001")] + base[k + 1:],
        "stream time changed": base[:k] + [base[k].replace("t=5500", "t=5501")] + base[k + 1:],
        "no stream frame": [l for l in base if "stream=BEGIN" not in l],
        "GAP value": [l.replace("value=993 text=overflow", "value=992 text=overflow") for l in base],
        "GAP time": [l.replace("seq=0 t=5992 side=probe", "seq=0 t=5991 side=probe") for l in base],
        "dump header": [l.replace("dump=BEGIN count=8 dropped=993", "dump=BEGIN count=8 dropped=992") for l in base],
        "dump row missing": base[:snap] + base[snap + 1:],
        "snapshot value changed (codex #4044)": base[:snap] + [base[snap].replace("value=48000", "value=123")] + base[snap + 1:],
        "extra event after done (codex #4044)": base + ["05:00:00.999  SCRIPT       : " + stream_line(1000)],
        "summary duplicated (codex #4044)": base[:-1] + [base[-2]] + base[-1:],
        "no dump end": [l for l in base if "dump=END" not in l],
        "summary": [l.replace("checks=7 passed=7", "checks=7 passed=6") for l in base],
        "no done": [l for l in base if "done: PASS" not in l],
        "two done": base + [base[-1]],
        "script error": base + ['05:00:00.999   SCRIPT    (E): boom'],
        "mismatch line": base + ["05:00:00.999  SCRIPT       : [RICON_LIVES] mismatch: x"],
        "tagged FAIL": base + ["05:00:00.999  SCRIPT       : [RICON_LIVES] done: FAIL"],
    }
    accepted = [name for name, lines in mutations.items() if not check(lines)]
    if accepted:
        print("SELF_CHECK_FAIL: mutations accepted:", accepted)
        return 1
    print(f"LIVE_STREAM_LOG_SELFCHECK_PASS: synthetic log and untagged noise accepted, {len(mutations)} mutations rejected")
    return 0


def main(argv):
    if argv and argv[0] == "--self-check":
        return self_check()
    raw = open(argv[0], "rb").read()
    errors = check(raw.decode("utf-8", errors="replace").splitlines())
    if errors:
        print("\n".join(errors))
        return 1
    print(f"LIVE_STREAM_LOG_CHECK_PASS: tagged transcript of {len(transcript())} lines identical "
          f"(1000 stream lines, snapshot GAP 993 + seq 993..999), log sha256 {hashlib.sha256(raw).hexdigest()}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
