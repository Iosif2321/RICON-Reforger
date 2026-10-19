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

Владелец разрешил (2026-10-19, чат; подтверждено codex в своём окне, agora #777/#778): выполняет claude, проверяет
логи codex; только чтение. Инструмент — `tools/RICON_WbReader/` (плагин Workbench, `ResourceDatabase.SearchResources` +
`FileIO.OpenFile(FileMode.READ)`, не больше 15 строк исходника за запуск, логи вне репозитория:
`%USERPROFILE%\Documents\My Games\ArmaReforgerWorkbench\logs\ricon-wbreader-<имя>-<время>\`). Игра 1.8.0.13.
Уровень всех выводов — `SOURCE_EVIDENCE`: видимая структура кода; поведение в игре — `НЕ УСТАНОВЛЕНО` / `BLOCKED_NO_RUNTIME`.

Пути ниже — точные пути Workbench (`$ArmaReforger:…`); номера строк — для версии 1.8.0.13.

### Пункт 3 — `ECommMethod` (логи `ecomm-*`)

- Файл `$ArmaReforger:scripts/Game/generated/ECommMethod.c`, помечен как сгенерированный («Do not modify, this script
  is generated»), т. е. перечисление задаёт движок.
- Ровно 5 значений, без явных чисел: `DIRECT` (стр. 12, комментарий «Speaking»), `SQUAD_RADIO` (16, «Soldiers squad
  radio»), `DIRECT_AI` (20), `GAME_MASTER_DIRECT` (24, «Game master speaking direct»), `GAME_MASTER_RADIO` (28, «Game
  master using radio»).
- Вывод для RICON: своего значения метода связи в перечислении нет и добавить его скриптом нельзя (generated); радио
  RICON должно ехать поверх `SQUAD_RADIO`.

### Пункт 4 — `SCR_VoNComponent.OnReceive`, `SCR_VONController`, `SCR_VONEntryRadio` (логи `i4-*`)

- `$ArmaReforger:scripts/GameCode/Network/SCR_VoNComponent.c:78–86`: `OnReceive(int playerId, bool isSenderEditor,
  BaseTransceiver receiver, int frequency, float quality)` — событие движка. Тело только передаёт аргументы в
  `SCR_VonDisplay.OnReceive(...)` (UI) и в `m_OnReceivedVON.Invoke(playerId, receiver, frequency, quality)`.
  **Звук скрипт не меняет**: ни громкости, ни фильтров, ни отказа в приёме.
- `quality` дальше: `SCR_VonDisplay.c:213` сохраняет её в `m_fQuality` структуры передачи; других использований
  `m_fQuality` нет (стр. 18, 213, 827 — объявление, запись, повторная передача в `OnReceive`). На событие
  `m_OnReceivedVON` подписан только `SCR_NameTagData.c:679/717` (UI имён).
- Выбор трансивера при передаче — `$ArmaReforger:scripts/Game/VON/SCR_VONController.c:622–637`, `SetActiveTransmit`:
  если `entry.GetVONMethod() == ECommMethod.SQUAD_RADIO` → `SetCommMethod(SQUAD_RADIO)` и
  `SetTransmitRadio(SCR_VONEntryRadio.Cast(entry).GetTransceiver())`; иначе → `SetCommMethod(DIRECT)` и
  `SetTransmitRadio(null)`. Оба метода — `proto external` (`generated/Components/VoNComponent.c:27,30`), т. е.
  доставку голоса выполняет движок.
- Задержка частоты — `SCR_VONEntryRadio.c:31`, комментарий: «Local frequency getter since atm the transceiver getter is
  delayed (network?) and not sufficient for instant UI changes»; `GetEntryFrequency()` возвращает локальное
  `m_iFrequency`. Причина задержки в коде не объяснена («network?») — НЕ УСТАНОВЛЕНО.
- Вывод для RICON (H3/Q2): из скрипта приёма нельзя управлять слышимостью — только наблюдать `quality`/`frequency`.
  Влияние `quality` на звук, если есть, — в звуковом графе (пункт 2), не в скриптах.

### Пункт 5 — `SCR_CoverageRadioComponent`, `SCR_RadioCoverageSystem` (логи `i5-*`)

- Файлы: `scripts/Game/Components/SCR_CoverageRadioComponent.c` (`: ScriptedRadioComponent`),
  `scripts/Game/Systems/SCR_RadioCoverageSystem.c` (`: GameSystem`),
  `scripts/Game/Components/Locations/SCR_RadioCoverageMapDescriptorComponent.c`.
- Во всех трёх **нет** упоминаний `VoN`, `VON`, `Transmit` (поиск — 0 совпадений).
- Граф покрытия — списки `m_aRadiosInRange` / `m_aInRangeOfRadios` (стр. 22–23) с подсчётом по фильтру ключа
  шифрования (`GetRadiosInRangeCount(string encryptionFilter)`, стр. 91).
- Пользователи `SCR_RadioCoverageSystem.GetInstance`: сам компонент, `SCR_GameModeCampaign.c:739`,
  `SCR_CampaignFactionCommanderHandlerComponent.c:373,400` — логика режима Conflict.
- Вывод: по исходнику покрытие с доставкой голоса **не связано**; это игровая логика Conflict. Ретранслирует ли
  движок голос через `RelayTransceiver` — пункт 1; runtime — НЕ УСТАНОВЛЕНО.

### Пункт 8 — radial-меню VON (логи `i8-*`)

- Классы записей: `SCR_VONEntry : SCR_SelectionMenuEntry` (`Game/VON/SCR_VONEntry.c:3`),
  `SCR_VONEntryRadio : SCR_VONEntry` (`SCR_VONEntryRadio.c:3`), `SCR_VONEntryComponent`.
- `SCR_VONController.c:230–242`, `AddEntry(SCR_VONEntry entry)` — **публичный** метод: добавляет запись, вызывает
  `entry.InitEntry()`, запоминает long-range радио, шлёт `m_OnEntriesChanged`, делает запись активной, если активной нет.
- Ванильный код вызывает его извне: `SCR_GadgetManagerComponent.c:597,971`, `SCR_EditorManagerEntity.c:182,192`.
  Меню строится в `SCR_VONMenu.c:86–88` (`category.AddEntry` / `m_RadialMenu.AddEntry`).
- Вывод (Q19): свою запись (наследник `SCR_VONEntry`) можно добавить через публичный `AddEntry` **без modded-замены**.
  Ограничение: `SetActiveTransmit` знает только `SQUAD_RADIO`, всё остальное передаётся как `DIRECT` — запись RICON
  для радио должна возвращать `SQUAD_RADIO` и реальный `BaseTransceiver`. Отрисовка в меню — runtime, НЕ УСТАНОВЛЕНО.

### Ещё не выполнено

Пункты 1, 2, 6, 7, 9 (префабы, звуковой граф, конфиги, материалы). Пункт 10 закрыт компиляцией №003/004 выше.
