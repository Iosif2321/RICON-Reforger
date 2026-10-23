"""claude: blind check of REQ-material-profile.md v0.1 (P3.2b) — written from the REQ text and the catalog only.

Does not import codex's material_profile_oracle / builder / tests. Re-implements MP02–MP05 (codes in MP03 order,
binary32 input rounding, stable power form) and additionally evaluates every OK case with the plain direct power
a * f^b (MP04: "прямая степень … служит независимой проверкой"), validates the catalog against MP06, and checks the
REQ / catalog LF-SHA bindings recorded in the corpus.
Usage: python -I -X utf8 p3-2b-independent-req-check.py <corpus.json> <catalog.json> <REQ.md> [out.json]
"""

import hashlib
import json
import math
import re
import struct
import sys
from pathlib import Path

PROV = ("SOURCE_DOMAIN", "ASSUMED_CONFIG", "TEST_ONLY")
PROFILE_KEYS = {"a", "b", "c", "d", "minMHz", "maxMHz", "maxInclusive", "provenance"}
CATALOG_KEYS = {"schema", "version", "source", "note", "profiles"}
ENTRY_KEYS = {"labelRu", "sourceRow", "profile"}
MATERIAL_KEYS = {"epsilonReal", "conductivitySm", "minMHz", "maxMHz", "provenance"}


def f32(v):
    """JSON number -> binary32; None for bool, str, null, NaN, Inf or binary32 overflow (MP02)."""
    if isinstance(v, bool) or not isinstance(v, (int, float)):
        return None
    try:
        fv = float(v)
        return struct.unpack("<f", struct.pack("<f", fv))[0] if math.isfinite(fv) else None
    except OverflowError:
        return None


def no_duplicate_keys(pairs):
    seen = set()
    for k, _ in pairs:
        if k in seen:
            raise ValueError(f"duplicate key {k}")
        seen.add(k)
    return dict(pairs)


def load(path):
    raw = Path(path).read_bytes()
    return raw, json.loads(raw, object_pairs_hook=no_duplicate_keys)


def lf_sha(raw):
    return hashlib.sha256(raw.replace(bytes([13, 10]), bytes([10]))).hexdigest()


def valid_profile(p):
    """MP02 profile rules; returns parsed tuple or None."""
    if not isinstance(p, dict) or set(p) != PROFILE_KEYS:
        return None
    a, b, c, d, lo, hi = (f32(p[k]) for k in ("a", "b", "c", "d", "minMHz", "maxMHz"))
    mi, pv = p["maxInclusive"], p["provenance"]
    if None in (a, b, c, d, lo, hi) or not a > 0 or not c >= 0 or not 0 < lo <= hi:
        return None
    if not isinstance(mi, bool) or (lo == hi and not mi) or not isinstance(pv, str) or pv not in PROV:
        return None
    return a, b, c, d, lo, hi, mi, pv


def power(coefficient, exponent, fghz):
    """MP04: exact cases first, then the stable exp/ln form."""
    if coefficient == 0:
        return 0.0
    if exponent == 0:
        return coefficient
    return math.exp(math.log(coefficient) + exponent * math.log(fghz))


def representable(value):
    """MP03.4: a positive binary64 value must round to a finite, non-zero binary32."""
    r = f32(value)
    return r is not None and r != 0.0


def evaluate(inp, direct=False):
    if not isinstance(inp, dict) or set(inp) != {"frequencyMHz", "profile"}:
        return {"code": "INVALID_INPUT"}
    f = f32(inp["frequencyMHz"])
    if f is None or not f > 0:
        return {"code": "INVALID_INPUT"}
    parsed = valid_profile(inp["profile"])
    if parsed is None:
        return {"code": "INVALID_PROFILE"}
    a, b, c, d, lo, hi, mi, pv = parsed
    if not 1 <= f <= 450000 or f < lo or (f > hi if mi else f >= hi):
        return {"code": "OUT_OF_MODEL_DOMAIN"}
    fghz = f / 1000.0
    try:
        if direct:
            eps = a * fghz ** b if b != 0 else a
            sigma = 0.0 if c == 0 else (c if d == 0 else c * fghz ** d)
        else:
            eps, sigma = power(a, b, fghz), power(c, d, fghz)
    except OverflowError:
        return {"code": "NUMERIC_FAILURE"}
    if not math.isfinite(eps) or not math.isfinite(sigma):
        return {"code": "NUMERIC_FAILURE"}
    # MP03.4: a > 0 always, so eps must stay representable; a positive c must not lose sigma to underflow
    # (binary64 underflow to 0.0 or binary32 rounding to 0.0 both count as loss).
    if not representable(eps) or (c > 0 and not representable(sigma)):
        return {"code": "NUMERIC_FAILURE"}
    e32, s32 = f32(eps), f32(sigma)
    if not 1 <= e32 <= 100 or s32 > 1e8:
        return {"code": "NUMERIC_DOMAIN_LIMIT"}
    return {"code": "OK", "material": {"epsilonReal": e32, "conductivitySm": s32, "minMHz": f, "maxMHz": f,
                                       "provenance": pv}}


