# REQ-lifecycle — server-authoritative жизненный цикл RICON (Э3)

Версия: **0.3 DRAFT FOR ORACLE**, 2026-10-19. Автор: codex. Ревью v0.2: claude (agora id686).

Источник истины: `PLAN_RU.md` v5, Э3; `REQ-core-contracts.md`; evidence `E-offline-api.md` и
`E1s-von-api.md`. Это requirements-артефакт, а не доказательство поведения Reforger.

**Implementation gate:** `RICON_Core` получил `WORKBENCH_COMPILE_PASS` 2026-10-19
(`docs/evidence/E-workbench-source.md`). Код Э3 можно писать и компилировать только после взаимного review v0.3;
Runtime/JIP/RPC/аудио остаются `BLOCKED_NO_RUNTIME`.

v0.3 закрывает только host-model semantics: reason/effect order, replay/seq, TTL/tick anomaly, reconnect cleanup,
bounded state и fixed-window limiter. Она не подтверждает callback order, authenticated RPC API или VoN actuator.

## Мини-intent

- **Проблема:** связь сейчас не имеет однозначного server lifecycle для connect/respawn/JIP/disconnect и PTT;
  клиентские RPC могут подменить владельца, устройство или порядок событий.
- **Результат:** проверяемые contracts состояния игрока/устройств, PTT lease, RPC trust boundary и owner-scoped
  snapshots, не зависящие от честности клиента.
- **Затронуто:** будущий `RICON_PlayerCommsComponent`, `RICON_CommsManagerComponent`, device/endpoint registries,
  `RICON_LinkSnapshotReceiver`, Rpl/RPC и `VoNIntegration` seam.
- **Ограничения:** сервер — источник истины; состояние по PlayerId, не entity; без Play/сервера/VoN actuator;
  direct voice не меняется; никакой глобальной монополии частоты по умолчанию.
- **Открыто:** фактический порядок callbacks, получение authenticated RPC sender/owner, JIP/streaming semantics,
  network/performance budgets и доказанный VoN actuator.

## 1. Нормативный язык и evidence

`MUST`/`MUST NOT` обязательны; `SHOULD` требует причины отклонения; `MAY` — опция policy.

- `DOC` подтверждает только сигнатуру/API.
- `CT` — contract/state-machine trace на host или исполняемый тест после компиляции.
- `WB` — Workbench compile.
- `RT` — runtime/dedicated/JIP; сейчас blocked.

### 1.1 Детерминированный результат host-модели

Каждое событие возвращает один закрытый `resultCode` и упорядоченный список `effects`. Oracle и Enforce MUST
использовать одинаковые значения и порядок:

| Приоритет | `resultCode` | Смысл |
|---:|---|---|
| 1 | `MALFORMED` | отсутствующее/неверного типа/вне int32 поле, пустой обязательный ID |
| 2 | `TICK_ANOMALY` | clock anomaly уже зафиксирована; mutable state закрыт до epoch rotation |
| 3 | `NOT_READY` | manager/session ещё не готовы |
| 4 | `INVALID_SESSION` | неверный `sessionEpoch` |
| 5 | `STALE_GENERATION` | generation не текущая |
| 6 | `STALE_ENTITY` | callback относится не к текущему opaque entity binding |
| 7 | `DUPLICATE` | точный replay найден в bounded response cache; возвращается сохранённый ответ без side effects |
| 8 | `STALE_REQUEST` | `requestSeq <= lastRequestSeq`, но точного cached response уже нет |
| 9 | `RATE_LIMITED` | fixed-window limit authenticated owner/method исчерпан |
| 10 | `FORGED_OWNER` | payload owner/player не совпадает с authenticated context |
| 11 | `INACTIVE_ENTITY` | current player не `BOUND_ACTIVE` |
| 12 | `UNKNOWN_DEVICE` | stable device ID отсутствует в server registry |
| 13 | `NOT_OWNER` | owner/access policy запрещает устройство |
| 14 | `POWER_OFF` | устройство не готово к передаче |
| 15 | `INVALID_CATALOG` | network/channel/tuning не разрешены server catalog |
| 16 | `DEVICE_BUSY` | на устройстве уже есть чужой active lease |
| 17 | `UNKNOWN_LEASE` | leaseId никогда не выдавался текущему authenticated owner/generation |
| 18 | `STALE_LEASE` | lease известен, но принадлежит другой session/generation/owner |
| 19 | `LIMIT_REACHED` | bounded state заполнен и безопасная cleanup не освободила место |
| 20 | `OK` | событие принято |

