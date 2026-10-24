"""Слепая проверка GW: собственный контракт и отдельная сборка оригинального NTIA; порт Claude не импортируется."""
import argparse
import copy
import csv
import hashlib
import io
import json
import math
from pathlib import Path
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[3]
SOURCE_SHA = "be6ed041d4f095c8845e8cce800e1a4ffb568401dec6f5d93c14b7a8ce58fd95"
TEST_DATA_SHA = "53f5331b2337a2b60376a3d2538fb14948a82611281a5bbed90e499ce0fa2be6"
REQ_SHA = "198033126e72322887ef6e9b0b0467e89fb9168d395bbdb74e74024d194d5259"
INPUT_KEYS = set("frequencyMHz distanceKm txHeightM rxHeightM polarization surfaceRefractivityN ground".split())
GROUND_KEYS = {"epsilonReal", "conductivitySm", "provenance"}
METHODS = ("FLAT_EARTH_CURVE", "RESIDUE_SERIES")


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def need(ok, message):
    if not ok:
        raise AssertionError(message)


def number(value):
    try:
        return type(value) in (float, int) and math.isfinite(value)
    except OverflowError:
        return False


def unique_object(pairs):
    result = {}
    for key, value in pairs:
        need(key not in result, "Повторный ключ JSON: " + key)
        result[key] = value
    return result


def rejection(value):
    if not isinstance(value, dict) or set(value) != INPUT_KEYS:
        return "INVALID_INPUT"
    ground = value["ground"]
    if not isinstance(ground, dict) or set(ground) != GROUND_KEYS:
        return "INVALID_INPUT"
    if value["polarization"] not in ("V", "H") or ground["provenance"] not in ("TEST_ONLY", "SOURCE_DOMAIN", "ASSUMED_CONFIG"):
        return "INVALID_INPUT"
    numbers = [value[k] for k in ("frequencyMHz", "distanceKm", "txHeightM", "rxHeightM", "surfaceRefractivityN")]
    numbers += [ground["epsilonReal"], ground["conductivitySm"]]
    if not all(number(v) for v in numbers):
        return "INVALID_INPUT"
    f, d, ht, hr, ns, eps, sigma = numbers
    if f <= 0 or d <= 0 or ht < 0 or hr < 0 or ns < 0 or eps < 1 or sigma < 0:
        return "INVALID_INPUT"
    if ht > 50 or hr > 50 or not .01 <= f <= 30 or not 250 <= ns <= 400 or not .001 <= d <= 10000 or sigma == 0 or value["polarization"] == "H":
        return "OUT_OF_MODEL_DOMAIN"
    return None


def arguments(value):
    g = value["ground"]
    return [value["txHeightM"], value["rxHeightM"], value["frequencyMHz"], 1000,
            value["surfaceRefractivityN"], value["distanceKm"], g["epsilonReal"], g["conductivitySm"], 1]


def result(value, native):
    if native["exception"]:
        return {"code": "NUMERIC_FAILURE"}
    need(native["rc"] == 0, "NTIA отверг вход внутри независимо проверенного домена")
    if not number(native["loss"]) or not number(native["field"]):
        return {"code": "NUMERIC_FAILURE"}
    f, d = value["frequencyMHz"], value["distanceKm"]
    method = 0 if d < 80 / math.cbrt(f) else 1
    need(native["method"] == method, "Ветка NTIA не совпала с GW04")
    wavelength = 299792458 / (f * 1e6)
    return {"code": "OK", "basicLossDb": native["loss"], "fieldDbuVm": native["field"],
            "method": METHODS[method], "kr": math.tau * d * 1000 / wavelength,
            "heightToWavelength": max(value["txHeightM"], value["rxHeightM"]) / wavelength,
            "provenance": value["ground"]["provenance"]}


