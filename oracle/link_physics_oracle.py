#!/usr/bin/env python3
"""Host-эталон физики радиолинии (спринт 20, фокус владельца agora #2020): stdlib, не production.

Модели потерь считаются раздельно, без склейки (agora #2083):
- `FSPL`          — свободное пространство, 20 lg(4πd/λ);
- `TWO_RAY_FLAT`  — прямой луч и луч, отражённый от плоской земли; коэффициенты отражения — ITU-R P.526-16
                    ур. (100)/(101), η = εr − j·18·10⁹·σ/f (стр. 40–41, сверено по изображению страницы);
- `P526_GENERAL`  — FSPL + дифракция на профиле по P.526-16 §4.5 (`l2_terrain_oracle.l2`, тот же метод, что L2
                    в RICON);
Вне матрицы — `knife_edge_4ray`: неудачная попытка (RESEARCH, agora #2203, #2214). Острый клин над плоской землёй
с отражением по обе стороны, 4 зеркальные пары, комплексное поле Френеля–Кирхгофа (модуль — ур. (30)). Первоисточника
метода нет, а предел «без препятствия» на земле с потерями не выполняется (тест). В сценарии и выводы не входит.
Шум — kTB, коэффициент шума системы f = fa + fr − 1 (P.372-17 ур. (5)) и медиана техногенного шума
Fam = c − d·lg f (P.372-17 ур. (17), Table 1; область 0.3–250 МГц для city/residential/rural).
Доступность — два отдельных критерия (agora #2132): `aboveSensitivity` (Prx ≥ kT0B + NF + требуемый SNR, как
чувствительность в RICON, без шума среды) и `meetsSnr` (SNR с шумом среды ≥ требуемого). Порог — параметр сценария
(TEST_ONLY); состояния clean/degraded/lost не моделируются (`snrState = DEFERRED`), речь — `NOT_CALIBRATED`.

Дальность ищется на геометрической сетке. Интервалы доступности и `outerLimitM` — оценки по выборке
(`SAMPLED_ESTIMATE`) с указанным шагом: между узлами возможны неучтённые провалы (agora #2072, #2083).
"""

from __future__ import annotations

import argparse
import cmath
import json
import math
import re
import sys
from pathlib import Path
from typing import Any, Callable

sys.path.insert(0, str(Path(__file__).resolve().parent))  # own directory only: reuse the L2 terrain oracle
import l2_terrain_oracle as l2o  # noqa: E402

SPEED_OF_LIGHT_MPS = 299792458.0       # exact by SI
BOLTZMANN_J_PER_K = 1.380649e-23       # exact by SI (2019)
T0_K = 290.0                           # P.372-17 reference temperature
MANMADE_NOISE = {                      # P.372-17 Table 1: (c, d) for Fam = c − d·lg f, f in MHz
    "city": (76.8, 27.7),
    "residential": (72.5, 27.7),
    "rural": (67.2, 27.7),
}
MANMADE_DOMAIN_MHZ = (0.3, 250.0)      # P.372-17 §6.1.1, for the three categories above
NOISE_ENVIRONMENTS = ("none",) + tuple(MANMADE_NOISE)
POLARIZATIONS = ("HORIZONTAL", "VERTICAL")
MODELS = ("FSPL", "TWO_RAY_FLAT", "P526_GENERAL")
SOURCE = "oracle/link_physics_oracle.py"
STANDARDS = ["Rec. ITU-R P.526-16 (11/2025): eq (2), (6)-(7b), (21), (22a-22e), (26), (30), (100), (101), §4.5",
             "Rec. ITU-R P.372-17 (08/2024): eq (5), (17), Table 1"]
SCENARIO_ID = re.compile(r"[A-Za-z0-9_.-]{1,64}")
ROOT_KEYS = {"schema", "version", "revision", "source", "standards", "provenance", "note", "speechQuality", "snrState",
             "relTolerance", "scenarios"}


def wavelength_m(f_mhz: float) -> float:
    return SPEED_OF_LIGHT_MPS / (f_mhz * 1e6)


def fspl_db(d_m: float, f_mhz: float) -> float:
    """Free-space basic loss between isotropic antennas (Friis): 20 lg(4πd/λ)."""
    return 20.0 * math.log10(4.0 * math.pi * d_m / wavelength_m(f_mhz))


def fresnel_radius_m(n: int, d1_m: float, d2_m: float, f_mhz: float) -> float:
    """P.526-16 eq (2): R_n = sqrt(n·λ·d1·d2/(d1 + d2)), self-consistent units."""
    return math.sqrt(n * wavelength_m(f_mhz) * d1_m * d2_m / (d1_m + d2_m))


