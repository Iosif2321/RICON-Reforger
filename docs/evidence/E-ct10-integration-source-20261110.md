# CT10: ordered Enforce port, authority capture и Agy QA

2026-11-10. Результат: **SOURCE_REVIEWED_UNCOMPILED / SOURCE_JOIN_REVIEWED_UNCOMPILED**.
Goal active; не Enforce parity, не игровой/звуковой релиз.

## Изменение

В canonical RICON_Core внесён typed port принятого HOST ordered protocol d2c0d648:
RICON_TemporalLifecycle.c и pure fixtures. Core author23a9ec39/a885df72.
Используются существующие Snapshot2/Expected/Binding/limits и CF/CD, без второго physics Core.

Добавлены RICON_TemporalAuthorityJournal.c и server-only explicit methods существующего
RICON_CommsManagerComponent. Journal заранее хранит detached addressing accepted publication,
а после Lifecycle result строит closure intents. Сам он не выделяет END/link/delivery seq и не
посылает RPC/голос; окончательный seq обязан назначить producer.
Manager по умолчанию capture не включает; Configure явный и не стирает старый scope.
Все entrypoints guarded IsAuthority; Capture также проверяет фактически выданный manager seq
и live Lifecycle epoch, BOUND_ACTIVE, TX/RX generations. Issuance не доказывает commit producer.

Существующий Lifecycle НЕ менялся. LEASE_TERMINATED сохраняет old IDs/generation/reason,
но его epoch не meaningful. Journal строго соединяет его со следующим
VON_EXPIRY_INTENDED, где есть old epoch, и explicit preEventEpoch.
Весь effect batch сначала валидируется; поздний invalid не оставляет prefix mutation.
ResultCode не фильтруется: BeginEvent мог sweep/revoke до malformed/duplicate response.

## Agy: реальная независимая проверка и исправления

По прямому запросу владельца Agy выполнил source review, Agora8312–8314.
Его два counter: EntityChanged/Unbound не выдают SUBSCRIPTIONS_CLEARED и не меняют generation;
поэтому initial journal не закрывал incoming RX, а blanket sweep мог задеть новую entity.

Root исправил оба:
- retained row хранит положительный receiverEntityId как source metadata, не wire Snapshot2 field;
- existing linkId нельзя переадресовать на другую entity, нужен fresh link ID;
- ENTITY_UNBOUND закрывает только old(listener,rxGeneration,entityId);
- manager получает entity через actual Lifecycle.GetPlayerEntityId, а не из packet/Expected.

В той же conversation Agy перепроверил diff и дал verdict8317:
SOURCE_REVIEWED_UNCOMPILED без новых source blockers (journal3b0a11e5/managerBDE099E3).
Модель Gemini4Argon не менялась. Shared memory operator projection rev6 подтверждена;
native memory API Agy не заявлял. [Квитанция проверки Agy](../review/R-agy-ct10-capture-20261110.md).

Первый provider connection EOF подтверждён terminalexit1; тот же conversation возобновлён,
финальный CLIexit0 и actual Agora verdict получены. Unknown старый agy.port не выдан за готовый review.

## Остальная встречная проверка

Reviewer до новых source прочёл existing Lifecycle и зафиксировал20 J expectations,
затем RX/checkpoint случаи. Source counter ToEnd null guard закрыт. Per-owner source pruning
позволял rebind global ID на другого owner после удаления pending; заменён GLOBAL source
checkpoint H + exact ALL active IDs всех captured owners. Это source-producer full-state handoff,
не per-owner network FULL и не untrusted client ACK.

Reviewer8307/8308 принял исходный выбранный source scope;8320 — новый entity delta.
Documentation8306/8319 проверила реальные source joins и new getter/pins.
Core helper SameSnapshot сравнивает validated scalars и существующие canonical RCFD1 bytes;
новый codec не добавлялся.

Fixtures включают реальные pure calls existing Lifecycle:
ProcessPtt(schema0) после BeginEvent TTL sweep; EntityChanged1002→1003 с прежней generation,
capture fresh link3 до Consume old result, replay не закрывает3; actual RotateEpoch123 выдаёт
old TERM7 + new FULL8. Missing VON, invalid last item, copies, caps и global checkpoint
также имеют Source Check. Их наличие не означает исполнение.

## Наблюдаемые проверки

| Проверка | Результат |
|---|---|
| Canonical7 files byte-identical staging | PASS |
| New-file whitespace и git diff --check | PASS |
| PowerShell runner Parser | 0 errors |
| Canonical run.ps1 integration DryRun | DRY_RUN_NO_WORKBENCH_STARTED |
| Strict knowledge graph after bounded source refresh | SOURCE_SCOPE_ONLY PASS,57nodes/83edges/22requirements, runtimefalse |
| Enforce compiler / ordered+capture RunAll | NOT_RUN |
| Actual server/client role, producer/exporter/dispatcher, RPC, audio | NOT_RUN |

Runner получил optional Plugin (default прежний), отдельный integration marker и DryRun.
DryRun возвращается до process enumeration/start. Реальный run откажется создавать второй
Workbench при уже открытом экземпляре; current user session не закрывается.
SDK executable и addons paths найдены; installed Workbench SteamDiag version1.9.0.37.
Только command preparation выполнена. Workbench/Game/server не запускались.

Подготовленный будущий command из canonical root:
`& '.\tools\RICON_TestRunner\run.ps1' -Name 'ct10-integration' -Plugin RICON_TemporalIntegrationProbePlugin`
Он не выполнен: нужен действующий допуск SDK ScriptEditor этапа в пределах Goal.
DryRun counterpart с -DryRun действительно выполнен.

## Граф, история и доказательства

Manager изменён c2644e9d→BDE099E3 только nullable journal field и explicit methods.
Поэтому старый graph validator честно дал9SOURCE_DRIFT. Сохранены byte-identical archives
с actual snapshot date9Nov; затем ограниченно обновлены manager hashes/unchanged-source
line mappings, one evidence line-range и dated partial_source_refresh.
Root проверил точный whitelist19semantic changes; topology/statuses/22requirements
(21sections и отдельныеA/B) не менялись. Snapshot/generated dates остаются историческими,
full_snapshot_reverified=false и runtime_qualified=false. Validator не менялся и checks не отключались.

Полная machine receipt: [ct10-integration-source-result-20261110.json](receipts/ct10-integration-source-result-20261110.json).
Staging provenance: соседний Cellular System/output/ct10-integration-20261110.

## Не закрыто

Actual committed producer/current Expected truth, capture-all atomic publication batch,
Lifecycle call coordinator и epoch retirement, real source checkpoint handoff, reserved-ID/seq
failure integration, source-owned entity identity uniqueness, RPC/auth/wire/ACK/liveness,
persistent medium/sharedRX/RF/world/calibration, Enforce compile/runtime/JIP/load и
payload/content/DSP/audio остаются OPEN. Source-issued seq — не producer commit.
Guarded callable APIs ещё не автоматический dispatcher. Без этих доказательств Goal не complete.
