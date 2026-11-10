# D1 — предложение bounded плана TOOL_ONLY actuation I2

Версия 0.2, 2026-11-02. Автор: Ядро / Логика / Последовательность; accountable: Lead. **DRAFT_PROPOSAL / V4_CORRECTION_PENDING / SPEC_GATE_BLOCKED / I2_WRITE_BLOCKED / RUNTIME_BLOCKED**. Corrective D1 scope3097: cold-start sole-owner feed и bounded provenance; остальные gates не открыты.

## 1. Контекст, запись и scope

Baseline `a04a14f`; Git — source of truth редакций, Team task:02bf7e6e-41c2-44dd-a991-4e037ff33e5a — doc-only D1, task:24f59dc3-f98e-44e0-93c6-5e85f277c56e — future I2, сейчас только чтение. Входы: [intent v0.2](<../req/INTENT-local-radio-slice.md>), [LA REQ v0.2 corrective DRAFT](<../req/REQ-local-radio-actuation.md>), [K0 v0.4 DRAFT](<../req/REQ-standard-ptt-K0.md>), [physical plan proposal](<PLAN-physical-ptt-K0.md>), [D2](<../evidence/E-k0-von-qualification.md>), [D3](<../evidence/E-local-audio-readiness-options.md>), [seam](<../req/REQ-von-seam.md>), [core](<../req/REQ-core-contracts.md>) и [P3/P4](<../req/REQ-von-marker-probe.md>). Их новые редакции пока не approved/recorded. Этот файл — плановое предложение V1, не обход SDLC spec→record→planning gate. Source input SHA и будущий approved revision фиксирует handoff; author не делает commit.

Historical fresh-session waiver Team799 закрытого родителя не перенесён. Новая фаза **FRESH_SESSION_REQUIRED**, freshness не PROVEN и нового waiver нет. Approval, immutable record, независимый review и code/run GO — отдельны. Применены ai-native-sdlc-cycle/artifact-contracts, code-quality-loop, docs-manage; версии skills не объявлены. Graphify-граф отсутствует, генерация вне scope; родительский [AGENTS](<../../../AGENTS.md>) прочитан, PBO workflow Arma 3 не перенесён на Reforger. Metadata/changelog — отдельный DOC/Lead scope, здесь не меняются.

**Сейчас нет квалифицированного актуатора.** Реализация не начинается даже при существующем назначении. Production/core/seam remain byte-stable; BLOCKED_NO_ACTUATOR остаётся отказом. Default-off TOOL_ONLY adapter требует отдельного разрешения своего test-only scope с учётом REQ-CORE-002, не неявного обхода production integration boundary.

## 2. Exact proposed code manifest

Только четыре **новых предлагаемых**, сейчас отсутствующих файла. Writer I2 — Ядро; V4 — Инженер радиомоделей, независимо read-only; operator X1 — Lead. I1/ops не пересекаются.

| Proposed path | Обязанность и boundary |
|---|---|
| `tools/RICON_LiveProbe/Scripts/Game/RICON_Live/RICON_LiveRadioSlice.c` | value-only ConsumeNext feed consumer, accepted-origin join/S3/S3b/receiver orchestration, cap4 provenance/terminal ledger; не владеет второй machine, без engine VoN effects |
| `tools/RICON_LiveProbe/Scripts/Game/RICON_Live/RICON_LiveRadioSliceGame.c` | default-off cold-start sole-owner lifecycle façade: OpenColdStart/Dispatch/ConsumeNext/Close semantic API REQ §4.1, authenticated context/resolver, canonical manager sequence delegate; refs только здесь/qualified actuator по общему cap8, no payload transport |
| `tools/RICON_LiveProbe/Scripts/Game/RICON_Live/RICON_LiveRadioActuator.c` | единственная test-only native effect boundary и conditional own restore; concrete implementation только после D2/X1 qualification; no guessed mute/stub APPLIED |
| `tools/RICON_LiveProbe/Scripts/WorkbenchGame/RICON_LiveRadioSliceCheckPlugin.c` | deterministic fake adapter/clock/ownership/source tests; fake не подключается live и не вызывает native receive/capture |

