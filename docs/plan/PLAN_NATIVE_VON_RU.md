# План native VoN: документация → player baseline → адресное управление

<!-- VON_NATIVE_CODE_REVIEW_20261104_BEGIN -->
## Принятое дополнение — 2026-11-04

**Пункт 1 завершён как STATIC_CODE_DATAFLOW_REVIEWED:** полный выбранный сквозной путь VoN/радио и его входящее/исходящее окружение разобран по native-коду, скриптам и текущим ресурсам. Источником актуального результата служат [HTML-граф](../analysis/VON_RADIO_DATAFLOW_RU.html) и [карта source → формат/единицы → обработка/очередь → получатель](../analysis/VON_RADIO_DATAFLOW_RU.md). Условия, версии, альтернативные ветви и границы Windows/common providers указаны там явно. Кодовый вывод SRC-SIG1 условен для stock-декларации без дополнительной реализации/override; runtime-вызов не утверждается.

Это обновляет прежний source-only статус пункта 1 в датированном плане ниже. Пункты runtime baseline, actuator/per-pair, RICON integration и дальнейшие проверки остаются открыты. Пункт 1.1 снят с текущей работы. Игра и движок не запускались; прежние failed/falsifier результаты сохраняются.
<!-- VON_NATIVE_CODE_REVIEW_20261104_END -->

Дата: 2026-11-03. Основание: прямой утверждённый порядок владельца Agora #6582. Статус: **CANONICAL_CONSOLIDATION / SOURCE_COVERAGE_REVIEWED**. Новый план продолжает проект; frozen PLAN_RU/v3.1/v4, исторические D0/V0/K0 и failed/falsifier evidence не заменяются и не переименовываются в PASS.

Цель: получить штатный ordinary-player RADIO путь A→B с независимым наблюдением, затем доказанный radio-only scope актуатора и интеграцию существующего RICON. Полный анализ пунктов1/2 — покрытие всех границ и конкретных UNKNOWN/probes, а не обещание знать закрытый native алгоритм. См. [pipeline](../analysis/VON_PIPELINE_RU.md), [32 radio influences](../analysis/VON_RADIO_MECHANISMS_RU.md), [последний runtime-срез](../evidence/E-agora-radio-native-20261103.md).

## 1. Исходное состояние и полномочия

- Два настоящих локальных процесса с персонажами/рациями и I0 discrete mapping уже получены. Повторная разработка spawn/второго клиента не является текущим blocker.
- Generic native message дошёл до exact same-world B; real RICON evaluator допускает/пропускает native TX, frequency control/recovery наблюдались. Это общий TX admission, не player VoN/content/per-pair RX.
- Native AI CLEAR/Seed index59+отдельное value/priority50 исполнен. Proxy event сохранился matched→all-B-wrong→matched recovery; обследованный own-B VoN RX0/0/0. Observable отвергнут как matched-frequency B-radio witness; другие B-process VoN owners не были тем самым отрицательно проверены.
- Qualified ordinary player input→B-process native RX→independent output пока нет. Public recipient veto, directed premix stream handle и native queue drain UNKNOWN; pure seam остаётся `BLOCKED_NO_ACTUATOR`.
- В действующем поручении автономные необходимые действия уже разрешены. SDK использует один назначенный root оператор — сейчас native_controls_executor; параллельные Workbench/игра/server/input/audio операции не допускаются. Полномочия остаются в границах текущего поручения. Ни этот план, ни старый waiver не создают повторный запрос владельцу; проверка технических предусловий и узкий reviewed implementation scope остаются инженерными условиями.
- Этот план не запускает ничего и не выбирает установку DLL/драйвера/нового voice backend. Missing source/sink/hook фиксируется как конкретный технический UNKNOWN, а не как выдуманный общий запрет или необходимость нового оборудования.

## 2. Утверждённый порядок0–8 и зависимости

| Пункт | Работа / зависимость | Acceptance и проверка | Текущий статус |
|---|---|---|---|
| 0 | Нормализация README/INDEX/STATUS/ROADMAP | Current vs history разделены, links present, frozen bytes не переписаны, direct authority и evidence tiers не смешаны | Навигация принята #6606/#6613;3 canonical документа SOURCE_COVERAGE_REVIEWED #6740/#6741 |
| 1 | Полный разбор VoN после0 | Все12 pipeline boundaries имеют primary source/level/observable/UNKNOWN→specific probe; player/AI/generic/editor/control/media различены; version pin/falsifier сохранены | SOURCE_COVERAGE_REVIEWED #6740/#6741; closed native UNKNOWN и probes остаются открыты |
| 2 | Полный разбор RADIO после1 |32 влияния с units/writer/observable/gap/probe, faction fallback/all-RX/receiver-owner/native-vs-RICON distinctions | SOURCE_COVERAGE_REVIEWED #6740/#6741;32 влияния покрыты документально, не player RADIO/runtime приёмка |
| 3 | Ordinary player baseline после1/2 | Qualified source/sink, actual RADIO+exactTX, native B-process RX qualified owner/exact B receiver, independent output; no-send/all-RX mismatch/fresh recovery | Capture/noSend/Wrong/Matched дали metadata pattern 82/79→0/0→74/(69 null+1 transition receiver)→80/77. Для Wrong tuning UNKNOWN, RF_PASS/falsifier/content/audio/per-pair не приняты. Parent text chain прочитана; effective range/overrides UNKNOWN. Numeric lookup выполнен INCOMPLETE без numeric values, terrain text open failed; effective range неизвестен. Edit inventory и пять высот прочитаны; границы объекта мира не live terrain/collision/standing bounds. Первый pre-setter отказ сохранён; поздняя диагностика выявила WORLD_NOT_ALLOWED на public-root представлении MpTest. После exact literal correction: A восемь samples на 100 m, B сначала около 1 m и затем семь на 100 m, один setter. Voice radius/full standing/physics и радио baseline открыты. Отдельная separated серия завершена на свежих 100 m: Positive 96/74 → Wrong 98/1 → Matched 97/88. Wrong RX post-false, B tuning UNKNOWN; counts/Q не packets/loss/latency/audio quality. Один известный short400 pilot дал A span 409 ms / B span 392 ms и 33 receiver rows; только feasibility-наблюдение, не blind/chosen information/content PASS. Новое known-LONG B window квалифицировано; один blind trial совпал LONG (A actual 1005 ms, B span 1000 ms), chance 1/2, не information transmission/reliability PASS. Первый sound опыт INVALID до release; исправленный ordinary SQUAD на 100 m дал A 92 / B 86 exact metadata, 14 READY rows/hooks 2+2 и started/finished = 0 в конечном окне. Joint token/stream binding/DIRECT/actuator не квалифицированы; не silence или universal callback absence. Power: старый B APPLIED_UNKNOWN сохранён, новый B LOCAL_STATE_CYCLE_OBSERVED с timely own getter ON, whole-device property only, capture/RX = 0. Authority/replication/RADIO-effect/DIRECT/per-pair не квалифицированы. Controlled OFF A 94 / B RX 0 → fresh ON A 91 / B 81 exact metadata на том же коде; finite association, не RF-block/DIRECT/audio/per-pair/old restore. Corrected actual DIRECT getter → A 86 TX-null/direct1 + B 80 own-B receiver-null rows даёт combined metadata candidate, не null-alone proof; raw direct UNQUALIFIED сохранён. Следующий шаг — короткий counter и один same-DIRECT + whole-B power OFF preservation control; Core BLOCKED_NO_ACTUATOR, полный voice/content/per-pair baseline открыт |
| 4 | Radio control/actuator после3 | One-variable method/authority/readback/postgate/own restore; DIRECT и соседние пути сохранены; capabilities явно ограничены | NOT_QUALIFIED; per-pair требуетA/B/C, не общий mute |
| 5 | RICON integration после4 | Accepted origin/lease/device/index/generations→pair/publication/receiver→qualified native adapter; model loss отдельно technical error; expiry/revoke доказаны | Production seam no-effects; планируемая интеграция |
| 6 | World data/physics после native binding | Actual antenna/terrain/object/material/medium inputs, domains/coverage/provenance/calibration; native и RICON predictions раздельны | Pure kernels есть; world/fading/weather gaps открыты |
| 7 | Lifecycle/multiplayer после3–6 | Respawn/reconnect/JIP/epoch/seq/lease/streamout, fresh resync/native binding, old effects/callbacks не касаются нового поколения | Pure контракты не заменяют native proof |
| 8 | Load/release после accepted capabilities | Reproducible server/client budgets, concurrent streams, memory/log bounds,30+ target/60 stress и документированная область поставки | NOT_RUN; публикация только по действующему release scope |

