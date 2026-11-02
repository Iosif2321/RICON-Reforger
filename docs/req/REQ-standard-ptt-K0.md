# REQ-standard-ptt-K0 — квалификация штатного физического radio-PTT

- Тип: спецификация исследовательского TOOL_ONLY опыта, **DRAFT**, v0.3, 2026-11-02. v0.2 применяет блокирующие замечания QA/VoN к таймерам, edge phases, provenance, route identity и overflow; v0.3 отдельно определяет границу пассивного preflight без ослабления требований физического K0. Источник истины редакций — Git; записанной является редакция в содержащем её commit.
- Статус: **DESIGN_DRAFT / IMPLEMENTATION_BLOCKED / RUNTIME_BLOCKED**. Текстовая поправка v0.3 прошла QA2441; разрешено только отдельное preflight planning по решению Lead2415 под автономным поручением799, не реализация или запуск.
- Входы Git: spec v0.2 на `dd6cfdf`, qualification-plan v0.1 на `d5018c9`. Источник цели: [INTENT-local-radio-slice v0.2](<INTENT-local-radio-slice.md>); источник статического mapping: [E-standard-radio-ptt-source](<../evidence/E-standard-radio-ptt-source.md>); текущее состояние: [state.json](<../../.opencode/goal/state.json>).
- Основание DRAFT: VoN-предложение Team1952, QA-review Team1970, поправка Team1978 и независимые review Team2028/2033/2048; поправка v0.3 — назначение Lead2415, автор VoN2432, review QA2441 в task:ba901e07-814a-4803-a1ae-3655d281a6b7.
- Текущий accountable после передачи Team2298 — **Lead / архитектор**. Будущий оператор процессов назначается отдельным bounded plan/runtime gate; физическое нажатие выполняет заранее подтверждённый человек на собственной роли A.
- Процесс: **FRESH_SESSION_WAIVED** по решению владельца Team799, не strict clean-session compliance и не разрешение implementation/runtime.
- Write-scope этой редакции: только `docs/req/REQ-standard-ptt-K0.md`. Plan, код, конфиги, профили, changelog, runtime и commit в этот scope не входят.

## 1. Цель и граница результата

K0 проверяет одну ограниченную попытку штатного физического `VONChannel` на свежем собственном A/B-стенде и раздельно фиксирует:

1. действие оператора;
2. доставку DOWN/UP в input listener;
3. настоящий capture callback A;
4. настоящий RX callback B;
5. состояние native release как отдельную неизвестную;
6. cleanup только собственных подписок и процессов.

K0 не является RICON actuator, transport PASS, radio-only PASS, доказательством непрерывной безопасности, доказательством слышимости или сохранности direct. Для звуковой ветви физический источник микрофона и изолированный выход B остаются отдельными обязательными условиями.

Статический mapping уже закрыт только на source-уровне: `ACTION_CHANNEL == "VONChannel"`, listeners зарегистрированы на DOWN/UP, handler вызывает typed activate/deactivate. Runtime delivery и безопасность удержания остаются `BLOCKED_RUNTIME_ACTION_DELIVERY` и `BLOCKED_HOLD_RELEASE_SAFETY`.

## 2. Раздельные уровни доказательства

