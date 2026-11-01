# RICON_LiveProbe — прибор live-проверки (TOOL_ONLY)

Отдельный аддон для опытов по `docs/plan/LIVE-order-frequency-mute.md` и `docs/analysis/AUDIO_CHAIN_CONTROL_RU.md`
(трек 1). Не для Workshop; подключается только на время сессии через `-addonsDir`. `addon.gproj` с e2a19a7 перечисляет
`RICON_Core` и `RICON_CoreTests` в зависимостях. С A1-S2 стенд только читает production-reader
`RICON_BaseRadioObservationReader.TryRead` из Core (codex #4491); код Core стенд не меняет.
Запуски — в рамках разрешения владельца 2026-10-23, с START/END и доказательствами на шине.
Статус среза: **проверки наблюдателя в Workbench пройдены; окончательное ревью ожидается**.
Продолжение без ожидания ACK Claude разрешено владельцем 2026-10-23, Agora #4011; окончательная совместная приёмка
оставлена открытой. После возвращения партнёра действует обычный START → ACK → run → END.

## Состав

- `Scripts/Game/RICON_Live/RICON_LiveJournal.c` — чистый журнал событий: кольцевой буфер с последовательным номером,
  явной записью `GAP` при переполнении (счётчик потерь и время последнего потерянного) и строгим форматом строки
  `[RICON_LIVE] seq=… t=… side=… kind=… player=… device=… tr=… value=… text=…`. Engine-вызовов нет.
  Потоковый режим `SetStream(true)` (claude #4028, codex #4035) печатает каждую запись сразу при `Add`; буфер
  остаётся ограниченным снимком, `GAP` в нём означает вытеснение из снимка, а не потерю уже выведенного потока.
  `Dump()` печатает снимок в рамке `[RICON_LIVE] dump=BEGIN count=… dropped=… nextSeq=…` … `[RICON_LIVE] dump=END`.
- `Scripts/WorkbenchGame/RICON_LiveJournalProbePlugin.c` — самопроверка журнала в Workbench (ScriptEditor, без
  мира): порядок, вытеснение, учёт потерь сразу после первого переполнения, ёмкость 2, формат, отсутствие слова FAIL.
- `Scripts/Game/RICON_Live/RICON_LiveObserver.c` — подписки и отписки от штатных invokers:
  `SCR_BaseGameMode.GetOnPlayerSpawned()`, `SCR_VoNComponent.GetOnVoNUsed()`, `m_OnCaptureVON`, `m_OnReceivedVON`.
  Пара клиентских подписок устанавливается целиком либо отклоняется. Привязка к устройствам остаётся UNKNOWN.
- `Scripts/Game/RICON_Live/RICON_LiveProbeComponent.c` — компонент запуска на GameMode тестового мира:
  `Start()`, `Dump()`, `Stop()`, переподключение к VoN управляемого персонажа раз в секунду, очистка в `OnDelete`,
  остановка через 900 секунд игрового времени. В режиме редактирования не подписывается.
- `Scripts/WorkbenchGame/RICON_LiveObserverProbePlugin.c` — 30 проверок наблюдателя: подписки, отказы, игнор
  прямой речи, сохранение качества меньше одной промилле, частичная подписка, отписка и исчезнувший invoker.
  Компонент запуска скомпилирован; его жизненный цикл в Play ещё не проверялся.
- `Scripts/Game/RICON_Live/RICON_LiveRadioReader.c` — показания раций управляемого персонажа для Q-F (только чтение):
  `SCR_GadgetManagerComponent.GetGadgetsByType(RADIO/RADIO_BACKPACK)` → `BaseRadioComponent` → getter-ы трансивера;
  `RADIO_STATE` только при изменении, `RADIO_GONE` при исчезновении; `device = −1`, `binding=LOCAL_ONLY,obs=…,slot=…`,
  идентичность — слабая ссылка на сущность рации и индекс трансивера (codex #4083); сырой `GetPrefabName` в тексте.
  Компонент запуска опрашивает его раз в секунду. Пробник `RICON_LiveRadioReaderProbePlugin.c` — 16 проверок.
- `Scripts/WorkbenchGame/RICON_LiveResourceProbePlugin.c` — только метаданные: ResourceName и путь префабов раций и
  миров через `ResourceDatabase.SearchResources`, файлы не открываются.
- `Scripts/Game/RICON_Live/RICON_LiveRadioBenchComponent.c` и `Scripts/WorkbenchGame/RICON_LiveBenchLauncherPlugin.c` —
  стенд фазы A0 (`docs/plan/LIVE-order-frequency-mute.md` §4): изолированные рации в Play без сохранения мира.
  Запуск — после source review codex и START/ACK. Прогон bench9 в Autotest_GameMode_Plain дошёл до `BENCH_DONE`
  после одного разрешённого владельцем «Yes» на запрос авторизации ванильного арсенала
  (`docs/evidence/receipts/live-bench-wb-claude-20261024.txt`).
  Необязательные параметры A0b (codex #4233):
  - `-riconBenchGameMode=SCR_BaseGameMode` (единственное допустимое значение) — создать в памяти один game mode
    по имени класса, если в мире его нет. **Экспериментально и в проверенном SlotFlatSmall неработоспособно**
    (A0b-1, `ddf8cd1`): SCRIPT (E) «missing RplComponent», Play не начался, модальное окно «World needs to be saved».
    Как готовый способ не предлагается;
  - `-riconBenchRadioManager=1` — создать одну `RadioManagerEntity` по имени класса и ожидать менеджер перед спавном;
  - `-riconBenchRadioManager=0` — ничего не создавать и ожидать, что менеджера нет.

  Значение вне списка (в том числе пустое) — `SETUP_REFUSED`. Уже существующий game mode отклоняется, только если
  запрошен `-riconBenchGameMode`. Уже существующий менеджер отклоняется в `SETUP_REFUSED`, только если запрошено
  создание (`=1`). При `=0` менеджер, найденный в мире, обнаруживается перед спавном: стенд пишет `BENCH_NOT_READY`.
  Стенд пишет наличие менеджера (`ChimeraWorld.GetRadioManager`) в `BENCH_START` и перед спавном (`BENCH_MANAGER`).
  Если оно расходится с явным значением, стенд пишет `BENCH_NOT_READY` и останавливается до спавна раций. Без
  параметра наличие менеджера только записывается.
  Необязательный параметр V2 (codex #4229, #4262) — метод шага частоты трансивера 0:
  - без параметра — прежний `BaseRadioComponent.SetTransceiverFrequency` (bench9, V1);
  - `-riconBenchSetter=component` — тот же метод, но выбранный явно;
  - `-riconBenchSetter=transceiver` — прямой `BaseTransceiver.SetFrequency`.

  - `-riconBenchSetter=entry` (A1-E, claude #4309, codex #4315, #4326) — путь меню VON на собственном ванильном
    объекте `SCR_VONEntryRadio`. Сразу после спавна стенд создаёт entry для трансивера 0 каждой своей рации
    (`SetRadioEntry(tr0, 1, gadget)`, номер 1-базный). Затем `SetChannelTextOverwrite` и `InitEntry`, запись
    `BENCH_ENTRY`. Шаг частоты — `AdjustEntryModif(±1)`. Mute — `ToggleMuteEntry` дважды, затем прямой `SetMuteState`
    туда и обратно с чтением `entry.GetIsMuted`. Чтения трансивера 0 дополняются `entryFreq`, `usable`, `emuted` и
    сырым текстом меню `dtext` в %XX-записи (`RICON_LiveText`). Уровень `MENU_ENTRY_RUNTIME`: экран, VON-контроллер
    игрока и ветка `SCR_VONMenu.GetKnownChannel` не исполняются. Последняя обходится channel-overwrite, потому что
    при менеджере фракций она разыменовывает `GetPlayerController()` без проверки (`a1-s0` квитанция).

  Другое значение, в том числе пустое, даёт `BENCH_NOT_READY reason=SETTER_PARAM` до спавна. Метод пишется в
  `BENCH_START` (`setter`, `setterGiven`) и в каждый `BENCH_SET_FREQUENCY` (`setter`). Остальные фазы не меняются.
  `BENCH_START` также пишет наличие контроллера игрока и менеджера фракций (`playerController`, `factionManager`).
- `Scripts/Game/RICON_Live/RICON_LiveText.c` — запись произвольной строки в поле `text` журнала без потерь: печатный
  ASCII 33…126, кроме `%`, `,` и `=`, остаётся как есть, остальное — `%XX` (`%uXXXX` для кодов вне 0…255, со счётчиком).
  Пробник `RICON_LiveTextProbePlugin.c` — 14 проверок. Обратимость проверяет внешний разбор
  `docs/evidence/receipts/live-text-log-check-claude.py`: он декодирует напечатанные формы и сравнивает байты.
- `Scripts/WorkbenchGame/RICON_LiveStreamProbePlugin.c` — потоковый режим: 1000 записей при ёмкости 8, 7 проверок;
  сами строки потока проверяет внешний разбор всего тегированного транскрипта
  `docs/evidence/receipts/live-stream-log-check-claude.py` (плагин не видит собственный вывод).
- `Scripts/Game/RICON_Live/RICON_LiveEndpointGame.c` и `RICON_LiveEndpointProbe.c` — B0a (codex, взаимно принят
  `dd0ada6`): наблюдатель роли процесса без сущности стенда. Modded `ArmaReforgerScripted` запускает его после
  `OnGameStart`, опрашивает после `OnUpdate`, останавливает в `OnGameEnd`/`OnBeforeWorldCleanup`. Свой журнал и прежний
  `RICON_LiveRadioReader`: 30 срезов `ENDPOINT_SAMPLE` (роль, rplMode, игрок, members), `ENDPOINT_BINDING` при смене
  привязки, `RADIO_STATE`, конец `ENDPOINT_DONE`/`ENDPOINT_ABORTED` и dump. Ни VoN-подписок, ни callqueue, ни Set*.
  Включается только `-riconEndpointProbe=1`; launcher с `-riconBenchMode=endpoint` не создаёт старый стенд.
  Необязательный тест `-riconEndpointRestart=1` (только вместе с двумя предыдущими): после ≥ 3 срезов Edit, ожидание
  устойчивого мира редактора ≥ 5 с, один второй Play с проверкой входа (маркеры `[RICON_LIVEB] restart=…`).
  Проверено только в Play одного Workbench (гейт выкл, гейт вкл, Play → Edit → Play); отдельный клиент, режим
  Server localhost и его адрес ещё исследуются (B0b), рабочей клиентской команды нет. Схема журнала и доказательства:
  [E-live-endpoint-bootstrap-codex.md](../../docs/evidence/E-live-endpoint-bootstrap-codex.md); разбор
  `docs/evidence/receipts/live-endpoint-log-check-claude.py <log> off|on|restart`.

## Пассивный двухролевой аудионаблюдатель (2026-11-02)

[Новый probe](<Scripts/Game/RICON_Live/RICON_LiveAudioProbe.c>) и [game lifecycle](<Scripts/Game/RICON_Live/RICON_LiveAudioGame.c>) включаются **только** `-riconAudioObserve=1`; [runner](<ops/wb-serverplay-claude.ps1>) добавляет этот параметр обеим ролям при `-AudioObserve`. Без флага создаваемого аудионаблюдателя нет. Это только подписки существующего radio-only observer, без SetCapture/PTT, PCM, изменений рации или direct.

Журнал использует отдельный `[RICON_AUDIO]`, не загрязняя `[RICON_LIVE]` endpoint-checker. Полный поток печатается по одному событию с возрастающим seq; остановка выводит счётчики кольцевого снимка и `subscriptionsRemaining`. `dropped` относится к снимку, не автоматически к потере потока. При урезанном stdout проверять полные исходные журналы.

До трактовки событий требуются `RUNNING` и `CLIENT_BINDING attached=1 subscriptions=2` каждой роли, у сервера также `SERVER_BINDING subscriptions=2`. Нет этих маркеров — `UNOBSERVED`. Переподключение опрашивается раз в секунду: переходы персонажа/ID неоднозначны до стабилизации. Полный binding рации/трансивера остаётся UNKNOWN; RX не доказывает слышимость.

[Чистый Workbench-пробник](<Scripts/WorkbenchGame/RICON_LiveAudioProbeCheckPlugin.c>) проверяет opt-in и раздельность журналов (10/10), не живые подписки. [Квитанция подготовки](<../../docs/evidence/receipts/audio-passive-preflight-20261102-lead.json>) сохраняет исправленный compile-отказ и финальный успешный лог. Дальнейшие gates — в [локальном аудиоплане](<../../docs/plan/LOCAL_AUDIO_VALIDATION_RU.md>).

## Запуск самопроверки (Workbench, без мира)

```
ArmaReforgerWorkbenchSteamDiag.exe -gproj "<repo>\tools\RICON_LiveProbe\addon.gproj"
  -addonsDir "<Steam>\Arma Reforger\addons,<Steam>\Arma Reforger Tools\Workbench\addons" -noBackend
  -wbModule=ScriptEditor -plugin=RICON_LiveJournalProbePlugin -wbSilent -exitAfterInit -noThrow
  -logLevel verbose -log-scr-checksum -logsDir ricon-livejournal-<stamp>
```

PASS — ровно одна строка `[RICON_LIVEJ] done: PASS`, `SCRIPT (E)` = 0 (строки «Resource leaks» и Steamworks
`RequestCurrentStats` — штатный шум). Exit 0 сам по себе ничего не доказывает.

Для наблюдателя в той же команде заменить плагин на `RICON_LiveObserverProbePlugin`, а каталог журнала — на новый
`ricon-liveobserver-<stamp>`. Требуются `checks=30 passed=30`, один `[RICON_LIVEO] done: PASS`, отсутствие
`mismatch` и `SCRIPT (E)`. Доказательства и ограничения: [E-live-observer.md](../../docs/evidence/E-live-observer.md).

## Подготовка опыта в Play

Рабочий сценарий пока не менялся. В отдельной копии тестового мира добавить ровно один `RICON_LiveProbeComponent`
на `SCR_BaseGameMode`. После запуска Play компонент создаёт ограниченный журнал и устанавливает доступные
подписки. `state=RUNNING` обозначает запуск прибора, а `state=CLIENT_BINDING … attached=1` — подключение к найденному
VoN-компоненту; ни то ни другое не подтверждает слышимость.

Компонент включает потоковый режим: каждое событие попадает в `console.log` сразу, поэтому покадровые
`TX_CAPTURE` не вытесняют из лога редкие `SPAWN`/`TX_SEEN` (буфер 256 — только снимок). Сохранность лога при аварии
процесса не заявляется. `Dump()` выводит снимок буфера в рамке BEGIN/END; `Stop()` отписывается и выводит снимок,
повторный вызов безопасен.
Для ручного управления в Script Console получить экземпляр через `FindComponent(RICON_LiveProbeComponent)`
на текущем GameMode и вызвать нужный публичный метод. Этот способ управления и выполнение компонента в Play
ещё требуют отдельного опыта. В течение сессии контролировать время по внешним часам: предел 900 секунд
в компоненте относится к игровому времени и не заменяет ограничение длительности опыта.

Опрос привязки раз в секунду оставляет ненаблюдаемое окно при появлении/смене персонажа. Ветка активного игрового
редактора, где ванильный контроллер использует иной VoN-компонент, не проверена. Нет сопоставления с epoch/linkId/
lease ядра, передачи журнала между клиентами, команды адресного mute или доступа к PCM.

## Правила журнала

- Теги не содержат слова FAIL (gate `regress_wb.ps1`); ключи шифрования не пишутся.
- Строки для разбора печатаются выражением или `PrintFormat`: `Print(локальная_строка)` в Enforce даёт отладочный вид
  `string имя = '…'` (найдено разбором лога 2026-10-24).
- Строки между `dump=BEGIN` и `dump=END` — снимок, их `seq` повторяют `seq` потока.
- Времена — источника события (сторона указана), общей аудиошкалы нет; разрешение не заявляется тоньше каденции
  native-событий.
- `TX_CAPTURE` — колбэк `OnCapture` каждый кадр записи; `TX_START`/`TX_STOP` — только после отдельной детекции фронта.
- `TX_SEEN` сервера не сообщает тип передачи: `channel=UNKNOWN`, даже при известном говорящем.
- В `RX_EVENT` поле `player` — говорящий, `listener` в тексте — локальный слушатель; неизвестные ID равны −1.
  `q` печатается с девятью знаками после запятой, без округления к промилле; это формат вывода, не аудиошкала.
- Частота сохраняется как сырое значение движка без установленной единицы. `device`/`tr` пока −1.
- Отсутствие события не означает тишину. Прибор не вызывает актуаторы звука, не меняет рации и пользовательский mute.