def complex_permittivity(f_mhz: float, eps_r: float, sigma_sm: float) -> complex:
    """P.526-16 (after eq 101): η = εr − j·18·10⁹·σ/f, f in Hz."""
    return complex(eps_r, -18e9 * sigma_sm / (f_mhz * 1e6))


def reflection_coefficient(psi_rad: float, f_mhz: float, eps_r: float, sigma_sm: float, polarization: str) -> complex:
    """P.526-16 eq (100) (perpendicular E, i.e. horizontal polarization over ground) and eq (101) (parallel E,
    vertical polarization); psi is the grazing angle."""
    if polarization not in POLARIZATIONS:
        raise ValueError(f"polarization {polarization!r}")
    eta = complex_permittivity(f_mhz, eps_r, sigma_sm)
    s, root = math.sin(psi_rad), cmath.sqrt(eta - math.cos(psi_rad) ** 2)
    if polarization == "HORIZONTAL":
        return (s - root) / (s + root)
    return (eta * s - root) / (eta * s + root)


def two_ray_flat(d_m: float, h1_m: float, h2_m: float, f_mhz: float, ground: dict[str, Any]) -> dict[str, Any]:
    """Direct ray plus one ray reflected from a flat smooth ground; d is the horizontal distance.
    Loss = FSPL(r1) − 20 lg|1 + R·(r1/r2)·exp(−jkΔr)|; the field-vector projection of the two rays is ignored
    (both treated as parallel, a small-angle assumption)."""
    r1 = math.hypot(d_m, h1_m - h2_m)
    r2 = math.hypot(d_m, h1_m + h2_m)
    psi = math.atan2(h1_m + h2_m, d_m)
    refl = reflection_coefficient(psi, f_mhz, ground["permittivity"], ground["conductivitySm"], ground["polarization"])
    k = 2.0 * math.pi / wavelength_m(f_mhz)
    delta_r = 4.0 * h1_m * h2_m / (r1 + r2)  # = r2 − r1 without cancellation (agora #2138)
    gain = abs(1.0 + refl * (r1 / r2) * cmath.exp(-1j * k * delta_r))
    excess = -20.0 * math.log10(gain) if gain > 0.0 else math.inf
    return {"lossDb": fspl_db(r1, f_mhz) + excess, "excessDb": excess, "pathDifferenceM": delta_r,
            "grazingRad": psi, "reflection": refl}


# ---------------------------------------------------------------------- knife edge with ground reflections

def fresnel_cs(x: float) -> tuple[float, float]:
    """Fresnel integrals C(x) = ∫0^x cos(πt²/2) dt and S(x) = ∫0^x sin(πt²/2) dt (the convention of P.526-16
    eq 6-7b). Power series for |x| ≤ 2 (cancellation below ~1e-13); beyond, C + jS = ((1 + j)/2)·(1 − erfc(w)) with
    w = (√π/2)(1 − j)x and erfc from its continued fraction (Abramowitz & Stegun 7.1.14), evaluated by the
    modified Lentz method."""
    if x < 0:
        c, s = fresnel_cs(-x)
        return -c, -s
    if x <= 2.0:
        # C = x·Σ (−1)^n z^(2n)/((2n)!(4n+1)), S = x·Σ (−1)^n z^(2n+1)/((2n+1)!(4n+3)), z = πx²/2
        z = math.pi * x * x / 2.0
        c = s = 0.0
        a_n = 1.0  # (−1)^n z^(2n)/(2n)!
        n = 0
        while True:
            tc = a_n * x / (4 * n + 1)
            ts = a_n * z / (2 * n + 1) * x / (4 * n + 3)
            c += tc
            s += ts
            if n > 2 and abs(tc) < 1e-17 and abs(ts) < 1e-17:
                return c, s
            a_n *= -z * z / ((2 * n + 1) * (2 * n + 2))
            n += 1
    w = math.sqrt(math.pi) / 2.0 * (1 - 1j) * x
    tiny = 1e-300
    f = w            # K = w + (1/2)/(w + 1/(w + (3/2)/(w + ...))): erfc(w) = exp(−w²)/(√π·K)
    cc, dd = f, 0j
    for n in range(1, 100000):
        a = n / 2.0
        dd = w + a * dd
        dd = 1.0 / (dd if dd != 0 else tiny)
        cc = w + a / (cc if cc != 0 else tiny)
        delta = cc * dd
        f *= delta
        if abs(delta - 1.0) < 1e-16:
            break
    else:
        raise ArithmeticError("Fresnel continued fraction did not converge")
    erfc = cmath.exp(-w * w) / (math.sqrt(math.pi) * f)
    cs = (1 + 1j) / 2.0 * (1.0 - erfc)
    return cs.real, cs.imag


