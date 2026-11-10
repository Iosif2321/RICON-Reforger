# CT10: подтверждённая компиляция SDK и проверка lease

Срез чтения: **2026-11-10T09:27:50.569269+00:00** → **2026-11-10T09:27:50.584576+00:00**. Исходный audit подготовил `codex.team.docs`; `codex.lead` проверил 27 SHA-pins и 6 журналов и опубликовал этот отдельный срез. Исторический draft сохранён в staging.

**Результат:** baseline fix03 и отдельный query fix04 имеют **ENFORCE_COMPILE_LOG_CONFIRMED**. Оба завершаются Root-observed exit−1 после SteamAPI_Init failed; во всех шести логах отсутствуют integration/fixture completion markers. **PURE_FIXTURES_NOT_RUN**, не overall SDK/runtime/Goal PASS.

Новый прямой допуск владельца: «Да, разрешаю ScriptEditor и pure fixtures» — Agora #8328. Только root SDK operator; без Play/world/server/mic. Этот допуск позже старой source-only записи AGENT_CONTEXT; старые evidence/receipts остаются immutable. Steam login остаётся ручным действием владельца; Documentation не проверяла UI заново и не выполняла auth/run.

## Фактическая compile-цепочка

Engine build в логах: **193322**, Game **1.9.0.37**, build **2026-10-27 15:39:59 UTC**. CLI указывает canonical TestRunner addon.gproj, `-wbModule ScriptEditor -plugin RICON_TemporalIntegrationProbePlugin -noBackend`. Инициализация SDK engine/Game instance наблюдалась; Play/world/server execution этим не заявляется.

| Attempt / actual log | SCRIPT(E) | Наблюдение |
|---|---:|---|
| [baseline · ricon-tests-ct10-integration-baseline-20261110-120600](<C:/Users/Ded/Documents/My Games/ArmaReforgerWorkbench/logs/ricon-tests-ct10-integration-baseline-20261110-120600/console.log:1>) | 2 | Game compile FAIL: AuthorityFixtures:56 Broken expression; patch01 out→closureResult. |
| [fix01 · ricon-tests-ct10-integration-fix01-20261110-120734](<C:/Users/Ded/Documents/My Games/ArmaReforgerWorkbench/logs/ricon-tests-ct10-integration-fix01-20261110-120734/console.log:1>) | 4 | Game compile FAIL: Snapshot Clone declarations :112/:26/:65; patch02 type-specific names/callers. |
| [fix02 · ricon-tests-ct10-integration-fix02-20261110-121004](<C:/Users/Ded/Documents/My Games/ArmaReforgerWorkbench/logs/ricon-tests-ct10-integration-fix02-20261110-121004/console.log:1>) | 50 | Game compile FAIL: RICON Formula too complex :229/:184/fixture:82; присутствуют и vanilla diagnostics. Только три RICON predicates изменены patch03. |
| [fix03 · ricon-tests-ct10-integration-fix03-20261110-121227](<C:/Users/Ded/Documents/My Games/ArmaReforgerWorkbench/logs/ricon-tests-ct10-integration-fix03-20261110-121227/console.log:1>) | 0 | WorkbenchGame195/673, CRC70b3248a; Game5797/11593, CRC0dc0257a. Compile confirmed; Steam failure :161, no fixtures. |
| [query_initial · ricon-tests-ct10-lease-query-20261110-122100](<C:/Users/Ded/Documents/My Games/ArmaReforgerWorkbench/logs/ricon-tests-ct10-lease-query-20261110-122100/console.log:1>) | 24 | Новый query package: SameLease Formula too complex :59 и два No return :11/:15 в query fixture; прочие vanilla diagnostics также есть. |
| [query_fix04 · ricon-tests-ct10-lease-query-fix04-20261110-122202](<C:/Users/Ded/Documents/My Games/ArmaReforgerWorkbench/logs/ricon-tests-ct10-lease-query-fix04-20261110-122202/console.log:1>) | 0 | Отдельный query compile: WorkbenchGame195/673 CRC2ba39cc3; Game5798/11595 CRCfeec6e28. SCRIPT(E)=0; Steam failure :161, no fixtures. |

