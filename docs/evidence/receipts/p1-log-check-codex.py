"""Независимая сверка напечатанных решений и чисел P1 с LF-корпусом, без импорта oracle/Enforce."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import sys

SOURCE_SHA = "b4ff545d34de7859aae3d71678689a727c835c57f46d429df1a722d492c654d8"
HOST_ONLY = {"C23", "C24", "C26"}
CASE = re.compile(r"\[RICON_CUTOFF\] (C[0-9]{2}) decision=(\w+) reason=(\w+) zero=(\w+) upper=(\S+) match=(\w+) immutable=(\w+)")
SUMMARY = re.compile(r"\[RICON_CUTOFF\] cases=(\d+) match=(\d+) immutable=(\d+) controls=(\d+) extras=(\d+) hostOnly=(\d+)")


def number_close(text, expected):
    if expected is None:
        return text == "null"
    try:
        value = float(text)
    except ValueError:
        return False
    return math.isfinite(value) and abs(value-expected) <= 0.001


def check(log, exit_code, corpus):
    errors = []
    native = [c for c in corpus["cases"] if c["id"] not in HOST_ONLY]
    if exit_code != 0:
        errors.append("process_exit")
    if re.search(r"SCRIPT\s*\(E\)", log):
        errors.append("script_error")
    if re.search(r"\[RICON_[^\]]+\].*(?<![A-Za-z])FAIL(?![A-Za-z])", log):
        errors.append("fail_marker")
    if log.count("[RICON_CUTOFF] done: PASS") != 1:
        errors.append("done_count")
    if re.findall(r"\[RICON_CUTOFF\] source=(\S+)", log) != [SOURCE_SHA]:
        errors.append("source_sha")
    rows = CASE.findall(log)
    if [r[0] for r in rows] != [c["id"] for c in native]:
        errors.append("case_identity_or_order")
    expected = {c["id"]: c["expect"] for c in native}
    for case_id, decision, reason, zero, upper, match, immutable in rows:
        exp = expected.get(case_id)
        if exp is None:
            errors.append(case_id+".unknown")
            continue
        if decision != exp["decision"] or reason != exp["reason"] \
                or zero not in ("0", "1", "true", "false") or (zero in ("1", "true")) != exp["usefulIsZero"] \
                or match not in ("1", "true") or immutable not in ("1", "true") \
                or not number_close(upper, exp["upperDbm"]):
            errors.append(case_id+".values")
    summary = SUMMARY.findall(log)
    if len(summary) != 1 or tuple(map(int, summary[0])) != (24, 24, 24, 7, 3, 3):
        errors.append("summary")
    limit = re.findall(r"\[RICON_CUTOFF_LIMIT\] paths=(\d+) upper=(\S+)", log)
    if len(limit) != 1 or limit[0][0] != "4096" or not number_close(limit[0][1], -200+20*math.log10(4096)):
        errors.append("limit_4096")
    return errors


def self_test(corpus):
    lines = [f"[RICON_CUTOFF] source={SOURCE_SHA}"]
    for c in corpus["cases"]:
        if c["id"] in HOST_ONLY:
            continue
        e = c["expect"]
        upper = "null" if e["upperDbm"] is None else str(e["upperDbm"])
        lines.append(f"[RICON_CUTOFF] {c['id']} decision={e['decision']} reason={e['reason']} "
                     f"zero={int(e['usefulIsZero'])} upper={upper} match=1 immutable=1")
    lines += [f"[RICON_CUTOFF_LIMIT] paths=4096 upper={-200+20*math.log10(4096)}",
              "[RICON_CUTOFF] cases=24 match=24 immutable=24 controls=7 extras=3 hostOnly=3",
              "[RICON_CUTOFF] done: PASS", "[RICON_DATA] reason=NUMERIC_FAILURE"]
    good = "\n".join(lines)
    assert not check(good, 0, corpus)
    variants = [good.replace(SOURCE_SHA, "0"*64), good.replace(lines[1], ""), good+"\n"+lines[1],
                good.replace("upper=-150.0", "upper=-149.99", 1), good.replace("upper=-150.0", "upper=NaN", 1),
                good.replace("upper=-150.0", "upper=null", 1), good.replace("zero=1", "zero=0", 1),
                good.replace("match=1 immutable=1", "match=0 immutable=1", 1),
                good.replace("controls=7", "controls=6"), good.replace("hostOnly=3", "hostOnly=4"),
                good.replace("paths=4096", "paths=4097"), good+"\nSCRIPT (E): broken",
                good+"\n[RICON_TEST] X_ENFORCE_PARITY_FAIL", good.replace("done: PASS", "done: FAIL")]
    assert all(check(text, 0, corpus) for text in variants)
    assert check(good, 5, corpus)
    print(f"P1_LOG_CHECK_SELF_PASS ({len(variants)+1} отрицательных контролей)")


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--corpus", type=Path, default=Path(__file__).resolve().parents[3]/"oracle/vectors/channel_cutoff_v1.json")
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--log", type=Path)
    parser.add_argument("--exit-code", type=int)
    parser.add_argument("--receipt", type=Path)
    args = parser.parse_args()
    raw = args.corpus.read_bytes()
    if hashlib.sha256(raw).hexdigest() != SOURCE_SHA:
        parser.error("SHA корпуса изменён")
    corpus = json.loads(raw)
    if args.self_test:
        self_test(corpus)
    if args.log:
        if args.exit_code is None:
            parser.error("Нужен exit-code из проверенного запуска")
        raw_log = args.log.read_bytes()
        errors = check(raw_log.decode("utf-8", errors="replace"), args.exit_code, corpus)
        report = {"operator": "codex", "level": "NATIVE_LOG_RECHECK", "corpusSha256": SOURCE_SHA,
                  "log": str(args.log), "logSha256": hashlib.sha256(raw_log).hexdigest(),
                  "processExit": args.exit_code, "nativeCases": 24, "hostOnlyCases": 3,
                  "errors": errors, "pass": not errors,
                  "limits": "Сверены напечатанные поля, не production-профиль и не поведение в мире."}
        if args.receipt:
            args.receipt.write_text(json.dumps(report, ensure_ascii=False, indent=2)+"\n", encoding="utf-8", newline="\n")
        print("P1_LOG_CHECK_FAIL" if errors else "P1_LOG_CHECK_PASS", errors)
        return bool(errors)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