def knife_edge_field(nu: float) -> complex:
    """Field behind a single knife edge relative to free space (Fresnel–Kirchhoff):
    ((1 + j)/2)·∫ν^∞ exp(−jπt²/2) dt; its magnitude in dB is −J(ν) of P.526-16 eq (30)."""
    c, s = fresnel_cs(nu)
    return (1 + 1j) / 2 * complex(0.5 - c, -(0.5 - s))


def knife_edge_j30(nu: float) -> float:
    """P.526-16 eq (30): J(ν) = −20 lg( sqrt((1 − C − S)² + (C − S)²) / 2 )."""
    c, s = fresnel_cs(nu)
    return -20.0 * math.log10(math.sqrt((1 - c - s) ** 2 + (c - s) ** 2) / 2.0)


def knife_edge_4ray(d_m: float, h1_m: float, h2_m: float, f_mhz: float, ground: dict[str, Any] | None,
                    edge_m: float, edge_height_m: float) -> dict[str, Any]:
    """One knife edge of height H at distance a from TX over flat ground, with a ground reflection allowed on each
    side: 4 source/receiver image pairs (h1 or −h1, h2 or −h2), each diffracted by the edge with its own ν (P.526-16
    eq 26) and summed coherently. R from eq (100)/(101) at the grazing angle towards the edge top. ground=None
    disables reflections (pure single knife edge). Not a P.526 method and RESEARCH only (agora #2203): the image
    construction has no verified primary source here, and its no-obstacle limit fails for low antennas: a 1 cm edge
    moves the 3 m / 50 MHz / 4 km loss by 7.5 dB against two-ray (with 200 m antennas the limit holds within 1 dB)."""
    a, b = edge_m, d_m - edge_m
    lam = wavelength_m(f_mhz)
    k = 2.0 * math.pi / lam
    r0 = math.hypot(d_m, h1_m - h2_m)
    if ground is None:
        r_tx = r_rx = 0j
    else:
        r_tx = reflection_coefficient(math.atan2(h1_m + edge_height_m, a), f_mhz, ground["permittivity"],
                                      ground["conductivitySm"], ground["polarization"])
        r_rx = reflection_coefficient(math.atan2(h2_m + edge_height_m, b), f_mhz, ground["permittivity"],
                                      ground["conductivitySm"], ground["polarization"])
    total, nus = 0j, []
    for zs, zr, refl in ((h1_m, h2_m, 1.0), (-h1_m, h2_m, r_tx), (h1_m, -h2_m, r_rx), (-h1_m, -h2_m, r_tx * r_rx)):
        clearance = edge_height_m - (zs + (zr - zs) * a / d_m)
        nu = clearance * math.sqrt(2.0 * d_m / (lam * a * b))                                    # (26)
        r_ij = math.hypot(d_m, zr - zs)
        extra = ((zr - zs) ** 2 - (h1_m - h2_m) ** 2) / (r_ij + r0)  # r_ij − r0 without cancellation
        total += refl * (r0 / r_ij) * knife_edge_field(nu) * cmath.exp(-1j * k * extra)
        nus.append(nu)
    gain = abs(total)
    excess = -20.0 * math.log10(gain) if gain > 0.0 else math.inf
    return {"lossDb": fspl_db(r0, f_mhz) + excess, "excessDb": excess, "nu": nus}


def radio_horizon_m(h1_m: float, h2_m: float, ae_km: float) -> float:
    """P.526-16 eq (21): d_los = sqrt(2·ae)·(sqrt(h1) + sqrt(h2)), metres."""
    return math.sqrt(2.0 * ae_km * 1000.0) * (math.sqrt(h1_m) + math.sqrt(h2_m))


def curvature_phase_error_rad(d_m: float, h1_m: float, h2_m: float, f_mhz: float, ae_km: float) -> float | None:
    """k·|Δr_flat − Δr_curved| with heights above the tangent plane at the reflection point of P.526-16 eq (22a-22e)
    and Δr ≈ 2·h1·h2/d. Only a comparison of the path phase: divergence, the grazing angle and R also change over
    a curved earth, so this is not a validation of the flat model, nor a threshold (agora #2095, #2111).
    None when a reduced height is not positive (no reflection point above the curved earth)."""
    ae = ae_km * 1000.0
    m = d_m ** 2 / (4.0 * ae * (h1_m + h2_m))                                                         # (22e)
    c = (h1_m - h2_m) / (h1_m + h2_m)                                                                 # (22d)
    arg = max(-1.0, min(1.0, 1.5 * c * math.sqrt(3.0 * m / (m + 1.0) ** 3)))
    b = 2.0 * math.sqrt((m + 1.0) / (3.0 * m)) * math.cos(math.pi / 3.0 + math.acos(arg) / 3.0)     # (22c)
    d1 = d_m / 2.0 * (1.0 + b)                                                                        # (22a)
    d2 = d_m - d1                                                                                     # (22b)
    h1c, h2c = h1_m - d1 ** 2 / (2.0 * ae), h2_m - d2 ** 2 / (2.0 * ae)
    if h1c <= 0.0 or h2c <= 0.0:
        return None
    k = 2.0 * math.pi / wavelength_m(f_mhz)
    return k * abs(2.0 * h1_m * h2_m / d_m - 2.0 * h1c * h2c / d_m)


