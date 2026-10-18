# REQ-core-contracts — нормативные контракты ядра RICON

Версия: **0.6 DRAFT (routing vectors)**, 2026-10-18. Автор: codex. Ревью: claude.

Взаимная проверка: claude согласовал итог в agora `ricon-reforger-build` id481; codex согласовал связанный
`REQ-link-eval.md` в id483. Документ принят как вход для **статической реализации**. Это не human/runtime
acceptance и не freeze Э2: ограничения ниже остаются обязательными.

Oracle-review: failure precedence и `intelligibilityModelId` сверены с `REQ-link-eval.md@3078f88` и
`oracle/vectors/link_eval_v1.json` revision 2; claude подтвердил runner/REQ sync в id513.

C13 code-review: `MODEL_CONTRACT_VIOLATION`, post-compute validation order и ADR sync сверены с Enforce L0;
claude подтвердил итог в agora id548. Это по-прежнему static evidence, не compile/runtime/freeze.

Snapshot/manager review: snapshot oracle revision 9 согласован в id587; Enforce manager/snapshot/allocator
согласованы в id602. `WORKBENCH_COMPILE_PASS` и runtime всё ещё не выполнялись.

Источник истины: Git-репозиторий `RICON-Reforger`. Входы: утверждённый `PLAN_RU.md` v5 с приоритетной
шапкой решений владельца, `ADR-002-core-contracts.md`, ревью `R-core-20261018.md` и
`R-core-20261018-api.md`. При написании применены инструкции текущей сессии (`AGENTS.md`,
`ai-native-sdlc-cycle`, `ponytail full`); это metadata процесса, а не файлы/runtime-зависимости репозитория.

Этот документ конкретизирует Э2, но **не замораживает Э2**. Freeze возможен только после реализации,
contract/oracle-проверок, `WORKBENCH_COMPILE_PASS` и повторной сверки с результатом Workbench №001.

## Мини-intent

- **Проблема:** текущие схемы не задают однозначные ownership, identity, lifetime, failure и extension contracts;
  часть полей не расширяется без правки ядра.
- **Результат:** проверяемый нормативный контракт для server-authoritative общего ядра радио, инфраструктуры и
  будущей сотовой сети.
- **Затронуто:** `LinkQuery`, `LinkBudget`, loss layers, `LinkSnapshot`, evaluator, registries, routing,
  `VoNIntegration`, `CommsManagerComponent` и будущий oracle/contract harness.
- **Ограничения:** Enforce Script; встроенный VoN; APL-SA; без копирования кода A3/модов; без RE, Play и сервера;
  физика link-eval определяется отдельно в `REQ-link-eval.md`; P.526 L2 пока вне решения.
- **Открыто:** конкретный источник session epoch, формат Rpl-сериализации расширяемых слоёв и реальные
  compile/runtime budgets.

## 1. Нормативный язык и уровни доказательств

`MUST`/`MUST NOT` — обязательный контракт; `SHOULD` — отклонение требует записи причины; `MAY` — опция.

- `CT` — host/Enforce contract-тест без утверждения runtime-поведения движка.
- `OR` — oracle-вектор по `REQ-link-eval.md`.
- `WB` — компиляция Workbench без Play исполнителем заказа №001.
- `RT` — runtime; сейчас `BLOCKED_NO_RUNTIME`.

Ни CT, ни OR, ни WB не заменяют RT.

## 2. Сквозные инварианты

| ID | Требование | Трассировка | Проверка |
|---|---|---|---|
| REQ-CORE-001 | Сервер MUST быть единственным владельцем registries, device/catalog параметров, выбора модели, маршрута и snapshot identity. Клиент MAY передавать только intent, который сервер валидирует. | PLAN §3, R-04/R-05 | CT adversarial; RT позже |
| REQ-CORE-002 | Ни одна физическая модель, routing policy или UI-компонент не MUST обращаться к VoN напрямую. Единственная точка — `VoNIntegration`. | PLAN §3, инв. 2–3 | CT dependency scan |
| REQ-CORE-003 | Все публичные numeric outputs MUST быть конечными. Invalid input/model result MUST давать детерминированный invalid budget, а не NaN/Inf/частично заполненный valid budget. | A8/A10, API-02 | OR/CT |
| REQ-CORE-004 | Stable IDs, enum/schema values и units MUST быть versioned и не переиспользоваться с другой семантикой. | PLAN инв. 4/6, B2 | CT serialization |
| REQ-CORE-005 | Неизвестный ID, duplicate registration, schema mismatch и stale/replay MUST fail closed с reason code; тихий fallback на `default` запрещён. | B2/B4, API-04 | CT negative |
| REQ-CORE-006 | Новые radio/device/band/model/network types MUST добавляться через config/registry/implementation seam, без `if <new type>` в общем ядре. | Intent §4, PLAN §3.1 | пять сценариев §10 |

