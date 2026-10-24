"""Сборка корпуса REQ-ground-wave v0.1; --check проверяет корпус без записи, в том числе при несовпадении.

Ожидания — из oracle/ground_wave_oracle.py (порт LFMF v1.1), сверенного с неизменённой сборкой NTIA LFMF v1.1
(receipt docs/evidence/receipts/ground-wave-port-vs-ntia-claude.json). Примеры NTIA — точная копия
LFMF_Examples.csv (NTIA/LFMF-test-data, US public domain) в oracle/data/lfmf_examples_ntia_v1.csv."""
import argparse
import copy
import csv
import hashlib
import json
import math

import ground_wave_oracle as o
from path_observation_oracle import water_permittivity

TARGET = o.ROOT / "oracle" / "vectors" / "ground_wave_v1.json"
EXAMPLES = o.ROOT / "oracle" / "data" / "lfmf_examples_ntia_v1.csv"
EXAMPLES_SHA = "c13ef54eae96b8196047d5ea7fb58ce1d34b2f32cb86a1ad85dedfd8ec2d0ff6"
REVISION = 1


def ground(eps, sigma, provenance="TEST_ONLY"):
    return {"epsilonReal": eps, "conductivitySm": sigma, "provenance": provenance}


def water(f_mhz, salinity):
    """Края профиля воды WL02 (P.527, T = 15 °C, S = 0 и 35 ‰) — как в E-ground-wave-candidates.md; TEST_ONLY."""
    er, ei = water_permittivity(f_mhz / 1000.0, 15.0, salinity)
    return ground(er, ei * f_mhz / 18000.0)


GROUNDS = (("суша ε=15 σ=0.005", lambda f: ground(15.0, 0.005)),
           ("вода S0 P.527 15 °C", lambda f: water(f, 0.0)),
           ("вода S35 P.527 15 °C", lambda f: water(f, 35.0)),
           ("сухой грунт ε=4 σ=0.001", lambda f: ground(4.0, 0.001)),
           ("влажный грунт ε=30 σ=0.02", lambda f: ground(30.0, 0.02)))


def baseline():
    return {"frequencyMHz": 25.0, "distanceKm": 10.0, "txHeightM": 2.0, "rxHeightM": 2.0, "polarization": "V",
            "surfaceRefractivityN": 301.0, "ground": ground(15.0, 0.005)}


