# Заказ live-проверки: единица `GetFrequency` и семантика `SetMuteState`

Дата: 2026-10-23. Автор: claude. Статус: **DRAFT — на согласование с codex, затем `question [Решение]` владельцу с
точной командой запуска**. Основание: разрешение владельца 2026-10-23 («Разрешаю всё»: запуск игры и локального
сервера для проверки актуатора H2 `SetMuteState` и единицы частоты рации). План без согласования codex не исполняется;
каждый запуск — START → ACK → run → END на шине, как слот Workbench.

## 1. Что известно из репозитория (факты, не предположения)

- В коде **нет ни одного вызова `SetMuteState`**. «H2» — гипотеза `PLAN_RU.md:136` («действует на весь RX, точна
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
- Play-пробников нет: все плагины — `WorkbenchPlugin` ScriptEditor без мира; аддоны не содержат мира, префабов и
  GameMode; `RICON_CommsManagerComponent` существует в Play только если прикреплён к GameMode.
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

Хук без правки миров и префабов: `modded class` ванильного класса, доступного в любом сценарии (кандидаты —
`SCR_BaseGameMode.OnPlayerSpawned` на authority и `SCR_PlayerController` на клиенте; точный выбор — после чтения
ванильных сигнатур через `RICON_WbReader`, не наугад). Прибор по таймеру (разрешён в этом аддоне) раз в 1 с находит
рации локального игрока и пишет в обычный лог игры строки с префиксом `[RICON_LIVE]`:
`side=server|client build=<ver> t=<ms> player=<id> device=<id> transceiver=<idx> raw=<int> min=<int> max=<int>
res=<int> powered=<bool> muted=<bool>`. Для Q-M — отдельная команда (консоль/клавиша прибора): `mute on|off` →
строка `[RICON_LIVE] mute call=<bool> ret=<bool> muted=<bool>`. Ключ шифрования не логируется никогда
(`REQ-radio-device-adapter.md:52`). Теги не содержат слово FAIL (gate `regress_wb`).

## 4. Среда запуска

**Фаза A (один ПК, достаточна для Q-F и локальной части Q-M).** Workbench World Editor → ванильный мир → Play:
процесс один, authority локальный, без BattlEye и публичного сервера. Сценарий и рация: одна известная модель с
одним каналом (предложение — ANPRC-68 или R-148 по таблице `E-radio-data-wb.md:50–63`); точный префаб и мир
согласуются с codex. Не более 15 минут на сессию. Команда запуска публикуется в `question [Решение]` до старта.

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

## 7. Открытые вопросы к codex до публикации заказа

1. Хук прибора: `modded class` какого класса; подтвердить сигнатуры через `RICON_WbReader` (≤ 15 строк за запуск).
2. Мир и рация фазы A; где лежит `console.log` игры при Play из Workbench (путь не задокументирован).
3. Достаточно ли фазы A для снятия runtime-gate единицы частоты в аудите (`AUDIT-bands-summary.md` §4 п. 5) при
   условии фиксации версии игры.
4. Формулировка `question [Решение]` владельцу: фаза B и установка dedicated server.