## 3. Ownership и lifetime

| Объект | Создаёт / владеет | Сильные ссылки | Lifetime | Запрещено |
|---|---|---|---|---|
| `CommsManagerComponent` | GameMode prefab, authority | registries и их entries | world/session | клиентская мутация |
| `PropagationModel` | manager при регистрации | registry и/или band-scoped evaluator | не меньше evaluator | transient model при weak field |
| `LinkEvaluator` | manager/service | MUST сильно владеть требуемыми model/policy dependencies | scope сервиса/registry | неявный borrowed lifetime |
| `LinkBudget` | evaluator | caller/route leg | immutable после возврата | менять после публикации |
| `Route` / `RouteLeg` | routing policy | route сильно владеет legs; leg — budget | один route calculation/snapshot | слабые dangling legs |
| `LinkSnapshot` | authority snapshot service | owner-scoped replication state | до supersede/TTL/session change | клиентская подмена identity |
| `VoNIntegration` | manager | authority service | world/session | хранить клиентские actuator values как истину |

`RICON_LinkEvaluator.m_Model` MUST быть strong `ref` либо evaluator MUST явно принимать model только на время
вызова без сохранения. Текущий hybrid «сохранить weak field» запрещён (API-01).

## 4. `LinkQuery`

`LinkQuery` — server-built immutable value для одного направления transmitter → receiver. Клиент не формирует
ни одно поле напрямую.

| Поле | Тип/единица | Домен | Владелец/источник | Invalid behavior |
|---|---|---|---|---|
| `txPos`, `rxPos` | vector, m world-space | каждый компонент finite | server entities/snapshot | `INVALID_COORDINATE` |
| `frequencyMHz` | float, MHz | finite, `> 0`; модель MAY сузить | validated Band/Device catalog | `INVALID_FREQUENCY`; положительное вне domain модели → `OUT_OF_MODEL_DOMAIN` |
| `bandwidthHz` | float, Hz | finite, `> 0`; catalog-specific bounds | validated Channel/Device catalog | `INVALID_BANDWIDTH` |
| `txPowerDbm` | float, dBm | finite; bounds из device catalog | server catalog + state | `CATALOG_VIOLATION` |
| `txGainDbi`, `rxGainDbi` | float, dBi | finite; результат AntennaPattern | server Antenna service | `INVALID_GAIN` |
| `rxNoiseFigureDb` | float, dB | finite, `>= 0`; catalog bounds | receiver catalog | `INVALID_NOISE_FIGURE` |
| `hasRxSensitivity` | bool | — | receiver catalog | — |
| `rxSensitivityDbm` | float, dBm | finite, meaningful iff flag=true | receiver catalog | `INVALID_SENSITIVITY` |
| `hasRequiredSinr` | bool | — | receiver catalog | — |
| `requiredSinrDb` | float, dB | finite, meaningful iff flag=true | receiver catalog | `INVALID_REQUIRED_SINR` |
| `noiseEnvironmentId` | stable string ID | non-empty, registered | world/mission server config | `UNKNOWN_NOISE_ENVIRONMENT` |
| `hasInterference` | bool | — | InterferenceModel | — |
| `interferenceDbm` | float, dBm | finite, meaningful iff flag=true | InterferenceModel | `INVALID_INTERFERENCE` |
| `bandId` | stable string ID | non-empty, registered | server catalog | `UNKNOWN_BAND` |
| `modelId` | stable string ID | non-empty, registered for band | registry resolution | `UNKNOWN_MODEL` |
| `intelligibilityModelId` | stable string ID | non-empty, registered | server config | `UNKNOWN_MODEL` |

Ровно один из `hasRxSensitivity` и `hasRequiredSinr` MUST быть true: каталог либо задаёт измеренную sensitivity,
либо требуемый SINR, из которого link-eval получает sensitivity по `REQ-link-eval.md`. Оба/ни одного — ошибка
catalog configuration.

