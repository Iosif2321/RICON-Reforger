# D1 — предложение bounded physical K0 plan

Версия 0.1, 2026-11-02, узкая correction V1-B10/Lead2885: reason→reference и независимый manual UP. Автор: Ядро / Логика / Последовательность; accountable: Lead. **DRAFT_PROPOSAL / V1_DELTA_REVIEW_PENDING / SPEC_GATE_BLOCKED / IMPLEMENTATION_BLOCKED / RUNTIME_BLOCKED**. Текущий write-scope — только этот proposal и K0 REQ; actuation docs/V1 report/N0/addons/tools/intent/evidence/metadata read-only. Числа/caps неизменны; B01…B09, refs12>8 и UNKNOWN не сняты.

## 1. Контекст и границы gate

Baseline `a04a14f`, Git — источник редакций, Team task:02bf7e6e-41c2-44dd-a991-4e037ff33e5a — назначение doc-only D1. Входы: [intent v0.2](<../req/INTENT-local-radio-slice.md>), [K0 v0.4 DRAFT](<../req/REQ-standard-ptt-K0.md>), [D2](<../evidence/E-k0-von-qualification.md>), [D3](<../evidence/E-local-audio-readiness-options.md>), frozen [N0 plan](<PLAN-standard-ptt-K0.md>). Новые D1/D2/D3 ещё не recorded approved inputs; точные SHA фиксируются в handoff. Этот документ — предложение для V1, **не переход к implementation planning из незаписанной спецификации**. После reviewed/recorded spec нужен отдельный planning gate и подтверждение этой редакции либо её корректировка до executable plan. Коммит не разрешён.

Исторический Team799 waiver закрытой task:ba901e07-814a-4803-a1ae-3655d281a6b7 не переносится: **FRESH_SESSION_REQUIRED**, не PROVEN/WAIVED. Нынешнее doc-only назначение не открывает code/pure/compile/process/input. Свежесть, approval, immutable record и runtime permission проверяются отдельно. Применены ai-native-sdlc-cycle/artifact-contracts, code-quality-loop, docs-manage; graphify-граф отсутствует, генерация вне scope; версии навыков не объявлены. Корневой AGENTS отсутствует, прочитан [родительский](<../../../AGENTS.md>); Arma 3 PBO workflow к Reforger не применяется.

**Результат среза:** только квалификация физического input→capture→RX в одной попытке. Не audio/transport/radio-only/release/RICON PASS. N0 остаётся неизменным самостоятельным neutral host-preflight; его gates и consumed D0/V0/I0 не наследуются.

## 2. Exact proposed manifest и непересечение

Все ниже — **новые предлагаемые** файлы, сейчас не созданы. Единственный будущий writer I1 — Enforce-разработчик, V3 — независимый read-only reviewer. Scope I2 не пересекается.

| Предлагаемый path | Единственная обязанность |
|---|---|
| `tools/RICON_LiveProbe/Scripts/Game/RICON_Live/RICON_LivePhysicalPttProbe.c` | default-off passive input/capture/RX observer + bounded queues; новый logger сохраняет null/direct/raw refs, не наследует фильтрацию старого observer; pure phase/quota/verdict functions |
| `tools/RICON_LiveProbe/Scripts/Game/RICON_Live/RICON_LivePhysicalPttGame.c` | отдельный guarded TOOL_ONLY bootstrap, A/B role/readiness/lifetime, detach; только getter/subscription, никаких setters или long-range accessor догадкой |
| `tools/RICON_LiveProbe/Scripts/WorkbenchGame/RICON_LivePhysicalPttCheckPlugin.c` | отдельный явный offline check plugin; fake events только в pure model, не вызовы invoker/OnCapture/OnReceive игры |
| `tools/RICON_LiveProbe/ops/k0-physical-run.ps1` | default-off fixed launcher/external watchdog; definitions-only pure model adapter для selftest; нет arbitrary executable/PID/command text, legacy lib не импортирует |
| `tools/RICON_LiveProbe/ops/k0-physical-selftest.ps1` | EnableK0/Mode Pure, deterministic fake ticks/wire/ownership; без новых процессов, файловой записи, game/input/audio/clock sampling |