Промежуточный результат пункта 3: [NRX runtime receipt](<../../../Cellular System/output/agora-native-von-20261103/nrx-passive-runtime-receipt.json>),
[SHA sidecar](<../../../Cellular System/output/agora-native-von-20261103/nrx-passive-runtime-receipt.sha256>) и
[pure receipt](<../../../Cellular System/output/agora-native-von-20261103/nrx-pure-receipt.json>) — только passive metadata/lifecycle.
У обеих ролей binding и каталог 2, heartbeat и 60 s terminals invalid0/subscriptions0/callbacks0;
никакого capture/PTT/radio setter/audio input. Cleanup RESTORE_OK за 216 s, own processes gone,
settings/profile diff0, source stable. Attempt4 synthetic 39/39 не включает три compile-отказа как
положительные результаты. Heartbeat gap A = 3432 ms требует отдельного external guard до будущего
capture; coordinator применён, но actual guard/Invoke/arm не получены; meter runtime не калиброван. Actor Rpl association и local catalog refs
не доказывают cross-process radio-device identity или media/callback ownership. Это не приёмка
player RX/content/audio и не успешный radio negative control. Источник, sink и content gates открыты.

Meter-подготовка: [44/44 pure receipt](<../../../Cellular System/output/agora-native-von-20261103/meter-pure-receipt.json>)
даёт PASS_PURE_ONLY на попытке 2, не native energy/playback. [Первый runtime failure](<../../../Cellular System/output/agora-native-von-20261103/audio-meter-runtime-failed-receipt.json>)
и [triage](<../../../Cellular System/output/agora-native-von-20261103/audio-meter-runtime-triage.md>) фиксируют host crash до B bind:
START/TERMINAL, generation0, samples/nativeCalls/requested0, без чтения masters и sound stimulus.
[Flag-off контроль](<../../../Cellular System/output/agora-native-von-20261103/audio-meter-flagoff-failed-control-receipt.json>)
также failed/incomplete: B crash до клиентских логов, PS5 operator exit1; finally/footer и RESTORE_OK
не наблюдались. Отсутствие footer не доказывает неисполнение finally.
Root выполнил identity-checked containment своих Workbench/watchdog. [WER signatures](<../../../Cellular System/output/agora-native-von-20261103/flagoff-client-operator-wer-signatures.json>)
различают первый host nvwgf2umx/c0000409 и последующий client ntdll/c0000005; совпадение времени
PowerShell OutOfMemoryException не доказывает PID или причинность.
[Sealed resource Event 2004](<../../../Cellular System/output/agora-native-von-20261103/flagoff-resource-exhaustion-memory.json>)
показывает commit 99,50% и headroom 202,91 MiB в 09:03:33 UTC, примерно за 2,5 секунды до PowerShell OOM.
Это contemporaneous pressure, не доказательство exact native/runner cause. Native причина UNKNOWN;
ни getter, ни stimulus не назначены виновными, отсутствие клиентских логов не счётчик 0.

[Последняя InspectOnly квитанция](<../../../Cellular System/output/agora-native-von-20261103/flagoff-edit-inspection/recovery-a8dcf64d413c485db28d966ab84a2e60.json>)
наблюдает 10 menu items, оба localhost Off, actions false, own target/watchdog exited, без Play/capture.
INCOMPLETE сохраняется из-за изменённого digest только собственного 84-byte scratch resourceDatabase.rdb;
это не RESTORE_OK и не доказательство исходного rollback. Baseline NOT_READY; source/sink/content и
external guard не квалифицированы. Перед следующим runtime root фиксирует новый bounded manifest,
проверяет текущие process/resource условия и исходное состояние, сохраняя failed серию. Это техническая
готовность, а не новый permission loop и не изменение принятого покрытия источников пунктов 1–2.