Числовые sentinel (`-999`, `-900`) MUST NOT кодировать отсутствие optional-поля. Global bounds мощности/усиления
MUST NOT быть зашиты в evaluator: они принадлежат server catalog конкретного устройства/диапазона.

### 4.1 Детерминированный порядок валидации

Если вход нарушает несколько границ, budget MUST сообщать первую причину в фиксированном порядке:

1. координаты;
2. частота (`f`);
3. полоса (`B`);
4. мощность передатчика;
5. усиления;
6. noise figure;
7. XOR «ровно один порог»;
8. значение выбранного порога;
9. interference flag/value;
10. `bandId`;
11. propagation `modelId`;
12. `noiseEnvironmentId`;
13. `intelligibilityModelId`;
13a. config выбранной модели разборчивости;
14. domain уже выбранной модели.

Этот порядок — часть schema revision и MUST проверяться multi-invalid vectors, чтобы Enforce и oracle не выбирали
разные reason codes (`oracle/vectors/link_eval_v1.json`, revision 2).

После вычисления действуют model-output boundaries в отдельном фиксированном порядке:

15. контракт списка loss layers;
16. `Q` конечна и находится в `[0,1]`;
17. при открытом пороге state входит в `{WEAK, DEGRADED, CLEAN}`.

Нарушение шага 13a даёт `CATALOG_VIOLATION`; шагов 15–17 — `MODEL_CONTRACT_VIOLATION`.

### 4.2 Stable reason codes

`RICON_EReason` использует явные стабильные integer values. Уже опубликованное значение MUST NOT
перенумеровываться или получать другую семантику.

| Reason | Граница |
|---|---|
| `OK` | budget valid |
| `INVALID_COORDINATE` | нечисловая/неконечная позиция |
| `INVALID_FREQUENCY`, `INVALID_BANDWIDTH`, `INVALID_GAIN`, `INVALID_NOISE_FIGURE` | соответствующий input-domain |
| `INVALID_SENSITIVITY`, `INVALID_REQUIRED_SINR`, `INVALID_INTERFERENCE` | выбранное optional-значение некорректно |
| `CATALOG_VIOLATION` | XOR/конфиг устройства или модели нарушен |
| `UNKNOWN_BAND`, `UNKNOWN_MODEL`, `UNKNOWN_NOISE_ENVIRONMENT` | ID не разрешён authority registry |
| `OUT_OF_MODEL_DOMAIN` | input физически валиден, но выбранная модель неприменима |
| `NUMERIC_FAILURE` | вычисление выпустило нечисловой/неконечный результат |
| `MODEL_CONTRACT_VIOLATION` | модель вернула недопустимые слои, Q или state при корректном input/config |

## 5. Loss layers и `LinkBudget`

### 5.1 Расширяемый слой

`LossLayer` MUST содержать:

| Поле | Контракт |
|---|---|
| `layerId` | stable, non-empty, уникален внутри budget; например `fspl`, но список не закрыт |
| `lossDb` | finite dB; MAY быть отрицательным только если модель документирует физический gain |
| `modelId` | источник значения для trace/oracle |
| `evidenceId` | опциональная ссылка на формулу/config revision, не путь к чужому коду |

Модель MAY добавлять новый слой без изменения common `LinkBudget`. Duplicate `layerId` MUST быть invalid.
Порядок сериализации MUST быть детерминирован: lexicographic `layerId` либо явно versioned registry order.

### 5.2 Budget

| Поле | Тип/единица | Пишет | Инвариант |
|---|---|---|---|
| `distanceM` | float, m | evaluator | finite, `>= 0` |
| `layers` | strong array of layers | propagation/noise services | unique IDs; all finite |
| `totalLossDb` | float, dB | evaluator | finite; равен сумме declared loss layers |
| `rxPowerDbm` | float, dBm | evaluator | finite |
| `thermalNoiseDbm` | float, dBm | evaluator | finite; `kT₀B` до NF/external noise |
| `noiseDbm` | float, dBm | NoiseModel/evaluator | finite; system noise до interference |
| `noisePlusInterferenceDbm` | float, dBm | evaluator | finite; stable power sum N+I |
| `sinrDb` | float, dB | evaluator | finite; `rxPowerDbm - noisePlusInterferenceDbm` |
| `noiseModelInDomain` | bool | NoiseModel | false не разрешает экстраполяцию модели |
| `intelligibility` | float | policy из `REQ-link-eval` | finite `[0,1]` |
| `state` | enum | state policy | согласован с valid/intelligibility |
| `fadingM` | optional value + flag | L3 model | finite и domain-valid либо отсутствует |
| `valid` | bool | evaluator | true только если весь budget прошёл invariants |
| `reason` | enum | first failing boundary | `OK` iff valid=true |

