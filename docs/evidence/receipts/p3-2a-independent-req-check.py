"""claude: blind check of REQ-material-slab.md v0.1 (P3.2a) — written from the REQ text only.

It does not import codex's material_slab_oracle. Section 1 uses a formulation different from MS05: the characteristic
(Abelès) matrix of the slab with admittances (TE: Y0 = C, Y = p; TM: Y0 = 1/C, Y = e/p), scaled by exp(-x) so that
large losses do not overflow:
    2 e^-x cos(delta) = e^{jy} + e^{-jy-2x},   2 e^-x sin(delta) = (e^{jy} - e^{-jy-2x}) / j,   delta = q = y - jx,
    t = 2 Y0 / (Y0 m11 + Y0^2 m12 + m21 + Y0 m22),   L = -20 log10 |t| = (20/ln10) x - 20 log10(4 |Y0| / |den'|),
    r = (Y0^2 m12 - m21) / (Y0 m11 + Y0^2 m12 + m21 + Y0 m22)   (energy balance: R + T = 1 when sigma = 0).
Section 2 emulates the MS05/MS06 recipe in binary32 (every operation rounded, libm assumed correctly rounded) to
estimate the native error for MS08, and the same recipe with the forbidden shortcuts (1 - r^2, 1 - exp(-2x), unscaled
division) to see which corpus cases actually catch them. Section 3 evaluates extra cases proposed for the corpus.
Usage: python -I -X utf8 p3-2a-independent-req-check.py <corpus.json> <out.json>
"""

import cmath
import hashlib
import json
import math
import struct
import sys
from pathlib import Path

C0 = 299792458.0
EPS0 = 8.8541878128e-12
TOP = ("frequencyMHz", "thicknessM", "cosIncidence", "polarization", "material")
MAT = ("epsilonReal", "conductivitySm", "minMHz", "maxMHz", "provenance")
PROV = ("SOURCE_DOMAIN", "ASSUMED_CONFIG", "TEST_ONLY")


def native_tol(loss):
    """MS08 preliminary native tolerance."""
    return max(0.05, 2e-6 * abs(loss))


def f32(v):
    """JSON number -> binary32 (round to nearest); None for bool, str, null, NaN, Inf or binary32 overflow."""
    if isinstance(v, bool) or not isinstance(v, (int, float)):
        return None
    try:
        fv = float(v)
        return struct.unpack("<f", struct.pack("<f", fv))[0] if math.isfinite(fv) else None
    except OverflowError:
        return None


# ------------------------------------------------------------------ 1. codes (MS03/MS04) and the matrix reference

def classify(inp):
    """Returns (code, parsed): the first applicable MS04 code wins."""
    if not isinstance(inp, dict) or set(inp) != set(TOP):
        return "INVALID_INPUT", None
    f, d, c = f32(inp["frequencyMHz"]), f32(inp["thicknessM"]), f32(inp["cosIncidence"])
    pol = inp["polarization"]
    if f is None or d is None or c is None or not isinstance(pol, str) or pol not in ("TE", "TM") \
            or not f > 0 or not d >= 0 or not 0 <= c <= 1:
        return "INVALID_INPUT", None
    m = inp["material"]
    if not isinstance(m, dict) or set(m) != set(MAT):
        return "INVALID_PROFILE", None
    er, sg, lo, hi = (f32(m[k]) for k in MAT[:4])
    pv = m["provenance"]
    if er is None or sg is None or lo is None or hi is None or not isinstance(pv, str) or pv not in PROV \
            or not er >= 1 or not sg >= 0 or not 0 < lo <= hi:
        return "INVALID_PROFILE", None
    if not 1 <= f <= 450000 or not lo <= f <= hi:
        return "OUT_OF_MODEL_DOMAIN", None
    if not f32(f * d) <= 500 or not c >= f32(0.05) or not er <= 100 or not sg <= 1e8:
        return "NUMERIC_DOMAIN_LIMIT", None
    return None, (f, d, c, pol, er, sg, pv)