| Уровень | Требуемое наблюдение | Допустимый вывод | Что не следует |
|---|---|---|---|
| `OPERATOR_ACTION_RECORDED` | Человек сообщил физические DOWN и UP, активное окно A и фактическую клавишу | Сохранён self-report оператора о заявленном действии | Независимое доказательство физического edge; ActionManager или vanilla handler получил событие |
| `INPUT_LISTENER_OBSERVED` | Собственный пассивный listener получил ровно один DOWN, затем ровно один UP для того же `VONChannel`, run/attempt и generation в допустимых phase windows | Runtime ActionManager доставил эту упорядоченную пару edges данному listener | Vanilla handler исполнился, capture начался или release завершён |
| `CAPTURE_OBSERVED` | A получил настоящий capture callback с ожидаемой локальной transmitter reference в разрешённой фазе того же run/attempt/generation | Согласованное capture-событие окна наблюдалось на A | Причинная принадлежность конкретному DOWN, непрерывное capture ownership, radio-only, сигнал микрофона, TX или RX |
| `RX_OBSERVED` | B получил настоящий RX callback в разрешённой фазе с проверенной player identity sender A и concrete radio receiver reference внутри B | Согласованное callback-событие окна наблюдалось на указанной receiver | Причинная принадлежность конкретному DOWN, целый тракт, слышимый звук, payload, качество, непрерывный radio-only или действие RICON |
| `NATIVE_RELEASE` | Отдельное доказательство native release, которого текущий DRAFT не устанавливает | Только явно наблюдённый факт будущего контракта | UP, отсутствие callbacks, timeout или process exit сами по себе не дают `RELEASE_PASS` |
| `CLEANUP_PASS` | Собственные listeners сняты, terminal records сохранены, собственные процессы завершены | Инструмент и стенд очищены в заявленной области | Корректный UP/native release до завершения процессов |

Комбинированный результат `K0_INPUT_CAPTURE_RX_QUALIFIED` допустим только при одновременном выполнении первых четырёх уровней, целостных журналах и всех validity guards. Он всё равно не повышается до `AUDIO_PASS`, `SAME_PATH_PACKET_PASS`, `RADIO_ONLY_PASS`, `RELEASE_PASS` или RICON actuation.

## 3. Предварительные блокеры до реализации и runtime

| Код | Условие снятия | При отсутствии |
|---|---|---|
| `BLOCKED_K0_EFFECTIVE_BINDING` | На свежем scratch profile штатным read-only способом установлены effective binding `VONChannel`, активный input context и отсутствие пересечения с direct/другим action | Не использовать vanilla default Caps Lock как доказательство |
| `BLOCKED_K0_INPUT_PROVENANCE` | Положительно квалифицированы собственный единственный автор ввода, отсутствие другого hold/writer и граница владения попыткой; пустой baseline/polling недостаточны | Provenance/эксклюзивность остаются UNKNOWN, runtime закрыт |
| `BLOCKED_K0_INPUT_LISTENER` | Независимо reviewed TOOL_ONLY listener регистрируется/снимается без protected wrapper, callback-инъекции или изменения action value | Нельзя объявить `INPUT_LISTENER_OBSERVED` |
| `BLOCKED_K0_READINESS_GETTERS` | Все нужные public getters/source и их null/identity semantics квалифицированы до написания кода | Не заменять UNKNOWN значением по умолчанию |
| `BLOCKED_K0_LONG_RANGE_READBACK` | Отдельно квалифицирован безопасный readback long-range toggle; существующий I0 accessor его не предоставляет | Type/toggle stability остаётся UNKNOWN, runtime закрыт |
| `BLOCKED_K0_RADIO_SETUP` | До DOWN уже существует active, usable radio entry нужного типа с reference-match на проверенный radio/transceiver | Отдельный штатный setup-контракт; запрещено выбирать entry/selector догадкой |
| `BLOCKED_K0_DIRECT_OBSERVABILITY` | Диагностика сохраняет radio, direct и null callbacks, не отбрасывая их | Отсутствие записанного direct нельзя считать отсутствием fallback |
| `BLOCKED_K0_HUMAN_OPERATOR` | Подтверждён человек, который удерживает фокус, нажимает и отпускает только свою физическую клавишу | Runtime не открывается; повторно согласовывать общую цель не требуется |
| `BLOCKED_K0_FOCUS_RUNNER` | Отдельный runner/режим доказанно сохраняет фокус A и ручной UP; текущий runner передаёт `noFocus` обеим ролям и для physical input не квалифицирован | Не менять существующий D0/V0/I0 runner молча; runtime закрыт |
| `BLOCKED_K0_PHYSICAL_SOURCE` | Для RX/звуковой ветви подтверждён собственный физический источник A без записи стороннего аудио | Допустим только более слабый input/capture результат; RX/аудио не квалифицируются |
| `BLOCKED_K0_B_OUTPUT_ISOLATION` | Для будущей слышимости подтверждён отдельный выход B | RX callback остаётся не-слышимостью; K0 не закрывает аудиоветвь |
| `BLOCKED_K0_WATCHDOG` | Независимые часы, внешний watchdog и bounded teardown собственных процессов реализованы и проверены до ARM | Runtime закрыт |