Fixed projection-поля (`fsplDb`, `weatherDb` и т.п.) MAY существовать только как versioned compatibility/oracle
view; они MUST NOT быть единственным способом добавить новый физический слой.

Invalid budget MUST иметь `valid=false`, `state=LOST`, `intelligibility=0`, непустой reason. Остальные numeric
поля MUST быть детерминированными finite defaults; invalid budget запрещено использовать в route quality.

## 6. Evaluator contracts

- Общий `LinkEvaluator` MUST быть интерфейсом/базовым контрактом `LinkQuery → LinkBudget` без проверки типа сети.
- Wireless evaluator MUST быть отдельной реализацией; wired stub MUST реализовать тот же контракт без radio noise.
- Band registry MUST разрешить `bandId + modelId` до вычисления. Mismatch query/evaluator MUST fail closed.
- Registration MUST быть authority-only, диагностируемой и отклонять empty/duplicate IDs/null model.
- Model registration MUST требовать declared domain и successful finite/self-check; golden/oracle status хранится как
  evidence metadata, а не вычисляется клиентом.
- `GetInstance()` MUST быть null-safe для `GetGame()==null` и отсутствующего GameMode/component.
- Authority check MUST быть централизован; callback, документированный server-only, SHOULD не повторять gate без
  причины. Неверный mutation-call MUST логировать reason в dev/test режиме.

## 7. Registries и extension seams

| Registry/seam | Key → value | Обязательный contract |
|---|---|---|
| Band | `bandId → BandDefinition` | frequency/channel domain, allowed model IDs, catalog revision |
| Propagation | `modelId → PropagationModel` | domain, layer IDs, finite self-check |
| Noise | `noiseEnvironmentId → NoiseModel + versioned environment config` | frequency/environment domain; out-of-domain is explicit |
| Interference | `interferenceModelId → InterferenceModel` | returns optional aggregate or documented richer result |
| Antenna | `patternId → AntennaPattern` | direction/frequency → finite dBi |
| Network | `networkTypeId → RoutingPolicy` | route context, access policy, topology limits |
| Call session | `networkTypeId → CallSessionFactory` | common state machine seam; cellular/satellite configure it |
| Evaluator | `evaluatorTypeId → LinkEvaluator` | wireless/wired implementations without core branching |

Registries MUST become read-only after initialization unless a versioned hot-reload transaction with rollback is
implemented later. Unknown key MUST NOT silently use free-space/default.

## 8. `LinkSnapshot`: identity, ordering и TTL

| Поле | Тип/единица | Contract |
|---|---|---|
| `schemaVersion` | positive int | exact supported version; mismatch reject |
| `sessionEpoch` | int `1..int.MAX-1` | authority-generated; changes before a new world/session may accept snapshots |
| `linkId` | positive int | unique within session for one logical talker→listener/network link |
| `linkSeq` | int `1..int.MAX-1` | strictly increasing per `(sessionEpoch, linkId)`; `int.MAX` запрещён |
| `ttlMs` | positive int, ms | duration; float forbidden |
| `talkerPlayerId`, `listenerPlayerId` | int | server-resolved identities |
| `route` / `routeId` | versioned identity | enough to correlate legs/effects without exposing hidden links in UI |
| `quality`, `state` | `[0,1]`, enum | derived from valid route |

### 8.1 Session epoch

Contract требует различие активных session/world generations, а не глобально уникальный timestamp. Authority MUST
изменить epoch до публикации нового session state. Конкретный источник (persisted counter, process counter с seed и
т.п.) выбирается реализацией и проходит collision/restart tests. Один `System.GetUnixTime()` не достаточен:
разрешение 1 с, int32/2038 и коррекция часов (`R-core-20261018-api.md`, API-07).

### 8.2 Sequence overflow

