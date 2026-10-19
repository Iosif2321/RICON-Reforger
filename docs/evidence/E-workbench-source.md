# E-workbench-source — результаты Workbench

Ответы на заказы `[Workbench]` и прогоны тестов в Workbench. Уровень — `WORKBENCH_COMPILE_PASS` / `SOURCE_EVIDENCE` /
`ENFORCE_PARITY` по пунктам.

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
- Вывод для RICON: публичной точки расширения для нового значения не найдено (файл сгенерирован движком); текущее
  сопоставление радио RICON — `SQUAD_RADIO` (контроллер различает только его и `DIRECT`, пункт 4); возможность своего или
  native-расширения — НЕ УСТАНОВЛЕНО.

### Пункт 4 — `SCR_VoNComponent.OnReceive`, `SCR_VONController`, `SCR_VONEntryRadio` (логи `i4-*`)

- `$ArmaReforger:scripts/GameCode/Network/SCR_VoNComponent.c:78–86`: `OnReceive(int playerId, bool isSenderEditor,
  BaseTransceiver receiver, int frequency, float quality)` — событие движка. Тело только передаёт аргументы в
  `SCR_VonDisplay.OnReceive(...)` (UI) и в `m_OnReceivedVON.Invoke(playerId, receiver, frequency, quality)`.
  **Звук скрипт не меняет**: ни громкости, ни фильтров, ни отказа в приёме.
- `quality` дальше: `SCR_VonDisplay.c:213` сохраняет её в `m_fQuality` структуры передачи; других использований
  `m_fQuality` нет (стр. 18, 213, 827 — объявление, запись, повторная передача в `OnReceive`). На событие
  `m_OnReceivedVON` подписан только `SCR_NameTagData.c:679/717` (UI имён).
