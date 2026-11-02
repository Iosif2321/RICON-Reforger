# N1 — DRAFT-план capability-free K0 phase model

Версия 0.2, 2026-11-02: узкая corrective N1-R01 по QA и Lead, только порядок fake FRAME/edge marks и отрицательные vectors; вход v0.1 SHA-256 `a320593409e6365dcdc16bd3fe57210656559e8076c03c088efac049c94bd58a`. Тип: **DRAFT_PROPOSAL / DELTA_REVIEW_PENDING**, doc-only план нейтрального proof/model slice. Автор — Enforce-разработчик; accountable/приёмка — Lead; независимый reviewer — QA. Task task:68d2e092-e413-4c61-aa3e-c18fcd44681d. **CODE_NOT_AUTHORIZED / TESTS_NOT_RUN / PHYSICAL_BLOCKED**.

## 1. Контекст, входы и граница перехода

Решение Lead в принятом Q2 task:0b0b1875-440c-4a1f-b72b-40fa7303c267 выбирает вариант A: чистую value-only модель. Она проверяет политику K0 до квалификации physical refs/timing/native API; это не обход этих blockers, поскольку модель вообще не имеет физических возможностей. Назначение N1 разрешает только этот документ, не создание кода или запуск.

Git RICON-Reforger — источник редакций; HEAD при подготовке `a04a14f125fe757ad0038d762e2b44ab4819085b`. Новые/изменённые technical inputs находятся в shared working tree, **NOT_RECORDED**, не approved executable specification. Исторический waiver закрытой задачи сюда не перенесён: следующий фазовый переход **FRESH_SESSION_REQUIRED**, не PROVEN/WAIVED. Этот текст — предложение bounded model slice для review, не объявление пройденного planning gate с незаписанной spec. Для writing нужны независимый review, решение/record точных входов, fresh transition либо новый явный waiver и отдельный GO Lead. Наличие файла/Claim/принятие Q2 не заменяет эти условия.

| Read-only вход | Идентичность прочитанного входа | Для чего |
|---|---|---|
| [Утверждённый intent v0.2](<../req/INTENT-local-radio-slice.md>) | исходная цель Team680; исходная v0.1 `249507a`, исторический контекст | Не подменять реальную A→B доставку локальной моделью |
| [Corrected K0 REQ v0.4](<../req/REQ-standard-ptt-K0.md>) | SHA-256 `aaf30975ea63a1d34574569049a222a73c07b2d6d0d32c236b3b5b5b414196a2` | §§6–10: phases, reference, quotas, command/exit separation; §11.1: отдельная pure-модель |
| [Physical proposal](<PLAN-physical-ptt-K0.md>) | SHA-256 `7788d7ad0691b071dd59393e9f9f90ed945f2eb71538de672d3afe1a3cfa1b36` | Только нормативный контекст, не adapter/CLI для reuse |
| [Q2 safety feasibility](<../review/R-k0-safety-feasibility.md>) | SHA-256 `b801760f99273dba1bfd8e904ad6edcb0983998e26e92878bcdd13db280821ee` | Failure domains и контрпримеры; принятый вариант A |
| [Frozen N0 plan v0.3](<PLAN-standard-ptt-K0.md>) | SHA-256 `13b573056a9b9eb1eda2e49cac4cb377e1f120e3fc3a9545610019cc432301cc`; frozen scripts на baseline | Разделение результатов; НЕ импортировать N0 protocol/core/runner |
| [V1 с delta-review](<../review/R-local-radio-design.md>) | §7, B10 закрыт только как specification finding | B01…B09 остаются открытыми физическими обязательствами |

Прочитан [родительский AGENTS](<../../../AGENTS.md>); корневой отсутствует. Arma 3/SQF/PBO workflow к Reforger не применяется. Применены ai-native-sdlc-cycle/artifact-contracts, code-quality-loop, docs-manage, graphify (граф отсутствует; генерация вне scope). Версии навыков независимо не установлены. Docs-коннектор недоступен: deliverable — назначенный Markdown. Changelog/goal/task-board ведёт DOC/Lead отдельно, этот scope их не меняет.

## 2. Exact manifest и capability boundary

Сейчас единственный write-scope — этот план. Следующий **предлагаемый**, ещё не существующий code-scope: ровно два новых файла вне `Scripts/Game`, `Scripts/WorkbenchGame` и `ops`:

| Предлагаемый path | Обязанность / writer |
|---|---|
| `tools/RICON_LiveProbe/model/k0-phase-model.ps1` | Definitions-only pure функции и фиксированные constants/schema; единственный writer Enforce |
| `tools/RICON_LiveProbe/model/k0-phase-selftest.ps1` | Default-off fixed deterministic vectors и ограниченный текстовый результат; тот же writer |

Папка model не подключается к addon.gproj или игровому bootstrap. Никаких третьего launcher/adapter/schema/data fixtures, правок N0, пяти reserved physical I1 paths, I2, existing launchers/observer/accessor, addon.gproj, addons, профилей, технических specs или metadata. Изменение manifest требует отдельной reviewed поправки до writing. QA не редактирует модель/тесты ради PASS.

**No-capability invariant N1-C0:** входы — только скалярные значения и ограниченные структуры, созданные фиксированным selftest; никаких ссылок на engine/.NET Process/Stream/Delegate, scriptblock/callback/command или arbitrary object. Model не читает system/game clocks, PID/HWND, environment, filesystem/registry/network, устройства, input/VoN/audio; не делает Start/Kill/Wait/Dispose, WriteFile, GUI/PInvoke/Add-Type, jobs/threads/tasks/runspaces, sleep/polling и не импортирует модули/runtime adapters. Нет dynamic eval/Invoke-Expression, reflection/COM/assembly loading, callable injected adapters, dot-source пути из входа. Запрещены hidden script/global state, timers и обработчики событий. Lookup команд/устройств в model отсутствует.

