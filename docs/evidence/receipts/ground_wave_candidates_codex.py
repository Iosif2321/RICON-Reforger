"""Независимый replay экспорта кандидатов; исходный NTIA exe, не физическая приёмка."""
import argparse
import hashlib
import importlib
import json
import math
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "oracle"))
l2 = importlib.import_module("l2_terrain_oracle")
water = importlib.import_module("path_observation_oracle")


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def close(actual, expected, tolerance=2e-7):
    assert math.isfinite(actual) and math.isfinite(expected)
    assert abs(actual - expected) <= tolerance, (actual, expected, tolerance)
    return abs(actual - expected)


def check(exe, source, target):
    data = json.loads(source.read_text(encoding="utf-8"))
    assert data["schema"] == "ground-wave-candidates/1"
    assert sha(exe) == data["exeSha256"]
    rows = data["cells"]
    assert len(rows) == 1296
    keys = {(r["fMHz"], r["ground"], r["pol"], r["h1"], r["h2"], r["dKm"]) for r in rows}
    expected_grid = {(f, g, p, h1, h2, d)
                     for f in (10, 15, 20, 25, 30, 40, 50, 60, 76)
                     for g in ("LAND_TEST_ONLY", "WATER_T15_S0_TEST_ONLY", "WATER_T15_S35_TEST_ONLY")
                     for p in ("VERTICAL", "HORIZONTAL")
                     for h1, h2 in ((1, 1), (2, 2), (10, 2), (50, 2))
                     for d in (.1, .3, 1, 3, 10, 30)}
    assert len(keys) == len(rows) and keys == expected_grid, "Неполная или повторённая сетка"
    close(data["earthRadiusKm"], 6371, 1e-12)
    close(data["kFactor"], 4 / 3, 1e-15)
    assert data["N_s"] == 301 and data["climate"] == 5
    commands, profiles = [], {}
    with tempfile.TemporaryDirectory(prefix="ricon-gw-codex-") as folder:
        path = Path(folder) / "flat.csv"
        assert not any(c.isspace() for c in str(path)), "Путь драйвера должен быть одним токеном"
        for distance in sorted({r["dKm"] for r in rows}):
            count = round(distance * 100)  # Независимая сборка плоского профиля с шагом 10 м.
            assert math.isclose(count * 10, distance * 1000)
            profiles[distance] = [count, 10] + [0] * (count + 1)
        path.write_text("\n".join(",".join(map(str, p)) for p in profiles.values()) + "\n", encoding="ascii")
        indices = {d: i for i, d in enumerate(profiles)}
        for row in rows:
            f, d, h1, h2 = (row[k] for k in ("fMHz", "dKm", "h1", "h2"))
            if row["ground"] == "LAND_TEST_ONLY":
                eps, sigma = 15.0, .005
            else:
                salinity = 35 if "S35_" in row["ground"] else 0
                eps, imag = water.water_permittivity(f / 1000, 15, salinity)
                sigma = imag * f / 18000
            close(row["eps"], eps, 1e-12)
            close(row["sigma"], sigma, 1e-12)
            pol = 1 if row["pol"] == "VERTICAL" else 0
            commands.append(f"LFMF {h1!r} {h2!r} {f!r} 1 301 {d!r} {eps!r} {sigma!r} {pol}")
            commands.append(f"ITM {h1!r} {h2!r} {f!r} {eps!r} {sigma!r} {pol} 301 5 1 50 50 50 {path} {indices[d]}")
        commands += ["LFMF 2 2 20 1 301 3 15 .005 1", "LFMF 2 2 20 1000 301 3 15 .005 1"]
        process = subprocess.run([str(exe)], input="\n".join(commands) + "\n", text=True,
                                 capture_output=True, check=True, timeout=45)
    lines = process.stdout.splitlines()
    assert len(lines) == len(commands), "Неполный вывод NTIA"
    parsed = []
    for command, line in zip(commands, lines):
        kind, *values = line.split()
        assert kind == command.split()[0]
        parsed.append(dict(item.split("=", 1) for item in values))
    maxima = {name: 0.0 for name in ("C1", "C2", "C3", "C4")}
    errors = {"C1": {}, "C3": {}}
    for index, row in enumerate(rows):
        lf, it = parsed[2 * index:2 * index + 2]
        for name, raw, key in (("C1", lf, "C1_LFMF"), ("C3", it, "C3_ITM")):
            rc = int(raw["rc"])
            assert rc == row[key]["rc"]
            errors[name][str(rc)] = errors[name].get(str(rc), 0) + 1
        assert int(lf["method"]) == row["C1_LFMF"]["method"]
        assert int(it["warnings"], 16) == int(row["C3_ITM"]["warnings"], 16)
        for name, raw, field, key, success in (
                ("C1", lf, "A_btl", "C1_LFMF", int(lf["rc"]) == 0),
                ("C3", it, "A", "C3_ITM", int(it["rc"]) in (0, 1))):
            if success:
                maxima[name] = max(maxima[name], close(float(raw[field]), row[key]["Lb"]))
            else:
                assert row[key]["Lb"] is None
        f, d, h1, h2, eps, sigma = (row[k] for k in ("fMHz", "dKm", "h1", "h2", "eps", "sigma"))
        cfg = {"permittivity": eps, "conductivitySm": sigma, "polarization": row["pol"]}
        wavelength = l2.SPEED_OF_LIGHT_MPS / (f * 1e6)
        fspl = 20 * math.log10(4 * math.pi * d * 1000 / wavelength)
        ae = 6371 * 4 / 3
        ls, case, accuracy = l2.spherical(d, h1, h2, wavelength, ae, f, cfg)
        maxima["C2"] = max(maxima["C2"], close(fspl + ls, row["C2_P526S3"]["Lb"]))
        assert case == row["C2_P526S3"]["case"] and accuracy == row["C2_P526S3"]["firstTermAccuracy"]
        profile = profiles[d]
        ds = [i * profile[1] / 1000 for i in range(profile[0] + 1)]
        hs = [0.0] * len(ds)
        bull, branch = l2.bullington(ds, hs, h1, h2, 1 / ae, wavelength)
        smooth_tx, smooth_rx = l2.smooth_heights(ds, hs, h1, h2)
        smooth, _ = l2.bullington(ds, hs, h1 - smooth_tx, h2 - smooth_rx, 1 / ae, wavelength)
        spherical, _, _ = l2.spherical(d, h1 - smooth_tx, h2 - smooth_rx, wavelength, ae, f, cfg)
        loss = fspl + bull + max(0, spherical - smooth)
        maxima["C4"] = max(maxima["C4"], close(loss, row["C4_L2FULL"]["Lb"]))
        assert branch == row["C4_L2FULL"]["branch"]
        expected = "P368_VERTICAL" if row["pol"] == "VERTICAL" else "IMPLEMENTATION_ONLY_H"
        assert row["C1_LFMF"]["sourceMechanism"] == expected
        if row["C1_LFMF"]["Lb"] is not None:
            close(row["C1_LFMF"]["diagIsoAssumed"], row["C1_LFMF"]["Lb"] - 20 * math.log10(2))
    low, high = parsed[-2:]
    assert int(low["rc"]) == int(high["rc"]) == 0
    close(float(low["A_btl"]), float(high["A_btl"]))
    close(float(high["E"]) - float(low["E"]), 30)
    close(float(high["P_rx"]) - float(low["P_rx"]), 30)
    result = {"level": "TOOL_ONLY_REPLAY_NOT_PHYSICAL_ACCEPTANCE", "sourceSha256": sha(source),
              "exeSha256": sha(exe), "checkerSha256": sha(Path(__file__)), "cells": len(rows),
              "maxDifferenceDb": maxima, "returnCodes": errors, "powerMetamorphic": "PASS", "itmMdvar": 1, "result": "PASS",
              "note": "C1/C3 перепроверены своим stdin-adapter; C2/C4 используют ранее проверенные общие L2 primitives. Это не новая независимая физическая модель."}
    target.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, ensure_ascii=False))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Независимый replay наземных кандидатов")
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--output", type=Path, default=Path(__file__).with_suffix(".json"))
    args = parser.parse_args()
    check(args.exe.resolve(), args.input.resolve(), args.output.resolve())