- Отклонение от правила минимизации: окна `i4-onreceive-93` и `i4-onreceive-108` прочитаны лишними (метод кончается
  на :86); ~30 строк кода редактора к заказу не относятся и в отчёт не вошли. Правило (agora #789): не больше 2 смежных
  окон одного файла на пункт, третье — только с обоснованием в отчёте.
- Выбор трансивера при передаче — `$ArmaReforger:scripts/Game/VON/SCR_VONController.c:622–637`, `SetActiveTransmit`:
  если `entry.GetVONMethod() == ECommMethod.SQUAD_RADIO` → `SetCommMethod(SQUAD_RADIO)` и
  `SetTransmitRadio(SCR_VONEntryRadio.Cast(entry).GetTransceiver())`; иначе → `SetCommMethod(DIRECT)` и
  `SetTransmitRadio(null)`. Оба метода — `proto external` (`generated/Components/VoNComponent.c:27,30`), т. е.
  настройку передачи принимает движок (native boundary; это не доказывает, что вся доставка — только движок).
- Задержка частоты — `SCR_VONEntryRadio.c:31`, комментарий: «Local frequency getter since atm the transceiver getter is
  delayed (network?) and not sufficient for instant UI changes»; `GetEntryFrequency()` возвращает локальное
  `m_iFrequency`. Причина задержки в коде не объяснена («network?») — НЕ УСТАНОВЛЕНО.
- Вывод для RICON (H3/Q2): в проверенных скриптах приёма влияния на слышимость **не видно** — они только наблюдают
  `quality`/`frequency`. Влияет ли `quality` на звук в native-коде движка или в звуковом графе (пункты 1/2) — НЕ
  УСТАНОВЛЕНО.

### Пункт 5 — `SCR_CoverageRadioComponent`, `SCR_RadioCoverageSystem` (логи `i5-*`)

- Файлы: `scripts/Game/Components/SCR_CoverageRadioComponent.c` (`: ScriptedRadioComponent`),
  `scripts/Game/Systems/SCR_RadioCoverageSystem.c` (`: GameSystem`),
  `scripts/Game/Components/Locations/SCR_RadioCoverageMapDescriptorComponent.c`.
- Во всех трёх **нет** упоминаний `VoN`, `VON`, `Transmit` (поиск — 0 совпадений).
- Граф покрытия — списки `m_aRadiosInRange` / `m_aInRangeOfRadios` (стр. 22–23) с подсчётом по фильтру ключа
  шифрования (`GetRadiosInRangeCount(string encryptionFilter)`, стр. 91).
- Пользователи `SCR_RadioCoverageSystem.GetInstance`: сам компонент, `SCR_GameModeCampaign.c:739`,
  `SCR_CampaignFactionCommanderHandlerComponent.c:373,400` — логика режима Conflict.
- Вывод: в трёх проверенных файлах **нет прямой текстовой связи** с доставкой голоса, а прямые пользователи — логика
  Conflict. Косвенная, native или runtime-связь этим не исключена — НЕ УСТАНОВЛЕНО (пункт 1, Э7r).

### Пункт 8 — radial-меню VON (логи `i8-*`)

- Классы записей: `SCR_VONEntry : SCR_SelectionMenuEntry` (`Game/VON/SCR_VONEntry.c:3`),
  `SCR_VONEntryRadio : SCR_VONEntry` (`SCR_VONEntryRadio.c:3`), `SCR_VONEntryComponent`.
- `SCR_VONController.c:230–242`, `AddEntry(SCR_VONEntry entry)` — **публичный** метод: добавляет запись, вызывает
  `entry.InitEntry()`, запоминает long-range радио, шлёт `m_OnEntriesChanged`, делает запись активной, если активной нет.
- Ванильный код вызывает его извне: `SCR_GadgetManagerComponent.c:597,971`, `SCR_EditorManagerEntity.c:182,192`.
  Меню строится в `SCR_VONMenu.c:86–88` (`category.AddEntry` / `m_RadialMenu.AddEntry`).
- Вывод (Q19): публичный `AddEntry`, вызываемый ванильным кодом извне, — **подтверждённая точка расширения-кандидат**
  без modded-замены. Своя запись (наследник `SCR_VONEntry`) ещё **NOT_TESTED** (нужны compile fixture и runtime).
  Ограничение по исходнику: `SetActiveTransmit` знает только `SQUAD_RADIO`, всё остальное передаётся как `DIRECT`, —
  запись RICON для радио должна возвращать `SQUAD_RADIO` и реальный `BaseTransceiver`.

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
  (`ConflictRelayRadio.et:11–12`), `BaseRadioComponent` (Ural, :11). В трёх просмотренных префабах (вышка,
  `ConflictRelayRadio`, `Ural4320_command`) частот вход/выход, мощности и ключа у `RelayTransceiver` не видно; остальные
  префабы не читались.
- Скрипты используют его только в `SCR_CampaignMilitaryBaseComponent.c:1136–1152, 2410–2414`: перебор трансиверов и
  `GetRange()` для дальности базы (Conflict).
- Вывод (Q1, gate M1): в проверенных скриптах и префабах логики пересылки голоса **не видно**. Делает ли движок
  ретрансляцию VoN через `RelayTransceiver` — **НЕ УСТАНОВЛЕНО** (`BLOCKED_NO_RUNTIME`, Э7r).

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

Во всех пунктах поведение в игре — НЕ УСТАНОВЛЕНО. «Частично» — получены фрагменты, но не все поля заказа.

| Пункт | Статус | Чего не хватает |
|---|---|---|
| 3 `ECommMethod` | `SOURCE_EVIDENCE` | — |
| 4 `OnReceive` / выбор трансивера / задержка | `SOURCE_EVIDENCE` (по скриптам) | native-использование `quality` — вне скриптов |
| 7 ввод VoN | `SOURCE_EVIDENCE` (имена действий и контексты) | привязки клавиш (не требовались) |
| 9 материалы | `SOURCE_EVIDENCE` (цепочка API + 6 примеров) | единица `Density` не подписана |
| 1 `RelayTransceiver` | `PARTIAL_SOURCE` | полный перечень префабов (упёрся в лимит 15) и атрибутов, включая значения класса по умолчанию |
| 5 покрытие | `PARTIAL_SOURCE` | построение графа (заполнение `m_aRadiosInRange`) не читалось; только прямой текст в 3 файлах |
| 6 рации | `PARTIAL_SOURCE` | мощность; второй трансивер R-148; трансиверы ANPRC-77; рации техники и стационара |
| 8 radial-меню | `PARTIAL_SOURCE` | `SCR_VONMenu`/`SCR_VONRadialDisplay` и layout; своя запись — нужна compile fixture, runtime NOT_TESTED |
| 2 звуковой граф | `OPEN_UNREADABLE` | `.acp`/`.sig` не открываются через `FileIO`; передано Audio Editor ассистенту владельца (agora #791); `Resource.Load`/`BaseContainer` без нового решения не используется |
| 10 компиляция | **закрыт** (№003/004) | — |

## Э3 final `8ba6d3e` — компиляция и исполнение тестов (2026-10-19)

**Разрешения владельца.**
- Компиляция в Workbench — как для №003/004.
- Исполнение тестов: «разрешаю запускать наши тесты в Workbench через плагин, без Play и сервера» (чат claude,
  2026-10-19).
- Play, мир и сервер не запускались. Плагин не читает и не пишет файлы и не трогает ресурсы игры.

**Target.** Код `8ba6d3e` (codex: `284dde5`, `f8b137d`, `ad24d51`, `9e6ce55`, `8ba6d3e`).
- Фикстура, генератор и раннер — `dfd15d1`, `8d3e450`, `36609bf`.
- Эталон — `1413291`.
- Дерево `addons/`, `tools/`, `oracle/` при прогоне совпадало с `8ba6d3e` (`git diff --quiet 8ba6d3e`).
- Workbench: engine 192142, game 1.8.0.13 (built 2026-08-21).

**Команды** (полные пути — в строке `CLI Params` каждого лога):
- компиляция: `ArmaReforgerWorkbenchSteamDiag.exe -gproj addons/<RICON_Core|RICON_CoreTests>/addon.gproj -addonsDir
  "<игра>\addons,<Tools>\Workbench\addons,<repo>\addons" -wbModule=ScriptEditor -validate -wbSilent -exitAfterInit
  -noThrow -logLevel verbose -logsDir <имя> -log-scr-checksum`;
- тесты: `tools/RICON_TestRunner/run.ps1 -Name r8ba6d3e`. Это то же с `-gproj tools/RICON_TestRunner/addon.gproj
  -noBackend -plugin=RICON_TestRunnerPlugin`, без `-validate`. PASS только при exit 0 и строке
  `[RICON_TestRunner] done: PASS`.

**Логи** (`Documents\My Games\ArmaReforgerWorkbench\logs\<папка>\console.log`, вне репозитория; SHA-256 сверил и codex,
agora #1073):

| Папка | Что | Итог | SHA-256 `console.log` |
|---|---|---|---|
| `ricon-validate-core-8ba6d3e-20261019-192847` | компиляция `RICON_Core` | exit 0, «Script validation successful», 0 script (E), 0 (W) по RICON, CRC `RICON_Lifecycle.c` `6975bed0` | `5486a6469ed0f357a86c1213e9f1fd3db94c4d149e654b27c8842dc8b26a16db` |
| `ricon-validate-coretests-8ba6d3e-20261019-192903` | компиляция `RICON_CoreTests` | exit 0, так же | `bac4c2a3c0f1c08c900e0680338770b05b8965e90c14cbda3a0d92940568bc9a` |
| `ricon-tests-r8ba6d3e-20261019-192920` | исполнение тестов | exit 0, строки ниже | `c2086549a33981304ceed730cbac7bb75b75edc7608c71435000fefc0d9983e6` |

Строки раннера (дословно из лога):
```
[RICON_TestRunner] start
[RICON_TEST] lifecycle parity: traces 14/14, steps 208/208, negative controls caught 3/3, adapter-mapped steps 3
[RICON_TEST] LIFECYCLE_ENFORCE_PARITY_PASS
[RICON_TestRunner] suite lifecycle: PASS
[RICON_TEST] extension fixtures: 1
[RICON_TestRunner] suite extension: PASS
[RICON_TestRunner] done: PASS
```

**Файлы прогона (SHA-256):**

| Файл | SHA-256 |
|---|---|
| `addons/RICON_Core/Scripts/Game/RICON/Core/RICON_Lifecycle.c` | `7deef3bc8bf8687f4d5685cb4c228c206d865204971c669e46542fb7b3de4917` |
| `addons/RICON_Core/Scripts/Game/RICON/Network/RICON_NetworkContracts.c` | `bf7316901813e7d5c629f7359bdb5a38f00652db3f4c834516ad6dadb1ccda60` |
| `oracle/vectors/lifecycle_v1.json` (source `REQ-lifecycle.md@9e6ce55`) | `7038c66c0941c3989b0427e121829e31cd2bb233f815381b4a47b5d57d232369` |
| `addons/RICON_CoreTests/.../RICON_LifecycleFixtures.c` (сгенерирован) | `e653089c38bf4c741077c08d6856515b1c9f09d2c1c94d804dd3c6f5839845ef` |
| `addons/RICON_CoreTests/.../RICON_LifecycleFixtureSupport.c` | `f2278970ddf316cf854af7e94785b0a74ad7359865190b1a3f07e7eaa25eefa2` |
| `tools/RICON_TestRunner/Scripts/WorkbenchGame/RICON_TestRunnerPlugin.c` | `1460c2378b0e209357ace0a1f13eb379e67d73d9181cd8abda5af15994b4be1f` |

**Что сверяется.**
- На каждом шаге: код ответа, поля кэшированного ответа и lease, число и порядок effects. У каждого effect
  сравниваются поля, которые REQ-lifecycle §1.1 задаёт для его типа.
- `ADAPTER_MAPPED` = 3 шага, где типизированный API не выражает отсутствие поля: T02#2 (config без ключа → `null`),
  T02#6 и T11#12 (нет `nowMs` → `−1`). Ожидание в эталоне — `MALFORMED`; так же поле обязан отображать будущий
  адаптер.
- Негативные контроли — копии T01 и T04 с испорченным ожиданием: изменён код, переставлены effects, удалён
  `VON_EXPIRY_INTENDED`. Все три обязаны провалиться, и все три провалились, поэтому сравнение не пустое.
- Фикстура совпадает с выводом генератора на текущих векторах (`test_gen_lifecycle_fixture.py`). Генератор пропускает
  в код только int32 и имена из закрытых наборов.

**Граница доказательства.**
- `ENFORCE_PARITY`: Enforce-код даёт те же результаты, что host-эталон, на тех же 14 трассах.
- Это **не** runtime. Не проверены порядок callbacks GameMode, отправитель RPC, репликация и JIP, VoN (actuator
  отсутствует), переполнение `GetTickCount` (`BLOCKED_NO_RUNTIME`).
- EXT-1…5 `SelfCheck()` — `SELF_CHECK_PASS` при том же прогоне.
- Эталоны расчёта связи, снимков и маршрутов в Enforce не исполнялись (`NOT_TESTED`, отложено agora #1064).

## Э4T-s final `fec7a30` — исполнение фикстуры VoN seam (2026-10-19)

**Разрешение и границы** — как в разделе «Э3 final»: только тесты в Workbench через плагин, без Play, мира и
сервера. Плагин не читает и не пишет файлы.

**Target.**
- Код codex: seam `fd23133`, `2bf70a7`, статус-комментарии `fec7a30`; `addons/RICON_Core` при прогоне == `fec7a30`.
- Фикстура, генератор и раннер (claude): `fc394b2`, `c4427e5`, `9804782`; дерево `addons/`, `tools/`, `oracle/` == HEAD
  `fc394b2` в момент прогона. Последующие `c4427e5` и `9804782` — только host-тесты и генератор; фикстура байт в байт
  та же (`cmp`).
- Эталон: `8816a37`, `46eacc3`; векторы `von_seam_v1.json`, source `REQ-von-seam.md@949f7ca`.
- Workbench: engine 192142, game 1.8.0.13.

**Команды** — те же формы, что в разделе «Э3 final»; раннер — `tools/RICON_TestRunner/run.ps1 -Name rfec7a30`.

**Важно про exit code.** `-validate` возвращает **exit 0 даже при ошибке компиляции**: лог
`ricon-validate-coretests-seam1-20261019-201939` — «Formula too complex» в фикстуре и «Can't compile "Game" script
module!», а exit = 0. Поэтому `WORKBENCH_COMPILE_PASS` засчитывается только при трёх условиях вместе: exit 0, строка
«Script validation successful» и **0 строк `SCRIPT (E)`** в логе. Раннер надёжнее: при несобранном скрипте плагин
не печатает `done: PASS`, и `run.ps1` возвращает FAIL.

**Логи** (`Documents\My Games\ArmaReforgerWorkbench\logs\<папка>\console.log`; SHA-256 сверил и codex, agora #1235):

| Папка | Что | Итог | SHA-256 `console.log` |
|---|---|---|---|
| `ricon-validate-core-fec7a30-20261019-202328` | компиляция `RICON_Core` | exit 0, success, 0 `SCRIPT (E)`, 0 (W) по RICON; CRC `RICON_Lifecycle.c` `3b1fb2ad`, `RICON_NetworkContracts.c` `82049322` | `40b3ac7a3bd4d4801c5ee11f7d9a65428a81a0557ed8fc54f98db6e9c8582881` |
| `ricon-validate-coretests-fec7a30-20261019-202349` | компиляция `RICON_CoreTests` | так же | `961f5b5d71a3dcce96c57ecdf44276b0ae086d30231b01f8b3da1886ec8e8280` |
| `ricon-tests-rfec7a30-20261019-202412` | исполнение тестов | exit 0, строки ниже | `cd434fcc9d832d139c5d9368f047daad29a526a97f69668699ae0ebfab63af38` |

Строки раннера (дословно):
```
[RICON_TEST] lifecycle parity: traces 14/14, steps 209/209, negative controls caught 3/3, adapter-mapped steps 3
[RICON_TEST] LIFECYCLE_ENFORCE_PARITY_PASS
[RICON_TestRunner] suite lifecycle: PASS
[RICON_TEST] extension fixtures: 1
[RICON_TestRunner] suite extension: PASS
[RICON_TEST] von seam parity: cases 11/11, calls 103/103, negative controls caught 3/3, adapter-mapped calls 1
[RICON_TEST] VON_SEAM_ENFORCE_PARITY_PASS
[RICON_TestRunner] suite von_seam: PASS
[RICON_TestRunner] done: PASS
```

**Файлы прогона (SHA-256):**

| Файл | SHA-256 |
|---|---|
| `addons/RICON_Core/Scripts/Game/RICON/Network/RICON_NetworkContracts.c` | `69f9fdaf905f962b65f35dffa599cb3dc90b614f9320306339c8c17cee4475d1` |
| `addons/RICON_Core/Scripts/Game/RICON/Core/RICON_Lifecycle.c` | `3f159470916f75d92118774ede66980133696d1d0cfc8f74d299129b315b9f4a` |
| `oracle/vectors/von_seam_v1.json` | `f78ad022bba25cc693a516c91954c33f586d01a241338bfc24893337966c101b` |
| `oracle/vectors/lifecycle_v1.json` (rev 2) | `1ca07405d752e56f1bbfc20b799913ab4f754f4b54a7d2903c1fdfc53e419268` |
| `addons/RICON_CoreTests/.../RICON_VoNSeamFixtures.c` (сгенерирован) | `b8e18ff16d744ee4f9ce798aa494c06379e8b2beb8cbab1a20c6643c48639a75` |
| `addons/RICON_CoreTests/.../RICON_VoNSeamFixtureSupport.c` | `f3b7be3cdddd710e6a1e2ed146947a360bd445588ace6c010b418103c56e7249` |
| `addons/RICON_CoreTests/.../RICON_LifecycleFixtures.c` (сгенерирован) | `4d7d614f9d6efe22bbbfd26ceabb99029dc1d0f9c9d1600bacefec282cbb21db` |
| `addons/RICON_CoreTests/.../RICON_LifecycleFixtureSupport.c` | `34ba90f909a444376bf7b20a3345779eec55b6b2ed765c76a730e7d072e01eb1` |
| `tools/RICON_TestRunner/Scripts/WorkbenchGame/RICON_TestRunnerPlugin.c` | `b6703878b6c4b69445db915964c8e760c89582b87c59319e85945c6326f2d85c` |

**Что сверяется.**
- На каждом из 103 вызовов `ApplySnapshot` или `OnTransmitLeaseExpired`: решение равно эталону, а snapshot и binding
  после вызова равны копиям, снятым до него (§7). У NaN равенство NaN-aware.
- NaN и ±Inf строятся в рантайме (`Math.Sqrt(-1)`, `float.MAX * 2`); `RequireNonFinite` проверяет их до
  использования, иначе случай проваливается.
- `ADAPTER_MAPPED` = 1: имя состояния вне LOST/WEAK/DEGRADED/CLEAN передаётся как значение enum −1. Неизвестные
  upstream 99 и −1 передаются как int в enum.
- Негативные контроли: изменённое решение apply, принятый mismatch expiry, принятый NaN. Все три провалились.
- Первый прогон (`ricon-tests-seam1-20261019-202101`) показал, что объект, переданный прямо в аргументе без
  переменной, Enforce освобождает во время вызова: сначала «binding changed by the call», затем `NULL_INPUT` во
  всех случаях. Теперь фикстура держит snapshot и binding в локальных переменных. Сравнение провал поймало, а не
  пропустило.
- CT-VON-S-010 — `tools/RICON_TestRunner/test_von_seam_scan.py`: статическая проверка текущего исходника, **не
  security proof**. В коде `RICON_NetworkContracts.c` (без комментариев, лексер учитывает строки) нет VoN/radio,
  Rpl, world/entity и file API; подложенная зависимость и маркеры комментариев внутри строк ловятся.

**Граница доказательства.** `ENFORCE_PARITY` seam: Enforce-код принимает те же решения, что host-эталон, на 103
синтетических вызовах. Любой валидный вход — `BLOCKED_NO_ACTUATOR`: голос не доставляется и не глушится.
Не проверены реальный трансивер и его число индексов, authority и Rpl, доставка, звук (`BLOCKED_NO_RUNTIME`, Э4T-f).
