# Э-A: адресные окна ванильных VON-классов (WbReader, read-only)

Дата: 2026-10-23. Исполнил: claude по заказу окон codex (agora #3939), ACK #3946, END #3951; вторая серия — ACK #3953.
Уровень: **WORKBENCH_STATIC** — чтение исходников через штатный `tools/RICON_WbReader/run.ps1`, ScriptEditor без мира,
Play и сервера; ≤ 15 строк на запуск. Это не runtime и не доказательство поведения в игре. Цель — точки подписки для
прибора наблюдения radio-only (`LIVE-order-frequency-mute.md`, `AUDIO_CHAIN_CONTROL_RU.md` §2, §5 Э-A).

Ванильный текст в репозиторий не копируется (правило `tools/RICON_WbReader/README`, codex #3965): здесь только
`путь:строки`, сигнатуры и смысл своими словами; сырые окна остаются в логах Workbench, их sha256 — в таблицах ниже.
Логи — `%USERPROFILE%\Documents\My Games\ArmaReforgerWorkbench\logs\<папка>\console.log`; у каждого запуска exit 0,
`SearchResources=true`, 1 кандидат, `[RICON_WbReader] done`, SCRIPT (E) = 0 (прочие (E) — штатные Steamworks
`RequestCurrentStats` и «Resource leaks» UI-текстур). Пути файлов — `$ArmaReforger:scripts/…`.

## Первая серия (ea1–ea7)

| Окно | Папка лога | sha256 | Строк |
|---|---|---|---|
| ea1 `SCR_VONController.c` grep `ActivateVON` C6 | `ricon-wbreader-ea1-activatevon-20261023-203039` | `867cfa25f0d92871b41a4ed2c49bff1087b6a08ae17421bdcd32c55a6f574483` | 15 (кап) |
| ea2 `SCR_VONController.c` grep `DeactivateVON` C6 | `ricon-wbreader-ea2-deactivatevon-20261023-203049` | `83109c8f209e06dc011c87fd750624ee16e3e8464507e85736c3677fbc2c3bcc` | 15 (кап) |
| ea3 `SCR_VONController.c` line 622 (grep `SetActiveTransmit`) C6 | `ricon-wbreader-ea3-vonctrl-622-20261023-203058` | `87c4d91f29d2a6a596d0966811d267eb45c10ad7218819bd85c66959eccdaf43` | 15 |
| ea4 `SCR_VoNComponent.c` line 78 (grep `OnReceive`) C6 | `ricon-wbreader-ea4-onreceive-78-20261023-203106` | `f7c1428342c26091f9ab14a68f1145cf5be3197c2a9c772f68479fce9d7198cd` | 15 |
| ea5 `SCR_VoNComponent.c` grep `OnCapture` C6 | `ricon-wbreader-ea5-oncapture-20261023-203115` | `405e8dc62897070acd77f5771ab849649892516ab30bcfead70536e4588a7936` | 15 (кап) |
| ea6 `SCR_BaseGameMode.c` grep `OnPlayerSpawned` C5 | `ricon-wbreader-ea6-onplayerspawned-20261023-203123` | `0c7d2d28f5c84d45fdfa24248e8da8c70999a42a95a8f18ce21e3eea36ff2339` | 15 (кап) |
| ea7 `SCR_VONEntryRadio.c` grep `ToggleMute` C5 | `ricon-wbreader-ea7-togglemute-20261023-203131` | `f0cbaa12f93112a477c2353edf54e9a3b2b0d94bef1e96819757603743028f6b` | 11 |

| Окно | Путь:строки | Сигнатура / смысл (своими словами) |
|---|---|---|
| ea1 | `Game/VON/SCR_VONController.c:307–319`, `:327–328` | Toggle-обёртка прямой речи: при активации с уже активным `EVONTransmitType.DIRECT` — ранний выход; иначе вызов `ActivateVON(DIRECT)` либо `DeactivateVON(DIRECT)`. Далее начало `protected void SetVONProximityToggle(bool activate)` с ранним выходом при неизменном `m_bIsToggledDirect`. Тело `ActivateVON` в окно не попало. |
| ea2 | `SCR_VONController.c:207–219`, `:249–250` | Активация entry: `entry.SetActive(true)`, запоминание в `m_ActiveEntry`, `m_OnEntriesActiveChanged.Invoke(entry, true)`; если активная entry непригодна (`IsUsable()` false) — `DeactivateVON()`; дисплей показывает подсказку выбранной entry; приведение к `SCR_VONEntryRadio` с ветвью для радио; в другом месте `m_OnEntriesChanged.Invoke(entry, false)`. Тело `DeactivateVON` не достигнуто. |
| ea3 | `SCR_VONController.c:622–636` | `protected void SetActiveTransmit(notnull SCR_VONEntry entry)`: без `m_VONComp` (и при неудачном `AssignVONComponent()`) — выход; если `entry.GetVONMethod()` = `ECommMethod.SQUAD_RADIO` — `m_VONComp.SetCommMethod(SQUAD_RADIO)`, `m_VONComp.SetTransmitRadio(<трансивер entry>)`, `SetEntryActive(entry)`; иначе `m_VONComp.SetCommMethod(DIRECT)` (продолжение не прочитано). |
| ea4 | `GameCode/Network/SCR_VoNComponent.c:78–92` | `override protected event void OnReceive(int playerId, bool isSenderEditor, BaseTransceiver receiver, int frequency, float quality)`: передаёт то же дисплею VON, затем `m_OnReceivedVON.Invoke(playerId, receiver, frequency, quality)`. `override protected event void OnVoNUsed(int senderId)` → `m_OnVoNUsed.Invoke(senderId)`, если invoker создан. |
| ea5 | `SCR_VoNComponent.c:8–20`, `:56–57` | Константа `TRANSMISSION_TIMEOUT_MS = 500`, поля таймаутов передачи и радио-передачи, ссылка на дисплей; публичные `ref ScriptInvoker m_OnReceivedVON` и `m_OnCaptureVON`; `protected static ref ScriptInvokerInt m_OnVoNUsed` с ленивым созданием в `static ScriptInvokerInt GetOnVoNUsed()`; объявление `bool IsTransmitingRadio()`. |
| ea6 | `Game/GameMode/SCR_BaseGameMode.c:157–167`, `:624–627` | Защищённые invokers жизненного цикла (`ScriptInvokerBase<…>`): audit revived, connected, registered, disconnected, post-comp disconnected, **spawned** (тип `SCR_BaseGameMode_PlayerIdAndEntity`), killed, controllable destroyed, deleted, role change; публичный getter `GetOnPostCompPlayerDisconnected()`. |
| ea7 | `Game/VON/SCR_VONEntryRadio.c:229–239` | Хвост соседнего метода: UI-звук включения радио. `bool ToggleMuteEntry()`: комментарий о том, что конструктор допускает entry без трансивера; без `m_RadioTransceiver` возвращает false. |

## Что из этого следует (только прочитанное)

1. `SCR_VoNComponent` публикует `ref ScriptInvoker m_OnReceivedVON` (`Invoke(playerId, receiver, frequency,
   quality)`), `ref ScriptInvoker m_OnCaptureVON` и статический `GetOnVoNUsed()` (`ScriptInvokerInt`, `Invoke(senderId)`).
   Значит, наблюдатели прибора на приём (`RX_EVENT`) и на «кто говорит» (`TX_SEEN`) можно подписать штатно, **без
   `modded class`**; подписка на `m_OnCaptureVON` — кандидат для `TX_START/TX_STOP` (см. ea10).
2. `SCR_VONController.SetActiveTransmit` (:623–636): радио → `SetCommMethod(SQUAD_RADIO)` + `SetTransmitRadio(
   entry.GetTransceiver())` + `SetEntryActive`; иначе → `SetCommMethod(DIRECT)` (продолжение не прочитано). Это
   подтверждает раздельные ветви радио/direct на стороне контроллера — основа требования «прямую речь не трогаем».
3. `SCR_BaseGameMode` держит защищённые invokers жизненного цикла (`m_OnPlayerSpawned`, `m_OnPlayerConnected`,
   `m_OnPlayerDisconnected`, …) с публичными getter-ами (прочитан `GetOnPostCompPlayerDisconnected()`); getter для
   `m_OnPlayerSpawned` — см. ea12.
4. `SCR_VONEntryRadio.ToggleMuteEntry()` возвращает false без трансивера; продолжение — см. ea11.

Не прочитано в первой серии: тела `ActivateVON(EVONTransmitType)`, `DeactivateVON(...)`, `OnCapture`, продолжение
`SetActiveTransmit` для DIRECT, тело `ToggleMuteEntry` после проверки, getter `GetOnPlayerSpawned` — закрыто второй
серией ниже, кроме DIRECT-ветви `SetActiveTransmit` и тела перегрузки `ActivateVON(entry, type)`.

## Вторая серия: адресные дочитки ea8–ea12 (ACK codex #3953, тот же метод)

| Окно | Папка лога | sha256 | Строк |
|---|---|---|---|
| ea8 `SCR_VONController.c` grep `void ActivateVON` C7 | `ricon-wbreader-ea8-activatevon-def-20261023-203658` | `851b2463cb87327b59a13db654e25d739fd1fff6b107e0659b2316c594486f8d` | 15 |
| ea9 `SCR_VONController.c` grep `void DeactivateVON` C7 | `ricon-wbreader-ea9-deactivatevon-def-20261023-203714` | `1dd8919757f967f12e80069a5f60b1f1b453479d2ab719996c297bfd50be8dca` | 15 |
| ea10 `SCR_VoNComponent.c` grep `void OnCapture` C7 | `ricon-wbreader-ea10-oncapture-def-20261023-203726` | `aa3e6c344f4c5731ae5b71842efcf95d81d5d98234a69e8ea6559faa39705cda` | 15 |
| ea11 `SCR_VONEntryRadio.c` line 240 C7 | `ricon-wbreader-ea11-togglemute-240-20261023-203735` | `c803759f100ea795383cb8bf5747b295218185620b115d2ff063d5a11b2e545f` | 15 |
| ea12 `SCR_BaseGameMode.c` grep `GetOnPlayerSpawned` C3 | `ricon-wbreader-ea12-getonplayerspawned-20261023-203747` | `13bee42bb3615ecc523396b6d3af2ee51bfd9ddd2decce22af15e28cb510aac5` | 7 |

| Окно | Путь:строки | Сигнатура / смысл (своими словами) |
|---|---|---|
| ea8 | `SCR_VONController.c:517–531` | Хвост соседнего метода: переключение дальней связи `SetVONLongRange(!m_bIsLongRangeToggled)`. `protected void ActivateVON(EVONTransmitType transmitType)`: entry ищется через `GetEntryByTransmitType(transmitType)`; без entry — выход; иначе перегрузка `ActivateVON(entry, transmitType)`. |
| ea9 | `SCR_VONController.c:578–591` | Хвост перегрузки активации: `m_VONComp.SetCapture(true)`, `m_bIsActive = true`, возврат true. `protected void DeactivateVON(EVONTransmitType transmitType = EVONTransmitType.NONE)`: выход без `m_VONComp`; выход, если задан тип и он не равен `m_eVONType` (комментарий: деактивируется только целевой тип, если уже активирован другой). |
| ea10 | `SCR_VoNComponent.c:55–69` | `bool IsTransmitingRadio()` — «недавно активна»: `m_fTransmitRadioTimeout` больше текущего `GetWorld().GetWorldTime()`. `override protected event void OnCapture(BaseTransceiver transmitter)`: дисплею `OnCapture(transmitter)`, затем `m_OnCaptureVON.Invoke(transmitter)`. |
| ea11 | `SCR_VONEntryRadio.c:240–254` | Продолжение `ToggleMuteEntry()`: новое состояние = инверсия `m_RadioTransceiver.IsMuted()`; `m_RadioTransceiver.SetMuteState(state)`; дублирование в entry через `SetMuted(state)`; возврат состояния. `bool GetIsMuted()`: без трансивера — true; комментарий :254 — трансивер entry нельзя использовать напрямую, если entry не активна. |
| ea12 | `SCR_BaseGameMode.c:626–632` | `ScriptInvokerBase<SCR_BaseGameMode_PlayerIdAndEntity> GetOnPlayerSpawned()` — публичный getter, возвращает `m_OnPlayerSpawned`. |

Следствия второй серии (только прочитанное):
5. `OnCapture` вызывает `m_OnCaptureVON.Invoke(transmitter)` — наблюдатель `TX_START/TX_STOP` прибора получает
   передатчик (по DOC — null при прямой речи); сам `OnCapture` — событие по кадрам записи, окончание выводится по
   таймауту (`TRANSMISSION_TIMEOUT_MS = 500`, `IsTransmitingRadio()` по `m_fTransmitRadioTimeout`).
6. Ванильный UI меню VON сам вызывает `m_RadioTransceiver.SetMuteState(state)` на клиенте (`ToggleMuteEntry`,
   :240–242) и дублирует состояние в entry (`SetMuted`). Это SOURCE-подтверждение, что вызов с клиента штатен; смысл
   результата, направление RX/TX, сохранение и репликация по-прежнему runtime. Оговорка :254: трансивер entry
   нельзя использовать напрямую, если entry не активна.
7. `ActivateVON(type)` → `GetEntryByTransmitType` → перегрузка `ActivateVON(entry, type)` (тело не прочитано; её
   хвост :578–580 — `SetCapture(true); m_bIsActive = true`). `DeactivateVON(type)` возвращается без `m_VONComp` и
   при несовпадении типа с `m_eVONType` — радио и direct деактивируются раздельно.
8. `SCR_BaseGameMode.GetOnPlayerSpawned()` публичен — lifecycle-хук прибора без `modded class`.

Для прибора фазы A остаются непрочитанными: тело `ActivateVON(entry, type)` (где именно `SetActiveTransmit`),
DIRECT-ветвь `SetActiveTransmit` после :636, таймаут/окончание в `OnCapture`. Воспроизведение второй серии —
`docs/evidence/receipts/ea-windows2-claude.ps1`.

Воспроизведение: `docs/evidence/receipts/ea-windows-claude.ps1` → по одному `tools/RICON_WbReader/run.ps1 -Name
ea<N>-… -Root '$ArmaReforger:scripts' -Ext c -Grep <…> -PathFilter <файл> -Context <N> -MaxFiles 1 [-Line <N>]`.
Сами окна (≤ 15 строк каждое) — в `console.log` папок из таблиц, проверяются по sha256; в Git они не хранятся.