Не менять existing `addons/**`, gproj, старые Game/Workbench files, N0/I1/runner/library/ops. Если четыре файла не позволяют bootstrap, live lease export или qualified observer, это **BLOCKED_MANIFEST_INTERFACE**: Lead расширяет отдельный bounded scope до implementation, review/record повторяются. Нельзя модифицировать protected registry/accessor или I1 logger ради прогресса.

## 3. Точные dependencies, срезы и stopping gates

| Срез / owner | До начала | Выход и stop |
|---|---|---|
| D1 + V1 | D2/D3, четыре DRAFT docs | independent review с unresolved blockers; не code |
| SPEC_RECORD→PLAN | reviewed/approved spec, immutable revision и fresh-session/новый explicit waiver | подтверждённая executable plan revision; этот proposal не считается пройденным gate |
| Interface qualification / D2+Ядро+X1 | отдельный source/read-only plan/permissions | exact authority callbacks/export, native refs, actuator rights/restore/post-gate/direct/native protocol; UNKNOWN останавливает |
| I2_WRITE / Ядро | recorded D1, D2 source qualification, V1, X1 native-path evidence, restore safety, explicit narrow I2 GO Lead | четыре файла, no compile/run; readback/diff/SHA handoff V4 |
| I2_STATIC review / V4 | frozen4 source SHA | no production effects/default-on/data leak, provenance/ownership/limits/negative cases; findings либо static approval |
| I2_COMPILE / X1 | отдельный compile GO + exact reviewed command/build manifest | no Play log/hash; compile не runtime |
| I2_PURE / X1 | отдельный pure plugin gate после review/compile | fake model vectors; не real RICON delivery |
| Native baseline/control / X1 | own executable runtime plan, native input/path/observer/direct квалифицированы | source→B independent evidence без I2 model effect; audio/packet limits separately |
| I2_RUNTIME / X1 | V4, baseline/native path/direct/restore evidence, exact current SHA/build + отдельный START | real RICON model→applied→observed series, все controls/attempts; stop при первом invalid |
| V5 + Lead | raw, seal-before-reveal, terminal/settings/own exits, all LS | независимая оценка LS-01…08, acceptance отдельно; deployment не входит |

K0/N0 — необходимые подготовительные evidence только в своих областях, не квалификация post-gate output/актуатора. I2 не требует создавать искусственный lease у K0: association физического PTT с настоящим accepted lifecycle request должна быть квалифицирована явно. Если H2/AI-event не даёт той же native chain, другой механизм требует новую spec, не fallback в реализации.

## 4. Форматы и ordered implementation proposal

### A. Authority projection (LA-01/02/03/05)

Выбран contract [AuthorityLifecycleFeed schema1 / REQ §4.1](<../req/REQ-local-radio-actuation.md>): cold-start, один TOOL_ONLY owner от Initialize до Close, никаких attach к чужой живой machine/второй lifecycle truth/protected reads. OpenColdStart/Dispatch/ConsumeNext/Close — новые semantic API, не существующие engine methods. Source qualification authenticated callback→request→device/index всё ещё **BLOCKED_AUTHORITY_EXPORT/BINDING**; выбранная схема не означает существующий live export.

Ordered future slice: (1) preflight header/config/catalog и qualified context до Initialize; canonical manager epoch, не CLI seed; (2) каждый public lifecycle call только Dispatch, serialized, authority recheck, before/after epoch; (3) копировать полный request/все input args/result/ordered effects, обработать sweep/terminal даже при rejected/duplicate; (4) join accepted START только по новому LEASE_GRANTED и exact request↔effect identity, duplicate не перезаписывает origin; (5) current player getters + тот же catalog clone + qualified endpoint/listener mapping → полный S3 input; (6) consume/free transaction до следующего call. Любая потеря envelope seq/capacity/integrity останавливает pipeline, не synthetic resync.