def transfer(f, d, c, pol, er, sg):
    """Scaled characteristic matrix in binary64: returns (L dB, power reflectance R)."""
    e = complex(er, -17.98 * sg / (f / 1000.0))
    p = cmath.sqrt(e - 1 + c * c)  # principal root; Re > 0 because er - 1 + c^2 > 0 in the slice, Im <= 0
    k0 = 2 * math.pi * f * 1e6 / C0
    q = k0 * d * p
    y, x = q.real, -q.imag
    y0, yy = (complex(c), p) if pol == "TE" else (complex(1 / c), e / p)
    a, b = cmath.exp(1j * y), cmath.exp(complex(-2 * x, -y))
    cs, sn = a + b, (a - b) / 1j  # 2e^-x cos(delta), 2e^-x sin(delta)
    m12, m21 = 1j * sn / yy, 1j * yy * sn
    den = 2 * y0 * cs + y0 * y0 * m12 + m21
    loss = (20 / math.log(10)) * x - 20 * math.log10(4 * abs(y0) / abs(den))
    return loss, abs((y0 * y0 * m12 - m21) / den) ** 2


def evaluate(inp):
    code, parsed = classify(inp)
    if code:
        return {"code": code}
    f, d, c, pol, er, sg, pv = parsed
    if d == 0 or (er == 1 and sg == 0):
        return {"code": "OK", "lossDb": 0.0, "provenance": pv}
    try:
        loss = transfer(f, d, c, pol, er, sg)[0]
    except (OverflowError, ZeroDivisionError, ValueError):
        return {"code": "NUMERIC_FAILURE"}
    if not math.isfinite(loss) or loss < -0.001:
        return {"code": "NUMERIC_FAILURE"}
    return {"code": "OK", "lossDb": max(loss, 0.0), "provenance": pv}


def invariants():
    """Physics checks independent of the corpus."""
    inv = {}
    loss, refl = transfer(100.0, C0 / 100e6 / 8, 1.0, "TE", 4.0, 0.0)
    inv["quarterWaveN2"] = {"L": loss, "expected": -10 * math.log10(16 / 25), "R+T": refl + 10 ** (-loss / 10)}
    loss, refl = transfer(100.0, C0 / 100e6 / 4, 1.0, "TE", 4.0, 0.0)
    inv["halfWaveN2"] = {"L": loss, "expected": 0.0, "R+T": refl + 10 ** (-loss / 10)}
    loss, refl = transfer(1000.0, 0.137, 1 / math.sqrt(10), "TM", 9.0, 0.0)
    inv["brewsterTMn3"] = {"L": loss, "expected": 0.0, "R": refl}
    inv["normalTEeqTM"] = [transfer(900.0, 0.2, 1.0, pol, 5.3, 0.03)[0] for pol in ("TE", "TM")]
    loss, refl = transfer(900.0, 0.2, 0.6, "TE", 5.3, 0.03)
    inv["lossyObliqueEnergy"] = {"R+T": refl + 10 ** (-loss / 10), "mustBeBelow": 1.0}
    loss, refl = transfer(900.0, 0.2, 0.6, "TM", 5.3, 0.0)
    inv["losslessObliqueTM"] = {"R+T": refl + 10 ** (-loss / 10), "mustBe": 1.0}
    # thin conducting sheet (MS022 inputs): sheet impedance 1/(sigma_eff d) with sigma_eff from the 17.98 convention
    f, d, sg = f32(30.0), f32(1e-12), f32(1e8)
    sig_eff = 2 * math.pi * f * 1e6 * EPS0 * (17.98 * sg / (f / 1000.0))
    z0 = 1 / (EPS0 * C0)
    inv["thinSheetMS022"] = {"L": transfer(f, d, 1.0, "TE", 1.0, sg)[0],
                             "sheetFormula": 20 * math.log10(1 + z0 * sig_eff * d / 2),
                             "sigmaEffOverSigma": sig_eff / sg}
    return inv


# ------------------------------------------------------------------ 2. binary32 emulation of the MS05/MS06 recipe