Для конкретного event применяются только релевантные ему строки, но их относительный порядок не меняется.
`DUPLICATE` возвращает текущий cached payload/lease terminal state, но не повторяет effects; terminal transition MAY
обновить cached payload того же request. Диагностика MAY уточнять причину, но не меняет `resultCode` и не раскрывает
чужое состояние.
Oracle JSON хранит имена строками; Enforce enum MUST задать явные integer values, равные номеру приоритета.

Все числовые пределы приходят из валидированного server config или из oracle-вектора: `maxPlayers`,
`maxActiveLeases`, `maxTombstones`, `tombstoneTtlMs`, `maxResponseCachePerPlayer`, `maxRateEntries`,
`rateWindowMs`, `rateMaxRequests`. Каждый параметр MUST быть `> 0`; невалидный config даёт `NOT_READY`.
Документ не задаёт production defaults до Э10/runtime budget.

Закрытый набор oracle-effects: `PLAYER_STATE_CHANGED`, `ENTITY_BOUND`, `ENTITY_UNBOUND`, `LEASE_GRANTED`,
`LEASE_TERMINATED`, `VON_EXPIRY_INTENDED`, `SUBSCRIPTIONS_CLEARED`, `CARRIED_DEVICES_DETACHED`,
`TOMBSTONE_WRITTEN`, `FULL_RESYNC_REQUIRED`. Effect record содержит только нужные stable IDs и новое состояние;
массив упорядочен фактическим порядком применения. При reject/duplicate/stale callback effects MUST быть пустым.
Clock anomaly обрабатывается отдельным authority `Tick(nowMs)`: именно Tick создаёт terminal effects; последующие
requests лишь возвращают latched `TICK_ANOMALY` без новых effects до epoch rotation.

### 1.2 События host-oracle v1

Oracle принимает упорядоченный trace. Общие integer IDs и `nowMs` — int32; обязательные IDs `> 0`, время `>= 0`.

| Event | Обязательные данные |
|---|---|
| `INIT` | `sessionEpoch`, все bounded config limits, начальный server `nowMs`, server device catalog |
| `CONNECTED` | `nowMs`, `playerId`, opaque `connectionId` |
| `REGISTERED` | `nowMs`, `playerId`, `connectionGeneration` |
| `ENTITY_BOUND` | `nowMs`, `playerId`, generation, opaque `entityId` |
| `ENTITY_UNBOUND` | `nowMs`, `playerId`, generation, текущий/старый `entityId`, reason |
| `DISCONNECTED` | `nowMs`, `playerId`, generation, `connectionId` |
| `PTT_START` | `nowMs`, authenticated/payload player, epoch, generation, `requestSeq`, `deviceId`, catalog IDs, server-resolved `ttlMs` (не payload) |
| `PTT_STOP` | `nowMs`, authenticated/payload player, epoch, generation, `requestSeq`, `leaseId` |
| `TICK` | новый authority `nowMs`; expiry/anomaly sweep |
| `ROTATE_EPOCH` | новый positive epoch; revoke active leases, invalidate generations, require full resync |

`RENEW` не входит в oracle schema v1: policy по умолчанию его не включает. Device catalog в vectors задаёт stable ID,
owner/access, power и допустимые network/channel values; клиент не меняет catalog событиями v1.

## 2. Офлайн API surface