def compare(actual, expected):
    need(set(actual) == set(expected) and actual["code"] == expected["code"], "Код или поля результата")
    worst = 0.0
    if actual["code"] == "OK":
        for key in ("method", "provenance"):
            need(actual[key] == expected[key], "Неверное " + key)
        for key in ("basicLossDb", "fieldDbuVm"):
            need(number(expected[key]), "Не конечное ожидание " + key)
            delta = abs(actual[key] - expected[key])
            need(delta <= 1e-8, f"{key}: {actual[key]} != {expected[key]}")
            worst = max(worst, delta)
        for key in ("kr", "heightToWavelength"):
            need(number(expected[key]) and math.isclose(actual[key], expected[key], rel_tol=1e-12, abs_tol=0), key)
    return worst


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--source-zip", type=Path, required=True)
    parser.add_argument("--source-dir", type=Path, required=True)
    parser.add_argument("--test-data-zip", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    source_raw, test_raw = args.source_zip.read_bytes(), args.test_data_zip.read_bytes()
    need(sha(source_raw) == SOURCE_SHA, "Другой архив NTIA LFMF")
    need(sha(test_raw) == TEST_DATA_SHA, "Другой архив примеров NTIA")
    checked_sources = 0
    with zipfile.ZipFile(io.BytesIO(source_raw)) as archive:
        commit = archive.comment.decode("ascii")
        for name in archive.namelist():
            if name.endswith((".cpp", ".h")):
                local = args.source_dir / Path(name).relative_to("LFMF-main")
                need(local.read_bytes() == archive.read(name), "Изменён оригинальный исходник: " + name)
                checked_sources += 1
    with zipfile.ZipFile(io.BytesIO(test_raw)) as archive:
        names = [n for n in archive.namelist() if n.endswith("/LFMF_Examples.csv")]
        need(len(names) == 1, "Неоднозначный CSV NTIA")
        csv_raw = archive.read(names[0])
    need(csv_raw == (ROOT/"oracle/data/lfmf_examples_ntia_v1.csv").read_bytes(), "Копия CSV отличается от NTIA")
    published = list(csv.DictReader(io.StringIO(csv_raw.decode("utf-8-sig"))))
    corpus_path = ROOT/"oracle/vectors/ground_wave_v1.json"
    req_path = ROOT/"docs/req/REQ-ground-wave.md"
    corpus_raw = corpus_path.read_bytes()
    data = json.loads(corpus_raw, object_pairs_hook=unique_object)
    need(data["schema"] == "ricon.oracle.ground_wave" and type(data["version"]) is int
         and data["version"] == 1 and type(data["revision"]) is int and data["revision"] >= 1, "Неверная версия корпуса")
    need(sha(req_path.read_bytes().replace(b"\r\n", b"\n")) == REQ_SHA == data["requirementsLfSha256"], "REQ изменился")
    cases = data["cases"]
    need([c["id"] for c in cases] == [f"GW{i:03d}" for i in range(1, len(cases)+1)], "Порядок/полнота ID")
    need({c["expect"]["code"] for c in cases} == {"OK", "INVALID_INPUT", "OUT_OF_MODEL_DOMAIN", "NUMERIC_FAILURE"}, "Не покрыты все коды GW03")
    need(len(published) == len(data["ntiaExamples"]) == 95, "Число примеров NTIA")
    jobs, positions = [], {}
    for case in cases:
        need(type(case["jsonOnly"]) is bool, "Неверная метка JSON-only")
        value = case["input"]
        if rejection(value) is None:
            positions[case["id"]] = len(jobs)
            jobs.append(arguments(value))
            swapped = copy.deepcopy(value)
            swapped["txHeightM"], swapped["rxHeightM"] = value["rxHeightM"], value["txHeightM"]
            jobs.append(arguments(swapped))
    examples_start = len(jobs)
    mappings = (("hTxM", "h_tx__meter"), ("hRxM", "h_rx__meter"), ("fMHz", "f__mhz"),
                ("pTxW", "P_tx__watt"), ("nS", "N_s"), ("dKm", "d__km"), ("epsilon", "epsilon"), ("sigma", "sigma"), ("pol", "pol"))
    for row_number, (row, example) in enumerate(zip(published, data["ntiaExamples"]), 2):
        need(example["row"] == row_number and example["rc"] == int(row["rtn"]), "Строка/код опубликованного примера")
        for key, column in mappings:
            need(number(example[key]) and example[key] == float(row[column]), "Вход примера отличается от публикации")
        jobs.append([float(row[column]) for _, column in mappings[:-1]]+[int(row["pol"])])
        if example["rc"] == 0:
            for key, column in (("aBtlDb", "A_btl__db"), ("eDbuVm", "E_dBuVm"), ("pRxDbm", "P_rx__dbm"), ("method", "method")):
                need(number(example[key]) and example[key] == float(row[column]), "Ожидание примера отличается от публикации")
    commands = "\n".join(str(i)+" "+" ".join(format(v, ".17g") for v in values) for i, values in enumerate(jobs))+"\n"
    process = subprocess.run([str(args.exe.resolve())], input=commands, text=True, capture_output=True, timeout=30, check=True)
    native = [json.loads(line) for line in process.stdout.splitlines()]
    need(len(native) == len(jobs) and [r["id"] for r in native] == list(range(len(jobs))), "Неполный ответ программы NTIA")
    worst, worst_example, numeric_failures = 0.0, 0.0, []
    code_counts = {}
    for case in cases:
        value = case["input"]
        code = rejection(value)
        if code is None:
            index = positions[case["id"]]
            actual = result(value, native[index])
            compare(result(value, native[index+1]), actual)
        else:
            actual = {"code":code}
        try:
            worst = max(worst, compare(actual, case["expect"]))
        except AssertionError as error:
            raise AssertionError(case["id"]+": "+str(error)) from error
        code_counts[actual["code"]] = code_counts.get(actual["code"], 0)+1
        if actual["code"] == "OK":
            invariant = actual["basicLossDb"]+actual["fieldDbuVm"]-20*math.log10(value["frequencyMHz"])
            need(abs(invariant-141.98599025116266) <= 1e-9, "Нарушена нормировка поля")
        elif actual["code"] == "NUMERIC_FAILURE":
            numeric_failures.append(case["id"])
    for offset, example in enumerate(data["ntiaExamples"]):
        got = native[examples_start+offset]
        need(not got["exception"] and got["rc"] == example["rc"], "Код NTIA example")
        if example["rc"] == 0:
            need(got["method"] == example["method"], "Метод NTIA example")
            for key, expected_key in (("loss", "aBtlDb"), ("field", "eDbuVm"), ("received", "pRxDbm")):
                delta = abs(got[key]-example[expected_key])
                need(delta <= .05, "Число NTIA example")
                worst_example = max(worst_example, delta)
    control = cases[0]["expect"]
    compare(control, control)
    for key in ("code", "method", "provenance", "basicLossDb", "fieldDbuVm", "kr", "heightToWavelength"):
        bad = copy.deepcopy(control)
        bad[key] = "WRONG" if isinstance(bad[key], str) else bad[key]+1
        try:
            compare(control, bad)
        except AssertionError:
            continue
        raise AssertionError("Проверка пропустила порчу поля: "+key)
    need(corpus_path.read_bytes() == corpus_raw, "Корпус изменился во время проверки")
    need(sha(req_path.read_bytes().replace(b"\r\n", b"\n")) == REQ_SHA, "REQ изменился во время проверки")
    report = {"status":"GW_BLIND_REFERENCE_PASS", "scope":"Контракт GW и паритет отдельной сборке NTIA; не физическая/игровая приёмка",
              "sourceZipSha256":SOURCE_SHA, "sourceCommit":commit, "sourceFilesChecked":checked_sources,
              "testDataZipSha256":TEST_DATA_SHA, "csvSha256":sha(csv_raw), "requirementsLfSha256":REQ_SHA,
              "corpusSha256":sha(corpus_raw), "referenceExeSha256":sha(args.exe.read_bytes()),
              "compilerFlags":["MSVC /EHs", "/O2", "/std:c++17"],
              "driverSha256":sha((ROOT/"docs/evidence/receipts/ground-wave-reference-codex.cpp").read_bytes()),
              "checkerSha256":sha(Path(__file__).read_bytes()), "cases":len(cases), "codes":code_counts,
              "nativeCalls":len(jobs), "numericFailures":numeric_failures, "ntiaExamples":len(published),
              "maxAbsoluteErrorDb":worst, "maxPublishedExampleErrorDb":worst_example, "negativeComparatorControls":7,
              "heightSwap":"PASS", "normalizationIdentity":"PASS"}
    args.report.write_text(json.dumps(report, ensure_ascii=False, indent=2)+"\n", encoding="utf-8")
    print(f"GW_BLIND_REFERENCE_PASS cases={len(cases)} examples=95 delta={worst:.3g}dB published={worst_example:.5g}dB")


if __name__ == "__main__":
    main()