`RICON_LifecycleLease` не имеет network/channel/TX index/epoch. Network/channel/correlation/requestSeq — copied accepted request; lease identity/timing/state — effect; epoch — beforeEpoch/current canonical check. TX index отсутствует даже в request: только проверенный server catalog/native endpoint resolver, не frequency/default0. Entity-binding generation отдельна от player generation. Поля native world/process/connection authentication/item/Rpl/antenna/profile/TX/RX/listener lifetime имеют QUALIFICATION_UNKNOWN до source/runtime proof. Device observation/profile — actual server world/catalog, antenna position не подменять origin; ASSUMED/TEST_ONLY RF provenance не measured physics. Четыре reserved файла вмещают façade/consumer, но внешний owner потребовал бы отдельный feed hook contract и revised manifest, не скрытое расширение production.

Value correlation record schema1: run/attempt/tick, epoch, players/generations/entities, devices/indices, lease/request identity+timing/network/channel, pair key/call/pair/device/budget reason, model/config identity, publication call/registry revision and emitted linkId/seq, receiver decision/time/TTL, binding generation, actuator token, observer health generations. Максимум4 correlated records одновременно; нет shared media payload/nonce/keys. Копии, не mutable alias последнего результата.

### B. Pair→publication→receiver (LA-04/07)

Один writer/server tick, полный текущий набор пар. Existing PairService.Evaluate/LinkPublication.Publish/receiver APIs не изменять; source delegates canonical manager sequence, собственный fake allocator запрещён live. Publication не сетевой ACK. ApplyFullResync атомарно устанавливает identity до Receive delta. Stale/equal/mismatch/invalid/expired даёт no new actuation.

Перед заменой registry сохранить old terminal binding+generations. На successful publication exact emitted seq связывается с тем же pair origin: nonterminal через fresh registry, terminal через old linkId→old pair key. MODEL_LOST только valid EVALUATED/reason OK/state LOST/Q0; invalid budget/device refusal/terminal vanish LOST — technical/terminal, не модель. No snapshot на такте не означает новый provenance для старого seq. Non-OK publication/rotation не выдаёт частичного positive effect/ledger commit. Terminal quartet mismatch не действует на новую lease/handle.

Terminal schema строго REQ §6: old `(leaseEpoch,leaseId,player/device/playerGeneration,entityBindingGeneration,bindingGeneration,requestSeq,correlationId)` + eventSeq/effectOrdinal/reason/state/sourceEventTime/dispatchNow/firstLocalTerminalMark + old descriptors/pair key/emitted publication key при наличии. RotateEpoch terminal относится к beforeEpoch до reset allocator; повтор leaseId в новой epoch не match. Не выдумывать terminalAt из отсутствующего effect поля. Пропущенный origin, conflicting same seq, GAP/overflow/rebind → no new effects/ABORT; после mutation integrity failure не объявляется rollback. Cap/age expiry outstanding ownership сохраняет final bounded failure receipt и останавливает окно с X1 containment, не clean eviction.

### C. Adapter/ownership (LA-06/07/09)

Сначала capability-only qualification — точный native method, radio RX/TX/direct scope, authority, readback, baseline и proof эксклюзивного writer + restore. Сейчас конкретный engine API **не выбран**. Implementer не может заполнить enum APPLIED по void return или назвать SetMuteState готовым решением.

Только valid accepted/provenanced MODEL_LOST накладывает own suppression; MODEL_ALLOW снимает только own delta. Повтор seq/token не повторяет call. Preexisting mute/unknown writer — no unmute. Error/abort/gap/expired lease останавливает новые actions; restore лишь при current exact binding+ownership, иначе blocker и containment собственных fresh roles через X1. Journal before call и APPLIED_UNKNOWN при разрыве между call/readback. Не трактовать bool unchanged как доказательство отсутствия чужого writer.