Final baseline [fix03:156–161](<C:/Users/Ded/Documents/My Games/ArmaReforgerWorkbench/logs/ricon-tests-ct10-integration-fix03-20261110-121227/console.log:156>) и query [fix04:156–161](<C:/Users/Ded/Documents/My Games/ArmaReforgerWorkbench/logs/ricon-tests-ct10-lease-query-fix04-20261110-122202/console.log:156>) отдельно подтверждают загрузку WorkbenchGame/Game modules. Пустой SCRIPT(E) не заменяет fixture done marker. В логах Game successfully created относится к SDK initialization перед platform failure.

## Compile fixes и текущие source pins

Четыре patch-файла изменили **6 разных файлов**: первые три — 5 baseline-файлов, patch04 — новую query fixture. Никакой per-file SHA в самих compile logs нет: pins ниже — точное **post-run наблюдение**, не ретроспективная hash-attestation лога.

| Patch | SHA-256 | Scope |
|---|---|---|
| [compile-fix-01.patch](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ct10-lease-query-20261110/compile-fix-01.patch>) | `0a1da4f7300d1f607eb8bbb48965d4a9be882889ae93ac2207419b9c6c61443f` | Reserved local out renamed closureResult. |
| [compile-fix-02.patch](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ct10-lease-query-20261110/compile-fix-02.patch>) | `264f0f90fecc28921bb6f73e15e2e6548f1b559f5f4225f7b1ef3ff9609aa125` | Clone renamed CloneBinding/CloneContext/CloneLimits with all recorded callers. |
| [compile-fix-03.patch](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ct10-lease-query-20261110/compile-fix-03.patch>) | `8981c36c4cb08e755be8ec6b7e426b205e3eab0e18de18afa1de14671d66d91e` | Three over-complex predicates split without changing recorded rejection/equality coverage. |
| [compile-fix-04.patch](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ct10-lease-query-20261110/compile-fix-04.patch>) | `2664b6cd1a4db5e7d12ead2d0b7b05bdc80a29054cb5eec9724d4936751235cb` | Lease-query fixture SameLease/Check predicates split; two return paths made multiline. |

| Current canonical file | SHA-256 |
|---|---|
| [addons/RICON_Core/Scripts/Game/RICON/Core/RICON_CommsManagerComponent.c](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_CommsManagerComponent.c:1>) | `74d892842fce7a3243ca2b5f67e140ff7e18a548e8fc8812b700868f07532fea` |
| [addons/RICON_Core/Scripts/Game/RICON/Core/RICON_Lifecycle.c](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_Lifecycle.c:1>) | `33935bc546231c4bd89d3b09cd3efef7e13fea2a9e25a2c7217ec698ac697767` |
| [addons/RICON_Core/Scripts/Game/RICON/Core/RICON_TemporalAuthorityJournal.c](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_TemporalAuthorityJournal.c:1>) | `6211c762a54b90a06bf7c7fa2b6fd0d54703b15026a9e3c1febc0ca6b239a937` |
| [addons/RICON_Core/Scripts/Game/RICON/Core/RICON_TemporalLifecycle.c](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_TemporalLifecycle.c:1>) | `d9dab7058aef64e9c2efe8fef1506a8812bc4fdb37b3dd1b88615eff51590b28` |
| [addons/RICON_Core/Scripts/Game/RICON/Core/RICON_TemporalSnapshot.c](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_TemporalSnapshot.c:1>) | `5104e10f99d5d3b7211034d07a6d9fffd73c0795b0cab2c429f27a3b9e88d28a` |
| [addons/RICON_CoreTests/Scripts/Game/RICON_Tests/RICON_LifecycleLeaseQueryFixtures.c](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_CoreTests/Scripts/Game/RICON_Tests/RICON_LifecycleLeaseQueryFixtures.c:1>) | `c06804d68b97c21b32c462d202235f90b94ab90b9db75d6abc8d0520b9e5dbbc` |
| [addons/RICON_CoreTests/Scripts/Game/RICON_Tests/RICON_TemporalAuthorityFixtures.c](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_CoreTests/Scripts/Game/RICON_Tests/RICON_TemporalAuthorityFixtures.c:1>) | `b8bc79923048c4310b3135227bb7cd176a451685207a39976e4ca96a2a556fc4` |
| [addons/RICON_CoreTests/Scripts/Game/RICON_Tests/RICON_TemporalLifecycleFixtures.c](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_CoreTests/Scripts/Game/RICON_Tests/RICON_TemporalLifecycleFixtures.c:1>) | `6ed9ad8dba26e152920cb7dea55acb225d77440b50a2b0374dfbf679ecedb8a5` |
| [tools/RICON_TestRunner/Scripts/WorkbenchGame/RICON_TemporalIntegrationProbePlugin.c](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/tools/RICON_TestRunner/Scripts/WorkbenchGame/RICON_TemporalIntegrationProbePlugin.c:1>) | `f4a9bf5f74e88ca0970a7f7043d91c6e9a183738d641a5b8ef86dfe01808860c` |
| [tools/RICON_TestRunner/run.ps1](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/tools/RICON_TestRunner/run.ps1:1>) | `0c6934205d389002a6194e7d9df2ec4c5d375bb17bacdbe80eea88a251e6fb90` |

