# Радио в ванильном Arma Reforger — игровой скриптовый слой (SCR_*)

Статус: DRAFT — отчёт субагента claude (owner agora #2960), ревью codex ожидается. Выборочная сверка claude с сырыми логами и HTML пройдена.

Автор: claude.radio_gameplay. Источник — только офлайн-документация Script API из Arma Reforger Tools
(Doxygen 1.16.1, файлы датированы 2026-10-18). Ссылки вида `файл:строка` даны относительно
`AR/` = `C:\Program Files (x86)\Steam\steamapps\common\Arma Reforger Tools\Workbench\docs\ArmaReforgerScriptAPIPublic\html\`.
Номер строки — строка сырого HTML. **Уровень всех находок — DOC**: документация объявляет член
или его краткое описание. Runtime-поведение это не доказывает. Пометка **INF** означает мой вывод из
сочетания DOC-фактов. Тел функций в документации нет: исходники не публикуются, есть только путь
`.c`-файла внизу страницы.

Вне моей зоны (codex): геттеры и сеттеры BaseRadioComponent/BaseTransceiver, путь голоса
(SCR_VONController/SCR_VoNComponent), RadioManagerEntity/RelayTransceiver. Ниже они упоминаются
только как точки стыка.

---

## Q1. Назначение частот (фракция / взвод / группа / задача / командир)

**Единицы**
- `interfaceSCR__PlayerControllerGroupComponent.html:183`: у `RequestSetCustomFrequency` в описании стоит «(KHz)». DOC.
- `interfaceSCR__Tutorial__Seizing__SET__FREQUENCY.html:121`: `DESIRED_FREQUENCY = 60000`; там же `:149` — префаб `Radio_ANPRC68.et`. DOC.
- В UI подпись единиц — `"#AR-VON_FrequencyUnits_MHz"`: `interfaceSCR__VONEntryRadio.html:233`, `interfaceSCR__VonDisplay.html:176`. DOC.
- У гаджета отдельные сигналы ручек `m_iSignalChannelMhz` и `m_iSignalChannelKhz` (`interfaceSCR__RadioComponent.html:211-212`), плюс `EFreqUnit` в `GetKnobAngle/GetKnobAnimStepCount` (`:179,187`). DOC.
- **INF:** частота хранится как `int` в кГц (60000 = 60,000 МГц), а в UI показывается в МГц. Шаг сетки и диапазон по моделям радио в docs отсутствуют → UNKNOWN.

**Фракция (= частота взвода)**
- `SCR_Faction.GetFactionRadioFrequency()` и protected `m_iFactionRadioFrequency`: `interfaceSCR__Faction.html:182,265`. Описания нет. Метод скриптовый, атрибут фракции. DOC.
- То же наследуют `SCR_CampaignFaction` (`interfaceSCR__CampaignFaction.html:201,286`) и `SCR_FreeForAllFaction` (`interfaceSCR__FreeForAllFaction.html:182,262`).
- `SCR_VONRadialDisplay.SetFrequencyDisplay(EVONMenuTable line, SCR_AIGroup group, SCR_Faction milFaction=null)`: `interfaceSCR__VONRadialDisplay.html:183`. Параметр описан так: если передана фракция, вместо частоты группы выставляется частота взвода (`:556`). DOC. Ещё есть метка `LABEL_CHANNEL_PLATOON` (`interfaceSCR__VonDisplay.html:178`).
- **INF:** «частота фракции» в UI и автотюне играет роль **частоты взвода**. Отдельной «HQ-частоты» среди SCR-членов фракции нет.

**Группа / отделение**
- `SCR_AIGroup` (`interfaceSCR__AIGroup.html`):
  - частота: `SetRadioFrequency(int)` `:250`, `RPC_DoSetFrequency(int)` `:251` (RPC-рассылка), `GetRadioFrequency()` `:252`, protected `m_iGroupRadioFrequency` `:514`, static `GetOnFrequencyChanged()` `:437` / `s_OnFrequencyChanged` `:560`;
  - `Get/SetDefaultActiveRadioChannel` `:153-154` и `m_iDefaultActiveRadioChannel` `:528` — индекс канала (трансивера), активного по умолчанию;
  - `Increase/Decrease/GetDeployedRadioCount` `:362-364` — счётчик развёрнутых радио группы.
- Распределитель `SCR_GroupsManagerComponent` (`interfaceSCR__GroupsManagerComponent.html`):
  - публичные `GetFreeFrequency(Faction)` `:175`, `ClaimFrequency/IsFrequencyClaimed/ReleaseFrequency(int, Faction)` `:176-178`, `TunePlayersFrequency(playerId, player)` `:193`;
  - protected `AssignGroupFrequency(SCR_AIGroup)` `:251`;
  - конфиг: `m_iPlayableGroupFrequencyOffset/Min/Max` `:261-263`, флаги `m_bTuneAgentsRadioToGroupFrequency`, `m_bTunePlayersRadioToGroupFrequency` `:264-265`;
  - учёт: `map<Faction,int> m_mPlayableGroupFrequencies` `:279`, `map<int, FactionHolder> m_mUsedFrequenciesMap` `:280`. DOC.
  - **INF:** частоты групп выдаются из пула [Min..Max] с шагом Offset отдельно для каждой фракции. Пул захватывается и освобождается; игроков и ИИ можно автоматически настраивать на частоту группы. Сами значения лежат в префабе менеджера → UNKNOWN.
- Ручная частота игрока: `SCR_PlayerControllerGroupComponent.RequestSetCustomFrequency(groupID, frequency)` → `RPC_AskSetFrequency` (`:182,186`), protected `GetActualGroupFrequency()` (`:209`). Пересказ описания (`:1540`): можно поставить уже занятую частоту, незанятую метод сам захватывает, а авто-группы её потом не берут. DOC.
- Пресеты: `SCR_GroupPreset.m_iRadioFrequency`, `m_iDefaultActiveRadioChannel`, Get/Set (`interfaceSCR__GroupPreset.html:129-130,139,153-154`); то же в `SCR_GroupRolePresetConfig` (`interfaceSCR__GroupRolePresetConfig.html:137-147,166-167`). DOC.

**Задачи / цели (objective)**
- `SCR_GroupTaskManagerComponent` (`interfaceSCR__GroupTaskManagerComponent.html`):
  - публичные: `IsEnabledAssigningFrequencies` `:118`, `IsAnyFrequencyFreeForFaction` `:120`, `CanAssignFrequencyToTask`, `GetTaskFrequency`, `GetTaskByFrequency(faction, freq)` `:124-126`;
  - RPC: `RpcDo_Add/RemoveTaskFrequency` `:150-151`;
  - protected: `AssignFrequency/ClaimFrequency/GetFreeFrequency/ReleaseFrequency` `:155-158`;
  - конфиг: `m_bEnabledAssigningFrequencies`, `m_iTaskFrequencyOffset/Min/Max`, `m_aAllowedTasksWithFrequencies` `:168-172`. DOC.
- **INF:** у задач собственный пул частот на фракцию.

**Командир**
- `SCR_VONAutoTune.GetCommanderFrequency(Faction)` (`interfaceSCR__VONAutoTune.html:128`). Откуда берётся значение, docs не говорят → UNKNOWN.

**Автонастройка (autotune)**
- `SCR_VONAutoTune` — член `SCR_VONController.m_VONAutoTune` (`interfaceSCR__VONController.html:217`).
  - методы: `SetRadios(entity, SCR_EVONAutoTuneFrequency)` `:114`, `SetChannels(SCR_Faction, BaseRadioComponent, SCR_VONAutoTuneRoleConfig, groupId, …)` `:124`, `GetFrequency(faction, groupId, type)` `:125`, `GetPlatoonFrequency` `:126`, `GetObjectiveOrPlatoonFrequency(faction, groupId)` `:127`, `FindFirstRadio(vonController, ERadioType, encryptionKey)` `:129-130`;
  - `UPDATE_DELAY = 0.5` `:151`. DOC.
- Конфиг — дерево:
  - `SCR_VONAutoTuneConfig.m_aAutoTuneList` (`interfaceSCR__VONAutoTuneConfig.html:115`);
  - → `IdentityConfig {ERadioType m_eRadioType; string m_sFactionKey; m_aRoles}` (`interfaceSCR__VONAutoTuneIdentityConfig.html:112-114`);
  - → `RoleConfig {SCR_EVONAutoTuneRole m_eRole; m_aChannels}` (`interfaceSCR__VONAutoTuneRoleConfig.html:112-113`);
  - → `ChannelConfig {int m_iChannel; SCR_EVONAutoTuneFrequency m_eAutoTuneFrequency}` (`interfaceSCR__VONAutoTuneChannelConfig.html:112-113`). DOC.
- Включатель в Conflict: `SCR_GameModeCampaign.GetAutotuneEnabled()` `:295` / `m_bAutotuneEnabled` с пометкой «is the autotune radio feature enabled» `:485-486`. DOC.
- **INF:** схема «тип радио × фракция × роль → номер трансивера → тип частоты (группа / взвод / цель / командир)».
- Значения перечислений `SCR_EVONAutoTuneFrequency`, `SCR_EVONAutoTuneRole`, `ERadioType`, `EFreqUnit` в docs отсутствуют (страниц enum нет; поиск находит только поля) → UNKNOWN.

**Прочее**
- `SCR_CampaignFaction.USE_GROUP_FREQUENCY = true` (static const) — `interfaceSCR__CampaignFaction.html:310,894`. Где используется — UNKNOWN.
- ScenarioFramework: `SCR_ScenarioFrameworkAIActionSetRadioFrequency` («Action to set a radio frequency», `:110`).
  - поля `int m_iFrequency` и `string m_sEncryptionKey` `:149-150`;
  - `GetCommunicationDevice(entity, encryptionKey)` возвращает гаджет `EGadgetType.RADIO` `:144-145`. DOC.
- Имя канала по частоте: static `SCR_VONMenu.GetKnownChannel(int frequency)` — «channel name or empty string if not found» (`interfaceSCR__VONMenu.html:124-125,366`). DOC.
- Отображение частот в UI:
  - `SCR_GroupButton.UpdateGroupFrequency`, `SGetGroupName(group, widget, freqWidget)` (`interfaceSCR__GroupButton.html:137,202`);
  - `SCR_GroupRequestUIComponent.UpdateGroupFrequency` (`:176`);
  - `SCR_PlayerListEntry.m_iSortFrequency/m_wFreq` (`:121,126`);
  - `SCR_MapMarkerSquadLeaderComponent.m_wGroupFrequency` (`:162`);
  - сортировка `SCR_CompareGroupRadioFreq.Compare(SCR_AIGroup, SCR_AIGroup)` (`:121`);
  - тултип редактора `SCR_FrequencyTooltipDetail` (`m_sNoFrequency`, `m_GadgetManager` `:130-132`). DOC.

**UNKNOWN (Q1):** числовые значения частот фракций и пулов, шаг тюнинга, диапазоны по моделям, происхождение командирской частоты, есть ли отдельная частота «рота/HQ».

---

## Q2. Ключи шифрования

- **Фракционный ключ:** `Faction.GetFactionRadioEncryptionKey()` — **proto external**, «Returns the encryption key used for radio transmissions» (`interfaceFaction.html:137-138,222`).
  - `SCR_Faction` наследует его от `Faction` (`interfaceSCR__Faction.html:212` заголовок раздела, `:226`). DOC.
  - Скриптового сеттера ключа фракции нет. **INF:** ключ задаётся в конфиге фракции и читается нативно. Имя атрибута → UNKNOWN.
- **Ключ радио:** `BaseRadioComponent.SetEncryptionKey/GetEncryptionKey` (proto external; видно в `interfaceSCR__CoverageRadioComponent.html:164-165`).
  - Событие `OnEncryptionKeyChanged(string)` «on client and server» `:185-186`, invoker `m_OnEncryptionKeyChanged` `:210`. DOC.
- **Кто в SCR-слое ставит или читает ключ:**
  - `SCR_VONController.InitEncryptionKey()` (protected) и `m_sLocalEncryptionKey` (`interfaceSCR__VONController.html:201,227`). Описаний нет; это зона codex. DOC.
  - `SCR_AntennaServicePointComponent.ChangeRadioSettings(SCR_Faction)` + `OnFactionChanged(...)` + `BaseRadioComponent m_RadioControl` (`interfaceSCR__AntennaServicePointComponent.html:122-123,156`). DOC.
  - `SCR_CampaignMilitaryBaseComponent.ChangeRadioSettings(SCR_Faction)` (`interfaceSCR__CampaignMilitaryBaseComponent.html:195`). DOC.
  - **INF:** при смене владельца антенна или база перенастраивает своё радио под новую фракцию. Что именно меняется (ключ, частота или оба) → UNKNOWN.
  - `SCR_VONAutoTune.FindFirstRadio(vonController, radioType, encryptionKey)` — «Find first equipped radio by type and encryptionKey» (`interfaceSCR__VONAutoTune.html:129-130`). **INF:** автотюн трогает только радио с ключом своей фракции.
  - ScenarioFramework `m_sEncryptionKey` (`interfaceSCR__ScenarioFrameworkAIActionSetRadioFrequency.html:150`).
- **Ключ сообщения:** `BaseRadioMessage.Set/GetEncryptionKey` (proto external), используется в `SCR_CampaignRadioMsg` и `SCR_TaskRadioMsg` (`interfaceSCR__CampaignRadioMsg.html:135-138`, `interfaceSCR__TaskRadioMsg.html:135-138`). DOC.
- **Граф покрытия по ключу:**
  - `SCR_RadioCoverageSystem.m_mEncryptionSignalSources` (map ключ→источник) `:155`, `FindNewSource(key)` `:144`, `UpdateCoverage(key)` `:146`;
  - `SCR_CoverageRadioComponent.GetCoverageByEncryption(key)` `:141`, `GetSavedEncryption` `:140`, `OnEncryptionChanged` `:148`, `m_aEncryptionKeyCoverSend/Receive` `:201-202`. DOC.
- Ключей на уровне группы или задачи в SCR-членах **нет** (поиск по `encrypt` во всех `interfaceSCR__*.html`).

**UNKNOWN (Q2):** формат и значения ключей; проверяется ли ключ движком при приёме голоса (это нативный путь, codex).

---

## Q3. Радио как предмет/гаджет; действия и UI

**Гаджет**
- `SCR_RadioComponent : SCR_GadgetComponent` (исходник `Game/Components/Gadgets/SCR_RadioComponent.c`, `interfaceSCR__RadioComponent.html:945`).
  - `RadioToggle()` — «Radio on/off toggle» `:122-123`;
  - `ChangeFrequencyStep(bool direction)` — шаг частоты плюс процедурная анимация `:124-125`, `true` = вверх `:304`;
  - `GetRadioType()` → `ERadioType` `:136`, `GetRadioComponent()` → `BaseRadioComponent` `:139`;
  - атрибуты `m_iRadioCategory`, `m_iRadioType`, `m_bIsPowered=true` `:204-206`;
  - анимация ручек: `GetKnobAngle/GetKnobAnimStepCount(EFreqUnit)`, сигналы `m_iSignalPower/ChannelKhz/ChannelMhz` `:179-189,210-212`. DOC.
- `SCR_GadgetComponent`: `ToggleActive(state, context)` — «Synchronise gadget state»; `OnToggleActive` вызывается из `SCR_GadgetManagerComponent` через RPC (`interfaceSCR__RadioComponent.html:147-148,155-156`). DOC.
- `SCR_GadgetManagerComponent`:
  - `m_iRadiosFlags = EGadgetType.RADIO | EGadgetType.RADIO_BACKPACK` (`interfaceSCR__GadgetManagerComponent.html:179`);
  - `RegisterVONEntries()` — «Register radio gadgets into the VON system as entries» (`:205-206`). DOC.
  - **INF:** в игре два класса радио — носимое и рюкзачное (long range), см. `SCR_VONEntryRadio.IsLongRange()`.

**Действия**
- `SCR_RadioToggleUserAction` (питание) и `SCR_RadioTuningUserAction` (`bool m_bTuneUp` `:209`):
  - оба наследуют `SCR_InventoryAction`, переопределяют `HasLocalEffectOnlyScript` (`:127`); исходники `Game/UserActions/Inspection/...` (`:388`, `:400`). DOC.
  - **INF:** это действия осмотра предмета в инвентаре.
- `SCR_RadioBaseUserAction`:
  - public `int m_iTargetFrequency` (`:212`), `BaseRadioComponent m_BaseRadioComponent`, `array<BaseTransceiver> m_aTsvList` (`:239-240`);
  - поддерживает зацикленное удержание (`IsActionLooping/GetLoopActionHoldDuration`);
  - наследник — `SCR_ScenarioFrameworkRadioBaseUserAction`. DOC.
  - **INF:** действие на мировом объекте (стационарное радио), которое выставляет заданную частоту.

**Радиальное меню VON**
- `SCR_VONMenu` (`interfaceSCR__VONMenu.html`):
  - `ActionTuneFrequency` `:154`, `ActionCycleChannel` `:156`, `OnMenuToggle` (вкл/выкл) `:158`, `OnMenuMuteToggle` `:160`;
  - `ADJUST_COOLDOWN = 0.175` `:129`;
  - `GetGroupedRadioIndex/GetGroupedEntries` — соседние трансиверы одного устройства `:133-136`. DOC.
- `SCR_VONEntryRadio` (`interfaceSCR__VONEntryRadio.html`):
  - `GetTransceiver()`, `GetTransceiverNumber()` (нумерация с 1) `:128-131`;
  - `GetEntryFrequency()`: локальный геттер, потому что геттер трансивера запаздывает (`:132-133`);
  - `IsLongRange()` `:137-138`, `AdjustEntry/AdjustEntryModif` (частота) `:144-147`, `ToggleEntry` (вкл/выкл) `:148-149`, `ToggleMuteEntry` `:150`, `SetChannelText/...Overwrite` `:140-142`. DOC.
- Выбор активного трансивера: `SCR_VONEntry.SetActive/IsActive` (`interfaceSCR__VONEntry.html:133-136`) и `SCR_VONMenu.OnActiveEntriesChanged` — событие `SCR_VONController` (`:151-152`). Логика выбора — в `SCR_VONController` (codex). DOC.
- `SCR_VONRadialDisplay`: `UpdateFrequencyList(entry)` `:129`, `SetFrequenciesVisible` `:136`, `SetFrequencyDisplay(...)` `:183`. DOC.

**HUD**
- `SCR_VonDisplay.OnCapture(BaseTransceiver)` и `OnReceive(playerId, isSenderEditor, receiver, frequency, quality)` (`interfaceSCR__VonDisplay.html:131,133`). DOC.
- `TransmissionData.m_fFrequency/m_fQuality` (`interfaceTransmissionData.html:130-131`).
- Виджеты `SCR_VoNOverlay_ElementWidgets`: `m_wFrequency`, `m_wChannelText`, `m_wNosignal` (`:123,132-133`). DOC.

**Условия**
- `SCR_CharacterHasLongRadioCondition`, `SCR_CharacterRadioCountCondition`, `SCR_IsCharacterUsingRadioCondition` (`:110` в каждом). Описание HasLongRadio дословно совпадает с «using radio» — похоже на копипасту в docs.

**Чат по радио**
- `SCR_ChatMessageRadioProtocol(string message, int frequency)` (`:121`) и `SCR_ChatPanelManager.OnNewMessageRadioProtocol(msg, frequency)` (`:146`). DOC.

**UNKNOWN (Q3):** какие префабы радио есть и сколько у каждого трансиверов, шаг `ChangeFrequencyStep`, раскладка клавиш.

---

## Q4. Дальность, качество сигнала, глушение, антенны/релеи в SCR

**Глушение** — поиск `jam|interferenc` по всем `interface*.html` пуст. Это наблюдение по сигнатурам и описаниям
в HTML; тела скриптов и native-код не проверены, поэтому отсутствие алгоритма помех не доказано. DOC (поиск).

**Качество** в проверенных API приходит входным `float quality`; вычисления quality в сигнатурах и описаниях
HTML не найдено (тела скриптов не читались):
- `SCR_VonDisplay.OnReceive(...)` `:133`;
- `SCR_NameTagData.OnReceivedVON(playerId, receiver, frequency, quality)` `:212`;
- `SCR_CampaignRadioMsg.OnDelivery(BaseTransceiver receiver, int freq, float quality)` `:132` и `SCR_TaskRadioMsg.OnDelivery` `:132`;
- `SCR_CampaignNetworkComponent.RpcDo_PlayRadioMsg(..., float seed, float quality)` `:169`. DOC.
- **INF:** качество считает движок (codex: путь приёма), а скрипт его только отображает или озвучивает. Как именно UI использует quality (например, `m_wNosignal`) → UNKNOWN.

**Дальность**
- Событие `BaseRadioComponent.OnTansceiverRangeChanged(transceiver, oldRange, newRange)` (`interfaceSCR__CoverageRadioComponent.html:181-182`).
- Сеттера дальности среди SCR-членов нет. Сеттер, если он есть, движковый (codex).

**Сеть покрытия** (логика режима, не доставка голоса; см. E1s §4)
- `SCR_CoverageRadioComponent : ScriptedRadioComponent` (`:110,126-150,189-202`):
  - роль источника: `Set/IsSource`;
  - `GetRadiosInRange(Of)` и подсчёт с фильтром по ключу;
  - `GetSavedRange`, `OnRangeChanged`, `OnPowerToggle`, `Ping(key, signature, reverse)`;
  - `m_mCoverage` (ключ → `SCR_ERadioCoverageStatus`).
- `SCR_RadioCoverageSystem` (`:125-159`): `RegisterRadioComponent(component, float range=-1)`, `HandleRanges`, `UpdateAll`, `TogglePeriodicUpdates`, `UPDATE_DELAY = 2000`.
- Статусы `SCR_ERadioCoverageStatus` по описанию: нет покрытия / до базы дотягиваются / база может ответить / в обе стороны (`interfaceSCR__CampaignMilitaryBaseManager.html:147`). DOC.

**Карта**
- `SCR_RadioCoverageMapDescriptorComponent`: `SetParentRadio`, `UpdateLinks`, `CreateLinks/ColorMapLink` (`:125-152`) — линии связи на карте.
- `SCR_CampaignMilitaryBaseMapDescriptorComponent.UpdateServices(isInRange)` (`:154-155`).
- `SCR_CampaignMapUIBase.SetAntennaIconVisible` (`:158`). DOC.

**Респаун на радио**
- `SCR_PlayerRadioSpawnPoint` (`OnItemAdded/OnItemRemoved` `:124-125`, `Set/GetSpawnPositionRange` `:190-191`);
- `SCR_PlayerRadioSpawnPointCampaign`, `SCR_RadioVehicleSpawnPoint`. Это радиус точки спавна, а не радиодальность. DOC.

**UNKNOWN (Q4):** формула quality(расстояние, мощность, рельеф), есть ли затухание по местности, физическая роль антенн вне логики режима.

---

## Q5. Игровой режим Conflict / campaign

**База** — `SCR_CampaignMilitaryBaseComponent` (`interfaceSCR__CampaignMilitaryBaseComponent.html`):
- дальность: `GetRadioRange` `:168`, `CanReachByRadio(entity)` `:169`, `GetIsEntityInMyRange` `:208`, `RecalculateRadioRange/OnRadioRangeChanged` `:240-241`, `GetRelayRadioRange(BaseRadioComponent)` `:340`;
- связь с HQ: `GetHQRadioCoverage(faction)` `:170`, `IsHQRadioTrafficPossible(faction, direction=RECEIVE)` `:171`;
- релеи и сигнал: `CaptureRelay(faction, playerId)` — передача релея фракции `:185-186`, `OnHasSignalChanged` `:191`, `ChangeRadioSettings(SCR_Faction)` `:195`;
- HQ: `IsHQ/SetAsHQ` `:164-165`;
- поля: `m_fRadioRangeDefault` `:394`, `m_fRadioRange` `:419`, `SCR_CoverageRadioComponent m_RadioComponent` `:404`;
- константы: `RADIO_RECONFIGURATION_DURATION = 10.0` `:316`, `RELAY_BASE_RADIUS = 100` `:322`. DOC.
- `SCR_CampaignSourceBaseComponent` наследует тот же набор (`:131,207-234,351,357`).

**Менеджер баз** — `SCR_CampaignMilitaryBaseManager`:
- `RecalculateRadioCoverage(faction)` — покрытие всех баз `:146-147`;
- `IsEntityInFactionRadioSignal` / `IsPositionInFactionRadioSignal(pos, faction, signalRangeOffset)`: true, если в точке есть сигнал от HQ `:156-157,955`;
- `GetOnSignalChanged` — смена покрытия базы `:128-129`. DOC.

**MHQ** — мобильный штаб:
- `SCR_CampaignMobileAssemblyStandaloneComponent.SetRadioRange(int)/GetRadioRange/IsInRadioRange/UpdateRadioCoverage`, `m_iRadioRange` `:127-132,160`;
- `SCR_CampaignMobileAssemblyComponent` (`:121,130-131`). DOC.

**Релеи**
- `SCR_CampaignReconfigureRelayUserAction` — действие перенастройки релея (`:110`), поле `m_Base` (`:229`).
- `SCR_CampaignRelayAlternativeSpot` — релеи ставятся в случайные точки карты, это сущность-кандидат (`:110`).
- Задача захвата релея и `OnRadioCoverageChanged(SCR_CoverageRadioComponent)` у `SCR_CampaignFactionCommanderHandlerComponent` (`:186-187,197`). DOC.

**Антенна**
- `SCR_AntennaServicePointComponent` (сервис базы): `ChangeRadioSettings(SCR_Faction)`, `m_RadioControl` (`:123,156`).

**Обратная связь игроку**
- `SCR_CampaignFeedbackComponent`: `SetIsPlayerInRadioRange`, `BaseOutOfRangeHint`, `CheckPlayerInsideRadioRange`, `m_bIsPlayerInRadioRange` (`:130-131,174,231`). DOC.

**Фракция и режим**
- `SCR_CampaignFaction`: `GetRadioPrefab()` `:129`, `m_RadioMsgWhitelistConfig` `:251`, `SendHQMessage(...)` `:124`, `USE_GROUP_FREQUENCY` `:310`.
- `SCR_GameModeCampaign`: `m_bAutotuneEnabled` `:485`, `m_aRadioSpawnPoints` `:426`, `GetHQRadioSoundEntityPrefab` `:135`.
- `SCR_CampaignRadioRegisteringComponent.SetRadioChatterSignal(faction)` (`:124,137`) — звуковой эфир на базе. DOC.

**Сообщения HQ** — скриптовые радиосообщения по частоте:
- `SCR_CampaignRadioMsg` с позывными роты/взвода/отделения (`:122-130`);
- белый список `SCR_CampaignRadioMsgWhitelistConfig.CanSendRadioMessage` (`:112`). DOC.

**UNKNOWN (Q5):** числовые дальности баз, релеев и MHQ (значения в префабах); участвуют ли релеи Conflict в доставке **голоса** или только в логике покрытия (E1s §4: не доказано); связь `m_RadioComponent` базы с `RelayTransceiver` (codex).

---

## Q6. Native (`proto external`) против скриптовых

- Docs явно помечают нативные методы префиксом `proto external` / `proto`. У **всех собственных членов SCR_*-классов** из этого отчёта такой пометки нет: это скриптовые методы Enforce. Внизу каждой страницы указан исходник `Game/...c`, например `interfaceSCR__RadioComponent.html:945`, `interfaceSCR__CampaignMilitaryBaseComponent.html:5161`. DOC.
- Нативные методы встречаются только среди унаследованных:
  - `Faction.GetFactionRadioEncryptionKey` (`interfaceFaction.html:137`);
  - `BaseRadioComponent`: `TransceiversCount`, `GetTransceiver`, `Set/GetEncryptionKey`, `SetPower`, `IsPowered`, `IsEditorRadio`, `SetTransceiverFrequency` («Set frequency of transceiver and sync with server») — `interfaceSCR__CoverageRadioComponent.html:161-171`;
  - `BaseRadioMessage.Set/GetEncryptionKey` (`interfaceSCR__CampaignRadioMsg.html:135-138`). DOC.
- События `BaseRadioComponent.OnTansceiverFrequencyChanged/RangeChanged/OnPowerChanged/OnEncryptionKeyChanged` объявлены без `proto`, с пометкой «invoked on client and server» (`interfaceSCR__CoverageRadioComponent.html:179-186`). **INF:** движок вызывает их как скриптовые колбэки.
- Сетевая доставка в SCR-слое: RPC-методы (`RPC_DoSetFrequency`, `RPC_AskSetFrequency`, `RpcDo_Add/RemoveTaskFrequency`, `RpcDo_PlayRadioMsg`) — скриптовые; атрибуты `[RplRpc]` в docs не видны. **INF** по префиксам имён.

---

## Что остаётся вопросом native/runtime

1. Числа: частоты фракций, пулы групп и задач (`Min/Max/Offset`), шаг тюнинга, диапазоны по моделям радио — всё это в префабах и конфигах, не в docs.
2. Значения перечислений `ERadioType`, `EFreqUnit`, `SCR_EVONAutoTuneFrequency/Role`, `SCR_ERadioCoverageStatus`, `EGadgetType.RADIO*`.
3. Как движок считает `quality` (расстояние, мощность, рельеф) и есть ли какие-либо помехи. Поиск `jam|interferenc`
   в HTML пуст, но тела скриптов и native-код не проверены.
4. Фильтрует ли движок приём голоса по ключу шифрования (фракционный ключ). Скрипт только сопоставляет ключи в автотюне и в графе покрытия.
5. Что меняет `ChangeRadioSettings(SCR_Faction)` на антенне и базе: ключ, частоту или дальность.
6. Влияет ли покрытие Conflict (`SCR_CoverageRadioComponent`, `RELAY_BASE_RADIUS`, `GetRelayRadioRange`) на голосовую связь игроков или только на игровую логику (HQ-трафик, сервисы, спавн, подсказки).
7. Откуда берётся командирская частота (`GetCommanderFrequency`) и как определяется частота цели (`GetObjectiveOrPlatoonFrequency`).
8. Задержка синхронизации частоты (комментарий у `GetEntryFrequency`) — её величина и механизм репликации `SetTransceiverFrequency`.