| API | DOC evidence | Что НЕ доказано |
|---|---|---|
| `SCR_BaseGameModeComponent.OnPlayerConnected(int)` | AR `interfaceSCR__BaseGameModeComponent.html:181,594-610`; server-only | порядок относительно identity/entity |
| `OnPlayerRegistered(int)` | там же `:191,760-775`; всегда после Connected, **вызывается на каждой машине** | JIP/reconnect ordering; callback MUST authority-gate mutation |
| `OnPlayerDisconnected(int, KickCauseCode, int)` | там же `:193,653-675` | доставка последних RPC/cleanup timing |
| `OnPlayerDeleted(int,IEntity)`, `OnPlayerKilled(...)`, `OnControllableSpawned(IEntity)` | там же `:200-210` | полный respawn порядок |
| `BaseGameMode.OnPlayerSpawned(int,IEntity)` / `SCR_BaseGameMode.GetOnPlayerSpawned()` | AR `interfaceBaseGameMode.html:145,686-695`; `interfaceSCR__BaseGameMode.html:168,1134-1140` | сервер/клиент ordering |
| `SCR_PlayerController.OnControlledEntityChanged(IEntity from,IEntity to)` + invoker | AR `interfaceSCR__PlayerController.html:128,799-805,1846-1852` | вызов при каждом виде respawn/streaming |
| `RplRpc`, `RplChannel.Reliable/Unreliable`, `RplRcver.Server/Owner/Broadcast` | EN `group__Replication.html:122,145-148,182-184,406-448` | authenticated sender extraction и enforcement |
| `RplCondition.OwnerOnly` | EN `group__Replication.html:149-152,284-299` | фактическая JIP/streaming доставка |
| `System.GetTickCount()` | EN `interfaceSystem.html:138,731-758`; ms с запуска игры | wrap/authority behavior в целевой версии |

## 3. Идентичность и ownership

| ID | Требование | Проверка |
|---|---|---|
| REQ-LC-001 | Authority state MUST быть keyed by `PlayerId`; entity — сменяемая привязка, не identity. | CT respawn×3 |
| REQ-LC-002 | Каждое подключение получает `connectionGeneration > 0`; reconnect/JIP MUST NOT оживлять lease, RPC seq или entity binding прежней generation. | CT reconnect/replay |
| REQ-LC-003 | Authenticated player/owner MUST выводиться из server/Rpl call context. `playerId`, owner или device owner из RPC payload MUST NOT считаться истиной. | adversarial CT/RT |
| REQ-LC-004 | Device/endpoint имеет stable server ID, catalog definition и owner/access policy. Перенос entity не меняет ID. | CT drop/pickup/vehicle |
| REQ-LC-005 | Ground/vehicle/site devices принадлежат server registry независимо от connected player set. Disconnect игрока не удаляет world-owned device. | CT lifecycle |
| REQ-LC-006 | Любая owner mutation проверяет `(sessionEpoch, connectionGeneration, authenticatedOwner, deviceId)` до изменения состояния. | CT stale/forged |

## 4. Player lifecycle state

### 4.1 Состояния

| State | Смысл |
|---|---|
| `CONNECTED_UNREGISTERED` | server connection известен, identity/entity ещё не готовы |
| `REGISTERED_UNBOUND` | PlayerId/identity готовы, controlled entity отсутствует |
| `BOUND_ACTIVE` | PlayerId связан с текущей entity; запросы устройств разрешаются policy |
| `BOUND_INACTIVE` | entity умерла/недоступна; mutable voice/PTT запрещены |
| `DISCONNECTED` | terminal cleanup выполнен; state удаляется после bounded tombstone window |

Переходы MUST быть идемпотентны: duplicate callback не создаёт новую generation/lease/device. Неожиданный порядок
fail closed и пишет diagnostic; не должен приводить к null dereference.