# ---------------------------------------------------------------------- noise and budget

def thermal_noise_dbm(bandwidth_hz: float) -> float:
    return 10.0 * math.log10(BOLTZMANN_J_PER_K * T0_K * bandwidth_hz) + 30.0


def manmade_noise_fam_db(environment: str, f_mhz: float) -> float | None:
    """Fam in dB above kT0b (P.372-17 eq 17); None outside the model's frequency domain."""
    if environment == "none":
        return None
    c, d = MANMADE_NOISE[environment]
    if not MANMADE_DOMAIN_MHZ[0] <= f_mhz <= MANMADE_DOMAIN_MHZ[1]:
        return None
    return c - d * math.log10(f_mhz)


def system_noise_dbm(bandwidth_hz: float, rx_noise_figure_db: float, environment: str, f_mhz: float) -> dict[str, Any]:
    """P.372-17 eq (5) with lossless antenna and line: f = fa + fr − 1. `inDomain` is false when the environment
    has man-made noise but the frequency is outside its domain; then there is no noise value."""
    fam = manmade_noise_fam_db(environment, f_mhz)
    if environment != "none" and fam is None:
        return {"inDomain": False, "noiseDbm": None, "famDb": None}
    fa = 1.0 if fam is None else 10.0 ** (fam / 10.0)
    f_total = fa + 10.0 ** (rx_noise_figure_db / 10.0) - 1.0
    return {"inDomain": True, "noiseDbm": thermal_noise_dbm(bandwidth_hz) + 10.0 * math.log10(f_total),
            "famDb": fam}


def sensitivity_dbm(radio: dict[str, Any]) -> float:
    """Receiver sensitivity as RICON defines it: kT0B + NF + required SNR, without external noise (agora #2130)."""
    return thermal_noise_dbm(radio["bandwidthHz"]) + radio["rxNoiseFigureDb"] + radio["snrRequiredDb"]


def budget(loss_db: float, radio: dict[str, Any], noise_dbm: float) -> dict[str, Any]:
    """Two separate criteria (agora #2132): `aboveSensitivity` ignores the environment, `meetsSnr` uses the actual
    system noise. A clean/degraded/lost state is not modelled here (DEFERRED); speech quality is NOT_CALIBRATED."""
    prx = radio["txPowerDbm"] + radio["txGainDbi"] + radio["rxGainDbi"] - loss_db
    snr = prx - noise_dbm
    return {"rxPowerDbm": prx, "snrDb": snr, "aboveSensitivity": prx >= sensitivity_dbm(radio),
            "meetsSnr": snr >= radio["snrRequiredDb"]}


# ---------------------------------------------------------------------- scenarios

def hill_height_m(x_km: float, hill: dict[str, Any] | None) -> float:
    """Symmetric triangular hill (a synthetic test shape, not a map)."""
    if hill is None:
        return 0.0
    return max(0.0, hill["heightM"] * (1.0 - abs(x_km - hill["centerKm"]) / hill["halfWidthKm"]))


def slant_fspl_db(d_m: float, h1_m: float, h2_m: float, f_mhz: float, hill: dict[str, Any] | None) -> float:
    """FSPL between the antennas at their real altitudes: ground height under each end plus the antenna height
    (agora #2132 item 3)."""
    z1, z2 = hill_height_m(0.0, hill) + h1_m, hill_height_m(d_m / 1000.0, hill) + h2_m
    return fspl_db(math.hypot(d_m, z1 - z2), f_mhz)


def p526_general_db(d_m: float, h1_m: float, h2_m: float, f_mhz: float, ground: dict[str, Any],
                    terrain: dict[str, Any]) -> float:
    cfg = {"earthRadiusKm": terrain["earthRadiusKm"], "kFactor": terrain["kFactor"],
           "permittivity": ground["permittivity"], "conductivitySm": ground["conductivitySm"],
           "polarization": ground["polarization"], "maxProfileStepKm": terrain["profileStepKm"],
           "maxProfilePoints": l2o.MAX_PROFILE_POINTS_CAP}
    hill = terrain["hill"]
    status, ds = l2o.provider_sampling(d_m, cfg)
    if status == "SKIP":
        return slant_fspl_db(d_m, h1_m, h2_m, f_mhz, hill)
    if status != "OK":
        raise ValueError("profile unavailable")
    result = l2o.l2({"profile": [[x, hill_height_m(x, hill)] for x in ds], "htg": h1_m, "hrg": h2_m,
                     "frequencyMHz": f_mhz, "config": cfg})
    if result["code"] != "OK":
        raise ValueError(result["code"])
    return slant_fspl_db(d_m, h1_m, h2_m, f_mhz, hill) + result["L"]


