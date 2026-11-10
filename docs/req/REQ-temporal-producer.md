# CT10: schema2 producer и единый владелец source state

2026-11-10. **EXPLICIT_V2_SOURCE_CO_COMMIT / SELECTED_PURE_SCRIPTEDITOR_PASS**. Это контракт текущего opt-in source API. [Фактические проверки](../evidence/E-ct10-producer-owner-20261110.md) и receipt фиксируют конкретные hashes. World/RPC/native manager role/physical builder/RECEIVER production parity/audio/load требуют отдельных доказательств.

## Формат и граница

`RICON_TemporalProducerTick` содержит `m_iSchemaVersion=2`, `m_PairInput` (существующий полный `RICON_PairServiceInput` одного configured band), presentation `m_iTtlMs`, binary32 `m_fQualityDelta`, requested `m_sMode` SERVER/RECEIVER, physical-clock key `m_sTimebaseId` и `m_ChannelByPair`. Это **полный текущий scope**, не patch; omitted old pair может означать withdrawal только при правдивом complete caller input. DTO и согласованные числа не доказывают настоящий world exporter, часы или RPC authority.

Ключ карты — existing S3b `(leaseId,rxDeviceId,rxTransceiverIndex,rxPlayerId)`, формируемый `RICON_TemporalPublicationService.PairKey`. Значение `RICON_TemporalChannelInput`: independent CF query `m_Query` и обязательный `m_Descriptor`. Query не выводится из descriptor. Он задаёт directed endpoints, frequency/bandwidth, profile/version/environment revision и explicit model `nowMs/subUs`; descriptor должен совпасть с ним. Model time отделён от Lifecycle/presentation time. Отсутствие этих данных — отказ, без scalar fallback.

Input PairService time должен совпадать с единственным явным `lifecycleNowMs`. `CanQueryAt` проверяет ready/tick anomaly/time floor без наблюдения времени; `QueryDevice(id,now)` возвращает глубокую копию текущего pure Lifecycle control record. Все supplied players, bindings, listeners и devices сверяются с current Lifecycle до PairService, включая строки, которые legacy plan мог бы удалить. Actual TX power/access/catalog и active lease owner/generation/device/timing обязательны. Полноту omitted world roster это не подтверждает.

## API и владение

Manager API: `ConfigureTemporalProducerV2(bandId,publicationLimits,temporalLimits)`, `PublishTemporalPairs2(tick,lifecycle,lifecycleNowMs)`, `ReadTemporalProducerRegistry()` и `BuildTemporalProducerFullHandoff()`. Configure требует authority, frozen registered evaluator и fresh namespace без existing journal/issued IDs. Mode default off. Один owner — subclass existing journal; manager хранит тот же объект. Direct capture/checkpoint bypass закрыт. В v2 mode внешние Next/Rotate запрещены вне producer operation; во время неё действует synchronous single-writer/no-interleaving contract. Ни публичного mutable prepare token, ни отдельного allocator/physics/voice Core не добавлено.

`RICON_TemporalPublicationOwner(epoch,temporalLimits,publicationLimits)` вызывает actual PairService и explicit schema2 sibling существующего S3b. Sibling переиспользует protected legacy Preflight/BuildPlan/sort/NeedSnapshot; temporary scalar registry удерживается strong ref до окончания заимствованного плана. Legacy Publish, registry v1, Snapshot v1 и RCFD/1 сохраняют прежний смысл.

Result: code, owner sessionEpoch, requiresFullResync, full flag, фактические sequenceCalls/epochReads, reservedHighWater, detached snapshots, typed ends, complete activeIds и ExpectedByLink. Failure не возвращает accepted partial output. Registry read — detached последняя принятая publication state; после actual Lifecycle close она может ещё содержать old row до producer reconciliation. Export текущего source active set обязательно учитывает journal closures.

## Публикация, идентичность и физика

Каждый tick проверяет current independent context всех post-active pairs, включая no-emission KEEP. NeedSnapshot — прежние scalar/state/TTL/ceil-half-TTL triggers плюс canonical descriptor bytes, mode/timebase changes. Descriptor-only change публикуется при прежнем Q. Canonical signed-zero equivalence не даёт лишнюю публикацию. Keepalive не изменяет anchor, phase или physical validity.

CREATE ID выделяет owner; caller link IDs не принимает. Existing binding/player/entity identity не rebind на том же ID: old typed END и новый ID. Legacy TERMINATE преобразуется в END со старой addressing identity; actual pending closure reason имеет приоритет над LINK_REMOVED. Уже closed ID не считается вторым tracked slot. RF LOST/Q0 с valid descriptor остаётся ACTIVE. Неопределённый temporal/current grant не превращается в выдуманный END или CLEAN.

Полный payload preflight выполняется до reservation/Next, включая shared Snapshot2 validation с временными внутренними positive identity values. Эти values не являются выдачей IDs; actual IDs назначаются только после завершения проверок.

## Транзакция и recovery

Новый registry, withdrawals и capture готовятся в private scratch. Перед adoption завершаются все проверки, callbacks и cloning. Последняя секция заменяет registry и journal refs/totals без callbacks/dispatch. Это synchronous single-writer source transaction; настоящий event/dispatcher/world pipeline остаётся отдельным gate.

`reservedHighWater` — отдельный control frontier. До первого Next резервируются все CREATE IDs; failed/rotated/invalid/lower seq прерывает выдачу. Уже consumed IDs/sequence не откатываются. Committed registry/journal/publishedAt и ready output не принимаются частично. Same-epoch retry идёт выше abandoned reservations. Legacy failed-Next empty reset нельзя подставить вместо нового committed payload. Source epoch mismatch сохраняет old-scope closure data и требует явного reconciliation; network epoch не reseed физический процесс.

`BuildFullSourceHandoff()` создаёт detached полный output из owned committed registry и captured active state; H берётся из собственного reservation frontier. Уже закрытые rows исключаются, каждый captured active обязан быть представлен. Только после сборки output private base checkpoint может prune pending/prefix и co-adopt registry. Это internal ALL-owner source handoff, не receiver FULL, ACK или доказательство доставки. После него следующий sequenced full обязателен. Caller IDs/H/complete flag не используются как доказательство полноты.

## Приёмка данного среза

Реально выполнены selected pure ScriptEditor suites: Snapshot57, Ordered40, Capture59, LeaseQuery67, CaptureBatch61 и Producer1496 — **1780 checks, failures0, SCRIPT(E)0, process/runner0, done PASS**. Legacy S3b до/после:106 cases match+immutable,15 controls,4 extras. PC37 — явно TEST_ONLY fault на private scratch после двух успешных Next, не live resource/race reproducer. Scalar-Q и sequence doubles названы; отдельный actual free-space evaluator smoke не доказывает калиброванную RF среду.

Финальная Agy implementation/log проверка ожидает отдельного разрешения на конкретный внешний payload; прежняя Agy design QA не расширена до этой приёмки. Независимые Core/Reviewer source/tests/log проверки выполнены. Полная Goal и перечисленные runtime/data-origin/transport/audio/load gates остаются OPEN.