Ни один блокер не снимается polling-снимком, именем метода, значением vanilla config или поручением продолжать проект.

## 4. Допустимые чтения и запрещённые воздействия

### 4.1 Public/read-only кандидаты

После source/API review разрешено использовать только реально установленные public getters:

- цепь собственной роли: `GetPlayerController()`, `GetControlledEntity()`, component lookup/cast;
- `SCR_VONController.IsVONDisabled()`, `GetVONComponent()`, `GetActiveEntry()`, `GetVONEntries(...)`;
- entry: `GetVONMethod()`, `IsUsable()`, `IsActive()`;
- только после успешного `SCR_VONEntryRadio.Cast`: `GetIsMuted()`, `GetGadget()`, `GetTransceiver()`, `GetTransceiverNumber()`;
- `SCR_VoNComponent.GetTransmitRadio()` — только post-DOWN readback-кандидат с local reference-match; до DOWN null/старое значение не является readiness failure без отдельного source-контракта;
- `ActionManager.IsContextActive(string)` и установленный public binding getter — только после отдельной квалификации точной сигнатуры и runtime semantics;
- существующий I0 accessor допускает только SCRIPT_STATE active/direct/hold/type; long-range toggle он не предоставляет. ALIVE и callback subscriptions допускаются только через отдельно reviewed diagnostic path. Ни один SCRIPT_STATE не равен native ownership.

Имя в списке не гарантирует non-null, атомарность или пригодность в текущем build. Номер transceiver и индекс каталога запрещено отождествлять без object-reference match. Последовательные getters не являются атомарным срезом; polling не исключает краткий fallback между снимками.

### 4.2 Запреты

K0 MUST NOT:

- вызывать `SetActionValue`, protected `ActionVONBroadcast`/`SetVONBroadcast` или synthetic callback;
- использовать `SetCapture`, selector/entry/transceiver/frequency/range/power/key/mute/Social setters;
- выполнять global stop/reset, снимать чужой hold/capture или посылать UP в другое окно;
- менять пользовательский профиль, production `addons/RICON_Core/**`, конфиг установленной игры или Windows audio/policy;
- вручную вызывать `OnCapture`/`OnReceive`, скрывать null/direct callbacks или превращать diagnostic RPC в data plane;
- записывать системный микс, сторонний микрофон или ключи шифрования.

## 5. Стенд и readiness до ARM

Используются ровно две свежие собственные роли A/B, раздельные scratch profiles и log roots, максимум два игровых процесса. Старые D0/V0/I0 роли и consumed runtime gates не переиспользуются.

До ARM обязательны:

- известные run/role/process/player/entity/world identities и generations обеих ролей;
- A и B ALIVE, стабильны, без rebind; A controller и VoN component существуют и совпадают по локальной reference;
- A VoN не disabled; active entry non-null, входит по reference в текущий каталог, active, usable, успешно cast как radio entry и имеет нужный тип;
- pre-DOWN radio entry/gadget/transceiver сопоставлены non-null local references; каталог и entry number не подменяют match. `GetTransmitRadio()` до активации может быть null/старым и не является обязательным pre-DOWN match без отдельного source-контракта;
- настоящий capture transmitter reference сопоставляется отдельно только после DOWN внутри A; его совпадение не исправляет selector и не доказывает ownership/radio-only;
- B имеет известную radio receiver identity и совместимые настройки, квалифицированные без setters;
- effective `VONChannel` binding проверен на свежем A profile; current context active; физическая клавиша не пересекается с direct/иным action;
- доступные hold/direct/type SCRIPT_STATE записаны до опыта, но не считаются native ownership; long-range toggle остаётся UNKNOWN до снятия отдельного blocker;
- observers/listeners активны, generation зафиксированы, потери/overflow отсутствуют;
- baseline не менее 3 s без **наблюдённых** foreign writer/hold, capture, RX, rebind, GAP или неожиданного события; пустой baseline и polling не доказывают их отсутствие, поэтому положительная input provenance/эксклюзивность обязательна отдельно;
- человек, физический источник A и, если заявляется слышимость, изолированный выход B подтверждены отдельными runtime preconditions.

