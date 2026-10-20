# RICON_TestRunner (TOOL_ONLY)

Workbench plugin that executes the `RICON_CoreTests` fixtures. Not part of the mod, not for Workshop.

- Owner permission (2026-10-19): run our own tests in Workbench through a plugin, **without Play and without a
  server**. The runner opens only `ScriptEditor` (`-wbModule=ScriptEditor -plugin=RICON_TestRunnerPlugin`), calls
  pure fixture code and prints results to the Workbench log. No world, no game resources, no files read or written.
- `run.ps1 -Name <label>` — one run, unique log folder; prints `PASS` only when Workbench exits with 0 and the log
  has `[RICON_TestRunner] done: PASS`. Steam must be running.
- Evidence level of a passing run: `ENFORCE_PARITY` (the Enforce code gives the same results as the host oracle on
  the same vectors). It is not runtime evidence: no replication, no game callbacks, no VoN.

## Suites

| Suite | Source | What passes |
|---|---|---|
| lifecycle | `RICON_LifecycleFixtures.c`, generated from `oracle/vectors/lifecycle_v1.json` | every step of every trace: result code, PTT response fields, ordered effects (fields of REQ-lifecycle §1.1 per type); 3 negative controls must fail |
| extension | `RICON_ExtFixtures.c` | `SelfCheck()` of EXT-1…5 (REQ-core §10) |
| von_seam | `RICON_VoNSeamFixtures.c`, generated from `oracle/vectors/von_seam_v1.json` | every call: decision equals the oracle, snapshot and binding unchanged (REQ-von-seam §7); special floats are verified non-finite at run time; 3 negative controls must fail |
| l2 | `RICON_L2Fixtures.c`, generated from `oracle/vectors/l2_terrain_v1.json`; `RICON_L2FixtureSupport.c` | every vector: result code exactly, L/Lba/Lbs/Lsph within the fixed 0.05 dB (REQ-L2 §6), branches, spherical case and eq. (19) flag exactly, profile and config unchanged by the call; 3 negative controls must fail; hand-written REQ §6 sampling cases and adapter checks (two layers with MODEL_ID, reason 16 through the evaluator, < 2 m bypass, config/domain mapping); the log prints the largest |Enforce − oracle| |

## Lifecycle fixture generator

`python -I tools/RICON_TestRunner/gen_lifecycle_fixture.py oracle/vectors/lifecycle_v1.json addons/RICON_CoreTests/Scripts/Game/RICON_Tests/RICON_LifecycleFixtures.c`

The generator copies expectations from the vectors and computes nothing. Absences a typed API cannot express are
mapped as a future adapter must map them and marked `ADAPTER_MAPPED` (missing `nowMs` → `-1`, INIT config with a
missing key → `null`); there are 3 such steps. `test_gen_lifecycle_fixture.py` fails when the committed fixture
differs from what the generator makes from the current vectors:
`python -m unittest discover -s tools/RICON_TestRunner -p "test_*.py"`.

## VoN seam fixture generator

`python -I tools/RICON_TestRunner/gen_von_seam_fixture.py oracle/vectors/von_seam_v1.json addons/RICON_CoreTests/Scripts/Game/RICON_Tests/RICON_VoNSeamFixtures.c`

Same trust boundary as the lifecycle generator (closed records, int32, closed names, strict source). One call is
`ADAPTER_MAPPED`: an unknown link state name becomes enum value `-1`. Snapshots and bindings are held in local
variables: in the first Workbench run a temporary passed straight as an argument was released during the call.
`test_von_seam_scan.py` is CT-VON-S-010: the seam file references no engine VoN/radio, Rpl, world/entity or file API
(comments excluded); a planted dependency is detected.

## L2 terrain fixture generator

`python -I tools/RICON_TestRunner/gen_l2_fixture.py oracle/vectors/l2_terrain_v1.json addons/RICON_CoreTests/Scripts/Game/RICON_Tests/RICON_L2Fixtures.c`

The vector envelope is checked exactly like the oracle checker does (standard, revision, note, `toleranceDb ≤ 1e-3`,
`enforceToleranceDb` exactly 0.05), so a vector file cannot loosen the parity tolerance. Three inputs are
`ADAPTER_MAPPED`: frequency null → NaN built at run time, a config with a missing key → `null`, an unknown
polarization name → enum value `-1`. Float literals are plain decimals (no exponent form).
`test_gen_l2_fixture.py` checks exact generation and rejects injection, huge numbers and looser metadata.

## Route fixture generator (separate plugin)

`python -I tools/RICON_TestRunner/gen_route_fixture.py oracle/vectors/route_v1.json addons/RICON_CoreTests/Scripts/Game/RICON_Tests/RICON_RouteFixtures.c`

Accepts only the accepted bytes of `route_v1.json` (SHA-256 pin). Runs as its own plugin `RICON_RouteProbePlugin`
(the main runner is unchanged): 48 vectors through the real `RICON_RouteContract.Evaluate`, reason and state exact,
quality within 1e-6, inputs unchanged by the call, 4 negative controls of the full comparison. Two inputs are
`ADAPTER_MAPPED` (route without "legs" → no legs; unknown state name → enum -1); three null legs are inserted through
the test subclass `RICON_TestRoute`, because the public `AddLeg` refuses null. Evidence: `docs/evidence/E-route-enforce.md`.
