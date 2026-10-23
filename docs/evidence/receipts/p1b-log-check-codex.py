#!/usr/bin/env python3
"""Независимая сверка native-лога P1b с замороженным JSON, без эталона и генератора."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[3]
SHA = "cf5030f6a4ab5e03b5c02ef63aa4211a012b6a36a059ee2b11b519491a925f61"
HOST = {"F27", "F28", "F29", "F30", "F57", "F70"}
SUMMARY = "cases=65 match=65 immutable=65 controls=9 extras=3 hostOnly=6"


def need(condition, message):
    if not condition:
        raise ValueError(message)


def cases():
    raw = (ROOT / "oracle/vectors/channel_fading_v1.json").read_bytes()
    need(hashlib.sha256(raw).hexdigest() == SHA, "Изменён замороженный корпус")
    data = json.loads(raw)
    need(set(data["hostOnly"]) == HOST, "Изменены host-only случаи")
    return {c["id"]: c["expect"] for c in data["cases"] if c["id"] not in HOST}


def check(text, expected):
    need(not re.search(r"SCRIPT\s*\(E\)|\[RICON[^\n]*\bFAIL\b", text), "В логе ошибка или FAIL")
    lines = [s.split("[RICON_FADING] ", 1)[1].strip() for s in text.splitlines() if "[RICON_FADING] " in s]
    need(lines.count("source=" + SHA) == 1 and sum(s.startswith("source=") for s in lines) == 1, "Неверный SHA источника")
    need(lines.count("done: PASS") == 1 and sum(s.startswith("done:") for s in lines) == 1, "Нет единственного PASS")
    need(lines.count(SUMMARY) == 1 and sum(s.startswith("cases=") for s in lines) == 1, "Неверные счётчики")
    rows, numeric_fields, max_gain_error = set(), 0, 0
    for line in lines:
        if not re.match(r"F\d", line):
            continue
        key, rest = line.split(" ", 1)
        need(key in expected and key not in rows, "Повторный, неизвестный или host-only случай")
        fields = dict(re.findall(r"(\w+)=(\S+)", rest))
        need(set(fields) == {"code", "real", "imag", "gain", "phases", "match", "immutable"}, "Неполная строка результата")
        exp = expected[key]
        need(fields["code"] == exp["code"], key + ": неверный код")
        need(fields["match"] == "1" and fields["immutable"] == "1", key + ": совпадение или неизменность не подтверждены")
        need(int(fields["phases"]) == len(exp["phasesQ"]), key + ": неверное число фаз")
        for name, exp_key in (("real", "real"), ("imag", "imag"), ("gain", "gainDb")):
            target = exp[exp_key]
            if target is None:
                need(fields[name] == "null", key + ": число вместо отсутствующего значения")
                continue
            number = float(fields[name])
            error = abs(number - target)
            # Строка стенда имеет девять знаков после точки; строгость f32 проверяет native comparator.
            tolerance = .00100000051 if name == "gain" else 5.1e-10
            need(math.isfinite(number) and error <= tolerance, f"{key}.{name}: {number} != {target}")
            numeric_fields += 1
            if name == "gain":
                max_gain_error = max(max_gain_error, error)
        rows.add(key)
    need(rows == set(expected), "Пропущены случаи")
    faults = [s.split("[RICON_FADING_FAULTS] ", 1)[1].strip() for s in text.splitlines() if "[RICON_FADING_FAULTS] " in s]
    need(len(faults) == 1, "Нет единственной калибровки субнормали")
    fields = dict(re.findall(r"(\w+)=(\S+)", faults[0]))
    need(fields.get("code") == "OK", "Субнормаль не поддержана в этой калибровке")
    gain = float(fields.get("gain", "nan"))
    need(math.isfinite(gain) and abs(gain - 20 * math.log10(2 ** -149)) <= .001, "Неверный gain минимальной субнормали")
    return {"cases": len(rows), "nativeControls": 9, "nativeExtras": 3, "hostOnly": 6,
            "printedNumericFields": numeric_fields, "maxObservedGainErrorDb": max_gain_error,
            "subnormal": {"code": "OK", "gainDb": gain},
            "unprintedPhases": "Внешне сверено число; значения строго проверены native comparator по fixture",
            "coefficientPrecision": "Печать округлена до 9 знаков; native comparator сравнивает f32 строго"}


def self_test(expected):
    lines = ["[RICON_FADING] source=" + SHA]
    for key, exp in expected.items():
        values = ["null" if exp[k] is None else f"{exp[k]:.9f}" for k in ("real", "imag", "gainDb")]
        lines.append(f"[RICON_FADING] {key} code={exp['code']} real={values[0]} imag={values[1]} gain={values[2]} "
                     f"phases={len(exp['phasesQ'])} match=1 immutable=1")
    lines += ["[RICON_FADING_FAULTS] subnormal code=OK gain=-897.069387078", "[RICON_FADING] " + SUMMARY,
              "[RICON_FADING] done: PASS"]
    text = "\n".join(lines)
    check(text, expected)
    target = next(s for s in lines if " F05 " in s)
    mutants = [text.replace(SHA, "0" * 64), text + "\n" + target, text.replace(target, ""),
               text.replace("match=65", "match=64"), text.replace("done: PASS", "done: FAIL"),
               text.replace("done: PASS", ""), text + "\n[RICON_FADING] done: PASS",
               text + "\nSCRIPT (E): намеренная ошибка", text.replace("F05 ", "F27 "),
               text.replace("subnormal code=OK", "subnormal code=ZERO_AT_SAMPLE"),
               text.replace("gain=-897.069387078", "gain=nan")]
    for field in ("code", "real", "imag", "gain", "phases", "match", "immutable"):
        corrupt = re.sub(rf"\b{field}=\S+", field + "=99999", target)
        mutants.append(text.replace(target, corrupt))
    mutants += [text.replace(target, re.sub(r"\breal=\S+", "real=nan", target)),
                text.replace(target, re.sub(r"\bgain=\S+", "gain=null", target)),
                text.replace(target, target.replace(" phases=2", ""))]
    for i, mutant in enumerate(mutants):
        try:
            check(mutant, expected)
        except (ValueError, KeyError):
            continue
        raise AssertionError(f"Не обнаружено искажение {i}")
    return len(mutants)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--receipt", type=Path)
    group.add_argument("--log", type=Path)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    expected = cases()
    result = {"validatorSelfTest": "PASS", "negativeControls": self_test(expected)}
    path = args.log
    if args.receipt:
        receipt = json.loads(args.receipt.read_text(encoding="utf-8-sig"))
        need(receipt["exitCode"] == 0 and receipt["sourceStable"] is True and receipt["worldStarted"] is False, "Процесс/исходники/мир")
        for source in receipt["sources"]:
            need(hashlib.sha256((ROOT / source["path"]).read_bytes()).hexdigest() == source["sha256"].lower(), "Исходник изменён после запуска")
        path = Path(receipt["logPath"])
        need(hashlib.sha256(path.read_bytes()).hexdigest() == receipt["logSha256"].lower(), "Лог изменён после запуска")
    if path:
        raw = path.read_bytes()
        result.update(check(raw.decode("utf-8", errors="replace"), expected))
        result.update(logSha256=hashlib.sha256(raw).hexdigest(), processReceiptChecked=bool(args.receipt), status="NATIVE_LOG_PASS")
    if args.report:
        args.report.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps(result, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