Existing observer, I0 accessor, launchers/libs, N0 scripts, addon.gproj, `addons/**`, profiles/settings read-only. Если bootstrap/plugin нельзя подключить в этих пяти файлах без изменения project/старого launcher, **BLOCKED_BOOTSTRAP_SCOPE** и новая manifest-ревизия через Lead; не разрешение править шестой файл. Другой файл, getter/accessor или IPC adapter сначала согласовать, затем review/record, не добавлять во время implementation.

## 3. Нормативные interfaces — это предложения, не существующие API

### CLI и startup

Предлагаемая публичная поверхность нового runner: `-EnableK0 -Mode Physical -RunId <32 lowercase hex> -OutRoot <existing local directory> -ReadinessReceipt <absolute existing JSON>`. Без EnableK0: только `K0_DISABLED`, без clock/file/process/UI effects и без обязательных параметров до disabled guard. Неизвестные args/mode/fields отвергаются до effects. Нет `-Force`, timeout overrides, arbitrary exe/world/profile/PID/seed, auto-input или compatibility modes. Selftest принимает только `-EnableK0 -Mode Pure`. Dot-source runner из selftest загружает только definitions; implementation review должен доказать это до любого pure invocation.

Предлагаемые новые game параметры: `-riconPhysicalPtt=1 -riconK0Role=A|B -riconK0Run=<32hex>`. Ровно обе fresh собственные роли; A — listen-host Workbench game, B — отдельный Diag client, с разными fresh scratch profiles/log roots и строгой текущей player/process/world provenance. Пока точные engine startup args, видимый A HWND, bounded startup readiness и способ запуска нового bootstrap **не квалифицированы**, runner обязан отказать до Process.Start. Приём этих параметров сам не запускает Play/ARM.

K0 взаимоисключён с I2 и всеми старым emitter/observer/bootstrap режимами, включая `riconCommunicationDiscovery`, `riconVoNInputShape`, `riconVoNMapping`, `riconAudioObserve`, `riconEndpointProbe`, `riconBenchPluginStart`. Отказ при наличии любого другого RICON runtime opt-in, а не только известного списка. Никакого старого inspect/launcher side effect. Legacy runner передаёт noFocus A и B: не использовать его CLI как physical preset. Новый запуск A не содержит noFocus, но **отсутствие noFocus не доказывает фокус**. B не должен перехватывать фокус. Qualified passive focus observation сопоставляет exact HWND с held A handle, не заголовком окна. Если такого API без нового неразрешённого bridge нет — BLOCKED_K0_FOCUS_RUNNER. Никаких SetForegroundWindow/UIA/SendInput/SendKeys.

### Evidence wire / clock

На каждую роль — собственный bounded append-only `k0-events.jsonl` в fresh run-dir; runner-only `manifest.json`, `watchdog.jsonl`, `receipt.json`; operator receipt отдельный. Это **предложение диагностического канала**, не квалифицированная FileIO API и не data plane. Records schema1, exact known keys, run/attempt/role/PID/process-start identity, monotonically increasing localSeq, binding/subscription generations, eventKind, raw localClock/frequency, phase, bounded raw args, classification/reason. Объектные ссылки преобразуются в локальные diagnostic IDs до сериализации, не общие A/B pointers. Никаких payload, waveform, выбранной фразы, keys/seed или per-symbol control B. Операторский DOWN_MARK/UP_MARK — control/self-report, не game edge.