[Первый pulse-ready отказ](<../../../Cellular System/output/agora-native-von-20261103/nrx-pulse-ready-runtime-receipt.json>)
сохранён как наш избыточный LOCAL_FACTION_UNKNOWN guard. [Точный policy triage](<../../../Cellular System/output/agora-native-von-20261103/pulse-local-faction-triage.md>)
и [source apply input](<../../../Cellular System/output/agora-native-von-20261103/pulse-policy-apply/compile-input.json>)
предшествуют actual [52/52 synthetic checks](<../../../Cellular System/output/agora-native-von-20261103/pulse-pure-20261103-135644/receipt.json>).
[Новый sealed run](<../../../Cellular System/output/agora-native-von-20261103/nrx-policy-ready-runtime-receipt.json>) с
[sidecar](<../../../Cellular System/output/agora-native-von-20261103/nrx-policy-ready-runtime-receipt.json.sha256>)
даёт только PREPARED по ADMIN при faction UNKNOWN: unarmed, capture NOT_REQUESTED. RESTORE_OK195,
наблюдённое меню/export/profile diff0 и own-process exit не закрывают native idle/release или armed
rehearsal. B post-window teardown assertion сохраняется отдельно, cause UNKNOWN. Armed coordinator
уже применён, но проверенные UI/FHC пути не дали actual arm. Source/sink/content и player baseline остаются NOT_READY;
не обобщать source52, passive windows или PREPARED в full-goal PASS.

Ресурсы: [native Low disk receipt](<../../../Cellular System/output/agora-native-von-20261103/native-low-cfg-disk-receipt.json>)
подтверждает собственный сохранённый OverallQuality0/config SHA 9A91E43D…, не memory savings.
[Normal close NVIDIA Overlay](<../../../Cellular System/output/agora-native-von-20261103/overlay-close-20261103-131943.json>)
был разрешён, сигнал принят, но exitfalse; другие приложения не трогались. Внешнее освобождение памяти
не является результатом нашего закрытия/Low или measured saving. **Pagefile не включать, не расширять
и не перенастраивать как решение — прямой запрет владельца.** Preflight проверяет текущие ресурсы в этих
границах; менять остальные приложения/OS или подменять UNKNOWN автоматическим fallback нельзя.

Ближайший control gate уточнён [goal-keeper](<../../../Cellular System/output/agora-native-von-20261103/goal-keeper.md>):
[UI attempts](<../../../Cellular System/output/agora-native-von-20261103/armed-rehearsal-failed-attempts-aggregate.json>)
не дали actual guard/Invoke/arm; Plugins disabled в Play по наблюдению root. Current Off после EDIT
inspection не переписывает historical restoration. [Held READ FHC](<../../../Cellular System/output/agora-native-von-20261103/file-handle-capability-runtime-receipt.json>)
survived Play, но parent write0/MethodInvocationException без inner/HResult; sharing cause UNKNOWN.
Это закрывает проверенный вариант file bridge, не доказывает невозможность всех FileIO путей.
[REST paired noBackend](<../../../Cellular System/output/agora-native-von-20261103/rest-capability-runtime-receipt.json>)
дал null context/no GET; pure22 не native PASS. WB context0/1 на том же URL с/без noBackend — отдельный
контекстный факт. [Предыдущий solo без noBackend](<../../../Cellular System/output/agora-native-von-20261103/rest-single-host-control-runtime-receipt.json>)
дал context nonnull/GET1 и parent accepted1/HTTP2001, но native callback отсутствовал до локального timeout. Оценочный parent
responseReturn17:51:38.387 позже native terminal17:51:37.868 основан на retrospective QPC/wall mapping,
не contemporaneous run anchor; своевременность и причина callback absence остаются UNKNOWN.