Core при dot-source только объявляет функции; никаких top-level allocations/clock/file effects и вывода. Constants создаются внутри вызванной pure функции, не top-level. Selftest импортирует только соседний фиксированный core, **после** disabled/parameter guards. Без `-EnableK0Model` selftest выдаёт только `N1_DISABLED`, exit0; core не загружает. Неизвестные параметры/Mode — отказ до импорта. Test stdout — единственный намеренный эффект тестовой оболочки, не модели. PowerShell function return использует pipeline как возврат значения: каждый API обязан вернуть ровно один объект, без случайных Insert/Add outputs.

## 3. Предлагаемые exact API, DTO и арифметика

Все имена ниже — **будущие функции модели**, не Enfusion/native API:

| API | Контракт |
|---|---|
| `New-K0ModelState -RunId <string> -StartMs <long>` | Создаёт новый value-state, phase STARTUP, attempt1, generations1, фиксированные лимиты; не ARM и не часы |
| `Test-K0ModelInput -State <hashtable> -Input <hashtable>` | Возвращает один `{Valid:bool, Reason:string, ReferenceMs:long|null}`; не мутирует state/input и не выбрасывает truthy array |
| `Update-K0Model -State <hashtable> -Input <hashtable>` | Один `{State:hashtable, Output:hashtable}` с отдельными копиями ограниченных вложенных value-структур; не изменяет вызывающий state/input |
| `Get-K0ModelVerdict -State <hashtable>` | Один фиксированный verdict DTO; не команды, не PASS физического тракта |

Параметров настройки caps/deadlines/API paths нет. RunId — ровно32 lowercase hex, attempt всегда1. Scalar string≤256 ASCII characters, reason/enum из whitelist; symbolic role A/B и owner `N1_OWNER_1`, symbolic handles `N1_HANDLE_A_1`/`N1_HANDLE_B_1` — **не PID/OS handles**. Идентификатор intent — run/attempt/role/ordinal1, value string≤64. Во входах нет реальных payload/сырых игровых callbacks.

Время — injected integer milliseconds в одной **вымышленной** шкале 0…1,000,000,000; StartMs≤999,800,000. Никакого QPC/GetTickCount conversion. Сложения проверяются до операции; все marks≤NowMs, NowMs монотонен. Null mark = UNKNOWN, не0. Допустимые integral values должны иметь Int64 type, не float/numeric string/Boolean; сравнения ordinal/case-sensitive, неизвестные поля отвергаются. Нет unbounded enumeration/recursive validation произвольного графа: state/input имеют фиксированную форму, проверяются известные keys и лимит количества keys до копирования.

Общий input: `Kind, RunId, AttemptId, NowMs`; event-specific keys ниже. Ровно перечисленные keys; неприменимые поля отсутствуют. Kind и state schema1. State имеет фиксированные scalar phase/deadline/fault flags, ровно2 role slots, ровно3 invalid-category flags и4 reserve-subtype counters/роль, не растущий список причин. PrimaryReason + EarliestReference + sticky `ProtocolFault, SafetyFault, IntegrityFault` сохраняются без unbounded history. Общая оценка state≤160 scalar slots и fixed output≤48 scalar slots; строки входят в лимиты §6. Никаких arrays записанных events; vectors подаются по одному.

| Семейство input / дополнительные keys | Смысл |
|---|---|
| `ROLES_READY`: `AOwned, BOwned, AIdentityValid, BIdentityValid, AHandleToken, BHandleToken` | Только fake facts; старт observation, не настоящий startup/ownership |
| `ARM`, `DOWN_MARK`, `TICK`, `FINALIZE` | Без дополнительных keys; fake operator/control/policy step |
| `FRAME`: `Role, FrameSeq, Event, ReceiveMs, BindingGeneration, SubscriptionGeneration, SenderMatch, RefClass, EncodingValid, SchemaValid, EncodedBytes, FieldChars, RawArgCount, DiagnosticBytes, CallbackLowMs, CallbackEstimateMs, CallbackHighMs, OtherErrorMs` | Event DOWN/UP/CAPTURE/RX/SNAPSHOT; RefClass MATCH/RADIO_MISMATCH/DIRECT/NULL_OR_UNKNOWN. Для SNAPSHOT и DOWN/UP RefClass MATCH — нейтральное value-поле, не наличие native transceiver; callback interval обязателен для DOWN/UP, остальные interval поля null |
| `PARTIAL`: `Role, ReadBeginMs, BufferedBytes` | Сохраняет earliest partial mark, не новый timeout |
| `FAULT`: `Role, Class, OperationBeginMs, ReceiveMs, PartialFault, DiagnosticBytes` | Class FRAME/FRAMING/IO/QUOTA/FOCUS/OWNERSHIP/GENERATION/CLOCK/UNKNOWN; reference rules §5 |
| `COMMAND_BEGIN`: `Role, IntentId, Owner, HandleToken, CallId` | Injected свидетельство входа в call, не выполнить call |
| `COMMAND_RETURN`: `Role, IntentId, CallId, Success` | Injected return в NowMs; exception представлен Success=false; отдельный return mark |
| `ROLE_EXIT`: `Role, HandleToken, ExitConfirmed, NaturalExit` | Injected exit, не следствие return |
| `DETACH`: `Role, SubscriptionsZero, TerminalComplete` | Injected cleanup facts, не actual Remove |
| `OWNER_CONFLICT`, `WATCHDOG_SUSPENDED`, `BLOCKING_FIRST_CALL`, `CRASH_AFTER_LATCH`, `HOST_HANG` | Только negative witness; никаких adapters/failover/retry |

