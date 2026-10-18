# Cycle 001 — Engine and legal surface

## [SYNTHESIZER'S PROPOSAL]

- `VoNComponent` and radio APIs expose useful control and observation signatures, but they do not prove per-link audio mutation or relay forwarding.
- Official audio documentation makes H3 plausible at the generic audio-system level: script/entity/global signals can feed `.sig` and `.acp` graphs.
- Official multiplayer documentation confirms a server-authoritative RPC pattern but warns that streaming relevance and performance are material constraints.
- Workbench static work cannot become valid until the game-data project finishes installing.
- Workshop publishing is rights-gated and version-retention-limited.
- Published Bohemia rules contradict the planned assumption that reverse engineering might be acceptable merely for compatibility.

## [CRITIC'S AUDIT]

- API names are not runtime behavior. `RelayTransceiver` has no documented relay-specific members.
- Generic audio signals are not evidence that vanilla VoN exposes a per-received-stream signal or a writable quality input.
- Search-result excerpts for wiki pages are useful discovery evidence but weaker than local versioned API pages or binding terms.
- The public Game Content Usage Rules are not necessarily the most specific Tools EULA, so RE is `disputed/blocked`, not conclusively impossible under every legal theory.
- No BattlEye-specific primary evidence was found; absence must not be treated as compatibility.

## [RESOLUTION]

- `fact_001` → `CLOSES` installed-version part of [[q_001]], `NARROWS` [[q_006]].
- `fact_002` / `fact_003` → `NARROWS` [[q_002]].
- `fact_004` → `NARROWS` [[q_004]]; rejects type-existence shortcut.
- `fact_005` / `fact_006` → `SPLITS` [[q_003]] into [[q_016]] and [[q_017]].
- `fact_007` / `fact_008` → `SPLITS` [[q_005]] into [[q_018]], [[q_019]], [[q_020]].
- `fact_009` / `fact_010` / `fact_015` → `CLOSES` [[q_011]].
- `fact_011` / `fact_012` → `NARROWS` [[q_014]].
- `fact_013` → `CONTRADICTS` prior RE assumption, marks [[q_007]] disputed and spawns [[q_021]].
- `fact_014` → `NARROWS` [[q_002]] and records a documentation defect.

