#!/usr/bin/env python3
"""claude: independent check of a P3.2a Workbench log against the frozen corpus (REQ-material-slab v0.2, rev2).

Re-reads [RICON_SLAB] lines: code, loss (finite, >= 0, within max(0.05 dB, 2e-6|L|)), provenance (NONE on refusal),
match and immutable flags per native case in corpus order, host-only absence, source LF-SHA, three [RICON_SLAB_TETM]
ok lines, the summary counts with a finite maxDeviationDb, one done: PASS, no SCRIPT (E), no tagged FAIL.
NaN never passes: comparisons with NaN are false, so finiteness is checked explicitly (codex #3837).
Usage: python -I p3-2a-log-check-claude.py <console.log> <material_slab_v1.json>
       python -I p3-2a-log-check-claude.py --self-check <material_slab_v1.json>   (synthetic log + mutations)"""
import hashlib
import json
import math
import re
import sys

CORPUS_LF_SHA = "2f5a4f10f5e1e472ea9eb253bc07230c806ad908002d59ab4db19b90cce1efbd"
CASE = re.compile(r"\[RICON_SLAB\] id=(MS\d{3}) code=(\w+) loss=(\S+) provenance=(\w+) match=(\w+) immutable=(\w+)$")
SUMMARY = re.compile(r"\[RICON_SLAB\] cases=71 match=71 immutable=71 controls=5 extras=3 hostOnly=9 maxDeviationDb=(\S+)$")


def tolerance(expected):
    return max(0.05, 2e-6 * abs(expected))


def load_corpus(path):
    raw = open(path, "rb").read().replace(bytes([13, 10]), bytes([10]))
    return raw, json.loads(raw)


def check(lines, raw, doc):
    """Returns (errors, worst deviation)."""
    errors = [] if hashlib.sha256(raw).hexdigest() == CORPUS_LF_SHA else ["corpus LF-SHA"]
    native = [c for c in doc["cases"] if c["native"]]
    host_only = {c["id"] for c in doc["cases"] if not c["native"]}
    if any(re.search(r"SCRIPT\s*\(E\)", l) for l in lines):
        errors.append("SCRIPT (E)")
    if [l for l in lines if re.search(r"\[RICON_[^\]]+\].*(?<![A-Za-z])FAIL(?![A-Za-z])", l)]:
        errors.append("tagged FAIL")
    if sum("[RICON_SLAB] done: PASS" in l for l in lines) != 1:
        errors.append("done: PASS count")
    if not any(f"[RICON_SLAB] source={CORPUS_LF_SHA}" in l for l in lines):
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
    worst = 0.0
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
        code, loss, provenance, match, immutable = got
        if code != want["code"] or match != "1" or immutable != "1":
            errors.append(f"{cid} {got} != {want}")
            continue
        if want["code"] != "OK":
            if provenance != "NONE":
                errors.append(f"{cid} provenance printed on refusal")
            continue
        try:
            actual = float(loss)
        except ValueError:
            errors.append(f"{cid} loss not numeric: {loss}")
            continue
        if not math.isfinite(actual) or actual < 0:
            errors.append(f"{cid} loss not finite or negative: {loss}")
            continue
        expected = want["lossDb"]
        deviation = abs(actual - expected)
        worst = max(worst, deviation)
        if provenance != want["provenance"] or not deviation <= tolerance(expected):
            errors.append(f"{cid} loss {actual} vs {expected} (dev {deviation:.3g}) provenance {provenance}")
    tetm = [l for l in lines if "[RICON_SLAB_TETM]" in l]
    if len(tetm) != 3 or any(not l.rstrip().endswith("ok=1") for l in tetm):
        errors.append("TETM lines")
    printed = [SUMMARY.search(l).group(1) for l in lines if SUMMARY.search(l)]
    if len(printed) != 1:
        errors.append("summary")
    else:
        try:
            dev = float(printed[0])
            if not math.isfinite(dev) or dev < 0 or dev > max(tolerance(c["expect"].get("lossDb", 0)) for c in native):
                errors.append(f"summary maxDeviationDb {printed[0]}")
        except ValueError:
            errors.append(f"summary maxDeviationDb {printed[0]}")
    return errors, worst


def synthetic(doc):
    """A log that prints exactly the frozen expectations (binary64 text, 9 decimals)."""
    lines = [f"[RICON_SLAB] source={CORPUS_LF_SHA}"]
    for c in doc["cases"]:
        if not c["native"]:
            continue
        e = c["expect"]
        loss = f"{e['lossDb']:.9f}" if e["code"] == "OK" else "0.000000000"
        prov = e["provenance"] if e["code"] == "OK" else "NONE"
        lines.append(f"[RICON_SLAB] id={c['id']} code={e['code']} loss={loss} provenance={prov} match=1 immutable=1")
    lines += ["[RICON_SLAB_TETM] eps=5.300000191 sigma=0.029999999 d=0.200000003 te=5.624226570 tm=5.624227047 ok=1",
              "[RICON_SLAB_TETM] eps=4.000000000 sigma=1.000000000 d=0.050000001 te=27.347900391 tm=27.347900391 ok=1",
              "[RICON_SLAB_TETM] eps=100.000000000 sigma=0.500000000 d=0.100000001 te=16.872211456 tm=16.872211456 ok=1",
              "[RICON_SLAB] cases=71 match=71 immutable=71 controls=5 extras=3 hostOnly=9 maxDeviationDb=0.000000000",
              "[RICON_SLAB] done: PASS"]
    return lines


