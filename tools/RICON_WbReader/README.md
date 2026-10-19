# RICON_WbReader (TOOL_ONLY)

Read-only Workbench plugin for `[Workbench]` orders (`docs/evidence/WB-order-001.md`). Not part of the mod, not for Workshop.

- Runs only through the documented Workbench CLI (`-wbModule=ScriptEditor -plugin=RICON_WbReaderPlugin`, see
  `Page_CLI.html`); reads resources with `Workbench.SearchResources` + `FileIO.OpenFile(..., FileMode.READ)`.
- No copying, writing or unpacking of game data; no Play, world, server or runtime.
- Prints at most 15 source lines per run into the Workbench log (outside the repo); whole files are never printed.
- Vanilla content is not committed: reports keep resource path, line numbers, short quotes and a paraphrase.
- Usage: `run.ps1` (unique log folder per run).