FRAME имеет ровно22 keys вместе с common keys (v0.2 добавляет один FrameSeq); остальные input≤16. Максимум24 keys — прежний общий admission bound. Для N1-R01 внутри прежнего state≤160 scalar slots зарезервированы восемь scalar order slots: `LastFrameSeq[A/B]` (изначально0), `LastReceiveMs[A/B]` (null), `DownMarkMs`, `AcceptedDownReceiveMs`, `AcceptedDownCallbackHighMs` (null), `PhaseEnteredMs` (StartMs). Никакой очереди/history map не добавляется; output≤48, input≤24 и все caps прежние. FrameSeq — Int64 1…400 на роль в данном run/attempt, не game/native sequence. LastFrameSeq/LastReceive обновляются атомарно только для полностью accepted FRAME; rebind/retry/смена phase их не reset. State shape/constants и exact enum types reviewer сверяет до implementation. Отсутствующий обязательный key/избыточный object/массив/callable → invalid schema; oversized arbitrary value не копируется/не сериализуется ради диагностики. EncodedBytes — **заявленный размер fake record**, не результат реального encoder. Model не является JSON/UTF-8 parser.

## 4. Конечный автомат и раздельные выходы

| State | Допустимый input | Переход/действие policy |
|---|---|---|
| STARTUP | ROLES_READY с двумя qualified fake own identities/handles до start+120000 | READINESS, единственный ObservationStart=NowMs, AbsoluteDue=NowMs+45000 |
| READINESS | ARM после≥3000ms baseline, до absolute due | ARMED, ArmDue=NowMs+5000; ARM не input edge |
| ARMED | DOWN_MARK полностью до ArmDue | PENDING_DOWN, PendingDue=NowMs+1000, FallbackDue=NowMs+1600 |
| PENDING_DOWN | валидный FRAME/DOWN роли A, checked NowMs<PendingDue и NowMs<ReceiveMs+1500 | HELD; атомарно установить HeldDue=ReceiveMs+1500, затем снять pending/fallback; сохранить game-interval отдельно |
| HELD | валидный FRAME/UP той же generation A и checked NowMs<HeldDue | POST_UP; атомарно установить PostUpDue=ReceiveMs+3000 и снять HeldDue; поздний UP не disarm |
| HELD/POST_UP | CAPTURE A / RX B с MATCH, sender match для RX | Только MODEL_CAPTURE_WINDOW / MODEL_RX_WINDOW флаги; не delivery/audio/source proof |
| POST_UP | TICK с expired PostUpDue без invalid fault | TEARDOWN normal, сформировать eligible symbolic intents один раз |
| Любая незапечатанная | fault/expired safety due/invalid transition | sticky INVALID; записать ABORT transition, итоговая phase TEARDOWN, eligible intents без ожидания UP/ACK/detach/log |
| TEARDOWN | COMMAND_BEGIN/RETURN, ROLE_EXIT, DETACH, TICK/FINALIZE; FRAME/FAULT только bounded diagnostics | Только наблюдения fake результатов и reserve diagnostics; no new ordinary/ARM/retry/positive flags. После полного cleanup либо FINALIZE/cleanup due — TERMINAL |
| TERMINAL | любой input | Phase не меняется; лишний input отвергается, sticky integrity fault запрещает итоговый model-positive; GetVerdict разрешён |

TICK не берёт реальные часы. Отсутствие нового шага модели не означает, что таймер сам сработал. STARTUP due действует только до ROLES_READY; absolute due после создания не reset ни одним input. CAPTURE/RX вне HELD/POST_UP — invalid/diagnostic, после abort не positive. Duplicate DOWN/UP, UP-before-DOWN, wrong role/sender/run/generation, raw direct/null/mismatch и clock anomaly — invalid; quiet baseline не доказывает provenance. SNAPSHOT admission≤5Hz, интервал≥200ms, без catch-up; первый snapshot разрешён в READINESS, не меняет observation origin.

Output фиксирует `PreviousPhase, Phase, PrimaryReason, ReferenceMs, Disposition, RawComplete, IntentA, IntentB, ModelInvalid, ValidationResult`; IntentA/B — null либо один symbolic intent DTO из фиксированных scalar fields. Нет executable code/commands/paths. Role slot хранит `IntentLatched, BeginObserved, ReturnObserved, ReturnSuccess, ExitConfirmed, NaturalExit, SubscriptionsZero, TerminalComplete` и marks. Intent, begin, successful return, exit и native release — разные факты.

Verdict поля: `Scope=N1_VALUE_MODEL`, `PolicyResult=MODEL_TRACE_CONSISTENT|MODEL_INVALID`, `DispatchObservation=NOT_OBSERVED|MODEL_WITHIN_THRESHOLD|MODEL_FAILED|UNKNOWN`, `CleanupObservation=MODEL_COMPLETE|INCONCLUSIVE`, `NativeRelease=UNKNOWN`, `PhysicalSafety=NOT_ESTABLISHED`, `PhysicalCommandOnce=NOT_ESTABLISHED`. Selftest PASS означает совпадение всех ожидаемых результатов **включая negative witnesses**, не объединяет MODEL_INVALID в положительную physical квалификацию. Строки K0_INPUT_CAPTURE_RX_QUALIFIED/AUDIO_PASS/RELEASE_PASS/SAME_PATH_PACKET_PASS не выдаются как verdict.

