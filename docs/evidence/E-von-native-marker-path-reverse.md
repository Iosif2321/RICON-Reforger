# Native VoN marker path — проверка gate и существующих доказательств

- Дата: 2026-11-02. Автор: Реверсинженер / Ghidra. Версия 0.1.
- Статус: **BLOCKED_LEGAL_BASIS_NOT_ESTABLISHED / PATH_UNKNOWN**.
- Уровень этой работы: **DOCUMENT_REVIEW + PROVENANCE_HASH_CHECK**, не новый NATIVE_RE, не runtime и не техническое опровержение API.
- Источник истины: Git RICON-Reforger; входной HEAD `170e9c02e2160e0d4f7140f37c4ff57f43c54663`. Этот evidence ещё не закоммичен.
- Поручение: message:8bd4324c-9c01-4d03-a1bb-d7dae4914fe4 в thread:9511c510-9ddd-4a5d-83e0-79760fd2e088. При недостаточном gate разрешено только описание имеющихся доказательств и точных вопросов, без анализа бинарника.
- Write-scope: только данный новый файл; spec/Core/ops/changelog и чужие изменения не затронуты. Применены `docs-manage`, `code-quality-loop`; `ai-native-sdlc-cycle` — разграничение фаз/полномочий, не новый механизм. Наличие роли Ghidra не является разрешением.

## 1. Ответ на вопрос

**Существующие доказательства не устанавливают, что VoN AI-event использует тот же transport/radio-delivery протокол, что и player voice. Также не установлены native serializer, B-side dispatch и положение переданного event/values относительно mute.** Нет основания ни для SAME_PATH_PACKET_PASS, ни для утверждения, что эти пути обязательно различаются.

Установленная сигнатура `VoNComponent.SoundEventPriority(string eventname, array<float> values, int priority, bool ignoreQueue=false)` и описание «Transfer AI sound message through VoN» подтверждают API-кандидат, но не идентичность формата/пакета/приёмника. Одноимённый метод CommunicationSoundComponent с другой сигнатурой не является доказанным caller/callee этого VoN метода. Известный SCR invoker начала звукового события не удостоверяет удалённую VoN-доставку.

Новый бинарный анализ **не начат** вследствие проверки ниже. Поэтому «не найден» здесь относится к прочитанному evidence, а не к исчерпывающему поиску в EXE.

## 2. Проверка проектного и правового gate