Frame ≤4096 UTF-8 bytes, строка заканчивается LF, complete frame до parse, strict schema/UTF-8/seq/run/generations; malformed/partial/unknown/duplicate/GAP/late → INVALID. Runner reader chunk≤4096 bytes, pending≤4096, cap до allocation; нет общего unbounded tail/Get-Content -Raw. Одно чтение с несколькими frames ставит отдельные receive marks, не kernel arrival. Не использовать wire transport N0 как доказанный Enforce→watchdog transport: game FileIO/flush/sharing/visibility, callback allocation/blocking и identity остаются D2/отдельным qualification blocker. Если диагностическая запись может задержать safety path без bound — physical runtime закрыт. Named pipes/network/native bridge/DLL не вводить как обход.

Часы игры локальные monotonic с известной resolution/wrap; watchdog QPC только после квалификации. Нужна interval mapping `[cMin,cMax]` в watchdog scale, с provenance/age/frequency и общим uncertainty≤50ms на 45s. Никакого прямого вычитания GetTickCount/QPC или game A/B timestamps. Без source+independent verification mapping и notification **BLOCKED_CLOCK_WIRE_CONTRACT**, не подстановка host N0 clock.

## 4. Readiness и исчерпывающий ref/allocation inventory

До ARM все [K0 blockers §3](<../req/REQ-standard-ptt-K0.md>) закрываются отдельной evidence; D2 подтверждает только часть source. Effective profile/device/preset/index, все alternatives/filters/conflicts и active context неизвестны. `IsUsingLRR` не toggle. Новый accessor не выбран; существующий I0 accessor не менять. Positive exclusive physical writer, отсутствие другого hold/writer и manual UP должны квалифицироваться отдельно; пустой baseline не доказательство. Fresh-role fixture сама по себе не даёт native ownership.

Предварительный inventory показывает, почему refs≤8 **пока не доказаны**:

| Candidate tool-held engine reference | Количество при прямом дизайне/роль | Lifetime/вопрос |
|---|---:|---|
| world + player controller + controlled entity | 3 | readiness→detach, world может заменяться value ID только после доказательства lifetime |
| SCR_VONController + собственный SCR_VoNComponent | 2 | до unregister, component owner/reference match |
| input/action manager | 1 | listener registration/removal на том же manager |
| active radio entry + gadget + transceiver + BaseRadioComponent | 4 | readiness/reference matching; aliases не считать освобождением без code proof |
| capture invoker + RX invoker | 2 | subscriptions→unregister, возможно отдельные Managed refs |
| **Кандидатный retained total** | **12 > 8** | это blocker, не разрешённый новый cap |
| temporary entry array + все его elements | UNKNOWN | bulk GetVONEntries не вызывать без pre-allocation bound |
| temporary keyStack/filterStack и их elements | UNKNOWN | GetActionKeybinding allocation bound отсутствует |
| callback arguments/borrowed refs, временные casts | bounded count UNKNOWN | нельзя retain raw refs в history; нужна lifetime/peak карта |

Input manager может совпадать с action manager, entry/transceiver может alias, но это не предполагается. Нужен code-level lifetime/alias граф с peak≤8 **включая invokers и временно переживающие callback ссылки**, zero retained refs после detach, и отдельная оценка non-engine DTO/arrays. Перевод world/ref в value ID, borrowing или повторный lookup без dangling допускается только после D2 qualification. Сериализация refs не лечит allocation. Post-count/обрезка массива не ограничивает уже случившийся bulk allocation; если безопасного bounded чтения нет — весь I1 implementation blocked. Cap не характеризует native-internal allocations.

## 5. Автомат, timing, quotas и containment

Одна попытка: DISABLED→READINESS→ARMED→PENDING_DOWN→HELD→POST_UP→TEARDOWN→TERMINAL; любая ошибка → ABORT→TEARDOWN без retry/rebind/re-ARM. Baseline≥3s, absolute observation due45s от единственного tObservation, snapshots≤5Hz/no catch-up; ARMED5s; PENDING due1000ms и fallback due1600ms от operator mark; целевой физический hold1s; post-UP3s. Реальный DOWN может быть позже mark — физическая длительность из него не выводится.

