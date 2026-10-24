# Заказ live-проверки: единица `GetFrequency` и семантика `SetMuteState`

Дата: 2026-10-23, обновлено 2026-10-24. Автор плана: claude; сверка и обновление готовности: codex. Статус: **план
согласован; A0, путь меню игрока и экран выполнены** (§4). В Play одного Workbench-процесса (isServer=1,
isClient=0): V2 — прямой `BaseTransceiver.SetFrequency` (`NATIVE_ROUNDTRIP_PASS`); A1-E — свой ванильный объект меню
(`MENU_ENTRY_RUNTIME_PASS`); A1-P0/P1 — локальный игрок есть, две его entry — трансиверы одной AN/PRC-68, текст меню
= raw/1000; A1-P2 — путь меню на его рации (`PLAYER_MENU_RUNTIME_PASS`, всё восстановлено); A1-S1 — прямой `Open()`
радиального меню упал без controller inputs (`LOG_INVALID`); A1-S1c — меню открыто штатным config-контроллером,
на экране «32.0 MHz»/«48.0 MHz»; A1-S2 — шаг частоты на экране 48.0 → 48.2 → 48.0 MHz и production-reader с тем же
raw. **Q-F: raw — кГц, RUNTIME_OBSERVED** для ванили 1.8.0.13 и пути AN/PRC-68 (A1-S2 на независимом ревью codex).
Q-M против меню VON на экране не закрыт.
Основание: разрешение владельца 2026-10-23 («Разрешаю всё»: запуск игры и локального сервера для проверки актуатора
H2 `SetMuteState` и единицы частоты рации) и **постоянное разрешение 2026-10-24** («Разрешены все необходимые
действия сейчас и в будущем», ответ на agora #4353, тот же ответ у codex #4357). По нему нужные запуски и одно нажатие
«Yes» на тот же ванильный запрос авторизации в каждом процессе больше не требуют отдельного вопроса. Контроль действия
сохраняется: один оператор, START → ACK → run → END, guard окна (точный текст, 4 кнопки, ровно одно нажатие, никогда
«Yes to All» и настройки безопасности), неизвестное окно — стоп, снимок реестра только хэшами и счётчиками.
Вне охвата остаются push, удалённые репозитории, переписывание истории, публикация и действия вне проекта.
По owner #4011 Codex продолжает без ожидания Claude, публикует START/END и оставляет окончательное ревью открытым.

**Готовность на 2026-10-24:**
- журнал и наблюдатель прошли 30/30 проверок в Workbench без мира;
- чтение раций (`RICON_LiveRadioReader`, 16/16) и стенд A0 реализованы;
- стенд исполнен в Play: bench9, V1 с `RadioManagerEntity`, V2 с прямым `SetFrequency`, A1-E с собственной
  ванильной entry меню; режимы игрока A1-P0, A1-P1, A1-P2 приняты взаимно; A1-S1 и A1-S1b — отрицательные,
  A1-S1c принят взаимно, A1-S2 — на ревью (§4).
Компонент запуска на GameMode в Play не исполнялся. Сопоставления epoch/linkId/lease нет.
[Доказательства](../evidence/E-live-observer.md), [инструкция компонента](../../tools/RICON_LiveProbe/README.md),
`receipts/live-bench-wb-claude-20261024.txt`. Q-F закрыт на экранном уровне (после ревью A1-S2), Q-M открыт.

## 1. Что известно из репозитория (факты, не предположения)

- В production-коде (`RICON_Core`) **нет ни одного вызова `SetMuteState`**. Единственный вызов — TOOL_ONLY стенд
  `RICON_LiveRadioBenchComponent` (§4), только на собственных рациях. «H2» — гипотеза `PLAN_RU.md:136` («действует на весь RX, точна
  только при одном активном источнике…»). Шов `RICON_VoNIntegration` (`RICON_NetworkContracts.c:347–387`) намеренно
  возвращает `BLOCKED_NO_ACTUATOR`; тест CT-VON-S-010 (`test_von_seam_scan.py:15–17`) запрещает в этом файле
  `BaseTransceiver`, `BaseRadioComponent`, `VoNComponent`, `Rpl*`, `GetGame`, `FileIO`, `CallLater`.
- Частота читается в одном месте: `RICON_RadioDeviceAdapter.c:192` `transceiver.GetFrequency()` → `m_iTuningValue`
  («непрозрачное native значение; единица не установлена», `:25`), только на сервере (`IsAuthority`, `:178`);
  единственный вызов — тест с `null` (`RICON_RadioDeviceFixtureSupport.c:359–368`). Деления на 1000 нигде нет
  (`REQ-radio-device-adapter.md:38–39, :163`). Единица объявлена UNKNOWN (`REQ-radio-device-adapter.md:33–34`,
  `E-radio-api-s1.md:13`, `STATUS_RU.md:95`, `AUDIT-bands-summary.md:30`).
- Косвенные указания на кГц (inference, не доказательство): значения префабов 32000…76000
  (`E-workbench-source.md:224–225`), `SetFrequency(int)` «кГц» (`E-radio-api-s1.md:12`), учебник
  `DESIRED_FREQUENCY = 60000` и подпись UI `#AR-VON_FrequencyUnits_MHz` (`E-radio-gameplay-doc.md:22–26`),
  `BeginTransmissionFreq` «явно кГц» (`LINK_CHAIN_RU.md:73`). Getter трансивера задержан
  («transceiver getter is delayed (network?)», `SCR_VONEntryRadio.c:31` по `E-workbench-source.md:149–151`).
- **Ванильный путь меню VoN (A1 S0/S0b, 2026-10-24, WORKBENCH_STATIC,** `receipts/a1-s0-wbreader-claude-20261024.txt`**).**
  - Перестройка в `Game/VON/SCR_VONEntryRadio.c:118–129`: шаг = modifier × `GetFrequencyResolution()`, ограничение
    [min, max], затем прямой `BaseTransceiver.SetFrequency` (`:129`, второй вызов `:204`).
  - `BaseRadioComponent.SetTransceiverFrequency` ванильные скрипты не вызывают. Во всех 5809 файлах он есть только в
    объявлении `Game/generated/Components/BaseRadioComponent.c:54`. Значит, V2 шёл тем же native-методом, что меню,
    а bench9/V1 — методом, которого ваниль не использует.
  - Текст частоты в меню: `:131–132` — raw/1000 с округлением до 0.01, `ToString(3, 1)` и подпись
    `#AR-VON_FrequencyUnits_MHz`. Второй путь `:206–208` — тот же делитель. Ванильный форматтер прямо считает raw
    килогерцами. Это отдельный факт уровня WORKBENCH_STATIC, не наблюдение экрана. Единица в production-контракте
    (`REQ-radio-device-adapter.md`) остаётся UNKNOWN (codex #4316).
  - При raw, не кратном 10, два пути могут разойтись на 0.01 до форматирования: raw 39949 → 39.95 и 39.94.
    Совпадёт ли текст с одним знаком, зависит от округления `ToString(3, 1)` и не установлено. У raw, кратных 100
    (все V2), оба пути дают одно число.
  - Объект меню (S0c–S0e): `GetDisplayText` возвращает этот текст (`SCR_VONEntry.c:77–79`). `SetRadioEntry` только
    присваивает поля. `InitEntry` вызывает `AdjustEntryModif(0)`. Номер трансивера entry 1-базный
    (`GetIsMuted` `:255–259` читает `IsMuted` трансивера «номер − 1»). `ToggleMuteEntry` инвертирует mute трансивера.
  - Риск null: при пустом channel-overwrite `InitEntry` вызывает `SCR_VONMenu.GetKnownChannel` (`SCR_VONMenu.c:41–46`).
    Тот при наличии менеджера фракций разыменовывает `GetGame().GetPlayerController()` без проверки. Есть ли
    контроллер в Plain Play, не измерялось (codex #4327).
- LiveProbe содержит компонент запуска для Play (скомпилирован, в Play не исполнялся) и стенд A0. Стенд исполнялся,
  но миры на диске не менялись: сущности создаются только в памяти World Editor. `RICON_LiveProbeComponent` и
  `RICON_CommsManagerComponent` требуют явного размещения на GameMode.
- Dedicated server («Arma Reforger Server», отдельное приложение Steam) на ПК **не установлен**; установлены
  `Arma Reforger` и `Arma Reforger Tools`.
- Протокол START/ACK/END в репозитории не записан; ближайшее — `ROADMAP_RU.md:75` и заказ
  `WB-order-link-quality.md` (R20/R23/R26, запуск игры не разрешал).

## 2. Два вопроса и критерии ответа

**Q-F. Единица `BaseTransceiver.GetFrequency()`.** Для известной рации и известной частоты в UI (МГц) записать
сырой int. Правило решения (сборка игры фиксируется в журнале):
- `raw == round(UI_MHz · 1000)` на всех точках → **кГц, RUNTIME_OBSERVED**;
- `raw == round(UI_MHz)` → МГц;
- иначе → UNKNOWN, значения сохраняются, вывод не делается.
Контроли: (а) сравнить с `ChannelFrequency` префаба (например 32000/48000 у ANPRC-68, 38000 у R-148);
(б) `GetMinFrequency`, `GetMaxFrequency`, `GetFrequencyResolution` рядом с raw; (в) перестройка через меню VON на
±1 шаг и повторное чтение в t+0, +1, +5 с (задержанный getter); (г) минимум две настройки, отличающиеся на один шаг.
Отдельно для production-читателя: `RICON_BaseRadioObservationReader.TryRead` с настоящей рацией на стороне
authority — первый вызов не с `null`; результат `m_iTuningValue` должен совпасть с raw.

**Q-M. `SetMuteState(bool)` в пределах одного ПК.** Что можно установить одним процессом-authority:
- вызываемость и возвращаемое bool; `IsMuted()` до и после;
- сохранение после перестройки частоты, выключения/включения питания, смены трансивера;
- взаимодействие с мутом меню VON (`ToggleMuteEntry`/`VONMenuMuteAction`): влияет ли одно на другое;
- восстановление только собственного состояния: опыты — на изолированной тестовой рации, а не на рации игрока с
  возможным пользовательским mute; автоматического `SetMuteState(false)` «в конце» нет — владение состояния по
  bool-API не выводится (`AUDIO_CHAIN_CONTROL_RU.md` §2.5, OWNERSHIP_UNKNOWN; codex #3937): снимаем лишь то, что
  поставили сами, по журналу своих вызовов — и этого достаточно только для изолированного экземпляра под
  исключительным управлением прибора: никакой другой писатель (включая меню VON игрока) к нему не допускается
  (codex #3949).
Что **не** устанавливается одним ПК и остаётся `BLOCKED_UNVERIFIED` (фаза B): направление RX/TX, слышимый эффект,
репликация на другую сторону, права вызова с клиента-прокси, фальсификатор H2 «A→B LOST и C→B CLEAN одновременно»
(`PLAN_RU.md:363`, нужны A, B, C).

## 3. Прибор

Отдельный TOOL_ONLY аддон `tools/RICON_LiveProbe/` (собственный `addon.gproj`, зависимость только от игры; в
`RICON_Core` и `RICON_CoreTests` ничего не добавляется; CT-VON-S-010 не затрагивается). Подключается только на время
сессии через `-addonsDir`; после END не используется и не публикуется.

Подписки без `modded class` — штатные invokers, подтверждённые чтением исходников
(`E-von-windows-ea.md`): `SCR_BaseGameMode.GetOnPlayerSpawned()` (authority, жизненный цикл),
`SCR_VoNComponent.GetOnVoNUsed()` (сервер, «кто говорит»), `m_OnCaptureVON.Invoke(transmitter)` и
`m_OnReceivedVON.Invoke(playerId, receiver, frequency, quality)` на компоненте локального игрока (клиент).
Для запуска подписок компонент размещается в отдельной копии тестового мира; текущие игровые миры не менялись.
Ванильное меню VON вызывает `BaseTransceiver.SetMuteState` с клиента (`SCR_VONEntryRadio.c:240–242`) — вызов с
клиента штатен, эффект/репликация — runtime. Чтение раций и стенд реализованы. Фактический формат строк —
`[RICON_LIVE] seq=… t=… side=… kind=… player=… device=… tr=… value=… text=…` (README прибора). Ниже — исходный
замысел для фазы A с игроком; эта часть пока не реализована: раз в 1 с находить
рации локального игрока и писать в обычный лог игры строки с префиксом `[RICON_LIVE]`:
`side=server|client build=<ver> t=<ms> player=<id> device=<id> transceiver=<idx> raw=<int> min=<int> max=<int>
res=<int> powered=<bool> muted=<bool>`. Для Q-M — отдельная команда (консоль/клавиша прибора): `mute on|off` →
строка `[RICON_LIVE] mute call=<bool> ret=<bool> muted=<bool>`. Ключ шифрования не логируется никогда
(`REQ-radio-device-adapter.md:52`). Теги не содержат слово FAIL (gate `regress_wb`).

## 4. Среда запуска

**Фаза A (один ПК, достаточна для Q-F и локальной части Q-M).** Workbench World Editor → ванильный мир → Play:
процесс один, authority локальный, без BattlEye и публичного сервера. Сценарий и рация: одна известная модель с
одним каналом (предложение — ANPRC-68 или R-148 по таблице `E-radio-data-wb.md:50–63`); точный префаб и мир
фиксируются до старта. Не более 15 минут на сессию. Точная команда и выбранные ресурсы публикуются в START;
уже выданное разрешение сохраняется.

**Фаза A0 — стенд изолированных раций (вариант (а′), claude #4089, условия codex #4091; source review до запуска).**
Плагин `RICON_LiveBenchLauncherPlugin` (World Editor) открывает ванильный тестовый мир с игровым режимом
`{3048828FE14AE687}worlds/Tests/TestFramework/Autotest_GameMode_Plain.ent` (мир из параметра `-riconBenchWorld`).
В памяти он создаёт одну GenericEntity с `RICON_LiveRadioBenchComponent` в проверенной точке внутри границ мира и
входит в Play. Save не вызывается, файлы мира не меняются. SlotFlatSmall — только история неудачных попыток (ниже).
Стенд спавнит собственные экземпляры ANPRC68, ANPRC77, R107M, RF10 по точным ResourceName (получены
`RICON_LiveResourceProbePlugin`, `receipts/live-radio-wb-claude-20261024.txt`), читает сырые частоту/границы/шаг,
меняет частоту трансивера 0 только своей рации на один шаг (`BaseRadioComponent.SetTransceiverFrequency`) с чтением
t+0/+1/+5 с, переключает `SetMuteState` на противоположное состояние и возвращает исходное только на ней, удаляет свои рации. Игроков,
меню VON и пользовательских раций нет — это изолированный экземпляр под исключительным управлением (§2 Q-M).
Уровень результата — NATIVE_ROUNDTRIP: закрывает поведение getter/setter на этой сборке, но не UI_MHz, направление
RX/TX, слышимость, репликацию и per-pair. Процесс завершается внешним таймаутом; журнал потоковый.

**Факт исполнения A0 (2026-10-24, `receipts/live-bench-wb-claude-20261024.txt`).**
- **SlotFlatSmall.** Мир без game mode, в Play не вошёл. Стенд перенесён в `{3048828FE14AE687}worlds/Tests/TestFramework/Autotest_GameMode_Plain.ent`
  (SCR_BaseGameMode из GameMode_Plain.et).
- **Запрос авторизации.** Там Play упирался в модальный запрос Workbench «Script Authorization Required» для ванильного
  `SCR_ArsenalManagerComponent.c:1101` → `BaseContainerTools.LoadContainer`.
- **bench9.** После одного разрешённого владельцем «Yes» дошёл до `BENCH_DONE`. Роль — только сервер.
  - Q-M: `SetMuteState` переключает `IsMuted` и возвращает исходное на всех четырёх рациях, хотя возвращает 0.
  - Q-F: `SetTransceiverFrequency` на один шаг не виден в `GetFrequency` через 0/1/5 с.
  - Движок предупредил: в мире нет `RadioManagerEntity`. Вики BI называет его условием радиосвязи; связь с Q-F — гипотеза.
  - Взаимное ревью: claude `556aa30`, codex `07d8224`.
- **A0b-1 (ddf8cd1).** Пара в SlotFlatSmall с голым `SCR_BaseGameMode` по имени класса. NOT_READY: SCRIPT (E)
  «missing RplComponent», Play не начался, модальное окно «World needs to be saved». A0b-2 не запускался.
- **V1 (2026-10-24).** Тот же Plain-мир, `-riconBenchRadioManager=1`: одна `RadioManagerEntity` по имени класса в
  памяти, один «Yes» под тем же контролем по решению владельца «(A)» (agora #4256). Менеджер подтверждён в
  `BENCH_START` и перед спавном, предупреждения движка о нём нет. Q-F по-прежнему `OBSERVED_MISMATCH`: шаг
  `SetTransceiverFrequency` в `GetFrequency` не виден через 0/1/5 с. Q-M — переключение и возврат, как в bench9. Без
  строк менеджера события совпадают с bench9. Это исторический контраст (разные версии диагностики), а не строгая
  причинность. Гипотеза о менеджере для серверного экземпляра ослаблена, но не опровергнута для клиента, UI и репликации.
- **V2 (2026-10-24).** V1 плюс `-riconBenchSetter=transceiver`: шаг частоты трансивера 0 прямым
  `BaseTransceiver.SetFrequency`. Один «Yes» по решению владельца «(A)» (agora #4278). Результат —
  `NATIVE_ROUNDTRIP_PASS` на всех четырёх рациях: новый шаг виден в `GetFrequency` в том же кадре (t0) и на t1/t5,
  трансивер 1 не изменился, mute — переключение и возврат. Против V1 различаются только строка START, шаги и чтения
  трансивера 0. На этом серверном экземпляре прямой setter виден, обёртка `SetTransceiverFrequency` — нет; причина не
  установлена, клиентский путь не проверен. Положительный контроль §5 на нативном уровне выполнен (шаг res = 200
  сырых единиц меняет raw на 200). Единица относительно UI МГц (Q-F) остаётся открытой.
  Взаимное ревью: claude `41904a4`, codex `2254be0` (#4284, воспроизведение #4286). Узкий серверный срез A0 закрыт.
- **A1-E (подготовлен, не запущен; claude #4309, условия codex #4315, #4319, #4326).** Тот же Plain-мир и флаги, что
  у V2, но `-riconBenchSetter=entry`. Для трансивера 0 каждой своей рации стенд создаёт собственную ванильную
  `SCR_VONEntryRadio` и выполняет путь меню на настоящем трансивере:
  - `SetRadioEntry(tr0, 1, gadget)` → `SetChannelTextOverwrite` (явная настройка своей entry) → `InitEntry`;
  - шаг `AdjustEntryModif(±1)`, чтения t0/t1/t5 raw, `GetEntryFrequency` и `GetDisplayText`;
  - mute: `ToggleMuteEntry` дважды, затем прямой `SetMuteState` туда и обратно с чтением `entry.GetIsMuted`.
  Текст меню пишется без потерь (%XX, `RICON_LiveText`, 14 native-проверок до Play). Критерий Q-F на этом уровне:
  число текста × 1000 = raw в каждой точке с raw, кратным 100; иначе `TEXT_ROUNDED` без вердикта. Уровень
  `MENU_ENTRY_RUNTIME`: экран, VON-контроллер игрока и ветка `GetKnownChannel` не исполняются, Q-F RUNTIME_OBSERVED
  этим не закрывается. Разбор `live-bench-log-check-claude.py … 1 entry`: строгий общий кодек, пять обязательных
  точек текста, `LOG_INVALID` при ошибке лога; контроли 36/36 (ревью codex #4338, #4339, #4343; agree `3bc76fa`).
  **Выполнен 2026-10-24** (постоянное разрешение владельца, START #4356, ACK #4358, END #4359; лог
  `ricon-a1e-claude-20261024-192824`, SHA df685d35…; `receipts/live-bench-wb-claude-20261024.txt`,
  `receipts/live-a1e-log-check-claude-20261024.txt`). Один «Yes» под тем же guard, других окон нет, `SCRIPT (E)` = 0,
  реестр и профиль не изменились. Итог — **`MENU_ENTRY_RUNTIME_PASS` на всех четырёх рациях**:
  - шаг меню `AdjustEntryModif(+1)` виден в `GetFrequency` на t0/t1/t5 (32000→32200, 48000→48200, 48000→48200,
    42000→42200), `GetEntryFrequency` = цель, трансивер 1 не изменился;
  - строка ванильного объекта: «32.0 #AR-VON_FrequencyUnits_MHz» → «32.2 …», 48.0 → 48.2, 48.0 → 48.2, 42.0 → 42.2;
    число × 1000 = raw во всех 20 точках (init/step/t0/t1/t5);
  - mute: `ToggleMuteEntry` 0 → 1 → 0 (возврат = новое состояние), прямой `SetMuteState` 0 → 1 → 0, `entry.GetIsMuted`
    следует обоим. Mute меню и прямой mute — одно native-состояние трансивера.
  - В Play на этом экземпляре нет `PlayerController`, но есть менеджер фракций. Без channel-overwrite `InitEntry`
    дошёл бы до непроверенного разыменования в `GetKnownChannel`.
  Что это закрывает: формула и путь штатного меню исполняются на настоящих рациях и дают «raw / 1000 MHz» и общий
  mute. Что не закрывает: экран и локализованный текст у игрока, VON-контроллер игрока, звук, RX/TX, репликацию.
- **A1-P0 (2026-10-24, только наблюдение; claude #4360, условия codex #4365, #4372, #4379; START #4380, END #4383).**
  Режим стенда `-riconBenchMode=player`: ни спавна, ни Set*, ни контроля, 30 срезов не чаще раза в секунду игрового
  времени. Лог `ricon-a1p0-claude-20261024-195517` (SHA 9f3ad5d0…), разбор `receipts/live-a1p0-log-check-claude-20261024.txt`:
  `LOCAL_PLAYER_PRESENT` во всех 30 срезах (1013…30193 мс). Состояние: isServer=1, isClient=0, один игрок, локальный
  `PlayerController` с playerId=1, управляемая сущность `Character_US_Rifleman` с `SCR_VoNComponent` и GadgetManager,
  `SCR_VONController` с 2 entry. Значит, Play, который запускает launcher, сам создаёт локального игрока в первую
  секунду; `playerController=0` в BENCH_START — только момент EOnInit. Codex независимо подтвердил (#4387).
  Следующий шаг A1-P1 — чтение этих entry и их точной связи с рациями персонажа. A1-P2 — путь меню на entry рации
  игрока (условия codex #4388: только RadioEntry, исходное снимается, перед возвратом проверяется своё состояние,
  иначе STOP).
- **A1-P1 (2026-10-24, только чтение; START #4392, END #4398; codex `7ed19b7`).** Обе entry VON-контроллера игрока —
  `SCR_VONEntryRadio` трансиверов 1 и 2 одной его AN/PRC-68 (связь entry ↔ трансивер ↔ рация в инвентаре MAPPED).
  Трансивер 1: 32000 → «32.0 MHz», выбран для передачи. Трансивер 2: 48000 → «48.0 MHz». Состояние не менялось все
  30 срезов; отдельной entry прямой речи в перечислении нет (это не доказывает, что прямой речи нет).
- **A1-P2 (2026-10-24; START #4416, END #4418; codex `199f27c`).** На неактивной entry трансивера 2 ванильный путь
  меню `AdjustEntryModif(+1)`: 48000 → 48200, «48.2 MHz» на t0/t1/t5, возврат точно в 48000; `ToggleMuteEntry`
  0 → 1 → 0 с возвратом нового состояния. Перед каждой записью — полная проверка привязки (игрок, entry, трансивер,
  рация, слот). Активная entry не тронута, entry после опыта равны исходным: `PLAYER_MENU_RUNTIME_PASS`.
  Статически (`receipts/a1-screen-wbreader-claude-20261024.txt`) клавиша настройки ведёт к тому же
  `AdjustEntryModif(±1)`, а mute из меню — к `ToggleMuteEntry`.
- **A1-S1 (2026-10-24; START #4442, END #4445) — отрицательный.** Прямой `SCR_SelectionMenu.Open()` радиального меню
  игрока упал в ванильном `SCR_SelectionMenu.c:244` (NULL controller inputs). Inputs и display меню получает только
  в штатном `SCR_VONMenu.OnInputOpenMenu` → `m_RadialController.Control(...)` (`SCR_VONMenu.c:159–166`). Экранный
  виджет не прочитан. Стенд больше не открывает меню без display и inputs (NOT_READY). Следующий шаг — штатный вход
  `SCR_RadialMenuController.OnInputOpen` (по действию ввода), без самодельных inputs и без Close/Perform.
- **A1-S1b (2026-10-24; START #4472) — без экрана из-за моста.** Modded `SCR_VONMenu` без `[BaseContainerProps()]`
  сломал загрузку атрибута `m_VONMenu` (WORLD (E) Unknown keyword); меню VoN не создано, стенд остановился на срезе 5.
- **A1-S1c (2026-10-24; START #4482; codex `6146b66`).** Мост повторяет атрибут класса ванили. Один
  `OnInputOpen` собственного config-контроллера меню (действие `VONMenu`, CloseOnRelease 0, PerformOnClose 0): меню
  открыто, `FrequencyText` видимы — «32.0 MHz» и «48.0 MHz» для raw 32000/48000, снимок окна совпадает. Радиосостояние
  не менялось. Разбор — `PLAYER_SCREEN_WIDGET_LABEL_UNVERIFIED` (подпись «MHz» — не проверенный перевод ключа).
- **A1-S2 (2026-10-24; START #4507, END #4512).** При открытом так же меню — шаг A1-P2 на неактивной entry 1:
  `AdjustEntryModif(+1)`, проверка target и только затем `entry.Update()` (обновление представления, как в штатном
  `OnAdjustEntry`; не физическая клавиша), READ t1/t5, возврат с проверкой raw0. Перед шагом и возвратом — то же
  открытое меню. `FrequencyText` entry 1: «48.0 MHz» → «48.2 MHz» → «48.0 MHz». Три снимка окна PRE/POST/BACK строго до
  STEP, между STEP и RESTORE и после RESTORE (по времени строк журнала). Production-reader `TryRead` (authority) на тех
  же точках: `m_iTuningValue` = raw = 48000 / 48200 / 48000. Mute не трогался. Разбор —
  `PLAYER_SCREEN_STEP_LABEL_UNVERIFIED` по тому же правилу подписи. Все критерии Q-F §2 выполнены: (а) 32000/48000 у
  AN/PRC-68, (б) min/max/res 32000/68000/200, (в) шаг через путь меню с t0/t1/t5, (г) две настройки на шаг,
  production-reader совпал с raw.

**Фаза B (сервер + клиент; два участника).** Dedicated server не установлен; player-hosted сервер требует входа в
backend. Постоянное разрешение владельца 2026-10-24 покрывает необходимые локальные действия, отдельный вопрос не
нужен (codex #4500). Codex нашёл установленные `ArmaReforgerSteamDiag.exe` и PeerTool/PeerConfig в API Workbench
(#4508) — второй локальный endpoint исследуется отдельным срезом. Наличие второго голосового клиента или источника
остаётся технической неопределённостью; один флаг не даёт AUDIO_PASS.

## 5. Журнал и доказательства

- Журнал «ожидалось / наблюдалось» по каждому шагу: что показано в UI (МГц), что в логе (raw), время.
- Receipts: полный `console.log` сессии (SHA-256) и выжимка `[RICON_LIVE]`; версия игры; команда запуска; список
  включённых аддонов. Положительный контроль: перестройка на известный шаг меняет raw на известную величину.
- Независимая проверка codex по тем же логам (свой парсер).
- После PASS меняются статусы: Q3 «права SetMuteState» (частично), `STATUS_RU.md` Э1r (часть A);
  `BLOCKED_UNVERIFIED` фазы B остаётся. Общий контракт `REQ-radio-device-adapter.md` v0.2 заморожен: `m_iTuningValue`
  остаётся непрозрачным, RF-профиль — явным; единица кГц для текущей сборки и AN/PRC-68 оформляется отдельным
  evidence-уточнением, без скрытого `/1000` в общем builder (codex #4494).

## 6. Вне охвата

`SocialComponent.SetMuted` (мут игрока), `TransmissionQuality` (H3), ретранслятор/интерком, три клиента,
измерения качества связи по `WB-order-link-quality.md`.

## 7. Следующие шаги

1. V1 и V2 выполнены и взаимно приняты (§4): обёртка `SetTransceiverFrequency` на сервере не видна, прямой
   `SetFrequency` виден. Production-адаптер остаётся читателем. Перенос записи частоты требует отдельного решения о
   контракте и репликации (codex #4282, #4285), автоматически не делается.
2. Q-F против UI МГц выполнен с игроком на экране (A1-S2, §4; после независимого ревью codex — evidence-уточнение
   единицы для 1.8.0.13 и AN/PRC-68). Q-M против меню VON на экране остаётся: значок mute и `ToggleMuteEntry` при
   открытом меню, взаимодействие с `SetMuteState` — только на изолированной тестовой рации (§2).
3. Подписки уже реализованы без `modded class`; проверить размещение компонента, Start/Stop и смену персонажа в Play.
4. Для фазы B определить доступных участников и среду сервера; для точечного A→B при одновременно C→B нужны три роли.
5. После возвращения Claude провести независимое ревью самостоятельного среза и его журналов.
