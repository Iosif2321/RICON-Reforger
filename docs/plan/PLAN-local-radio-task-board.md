# Карта задач локального радиосреза

Статус: DRAFT, 2026-11-02, снимок реестра Team2801. Автор: Картотека; проверка и запись — Lead. Это навигационная карта, не технический план и не разрешение на исполнение. Team — источник назначений и решений, Git — источник записанных редакций; эта карта пока не записана в Git. Исходный снимок кода `a04a14f` не является результатом AST/pure/run. Актуальные условия START и точные scope каждого исполнителя — в соответствующей карточке Team; изменения после снимка здесь не предполагаются автоматически.

Родитель task:b5e95a66-d9cc-40b5-8bca-649dc64c9c5d — accountable Human; прежняя task:ba901e07-814a-4803-a1ae-3655d281a6b7 закрыта без принятия. Исходные задачи реестра ниже accountable Lead, по одному исполнителю. Назначение и статус `in_progress` из-за Claim не открывают code/run gates. D2/D3/V2 приняты только в source/options/static-readiness scope; D1 принят как reviewed DRAFT, V1 — как независимый doc-review, не как executable design. Исторические waiver/Bypass и использованные D0/V0/I0 gates не переносятся на новую задачу; свежесть следующего перехода требует отдельного определения, строгая fresh-session compliance не заявляется.

## Исторический реестр и независимость

Имена исполнителей в таблице — исторические/предложенные, не текущие назначения. На снимке3348 I1/I2/V3/V4/V5/X1 открыты todo/unassigned после освобождения Claims; Lead остаётся accountable. Новая работа потребует отдельных Claims и gates.

| ID / задача | Исполнитель | Запись / границы | Зависимость содержательного результата |
|---|---|---|---|
| D1 task:02bf7e6e-41c2-44dd-a991-4e037ff33e5a | Ядро | K0 spec и предлагаемые physical K0 plan, actuation spec/plan; только DRAFT, без N0/кода | D2/D3 факты → V1 |
| D2 task:13fbeb09-aa24-4889-a536-b602584acac5 | Специалист по VoN | `docs/evidence/E-k0-von-qualification.md`, source/API-only | D1/V1; runtime UNKNOWN |
| D3 task:e7fc9f14-1d84-4666-97c3-4a4c1ac3e270 | Поиск информации | `docs/evidence/E-local-audio-readiness-options.md`; без записи/перенастройки аудио | D1/V1/X1; физическая готовность отдельно |
| V1 task:4153ba50-b969-4737-87f0-3fee4d61b36f | Реверсинженер / Ghidra | `docs/review/R-local-radio-design.md`; только public source, не binary RE | frozen D1/D2/D3 → independent review → запись дизайна и GO Lead |
| V2 task:d8b1e554-35d9-432a-8354-7200bd034b03 | QA / ревьюер | `docs/review/R-n0-readiness.md`; frozen N0 read-only | readiness → X1-A, отдельный START для каждого gate |
| I1 task:f6c294ac-5715-4c86-b47c-2cc6d603e561 | Enforce-разработчик | пять предлагаемых K0 TOOL_ONLY files по карточке; пока только чтение интерфейсов | recorded D1/D2/D3 + V1 + N0 evidence + GO Lead → code → V3 |
| V3 task:88bacb34-e040-4927-8bb5-b6d1d94646aa | QA / ревьюер | `docs/review/R-k0-implementation.md`; независим от I1 | frozen I1 + V1 + N0 evidence → review → X1-B при физических preconditions |
| I2 task:24f59dc3-f98e-44e0-93c6-5e85f277c56e | Ядро | четыре предлагаемых radio slice/actuator TOOL_ONLY files по карточке; production и I1 read-only | recorded D1-actuation + D2 + V1 + квалифицированный штатный путь X1 + GO Lead → code → V4 |
| V4 task:51e3cc4d-e58c-4493-8d66-96f6dff59e06 | Инженер радиомоделей | `docs/review/R-local-radio-actuation.md`; independent controls | frozen I2 + D2/V1 → review → X1-C |
| X1 task:a50f5d08-6102-4852-bc07-f024bd0ecef3 | Lead / архитектор | единственный оператор; fresh raw dirs и execution evidence по карточке | V2 → A; D1/V1/I1/V3 и физическая готовность → B; I2/V4 и отдельные gates → C |
| V5 task:7c0d6220-a985-4d6d-b306-5b878944aad5 | QA / ревьюер | `docs/review/R-local-radio-acceptance.md`; не оператор и не автор реализации | sealed X1 evidence/cleanup → независимая LS-01…08 матрица |
| DOC task:0ed7b305-1256-4025-a586-62932638fe3a | Картотека / Документация | state, goal plan/log, дневной changelog, эта карта | только фактические handoff/evidence; не объявляет конечную приёмку |
| Q1 task:d551cf6e-c118-497f-8941-93468bc11ebf | Специалист по VoN | назначенная public API квалификация radio-only actuator; без native effects | результат квалификации → оценка дальнейшего I2, не GO |
| Q2 task:0b0b1875-440c-4a1f-b72b-40fa7303c267 | Реверсинженер / Ghidra | назначенный независимый safety feasibility review B01/B04/B05 | reviewed результат → оценка блокеров Lead, не runtime PASS |