Полный [timing contract §10](<../req/REQ-standard-ptt-K0.md>): N100 + H1500 + D150 + E50 + R200 =2000ms; R не расходуется. heldDue=tReceiveDown+1500ms, не tCheck; для scheduled HELD parse/decision/API включены в D по lastCommandReturn−heldDue; immediate faults используют ранний reference таблицы ниже, не будущий heldDue. Callback→receive не доказывается N0. Pre-DOWN fallback остаётся до атомарной установки HELD; equality due приоритетна, qualified UP должен быть полностью проверен раньше due. Late/duplicate/generation mismatch не disarm. Absolute deadline никогда не reset. Все thresholds лишь требования конкретного опыта, не hard real-time гарантия. Неохваченный OS/IO/scheduler/watchdog hang = неустранённый safety blocker до ARM.

На роль сохраняются отдельные заранее ограниченные каналы: snapshots240, raw128, reserve32 (4 safety/overflow,16 post-abort,12 detach/terminal/cleanup); ≤400×4096=1,638,400 serialized bytes. Raw callback args≤16, strings≤256 characters и общий byte cap; никакого truncation для positive evidence. 241-я snapshot/129-й raw целиком помещается в первый reserve overflow record; если не помещается/oversize — bounded diagnostic с rawComplete=false, INCONCLUSIVE_LOG_INTEGRITY, не lossless PASS. После первого overflow новые ordinary запрещены, diagnostics≤16. Память bounded buffers + DTO inventory фиксируется до GO. Внешний watchdog журнал≤128×4096=524288 bytes, manifest/receipt/operator≤16384 каждый, run protocol files≤4MiB; native engine console logs не названы этим cap и требуют собственного квалифицированного stop/quota, иначе runtime blocked.

Не более2 игровых процессов (Workbench/host и Diag), runner1, дополнительный watchdog helper максимум1, если его независимость/held handles/launch path квалифицированы отдельно; children tools0, engine child требует revised scope. Startup due120s от одного external start, отсутствие готовности до него → teardown/no ARM; observation45s, process-exit target≤10s после containment, planned series≤175s без setup/review. Это deadlines/targets, не гарантированный wall-time при OS hang. Handles удерживаются при создании, PID+StartTime+executable проверяются; нет attach/name/global kill, orphan search или завершения чужой игры. Независимость watchdog от runner/игры и безопасность его запуска **UNKNOWN** до отдельной реализации/review; без неё не ARM.

При abort обязанности независимы: внешний containment немедленно выдаёт команды только удержанным собственным процессам по reference/budget ниже, **не ждёт manual UP, operator ACK, человека, логов, detach или exit первой роли**. Оператор независимо отпускает только свою клавишу в исходном контролируемом A; при потере фокуса не посылать UP в другое окно и не возвращать фокус автоматически. Ручной UP не prerequisite/ACK для внешней команды и не откладывает её. Нормально: qualified UP→окно→снять свои listeners/subscriptions→terminal→собственные process exits. Remove false/невозможность подтвердить detach, missing terminal/exit, disk error → INCONCLUSIVE_CLEANUP/LOG_INTEGRITY. Не применять SetCapture(false), global stop, selector reset. Process exit/UP не NATIVE_RELEASE.

### 5.1 Reason → reference для dispatch, в том числе до HELD

Нормативен [K0 REQ §10.2](<../req/REQ-standard-ptt-K0.md>); таблица здесь фиксирует будущий runner interface, не исполнение проверки. Все reference в qualified external clock. До появления своих held handles fault останавливает дальнейший startup/ARM, не разрешает attach/kill чужого PID.

