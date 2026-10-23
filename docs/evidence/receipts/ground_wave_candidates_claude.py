"""claude, TOOL_ONLY: compares ground-path candidates at 10-30 MHz before any model is chosen (audit §4 item 1).

Candidates are compared side by side and never summed or averaged (ensemble rule). Each cell keeps the raw number
together with its own domain verdict, so a value returned outside a method's domain is never read as SOURCE.

  C1 LFMF   NTIA/ITS LFMF v1.1 (P.368 integral method), unmodified C++, basic transmission loss A_btl.
  C2 P526S3 RICON l2_terrain_oracle.spherical(): P.526-16 §3.2 smooth-earth diffraction only, Lb = FSPL + Lsph.
  C3 ITM    NTIA/ITS ITM v1.4 point-to-point, unmodified C++, flat profile, 50/50/50 %, basic transmission loss.
  C4 L2FULL RICON l2_terrain_oracle §4.5 (Bullington + §3.2) on a flat profile, called below its 30 MHz gate.
            Experimental only: this is the ASSUMED mode of the audit, not the production L2.

Usage: python -I -X utf8 ground_wave_candidates_claude.py <ntia_ref.exe> <out.json>
The executable is the scratchpad build of NTIA sources (driver: stdin commands LFMF/ITM); it is not in the repo.
"""

import hashlib
import importlib
import json
import math
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "oracle"))
rf = importlib.import_module("ricon_oracle")
l2 = importlib.import_module("l2_terrain_oracle")
water = importlib.import_module("path_observation_oracle")

FREQUENCIES_MHZ = (10.0, 15.0, 20.0, 25.0, 30.0, 40.0, 50.0, 60.0, 76.0)  # LFMF stops at 30 MHz (rc 34)
DISTANCES_KM = (0.1, 0.3, 1.0, 3.0, 10.0, 30.0)
HEIGHTS_M = ((1.0, 1.0), (2.0, 2.0), (10.0, 2.0), (50.0, 2.0))
EARTH_RADIUS_KM = 6371.0
K_FACTOR = 4.0 / 3.0
N_S = 301.0  # ITM gamma_e(301) gives a_e = 8493 km, close to 4/3 * 6371 = 8495 km used by C2/C4
CLIMATE = 5  # ITM continental temperate
WATER_T_C = 15.0
WATER_S_PPT = (0.0, 35.0)  # bounds of the single owner-configured water profile (WL02), TEST_ONLY
POLARIZATIONS = ((1, "VERTICAL"), (0, "HORIZONTAL"))
MONOPOLE_IMAGE_DB = 20.0 * math.log10(2.0)


def grounds(f_mhz):
    yield "LAND_TEST_ONLY", 15.0, 0.005
    for s in WATER_S_PPT:
        er, ei = water.water_permittivity(f_mhz / 1000.0, WATER_T_C, s)
        yield f"WATER_T15_S{int(s)}_TEST_ONLY", er, ei * f_mhz / 18000.0  # eps'' = 18000 sigma / f_MHz


def itm_profile(d_km):
    n = max(10, min(3000, math.ceil(d_km * 1000.0 / 10.0)))
    return [n, d_km * 1000.0 / n] + [0.0] * (n + 1)


def l2_cfg(eps, sigma, pol):
    return {"permittivity": eps, "conductivitySm": sigma, "polarization": pol}


def c2(f, d, h1, h2, eps, sigma, pol):
    lam = l2.SPEED_OF_LIGHT_MPS / (f * 1e6)
    lsph, case, acc = l2.spherical(d, h1, h2, lam, K_FACTOR * EARTH_RADIUS_KM, f, l2_cfg(eps, sigma, pol))
    domain = "OUT_OF_DOMAIN" if f < 10.0 else ("FIRST_TERM_NOT_MET" if acc == "NOT_MET" else "IN_DOMAIN")  # §3.2, eq (19)
    return {"Lb": rf.free_space_loss(d * 1000.0, f)["lossDb"] + lsph, "excess": lsph, "case": case,
            "firstTermAccuracy": acc, "domain": domain}


def c4(f, d, h1, h2, eps, sigma, pol):
    n = max(3, math.ceil(d / 0.01) + 1)
    ds, hs = [d * i / (n - 1) for i in range(n)], [0.0] * n
    lam = l2.SPEED_OF_LIGHT_MPS / (f * 1e6)
    ae = K_FACTOR * EARTH_RADIUS_KM
    lba, br = l2.bullington(ds, hs, h1, h2, 1.0 / ae, lam)
    hst, hsr = l2.smooth_heights(ds, hs, h1, h2)
    lbs, _ = l2.bullington(ds, [0.0] * n, h1 - hst, h2 - hsr, 1.0 / ae, lam)
    lsph, case, _ = l2.spherical(d, h1 - hst, h2 - hsr, lam, ae, f, l2_cfg(eps, sigma, pol))
    loss = lba + max(lsph - lbs, 0.0)
    return {"Lb": rf.free_space_loss(d * 1000.0, f)["lossDb"] + loss, "excess": loss, "branch": br, "case": case,
            "domain": "ASSUMED_EXPERIMENTAL" if f < 30.0 else "PRODUCTION_L2_DOMAIN"}


def parse(line):
    head, *kv = line.split()
    return head, {k: v for k, v in (x.split("=", 1) for x in kv)}