`linkSeq` MUST NOT wrap и authority MUST NOT выдавать значение `int.MAX`. Максимальное выданное значение —
`int.MAX - 1`. Если следующий seq был бы `int.MAX`, authority MUST получить новый `sessionEpoch` в диапазоне
`1..int.MAX-1`, отличный от прежнего, выполнить full resync и начать seq нового epoch с 1. Epoch allocator также
MUST оставаться в signed-int domain: после `int.MAX-1` он MAY продолжить с 1, если это не текущий epoch.
Saturating/wrapped сравнение запрещено.

### 8.3 Ordering

Сравнение возвращает стабильный decision enum, а не bool:

| Decision | Условие | Эффект |
|---|---|---|
| `ACCEPT_FIRST` | session identity уже установлена full resync; текущего snapshot для linkId нет; вход валиден и совпадает с identity | принять и сохранить |
| `ACCEPT_NEWER` | schema/session/link совпадают, новый seq строго больше | заменить текущий |
| `STALE` | identity совпадает, новый seq меньше либо равен | игнорировать без side effects |
| `INVALID_SNAPSHOT` | сам вход нарушает field domains §8 | отклонить, сохранить последний valid до TTL, diagnostic |
| `INVALID_IDENTITY` | schema/session/link не совпадают | отклонить; запросить full resync |
| `ACCEPT_RESYNC` | full-resync event целиком прошёл atomic validation | заменить identity и slots; effects нет |
| `INVALID_RESYNC` | full-resync event нарушил любое atomic условие | очистить identity/slots; `REQUEST_RESYNC` + `DIAGNOSTIC` |

До первого успешного full resync identity отсутствует: обычный snapshot MUST дать `INVALID_IDENTITY` +
`REQUEST_RESYNC`, а не `ACCEPT_FIRST`.

При `INVALID_IDENTITY` из-за нового `sessionEpoch` получатель MUST очистить slots **и сбросить установленную
session identity**, затем выставить эффект `REQUEST_RESYNC`. После этого обычные snapshots как старого, так и нового
epoch отклоняются до full resync; delayed packet старого epoch не может стать `ACCEPT_FIRST`. Новый epoch принимается
только отдельным full-resync event, который атомарно меняет session identity и начальное состояние. Mismatch `linkId` —
ошибка маршрутизации к slot: snapshot не принимается; `REQUEST_RESYNC` остаётся fail-closed эффектом.

Valid snapshot для ordering имеет `schemaVersion > 0`, `sessionEpoch` в `1..int.MAX-1`, `linkId > 0`, `linkSeq > 0`,
`ttlMs > 0`, `routeId > 0`, `routeNodeCount > 0`, finite `quality` в `[0,1]` и допустимое state. LOST MUST иметь
quality 0. Дополнительно `linkSeq < int.MAX`. Нарушение любого из этих условий — `INVALID_SNAPSHOT` до сравнения
identity/seq.

Full resync MUST быть атомарным. Он принимается только если schema поддерживается, epoch в `1..int.MAX-1`, каждый snapshot
валиден, совпадает с schema/epoch resync и `linkId` уникален. При успехе decision = `ACCEPT_RESYNC`, effects пусты,
identity и все slots заменяются одной транзакцией. При любой ошибке decision = `INVALID_RESYNC`, весь resync
отклоняется, identity/slots очищаются, effects = `REQUEST_RESYNC` + `DIAGNOSTIC`; частичное
принятие/фильтрация отдельных entries запрещены.

### 8.4 TTL

Authority отвечает за route/lease expiry. Получатель owner-scoped UI сохраняет локальное время получения и скрывает
snapshot после `ttlMs`; это presentation timeout, не authority. Network delay не превращает TTL в доказательство
серверного lease. Clock synchronization между клиентом и сервером не предполагается.

Время TTL — локальное монотонное время получателя. Snapshot видим при `elapsedMs < ttlMs` и истёк при
`elapsedMs >= ttlMs`. Отрицательный elapsed/clock anomaly MUST fail closed как expired и дать diagnostic; wall-clock
и server UTC для сравнения TTL запрещены. Запрос видимости отсутствующего link slot возвращает false без effects.

## 9. Routing и VoN seam

### 9.1 Route и порядок валидации

Route MUST явно хранить `talkerEndpointId`, `listenerEndpointId` и непустой упорядоченный список legs. Каждая leg
сильно владеет valid budget и содержит положительные `fromEndpointId`/`toEndpointId`; self-leg запрещена.

