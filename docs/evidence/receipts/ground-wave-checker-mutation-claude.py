#!/usr/bin/env python3
"""claude: мутационное ревью независимого GW-checker codex (ground-wave-blind-check-codex.py, aafebfc).

Копирует минимальное дерево проекта (checker, оболочку, REQ, корпус, CSV NTIA) во временную папку --work, портит
копию корпуса по одной мутации за раз и ожидает, что checker завершится ошибкой. Файлы проекта не изменяются.
Эталонная программа — своя сборка оболочки codex на неизменённых исходниках LFMF v1.1 (MSVC /EHs /O2 /std:c++17).
На aafebfc пропущено только усечение корпуса; codex 2fb6e4a закрепил ровно 265 случаев и LF-SHA корпуса, теперь
пропусков быть не должно. Проверка SHA стоит в конце checker, поэтому мутация, пойманная только ею («Изменён
замороженный корпус»), помечается SHA_GATE_ONLY: сравнение чисел её бы не заметило.
Уровень: проверка проверяющего, не физическая и не игровая приёмка.
Запуск: python -B -X utf8 ground-wave-checker-mutation-claude.py --exe <exe> --source-zip <zip> --source-dir <LFMF-main>
        --test-data-zip <zip> --work <temp>"""
import argparse
import copy
import json
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
FILES = ["docs/evidence/receipts/ground-wave-blind-check-codex.py", "docs/evidence/receipts/ground-wave-reference-codex.cpp",
         "docs/req/REQ-ground-wave.md", "oracle/vectors/ground_wave_v1.json", "oracle/data/lfmf_examples_ntia_v1.csv"]
CORPUS = FILES[3]
EXPECTED_MISS = None  # aafebfc: "drop last case"; 2fb6e4a закрыл (codex #4154)
SHA_GATE = "Изменён замороженный корпус"


def first(cases, code):
    return next(c for c in cases if c["expect"]["code"] == code)


def set_ok(key, change):
    def apply(d):
        e = first(d["cases"], "OK")["expect"]
        e[key] = change(e[key])
    return apply


def mutations():
    other = {"FLAT_EARTH_CURVE": "RESIDUE_SERIES", "RESIDUE_SERIES": "FLAT_EARTH_CURVE"}
    yield "OK basicLossDb +2e-8", set_ok("basicLossDb", lambda v: v + 2e-8)
    yield "OK fieldDbuVm -2e-8", set_ok("fieldDbuVm", lambda v: v - 2e-8)
    yield "OK method swapped", set_ok("method", lambda v: other[v])
    yield "OK kr *(1+1e-11)", set_ok("kr", lambda v: v * (1 + 1e-11))
    yield "OK heightToWavelength *(1+1e-11)", set_ok("heightToWavelength", lambda v: v * (1 + 1e-11) if v else 1e-300)
    yield "OK provenance changed", set_ok("provenance", lambda v: v + "_X")
    yield "OK extra field", lambda d: first(d["cases"], "OK")["expect"].__setitem__("extra", 1)
    yield "INVALID -> OOD", lambda d: first(d["cases"], "INVALID_INPUT")["expect"].__setitem__("code", "OUT_OF_MODEL_DOMAIN")
    yield "OOD -> INVALID", lambda d: first(d["cases"], "OUT_OF_MODEL_DOMAIN")["expect"].__setitem__("code", "INVALID_INPUT")
    yield "NUMERIC_FAILURE -> OK", lambda d: first(d["cases"], "NUMERIC_FAILURE")["expect"].update(
        {"code": "OK", "basicLossDb": 1.0, "fieldDbuVm": 1.0, "method": "RESIDUE_SERIES", "kr": 1.0,
         "heightToWavelength": 1.0, "provenance": "TEST_ONLY"})
    yield "OK input distance x2", lambda d: first(d["cases"], "OK")["input"].__setitem__(
        "distanceKm", first(d["cases"], "OK")["input"]["distanceKm"] * 2)
    yield "case id gap", lambda d: d["cases"][5].__setitem__("id", "GW999")
    yield "jsonOnly not bool", lambda d: d["cases"][0].__setitem__("jsonOnly", 0)
    yield "ntia example aBtlDb +0.01", lambda d: (lambda e: e.__setitem__("aBtlDb", e["aBtlDb"] + 0.01))(
        next(e for e in d["ntiaExamples"] if e["rc"] == 0))
    yield "requirementsLfSha256 changed", lambda d: d.__setitem__("requirementsLfSha256", "0" * 64)
    yield "drop last case", lambda d: d["cases"].pop()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ("--exe", "--source-zip", "--source-dir", "--test-data-zip", "--work"):
        p.add_argument(name, type=Path, required=True)
    a = p.parse_args()
    tree = a.work / "tree"
    if tree.exists():
        shutil.rmtree(tree)
    for f in FILES:
        (tree / f).parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(ROOT / f, tree / f)

    def run():
        r = subprocess.run([sys.executable, "-B", "-X", "utf8", str(tree / FILES[0]), "--exe", str(a.exe),
                            "--source-zip", str(a.source_zip), "--source-dir", str(a.source_dir),
                            "--test-data-zip", str(a.test_data_zip), "--report", str(a.work / "mutation-report.json")],
                           capture_output=True, text=True, encoding="utf-8", errors="replace")
        out = r.stdout if r.returncode == 0 else r.stderr
        return r.returncode, (out.strip().splitlines() or [""])[-1][:160]

    base_raw = (ROOT / CORPUS).read_text(encoding="utf-8")
    base = json.loads(base_raw)
    rc, last = run()
    print(f"baseline rc={rc} {last}")
    if rc != 0:
        print("GW_CHECKER_MUTATION_FAIL baseline")
        return 1
    rows = []
    for name, apply in mutations():
        d = copy.deepcopy(base)
        apply(d)
        (tree / CORPUS).write_text(json.dumps(d, ensure_ascii=False, indent=1), encoding="utf-8")
        rows.append((name,) + run())
    (tree / CORPUS).write_text(base_raw.replace('"schema"', '"version": 1, "schema"', 1), encoding="utf-8")
    rows.append(("duplicate JSON key",) + run())
    shutil.copyfile(ROOT / CORPUS, tree / CORPUS)
    rc_restored, last_restored = run()
    bad = []
    for name, rc, last in rows:
        caught = rc != 0
        label = ("SHA_GATE_ONLY" if SHA_GATE in last else "CAUGHT") if caught else "MISSED"
        print(f"{label:13} {name}: {last}")
        if caught == (name == EXPECTED_MISS):
            bad.append(name)
    print(f"restored rc={rc_restored} {last_restored}")
    caught = sum(rc != 0 for _, rc, _ in rows)
    print(f"mutations={len(rows)} caught={caught} expectedMiss={EXPECTED_MISS!r} unexpected={bad}")
    ok = not bad and rc_restored == 0
    print("GW_CHECKER_MUTATION_" + ("PASS" if ok else "FAIL"))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