Новый отдельный async опыт дал PASS только CONTROL_NATIVE_TOKEN_CALLBACK_SINGLE_HOST:
callback служебного токена за 684 ms в том же Game clock, HTTP200/tokenMatch1, TOKEN_OBSERVED;
parent anchor contemporaneous. Sealed receipt указан в текущем STATUS; own RESTORE_OK217 и captured
diff0/0 не квалифицируют arm/capture/release/VoN. Старый timeout и его ретроспективная timing-оценка
сохранены отдельно, не переименованы в PASS.
Предыдущий NoPlay цикл подготовки завершён в ограниченном scope: применены Game5 и Parent3 после review
#7046/#7047. Actual ScriptEditor REST (22 проверки) и scoped REHEARSAL (22 проверки) pure PASS раздельны, actualCaptureCalls0;
socket13/gate9/writer7/OPS15 и один stream-cleanup case не считаются VoN/end-to-end проверками.
NoPlay receipt указан в STATUS: own Off→On и held guard KILLED/exit подтверждены; 1748,732 ms — только
наблюдение, не hard1500/native release. Fresh Off с action=false известен, restorative setter UNKNOWN.
Worker INCOMPLETE сохраняется по own scratch resourceDatabase.rdb: 51→84 bytes metadata; содержимое не читалось,
безвредность не доказана. Это не upgraded negative и не production recovery PASS.
Canonical paired DryRun PASS не запускал Game; memory-preflight отказал на неизменном commit≥8 GiB /
available physical≥6 GiB gate. Тогдашний датированный запас commit 4,76 GiB/available 7,74 GiB был ниже первого
порога. Все пять исходных Overlay живы по observation-only snapshot; новый close не посылался,
force/другие приложения/pagefile не менялись. Старые 4,91/7,82 — предыдущий snapshot, не savings.
Поздний paired milestone принят по sealed receipt в STATUS строго как CONTROL_GUARDED_REHEARSAL_ONLY_PAIRED:
parent witness 167,3453 ms (<400), WouldRequestTrue/False и INTENDED_STOP, actual probe capture/setter calls=0.
Coordinator releaseBefore→observation 74,5954 ms отдельно от parent releaseObserved→observation 64,6847 ms.
Kill request 1508,9223/exit 1714,5988 ms не являются hard1500/nativeRelease; Game не проверял внешний guard.
Fresh Off/action=false — наблюдение исхода; restorative setter и полное восстановление не испытаны,
worker INCOMPLETE/profile metadata drift сохраняется, contents не читались и benignness не доказана.
Локальная calibration дала только SEQUENCE_ONLY: один PAIN_RELIEVE start/finish handle 256/1267 ms,
140 raw=0 и master snapshots=1; energy/semantics/playback/B sink UNKNOWN. Старый CLI endpointSamples=90
отказ сохранён отдельно; правильный operator SHA 87717… использует 120, не подменяя им audio sample count.
Diag drift claim отозван correction 55A5: literal coordinator 2C совпадает, standalone не B.
Actual pair по наблюдению root имел headroom 9,66/available 8,29 GiB при неизменном 8/6. Для host-backend
устранена setup-ошибка отсутствующего Steam штатным silent запуском без credentials; приложения не закрывались,
pagefile не менялся. Это не measured memory savings от Low/закрытий и не постоянная capacity-гарантия.
Поздняя [capture-проба](<../../../Cellular System/output/agora-native-von-20261103/native-capture-runtime-receipt.json>)
дала NATIVE_CAPTURE_INGRESS_AND_RADIO_RX_METADATA_CANDIDATE: accepted true/false requests, окно 1005 ms,
A capture metadata и B own VoN exact local catalog0 RX(senderA,32000). Это не уникальные native frames,
PCM/energy/content/playback или causal/exclusive radio proof; post-stop metadata не exact release.
[No-send](<../../../Cellular System/output/agora-native-von-20261103/native-no-send-runtime-receipt.json>)
на том же SQUAD_RADIO/exactTx=1 пропустил оба API calls: окно 1002 ms, calls0, A 21 / B 8 NRX rows и RAW_CAPTURE/RX/USED0.
Каталоги A/B по два трансивера, tune 32000/48000 kHz; policy/key наблюдения только A-server. B last heartbeat за 50 ms до stop,
поэтому negative — конечное наблюдение, не атомарная health/global idle или microphone-off гарантия.
Guard kill request 1520,9186/exit 1756,2518 ms не hardRT/native release. Recovery 4c61 exact baseline Off,
original proven/action=false/own exits подтверждены, но worker exit1 INCOMPLETE с secondary profile RDB drift;
restorative action/full restore/RF/audio не квалифицированы, contents не читались.
Wrong и fresh Matched теперь выполнены; receipts и geometry указаны в STATUS. Серия82/79→0/0→
74/(69 null+1 receiver)→80/77 отражает metadata pattern, не controlled content/audio/per-pair proof.
Единственный Wrong exact receiver находится в sampled transition: last detuned 152 ms / next restored 348 ms,
поэтому фактическая B tuning UNKNOWN, не RF_PASS/falsifier. Null не DIRECT. Matched B 77 / null0 / 39 snapshots,
A/B own spans1010/999 ms не являются network latency или packet counts. Только Wrong/Matched делят full
frozen sources/prepared contexts; ранние first-positive/NoSend — намеренные прежние revisions.
Server restore 2 / conflict 0 / unchanged bits 1 не атомарный B receiver-state proof. Kill/exit обеих серий не
hardRT/nativeRelease; typed fresh baseline Off/action=false и own exits не повышают INCOMPLETE/profile RDB
worker exit1 до active/full restore. Reserved vector compile failure и reviewed BF79 rename с scoped pure
22/22/34/20/20 PASS сохранены отдельно от native результата.
Geometry ограничена radio-owner positions ≈1,000046 m; character GetOrigin NOT_LOGGED, proximity-disable
UNQUALIFIED. Bounded existing WBReader text chain завершена в заданном scope:
US_Rifleman→US_BaseLoadout→US_Base→Core/Character_Base (root без parent). В полном VoN block 379–381
только Filename von.acp; effective/native default Speaking Range и descendant overrides вне прежних
окон UNKNOWN. Последующий actual SDK numeric SourceContainer lookup завершён INCOMPLETE/exit = 2,
ANCESTOR_CAP_OR_NO_NUMERIC_FIELDS: IEntitySource components = 51, unique SCRVoN, один level = 0; Enabled BOOLEAN,
components OBJECT_ARRAY и Filename RESOURCE_NAME — descriptor names/types, все values NOT_READ/numericRead = 0.
OR reason не выбирает доказанную ancestor/cap причину. Play instance NOT_TESTED, setters = 0; не Range = 0 и
не blanket невозможность другого доступа. Existing TerrainEntity text-read exact MpTest.ent дал
search=true/candidate=1, затем FileIO text-open failure/native4/runner1/source lines = 0. Это не TerrainAbsent
и не known bounds; Line1 fallback не повторён, потому что open failure не no-match. Квитанции в STATUS.
Оба lookup без Play/teleport/capture/setters, старые C165/native/geometry seals сохранены. Поздний Edit
WorldList подтвердил 25 сущностей, terrain entity и 17 классов при лимите вывода 15. GetBoundBox объекта
мира редактора и пять GetSurfaceY = 1 — только EDITOR_SOURCE, не live terrain/collision/standing bounds
или Speaking Range. Root выбрал внутренний кандидат X/Z = (28, 128), не граничный X = 228.
Парная пассивная проба отказала на обеих сторонах: UNBOUND/CONFIG_OR_WORLD, attempted = 0,
setterResult = -1, samples = 0, capture calls = 0. Teleport/landing не выполнялись, точная ветка guard
неизвестна. Отдельные NRX окна завершены LIMIT/invalid = 0/callbacks = 0; typed recovery подтвердил
свежий Off/action = false и выход своих процессов, но остался INCOMPLETE из-за worker resourceDatabase.rdb.
Квитанции и отдельные scopes runtime diff, worker drift и pure/mocks — в STATUS.
Поздняя диагностика выделила WORLD_NOT_ALLOWED на известном public-root представлении MpTest;
остальные проверенные config/world predicates корректны. Exact literal correction D3FB/#7216 не
разрешила произвольные aliases. Старый CONFIG_OR_WORLD receipt сохраняется как прежний log.
Исправленная пассивная проба наблюдала A-server восемь samples на 100 m; B-local сначала около 1 m,
затем семь на 100 m. Один B setter/result = 1, A delta = 0, actor identities согласованы,
собственные radio positions совпадают с actor positions в settled samples. Это конечная выборка,
не все 16 samples на 100 m и не измерение межпроцессной latency. Actual Game landing/helper и owner
guards подтверждены; full standing/physics, voice radius, audio/content/per-pair не приняты.
Capture calls = 0; typed recovery остаётся INCOMPLETE при свежем Off/action = false и своих exits.
Отдельная default-off CaptureSeparatedPair серия для existing RestGuardedCapture завершена:
Positive A/B 96/74, Wrong 98/1, fresh Matched 97/88. Свежие Game geometry 100 m, current refs/RPL,
QPC progress и parent release квалифицированы отдельно для каждого запуска; frozen sources/contexts/inputs
одинаковы. Wrong RX post-false находится в B getter gap 203/298 ms, actual tuning UNKNOWN;
server restore за 57 ms по wall не current B tuning/queue drain. RF_PASS/falsifier не приняты.
Metadata counts не packets/loss, own spans не latency, Q = 0.994082868 не causal RF/audio quality.
Recovery всех случаев INCOMPLETE/profile metadata при свежем Off/action = false и своих exits;
guard process times не hard 1500 ms/native release. Подробное сравнение и три sealed receipts — в STATUS.
Один известный short400 pilot наблюдал A request/return span = 409 ms при planned 400 ms, overshoot 9 ms;
B-only span = 392 ms, 33 receiver rows/null = 0/distinct = 15. Metadata B продолжается ещё 1048 ms после
последнего RX; independent common guard/after-guard read до B stop исключает только process cutoff
на selected 400 ms. VAD/queue/другая truncation UNKNOWN; public LIVE samples внутри 409 ms отсутствуют,
before/after/admission 100 m не continuous pose. Long B spans 949/1000 ms — только descriptive comparison.
Новое known-LONG окно B квалифицировано: heartbeat tick 18400 прошёл first 17201 + 1100 = 18301
в том же binding/context. Старые INVALID не исправлялись. SDK original INCOMPLETE matcher и postparse
того же log/scoped pure 39/30/27/17/19 учитываются отдельно, native rerun не было.
Один exploratory blind trial после freeze rule/window/code дал match LONG: выбор 1/LONG1000,
A request/return = 1005 ms, B span = 1000 ms/distinct = 27/input = 113 rows. B-only decision seal до
A reveal и commitment до body release/arm проверены; ранний held GET до commitment ожидаем.
Одна запись выбора, без reroll; raw nonce не публикуется. При независимом равномерном бинарном null
вероятность случайного match = 1/2. Information transmission/reliability/full baseline не установлены.
Поздние sound-observe outcomes разделены: первый INVALID до body release/capture — только
availability / 17 READY rows; исправленный ordinary SQUAD на 100 m дал A 92 / B 86 exact metadata, null = 0, B distinct = 33.
OwnB/sourceAproxy components уже enabled, subscriptions 2+2 / 14 READY rows до capture; started/finished = 0
в конечном окне. Исправление READY progress/QPC 500 ms не ослабляет admission; pure scopes отдельно.
Нулевые события не доказывают silence, отсутствие routing/stream или невозможность native callback.
Joint RX token/attributable stream binding/DIRECT positive/actuator/content/per-pair/Core не приняты.
Recovery 60ea stale-parent/finalMenu{} не повышен поздним a142 InspectOnly; ee7b typed baseline Off/
action = false/exits/profile-only INCOMPLETE — третий отдельный исход, не full restore/native release.
Поздние power outcomes раздельны: первый B APPLIED_UNKNOWN после ON call/немедленного OFF getter,
subscription detached; A mirror ON не заменяет собственный B readback. Новый B local whole-device
OFF/ON cycle наблюдён: setters = 2/events = 2, timely ON getter 14193 после ON call 14142,
до fixed deadline 15142. Endpoint 15192 поздний на 50 ms и не основание qualification.
Native calls void; времена только B-local, не latency/hard RT. Capture/RX = 0, RADIO/DIRECT/voice effect
не проверены; authority/replication/per-pair/Core actuator не приняты. Старые SDK failure/rename/pure
и restore-observe/pure сохранены раздельно. Typed editor recoveries b438/f570 Off/action = false/profile-only
INCOMPLETE отдельно от native radio ON, не full restore. Подробности и два final seals — в STATUS.
Поздний controlled OFF request 1003 ms дал A 94 / B raw RX 0 при actual B/A-server OFF getters,
refs/catalog/protected и здоровом B observer. Conservative budget 2449/final HTTP 2258 ms ≥ 2000;
B OFF call 12211/deadline 18211/last getter 17314/remain 897 ms, OFF calls = 1/ON calls = 0.
Teardown до deadline; conditional cleanup и editor Off не доказывают native ON restoration.
Fresh ON без PowerOff flag на тех же frozen sources/contexts дал A 91 / B 81 exact receiver/null = 0,
B distinct = 32, Q scalar valid, request 1006 ms. Native reader B IsPowered = 1 — LOCAL_ONLY context,
не атомарный getter каждого RX или cross-process radio identity. Fresh ON не old-object restore.
Metadata association не causal RF block/DIRECT/audio/per-pair/Core actuator; editor recoveries
9d53588/baf173 Off/action = false/profile-only INCOMPLETE отдельно от radio/full restore.
Поздний explicit DIRECT candidate: prerelease INVALID из-за отсутствующего producer scriptIdleOnly
до release/route/capture сохранён, parent consumer не ослаблен. Exact metadata fix/replay отдельно.
Actual route getter DIRECT/TX null tick 78893 до true 78894/false 79897, span 1003 ms;
A 86 native TX-null/direct1 и B 80 sender-A/own-B receiver-null rows при current context, B distinct = 33.
Q = 0 metadata valid, не RF/audio/loss. Combined route/source/callback evidence не равен null-alone proof;
raw B direct UNQUALIFIED и старые Wrong null rows сохранены. Near A 1–1.73208 m/final 1, B 1.00005 m,
не continuous pose/Speaking Range. SDK failure/rename/pure и metadata-fix pure учитываются раздельно.
Editor recoveries ad1/ef71 exact Off/action = false/profile-only INCOMPLETE не full restore.
Следующий шаг keeper — короткий counter и один near same-DIRECT + whole-B power OFF preservation control.
Новый control ещё не выполнен; без третьего flag/framework и повторного standalone power опыта;
Core BLOCKED_NO_ACTUATOR до qualified capability. Max capture 1 s и common B horizon сохраняются; chosen content/playback/radio-only/per-pair/
native-release/full-restore PASS нет. Две sealed квитанции и точные пределы — в STATUS.
Пассивный NativePairPlacement без Pulse, budget 8/6 и guard 400/1500/2000 сохраняются, новый RPC/framework
не вводится. Radio-only/content/output/playback/per-pair, native voice baseline и source/sink открыты.
SDK оператор — native_controls_executor; вся цель, source/sink, Core/physics/lifecycle/load и этапы 3–8 открыты.
Не переносить generic/AI controls на player VoN. Полная цель, Core integration, source/sink, 32 native
influences, physics/lifecycle/load и этапы 3–8 остаются открытыми.
HTTP control/payload отдельно от guarded Arm/rehearsal, тот отдельно от native capture/VoN. Не
повторять прежние неразличающие UI/file/recovery опыты; этапы 3–8 и полная цель остаются открытыми.

