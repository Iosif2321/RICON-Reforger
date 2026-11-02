# V1 — независимая проверка локального радиодизайна D1–D3

Версия 0.1, 2026-11-02. Тип: независимый design/source review. Автор: Реверсинженер / Ghidra; accountable/приёмка — Lead. Задача task:4153ba50-b969-4737-87f0-3fee4d61b36f, исходное назначение — task:02bf7e6e-41c2-44dd-a991-4e037ff33e5a (D1), task:13fbeb09-aa24-4889-a536-b602584acac5 (D2), task:e7fc9f14-1d84-4666-97c3-4a4c1ac3e270 (D3).

**Итог: REVIEWED_DRAFT_WITH_BLOCKERS / I1_WRITE_BLOCKED / I2_WRITE_BLOCKED / RUNTIME_BLOCKED. DESIGN_REVIEW_PASS готового исполнимого дизайна не присваивается.** D1 устранил арифметический дефект старого timing budget и явно оставил неосуществлённые interfaces/предпосылки закрытыми. Доказательств готовности physical K0, актуатора, доставки, сохранности direct и финального LS PASS нет. Это завершённый независимый обзор DRAFT-пакета с конкретными stopping blockers, не разрешение следующей фазы.

Источник редакций — Git RICON-Reforger; проверенный HEAD/baseline `a04a14f125fe757ad0038d762e2b44ab4819085b`. Новые документы и изменение K0 не закоммичены; SHA ниже идентифицируют проверенные рабочие байты, но не заменяют recorded/approved revision. D2/D3 приняты Lead только как source/options evidence; это не runtime qualification. Следующий переход **FRESH_SESSION_REQUIRED**, не PROVEN/WAIVED; исторический Team799 закрытой задачи не перенесён. V1 остаётся в текущем назначенном doc-only review, не переходит к build/test runtime.

Прочитан [родительский AGENTS](<../../../AGENTS.md>); корневой AGENTS отсутствует. Arma 3/SQF/PBO workflow не применялся к Reforger. Применены ai-native-sdlc-cycle и artifact-contracts, code-quality-loop, docs-manage; graphify прочитан, граф отсутствует, установка/генерация вне scope. Версии навыков независимо не установлены. Единственная запись V1 — этот отчёт; changelog/metadata ведёт отдельный DOC/Lead scope.

## 1. Frozen входы

Все шесть SHA-256 независимо вычислены и совпали с handoff авторов; D2 без авторского hash проверен отдельно. Полностью прочитаны эти документы, исходные intent/N0/audio/core/lifecycle/S3/S3b/seam/marker-probe; source-чтения перечислены ниже, не выдаются за полный аудит реализации.

| Вход | Редакция | SHA-256 рабочих байтов |
|---|---|---|
| [K0 REQ](<../req/REQ-standard-ptt-K0.md>) | v0.4 DRAFT | `84DF7FDA3C14A5F754A6260CC0E945067914405E766D49DBBBCBBEDDBAD5333B` |
| [Physical K0 plan](<../plan/PLAN-physical-ptt-K0.md>) | v0.1 DRAFT_PROPOSAL | `eb1ae6530f8072ac4e66b036f4684d3fa4e24576cdc699468cd51b84fe44b25c` |
| [Actuation REQ](<../req/REQ-local-radio-actuation.md>) | v0.1 DRAFT | `91ca03ea569e0a8d29a4dc4f51061c7074d4eb207978aabcd8331107a5dd005d` |
| [I2 plan](<../plan/PLAN-local-radio-actuation.md>) | v0.1 DRAFT_PROPOSAL | `ee168908e9c2bd5d230fcfb0e0313dff66cd56e7c1f130cf2b20cfaaf577eea2` |
| [D2 qualification](<../evidence/E-k0-von-qualification.md>) | SOURCE_PARTIAL | `638968fae0dc4e6a65f0c7979c51658792c4a1c24733bc1bca3fe376b5ce2808` |
| [D3 options](<../evidence/E-local-audio-readiness-options.md>) | BLOCKED / NOT RUN | `013986aeb869b96eb9ace25be1e473d648fcfa622d6ff73773f7183e00f60742` |

