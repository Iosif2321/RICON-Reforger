# Э1s — статическая экспертиза VoN/радио по офлайн Script API

Автор: claude. Дата: 2026-10-18. Уровень доказательств: **DOC_EVIDENCE** (если не указано иное).
Ни один пункт ниже не является runtime PASS (PLAN v5 §4.2).

## Источник
Офлайн-документация Doxygen из Arma Reforger Tools:

```
DOCS = C:\Program Files (x86)\Steam\steamapps\common\Arma Reforger Tools\Workbench\docs\ArmaReforgerScriptAPIPublic\html
```

Извлечение: скрипт-дампер (только чтение HTML), сигнатуры взяты из `<table class="memname">`,
наследование — из `alt` диаграммы. Исходников ванильных скриптов в docs нет: `*_source.html` = 0.
Ванильные скрипты и префабы — после установки игры, в Workbench (`SOURCE_EVIDENCE`).

## 1. Голос: VoNComponent / SCR_VoNComponent

Файл: `DOCS\interfaceVoNComponent.html`. Описание: «Component responsible for recording and playback of
voice over network».

| Член | Сигнатура / цитата | Значение для RICON |
|---|---|---|
| `SetCapture` | `proto external bool SetCapture(bool isCapturing)` — «Request to starts/stop audio capturing» | клиентский захват |
| `SetCommMethod` / `GetCommMethod` | `proto external void SetCommMethod(ECommMethod type)` | direct / radio |
| `SetTransmitRadio` / `GetTransmitRadio` | `proto external void SetTransmitRadio(BaseTransceiver transceiver)` | **один** передатчик на компонент |
| `OnCapture` | `void OnCapture(BaseTransceiver transmitter)` — «polled each frame while … recording … Null if only direct speech» | клиент |
| `OnReceive` | `void OnReceive(int playerId, bool isSenderEditor, BaseTransceiver receiver, int frequency, float quality)` — «Event invoked when component receives audio data for playback … frequency … Can be different from the frequency which transceiver have set. This is very questionable and could be changed in future. quality — Value in range <0,1> describing quality of the transmission» | **только уведомление**; звук не меняет. Предупреждение BI о нестабильности `frequency` |
| `OnVoNUsed` | `void OnVoNUsed(int senderId)` — «Event invoked on the server, each time VoN is used» | сервер знает говорящего |
| `SoundEventPriority` | `proto external void SoundEventPriority(string eventname, array<float> values, int priority, bool ignoreQueue = false)` — «Transfer AI sound message through VoN» | AI-сообщения, не голос игрока |

`SCR_VoNComponent` (`DOCS\interfaceSCR__VoNComponent.html`):
- `m_OnReceivedVON`, `m_OnCaptureVON`, `GetOnVoNUsed()`;
- `IsTransmiting()`, `IsTransmitingRadio()`.

Методов громкости, фильтра или эффекта на говорящего нет.

## 2. Трансиверы

### BaseTransceiver
Файл: `DOCS\interfaceBaseTransceiver.html`.
- `SetFrequency(int freq)` — «Sets frequency. Supports proxies and server», кГц.
- `SetRange(float range)` — «Sets maximum transmitting range of radio. **Has effect only on owner (server)**.
  Valid values are 1 to 65535. Values outside this range are clamped.» → **дальность TX ≤ 65 535 м**.
- `GetRange`, `GetFrequency`, `GetMin/MaxFrequency`, `GetFrequencyResolution` (кГц).
  В документации описания Min/Max перепутаны.
- `SetMuteState(bool) → bool` — «Toggles the mute state of the transceiver»; `IsMuted`. Кто вправе
  вызывать — не указано → runtime.
- `BeginTransmission(BaseRadioMessage)`, `BeginTransmissionFreq(BaseRadioMessage, int frequency)` — передача
  **сообщения** (не голоса) на частоте; `BaseRadioMessage.SetEncryptionKey/GetEncryptionKey`.
- `GetRadio() → BaseRadioComponent`.

### Наследники BaseTransceiver
`RadioTransceiver`, `RelayTransceiver`, `IntercomTransceiver`, `EditorTransceiver`,
`EditorFactionTransceiver`.
- **`RelayTransceiver` и `IntercomTransceiver` не имеют ни одного скриптового члена** (страницы пустые,
  только «Inherits BaseTransceiver»).
- Вывод: поведение relay/intercom задаётся движком и/или атрибутами префаба, а не скриптом. Что именно
  ретранслируется (голос? с какой частоты на какую?) из docs **не следует** → нужен `SOURCE_EVIDENCE`
  (ванильные префабы/конфиги в Workbench) и затем Э1r.

## 3. Радио-компоненты

### BaseRadioComponent
Файл: `DOCS\interfaceBaseRadioComponent.html`.
- `SetEncryptionKey(string)` / `GetEncryptionKey()`.
- `SetPower(bool)` / `IsPowered()`.
- `TransceiversCount()`, `GetTransceiver(int idx)`.
- `SetTransceiverFrequency(BaseTransceiver, int freq)` — «Set frequency of transceiver and sync with server».
- События на клиенте и сервере: `OnEncryptionKeyChanged`, `OnPowerChanged`, `OnTansceiverFrequencyChanged`,
  `OnTansceiverRangeChanged` (+ ScriptInvoker).
- `GetOwner()`, `IsEditorRadio()`.

### ScriptedRadioComponent — ключевая находка для H7
Файл: `DOCS\interfaceScriptedRadioComponent.html`.
- Наследование по диаграмме: `GameComponent → BaseRadioComponent → ScriptedRadioComponent →
  SCR_CoverageRadioComponent`.
