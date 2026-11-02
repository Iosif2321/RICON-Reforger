# Bounded plan — K0 / N0 neutral host-preflight

Версия 0.3, 2026-11-02. Автор: Ядро / Логика / Последовательность. Accountable: Lead / архитектор. **DRAFT / N0_CORRECTIVE_REVIEW_PENDING / N0_EXECUTION_BLOCKED / PTT_BLOCKED**. Точечная reviewed correction по2601/2613: wire representation и exact negative acceptance; caps/CLI и границы gates не меняются.

## 1. Входы, решение и единственный текущий write-scope

- Git — источник редакций; Team task:ba901e07-814a-4803-a1ae-3655d281a6b7 — источник решений. Вход этой поправки — frozen plan v0.2 на `2880204` (исходный planning baseline `8b56b5e`); до отдельной фиксации v0.3 этот текст не recorded revision.
- [INTENT v0.2](<../req/INTENT-local-radio-slice.md>) неизменен. [Spec K0 v0.3 §11.1](<../req/REQ-standard-ptt-K0.md#L209-L224>) на входе8b56b5e, SHA-256 `ddc9ac1302ab3d08086eafc238dcff13f499c46cd0c7ed1001d41af4fe4c9f22`, разрешает только отдельное preflight planning. Поручение Lead2494 задаёт N0 без Workbench/игры/ввода/VoN/аудио/фокуса.
- Утверждённая цель680 и автономия799 не открывают writing/pure/process gates автоматически. **FRESH_SESSION_WAIVED**, не strict clean-session compliance. Применены `goal`, `ai-native-sdlc-cycle`/artifact-contracts, `code-quality-loop`, `docs-manage`; версии навыков не объявлены.
- Текущий author-scope — **только этот существующий план**. Metadata/changelog, spec, source evidence, код, commit и процессы N0 не входят. Предыдущий qualification-only v0.1 сохранён в `d5018c9`; настоящий текст заменяет его как активный план следующего среза, не меняя PTT requirements.
- На исходном planning baseline8b56b5e дерево было чистым; текущая v0.3 правится параллельно отдельному G_WRITE corrective, чужие code-файлы автор плана не меняет. Runtime snapshot Lead2507: Windows10.0.19045, Windows PowerShell5.1.19041.6456 Desktop, IsHighResolution=true, Frequency10000000. Автор независимо сверил PS5.1/path/policy: CurrentUser=RemoteSigned, остальные scopes Undefined. Snapshot — возможность API, не измерение доставки или dispatch.

## 2. Результат N0 и строгие исключения

Первый конечный исполнимый срез после отдельных gates: **один новый собственный консольный helper посылает только нейтральные записи своему runner на том же Windows-host; runner измеряет наблюдённую доставку и вызов завершения именно этого helper, затем проверяет exit и сохраняет evidence**. Никакого моделирования физического DOWN/UP. Fake данные имеют префикс N0 и никогда не подаются игре.

N0 проверяет механику bounded parser, общей raw QPC-шкалы, deadline decision и process ownership; не доказывает игровой callback→IPC, input provenance, binding, hold/release safety, media path или real-time bound. Реальный фокус — **NOT_RUN / OUT_OF_SCOPE**: не читать/менять foreground HWND, не создавать окно-приманку, не посылать ввод, не переключать UI.

Запрещены: Workbench/игра/Enforce plugin, VoN/radio/audio/микрофон, UIA/SendKeys/SendInput, network/remote endpoints, arbitrary executable/script/PID, подключение к уже работающему процессу, shell-command payload, DLL/driver/Add-Type, настройка приоритетов/timer resolution/affinity, execution-policy bypass/изменение Zone.Identifier, удаление/перезапись evidence. Старые D0/V0/I0 runner/lib не импортируются и не меняются.

## 3. Минимальный будущий code scope и интерфейсы

Четыре файла ниже — исходный G_WRITE scope v0.2 только в `tools/RICON_LiveProbe/ops/`; они уже написаны2570, core+selftest проходят corrective2608/2613. Это не execution PASS и не открытие G_PARSE/G_PURE/G_RUN. Один назначенный Lead разработчик владеет четырьмя файлами: API-зависимости не делятся между параллельными авторами. QA имеет read-only review, не правит тесты ради PASS.

| Новый файл | Публичный контракт / разрешённые действия |
|---|---|
| [k0-neutral-core.ps1](<../../tools/RICON_LiveProbe/ops/k0-neutral-core.ps1>) | Только определения pure functions `New-N0State`, `Test-N0Record`, `Update-N0State`, `Get-N0DeadlineDecision`, `Get-N0Verdict`; входы передаются явно (ticks/frequency/state/bytes). Dot-source ничего не запускает/не пишет и не читает часы/процессы. Возвращает решение, не выполняет Kill |
| [k0-neutral-helper.ps1](<../../tools/RICON_LiveProbe/ops/k0-neutral-helper.ps1>) | `-EnableN0 -RunId <32hex> -RunDir <own-directory> -OwnerToken <32hex> -Scenario Normal|DelayEmit|Hanging`. Только фиксированный генератор, bounded wire-file и собственный выход; ни children, ни произвольные аргументы команд |
| [k0-neutral-preflight.ps1](<../../tools/RICON_LiveProbe/ops/k0-neutral-preflight.ps1>) | `-EnableN0 -Scenario Normal|DelayEmit|Hanging -OutRoot <existing-local-directory>`. Создаёт один fresh run-dir, запускает только соседний helper фиксированным powershell.exe, удерживает handle, наблюдает wire/deadlines, завершает только helper, пишет receipt |
| [k0-neutral-selftest.ps1](<../../tools/RICON_LiveProbe/ops/k0-neutral-selftest.ps1>) | `-EnableN0 -Mode Pure`. Только deterministic vectors/core и bounded stdout; fake clocks/byte buffers/ownership facts, без реального процесса, QPC sampling, файловой записи и OutRoot |

При отсутствии EnableN0 helper/runner/selftest завершаются с `N0_DISABLED` без directory creation, clocks/IPC/process effects; core остаётся definitions-only. Неизвестные параметры/сценарии отвергаются до side effects. Helper принимается только с проверенным runner-created manifest/token в собственном fresh run-dir; token — идентичность опыта, **не** доказательство защиты от злонамеренного same-user процесса.

## 4. Clock/API contract и механизм обмена

### 4.1 Одна host clock-шкала

Оба участника используют только raw `[System.Diagnostics.Stopwatch]::GetTimestamp()` и `Stopwatch.Frequency`, не отдельные `Stopwatch.Elapsed` origins. На обоих `IsHighResolution=true`, положительная одинаковая frequency и Windows same-host; иначе `N0_BLOCKED_CLOCK`, no positive result. Raw ticks/frequency сохраняются Int64 decimal strings в JSON, не floating-point epoch. Delta вычисляется после Int64 range checks; перевод в ms — через decimal arithmetic с проверкой overflow. Deadlines сравниваются в raw ticks; переход ms→ticks округляется вверх проверенной pure function.

[Microsoft QPC contract](https://learn.microsoft.com/en-us/windows/win32/sysinfo/acquiring-high-resolution-time-stamps): same-system readings согласованы между процессами; frequency постоянна в пределах boot, ±1tick межпоточного порядка неоднозначен. [Stopwatch.GetTimestamp](https://learn.microsoft.com/en-us/dotnet/api/system.diagnostics.stopwatch.gettimestamp?view=netframework-4.8) при отсутствии high-resolution может использовать другой timer — поэтому fallback здесь запрещён. Разность [-1,+1]tick помечается ORDER_AMBIGUOUS, не превращается в доказанную нулевую latency. Меньше -1tick, local clock rollback или frequency mismatch — INVALID_CLOCK. Нельзя экстраполировать эту шкалу на Enforce/GetTickCount, другой host/boot или native callback.

### 4.2 Один bounded file-канал, без pipes/сети

Runner создаёт fresh `ricon-n0-<RunId>` под явно заданным существующим OutRoot; только локальный filesystem, вне checkout, без UNC/reparse points в root/ancestor chain. New-Item без Force и все output files с CreateNew: существующий run-dir/file → отказ, не reuse/overwrite. Никаких удаления/move/cleanup каталогов. Root не создаётся инструментом; evidence остаётся после run.

Файлы только `manifest.json` (runner до запуска), `wire.jsonl` (helper-only), `runner.jsonl` и `receipt.json` (runner-only). Helper проверяет canonical RunDir, manifest run/token/scenario и exact neighbouring-script identity до записи. Manifest содержит hashes всех4 scripts, fixed executable path/version, host/OS/PS, clock source/frequency и run/token. Это ограниченный доверенный локальный стенд, не защищённый IPC от постороннего writer; неожиданные записи инвалидируют опыт.

Wire — UTF-8 без BOM, один JSON object + LF на запись. Helper открывает CreateNew FileStream с FileShare.Read и пишет bounded bytes, затем `Flush(true)`; runner читает FileStream с совместимым FileShare.ReadWrite, bounded chunks≤4096 bytes и pending buffer≤1024 bytes. [Flush API](https://learn.microsoft.com/en-us/dotnet/api/system.io.filestream.flush?view=netframework-4.8) доступен в .NET Framework; flush может блокироваться, его deadline не гарантирован Windows. Получение байтов **не доказывает** завершённый flush/durable disk commit: reader может видеть запись раньше возврата Flush.

**Accepted wire representation v0.3 — ограниченный flat JSON subset, не общий JSON parser и не RFC canonicalization.** Body содержит один плоский object с exact schema keys; ключи/строковые значения — только неэкранированный printable ASCII (0x20…0x7E без literal quote/backslash внутри строки), integer scalar tokens — десятичные Int64 без fraction/exponent/плюса/лишних leading zeros, boolean tokens — только `true`/`false`. Frequency/tEmit остаются строками десятичных Int64 по §4.1, не numeric JSON tokens; остальные типы определяет schema. Между tokens допустим только ASCII space0x20, framing — ровно конечный LF0x0A; BOM, CR/tab/control whitespace, non-ASCII (включая Unicode/ignorable aliases), escapes в ключах/значениях, float/null/arrays/nested objects и неизвестные/duplicate/case-alias keys отвергаются. На совпадение известных строк как замену явному ASCII guard не полагаться. JSON-эквивалентные escaped/Unicode представления не нормализуются и не объявляются поддержанными; fixed helper обязан выпускать этот subset. Это явное ужесточение accepted representation v0.2 по решению2613, без изменения байтовых caps и protocol identities.

Запись распознаётся только после полного LF и strict UTF-8/subset/schema validation. Runner ставит `tRead` сразу после выделения полной bounded строки до JSON parse; если при одном чтении получено несколько строк, каждая получает собственный read mark (это software observation, не kernel arrival). HELLO seq0 содержит `kind=N0_HELLO`, schema1/run/token/helperPid/scenario/frequency/highResolution; SAMPLE seq1…8 — те же identity, `kind=N0_SAMPLE`, `tEmit` непосредственно перед сериализацией, фиксированный `payload=N0_NEUTRAL`; DONE seq9 — те же identity, `kind=N0_DONE`, sampleCount8. В первой реализации нет иных wire fields, optional extensions или arbitrary payload. Порядок строгий, дубликаты/неизвестные fields/kind/run/token/PID/frequency, GAP и early DONE → INVALID_PROTOCOL. Enum schema и byte-limit проверяются до положительного verdict.

Наблюдаемая `tRead−tEmit` включает serialization/file visibility/polling/scheduling; это **не** чистая transport latency и не game Lnotify. В DelayEmit tEmit намеренно ставится до600ms задержки записи: это управляемая fault injection внутри helper, не найденная задержка ОС. Не добавлять echo-передачу полезного содержимого или универсальный executable callback.

## 5. Process ownership, автомат и численные caps

### 5.1 Фиксированный запуск и удержанный handle

Runner использует только `C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe -NoProfile -NonInteractive -File <fixed-sibling-helper> ...` через ProcessStartInfo/Process.Start с UseShellExecute=false, CreateNoWindow=true, без redirected stdio/pipes. Никаких -Command/-EncodedCommand/-ExecutionPolicy, пользовательского executable или script text. Аргументы — только проверенные token/run/scenario и безопасно quoted canonical RunDir. Пути с недопустимыми quote/control chars отвергаются, не «исправляются».

Process object/handle удерживается сразу после Start; записываются PID, StartTime и executable identity. Положительный HELLO должен совпасть с именно этим объектом. Не использовать Stop-Process, GetProcessById повторно, process-name/global/tree kill или чужие PID. Helper не запускает потомков. Start failure/невозможность удержать handle → BLOCKED_OWNERSHIP; запрещён поиск по PID для исправления. Если Process object уже получен, finally использует только его для best-effort teardown. Если handle вообще не получен и существование helper не исключено — INCONCLUSIVE_CLEANUP, остановка всей серии и ручная передача Lead, не автоматическая охота за процессом.

### 5.2 Автомат и timers

`DISABLED → PREFLIGHT → STARTING → OBSERVING → STOP_REQUESTED → EXIT_CONFIRMED → RECEIPT`. Любая ошибка ведёт в `INVALID → STOP_REQUESTED`, no restart/retry. Reserve и held handle сохраняются до receipt. Перед каждым bounded read/parse batch проверяются deadlines; ordinary logging не задерживает вызов containment.

| Лимит | Значение и точка отсчёта |
|---|---|
| Helper processes | Ровно1/команда runner; runner+helper≤2 процесса. Helper children0, попыток1 |
| Absolute observation | t0 берётся один раз непосредственно до Process.Start;10s до stop-command decision, без reset |
| Startup | HELLO должен наблюдаться до t0+5s, иначе INVALID_STARTUP и stop |
| Probe deadline | Один раз `tHelloRead+2000ms`; не reset от SAMPLE/DONE. Всегда минимум с absolute deadline |
| First SAMPLE | До tHelloRead+500ms; absence → INVALID_MISSING_SAMPLE и stop, не native/input вывод |
| Normal generator |8 samples, target spacing100ms без catch-up; затем DONE и ожидание stop/self-cap |
| DelayEmit | tEmit SAMPLE1 → намеренное ожидание600ms → запись; следующий catch-up запрещён |
| Hanging | Только HELLO; не пишет samples, ждёт self-cap |
| Helper self-cap |10s от первого GetTimestamp helper, best-effort на собственном цикле; не защита от зависшей ОС/IO |
| Reader polling | target sleep10ms, без busy-spin; фактический интервал не гарантирован |
| N0 observation thresholds | Для Normal каждый однозначный sample delta≤250ms; dispatch bracket return≤100ms после stop deadline, exit≤5000ms от call-begin; это **критерии конкретного run**, не обещанные bounds |
| Total planned run |10s observation +5s wait/cleanup;15s target до receipt. Системный hang/IO может превысить; факт превышения сохраняется, hard cap не обещается |

First-SAMPLE guard действует только до первого valid SAMPLE и затем снимается; probe/absolute deadlines не меняются. Если сообщений нет, выбирается минимум применимых startup/first-SAMPLE/probe/absolute deadlines; при совпадении действует абсолютный лимит, затем invalid timeout, затем normal probe completion. Для complete-record validation ошибок (malformed/schema/sample-threshold) conservative reference=tRead до parse: parse/validation входят в тот же100ms budget; reference не называется моментом обнаружения. Для ошибок, наблюдаемых вне validation и без заранее scheduled deadline, reference — соответствующий внешний check mark; receipt не подменяет его probe deadline. Существующий wakeTick хранится отдельно как внешний post-validation/check observation, не как точный момент parser instruction; pure core часы не читает.

NORMAL helper остаётся жив после DONE для наблюдения вызова stop на probe deadline. INVALID/negative case вызывает stop раньше. Runner в памяти ставит `tCommandBegin` непосредственно перед `Process.Kill()`, `tCommandReturn` после возврата/exception; затем bounded `WaitForExit(5000)` и отдельный tExitObserved/HasExited. [Kill asynchronous](https://learn.microsoft.com/en-us/dotnet/api/system.diagnostics.process.kill?view=netframework-4.8): возврат не равен exit. Эти метки — bracket API вызова, **не** точный момент kernel command execution. Сохраняются planned deadline, wake/check tick, call begin/return, errors и exit tick; реальный dispatch оценивается консервативно по return-deadline, но только если вызов успешен.

На error/timeout не ждать записи логов/receipt перед Kill. Команда один раз; finally не вызывает повторный Kill, если попытка уже зарегистрирована. Если helper завершился самостоятельно раньше команды, записать NATURAL_EXIT и отсутствие command observation; Normal не получает полный N0 positive verdict. Wait/HasExited failure или отсутствие подтверждённого exit → INCONCLUSIVE_CLEANUP и стоп последующих сценариев. Никакого «exit0 значит PASS».

### 5.3 Физические квоты на run

- Wire≤16 records по≤1024 UTF-8 bytes включая LF, file≤16384 bytes; штатно10 records. Byte-limit до decode/JSON allocation; max object fields16, string field≤128 chars. Unknown/overflow invalidates, строки не накапливаются целиком сверхcap.
- Runner ordinary≤64 records + reserve16 (4 invalid/limit,4 stop/exit,8 receipt/cleanup), каждая≤2048 UTF-8 bytes включая LF; file≤163840 bytes. Ordinary не расходует reserve. Первое превышение — INVALID_QUOTA; дальнейшие ordinary запрещены.
- При oversized/неполной wire записи сохранить bounded diagnostic prefix≤1025 raw bytes в Base64 и observed length/reason в reserve; явно `rawComplete=false`, **не lossless positive evidence**. Нельзя хэшировать/читать бесконечный oversized input ради диагностики. Wire cap не снимается постфактум счётчиком.
- Manifest≤8192 bytes, receipt≤8192 bytes, суммарно все4 файла≤196608 bytes/run. Reader buffer4096+pending1024;8 sample summaries в памяти, без unbounded history. До любого write проверить serialized byte-size; при отсутствии reserve/disk error — INCONCLUSIVE_LOG_INTEGRITY. Точные retained .NET objects/Dispose paths перечисляются в code review; Enforce cap8 сюда не переносится.

## 6. Проверки и ожидаемые verdicts

### 6.1 Pure vectors (не запускают helper)

Selftest получает fake ticks/frequency и byte buffers. Обязательные группы: disabled/no side effects; parameter/scenario whitelist; safe path/quote/reparse/preexisting-output rejection через injected facts; HELLO+8 samples+DONE; seq duplicate/gap/early DONE/wrong run/token/PID/schema/frequency; strict byte limits1023/1024/1025, missing LF/partial UTF-8/malformed JSON; Int64 overflow, backwards clock и ±1tick ambiguity; startup/first-sample/absolute/probe boundaries и no-reset; deadline precedence; reserve16/ordinary64/overflow; serialization size; command once/finally idempotence; no handle/no authority/no kill; natural exit, Kill exception, wait timeout; missing terminal/integrity; separate negative-case expectation versus measured positive verdict. Все имена/число vector cases фиксируются в frozen code, failures не игнорируются.

`N0_PURE_MODEL_PASS` требует все vectors PASS и default-off/no forbidden dependencies. Это не процессное/clock/IPC evidence. Core/test runner не может незаметно открыть файлы или заменить fake process adapter настоящим Process.Start.

### 6.2 Три отдельно разрешаемых process-сценария

| Сценарий | Ожидание | Что не повышается до PASS |
|---|---|---|
| Normal | HELLO+8 samples+DONE valid, clock/quotas целостны, sample deltas≤250ms; стоп на probe deadline, успешный call bracket и exit в run thresholds | При любом gap/ambiguous sample/overshoot/incomplete receipt → INVALID/INCONCLUSIVE, не подбор повторного опыта |
| DelayEmit | SAMPLE1 появится после injected600ms, runner обнаружит violation threshold250ms или first-sample deadline500ms и прекратит helper | `N0_NEGATIVE_DETECTED` только при ожидаемой причине и полном cleanup; **не N0_NEUTRAL_OBSERVED**. Если сработал500ms stop до записи — записать TIMEOUT, не выдумывать sample latency |
| Hanging | HELLO без SAMPLE, first-sample500ms timeout, call bracket и exit собственного helper сохранены | Независимое от новых сообщений timeout decision; не гарантия исполнения при OS hang |

**Exact negative acceptance v0.3 (corrective2601):**

- Общие обязательные условия для `N0_NEGATIVE_DETECTED`: successful однократный command, `commandObservation=OBSERVED_WITHIN_RUN_THRESHOLD` (тот же 0≤return−scheduled deadline≤100ms, begin≥deadline и return≥begin; не новый лимит), подтверждённый exit≤5000ms от call-begin, отсутствие NaturalExit, валидные clock/ownership/final wire/tail/log integrity и **ни одного вторичного disqualifying fault**. StopReason совпадает с ожидаемым InvalidReason; original reason сохраняется отдельно от sticky disqualifying-fault latch. Один successful Kill сам по себе недостаточен.
- **Hanging:** правильный HELLO своего scenario, только1 accepted record, NextSeq1, Samples0, Done=false; ожидается исключительно `INVALID_MISSING_SAMPLE` на FirstDeadline=tHelloRead+500ms, command deadline — этот scheduled FirstDeadline. SAMPLE_THRESHOLD, полученный SAMPLE/DONE или complete Normal trace не являются ожидаемым Hanging outcome.
- **DelayEmit / timeout:** тот же HELLO-only trace (Records1/NextSeq1/Samples0/Done=false) со scenario DelayEmit; ожидается `INVALID_MISSING_SAMPLE`, command deadline — scheduled FirstDeadline. Задержанный SAMPLE не наблюдён: latency не подставляется из600ms injection.
- **DelayEmit / observed threshold:** HELLO и ровно SAMPLE seq1 (Records2/NextSeq2/Samples1/Done=false), однозначная измеренная sample delta>250ms; ожидается `INVALID_SAMPLE_THRESHOLD`. Выбран **консервативный reference=tRead** этого SAMPLE: InvalidTick и command deadline равны tRead; это не момент обнаружения ошибки, а начало100ms budget, включающего parse/validation. Отдельный существующий `receipt.wakeTick` (`WakeTick` соответствующего invalid decision) — внешний post-validation detection/check observation после возврата validation с invalid result; он не заменяет tRead и не является точным моментом обнаружения внутри parser. Новое поле или sampling point внутри runner не требуется; pure core часы не читает. Receipt различает tRead/reference, этот wake/check mark и command begin/return. Это иной допустимый порядок наблюдений того же fault injection, не любой SAMPLE_THRESHOLD в произвольном месте trace.
- Cross-scenario reason/trace, dispatch THRESHOLD_EXCEEDED/ошибка/отсутствие command, natural exit или любая дополнительная неожиданная protocol/identity/clock/quota/ownership/command ошибка исключают `N0_NEGATIVE_DETECTED`. Первичный ожидаемый timeout не маскирует последующий65-й ordinary record, reserve overflow, malformed/late record или rollback. Sticky fault не очищается записью terminal/receipt; приоритет INCONCLUSIVE_CLEANUP/LOG_INTEGRITY сохраняется, прочие нарушения дают N0_INVALID.

Pure regressions обязаны строить настоящие отдельные fake histories этих трёх negative trace-вариантов от STARTING/HELLO со своим scenario, а не менять Scenario/InvalidReason у complete Normal state. Дополнительно проверяются cross-scenario predicates, complete Normal→Hanging substitution, >100ms dispatch, wrong command deadline/reason, раздельные tRead и post-validation detection с ненулевым parse interval (100ms проверяются от tRead, не от detection), secondary quota/protocol/clock faults при сохранённой primary причине, non-ASCII/Unicode-ignorable и escaped aliases. Каждый vector возвращает ровно один Boolean; truthy array не считается PASS.

Normal positive: `N0_NEUTRAL_OBSERVED` только для данного frozen run при всех проверках, включая successful command/cleanup. Отдельные поля `protocolResult`, `sampleObservation`, `commandObservation`, `cleanupResult`, `scenarioExpectation` не сворачиваются в один all-purpose PASS. Negative-case detection не превращает invalid measurement в положительную квалификацию. Ни один исход не равен K0_INPUT_CAPTURE_RX_QUALIFIED, INPUT_LISTENER_OBSERVED, CAPTURE_OBSERVED, RX_OBSERVED, RELEASE_PASS, audio/transport/RICON/direct-preservation.

Receipt связывает run/scenario, input Git и SHA4 scripts, manifest/wire/runner hashes, host/PS/clock identity, все raw ticks/threshold comparisons, dropped/incomplete counts, held-process identity, command exception/exit outcome и verdicts. Receipt не включает собственный hash внутрь себя; финальный SHA receipt фиксирует оператор в Team END после закрытия файла. Не заявлять непрерывное отсутствие foreign writer; trusted own helper — ограниченная модель.

## 7. Exact commands и независимые gates

Все execution-команды ниже остаются **будущими**: G_WRITE уже выполнен2570, core+selftest корректируются2608/2613; G_PARSE/G_PURE/G_RUN ещё закрыты. Автором v0.3 ни одна из этих команд не запускалась. Cwd — checkout root. Их выполняет единственный оператор, отдельно назначенный Lead. Значения quota/timeouts фиксированы кодом/планом, не CLI overrides. Отказ policy/signature/sandbox — STOP с точной причиной, не Bypass/переименование/альтернативный запуск.

**G_WRITE:** frozen reviewed plan + отдельное решение Lead на limited implementation четырёх файлов. Разрешает писать/readback/diff, не dot-source/запуск tests/helper. Source/spec/production/старые ops byte-stable. Затем независимый code review.

**G_PARSE:** отдельный допуск статического PowerShell AST parsing без выполнения файлов. Подходит Windows PowerShell5.1; Enforce compile не требуется и НЕ ЗАПУСКАЕТСЯ. AST invocation после наличия файлов:

```powershell
$ops = Join-Path (Get-Location) 'tools\RICON_LiveProbe\ops'
foreach ($n in @('k0-neutral-core.ps1','k0-neutral-helper.ps1','k0-neutral-preflight.ps1','k0-neutral-selftest.ps1')) {
    $tok = $null; $err = $null
    $null = [System.Management.Automation.Language.Parser]::ParseFile((Join-Path $ops $n), [ref]$tok, [ref]$err)
    if ($err.Count) { throw "N0_AST_ERRORS $n" }
}
```

**G_PURE:** отдельно разрешённое выполнение pure selftest **в текущей PS5.1 оболочке**, без нового helper/process; не объединять с G_RUN. После code review/AST:

```powershell
& .\tools\RICON_LiveProbe\ops\k0-neutral-selftest.ps1 -EnableN0 -Mode Pure
```

Default-off checks отдельно в том же pure gate: вызовы selftest/runner/helper **без EnableN0 и без остальных параметров** должны только вернуть N0_DISABLED; параметры нельзя сделать Mandatory до disabled guard. Core только статически reviewed/definitions-only.

**G_RUN_NORMAL:** отдельный review exact4 SHA/outputs/ownership + явное one-run решение Lead. OutRoot — существующий локальный Temp, run-dir создаётся инструментом, raw evidence сохраняется. До START проверить hashes/runtime policy и отсутствие другого N0 run; не завершать чужой процесс. После START выполнить ровно:

```powershell
& .\tools\RICON_LiveProbe\ops\k0-neutral-preflight.ps1 -EnableN0 -Scenario Normal -OutRoot "$env:TEMP"
```

**G_RUN_NEGATIVE:** только после END Normal, сбора/cleanup и отдельного решения Lead. Каждый case — собственный fresh run-id/receipt, не retry. Максимум2 дополнительные команды, строго последовательно:

```powershell
& .\tools\RICON_LiveProbe\ops\k0-neutral-preflight.ps1 -EnableN0 -Scenario DelayEmit -OutRoot "$env:TEMP"
& .\tools\RICON_LiveProbe\ops\k0-neutral-preflight.ps1 -EnableN0 -Scenario Hanging -OutRoot "$env:TEMP"
```

Неполный cleanup любого case блокирует следующий. Серия≤3 helpers всего, никогда параллельно; planned cumulative≤45s инструментального observation/teardown, setup/review время отдельно. Exact generated helper CLI (token/run-dir) фиксируется в manifest/START; оператор не может заменить фиксированный script/executable. stdout≤128 коротких строк/run, full evidence только own files. Любое отклонение mechanism/files/API/limits → revised plan и review до реализации/запуска, не изменение условий задним числом.

## 8. Остаточные PTT blockers и риски

[Source evidence](<../evidence/E-standard-radio-ptt-source.md#L55-L87>) независимо сверено2481: IsUsingLRR сравнивает active/long-range entry references, **не читает toggle**; binding defaults/filters/effective semantics UNKNOWN, первый match из5576 не полный обзор. Эти результаты заменяют старое ожидание дочитки в v0.1. Дальнейший source-slot данным планом не открывается.

PTT Q1–Q8 остаются отдельными: listener lifecycle/delivery, effective binding/conflicts, штатный radio setup/long-range readback, positive provenance, real focus/manual UP, game-callback→watchdog clock/notification, Enforce refs≤8/quotas/direct observability, человек/физический источник/изолированный B-output. Legacy100+1900ms не оставляет dispatch/error запаса; отдельная timing-spec correction для PTT всё ещё обязательна. N0 не требует закрыть эти вопросы заранее, но **ничего из них автоматически не закрывает**. Physical K0 implementation/ARM/runtime=false.

**Что может сломаться:** policy отказ; неверный shared-file mode/partial UTF-8; schema/clock conversion; latency contamination polling/IO; IO/scheduler hang; handle acquisition gap; потеря receipt при принудительном exit; path reuse/foreign same-user writer. Ошибка означает bounded diagnostics/INVALID/INCONCLUSIVE и остановку, не «чинить» систему.

**Самый рискованный шаг:** Process.Start→held handle→Kill/exit при отказе IO/host scheduling. Neutral helper не владеет пользовательскими данными и не делает иных действий; его10s self-cap — best-effort, не гарантированный watchdog. Runner не защищён от остановки/гибели ОС, в N0 не заявляется абсолютный15s cap или универсальная100ms dispatch гарантия. Нужная более сильная защита потребует отдельного дизайна/review.

**Отклонено:** named pipes/network/general command executor (лишняя сложность/границы sandbox), PS7-only APIs, использование игрового callback для self-precondition, focus qualification в этом срезе, старый Workbench runner, минимизация чужих окон, policy bypass, общий process-name cleanup. Один bounded file transport выбран ради простого проверяемого first slice, а не как готовый media/real-time transport.

Rollout/deploy/migration отсутствуют. Rollback будущего N0 — не подключать новые default-off scripts; завершить только held helper и Dispose собственные streams/process object; сохранить все evidence. Ничего не удалять и не восстанавливать чужие состояния.

## 9. Ledger и handoff

| Стадия | Факт этой редакции | Следующее условие |
|---|---|---|
| Plan | v0.2 записан2880204; v0.3 DRAFT точечно исправляет representation/negative acceptance по2601/2613 | QA только изменённых участков и frozen handoff Lead |
| Writing | G_WRITE выполнен2570; corrective core+selftest2608/2613 в работе другого автора | Независимый corrective code review; эта правка не объявляет код принятым |
| AST / pure | NOT_RUN | Code review, отдельные G_PARSE/G_PURE |
| Helper IPC / command / cleanup | NOT_RUN | Отдельные G_RUN_NORMAL/G_RUN_NEGATIVE, fresh evidence |
| Real focus / Enforce / PTT / audio | NOT_RUN / BLOCKED | Только следующие отдельные контракты/gates, не этот N0 |

Handoff Lead: после frozen v0.3 и QA изменённых участков записать эту редакцию; отдельно свести corrective code review и решать G_PARSE/G_PURE. Прежний G_WRITE не переоткрывается и process gates не следуют из записи текста. Автор плана не открывает writing/pure/run gates и не меняет metadata/commit. Проверки, будущие результаты и источниковые SHA записываются исполнителем соответствующего среза, не подставляются из capability snapshot. Нового согласования конечного intent или смены сессии не требуется; технические и process gates сохраняются.
