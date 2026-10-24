"""claude: positive and negative controls for the V2 setter checks of live-bench-log-check-claude.py (codex #4262).
Synthetic logs are derived from the real V1 console.log (A0b fields present, expectation 1) and from the real bench9
console.log (no A0b fields, expectation "-"): BENCH_START gets setter/setterGiven, every BENCH_SET_FREQUENCY gets setter.
Usage: python -I live-bench-log-check-v2-controls-claude.py <repo> <V1 log> <bench9 log> <outdir>"""
import json
import re
import subprocess
import sys

repo, log_v1, log_b9, out = sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4]
parser = repo + "/docs/evidence/receipts/live-bench-log-check-claude.py"
catalog = repo + "/oracle/data/radio_model_catalog_v1.json"


def stream(path):
    """Lines up to and including the first BENCH_DONE (the dump after it repeats seq values, skipped here)."""
    lines = open(path, encoding="utf-8", errors="replace").read().splitlines()
    end = next(i for i, l in enumerate(lines) if "kind=BENCH_DONE" in l)
    return lines[:end + 1]


v1, b9 = stream(log_v1), stream(log_b9)


def build(lines, start=None, step="same", odd=None):
    """start: text appended to BENCH_START (None = nothing); step: setter for every BENCH_SET_FREQUENCY ("same" = the
    value from start, None = no field); odd: (index of the step, its setter or None) for one deviating step."""
    body, k = [], 0
    value = start.split("setter=")[1].split(",")[0] if start else None
    for l in lines:
        if "kind=BENCH_START" in l and start is not None:
            l = l + start
        if "kind=BENCH_SET_FREQUENCY" in l:
            s = value if step == "same" else step
            if odd is not None and k == odd[0]:
                s = odd[1]
            if s is not None:
                l = l + ",setter=" + s
            k += 1
        body.append(l)
    return "\n".join(body)


def reach(text, devices):
    """codex #4269 positive control: the t5 read of transceiver 0 equals the logged target on the given devices."""
    out = []
    for l in text.split("\n"):
        m = re.search(r"tr=0 value=(-?\d+) text=bench=(\d+),phase=t5,.*targetTr0=(-?\d+)", l)
        if m and int(m.group(2)) in devices:
            l = l.replace(f"tr=0 value={m.group(1)} ", f"tr=0 value={m.group(3)} ", 1)
        out.append(l)
    return "\n".join(out)


i_start = next(i for i, l in enumerate(v1) if "kind=BENCH_START" in l)
notready = "\n".join([
    v1[i_start] + ",setter=INVALID,setterGiven=1",
    "[RICON_LIVE] seq=1 t=1001 side=probe kind=BENCH_MANAGER player=-1 device=-1 tr=-1 value=1 "
    "text=where=preSpawn,radioManager=1,expected=1",
    "[RICON_LIVE] seq=2 t=1001 side=probe kind=BENCH_NOT_READY player=-1 device=-1 tr=-1 value=-1 "
    "text=reason=SETTER_PARAM,setter=INVALID,setterGiven=1",
    "[RICON_LIVE] seq=3 t=1002 side=probe kind=BENCH_ABORTED player=-1 device=-1 tr=-1 value=0 "
    "text=level=NATIVE_ROUNDTRIP,phase=0"])

T, C = ",setter=transceiver,setterGiven=1", ",setter=component,setterGiven=1"
# name, log text, parser args, expected exit code, expected overall
cases = [
    ("v1_as_is", "\n".join(v1), ["1"], 0, "OBSERVED_MISMATCH"),
    ("v1_expect_component", "\n".join(v1), ["1", "component"], 1, "OBSERVED_MISMATCH"),
    ("transceiver_ok", build(v1, T), ["1", "transceiver"], 0, "OBSERVED_MISMATCH"),
    ("transceiver_reached_all", reach(build(v1, T), {0, 1, 2, 3}), ["1", "transceiver"], 0, "NATIVE_ROUNDTRIP_PASS"),
    ("transceiver_reached_three", reach(build(v1, T), {0, 1, 3}), ["1", "transceiver"], 0, "OBSERVED_MISMATCH"),
    ("component_given_ok", build(v1, C), ["1", "component"], 0, "OBSERVED_MISMATCH"),
    ("default_ok", build(v1, ",setter=component,setterGiven=0"), ["1"], 0, "OBSERVED_MISMATCH"),
    ("bench9_dash_transceiver_ok", build(b9, T), ["-", "transceiver"], 0, "OBSERVED_MISMATCH"),
    ("v1_dash_manager", build(v1, T), ["-", "transceiver"], 1, "OBSERVED_MISMATCH"),
    ("given_noarg", build(v1, T), ["1"], 1, "OBSERVED_MISMATCH"),
    ("expect_mismatch", build(v1, T), ["1", "component"], 1, "OBSERVED_MISMATCH"),
    ("default_expect_given", build(v1, ",setter=component,setterGiven=0"), ["1", "component"], 1, "OBSERVED_MISMATCH"),
    ("step_mismatch", build(v1, T, odd=(2, "component")), ["1", "transceiver"], 1, "OBSERVED_MISMATCH"),
    ("step_missing", build(v1, T, odd=(3, None)), ["1", "transceiver"], 1, "OBSERVED_MISMATCH"),
    ("steps_only", build(v1, None, step="transceiver"), ["1"], 1, "OBSERVED_MISMATCH"),
    ("default_not_component", build(v1, ",setter=transceiver,setterGiven=0"), ["1"], 1, "OBSERVED_MISMATCH"),
    ("given_bad", build(v1, ",setter=transceiver,setterGiven=2"), ["1", "transceiver"], 1, "OBSERVED_MISMATCH"),
    ("start_setter_bad", build(v1, ",setter=direct,setterGiven=1"), ["1", "transceiver"], 1, "OBSERVED_MISMATCH"),
    ("arg_bad", build(v1, T), ["1", "direct"], 1, "OBSERVED_MISMATCH"),
    ("not_ready", notready, ["1", "transceiver"], 1, "NOT_READY"),
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
          f"errors={report['errors'][:2]} setter={report['setter']}")
print(f"V2_PARSER_CONTROLS {passed}/{len(cases)}")
sys.exit(0 if passed == len(cases) else 1)
