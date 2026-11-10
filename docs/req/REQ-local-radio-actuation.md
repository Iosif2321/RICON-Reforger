# D1 — TOOL_ONLY контракт реального RICON→radio-only воздействия

Версия 0.2, 2026-11-02. Автор: Ядро / Логика / Последовательность. Accountable/решения gate: Lead. **DRAFT / V4_CORRECTION_PENDING / BLOCKED_NO_QUALIFIED_ACTUATOR / IMPLEMENTATION_BLOCKED / RUNTIME_BLOCKED**. Delta v0.2: corrective D1, Team scope3097 — выбран cold-start sole-owner feed и bounded provenance contract; это не approval/record/source/runtime PASS.

## 1. Источник истины, traceability и переходы

Git RICON-Reforger, baseline `a04a14f` — источник входных редакций; новые D1/D2/D3 не recorded approved inputs до явной фиксации. Team task:02bf7e6e-41c2-44dd-a991-4e037ff33e5a задаёт doc-only scope D1; I2 task:24f59dc3-f98e-44e0-93c6-5e85f277c56e сейчас только read-only interfaces. Родитель task:b5e95a66-d9cc-40b5-8bca-649dc64c9c5d — продолжение к валидной передаче звука на одном ПК; утверждённый [intent v0.2](<INTENT-local-radio-slice.md>) допускает также независимое содержимое только по доказанному тому же пути/протоколу. Повторного утверждения цели не просим, конкретный неизвестный механизм за утверждённый не выдаём.

Нормативные read-only входы: [core v0.9](<REQ-core-contracts.md>), [lifecycle v0.4](<REQ-lifecycle.md>), [S3 v0.2](<REQ-pair-service.md>), [S3b v0.2](<REQ-link-publication.md>), [чистый seam v0.2](<REQ-von-seam.md>), [marker probe v0.4, P3/P4](<REQ-von-marker-probe.md>), [K0](<REQ-standard-ptt-K0.md>), новые [D2](<../evidence/E-k0-von-qualification.md>)/[D3](<../evidence/E-local-audio-readiness-options.md>). Они не доказывают complete live integration.

Процесс: historical Team799 **FRESH_SESSION_WAIVED** был в закрытой task:ba901e07-814a-4803-a1ae-3655d281a6b7, на новые переходы не перенесён. Для следующей фазы **FRESH_SESSION_REQUIRED**, не proven/waived. D1 может дать только DRAFT и review handoff; approval/immutable record/fresh session/отдельный I2 GO различны. Применены ai-native-sdlc-cycle/artifact-contracts, code-quality-loop, docs-manage; версии skills не объявлены. Graphify-граф отсутствует, генерация вне scope. Код, intent, evidence, ops и metadata автор не меняет; changelog/goal status синхронизирует DOC/Lead, без расширения четырёхфайлового D1 scope.

## 2. Цель и строгое исключение production

Требуемая цепь, **пока не реализованная целиком**:

`реальный authority lifecycle/device/PTT lease → RICON_PairService.Evaluate → RICON_LinkPublication.Publish → accepted RICON_LinkSnapshotReceiver → проверенный binding + provenance → отдельный qualified TOOL_ONLY radio adapter → native readback → независимый post-gate B-output`.

