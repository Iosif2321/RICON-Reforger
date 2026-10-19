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

### Пункт 1 — `RelayTransceiver` (логи `i1-*`)

- Класс: `$ArmaReforger:scripts/Game/generated/Radio/RelayTransceiver.c:12` — `class RelayTransceiver: BaseTransceiver {}`,
  сгенерированный, **без собственного скриптового API** (тело пустое).
- Префабы (поиск по `.et`, первые 15 — лимит вывода, полный список может быть длиннее): вышки
  `Prefabs/Structures/Infrastructure/Towers/TransmitterTower_01/TransmitterTower_01{,_small,_medium}_base.et`, антенны
  `Prefabs/Props/Military/Compositions/Antenna_0{1,2}_base.et`, Conflict `Prefabs/Systems/MilitaryBase/{ConflictRelayRadio,
  ConflictMilitaryBase,ConflictBase_Base,ConflictControlPoint}.et`, `Prefabs/MP/Campaign/…MobileAssembly*.et`, командные
  и ремонтные грузовики `Ural4320_{command,repair}.et`, `M923A1_command.et`.
- Атрибуты в префабах (только переопределения): `"Transmitting Range"` — вышка 5000 (`TransmitterTower_01_base.et:95`),
  `ConflictRelayRadio` 3000 (:15), `Ural4320_command` 2000 (:14); `"Frequency resolution" 1` (`ConflictRelayRadio.et:16`).
- Родительский компонент разный: `ScriptedRadioComponent` (вышка, :92), `SCR_CoverageRadioComponent` с `"Turned on" 0`
  (`ConflictRelayRadio.et:11–12`), `BaseRadioComponent` (Ural, :11). Частот вход/выход, мощности, ключа у самого
  `RelayTransceiver` в префабах нет.
- Скрипты используют его только в `SCR_CampaignMilitaryBaseComponent.c:1136–1152, 2410–2414`: перебор трансиверов и
  `GetRange()` для дальности базы (Conflict).
- Вывод (Q1, gate M1): логики пересылки голоса в скриптах и префабах **нет**. Делает ли движок ретрансляцию VoN через
  `RelayTransceiver` — **НЕ УСТАНОВЛЕНО** (`BLOCKED_NO_RUNTIME`, Э7r).

### Пункт 2 — звуковой граф VON — **НЕ ПРОЧИТАН**

- Найдены `$ArmaReforger:Sounds/VON/von.acp` и `$ArmaReforger:Sounds/VON/Von.sig`, но `FileIO.OpenFile(READ)` их **не
  открывает** («cannot open», логи `i2-*`). Ранний поиск `TransmissionQuality` по ним — недействителен (файл не прочитан).
  После этого инструмент стал строгим: непрочитанный кандидат → `INCOMPLETE`, exit 4, без PASS (лог `i2-strict-*`).
- Нужен другой путь только для чтения: Audio Editor (GUI, ассистент владельца) или отдельно согласованный способ
  через API ресурсов Workbench. До этого Q2/H3 по звуку — OPEN.

### Пункт 6 — носимые рации (логи `i6-*`)

Префабы `$ArmaReforger:Prefabs/Items/Equipment/Radios/`: `Radio_ANPRC68`, `Radio_ANPRC77`, `Radio_R148`(`_FIA`),
`Radio_R107M`(`_FIA`), `Radio_Deployable_Base`, `Radio_RF10/Radio_RF10_base`. Наследуются от
`Prefabs/Items/Core/Radio_base.et` (там `BaseRadioComponent` и `SCR_RadioComponent` со звуками настройки).