### D. Output/control integration (LA-08/10)

Observer/decoder не читает model DESIRED как output и не прекращает callbacks на LOST. Raw RX/direct/null/mismatch сохраняются до интерпретации. Exact post-gate method должен быть заранее выбран по D2/X1; пока UNKNOWN. Если mute стоит ниже callback, нельзя изобразить его действие выключением callback logging/decoder. Direct — отдельно qualified native source/output с distinct fresh content до/во время/после radio effect.

B decoder получает только public algorithm/alphabet/window/attempt/control mode; не nonce/seed/A logs/selected sequence/per-symbol schedule/model outcome. Seal B terminal/raw/decode до reveal A через независимого verifier. Audio source/output/blind comparison/route isolation D3 блокируют аудиоветвь до конкретного executable X1 плана; process-loopback build20348+ не решение19045. Packet alternative требует доказанного same-path/protocol/remote attribution, не тайминга RX.

## 5. Bounded state, quotas и process contract

Следующие DRAFT caps — ограничения реализации модели/инструмента, не production defaults и не измеренные native guarantees:

| Ресурс | Cap / отказ |
|---|---|
| Роли/направления | ровно A/B; один talker A, listener B, одна активная направленная пара; третий player/device/flow → INVALID_SCOPE |
| Authority projected sets | players2/devices2/endpoints2/listeners1/active bindings1/pairs1; проверять upstream bounded extraction до allocation, не truncation после bulk read |
| Core lifecycle config proposal | maxPlayers2, maxActiveLeases1, maxTerminalLeases4, terminalLeaseTtlMs10000, maxTombstones2, tombstoneTtlMs10000, maxResponseCachePerPlayer4, maxRateEntries4, rateWindowMs1000/rateMaxRequests4; это согласуемые tool fixtures limits, не bypass native lifecycle |
| S3 limits | bindings1/listeners1/players2/devices2/endpoints2/pairs1; no reduction filters вместо preflight |
| S3b limits | maxBindings1/maxPairs1/maxActiveLinks1; current+old/terminal merged output preflight≤2 до allocations; если больше — stop/revised plan |
| RF/publication proposal | tick target≤5Hz/no catch-up; snapshot ttl1000ms/qualityDelta0.1f; authority lease ttl3000ms, no renew, physical K0 safety deadlines строже. Все значения из approved server tool config, не client payload |
| Feed/header/input caps | header1 + synchronous transaction slot1/effects≤16; no queue/reentrancy/next call before consume; eventSeq1…int.MAX−1, before wrap stop. Config выше: rotate max12 effects (lease2+players2×5). Catalog devices2/access IDs≤2/allowed pairs≤4 per device; preflight до clone/Initialize, не truncation |
| Tick/provenance buffers | current tick1 + in-flight seq1 + terminal contexts2 =≤4 value records; active accepted origin1 вложен в current/in-flight, не отдельный cache. Terminal age≤10s от original local terminal mark, no-reset; event loss/GAP invalid |
| Engine retained refs | ≤8/роль по qualified peak/alias/lifetime inventory; новый I2 не получает второй pool8 поверх I1. Invokers/controller/entity/world/VoN/entry/transceiver/input manager/arrays включены; если не доказано — BLOCKED_REF_ALLOCATION_BUDGET |
| Raw per role/window | snapshots240/raw128/reserve32, record≤4096 UTF-8 bytes/strings≤256/raw args≤16; максимум400=1,638,400 bytes; channels физически раздельны |
| Overflow | первый over-cap raw сохранён в reserve; oversize/missing reserve/неполный raw→INCONCLUSIVE_LOG_INTEGRITY; no positive truncation и no retry |
| Окно | ≤45s original local tObservation, no reset по tick/phase/rebind; baseline≥3s/постнаблюдение≤3s внутри cap; heartbeat target1s, gap>2s invalid, это не continuous guarantee |
| Effects | в одном qualified runtime window максимум3 native changes total, включая own restore: suppression1, release-own1, emergency same-owned restore1 при необходимости; emergency не повторяет уже завершённый restore, idempotent token. Два одинаковых вызова не способ подбора |
| Processes | I2 сам processes0; X1 owns максимум2 игровых +runner1/qualified watchdog≤1; no children tools, no attach/kill чужих PID |

