# E-workbench-source — результаты Workbench

Ответы на заказы `[Workbench]`. Уровень — `WORKBENCH_COMPILE_PASS` / `SOURCE_EVIDENCE` по пунктам.

## Компиляция №003 (и повтор после исправления — №004)

Дата: 2026-10-19. Исполнитель: claude по **прямому разрешению владельца в чате** («разрешаю запустить компиляцию
Workbench»). Только проверка скриптов: без Play, без сервера, без мира, без распаковки `.pak`; ванильные файлы не
открывались. Ревью сырых логов — codex (agora #759, #762).

- Игра: `ArmaReforgerScripted`, version **1.8.0.13**, built 2026-08-21 14:30:18 UTC (из лога Workbench).
- Workbench: `C:\Program Files (x86)\Steam\steamapps\common\Arma Reforger Tools\Workbench\ArmaReforgerWorkbenchSteamDiag.exe`;
  Steam запущен.
- Ревизия: HEAD `6263310`, `git status` пуст; `git diff --exit-code 06979fc..HEAD -- addons` — без изменений
  (целевая ревизия заказа №003).

### Команда, которая сработала

Запуск через PowerShell `Start-Process … -PassThru`, код возврата — `$p.ExitCode`. Фактическая строка параметров
из `console.log` (строка 10, `CLI Params:`), прогон 004 `RICON_Core`:

```text
-gproj C:\Users\Ded\Documents\Projects\ARMA 3 Scripts\RICON-Reforger\addons\RICON_Core\addon.gproj -addonsDir C:\Program Files (x86)\Steam\steamapps\common\Arma Reforger\addons,C:\Program Files (x86)\Steam\steamapps\common\Arma Reforger Tools\Workbench\addons,C:\Users\Ded\Documents\Projects\ARMA 3 Scripts\RICON-Reforger\addons -wbModule ScriptEditor -validate -wbSilent -exitAfterInit -noThrow -logLevel verbose -logsDir ricon-validate-004-RICON_Core -log-scr-checksum
```

Прогон 004 `RICON_CoreTests` — та же строка с `addons\RICON_CoreTests\addon.gproj` и
`-logsDir ricon-validate-004-RICON_CoreTests`.

Отличия от `E-workbench-build.md`, найденные на практике:

1. **Нужен `-addonsDir` с папкой игры.** Без него: `Addon 'RICON_Core' dependency '58D0FB3206B6F859' can't be added`,
   `Game addon '58D0FB3206B6F859' not found`, `Cannot initialize game project settings!`, exit −1.
   `58D0FB3206B6F859` — GUID проекта `ArmaReforger` (`Arma Reforger\addons\data\ArmaReforger.gproj`), зависимость верная.
2. **Нужен запущенный Steam.** Без него: `Steamworks: SteamAPI_Init failed`, exit −1, хотя скрипты при этом компилируются.
3. **Нужен `-exitAfterInit`.** Со Steam, но без этого флага Workbench после «Script validation successful.» остаётся
   висеть (вход в backend) и не завершается — процесс пришлось закрыть.
4. Список файлов `-log-scr-checksum` пишется только при `-logLevel verbose` (или подробнее), не при `warning`.
5. Логи: `%USERPROFILE%\Documents\My Games\ArmaReforgerWorkbench\logs\<logsDir>\` (`console.log`, `error.log`, `script.log`).
6. Workbench создаёт кэш `<проект>\resourceDatabase.rdb` — добавлен в `.gitignore` (`**/resourceDatabase.rdb`).

### Результаты

| Прогон | Проект | Ревизия аддонов | Код | Итог |
|---|---|---|---|---|
| 003 | `RICON_Core` | `06979fc` | не считан* | `Script validation successful.` — WORKBENCH, PC, XBOX, PS4, PS5 |
| 003 | `RICON_CoreTests` | `06979fc` | не считан* | `Script validation failed.` — 1 ошибка (×5 конфигураций) |
| 004 | `RICON_CoreTests` | `06979fc` + правка ниже (`1c72797`) | **0** | `Script validation successful.` — 5 конфигураций |
| 004 | `RICON_Core` | `06979fc` | **0** | `Script validation successful.` — 5 конфигураций |

\* в прогоне 003 код не был считан (PowerShell без `$p.Handle`). Коды прогона 004 (`0`) сняты оператором (claude) из
`Start-Process`; сам лог код возврата не хранит, но независимо подтверждает успех: `Script validation successful.`
и 0 строк `error:` (проверено codex, #759/#762).

Сырые логи хранятся вне репозитория (путь выше):

| Прогон 004 | `console.log` SHA-256 | `error.log` SHA-256 |
|---|---|---|
| `RICON_Core` | `de4ade1b03fea8be33fc10d2ac9ffd0503cfb2db0693b555ab89f8dcdd266c85` | `d11ec4c9f69d00c1299ef4e07bda66c34cc61b66d5bc93431703898ee24cc254` |
| `RICON_CoreTests` | `b185778d6126c04ff92129573289445b60c0a54a321f363b0e54076d6b281b96` | `414dde6b44931dd137d01f7b0eb6a6addd958e7b68846d5a1421309d9c821d9c` |

**Ошибка 003 (дословно):** `Scripts/Game/RICON_Tests/RICON_ExtFixtures.c(233): error: Broken expression (missing ';'?)`.
Строка 233 объявляла локальную переменную `reference`. Идентификатор отвергнут парсером; вероятно, это ключевое
слово Enforce Script — **не подтверждено**: списка ключевых слов в офлайн-документации нет, вывод только по тому, что
после переименования в `baseline` ошибка исчезла. Правило для кода: не использовать `reference` как имя.
Ядро `RICON_Core` ошибок не имело.

**Предупреждения:** в `error.log` каждого прогона 004 — 84 строки `(W)`: 14 различных предупреждений, повторённых при
компиляции на старте и в каждой из 5 конфигураций. Все — в ванильных скриптах; в файлах RICON — 0. Список (дословно,
из вывода `-validate`):

```text
Scripts/Game/Campaign/SCR_PlayerProfileManagerComponent.c(142): warning: 'PlayerData' is obsolete
Scripts/Game/DataCollection/PlayerData/SCR_PlayerData.c(210): warning: 'PlayerData' is obsolete
Scripts/Game/DataCollection/SCR_DataCollectorComponent.c(481): warning: 'GetStorage' is obsolete
Scripts/Game/DataCollection/SCR_DataCollectorComponent.c(483): warning: 'GetOnlineWritePrivilege' is obsolete
Scripts/Game/Editor/Components/EditableEntity/SCR_EditableEntityComponent.c(785): warning: 'GetPlayerPlatformKind' is obsolete
Scripts/Game/Editor/Components/EditableEntity/SCR_EditableEntityComponent.c(786): warning: 'GetPlayerPlatformId' is obsolete
Scripts/Game/UI/Backend/CareerBackend.c(159): warning: 'GetDSSession' is obsolete
Scripts/Game/UI/Backend/CareerBackend.c(162): warning: 'RoomID' is obsolete
Scripts/Game/UI/Backend/CareerBackend.c(197): warning: 'GetDSSession' is obsolete
Scripts/Game/UI/Backend/CareerBackend.c(201): warning: 'RoomID' is obsolete
Scripts/Game/UI/Menu/MainMenuUI.c(643): warning: 'GetNotifyCount' is obsolete: Will be substitued by Notification Api.
Scripts/Game/UI/Menu/MainMenuUI.c(645): warning: 'GetNotifyItem' is obsolete: Will be substitued by Notification Api.
Scripts/Game/Utilities/SCR_PlayerIdentityUtils.c(20): warning: 'GetPlayerIdentityId' is obsolete
Scripts/Game/Workshop/SCR_AddonManager.c(876): warning: 'GetRefCount' is obsolete: Will be removed
```

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

- `RICON_Core` @ `06979fc` (не менялся в `1c72797`): **`WORKBENCH_COMPILE_PASS`**.
- `RICON_CoreTests` (EXT-1…5) @ `1c72797`: **`COMPILE_FIXTURE_PASS`**.
- `SelfCheck()` не выполнялся (нужен runtime): `CONTRACT_PASS` — `NOT_TESTED`, поведение — `BLOCKED_NO_RUNTIME`.

## Заказ №001 — ванильные файлы

Не выполнен. Владелец разрешил (2026-10-19, чат) выполнить его claude в паре с codex, только чтение. Способ чтения
согласуется в agora (#757); прототип инструмента — `tools/RICON_WbReader/` (не запускался).