def same_number(got, want):
    if got == want:
        return True
    return want != 0 and abs(got - want) <= 1e-12 * abs(want)


def same_result(got, want):
    if got["code"] != want.get("code"):
        return False
    if got["code"] != "OK":
        return set(want) == {"code"}
    m, w = got["material"], want.get("material")
    if not isinstance(w, dict) or set(w) != MATERIAL_KEYS or set(want) != {"code", "material"}:
        return False
    return (w["provenance"] == m["provenance"] and all(same_number(m[k], w[k]) for k in
                                                       ("epsilonReal", "conductivitySm", "minMHz", "maxMHz")))


def check_catalog(doc):
    errors = []
    if not isinstance(doc, dict) or set(doc) != CATALOG_KEYS:
        return ["catalog keys"]
    if doc["schema"] != "ricon.material_profiles.p2040" or doc["version"] != 1 or not isinstance(doc["source"], str):
        errors.append("catalog schema/version/source")
    profiles = doc["profiles"]
    if not isinstance(profiles, dict) or not profiles:
        return errors + ["profiles not a non-empty object"]
    rows = []
    for pid, entry in profiles.items():
        if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9._-]{0,63}", pid):
            errors.append(f"{pid}: id")
        if not isinstance(entry, dict) or set(entry) != ENTRY_KEYS:
            errors.append(f"{pid}: entry keys")
            continue
        if not isinstance(entry["labelRu"], str) or not entry["labelRu"].strip():
            errors.append(f"{pid}: labelRu")
        row = entry["sourceRow"]
        if isinstance(row, bool) or not isinstance(row, int) or row <= 0:
            errors.append(f"{pid}: sourceRow")
        rows.append(row)
        parsed = valid_profile(entry["profile"])
        if parsed is None:
            errors.append(f"{pid}: profile invalid per MP02")
            continue
        if parsed[7] != "SOURCE_DOMAIN":
            errors.append(f"{pid}: provenance must be SOURCE_DOMAIN in this catalog")
    if len(rows) != len(set(rows)):
        errors.append("sourceRow duplicates")
    if len(profiles) != 38:
        errors.append(f"profiles count {len(profiles)} != 38")
    open_upper = [pid for pid, e in profiles.items() if isinstance(e, dict) and isinstance(e.get("profile"), dict)
                  and e["profile"].get("maxInclusive") is False]
    if len(open_upper) != 3:
        errors.append(f"open upper bounds {open_upper} != 3 floorboard parts")
    return errors


def main(argv):
    corpus_path, catalog_path, req_path = argv[0], argv[1], argv[2]
    out = argv[3] if len(argv) > 3 else None
    corpus_raw, corpus = load(corpus_path)
    catalog_raw, catalog = load(catalog_path)
    req_raw = Path(req_path).read_bytes()
    errors = []
    if corpus.get("requirementsLfSha256") != lf_sha(req_raw):
        errors.append("requirementsLfSha256 != REQ LF-SHA")
    if corpus.get("catalogLfSha256") != lf_sha(catalog_raw):
        errors.append("catalogLfSha256 != catalog LF-SHA")
    if corpus.get("schema") != "ricon.oracle.material_profile" or corpus.get("version") != 1:
        errors.append("corpus schema/version")
    errors += [f"catalog: {e}" for e in check_catalog(catalog)]
    rows, bad, direct_mismatch = [], 0, []
    ids = []
    for case in corpus["cases"]:
        if set(case) != {"id", "note", "native", "provenance", "input", "expect"}:
            errors.append(f"{case.get('id')}: case keys")
        ids.append(case["id"])
        got = evaluate(case["input"])
        ok = same_result(got, case["expect"])
        bad += not ok
        if got["code"] == "OK":
            alt = evaluate(case["input"], direct=True)
            if not same_result(alt, case["expect"]):
                direct_mismatch.append(case["id"])
        rows.append({"id": case["id"], "native": case["native"], "ok": ok, "got": got, "expect": case["expect"]})
    if len(ids) != len(set(ids)):
        errors.append("duplicate case ids")
    native = sum(1 for c in corpus["cases"] if c["native"])
    report = {"checker": "claude p3-2b-independent-req-check", "corpusLfSha256": lf_sha(corpus_raw),
              "catalogLfSha256": lf_sha(catalog_raw), "reqLfSha256": lf_sha(req_raw), "cases": len(rows),
              "native": native, "mismatches": bad, "directPowerMismatches": direct_mismatch, "errors": errors,
              "rows": rows}
    if out:
        Path(out).write_text(json.dumps(report, ensure_ascii=False, indent=1) + "\n", encoding="utf-8", newline="\n")
    for r in rows:
        if not r["ok"]:
            print("MISMATCH", r["id"], r["expect"], r["got"])
    for e in errors:
        print("ERROR", e)
    print(f"cases {len(rows)} native {native} mismatches {bad} directPowerMismatches {len(direct_mismatch)} "
          f"errors {len(errors)}")
    passed = bad == 0 and not errors
    print("P3_2B_BLIND_CHECK_PASS" if passed else "P3_2B_BLIND_CHECK_FAIL")
    return 0 if passed else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