Host-модель получает непрозрачные `connectionId` и `entityId`; она не выводит их из указателей/адресов. Повторный
`Connected` с тем же `(playerId, connectionId)` — `DUPLICATE`. Новый `connectionId` для уже живого `playerId` —
reconnect: старое состояние завершается с reason `RECONNECT`, затем создаётся следующая generation. Generation строго
возрастает внутри session epoch; перед int32 overflow manager MUST ротировать epoch/full-resync и начать generation с 1.

Любой callback, который API не помечает server-only (в частности `OnPlayerRegistered`), MUST проверить authority
до изменения registries/player state. Клиент MAY обновить только presentation state, отделённое от authority.

### 4.2 События и эффекты

| Событие | Preconditions | Обязательные эффекты |
|---|---|---|
| Connected | authority, PlayerId/connectionId valid | тот же connection — `DUPLICATE`; новый connection при живом state — cleanup с `RECONNECT`; затем следующая generation, `CONNECTED_UNREGISTERED`, пустые owner leases/snapshots |
| Registered | current generation connected | `REGISTERED_UNBOUND`; подготовить owner channel, но не считать entity существующей |
| Spawned / controlled entity `null→entity` | registered/current generation | bind entity; `BOUND_ACTIVE`; rebuild device attachments из server inventory/catalog |
| Entity changed `A→B` | current PlayerId | revoke active PTT; atomарно отвязать A/привязать B; generation не меняется |
| Entity `A→null`, killed/deleted | current binding | revoke PTT; `BOUND_INACTIVE`; carried device bindings снять, world devices сохранить |
| Respawn | inactive/current generation | новый binding; старые entity callbacks после bind игнорировать как stale |
| Disconnected | current connection | revoke all leases; remove owner subscriptions; clear entity binding; tombstone generation; world devices сохранить |
| Reconnect/JIP | новый connection context | новая generation; полный resync до incremental snapshots/RPC |

### 4.3 Stale entity и terminal cleanup

- Entity callback применим только когда его opaque `entityId` равен текущему binding. Callback от старой entity после
  respawn возвращает `STALE_ENTITY`, не меняет binding и не отзывает новые leases.
- `Disconnected` и live reconnect выполняют effects строго в порядке: (1) revoke active leases по `leaseId` по
  возрастанию; (2) удалить owner subscriptions/snapshot state; (3) снять entity/carried-device bindings;
  (4) записать bounded tombstone `(playerId, generation, connectionId, lastRequestSeq, expiresAtMs)`; (5) удалить live
  player state. World-owned devices не удаляются.
- Перед добавлением tombstone удаляются истёкшие. Если после cleanup достигнут `maxTombstones`, удаляется самый старый
  terminal tombstone по `(expiresAtMs, playerId)`; live state и active lease ради освобождения места не удаляются.

## 5. PTT lease

### 5.1 Contract

PTT — server lease, а не клиентский bool. Lease MUST содержать:

- `leaseId`, `sessionEpoch`, `connectionGeneration`, authenticated `playerId`;
- `deviceId`, `networkTypeId`, `channelId`/настройку из server catalog;
- `requestSeq`/correlation ID;
- server monotonic `issuedAtMs` и positive `ttlMs`;
- state `ACTIVE | EXPIRED | REVOKED` и terminal reason.

Terminal reason — закрытое значение: `TTL_EXPIRED`, `CLIENT_STOP`, `DISCONNECT`, `RECONNECT`, `ENTITY_CHANGED`,
`ENTITY_INACTIVE`, `ACCESS_LOST`, `POWER_LOST`, `DEVICE_REMOVED`, `CATALOG_INVALIDATED`, `SESSION_ROTATED`,
`ADMIN_REVOKED`, `TICK_ANOMALY`. `EXPIRED` допустим только с `TTL_EXPIRED`; остальные terminal reasons дают
`REVOKED`.

Клиент передаёт только intent (`start`, `stop`, допустимая catalog selection). Server clamps/разрешает физические
параметры сам. У lease нет клиентского срока/мощности/range/mute.

### 5.2 Start/renew/stop