def cases():
    rows = []

    def add(note, value, expected_code=None, json_only=False):
        result = o.evaluate(value)
        if expected_code is not None:
            assert result["code"] == expected_code, (note, result, expected_code)
        rows.append({"id": f"GW{len(rows) + 1:03d}", "note": note, "jsonOnly": json_only,
                     "provenance": "HAND_DERIVED" if result["code"] != "OK" else "CROSS_CHECKED",
                     "input": copy.deepcopy(value), "expect": result})

    def change(note, expected_code=None, json_only=False, **fields):
        value = baseline()
        for key, val in fields.items():
            if key == "ground":
                value["ground"] = val
            elif key in ("epsilonReal", "conductivitySm", "provenance"):
                value["ground"][key] = val
            else:
                value[key] = val
        add(note, value, expected_code, json_only)

    # 1. Основная сетка 20–30 МГц: расстояния с обеих сторон порога метода 80/f^(1/3), пять грунтов, h = 2/2 м.
    for f in (20.0, 25.0, 30.0):
        d_test = 80 / math.cbrt(f)
        for gname, gfun in GROUNDS:
            for d in (0.001, 0.1, 1.0, 5.0, 10.0, d_test * (1 - 1e-9), d_test * (1 + 1e-9), 50.0, 300.0, 10000.0):
                change(f"сетка f={f} d={d:.6g} {gname}", "OK", frequencyMHz=f, distanceKm=d, ground=gfun(f))
    # 2. Высоты: 0, 1, 10, 50 м, асимметрия и перестановка TX/RX (симметрия GW05).
    for f in (20.0, 30.0):
        for d in (1.0, 30.0):
            for h_tx, h_rx in ((0.0, 0.0), (1.0, 1.0), (10.0, 2.0), (2.0, 10.0), (50.0, 2.0), (2.0, 50.0), (50.0, 50.0)):
                for gname, gfun in (GROUNDS[0], GROUNDS[2]):
                    change(f"высоты {h_tx}/{h_rx} f={f} d={d} {gname}", "OK", frequencyMHz=f, distanceKm=d,
                           txHeightM=h_tx, rxHeightM=h_rx, ground=gfun(f))
    # 3. Nₛ: границы и середина; эффективный радиус aₑ.
    for n_s in (250.0, 301.0, 400.0):
        for d in (30.0, 300.0):
            change(f"Ns={n_s} d={d}", "OK", surfaceRefractivityN=n_s, distanceKm=d)
    # 4. Границы области (включены) и стык 30 МГц.
    change("f = 0.01 МГц, нижняя граница", "OK", frequencyMHz=0.01, distanceKm=100.0)
    change("f = 30 МГц ровно", "OK", frequencyMHz=30.0)
    change("f выше 30 на 1 ulp", "OUT_OF_MODEL_DOMAIN", frequencyMHz=math.nextafter(30.0, math.inf))
    change("f ниже 0.01 на 1 ulp", "OUT_OF_MODEL_DOMAIN", frequencyMHz=math.nextafter(0.01, 0.0))
    change("f = 31 МГц", "OUT_OF_MODEL_DOMAIN", frequencyMHz=31.0)
    change("txHeightM = 50 ровно", "OK", txHeightM=50.0)
    change("txHeightM выше 50 на 1 ulp", "OUT_OF_MODEL_DOMAIN", txHeightM=math.nextafter(50.0, math.inf))
    change("rxHeightM выше 50 на 1 ulp", "OUT_OF_MODEL_DOMAIN", rxHeightM=math.nextafter(50.0, math.inf))
    change("distanceKm = 0.001 ровно", "OK", distanceKm=0.001)
    change("distanceKm ниже 0.001", "OUT_OF_MODEL_DOMAIN", distanceKm=math.nextafter(0.001, 0.0))
    change("distanceKm = 10000 ровно", "OK", distanceKm=10000.0)
    change("distanceKm выше 10000", "OUT_OF_MODEL_DOMAIN", distanceKm=math.nextafter(10000.0, math.inf))
    change("Ns ниже 250", "OUT_OF_MODEL_DOMAIN", surfaceRefractivityN=math.nextafter(250.0, 0.0))
    change("Ns выше 400", "OUT_OF_MODEL_DOMAIN", surfaceRefractivityN=math.nextafter(400.0, math.inf))
    change("ε′ = 1 ровно", "OK", epsilonReal=1.0)
    change("σ малая положительная 1e-9", "OK", conductivitySm=1e-9)
    change("σ = 0: идеальный диэлектрик вне области LFMF", "OUT_OF_MODEL_DOMAIN", conductivitySm=0.0)
    change("H-поляризация: P.368 даёт вертикальную составляющую", "OUT_OF_MODEL_DOMAIN", polarization="H")
    # 5. Физически недопустимые значения.
    change("f = 0", "INVALID_INPUT", frequencyMHz=0.0)
    change("f < 0", "INVALID_INPUT", frequencyMHz=-25.0)
    change("d = 0", "INVALID_INPUT", distanceKm=0.0)
    change("txHeightM < 0", "INVALID_INPUT", txHeightM=-1e-9)
    change("rxHeightM < 0", "INVALID_INPUT", rxHeightM=-1.0)
    change("Ns < 0", "INVALID_INPUT", surfaceRefractivityN=-1.0)
    change("ε′ < 1", "INVALID_INPUT", epsilonReal=math.nextafter(1.0, 0.0))
    change("σ < 0", "INVALID_INPUT", conductivitySm=-0.001)
    # 6. Порядок отказов GW03.
    change("порядок: INVALID (f=0) раньше OUT_OF_MODEL (h=60)", "INVALID_INPUT", frequencyMHz=0.0, txHeightM=60.0)
    change("порядок: INVALID (ε′<1) раньше OUT_OF_MODEL (f=40)", "INVALID_INPUT", epsilonReal=0.5, frequencyMHz=40.0)
    change("порядок: tx>50 и f=40 — оба OUT_OF_MODEL", "OUT_OF_MODEL_DOMAIN", txHeightM=60.0, frequencyMHz=40.0)
    change("порядок: σ=0 и H — OUT_OF_MODEL", "OUT_OF_MODEL_DOMAIN", conductivitySm=0.0, polarization="H")
    # 7. Происхождение копируется без изменения.
    for provenance in ("SOURCE_DOMAIN", "ASSUMED_CONFIG"):
        change(f"provenance {provenance} копируется", "OK", provenance=provenance)
    change("целые числа JSON допустимы", "OK", frequencyMHz=25, distanceKm=10, txHeightM=2, rxHeightM=2,
           surfaceRefractivityN=301)
    # 8. Численные границы (codex #4088): конечные, но крайние ε′/σ. Сборка NTIA здесь возвращает ±inf/NaN или
    # прерывается исключением C++ — по GW03 это NUMERIC_FAILURE; огромная σ без переполнения остаётся OK.
    # Совпадение границы отказа с неизменённой сборкой — receipts/ground-wave-extremes-claude.py.
    numeric = (("σ = 1e308, f = 0.01 МГц, d = 1 км: поле не конечно (codex #4088)", 0.01, 1.0, 15.0, 1e308, "NUMERIC_FAILURE"),
               ("ε′ = 1e308, f = 30 МГц, d = 1 км: поле не конечно (codex #4088)", 30.0, 1.0, 1e308, 1.0, "NUMERIC_FAILURE"),
               ("ε′ = 1e308, f = 1 МГц, d = 100 км: исключение C++ в ряде вычетов", 1.0, 100.0, 1e308, 1.0, "NUMERIC_FAILURE"),
               ("σ = 1e300, f = 1 МГц, d = 100 км: исключение C++ в ряде вычетов", 1.0, 100.0, 15.0, 1e300, "NUMERIC_FAILURE"),
               ("σ = 1e20, f = 0.01 МГц, d = 1 м: огромная σ без переполнения", 0.01, 0.001, 15.0, 1e20, "OK"),
               ("σ = 1e9, f = 25 МГц, d = 1 км: огромная σ без переполнения", 25.0, 1.0, 15.0, 1e9, "OK"))
    for note, f, d, eps, sigma, code in numeric:
        change(note, code, frequencyMHz=f, distanceKm=d, txHeightM=0.0, rxHeightM=0.0, ground=ground(eps, sigma))
    # 9. JSON-only: типы и поля.
    change("bool вместо числа", "INVALID_INPUT", True, frequencyMHz=True)
    change("строка вместо числа", "INVALID_INPUT", True, distanceKm="10")
    change("null вместо числа", "INVALID_INPUT", True, txHeightM=None)
    change("целое JSON вне binary64 (codex #4118)", "INVALID_INPUT", True, frequencyMHz=10 ** 400)
    change("отрицательное целое JSON вне binary64", "INVALID_INPUT", True, epsilonReal=-(10 ** 400))
    change("поляризация в нижнем регистре", "INVALID_INPUT", True, polarization="v")
    change("неизвестная поляризация", "INVALID_INPUT", True, polarization="CIRCULAR")
    change("неизвестный provenance", "INVALID_INPUT", True, provenance="MEASURED")
    change("ground не объект", "INVALID_INPUT", True, ground=[15.0, 0.005])
    value = baseline()
    value["extra"] = 1
    add("лишнее поле входа", value, "INVALID_INPUT", True)
    value = baseline()
    del value["surfaceRefractivityN"]
    add("нет поля surfaceRefractivityN", value, "INVALID_INPUT", True)
    value = baseline()
    value["ground"]["permeability"] = 1.0
    add("лишнее поле ground", value, "INVALID_INPUT", True)
    value = baseline()
    del value["ground"]["provenance"]
    add("нет provenance в ground", value, "INVALID_INPUT", True)
    add("корень не объект", [baseline()], "INVALID_INPUT", True)
    return rows