Read-only baseline: [intent v0.2](<../req/INTENT-local-radio-slice.md>), [frozen N0 plan v0.3](<../plan/PLAN-standard-ptt-K0.md>), [аудиопротокол](<../plan/LOCAL_AUDIO_VALIDATION_RU.md>), [core v0.9](<../req/REQ-core-contracts.md>), [lifecycle v0.4](<../req/REQ-lifecycle.md>), [S3 v0.2](<../req/REQ-pair-service.md>), [S3b v0.2](<../req/REQ-link-publication.md>), [seam v0.2](<../req/REQ-von-seam.md>), [marker probe v0.4/P3/P4](<../req/REQ-von-marker-probe.md#L197-L218>). Исторические NOT_RUN/waiver в старых файлах не используются как текущие полномочия.

## 2. Независимо сверенные факты и пределы

- [Raw registration:148–157](<C:/Users/Ded/Documents/My Games/ArmaReforgerWorkbench/logs/ricon-wbreader-ptt-registration-20261102-20261102-065303/console.log#L148-L157>) подтверждает VONChannel и регистрацию/снятие vanilla DOWN/UP. [Public ActionManager:139–140](<C:/Program Files (x86)/Steam/steamapps/common/Arma Reforger Tools/Workbench/docs/EnfusionScriptAPI/html/interfaceActionManager.html#L139-L140>) объявляет Add void/Remove bool. Это не полный lifecycle собственного listener и не доставка ввода.
- [InputManager:202–240](<C:/Program Files (x86)/Steam/steamapps/common/Arma Reforger Tools/Workbench/docs/EnfusionScriptAPI/html/interfaceInputManager.html#L202-L240>) содержит notnull key/filter arrays и defaults, но пустое описание semantics. Effective binding/conflicts, filter/preset/device и предварительный allocation bound не доказаны. [IsContextActive:126](<C:/Program Files (x86)/Steam/steamapps/common/Arma Reforger Tools/Workbench/docs/EnfusionScriptAPI/html/interfaceActionManager.html#L126>) — только объявление getter.
- [Raw LRR:148–161](<C:/Users/Ded/Documents/My Games/ArmaReforgerWorkbench/logs/ricon-wbreader-k0-lrr-tail-20261102-20261102-090757/console.log#L148-L161>) сравнивает active/long-range entries, не toggle. [I0 accessor:1–8](<../../tools/RICON_LiveProbe/Scripts/Game/RICON_Live/RICON_LiveVoNMappingAccess.c#L1-L8>) действительно не экспортирует toggle.
- [VoN OnCapture:325–355](<C:/Program Files (x86)/Steam/steamapps/common/Arma Reforger Tools/Workbench/docs/ArmaReforgerScriptAPIPublic/html/interfaceVoNComponent.html#L325-L355>) описывает recording frames и null transmitter для only direct. [OnReceive:360–408](<C:/Program Files (x86)/Steam/steamapps/common/Arma Reforger Tools/Workbench/docs/ArmaReforgerScriptAPIPublic/html/interfaceVoNComponent.html#L360-L408>) — protected native callback с isSenderEditor, не тот же публичный SCR invoker. Не указан порядок playback/mute. [Старый observer:150–191](<../../tools/RICON_LiveProbe/Scripts/Game/RICON_Live/RICON_LiveObserver.c#L150-L191>) отбрасывает null и не сохраняет receiver ref. Он не квалифицирован как K0/direct/post-gate прибор.
- [Lease:391–402](<../../addons/RICON_Core/Scripts/Game/RICON/Core/RICON_Lifecycle.c#L391-L402>) не содержит network/channel/TX index/sessionEpoch; [effects/result:254–285](<../../addons/RICON_Core/Scripts/Game/RICON/Core/RICON_Lifecycle.c#L254-L285>) не заменяют полный принятый request context. [TransmissionBinding:26–39](<../../addons/RICON_Core/Scripts/Game/RICON/Core/RICON_PairService.c#L26-L39>) — отдельный caller-supplied DTO. Его ручное заполнение не доказывает authority/lifecycle integration.
- [PairResult:87–103](<../../addons/RICON_Core/Scripts/Game/RICON/Core/RICON_PairService.c#L87-L103>) хранит значения. [BuildPlan:389–412](<../../addons/RICON_Core/Scripts/Game/RICON/Core/RICON_LinkPublication.c#L389-L412>) сводит и valid model LOST, и технические случаи к LOST/Q0. [Publish:633–670](<../../addons/RICON_Core/Scripts/Game/RICON/Core/RICON_LinkPublication.c#L633-L670>) выдаёт terminal snapshot, но TERMINATE отсутствует в fresh registry. D1 правильно требует immutable origin по emitted seq и старый terminal descriptor до замены registry.
- [Binding/seam:323–401](<../../addons/RICON_Core/Scripts/Game/RICON/Network/RICON_NetworkContracts.c#L323-L401>) не содержит native handle и возвращает BLOCKED_NO_ACTUATOR при валидном input/expiry. Отказ не разрешает эффект сбоку. D1 отдельно оставляет BLOCKED_TOOL_ADAPTER_AUTHORIZATION, не расширяет REQ-CORE-002 автоматически.
- [Маршрутная квитанция:32–93](<../evidence/receipts/audio-route-inventory-20261102-lead.json#L32-L93>) подтверждает только прежние две игровые сессии на общем USB и отсутствие выполненного split. [Inventory:72–88](<../../tools/RICON_LiveProbe/ops/audio-inventory-enforce.ps1#L72-L88>) явно point-in-time. D3 не объявляет HDMI/микрофон/слепого наблюдателя готовыми. Ссылки D3 на официальные внешние страницы не переисполнялись как измерение ОС; фактическая совместимость/маршрут остаются BLOCKED.

Версия установленных HTML не квалифицирована заново; raw logs исторические. Build/source/runtime equivalence не выводится из совпадения сигнатуры. Новый vanilla source, бинарники, память и debugger не читались.

## 3. Матрица дизайна и stopping blockers

PASS в колонке review означает только корректно заданное ограничение DRAFT; BLOCKED не означает невозможность механизма.

| ID | Область / источник | Review / воспроизводимый blocker | Следующий owner и условие снятия |
|---|---|---|---|
| V1-B01 | Timing; [K0 §10](<../req/REQ-standard-ptt-K0.md#L182-L203>) | Арифметика PASS: 100+1500+150+50=1800ms, ещё 200ms нерасходуемого резерва; pre-DOWN 1600+150+50=1800ms. Квалификация BLOCKED: estimator/mapping, game FileIO/notification, error decomposition и независимый safety case отсутствуют. N0 и наблюдённый максимум не доказывают будущий bound | D2/I1/X1, V3: точные qualified source/clock/wire/dispatch mechanisms и failure safety до GO/ARM; не тест K0 для доказательства собственного предусловия |
| V1-B02 | Refs/allocations; [physical inventory](<../plan/PLAN-physical-ptt-K0.md#L45-L63>) | Прямой inventory 3+2+1+4+2=12>8; alias/lifetime reduction и bulk entry/key/filter array pre-bound UNKNOWN. Post-count не решает уже выполненную allocation; оба implementation gates закрыты | D2/I1/I2: peak/lifetime/alias proof ≤8 на роль, включая invokers/временные refs; безопасные bounded APIs либо revised contract до кода |
| V1-B03 | Input/readiness; [D2 таблица](<../evidence/E-k0-von-qualification.md#L11-L22>) | Mapping SOURCE_PASS, но listener lifecycle, effective binding/context/filters, toggle/readiness/reference semantics и штатный radio setup не qualified. Нет права пробно исправлять selector/accessor | D2: отдельные ограниченные source/API scopes и точные недостающие evidence; не новая выгрузка по этому review |
| V1-B04 | Provenance/focus/manual UP; [physical startup/readiness](<../plan/PLAN-physical-ptt-K0.md#L29-L47>) | Fresh fixture и quiet polling не доказывают exclusive physical writer/native hold. HWND↔held handle, отсутствие focus steal, реальный человек/manual UP и independent watchdog setup UNKNOWN | I1/X1, V3: положительная ownership/focus/input qualification и safety case; Human только для физических действий после отдельного разрешения |
| V1-B05 | Bounded logging/processes; [physical quotas](<../plan/PLAN-physical-ptt-K0.md#L65-L75>) | Каналы 240/128/32, overflow/equality/no-reset и own handles заданы корректно. Native console log quota, startup/bootstrap/engine arguments и watchdog independence не qualified; aggregate cap не ограничивает engine-internal logs | I1/X1: concrete manifest/commands, pre-allocation/serialized-size checks, собственные handles/exit/detach receipts; incomplete cleanup запрещает PASS |
| V1-B06 | Authority/native projection; [LA interfaces/binding](<../req/REQ-local-radio-actuation.md#L38-L89>) | Source DTOs существуют, полного qualified authority export и stable→native resolver нет. Принятый lifecycle request/lease/catalog/tick не заменить fake initializer, новым epoch или frequency match | Ядро/D2/X1, V4: authenticated callbacks/export, accepted request correlation, bounded device projection и terminal generation checks |
| V1-B07 | Actuator/radio-only/restore; [LA §7–8](<../req/REQ-local-radio-actuation.md#L91-L111>) | DESIRED/APPLIED/OBSERVED разделены; права и concrete native actuator не выбраны. H2/bool readback не доказывают radio-only/exclusivity/own restore; UNKNOWN блокирует первый call | D2/Ядро/X1, V4: отдельное test-only authorization, qualified method/target/authority/baseline/writer token/readback/conditional restore, direct scope |
| V1-B08 | Независимый B/same path/direct; [LA §9](<../req/REQ-local-radio-actuation.md#L113-L119>), [D3](<../evidence/E-local-audio-readiness-options.md>) | Нет qualified post-gate observable/протокола, физического input A, изолированного B-output/независимого распознавания. RX или decoder suppress не заменяют выход; packet не равен audio | D2/D3/X1, V5: конкретная аудио- либо same-path packet методика, healthy controls, seal B до reveal A и direct before/during/after |
| V1-B09 | Record/gates/series; [I2 gates](<../plan/PLAN-local-radio-actuation.md#L26-L41>), [runtime series](<../plan/PLAN-local-radio-actuation.md#L91-L116>) | Новые редакции не recorded, fresh transition не доказан; планы proposal, exact runnable commands и полный конечный series protocol BLOCKED. Назначение не GO, исторические consumed gates не наследуются | Lead: независимая приёмка/record при полномочии, fresh session либо новый explicit waiver; отдельные bounded qualification/implementation/process решения |
| V1-B10 | Новый specification finding: immediate abort timing; [K0 §7/§10](<../req/REQ-standard-ptt-K0.md#L121-L125>), [dispatch definition](<../req/REQ-standard-ptt-K0.md#L186-L203>), [physical plan](<../plan/PLAN-physical-ptt-K0.md#L65-L75>) | D150 формально привязан к scheduled due/heldDue, но immediate malformed/direct/null/ownership/focus abort может случиться раньше due либо до HELD. Отдельные reason→reference rules, command budget и ранний reference до parse не определены. Нельзя корректно проверить этот путь только return−heldDue | D1/Lead: точечная reviewed correction REQ/physical proposal до executable planning; independent delta-review V1/V3, без правки автора reviewer-ом |

**Timing scope:** H начинается от complete frame receive, не от конца parse; D включает due→validation/decision→последний command-return обеим ролям, не 150ms на каждую. E учитывается один раз: N_estimate + E_mapping, не N_upper плюс тот же error повторно. Эти определения согласованы, но ни estimator, ни safety case не реализованы/квалифицированы. Частный успешный N0 run не создаёт гарантии callback→watchdog/Windows scheduling. UP, silence и process exit не равны NATIVE_RELEASE.

**Воспроизведение V1-B10 без запуска:** пусть complete malformed frame получен в external tick 100ms, parse закончен в 250ms, command-return в 300ms, а heldDue=1500ms. Вычитание будущего heldDue даёт −1200ms вместо elapsed200ms от receipt; отсчёт от позднего parse даёт50ms и скрывает150ms validation. До HELD heldDue вообще отсутствует. Это мысленный contract counterexample, не исполненный pure test. Нужны отдельные references для scheduled containment и immediate faults: для complete-frame validation — исходный receive mark до parse; для independently observed focus/ownership fault — квалифицированный observation/check mark с явным observation-latency scope; unknown occurrence time нельзя объявлять измеренным. Численный budget, precedence, command-once и boundary vectors должны быть явно утверждены D1 до executable gate. Lead независимо указал этот риск в V1 thread; он проверен по указанным definitions и зафиксирован здесь, не закрыт дополнительным timeout.

**Перед executable plan уточнить формулировку:** [K0 §7](<../req/REQ-standard-ptt-K0.md#L123-L125>) говорит «сначала» ручной UP, «затем» containment, тогда как [§10](<../req/REQ-standard-ptt-K0.md#L203-L214>) и physical plan требуют немедленную внешнюю команду. Безопасное нормативное прочтение — ручной UP не блокирует внешний containment, внешняя команда никогда не ждёт человека/логов/detach. Не реализовывать последовательное ожидание operator ACK. Сейчас это clarification до executable gate, не разрешение best-effort ARM.

## 4. LS-01…08: ни один финальный критерий не закрыт

| Intent | Что проверено в дизайне | Фактический статус / недостающее evidence |
|---|---|---|
| LS-01 | LA-01/02/05, canonical epoch/lease/device/native generations, terminal quartet | PARTIAL исторически по intent; новый complete authority/native binding BLOCKED B03/B06 |
| LS-02 | LA-10, свежий контент неизвестен B; operator/capture не payload | BLOCKED B03/B08; физический источник/qualified native packet input не доказаны |
| LS-03 | LA-08/10, independent content и отдельно same-path/protocol либо изолированное audio | BLOCKED B08; OnReceive не достаточный output |
| LS-04 | No-send, реальный path break/absent-B verifier control, local imitation, fresh positives с healthy observer | NOT_RUN / BLOCKED_RUNTIME_SERIES B09. Конкретные безопасные controls/cadence/число окон ещё не frozen |
| LS-05 | Exact valid S3 MODEL_LOST→emitted/accepted seq→APPLIED→post-gate suppression/restoration; technical/terminal/abort отдельно | BLOCKED B06/B07/B08; snapshot LOST и ручной mute не доказательства |
| LS-06 | Отдельный здоровый direct source/output до/во время/после того же radio effect | BLOCKED B07/B08; null/direct diagnostics не preservation PASS |
| LS-07 | Только own delta/conditional restore на том же generation, zero subscriptions и own process exits | NOT_RUN / OWNERSHIP_BLOCKED B04/B05/B07; unknown effect или cleanup не PASS |
| LS-08 | Полные raw/source/build/параметры/все попытки/SHA, B terminal/seal before reveal | NOT_RUN для опытов; этот review фиксирует лишь design inputs, не runtime evidence |

Положительные исходы K0/N0 в будущем не суммируются в LS acceptance. D3 два положительных окна — нижняя граница предлагаемого аудиопротокола, не отмена более строгих контролей выбранного frozen series. Marker 112-symbol схема не перенесена автоматически. До серии нужна отдельная branch-specific спецификация/калибровка, число окон, safe path-break и точные критерии до просмотра результата.

## 5. Проверки V1 и граница результата

- PASS: полный readback шести frozen docs и нормативных baseline-входов; выборочные первичные source/API/receipt сверки §2, независимые шесть hashes. HASH_PASS означает идентичность прочитанных байтов, не источник runtime события.
- PASS: read-only `git diff --check -- docs/req/REQ-standard-ptt-K0.md`; new-file whitespace checks отдельно от tracked diff. Для `git diff --no-index --check -- NUL <new-file>` ожидаемое отличие входов может вернуть 1 без диагностик; это не project-test failure и не evidence выполнения документа.
- PASS: `git diff --exit-code a04a14f -- addons tools docs/plan/PLAN-standard-ptt-K0.md` без изменений. Чужие goal/changelog/review/task-board изменения наблюдались в shared tree, не редактировались и не приписаны V1/D1.
- NOT_RUN: project scripts, dot-source, pure tests, compile, Workbench/WbReader/game, input/capture/audio/inventory, setters, binary RE, commit. Source inspection не называется runtime/parity PASS.
- BLOCKED: I1/I2 ready-for-code и physical/native series по B01…B10. Требование отсутствия критических открытых пунктов для executable handoff не выполнено; неполнота не скрыта условным DESIGN_REVIEW_PASS.

## 6. Самодостаточный handoff Lead

V1 завершён в пределах независимого doc/source review; Lead может принять сам отчёт и DRAFT-пакет как описание ограничений, не принимать отсутствующее выполнение. Следующий безопасный шаг — выбрать отдельный bounded qualification scope для перечисленных blockers и отдельно урегулировать record/fresh-transition. I1/I2 не начинать до их собственных зависимостей и явного узкого GO; при невозможности квалификации оставить BLOCKED либо пересмотреть контракт до реализации.

Читать baseline `a04a14f125fe757ad0038d762e2b44ab4819085b`, этот отчёт после фиксации точной редакции, шесть frozen inputs §1, parent AGENTS и normative refs. После любого изменения этих шести байтов требуется re-review соответствующей дельты. Report source-of-truth revision пока NOT_RECORDED; запись/commit — только Lead при наличии полномочия. Приёмку V1 выполняет не исполнитель. Никакой external/source extraction, code/run gate, Windows-routing/recording action, destructive cleanup, release/deploy этим handoff не открыт.

## 7. Delta-review v0.2 — 2026-11-02, после frozen handoff 20:56:49 UTC+8

**DELTA_REVIEW_PASS только для specification finding V1-B10 и UP/containment wording. V1-B10 CLOSED_SOURCE_CONTRACT; manual-UP ambiguity CLOSED_SOURCE_CONTRACT.** Это не DESIGN_REVIEW_PASS исполнимого дизайна, не runtime safety/dispatch PASS и не разрешение I1/I2. B01…B09, LS-статусы §4 и все code/run gates остаются BLOCKED/NOT_RUN в прежней области.

Новое направление claim:c16e1676-a7d5-4392-ba9b-c068fd302f00 в той же задаче V1 разрешило только эту независимую дельту; авторские документы reviewer не менял. Основание — Lead message в V1 от 20:51:25 UTC+8, handoff D1 message:45519295-7a0f-4e5e-b2b3-4105370823c4. Исходные §§1–6 и finding B10 выше сохранены как история первоначального review, не описывают новое состояние двух исправленных входов. Их исходный полный SHA-256: `D31BB8EEAA27F46A3492962FA7170D2229158479337D78A405FBCB3CDD95E224`; прежние SHA в §1 остаются историческими, не подменены новыми.

### 7.1 Точный вход дельты

| Документ | Новый frozen SHA-256, независимо совпал с handoff | Что проверено |
|---|---|---|
| [K0 REQ v0.4 corrective](<../req/REQ-standard-ptt-K0.md>) | `AAF30975EA63A1D34574569049A222A73C07B2D6D0D32C236B3B5B5B414196A2` | Полный текст; непосредственно B10 — [§7:123–125](<../req/REQ-standard-ptt-K0.md#L123-L125>) и [§10.2:205–228](<../req/REQ-standard-ptt-K0.md#L205-L228>) |
| [Physical proposal v0.1 corrective](<../plan/PLAN-physical-ptt-K0.md>) | `7788D7AD0691B071DD59393E9F9F90ED945F2EB71538DE672D3AFE1A3CFA1B36` | Полный текст; [§5/§5.1:69–98](<../plan/PLAN-physical-ptt-K0.md#L69-L98>) и [boundary vectors:132–145](<../plan/PLAN-physical-ptt-K0.md#L132-L145>) |

Два actuation inputs независимо byte-identical по прежним SHA: [REQ](<../req/REQ-local-radio-actuation.md>) `91CA03EA569E0A8D29A4DC4F51061C7074D4EB207978AABCD8331107A5DD005D`; [plan](<../plan/PLAN-local-radio-actuation.md>) `EE168908E9C2BD5D230FCFB0E0313DFF66CD56E7C1F130CF2B20CFAAF577EEA2`. Исходный review SHA подтверждён до записи этой секции. Проверка addons/tools/frozen N0 относительно baseline по-прежнему без изменений. Новые входы не закоммичены; точный record/fresh transition остаётся B09, не закрывается hash совпадением.

### 7.2 Независимая матрица исправления

| Проверка | Вывод чтением, не выполненный тест |
|---|---|
| Scheduled vs immediate | PASS_SOURCE: scheduled reference — original due; будущий heldDue исключён. Immediate rules действуют с STARTUP/READINESS/ARMED/PENDING до HELD и далее; H до HELD не применяется |
| Complete frame и затраты validation | PASS_SOURCE: tReceive frame до decode/parse/semantic validation; поздний tCheck хранится отдельно. Поэтому reference100/parse250/return300 означает D200 и INVALID, не50 от detection |
| Partial/framing/oversize | PASS_SOURCE: ранний read mark сохраняется, chunks не reset reference; oversize минимум с partial mark, проверки до allocation. Нового grace/таймера не создано; возникший partial fault не превращается в valid от позднего полного frame |
| IO/quota/reserve | PASS_SOURCE: begin соответствующей операции/check до затрат; frame-triggered причина берёт минимум с tReceive. Невозвратный IO hang не назван покрытым, B01/B05 остаются; reserve не добавляет время |
| Focus/ownership до HELD | PASS_SOURCE: начало первого qualified внешнего check/sample с нарушением; check return/detection отдельно. Это не время физического изменения; occurrence→observation UNKNOWN, не включено молча в D и не закрывает B04 |
| Fault vs due / primary outcome | PASS_SOURCE: minimum уже наблюдённых references и уже наступивших due; due, наступивший при parse, участвует. Primary reason не сдвигает reference; absolute→invalid fault→safety phase→normal POST_UP, дополнительные faults sticky, equality не disarm |
| Dispatch обеих ролей | PASS_SOURCE: begin≥reference, return≥begin, последний required successful return−reference≤150ms для общей последовательности. Missing/exception одной роли не скрывается успехом другой; оба receipts обязательны. E един, не дополнительные50ms к D; R200 не расходуется |
| Command once / cleanup | PASS_SOURCE: latch до call, одна команда на held handle за всю попытку/finally/watchdog race; exception/late UP не retry. Natural exit/no role не command PASS; exit/detach/terminal и ошибки отдельно |
| Независимость ручного UP | PASS_SOURCE: [REQ:125](<../req/REQ-standard-ptt-K0.md#L125>) и [plan:75](<../plan/PLAN-physical-ptt-K0.md#L75>) прямо запрещают ожидание человека/UP/ACK/логов/detach/exit первой роли перед внешней командой; физическая обязанность оператора не снята |

### 7.3 Контрпримеры и границы — только анализ текста

- Старый B10 reference100/parse250/return300 теперь однозначно D200>150, в том числе до HELD. Вычитание будущего due запрещено.
- При reference100 return249/250/251 даёт149/150/151: равенство150 допустимо лишь для частного D threshold,151 отвергается без компенсации E/R. Успех одного порога не combined PASS.
- Fault100 и due200, замеченные после parse250, сохраняют reference100. При fault/due100 оба reasons сохраняются, normal POST_UP не маскирует invalid; absolute при наличии имеет primary precedence.
- Partial100/oversize180/return260 даёт160, не80; IO/sample begin100/detection180/return249 даёт149 от software-operation mark, не от неизвестного возникновения внешнего события.
- Отсутствие manual UP/ACK не откладывает call; первая exception/missing second return запрещают dispatch PASS и повторный call того же handle.

Эти ожидания независимо сверены с формулировками и арифметикой; **vectors NOT_RUN**. Никакие pure/project scripts, новая source extraction, compile/Workbench/game/input/audio/setters/RE/commit не выполнялись. Readback и обычные read-only hash/whitespace/local-link checks относятся только к документам.

### 7.4 Остаток и handoff

B10 закрыт только как конкретная неполнота specification reference/dispatch contract. Не закрыты B01 clock/wire/estimator/safety case, B02 refs12>8 и allocation bounds, B03 getters/binding/toggle/lifecycle, B04 positive provenance/focus/manual operator, B05 startup/bootstrap/log quotas/process safety, B06 authority/native projection, B07 actuator/ownership/restore, B08 post-gate/audio-or-same-path/direct независимость, B09 record/fresh-transition/exact executable series. Ни один LS-01…08 не повышен от этой дельты.

Следующий owner — Lead: независимая приёмка doc-review/исправленного DRAFT с retained blockers, затем отдельное решение по безопасному qualification scope и record/fresh gate. Исходные findings/inputs остаются в отчёте для воспроизводимости; текущий K0 вход — новые два SHA §7.1. Ни принятие этой дельты, ни закрытие B10 не открывает executable planning, writing, compile/pure/process/ARM/runtime или разрешение физических/Windows действий.