## 5. Reference, precedence и command-once policy

### N1-R01: anti-replay и причинный порядок до phase transition

Следующие guards обязательны для любого FRAME, а не только контрпримера QA. Сначала schema/run/attempt/role/generation, затем sequence, затем receive order, затем edge/phase-order; одновременно наступивший due всё равно не disarm и сохраняет приоритет §5. Ошибка на любом guard не обновляет accepted frame/edge history, не ставит positive flags и не переводит в HELD/POST_UP. Она sticky INVALID → protective TEARDOWN, без retry; прежние due сохраняются для reference/receipt и не заменяются PostUpDue. Diagnostic/reserve admission не делает rejected frame accepted.

1. **Per-role anti-replay:** `FrameSeq == LastFrameSeq[Role]+1` с первым1; duplicate/backwards/gap/out-of-range sequence → `INVALID_FRAME_SEQUENCE`. Seq не reset при phase/generation change; новая generation в одной попытке сама invalid. У двух ролей независимые counters; это не сравнение native A/B sequence или clocks.
2. **Receive order:** `ObservationStart ≤ ReceiveMs ≤ NowMs`, `ReceiveMs ≥ PhaseEnteredMs`, при non-null LastReceive — `ReceiveMs ≥ LastReceiveMs[Role]`. Нарушение → `INVALID_FRAME_TIME_ORDER`. FrameSeq увеличивается строго; одинаковый integer-ms ReceiveMs допускается только для non-edge records с разными последовательными seq и означает порядок обработки, **не** доказательство порядка callbacks. Для edge equality недостаточно по п.3. PhaseEnteredMs обновляется только при реальном accepted phase transition на injected NowMs; не берётся из backdated frame.
3. **Причинность DOWN_MARK→DOWN→UP:** accepted DOWN_MARK сохраняет DownMarkMs=NowMs. Qualified DOWN требует `CallbackLowMs > DownMarkMs`, `ReceiveMs > DownMarkMs` и прежние PENDING/due/interval guards; только accepted DOWN сохраняет AcceptedDownReceiveMs и AcceptedDownCallbackHighMs. Qualified UP требует non-null этих anchors, `ReceiveMs > AcceptedDownReceiveMs`, `CallbackLowMs > AcceptedDownCallbackHighMs`, receive-order п.2 и прежние HELD/due/interval guards. Отсутствующий anchor, equality либо overlap callback intervals → `INCONCLUSIVE_EDGE_TIME_ORDER` с sticky INVALID, не valid UP/disarm/release. Умышленно консервативно: миллисекундная equality/перекрытие не интерпретируются догадкой о реальном порядке. NativeRelease остаётся UNKNOWN даже для strictly ordered fake pair.
4. **История phase для остальных FRAME:** CAPTURE/RX/SNAPSHOT тоже подчиняются sequence/receive-order пп.1–2 и допустимым phase windows §4. Ни новый seq, ни свежий NowMs не легализуют receive до phase entry. У CAPTURE/RX нет доказанного callback interval: их model window относится только к injected receive, не причинности аудио. PARTIAL сохраняет своё раннее reference, но не продвигает LastFrameSeq/LastReceive и не даёт accepted edge. Schema/sequence/order ошибка полного FRAME использует его валидный ReceiveMs как fault reference, не поздний NowMs; если сам mark отсутствует/не имеет допустимого type/range — UNKNOWN reference. Ошибочный старый receive сохраняется как diagnostic reference, но не перезаписывает accepted history.

До обработки input составляется множество уже наступивших due по injected NowMs; equality due всегда побеждает DOWN/UP. Валидация нового input может добавить fault с более ранним reference, но не erase due. Из наступивших due и известных fault references выбирается minimum. Future due не кандидат. Primary outcome независимо: absolute due → invalid fault → safety phase due → normal POST_UP. Одновременно увиденные причины sticky; поздний primary не сдвигает ранний reference. При равенстве fault classes порядок строк таблицы ниже.

| Fault | Reference для модели |
|---|---|
| Scheduled startup/arm/pending/fallback/held/absolute/post-up | Исходный due, не NowMs/check |
| Complete malformed/schema/route/phase/generation/clock FRAME | ReceiveMs до проверки, не NowMs после неё |
| PARTIAL_FAULT / incomplete tail | Earliest сохранённый ReadBeginMs этой роли |
| FRAMING/oversize | OperationBeginMs, minimum с ранее сохранённым partial mark |
| IO | OperationBeginMs, minimum с non-null ReceiveMs triggering frame |
| QUOTA | Capacity-check OperationBeginMs, minimum с ReceiveMs при frame-trigger |
| FOCUS/OWNERSHIP/GENERATION/CLOCK external sample | OperationBeginMs qualified fake sample; occurrence time не моделируется |
| UNKNOWN / bad or missing mark | Reference UNKNOWN, DispatchObservation UNKNOWN; не future heldDue/late detection |

Valid complete FRAME после частичных bytes очищает partial buffer лишь если partial fault ещё не был объявлен; не вводит нового grace period. Уже возникший partial/oversize fault sticky и не лечится поздним complete FRAME. PARTIAL не reset существующие due. Если NowMs перешёл due во время смоделированной validation, due участвует на том же шаге: ReceiveMs100/NowMs250 — именно такой input, не функция, читающая часы второй раз.