Параллельные source-исследования допустимы; переход исполнения не закрывается назначением агента, compile/HOST green или датированной approval другого опыта. Ни один последующий пункт не расширяет фактически принятую capability молча.

## 3. Ближайший минимальный reach изменения

Не писать ещё одну RF модель, транспорт, универсальный audio framework или второй lifecycle registry. Переиспользовать MpTest+Diag runner, existing binding/generation/detach и существующие проверки. До кода root фиксирует точный список изменяемых файлов и их callers; текущие новые режимы ниже **PROPOSED**, не готовые CLI flags.

| Reach | Минимальная работа | Связанные callers/проверка |
|---|---|---|
| Source/sink preflight | Read-only actual capture/render/device/process inventory, существующие endpoints/tools, fresh OS; отдельно source input и B output health | Render-only inventory не отвечает о mic binding; lookup metadata не recording |
| Fresh host isolation | В baseline ветви собственные host profile/logs, как у client; Busy/held handles/noBackend/known prompt guards сохранены | `ops/wb-serverplay-claude.ps1`; AudioObserve сейчас вне discovery isolation branch и не real-player baseline |
| Baseline-only raw observer | Сохранить null TX/RX, exact receiver и owner tags; B-own VoN и source-A-proxy VoN отдельно, Sound invoker отдельно; refs/generations/clock/GAP/terminal | Shared `RICON_LiveObserver` вызывают AudioProbe/ProbeComponent; старые null-ignore tests нельзя сломать скрыто. Baseline-only adapter/mode либо deliberate shared-contract update |
| Native player capture adapter | Один default-off attempt с actual native method/TX, own capture start/stop и exact binding; без AI/generic input | Ordinary PTT effective context/paired DOWN-UP отдельно от public native capture-API test; не называть API-start проверкой keyboard UX |
| Verdict/raw checker | Independent B result и full terminals, callback/content/output/restores раздельно; decoded expected contents B не передавать | Existing observer/audio plugins+selftests для refs/null/detach/quota и legacy passive compatibility; новый meaningful gate self-check |

