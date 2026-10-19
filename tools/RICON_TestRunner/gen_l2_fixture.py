#!/usr/bin/env python3
"""TOOL_ONLY: generate the Enforce L2 terrain parity fixture from the host oracle vectors (stdlib only).

Usage: python -I gen_l2_fixture.py <oracle/vectors/l2_terrain_v1.json> <out RICON_L2Fixtures.c>

Every vector becomes one `RICON_TerrainDiffractionCalculator.Evaluate(...)` call with its expectation copied from the
vectors; the generator computes nothing. Numbers are compared within the vectors' `enforceToleranceDb`, codes, branches,
spherical case and first-term accuracy exactly. Inputs a typed API cannot carry are mapped the way an adapter must
map them and counted as ADAPTER_MAPPED:
  - frequency JSON null      -> NaN built at run time (checked non-finite before use)
  - config with a missing key -> config = null (INVALID_CONFIG)
  - polarization name outside VERTICAL/HORIZONTAL -> enum value -1
Trust boundary: validate() admits only the closed vector schema with bounded plain numbers and closed names before any
code is written. Three negative controls replay a vector with a broken expectation and must fail.
"""

from __future__ import annotations

import copy
import hashlib
import json
import math
import re
import sys
from pathlib import Path

SOURCE = re.compile(r"docs/req/REQ-L2-terrain\.md@[0-9a-f]{7,40}")
CASE_ID = re.compile(r"[A-Za-z0-9_-]{1,64}")
CT_ID = re.compile(r"CT-L2-[0-9]{3}")
NAME = re.compile(r"[A-Z_]{1,32}")
ROOT_KEYS = {"schema", "version", "revision", "source", "standard", "note", "toleranceDb", "enforceToleranceDb", "cases"}
CASE_KEYS = {"id", "ct", "provenance", "note", "input", "expect"}
INPUT_KEYS = {"profile", "htg", "hrg", "frequencyMHz", "config"}
CONFIG_KEYS = ("earthRadiusKm", "kFactor", "permittivity", "conductivitySm", "polarization", "maxProfileStepKm",
               "maxProfilePoints")
CODES = ("INVALID_CONFIG", "OUT_OF_DOMAIN_FREQUENCY", "INVALID_PROFILE", "PROFILE_STEP_TOO_LARGE",
         "INVALID_ANTENNA_HEIGHT", "NUMERIC_FAILURE", "OK")
PROVENANCE = ("STD_INVARIANT", "HAND_DERIVED", "CROSS_CHECKED")
BRANCHES = ("LOS", "TRANS")
SPH_CASES = ("BEYOND", "CLEAR", "AH_NEGATIVE", "INTERPOLATED")
ACCURACY = ("MET", "NOT_MET", "NOT_EVALUATED")
POLARIZATIONS = ("VERTICAL", "HORIZONTAL")
NUMERIC_FIELDS = ("L", "Lba", "Lbs", "Lsph")
EXACT_FIELDS = ("branchActual", "branchSmooth", "sphCase", "firstTermAccuracy")
MAGNITUDE = 1e12
MAX_POINTS = 100000
RUN = "RICON_L2Run"


class VectorError(ValueError):
    pass


def require(condition: bool, where: str) -> None:
    if not condition:
        raise VectorError(f"unsupported vector value at {where}")


def is_plain_number(value: object) -> bool:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        return False
    try:
        return math.isfinite(float(value)) and abs(float(value)) <= MAGNITUDE
    except OverflowError:
        return False