Для DOWN/UP fake interval low≤estimate≤high≤receive≤NowMs. N_est=receive−estimate≥0 и≤100; E_mapping=max(estimate−low, high−estimate), E_total=E_mapping+OtherErrorMs≤50. Проверить receive−low≤N_est+E_mapping; E_total учитывается **однократно**, не N_upper+тот же E_mapping. При invalid/unknown interval нет qualified edge. H1500, D150, E50, reserve200 — неизменённые thresholds контракта; reserve не покрывает overshoot. Модель не квалифицирует clock mapping.

При переходе в TEARDOWN одна policy step **сначала** ставит per-role IntentLatched, **затем** возвращает оба eligible intents в одном Output. Eligibility: fake own handle/identity known, не natural exit, единственный owner assumption, нет prior latch. Нет handle/authority → intent=null, outcome BLOCKED_OWNERSHIP/INCONCLUSIVE. Ни raw logging failure, ни отсутствие manual UP/ACK не prerequisite для intent. Если role уже завершилась естественно — NOT_OBSERVED, не успешный command.

COMMAND_BEGIN допускается ровно1/role для matching latched IntentId и owner/token; COMMAND_RETURN требует matching CallId, один begin и один return, return≥begin. При известном reference дополнительно begin≥reference; при UNKNOWN reference injected begin/return всё равно сохраняются, но DispatchObservation навсегда UNKNOWN. Исполнитель ничего реально не вызывает. Dispatch threshold проверяется только после **двух** required successful returns: max(returnA,returnB)−ReferenceMs≤150. Error/missing return любой required роли запрещает MODEL_WITHIN_THRESHOLD; второй fake begin не обязан ждать return/exit первой. Cleanup deadline=TEARDOWN-entry NowMs+10000 — только срок model decision, не гарантия exit. Role exits и detach/terminal нужны отдельно; success return не устанавливает ни один cleanup flag.

Latch ограничивает выдачу intents в одном сериализованном model state, не actual calls между процессами. CRASH_AFTER_LATCH оставляет latched=true, begin=false, ноль гарантированных calls; восстановление новой независимой state не считается продолжением с общей durable latch. OWNER_CONFLICT даёт invalid без обещания предотвратить уже выданные intents другого owner. Selftest с двумя независимыми states обязан показать возможность двух intents для одного символического handle. Это negative witness, не реализация shared atomic protocol.

## 6. Физические caps модели и injected byte-budget

Model хранит счётчики/решения, не полные records. На роль: snapshot240, raw128, safety reserve4, post-abort diagnostics16, cleanup/terminal12; reserve total32, сумма400. Лимиты K0 не увеличиваются. Каждая роль имеет независимые counters/LastSnapshotMs; модели достаточно fixed2 role slots. EncodedBytes1…4096, FieldChars0…256, RawArgCount0…16. DiagnosticBytes1…4096 отдельно передаётся для bounded failure envelope. Проверки до выдачи ACCEPT decision; никогда не сериализовать произвольный oversized object ради size check.

240-я/128-я допустимая запись сохраняется в ordinary decision. Первая 241-я/129-я, если полный fake record помещается, занимает **один safety reserve slot**, Disposition RESERVED_OVERFLOW/RawComplete=true, INVALID sticky; ordinary freeze сразу для обеих ролей после abort. Если oversized/encoding-invalid record не может быть lossless — только bounded diagnostic decision, RawComplete=false, IntegrityFault; не выдать PASS сохранности. Reserve exhaustion/missing terminal/injected invalid size дают INCONCLUSIVE_LOG_INTEGRITY, не silent drop. Последующие diagnostic decisions только из16 post-abort; terminal/detach только из12 cleanup; ordinary не использует ни один reserve. После exhaustion счётчики не увеличиваются, число отвергнутых событий saturating scalar, не растущий список. Первичный safety/overflow diagnostic расходует safety slot соответствующей роли; последующие FRAME/FAULT после abort — post-abort slot без positive flags. DETACH и ROLE_EXIT расходуют по одному cleanup slot своей роли, FINALIZE — один terminal slot каждой роли; exhaustion любой требуемой terminal записи запрещает MODEL_COMPLETE. COMMAND facts сохраняются фиксированными scalar flags даже при log-integrity fault: отсутствие резервного слота не создаёт зависимости intent от записи.

Per-role serialized-budget decision≤400×4096=1,638,400 bytes, обе роли≤3,276,800; считать дополнительно actual state/output нельзя этим cap: это логический budget воображаемых records, **не запись файла**. Proposed implementation memory: state≤160 scalar slots, output≤48, current input≤24; каждая string≤256 ASCII, fixed2 role objects и максимум2 intent DTO. Нельзя хранить trace целиком, все vectors/frames заранее в unbounded arrays или произвольный diagnostic payload. Fixed vector generator использует не более двух states одновременно,≤4096 шагов/vector,≤96 vectors; boundary loops не материализуют240/128 events. Это отдельно ограничивает работу теста, но не гарантирует real-time completion PS/host.

Общий bound всех frame fields должен быть проверен до переноса в state. Модель доверяет injected encoded size как тестовому числу; **не доказывает** реальную UTF-8 длину, JSON escape/framing, FileIO sharing/flush или native allocations. No-engine-refs invariant здесь0, но это не proof физического refs≤8. Input byte buffers/parser намеренно вне manifest.

## 7. Failure-domain graph и uncovered obligations

Физический контекст, НЕ создаваемый моделью:

```text
physical writer → game/input → callback → encoder/IO → reader/runner → dispatch owner → callA/callB → exit
       focus ───────┘                storage ───────────┘                    │
       manual UP ───── отдельная обязанность                               kernel/IO
host scheduler/kernel/power ── общий failure domain всех user-space участников
```