def mutate(lines, cid, field, value):
    out = []
    for l in lines:
        m = CASE.search(l)
        if m and m.group(1) == cid:
            parts = dict(zip(("code", "loss", "provenance", "match", "immutable"), m.groups()[1:]))
            parts[field] = value
            l = (f"[RICON_SLAB] id={cid} code={parts['code']} loss={parts['loss']} provenance={parts['provenance']} "
                 f"match={parts['match']} immutable={parts['immutable']}")
        out.append(l)
    return out


def self_check(raw, doc):
    base = synthetic(doc)
    errors, _ = check(base, raw, doc)
    if errors:
        print("SELF_CHECK_BASE_FAIL", errors)
        return 1
    mutations = {
        "MS003 loss nan": mutate(base, "MS003", "loss", "nan"),
        "MS003 loss -nan(ind)": mutate(base, "MS003", "loss", "-nan(ind)"),
        "MS003 loss inf": mutate(base, "MS003", "loss", "inf"),
        "MS003 loss negative": mutate(base, "MS003", "loss", "-0.100000000"),
        "MS003 loss out of tolerance": mutate(base, "MS003", "loss", "2.000000000"),
        "MS044 loss out of relative tolerance": mutate(base, "MS044", "loss", "345301.500000000"),
        "MS003 code": mutate(base, "MS003", "code", "NUMERIC_FAILURE"),
        "MS029 code": mutate(base, "MS029", "code", "OK"),
        "MS003 provenance": mutate(base, "MS003", "provenance", "ASSUMED_CONFIG"),
        "MS029 provenance on refusal": mutate(base, "MS029", "provenance", "TEST_ONLY"),
        "MS003 match 0": mutate(base, "MS003", "match", "0"),
        "MS003 immutable 0": mutate(base, "MS003", "immutable", "0"),
        "missing MS080": [l for l in base if "id=MS080 " not in l],
        "duplicate MS001": base[:2] + [base[1]] + base[2:],
        "host-only printed": base[:2] + ["[RICON_SLAB] id=MS054 code=INVALID_INPUT loss=0.000000000 provenance=NONE match=1 immutable=1"] + base[2:],
        "wrong order": [base[0], base[2], base[1]] + base[3:],
        "source sha": [base[0].replace("2f5a4f10", "deadbeef")] + base[1:],
        "no done": [l for l in base if "done: PASS" not in l],
        "two done": base + ["[RICON_SLAB] done: PASS"],
        "script error": base + ['SCRIPT    (E): @"x.c,1": boom'],
        "tagged FAIL": base + ["[RICON_TEST] FAIL MS003: x"],
        "TETM ok=0": [l.replace("ok=1", "ok=0") if "TETM" in l else l for l in base],
        "summary counts": [l.replace("match=71", "match=70") if "cases=71" in l else l for l in base],
        "summary maxDeviation nan": [l.replace("maxDeviationDb=0.000000000", "maxDeviationDb=nan") if "cases=71" in l else l for l in base],
        "summary maxDeviation too large": [l.replace("maxDeviationDb=0.000000000", "maxDeviationDb=1.000000000") if "cases=71" in l else l for l in base],
    }
    accepted = [name for name, lines in mutations.items() if not check(lines, raw, doc)[0]]
    if accepted:
        print("SELF_CHECK_FAIL: mutations accepted:", accepted)
        return 1
    print(f"P3_2A_LOG_SELFCHECK_PASS: synthetic log accepted, {len(mutations)} mutations rejected")
    return 0


def main(argv):
    if argv[0] == "--self-check":
        raw, doc = load_corpus(argv[1])
        return self_check(raw, doc)
    log_path, corpus_path = argv[0], argv[1]
    raw, doc = load_corpus(corpus_path)
    log_bytes = open(log_path, "rb").read()
    lines = log_bytes.decode("utf-8", errors="replace").splitlines()
    errors, worst = check(lines, raw, doc)
    if errors:
        print("\n".join(errors))
        return 1
    print(f"P3_2A_LOG_CHECK_PASS: 71 native, host-only 9 absent, SCRIPT(E) 0, worst |dev| {worst:.3g} dB, "
          f"log sha256 {hashlib.sha256(log_bytes).hexdigest()}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
