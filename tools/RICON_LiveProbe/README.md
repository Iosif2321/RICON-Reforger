# RICON_LiveProbe — прибор live-проверки (TOOL_ONLY)

Отдельный аддон для опытов по `docs/plan/LIVE-order-frequency-mute.md` и `docs/analysis/AUDIO_CHAIN_CONTROL_RU.md`
(трек 1). Не для Workshop, не зависит от `RICON_Core`; подключается только на время сессии через `-addonsDir`.
Любой запуск игры/Play — только по START → ACK → END на шине и в рамках разрешения владельца 2026-10-23.

## Состав

- `Scripts/Game/RICON_Live/RICON_LiveJournal.c` — чистый журнал событий: кольцевой буфер с последовательным номером,
  явной записью `GAP` при переполнении (счётчик потерь и время последнего потерянного) и строгим форматом строки
  `[RICON_LIVE] seq=… t=… side=… kind=… player=… device=… tr=… value=… text=…`. Engine-вызовов нет.
- `Scripts/WorkbenchGame/RICON_LiveJournalProbePlugin.c` — самопроверка журнала в Workbench (ScriptEditor, без
  мира): порядок, вытеснение, учёт потерь сразу после первого переполнения, ёмкость 2, формат, отсутствие слова FAIL.
- Хуки движка (наблюдатели) — следующий срез, по подтверждённым штатным invokers (`docs/evidence/E-von-windows-ea.md`):
  `SCR_BaseGameMode.GetOnPlayerSpawned()`, `SCR_VoNComponent.GetOnVoNUsed()`, `m_OnCaptureVON`, `m_OnReceivedVON`.

## Запуск самопроверки (Workbench, без мира)

```
ArmaReforgerWorkbenchSteamDiag.exe -gproj "<repo>\tools\RICON_LiveProbe\addon.gproj"
  -addonsDir "<Steam>\Arma Reforger\addons,<Steam>\Arma Reforger Tools\Workbench\addons" -noBackend
  -wbModule=ScriptEditor -plugin=RICON_LiveJournalProbePlugin -wbSilent -exitAfterInit -noThrow
  -logLevel verbose -log-scr-checksum -logsDir ricon-livejournal-<stamp>
```

PASS — ровно одна строка `[RICON_LIVEJ] done: PASS`, `SCRIPT (E)` = 0 (строки «Resource leaks» и Steamworks
`RequestCurrentStats` — штатный шум). Exit 0 сам по себе ничего не доказывает.

## Правила журнала

- Теги не содержат слова FAIL (gate `regress_wb.ps1`); ключи шифрования не пишутся.
- Времена — источника события (сторона указана), общей аудиошкалы нет; разрешение не заявляется тоньше каденции
  native-событий.
- `TX_CAPTURE` — колбэк `OnCapture` каждый кадр записи; `TX_START`/`TX_STOP` — только после отдельной детекции фронта.