Граф N1: fixed selftest values → pure Update → value decisions/assertions → bounded stdout. Никакого ребра из Output в physical APIs. Host и PS executor могут зависнуть, поэтому даже test timeout — внешняя best-effort containment мера, не математическая liveness гарантия.

| Witness / cut set | Что модель обязана показать | Что остаётся физически открытым |
|---|---|---|
| Watchdog не исполняется до due+500 | Нет автономного шага; при injected resume исходный reference сохраняется, D превышен | Scheduling bound, безопасность во время suspend |
| Первый call завис навсегда | IntentB мог быть сформирован, но returnA/returnB не выдумываются; MODEL_WITHIN_THRESHOLD запрещён | Исполнимость second call на зависшем serial executor |
| Crash после latch до call | At-most-one intent не at-least-one call; begin/return/exit отсутствуют | Durable latch/failover/actual containment |
| Два независимых owner/state | Возможны duplicate intents; локальные latches не общий lock | Межпроцессный atomic protocol/owner election |
| Storage/host hang | Нет model progression/реального IO; HOST_HANG witness invalid/inconclusive | Ни N0, ни второй user-space watchdog не лечат общий host failure |
| Потеря фокуса между samples/чужой writer | Fake focus/owner facts не continuous evidence | B04 provenance, focus/manual UP |
| Tool budgets целы, engine log burst | Model budgets не распространяются на engine | B05 native log cap, реальные memory/IO/exit caps |

B01 clock/wire/safety, B02 refs/allocation, B03 getters/bindings/toggle, B04 provenance/focus/manual UP, B05 startup/own handles/native logs, B06 authority export, B07 actuator/restore, B08 независимая A→B/audio-or-same-path/direct, B09 record/fresh/exact series не закрываются моделью. Native release всегда UNKNOWN. N0 — другой host-preflight со своими gates; N1 не повторяет его flat-wire protocol и не зависит от его process-result для написания pure функций.

## 8. Конечные vectors и acceptance matrix

Все ниже **NOT_RUN**. До writing review фиксирует имена и точное число случаев (v0.2 proposed68 = прежние62 + шесть order vectors R01…R06), а не меняет ожидания после результата. Один named vector → ровно один Boolean assertion result; exception/лишний pipeline output/нулевой assertion/duplicate name → suite failure. Каждый vector начинает независимый state, кроме явно названных двух-state witnesses. Negative trace проходит тест только при точном ожидаемом invalid/inconclusive результате, не при любом FAIL.

| IDs / число | Обязательные cases | Проверяемый контракт |
|---|---|---|
| C01…C06 /6 | disabled без core import; unknown Mode/params; nested/callable input rejection; no caller-state mutation; scalar single-return; terminal extra-input rejection | Capability/default-off/schema |
| P01…P08 /8 | legal fake cycle; startup equality; baseline2999/3000; arm equality; UP-before-DOWN; duplicate DOWN; duplicate UP; wrong role/generation/run | Phase policy; P03 содержит обе baseline границы одним Boolean |
| D01…D10 /10 | pending999/1000; held1499/1500; UP parse after due; absolute no-reset; post-up normal due; atomic fallback→held; malformed receive100/check250; fault100/due200 minimum; fault=due/absolute precedence; unknown reference | Reference/no-reset/equality |
| T01…T07 /7 | N100/101; E50/51; D149/150/151; interval negative/unknown; E_mapping однократно; clock rollback/range overflow; physical release UNKNOWN после fake success | Timing-threshold separation, не гарантия |
| Q01…Q09 /9 | snapshot240/241; raw128/129; reserve channels not shared; post-abort16/17; cleanup12/13; safety4/5; encoded4096/4097 и diagnostic oversize; fields256/257 + args16/17; A/B independent counters + no catch-up | Physical quotas решения, injected sizes |
| F01…F06 /6 | partial earliest mark; complete after partial без fault; partial fault затем complete; oversize минимум с partial; IO operation-begin; quota check-begin минимум с receive | Early reference/partial semantics |
| O01…O08 /8 | no handle; no authority; wrong token/owner; command-once при repeated due/finally facts; first return failure + second success; missing second return; natural exit; incomplete detach/terminal/exit | Intent vs actual observation, cleanup |
| X01…X08 /8 | suspended watchdog; blocking first call; crash latch→call; two owners без shared latch; host hang; burst after overflow; late UP после ABORT; focus sample без continuous provenance | Required failure witnesses, no physical PASS |
| R01 /1 | Точный backdated UP trace QA из New, описанный ниже | INVALID_FRAME_TIME_ORDER, не POST_UP |
| R02 /1 | После accepted DOWN seq1 повторный FRAME с seq1 и fresh receive | INVALID_FRAME_SEQUENCE; replay не продвигает anchors |
| R03 /1 | После seq1 следующий fresh FRAME seq3 вместо2 | INVALID_FRAME_SEQUENCE; gap не легализуется свежим временем |
| R04 /1 | DOWN interval low равен DownMarkMs либо раньше него, при остальных валидных N/E/type/due guards; два fresh histories | INCONCLUSIVE_EDGE_TIME_ORDER; нет HELD |
| R05 /1 | UP callback low равен accepted DOWN high либо перекрывает его при fresh receive/seq; отдельно UP receive равен down receive | Первые два histories INCONCLUSIVE_EDGE_TIME_ORDER; receive equality после DOWN check3101 даёт INVALID_FRAME_TIME_ORDER; no disarm |
| R06 /1 | Strictly ordered legal pair; два non-edge CAPTURE с одинаковым receive и последовательными seq внутри HELD | Контраст: эти histories разрешены; native release всё равно UNKNOWN |