Pure [RICON_VoNIntegration](<../../addons/RICON_Core/Scripts/Game/RICON/Network/RICON_NetworkContracts.c#L347-L401>) не становится актуатором: валидный snapshot/expiry остаётся `BLOCKED_NO_ACTUATOR`, enum не меняется. Этот результат — refusal/no effects, **не разрешение** вызвать engine в другом месте. D1 не даёт исключение из REQ-CORE-002: production имеет единственную интеграционную границу, новый test-only adapter должен получить отдельное reviewed разрешение своего ограниченного scope, не использоваться evaluator/routing/UI и не подключаться production. Без такого решения **BLOCKED_TOOL_ADAPTER_AUTHORIZATION**.

Не входят: `addons/**`, формулы RF, production seam, Social/whole-player mute, direct mute, SetCapture, selector/frequency/range/power/key как заменитель неизвестного актуатора, custom RPC/file/seed data plane, decoder disable/hide/drop, injected capture/RX, DLL/driver/новая бинарная RE. Один ПК/две роли/один isolated radio binding — не per-flow/multiplayer/quality/Workshop acceptance.

## 3. Требования и ответственность

| ID | MUST / запрет | Intent | Проверка/owner |
|---|---|---|---|
| LA-01 | Authority получает player/connection/entity/device/epoch из проверенного engine/server context; client payload и локальные счётчики не источник истины | LS-01, §5 | adversarial identity traces; Ядро/V4 |
| LA-02 | Lease создаётся реальным принятым PTT_START lifecycle, active/issuedAt/TTL/terminal проверяются; физический K0 сам не экспортирует lease | LS-01/05 | forged/expired/terminal cases; Ядро |
| LA-03 | Полный S3 input одного server tick; S2/profile/antenna provenance и allowed network/channel известны, нет synthetic budget/state/seed | LS-05 | source audit + runtime provenance; V4 |
| LA-04 | Каждая DESIRED action связана с конкретными publication/accepted snapshot seq и тем же результатом S3; LOST без origin недостаточен | LS-05/08 | exact correlation, technical LOST controls; V4 |
| LA-05 | Stable descriptor проецируется на квалифицированный native handle по reference/replication provenance, не частоте/имени/entry number | LS-01 | stale/rebind/index/handle tests; D2/Ядро |
| LA-06 | Только доказанный isolated radio-only scope; rights, exclusivity, restore contract до первой mutation | §5.1/7, LS-06/07 | ownership conflict/readback; D2/V4 |
| LA-07 | DESIRED / APPLIED / OBSERVED и MODEL_LOST / TECHNICAL_ERROR / ABORT / UNKNOWN раздельны | LS-05/08 | enum/receipt checks; V4/V5 |
| LA-08 | Независимый post-gate B-output и direct до/во время/после; callbacks/decoder не подавляются | LS-03/05/06 | native path X1 + независимый V5 |
| LA-09 | Bound state/time/events; rebind/gap/overflow/clock invalidates; unknown cleanup не PASS | LS-07/08 | deterministic negatives + runtime terminal; V4 |
| LA-10 | Содержимое B независимо, seal до reveal A; control plane не переносит payload/расписание | LS-02/03/04/08 | dataflow audit; V1/V5 |

## 4. Известные реальные interfaces и недостающие adapters

Существуют чистые data/model APIs; наличие класса не означает engine integration:

| Реальная граница на baseline | Что можно использовать read-only | Чего нет / blocker |
|---|---|---|
| [RICON_LifecycleStateMachine](<../../addons/RICON_Core/Scripts/Game/RICON/Core/RICON_Lifecycle.c#L421-L434>) | Initialize, Connected/Registered/EntityBound, ProcessPtt/Tick; public GetSessionEpoch/GetPlayerGeneration/GetPlayerEntityId | engine event order/authenticated sender/native entity/device mapping квалифицировать; fake Initialize trace не live |
| [RICON_LifecycleLease](<../../addons/RICON_Core/Scripts/Game/RICON/Core/RICON_Lifecycle.c#L391-L402>) | lease values; [effects/result](<../../addons/RICON_Core/Scripts/Game/RICON/Core/RICON_Lifecycle.c#L254-L285>) | lease не содержит network/channel/TX index и собственного sessionEpoch; нет выбранного public bounded exporter полных live lease records; не читать protected registry через новый wrapper догадкой |
| [RICON_RadioObservation/RfProfile](<../../addons/RICON_Core/Scripts/Game/RICON/Device/RICON_RadioDeviceAdapter.c#L19-L61>) | device/index/position и server profile; opaque tuning, keys не логировать | native→server stable device, antenna origin, profile provenance и reference lifetime неизвестны; TEST_ONLY/ASSUMED профиль не measured RF truth |
| [RICON_TransmissionBinding и PairServiceInput](<../../addons/RICON_Core/Scripts/Game/RICON/Core/RICON_PairService.c#L26-L84>) | explicit DTO epoch/lease/player/generation/device/index/network/channel/active/timing | DTO caller-supplied, не подтверждает authority; нельзя заполнить ручным synthetic fixture ради real RICON PASS |
| [RICON_PairResult](<../../addons/RICON_Core/Scripts/Game/RICON/Core/RICON_PairService.c#L87-L103>) | value-only pairCode/hasBudget/valid/reason/state/Q | полное linkSeq provenance надо хранить вне snapshot |
| [RICON_LinkPublication.Publish](<../../addons/RICON_Core/Scripts/Game/RICON/Core/RICON_LinkPublication.c#L566-L675>) | snapshots/new registry; manager source delegate без нового allocator | publication — выдача данных, не доставка; terminal entries пропадают из new registry |
| [RICON_LinkSnapshotReceiver](<../../addons/RICON_Core/Scripts/Game/RICON/Core/RICON_LinkSnapshot.c#L105-L227>) | ApplyFullResync/Receive/IsVisible и accepted decisions | bool/decision переданный клиентом не authority receipt; TTL presentation не lease |
| [RICON_VoNDeviceBinding](<../../addons/RICON_Core/Scripts/Game/RICON/Network/RICON_NetworkContracts.c#L323-L345>) | stable descriptor, оба explicit indices, talkerLeaseId | native handle/верхняя граница index/generation/readback отсутствуют |

Предлагаемые **семантические interfaces, не имена существующих engine методов**:

1. `AuthorityInputAdapter`: проверенные engine callbacks и authenticated owner context → cold-start sole-owner `AuthorityLifecycleFeed` (§4.1). Epoch один canonical manager/lifecycle, не новый runId/time seed. Server authority перепроверяется на mutation boundary. Нет qualified input/feed → BLOCKED_AUTHORITY_EXPORT; не создавать вторую альтернативную lifecycle truth и не подключаться к чужой уже живой machine.
2. `DeviceBindingResolver`: из server IDs/generations/world + native item/Rpl provenance → exact local TX/RX component/transceiver refs и catalog bounds. При unresolved proxy/authority/lifetime unknown — BLOCKED_BINDING. Две null не MATCH, pointers разных процессов не сравниваются.
3. `PublicationProvenanceLedger`: immutable tick-origin + emitted seq + terminal context, до замены registry. Только diagnostic/control metadata, не media payload.
4. `QualifiedRadioActuator`: capability/rights/baseline/ownership/readback/restore receipts на одном радио; сегодня implementation **не выбран**. H2 BaseTransceiver.SetMuteState лишь кандидат из marker probe, не одобренный native actuator.
5. `PostGateObserver` + direct control: независимо доказанные реальные выходы B с непрерывным health, неизменённым decoder и source attribution; сейчас BLOCKED D2/D3/X1.

I2 может реализовать adapter только после qualification в этих interfaces; неизвестное API не заменять stub с APPLIED=true. Отсутствие законного export/handle/readback/restore в четырёх файлах I2 → revised manifest/contract и новый review через Lead, не изменение core/accessor.

### 4.1. AuthorityLifecycleFeed v1 — выбранный TOOL_ONLY контракт (LA-01/02/03/09)

Это **новый semantic API**, не существующий engine метод. Выбор corrective D1: `SliceGame` единолично владеет одним новым lifecycle instance от Initialize до terminal в отдельном TOOL_ONLY scope; production machine не дублируется. Нельзя attach mid-session, переиграть fixture как live, наследованием читать protected arrays или отдать mutable machine consumer-у. Пока authenticated engine callbacks/native mapping не квалифицированы, feed не может объявить LIVE_READY. [Менеджер](<../../addons/RICON_Core/Scripts/Game/RICON/Core/RICON_CommsManagerComponent.c#L59-L112>) сегодня не содержит этой machine; готового live owner/export нет.

**Ownership API (предлагаемый):** `OpenColdStart(canonicalManager, qualifiedContext, config, catalog)` один раз при default-off→qualified переходе; `Dispatch(kind, sourceContext, input)` — единственный serialized вход ко всем public lifecycle mutations; `ConsumeNext(expectedEventSeq)` — синхронная передача одной immutable value transaction в Slice; `Close(reason)` — прекращение новых inputs/передача terminal cleanup, не native unmute. Эти имена не квалифицированные engine signatures. Consumer не вызывает lifecycle/менеджер allocation сам. Другой caller, reentrancy, позднее подключение или недостаточный preflight capacity → INVALID_FEED и запрет дальнейших effects. Изначально machine epoch задаёт manager; before/after getter сверяется с canonical manager на границе S3. Менеджерную rotation нельзя имитировать локальным счётчиком; рассогласование → full-resync/abort, не silent repair.

| Schema1 record | Точные поля / источник |
|---|---|
| Session header (один) | schema=1, local diagnostic run/attempt, authority process/world identity, feedOwnerGeneration, canonicalEpoch, copied lifecycle config, initial cloned catalog. Последние IDs — guard/диагностика, не второй authority epoch. Native process/world source пока UNKNOWN |
| Transaction envelope | eventSeq начиная с1, kind, sourceEventTimeMs, dispatchNowMs, beforeEpoch/afterEpoch из GetSessionEpoch вокруг вызова, copied method input, resultCode/hasCachedResult/cachedCode/hasLease/leaseId/state/reason, effects[] в исходном порядке. eventSeq/effectOrdinal — локальный order, не linkSeq/lease allocator |
| PTT input | полная копия всех полей RICON_LifecyclePttRequest: method, nowMs, schemaVersion, correlationId, authenticatedPlayerId, payloadPlayerId, sessionEpoch, generation, requestSeq, deviceId, networkTypeId, channelId, serverTtlMs, leaseId. AuthenticatedPlayerId приходит от qualified server sender, не от client payload; serverTtlMs от tool config |
| Other input | точные аргументы Initialize/Connected/Registered/EntityBound/Changed/Unbound/Disconnected/Tick/RotateEpoch; sourceContext хранит connectionId и bindingGeneration там, где применимы. Каждый вызов проходит Dispatch, включая rejected и duplicate |
| Effect copy | type + все value-поля RICON_LifecycleEffect; смысл только по type. Не считать default0 отсутствующего epoch/terminalAt истинным значением. Никаких engine refs в feed |
| Context revision | catalogRevision, endpointBindingRevision; players≤2 со state/generation/entity через public getters для известных owner IDs, connection из Connected input; devices≤2 из того самого Initialize clone; native endpoint descriptors≤2 и listener≤1 только от qualified resolver |

**Accepted join.** Создать origin только по новому LEASE_GRANTED effect в том же ProcessPtt transaction, с method START и result OK; связать `(beforeEpoch,leaseId)` с copied `(authenticatedPlayerId,generation,requestSeq,correlationId,deviceId,networkTypeId,channelId)` и issuedAt/ttl из effect. Проверить равенство player/device/generation request↔effect. DUPLICATE без нового grant не заменяет origin даже при cached OK. Effects обрабатывать **до** классификации resultCode: [BeginEvent](<../../addons/RICON_Core/Scripts/Game/RICON/Core/RICON_Lifecycle.c#L650-L695>) может истечь старую lease на rejected/duplicate PTT. Terminal join использует сохранённый origin и old binding, не STOP network/channel по умолчанию.

[Lease/grant](<../../addons/RICON_Core/Scripts/Game/RICON/Core/RICON_Lifecycle.c#L837-L870>) теряет network/channel/correlation/seq; два разрешённых START с разными channel, но одинаковыми остальными параметрами дают одинаковые lease values. Поэтому поздний ExportLeases getter не исправляет B06. `txTransceiverIndex` вообще отсутствует в request: только server endpoint mapping device→native item/component→explicit index. [TryRead](<../../addons/RICON_Core/Scripts/Game/RICON/Device/RICON_RadioDeviceAdapter.c#L174-L195>) проверяет index/ref и читает power/tuning/key, но не доказывает stable mapping/ownership/antenna. Entity-binding generation увеличивает owner на смене/unbind независимо от lifecycle player generation; rebind прекращает попытку. Связь native event→request, connection authentication, world/item/Rpl provenance, antenna/profile, TX/RX indices/lifetime/listener source остаются **QUALIFICATION_UNKNOWN**, не закрыты feed schema. Частота и default index0 запрещены как замена.

**Capacity/atomicity.** Один header, одна transaction slot, максимум16 effects/transaction; players2/devices2/endpoints2/listener1/active lease origin1. Catalog preflight: access IDs≤2 и allowed pairs≤4 на device, без truncation; превышение отклоняется до Initialize. При config plan §5 (active lease1, players2) RotateEpoch даёт максимум2 lease effects +5 на player =12, slot16 достаточен с запасом, не разрешает новые event kinds без пересчёта. Начать transaction можно только после полного ConsumeNext предыдущей; все variable inputs preflight до clone/call. eventSeq ограничен 1…int.MAX−1, перед переполнением stop без wrap. Если после lifecycle mutation output нарушил cap/consume failed, состояние уже могло измениться: пометить FEED_INTEGRITY_FAILED, запретить S3/native effects, сохранить bounded failure receipt; не объявлять rollback/PASS.

Feed не raw history: после обработки transaction slot освобождается; accepted metadata живут в current provenance record, terminal — только в terminal slots (§6/plan §5). Исторические записи только bounded raw channels, без ключей шифрования/media payload. GAP, out-of-order, повтор envelope seq, overflow, clock rollback, catalog/binding revision mismatch или owner/world generation change → ABORT/no new effects, conditional own cleanup по §8. Не обрезать массивы/не пропускать ошибку ради capacity. DTO cap не оправдывает engine refs: общий borrowed-ref cap8 на роль с I1 остаётся обязательным, qualification peak пока BLOCKED.

## 5. Binding и identity contract

Для каждой попытки до effects сохранить value record:

- schema/run/attempt/authority process/world generation, canonical sessionEpoch;
- connectionGeneration, entity-binding generation обеих сторон, authenticated player IDs;
- stable talker/listener device IDs, explicit TX/RX catalog indices, item/Rpl provenance и локальная reference projection с bindingGeneration;
- talkerLeaseId, accepted PTT request correlation/seq, issuedAt/ttl/state и network/channel из принятого authority context;
- `(leaseId, rxDeviceId, rxTransceiverIndex, rxPlayerId)` pair key, publication tick, `(epoch, linkId, linkSeq)`, upstream accepted decision, local receipt time/TTL validity;
- actuator capability/source/build identity, exclusive ownership token/proof, exact native handle generation, observer subscription generation.

Lease epoch берётся из accepted lifecycle context/effect и проверяется с текущим authority epoch; не из отсутствующего поля lease DTO. Network/channel/TX index привязываются к **тому же** accepted request и server device catalog, не восстанавливаются по частоте. Не логировать encryption key или чужие hidden routes.

Before each mutation/restore сверяются все relevant authority/lease/binding/engine ownership generations и отсутствие конкурирующего writer. Accepted decisions только ACCEPT_FIRST/ACCEPT_NEWER/ACCEPT_RESYNC от настоящего receiver. Перед первым delta обязателен atomic full resync; не вызывать CompareAgainst(null) вместо receiver для обхода identity. Неподдержанная schema, stale/equal seq, invalid identity, expired presentation TTL, missing lease/provenance, unknown index/ref → no new effects + явный BLOCKED/INVALID; expired lease отдельно terminal, не model loss. Rebind заканчивает попытку, не автоматический retry/requalify.

## 6. MODEL_LOST и terminal provenance

Snapshot не несёт причины и device binding. [BuildPlan](<../../addons/RICON_Core/Scripts/Game/RICON/Core/RICON_LinkPublication.c#L389-L412>) публикует LOST/Q0 и для valid model LOST, и для invalid budget/device-query/state mismatch/пропавшего ключа. Поэтому:

- **MODEL_LOST** только S3 call OK, exact pair key того же tick, pairCode EVALUATED, hasBudget=true, valid=true, reason=OK, state LOST, Q0, publication OK и именно этот emitted `(epoch,linkId,linkSeq)` затем accepted с той же projection. Модель/config/profile provenance сохраняются; это model outcome, не измерение реальной RF-потери.
- **MODEL_ALLOW** — такая же valid/evaluated provenance, state WEAK/DEGRADED/CLEAN и finite Q в допустимом domain; DRAFT binary policy не симулирует analog degradation.
- **TECHNICAL_ERROR** — invalid budget, numeric/domain/device error, upstream refusal, state mismatch, protocol/clock/identity failure. Даже LOST snapshot не переименовывать MODEL_LOST.
- **LEASE_TERMINAL** — stop/expiry/revoke/disconnect/session rotation с lifecycle reason; **ABORT** — safety/operator/focus/gap/ownership violation. **UNKNOWN_PROVENANCE** — отсутствует доказанная корреляция. Все отдельны от model outcome и от measured no-output.

Ledger keyed by `(epoch,linkId,linkSeq)` хранит копии value-only pair/request/publication origin, не mutable reference на «последний» result. Unchanged publication tick без нового snapshot не даёт права приписать старому seq новую причину; такая классификация остаётся происхождением его выдачи. Если требуется новое действие по changed cause без emitted snapshot, это новый contract revision, не повтор старого seq.

Перед Publish/заменой registry preserve старый checked descriptor и generations для потенциального terminal: [TERMINATE пропускается в fresh registry](<../../addons/RICON_Core/Scripts/Game/RICON/Core/RICON_LinkPublication.c#L633-L670>). Terminal snapshot связывается с old context, не lookup только в новом registry. Delayed expiry quartet `(epoch,talkerPlayerId,talkerDeviceId,leaseId)` должен совпасть с сохранённым binding; old epoch/lease никогда не действует на новый handle/передачу. Сохранённый descriptor не даёт прав после rebind. План ограничивает terminal lifetime/число records; нерешённое restore при истечении срока остаётся cleanup failure и не забывается как успех.

Terminal schema1: `(leaseEpoch,leaseId,playerId,deviceId,playerGeneration,entityBindingGeneration,bindingGeneration,originRequestSeq,originCorrelationId,terminalEventSeq,effectOrdinal,terminalReason,leaseState,sourceEventTimeMs,dispatchNowMs,firstLocalTerminalMark)` + copied old TX/RX descriptor/pair key и, если выдавался, exact `(publicationEpoch,linkId,linkSeq)`. Epoch terminal берётся из transaction.beforeEpoch и сохранённого origin: RotateEpoch сначала завершает старые leases, затем очищает registry и обнуляет allocator; новые leaseId могут совпасть. `m_iTerminalAtMs` не экспортируется effect-ом — не выдавать dispatch timestamp за engine terminal measurement. Order/event time/локальный age — разные поля. Missed origin→UNKNOWN_PROVENANCE/no actuation.

Ledger commit только при publication OK: exact emitted snapshot связывается с current pair по fresh registry, terminal по old registry linkId→old pair key до замены. Non-OK не даёт partial ledger commit. Дубликат ключа с иным origin → integrity failure. Current1 + in-flight1 + terminal2 = общий cap4 correlation records, accepted lease metadata вложены в current/in-flight, не отдельный безлимитный cache. Terminal хранится≤10s от первого local mark, без продления на повторном effect/receipt; after expiry неизвестное outstanding ownership превращается в один final failure receipt и остановку окна/containment X1, не clean eviction. Raw reserve и raw caps плана обязательны. Зафиксированная terminal запись никогда не даёт права восстановить новый native binding.

## 7. DESIRED / APPLIED / OBSERVED и state machine

`DISABLED → QUALIFIED_READ_ONLY → OWNERSHIP_READY → ACTIVE → STOPPING → TERMINAL`. Unknown precondition не пропускает QUALIFIED_READ_ONLY. Новый run никогда сам не начинает START/Play/изменение радио. Один isolated receive scope B; две стороны и один источник, не concurrent flows.

| Уровень | Содержимое записи | Что не следует |
|---|---|---|
| DESIRED | binary MODEL_ALLOW/MODEL_LOST либо technical/terminal/abort reason + exact provenance | setter вызван, native delivery изменена |
| APPLIED | точный разрешённый native call/target + before/after readback + exclusive ownership receipts + generation; outcome CONFIRMED/FAILED/UNKNOWN/NOT_ATTEMPTED | void возврат или совпадение bool не доказывают causal delivery/radio-only |
| OBSERVED | raw неизменённый post-gate B-output + source/health/validity + результат независимого сравнения и direct controls | отсутствие callback при broken observer не MODEL_LOST и не отрицательный PASS |

DRAFT policy после отдельного actuator qualification: valid MODEL_LOST может наложить только собственное radio-only suppression на текущий qualified binding. MODEL_ALLOW может **снять только собственное** suppression при сохранённом ownership token/generation и baseline; baseline already-muted не превращать в ALLOW/unmute. Technical error, abort, provenance/gap/ownership loss запрещают новые effects; safe restore только при всё ещё доказанном владении тем же state, иначе STOP/cleanup blocker + containment своих fresh процессов через X1. Не выполнять «fail closed» через произвольный mute, не обещать native block без разрешённого scope. Failure cleanup и model suppression — разные причины.

Количество действий ограничено планом; command once/idempotence на seq+binding+effect token. Stale/equal accepted receipt replay не повторяет call. После ABORT положительная классификация запрещена даже если позднее появился правильный output.

## 8. Ownership / restore contract до mutation

Обязательны positive source/runtime proof области radio RX/TX и отсутствия direct/whole-player влияния; authority права, baseline/readback exact target/generation, exclusive writer и release/restore semantics. False bool getter не exclusive ownership. Preexisting true/unknown mute — STOP до действия либо read-only observation без присвоения себе состояния; нельзя снимать его на MODEL_ALLOW.

До call сохранить own journal token, исходное значение/identity, intended delta; после call прочитать qualified native state и receipt. Если setter мог сработать, но readback/receipt не завершён, состояние APPLIED_UNKNOWN; rollback не угадывать. Restore — условное удаление только собственного delta с доказательством, что внешний writer не вмешался. Если API не поддерживает различение writer/ownership и свежая isolated fixture не обеспечивает доказанную эксклюзивность, **BLOCKED_RESTORE_OWNERSHIP до первого call**. На new handle после rebind не restore. No-op baseline restore не даёт LS-07 при unverified outstanding effect.

Exit/завершение собственных изолированных ролей допустимо как bounded containment только в отдельном X1 plan; оно не доказательство native release/drain, возврата глобальных настроек или direct-preservation. Не трогать пользовательский/платформенный mute, общие очереди, settings и чужие процессы.

## 9. Независимость B и controls

По D2 OnReceive может стоять до playback/mute; точка не квалифицирована. Если raw RX/decoded frame продолжается при реальном downstream suppression, observer **не годится для LS-05**, и его запрещено скрыть/выключить ради результата. Нужен независимый post-gate output, sender/RX attribution и эквивалентность проверяемому player-voice/native участку. Direct отдельно проверяется до/во время/после того же radio воздействия, с собственным здоровым output, не по отсутствию null callbacks.

Аудиоветвь: D3 INPUT/OUTPUT/OBSERVER BLOCKED. Старый общий USB вместе с чужой сессией не B-output; process-loopback sample build≥20348 не готовое решение Windows19045. Физический источник, game capture binding, физический изолированный B-output/слепой наблюдатель, утечки и отдельное разрешение recording/routing обязательны, если выбран звук. Пока не открыты.

Packet-ветвь: допустимый native input/remote sink/protocol/post-gate stage квалифицируются отдельно; ни AI-event, timing callback, одна частота, CRC, local PlaySound или RPC не эквивалентность. B получает только public decoder/alphabet/attempt/control-window metadata, не выбранный payload/seed/расписание/TX log/outcome. B сохраняет decode/NO_FRAME/INVALID и raw terminal, verifier фиксирует seal/SHA до reveal A; затем сравнение. Нельзя передавать payload даже через «diagnostics». Нужны fresh contents, положительные до/после MODEL_LOST и healthy negatives: no-send, real disconnected native path, local TX imitation without send, absent B как verifier-control без ложного B receipt. Exact calibration/число окон фиксирует X1 plan до run; отсутствие доступного safe control — BLOCKED, не ослабление LS.

## 10. Приёмка, риски, unknown и handoff

| LS | Нужное evidence | Статус D1 |
|---|---|---|
| LS-01 | real authority chain+stable handles/lease/gens без gap | PARTIAL по intent; integration BLOCKED_AUTHORITY_EXPORT/BINDING |
| LS-02 | квалифицированный input и отличимое свежее содержимое неизвестное B | BLOCKED D2/D3 |
| LS-03 | independent content B + same-path/protocol либо isolated audio | BLOCKED_POST_GATE_OUTPUT |
| LS-04 | все frozen controls/повторы с healthy observer, без retries/filtering | NOT_RUN |
| LS-05 | valid model provenance→native applied→changed/restored post-gate output | BLOCKED_NO_QUALIFIED_ACTUATOR |
| LS-06 | отдельный direct output до/во время/после | BLOCKED_DIRECT_CONTROL |
| LS-07 | only own restore/detach/process exits, нет outstanding unknown | NOT_RUN / OWNERSHIP_BLOCKED |
| LS-08 | source/build/manifest/raw/все attempts/SHA/seal-before-reveal | NOT_RUN, этот doc не runtime evidence |

**Что сломается:** ошибочное сопоставление stable device/native handle, attribution причины чужому seq, technical LOST как модель, безусловный unmute, pre-gate callback как измерение suppression, межпроцессный leak ожидаемого содержимого. **Самый рискованный шаг:** первый native radio effect при недоказанных ownership/restore/direct boundaries. **Отклонено:** engine call из чистого seam; synthetic snapshot/manual mute как RICON; whole-player Social; packet RPC/shared seed; фильтрация B/остановка decoder; unlimited terminal registry. Они нарушают intent/core либо создают ложные PASS.

Владельцы unknown: D2 — signatures/lifetime/toggle/binding/receiver stage/radio-only actuator; Ядро — authority export, lease correlation, stable↔native projection и terminal ledger; D3/X1 — physical source/isolation/blind output; V1 — независимый дизайн и корректность gates; V4 — независимый code/dataflow/controls; V5 — независимая финальная LS-проверка; Lead — recorded decisions/manifest/сценарии единственного оператора. Ни один unknown в D1 не закрыт предлагаемым именем interface.

Corrective v0.2 handoff: V4 независимо проверяет §4.1 и terminal/cap4 delta §6; Lead фиксирует проверенный contract/record отдельно. Scope только два actuation docs; B06 архитектурно конкретизирован, но qualified callbacks/export ещё BLOCKED. B07–B09, borrowed refs≤8, Q2 timing/ownership, spec-record/fresh transition и actuator/native-path gates не закрываются этой правкой. SHA обеих полных редакций передаются в Team, без self-acceptance/commit.

Выход D1 — DRAFT review package. [План-предложение I2](<../plan/PLAN-local-radio-actuation.md>) не executable plan до spec approval/record/fresh-session gate. Handoff V1: читать baseline a04a14f, этот REQ, K0 correction, оба plan proposals, D2/D3 с handoff SHA, core/seam/P3/P4; выдать findings/явные blockers. После V1 Lead фиксирует reviewed contracts/новый transition status и только отдельно решает I2_WRITE после D2 и X1 qualified native-path evidence. Ни имплементация, ни compile/pure/process, ни настройка аудио, ни commit не разрешены этим документом.