| ID | Требование |
|---|---|
| REQ-LC-020 | Start MUST проверить current session/generation, RPC owner, active entity, device ownership/access, power/catalog/tuning и rate limit до создания lease. |
| REQ-LC-021 | Один device MUST иметь не более одного active transmit lease. Policy MAY ограничить player/network concurrency; общей блокировки всей frequency нет по умолчанию. |
| REQ-LC-022 | Duplicate `(generation, requestSeq)` MUST быть идемпотентен: тот же ответ/lease, без продления скрытым replay. |
| REQ-LC-023 | Stop текущего lease идемпотентен. Stop чужого/stale lease отклоняется без side effects. |
| REQ-LC-024 | Renew MAY существовать только как явный policy; он повторно проверяет generation/owner/device и имеет bounded max lifetime. |

### 5.2.1 Request sequence и replay

- `requestSeq` — positive int32, строго возрастающий для `(playerId, connectionGeneration)`.
- Bounded response cache хранит не больше `maxResponseCachePerPlayer` последних `(requestSeq, response, leaseId?)`.
  Новая запись вытесняет самый маленький `requestSeq`; active lease при этом не удаляется.
- Точный replay cached `requestSeq` возвращает `DUPLICATE` и тот же response без продления TTL/повтора effects.
  Если связанный lease уже terminal, cached response обновляется до его terminal state/reason, поэтому replay не может
  «оживить» ранее выданный `ACTIVE`.
- `requestSeq <= lastRequestSeq` без cache entry даёт `STALE_REQUEST`. Gap вперёд допустим: новый seq MUST быть
  `> lastRequestSeq`, contiguous sequence не требуется.
- Stop текущего lease делает terminal transition один раз. Повтор того же stop request — `DUPLICATE`; новый stop для
  уже terminal своего lease возвращает `OK` + terminal state без effect; никогда не выдававшийся текущему owner/gen
  lease — `UNKNOWN_LEASE`; известный lease другой session/generation/owner — `STALE_LEASE`.

### 5.3 Expiry/revoke

Lease MUST стать terminal при первом из:

- server elapsed `>= ttlMs`;
- disconnect, death/delete, controlled-entity change;
- loss of ownership/access, power/device removal, catalog invalidation;
- session epoch rotation, admin/policy revoke.

Terminal transition exactly once вызывает VoN expiry seam с `(playerId, deviceId, leaseId)` **только когда actuator
будет разрешён ADR-001**. Одного playerId недостаточно: несколько устройств/leases одного игрока не должны взаимно
останавливаться. Текущая сигнатура `OnTransmitLeaseExpired(int talkerPlayerId)` в `RICON_NetworkContracts.c` —
известный code gap, исправляемый в Э3 после review v0.3/oracle. До actuator записывается intended effect/no-op; отсутствие
actuator не выдаётся за silence PASS.
Server clock MUST быть monotonic abstraction; прямое сравнение wall UTC запрещено. `System.GetTickCount` — int ms
с запуска; signed-int предел достигается примерно через `2^31 ms = 24.86 суток`. Поведение переполнения Enforce не
документировано. До этой границы server MUST ротировать session/restart по operational policy либо использовать
доказанный monotonic источник без переполнения; CT-LC-010 проверяет именно boundary/wrap.

TTL истекает при `elapsedMs >= ttlMs`: равенство уже `EXPIRED/TTL_EXPIRED`. Если `nowMs < issuedAtMs` либо меньше
последнего принятого server tick, manager fail closed: все active leases переходят в `REVOKED/TICK_ANOMALY` по
`leaseId` по возрастанию, каждый intended expiry effect записывается ровно один раз; новые mutable requests получают
`TICK_ANOMALY` до ротации session epoch/full-resync.

`ROTATE_EPOCH` принимает epoch строго больше текущего. Он отзывает active leases с `SESSION_ROTATED` по `leaseId`,
очищает live generations/request/rate state, записывает `FULL_RESYNC_REQUIRED` последним effect и снимает
`TICK_ANOMALY` latch. Невозрастающий/неположительный epoch — `MALFORMED`, без mutation.