Stable route reasons:

| Reason | Значение |
|---|---|
| `OK` | маршрут прошёл contract |
| `INVALID_CONTEXT` | maxHops/session/context недопустимы |
| `EMPTY_ROUTE` | legs отсутствуют |
| `TOO_MANY_HOPS` | `legs.Count() > maxHops`; равенство допустимо |
| `NULL_LEG` | элемент legs отсутствует |
| `INVALID_ENDPOINT` | endpoint ID неположителен либо leg ведёт сама в себя |
| `INVALID_BUDGET` | budget отсутствует/invalid/Q не finite `[0,1]`/state недопустим |
| `WRONG_START` | первая leg начинается не с talker |
| `DISCONTINUITY` | `leg[i].from != leg[i-1].to` |
| `CYCLE` | endpoint уже встречался ранее в route |
| `WRONG_DESTINATION` | последняя leg заканчивается не listener |

Сообщается первая причина в **глобальном** порядке таблицы, а не «все проверки первой leg, затем второй»:
после context/empty/hops выполняются отдельные полные проходы NULL_LEG → INVALID_ENDPOINT → INVALID_BUDGET,
затем WRONG_START, затем один упорядоченный проход continuity+cycle, затем WRONG_DESTINATION. Поэтому более поздняя
null-leg имеет приоритет над более ранним cycle.

В проходе continuity+cycle для каждой leg сначала проверяется continuity, затем endpoint `to` проверяется/добавляется
в seen-set; talker находится в set до первой leg. Возврат к talker и повтор любого relay — `CYCLE`; если одна leg
одновременно разрывает путь и повторяет узел, результат `DISCONTINUITY`.

Route quality = minimum leg intelligibility; route state = worst leg state по stable enum ordering. Invalid route
MUST давать quality 0 и state LOST, не частичный результат. `maxHops` — path limit и не связан с snapshot `ttlMs`.

### 9.2 RoutingContext и policy

`RoutingContext` — immutable input policy: positive `sessionEpoch`, non-negative monotonic `serverTimeMs`, positive
`maxHops`, positive correlation ID и strong reference на abstract `AccessPolicy`. Он MUST NOT содержать concrete
`CommsManagerComponent` или изменяемые registries. Дополнительные topology/capacity services подключаются через
read-only interfaces в следующей версии context.

`AccessPolicy.CanUseEndpoint(playerId, endpointId)` — server-only seam. Base implementation fail closed.
`RoutingPolicy.FindRoute(context, talkerEndpointId, listenerEndpointId)` MUST получать context явно и не читать
скрытые globals.

### 9.3 CallSession и VoN

`CallSession`/`CallSessionFactory` в Э2 — только abstract seam для EXT-3; состояния звонка не придумываются до
этапа конкретной сети. Factory и policy имеют stable non-empty `networkTypeId`.

`VoNIntegration` не владеет route/snapshot registries. До ADR-001/Workbench evidence методы MAY быть no-op
scaffold; никакой недоказанный actuator не получает `PASS`. Snapshot должен пройти receiver/ordering contract до
передачи в seam; server authority проверяется вызывающим service.

## 10. Пять обязательных extension-сценариев

| # | Сценарий | PASS без изменения общего ядра |
|---|---|---|
| EXT-1 | Новая УКВ-рация | новый catalog/config с существующими Band/Model/Pattern; ни один common `.c` не меняется |
| EXT-2 | HF sky-wave | новая PropagationModel объявляет собственные layer IDs/domain; query/budget/schema не меняются |
| EXT-3 | Спутниковая сеть | новый NetworkType/RoutingPolicy + общий CallSession seam; cellular код не копируется |
| EXT-4 | Направленная антенна | новый AntennaPattern config/implementation возвращает gain; evaluator не знает тип антенны |
| EXT-5 | Проводной evaluator | новая evaluator implementation; wireless formulas и `if wired` в core не добавляются |

Каждый сценарий MUST иметь compile fixture и CT. EXT-2/4 дополнительно требуют OR после физического REQ; EXT-3/5
не должны зависеть от VoN runtime для CONTRACT_PASS.

## 11. Failure semantics

