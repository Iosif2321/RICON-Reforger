#!/usr/bin/env python3
"""claude: independent check of a P3.2b Workbench log against the frozen corpus (REQ-material-profile v0.2, rev2).

Re-reads [RICON_MPROF] lines: code, binary32 bit patterns of epsilonReal / conductivitySm / minMHz / maxMHz,
provenance (all NONE on refusal), match and immutable flags per native case in corpus order, host-only absence,
source LF-SHA; [RICON_MPROF_CHAIN] lines: exactly one per native OK case with sigma > 0, in corpus order, thickness
rule d = 0.1 m (f <= 2000 MHz) else 200/f in binary32, slab=OK with finite loss >= 0 (within MS08 of the accepted slab
oracle when --slab-oracle <oracle dir> is given), shifted=OUT_OF_MODEL_DOMAIN, ok=1; [RICON_MPROF_POWER] lines 1..4 (codex #3980/#3981/#3995: id, code
and ok exact, sigma bits decoded and compared with the MP07 tolerance, NONE on refusal); the summary counts with a
finite maxRelativeError <= 2e-5; one done: PASS; no SCRIPT (E); no tagged FAIL. Decoded numbers are compared with the
frozen binary64 expectations: relative error <= 2e-5 for a positive expectation, zero exact (MP07 native tolerance,
no positive sigma replaced by zero), minMHz/maxMHz bits exact.
NaN or Inf never passes: bit patterns must decode to finite binary32, loss is checked for finiteness explicitly.
Usage: python -I p3-2b-log-check-claude.py <console.log> <material_profile_v1.json> [--slab-oracle <oracle dir>]
       python -I p3-2b-log-check-claude.py --self-check <material_profile_v1.json> [--slab-oracle <oracle dir>]"""
import hashlib
import importlib.util
import json
import math
import os
import re
import struct
import sys

CORPUS_LF_SHA = "36f854eaa5cc2b9e1dc10d8ee6e158f4db55f2b4ae6bee7ccfcc38042135ebc2"
TOLERANCE = 2e-5
CASE = re.compile(r"\[RICON_MPROF\] id=(MP\d{3}) code=(\w+) epsBits=(\S+) sigmaBits=(\S+) minBits=(\S+) maxBits=(\S+) "
                  r"provenance=(\w+) match=(\w+) immutable=(\w+)$")
CHAIN = re.compile(r"\[RICON_MPROF_CHAIN\] fBits=(\S+) dBits=(\S+) epsBits=(\S+) sigmaBits=(\S+) slab=(\w+) loss=(\S+) "
                   r"shifted=(\w+) ok=(\w+)$")
POWER = re.compile(r"\[RICON_MPROF_POWER\] id=(\d+) code=(\w+) sigmaBits=(\S+) ok=(\w+)$")
SUMMARY = re.compile(r"\[RICON_MPROF\] cases=(\d+) match=(\d+) immutable=(\d+) controls=(\d+) extras=(\d+) hostOnly=(\d+) "
                     r"chains=(\d+) maxRelativeError=(\S+)$")
# (id, code, expected sigma as binary64 -> compared through decode + close; None = refusal, sigmaBits must be NONE)
POWER_EXPECTED = (("1", "OK", 1.401298464324817e-45), ("2", "OK", 9.999999406318238e-08),
                  ("3", "NUMERIC_DOMAIN_LIMIT", None), ("4", "NUMERIC_DOMAIN_LIMIT", None))
EXTRAS = 7  # null/restore, chain group, reverse repeat, 4 Power counterexamples
NONE5 = ("NONE",) * 5


def f32(x):
    return struct.unpack("<f", struct.pack("<f", x))[0]


def bits_of(x):
    return struct.unpack("<I", struct.pack("<f", x))[0]


def decode(text):
    """uint31 decimal bit pattern -> finite binary32, else None."""
    if not re.fullmatch(r"\d{1,10}", text) or int(text) >= 2 ** 31:
        return None
    value = struct.unpack("<f", struct.pack("<I", int(text)))[0]
    return value if math.isfinite(value) else None