Предлагаемые имена файлов в anchors — зарезервированный scope, не утверждение, что файлы уже существуют; изменение scope согласует Lead. Реализация I1/I2 и независимые V3/V4 разделены; V1 проверяет дизайн, V5 — результаты, а не заменяет оператора X1.

## Снимок после V1 delta-review

[Первичный V1](<../review/R-local-radio-design.md>) SHA-256 `d31bb8eeaa27f46a3492962fa7170d2229158479337d78a405fbcb3cdd95e224` исторически выявил B01…B10. [Delta-review §7](<../review/R-local-radio-design.md#L91-L134>) SHA-256 `b75f9f767cae4cb892ef7ff986d164f80fd701201fa9c9fd912f255199fa331d` закрыл B10 и manual-UP ambiguity только как CLOSED_SOURCE_CONTRACT. D1 (#5) принят Lead только как reviewed DRAFT, V1 (#8) — как независимый doc-review; это не executable design/runtime safety PASS. Corrected K0 REQ `aaf30975ea63a1d34574569049a222a73c07b2d6d0d32c236b3b5b5b414196a2`, physical proposal `7788d7ad0691b071dd59393e9f9f90ed945f2eb71538de672d3afe1a3cfa1b36`; actuation inputs неизменны. B01…B09 остаются BLOCKED; арифметика timing DRAFT исправлена, но qualification не доказана. Нет нового record/waiver/GO; I1/I2 и X1 runs закрыты. Дальнейшие статусы — в Team, этот документ остаётся снимком.

## Актуализация после X1 G_PARSE

X1 START message:600324f3-69a9-4818-8554-fc4d8a4d9ee3 / END message:42d1e9d8-acaa-4340-b9d3-0edbc0e08b8d: PS5.1, errors=0/4; четыре source SHA и plan SHA неизменны. Это PASS_PARSE_ONLY, G_PARSE одноразово использован и закрыт. Pure/helper/game/input/audio NOT_RUN; IPC/K0/audio PASS нет. Pure блокирован: план требует current PS5.1/no-new-process, tools.pwsh — PS7; нужен reviewed executor contract до отдельного gate. Q1/Q2 назначены; I1 B02/B03 и I2 B06 — фактическое чтение, не writing; V3–V5 WAITING_INPUT, не code-review. B01…B09 и record/fresh gates сохраняются. Следующий шаг: квалификации Q1/Q2 и reviewed контракт pure, затем отдельное решение Lead.

## Снимок handoff3252: N1 и зависимости

- N1 task:68d2e092-e413-4c61-aa3e-c18fcd44681d принят только REVIEWED_CAPABILITY_FREE_PROPOSAL: [v0.2](<PLAN-k0-neutral-model.md>) SHA `93709d6287bd29563a43f36f3909829a15931bcea3dab701a062c73d8ea43fd9`; [V3](<../review/R-k0-implementation.md>) SHA `9780db0c4763b9764da5c9072b79e9d768be4ddd7f447590ea15a2c856a6dfce`. R01 CLOSED_PLAN_CONTRACT, 68 cases специфицированы/NOT_RUN, два model paths не созданы. Source/AST/pure/executor gates закрыты; record/fresh запрошен message:f960e857-2e04-4475-ac1d-9a089af950f0, ответа/waiver/commit на снимке нет.
- D1 actuation v0.2 принят только contract delta: [REQ](<../req/REQ-local-radio-actuation.md>) SHA `c95f2fdb8a7214fe4ff7007397fbb50ae2da83fe20660941820c91719f67e87e`; [план](<PLAN-local-radio-actuation.md>) `fe82f53d9a8a57e29a080718a04d552ee479bb2bd78fb0b74381fb6339092ad9`; [V4](<../review/R-local-radio-actuation.md>) `e9bb3a1ad64e5a4d30ffa050f34a0e170ebd521707e6d3047a29aa6e5497301a`. Исторические v0.1 SHA: REQ `91ca03ea569e0a8d29a4dc4f51061c7074d4eb207978aabcd8331107a5dd005d`, план `ee168908e9c2bd5d230fcfb0e0313dff66cd56e7c1f130cf2b20cfaaf577eea2`. B06 feed конкретен, native authentication/projection/runtime UNKNOWN; I2 native-context read завершён, WAITING_INPUT безопасного source gate.
- Q1/Q2/Q3 приняты только в source/feasibility/candidate scopes с blockers; не qualified actuator/reader/physical safety. R1 task:b7877eb8-a9d0-4df0-b73e-47c715d2ae51 — DRAFT/IN_REVIEW, [план](<PLAN-bounded-source-reader.md>) SHA `d8e616bdb93e4385722a23c464bcae118d6c33f5dc3f1e32315bb1a68431f820`; RV1 task:e9cf11c0-0eb1-4163-9b0d-4e7f4c5a3ac5 — независимый review IN_PROGRESS, не PASS.
- X1 [parse receipt](<../evidence/receipts/n0-ast-20261102-lead.json>) SHA `e88571511585ec39bd81ba2c7e5fd066d4e2418d4cb9381e896952741a1112fa`: operator LF-normalized transcript, QA evidence-consistent, не independent rerun. G_PARSE consumed; операторский Claim освобождён, X1 WAITING_GATE. I1/V3 physical/V4 implementation/V5 WAITING_INPUT. B01…B09 и LS NOT_RUN сохраняются. Все SHA — идентичность рабочих байтов, не Git recording.

## Closeout design-среза: снимок 3348

- BP1 task:7812c2b1-1a8c-4c72-8307-21e7dbfbc865 и BPV1 task:6465977d-9d2a-469e-83e7-f357a755d005 приняты только REVIEWED_DRAFT_SPEC_PASS value contract; BPV1-01 CLOSED_SPEC_CONTRACT. [Спецификация v0.2](<../req/REQ-bounded-byte-parser.md>) SHA `691ee0720f6b21ba152a140cfdca62c23ba08094a88bec0ec293fd368f17de4e`; [review](<../review/R-bounded-byte-parser.md>) SHA `3405166d911a2270d65711142634b1e19a06acf01296af7c0fb9eb863673da34`. A…L/mixed — аналитические NOT_RUN, отдельно от N1 68 NOT_RUN; implementation plan/model/selftest отсутствуют, AST/pure закрыты.
- R1 task:b7877eb8-a9d0-4df0-b73e-47c715d2ae51 и RV1 task:e9cf11c0-0eb1-4163-9b0d-4e7f4c5a3ac5 приняты только как DRAFT/feasibility review WITH9BLOCKERS. [R1 plan](<PLAN-bounded-source-reader.md>) SHA `d8e616bdb93e4385722a23c464bcae118d6c33f5dc3f1e32315bb1a68431f820`; [RV1 review](<../review/R-bounded-source-reader.md>) SHA `e734d7fb0c3c2343d43ffd3fe4c351b2f15bff742ac89352bbdb452c84818764`. Managed .NET handle — source candidate; target runtime/error/log/child/IO/native qualification BLOCKED. SHA — рабочие байты, не Git recording.
- I1/I2/V3/V4/V5/X1 — todo/unassigned, не done/accepted; Lead accountable. Завершены design/doc/source qualification, не цель LS/audio/RICON. До ответа на record/fresh message:f960e857-2e04-4475-ac1d-9a089af950f0 — без кода, нового executable plan и commit; новый waiver/record/implementation GO отсутствует. После допуска Lead сверит exact inputs, запишет разрешённый reviewed slice и назначит bounded N1 writing + независимый source review; BP1 сначала требует отдельный bounded plan. Native/PTT/source/executor/audio gates отдельно, production закрыт.

## Актуализация N1: запись и writing GO, 03:55 UTC+8

- Human message:a6c21bbd-04c0-4682-bd71-f65ae031242b разрешил локальные коммиты reviewed изменений и продолжение без новой сессии для текущего bounded workflow. FRESH_SESSION_WAIVED не означает strict fresh-session, наследование waiver799 или physical/runtime GO.
- REC1 task:df05b38a-ca51-46ea-8984-fcd7c82f3762 принят. Lead записал шесть reviewed документов в `a983aaad2db90790d636e30a6461cb3d60e6c598`; QA независимо подтвердил RECORD_PASS message:b361db51-cdb2-4fa0-a27b-9053986b8d14. GIT1 task:ee3ac651-e863-464f-b2b9-da0ab46bc9e4 in_review передана Human для формальной приёмки; Lead — исполнитель записи. Это record DRAFT inputs, не physical approval.
- N1W task:d90fa3bc-bf24-4547-bfa7-16873c5c4dd6 — Enforce, writing GO message:f2c3c577-e4c5-4fb8-95ff-e3ced4b82965 только два model/selftest исходника. N1V task:7ae5f174-fb05-4df6-8615-8d66fee84bc6 — QA, record проверен; source review ожидает отдельного SHA-handoff. AST/pure/68 и runtime NOT_RUN, отдельные gates; B01…B09, physical I1/I2 и LS BLOCKED. Исторический снимок3348 выше не описывает текущий record/waiver/GO.

## Текущие gates и последовательность

- D1/V1 и D2/D3/V2 приняты только в документальных/ограниченных scopes; I1/I2/V3/V4/V5/X1 todo/unassigned после подготовительной работы, I2 ожидает безопасный source gate. Актуальный статус — в Team.
- X1-A: G_PARSE выполнен и закрыт; pure, Normal и negative требуют отдельных gates и последовательных START. Pure/helper NOT_RUN, host blocker указан выше.
- X1-B: reviewed/recorded D1 и V1, I1/V3 и подтверждённые физические условия; человек отдельно подтверждает участие/микрофон и изоляцию B. K0/PTT implementation/runtime сейчас закрыты.
- X1-C: квалифицированный штатный путь, I2/V4, записанный план и отдельные разрешения; сохранность direct и отрицательные контроли — отдельные доказательства. Production остаётся закрытым.
- Lead принимает независимые работы; X1 исполняет Lead, поэтому его приёмка — Human. V5 не объявляет общую цель принятой.

Открыты: физический источник и изоляция B; binding/toggle/refs/provenance, native путь и radio-only actuator; timing/focus и hold/release safety. Общий USB-микс не является изолированным B, а отдельный RPC/локальный callback не доказывает тот же путь/протокол. Старый waiver не является разрешением на новые процессы или обход политики. До evidence и отдельного GO никаких новых AUDIO/RICON/transport PASS.

## Handoff и риски

Следующий переход: Lead сверяет эту карту и текущие карточки; авторы передают frozen результаты независимым ревьюерам; любые кодовые или runtime START выдаются отдельно по зафиксированным входам. Ошибка в ownership/gate может вызвать несанкционированный процесс или ложный PASS; наиболее рискован физический PTT и actuator с непроверенной безопасностью/сохранностью direct. Отвергнуто считать назначение, shared USB, synthetic TX, обходной data plane или один callback доказательством доставки. Rollback проекта/настроек здесь не выполняется; для будущих испытаний обязательны scoped cleanup и сохранение raw evidence по X1.
