# Э-A: адресные окна ванильных VON-классов (WbReader, read-only)

Дата: 2026-10-23. Исполнил: claude по заказу окон codex (agora #3939), ACK #3946, END #3948. Уровень:
**WORKBENCH_STATIC** — чтение исходников через штатный `tools/RICON_WbReader/run.ps1`, ScriptEditor без мира, Play и
сервера; ≤ 15 строк на запуск. Это не runtime и не доказательство поведения в игре. Цель — точки подписки для
прибора наблюдения radio-only (`LIVE-order-frequency-mute.md`, `AUDIO_CHAIN_CONTROL_RU.md` §2, §5 Э-A).

Семь запусков подряд, у каждого exit 0, `SearchResources=true`, 1 кандидат, `[RICON_WbReader] done`, SCRIPT (E) = 0
(прочие (E) — штатные Steamworks `RequestCurrentStats` и «Resource leaks» UI-текстур). Логи —
`%USERPROFILE%\Documents\My Games\ArmaReforgerWorkbench\logs\<папка>\console.log`.

| Окно | Папка лога | sha256 | Строк |
|---|---|---|---|
| ea1 `SCR_VONController.c` grep `ActivateVON` C6 | `ricon-wbreader-ea1-activatevon-20261023-203039` | `867cfa25f0d92871b41a4ed2c49bff1087b6a08ae17421bdcd32c55a6f574483` | 15 (кап) |
| ea2 `SCR_VONController.c` grep `DeactivateVON` C6 | `ricon-wbreader-ea2-deactivatevon-20261023-203049` | `83109c8f209e06dc011c87fd750624ee16e3e8464507e85736c3677fbc2c3bcc` | 15 (кап) |
| ea3 `SCR_VONController.c` line 622 (grep `SetActiveTransmit`) C6 | `ricon-wbreader-ea3-vonctrl-622-20261023-203058` | `87c4d91f29d2a6a596d0966811d267eb45c10ad7218819bd85c66959eccdaf43` | 15 |
| ea4 `SCR_VoNComponent.c` line 78 (grep `OnReceive`) C6 | `ricon-wbreader-ea4-onreceive-78-20261023-203106` | `f7c1428342c26091f9ab14a68f1145cf5be3197c2a9c772f68479fce9d7198cd` | 15 |
| ea5 `SCR_VoNComponent.c` grep `OnCapture` C6 | `ricon-wbreader-ea5-oncapture-20261023-203115` | `405e8dc62897070acd77f5771ab849649892516ab30bcfead70536e4588a7936` | 15 (кап) |
| ea6 `SCR_BaseGameMode.c` grep `OnPlayerSpawned` C5 | `ricon-wbreader-ea6-onplayerspawned-20261023-203123` | `0c7d2d28f5c84d45fdfa24248e8da8c70999a42a95a8f18ce21e3eea36ff2339` | 15 (кап) |
| ea7 `SCR_VONEntryRadio.c` grep `ToggleMute` C5 | `ricon-wbreader-ea7-togglemute-20261023-203131` | `f0cbaa12f93112a477c2353edf54e9a3b2b0d94bef1e96819757603743028f6b` | 11 |

## Прочитанные строки (ровно как в логе; пути `$ArmaReforger:scripts/…`)

**ea1 — `Game/VON/SCR_VONController.c:307–319`, `:327–328`** (контекст вызова, тело `ActivateVON` не достигнуто):
```
307:            return;
309:        if (activate && m_eVONType == EVONTransmitType.DIRECT)
310:            return;
312:        if (activate)
313:            ActivateVON(EVONTransmitType.DIRECT);
314:        else
315:            DeactivateVON(EVONTransmitType.DIRECT);
316:    }
319:    protected void SetVONProximityToggle(bool activate)
327:        if (m_bIsToggledDirect == activate)
328:            return;
```

**ea2 — `SCR_VONController.c:207–219`, `:249–250`** (тело `DeactivateVON` не достигнуто):
```
208:        entry.SetActive(true);
209:        m_ActiveEntry = entry;
210:        m_OnEntriesActiveChanged.Invoke(entry, true);
212:        if (m_ActiveEntry && !m_ActiveEntry.IsUsable())    // turn off toggle when switching to disabled entry
213:            DeactivateVON();
215:        if (m_VONDisplay)
216:            m_VONDisplay.ShowSelectedVONHint(m_ActiveEntry);
218:        SCR_VONEntryRadio radioEntry = SCR_VONEntryRadio.Cast(entry);
219:        if (radioEntry)
250:        m_OnEntriesChanged.Invoke(entry, false);
```

**ea3 — `SCR_VONController.c:622–636`:**
```
622:    //! Set transmission method depending on entry type when starting VON transmit
623:    protected void SetActiveTransmit(notnull SCR_VONEntry entry)
624:    {
625:        if (!m_VONComp && !AssignVONComponent())
626:            return;
628:        if (entry.GetVONMethod() == ECommMethod.SQUAD_RADIO)
629:        {
630:            m_VONComp.SetCommMethod(ECommMethod.SQUAD_RADIO);
631:            m_VONComp.SetTransmitRadio(SCR_VONEntryRadio.Cast(entry).GetTransceiver());
632:            SetEntryActive(entry);
633:        }
634:        else 
635:        {
636:            m_VONComp.SetCommMethod(ECommMethod.DIRECT);
```

**ea4 — `GameCode/Network/SCR_VoNComponent.c:78–92`:**
```
78:     override protected event void OnReceive(int playerId, bool isSenderEditor, BaseTransceiver receiver, int frequency, float quality)
79:     {
80:         SCR_VonDisplay display = GetDisplay();
82:         if (display)
83:             display.OnReceive(playerId, isSenderEditor, receiver, frequency, quality);
85:         m_OnReceivedVON.Invoke(playerId, receiver, frequency, quality);
86:     }
89:     override protected event void OnVoNUsed(int senderId)
90:     {
91:         if (m_OnVoNUsed)
92:             m_OnVoNUsed.Invoke(senderId);
```

**ea5 — `SCR_VoNComponent.c:8–20`, `:56–57`** (тело `OnCapture` не достигнуто):
```
8:      private const float TRANSMISSION_TIMEOUT_MS = 500;
9:      private float m_fTransmitingTimeout;
10:     private float m_fTransmitRadioTimeout;
11:     private SCR_VonDisplay m_VONDisplay;
13:     ref ScriptInvoker m_OnReceivedVON = new ScriptInvoker();
14:     ref ScriptInvoker m_OnCaptureVON = new ScriptInvoker();
15:     protected static ref ScriptInvokerInt m_OnVoNUsed;
18:     static ScriptInvokerInt GetOnVoNUsed()
19:     {
20:         if (!m_OnVoNUsed)
56:     bool IsTransmitingRadio()
57:     {
```

**ea6 — `Game/GameMode/SCR_BaseGameMode.c:157–167`, `:624–627`:**
```
157:    protected ref ScriptInvokerBase<SCR_BaseGameMode_PlayerId> m_OnPlayerAuditRevived = new ScriptInvokerBase<SCR_BaseGameMode_PlayerId>();
158:    protected ref ScriptInvokerBase<SCR_BaseGameMode_PlayerId> m_OnPlayerConnected = new ScriptInvokerBase<SCR_BaseGameMode_PlayerId>();
159:    protected ref ScriptInvokerBase<SCR_BaseGameMode_PlayerId> m_OnPlayerRegistered = new ScriptInvokerBase<SCR_BaseGameMode_PlayerId>();
160:    protected ref ScriptInvokerBase<SCR_BaseGameMode_OnPlayerDisconnected> m_OnPlayerDisconnected = new ScriptInvokerBase<SCR_BaseGameMode_OnPlayerDisconnected>();
161:    protected ref ScriptInvokerBase<SCR_BaseGameMode_OnPlayerDisconnected> m_OnPostCompPlayerDisconnected = new ScriptInvokerBase<SCR_BaseGameMode_OnPlayerDisconnected>(); //~ Called after the GameMode Components are notified that a player was disconnected
162:    protected ref ScriptInvokerBase<SCR_BaseGameMode_PlayerIdAndEntity> m_OnPlayerSpawned = new ScriptInvokerBase<SCR_BaseGameMode_PlayerIdAndEntity>();
163:    protected ref ScriptInvokerBase<SCR_BaseGameMode_OnControllableDestroyed> m_OnPlayerKilled = new ScriptInvokerBase<SCR_BaseGameMode_OnControllableDestroyed>();
164:    protected ref ScriptInvokerBase<SCR_BaseGameMode_OnControllableDestroyed> m_OnControllableDestroyed = new ScriptInvokerBase<SCR_BaseGameMode_OnControllableDestroyed>();
165:    protected ref ScriptInvokerBase<SCR_BaseGameMode_PlayerIdAndEntity> m_OnPlayerDeleted = new ScriptInvokerBase<SCR_BaseGameMode_PlayerIdAndEntity>();
166:    protected ref ScriptInvokerBase<SCR_BaseGameMode_OnPlayerRoleChanged> m_OnPlayerRoleChange = new ScriptInvokerBase<SCR_BaseGameMode_OnPlayerRoleChanged>();
625:    ScriptInvokerBase<SCR_BaseGameMode_OnPlayerDisconnected> GetOnPostCompPlayerDisconnected()
626:    {
627:        return m_OnPostCompPlayerDisconnected;
```

**ea7 — `Game/VON/SCR_VONEntryRadio.c:229–239`** (сам вызов mute не достигнут):
```
229:        else
230:            SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_RADIO_TURN_ON);
231:    }
234:    bool ToggleMuteEntry()
235:    {
236:        // Constructor allows the entry to not have m_RadioTransceiver assigned
237:        if (!m_RadioTransceiver)
238:            return false;
```

## Что из этого следует (только прочитанное)

1. `SCR_VoNComponent` публикует `ref ScriptInvoker m_OnReceivedVON` (`Invoke(playerId, receiver, frequency,
   quality)`), `ref ScriptInvoker m_OnCaptureVON` и статический `GetOnVoNUsed()` (`ScriptInvokerInt`, `Invoke(senderId)`).
   Значит, наблюдатели прибора на приём (`RX_EVENT`) и на «кто говорит» (`TX_SEEN`) можно подписать штатно, **без
   `modded class`**; подписка на `m_OnCaptureVON` — кандидат для `TX_START/TX_STOP` (тело `OnCapture` и аргументы
   `Invoke` не прочитаны — следующее окно).
2. `SCR_VONController.SetActiveTransmit` (:623–636): радио → `SetCommMethod(SQUAD_RADIO)` + `SetTransmitRadio(
   entry.GetTransceiver())` + `SetEntryActive`; иначе → `SetCommMethod(DIRECT)` (продолжение не прочитано). Это
   подтверждает раздельные ветви радио/direct на стороне контроллера — основа требования «прямую речь не трогаем».
3. `SCR_BaseGameMode` держит защищённые invokers жизненного цикла (`m_OnPlayerSpawned`, `m_OnPlayerConnected`,
   `m_OnPlayerDisconnected`, …) с публичными getter-ами (прочитан `GetOnPostCompPlayerDisconnected()`); getter для
   `m_OnPlayerSpawned` вероятен, но в окно не попал — следующее окно.
4. `SCR_VONEntryRadio.ToggleMuteEntry()` возвращает false без трансивера; какой native-вызов он делает дальше
   (`SetMuteState`?) — не прочитано — следующее окно.

Не прочитано и потому не утверждается: тела `ActivateVON(EVONTransmitType)`, `DeactivateVON(...)`, `OnCapture`,
продолжение `SetActiveTransmit` для DIRECT, тело `ToggleMuteEntry` после проверки, getter `GetOnPlayerSpawned`.
Предложенные дочитки (по одному окну): `SCR_VONController.c` grep `void ActivateVON` C7 и `void DeactivateVON` C7;
`SCR_VoNComponent.c` grep `void OnCapture` C7; `SCR_VONEntryRadio.c` line 240 C7; `SCR_BaseGameMode.c` grep
`GetOnPlayerSpawned` C3.

Воспроизведение: `docs/evidence/receipts/ea-windows-claude.ps1` → по одному `tools/RICON_WbReader/run.ps1 -Name
ea<N>-… -Root '$ArmaReforger:scripts' -Ext c -Grep <…> -PathFilter <файл> -Context <N> -MaxFiles 1 [-Line <N>]`.
