# RICON for Arma Reforger

Wireless communications simulation platform for Arma Reforger: server-authoritative signal model (HDSP),
radio networks with towers and repeaters, and (later) cellular networks — on top of the built-in VoN voice.

Status: **pre-alpha, static phase** — the game is not launched yet; Enforce code compiles in Workbench
(`WORKBENCH_COMPILE_PASS`, 2026-10-19, `docs/evidence/E-workbench-source.md`); host-side oracle checks pass (`ORACLE_PASS`);
behaviour in the game is not tested yet (runtime verification is stage Э1r). See `docs/plan/PLAN_RU.md` (plan v5) and `docs/plan/STATUS_RU.md`.

## Layout

| Path | Purpose |
|---|---|
| `addons/RICON_Core/` | Reforger addon (Enforce Script): link evaluation, noise/antenna/intelligibility seams, snapshots, routing, comms manager |
| `addons/RICON_CoreTests/` | TEST_ONLY addon: compile fixtures for the five extension scenarios; not published |
| `oracle/` | host-side reference (Python stdlib): link-eval, snapshot and route vectors with runners (`ORACLE_PASS`) |
| `docs/req/` | requirements written from primary sources (link evaluation, core contracts, lifecycle) |
| `docs/review/` | reviews (core, API, compile readiness) |
| `docs/adr/` | architecture decision records (ADR-002: core contracts) |
| `docs/evidence/` | evidence notes with levels DOC/SOURCE/STD_COPY/…; `codex-are/` — immutable research snapshots (SHA-256 manifest); `WB-order-*.md` — Workbench requests |
| `docs/plan/` | agreed plan and live status (`STATUS_RU.md`) |

First Workbench compile: `docs/evidence/E-workbench-source.md` (orders `WB-order-003.md`/`004`).

## Rules

- Only built-in VoN; no TeamSpeak, DLLs, external processes.
- Clean-room: Coalition-VON / Lima-Charlie are ideas-only (no license); TFAR (APL-SA) and ACRE2 (GPL-3.0) —
  ideas-only by project policy, no code or assets copied.
- No reverse engineering of the game or Tools (EULA, `Arma Reforger Tools/Docs/License.txt`, "You May Not").
  Vanilla scripts and assets are studied only through Workbench.

## License

RICON-Reforger is licensed under the **Arma Public License Share Alike (APL-SA)** — see `LICENSE`
(official text: <https://www.bohemia.net/en/licenses/arma-public-license-share-alike>).

In short (the full text in `LICENSE` is binding):

- **Attribution** — credit "RICON-Reforger by Iosif2321" and link to this license;
- **Noncommercial** — no commercial use or monetization;
- **Arma Only** — the material may not be converted or adapted for games other than Arma;
- **Share Alike** — adaptations may be distributed only under APL-SA (or a listed compatible license).

This is a new, independent codebase. The earlier RICON for Arma 3 stays under GPL-3.0; no GPL code from it
was copied here — see `NOTICE.md` for provenance and the clean-room statement.
