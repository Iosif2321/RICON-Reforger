"""Внешняя сверка напечатанных результатов S3b с замороженным JSON; без импорта production/oracle."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import sys

SOURCE_SHA = "a60ed14590222c103aceea9030fb80ee233eea432b5969b8ed7fe93ad5c10be3"
CASE = re.compile(r"\[RICON_LINKPUB\] ([A-Z][0-9]{2,3}) call=(\w+) snapshots=(\d+) seq=(\d+) reads=(\d+) "
                  r"full=(\w+) match=(\w+) immutable=(\w+)")
SUMMARY = re.compile(r"\[RICON_LINKPUB\] cases=(\d+) match=(\d+) immutable=(\d+) controls=(\d+) extras=(\d+) snapshots=(\d+)")
SNAP = re.compile(r"\[RICON_LINKPUB_SNAP\] link=(\d+) seq=(\d+) epoch=(\d+) talker=(\d+) listener=(\d+) "
                  r"state=(\w+) q=(\S+) ttl=(\d+)")


def truth(value):
    return value in ("true", "1")


def check(log, exit_code, corpus, extras):
    errors = []
    cases = corpus["cases"]
    expected = {row["id"]: row["expect"] for row in cases}
    if exit_code != 0:
        errors.append("process_exit")
    if re.search(r"SCRIPT\s*\(E\)", log):
        errors.append("script_error")
    if re.search(r"\[RICON_[^\]]+\].*(?<![A-Za-z])FAIL(?![A-Za-z])", log):
        errors.append("fail_marker")
    if log.count("[RICON_LINKPUB] done: PASS") != 1:
        errors.append("done_count")
    if re.findall(r"\[RICON_LINKPUB\] source=(\S+)", log) != [SOURCE_SHA]:
        errors.append("source_sha")
    found = CASE.findall(log)
    if [r[0] for r in found] != [r["id"] for r in cases]:
        errors.append("case_identity_or_order")
    for case_id, code, snapshots, seq, reads, full, match, immutable in found:
        exp = expected.get(case_id)
        if exp is None:
            errors.append(case_id+".unknown")
            continue
        if (code, int(snapshots), int(seq), int(reads), truth(full), truth(match), truth(immutable)) != (
                exp["callCode"], len(exp["snapshots"]), exp["sequenceCalls"], exp["epochReads"],
                exp["requiresFullResync"], True, True) or full not in ("true", "false", "0", "1"):
            errors.append(case_id+".values")
    total = sum(len(r["snapshots"]) for r in expected.values())
    summary = SUMMARY.findall(log)
    if len(summary) != 1 or tuple(map(int, summary[0])) != (len(cases), len(cases), len(cases), 15, extras, total):
        errors.append("summary")
    snaps = SNAP.findall(log)
    reference = expected["P04"]["snapshots"]
    if len(snaps) != len(reference):
        errors.append("snapshot_count")
    for index, (row, exp) in enumerate(zip(snaps, reference)):
        link, seq, epoch, talker, listener, state, quality, ttl = row
        try:
            quality = float(quality)
        except ValueError:
            quality = math.nan
        if (int(link), int(seq), int(epoch), int(talker), int(listener), state, int(ttl)) != (
                exp["linkId"], exp["linkSeq"], exp["sessionEpoch"], exp["talkerPlayerId"],
                exp["listenerPlayerId"], exp["state"], exp["ttlMs"]) or not math.isfinite(quality) \
                or abs(quality-exp["quality"]) > 1e-6:
            errors.append(f"snapshot[{index}]")
    return errors


def self_test(corpus):
    lines = [f"[RICON_LINKPUB] source={SOURCE_SHA}"]
    for c in corpus["cases"]:
        e = c["expect"]
        lines.append(f"[RICON_LINKPUB] {c['id']} call={e['callCode']} snapshots={len(e['snapshots'])} "
                     f"seq={e['sequenceCalls']} reads={e['epochReads']} full={int(e['requiresFullResync'])} match=1 immutable=1")
    for s in next(c for c in corpus["cases"] if c["id"] == "P04")["expect"]["snapshots"]:
        lines.append(f"[RICON_LINKPUB_SNAP] link={s['linkId']} seq={s['linkSeq']} epoch={s['sessionEpoch']} "
                     f"talker={s['talkerPlayerId']} listener={s['listenerPlayerId']} state={s['state']} "
                     f"q={s['quality']} ttl={s['ttlMs']}")
    total = sum(len(c["expect"]["snapshots"]) for c in corpus["cases"])
    lines += [f"[RICON_LINKPUB] cases=106 match=106 immutable=106 controls=15 extras=4 snapshots={total}",
              "[RICON_LINKPUB] done: PASS", "[RICON_DATA] reason=NUMERIC_FAILURE"]
    good = "\n".join(lines)
    assert not check(good, 0, corpus, 4)
    damaged = [good.replace(SOURCE_SHA, "0"*64), good.replace(lines[1], ""), good+"\n"+lines[1],
               good.replace("match=1 immutable=1", "match=0 immutable=1", 1),
               good.replace("q=0.75", "q=0.76", 1), good.replace("q=0.75", "q=NaN", 1),
               good.replace("controls=15", "controls=14"), good.replace("extras=4", "extras=3"),
               good+"\nSCRIPT (E): broken", good+"\n[RICON_TEST] X_ENFORCE_PARITY_FAIL",
               good.replace("done: PASS", "done: FAIL"), good+"\n[RICON_LINKPUB] done: PASS",
               good.replace("state=CLEAN", "state=LOST", 1), good.replace("seq=1 reads=2", "seq=2 reads=2", 1)]
    assert all(check(log, 0, corpus, 4) for log in damaged)
    assert check(good, 5, corpus, 4)
    print(f"S3B_LOG_CHECK_SELF_PASS ({len(damaged)+1} отрицательных контролей)")


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--corpus", type=Path, default=Path(__file__).resolve().parents[3]/"oracle/vectors/link_publication_v1.json")
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--log", type=Path)
    parser.add_argument("--exit-code", type=int)
    parser.add_argument("--extras", type=int)
    parser.add_argument("--receipt", type=Path)
    args = parser.parse_args()
    raw = args.corpus.read_bytes()
    if hashlib.sha256(raw).hexdigest() != SOURCE_SHA:
        parser.error("SHA корпуса изменён")
    corpus = json.loads(raw)
    if args.self_test:
        self_test(corpus)
    if args.log:
        if args.exit_code is None or args.extras is None:
            parser.error("Для журнала нужны exit-code и extras из проверенного запуска")
        log_bytes = args.log.read_bytes()
        errors = check(log_bytes.decode("utf-8", errors="replace"), args.exit_code, corpus, args.extras)
        receipt = {"operator": "codex", "level": "NATIVE_LOG_RECHECK", "log": str(args.log),
                   "logSha256": hashlib.sha256(log_bytes).hexdigest(), "corpusSha256": SOURCE_SHA,
                   "processExit": args.exit_code, "expectedExtras": args.extras, "errors": errors,
                   "pass": not errors, "limits": "Сверены напечатанные поля; остальные поля сравнивает native-стенд."}
        if args.receipt:
            args.receipt.write_text(json.dumps(receipt, ensure_ascii=False, indent=2)+"\n", encoding="utf-8")
        print("S3B_LOG_CHECK_FAIL" if errors else "S3B_LOG_CHECK_PASS", errors)
        return bool(errors)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