DTО/internal arrays bounded отдельно от retained engine refs; raw history хранит значения/diagnostic IDs, не engine object refs. Native allocations у getters/SDK не покрыты этими числами. Доказательство pre-count/indexed enumeration и peak refs пока отсутствует (D2), поэтому **I2_WRITE blocked**. Нельзя скрыто поднять cap или использовать post-count прежнего I0.

Terminal context expiry при outstanding unknown effect: сохранить terminal failure receipt, запретить дальнейшее применение/попытки, передать X1 controlled containment; не бесконечно хранить registry и не потерять конфликт как clean. Пропущенный delayed expiry после освобождения token = reject/no mutation, не обращение к новому binding.

Число окон серии и новые leases выдаёт только будущий X1 план: это не разрешение3+сценариев сейчас. Минимальный конечный дизайн требует свежих положительных open→MODEL_LOST→restored-open contents, no-send/path-break/local-imitation/absent-B controls и direct before/during/after; если они не укладываются в45s и безопасные physical gates, заранее разделить на отдельно разрешённые fresh windows с новым authority binding, а не продлить lifetime. Exact cadence/drain/decoder/число positive windows/независимые права для разрыва пути пока UNKNOWN → BLOCKED_RUNTIME_SERIES. Не переносить packet112-symbol protocol или акустические метрики из другого исследования без qualification.

## 6. Проверки, exact commands и ожидаемая evidence

Разрешённые сейчас doc-only проверки из root:

```powershell
git diff --check -- docs/req/REQ-standard-ptt-K0.md docs/plan/PLAN-physical-ptt-K0.md docs/req/REQ-local-radio-actuation.md docs/plan/PLAN-local-radio-actuation.md
git diff -- docs/req/REQ-standard-ptt-K0.md
Get-FileHash docs/req/REQ-standard-ptt-K0.md,docs/plan/PLAN-physical-ptt-K0.md,docs/req/REQ-local-radio-actuation.md,docs/plan/PLAN-local-radio-actuation.md -Algorithm SHA256
```

New untracked docs требуют полного readback; `git diff --check` их не покрывает до записи в index, поэтому отдельная read-only проверка whitespace/link integrity обязательна без git add. Не stage/commit ради PASS. Runtime statements не повышаются от doc PASS.

Будущие source-review команды, только после создания четырёх файлов и отдельного static gate:

```powershell
git diff --check -- tools/RICON_LiveProbe/Scripts/Game/RICON_Live/RICON_LiveRadioSlice.c tools/RICON_LiveProbe/Scripts/Game/RICON_Live/RICON_LiveRadioSliceGame.c tools/RICON_LiveProbe/Scripts/Game/RICON_Live/RICON_LiveRadioActuator.c tools/RICON_LiveProbe/Scripts/WorkbenchGame/RICON_LiveRadioSliceCheckPlugin.c
Get-FileHash tools/RICON_LiveProbe/Scripts/Game/RICON_Live/RICON_LiveRadioSlice.c,tools/RICON_LiveProbe/Scripts/Game/RICON_Live/RICON_LiveRadioSliceGame.c,tools/RICON_LiveProbe/Scripts/Game/RICON_Live/RICON_LiveRadioActuator.c,tools/RICON_LiveProbe/Scripts/WorkbenchGame/RICON_LiveRadioSliceCheckPlugin.c -Algorithm SHA256
```

