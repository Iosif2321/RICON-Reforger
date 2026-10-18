# Сбор информации: офлайн Script API (claude)

Дата: 2026-10-18. Уровень доказательств — **DOC_EVIDENCE**, если не указано иное.

## Источник и метод
- **AR** = `...\Arma Reforger Tools\Workbench\docs\ArmaReforgerScriptAPIPublic\html`.
- **EN** = `...\Arma Reforger Tools\Workbench\docs\EnfusionScriptAPI\html`.
- Индекс всех членов: 74 117 сигнатур (скрипт-индексатор только читает HTML).
- Ссылки даны в виде `API/файл.html`.

## Вопросы реестра (id340)

| Q | Вопрос | Ответ | Доказательство | Статус |
|---|---|---|---|---|
| Q3 | Права `SetMuteState` | В docs только «Toggles the mute state of the transceiver»; кто вправе вызывать — не указано | AR/interfaceBaseTransceiver.html | **ЧАСТИЧНО** → Э1r |
| Q4 | `ECommMethod` | Используется в `VoNComponent.Set/GetCommMethod`, `SCR_VONEntry.GetVONMethod`; значения перечисления в индексе **не найдены** | AR/interfaceVoNComponent.html, AR/interfaceSCR__VONEntry.html | **ЧАСТИЧНО** → SOURCE (Workbench) |
| Q6 | Несколько трансиверов | `BaseRadioComponent.TransceiversCount()`, `GetTransceiver(int idx)` — несколько на радио. На одного персонажа — несколько радио в инвентаре (SCR_VONEntryRadio на каждый трансивер) | AR/interfaceBaseRadioComponent.html, AR/interfaceSCR__VONEntryRadio.html | **ЗАКРЫТ** (DOC); число и лимиты — runtime |
| Q7 | Свой радиокомпонент мода | `ScriptedRadioComponent` : `BaseRadioComponent`; хуки `OnInit/OnPostInit/OnFrame`; ванильный пример — `SCR_CoverageRadioComponent` | AR/interfaceScriptedRadioComponent.html (диаграмма) | **ЗАКРЫТ** (DOC) |
| Q8 | Пространственный поиск трансиверов | `ChimeraWorld.GetRadioManager()` → `RadioManagerEntity.GetTransceiversInRange(vector position, float range, out notnull array<BaseTransceiver>)` — только включённые | AR/interfaceChimeraWorld.html, AR/interfaceRadioManagerEntity.html | **ЗАКРЫТ** (DOC); стоимость — Э1r |
| Q10 | Ключ шифрования | `BaseRadioComponent.SetEncryptionKey`; `BaseRadioMessage`: «Empty string means no encryption»; `Faction.GetFactionRadioEncryptionKey()` — ванильно ключ = фракция | AR/interfaceBaseRadioMessage.html, AR/interfaceFaction.html | **ЗАКРЫТ** (DOC). Что ключ делает с голосом (не слышно / шум) — runtime |
| Q11 | Rpl | `RplRpc`/`RplProp`; `RplChannel` {Reliable, Unreliable}; `RplRcver` {Server, Owner, Broadcast}; `RplCondition` {None, OwnerOnly, NoOwner, Custom}; `RplComponent`: `Give`/`GiveExt`, `EnableStreamingConNode`, `EnableStreamingForConnection` («Use with caution… performance»), `InsertMPObserver`; `RplError.JIP_ERROR` | EN/group__Replication.html, AR/interfaceRplComponent.html, EN/interfaceEntityWithRplRpc.html (пример в `GameLib/replication/RplDocs.c`) | **ЗАКРЫТ** (DOC) по сигнатурам; поведение JIP — runtime |
| Q12 | Respawn / смена сущности | `SCR_PlayerController.OnControlledEntityChanged(IEntity from, IEntity to)` + invoker; `BaseGameMode.OnPlayerSpawned(int, IEntity)`; `SCR_BaseGameMode.GetOnPlayerSpawned()`; `SCR_BaseGameModeComponent.OnPlayerKilled / OnPlayerDeleted / OnControllableSpawned`; `OnPlayerConnected` — «Server-only» | AR/interfaceSCR__PlayerController.html, AR/interfaceBaseGameMode.html, AR/interfaceSCR__BaseGameModeComponent.html | **ЗАКРЫТ** (DOC) |
| Q13 | Лимиты трафика Rpl | Не найдено. Есть только UI-признак low bandwidth (`SCR_ConnectionStateButtonComponent.CheckLowBandwidth`) | AR/interfaceSCR__ConnectionStateButtonComponent.html | **НЕ НАЙДЕНО** → сеть (web1/Q27) / Э1r |
| Q14 | Высота рельефа | `BaseWorld.GetSurfaceY(float x, float z)`; `WorldEditorAPI.GetTerrainSurfaceY` — «Very fast method» (только редактор); `SCR_WorldTools.GetWaterSurfaceY` | EN/interfaceBaseWorld.html, EN/interfaceWorldEditorAPI.html, AR/interfaceSCR__WorldTools.html | **ЗАКРЫТ** (DOC); стоимость — Э1r |
| Q15 | Трассировка | `BaseWorld.TraceMove(inout TraceParam, TraceFilterCallback = null)` → 0..1 (**только первое попадание**); `AsyncTraceMove(...)` — асинхронно с callback; `TracePosition`, `TraceLineToEntity`; `ChimeraCharacter.TraceMoveWithoutCharacters` — оптимизация. TraceParam: `Start/End/Flags/LayerMask/TargetLayers/Exclude(Array)/Include(Array)`; выход `TraceEnt`, `SurfaceProps`, `TraceMaterial` («usualy only on terrain»), `TraceNorm`, `TraceDist`, `ColliderName` | EN/interfaceBaseWorld.html, EN/interfaceTraceParam.html, AR/interfaceChimeraCharacter.html | **ЗАКРЫТ** (DOC). Для подсчёта нескольких препятствий нужна серия трасс; `AsyncTraceMove` — разгрузка тика |
| Q16 | Погода | `BaseWeatherManagerEntity.GetRainIntensity()`, `GetFogAmount()`, `GetFogHeightDensity()` (+ override только у authority); `BaseTimeAndWeatherManagerEntity.TryGetCompleteLocalWeather(LocalWeatherSituation, float swayFrequency, vector location)` — «rain, fog, overcast, wind»; `LocalWeatherSituation.GetOvercast/GetFog/GetGlobalWindSpeed` | EN/interfaceBaseWeatherManagerEntity.html, AR/interfaceBaseTimeAndWeatherManagerEntity.html, AR/interfaceLocalWeatherSituation.html | **ЗАКРЫТ** (DOC) |
| Q17 | Материалы препятствий | `TraceParam.SurfaceProps` (SurfaceProperties); `GameMaterial.GetBallisticInfo()` → `BallisticInfo.GetDensity/GetThickness/GetThicknessMax/IsWaterSurface` | AR/interfaceGameMaterial.html, AR/interfaceBallisticInfo.html | **ЧАСТИЧНО**: связь SurfaceProperties → GameMaterial не подтверждена; кандидат на потери по плотности и толщине |
| Q18 | Ввод / действия | `InputManager` (EN): `GetActionKeybinding`, `CreateUserBinding`; `AddActionListener` в индексе есть лишь как обёртки (`ArmaReforgerScripted.AddActionListeners`); конфиг действий — `chimeraInputCommon.conf` (по LC/CVON, notes_refs §6) | EN/interfaceInputManager.html | **ЧАСТИЧНО** → cx (LC/CVON) |
| Q20 | Свои меню | `MenuManager` / `ChimeraMenuPreset` + `chimeraMenus.conf` («Specify presets in ChimeraMenuPreset enum and add them in chimeraMenus.conf»); `MenuBase` | AR/interfaceArmaReforgerScripted.html, AR/interfaceEditorMenuBase.html | **ЗАКРЫТ** (DOC) по механизму |
| Q21 | Persistence | **Движковая `PersistenceSystem`** (Plugins_Persistence_System): `GetInstance()`, `Save(Managed entityOrState, ESaveGameType)`, `RequestLoad`, `RequestSpawn`, `CommitStorage`, `ClearStorage`; `PersistenceIdUtils.ConfigureHive` (общие БД). Альтернативы: `JsonApiStruct.LoadFromFile/PackToFile/SaveToFile`; `FileIO.OpenFile('$profile:…')` | AR/interfacePersistenceSystem.html, AR/interfacePersistenceIdUtils.html, EN/interfaceJsonApiStruct.html, EN/interfaceFileIO.html | **ЗАКРЫТ** (DOC); выбор — Э8a |
| Q22 | Предметы SIM/телефон | `InventoryStorageManagerComponent.Contains`, `FindItem(InventorySearchPredicate)`, `FindItemsWithComponents(out array<IEntity>, array<TypeName>)` | AR/interfaceInventoryStorageManagerComponent.html | **ЗАКРЫТ** (DOC) |
| Q23 | Серверный конфиг мода | `JsonApiStruct.LoadFromFile` + `$profile:` — кандидат; `SCR_CombinedDSConfig` / `DSGameConfig` — конфиг DS (UI) | EN/interfaceJsonApiStruct.html, AR/interfaceSCR__CombinedDSConfig.html | **ЧАСТИЧНО** → web2 |