| Fault/deadline class | Saved reference до обработки | Dispatch acceptance |
|---|---|---|
| Scheduled startup/ARMED/PENDING/fallback/HELD/absolute/normal POST_UP | original scheduled due, не wake/check | до последнего успешного command-return **обеих** held game roles ≤150ms |
| Complete frame: malformed/schema/identity/seq/phase/direct/null/mismatch/GAP/clock/quota/ownership-fault | tReceive соответствующего полного frame до decode/parse/validation | validation+decision+оба calls входят в те же150ms; detection после parse не начало отсчёта |
| Partial/incomplete frame/tail fault | earliest bounded read mark первого batch с байтами этого partial frame до inspection | mark не reset новыми chunks; existing deadlines действуют, нового grace/timeout нет |
| Oversize/buffer/file/framing fault | begin mark первого bounded read/check со свидетельством oversize; минимум с ранее сохранённым partial mark | byte/size checks до allocation и оба calls внутри150ms; не поздняя диагностика |
| IO open/read/write/flush error | operation begin до вызова; минимум с tReceive, если операция triggered by frame | blocking/validation/error handling входит в D; unknown hang не covered bound |
| Внешняя capacity/quota/reserve failure | capacity-check begin до serialization/allocation; минимум с tReceive triggering frame | проверки и dispatch входят в150ms, reserve не добавляет время |
| Независимый focus/HWND/held-process ownership/ref-generation/foreign-writer check | begin первого qualified внешнего sample/check, вернувшего нарушение; return/detection отдельно | scope только observation/check→commands, не неизвестное время фактической потери фокуса/ownership |
| Unknown/missing/bad reference | UNKNOWN_REFERENCE/INVALID; не подставлять heldDue либо late detection | немедленный best-effort own containment, но dispatch не PASS |

Внутриигровой fault сообщённый wire относится к complete-frame rule, а не независимо наблюдённому внешнему событию; его occurrence→notification остаётся отдельной квалификацией N/E. Partial fault не становится допустимым от поздней сборки frame; bounded buffering не даёт нового срока. Независимая observation latency focus/ownership до sample пока UNKNOWN/BLOCKED B01/B04. External operation begin/reference не называется реальным временем возникновения native/physical события.

Runner до batch/check и перед каждым call проверяет все active due; reference — минимум уже наблюдённых fault references и уже наступивших deadlines, **не будущий heldDue**. Due наступивший во время parse участвует, fault сохраняет свой pre-parse reference. Primary outcome отдельно: absolute due, затем INVALID fault, затем иной expired safety phase, затем normal POST_UP; при равных invalid reasons — порядок строк сверху вниз. Все дополнительные ошибки sticky. Более поздний primary reason не сдвигает ранний reference; normal completion не маскирует fault, equality due не даёт disarm от UP/DOWN.

Для каждой команды begin≥reference и return≥begin; last successful return обеих требуемых ролей−reference≤150ms. Нет required return/exception/ошибка порядка/unknown held identity → dispatch FAILED/UNKNOWN, даже если другая роль завершена. Ошибка первой не задерживает попытку команды второй ожиданием человека/логов/exit. Call не более одного на каждый held handle за весь run, включая finally/watchdog race, latch before call; ни поздний UP, ни exception не дают retry. При natural exit/no-held-role записать отсутствие command observation по проверенным facts, не dispatch PASS. Cleanup exit/detach/terminal отдельно; недостающие records/неподтверждённые exits остаются INCONCLUSIVE, не стираются удачным command-return другой роли.

Числа N100/H1500/D150/E50/R200 неизменны. N_estimate+E_mapping учитываются однократно по REQ; E — один общий decomposition ошибок измерения, **не дополнительные50ms к D150**, R не расходуется на overshoot. До HELD H отсутствует: immediate fault не ждёт1500ms и проверяет тот же D150 от своего reference. Никакой real-time гарантии или закрытия B01…B09 это не создаёт.

## 6. Порядок будущих slices и точные команды

