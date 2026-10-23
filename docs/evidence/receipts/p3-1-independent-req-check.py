#!/usr/bin/env python3
"""P3.1 blind checker (claude): REQ-object-rf-profile v0.1 OP02-OP06 re-implemented from the REQ text only.

Inputs: the REQ and oracle/vectors/object_rf_profile_v1.json; the author oracle/builder/tests are NOT read.
Usage: python -I p3-1-independent-req-check.py <corpus> [--json out]"""
from __future__ import annotations

import copy
import hashlib
import json
import re
import sys

EXPECTED_SHA = "b0b93be3238b49c223c859ac14e22f045ae708ce03f1cae2ed731eaf0531e4c5"
HOST_ONLY = {"R31", "R32", "R33", "R34", "R35", "R36", "R41"}
CASES = 47
KEY = re.compile(r"[A-Za-z0-9_.:-]{1,64}")
ROOT = {"object", "profiles", "prefabParents", "classParents", "prefabRules", "classRules"}
OBJECT = {"componentProfileId", "surfaceProfileId", "prefabId", "classId", "ballisticHint"}


class Bad(Exception):
    pass


def need(c):
    if not c:
        raise Bad


def is_key(s):
    return type(s) is str and KEY.fullmatch(s) is not None


def key_or_empty(s):
    need(type(s) is str and (s == "" or KEY.fullmatch(s) is not None))
    return s


def graph(g):
    """OP02: id -> parent|null; parent in the same graph; no self-reference or cycle anywhere (iterative)."""
    need(type(g) is dict)
    for k, parent in g.items():
        need(is_key(k))
        need(parent is None or (is_key(parent) and parent in g and parent != k))
    state = {}
    for start in g:
        if state.get(start) == 2:
            continue
        path, node = [], start
        while node is not None and state.get(node) != 2:
            need(state.get(node) != 1)  # back on the current walk -> cycle
            state[node] = 1
            path.append(node)
            node = g[node]
        for n in path:
            state[n] = 2
    return g


def rules(r, g, profiles):
    need(type(r) is dict)
    for node, prof in r.items():
        need(is_key(node) and node in g and is_key(prof) and prof in profiles)
    return r


def walk(start, g, r):
    node = start
    while node is not None:
        if node in r:
            return node
        node = g[node]
    return None


def resolve(inp):
    try:
        need(type(inp) is dict and set(inp) == ROOT)
        prof_list = inp["profiles"]
        need(type(prof_list) is list and all(is_key(p) for p in prof_list) and len(set(prof_list)) == len(prof_list))
        profiles = set(prof_list)
        pg, cg = graph(inp["prefabParents"]), graph(inp["classParents"])
        pr, cr = rules(inp["prefabRules"], pg, profiles), rules(inp["classRules"], cg, profiles)
        obj = inp["object"]
        need(type(obj) is dict and set(obj) == OBJECT)
        o = {k: key_or_empty(obj[k]) for k in OBJECT}
        need(o["prefabId"] == "" or o["prefabId"] in pg)
        need(o["classId"] == "" or o["classId"] in cg)
    except Bad:
        return {"code": "INVALID_INPUT"}
    for field, source in (("componentProfileId", "COMPONENT"), ("surfaceProfileId", "MATERIAL")):
        pid = o[field]
        if pid:
            if pid in profiles:
                return {"code": "RESOLVED", "knowledge": "PROFILE", "materialId": pid, "source": source, "matchedKey": pid}
            return {"code": "UNRESOLVED_PROFILE", "knowledge": "UNKNOWN", "materialId": "", "source": source, "matchedKey": pid}
    for start, g, r, source in ((o["prefabId"], pg, pr, "PREFAB"), (o["classId"], cg, cr, "CLASS")):
        if start:
            hit = walk(start, g, r)
            if hit is not None:
                return {"code": "RESOLVED", "knowledge": "RULE", "materialId": r[hit], "source": source, "matchedKey": hit}
    if o["ballisticHint"]:
        return {"code": "UNCLASSIFIED", "knowledge": "HINT_ONLY", "materialId": "", "source": "HINT",
                "matchedKey": o["ballisticHint"]}
    return {"code": "UNCLASSIFIED", "knowledge": "UNKNOWN", "materialId": "", "source": "NONE", "matchedKey": ""}


def main(argv):
    raw = open(argv[1], "rb").read()
    sha = hashlib.sha256(raw).hexdigest()
    errors = [] if sha == EXPECTED_SHA else [f"corpus SHA {sha}"]
    doc = json.loads(raw)
    if set(doc.get("hostOnly", [])) != HOST_ONLY:
        errors.append(f"hostOnly {doc.get('hostOnly')}")
    if len(doc["cases"]) != CASES:
        errors.append("case count")
    codes, sources = {}, set()
    for c in doc["cases"]:
        inp = copy.deepcopy(c["input"])
        got = resolve(inp)
        if inp != c["input"]:
            errors.append(f"{c['id']}: input mutated")
        codes[c["id"]] = got["code"]
        sources.add(got.get("source"))
        if json.dumps(got, sort_keys=True) != json.dumps(c["expect"], sort_keys=True):
            errors.append(f"{c['id']}: {got} != {c['expect']}")
    # Controls: a cycle in an unused branch, a dangling parent and a rule on a missing node must all be INVALID_INPUT,
    # and a 5000-node chain resolves without recursion.
    base = copy.deepcopy(next(c["input"] for c in doc["cases"] if c["expect"]["code"] == "RESOLVED"))
    ctl = 0
    for mut in (lambda x: x["prefabParents"].update({"zz.a": "zz.b", "zz.b": "zz.a"}),
                lambda x: x["classParents"].update({"zz.c": "zz.missing"}),
                lambda x: x["prefabRules"].update({"zz.none": next(iter(x["profiles"]), "p")}),
                lambda x: x["object"].update(prefabId="zz.unknown"),
                lambda x: x["profiles"].append(x["profiles"][0]) if x["profiles"] else x["profiles"].extend(["q", "q"])):
        y = copy.deepcopy(base)
        mut(y)
        ctl += resolve(y)["code"] == "INVALID_INPUT"
    chain = {f"n{i}": (f"n{i + 1}" if i < 4999 else None) for i in range(5000)}
    deep = {"object": {"componentProfileId": "", "surfaceProfileId": "", "prefabId": "n0", "classId": "", "ballisticHint": ""},
            "profiles": ["deep.profile"], "prefabParents": chain, "classParents": {}, "prefabRules": {"n4999": "deep.profile"},
            "classRules": {}}
    deep_ok = resolve(deep) == {"code": "RESOLVED", "knowledge": "RULE", "materialId": "deep.profile", "source": "PREFAB",
                                "matchedKey": "n4999"}
    summary = {"checker": "claude p3-1-independent-req-check (REQ v0.1 + JSON only)", "corpusSha256": sha,
               "cases": len(codes), "native": len(codes) - len(HOST_ONLY), "errors": errors,
               "codes": sorted(set(codes.values())), "sources": sorted(s for s in sources if s),
               "controls": f"{ctl}/5", "deepChain5000": deep_ok}
    print(json.dumps(summary, ensure_ascii=False, indent=1))
    if "--json" in argv:
        with open(argv[argv.index("--json") + 1], "w", encoding="utf-8", newline="\n") as fh:
            fh.write(json.dumps(dict(summary, perCase=codes), ensure_ascii=False, indent=1) + "\n")
    ok = not errors and ctl == 5 and deep_ok
    print("P3_1_BLIND_CHECK_" + ("PASS" if ok else "FAIL"))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    raise SystemExit(main(sys.argv))
