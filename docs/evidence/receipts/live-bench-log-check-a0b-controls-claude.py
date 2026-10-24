"""claude: positive and negative controls for the A0b manager checks of live-bench-log-check-claude.py (codex #4237).
Synthetic logs are derived from the real bench9 console.log: BENCH_START gets the A0b fields, a preSpawn BENCH_MANAGER
is inserted between BENCH_START and the first BENCH_SPAWN. Usage: python -I a0b_parser_test.py <repo> <bench9 log> <outdir>"""
import json
import subprocess
import sys

repo, log, out = sys.argv[1], sys.argv[2], sys.argv[3]
parser = repo + "/docs/evidence/receipts/live-bench-log-check-claude.py"
catalog = repo + "/oracle/data/radio_model_catalog_v1.json"
lines = open(log, encoding="utf-8", errors="replace").read().splitlines()
i_start = next(i for i, l in enumerate(lines) if "kind=BENCH_START" in l)
i_spawn = next(i for i, l in enumerate(lines) if "kind=BENCH_SPAWN" in l)


def manager(value, rm=None, expected=1, where="preSpawn"):
    rm = value if rm is None else rm
    return (f"[RICON_LIVE] seq=1000 t=1004 side=probe kind=BENCH_MANAGER player=-1 device=-1 tr=-1 value={value} "
            f"text=where={where},radioManager={rm},expected={expected}")


def build(start_rm="0", start_exp="1", mgr=None, pos="ok"):
    body = list(lines)
    if start_exp is not None and start_rm is None:
        body[i_start] = body[i_start] + f",expectedManager={start_exp}"
    elif start_exp is not None:
        body[i_start] = body[i_start] + f",radioManager={start_rm},expectedManager={start_exp}"
    if mgr is not None:
        at = {"ok": i_spawn, "beforeStart": i_start, "afterSpawn": i_spawn + 1}[pos]
        body.insert(at, mgr)
    return "\n".join(body)


notready = "\n".join([
    lines[i_start] + ",radioManager=0,expectedManager=1",
    manager(0, expected=1),
    "[RICON_LIVE] seq=1001 t=1004 side=probe kind=BENCH_NOT_READY player=-1 device=-1 tr=-1 value=-1 "
    "text=reason=RADIO_MANAGER_MISMATCH,where=preSpawn,radioManager=0,expected=1",
    "[RICON_LIVE] seq=1002 t=1005 side=probe kind=BENCH_ABORTED player=-1 device=-1 tr=-1 value=0 "
    "text=level=NATIVE_ROUNDTRIP,phase=0"])

# name, log text, parser args, expected exit code, expected overall
cases = [
    ("bench9_noarg", "\n".join(lines), [], 0, "OBSERVED_MISMATCH"),
    ("a0b2_ok", build(mgr=manager(1)), ["1"], 0, "OBSERVED_MISMATCH"),
    ("a0b1_ok", build(start_exp="0", mgr=manager(0, expected=0)), ["0"], 0, "OBSERVED_MISMATCH"),
    ("observe_ok", build(start_rm="1", start_exp="-1", mgr=manager(1, expected=-1)), [], 0, "OBSERVED_MISMATCH"),
    ("value1_text0", build(mgr=manager(1, rm=0)), ["1"], 1, "OBSERVED_MISMATCH"),
    ("text_expected0", build(mgr=manager(1, expected=0)), ["1"], 1, "OBSERVED_MISMATCH"),
    ("start_expected0", build(start_exp="0", mgr=manager(1)), ["1"], 1, "OBSERVED_MISMATCH"),
    ("arg2", build(mgr=manager(1)), ["2"], 1, "OBSERVED_MISMATCH"),
    ("before_start", build(mgr=manager(1), pos="beforeStart"), ["1"], 1, "OBSERVED_MISMATCH"),
    ("after_spawn", build(mgr=manager(1), pos="afterSpawn"), ["1"], 1, "OBSERVED_MISMATCH"),
    ("no_record", build(), ["1"], 1, "OBSERVED_MISMATCH"),
    ("start_expects_noarg", build(mgr=manager(1)), [], 1, "OBSERVED_MISMATCH"),
    ("start_rm_bad", build(start_rm="x", mgr=manager(1)), ["1"], 1, "OBSERVED_MISMATCH"),
    ("start_rm_missing", build(start_rm=None, mgr=manager(1)), ["1"], 1, "OBSERVED_MISMATCH"),
    ("where_early", build(mgr=manager(1, where="early")), ["1"], 1, "OBSERVED_MISMATCH"),
    ("value2", build(mgr=manager(2, rm=2)), ["1"], 1, "OBSERVED_MISMATCH"),
    ("not_ready", notready, ["1"], 1, "NOT_READY"),
]
passed = 0
for name, text, args, rc_expected, overall_expected in cases:
    path = f"{out}/{name}.log"
    open(path, "w", encoding="utf-8").write(text)
    run = subprocess.run([sys.executable, "-X", "utf8", "-I", parser, path, catalog, *args],
                         capture_output=True, text=True, encoding="utf-8")
    report = json.loads(run.stdout[:run.stdout.rindex("}") + 1])
    ok = run.returncode == rc_expected and report["overall"] == overall_expected
    passed += ok
    print(f"{'PASS' if ok else 'FAIL'} {name}: rc={run.returncode} overall={report['overall']} "
          f"errors={report['errors'][:2]} manager={report['radioManager']}")
print(f"A0B_PARSER_CONTROLS {passed}/{len(cases)}")
sys.exit(0 if passed == len(cases) else 1)