| Условие | Проверенный источник | Вывод |
|---|---|---|
| Есть точный нерешённый вопрос | Поручение Lead и §4 ниже | Да: input → serializer → remote dispatch/radio → mute-stage, отдельно от player voice |
| Проект допускает узкую статическую Ghidra | [PLAN:25–43](<../plan/PLAN_RU.md#L25-L43>); [STATUS:45–49](<../plan/STATUS_RU.md#L45-L49>), [STATUS:542](<../plan/STATUS_RU.md#L542>) | Да, owner #2970/#2982: файлы на диске, без target execution/debugger/memory/patch; без декомпилята в репозитории. Старый полный проектный запрет не выдаётся за отмену этого позднего исключения |
| Прежний narrow NATIVE_RE был выполнен | [E-native-von-stream](<E-native-von-stream.md>) | Да, записан как agent-reviewed; прошлое выполнение не доказывает правовое основание нового исследования |
| Письменная EULA/законность для нового разбора установлена | [E1s §5](<E1s-von-api.md#L123-L131>), [E-web1 Q26](<E-web1-von-legal.md#L159-L176>) | В этих проверках записан запрет reverse engineering игры/Tools; применимое исключение/право на новый разбор не установлено |
| Полномочие владельца проекта равно разрешению правообладателя | [PLAN:28–30](<../plan/PLAN_RU.md#L28-L30>) и названные EULA-проверки | Нет: зафиксированное проектное разрешение инструмента не является само по себе правовым выводом или лицензией BI |
| Условия новой работы не включают runtime/обход политики | Поручение Lead | Да: только офлайн; rejected unsigned WbReader wrapper не обходится другим CLI/скриптом |

Результат: **PROJECT_STATIC_SCOPE_RECORDED**, но **LEGAL_BASIS_NOT_ESTABLISHED** в прочитанном корпусе; достаточный совокупный gate для нового разбора не объявлен пройденным. Это осторожный предел полномочий этой работы, не юридическое заключение о применимом законе и не утверждение, что статический анализ запрещён при любых обстоятельствах.

[README:30–31](<../../README.md#L30-L31>) и [NOTICE:46–48](<../../NOTICE.md#L46-L48>) сохраняют более старую общую формулировку. Она расходится с поздним узким проектным исключением PLAN/STATUS; документы здесь не исправлялись. Аналогично старое NO-GO в E1s не отменяет факт позднего разрешения Ghidra, но позднее разрешение не содержит новой письменной правовой оценки. Для снятия данного блокера требуется запись применимого основания/исключения и охвата, а не ещё одна роль или повтор прежнего общего согласия. Lead владеет дальнейшим маршрутом этого gate.

## 3. Что именно уже известно

| Звено | Доказательство | Граница |
|---|---|---|
| Script API AI-event input | [Оценка кандидатов §2](<E-von-marker-feasibility-lead.md#L13-L21>) и [манифест](<receipts/von-marker-feasibility-20261102-lead.json>) | Сигнатура/описание есть; player-role, допустимый event/config, values и priority UNKNOWN |
| Receiver observable-кандидат | SCR GetOnSoundEventStarted; vanilla override передаёт component/eventName/AudioHandle/priority, согласно той же оценке | Callback не содержит sender/RX/values; native/remote origin и связь с VoN input UNKNOWN |
| Player-voice observer | VoN OnReceive: sender, receiver, frequency, quality, audio data for playback | Нет payload/seq/serializer/mute-stage; совпадение времени с sound-start не доказывает общий тракт |
| Campaign radio example | OnDelivery → PlayRadioMsg → отдельный RpcDo_PlayRadioMsg в прочитанном примере | Это не media transport proof; вывод не распространяется на все потенциальные radio messages |
| Исторический VonStream | [NATIVE_RE:24–41](<E-native-von-stream.md#L24-L41>), [helper:73–108](<E-native-von-stream.md#L73-L108>) | Для build 1.8.0.13 описаны lookup TransmissionQuality и сохранение индекса в VonStream+0x50; не formula/recipient filter/AI-event serializer |
| H2 mute | [Границы контроля:39–42](<../analysis/AUDIO_CHAIN_CONTROL_RU.md#L39-L42>) и [intent:73](<../req/INTENT-local-radio-slice.md#L73>) | RX/TX scope и положение относительно OnReceive/playback не доказаны; marker до mute не закрывает LS-05/06 |

Две ранее разобранные функции из NATIVE_RE — `[0x1407CEE30, 0x1407CF23B)` и `[0x1409FC090, 0x1409FC115)` при preferred base `0x140000000`. Адреса **цитируются из старого отчёта и receipts**, не перепроверены здесь дизассемблированием. Описанный helper возвращает индекс/отсутствие, не числовое качество. Это не entry point SoundEventPriority и не доказанный serializer. Соседство функций, строки VON_RADIO/VON_DIRECT и общий класс не заменяют dataflow.

## 4. Точные открытые вопросы для разрешённого продолжения

1. **Input binding:** где зарегистрирован именно VoNComponent.SoundEventPriority, какой native entry получает eventname/values/priority/ignoreQueue, какие owner/component/AI-player ограничения и допустимый event/config? Разделить обёртку script ABI и реальную реализацию; имя строки не есть вызов.
2. **Сериализация:** передаются event ID/name и values, закодированные audio bytes, управляющее событие или локальная команда? Где граница создания сообщения, его version/type/length и queue policy? Что не сериализуется?
3. **Сравнение с voice:** где находятся независимо установленный player-voice send/receive путь и точка их реального слияния/разделения с AI-event? Общий сетевой объект/VoNSystem/трансивер не доказывает одинаковый протокол или media-stage.
4. **Radio routing:** как AI-event выбирает TX, sender, recipient и конкретный RX? Использует ли те же power/frequency/key/range проверки, что voice, и есть ли direct/AI-broadcast альтернативная ветвь? Не угадывать по одинаковой частоте.
5. **B-side dispatch:** какая функция распаковывает сообщение, где появляются eventName/values, какой компонент и чей owner их получает? Существует ли публичный output с достаточной attribution, или на скриптовой границе виден только playback event без payload?
6. **Mute ordering:** где проверяется конкретное RX mute относительно native dispatch, VoN OnReceive, communication sound-start и фактического playback? Может ли callback/decoded marker сохраняться при подавленном выходе? Direct/user mute рассматривать отдельно.
7. **Multiplicity/lifetime:** один input соответствует одному callback либо queue сливает/дублирует/отбрасывает одинаковые события? Как задаются завершение, generation, rebind и late delivery? Не создавать фиктивный frame ID или lease.

Ответы должны различать доказанный участок, гипотезу, неизвестный indirect call и необходимое runtime подтверждение. Даже положительная статическая цепочка не доказывает успешный приём nonce, безопасность H2, сохранность direct или слышимость.

## 5. Provenance и сверка SHA-256

На диске подтверждены размер **37 272 632 байт**, FileVersion **1.8.0.13** и SHA-256 установленного [ArmaReforgerSteam.exe](<C:/Program Files (x86)/Steam/steamapps/common/Arma Reforger/ArmaReforgerSteam.exe>) `1d221a990fa16c78fe02f685a7ba1330be115ca93cd3b641e67e5d3f1b40d9c9`. Путь совпадает с записанным в NATIVE_RE/receipts. Выполнены только Get-Item/VersionInfo и Get-FileHash; EXE не импортировался в Ghidra, не разбирался и не запускался.

Прочитаны обе существующие receipts в исключённой `.re/notes/codex/`; их полные хэши совпали с NATIVE_RE. Они сообщают `temporaryProjectRemains=false`: исторический readOnly-проход не оставил пригодную analysis database. Поиск файлов `.gpr/.gzf/.gdt` внутри `.re` нашёл только штатные Ghidra type archives, не базу исследованной игры. Сохранённые логи/псевдокод не считаются готовой базой; их содержимое в новом исследовании не анализировалось и сюда не переносилось. Установка Ghidra/JDK уже существует; версии/архивные SHA в старом отчёте не заменяются утверждением новой проверки установки.

| Прочитанный артефакт | SHA-256, сверено в этом проходе |
|---|---|
| [PLAN](<../plan/PLAN_RU.md>) | `980cdcf18204941aed0bfd950c4c41cfa35078774ab266d7cd0d6ddfcdd2b427` |
| [E1s](<E1s-von-api.md>) | `6af0e61208b5319cca369185286a78b15da939956407fffd0976d6f7fade058f` |
| [E-web1](<E-web1-von-legal.md>) | `5a0e7df45c3b9745a9c8459fee10580bb18bf1b1511c9b4184c51cde56ba7257` |
| [Native VonStream](<E-native-von-stream.md>) | `42b1b27506cf04169d4851b2950645205dccd7e7502dc4ccd5d0746853fba191` |
| [Marker feasibility](<E-von-marker-feasibility-lead.md>) | `66ed14ba41fdf59733d4245f7a8f75e0a05c2de78f1188ca6789e3d386c161e5` |
| [Marker source manifest](<receipts/von-marker-feasibility-20261102-lead.json>) | `aa08de14d1327fbbd116f049c9b0a45ad7b26cecf10d1e78226f7477926b0c96` |
| [VonStream receipt](<../../.re/notes/codex/native-von-stream-20261021/receipt.json>) | `989f98dbb7639e34dbb27fad6917c0316e55152b84b464378a592d9904a916ff` |
| [Helper receipt](<../../.re/notes/codex/native-helper-20261021-120000/receipt.json>) | `b4146127f485adb7b28981fcc528b3f99e09b5b808746c69e2911bd31ad5d9ff` |

Hash receipt подтверждает её неизменность относительно опубликованного отчёта, но не независимое подтверждение каждого старого вывода. Чужие изменения рабочего дерева на старте — state цели и DRAFT probe — сохранены без правок.

## 6. Итог и остановка

- Нового NATIVE_RE, Ghidra/Java запуска, дизассемблирования или декомпиляции **не было**. Не запускались игра, Workbench, debugger, hooks, live/SetCapture, передача, PAK extraction; политика Windows и rejected wrapper не обходились.
- Исторические findings сохранены на их ограниченном уровне; протокол/serializer/RX/mute-stage остаются **UNKNOWN**. Отсутствие нового исследования не является FAIL механизма.
- Ничего из этого не меняет fail-closed seam, его enum, RF-модель, direct или статусы LS-02…08. Декомпилят и проприетарный код не включены.
- Следующий владелец gate — Lead: записать применимое правовое основание и точный допустимый scope либо продолжать по официальным API/штатным source evidence без нового бинарного разбора. Сам факт старого разрешения статической Ghidra уже подтверждён и повторно запрашивать его как отсутствующий не требуется.