**Compile/pure-plugin/runtime exact executable commands сейчас BLOCKED_COMMAND_QUALIFICATION.** Имена plugin/check являются предлагаемыми, не проверенными registered Workbench APIs. Новый CLI proposal: `-riconRadioSlice=1 -riconRadioSliceRun=<32hex> -riconRadioSliceRole=A|B`; default-off, malformed/unknown roles/conflicting other runtime RICON gates → no bootstrap/effects. Сам opt-in не ARM. I2 и I1 одновременно запрещены до отдельного combined manifest/ref-budget review; старые D0/V0/I0/N0/bench emit modes не импортируются. No generic mode/script/exe inputs.

Перед каждым future compile/process START X1 обязан записать exact command с executable/gproj/world/plugin/args/run/profiles/log roots, все hashes/build, соответствие отдельному approved scope и отсутствие чужих процессов. Сейчас runner/path не выбран и не входит в I2: выдумывать executable command, reuse noFocus runner или добавлять новый ops запрещено. Отсутствие полной команды — blocker, не постфактум receipt. `BLOCKED_COMMAND_QUALIFICATION` закрывается revised recorded executable plan, не GO по названию класса.

V4 pure case matrix (каждый expected outcome отдельно, без real effects):

| IDs | Обязательные traces | Ожидание |
|---|---|---|
| LA-T01 | no opt-in, bad/combined mode, wrong authority/forged owner | effects0, source registries unchanged |
| LA-T02 | lifecycle accepted vs fake/expired lease, generation/session rotation | only real qualified inputs; terminal≠MODEL_LOST |
| LA-T03 | exact valid model LOST vs invalid budget/device/state mismatch/missing pair | MODEL_LOST только exact valid provenance, others TECHNICAL/TERMINAL |
| LA-T04 | publication non-OK/partial/resync; empty first resync; duplicate/stale/unsupported schema/TTL boundaries | no new effects, no false accepted |
| LA-T05 | same seq different origin, no emitted snapshot new tick, mutable alias overwrite | reject/unknown, old immutable provenance сохраняется |
| LA-T06 | terminal not in new registry, delayed old epoch/lease quartet | old saved context, new binding untouched |
| LA-T07 | index out of range/null/unresolved proxy/rebind/foreign writer/preexisting mute | no call/unmute; explicit BLOCKED/cleanup failure |
| LA-T08 | setter error/unknown partial apply/readback mismatch/restore conflict | APPLIED_FAILED/UNKNOWN separate, never OBSERVED_PASS |
| LA-T09 | gap/clock rollback/quota/off-by-one/reserve/log missing/terminal age cap | invalidate, no retry, bounded receipt |
| LA-T10 | decoder/raw stream continues on LOST; B reads A/payload/seed; pre-gate output | no LS-05/independence PASS; no hide/drop fix |
| LA-T11 | exact own apply→own release, repeats/late expiry/current handle replaced | idempotence; no restore на новом handle |
| LA-T12 | healthy post-gate/direct before-during-after vs disabled/missing observer | evidence separated; broken observer INCONCLUSIVE |
| LA-T13 | accepted channel1/channel2 при одинаковом lease DTO; duplicate cached OK; rejected/duplicate PTT со sweep | exact grant↔request join; no inferred channel/index; terminal effects не теряются из-за resultCode |
| LA-T14 | cold-start vs late attach; bypass Dispatch/reentrant call; envelope GAP/duplicate/seq exhaustion; slot/effects/catalog cap±1 | late attach/bypass/reentrancy/over-cap reject; после mutable call integrity failure/abort, не rollback/PASS |
| LA-T15 | RotateEpoch old terminal/new same leaseId; entity changed при неизменной player generation; terminal age10s boundary/outstanding ownership | beforeEpoch/old binding preserved; rebind abort; failure receipt, no clean eviction |

Pure synthetic traces получают только CONTRACT/PURE_MODEL evidence, не runtime real model/audio/LS PASS. Ни один expected fault не разрешает simulate actual RX. Code parity/compile и live evidence записываются отдельно с raw/command/source/build SHA; здесь всё NOT_RUN.