R01 — независимый negative vector, не counter fixture: New Start0 → ROLES_READY0 → ARM3000 → DOWN_MARK3001 → DOWN A seq1 Receive3100/Now3101 interval[3050,3060,3070] → UP A seq2 Receive2000/Now3200 interval[1900,1950,1960], OtherError0, прочие identity/generation/size facts валидны. Expected: `INVALID_FRAME_TIME_ORDER`, sticky invalid/TEARDOWN, fault reference2000 (не3200), accepted A seq остаётся1, accepted receive3100 и callbackHigh3070; HeldDue4600 сохраняется для receipt, PostUpDue не назначен, UP не disarm, no positive/no retry. Поздний TICK не переиздаёт уже latched intents. Ни один expected flag не подставляется напрямую в initial state.

R04 допустимый equality пример: DownMark3001, DOWN interval[3001,3040,3050], Receive3100/Now3101 (N60/E39) — reject по order, не случайному порогу. R05 fresh equality пример: после того же accepted DOWN из R01 взять UP seq2 interval[3070,3110,3130], Receive3150/Now3151 (N40/E40); overlap вариант low3069 тоже reject. R06 contrast: DOWN как R01, CAPTURE A seq2 Receive3120/Now3121 и CAPTURE A seq3 Receive3120/Now3122, затем UP A seq4 interval[3110,3130,3140], Receive3150/Now3151 → POST_UP допустим; seq независим по ролям, но все времена — только fake common scale. R04/R05 subcases — каждый от New, результат named vector — conjunction точных outcomes.

Всего68 именованных vectors. C01/C02 в enabled suite проверяют только value-only gate/schema helper selftest; отсутствие фактического импорта/эффектов disabled path доказывается source review и отдельным disabled invocation §9, не рекурсивным запуском selftest/новым child. Границы snapshot240/241 и reserve exhaustion могут быть недостижимы в одном легальном45s trace при5Hz: Q01/Q03/Q05/Q06 используют явно помеченные `N1_SYNTHETIC_COUNTER_STATE` fixtures — bounded копии New-state с заданными counters, не положительные end-to-end histories. Остальные phase/negative witnesses строятся последовательными inputs от New; нельзя подменять их готовым successful state с переписанной причиной. Fixture даёт только доказательство counter arithmetic, не достижимость/physical evidence. Внутри boundary-vector несколько связанных сравнения соединяются одним строгим Boolean, не массивом truthy значений. Тестовый loop cap4096/96 — guard, не разрешение добавить незарегистрированные cases. Raw burst моделируется последовательными scalar inputs, не массивом callback objects. MODEL_CAPTURE/RX flags проверяются legal-cycle и wrong-route cases; direct/null/mismatch по отдельности входят P08 с независимыми fresh histories и conjunction одного Boolean. Missing measurement не становится0, timeout не «release».

Acceptance N1-A1: source review подтверждает N1-C0 и exact manifest. N1-A2: finite state/reference/intent/quota invariants соответствуют §§3–6 и всем68 vectors, включая N1-R01 guards; старые62 fixtures получают корректный FrameSeq/ordered marks, не обход новой валидации. N1-A3: все X witnesses сохраняют physical UNKNOWN и не обещают исполнение. N1-A4: исходники byte-identified, AST PASS, pure68/68 в отдельном gate, stdout/stderr/exit и own executor cleanup целостны. **N1_PURE_MODEL_PASS** допустим только при A1…A4; это ещё не присвоено. Ни один LS-01…08 не закрывается.

## 9. Последующие slices и exact execution proposal

| Slice / owner | Вход / разрешение | Выход и stop |
|---|---|---|
| N1 doc / Enforce | Текущее назначение, только этот файл | DRAFT, readback/SHA/whitespace → QA/Lead; STOP |
| N1 review / QA | Frozen SHA этого плана и прочитанных входов | Findings либо PLAN_REVIEW_PASS только value-only scope; не physical approval |
| N1 record/fresh / Lead | Approving-role decision, immutable record exact inputs, fresh session/новый explicit waiver | Только переход к конкретному writing gate; без commit authority author не коммитит |
| N1 writing / Enforce | Отдельный bounded GO после предыдущих gates | Только два manifest files, full readback/diff/SHA, независимый source review; не запуск |
| N1 AST / назначенный оператор | Отдельный static GO после source review | PS5.1 parse обоих файлов без выполнения; AST ошибки STOP |
| N1 pure / Lead или отдельно назначенный executor | Отдельный process/pure GO с exact2 SHA/executor contract | Disabled и enabled — по одной собственной PS5.1 invocation последовательно; raw bounded output, independent QA |
| N1 verification / QA → Lead | Полные результаты/состояние дерева/SHA/cleanup | Принятие только neutral model result; дальнейшая задача отдельно |

Файлы ещё отсутствуют; команды ниже **НЕ ИСПОЛНЯТЬ СЕЙЧАС**. Cwd checkout root. Windows PowerShell5.1 Desktop, fixed executable `C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe`, не pwsh7, не alias PATH, без -Command/-EncodedCommand/-ExecutionPolicy/Unblock-File. Проверка OS/PS/policy/hash выполняется оператором в отдельном разрешённом setup, не core. Policy/signature/sandbox/stdio refusal — STOP, сохранить точную причину; не alternate host/dot-source bypass. Отсутствие project process gate означает NOT_RUN.

