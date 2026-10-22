"""R29 host regression (claude): every oracle CLI, every fixture generator (byte compare) and every unit test.
Exit 1 when any check fails (added after the receipted run, codex #2859)."""
import importlib.util
import subprocess
import sys
import tempfile
from pathlib import Path

repo = Path(sys.argv[1])
sys.stdout.reconfigure(encoding="utf-8")
O, T = repo / "oracle", repo / "tools" / "RICON_TestRunner"
TESTS = repo / "addons" / "RICON_CoreTests" / "Scripts" / "Game" / "RICON_Tests"
WG = T / "Scripts" / "WorkbenchGame"
V = O / "vectors"
results = []


def run(label, args, ok_marker=None, cwd=repo, timeout=900):
    p = subprocess.run([sys.executable, "-B", *args], cwd=cwd, capture_output=True, text=True, encoding="utf-8",
                       errors="replace", timeout=timeout)
    out = (p.stdout + p.stderr).strip().splitlines()
    tail = out[-1] if out else ""
    ok = p.returncode == 0 and (ok_marker is None or any(ok_marker in l for l in out))
    results.append((label, ok, p.returncode, tail[:110]))


# oracle CLIs
run("ricon_oracle link_eval", [str(O / "ricon_oracle.py"), "--check", str(V / "link_eval_v1.json")])
run("l2_terrain", [str(O / "l2_terrain_oracle.py"), "--check", str(V / "l2_terrain_v1.json")])
run("lifecycle", [str(O / "lifecycle_oracle.py"), "--check", str(V / "lifecycle_v1.json")])
run("link_physics scenarios", [str(O / "link_physics_oracle.py"), "--check", str(V / "link_scenarios_v1.json")], "PASS")
run("link_physics probes", [str(O / "link_physics_oracle.py"), "--check", str(V / "link_probe_points_v1.json")], "PASS")
run("route", [str(O / "route_oracle.py"), "--check", str(V / "route_v1.json")])
run("snapshot", [str(O / "snapshot_oracle.py"), "--check", str(V / "snapshot_v1.json")])
run("von_seam", [str(O / "von_seam_oracle.py"), "--check", str(V / "von_seam_v1.json")])
run("cellular", [str(O / "cellular_oracle.py"), "--check", str(V / "cellular_scenarios_v1.json")])
run("world_profile", [str(O / "world_profile_oracle.py"), "--check", str(V / "world_profile_v1.json")], "PASS")
run("radio_device", [str(O / "radio_device_oracle.py"), "--check", str(V / "radio_device_v1.json")], "PASS")
run("pair_service", [str(O / "pair_service_oracle.py"), "--check", str(V / "pair_service_v1.json")], "PASS")
run("link_publication", [str(O / "link_publication_oracle.py"), "--check", str(V / "link_publication_v1.json")], "PASS")
run("channel_cutoff", [str(O / "channel_cutoff_oracle.py"), "--check", str(V / "channel_cutoff_v1.json")], "PASS")
run("sensitivity anchors", [str(O / "sensitivity_oracle.py"), "--anchors"], "SENSITIVITY_ANCHORS_PASS")
run("sensitivity full", [str(O / "sensitivity_oracle.py")], "SENSITIVITY_ORACLE_PASS")
run("p527 checker (codex)", ["-I", str(T / "check_p527_complex.py")], "PASS")

# generators with --check
for gen, src, dst in (("gen_l0_fixture.py", V / "link_eval_v1.json", TESTS / "RICON_L0Fixtures.c"),
                      ("gen_p526_matrix.py", V / "link_scenarios_v1.json", WG / "RICON_P526MatrixPoints.c"),
                      ("gen_route_fixture.py", V / "route_v1.json", TESTS / "RICON_RouteFixtures.c"),
                      ("gen_snapshot_fixture.py", V / "snapshot_v1.json", TESTS / "RICON_SnapshotFixtures.c"),
                      ("gen_sensitivity_points.py", V / "l2_sensitivity_anchors_v1.json",
                       WG / "RICON_SensitivityPoints.c"),
                      ("gen_world_profile_fixture.py", V / "world_profile_v1.json",
                       TESTS / "RICON_WorldProfileFixtures.c"),
                      ("gen_radio_device_fixture.py", V / "radio_device_v1.json",
                       TESTS / "RICON_RadioDeviceFixtures.c"),
                      ("gen_pair_service_fixture.py", V / "pair_service_v1.json",
                       TESTS / "RICON_PairServiceFixtures.c"),
                      ("gen_link_publication_fixture.py", V / "link_publication_v1.json",
                       TESTS / "RICON_LinkPublicationFixtures.c"),
                      ("gen_channel_cutoff_fixture.py", V / "channel_cutoff_v1.json",
                       TESTS / "RICON_ChannelCutoffFixtures.c")):
    run(gen + " --check", ["-I", str(T / gen), str(src), str(dst), "--check"])

# generators without --check: write to a temp file and compare bytes
tmp = Path(tempfile.mkdtemp())
for gen, src, dst in (("gen_l2_fixture.py", V / "l2_terrain_v1.json", TESTS / "RICON_L2Fixtures.c"),
                      ("gen_lifecycle_fixture.py", V / "lifecycle_v1.json", TESTS / "RICON_LifecycleFixtures.c"),
                      ("gen_von_seam_fixture.py", V / "von_seam_v1.json", TESTS / "RICON_VoNSeamFixtures.c")):
    out = tmp / dst.name
    p = subprocess.run([sys.executable, "-B", "-I", str(T / gen), str(src), str(out)], cwd=repo, capture_output=True,
                       text=True, encoding="utf-8", errors="replace")
    same = p.returncode == 0 and out.read_bytes() == dst.read_bytes()
    results.append((gen + " byte-compare", same, p.returncode, "identical" if same else "DIFFERS"))

# unit tests
run("unittest oracle", ["-m", "unittest", "discover", "-s", "oracle", "-p", "test_*.py"], timeout=1800)
run("unittest tools", ["-m", "unittest", "discover", "-s", "tools/RICON_TestRunner", "-p", "test_*.py"])

for label, ok, code, tail in results:
    print(f"{'PASS' if ok else 'FAIL'} | {label} | exit {code} | {tail}")
print(f"TOTAL {sum(r[1] for r in results)}/{len(results)} PASS")
sys.exit(0 if results and all(r[1] for r in results) else 1)  # gate (codex #2859)
