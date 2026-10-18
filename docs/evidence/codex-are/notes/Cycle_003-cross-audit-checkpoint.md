# Cycle 003 — Cross-audit checkpoint

## [SYNTHESIZER'S PROPOSAL]

- Claude's prior corpus is independently useful and mostly agrees with the Codex engine/API findings.
- Codex supplements it with stronger official audio, Rpl, Workbench, Workshop, EULA, ITU, performance, and 3GPP evidence.
- The combined corpus clearly separates known public signatures from open engine behavior.
- Static research can continue after mutual validation; runtime/audio claims cannot.

## [CRITIC'S AUDIT]

- Claude's prior corpus predates the research-first requirement and does not cover every current v5 section.
- Claude has not yet audited Codex's new sources or the new legal contradiction.
- One-sided cross-audit cannot satisfy the owner's explicit two-agent completeness check.
- Vanilla packed source/audio inspection via Workbench has not been performed in this immutable ARE run.
- BattlEye compatibility, physical calibration, runtime scale, and multi-client voice remain open regardless of static completeness.

## [RESOLUTION]

- `fact_031`–`fact_033` → `NARROWS` [[q_006]] and confirms [[q_011]].
- `fact_034` / `fact_042` → `CLOSES` [[q_007]] and [[q_021]] against RE under the current EULA.
- `fact_035` → keeps BattlEye part of [[q_014]] open.
- `fact_036`–`fact_039` → cross-checks Claude's corpus and records its missing current-plan sections.
- `fact_040` / `fact_045` → `NEEDS_USER` for [[q_015]] because only the other agent can supply its independent audit.
- `fact_041` / `fact_044` → keeps [[q_000]] open and code frozen.

## Completeness matrix

| Area | Combined status | Blocking evidence still needed |
|---|---|---|
| Scope/source of truth | CLOSED | None for static phase |
| VoN/radio controls | PARTIAL | Vanilla source + runtime semantics |
| Per-link audio | OPEN | VON ACP/SIG binding and runtime audio |
| Relay/intercom | OPEN | Vanilla implementation + runtime leak tests |
| Rpl/JIP/streaming | PARTIAL | Far-radio design and runtime profiling |
| Workbench build | PARTIAL | Read-only project inspection and compile after ARE |
| Legal/Workshop/BE | PARTIAL | BE-specific proof; RE currently prohibited |
| HDSP provenance/calibration | PARTIAL | Per-layer mapping and datasets |
| 30/60 scale | OPEN | Runtime budgets |
| Core extensibility | PARTIAL | Executable contract suite |
| Verification | PARTIAL | Runtime/multi-client evidence |
| Cellular/SMS | PARTIAL | Queue/persistence runtime validation |
| Reference mods | CLOSED policy | Per-file review if any reuse is proposed |
| Packaging/operations | PARTIAL | CI/bundle/rollback implementation |
| Two-agent cross-audit | BLOCKED | Claude audit of Codex dossier |

