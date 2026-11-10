# BPV1 — независимый review BP1 value-only byte-parser spec

2026-11-02. QA / ревьюер. Task task:6465977d-9d2a-469e-83e7-f357a755d005; accountable/приёмка Lead. **REVIEWED_DRAFT_WITH_FINDING / SPEC_CORRECTIVE_REQUIRED**, не code/writing/process GO. Единственная запись reviewer — этот файл.

## 1. Frozen вход и область

[BP1 v0.1](<../req/REQ-bounded-byte-parser.md>) прочитан полностью; SHA-256 независимо совпал перед review: `E3F90F95BF913A9D7252E6D2C268C8D2C0E5780133DF6DD33A32A9623E9E351A`. Прочитаны [R1](<../plan/PLAN-bounded-source-reader.md>), [RV1](<R-bounded-source-reader.md>) и [intent LS-01…08](<../req/INTENT-local-radio-slice.md#L49-L64>). Использована их traceability, не перенос native/runtime выводов. Корневой AGENTS отсутствует; родительский прочитан ранее, Arma3/SQF/PBO workflow к Reforger не применяется. Graphify graph отсутствует; генерация/установка вне scope. Применены code-quality-loop/docs-manage; авторская spec/планы/tools/addons/metadata/tests не изменялись.

Все traces ниже — независимый анализ текста и целочисленная арифметика, **NOT_RUN**. Не создавались модели, fixtures, project scripts, AST/pure/compile/process/Workbench/source extraction/input/audio; commit не выполнялся. Проверка хэшей и whitespace не является исполнением A…L.

## 2. BPV1-01 — требуется единый порядок ошибок и счётчиков

**Blocker для принятия spec как однозначной нормативной основы.** [BP1-03:37](<../req/REQ-bounded-byte-parser.md#L37>) требует немедленно INCOMPLETE_ENCODING на byte≥80; [BP1-04:39](<../req/REQ-bounded-byte-parser.md#L39>) требует INCOMPLETE_BARE_CR на любом следующем за CR byte, кроме LF. При sticky первой причине порядок проверок становится частью публичного результата, но приоритет этих правил не установлен.

Минимальный trace от Config(anchor1): READ(requested1,count1,scalar0D,error=false) → READ(1,1,80,false). После второго принятого однобайтного запроса counters=2/2/2, totalLines0, pending CR до payload проверки. Проверка encoding первой даёт INCOMPLETE_ENCODING; проверка CR-state первой даёт INCOMPLETE_BARE_CR. Обе чтения формы схемно валидны, budgets не достигнуты. Нельзя считать обе произвольные причины точным deterministic expected outcome; будущие тесты не должны выбирать его за spec.

Вторая коллизия той же недостающей таблицы: ACTIVE counters C/C/C, без EOF, следующий схемно валидный READ(1,0,0,error=true). [BP1-01:27](<../req/REQ-bounded-byte-parser.md#L27>) говорит «сразу sticky INCOMPLETE_IO», а [BP1-02:33](<../req/REQ-bounded-byte-parser.md#L33>) требует INCOMPLETE_BUDGET до инкремента/обработки payload. Нужна явная приоритетность budget admission над IO либо иной согласованный общий порядок; C+1 инкремент в любом случае запрещён. Это не дополнительная квота и не причина разрешать probing.

Минимальная corrective работа автора через Lead: одна нормативная последовательность terminal-state dispatch → schema/type/range → READ budget admission/counters → IO/count → encoding/pendingCR/content/framing → window/output/EOF finalization, либо явно выбранный другой порядок, соблюдающий запрет арифметики/индексов до type/range guards. Зафиксировать exact reason/counters/totalLines/output для одновременных нарушений; не менять C/K/L/W/O, не переписывать чужие тесты и не добавлять запуск.

| Обязательный mixed/boundary trace для корректировки | Что требуется однозначно зафиксировать |
|---|---|
| CR→80, CR→NUL, CR→printable неLF | Приоритет encoding vs pendingCR; один exact reason и consumed counters |
| CR→EOF(true), CR→EOF(false), CR→FINISH без EOF | Qualifier/pendingCR/преждевременный FINISH precedence; pendingCR никогда не successful EOF |
| ACTIVE на C → valid error=true READ | Budget vs IO, counters остаются C/C/C |
| ACTIVE на C → malformed count=-1/poison scalar | Schema vs budget; ни arithmetic payload coercion, ни инкремента не допускается |
| Свежий valid count0/scalar0; valid error=true/count0/scalar0 | Явно requested/calls1, success0 либо иной нормативно выбранный no-increment IO порядок; точные SHORT_READ/IO outcomes |
| FAILED затем любой malformed event; DONE затем READ/FINISH | Sticky первая failure; afterDONE misuse bounded/no output/no reuse без изменения прежнего terminal |

Для последней строки spec уже задаёт намерение; corrective должен лишь связать misuse с фиксированным возвращаемым status/reason и пустым output, не возвращать ранее выданный queued output повторно. В таблице C фразу «CR как byte C без EOF» следует сделать полной трассой с FINISH: отсутствие следующего event само по себе оставляет ACTIVE и не создаёт INCOMPLETE_EOF.

## 3. Независимые boundary расчёты A…L

Тройка ниже — requestedBytes/successfulBytes/readCalls. В строках без коллизии BPV1-01 семантика текста согласована.

| Область | Независимый expected result / предел |
|---|---|
| A: 41 LF 42 CR LF 43,E,F | 6/6/6, totalLines3, три output records по5 bytes + OK3 =18 bytes |
| Empty / LF / A LF / A LF LF | Empty E,F → INCOMPLETE_ANCHOR при0/0/0,total0; остальные total1/1/2, counters1/2/3 и output7/8/12 bytes соответственно; trailingLF не добавляет строку |
| C: A CR LF,E,F | 3/3/3,total1, output8. A CR,E(true) → BARE_CR,2/2/2,total0. CR на C без EOF затем F → INCOMPLETE_EOF, не успешное закрытие pendingCR |
| Encoding вне окна | NUL/80/FF/BOM первый EF invalid даже после15 сохранённых строк; output пуст, ранние queued строки не возвращаются. TAB9 — один допустимый content/output byte, не расширение на Unicode |
| E: line4095/4096/4097 | С LF первые две: counters4096/4097,total1, T4102/4103. Третий падает на4097-м content byte до LF: counters4097/4097/4097,total0, output пуст; LF уже не обрабатывается |
| F: 4096×3 + 4076/4077/4078 | Content sum16364/16365/16366, framing четырёх однозначных номеров 4×4=16, OK3: T16383/16384/16385. Первые два допустимы; третий OUTPUT_CAP до построения/queue последней строки. Input с четырьмяLF16368/16369/16370, а не T |
| G: anchor2 | Total1 → ANCHOR; total3 → n2; total16 → n15. Нельзя прекращать scan после заполнения window, иначе invalid byte позже дал бы ложный успех |
| I/J: counts/types/poison | requested0/count−1/count2/scalar256/−1/10041, bool/string вместо Int32 — schema protocol, без маски &255/преобразования до range check. Последовательные66,65,E,F → BA,2/2/2,total1,T9; предыдущий scalar не наследуется |
| K: EOF/FINISH | EOF(false) → EOF failure; EOF(true)→EOF/READ → protocol. FINISH безEOF → EOF failure. error затем E/F не очищает первую причину. ДоFINISH success output отсутствует |

Строки F включают prefix, каждую LF и OK в одном O, не только content. Для двух/шестизначных номеров overhead меняется: например L262144: + LF занимает9 bytes даже при пустой строке; модель обязана считать digits(i), а не постоянные4 bytes из F. Все значения и sums помещаются в Int32, но будущий код всё равно проверяет тип/range до сложения.

### Полные exact-cap witnesses H/L, не созданные fixtures

Пусть P — 15 LF, затем63 последовательных блока (4096 раз41, LF). |P|=15+63×4097=258126, завершено78 строк. Для anchor1 выбраны15 пустых строк; их framing9×4+6×5=66, вместеOK3 → T69.

- P + 4017 раз41 = C−1 bytes. E(true),F → counters(C−1)/(C−1)/(C−1), totalLines79, T69.
- P + 4018 раз41 = C bytes. E(true),F → C/C/C,total79,T69. Без E, сразу F → INCOMPLETE_EOF, C/C/C,total78: последняя накопленная строка ещё не закрыта.
- Тот же C-byte trace без EOF, затем ещё valid READ → INCOMPLETE_BUDGET без изменения C/C/C; total78, никакого C+1 byte. Если сначала уже принят EOF, READ после него — protocol, не budget: это отдельная state-dispatch ветвь.
- P + 4016 раз41 + CR + LF = C bytes: CR находится на C−1, LF на C; E,F → total79,T69, C/C/C. CR как последний byte C перед E(true) не завершает строку и даёт BARE_CR.

Это достигаемые последовательные byte traces без whole-stream array. Они снимают численную неопределённость H/L только на модельных facts; qualified EOF — injected утверждение caller, не доказательство native read на точном cap. FINISH не заменяет EOF.

## 4. Requirement traceability / feasibility

| Requirement | Review и следующий предел |
|---|---|
| BP1-01/02; RV1-01/02/04 | Самостоятельный PS5.1 value host, exact events и отдельные requested/success/call, no C+1 заданы. Требуется BPV1-01 precedence/counters прежде чем объявлять нормативную однозначность |
| BP1-03/04/05; RV1-05 | ASCII/TAB, L без terminators, пустые/trailing/unterminated линии, min15 и invalid outside window заданы; encoding/pendingCR collision требует corrective |
| BP1-06 | Самостоятельная pre-allocation арифметика ri=digits(i)+li+3, T3+Σri; O включаетOK. Failure terminal ровно6 полей отдельно отO, без exception text; доF output отсутствует |
| BP1-07; RV1-03 | Логический retained inventory L+O=20480 code units,≤15 fixed descriptors и fixed scalars достижим для streaming конструкции. Это не actual heap bound; copied return/currentline/queue/prefix/temporary strings и PS headers должны иметь отдельную lifetime карту в bounded plan |
| BP1-08 | A…L — конечные группы synthetic cases, не исполненные tests; mixed-error negative expected должны стать exact до будущей реализации |
| BP1-09; intent LS-08 | Reproducibility/byte framing помогают будущей evidence, но ни один LS не закрыт. Native scalar byte position/sign/high bits, FileIO/EOF/mount/snapshot, allocation/Enfusion parity и process/log ownership остаются UNKNOWN/BLOCKED |

### Память и будущий bounded plan

Current line≤4096, приватная selected queue≤16384 с заранее reservedOK3, descriptors≤15; lineNo/totalLines при inputC не превысят262145/262144. Нет основания хранить предыдущие строки вне окна или входной массивC. На ошибке не выдавать частичную queue наружу; retained failed state также не должен накапливать новые events.

При FINISH сборка строки output из queue может создать вторую живую O-копию; обычный concat old+new способен держать обе очереди одновременно. Логическая формула20480 не покрывает эти объекты. Spec правильно оставляет actual heap UNKNOWN: последующий plan должен перечислить backing storage, все одновременно живые copies/aliases/return values, запрет скрытых unbounded коллекций и peak на шаг/terminal. Требование pure неизменяемых на границе records не разрешает deep-copy всего накопленного stream. Это реализуемое обязательство будущего design/source review, не доказанный memory PASS данного текста.

## 5. Handoff

Lead: принять этот review как завершённый с одним группирующим finding BPV1-01; назначить автору узкую doc-only precedence/counter/terminal clarification, frozen новый SHA → independent delta-review. Я авторскую spec/тесты не меняю. После закрытия finding возможен только REVIEWED_DRAFT_SPEC_PASS для value contract; затем approval/immutable record/fresh transition и отдельный bounded plan с exact API/manifest/memory/finite tests/host. Spec не является готовым implementation plan и не открывает writing/AST/pure/native gates.

Readback полного review и whitespace/SHA проверяются перед handoff; frozen spec SHA повторно проверяется на неизменность. Все расчёты — статические, ни один A…L/runtime/source тест не выполнен.

## 6. Независимый delta-review BP1 v0.2 — BPV1-01 закрыт

**BPV1-01 CLOSED_SPEC_CONTRACT / DELTA_REVIEW_PASS / REVIEWED_DRAFT_SPEC_PASS только value contract.** §§1–5 выше сохранены как история первоначального v0.1 review. Новый вход: [BP1 v0.2](<../req/REQ-bounded-byte-parser.md>), SHA-256 `691EE0720F6B21BA152A140CFDCA62C23BA08094A88BEC0EC293FD368F17DE4E`, независимо совпал с frozen handoff Lead. Направление claim:1fe157b6-ccb1-4cb3-8b3b-57b83a0a5537 разрешает только эту дельту. Новая spec прочитана; проверены precedence, admitted counters, mixed traces, after-DONE result и неизменность caps/manifest/value/native boundary. Общий аудит не повторяется; новых blockers в этой дельте нет.

| Изменённый контракт | Независимый результат чтением |
|---|---|
| [Единая precedence:35–41](<../req/REQ-bounded-byte-parser.md#L35-L41>) | Terminal dispatch → schema/type/range/coherence → phase → budget → counters → IO/short → pendingCR → encoding/framing → window/finalization. Невалидный scalar не преобразуется до guard, malformed event на C не превращается в budget failure |
| CR mixed priority | CR→80/00/41 теперь однозначно BARE_CR с2/2/2,total0,output пуст; без pendingCR80/00 → ENCODING с1/1/1. Общее encoding правило дополнено explicit pendingCR exception, исходный конфликт устранён |
| EOF/FINISH | CR→EOF(false) даёт EOF failure, CR→EOF(true) BARE_CR; CR→FINISH безEOF даёт EOF failure. Пустое продолжение остаётся ACTIVE. Phase EOF_SEEN допускает толькоFINISH; READ/повторEOF не расходует budget/counters |
| C budget/schema/counters | На достигаемом P+4018×41 coherent error READ → BUDGET, C/C/C,total78. На том же состоянии count−1/poison10041 → PROTOCOL раньше budget, counters C/C/C. СвежиеR0/RE обе1/0/1,total0, но точные причины SHORT_READ/IO соответственно |
| Sticky FAILED и afterDONE | FAILED игнорирует malformed payload и сохраняет первую причину. [AfterDONE:31](<../req/REQ-bounded-byte-parser.md#L31>) задаёт отдельный шестипольный FAILED/PROTOCOL result с финальными counters/totalLines и пустым output; прежний DONE/state не мутирует, output повторно не возвращается. Контраст R(41),E,F даёт один success1/1/1,total1,T8, затем только misuse |
| [Mixed matrix:80–97](<../req/REQ-bounded-byte-parser.md#L80-L97>) | Все выбранные exact reasons/counters согласованы с общей precedence; P=258126 и tail4018 даютC,78 завершённых строк доEOF, не79. Матрица содержит отдельные fresh traces и terminal/malformed regressions, не подменённые готовые PASS-state |

Пределы C=K262144/L4096/W15/O16384, output ri/T с OK3, logical20480 code units и actual heap UNKNOWN не изменены. Два предложенных model/selftest paths и отсутствие native/IO/process capabilities сохранены; explicit EOF остаётся injected value, PS-модель не Enfusion/native test. Исходные output16383/16384/16385 и4095/4096/4097 расчёты этим corrective не меняются.

Следующий owner Lead: независимая приёмка reviewed DRAFT-spec, отдельно approval/record/fresh и назначение bounded implementation plan с API/peak copies/finite tests/host. Это не implementation plan, writing GO, AST/pure/native разрешение или исполнение mixed/A…L tests. Изменён только existing review, авторская spec/планы/tests/tools/metadata не редактировались, commit нет. Полный readback/whitespace и повторный SHA spec/report выполняются перед handoff.