Existing implementation points для чтения: `tools/RICON_LiveProbe/Scripts/Game/RICON_Live/RICON_LiveObserver.c`, `RICON_LiveAudioProbe.c`, `RICON_LiveProbeComponent.c`; `Scripts/WorkbenchGame/RICON_LiveObserverProbePlugin.c`, `RICON_LiveAudioProbeCheckPlugin.c`; `ops/audio-passive-check-enforce.py` и `audio-passive-check-selftest-enforce.py`. Точные changes author review предлагает после чтения callers. Production Core не меняется ради квалификации baseline.

## 4. Source и sink: technical qualification без обходного data plane

**Источник A:** настоящий capture endpoint, реально используемый ordinary player AudioCaptureDevice. Known speech/audio сначала подтверждается на входе; playing WAV/PlaySound/FFplay сам по себе только render, не file-to-VoN injection. Existing virtual capture route может быть использован лишь после actual qualification; неизвестный маршрут не заменяется установкой нового драйвера по умолчанию. Silence detection/normalization могут подавлять тон: noRX с неподтверждённым input не native denial.

**Trigger:** (а) штатный PTT требует effective binding/context/focus/real DOWN+UP; (б) bounded public native capture API на правильном player component может исследовать mic/media без приёмки PTT UX. Оба требуют actual SQUAD_RADIO/exact TX и отдельного stop/ownership. Не вызывать protected wrappers наугад. Native eligibility проверяется по actual cross/admin/effective-key policy, не обязательному faction-known. Для non-DIRECT faction-policy допускает crossFaction || admin || effectiveLocalKey.IsEmpty() || radioKey==effectiveLocalKey; остальные binding/life/entry/TX guards сохраняются. Unknown faction сохраняет controller cache; известная faction задаёт prospective key, включая пустой. Same-faction — контролируемый вариант, не универсальное native требование; A/B synthetic equality сама недостаточна. DIRECT toggle и nearby proximity контролируются, а не считаются радио.

**B-process sink:** native receive owner заранее не угадывается. Observe B-own и mapped A-proxy VoN отдельно; positive qualifies конкретный owner. SenderA + exact B receiver/catalog/generations обязательны; proxy SoundStarted другой тип события. Нет current own-B event — не глобальный вывод обо всех native components.

