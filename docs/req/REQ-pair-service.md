# REQ-pair-service — серверная служба пар (S3)

Версия: **0.1 DRAFT**, 2026-10-21. Автор: claude. Основание: owner agora #2937 («ядро и оболочки»),
#2936 (качество зависит от отправителя и приёмника). План и разделение работ: agora #3172, #3178, #3179, #3180.
Опирается на `REQ-radio-device-adapter.md` v0.2 (S2), `REQ-lifecycle.md`, `REQ-core-contracts.md`, `REQ-link-eval.md`.

Это requirements-артефакт. Он не доказывает поведение в игре: ни одна строка не проверена в мире или по сети.

## Мини-intent

- **Проблема:** S2 собирает запрос для одной заданной пары раций, но никто не решает, какие пары вообще считать.
  Активная передача, слушатели, их доступ к рациям и разрешённые каналы пока не сведены вместе.
- **Результат:** один чистый серверный вызов. Он берёт активные передачи и слушателей, отсекает недопустимые пары
  по нормативному порядку, для остальных собирает запрос S2 и считает бюджет существующим evaluator. Выход —
  детерминированный список всех пар с кодом и, где дошло до расчёта, бюджетом.
- **Затронуто:** новый Enforce-файл службы (codex), эталон `oracle/pair_service_oracle.py` и векторы (claude).
  Lifecycle, S2, evaluator, `RICON_EReason` и формулы не меняются.
- **Ограничения:** без сети, RPC, JIP, клиентов, звука, actuator, таймера тиков и discovery настоящих раций.
  Без linkId, linkSeq и снимков — это S3b (§7).

## 1. Происхождение входов и доверие

Все входы строит сервер. Служба не принимает клиентских значений и не проверяет, откуда пришли объекты.