## Новые факты, влияющие на план
1. **`PersistenceSystem`** — движковое сохранение. Кандидат для номеров и SMS через рестарт (Э8a persistence
   seam). Не нужен самописный файловый формат.
2. **`AsyncTraceMove`** — асинхронные трассы снимают нагрузку с тика HDSP L1/L2 (R-25, R5).
3. **`TraceMove`** возвращает одно попадание → многослойная окклюзия требует итеративных трасс или
   `TraceLineToEntity`. Учесть в бюджете Э4.
4. **`Faction.GetFactionRadioEncryptionKey`** — ванильный ключ = фракция. Уникальный ключ на вызов (H6) должен
   не ломать ванильную фракционную логику.
5. Значения `ECommMethod` в публичных docs не перечислены → SOURCE.

## Пробел, который нельзя закрыть без Workbench
Ванильные скрипты, префабы и ассеты (`RelayTransceiver` в префабах, `VON.acp`, значения `ECommMethod`,
`SCR_VONController`) лежат в `.pak` игры (игра установлена, 26 ГБ).
- Свой распаковщик формата `.pak` = reverse engineering данных → запрещено лицензией Tools
  (License.txt, «You May Not»).
- Законный путь — открыть Workbench (без Play-режима, разрешено владельцем) и читать их в Resource
  Browser / Script Editor.

Это действие в GUI. Нужен либо владелец за Workbench, либо разрешённый инструмент управления экраном.