| Gate / owner | Зависимости | Проверка/выход |
|---|---|---|
| D1→V1 | четыре doc DRAFT, D2/D3 reviewed | reviewed spec с unresolved blockers; без code |
| Recorded spec→planning | утверждение scope Lead, source-of-truth revision, fresh session/новый explicit waiver | executable plan revision либо BLOCKED; данный DRAFT ещё не это |
| I1_WRITE / Enforce | записанные D1/D2/D3, V1, N0 evidence X1/V2, refs/clock/getters/provenance qualification, отдельный GO Lead | только пять файлов §2, readback/diff/SHA, handoff V3 |
| I1_AST/PURE | independent source review, separate exact static/pure GO | pure vectors, не evidence game/OS |
| I1_COMPILE | separate compile GO и точный проверенный launcher | log/build/hash, no Play; команда пока BLOCKED_COMMAND_QUALIFICATION |
| K0_ARM / X1 Lead | V3, readiness всех blockers, approved safety case, current hashes/build/process/focus/Human/source и отдельный START | ровно одна физическая попытка, raw/receipt/cleanup; не аудио/RICON |

Следующие команды — **только проект**, файлы ещё отсутствуют; сейчас не исполнять. Cwd checkout root. Не обходить execution policy, не импортировать legacy lib. AST-проверка после отдельного static gate:

```powershell
$names = @('k0-physical-run.ps1','k0-physical-selftest.ps1')
foreach ($name in $names) {
  $tokens = $null; $errors = $null
  $null = [System.Management.Automation.Language.Parser]::ParseFile((Join-Path (Get-Location) "tools\RICON_LiveProbe\ops\$name"), [ref]$tokens, [ref]$errors)
  if ($errors.Count) { throw "K0_AST_ERRORS $name" }
}
```

Будущий отдельный pure gate в текущей qualified Windows PS5.1 оболочке (не запуск новой PS/игры):

```powershell
& .\tools\RICON_LiveProbe\ops\k0-physical-selftest.ps1 -EnableK0 -Mode Pure
& .\tools\RICON_LiveProbe\ops\k0-physical-selftest.ps1
& .\tools\RICON_LiveProbe\ops\k0-physical-run.ps1
```

Две последние проверки без EnableK0 обязаны дать только K0_DISABLED. Часы/файлы/process APIs selftest не исполняет; pure clocks/process/wire/foreground facts injected. Fixed vectors: disabled/conflicting legacy mode; absent roles/readiness/foreign writer; ref peak8/9 и unknown allocation; null/direct/mismatch; один DOWN→UP; duplicate/UP-before-DOWN; mapping/clock rollback; N100/101,E50/51,D150/151; held1500/equality/late UP; dispatch parse interval; no-reset absolute45s; pending1000/fallback1600 atomic handoff; quota240/241,128/129, reserve/oversize; missing detach/terminal/exit; command-once/ownership/no handle/no kill. Каждый case — Boolean, не truthy array; expected invalid не positive qualification.

**Дополнительные contract boundary vectors V1-B10 — только проект, NOT_RUN:**

| Сценарий во внешних ms | Обязательное ожидаемое правило |
|---|---|
| До HELD: malformed frame receive100/parse-end250/last-return300 | reference100, D200→INVALID_DISPATCH; не D50 от detection и не отрицательное значение от будущего heldDue |
| Reference100, first-return≤last-return249/250/251 | D149/150/151; первые два удовлетворяют только порогу D, третий FAIL без расхода E/R |
| Frame fault reference100, active due200 наступил во время parse до250 | reference100 (minimum), оба reasons sticky; primary reason не reset reference |
| Fault reference100 и due100 одновременно | reference100, absolute due при наличии приоритетен; иначе INVALID выше normal POST_UP; late UP не отменяет dispatch |
| Partial first-read100/oversize-check180/last-return260 | reference100, D160→FAIL; нельзя reference180 и D80 |
| IO/focus/ownership sample-begin100/detection180/last-return249 до HELD | reference100, D149; occurrence time до sample UNKNOWN, не измеренный100 |
| Reference100, manual UP/ACK отсутствует | команды обеим held ролям без ожидания UP/ACK; manual duty остаётся отдельной |
| First call exception/second success либо missing second return; повтор finally/поздний UP | dispatch не PASS при любом missing required success; второй call не ждёт exit первого; command-once per handle сохраняется |