def loss_function(model: str, sc: dict[str, Any]) -> Callable[[float], float]:
    f, h1, h2, ground = sc["frequencyMHz"], sc["txHeightM"], sc["rxHeightM"], sc["ground"]
    if model == "FSPL":
        return lambda d: slant_fspl_db(d, h1, h2, f, sc["terrain"]["hill"])
    if model == "TWO_RAY_FLAT":
        return lambda d: two_ray_flat(d, h1, h2, f, ground)["lossDb"]
    if model == "P526_GENERAL":
        return lambda d: p526_general_db(d, h1, h2, f, ground, sc["terrain"])
    raise ValueError(f"model {model!r}")


def geometric_grid(grid: dict[str, Any]) -> list[float]:
    lo, hi, ratio = grid["minM"], grid["maxM"], grid["ratio"]
    n = int(math.floor(math.log(hi / lo) / math.log(ratio) + 1e-9))
    return [lo * ratio ** i for i in range(n + 1)]


def availability(points: list[tuple[float, bool]]) -> list[list[float]]:
    """Maximal runs of available grid nodes as [first, last] node distances (sampled, agora #2083)."""
    runs, start, prev = [], None, None
    for d, ok in points:
        if ok and start is None:
            start = d
        if not ok and start is not None:
            runs.append([start, prev])
            start = None
        prev = d
    if start is not None:
        runs.append([start, prev])
    return runs


def model_applicability(model: str, sc: dict[str, Any], d_m: float) -> bool:
    if model == "TWO_RAY_FLAT":   # flat ground is meaningless beyond the radio horizon (P.526-16 eq 21)
        return d_m < radio_horizon_m(sc["txHeightM"], sc["rxHeightM"], sc["terrain"]["earthRadiusKm"] *
                                     sc["terrain"]["kFactor"])
    return True


def sample(model: str, sc: dict[str, Any], d_m: float, loss: Callable[[float], float],
           noises: dict[str, dict[str, Any]]) -> dict[str, Any]:
    """One distance with the whole budget (agora #2162 follow-up): loss, Prx, both criteria and SNR per environment."""
    loss_db = loss(d_m)
    radio = sc["radio"]
    point: dict[str, Any] = {"distanceM": d_m, "lossDb": loss_db,
                             "rxPowerDbm": budget(loss_db, radio, sensitivity_dbm(radio))["rxPowerDbm"],
                             "aboveSensitivity": budget(loss_db, radio, sensitivity_dbm(radio))["aboveSensitivity"],
                             "noise": {}}
    for env, n in noises.items():
        if not n["inDomain"]:
            point["noise"][env] = {"status": "NOISE_OUT_OF_MODEL_DOMAIN"}
            continue
        b = budget(loss_db, radio, n["noiseDbm"])
        point["noise"][env] = {"snrDb": b["snrDb"], "meetsSnr": b["meetsSnr"]}
    if model == "TWO_RAY_FLAT":
        point["curvaturePhaseErrorRad"] = curvature_phase_error_rad(
            d_m, sc["txHeightM"], sc["rxHeightM"], sc["frequencyMHz"],
            sc["terrain"]["earthRadiusKm"] * sc["terrain"]["kFactor"])
    return point


def evaluate_scenario(sc: dict[str, Any]) -> dict[str, Any]:
    reason = validate_scenario(sc)
    if reason is not None:
        raise ValueError(f"scenario input: {reason}")
    radio = sc["radio"]
    noises = {env: system_noise_dbm(radio["bandwidthHz"], radio["rxNoiseFigureDb"], env, sc["frequencyMHz"])
              for env in sc["noiseEnvironments"]}
    out: dict[str, Any] = {"noise": {env: n["noiseDbm"] for env, n in noises.items()}, "models": {}}
    for model in sc["models"]:
        grid = geometric_grid(sc["grids"][model])
        loss = loss_function(model, sc)
        valid = [d for d in grid if model_applicability(model, sc, d)]
        losses = [(d, loss(d)) for d in valid]
        entry: dict[str, Any] = {"grid": dict(sc["grids"][model], nodes=len(grid)),
                                 "applicableUpToM": valid[-1] if valid else None,
                                 "samples": [sample(model, sc, d, loss, noises) for d in sc["sampleDistancesM"]
                                             if model_applicability(model, sc, d)]}
        sens = sensitivity_dbm(radio)
        entry["aboveSensitivity"] = range_summary(
            [(d, budget(l, radio, sens)["aboveSensitivity"]) for d, l in losses], valid, grid)
        entry["meetsSnr"] = {}
        for env, n in noises.items():
            if not n["inDomain"]:
                entry["meetsSnr"][env] = {"status": "NOISE_OUT_OF_MODEL_DOMAIN"}
                continue
            entry["meetsSnr"][env] = range_summary(
                [(d, budget(l, radio, n["noiseDbm"])["meetsSnr"]) for d, l in losses], valid, grid)
        out["models"][model] = entry
    return out