**Output:** две альтернативы isolation: qualified process+children capture на совместимом OS либо qualified B-only endpoint/другой проверенный инструмент. Endpoint WASAPI loopback — shared render mix, не PID separation. [Microsoft process-loopback sample](https://learn.microsoft.com/en-us/samples/microsoft/windows-classic-samples/applicationloopbackaudio-sample/) требует build20348+; [endpoint loopback](https://learn.microsoft.com/en-us/windows/win32/coreaudio/loopback-recording) имеет другую область. Read-only инвентарь baseline автора2026-11-03 сообщает Windows10 Enterprise19045/UBR6466: process-loopback sample не документирован как поддержанный на этом текущем OS; activation не пробовали. Перед исполнением fresh check обязателен. Это не доказательство необходимости OS upgrade, нового hardware или невозможности software isolation.

Static AudioSystem.OutputVolume можно отдельно калибровать silence/known local sound без VoN handle, masters неизменны; установить setting-vs-energy, units/aggregation/stage/latency. Qualified process meter может дать presence/level без PCM export только с контролем всех других sources. Он не декодирует свежий контент и не адресуетA→B. GetGain требует real attributed handle/class/lifetime; AI handle не player stream token.

Для CONTENT verdict: A выбирает свежий неизвестный B token после setup; B фиксирует raw+decode/NO_FRAME/INVALID+terminal, verifier запечатывает hash **до A reveal**. RPC/общие файлы/log/seed/symbol schedule/outcome не переносят выбранное содержимое. Decoder сначала проверяется offline positive/silence/wrong/truncated/malformed/actual format; thresholds/window/preprocessing заморожены до live. Energy, CRC, одна удача или один человек, знающий ожидаемую фразу, не независимое content proof.

## 5. Baseline acceptance и контрольные окна

| ID | Окно | Обязательное evidence / классификация |
|---|---|---|
| B00 | Source и sink independent health | Capture endpoint/device binding и B-specific output/decoder health; no foreign/game/control sources. Отсутствие health → UNQUALIFIED, не negative PASS |
| B01 | No-emitter/idle | Нет выбранногоA content при живом sink; logs/observers работают |
| B02 | Source-only/local imitation | A source активен, native capture OFF/реальный путь отсутствует; B не распознаёт token. Иначе acoustic/system/control leak |
| B03 | Ordinary RADIO positive1 | Source eligible по actual controller policy без обязательного faction-known; actual method/TX+useful capture; server-use diagnostic; B-process genuine OnReceive(senderA,exactB-RX,qualifiedowner), independent B output/content |
| B04 | No-PTT/no-capture | Тот же qualified source; нет radio content при здоровом sink. Не требовать исчезновения каждого callback класса |
| B05 | All-B wrong-frequency | Весь bounded relevant B candidate catalog или доказанный singleton; actual tune/key/power/refs на callback+window, source unchanged; no content; alternate DIRECT/editor/intercom/relay пути исключены |
| B06 | Fresh matched recovery2 | Restore только own deltas на тех же refs; свежий token2 независимо найденB. Failed recovery делает предшествующий negative INCONCLUSIVE |
| B07 | Power/key/range | Отдельные one-variable positive→negative→recovery серии послеB03–06; не смешивать radio rules с source eligibility/direct fallback |
| B08 | DIRECT control | Distinct qualified native DIRECT source до/DURING/после actual held radio restriction; null/raw capture/RX сохраняются, direct не выводится из отсутствия radio callbacks |

Успех B03 отдельно классифицирует capture request, useful input, RAW_NATIVE_RADIO_RX_OBSERVED, independent output и CONTENT. Возможен external content PASS при private codec/framing UNKNOWN; внутренний OPUS byte parity им не доказывается. При отсутствии квалифицированного OnReceive owner/readback честный уровень ниже, а не выдуманный native fail.

Новые observations не ретроактивно повышают старые AI/generic попытки. Старый all-B wrongRF proxy counterexample остаётся обязательным regression/falsifier для выбора sink. Не отключать decoder/log/observer на model LOST ради иллюзии эффекта.

## 6. После baseline: actuator и RICON

Каждая capability — отдельно: TX_ADMISSION (всем RX), RX_TRANSCEIVER_MUTE, DIRECTED_RADIO_GATE, PER_STREAM_DSP. Public setter/getter не доказывает scope/authority/exclusive writer/native applied status. Preexisting user/platform mute не присваивается RICON. Baseline/own-change/readback/restore привязаны к exact same refs/generation; unknown writer/rebind → no restore onto new handle и containment.

Для per-pair gate нужныA/B/C: A→B подавлен, пока **A→C и C→B на SAME qualified B receiver** живы; reverse target/source, B→A и второй RX A→B тоже проверяются. Если half-duplex, контрольные передачи неперекрывающиеся, но tested gate непрерывно удерживается. Простая смена target после снятия gate не исключает whole-B mute. DIRECT положителен во время запрета. Нет C/control path — только narrower capability, не адресность.

Integration использует existing lifecycle/pair/publication/receiver/seam: authenticated accepted-origin request, TX lease и device/index обоих endpoints, generations, epoch/link/seq, reason и время. Technical mismatch/expiry/GAP не записывается как физическое MODEL_LOST. Snapshot LOST один недостаточен. New allocator/epoch/TTL registry не вводится; существующий manager sequence используется как есть.

Native binding должен выдать opaque radio-only stream/recipient token с qualified scope. Apply — idempotent seq и фактическое applied/readback; unsupported→UNSUPPORTED. DESIRED, REQUESTED/APPLIED и independent OBSERVED/postgate раздельны. Policy validity — пересечение lifecycle lease, snapshot presentationTTL и descriptor lifetime после qualified server→audio clock mapping; unknown clock запрещает новые effects, keepalive исходные сроки не продлевает.

**ReleaseOwn DSP ≠ revoke recipient permission.** Удаление собственного mute/DSP может вернуть vanilla слышимость; для lease expiry нужен отдельный доказанный authoritative radio subscription/recipient gate. Старый expiry не влияет на new stream/user mute. Пока такой hook не найден, `BLOCKED_NO_ACTUATOR` сохраняется; C++ proto bridge DS/Workbench, signal index, global gain/Rpl stream deletion не подмена directed engine capability.

## 7. Исполнение, stop и проверка

Root фиксирует перед каждым конкретным run: exact executable SHA/mode, gproj/addons/world/plugin/args, source hashes, собственные process handles+creation time, fresh profiles/log roots, caps/deadlines/observation windows/decoder thresholds, baseline/restore и роль независимого verifier. Эти параметры не угадываются из старого runtime разрешения; это bounded technical manifest, а не новый вопрос о permission.

- **NO_ARM:** чужая game/tool сессия, identity/build/source mismatch, нет qualified input/sink/binding/полного candidate catalog, нет контролируемых log/deadline/cleanup средств. Завершать независимую source работу можно.
- **INVALID/INCONCLUSIVE:** rebind/world/clock jump, unrelated audio/direct fallback, lost observer/decoder, GAP/overflow, startup/release uncertainty. Preserve all attempts; отсутствие данных не successful suppression.
- **Containment:** stop только own still-matching capture/policy; own client→host teardown через удержанные handles. Не ждать завершения одной роли прежде, чем запросить stop у другой. Никаких global name-kill/PID rediscovery/foreign window input. Native request stop не обещает drain.
- **Terminal:** detach subscriptions, explicit per-property restore/readback, own processes/watchdogs gone, source/settings/profile diffs. MRU != security; общий HKCU import не нужен. Runner RESTORE_OK не заменяет radio-property/native stop/output evidence.

Существующие команды **диагностики**, не готового player baseline, из canonical корня в Windows PowerShell5.1 x64:

```powershell
# Только existing read-only route inventory, не запись и не mic qualification.
& .\tools\RICON_LiveProbe\ops\audio-inventory-enforce.ps1 -Pretty

# Existing passive paired path: DryRun не запускает игру, passive flag не real-player content mode.
& .\tools\RICON_LiveProbe\ops\wb-serverplay-claude.ps1 `
  -Tag native-player-check -OutDir '<NEW EXISTING OUTSIDE-REPO SCRATCH>' `
  -Seconds 300 -Client -AudioObserve `
  -World '{96A8AF57260A7392}worlds/MP/MpTest/MpTest.ent' -DryRun
```

Не совмещать AudioObserve с D0/V0/I0 flags; WrongFrequencyControl сейчас V0/AI mode, не готовый player-radio control. OutDir literal placeholder требует concrete existing owned directory. Существующий300s outer runner не заменяет конкретный capture safety bound. Proposed baseline flags/decoder/capture commands пока не существуют — их нет в executable recipe.

После минимального patch: targeted selfchecks на owner/null/receiver/catalog/seq/quotas/deadline/own restore/detach и legacy passive compatibility; fresh ScriptEditor compile/check по exact plugin/log/expected count, не одному exit0. Затем baseline/control/runtime root. Не повторять480 старых HOST suites вместо нового native evidence; broad regression нужен при реальном core/interface change.

Documentary проверка источников: `python -B <task-output>/verify_verification_matrix.py --self-test` проверяет7 boundaries/7controls/9 source hashes и7 planted contradictions; это DOCUMENTARY_CHECK_PASS, не native/RF/model run. Canonical consolidation дополнительно проверяется links/IDs/32influences/0–8order/sourcehash и независимым reviewer до закрытия1/2.

## 8. World, lifecycle и нагрузка: следующие измеримые выходы

| Пункт | Минимальный output | Не допускать |
|---|---|---|
| 6 World | Actual antenna coordinate/datum/terrain bounds/object opening/material thickness/forest canopy/water medium; validity/coverage и RICON domain; отдельно native q/output | Неизвестный объект не воздух, no provider не0dB/LOST; acoustic signal не RF law; campaign XZ graph не native terrain |
| 6 Fading/receiver | Geometry→descriptor builder/time anchor, calibrated receiver/demod→audio policy, packet/PCM/clock scope | h(t)/dBm не PCM gain/pitch; sampled47runs не число слышимых просветов; weather OPEN не implicit0 |
| 7 Lifecycle | JIP full-resync + fresh native generations, disconnect/rebind/PTT restart/old seq/epoch/expiry, streamout и late queued input classification | Proxy presence не authority; old handle/permission не revive; DSP release не revoke |
| 8 Load | Pair/stream/radio candidate budgets, worst-case concurrency, bounded memory/log, latency/audio/dropout distributions и reproducible raw | Target30+/stress60 — критерии, не выполненная поддержка; UI tick не native cadence |

## 9. Происхождение supplemental синтеза

Основные переносимые ссылки — existing canonical evidence/REQ и pinned primary URLs из двух analysis-документов. Supplemental root этого прохода: `C:\Users\Ded\Documents\Projects\ARMA 3 Scripts\Cellular System\output\agora-native-von-20261103`. Таблица фиксирует конкретные прочитанные bytes; дальнейшее изменение output требует отдельного evidence, не обновления старого SHA молча.

| Supplemental input | SHA-256 |
|---|---|
| [von-chain.md](<../../../Cellular System/output/agora-native-von-20261103/von-chain.md>) | `d5592021a8799f005d9d383ecfcaa1a7927860bd8d34784eb8dc003a2cb72899` |
| [von-source-pin-manifest.json](<../../../Cellular System/output/agora-native-von-20261103/von-source-pin-manifest.json>) | `eff4dc931e7b1a5246d94b4edc45021083f31907f8b18b36e4d41c99eb32bbab` |
| [radio-chain.md](<../../../Cellular System/output/agora-native-von-20261103/radio-chain.md>) | `a1b4702a87b343ff4802d55248a2c4417ca75341afcfd6bd52c7f3b3b566b0a7` |
| [von-network.md](<../../../Cellular System/output/agora-native-von-20261103/von-network.md>) | `4425cc7e6be4190cb785e08190062405471ad716501577443b15aaa785dffc6e` |
| [von-audio-chain.md](<../../../Cellular System/output/agora-native-von-20261103/von-audio-chain.md>) | `4c8e0dc8246ee26e604a140f32666b536223c022a779b7eb8ef25d1d05443c57` |
| [radio-range-power.md](<../../../Cellular System/output/agora-native-von-20261103/radio-range-power.md>) | `8bfea40209809428b279369538fc58f5d92a98536294f79ad6595cf88c89d203` |
| [radio-terrain.md](<../../../Cellular System/output/agora-native-von-20261103/radio-terrain.md>) | `1ffb71c05920644c9b770070bdaf39d9a01c6261ff22b754f8f0d5ccefc58e61` |
| [radio-environment-devices.md](<../../../Cellular System/output/agora-native-von-20261103/radio-environment-devices.md>) | `8e5f50471899450e42aaf2acb1d95d6b6d060f10079f59f65b9202a2fb50df85` |
| [native-baseline-plan.md](<../../../Cellular System/output/agora-native-von-20261103/native-baseline-plan.md>) | `1e336c5a3dc4100f7d7615f67201479d2e33bd3884c463f08b2bacc1cc1cfab0` |
| [verification-matrix.md](<../../../Cellular System/output/agora-native-von-20261103/verification-matrix.md>) | `6fade9d658256fca96e29c3b41fab7c19672c1fe43950b1ae869b6646e064ff2` |
| [verification-matrix.json](<../../../Cellular System/output/agora-native-von-20261103/verification-matrix.json>) | `a6c28f4fc14b529cb3a8c9863223fb850727e087800b8bdc5b53d0da0db80ae4` |
| [docs-audit.md](<../../../Cellular System/output/agora-native-von-20261103/docs-audit.md>) | `9fdd6e0c4662147364311d217484ea748194126373f59738c39173e00938f0a9` |

Author cross-reviews: source pin/network/radio/audio #6675/#6677/#6679/#6681; Seed correction #6686; radio owner correction accepted #6673; documentary per-pair correction independently checked #6685. Исторические source/runtime scopes различены, raw приватные значения в canonical синтез не копируются.

Sealed earlier facts/provenance: [E-agora-radio-native](../evidence/E-agora-radio-native-20261103.md) ведёт к generic receipt и matched/wrong/recovery comparison. Wrong receipt SHA `5367db03e5a46b9fee4ff400623eda209a38a5f1bc2aa77b1cce2ffca75180ca`, recovery `2771158ad84630b1b71a2c9face05ba597926bd474e9b2cfdab4d54983b1cd53`, comparison `611539d8b3922968e9a1d739e019523badf7106368c6570f5fd6ac70ab750802`. Final source SHA+SOURCES_STABLE+actualCRC/root freeze не выданы за отсутствующий persisted full runtime SrcSnapSHA.

Этот документ не закрывает native baseline/control/integration исполнением текста. Root принимает отдельные результаты и изменяет current status по факту; независимый QA этой консолидации обязателен перед объявлением coverage пунктов1/2 закрытым.