Ни один vector здесь не исполнялся и не квалифицирует wire/OS safety; прежние B01…B09/refs12>8 и UNKNOWN сохраняются BLOCKED. Числа/caps прежние, новых запускаемых команд и разрешений нет.

Будущий runtime CLI **зафиксирован по форме, не executable authorization**: `& .\tools\RICON_LiveProbe\ops\k0-physical-run.ps1 -EnableK0 -Mode Physical -RunId <32hex> -OutRoot <existing-local-root> -ReadinessReceipt <absolute-json>`. До START X1 должен записать exact concrete substituted command, reviewed manifest/SHA, startup engine arguments/executables, собственные profiles/log paths и focus/watchdog qualification. Значения пока UNKNOWN → **BLOCKED_EXACT_RUNTIME_COMMAND**; выдавать сейчас выдуманный runnable START нельзя. ReadinessReceipt — schema1/нет unknown fields, hashes/build/run/role identities, qualified interfaces, input ownership/focus/manual UP consent, source/output branch, timing safety case и V3 decision; содержимое не создаёт согласие само по себе.

## 7. Acceptance, риск и следующий owner

| LS | Роль K0 / достаточность | Сейчас |
|---|---|---|
| LS-01 | raw A/B/radio refs помогают, не полный authority binding | PARTIAL исторически, K0 NOT_RUN |
| LS-02 | физический источник/отличимое содержимое отдельны, operator mark недостаточен | BLOCKED D3 |
| LS-03 | RX callback не post-gate звук/protocol/content | BLOCKED D2/D3 |
| LS-04 | одна K0 попытка не серия delivery controls | NOT_RUN |
| LS-05 | K0 не actuator/RICON | BLOCKED, отдельный D1 actuation |
| LS-06 | direct/null diagnostics не direct-preservation test | BLOCKED |
| LS-07 | новые terminal/zero subscriptions/own exits, не release | NOT_RUN |
| LS-08 | все raw/SHA/gens/attempts, thresholds и exact cmds | NOT_RUN, документ не evidence |

**Что сломается:** ложная effective binding, unexpected direct fallback, другой автор удержания, ref/allocation overflow, задержанная запись/dispatch, потеря фокуса или input UP, process ownership gap. **Самый рискованный шаг:** physical DOWN при недоказанных native ownership/release и независимом containment. Он не открывается от хорошего N0. **Отклонено:** synthetic input/protected wrappers, SetCapture/global stop; механический reuse noFocus runner/old observer/I0 bulk arrays; 100+1900 без запаса; hidden receiver filtering и универсальная Windows guarantee. Причина — нарушение безопасности либо недоказанный вывод, не удобство реализации.

Ledger D1: docs proposed; code/AST/pure/compile/physical/input/audio NOT_RUN. Migration/deploy отсутствуют; rollback — не включать default-off инструмент, не менять existing files/профили, сохранить evidence, убрать только свои subscriptions/handles после разрешённого опыта, ничего не удалять.

Handoff V1: читать baseline a04a14f и четыре D1 документа, D2/D3 по handoff SHA, parent AGENTS. Проверить ref/allocation infeasibility, clock budget/wire/focus/ownership и отсутствие открытых executable gates. Выход — замечания либо reviewed DRAFT с явными blockers, не PASS физического K0. После записи/утверждения Lead заново проверяет planning/fresh-session gate, затем только отдельный I1 GO. Никаких процессов, ввода, settings, source extraction или commit этим handoff не разрешено.
