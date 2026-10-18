# REQ-lifecycle — server-authoritative жизненный цикл RICON (Э3)

Версия: **0.2 DRAFT (review fixes)**, 2026-10-19. Автор: codex. Ревью: claude.

Источник истины: `PLAN_RU.md` v5, Э3; `REQ-core-contracts.md`; evidence `E-offline-api.md` и
`E1s-von-api.md`. Это requirements-артефакт, а не доказательство поведения Reforger.

**Implementation gate:** код Э3 MUST NOT начинаться, пока актуальный `RICON_Core` не получит
`WORKBENCH_COMPILE_PASS`. Runtime/JIP/RPC/аудио остаются `BLOCKED_NO_RUNTIME`.

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

Любой callback, который API не помечает server-only (в частности `OnPlayerRegistered`), MUST проверить authority
до изменения registries/player state. Клиент MAY обновить только presentation state, отделённое от authority.

### 4.2 События и эффекты

| Событие | Preconditions | Обязательные эффекты |
|---|---|---|
| Connected | authority, PlayerId valid | если живое состояние этого PlayerId уже есть: revoke все leases прежней generation + diagnostic; затем новая generation, `CONNECTED_UNREGISTERED`, пустые owner leases/snapshots |
| Registered | current generation connected | `REGISTERED_UNBOUND`; подготовить owner channel, но не считать entity существующей |
| Spawned / controlled entity `null→entity` | registered/current generation | bind entity; `BOUND_ACTIVE`; rebuild device attachments из server inventory/catalog |
| Entity changed `A→B` | current PlayerId | revoke active PTT; atomарно отвязать A/привязать B; generation не меняется |
| Entity `A→null`, killed/deleted | current binding | revoke PTT; `BOUND_INACTIVE`; carried device bindings снять, world devices сохранить |
| Respawn | inactive/current generation | новый binding; старые entity callbacks после bind игнорировать как stale |
| Disconnected | current connection | revoke all leases; remove owner subscriptions; clear entity binding; tombstone generation; world devices сохранить |
| Reconnect/JIP | новый connection context | новая generation; полный resync до incremental snapshots/RPC |

## 5. PTT lease

### 5.1 Contract

PTT — server lease, а не клиентский bool. Lease MUST содержать:

- `leaseId`, `sessionEpoch`, `connectionGeneration`, authenticated `playerId`;
- `deviceId`, `networkTypeId`, `channelId`/настройку из server catalog;
- `requestSeq`/correlation ID;
- server monotonic `issuedAtMs` и positive `ttlMs`;
- state `ACTIVE | EXPIRED | REVOKED` и terminal reason.

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

### 5.3 Expiry/revoke

Lease MUST стать terminal при первом из:

- server elapsed `>= ttlMs`;
- disconnect, death/delete, controlled-entity change;
- loss of ownership/access, power/device removal, catalog invalidation;
- session epoch rotation, admin/policy revoke.

Terminal transition exactly once вызывает VoN expiry seam с `(playerId, deviceId, leaseId)` **только когда actuator
будет разрешён ADR-001**. Одного playerId недостаточно: несколько устройств/leases одного игрока не должны взаимно
останавливаться. Текущая сигнатура `OnTransmitLeaseExpired(int talkerPlayerId)` в `RICON_NetworkContracts.c` —
известный code gap, исправляемый после compile gate. До actuator записывается intended effect/no-op; отсутствие
actuator не выдаётся за silence PASS.
Server clock MUST быть monotonic abstraction; прямое сравнение wall UTC запрещено. `System.GetTickCount` — int ms
с запуска; signed-int предел достигается примерно через `2^31 ms = 24.86 суток`. Поведение переполнения Enforce не
документировано. До этой границы server MUST ротировать session/restart по operational policy либо использовать
доказанный monotonic источник без переполнения; CT-LC-010 проверяет именно boundary/wrap.

## 6. RPC trust boundary

| ID | Требование | Evidence |
|---|---|---|
| REQ-LC-030 | Discrete owner intent SHOULD использовать reliable server RPC; выбор точной annotation/signature подтверждается WB compile. | DOC `RplRpc`, `Reliable`, `Server` |
| REQ-LC-031 | RPC handler MUST связать call с authenticated owner/PlayerController; payload PlayerId игнорируется либо сверяется и отклоняется при mismatch. | R-04/R-05 |
| REQ-LC-032 | Payload MUST иметь schemaVersion, sessionEpoch, connectionGeneration, requestSeq, correlationId и bounded catalog IDs. | CT malformed/replay |
| REQ-LC-033 | Неизвестные/oversize strings, отрицательные IDs, NaN/Inf, stale seq/generation и чужой device MUST быть отклонены до lookup/mutation. | CT fuzz |
| REQ-LC-034 | Каждый RPC method имеет server-configured per-owner rate limit и bounded state. Reject не продлевает PTT и не меняет snapshot. | CT flood |
| REQ-LC-035 | Security diagnostic содержит correlation/reason, но не hidden route, SMS content или чужое состояние. | DOC review |

Rate-limit algorithm MAY быть token bucket/sliding window, но config MUST задавать capacity/window и bounded number
of owner entries. Значения по умолчанию не придумываются до Э2/Э10 performance budget.

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

## 10. Acceptance matrix

| Test ID | Сценарий | Статический результат сейчас | Runtime gate |
|---|---|---|---|
| CT-LC-001 | connect→register→spawn; duplicate callbacks | trace ожидается | WB/RT |
| CT-LC-002 | respawn ×3, A→null→B, stale A event | trace ожидается | RT |
| CT-LC-003 | disconnect посреди PTT | lease revoke intended once | RT/ADR-001 |
| CT-LC-004 | reconnect/JIP: incrementals до resync | snapshot oracle переиспользуется | Rpl/JIP RT |
| CT-LC-005 | forged owner/player/device/catalog fields | reject/no mutation | adversarial RT |
| CT-LC-006 | stale/replayed requestSeq/generation/session | idempotent/reject | CT/RT |
| CT-LC-007 | rate-limit burst и bounded cleanup | trace ожидается | perf RT |
| CT-LC-008 | ground/vehicle device переживает disconnect owner | registry semantics | RT |
| CT-LC-009 | owner-only snapshot; client forgery не меняет delivery | static contract | Rpl/VoN RT |
| CT-LC-010 | lease TTL `<`, `=`, `>`, tick anomaly/wrap | trace ожидается | CT/WB/RT |

## 11. Открытые вопросы и следующий gate

1. Workbench №003 должен дать compile result Э2. До PASS код Э3 не пишется.
2. Workbench/source №001: vanilla RPC ownership patterns, player/entity callbacks и VoN boundaries.
3. Runtime: callback order, JIP/streaming, reliable RPC behavior, tick wrap, disconnect races.
4. Э2/Э10: rate/traffic/state budgets для 30/60.
5. ADR-001: какой доказанный actuator вызывается при lease expiry.

Следующий разрешённый шаг после compile gate: host state-machine traces для CT-LC-001…010 и bounded implementation
plan. `AGENT-REVIEWED` этого документа не означает runtime/functional acceptance.