**REQ-PS-01. `RICON_TransmissionBinding` — граница, а не экспорт lifecycle.** `RICON_LifecycleLease`
(`RICON_Lifecycle.c:391–402`) не хранит сеть, канал и индекс трансивера (codex #3178). Поэтому вход службы —
отдельный DTO, который будущий серверный код заполняет из PTT-запроса, **принятого** lifecycle, и из выданного
lease. В S3 этот код не пишется; векторы задают DTO явно.

| Поле | Смысл |
|---|---|
| leaseId | id выданного lease, 1 … int.MAX−1 |
| sessionEpoch | epoch, в которой выдан lease |
| txPlayerId, txGeneration | говорящий игрок и его generation на момент выдачи |
| txDeviceId, txTransceiverIndex | рация и трансивер отправителя |
| networkTypeId, channelId | сеть и канал из принятого PTT-запроса, > 0 |
| active | lease в состоянии ACTIVE по данным lifecycle |
| issuedAtMs, ttlMs | время выдачи и срок, ms; ttlMs > 0 |

**REQ-PS-02. Состояние сервера на момент вызова** передаётся явно:

- `nowMs` и текущая `sessionEpoch` сервера;
- таблица игроков: `playerId → (state, generation)` (`RICON_EPlayerLifecycleState`);
- таблица устройств: `deviceId → (RICON_LifecycleDevice, RICON_RadioObservation, RICON_RadioRfProfile)`;
- слушатели: `(playerId, deviceId, transceiverIndex)` — какой игрок слушает какой трансивер какой рации;
- `RICON_RadioQueryContext` S2 и один `RICON_LinkEvaluator` для его `bandId`;
- лимиты `RICON_PairServiceLimits`: maxBindings, maxDevices, maxPlayers, maxListeners, maxPairs — все > 0.

## 2. Отказ всего вызова (preflight)

**REQ-PS-03.** До первого обращения к S2 или evaluator вызов проверяет вход целиком. При любом отказе результат —
код вызова и **пустой** список пар; S2 и evaluator не вызываются. Порядок нормативный, срабатывает первый
применимый код:

| Код вызова | Условие |
|---|---|
| NULL_INPUT | отсутствует любой массив, контекст, evaluator или лимиты |
| INVALID_LIMITS | любой лимит ≤ 0 |
| LIMIT_EXCEEDED | длина любого массива больше своего лимита, либо `bindings × listeners > maxPairs` |
| INVALID_ELEMENT | null-элемент; домен поля из §1 нарушен; запись устройства с null-частью; `RICON_LifecycleDevice.IsValid()` ложно |
| DUPLICATE | повтор leaseId; повтор txDeviceId среди bindings; повтор playerId в таблице игроков; повтор deviceId в таблице устройств; повтор тройки слушателя |
| OK | вход принят; пар ровно `bindings × listeners` |

Лимит пар считается до фильтров: число кандидатов известно заранее, работа ограничена сверху. Обрезки нет.

## 3. Код пары

**REQ-PS-04.** Каждая пара (binding, listener) получает ровно один код. Порядок нормативный:

| № | Код пары | Первая применимая причина |
|---|---|---|
| 1 | BINDING_INACTIVE | `active` ложно; `sessionEpoch` binding ≠ текущей; `nowMs ≥ issuedAtMs + ttlMs` (сумма в int64, без переполнения) |
| 2 | TX_PLAYER_INVALID | говорящего нет в таблице; его state не BOUND_ACTIVE; generation ≠ txGeneration |
| 3 | TX_DEVICE_INVALID | рации отправителя нет в таблице; `!CanAccess(txPlayerId)`; `!Allows(networkTypeId, channelId)` |
| 4 | SELF_PAIR | deviceId слушателя равен txDeviceId (любой индекс трансивера) |
| 5 | RX_PLAYER_INVALID | слушателя нет в таблице; state не BOUND_ACTIVE |
| 6 | RX_DEVICE_INVALID | рации слушателя нет в таблице; `!CanAccess(rxPlayerId)`; `!Allows(networkTypeId, channelId)` |
| 7 | STATE_MISMATCH | у любой стороны: наблюдение S2 не той рации или трансивера (deviceId, index не равны binding/слушателю); питание lifecycle ≠ питанию наблюдения |
| 8 | DEVICE_QUERY | S2 Build вернул не READY; код S2 сохраняется в паре |
| 9 | EVALUATED | S2 READY; бюджет посчитан evaluator |

Generation слушателя не проверяется: в слушателе её нет, а state берётся из той же таблицы. Это упрощение
v0.1 (вопрос §8).

**REQ-PS-05. S2 вызывается так:** TX — наблюдение и профиль рации отправителя, RX — рации слушателя, контекст —
общий. Сборщик S2 по-прежнему проверяет питание, настройку, частоту и ключ. Служба этих правил не дублирует.

**REQ-PS-06. Бюджет** — результат существующего `evaluator.Evaluate(query)` без изменений: `valid`, `reason`,
`state`, `intelligibility`, `sinrDb`. EVALUATED с невалидным бюджетом — нормальный исход: холм, дальность или модель
могли отказать. Служба его не скрывает и не превращает в другой код.

## 4. Выход

**REQ-PS-07.** Список из `bindings × listeners` записей при OK. Порядок по ключу:
`(leaseId, rxDeviceId, rxTransceiverIndex, rxPlayerId)`, все по возрастанию. Входной порядок на выход не влияет.

| Поле записи | Значение |
|---|---|
| leaseId, txDeviceId | из binding |
| rxPlayerId, rxDeviceId, rxTransceiverIndex | из слушателя |
| pairCode | код §3 |
| hasDeviceCode, deviceCode | true и код S2, если дошло до шага 8; иначе false и NULL_INPUT (0) |
| hasBudget | true только для EVALUATED |
| valid, reason, state, intelligibility, sinrDb | из бюджета при hasBudget; иначе false, NUMERIC_FAILURE, LOST, 0, 0 |

Числа выхода конечны. Входы не меняются, выход не хранит ссылок на входы.

## 5. Детерминизм и стоимость

**REQ-PS-08.** Одинаковый вход в любом порядке элементов массивов даёт одинаковый выход. S2 вызывается не больше
одного раза на пару и только при коде ≥ 8; evaluator — только для READY. Эталон считает эти вызовы, векторы их
фиксируют: стоимость — часть контракта (нагрузка 30/60, PLAN Э10).

## 6. Векторы и приёмка

Файл `oracle/vectors/pair_service_v1.json`, эталон `oracle/pair_service_oracle.py`. Бюджет — существующий
`ricon_oracle.evaluate_budget` с моделью свободного пространства; рельеф уже проверен в S1 и S2.

| Test ID | Что проверяет |
|---|---|
| CT-PS-001 | 2 binding × 3 слушателя: полный список, порядок, бюджеты; перестановка входа не меняет выход |
| CT-PS-002 | каждый код вызова §2 по отдельности и несколько нарушений сразу; при отказе 0 вызовов S2 и evaluator |
| CT-PS-003 | каждый код пары §3 по отдельности и несколько причин сразу |
| CT-PS-004 | TTL: `nowMs = issuedAtMs + ttlMs − 1` активен, `= issuedAtMs + ttlMs` нет; большие значения без переполнения int32 |
| CT-PS-005 | SELF_PAIR при другом индексе трансивера той же рации |
| CT-PS-006 | DEVICE_QUERY для каждого не-READY кода S2, достижимого после фильтров службы |
| CT-PS-007 | EVALUATED с невалидным бюджетом (слишком далеко или вне модели) остаётся EVALUATED |
| CT-PS-008 | асимметрия: обмен ролями двух раций меняет бюджет согласно их профилям |
| CT-PS-009 | счётчики вызовов S2 и evaluator совпадают с числом пар на шагах 8–9 |

Приёмка S3: согласованный корпус, независимый код, Enforce parity с отрицательными контролями, общая регрессия.

## 7. S3b — следующий шаг, не часть S3

linkId для логической пары talker → listener → network, linkSeq и epoch через существующий
`RICON_CommsManagerComponent.NextLinkSequence` (`:290`), снимки `RICON_LinkSnapshot`, TTL и ротация
(REQ-core §8). Требования S3b — отдельным документом после закрытия S3.

## 8. Открытые вопросы

- Взаимные помехи нескольких передач на одном канале и захват канала. В S3 помеха задаётся только контекстом.
- Generation слушателя (REQ-PS-04, шаг 5): нужен ли явный generation в записи слушателя, как у binding.
- Один контекст и один evaluator на вызов: разные полосы в одном вызове пока не поддерживаются.
- Стоимость при 30/60 игроках — Э10, runtime.