def run(exe, out):
    cells = []
    with tempfile.TemporaryDirectory() as tmp:
        pfl_file = Path(tmp) / "pfl.csv"
        pfl_file.write_text("\n".join(",".join(repr(float(v)) for v in itm_profile(d)) for d in DISTANCES_KM) + "\n")
        cmds = []
        for f in FREQUENCIES_MHZ:
            for gname, eps, sigma in grounds(f):
                for pol_i, pol in POLARIZATIONS:
                    for h1, h2 in HEIGHTS_M:
                        for row, d in enumerate(DISTANCES_KM):
                            cells.append({"fMHz": f, "ground": gname, "eps": eps, "sigma": sigma, "pol": pol,
                                          "h1": h1, "h2": h2, "dKm": d})
                            cmds.append(f"LFMF {h1!r} {h2!r} {f!r} 1.0 {N_S!r} {d!r} {eps!r} {sigma!r} {pol_i}")
                            cmds.append(f"ITM {h1!r} {h2!r} {f!r} {eps!r} {sigma!r} {pol_i} {N_S!r} {CLIMATE} 1 "
                                        f"50 50 50 {pfl_file} {row}")
        res = subprocess.run([exe], input="\n".join(cmds) + "\n", capture_output=True, text=True, check=True)
    lines = res.stdout.splitlines()
    assert len(lines) == 2 * len(cells), (len(lines), len(cells))
    for i, c in enumerate(cells):
        _, lf = parse(lines[2 * i])
        _, it = parse(lines[2 * i + 1])
        f, d, h1, h2 = c["fMHz"], c["dKm"], c["h1"], c["h2"]
        fspl = rf.free_space_loss(d * 1000.0, f)["lossDb"]
        lam = l2.SPEED_OF_LIGHT_MPS / (f * 1e6)
        lf_ok = int(lf["rc"]) == 0
        lf_dom = ("IN_DOMAIN" if max(h1, h2) < 50.0 else "BOUNDARY_H50") if lf_ok else "ERROR_RC_" + lf["rc"]
        it_rc, it_warn = int(it["rc"]), int(it["warnings"], 16)
        it_ok = it_rc in (0, 1)  # 1 = success with warnings; the flags are kept
        it_dom = ("IN_DOMAIN" if f >= 20.0 and d > 1.0 else "OUTSIDE_README_DOMAIN") if it_ok else f"ERROR_RC_{it_rc}"
        lf_lb = float(lf["A_btl"]) if lf_ok else None
        it_lb = float(it["A"]) if it_ok else None
        c["FSPL"] = fspl
        # P.368-10 Annex 1: reference antenna is a short vertical monopole on perfect ground (LFMF: 4.77 dBi each end).
        # A_btl stays raw. A_btl - 20 log10(2) is only an ASSUMED_DIAGNOSTIC isotropic re-reference (codex #3790): it
        # presumes both antennas near the ground, h << lambda, which is not quantified; max(h)/lambda is reported instead
        # of a verdict. P.368 describes the vertical field component, so H is implementation output, not the P.368 source.
        c["C1_LFMF"] = {"Lb": lf_lb, "excess": None if lf_lb is None else lf_lb - fspl, "method": int(lf["method"]),
                        "rc": int(lf["rc"]), "domain": lf_dom,
                        "sourceMechanism": "P368_VERTICAL" if c["pol"] == "VERTICAL" else "IMPLEMENTATION_ONLY_H",
                        "diagIsoAssumed": None if lf_lb is None else lf_lb - MONOPOLE_IMAGE_DB,
                        "diagMaxHOverLambda": max(h1, h2) / lam,
                        # P.368-10 recommends 3 NOTE 1: if eps_r << 60 lambda sigma, usable up to h = 1.2 sqrt(sigma)
                        # lambda^1.5. "<<" is not quantified, so only the ratio and the height are reported.
                        "p368Ratio60LambdaSigmaOverEps": 60.0 * lam * c["sigma"] / c["eps"],
                        "p368HeightM": 1.2 * math.sqrt(c["sigma"]) * lam ** 1.5}
        c["C3_ITM"] = {"Lb": it_lb, "excess": None if it_lb is None else it_lb - fspl, "A_ref": float(it["A_ref"]),
                       "mode": int(it["mode"]), "rc": it_rc, "warnings": f"0x{it_warn:04x}", "domain": it_dom}
        c["C2_P526S3"] = c2(f, d, h1, h2, c["eps"], c["sigma"], c["pol"])
        c["C4_L2FULL"] = c4(f, d, h1, h2, c["eps"], c["sigma"], c["pol"])
    doc = {"schema": "ground-wave-candidates/1", "tool": "TOOL_ONLY claude", "exeSha256":
           hashlib.sha256(Path(exe).read_bytes()).hexdigest(), "earthRadiusKm": EARTH_RADIUS_KM, "kFactor": K_FACTOR,
           "N_s": N_S, "climate": CLIMATE, "cells": cells}
    # Full repr precision for inputs and outputs: the receipt must describe exactly the computation that ran.
    # (codex #3798: an earlier 4-digit rounding had turned the S0 water sigma at 10 MHz into 0.0.)
    text = json.dumps(doc, separators=(",", ":"), ensure_ascii=False).replace('{"fMHz"', '\n{"fMHz"')
    Path(out).write_text(text + "\n", encoding="utf-8", newline="\n")
    return doc


if __name__ == "__main__":
    d = run(sys.argv[1], sys.argv[2])
    print(len(d["cells"]), "cells", d["exeSha256"])
