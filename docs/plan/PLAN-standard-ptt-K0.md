# Bounded plan — K0: квалификация физического radio-PTT

Версия 0.1, 2026-11-02. Автор плана: Ядро / Логика / Последовательность. Текущий accountable/координатор задачи: Lead / архитектор (передача2298). Статус **DRAFT / QUALIFICATION_ONLY / IMPLEMENTATION_BLOCKED / RUNTIME_BLOCKED**. Это рабочий план закрытия предпосылок, не разрешение писать или запускать PTT-инструмент.

## 1. Основание и граница

- Источник редакций — Git RICON-Reforger, вход `dd6cfdf`; этот DRAFT до отдельной фиксации не является recorded plan. Источник решений — task:ba901e07-814a-4803-a1ae-3655d281a6b7, thread:9511c510-9ddd-4a5d-83e0-79760fd2e088.
- Утверждённый [INTENT v0.2](<../req/INTENT-local-radio-slice.md>): исходная v0.1 `249507a` + поправка владельца680. [K0 spec v0.2](<../req/REQ-standard-ptt-K0.md>) записана в `dd6cfdf`, SHA-256 `a51e6ba8c132b11a9285a69a0d945e8cc58ebd32285bcb0993c51a13ff326e58`; техническое текстовое review QA2173/VoN2155 не является утверждением реализации или runtime-ready API.
- Автономное продолжение799 и2234; **FRESH_SESSION_WAIVED**, не strict clean-session compliance. Повторного утверждения общей цели не требуется. Недоказанная техническая предпосылка всё равно останавливает следующую стадию.
- Применены `goal`, `ai-native-sdlc-cycle` + artifact-contracts, `code-quality-loop`, `docs-manage`; версии навыков в каталогах не объявлены. Существующая структура документации сохраняется. Корневой AGENTS.md отсутствует; прочитан родительский AGENTS.md каталога ARMA 3 Scripts, его SQF/PBO-рецепты не применяются к Enforce/Reforger.
- Текущий write-scope координатора: только этот план, навигация/журнал/машинное состояние `.opencode/goal/` и дневной changelog. Спецификация, исходники и профили не меняются.

K0 — диагностическая квалификация одной физической попытки на новом собственном стенде A/B. Он не реализует RICON actuator, не закрывает LS-02…LS-08 и не доказывает слышимость, тот же пакетный протокол, radio-only или native release. [Чистый production seam](<../req/REQ-von-seam.md>) остаётся BLOCKED_NO_ACTUATOR. D0/V0/I0 runtime-gates использованы и не наследуются.

## 2. Проверенный baseline

