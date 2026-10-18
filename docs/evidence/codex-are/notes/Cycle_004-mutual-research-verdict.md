# Cycle 004 — Mutual research verdict

## [SYNTHESIZER'S PROPOSAL]

- Accept the combined Claude and Codex evidence corpus as complete enough to expose every material section and every remaining gap.
- Keep engine behavior, audio binding, relay semantics, far-radio streaming, BattlEye, calibration, performance, and multi-client audio explicitly open or blocked.
- Remove reverse engineering from the active path under the current Tools EULA.
- Use the clean new `RICON-Reforger` repository as canonical implementation target after owner confirmation.
- Resolve Workshop licensing before any code is committed or distributed.

## [CRITIC'S AUDIT]

- Mutual review does not turn open runtime questions into answers.
- The Workshop/GPL conclusion is a legal interpretation, not legal advice; do not pick a dual-license scheme without owner/legal approval.
- The legacy repository has multiple authors; only inspected out-of-scope Gak88 contributions were identified, so do not make global sole-authorship claims.
- P.526-15 evidence cannot be silently projected onto P.526-16.
- Existing uncommitted Enforce scaffolds were written before research-first freeze and have not compiled in Workbench.

## [RESOLUTION]

- Claude cross-audit requirement [[q_015]] → `CLOSES` via agora ids 354-366.
- Reverse-engineering questions [[q_007]] / [[q_021]] → `CLOSES` as prohibited under the current EULA.
- Workshop license → `NEEDS_USER`, new [[q_027]].
- Competing targets → `NEEDS_USER`, new [[q_028]], with both agents recommending the clean new repository.
- HDSP L2 and rain claims → `NARROWS`: delta-Bullington evidence is version-qualified; Epstein-Peterson/rain remain legacy heuristics.
- Overall implementation gate [[q_000]] → `needs_user_input`; code remains frozen.

## Final combined coverage

| Area | Verdict |
|---|---|
| Scope / reference reuse / evidence boundary / reciprocal audit | CLOSED |
| VoN controls, Rpl, Workbench, legal/Workshop, HDSP provenance, cellular, packaging | PARTIAL |
| Per-link audio, relay behavior, scale budgets, executable contract coverage | OPEN |
| Reverse engineering | BLOCKED_LEGAL |
| Runtime/audio/multi-client/BattlEye | BLOCKED_NO_RUNTIME or OPEN_NO_PRIMARY_PROOF |
| Workshop license and canonical target | OWNER DECISION REQUIRED |