Если usable radio entry отсутствует, K0 останавливается до отдельного штатного setup-контракта. Auto-select или proximity fallback не используются как подготовка стенда.

## 6. Единственная попытка

Автомат попытки: `ARMED → PENDING_DOWN → HELD → POST_UP → TEARDOWN`. Любой invalid transition ведёт в `ABORT → TEARDOWN`, без возврата и retry. Абсолютный `observationDeadline = tObservation + 45 s` создаётся один раз до ARMED, никогда не сбрасывается и имеет приоритет над всеми phase deadlines.

1. **ARMED.** Сохранить readiness receipt, terminal budget и внешнее monotonic `armDeadline = tArm + 5 s`. ARM не нажимает клавишу и не меняет radio/input state. До deadline оператор подтверждает фокус единственного окна A и готовность физически отпустить свою клавишу; иначе teardown без попытки.
2. **PENDING_DOWN.** Внешний процесс сохраняет operator `DOWN_MARK`, запускает `pendingDeadline = tDownMark + 1 s` и pre-DOWN fallback `tDownMark + 2 s`; затем оператор выполняет ровно одно физическое DOWN собственной effective binding. Если qualified listener DOWN не доставлен внешнему watchdog до pending deadline, оператору немедленно предписывается физический UP и попытка становится `INPUT_NOT_OBSERVED/INVALID`. Пока состояние остаётся PENDING_DOWN, fallback в 2 s запускает containment независимо от listener.
3. **HELD.** `CONFIRMED_DOWN` — ровно один ожидаемый DOWN callback для совпавших action, run/attempt и generation. Game-callback timestamp и время получения внешним watchdog сохраняются раздельно. До runtime должен быть независимо измерен конечный callback→watchdog delivery bound `Lnotify ≤ 100 ms`; неизвестный или превышенный bound = `BLOCKED_K0_WATCHDOG/INVALID`. Только атомарный переход внешнего автомата PENDING_DOWN→HELD отменяет pending deadline и pre-DOWN fallback и сразу заменяет их held deadline через `1900 ms`. Команда containment при подтверждённом bound должна быть выдана не позднее 2 s от callback. Это bound команды containment, не native duration/release.
4. Целевое физическое удержание — 1 s. Оператор выполняет ровно одно физическое UP той же клавиши в том же окне. Дубликат DOWN, UP-before-DOWN, иной action/generation или callback вне phase window немедленно инвалидируют комбинированный результат.
5. **POST_UP.** Переход разрешён только после ровно одного qualified UP для той же action/generation, полученного внешним watchdog до held deadline и при соблюдённом `Lnotify`. Только тогда held watchdog disarm и создаётся отдельный `postUpDeadline = tWatchUp + 3 s`. UP не повышается до `NATIVE_RELEASE`.
6. Capture/RX для положительной классификации допустимы только в `HELD` или `POST_UP` того же run/attempt/generation. События после `ABORT` сохраняются лишь как bounded diagnostics и не дают positive verdict.
7. **TEARDOWN.** По post-UP deadline либо раньше при abort/overflow/watchdog перейти в teardown. Попытка завершается без retry, повторного ARM или иной клавиши.

Clocks game и watchdog не считаются общими. Если измерить и независимо проверить `Lnotify` невозможно, K0 остаётся runtime-blocked. Время внешнего получения нельзя называть временем callback.

