#!/usr/bin/env python3
"""Независимая проверка native-лога P2 по frozen JSON, без импорта oracle или генератора."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[3]
SOURCE_SHA = "7a12ed92587318108059a213ace1d3f4286ad9a66bf86e8eb63dd4cdc7eea032"
HOST = {"O16", "O19", "W17", "W21"}
CODES = ("INVALID_OBSERVATION", "INVALID_PROFILE", "WATER_COVERAGE_INCOMPLETE", "NO_WATER",
         "OUT_OF_MODEL_DOMAIN", "NUMERIC_FAILURE", "OK")
EXTRAS = {"null_input", "null_profile", "segments_1024_revision_MAX", "segments_1025",
          "global_invalid_before_profile", "null_segment", "null_segments"}


def need(condition, message):
    if not condition:
        raise ValueError(message)


def cases():
    raw = (ROOT / "oracle/vectors/path_observation_v1.json").read_bytes()
    need(hashlib.sha256(raw).hexdigest() == SOURCE_SHA, "Изменён frozen JSON")
    return {c["id"]: c["expect"]["water"] for c in json.loads(raw)["cases"] if c["id"] not in HOST}


def check(text, expected):
    need(not re.search(r"SCRIPT\s+\(E\)|\[RICON[^\n]*\bFAIL\b", text), "В логе ошибка или FAIL")
    lines = [line.split("[RICON_PATH] ", 1)[1].strip() for line in text.splitlines() if "[RICON_PATH] " in line]
    need(lines.count("source=" + SOURCE_SHA) == 1 and sum(x.startswith("source=") for x in lines) == 1, "Неверный source SHA")
    need(lines.count("done: PASS") == 1, "Нет единственного PASS")
    need(sum(x.startswith("cases=") for x in lines) == 1, "Повтор/отсутствие итога")
    need(f"cases={len(expected)} match={len(expected)} immutable={len(expected)} controls=11 extras=7 hostOnly=4" in lines,
         "Неверные счётчики")
    rows, extras = {}, {}
    max_relative_error = 0.0
    for line in lines:
        if not line.startswith(("case=", "extra=")):
            continue
        fields = dict(re.findall(r"(\w+)=(\S+)", line))
        if "extra" in fields:
            key = fields["extra"]
            need(key not in extras and fields.get("pass") == "1", "Повтор или отказ extra")
            extras[key] = True
            continue
        key = fields["case"]
        need(key in expected and key not in rows, "Неизвестный/повторный случай")
        need(set(fields) == {"case", "code", "hasLoss", "loss", "alpha", "interface", "waterLength", "crossings", "intervals", "immutable"},
             "Неполная строка результата")
        exp = expected[key]
        need(int(fields["code"]) == CODES.index(exp["code"]), f"{key}: неверный код")
        need(fields["hasLoss"] == str(int("lossDb" in exp)), f"{key}: неверное наличие потерь")
        need(fields["immutable"] == "true", f"{key}: изменён вход/повторный результат")
        numeric = {"loss": exp.get("lossDb", 0)}
        if exp["code"] == "OK":
            numeric.update(alpha=exp["alphaDbPerM"], interface=exp["interfaceDb"], waterLength=exp["waterLengthM"])
            need(int(fields["crossings"]) == exp["crossings"] and int(fields["intervals"]) == len(exp["intervals"]), f"{key}: геометрия")
        else:
            numeric.update(alpha=0, interface=0, waterLength=0)
            need(fields["crossings"] == "0" and fields["intervals"] == "0", f"{key}: данные при отсутствии OK")
        for name, value in numeric.items():
            actual = float(fields[name])
            error = abs(actual - value)
            need(math.isfinite(actual) and error <= max(0.001, 1e-5 * abs(value)), f"{key}.{name}: {actual} != {value}")
            if value:
                max_relative_error = max(max_relative_error, error / abs(value))
        rows[key] = True
    need(set(rows) == set(expected), "Пропущены случаи")
    need(set(extras) == EXTRAS, "Пропущены/добавлены extras")
    return {"cases": len(rows), "printedNumericFields": 4 * len(rows), "maxObservedRelativeError": max_relative_error,
            "controls": 11, "extras": len(extras), "hostOnly": len(HOST)}


def self_test(expected):
    lines = ["[RICON_PATH] source=" + SOURCE_SHA]
    for key, exp in expected.items():
        lines.append(f"[RICON_PATH] case={key} code={CODES.index(exp['code'])} hasLoss={int('lossDb' in exp)} "
                     f"loss={exp.get('lossDb', 0)} alpha={exp.get('alphaDbPerM', 0)} interface={exp.get('interfaceDb', 0)} "
                     f"waterLength={exp.get('waterLengthM', 0)} crossings={exp.get('crossings', 0)} "
                     f"intervals={len(exp.get('intervals', []))} immutable=true")
    lines += [f"[RICON_PATH] extra={key} pass=1" for key in sorted(EXTRAS)]
    lines += [f"[RICON_PATH] cases={len(expected)} match={len(expected)} immutable={len(expected)} controls=11 extras=7 hostOnly=4",
              "[RICON_PATH] done: PASS"]
    text = "\n".join(lines)
    check(text, expected)
    target = next(line for line in lines if "case=W06 " in line)
    mutants = [text.replace(SOURCE_SHA, "0" * 64), text + "\n" + target, text.replace(target, ""),
               text.replace("immutable=true", "immutable=false", 1), text.replace("match=47", "match=46"),
               text.replace("done: PASS", "done: FAIL"), text.replace("done: PASS", ""),
               text + "\n[RICON_PATH] done: PASS", text + "\nSCRIPT (E): пример ошибки",
               text.replace("pass=1", "pass=0", 1), text.replace(lines[-3], "")]
    for name in ("code", "hasLoss", "loss", "alpha", "interface", "waterLength", "crossings", "intervals"):
        corrupted = re.sub(rf"\b{name}=\S+", name + "=99999", target)
        mutants.append(text.replace(target, corrupted))
    mutants.append(text.replace(target, re.sub(r"\bloss=\S+", "loss=nan", target)))
    for index, mutant in enumerate(mutants):
        try:
            check(mutant, expected)
        except (ValueError, KeyError):
            pass
        else:
            raise AssertionError(f"Контроль {index} пропущен")
    return len(mutants)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--receipt", type=Path)
    args = parser.parse_args()
    expected = cases()
    controls = self_test(expected)
    report = {"validatorSelfTest": "PASS", "negativeControls": controls}
    if args.receipt:
        receipt = json.loads(args.receipt.read_text(encoding="utf-8-sig"))
        need(receipt["exitCode"] == 0 and receipt["sourceStable"] is True and receipt["worldStarted"] is False, "Процесс/исходники/мир")
        log = Path(receipt["logPath"]).read_bytes()
        need(hashlib.sha256(log).hexdigest() == receipt["logSha256"].lower(), "Изменён native-лог")
        for source in receipt["sources"]:
            need(hashlib.sha256((ROOT / source["path"]).read_bytes()).hexdigest() == source["sha256"].lower(), "Исходники изменились после запуска")
        report.update(check(log.decode("utf-8", errors="replace"), expected))
        report.update(logSha256=receipt["logSha256"].lower(), status="NATIVE_LOG_PASS",
                      unprintedIntervalBounds="Проверены native comparator; в лог выведено только число интервалов")
    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