def range_summary(flags: list[tuple[float, bool]], valid: list[float], grid: list[float]) -> dict[str, Any]:
    runs = availability(flags)
    last_ok = runs[-1][1] if runs else None
    if last_ok is None:
        outer, status = None, "NOT_AVAILABLE_ON_GRID"
    elif last_ok == valid[-1]:
        outer = None
        status = "AVAILABLE_TO_GRID_END" if valid[-1] == grid[-1] else "AVAILABLE_TO_APPLICABILITY_END"
    else:
        outer, status = last_ok, "SAMPLED_ESTIMATE"
    return {"status": status, "intervalsM": runs, "outerLimitM": outer}


# ---------------------------------------------------------------------- closed input domain (agora #2132 item 1)

def _num(value: Any, low: float, high: float, low_open: bool = False) -> bool:
    return l2o.is_number(value) and (value > low if low_open else value >= low) and value <= high


def _keys(value: Any, keys: set[str]) -> bool:
    return isinstance(value, dict) and set(value) == keys


def _names(value: Any, allowed: tuple[str, ...]) -> bool:
    return isinstance(value, list) and len(value) >= 1 and len(set(map(str, value))) == len(value) \
        and all(isinstance(v, str) and v in allowed for v in value)


def validate_scenario(sc: Any) -> str | None:
    """Reason a scenario input is outside the closed domain, or None."""
    if not _keys(sc, {"frequencyMHz", "txHeightM", "rxHeightM", "ground", "terrain", "radio", "noiseEnvironments",
                      "models", "grids", "sampleDistancesM"}):
        return "scenario keys"
    if not _num(sc["frequencyMHz"], l2o.MIN_FREQUENCY_MHZ, 6000.0):
        return "frequencyMHz"
    if not (_num(sc["txHeightM"], l2o.MIN_ANTENNA_M, 1000.0) and _num(sc["rxHeightM"], l2o.MIN_ANTENNA_M, 1000.0)):
        return "antenna height"
    g = sc["ground"]
    if not _keys(g, {"permittivity", "conductivitySm", "polarization"}) or not _num(g["permittivity"], 1.0, 1000.0, True) \
            or not _num(g["conductivitySm"], 0.0, 1000.0) or g["polarization"] not in POLARIZATIONS:
        return "ground"
    t = sc["terrain"]
    if not _keys(t, {"earthRadiusKm", "kFactor", "profileStepKm", "hill"}):
        return "terrain keys"
    for key, cfg_key in (("earthRadiusKm", "earthRadiusKm"), ("kFactor", "kFactor"), ("profileStepKm", "maxProfileStepKm")):
        if not l2o.in_domain(t[key], *l2o.CONFIG_DOMAIN[cfg_key]):
            return f"terrain {key}"
    hill = t["hill"]
    if hill is not None and (not _keys(hill, {"centerKm", "heightM", "halfWidthKm"}) or not _num(hill["centerKm"], 0.0, 500.0)
                             or not _num(hill["heightM"], 0.0, 9000.0) or not _num(hill["halfWidthKm"], 0.0, 100.0, True)):
        return "hill"
    r = sc["radio"]
    if not _keys(r, {"txPowerDbm", "txGainDbi", "rxGainDbi", "bandwidthHz", "rxNoiseFigureDb", "snrRequiredDb"}) \
            or not _num(r["txPowerDbm"], -30.0, 70.0) or not _num(r["txGainDbi"], -30.0, 40.0) \
            or not _num(r["rxGainDbi"], -30.0, 40.0) or not _num(r["bandwidthHz"], 1.0, 1e8) \
            or not _num(r["rxNoiseFigureDb"], 0.0, 40.0) or not _num(r["snrRequiredDb"], -30.0, 60.0):
        return "radio"
    if not _names(sc["noiseEnvironments"], NOISE_ENVIRONMENTS):
        return "noiseEnvironments"
    if not _names(sc["models"], MODELS):
        return "models"
    if "TWO_RAY_FLAT" in sc["models"] and hill is not None:
        return "TWO_RAY_FLAT needs flat terrain"
    if not _keys(sc["grids"], set(sc["models"])):
        return "grids keys"
    for grid in sc["grids"].values():
        if not _keys(grid, {"minM", "maxM", "ratio"}) or not _num(grid["minM"], 1.0, 500000.0) \
                or not _num(grid["maxM"], grid["minM"], 500000.0, True) or not _num(grid["ratio"], 1.0001, 10.0):
            return "grid"
    s = sc["sampleDistancesM"]
    if not isinstance(s, list) or not 1 <= len(s) <= 20 or not all(_num(d, 1.0, 500000.0) for d in s):
        return "sampleDistancesM"
    return None


