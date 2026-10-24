# Заказ live-проверки: единица `GetFrequency` и семантика `SetMuteState`

Дата: 2026-10-23, обновлено 2026-10-24. Автор плана: claude; сверка и обновление готовности: codex. Статус: **план
согласован; фаза A0 выполнена частично** (§4): стенд прошёл в Play трижды (bench9, V1, V2), только на сервере,
уровень `NATIVE_ROUNDTRIP`. В V2 прямой `BaseTransceiver.SetFrequency` впервые дал `NATIVE_ROUNDTRIP_PASS`. Q-F и Q-M
по критериям §2 не закрыты. Основание: разрешение владельца 2026-10-23
(«Разрешаю всё»: запуск игры и локального сервера для проверки актуатора H2 `SetMuteState` и единицы частоты рации).
Повторное разрешение на уже согласованный опыт не требуется. Обычно запуск — START → ACK → run → END на шине;
по owner #4011 Codex продолжает без ожидания Claude, публикует START/END и оставляет окончательное ревью открытым.

**Готовность на 2026-10-24:**
- журнал и наблюдатель прошли 30/30 проверок в Workbench без мира;
- чтение раций (`RICON_LiveRadioReader`, 16/16) и стенд A0 реализованы;
- стенд исполнен в Play трижды: bench9, V1 с `RadioManagerEntity`, V2 с прямым `SetFrequency` (§4).
Компонент запуска на GameMode в Play не исполнялся. Сопоставления epoch/linkId/lease нет.
[Доказательства](../evidence/E-live-observer.md), [инструкция компонента](../../tools/RICON_LiveProbe/README.md),
`receipts/live-bench-wb-claude-20261024.txt`. Q-F/Q-M остаются открытыми.

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
    килогерцами.
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

**Фаза B (сервер + клиент; два участника).** Dedicated server не установлен; player-hosted сервер требует входа в
backend. Открыт вопрос владельцу: установить «Arma Reforger Server» (бесплатно в Steam) и допускается ли второй
клиент/участник. До ответа фаза B не планируется.

## 5. Журнал и доказательства

- Журнал «ожидалось / наблюдалось» по каждому шагу: что показано в UI (МГц), что в логе (raw), время.
- Receipts: полный `console.log` сессии (SHA-256) и выжимка `[RICON_LIVE]`; версия игры; команда запуска; список
  включённых аддонов. Положительный контроль: перестройка на известный шаг меняет raw на известную величину.
- Независимая проверка codex по тем же логам (свой парсер).
- После PASS меняются статусы: единица в `REQ-radio-device-adapter.md` (UNKNOWN → RUNTIME_OBSERVED с версией),
  Q3 «права SetMuteState» (частично), `STATUS_RU.md` Э1r (часть A); `BLOCKED_UNVERIFIED` фазы B остаётся.

## 6. Вне охвата

`SocialComponent.SetMuted` (мут игрока), `TransmissionQuality` (H3), ретранслятор/интерком, три клиента,
измерения качества связи по `WB-order-link-quality.md`.

## 7. Следующие шаги

1. V1 и V2 выполнены и взаимно приняты (§4): обёртка `SetTransceiverFrequency` на сервере не видна, прямой
   `SetFrequency` виден. Production-адаптер остаётся читателем. Перенос записи частоты требует отдельного решения о
   контракте и репликации (codex #4282, #4285), автоматически не делается.
2. Q-F против UI МГц и Q-M против меню VON требуют игрока, это фаза A с игроком; стенд A0 их не закрывает.
3. Подписки уже реализованы без `modded class`; проверить размещение компонента, Start/Stop и смену персонажа в Play.
4. Для фазы B определить доступных участников и среду сервера; для точечного A→B при одновременно C→B нужны три роли.
5. После возвращения Claude провести независимое ревью самостоятельного среза и его журналов.