def r32(v):
    """Round to binary32; OverflowError when the result is not finite (a native float would be Inf)."""
    out = struct.unpack("<f", struct.pack("<f", v))[0]
    if not math.isfinite(out):
        raise OverflowError
    return out


def cadd(a, b):
    return (r32(a[0] + b[0]), r32(a[1] + b[1]))


def csub(a, b):
    return (r32(a[0] - b[0]), r32(a[1] - b[1]))


def cmul(a, b):
    return (r32(r32(a[0] * b[0]) - r32(a[1] * b[1])), r32(r32(a[0] * b[1]) + r32(a[1] * b[0])))


def cabs(a):
    return r32(math.hypot(a[0], a[1]))


def cdiv_naive(a, b):
    den = r32(r32(b[0] * b[0]) + r32(b[1] * b[1]))
    return (r32(r32(r32(a[0] * b[0]) + r32(a[1] * b[1])) / den),
            r32(r32(r32(a[1] * b[0]) - r32(a[0] * b[1])) / den))


def cdiv_smith(a, b):
    if abs(b[0]) >= abs(b[1]):
        t = r32(b[1] / b[0])
        den = r32(b[0] + r32(b[1] * t))
        return (r32(r32(a[0] + r32(a[1] * t)) / den), r32(r32(a[1] - r32(a[0] * t)) / den))
    t = r32(b[0] / b[1])
    den = r32(b[1] + r32(b[0] * t))
    return (r32(r32(r32(a[0] * t) + a[1]) / den), r32(r32(r32(a[1] * t) - a[0]) / den))


def csqrt(z):
    """Principal root without cancellation (Re z > 0 in the MS03 slice)."""
    re = r32(math.sqrt(r32(r32(cabs(z) + z[0]) / 2)))
    return (re, r32(z[1] / r32(2 * re)))


def ms05_f32(f, d, c, pol, er, sg, stable_div=True, stable_a=True, stable_w=True):
    """MS05/MS06 as written, every operation rounded to binary32. Flags switch in the forbidden shortcuts."""
    f, d, c, er, sg = (r32(v) for v in (f, d, c, er, sg))
    e = (er, r32(-r32(r32(r32(17.98) * sg) / r32(f / 1000))))
    p = csqrt((r32(r32(er - 1) + r32(c * c)), e[1]))
    kd = r32(r32(r32(r32(2 * math.pi) * r32(f * 1e6)) / r32(C0)) * d)
    y, x = r32(kd * p[0]), r32(kd * -p[1])
    a = (c, 0.0) if pol == "TE" else cmul(e, (c, 0.0))
    s = cadd(a, p)
    div = cdiv_smith if stable_div else cdiv_naive
    u, v = div(a, s), div(p, s)
    rr = csub(u, v)
    big_a = cmul((4.0, 0.0), cmul(u, v)) if stable_a else csub((1.0, 0.0), cmul(rr, rr))
    if x >= 12:
        big_d = (1.0, 0.0)
    else:
        e2 = r32(math.exp(r32(-2 * x)))
        m = r32(-math.expm1(r32(-2 * x))) if stable_w else r32(1 - e2)
        sy = r32(math.sin(y))
        w = (r32(m + r32(r32(2 * e2) * r32(sy * sy))), r32(e2 * r32(math.sin(r32(2 * y)))))
        big_d = cadd(big_a, cmul(cmul(rr, rr), w))
    return r32(r32(20 / math.log(10)) * r32(r32(x + r32(math.log(cabs(big_d)))) - r32(math.log(cabs(big_a)))))


VARIANTS = {"stable": {}, "naiveDiv": {"stable_div": False}, "naiveA": {"stable_a": False}, "naiveW": {"stable_w": False}}