# ---------------------------------------------------------------------- vectors

def _close(a: Any, b: Any, tol: float, path: str, errors: list[str]) -> None:
    if isinstance(a, dict) and isinstance(b, dict):
        if set(a) != set(b):
            errors.append(f"{path}: keys {sorted(a)} != {sorted(b)}")
            return
        for k in a:
            _close(a[k], b[k], tol, f"{path}.{k}", errors)
    elif isinstance(a, list) and isinstance(b, list):
        if len(a) != len(b):
            errors.append(f"{path}: length {len(a)} != {len(b)}")
            return
        for i, (x, y) in enumerate(zip(a, b)):
            _close(x, y, tol, f"{path}[{i}]", errors)
    elif isinstance(a, bool) or isinstance(b, bool) or a is None or b is None or isinstance(a, str):
        if type(a) is not type(b) or a != b:  # True is not 1 (agora #2138)
            errors.append(f"{path}: {a!r} != {b!r}")
    elif isinstance(a, (int, float)) and isinstance(b, (int, float)):
        if not (math.isfinite(a) and math.isfinite(b)):  # Inf <= Inf would pass (agora #2162)
            errors.append(f"{path}: non-finite {a!r} / {b!r}")
        elif not abs(a - b) <= tol * max(1.0, abs(b)):
            errors.append(f"{path}: {a!r} != {b!r}")
    else:
        errors.append(f"{path}: type {type(a).__name__} != {type(b).__name__}")


def check_vector_file(data: dict[str, Any], evaluated: dict[str, Any] | None = None) -> list[str]:
    """`evaluated` (scenario id -> evaluate_scenario result) lets tests reuse one evaluation of the inputs."""
    errors: list[str] = []
    if not _keys(data, ROOT_KEYS) or data["schema"] != "ricon.oracle.link_scenarios" \
            or type(data["version"]) is not int or data["version"] != 1 \
            or type(data["revision"]) is not int or data["revision"] < 1 \
            or data["source"] != SOURCE or data["standards"] != STANDARDS \
            or not isinstance(data["note"], str) or not data["note"] \
            or data["speechQuality"] != "NOT_CALIBRATED" or data["snrState"] != "DEFERRED" \
            or data["relTolerance"] != 1e-9 or data["provenance"] != "ORACLE_SNAPSHOT" \
            or not isinstance(data["scenarios"], list) or not data["scenarios"]:
        return ["envelope"]
    ids = [sc.get("id") if isinstance(sc, dict) else None for sc in data["scenarios"]]
    if len(set(map(str, ids))) != len(ids) or not all(isinstance(i, str) and SCENARIO_ID.fullmatch(i) for i in ids):
        return ["scenario ids"]
    for sc in data["scenarios"]:
        if not _keys(sc, {"id", "input", "expect"}):
            errors.append(f"{sc['id']}: keys")
            continue
        reason = validate_scenario(sc["input"])
        if reason is not None:
            errors.append(f"{sc['id']}: input {reason}")
            continue
        actual = evaluated[sc["id"]] if evaluated is not None else evaluate_scenario(sc["input"])
        _close(actual, sc["expect"], data["relTolerance"], sc["id"], errors)
    return errors


# ---------------------------------------------------------------------- probe points (agora #2201, #2206)
# The budget of one link exactly as docs/req/REQ-link-eval.md defines it, for the Enforce probe of codex. Written from
# the REQ text by claude, independently of RICON_LinkEvaluator.c. Q and the state use the TEST_ONLY `linear_test`
# fixture (REQ-LE-74/75): they check the code path, they are not a speech model.

PROBE_LAYERS = ("FSPL", "P526_GENERAL")
LINEAR_TEST = {"a": -15.0, "b": 15.0, "qClean": 0.8, "qDegraded": 0.5, "status": "TEST_ONLY"}