- Скриптовые хуки: `OnInit`, `OnPostInit`, `OnFrame(IEntity owner, float timeSlice)`.
- **Вывод (DOC):** мод может объявить собственный радиокомпонент, наследник `BaseRadioComponent`, с
  трансиверами из префаба и своей логикой. Это законная точка расширения внутри мода (H7) без RE.
  Поведение трансиверов такого компонента в VoN — runtime.

### RadioManagerEntity
Файл: `DOCS\interfaceRadioManagerEntity.html`.
- `proto external int GetTransceiversInRange(vector position, float range, out notnull array<BaseTransceiver> tsvList)`
  — «Gets all transceivers in given range that are in power ON state».
- **Вывод:** движковый пространственный индекс трансиверов. Это кандидат для CandidateGraph (вместо
  O(P²), R-25). Стоимость — runtime, Э1r п.13.

### SCR_RadioComponent (ванильный гаджет)
Файл: `DOCS\interfaceSCR__RadioComponent.html`.
- Обёртка `SCR_GadgetComponent` над `BaseRadioComponent`: `GetRadioComponent()`, `ChangeFrequencyStep`,
  `RadioToggle`, `GetRadioType`, анимация ручек, сигналы `m_iSignalChannelMhz/Khz/Power`.

### SCR_VONEntryRadio
Файл: `DOCS\interfaceSCR__VONEntryRadio.html`.
- `GetTransceiver()`, `GetTransceiverNumber()`, `IsLongRange()`, `ToggleMuteEntry()`.
- `GetEntryFrequency()` — «Local frequency getter since atm the transceiver getter is delayed (network?)».
  Это **предупреждение о задержке синхронизации частоты** — учесть в Э3/Э4T.

## 4. Ванильная «покрытийная» сеть ретрансляторов (Conflict)

### SCR_CoverageRadioComponent
Файл: `DOCS\interfaceSCR__CoverageRadioComponent.html`.
- Описание: «Determinates if the owner entity is covered by the radio signal eminating from the component
  marked as Source with the same encryption key».
- Члены: `IsSource/SetIsSource`, `GetCoverageByEncryption`, `GetRadiosInRange(Of)`, `Ping(encryptionKey, …)`,
  `m_mCoverage`.

### SCR_RadioCoverageSystem
Файл: `DOCS\interfaceSCR__RadioCoverageSystem.html`.
- `UpdateCoverage(encryptionKey)`, `RegisterRadioComponent`, `UPDATE_DELAY = 2000` (мс).

### Вывод
В ванильной игре есть граф покрытия радиоточек: источник → цепочка релеев по дальности и ключу, обновление
раз в 2 с. Это **логика игрового режима** (связь баз), а не доставка голоса. Как идея пригодна для
control-plane RICON:
- граф связности по ключу сети;
- инкрементальный пересчёт.

Голосовой ретранслятор она не доказывает. Код — BI (проприетарный): только идеи, не копировать.

## 5. Лицензия Tools и reverse engineering
Файл: `C:\Program Files (x86)\Steam\steamapps\common\Arma Reforger Tools\Docs\License.txt`, раздел «You May Not».
- Запрет: «You are not allowed to hack, modify or reverse engineer the software or any software files, its
  data or network Services».
- Также: «You may use the software and the content you create using the software for non-commercial
  purposes only».

**Решение:** условие 3 RE-gate (законность/EULA) не выполнено → **RE = NO-GO**. Чтение ванильных скриптов и
префабов в Workbench — штатный моддинг, это не RE.

## 6. Статус гипотез после этого шага

| Гипотеза | Уровень | Вывод | Что дальше |
|---|---|---|---|
| H1 доставка радио сервером | DOC | `SetRange` только сервер, ≤65 535 м; `SetFrequency` — прокси и сервер; ключ; питание | Э1r |
| H2 mute RX | DOC | метод есть, права не указаны | Э1r |
| H3 качество → звук | DOC (отсутствие) | скриптового пути нет; `quality` — только уведомление | SOURCE: граф VON в Workbench |
| H4 SetRange-аппроксимация | DOC | один скаляр на TX | Э1r |
| H6 скрытые приёмники | DOC | несколько трансиверов на компонент (`TransceiversCount`) | Э1r |
| H7 in-mod функционал | **DOC: точка расширения есть** | `ScriptedRadioComponent` (наследник `BaseRadioComponent`); `RadioManagerEntity.GetTransceiversInRange` | SOURCE + Э1r |
| Relay (голос) | DOC | `RelayTransceiver` без скриптовых членов; поведение — в движке/префабе | SOURCE: ванильные префабы с RelayTransceiver; Э1r |
| Ванильная сеть покрытия | DOC | `SCR_CoverageRadioComponent` / `SCR_RadioCoverageSystem` — граф по ключу, 2 с | идея для control plane |

## 7. Открыто для SOURCE_EVIDENCE (нужна установленная игра + Workbench без Play)
1. Префабы с `RelayTransceiver` и его атрибуты: входная/выходная частота? голос или только покрытие?
2. Звуковой граф VON (`.acp`/`.sig`): есть ли входы quality/transceiver/frequency.
3. `SCR_VONController`, `SCR_VoNComponent.OnReceive`: что ванильный код делает с `quality`.
4. Атрибуты `BaseRadioComponentClass` / трансиверов в префабах раций (диапазоны, мощность, дальность).
5. `ECommMethod` — значения перечисления.