## 7. Abort и риск direct fallback

Немедленный abort вызывают: потеря фокуса, rebind/generation change, ALIVE/usability/type/toggle change или UNKNOWN, foreign writer/event/hold, GAP, clock anomaly, quota overflow, неожиданный direct либо null/unknown callback, несовпавший sender/receiver, отсутствие контролируемого физического UP, истечение ARMED/PENDING_DOWN/pre-DOWN/HELD safety deadline либо абсолютного 45 s observation deadline. Ожидаемое истечение `postUpDeadline` после валидного POST_UP — нормальное завершение окна и переход в teardown, не abort/INVALID.

При abort оператор сначала отпускает только свою физическую клавишу, если всё ещё контролирует исходное окно. Нельзя посылать synthetic UP, переключаться в произвольное окно, вызывать global/native stop или исправлять selector. Затем запускается containment только собственных процессов. Краткий bounded post-abort logging разрешён только из diagnostic/reserve quota и никогда не восстанавливает positive verdict.

Readiness guards уменьшают известный риск fallback, но не исключают его непрерывно. Неизменные снимки до/после и опрос до 5 Hz не доказывают, что краткого direct fallback не было. Явно категоризированный direct делает radio-qualified результат INVALID. Null transmitter/receiver записывается как `NULL_OR_UNKNOWN`: он тоже исключает radio-qualified result, но сам по себе не диагностирует direct fallback. Отсутствие direct/null записей при неквалифицированной диагностике оставляет direct `UNKNOWN`, а не PASS.

## 8. Evidence schema и численные квоты

Каждая запись содержит минимум: `schemaVersion`, `runId`, `attemptId`, `role`, `processId`, `world/entity/player identities`, `localClock`, `localSeq`, `phase`, `bindingGeneration`, `subscriptionGeneration`, `eventKind`, bounded raw callback arguments/reference identities, `interpretation`, `validityReason`.

A и B используют только собственные monotonic clocks; timestamps разных процессов напрямую не сравниваются. Sender A сопоставляется по проверенной player identity, receiver — по object-reference только внутри B. A/B object pointers и clocks не сравниваются. Operator marks, listener events, capture, RX, snapshots и cleanup records имеют разные `eventKind` и не взаимозаменяются.

Размер каждой сериализованной записи ограничен 4096 UTF-8 bytes; строковое поле — 256 characters; массив raw arguments — 16 элементов. При общем cap 400 максимальный сериализованный budget равен 1 638 400 bytes на роль. Превышение даёт reserved `OVERSIZE_RECORD` и INVALID, без усечения в positive evidence. Предлагаемый tool-held cap — не более 8 долговременно удерживаемых локальных object references на роль только для readiness/reference comparison. Bounded plan обязан перечислить entry-каталог, временные arrays и все retained refs и доказать, что они укладываются в cap; иначе implementation остаётся BLOCKED. Этот cap не ограничивает и не характеризует внутренние native allocations. Retained refs не сериализуются как общие A/B identity и освобождаются при teardown.

### 8.1 Квоты на роль

| Канал | Cap | Правило |
|---|---:|---|
| Periodic snapshots | 240 | Записи 1…240; не чаще 5 Hz, без catch-up; отдельный счётчик |
| Raw input/capture/RX events | 128 | Записи 1…128; сохранять все события, включая null/direct/mismatch |
| Control/terminal reserve | 32 | 4 slots overflow/abort/watchdog + 16 bounded post-abort diagnostics + 12 detach/terminal/cleanup; ordinary records не расходуют резерв |
| Общий физический максимум | 400 | Сумма трёх каналов; память заранее ограничена |

Cap считается достигнутым после сохранения записи с номером cap. Следующая snapshot/raw запись (241-я/129-я) сохраняется в reserved `OVERFLOW` с её bounded raw содержимым, после чего попытка немедленно становится INVALID и переходит к containment. Raw событие нельзя отбросить или заменить только счётчиком. Post-abort diagnostics ограничены 16 reserved records и никогда не дают positive verdict. Если нужный reserved slot недоступен либо журнал/terminal неполон, результат `INCONCLUSIVE_LOG_INTEGRITY`; продолжение и любой комбинированный PASS запрещены.