| Рация | Ключ шифрования | Трансиверы | Частота канала | Диапазон | Шаг | Дальность |
|---|---|---|---|---|---|---|
| ANPRC-68 (`Radio_ANPRC68.et:4–21`) | `"chickenNuggets"` | 2 × `RadioTransceiver` | 32000 и 48000 | 32000–68000 | 200 | 1300 |
| R-148 (`Radio_R148.et:4–14`) | `"coldBorscht"` | 2 (второй — с :15, не читался) | 38000 | 38000–54000 | 200 | 1300 |
| ANPRC-77 (`Radio_ANPRC77.et:4–15`, от `Radio_Deployable_Base`) | — (в префабе нет) | ≥ 2 | — | 30000–76000 | 200 | 2000 |

- Атрибуты `RadioTransceiver`: `ChannelFrequency`, `"Transmitting Range"`, `"Min tunable frequency"`, `"Max tunable
  frequency"`, `"Frequency resolution"`; у `BaseRadioComponent`: `"Encryption key"`, `"Turned on"`.
- Единицы частот в префабах не подписаны; по значениям (32000…76000 при реальных диапазонах 30–76 МГц) это, вероятно,
  кГц — **inference**. Единица дальности (вероятно, метры) — тоже не подписана.
- Рации техники: `Ural4320_command` — `BaseRadioComponent` с `RelayTransceiver` 2000 (пункт 1); подробно не читались.

### Пункт 7 — ввод (лог `i7-von-*`)

`$ArmaReforger:Configs/System/chimeraInputCommon.conf`:
- контекст `VONContext` (:8848): действия `VONDirect` (:8852), `VONChannel` (:8862), `VONDirectToggle` (:8879),
  `VONLongRangeToggle` (:8898), `VONTransceiverCycle` (:8930);
- контекст `VONMenuContext` (:9019): `VONMenuAction` (:9023), `VONMenuMuteAction` (:9041), `VONMenuTuneFrequency`
  (:9059), `VONMenuCycleChannel` (:9084);
- действие `VONMenu` (:4723) и контекст `VONMenuOpeningContext` (:9119).
- Привязки клавиш не читались (не нужны для Q18 на этом этапе).

### Пункт 9 — материалы (логи `i9-*`)

- Скрипты получают материал присваиванием: `GameMaterial material = trace.SurfaceProps;` (например,
  `SCR_ImpactEffectComponent.c:417`, `SCR_MineAnimationComponent.c:85`, `SCR_RecoilForceAimModifier.c:108`), затем
  `material.GetBallisticInfo()` (`SCR_RecoilForceAimModifier.c:111`).
- `GameMaterial` (`generated/Core/GameMaterial.c:14–18`): `GetVehicleInfo`, `GetSoundInfo`, `GetHitEffectInfo`,
  `GetBallisticInfo`, `GetParticleEffectInfo`.
- `BallisticInfo` (`generated/Core/BallisticInfo.c:14–18`): `AllowsDeflection()`, `GetDensity()`, `GetThickness()`,
  `GetThicknessMax()`, `IsWaterSurface()`.
- Примеры `Density` из `$ArmaReforger:Common/Materials/Game/*.gamemat`: `concrete` 2.3 (:16), `wood_logs` 0.7,
  `wood_floor` 0.85, `metal_sheet` 2.7, `metal_grid` 7.8, `metal_wire_mesh` 0.001. Единица не подписана (по значениям —
  вероятно г/см³, inference).
- Вывод для Э4 (потери на препятствиях): плотность и толщина доступны скрипту через `TraceParam.SurfaceProps` →
  `GameMaterial` → `BallisticInfo`. Значения рассчитаны на баллистику, не на радиоволны; коэффициенты затухания RICON
  берутся из своего конфига, а плотность — лишь ключ для выбора категории (решение Э4).

### Итог по №001

| Пункт | Статус |
|---|---|
| 1, 3, 4, 5, 6, 7, 8, 9 | `SOURCE_EVIDENCE` (выше); поведение — НЕ УСТАНОВЛЕНО |
| 2 (звуковой граф) | **OPEN**: `.acp`/`.sig` не открываются через `FileIO`; нужен Audio Editor или согласованный способ |
| 10 | закрыт компиляцией №003/004 |
