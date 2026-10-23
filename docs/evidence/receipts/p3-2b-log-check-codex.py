"""Независимая проверка чисел P3.2b в native-логе; процесс подтверждается отдельным receipt."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import struct
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "oracle"))
from material_slab_oracle import evaluate as slab_evaluate

CORPUS_SHA = "36f854eaa5cc2b9e1dc10d8ee6e158f4db55f2b4ae6bee7ccfcc38042135ebc2"
REQ_SHA = "2385e37bcb24f8183eaee84e477bf6211a17f557c808d7a011998dbdfb9597dd"
CATALOG_SHA = "9d5b720157407d2562b5f0f8756052f1cacd1f3d43d3688f34b26c1a679649e9"
P = "[RICON_MPROF] "
CHAIN = "[RICON_MPROF_CHAIN] "
POWER = "[RICON_MPROF_POWER] "
SUMMARY = "cases=223 match=223 immutable=223 controls=5 extras=7 hostOnly=13 chains=119 maxRelativeError="


def need(condition, reason):
    if not condition:
        raise ValueError(reason)


def f32(value):
    return struct.unpack("<f", struct.pack("<f", value))[0]


def bits(value):
    return str(struct.unpack("<I", struct.pack("<f", value))[0])


def decode(value):
    need(re.fullmatch(r"0|[1-9][0-9]*", value) is not None, "Некорректный битовый образ")
    raw = int(value)
    need(raw < 0x7f800000, "Отрицательное или неконечное число")
    return struct.unpack("<f", struct.pack("<I", raw))[0]


def close(actual, expected):
    if expected == 0:
        return actual == 0
    return actual > 0 and abs(actual - expected) / expected <= 2e-5


def number(value):
    x = float(value)
    need(math.isfinite(x), "Неконечное число")
    return x


def fields(row, prefix, names):
    need(row.startswith(prefix), "Неизвестный маркер")
    pairs = row[len(prefix):].split()
    result = {}
    for token in pairs:
        key, sep, value = token.partition("=")
        need(sep and value and key not in result, "Неверное/повторное поле")
        result[key] = value
    need(set(result) == set(names.split()), "Неверные поля строки")
    return result


def inputs():
    paths = (("oracle/vectors/material_profile_v1.json", CORPUS_SHA),
             ("docs/req/REQ-material-profile.md", REQ_SHA),
             ("oracle/data/material_profiles_p2040_v1.json", CATALOG_SHA))
    for name, expected in paths:
        raw = (ROOT / name).read_bytes().replace(b"\r\n", b"\n")
        need(hashlib.sha256(raw).hexdigest() == expected, "Изменён frozen-файл: " + name)
    corpus = json.loads((ROOT / paths[0][0]).read_text(encoding="utf-8"))
    need(corpus["revision"] == 2 and corpus["requirementsLfSha256"] == REQ_SHA
         and corpus["catalogLfSha256"] == CATALOG_SHA, "Неверная привязка корпуса")
    need([c["id"] for c in corpus["cases"]] == [f"MP{i:03}" for i in range(1, 237)], "Неверный набор случаев")
    native = {c["id"]: c["expect"] for c in corpus["cases"] if c["native"]}
    need(len(native) == 223, "Неверный native-набор")
    return native


def chain_input(row, material, observed_bits):
    data = fields(row, CHAIN, "fBits dBits epsBits sigmaBits slab loss shifted ok")
    f, d, epsilon, sigma = [decode(data[k]) for k in ("fBits", "dBits", "epsBits", "sigmaBits")]
    need(data["slab"] == "OK" and data["shifted"] == "OUT_OF_MODEL_DOMAIN" and data["ok"] == "1", "Отказ цепи")
    need((data["epsBits"], data["sigmaBits"]) == observed_bits, "Материал цепи отличается от результата случая")
    need(f == material["minMHz"] == material["maxMHz"], "Чужая частота цепи")
    need(d == f32(.1 if f <= 2000 else 200 / f), "Неверная толщина цепи")
    need(close(epsilon, material["epsilonReal"]) and close(sigma, material["conductivitySm"]), "Чужой материал цепи")
    value = {"frequencyMHz": f, "thicknessM": d, "cosIncidence": 1, "polarization": "TE",
             "material": {"epsilonReal": epsilon, "conductivitySm": sigma, "minMHz": f, "maxMHz": f,
                          "provenance": material["provenance"]}}
    expected = slab_evaluate(value)
    loss = number(data["loss"])
    need(expected["code"] == "OK" and loss >= 0, "Цепь вне области slab")
    need(abs(loss - expected["lossDb"]) <= max(.05, 2e-6 * abs(expected["lossDb"])), "Потери цепи вне MS08")
    return abs(loss - expected["lossDb"])


def check(text, expected):
    need(not re.search(r"SCRIPT\s*\(E\)|\[RICON[^\n]*\bFAIL\b", text), "Ошибка движка/тестов")
    rows = [line[line.index("[RICON_MPROF"):].strip() for line in text.splitlines() if "[RICON_MPROF" in line]
    chain_materials = [exp["material"] for exp in expected.values()
                       if exp["code"] == "OK" and exp["material"]["conductivitySm"] > 0]
    need(len(chain_materials) == 119, "Неверный набор ожидаемых цепей")
    need(len(rows) == len(expected) + len(chain_materials) + 7, "Лишние/пропущенные записи")
    need(rows[0] == P + "source=" + CORPUS_SHA and rows[-1] == P + "done: PASS", "Неверный заголовок/итог")
    maximum = 0
    observed_bits = []
    for row, (ident, exp) in zip(rows[1:224], expected.items()):
        data = fields(row, P, "id code epsBits sigmaBits minBits maxBits provenance match immutable")
        need(data["id"] == ident and data["code"] == exp["code"], "Неверный id/код/порядок: " + ident)
        need(data["match"] == data["immutable"] == "1", "Отказ сравнения/неизменности: " + ident)
        if exp["code"] != "OK":
            need(all(data[k] == "NONE" for k in ("epsBits", "sigmaBits", "minBits", "maxBits", "provenance")),
                 "Материал при отказе: " + ident)
            continue
        material = exp["material"]
        if material["conductivitySm"] > 0:
            observed_bits.append((data["epsBits"], data["sigmaBits"]))
        need(data["provenance"] == material["provenance"], "Происхождение: " + ident)
        for bit_key, value_key in (("epsBits", "epsilonReal"), ("sigmaBits", "conductivitySm"),
                                   ("minBits", "minMHz"), ("maxBits", "maxMHz")):
            actual, reference = decode(data[bit_key]), material[value_key]
            if bit_key in ("minBits", "maxBits"):
                need(actual == reference, "Материал используется на чужой частоте: " + ident)
            else:
                need(close(actual, reference), "Параметр вне допуска: " + ident + ": " + bit_key)
                if reference:
                    maximum = max(maximum, abs(actual - reference) / reference)
    extras = rows[224:-2]
    chains = [chain_input(row, m, b) for row, m, b in zip(extras[:119], chain_materials, observed_bits)]
    powers = [fields(row, POWER, "id code sigmaBits ok") for row in extras[119:]]
    need([p["id"] for p in powers] == ["1", "2", "3", "4"], "Неверные Power-контрпримеры")
    for p in powers:
        need(p["ok"] == "1", "Отказ Power-контрпримера")
        if p["id"] in ("3", "4"):
            need(p["code"] == "NUMERIC_DOMAIN_LIMIT" and p["sigmaBits"] == "NONE", "Неверный край области")
        else:
            need(p["code"] == "OK", "Неверный Power-код")
            ref = f32(1e-45) if p["id"] == "1" else f32(f32(1e38) * .001 ** 15)
            need(close(decode(p["sigmaBits"]), ref), "Неверный Power-параметр")
    need(len(chains) + len(powers) == len(extras), "Неизвестная дополнительная строка")
    need(rows[-2].startswith(P + SUMMARY), "Неверные счётчики")
    printed = number(rows[-2][len(P + SUMMARY):])
    need(0 <= printed <= 2e-5 and abs(printed - maximum) <= 5e-8, "Неверная максимальная ошибка")
    return {"cases": 223, "hostOnly": 13, "nativeControls": 5, "nativeExtras": 7,
            "maxRelativeErrorFromBits": maximum, "maxRelativeErrorPrinted": printed,
            "chains": len(chains), "maxChainLossErrorDb": max(chains), "powerRegressions": 4}


def self_test(expected):
    rows = [P + "source=" + CORPUS_SHA]
    for ident, exp in expected.items():
        m = exp.get("material")
        values = [bits(m[k]) if m else "NONE" for k in ("epsilonReal", "conductivitySm", "minMHz", "maxMHz")]
        rows.append(f"{P}id={ident} code={exp['code']} epsBits={values[0]} sigmaBits={values[1]} "
                    f"minBits={values[2]} maxBits={values[3]} provenance={m['provenance'] if m else 'NONE'} match=1 immutable=1")
    for exp in expected.values():
        if exp["code"] != "OK" or exp["material"]["conductivitySm"] == 0:
            continue
        m = exp["material"]
        f, epsilon, sigma = m["minMHz"], m["epsilonReal"], m["conductivitySm"]
        d = f32(.1 if f <= 2000 else 200 / f)
        value = {"frequencyMHz": f, "thicknessM": d, "cosIncidence": 1, "polarization": "TE", "material": {
            "epsilonReal": epsilon, "conductivitySm": sigma, "minMHz": f, "maxMHz": f, "provenance": m["provenance"]}}
        loss = slab_evaluate(value)["lossDb"]
        rows.append(f"{CHAIN}fBits={bits(f)} dBits={bits(d)} epsBits={bits(epsilon)} sigmaBits={bits(sigma)} "
                    f"slab=OK loss={loss} shifted=OUT_OF_MODEL_DOMAIN ok=1")
    rows += [POWER + "id=1 code=OK sigmaBits=1 ok=1",
             POWER + "id=2 code=OK sigmaBits=869711764 ok=1",
             POWER + "id=3 code=NUMERIC_DOMAIN_LIMIT sigmaBits=NONE ok=1",
             POWER + "id=4 code=NUMERIC_DOMAIN_LIMIT sigmaBits=NONE ok=1", P + SUMMARY + "0", P + "done: PASS"]
    good = "\n".join(rows)
    check(good, expected)
    check(good.replace("sigmaBits=869711764 ok=1", "sigmaBits=869711765 ok=1"), expected)
    target = next(row for row in rows if row.startswith(P + "id=MP006 "))
    mutations = [good.replace(CORPUS_SHA, "0" * 64), good.replace(target, ""), good + "\n" + target,
                 good.replace("id=MP006", "id=MP222"), good.replace("controls=5", "controls=4"),
                 good.replace("done: PASS", "done: FAIL"), good + "\nSCRIPT (E): намеренная ошибка",
                 good + "\n[RICON_TEST] FAIL", good.replace("maxRelativeError=0", "maxRelativeError=0.001"),
                 good.replace("extras=7", "extras=6"), good.replace("shifted=OUT_OF_MODEL_DOMAIN", "shifted=OK"),
                 good.replace("[RICON_MPROF_POWER] id=1 code=OK sigmaBits=1", "[RICON_MPROF_POWER] id=1 code=OK sigmaBits=0"),
                 good.replace(rows[-3], rows[-3].replace("NUMERIC_DOMAIN_LIMIT", "NUMERIC_FAILURE")),
                 good.replace(rows[-4], rows[-4].replace("NUMERIC_DOMAIN_LIMIT", "NUMERIC_FAILURE")),
                 good.replace(rows[-3], ""),
                 good.replace("chains=119", "chains=118"), good.replace(rows[224], "", 1),
                 good.replace(rows[224], re.sub(r"loss=\S+", "loss=100000000", rows[224]), 1)]
    for field in ("fBits", "dBits", "epsBits", "sigmaBits"):
        mutations.append(good.replace(rows[224], re.sub(rf"\b{field}=\S+", field + "=0", rows[224]), 1))
    sigma = int(fields(rows[224], CHAIN, "fBits dBits epsBits sigmaBits slab loss shifted ok")["sigmaBits"])
    mutations.append(good.replace(rows[224], rows[224].replace("sigmaBits=" + str(sigma), "sigmaBits=" + str(sigma + 1)), 1))
    for field, value in (("code", "NUMERIC_FAILURE"), ("epsBits", "2139095040"), ("sigmaBits", "0"),
                         ("epsBits", "-1"), ("epsBits", "nan"), ("minBits", "0"), ("maxBits", "0"),
                         ("provenance", "TEST_ONLY"), ("match", "0"), ("immutable", "0")):
        mutations.append(good.replace(target, re.sub(rf"\b{field}=\S+", field + "=" + value, target)))
    for i, bad in enumerate(mutations):
        try:
            check(bad, expected)
        except (ValueError, KeyError):
            continue
        raise AssertionError("Не замечена порча " + str(i))
    return len(mutations)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--log", type=Path)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    expected = inputs()
    report = {"validatorSelfTest": "PASS", "negativeControls": self_test(expected)}
    if args.log:
        raw = args.log.read_bytes()
        report.update(check(raw.decode("utf-8", errors="replace"), expected))
        report.update(status="NATIVE_LOG_PASS", logSha256=hashlib.sha256(raw).hexdigest(), processReceiptChecked=False)
    if args.report:
        args.report.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps(report, ensure_ascii=False, indent=2))