def emulate(inp, expected):
    """Per variant: the emulated loss (after MS07 clamp), its error against the binary64 expectation, and whether
    the preliminary native tolerance holds."""
    code, parsed = classify(inp)
    if code:
        return None
    f, d, c, pol, er, sg, _ = parsed
    out = {}
    for name, flags in VARIANTS.items():
        if d == 0 or (er == 1 and sg == 0):
            loss = 0.0
        else:
            try:
                loss = ms05_f32(f, d, c, pol, er, sg, **flags)
            except (OverflowError, ZeroDivisionError, ValueError):
                out[name] = {"loss": None, "failure": True}
                continue
            if loss < -0.001:
                out[name] = {"loss": loss, "failure": True}
                continue
            loss = max(loss, 0.0)
        err = loss - expected
        out[name] = {"loss": loss, "err": err, "withinTol": abs(err) <= native_tol(expected)}
    return out


# ------------------------------------------------------------------ 3. extra cases proposed for the corpus

def mk(f, d, c, pol, er, sg, lo=1.0, hi=450000.0, pv="TEST_ONLY"):
    return {"frequencyMHz": f, "thicknessM": d, "cosIncidence": c, "polarization": pol,
            "material": {"epsilonReal": er, "conductivitySm": sg, "minMHz": lo, "maxMHz": hi, "provenance": pv}}


EXTRA = [
    ("E01", "native", "f*d tie-to-even: 3 * f32(500/3) = 500 + 2^-16 in binary64, exactly 500 in binary32 -> OK",
     mk(3.0, 166.6666717529297, 1.0, "TE", 4.0, 0.0)),
    ("E02", "native", "f*d rounds down: 76 * f32(500/76) = 500.0000134 in binary64, 500 in binary32 -> OK",
     mk(76.0, 6.5789475440979, 1.0, "TE", 4.0, 0.0)),
    ("E03", "native", "f*d tie-to-even, second pair: 7 * f32(500/7)", mk(7.0, 71.42857360839844, 1.0, "TE", 4.0, 0.0)),
    ("E04", "native", "precedence: outer invalid (f = 0) and material null -> INVALID_INPUT",
     {"frequencyMHz": 0, "thicknessM": 0.2, "cosIncidence": 1.0, "polarization": "TE", "material": None}),
    ("E05", "native", "precedence: profile invalid (eps 0.5) and f = 0.5 out of domain -> INVALID_PROFILE",
     mk(0.5, 0.2, 1.0, "TE", 0.5, 0.0)),
    ("E06", "native", "precedence: f = 0.5 out of domain and f*d = 1000 -> OUT_OF_MODEL_DOMAIN",
     mk(0.5, 2000.0, 1.0, "TE", 4.0, 0.0)),
    ("E07", "native", "precedence: f outside [min,max] and eps > 100 -> OUT_OF_MODEL_DOMAIN",
     mk(100.0, 0.2, 1.0, "TE", 100.00000762939453, 0.0, lo=200.0, hi=300.0)),
    ("E08", "native", "cosIncidence = 0 is structurally valid, numerically refused -> NUMERIC_DOMAIN_LIMIT",
     mk(100.0, 0.2, 0.0, "TE", 4.0, 0.0)),
    ("E09", "json", "binary32 overflow 1e39 -> INVALID_INPUT", mk(1e39, 0.2, 1.0, "TE", 4.0, 0.0)),
    ("E10", "json", "3.4028236e38 rounds to Inf -> INVALID_INPUT", mk(3.4028236e38, 0.2, 1.0, "TE", 4.0, 0.0)),
    ("E11", "json", "3.4028235e38 rounds to FLT_MAX -> OUT_OF_MODEL_DOMAIN", mk(3.4028235e38, 0.2, 1.0, "TE", 4.0, 0.0)),
    ("E12", "native", "MS06 corner: f*d = 500, eps 100, lossless -> y = 104.8 rad, native trig at large argument",
     mk(100.0, 5.0, 1.0, "TE", 100.0, 0.0)),
    ("E13", "native", "thin sheet with W ~ A (L ~ 7 dB): catches naive 1 - r^2", mk(30.0, 7e-11, 1.0, "TE", 1.0, 1e8)),
    ("E14", "native", "thin sheet with x ~ 1e-8: catches naive 1 - exp(-2x)", mk(1.0, 5e-13, 1.0, "TE", 1.0, 1e8)),
    ("E15", "native", "TM conductor sheet at grazing: a = eC huge", mk(30.0, 1e-12, 0.05, "TM", 1.0, 1e8)),
    ("E16", "native", "thin lossy dielectric, x ~ 2e-5", mk(100.0, 1e-6, 1.0, "TE", 4.0, 1.0)),
    ("E17", "json", "negative zero thickness -> OK 0", mk(100.0, -0.0, 1.0, "TE", 4.0, 0.0)),
]


