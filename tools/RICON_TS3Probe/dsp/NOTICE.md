# NOTICE — RICON Cellular System

This NOTICE file documents third-party references, attribution boundaries, and the license policy for RICON.

RICON Cellular System is licensed under **GNU GPL-3.0-only**. See [LICENSE](LICENSE).

This file is not legal advice. It is the project’s engineering/legal hygiene ledger: what was used as conceptual or technical reference, what is a direct implementation source, and what must remain clean-room only.

## Copyright

Copyright (C) 2026 RICON contributors.

## Current project license

- Repository license: **GNU GPL-3.0-only**.
- Contributions to this repository are expected to be compatible with GPL-3.0-only unless a file explicitly states otherwise.
- If a third-party permissive component is added later, keep its original copyright/license notice and record it here.

## Core source policy

- RICON has **no direct code dependency** on ACRE2, TFAR, ACREWT, Sionna, DiffeRT, Steam Audio, splatlib/ITWOM or similar reference projects.
- RICON does **not** copy runtime code, documentation text, assets, comments, configs, formulas-as-code, or line-by-line translations from those projects.
- Those projects were used only as **conceptual and engineering references**: to understand existing mod behavior, common failure modes, UX/API constraints, and broad approaches to similar problems.
- Direct technical reliance for implementation is limited to official/external primary sources such as **TeamSpeak Plugin SDK/documentation** and radio/cellular/telecommunications standards or regulatory documentation: propagation recommendations, frequency/radio rules, cellular/radio engineering references, and similar normative material.

## Reference / attribution ledger

| Project / source | Local path used for audit | Observed license/status | RICON policy | Current use |
|---|---|---|---|---|
| ACRE2 | `..\acre2` | GPL-3.0; some subfolders have extra restrictions | **CONCEPTUAL / ENGINEERING REFERENCE ONLY** | studied behavior, architecture trade-offs and failure modes; no dependency and no copied code/docs/assets/config |
| ACREWT-Wired-Telephones | `..\ACREWT-Wired-Telephones` | GPL-3.0 | **CONCEPTUAL / HISTORICAL REFERENCE ONLY** | reviewed as historical planning context; the former separate migrated planning note was removed during docs consolidation; no retained copied text required for implementation |
| Task Force Arrowhead Radio (TFAR) | `..\task-force-arma-3-radio` | Arma Public License Share Alike / restrictive project terms | **CONCEPTUAL / ENGINEERING REFERENCE ONLY** | studied behavior/UX/IPC problem space; no dependency and no copied code/docs/assets/config |
| Sionna / Sionna RT | `..\SIONNA`, `..\sionna-rt` | Apache-2.0 | **CONCEPTUAL / ENGINEERING REFERENCE ONLY** | studied high-level ideas around noise, materials, Doppler and ray/field concepts; no direct code adoption in current project |
| DiffeRT | `..\DiffeRT` | MIT | **CONCEPTUAL / ENGINEERING REFERENCE ONLY** | studied high-level Fresnel/image-method ideas; no direct code adoption in current project |
| Steam Audio | `..\steam-audio` | Apache-2.0 | **CONCEPTUAL / ENGINEERING REFERENCE ONLY** | studied real-time budget/cache/bake approaches; no direct code adoption in current project |
| splatlib / ITWOM | `..\splatlib` | local checkout has no root LICENSE despite source comments referencing one; ITM/ITWOM lineage is treated cautiously | **CONCEPTUAL / ENGINEERING REFERENCE ONLY** | studied the Longley-Rice/ITWOM problem space; direct implementation should come from public standards/normative docs, not copied source |
| TeamSpeak Plugin SDK / documentation | fetched or supplied via `tsplugin/CMakeLists.txt` plus official TeamSpeak documentation | external SDK terms controlled by TeamSpeak | **DIRECT TECHNICAL SOURCE / external SDK** | required plugin API headers/function signatures and official behavior contract |
| Radio/cellular standards and regulatory documentation | public standards/specs/regulatory references | standards/regulatory text; not project code | **DIRECT TECHNICAL SOURCE** | propagation, frequency/radio/cellular rules, engineering constraints and formulas implemented cleanly |
| Arma 3 / CBA_A3 / HEMTT | external runtime/build dependencies | respective upstream licenses | external dependencies | not bundled as RICON source |

## Documentation consolidation

The project documentation was consolidated to avoid a large Obsidian graph of planning/research fragments:

- short entry point: `README.md`;
- single technical wiki source: `docs/WIKI.md` and mirrored `docs/WIKI_RU.md`;
- generated HTML wiki site: `docs/site/`;
- player install guide copied into dist: `tools/dist/install_ru.txt`;
- legal ledger: this `NOTICE.md` and `LICENSE`.

The former separate research/planning/license-audit Markdown files under `docs/` were removed as standalone documents. Their current useful content is represented only as short clean-room summaries in `docs/WIKI.md` and this NOTICE, without retaining copied source text from reference projects.

## Clean-room rules

1. **Reference projects such as ACRE2, TFAR, ACREWT, Sionna, DiffeRT, Steam Audio, splatlib/ITWOM and similar repos:**
   - OK: read, summarize, cite behavior, cite public standards, write fresh implementation from project requirements.
   - OK: use them to understand the engineering space, common bugs, UX constraints and solution patterns.
   - Not OK: paste code, translate line-by-line, reuse assets, copy comments, copy config data, copy documentation text, or import binary artifacts.

2. **Primary implementation sources:**
   - Direct implementation should be based on official TeamSpeak Plugin SDK/documentation and public radio/cellular/telecommunications standards or regulatory documentation.
   - Standards-derived formulas should be implemented cleanly and cited by standard/reference name where practical.

3. **Future direct third-party adoption:**
   - If the project ever intentionally adopts third-party code/text/assets, it requires an explicit owner decision, compatible license review, attribution, file headers where needed, and an updated NOTICE entry.

4. **TeamSpeak SDK:**
   - Keep SDK headers external/fetched where possible.
   - If vendored later, record exact SDK version, license/terms, and source URL here.

## Audit summary

The local audit on 2026-06-26 compared RICON against the reference paths supplied by the project owner.

- Runtime/source files scanned: 67 RICON source/config/build files.
- Reference source chunks indexed: ACRE2, Sionna, DiffeRT, splatlib, Steam Audio, sionna-rt, TFAR, ACREWT.
- Exact normalized whole-file code matches: **0**.
- Exact normalized 7-line code matches: **0**.
- Exact normalized 5-line code matches: **0**.
- Exact normalized 3-line code matches: 9 boilerplate-only hits (`strategy/fail-fast/matrix`, common C/C++ includes), not evidence of copy-paste.
- Structural 80-token code-window matches: **0**.

The GPL-3.0-only project license is therefore **not** a claim that runtime code, docs or assets were copied from ACRE2/TFAR/ACREWT or other reference projects. The current project position is: no copied code/docs/assets and no direct dependency on those reference projects; only conceptual/engineering learning from them, with direct technical implementation grounded in TeamSpeak documentation/SDK and radio/cellular standards or regulatory references.
