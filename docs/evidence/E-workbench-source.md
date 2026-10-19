# E-workbench-source — результаты Workbench

Ответы на заказы `[Workbench]`. Уровень — `WORKBENCH_COMPILE_PASS` / `SOURCE_EVIDENCE` по пунктам.

## Компиляция №003 (и повтор после исправления — №004)

Дата: 2026-10-19. Исполнитель: claude по **прямому разрешению владельца в чате** («разрешаю запустить компиляцию
Workbench»). Только проверка скриптов: без Play, без сервера, без мира, без распаковки `.pak`; ванильные файлы не
открывались.

- Игра: `ArmaReforgerScripted`, version **1.8.0.13**, built 2026-08-21 14:30:18 UTC (из лога Workbench).
- Workbench: `Arma Reforger Tools\Workbench\ArmaReforgerWorkbenchSteamDiag.exe`; Steam запущен.
- Ревизия: HEAD `6263310`, `git status` пуст; `git diff --exit-code 06979fc..HEAD -- addons` — без изменений
  (целевая ревизия заказа №003).

### Команда, которая сработала

```bat
ArmaReforgerWorkbenchSteamDiag.exe -gproj "<repo>\addons\<проект>\addon.gproj" ^
  -addonsDir "<Arma Reforger>\addons,<Arma Reforger Tools>\Workbench\addons,<repo>\addons" ^
  -wbModule=ScriptEditor -validate -wbSilent -exitAfterInit -noThrow ^
  -logLevel verbose -logsDir <имя> -log-scr-checksum
```

Отличия от `E-workbench-build.md`, найденные на практике:

1. **Нужен `-addonsDir` с папкой игры.** Без него: `Addon 'RICON_Core' dependency '58D0FB3206B6F859' can't be added`,
   `Game addon '58D0FB3206B6F859' not found`, `Cannot initialize game project settings!`, exit −1.
   `58D0FB3206B6F859` — GUID проекта `ArmaReforger` (`Arma Reforger\addons\data\ArmaReforger.gproj`), зависимость верная.
2. **Нужен запущенный Steam.** Без него: `Steamworks: SteamAPI_Init failed`, exit −1, хотя скрипты при этом компилируются.
3. **Нужен `-exitAfterInit`.** Со Steam, но без этого флага Workbench после «Script validation successful.» остаётся
   висеть (вход в backend) и не завершается — процесс пришлось закрыть.
4. Список файлов `-log-scr-checksum` пишется только при `-logLevel verbose` (или подробнее), не при `warning`.
5. Логи: `%USERPROFILE%\Documents\My Games\ArmaReforgerWorkbench\logs\<имя>\` (`console.log`, `error.log`, `script.log`).
6. Workbench создаёт кэш `addons\<проект>\resourceDatabase.rdb` — добавлен в `.gitignore`.

### Результаты

| Прогон | Проект | Ревизия аддонов | Код | Итог |
|---|---|---|---|---|
| 003 | `RICON_Core` | `06979fc` | 0* | `Script validation successful.` — WORKBENCH, PC, XBOX, PS4, PS5 |
| 003 | `RICON_CoreTests` | `06979fc` | — | `Script validation failed.` — 1 ошибка (×5 конфигураций) |
| 004 | `RICON_CoreTests` | `06979fc` + правка ниже | **0** | `Script validation successful.` — 5 конфигураций |
| 004 | `RICON_Core` | `06979fc` | **0** | `Script validation successful.` — 5 конфигураций |

\* в первом прогоне код не был считан (особенность PowerShell); в прогоне 004 — 0.

**Ошибка 003 (дословно):** `Scripts/Game/RICON_Tests/RICON_ExtFixtures.c(233): error: Broken expression (missing ';'?)`.
Причина: локальная переменная названа `reference` — в Enforce Script это зарезервированное слово (вывод по факту
ошибки; в офлайн-документации отдельного списка ключевых слов не найдено). Исправление: переименована в `baseline`.
Ядро `RICON_Core` ошибок не имело.

**Предупреждения:** 14 `'…' is obsolete` на конфигурацию — все в ванильных скриптах (`SCR_EditableEntityComponent.c`,
`CareerBackend.c`, `MainMenuUI.c` и др.). В файлах RICON предупреждений нет.

**Скомпилированные файлы RICON (`-log-scr-checksum`, прогон 004):**

| Файл | CRC |
|---|---|
| `Scripts/Game/RICON/Core/RICON_CommsManagerComponent.c` | `034528a3` |
| `Scripts/Game/RICON/Core/RICON_LinkSnapshot.c` | `2c666caa` |
| `Scripts/Game/RICON/Core/RICON_LinkTypes.c` | `a789603e` |
| `Scripts/Game/RICON/Network/RICON_NetworkContracts.c` | `dad7d7e6` |
| `Scripts/Game/RICON/Propagation/RICON_AntennaPattern.c` | `03de07e4` |
| `Scripts/Game/RICON/Propagation/RICON_IntelligibilityModel.c` | `f0ba1e8f` |
| `Scripts/Game/RICON/Propagation/RICON_LinkEvaluator.c` | `9a9d09d9` |
| `Scripts/Game/RICON/Propagation/RICON_NoiseModel.c` | `cef8bbf0` |
| `Scripts/Game/RICON/Propagation/RICON_PropagationModel.c` | `ad39ff79` |
| `Scripts/Game/RICON_Tests/RICON_ExtFixtures.c` | `4320a84a` |
| `Scripts/Game/RICON_Tests/RICON_TestSupport.c` | `f280bb40` |

### Статусы

- `RICON_Core` @ `06979fc`: **`WORKBENCH_COMPILE_PASS`**.
- `RICON_CoreTests` (EXT-1…5) @ коммит с правкой `reference` → `baseline`: **`COMPILE_FIXTURE_PASS`**.
- `SelfCheck()` не выполнялся (нужен runtime): `CONTRACT_PASS` — `NOT_TESTED`, поведение — `BLOCKED_NO_RUNTIME`.

## Заказ №001 — ванильные файлы

Не выполнен (ждёт ассистента владельца в Workbench).