def ntia_examples():
    raw = EXAMPLES.read_bytes()
    if hashlib.sha256(raw).hexdigest() != EXAMPLES_SHA:
        raise SystemExit("LFMF_Examples.csv изменён")
    out = []
    for n, r in enumerate(csv.DictReader(raw.decode("utf-8").splitlines()), start=2):
        row = {"row": n, "hTxM": float(r["h_tx__meter"]), "hRxM": float(r["h_rx__meter"]), "fMHz": float(r["f__mhz"]),
               "pTxW": float(r["P_tx__watt"]), "nS": float(r["N_s"]), "dKm": float(r["d__km"]),
               "epsilon": float(r["epsilon"]), "sigma": float(r["sigma"]), "pol": int(r["pol"]), "rc": int(r["rtn"])}
        if row["rc"] == 0:
            row.update(aBtlDb=float(r["A_btl__db"]), eDbuVm=float(r["E_dBuVm"]), pRxDbm=float(r["P_rx__dbm"]),
                       method=int(r["method"]))
        out.append(row)
    return out


def build():
    return {"schema": "ricon.oracle.ground_wave", "version": 1, "revision": REVISION,
            "source": "docs/req/REQ-ground-wave.md", "requirementsLfSha256": o.lf_sha(o.REQ),
            "note": ("Ожидания OK — порт LFMF v1.1 (CROSS_CHECKED с неизменённой сборкой NTIA, |Δ| ≤ 2e-9 дБ); отказы — "
                     "HAND_DERIVED по GW03. ntiaExamples — опубликованные NTIA значения (1 знак), проверяются через lfmf() "
                     "с допуском 0.05 дБ. Грунты TEST_ONLY, если не указано иное."),
            "cases": cases(), "ntiaExamples": ntia_examples()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    data = build()
    text = json.dumps(data, ensure_ascii=False, indent=2, allow_nan=False) + "\n"
    if args.check:
        if TARGET.read_text(encoding="utf-8") != text:
            raise SystemExit("Корпус GW отличается; --check не пишет файлы")
    else:
        TARGET.write_text(text, encoding="utf-8", newline="\n")
    ok = sum(c["expect"]["code"] == "OK" for c in data["cases"])
    print(f"PASS: {len(data['cases'])} случаев GW (OK {ok}, JSON-only {sum(c['jsonOnly'] for c in data['cases'])}); "
          f"примеров NTIA {len(data['ntiaExamples'])}")


if __name__ == "__main__":
    main()