### 8.2 Временные и процессные caps

- попыток: ровно 1;
- игровых процессов: не более 2;
- абсолютный observation cap: 45 s от единственного `tObservation`; не сбрасывается readiness, ARM, DOWN, UP, rebind или phase transition;
- quiet baseline: не менее 3 s;
- snapshot cadence: не более 5 Hz;
- ARMED deadline: 5 s от внешнего `tArm`;
- PENDING_DOWN deadline: 1 s от внешнего `DOWN_MARK`; независимый pre-DOWN containment fallback: 2 s от того же mark;
- target physical hold: 1 s;
- HELD containment-command cap: независимо подтверждённый `Lnotify ≤ 100 ms` плюс `1900 ms` от внешнего получения qualified DOWN; суммарно не более 2 s от callback при доказанном bound;
- POST_UP deadline: 3 s от внешнего получения qualified UP после disarm HELD;
- teardown process cap: 10 s после команды внешнего watchdog/runner; callback delivery, watchdog detection и teardown latencies записываются отдельно.

Числа являются containment bounds DRAFT и требуют независимого review в будущем bounded plan. Они не описывают native latency, duration или drain.

## 9. Классификация исходов

| Наблюдение | Вердикт | Запрещённое повышение |
|---|---|---|
| Operator self-report DOWN/UP без listener edges | `OPERATOR_ACTION_RECORDED / INPUT_NOT_OBSERVED` | Независимый physical/input PASS, API failure, transport failure |
| Ровно один listener DOWN, затем один UP той же action/generation в PENDING_DOWN/HELD | `INPUT_LISTENER_OBSERVED`; без capture дополнительно `CAPTURE_NOT_OBSERVED` | Vanilla handler, native release, transport failure |
| Correct A capture ref в HELD/POST_UP, без qualified B RX | `CAPTURE_OBSERVED / RX_INCONCLUSIVE` | Причинная принадлежность одному DOWN, сетевая ошибка, radio-only, звук |
| Verified player identity sender A + concrete expected radio receiver reference внутри B в HELD/POST_UP | `RX_OBSERVED` как согласованное событие окна | Причинность, целый тракт, слышимость, payload, качество, RICON effect |
| Явно категоризированный direct либо mismatch | `INVALID_ROUTE_OR_FALLBACK` | Radio-only PASS |
| Null transmitter/receiver | `NULL_OR_UNKNOWN / INVALID_RADIO_QUALIFICATION` | Категоричный диагноз direct fallback |
| Дубликат edge, UP-before-DOWN, edge вне phase, событие после ABORT | `INVALID_PHASE_OR_DIAGNOSTIC_ONLY` | Комбинированный positive verdict |
| Ожидаемое истечение `postUpDeadline` после валидного POST_UP без anomaly | `POST_WINDOW_COMPLETE → NORMAL_TEARDOWN` | Native release, cleanup PASS до проверки teardown |
| Rebind/GAP/overflow/foreign writer/hold/watchdog либо ARMED/PENDING_DOWN/pre-DOWN/HELD/absolute deadline | `INVALID` с точной причиной | Retry или отрицательный PASS |
| Собственные listeners/processes очищены | `CLEANUP_PASS` | `RELEASE_PASS` |
| Cleanup/terminal неполны | `INCONCLUSIVE_CLEANUP` | Любой комбинированный PASS |

Отсутствие capture/RX даже при здоровой диагностике остаётся INCONCLUSIVE для причины: input guard, capture, route, receiver и observer не отождествляются. RX callback отделён от слышимости; аудио требует подтверждённый физический источник A, изолированный B-output и отдельные положительные/отрицательные контроли.

## 10. Watchdog, teardown и native release