def close(actual, expected):
    if expected == 0:
        return actual == 0
    return actual > 0 and abs(actual - expected) / expected <= TOLERANCE


def thickness(frequency):
    """Chain rule of the native support: binary32 0.1 up to 2000 MHz, else binary32(200 / f)."""
    return f32(0.1) if frequency <= 2000 else f32(200.0 / frequency)


def slab_tolerance(expected):
    return max(0.05, 2e-6 * abs(expected))


def load_slab_oracle(directory):
    sys.path.insert(0, directory)
    spec = importlib.util.spec_from_file_location("material_slab_oracle", os.path.join(directory, "material_slab_oracle.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module.evaluate


def slab_loss(slab, frequency, depth, epsilon, sigma, provenance):
    result = slab({"frequencyMHz": frequency, "thicknessM": depth, "cosIncidence": 1.0, "polarization": "TE",
                   "material": {"epsilonReal": epsilon, "conductivitySm": sigma, "minMHz": frequency, "maxMHz": frequency,
                                "provenance": provenance}})
    return result.get("lossDb") if result.get("code") == "OK" else None


def load_corpus(path):
    raw = open(path, "rb").read().replace(bytes([13, 10]), bytes([10]))
    return raw, json.loads(raw)


def chain_cases(doc):
    return [c for c in doc["cases"] if c["native"] and c["expect"]["code"] == "OK" and c["expect"]["material"]["conductivitySm"] > 0]


def check(lines, raw, doc, slab=None):
    """Returns (errors, worst relative error of eps/sigma, worst |loss deviation| against the slab oracle)."""
    errors = [] if hashlib.sha256(raw).hexdigest() == CORPUS_LF_SHA else ["corpus LF-SHA"]
    native = [c for c in doc["cases"] if c["native"]]
    host_only = {c["id"] for c in doc["cases"] if not c["native"]}
    chained = chain_cases(doc)
    if any(re.search(r"SCRIPT\s*\(E\)", l) for l in lines):
        errors.append("SCRIPT (E)")
    if [l for l in lines if re.search(r"\[RICON_[^\]]+\].*(?<![A-Za-z])FAIL(?![A-Za-z])", l)]:
        errors.append("tagged FAIL")
    if sum("[RICON_MPROF] done: PASS" in l for l in lines) != 1:
        errors.append("done: PASS count")
    if not any(f"[RICON_MPROF] source={CORPUS_LF_SHA}" in l for l in lines):
        errors.append("source line")
    seen, order = {}, []
    for l in lines:
        m = CASE.search(l)
        if m:
            if m.group(1) in seen:
                errors.append(f"dup {m.group(1)}")
            seen[m.group(1)] = m.groups()[1:]
            order.append(m.group(1))
    if order != [c["id"] for c in native]:
        errors.append("case order")
    worst, decoded = 0.0, {}
    for case in doc["cases"]:
        cid, want = case["id"], case["expect"]
        if cid in host_only:
            if cid in seen:
                errors.append(f"{cid} host-only printed")
            continue
        got = seen.get(cid)
        if not got:
            errors.append(f"{cid} missing")
            continue
        code, eps_b, sig_b, lo_b, hi_b, provenance, match, immutable = got
        if code != want["code"] or match != "1" or immutable != "1":
            errors.append(f"{cid} {got} != {want['code']}")
            continue
        if want["code"] != "OK":
            if (eps_b, sig_b, lo_b, hi_b, provenance) != NONE5:
                errors.append(f"{cid} fields printed on refusal")
            continue
        m = want["material"]
        eps, sig, lo, hi = decode(eps_b), decode(sig_b), decode(lo_b), decode(hi_b)
        if None in (eps, sig, lo, hi):
            errors.append(f"{cid} bits not finite binary32: {got}")
            continue
        if provenance != m["provenance"]:
            errors.append(f"{cid} provenance {provenance} != {m['provenance']}")
        if int(lo_b) != bits_of(f32(m["minMHz"])) or int(hi_b) != bits_of(f32(m["maxMHz"])):
            errors.append(f"{cid} minMHz/maxMHz bits {lo_b}/{hi_b}")
        for name, actual, expected in (("epsilonReal", eps, m["epsilonReal"]), ("conductivitySm", sig, m["conductivitySm"])):
            if not close(actual, expected):
                errors.append(f"{cid} {name} {actual!r} vs {expected!r}")
            elif expected:
                worst = max(worst, abs(actual - expected) / expected)
        decoded[cid] = (eps_b, sig_b, provenance)
    chains = [CHAIN.search(l).groups() for l in lines if CHAIN.search(l)]
    if len(chains) != len(chained):
        errors.append(f"chain count {len(chains)} != {len(chained)}")
    worst_loss = 0.0
    for case, got in zip(chained, chains):
        cid = case["id"]
        f_b, d_b, eps_b, sig_b, same, loss_text, shifted, ok = got
        frequency = f32(case["input"]["frequencyMHz"])
        if f_b != str(bits_of(frequency)) or d_b != str(bits_of(thickness(frequency))):
            errors.append(f"chain {cid} fBits/dBits {f_b}/{d_b}")
        if cid in decoded and (eps_b, sig_b) != decoded[cid][:2]:
            errors.append(f"chain {cid} material bits differ from the case line")
        if same != "OK" or shifted != "OUT_OF_MODEL_DOMAIN" or ok != "1":
            errors.append(f"chain {cid} slab={same} shifted={shifted} ok={ok}")
        try:
            loss = float(loss_text)
        except ValueError:
            errors.append(f"chain {cid} loss not numeric: {loss_text}")
            continue
        if not math.isfinite(loss) or loss < 0:
            errors.append(f"chain {cid} loss not finite or negative: {loss_text}")
            continue
        if slab and cid in decoded:
            expected = slab_loss(slab, frequency, thickness(frequency), decode(eps_b), decode(sig_b), decoded[cid][2])
            if expected is None:
                errors.append(f"chain {cid} slab oracle refused the logged material")
                continue
            deviation = abs(loss - expected)
            worst_loss = max(worst_loss, deviation)
            if not deviation <= slab_tolerance(expected):
                errors.append(f"chain {cid} loss {loss} vs oracle {expected} (dev {deviation:.3g})")
    powers = [POWER.search(l).groups() for l in lines if POWER.search(l)]
    if len(powers) != len(POWER_EXPECTED):
        errors.append(f"power lines {powers}")
    for (pid, code, sigma), got in zip(POWER_EXPECTED, powers):
        gid, gcode, bits, ok = got
        if gid != pid or gcode != code or ok != "1":
            errors.append(f"power {pid} {got}")
        elif sigma is None:
            if bits != "NONE":
                errors.append(f"power {pid} bits printed on refusal")
        else:
            value = decode(bits)
            if value is None or not close(value, sigma):
                errors.append(f"power {pid} sigma bits {bits} vs {sigma!r}")
    printed = [SUMMARY.search(l).groups() for l in lines if SUMMARY.search(l)]
    if len(printed) != 1:
        errors.append("summary")
    else:
        counts = tuple(int(v) for v in printed[0][:7])
        if counts != (len(native), len(native), len(native), 5, EXTRAS, len(host_only), len(chained)):
            errors.append(f"summary counts {counts}")
        try:
            rel = float(printed[0][7])
            if not math.isfinite(rel) or rel < 0 or rel > TOLERANCE:
                errors.append(f"summary maxRelativeError {printed[0][7]}")
        except ValueError:
            errors.append(f"summary maxRelativeError {printed[0][7]}")
    return errors, worst, worst_loss


def case_line(cid, code, eps_b, sig_b, lo_b, hi_b, provenance, match="1", immutable="1"):
    return (f"[RICON_MPROF] id={cid} code={code} epsBits={eps_b} sigmaBits={sig_b} minBits={lo_b} maxBits={hi_b} "
            f"provenance={provenance} match={match} immutable={immutable}")


def chain_line(f_b, d_b, eps_b, sig_b, same="OK", loss="0.000000000", shifted="OUT_OF_MODEL_DOMAIN", ok="1"):
    return f"[RICON_MPROF_CHAIN] fBits={f_b} dBits={d_b} epsBits={eps_b} sigmaBits={sig_b} slab={same} loss={loss} shifted={shifted} ok={ok}"


def synthetic(doc, slab=None):
    """A log that prints exactly the frozen expectations as binary32 bit patterns."""
    native = [c for c in doc["cases"] if c["native"]]
    lines = [f"[RICON_MPROF] source={CORPUS_LF_SHA}"]
    for c in native:
        e = c["expect"]
        if e["code"] == "OK":
            m = e["material"]
            lines.append(case_line(c["id"], "OK", bits_of(f32(m["epsilonReal"])), bits_of(f32(m["conductivitySm"])),
                                   bits_of(f32(m["minMHz"])), bits_of(f32(m["maxMHz"])), m["provenance"]))
        else:
            lines.append(case_line(c["id"], e["code"], *NONE5))
    for c in chain_cases(doc):
        m = c["expect"]["material"]
        frequency = f32(c["input"]["frequencyMHz"])
        epsilon, sigma = f32(m["epsilonReal"]), f32(m["conductivitySm"])
        loss = slab_loss(slab, frequency, thickness(frequency), epsilon, sigma, m["provenance"]) if slab else 0.0
        lines.append(chain_line(bits_of(frequency), bits_of(thickness(frequency)), bits_of(epsilon), bits_of(sigma), loss=f"{loss:.9f}"))
    lines += [f"[RICON_MPROF_POWER] id={i} code={c} sigmaBits={'NONE' if s is None else bits_of(s)} ok=1" for i, c, s in POWER_EXPECTED]
    host_only = sum(not c["native"] for c in doc["cases"])
    lines.append(f"[RICON_MPROF] cases={len(native)} match={len(native)} immutable={len(native)} controls=5 extras={EXTRAS} "
                 f"hostOnly={host_only} chains={len(chain_cases(doc))} maxRelativeError=0.000000000")
    lines.append("[RICON_MPROF] done: PASS")
    return lines


def mutate_case(lines, cid, field, value):
    names = ("code", "epsBits", "sigmaBits", "minBits", "maxBits", "provenance", "match", "immutable")
    out = []
    for l in lines:
        m = CASE.search(l)
        if m and m.group(1) == cid:
            parts = dict(zip(names, m.groups()[1:]))
            parts[field] = value
            l = case_line(cid, *(parts[n] for n in names))
        out.append(l)
    return out


def mutate_chain(lines, index, field, value):
    names = ("fBits", "dBits", "epsBits", "sigmaBits", "slab", "loss", "shifted", "ok")
    out, k = [], 0
    for l in lines:
        m = CHAIN.search(l)
        if m:
            if k == index:
                parts = dict(zip(names, m.groups()))
                parts[field] = value
                l = chain_line(*(parts[n] for n in names))
            k += 1
        out.append(l)
    return out


def shift_bits(lines, cid, field, delta):
    m = [CASE.search(l) for l in lines if CASE.search(l) and CASE.search(l).group(1) == cid][0]
    index = ("code", "epsBits", "sigmaBits", "minBits", "maxBits").index(field)
    return mutate_case(lines, cid, field, str(int(m.groups()[1 + index]) + delta))


def self_check(raw, doc, slab=None):
    base = synthetic(doc, slab)
    errors, _, _ = check(base, raw, doc, slab)
    if errors:
        print("SELF_CHECK_BASE_FAIL", errors)
        return 1
    ok_case = chain_cases(doc)[0]["id"]           # first native OK case with sigma > 0 (MP002)
    refusal = next(c["id"] for c in doc["cases"] if c["native"] and c["expect"]["code"] != "OK")
    subnormal = next(c["id"] for c in doc["cases"] if c["native"] and c["expect"]["code"] == "OK"
                     and 0 < c["expect"]["material"]["conductivitySm"] < 1e-38)
    high = next(i for i, c in enumerate(chain_cases(doc)) if c["input"]["frequencyMHz"] > 2000)
    power2 = bits_of(POWER_EXPECTED[1][2])
    # positive control (codex #4000): Power 2 one ulp above the host value is within the MP07 tolerance and must pass
    plus_one = [l.replace(f"id=2 code=OK sigmaBits={power2} ", f"id=2 code=OK sigmaBits={power2 + 1} ") for l in base]
    if check(plus_one, raw, doc, slab)[0]:
        print("SELF_CHECK_FAIL: Power 2 at +1 ulp rejected")
        return 1
    mutations = {
        f"{ok_case} code": mutate_case(base, ok_case, "code", "NUMERIC_FAILURE"),
        f"{refusal} code OK": mutate_case(base, refusal, "code", "OK"),
        f"{ok_case} epsBits +1000 ulp": shift_bits(base, ok_case, "epsBits", 1000),
        f"{ok_case} sigmaBits -1000 ulp": shift_bits(base, ok_case, "sigmaBits", -1000),
        f"{ok_case} sigma zero": mutate_case(base, ok_case, "sigmaBits", "0"),
        f"{subnormal} sigma 2 ulp (100 %)": mutate_case(base, subnormal, "sigmaBits", "2"),
        f"{ok_case} minBits +1": shift_bits(base, ok_case, "minBits", 1),
        f"{ok_case} maxBits -1": shift_bits(base, ok_case, "maxBits", -1),
        f"{ok_case} epsBits nonfinite": mutate_case(base, ok_case, "epsBits", "2139095040"),
        f"{ok_case} epsBits text": mutate_case(base, ok_case, "epsBits", "nonfinite"),
        f"{ok_case} provenance": mutate_case(base, ok_case, "provenance", "ASSUMED_CONFIG"),
        f"{refusal} provenance printed": mutate_case(base, refusal, "provenance", "TEST_ONLY"),
        f"{refusal} bits printed": mutate_case(base, refusal, "sigmaBits", "0"),
        f"{ok_case} match 0": mutate_case(base, ok_case, "match", "0"),
        f"{ok_case} immutable 0": mutate_case(base, ok_case, "immutable", "0"),
        f"missing {ok_case}": [l for l in base if f"id={ok_case} " not in l],
        "duplicate MP001": base[:2] + [base[1]] + base[2:],
        "host-only printed": base[:2] + [case_line(next(c["id"] for c in doc["cases"] if not c["native"]), "INVALID_INPUT", *NONE5)] + base[2:],
        "wrong order": [base[0], base[2], base[1]] + base[3:],
        "source sha": [base[0].replace("36f854ea", "deadbeef")] + base[1:],
        "no done": [l for l in base if "done: PASS" not in l],
        "two done": base + ["[RICON_MPROF] done: PASS"],
        "script error": base + ['SCRIPT    (E): @"x.c,1": boom'],
        "tagged FAIL": base + ["[RICON_TEST] FAIL MP002: x"],
        "chain missing": [l for i, l in enumerate(base) if not (CHAIN.search(l) and i == next(k for k, x in enumerate(base) if CHAIN.search(x)))],
        "chain ok 0": mutate_chain(base, 0, "ok", "0"),
        "chain slab refused": mutate_chain(base, 0, "slab", "NUMERIC_DOMAIN_LIMIT"),
        "chain shifted OK": mutate_chain(base, 0, "shifted", "OK"),
        "chain thickness rule": mutate_chain(base, high, "dBits", str(bits_of(f32(0.1)))),
        "chain fBits": mutate_chain(base, 0, "fBits", str(bits_of(f32(2.0)))),
        "chain material bits": mutate_chain(base, 0, "sigmaBits", "0"),
        "chain loss nan": mutate_chain(base, 0, "loss", "nan"),
        "chain loss -nan(ind)": mutate_chain(base, 0, "loss", "-nan(ind)"),
        "chain loss negative": mutate_chain(base, 0, "loss", "-0.100000000"),
        "chain loss text": mutate_chain(base, 0, "loss", "out_of_print_range"),
        "power code": [l.replace("id=3 code=NUMERIC_DOMAIN_LIMIT", "id=3 code=NUMERIC_FAILURE") for l in base],
        "power 4 code OK": [l.replace("id=4 code=NUMERIC_DOMAIN_LIMIT", "id=4 code=OK") for l in base],
        "power 1 sigma zero": [l.replace("id=1 code=OK sigmaBits=1 ", "id=1 code=OK sigmaBits=0 ") for l in base],
        "power 1 sigma 2 ulp (100 %)": [l.replace("id=1 code=OK sigmaBits=1 ", "id=1 code=OK sigmaBits=2 ") for l in base],
        "power 2 sigma +1000 ulp": [l.replace(f"sigmaBits={power2} ", f"sigmaBits={power2 + 1000} ") for l in base],
        "power 3 bits printed": [l.replace("id=3 code=NUMERIC_DOMAIN_LIMIT sigmaBits=NONE", "id=3 code=NUMERIC_DOMAIN_LIMIT sigmaBits=0") for l in base],
        "power ok 0": [l.replace(f"sigmaBits={power2} ok=1", f"sigmaBits={power2} ok=0") for l in base],
        "power missing": [l for l in base if "_POWER] id=2 " not in l],
        "summary counts": [l.replace(f"extras={EXTRAS}", f"extras={EXTRAS - 1}") if "cases=" in l else l for l in base],
        "summary chains": [l.replace(f"chains={len(chain_cases(doc))}", "chains=3") if "cases=" in l else l for l in base],
        "summary maxRelativeError nan": [l.replace("maxRelativeError=0.000000000", "maxRelativeError=nan") if "cases=" in l else l for l in base],
        "summary maxRelativeError too large": [l.replace("maxRelativeError=0.000000000", "maxRelativeError=0.001000000") if "cases=" in l else l for l in base],
    }
    if slab:
        mutations["chain loss out of MS08 tolerance"] = mutate_chain(base, 0, "loss", f"{float(CHAIN.search([l for l in base if CHAIN.search(l)][0]).group(6)) + 1.0:.9f}")
    accepted = [name for name, lines in mutations.items() if not check(lines, raw, doc, slab)[0]]
    if accepted:
        print("SELF_CHECK_FAIL: mutations accepted:", accepted)
        return 1
    print(f"P3_2B_LOG_SELFCHECK_PASS: synthetic log accepted, {len(mutations)} mutations rejected, slab oracle {'on' if slab else 'off'}")
    return 0


def main(argv):
    slab = None
    if "--slab-oracle" in argv:
        k = argv.index("--slab-oracle")
        slab = load_slab_oracle(argv[k + 1])
        argv = argv[:k] + argv[k + 2:]
    if argv[0] == "--self-check":
        raw, doc = load_corpus(argv[1])
        return self_check(raw, doc, slab)
    raw, doc = load_corpus(argv[1])
    log_bytes = open(argv[0], "rb").read()
    lines = log_bytes.decode("utf-8", errors="replace").splitlines()
    errors, worst, worst_loss = check(lines, raw, doc, slab)
    if errors:
        print("\n".join(errors))
        return 1
    native = sum(c["native"] for c in doc["cases"])
    print(f"P3_2B_LOG_CHECK_PASS: {native} native, host-only {len(doc['cases']) - native} absent, chains {len(chain_cases(doc))}, "
          f"SCRIPT(E) 0, worst rel {worst:.3g}, worst |loss dev| {worst_loss:.3g} dB (slab oracle {'on' if slab else 'off'}), "
          f"log sha256 {hashlib.sha256(log_bytes).hexdigest()}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
