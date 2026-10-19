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
