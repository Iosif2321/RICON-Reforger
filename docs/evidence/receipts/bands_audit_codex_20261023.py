"""Независимые проверки областей существующих oracle, без изменения моделей."""

import copy
import hashlib
import importlib
import json
import math
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "oracle"))
rf = importlib.import_module("ricon_oracle")
l2 = importlib.import_module("l2_terrain_oracle")
veg = importlib.import_module("vegetation_oracle")
water = importlib.import_module("path_observation_oracle")


def vectors(name):
    return json.loads((ROOT / "oracle/vectors" / (name + ".json")).read_text(encoding="utf-8"))["cases"]


def adjacent32(value, delta):
    bits = struct.unpack(">I", struct.pack(">f", value))[0]
    return struct.unpack(">f", struct.pack(">I", bits + delta))[0]


def run():
    frequencies = sorted(set([.03, .1, .15, .3, 1, 1.7, 2, 3, 10, 20, 25, 30, 60,
                              76, 88, 100, 105.9, 150, 230, 250, 300, 400, 466.475,
                              1000, 3000] + [adjacent32(f, d) for f in (.3, 30, 250) for d in (-1, 1)]))
    terrain = next(c["input"] for c in vectors("l2_terrain_v1") if c["id"] == "S01-smooth-5km-low")
    cases = {c["id"]: c["input"] for c in vectors("vegetation_v1")}
    rows = []
    for f in frequencies:
        ti = copy.deepcopy(terrain)
        ti["frequencyMHz"] = f
        vr = []
        # Реальные TEST_ONLY каталоги: умеренный S1, тропический S2, линейная S1.
        for cid, material in [("V03", "forest.mixed_spb"), ("V08", "forest.tropical"), ("V03", "woodland.rice71")]:
            vi = copy.deepcopy(cases[cid])
            vi["observation"]["frequencyMHz"] = f
            vi["observation"]["segments"][0]["materialId"] = material
            result = veg.vegetation_layer(vi)
            vr.append({"fixture": cid, "material": material, "code": result["code"],
                       "intervals": [{k: r[k] for k in ("scenario", "primaryModel", "primaryStatus", "scenarioClosed")}
                                     for r in result.get("intervals", [])]})
        er, ei = water.water_permittivity(f / 1000, 20, 0)
        rows.append({"frequencyMHz": f, "wavelengthM": rf.SPEED_OF_LIGHT_MPS / (f * 1e6),
                     "fspl": [{"distanceM": d, **rf.free_space_loss(d, f)} for d in (1, 100, 1000, 10000)],
                     "noiseRuralTestOnly": rf.system_noise(25000, 6, "rural", f),
                     "l2Code": l2.l2(ti)["code"], "vegetation": vr,
                     "waterBulkDbPerM_T20_S0_TEST_ONLY": water.attenuation_db_per_m(f, water.refractive_index(er, ei))})

    # Проверяем существенные утверждения аудита, включая обе стороны binary32 границ.
    byf = {r["frequencyMHz"]: r for r in rows}
    assert byf[20]["l2Code"] == "OUT_OF_DOMAIN_FREQUENCY"
    assert byf[adjacent32(30, -1)]["l2Code"] == "OUT_OF_DOMAIN_FREQUENCY"
    assert byf[30]["l2Code"] == byf[adjacent32(30, 1)]["l2Code"] == "OK"
    for edge, inside, outside in [(.3, 1, -1), (250, -1, 1)]:
        assert byf[edge]["noiseRuralTestOnly"]["noiseModelInDomain"]
        assert byf[adjacent32(edge, inside)]["noiseRuralTestOnly"]["noiseModelInDomain"]
        assert not byf[adjacent32(edge, outside)]["noiseRuralTestOnly"]["noiseModelInDomain"]
    floor = 20 * math.log10(4 * math.pi)
    assert all(math.isclose(x["lossDb"], floor, abs_tol=1e-12) for x in byf[.03]["fspl"][:3])
    assert math.isclose(byf[30]["fspl"][3]["lossDb"] - byf[30]["fspl"][2]["lossDb"], 20, abs_tol=1e-12)
    assert all(v["code"] == "NO_MODEL_FOR_SCENARIO" for v in byf[3]["vegetation"])
    assert all(math.isfinite(r["waterBulkDbPerM_T20_S0_TEST_ONLY"]) for r in rows)

    # Другая алгебраическая форма проверки таблицы партнёра, не независимая физическая модель.
    k_rows = []
    for name, eps, sigma in [("dry", 4, .001), ("medium", 15, .01), ("wet", 30, .02),
                             ("sea", 70, 5), ("fresh", 80, .01)]:
        for f in (.1, .3, 1, 3, 10, 20, 30, 76):
            eta = complex(eps, -18000 * sigma / f)
            k_value = .36 * abs(eta) / ((8500 * f) ** (1 / 3) * math.sqrt(abs(eta - 1)))
            config = {"permittivity": eps, "conductivitySm": sigma, "polarization": "VERTICAL"}
            assert math.isclose(k_value, l2.surface_k(8500, f, config), rel_tol=1e-14)
            k_rows.append({"surfaceTEST_ONLY": name, "frequencyMHz": f, "kVertical": k_value})

    tracked = [Path(__file__).relative_to(ROOT)] + [Path("oracle") / (m.__name__ + ".py") for m in (rf, l2, veg, water)]
    tracked += [Path("oracle/vectors") / (n + ".json") for n in ("l2_terrain_v1", "vegetation_v1")]
    return {"schema": "ricon.audit.bands.codex.v1", "level": "HOST_DIAGNOSTIC_NOT_CALIBRATION",
            "note": "Вода T=20 C, S=0; шум B=25 кГц, NF=6 дБ; каталоги и рельеф — TEST_ONLY. Это не профили устройств.",
            "sha256": {str(p).replace("\\", "/"): hashlib.sha256((ROOT / p).read_bytes()).hexdigest() for p in tracked},
            "rows": rows, "kChecksTEST_ONLY": k_rows, "selfCheck": "PASS"}


if __name__ == "__main__":
    result = run()
    target = Path(__file__).with_suffix(".json")
    target.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"PASS: {len(result['rows'])} частот и {len(result['kChecksTEST_ONLY'])} проверок K; binary32 границы, FSPL, шум, L2, растительность, объёмные свойства воды. {target.name}")