1. [Штатная source-цепочка](<../evidence/E-standard-radio-ptt-source.md>) подтверждает VONChannel DOWN/UP → typed activate/deactivate только статически. Возможны null-entry auto-select и conditional direct fallback; тип при UP вычисляется заново по toggle.
2. [I0 receipt](<../evidence/receipts/i0-live-001-lead.json>) содержит active/usable entry A с локальным reference-match; B-window invalid MAPPING_CHANGED. Это не готовность нового стенда, ownership или native idle.
3. [Штатный AddEntry](<../evidence/E-workbench-source.md#L169-L180>) инициализирует/при необходимости выбирает entry. Наличие этой ветви не является воспроизводимым разрешённым setup-рецептом A/B; K0 не вызывает AddEntry или selector setters.
4. [Старый observer](<../../tools/RICON_LiveProbe/Scripts/Game/RICON_Live/RICON_LiveObserver.c#L150-L167>) отбрасывает null callbacks и не сохраняет receiver reference. Его нельзя использовать как доказательство отсутствия direct/null.
5. [I0 accessor](<../../tools/RICON_LiveProbe/Scripts/Game/RICON_Live/RICON_LiveVoNMappingAccess.c>) читает только script active/direct/hold/type, не long-range toggle и не native ownership.
6. [Существующий runner](<../../tools/RICON_LiveProbe/ops/wb-serverplay-claude.ps1#L137-L165>) передаёт noFocus обеим ролям; его секундные ожидания не квалифицированы для уведомления ≤100 ms. [Старый watchdog](<../../tools/RICON_LiveProbe/ops/wb-run-lib-claude.ps1#L142-L151>) — отдельный однократный process timeout, без фазового K0-протокола.
7. На входе дерево чистое (`git status --short` без вывода), игровых/Workbench процессов не обнаружено. Baseline runner SHA-256 `a09e57b1c57c2a70013d817098b55a3c6ed6a6cda92f9a719e830604dc65b8bf`, lib `ae28e7e4aab955fda726cc34a649c9cbbf54709b33ac4d69b4716e7d00bd1e57`.

## 3. Сначала квалификация, затем решение о реализации

Каждый пункт Q обязателен. UNKNOWN не заменяется default, пустым baseline или названием API. Сначала read-only проверка уже доступных исходников/API/квитанций; новый WbReader/Workbench/process/preflight требует отдельной точной команды, scope и допуска. Эта редакция их не открывает.

| ID / ответственный | Что получить | При отсутствии |
|---|---|---|
| Q1 Enforce | Exact signatures и source locations public getters, input Add/Remove listener, callback arguments и lifecycle; pairing регистрации/снятия без override/protected-wrapper | BLOCKED_K0_INPUT_LISTENER / READINESS_GETTERS |
| Q2 Enforce | Read-only effective binding именно свежего A profile + active context + отсутствие пересечения direct/другого action; default Caps Lock недостаточен | BLOCKED_K0_EFFECTIVE_BINDING |
| Q3 VoN | Отдельный штатный setup-контракт нового стенда: active usable entry, ALIVE, собственные gadget/radio/transceiver references A и receiver B, совместимость без setters; safe long-range readback | BLOCKED_K0_RADIO_SETUP / LONG_RANGE_READBACK |
| Q4 QA/координатор | Положительный input provenance: один подтверждённый автор, перечень доступных устройств/писателей и обоснованная граница исключения других hold/writers. Self-report и пустой polling не доказательство эксклюзивности | BLOCKED_K0_INPUT_PROVENANCE |
| Q5 QA/координатор | Контракт focus observation и физического UP без перехвата фокуса управляющим UI; точный role-A window/process match и поведение при loss/UNKNOWN | BLOCKED_K0_FOCUS_RUNNER |
| Q6 QA/координатор | Конкретный bounded callback→external notification канал и независимо проверяемый Lnotify≤100 ms; separate clocks, delivery/detection/teardown latencies; условия применимости bound | BLOCKED_K0_WATCHDOG |
| Q7 Enforce/QA | Полный refs/catalog/temporary-array ledger и доказательство tool-held refs≤8/роль, record-size/quota lifecycle, сохранение null/direct/mismatch | BLOCKED_REFS_BUDGET / DIRECT_OBSERVABILITY |
| Q8 координатор/человек | Подтверждённый оператор уже для K0 runtime; собственный физический источник для RX/аудиоветви; изолированный B-output отдельно до слышимости | BLOCKED_K0_HUMAN_OPERATOR / PHYSICAL_SOURCE / B_OUTPUT_ISOLATION |

Для Q6 нельзя вычитать timestamps несогласованных часов A и watchdog и назвать разность задержкой; RTT/2 без доказанной симметрии также недопустим. Нужны callback/send/receive timestamps, independently qualified clock mapping с ограниченной ошибкой, полный budget transport/flush/queue и внешнего dispatch. Максимум конечной выборки сам по себе не устанавливает верхнюю границу будущих доставок; обычный Windows scheduler не даёт абсолютной real-time гарантии.

**QA2282 обнаружил дополнительный дефект текущего DRAFT spec:** 100 ms notification +1900 ms HELD исчерпывают 2000 ms до учёта выдачи команды. Необходимое условие `Lnotify + Theld + Ldispatch + clockError ≤2000 ms` пока не обеспечено. Ни этот план, ни эмпирический PASS не исправляют spec молча: сначала отдельный reviewed timing-contract correction с явным запасом/границами гарантий, затем revised plan. До него P1/P4 BLOCKED. Нельзя обещать выполнимость K0, писать готовый runner, сокращать HELD или ослаблять 100 ms скрытой поправкой.

Для Q7 не наследуется I0 budget (до8 entries +16 transceivers). Требуется учесть уникальные retained references, каталоги, aliases, invokers, временные массивы и время освобождения; native bulk allocation нельзя назвать ограниченной только по post-call Count. Эта редакция не заявляет refs≤8 доказанным.

### 3.1 Результат read-only inventory (Enforce2311 / VoN2272 / QA2282)

- **Public listener найден:** `Game.GetInputManager(): InputManager`; `AddActionListener(string,EActionTrigger,ActionListenerCallback): void`, `RemoveActionListener(...): bool`, `IsContextActive(string): bool` — [ActionManager API](<C:/Program Files (x86)/Steam/steamapps/common/Arma Reforger Tools/Workbench/docs/EnfusionScriptAPI/html/interfaceActionManager.html#L188-L211>). Callback `(float value, EActionTrigger reason, string actionName)` не сообщает физическое устройство/автора. Signature evidence не закрывает delivery/порядок vanilla/detach или Q4.
- **Binding getter — кандидат, не effective proof:** `GetActionKeybinding(string, notnull array<string>, notnull array<BaseContainer>, EInputDeviceType=INVALID, string preset=string.Empty, int keyBindIndex=-1): bool` — [точная сигнатура и пустое описание](<C:/Program Files (x86)/Steam/steamapps/common/Arma Reforger Tools/Workbench/docs/EnfusionScriptAPI/html/interfaceInputManager.html#L202-L240>). Defaults не доказывают effective scratch binding, полный перечень альтернатив/filters/conflicts или pre-allocation bound. Q2 не закрыт.
- **Readiness public signatures найдены:** controller `IsVONDisabled/GetVONComponent/GetVONEntries/GetVONEntryCount/GetActiveEntry`, entry `GetVONMethod/IsUsable/IsActive`, radio entry `GetTransceiver/GetTransceiverNumber/GetGadget/IsLongRange/GetIsMuted` — [controller API](<C:/Program Files (x86)/Steam/steamapps/common/Arma Reforger Tools/Workbench/docs/ArmaReforgerScriptAPIPublic/html/interfaceSCR__VONController.html#L129-L140>), [entry API](<C:/Program Files (x86)/Steam/steamapps/common/Arma Reforger Tools/Workbench/docs/ArmaReforgerScriptAPIPublic/html/interfaceSCR__VONEntry.html#L131-L151>), [radio entry API](<C:/Program Files (x86)/Steam/steamapps/common/Arma Reforger Tools/Workbench/docs/ArmaReforgerScriptAPIPublic/html/interfaceSCR__VONEntryRadio.html#L128-L151>). Non-null/атомарность/runtime readiness не следуют из наличия API; ALIVE отдельно от unconcious/wake-up state.
- **Минимальная новая source-дочитка:** public `bool IsUsingLRR()` — [API с описанием только Using long range radio](<C:/Program Files (x86)/Steam/steamapps/common/Arma Reforger Tools/Workbench/docs/ArmaReforgerScriptAPIPublic/html/interfaceSCR__VONController.html#L924-L940>). Тело getter и равенство long-range toggle не прочитаны; сначала тело этого getter и binding getter call-sites/semantics, не новый protected accessor. Новые reader команды в этой редакции не разрешены.
- **Callback layers различаются:** native RX имеет пять аргументов, `m_OnReceivedVON` invoker — четыре `(playerId, receiver, frequency, quality)` без `isSenderEditor`; [source evidence](<../evidence/E-workbench-source.md#L134-L136>) и [локальный handler reference](<../../tools/RICON_LiveProbe/Scripts/Game/RICON_Live/RICON_LiveCommunicationDiscovery.c#L1233-L1252>). Не объявлять invoker-подписку native override и не вызывать callbacks вручную.
- **Ref-ledger не закрыт:** [I0 поля](<../../tools/RICON_LiveProbe/Scripts/Game/RICON_Live/RICON_LiveVoNMappingInspection.c#L60-L68>) уже имеют восемь engine-reference slots плюс budget object; добавятся input manager и invokers для detach. Нужно различать slots, aliases, уникальные refs, tool objects и временные arrays. Это не доказательство невозможности компактного дизайна, но прямой reuse не подтверждает cap8. Binding/entries bulk API не имеют доказанного pre-allocation bound.
- **Незакрытые реальные блокеры:** Q2–Q7; Q1 public signatures найдены, lifecycle/delivery ещё не квалифицированы. Q8 подтверждения отсутствуют. Результат inventory — частичное DOC/SOURCE evidence, не снятие implementation/runtime gates.

### 3.2 Следующий исполнимый срез по типу действия

| Тип | Сейчас можно делать | Отдельное условие продолжения |
|---|---|---|
| Offline, без процессов игры | Читать уже сохранённые source/API/evidence; закрыть inventory и независимо проверить этот DRAFT; сформулировать исправление timing-контракта и границу provenance для Lead | Не превращать документацию/модельные checks в runtime qualification |
| Новые source-наблюдения | Только составить адресный список недостающих getters/listener/binding/lifecycle окон | Перед WbReader/ScriptEditor — точный read-only scope/команды, текущий допуск и ownership; здесь запуск не разрешён |
| Preflight-инструмент | Спроектировать минимальный отдельный контур проверки clock mapping/notification/focus без PTT | Собственные bounded plan/review/implementation и process gates; K0 не доказывает собственные preconditions круговым образом |
| Физическое действие человека | Подтвердить возможность быть оператором и собственный микрофон; B-output отдельно до слышимости | DOWN/UP только в будущем явно разрешённом K0 runtime, после всех guards; сейчас не нажимать PTT |

## 4. Предложенные будущие файлы и непересекающиеся scopes

Имена ниже — **новые предлагаемые файлы, сейчас их нет**. Это границы будущего implementation gate, а не факт реализованного API. Перед открытием gate список уточняется по Q1–Q7 и проходит review.

| Scope / будущий исполнитель | Файлы относительно tools/RICON_LiveProbe/ | Назначение |
|---|---|---|
| Enforce | `Scripts/Game/RICON_Live/RICON_LivePhysicalPTTQualification.c` | Passive listener/capture/RX lifecycle, local-reference readiness, раздельный bounded журнал и verdict |
| Enforce | `Scripts/Game/RICON_Live/RICON_LivePhysicalPTTQualificationGame.c` | Отдельный opt-in bootstrap/default-off, без изменений D0/V0/I0 |
| Enforce | `Scripts/WorkbenchGame/RICON_LivePhysicalPTTQualificationCheckPlugin.c` | Чистые проверки phase/generation/quota/verdict/detach, без мира/input/VoN activation |
| Ops, назначается отдельно | `ops/k0-physical-ptt-runner.ps1`, `ops/k0-watchdog.ps1` | Новый focus-safe runner и независимый фазовый watchdog только после Q5/Q6 |
| Ops/QA, непересекающиеся файлы | `ops/k0-contract-check.ps1`, `ops/k0-evidence-check.ps1` | Чистые негативные/граничные сценарии и проверка сырых журналов без game processes |

Ни существующий runner/lib, ни I0 accessor, ни production addon этим списком не открыты для записи. Если для long-range необходим новый accessor, сначала квалифицировать безопасность и отдельно обновить exact scope; нельзя молча добавлять protected access. Существующие callbacks допускаются только как source reference, не как готовый K0 observer. Новые diagnostic RPC/IPC не переносят тестовое аудио/маркер и не становятся data plane.

## 5. Порядок срезов и gates

| Срез | Зависимость / результат | Проверка / evidence | Текущее состояние |
|---|---|---|---|
| P0 | Q1–Q8 inventory; независимое review против spec v0.2 | source locations, limits, blocked reasons в этом плане; точная редакция и SHA | IN_PROGRESS |
| P1 | Все implementation prerequisites Q1–Q7 доказаны; recorded reviewed plan + явное limited implementation решение координатора под799 | Нет guessed API, budget/reference ledger закрыт; отдельный gate в Team и state | BLOCKED |
| P2 | Только новые разрешённые TOOL_ONLY файлы, default-off; production byte-stable | Полное чтение diff, forbidden-call scan; чистые tests и внешние checks | NOT_RUN |
| P3 | P2 review → отдельный headless compile/pure gate | ScriptEditor без мира/K0-runtime flag; raw hashes, CRC, SCRIPT(E)=0, все заданные checks PASS | NOT_RUN |
| P4 | P3 + Q8 + runtime qualification Q2–Q6 на точном новом стенде; operator и process owner назначены явно | reviewed preflight receipt, focus/binding/provenance/Lnotify receipts и START | BLOCKED |
| P5 | Отдельный explicit one-attempt runtime gate | 1 DOWN→UP на A, ≤2 собственных игровых процессов, сбор A/B/raw/watchdog/operator/cleanup | NOT_RUN |
| P6 | Полные raw journals/hash/terminal + независимое review | Отдельные input/capture/RX/release/cleanup verdicts без повышения до audio/transport | NOT_RUN |

Q8 может оставаться runtime-блокером при планировании, но не позволяет пропустить Q1–Q7 до implementation. Если квалификация конкретного runtime-свойства требует отдельного инструмента/запуска, он получает собственный bounded preflight plan и gate; P1 не открывается круговым утверждением «будущий K0 сам это докажет».

## 6. Проверки будущей реализации

Точные plugin/CLI команды фиксируются до соответствующего compile/runtime gate, после наличия файлов; сейчас команды запуска не исполняются. Шаблон ScriptEditor уже описан в [TOOL_ONLY README](<../../tools/RICON_LiveProbe/README.md#L104-L118>), но прежнее имя plugin и разрешение не являются K0 compile gate.

Чистые проверки новых файлов после implementation gate: `& tools/RICON_LiveProbe/ops/k0-contract-check.ps1`. Проверка evidence после отдельно разрешённого опыта: `& tools/RICON_LiveProbe/ops/k0-evidence-check.ps1 -Manifest <own-run-manifest>`. Это будущие интерфейсы, не существующие команды или доказательство выполненных тестов.

| Spec / предмет | Обязательные cases / ожидаемый результат |
|---|---|
| §3–5 readiness | missing/null/две null не MATCH, mismatched refs/identity/generation, no usable active entry, binding/context/toggle/provenance UNKNOWN → no ARM |
| §6 phases | ровно DOWN→UP, UP-before-DOWN, duplicate/late edges, wrong action/generation; после ABORT только diagnostics, no combined PASS |
| §6/10 clocks | HELD1900ms — дефектный legacy baseline spec v0.2, **не acceptance будущего runner**. После отдельно reviewed timing-spec correction и revised plan проверить исправленные параметры и полный бюджет Lnotify+Theld+Ldispatch+clockError; затем атомарную замену таймеров, disarm только по qualified UP, normal POST_UP и несбрасываемый observation deadline. Самовольно числа не менять |
| §7 anomalies | focus loss, rebind, foreign writer, direct/null/mismatch, clock anomaly, GAP/overflow → immediate INVALID/containment |
| §8 quotas | snapshots240/raw128/reserve32/total400; первая over-cap raw запись сохранена в reserve; 4096 UTF-8 bytes/256 chars/16 args; нехватка reserve/terminal → INCONCLUSIVE_LOG_INTEGRITY |
| §8 refs | peak live distinct refs/arrays по каждому phase/callback, включая временные; cap8/роль не заявлять по одному final count |
| §9 verdicts | self-report ≠ listener ≠ capture ≠ RX; RX ≠ audible/causal/transport; UP/exit ≠ native release; cleanup оценивается отдельно |
| §10 cleanup | zero own subscriptions normal; deadline/hang сначала команда process containment, без ожидания detach; only held owned handles; incomplete terminal → INCONCLUSIVE_CLEANUP |
| §11 isolation | без opt-in нет K0 listener/ARM; старые D0/V0/I0 flags не включают K0; Core/старый runner/lib не меняются |

Запись результата не зависит от того, положительный он или отрицательный: все попытки/ошибки и первый over-cap event сохраняются; retry запрещён. Физический hold target1s, max45s observation и ≤10s teardown после команды — ограничения spec, не измеренные native гарантии.

## 7. Риски, исключённые альтернативы и cleanup

**Что можно сломать:** не тот input context/окно, чужой hold, fallback на direct, смена toggle между DOWN/UP, неполный radio binding, потеря событий/часов, буферизация notification, завершение не своего процесса. Поэтому UNKNOWN запрещает ARM; автоматический ремонт state не допускается.

**Наиболее рискованное место:** реальный DOWN до доказанной доставки внешнему watchdog, затем UP с заново вычисленным типом. Единственные меры этого контракта — квалифицированный bound, независимые fallback/deadlines, контролируемый физический UP и containment собственных процессов; это не доказательство native release.

**Отклонено:** SetActionValue (семантика edges не доказана), protected handler wrappers, SetCapture/selector/entry/transceiver/frequency/mute setters, null-only baseline как ownership, noFocus runner как physical runner, готовый audio observer с отбрасыванием null, schema-only fixture вместо delivery, отдельный RPC вместо штатного media path. Новая бинарная RE/DLL/драйверы и обход подписей не входят в этот план.

Миграции/деплой/изменение Windows audio отсутствуют. До runtime нет состояния, которое надо возвращать. После будущего опыта: нормальный POST_UP → остановка ordinary logging → снятие своих подписок → terminal → завершение только удержанных собственных процессов. При deadline/hang команда containment не ждёт detach. Собственные scratch profiles/logs сохраняются как evidence, автоматически не удаляются; чужие профили, процессы и состояния не трогаются.

## 8. Execution ledger и handoff

| Выполнено этим срезом | Проверено | Не выполнено |
|---|---|---|
| Восстановлены spec/intent/предыдущий план/evidence и ops baseline; составлен qualification-first DRAFT | spec SHA совпал; исходное дерево чистое; game/Workbench0; runner/lib прочитаны в релевантных окнах | K0 implementation, compile, runtime, физический input, capture/RX, audio/RICON proof |

Следующая передача: читать вход `dd6cfdf`, этот DRAFT, spec v0.2, source evidence и Q-матрицу; закрыть точными источниками Q1–Q7 и независимым review. До этого **IMPLEMENTATION_BLOCKED / RUNTIME_BLOCKED**. Если Q6 или provenance не квалифицируются в текущем контракте, зафиксировать технический стоп и минимальную spec-поправку/альтернативу на review, не строить видимость исполнимого плана. FRESH_SESSION_WAIVED действует в пределах текущей задачи; нового согласования общей цели не требуется.