def main(corpus, out=None):
    raw = Path(corpus).read_bytes()
    doc = json.loads(raw)
    rows, bad, maxdiff, emu = [], 0, 0.0, {}
    for case in doc["cases"]:
        got = evaluate(case["input"])
        exp = case["expect"]
        ok = got["code"] == exp["code"] and got.get("provenance") == exp.get("provenance")
        diff = None
        if ok and "lossDb" in exp:
            diff = abs(got["lossDb"] - exp["lossDb"])
            tol = max(1e-8, 1e-12 * abs(exp["lossDb"]))
            ok = diff <= tol
            maxdiff = max(maxdiff, diff / tol)
            if case["native"]:
                emu[case["id"]] = {"expected": exp["lossDb"], "variants": emulate(case["input"], exp["lossDb"])}
        bad += not ok
        rows.append({"id": case["id"], "native": case["native"], "ok": ok, "expect": exp, "got": got, "absDiff": diff})
    extras = []
    for cid, kind, note, inp in EXTRA:
        got = evaluate(inp)
        row = {"id": cid, "kind": kind, "note": note, "input": inp, "got": got}
        if got["code"] == "OK" and kind == "native":
            row["emulation"] = emulate(inp, got["lossDb"])
        extras.append(row)
    inv = invariants()
    report = {"checker": "claude p3-2a-independent-req-check", "corpusSha256": hashlib.sha256(raw).hexdigest(),
              "corpusLfSha256": hashlib.sha256(raw.replace(bytes([13, 10]), bytes([10]))).hexdigest(),
              "cases": len(rows), "mismatches": bad, "maxDiffOverTol": maxdiff, "invariants": inv,
              "emulationNative": emu, "extras": extras, "rows": rows}
    if out:
        Path(out).write_text(json.dumps(report, ensure_ascii=False, indent=1) + "\n", encoding="utf-8", newline="\n")
    print(f"cases {len(rows)} mismatches {bad} maxDiff/tol {maxdiff:.3g}")
    for r in rows:
        if not r["ok"]:
            print("MISMATCH", r["id"], r["expect"], r["got"])
    for k, v in inv.items():
        print("INV", k, v)
    print("EMULATION (native OK cases): variant -> worst |err|/tol, cases outside tolerance")
    for name in VARIANTS:
        worst, outside = 0.0, []
        for cid, res in emu.items():
            v = res["variants"][name]
            if v.get("failure"):
                outside.append(cid + ":FAIL")
                continue
            ratio = abs(v["err"]) / native_tol(res["expected"])
            worst = max(worst, ratio)
            if not v["withinTol"]:
                outside.append(f"{cid}:{v['err']:+.3g}")
        print(f"  {name:9s} worst {worst:.3g}  outside {outside}")
    print("EXTRAS")
    for row in extras:
        line = f"  {row['id']} {row['kind']:6s} {row['got']}"
        if "emulation" in row:
            line += "  f32 " + " ".join(f"{n}:{v['err']:+.2e}{'' if v['withinTol'] else '!'}" if not v.get("failure")
                                        else f"{n}:FAIL" for n, v in row["emulation"].items())
        print(line, "|", row["note"])
    print("P3_2A_BLIND_CHECK_PASS" if bad == 0 else "P3_2A_BLIND_CHECK_FAIL")
    return 0 if bad == 0 else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1], sys.argv[2] if len(sys.argv) > 2 else None))