| Boundary | Failure | Обязательный результат |
|---|---|---|
| Query | `f <= 0` | invalid budget + `INVALID_FREQUENCY`; это malformed input, не domain модели |
| Query | `f > 0`, но вне domain выбранной модели | invalid budget + `OUT_OF_MODEL_DOMAIN` |
| Query | прочий malformed/non-finite/domain | invalid budget + specific reason; no exception/NaN |
| Registry | empty/duplicate/unknown | reject + diagnostic; no fallback |
| Model | null/unknown | invalid budget + соответствующий `UNKNOWN_*` |
| Model | out-of-domain/non-finite | `OUT_OF_MODEL_DOMAIN` / `NUMERIC_FAILURE` |
| Model | invalid layers/Q/state при корректном input | `MODEL_CONTRACT_VIOLATION` |
| Snapshot | schema/session/link mismatch | reject + resync/diagnostic policy |
| Snapshot | stale/equal seq | ignore as stale, no side effects |
| Snapshot | seq near overflow | rotate session + full resync before wrap |
| Route | empty/invalid leg/loop | LOST + reason; hop limit enforced |
| Component | `GetGame`/GameMode/component absent | return null/NOT_READY, no dereference |
| Authority | client mutation attempt | reject, rate-limit/log in security evidence |
| VoN | actuator not proven | no-op scaffold + explicit BLOCKED evidence |

## 12. Acceptance matrix

| Test ID | Что проверяет | Findings | Evidence |
|---|---|---|---|
| CT-CORE-001 | null/transient model lifetime не исчезает | API-01 | CT + WB |
| CT-CORE-002 | ровно один из sensitivity/required-SINR; omitted interference; explicit `0 dBm` не absent | A13/API-02 | CT |
| CT-CORE-003 | NaN/Inf в каждом scalar/vector component; invalid outputs finite; multi-invalid соблюдает §4.1 | A8/A10/API-02 | CT + OR |
| CT-CORE-004 | empty/duplicate/unknown registry IDs и band/model mismatch | B4/API-04 | CT |
| CT-CORE-005 | stale/equal/new seq; schema/session/link mismatch | B2 | CT |
| CT-CORE-006 | seq overflow инициирует epoch rotation до wrap | B2 | CT |
| CT-CORE-007 | TTL presentation expiry отдельно от server lease | B2 | CT; RT позже |
| CT-CORE-008 | server-only mutation; fake client parameters не меняют registries | R-04/R-05 | CT; RT позже |
| CT-CORE-009 | route reasons/order; min/worst; continuity/cycle/maxHops boundary | B8/B9 | CT + OR |
| CT-CORE-010 | `GetGame()==null`, no GameMode, no component | API-03 | CT/WB |
| CT-CORE-011 | model: empty/null/duplicate/wrong-owner layer; Q вне `[0,1]`; LOST выше threshold | C13/REQ-LE-31/73/75 | CT |
| CT-EXT-001…005 | сценарии EXT-1…EXT-5 | B3/B5/B6/B7/B9 | CT + WB |

## 13. Mapping findings → требования

| Finding | Требования / тесты |
|---|---|
| B2 | §8, CT-CORE-005…007 |
| B3 | §5, EXT-2 |
| B4 | §4, §6–7, CT-CORE-004 |
| B5 | §6, EXT-5 |
| B6 | Antenna registry, EXT-4 |
| B7 | Interference registry, REQ-CORE-003 |
| B8/B9 | §9, CT-CORE-009, EXT-3 |
| B11/B12 | §3–4, field tables, lifecycle tests |
| API-01 | §3, CT-CORE-001 |
| API-02 | §4–5, CT-CORE-002/003 |
| API-03 | §6, CT-CORE-010 |
| API-04 | REQ-CORE-001/005, CT-CORE-004/008 |
| API-05/06 | component attachment + WB gate |
| API-07/B1 | §8.1, CT-CORE-005/006 |

## 14. Stopping conditions и следующий gate

До правок `.c` обязательны:

1. mutual agree claude+codex на этот документ и `REQ-link-eval.md`;
2. единый bounded implementation plan: точные файлы, owners, CT/OR/WB для каждого slice;
3. отсутствие нерешённых противоречий между двумя REQ.

После реализации статус может быть только `STATIC_IMPLEMENTED` + отдельные `CONTRACT_PASS`/`ORACLE_PASS`.
Без исполненного Workbench №001 — `WORKBENCH_COMPILE_PASS = BLOCKED`; без разрешения Play — RT/AUDIO остаются
`BLOCKED_NO_RUNTIME`.