def req_budget(probe: dict[str, Any]) -> dict[str, Any]:
    f, d = probe["frequencyMHz"], probe["distanceM"]
    hill = probe["terrain"]["hill"] if probe["layers"] == "P526_GENERAL" else None
    z1 = hill_height_m(0.0, hill) + probe["txHeightM"]
    z2 = hill_height_m(d / 1000.0, hill) + probe["rxHeightM"]
    d3 = math.hypot(d, z1 - z2)
    fspl = fspl_db(max(d3, wavelength_m(f)), f)                                               # REQ-LE-40, 41
    terrain = None
    if probe["layers"] == "P526_GENERAL":
        terrain = p526_general_db(d, probe["txHeightM"], probe["rxHeightM"], f, probe["ground"], probe["terrain"]) \
            - slant_fspl_db(d, probe["txHeightM"], probe["rxHeightM"], f, hill)
    total = fspl + (terrain or 0.0)
    radio = probe["radio"]
    prx = radio["txPowerDbm"] + radio["txGainDbi"] + radio["rxGainDbi"] - total                # REQ-LE-50
    n0 = thermal_noise_dbm(radio["bandwidthHz"])                                               # REQ-LE-60
    fam = manmade_noise_fam_db(probe["noiseEnvironment"], f)
    in_domain = probe["noiseEnvironment"] == "none" or fam is not None                        # REQ-LE-63/64
    fa = 1.0 if fam is None else 10.0 ** (fam / 10.0)
    noise = n0 + 10.0 * math.log10(fa + 10.0 ** (radio["rxNoiseFigureDb"] / 10.0) - 1.0)        # REQ-LE-61
    sinr = prx - noise                                                                         # REQ-LE-70
    sens = n0 + radio["rxNoiseFigureDb"] + radio["snrRequiredDb"]                              # REQ-LE-71
    if prx < sens:                                                                             # REQ-LE-72
        q, state = 0.0, "LOST"
    else:
        q = min(1.0, max(0.0, (sinr - LINEAR_TEST["a"]) / (LINEAR_TEST["b"] - LINEAR_TEST["a"])))   # REQ-LE-74
        state = "CLEAN" if q >= LINEAR_TEST["qClean"] else "DEGRADED" if q >= LINEAR_TEST["qDegraded"] else "WEAK"
    return {"fsplDb": fspl, "terrainDb": terrain, "totalLossDb": total, "rxPowerDbm": prx, "thermalNoiseDbm": n0,
            "noiseDbm": noise, "noiseModelInDomain": in_domain, "sinrDb": sinr, "sensitivityDbm": sens,
            "intelligibility": q, "state": state}


def validate_probe(probe: Any) -> bool:
    """Closed probe input: the scenario domain of validate_scenario plus one distance, one environment, one layer set."""
    if not _keys(probe, {"frequencyMHz", "txHeightM", "rxHeightM", "ground", "terrain", "radio", "noiseEnvironment",
                         "distanceM", "layers"}) or probe["layers"] not in PROBE_LAYERS \
            or probe["noiseEnvironment"] not in NOISE_ENVIRONMENTS or not _num(probe["distanceM"], 1.0, 500000.0):
        return False
    as_scenario = {key: probe[key] for key in ("frequencyMHz", "txHeightM", "rxHeightM", "ground", "terrain", "radio")}
    as_scenario.update(noiseEnvironments=[probe["noiseEnvironment"]], models=["FSPL"],
                       grids={"FSPL": {"minM": 1.0, "maxM": 10.0, "ratio": 2.0}}, sampleDistancesM=[probe["distanceM"]])
    return validate_scenario(as_scenario) is None


def check_probe_file(data: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    if not _keys(data, {"schema", "version", "source", "fixture", "note", "relTolerance", "points"}) \
            or data["schema"] != "ricon.oracle.link_probe_points" or type(data["version"]) is not int \
            or data["version"] != 1 or data["fixture"] != LINEAR_TEST or data["relTolerance"] != 1e-9 \
            or not isinstance(data["points"], list) or not data["points"]:
        return ["envelope"]
    for point in data["points"]:
        if not _keys(point, {"id", "scenarioId", "note", "input", "expect"}) or not validate_probe(point["input"]):
            errors.append(f"{point.get('id')}: input")
            continue
        _close(req_budget(point["input"]), point["expect"], data["relTolerance"], point["id"], errors)
    return errors


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", type=Path, required=True)
    args = parser.parse_args(argv)
    data = json.loads(args.check.read_text(encoding="utf-8"))
    errors = check_probe_file(data) if data.get("schema") == "ricon.oracle.link_probe_points" \
        else check_vector_file(data)
    for e in errors[:50]:
        print(e)
    print("LINK_PHYSICS_ORACLE_PASS" if not errors else f"LINK_PHYSICS_ORACLE_FAIL ({len(errors)})")
    return 0 if not errors else 1


if __name__ == "__main__":
    sys.exit(main())