Watchdog использует независимые внешние часы и автомат §6. Game callback не запускает внешние часы напрямую: между ним и внешним получением существует отдельно измеряемый `Lnotify`. Защитная команда привязана к доказанному `Lnotify ≤100 ms` и held deadline 1900 ms после внешнего получения; при UNKNOWN/превышении K0 блокируется или становится INVALID. Pre-DOWN fallback от operator mark действует независимо. Любое срабатывание означает containment breach, а не native duration/release.

**Нормальный teardown после POST_UP:**

1. оператор уже отпустил только свою физическую клавишу в исходном окне;
2. инструмент прекращает ARM/ordinary snapshots и сохраняет terminal budget;
3. снимаются только собственные diagnostic subscriptions/listeners;
4. записываются detach/terminal/cleanup generations и ноль оставшихся собственных подписок;
5. внешний runner завершает только удержанные собственные процессы в пределах 10 s;
6. отдельно записываются callback delivery, watchdog detection, команда teardown и фактическое завершение каждого процесса.

**Защитный teardown при deadline/hang:** внешний watchdog сначала немедленно выдаёт команду завершения только собственных зависших процессов и не ждёт detach/subscription terminal. Затем сохраняет внешний process-exit receipt; in-process terminal может отсутствовать и тогда результат `INCONCLUSIVE_CLEANUP`. Best-effort post-abort diagnostics ограничены reserve и не задерживают защитную команду.

Нельзя очищать общую очередь, принудительно выключать capture, снимать чужое удержание или восстанавливать state на новом binding. Завершение процессов является containment/cleanup, но не доказательством корректного UP или native release до exit. `NATIVE_RELEASE` остаётся `UNKNOWN`, пока отдельный reviewed observable не установит иное.

## 11. Будущий минимальный scope и gates

Эта редакция не открывает implementation/compile/runtime физического K0 и не разрешает действия с preflight. Будущая реализация K0 возможна только после отдельного bounded plan и должна оставаться в `tools/RICON_LiveProbe/**`:

- TOOL_ONLY lifecycle/observer, сохраняющий null/direct/input/capture/RX без setters; существующие D0/V0 capture/RX subscriptions можно использовать только как source reference, их guards/modes не являются K0 gate;
- отдельный checker/static tests для квот, overflow, generations, verdict separation и teardown;
- отдельный квалифицированный physical-input runner/operator receipt с фокусом A, ручным UP, process ownership и внешним watchdog; текущий `noFocus` runner не переиспользовать как готовый K0 runner.

Точные файлы и непересекающиеся write scopes обязан назвать будущий plan до изменений. `addons/RICON_Core/**`, production VoN seam, пользовательские профили и game configs не меняются.

Переходы: `DESIGN_DRAFT → INDEPENDENT_REVIEW → RECORDED_SPEC → BOUNDED_PLAN → IMPLEMENTATION/COMPILE_REVIEW → EXPLICIT_RUNTIME_GATE`. Текущий статус останавливается на `DESIGN_DRAFT`. Старые D0/V0/I0 разрешения consumed и не открывают K0.

### 11.1 Отдельный пассивный preflight на нейтральных сигналах

Для устранения круговой зависимости допускается **планировать отдельный TOOL_ONLY preflight** до квалификации runtime-предпосылок физического K0. Его собственные проверки не требуют предварительного доказательства всех Q1–Q8 или выполнения K0. Preflight default-off, изолирован от production и старых D0/V0/I0 режимов; он не активирует VoN, PTT, `VONChannel`, реальные игровые input actions, capture, радио или аудио, не использует setters §4.2 и не модифицирует профили, конфиги или системную политику. Допустимы только нейтральные/fake события в контролируемом тестовом контуре, явно отличимые от настоящих игровых callback; они не подаются в игровой input или VoN и не выдаются за физический ввод.

**Отдельные gates preflight:**