def validate(data: object) -> None:
    require(isinstance(data, dict) and set(data) == ROOT_KEYS, "root keys")
    require(data["schema"] == "ricon.oracle.l2_terrain" and data["version"] == 1, "schema/version")
    require(isinstance(data["source"], str) and SOURCE.fullmatch(data["source"]) is not None, "source")
    require(is_plain_number(data["enforceToleranceDb"]) and 0 < data["enforceToleranceDb"] <= 0.5, "enforceToleranceDb")
    require(isinstance(data["cases"], list) and len(data["cases"]) > 0, "cases")
    seen = set()
    for case in data["cases"]:
        require(isinstance(case, dict) and set(case) == CASE_KEYS, "case keys")
        cid = case["id"]
        require(isinstance(cid, str) and CASE_ID.fullmatch(cid) is not None and cid not in seen, "case id")
        seen.add(cid)
        require(isinstance(case["ct"], list) and case["ct"] and all(isinstance(c, str) and CT_ID.fullmatch(c)
                                                                     for c in case["ct"]), f"{cid}.ct")
        require(isinstance(case["provenance"], str) and case["provenance"] in PROVENANCE, f"{cid}.provenance")
        inp = case["input"]
        require(isinstance(inp, dict) and set(inp) == INPUT_KEYS, f"{cid}.input")
        profile = inp["profile"]
        require(isinstance(profile, list) and 0 < len(profile) <= MAX_POINTS and all(
            isinstance(p, list) and len(p) == 2 and all(is_plain_number(v) for v in p) for p in profile), f"{cid}.profile")
        for key in ("htg", "hrg"):
            require(is_plain_number(inp[key]), f"{cid}.{key}")
        require(inp["frequencyMHz"] is None or is_plain_number(inp["frequencyMHz"]), f"{cid}.frequencyMHz")
        cfg = inp["config"]
        require(isinstance(cfg, dict) and set(cfg) <= set(CONFIG_KEYS), f"{cid}.config")
        for key, value in cfg.items():
            if key == "polarization":
                require(isinstance(value, str) and NAME.fullmatch(value) is not None, f"{cid}.config.polarization")
            elif key == "maxProfilePoints":
                require(isinstance(value, int) and not isinstance(value, bool) and abs(value) <= MAX_POINTS * 10,
                        f"{cid}.config.maxProfilePoints")
            else:
                require(is_plain_number(value), f"{cid}.config.{key}")
        expect = case["expect"]
        require(isinstance(expect, dict) and isinstance(expect.get("code"), str) and expect["code"] in CODES,
                f"{cid}.expect")
        keys = {"code"} | (set(NUMERIC_FIELDS) | set(EXACT_FIELDS) if expect["code"] == "OK" else set())
        require(set(expect) == keys, f"{cid}.expect keys")
        if expect["code"] == "OK":
            require(all(is_plain_number(expect[k]) for k in NUMERIC_FIELDS), f"{cid}.expect numbers")
            require(expect["branchActual"] in BRANCHES and expect["branchSmooth"] in BRANCHES
                    and expect["sphCase"] in SPH_CASES and expect["firstTermAccuracy"] in ACCURACY,
                    f"{cid}.expect names")


def literal(value: float) -> str:
    """Plain decimal float literal (no exponent form)."""
    text = repr(float(value))
    if "e" in text or "E" in text:
        text = format(float(value), ".17f").rstrip("0")
        if text.endswith("."):
            text += "0"
    return text


class Generator:
    def __init__(self) -> None:
        self.adapter_mapped = 0
        self.local = 0

    def name(self, prefix: str) -> str:
        self.local += 1
        return f"{prefix}{self.local}"

    def config(self, cfg: dict, pre: list[str]) -> str:
        if set(cfg) != set(CONFIG_KEYS):
            self.adapter_mapped += 1
            return "null /* ADAPTER_MAPPED: config key missing */"
        pol = cfg["polarization"]
        if pol in POLARIZATIONS:
            pol_code = f"RICON_ETerrainPolarization.{pol}"
        else:
            self.adapter_mapped += 1
            pol_code = self.name("pol")
            pre.append(f"RICON_ETerrainPolarization {pol_code} = -1; // ADAPTER_MAPPED: unknown polarization name")
        local = self.name("c")
        pre.append(f"RICON_TerrainDiffractionConfig {local} = {RUN}.Config({literal(cfg['earthRadiusKm'])}, "
                   f"{literal(cfg['kFactor'])}, {literal(cfg['permittivity'])}, {literal(cfg['conductivitySm'])}, "
                   f"{pol_code}, {literal(cfg['maxProfileStepKm'])}, {int(cfg['maxProfilePoints'])});")
        return local

    def case(self, name: str, case: dict, tolerance: float) -> list[str]:
        inp, expect = case["input"], case["expect"]
        pre: list[str] = []
        cfg = self.config(inp["config"], pre)
        profile = self.name("p")
        pre.append(f"RICON_TerrainProfile {profile} = new RICON_TerrainProfile();")
        for d, h in inp["profile"]:
            pre.append(f"{profile}.AddPoint({literal(d)}, {literal(h)});")
        if inp["frequencyMHz"] is None:
            self.adapter_mapped += 1
            freq = self.name("f")
            pre.append(f"float {freq} = {RUN}.NaN(); // ADAPTER_MAPPED: frequency is JSON null")
            pre.append(f"t.RequireNonFinite({freq});")
        else:
            freq = literal(inp["frequencyMHz"])
        call = (f"RICON_TerrainDiffractionCalculator.Evaluate({profile}, {literal(inp['htg'])}, {literal(inp['hrg'])}, "
                f"{freq}, {cfg})")
        result = self.name("r")
        pre.append(f"RICON_L2Result {result} = {call};")
        code = f"RICON_EL2ResultCode.{expect['code']}"
        if expect["code"] == "OK":
            pre.append(f"t.CheckOk({result}, {literal(expect['L'])}, {literal(expect['Lba'])}, {literal(expect['Lbs'])}, "
                       f"{literal(expect['Lsph'])}, RICON_EL2Branch.{expect['branchActual']}, "
                       f"RICON_EL2Branch.{expect['branchSmooth']}, RICON_EL2SphericalCase.{expect['sphCase']}, "
                       f"RICON_EFirstTermAccuracy.{expect['firstTermAccuracy']}, {literal(tolerance)});")
        else:
            pre.append(f"t.CheckCode({result}, {code});")
        return [f"\t//! {case['id']} ({', '.join(case['ct'])}, {case['provenance']})",
                f"\tprotected static void {name}({RUN} t)", "\t{"] + ["\t\t" + line for line in pre] + ["\t}", ""]