AST proposal (в уже разрешённой PS5.1 оболочке, без выполнения файлов):

```powershell
$names = @('k0-phase-model.ps1', 'k0-phase-selftest.ps1')
foreach ($name in $names) {
  $tokens = $null; $errors = $null
  $null = [System.Management.Automation.Language.Parser]::ParseFile((Join-Path (Get-Location) "tools\RICON_LiveProbe\model\$name"), [ref]$tokens, [ref]$errors)
  if ($errors.Count) { throw "N1_AST_ERRORS $name" }
}
```

Future exact child CLI, сначала disabled, затем только после собственного exit/cleanup enabled:

```text
C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe -NoProfile -NonInteractive -File "<checkout>\tools\RICON_LiveProbe\model\k0-phase-selftest.ps1"
C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe -NoProfile -NonInteractive -File "<checkout>\tools\RICON_LiveProbe\model\k0-phase-selftest.ps1" -EnableK0Model -Mode Pure
```

`<checkout>` оператор подставляет в reviewed START absolute cwd; placeholder не runnable START. Selftest не создаёт children, не читает files кроме загрузки fixed sibling core самим PS, не пишет evidence. Executor вне manifest — существующая управляющая среда оператора, **не новый repo runtime adapter**. До process GO оператор обязан показать своё create-time ownership/held handle и независимый best-effort timeout20s на child; timeout → interrupt/завершить только этот удержанный child, collect exit/output, STOP, no retry и без следующей команды. Сам selftest timeouts не исполняет. Зависание executor/host не объявляется покрытым; неизвестный handle/orphan state → INCONCLUSIVE_CLEANUP, не искать PID/name и не убивать чужие процессы.

Stdout≤96 строк, каждая≤256 UTF-8 bytes, суммарно≤24576 bytes; нормальный enabled результат68 коротких case lines + summary, expected `checks=68 passed=68` и одна `N1_PURE_MODEL_PASS`. Stderr budget8192 bytes; нормальный stderr пустой. Любой stderr, output overflow/truncation, timeout, missing summary, invalid count, exit≠0 или неполный exit receipt → FAIL/INCONCLUSIVE, не PASS по одному exit0. Capture streams должны читаться без deadlock и bound применяться оператором до накопления; если существующий harness не умеет такой owned bounded executor либо named-pipe capture запрещён sandbox — process gate BLOCKED до отдельно reviewed способа. Не обходить отказ другим capture/host. Этот план не добавляет capture runner.

Operator evidence содержит exact child CLI/cwd/2 script hashes/plan hash/PS5.1 identity/policy snapshot, exit/interrupt, stdout/stderr byte counts/truncation, case summary и confirmed own child exit. Внешний receipt location/operator ownership фиксируются до process GO, не произвольный path из CLI модели. Не обещать20s hard real-time или выход игры: игры нет.

## 10. Риски, альтернативы и self-contained handoff

**Что сломается:** случайный pipeline output изменит shape/Boolean тестов; shallow clone позволит менять прошлое state; смешение intent/return/exit создаст ложный PASS; новая reference от detection скроет validation delay; partition reserve обойдётся общим счётчиком; implicit imports/clock API вернут capability. Подменять fake encoded size реальной проверкой нельзя. Защита — bounded shape validation, независимый source review, раздельные flags и точные negative witnesses.

**Самый рискованный шаг:** ошибочно подключить value Output к настоящему process/input/VoN adapter либо назвать успешный тест физической гарантией. Manifest и N1-C0 запрещают оба шага конструктивно; нет dispatch consumer, real handle, game bootstrap или foreground API. Следующий actual pure process launch также требует независимый executor gate, не следует из code GO.

**Отклонено:** (1) расширение frozen N0 — другой protocol и ownership, нарушает scope; (2) чистые функции внутри physical runner — импорт может дать process/game capabilities и облегчить случайный ARM; (3) Enforce/Workbench plugin — требует compile/process/game refs без необходимости. Выбраны два отдельно изолированных PS5.1 файла с value DTO и no adapter, не N0 reuse и не новая физическая архитектура.

Миграции/deploy/Workshop/monitoring отсутствуют. Rollback — не создавать/не вызывать либо не подключать новые default-off файлы; не удалять evidence и не менять чужие состояния. Никакой автозапуск/heartbeat/recurring work. Для остановленного собственного pure child — только ранее оговорённый owned executor cleanup; это не native release.

Ledger этой редакции: Q2 принят как doc-review; N1 DRAFT подготовлен. Код отсутствует; AST/pure/process/physical/source extraction/input/audio **NOT_RUN**. Проверки автора — full readback плана, локальные ссылки/whitespace и SHA в Team handoff. Shared goal/changelog/spec/review изменения принадлежат другим scope и не приписываются N1.

Следующий owner — QA: получить frozen полный SHA этого файла из Thread, проверить §§2–9, соответствие corrected K0 и все crash/blocked-call witnesses, вернуть findings/PLAN_REVIEW_PASS без правки авторского файла. Lead принимает reviewed proposal, отдельно решает record/fresh и bounded writing. В свежей build-session перечитать parent AGENTS, intent, exact recorded K0/Q2/physical/N0 inputs и этот план с SHA; проверить дерево/разрешение и создать только два manifest files. При изменении этих входов — delta-review до GO. Если revision/approval/fresh либо executor contract не установлены, соответствующий следующий gate BLOCKED, но это не требует Human повторно утверждать сам intent. Полный I1, I2 и физические обязанности остаются отдельными задачами и незакрытыми.