1. **План и разработка:** сначала записать точный bounded plan с файлами, механизмом нейтральных сигналов, limits, владельцами, process ownership, cleanup, отрицательными сценариями и независимым review. После отдельного решения об ограниченной разработке можно написать только согласованный default-off инструмент; ни эта редакция, ни наличие плана не открывают разработку автоматически.
2. **Pure checks:** отдельный разрешённый срез проверяет детерминированную модель и отрицательные случаи без игровых процессов, реального ввода и VoN. Успех относится только к проверенной модели и не доказывает поведение ОС или игры.
3. **Compile:** только после отдельного compile gate и review исходников; сам compile не даёт разрешения на запуск preflight или K0.
4. **Любой process/run:** до запуска указать точный scope, команды, процессы, границы времени/логов и cleanup; нужны отдельный review и явный process/run gate. Результат записывается с идентичностью сборки и среды, сырой evidence и отдельным verdict; отсутствие gate означает NOT_RUN. Никакой preflight gate не открывает ARM или физическую попытку K0.

Preflight может исследовать механику часов, уведомления, защитных таймеров и наблюдения фокуса на нейтральной тестовой цели; соответствие конкретному игровому окну, binding, доставке и автору `VONChannel` остаётся отдельной квалификацией. Разные часы нельзя вычитать без квалифицированного mapping; конечные наблюдения задержек не доказывают универсальную верхнюю границу или real-time гарантию Windows. Потеря фокуса, данных, превышение оговорённых лимитов или неполный cleanup дают INVALID/INCONCLUSIVE в области preflight. Точные численные пределы и тестовые критерии задаются его reviewed plan до запуска, а не выводятся задним числом.

QA выявил, что 100 ms notification + 1900 ms HELD из §6/§8/§10 не оставляют запаса для dispatch и ошибки часов. Эти числа сохранены как **дефектный baseline v0.2, не acceptance готового runner**; для физического K0 нужна отдельная reviewed timing-spec correction и revised plan с полным бюджетом. До неё и выполнения всех действующих требований PTT implementation/ARM/runtime остаются BLOCKED. Best-effort или условный физический опыт этой редакцией не принимается.

Preflight не снимает автоматически ни один K0 blocker §3 и не присваивает `K0_INPUT_CAPTURE_RX_QUALIFIED`, `INPUT_LISTENER_OBSERVED`, `CAPTURE_OBSERVED`, `RX_OBSERVED`, `RELEASE_PASS` или audio/transport PASS. Он не доказывает provenance, native release, работу радио или слышимость. Результаты служат только ограниченной evidence для дальнейшего отдельного review; условия и verdicts §§2–10 не ослабляются.

## 12. Условия независимого review

Review MUST подтвердить до планирования реализации физического K0; отдельное планирование preflight §11.1 этими условиями не блокируется:

- каждый getter/listener/callback реально существует и доступен в заявленном build;
- effective binding/context можно проверить без изменения профиля;
- radio-entry readiness и reference matching не используют selector mutation;
- input provenance/эксклюзивность квалифицируется положительно, а пустой baseline/polling не объявляется её доказательством;
- null/direct callbacks сохраняются раздельно; null остаётся `NULL_OR_UNKNOWN`, polling не объявляется непрерывной гарантией;
- listener contract требует ровно DOWN→UP той же action/generation в правильных phases; дубликаты и события после ABORT не дают positive verdict;
- квоты snapshots/events/reserve и record-size caps физически раздельны, overflow fail-closed и сохраняет первый over-cap raw event; tool-held refs проинвентаризированы и не выданы за cap native allocations;
- автомат ARMED/PENDING_DOWN/HELD/POST_UP/TEARDOWN атомарно заменяет pre-DOWN fallback на HELD deadline, disarm выполняется только по qualified UP, абсолютный 45 s deadline не сбрасывается;
- `Lnotify ≤100 ms`, все phase deadlines и callback/detection/teardown latencies независимо проверяемы;
- operator self-report, input, capture, RX, native release и cleanup не объединены одним PASS;
- участие человека, физический источник A и изоляция B-output оформлены отдельными runtime blockers;
- implementation/runtime остаются закрыты до собственных recorded gates.