## 7. LS mapping, stress test и rollback

LA-01/02/05→LS-01; LA-10+qualified input→LS-02; LA-08/10→LS-03/04; LA-03/04/06/07/08→LS-05; LA-06/08→LS-06; LA-06/09→LS-07; LA-04/07/09/10→LS-08. Входной intent LS-01 PARTIAL; LS-02…08 не закрыты. Final PASS требует **всех** LS, не суммы отдельных source/static passes. D2/D3 unknown blockers сохраняются.

**Что сломается:** authority/model provenance, mapping stable IDs/handles, старый terminal/new lease, ownership restoration, прямой голос и независимость B. **Самый рискованный шаг:** первый native call на target, затем восстановление при ambiguity/внешнем writer; до proof безопасного own restore call не разрешён. **Отклонено:** изменение pure seam/enum, второй epoch/lease allocator, искусственный модельный LOST, manual mute как LS-05, Social/direct mute, SetCapture/частота/power fallback, RPC/file payload, decoder off, unlimited logging/terminal registry. Причина — нарушение core/intent, опасное вмешательство либо ложноположительное доказательство.

Migration/deploy нет. Rollback будущей реализации — оставить default-off, не подключать production, снять только свои observers/subscriptions, conditional restore own delta на том же binding при сохранённом ownership, иначе STOP/сохранить cleanup blocker и передать оператору controlled teardown собственных fresh процессов. Никакого unconditional unmute, cleanup чужих состояний/профилей/процессов или удаления evidence. Точные exit/SHA/settings receipts X1; отсутствие terminal/exit не PASS от exit0.

## 8. Ledger и самодостаточный handoff

| Состояние | Факт D1 | Кто далее |
|---|---|---|
| Design / plan proposal | исторический D1 четыре docs DRAFT; corrective v0.2 только actuation REQ/этот plan, input a04a14f + Team scope3097 | V4 независимо проверяет feed/ledger delta, затем Lead |
| Approval / immutable record | NOT_APPROVED / NOT_RECORDED новой редакции | Lead организует запись при полномочии |
| Fresh transition | REQUIRED, не PROVEN/WAIVED | отдельный новый переход, старый waiver не наследуется |
| Qualified actuator/restore/post-gate/direct/export | BLOCKED/UNKNOWN | D2/Ядро/X1 в отдельных scopes |
| I2 code/static tests/compile/pure/runtime | NOT_RUN / WRITE_BLOCKED | только после dependencies и GO, затем V4 |
| Final LS / release | NOT_RUN / deployment OUT_OF_SCOPE | V5→Lead, не авторская self-acceptance |

Corrective v0.2: V4 читает две полные actuation редакции по новым SHA и проверяет LA-T13…15/REQ §4.1/§6, caps/owner/old binding; Lead принимает отдельно. Это завершение doc correction, не build. K0/N0 bytes не меняются; borrowed cap8/Q2 timing/B07–B09, native/source qualification, record/fresh-session и I2 GO сохранены. Metadata/changelog обновляет DOC/Lead по Team handoff, не автор в этом scope.

Исторический handoff V1: V1 читает baseline a04a14f, четыре D1 docs и D2/D3 по handoff SHA, parent AGENTS, core/seam/P3/P4. Выход V1 — findings либо reviewed draft с конкретным списком blocker owners; никакого closure UNKNOWN через proposed interface. Lead после review/record/fresh gate фиксирует exact executable manifest/commands/qualification; I2_WRITE только после D2/X1 native-path evidence и отдельного GO, а не от назначения. Автор I2 не проверяет собственную работу: handoff V4 с4 SHA/diff/readback; X1 — единственный процессный оператор; V5 — отдельная конечная проверка. Данный handoff не разрешает код, процессы, ввод, mic/audio/Windows actions или commit.