## Small query: source отдельно от SDK и исполнения

Canonical QueryActiveLease getter, manager lifecycleNowMs admission, query fixture и probe call фактически присутствуют на момент pins. Query не observe/sweep: explicit lifecycle time, ready/anomaly/time-floor/ACTIVE/TTL guards и detached copy всех **9** lease fields. Manager сопоставляет returned leaseId/player/generation/device; network/channel/access/exporter/retired-epoch/source timestamp truth этим getter не квалифицированы.

**Baseline fix03 не компилировал query.** Его отдельное SDK подтверждение — новый query fix04 log с 5798 files/11595 classes. Independent source review сообщает leaf checker **16 literals / 3 mutants PASS**, только SOURCE_EXTRACTED_QUERY_ALGORITHM_ONLY; Documentation checker не перезапускала. Pure query RunAll, Tick/effects и actual manager role/admission execution по-прежнему NOT_RUN.

## История и незакрытая граница

Agy #8312–#8317 — фактический source review прежнего integration/receiverEntityId diff; это **не SDK review** этих compile fixes/query. [E-ct10-integration-source-20261110.md](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/evidence/E-ct10-integration-source-20261110.md:3>) и [ct10-integration-source-result-20261110.json](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/evidence/receipts/ct10-integration-source-result-20261110.json:1>) сохраняют прежний SOURCE_REVIEWED_UNCOMPILED scope и старые pins. Они не перезаписаны новым compile результатом.

Исторические source evidence/receipt hashes, все 6 log hashes, 4 patch hashes и машинные module/diagnostic refs сохранены в [ct10-sdk-compile-result-20261110.json](receipts/ct10-sdk-compile-result-20261110.json). Missing final fixture marker и Steam platform failure остаются явными. Следующий разрешённый root pure-run требует рабочего Steam login/контекста владельца; Documentation новых запусков не делала.

До runtime/source-provenance/producer/RPC/clock/sharedRX/calibration/content/audio/load acceptance большая Goal остаётся открытой. Documentation записала только два staging draft; root публикует новые evidence/receipt и текущий контекст отдельно. Старые evidence и receipts не изменены.


## Независимая проверка после SDK fixes

Reviewer #8340–#8341: **ACCEPT_SCOPED_COMPILE_FIXES_01_04 / SEMANTIC_EXPECTATIONS_PRESERVED**. Применение четырёх патчей в памяти к frozen baselines точно воспроизводит normalized current6files/22hunks; переименованы все11objectClonecallers; ordered predicate atoms19/11/11 сохранены. В fixture04 все9полей и null safety сохранены, но static Check sites38→42: старые diagnostic counts/first-failure indices не переносить. [Полный source/log review](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ct10-lease-query-20261110/review/COMPILE_FIX_REVIEW_RU.md>), SHA051ed69b….

Agy #8338 повторно реально прочитал canonical Lifecycle33935bc5, Manager74d89284 и queryfixturec06804d6. По TTL/overflow, owner/generation/device, clock, mutation и nullgate новых source blockers не нашёл. Это его **source QA**, не собственный SDK запуск или proof истинного server-time/producer. [Agy receipt](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ct10-lease-query-20261110/AGY_LEASE_REVIEW_RECEIPT.json>). Хэши предоставлены root, самостоятельный hash-compute Agy не заявлял. Gemini4Argon/профиль не менялись: разовое разрешение на смену модели не понадобилось. Его локальная memoryprojectionrev7 историческая; root через действующий API отдельно обновил/readback shared current-stage rev8.

Компиляция актуального пакета подтверждена двумя сторонами по actual logs. Исполнение fixtures, Enforce parity, actual producer/manager role/epoch coordinator/RPC, voice/content/audio/load не подтверждены. Для следующего разрешённого pure-run нужен ручной вход владельца в Steam; неизменные неудачные запуски не повторяются.