## 6. RPC trust boundary

| ID | Требование | Evidence |
|---|---|---|
| REQ-LC-030 | Discrete owner intent SHOULD использовать reliable server RPC; выбор точной annotation/signature подтверждается WB compile. | DOC `RplRpc`, `Reliable`, `Server` |
| REQ-LC-031 | RPC handler MUST связать call с authenticated owner/PlayerController; payload PlayerId игнорируется либо сверяется и отклоняется при mismatch. | R-04/R-05 |
| REQ-LC-032 | Payload MUST иметь schemaVersion, sessionEpoch, connectionGeneration, requestSeq, correlationId и bounded catalog IDs. | CT malformed/replay |
| REQ-LC-033 | Неизвестные/oversize strings, отрицательные IDs, NaN/Inf, stale seq/generation и чужой device MUST быть отклонены до device/lease mutation; только bounded rate accounting §6 MAY измениться. | CT fuzz |
| REQ-LC-034 | Каждый RPC method имеет server-configured per-owner rate limit и bounded state. Reject не продлевает PTT и не меняет snapshot. | CT flood |
| REQ-LC-035 | Security diagnostic содержит correlation/reason, но не hidden route, SMS content или чужое состояние. | DOC review |

Rate limit v0.3 — deterministic fixed window, keyed by `(authenticatedPlayerId, methodId)`. Config задаёт positive
`rateWindowMs` и `rateMaxRequests`; production defaults не придумываются до Э10/runtime budget.

1. Точный cached duplicate возвращается до limiter и не расходует attempt.
2. Для нового well-formed request с current session/generation окно начинается в `windowStartMs`; при
   `nowMs - windowStartMs >= rateWindowMs` счётчик сбрасывается и новое окно начинается в `nowMs`.
3. Если `count >= rateMaxRequests`, результат `RATE_LIMITED`; уже полный счётчик не увеличивается.
4. Иначе `count++` выполняется до owner/device/catalog checks: forged/inactive/invalid request текущего authenticated
   owner расходует attempt и не может бесплатно нагружать нижние проверки.
5. При `nowMs < windowStartMs` применяется общий `TICK_ANOMALY` fail-closed. Entries bounded `maxRateEntries`;
   сначала удаляются окна terminal/tombstone owners, затем при отсутствии места возвращается `LIMIT_REACHED`.

Изменение внутреннего rate counter не включается в публичный `effects` array и является единственной mutation,
допустимой для request, который позже отклонён на шагах 10–19 таблицы §1.1.

## 7. Snapshot replication и JIP

- Enforcement доставки — server/engine path; owner-scoped snapshot используется для UI/диагностики и MUST NOT
  давать клиенту право изменить маршрут/VoN.
- Incremental snapshot принимается только после atomic full resync identity по `REQ-core` §8.
- JIP/reconnect MUST сначала получить `(schemaVersion, sessionEpoch, connectionGeneration)` и полный набор owner-visible
  slots; затем incremental seq. Обычный snapshot до resync → `INVALID_IDENTITY`/`REQUEST_RESYNC`.
- Репликация SHOULD быть owner-only (`RplCondition.OwnerOnly`/Owner receiver после WB-подтверждения). Broadcast hidden
  per-pair routes запрещён.
- Client mutation/forged snapshot никогда не меняет authority route/device/lease.
- Far-radio streaming behavior остаётся `BLOCKED_NO_RUNTIME`; отсутствие entity на клиенте не должно удалять server
  endpoint/device state.

## 8. Failure semantics

| Failure | Результат |
|---|---|
| Manager/player state отсутствует | `NOT_READY`; no mutation; bounded resync request |
| Callback duplicate/stale entity | ignore + diagnostic; current binding сохраняется |
| Invalid/forged RPC | reject + reason/rate accounting; no lease/snapshot effect |
| Lease expired/revoked | terminal once; intended actuator effect once |
| Snapshot schema/identity mismatch | правила `REQ-core` §8; full resync only |
| Owner disconnects mid-PTT | revoke before player state removal |
| Device becomes world-owned | owner lease revoked; device registry entry может сохраниться по policy |
| Rate-limit state overflow | fail closed; oldest terminal/tombstone cleanup по bounded policy |