def controls(cases: list, tolerance: float) -> list[tuple[str, dict]]:
    """Three broken copies of real vectors; each must fail in Enforce."""
    ok = next(c for c in cases if c["expect"]["code"] == "OK" and c["expect"]["L"] > 1.0)
    shifted = copy.deepcopy(ok)
    shifted["expect"]["L"] += 2 * tolerance
    branch = copy.deepcopy(next(c for c in cases if c["expect"]["code"] == "OK"
                                and c["expect"]["branchActual"] == "TRANS"))
    branch["expect"]["branchActual"] = "LOS"
    error = copy.deepcopy(next(c for c in cases if c["expect"]["code"] == "INVALID_PROFILE"))
    error["expect"]["code"] = "OK"
    error["expect"].update({"L": 0.0, "Lba": 0.0, "Lbs": 0.0, "Lsph": 0.0, "branchActual": "LOS",
                            "branchSmooth": "LOS", "sphCase": "CLEAR", "firstTermAccuracy": "NOT_EVALUATED"})
    return [("loss shifted by twice the tolerance", shifted), ("wrong actual branch", branch),
            ("invalid profile accepted", error)]


def generate_bytes(raw: bytes) -> str:
    data = json.loads(raw.decode("utf-8"))
    validate(data)
    tolerance = float(data["enforceToleranceDb"])
    cases = data["cases"]
    gen = Generator()
    body: list[str] = []
    for index, case in enumerate(cases, 1):
        body += gen.case(f"Case{index:02d}", case, tolerance)
    adapter_mapped = gen.adapter_mapped
    control_list = controls(cases, tolerance)
    for index, (_, case) in enumerate(control_list, 1):
        body += gen.case(f"Control{index}", case, tolerance)

    run_all = ["\t//! Replays every vector; true only when all pass, every negative control fails and sampling matches.",
               "\tstatic bool RunAll()", "\t{", "\t\tint passed = 0;", f"\t\t{RUN}.s_fMaxDeviationDb = 0;",
               f"\t\t{RUN} t;"]
    for index, case in enumerate(cases, 1):
        run_all += [f"\t\tt = new {RUN}(\"{case['id']}\");", f"\t\tCase{index:02d}(t);", "\t\tif (t.IsPassed())",
                    "\t\t\tpassed++;"]
    run_all += ["\t\tint controlsCaught = 0;"]
    for index, (what, _) in enumerate(control_list, 1):
        run_all += [f"\t\tt = new {RUN}(\"control: {what}\", true);", f"\t\tControl{index}(t);",
                    "\t\tif (t.IsFailed())", "\t\t\tcontrolsCaught++;", "\t\telse",
                    f"\t\t\tPrint(\"[RICON_TEST] FAIL negative control not detected: {what}\");"]
    run_all += [
        f"\t\tbool sampling = {RUN}.CheckSampling();",
        f"\t\tbool ok = passed == {len(cases)} && controlsCaught == {len(control_list)} && sampling;",
        f"\t\tPrintFormat(\"[RICON_TEST] l2 parity: cases %1/{len(cases)}, negative controls caught %2/{len(control_list)}, "
        f"sampling %3, adapter-mapped inputs {adapter_mapped}, tolerance {literal(tolerance)} dB\", passed, controlsCaught, sampling);",
        f"\t\tPrintFormat(\"[RICON_TEST] l2 max |Enforce - oracle| = %1 dB\", {RUN}.s_fMaxDeviationDb);",
        "\t\tif (ok)", "\t\t\tPrint(\"[RICON_TEST] L2_ENFORCE_PARITY_PASS\");", "\t\telse",
        "\t\t\tPrint(\"[RICON_TEST] L2_ENFORCE_PARITY_FAIL\");", "\t\treturn ok;", "\t}", ""]
    header = [
        "// GENERATED by tools/RICON_TestRunner/gen_l2_fixture.py - do not edit by hand.",
        f"// Source: oracle/vectors/l2_terrain_v1.json sha256 {hashlib.sha256(raw).hexdigest()}",
        f"// Vectors: {data['source']}, {len(cases)} cases, adapter-mapped inputs {adapter_mapped}, "
        f"tolerance {literal(tolerance)} dB.",
        "// TEST_ONLY L2 terrain parity fixture: RICON_TerrainDiffractionCalculator against the host oracle.",
        "", "class RICON_L2Fixtures", "{"]
    return "\n".join(header + run_all + body).rstrip() + "\n}\n"


def generate(path: Path) -> str:
    return generate_bytes(path.read_bytes())


def main(argv: list[str]) -> int:
    if len(argv) != 3:
        print(__doc__)
        return 2
    with open(argv[2], "w", encoding="utf-8", newline="\n") as out:
        out.write(generate(Path(argv[1])))
    print(f"written {argv[2]}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