## 9. Bounded state и performance

- Player/session maps bounded connected players + short tombstones; cleanup имеет явный срок.
- Active leases bounded devices/players; terminal leases не копятся без лимита.
- Rate-limit entries bounded active/tombstone owners and methods.
- Lifecycle callbacks MUST NOT запускать O(P²) link evaluation; они только обновляют registries/dirty events.
- Full snapshot size/частота и RPC budgets определяются после compile и synthetic profile; до чисел — OPEN, не PASS.

Deterministic capacity rules:

- Перед любой вставкой очищаются истёкшие tombstones, terminal leases и response-cache entries текущей generation,
  которые старше tombstone window.
- `maxPlayers` считает live states, `maxActiveLeases` — только `ACTIVE`, `maxTombstones` — terminal generations,
  `maxRateEntries` — пары owner/method. Ни live player, ни active lease не вытесняются.
- Если safe cleanup не освободила место, событие возвращает `LIMIT_REACHED` без частичной mutation.
- Cleanup order стабилен: `expiresAtMs`, затем `playerId`, затем `generation`, затем `leaseId/requestSeq`.

## 10. Acceptance matrix

| Test ID | Сценарий | Статический результат сейчас | Runtime gate |
|---|---|---|---|
| CT-LC-001 | connect→register→spawn; duplicate same connection | deterministic state/effects, generation не растёт | WB/RT |
| CT-LC-002 | respawn ×3, A→null→B, stale A event | `STALE_ENTITY`, новый binding/lease сохраняются | RT |
| CT-LC-003 | disconnect посреди PTT | revoke по leaseId → subscriptions → binding → tombstone, effects once | RT/ADR-001 |
| CT-LC-004 | reconnect/JIP: incrementals до resync | snapshot oracle переиспользуется | Rpl/JIP RT |
| CT-LC-005 | forged owner/player/device/catalog fields | reject/no mutation | adversarial RT |
| CT-LC-006 | cached replay, evicted replay, stale generation/session | `DUPLICATE` / `STALE_REQUEST` / reject без mutation | CT/RT |
| CT-LC-007 | fixed-window boundary, forged burst и bounded cleanup | deterministic count/reset; `RATE_LIMITED`/`LIMIT_REACHED` | perf RT |
| CT-LC-008 | ground/vehicle device переживает disconnect owner | registry semantics | RT |
| CT-LC-009 | owner-only snapshot; client forgery не меняет delivery | static contract | Rpl/VoN RT |
| CT-LC-010 | lease TTL `<`, `=`, `>`, tick anomaly/wrap | ACTIVE / EXPIRED / EXPIRED; anomaly revokes once и требует epoch rotation | CT/WB/RT |

## 11. Открытые вопросы и следующий gate

1. Workbench №003 выполнен: Э2 compile PASS. До взаимного review v0.3 код Э3 не пишется.
2. Workbench/source №001 выполнен частично: player/entity/VON script boundaries получены; authenticated RPC sender и
   audio graph остаются OPEN/BLOCKED_NO_RUNTIME.
3. Runtime: callback order, JIP/streaming, reliable RPC behavior, tick wrap, disconnect races.
4. Э2/Э10: rate/traffic/state budgets для 30/60.
5. ADR-001: какой доказанный actuator вызывается при lease expiry.

Следующий разрешённый шаг после взаимного review v0.3: host state-machine traces для CT-LC-001…010 и bounded
implementation plan. Только после взаимного agree oracle/vectors пишется Enforce state machine и исправляется expiry
seam на `(playerId, deviceId, leaseId)`. `AGENT-REVIEWED` не означает runtime/functional acceptance.
