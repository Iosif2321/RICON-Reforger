# REQ-pair-service — серверная служба пар (S3)

Версия: **0.2 AGENT-REVIEWED** (codex #3192), 2026-10-21. Автор: claude. Основание: owner agora #2937 («ядро и оболочки»),
#2936 (качество зависит от отправителя и приёмника). План и разделение работ: agora #3172, #3178, #3179, #3180.
Ревью codex: #3185, #3187 (учтены в v0.2: generation и epoch слушателя, будущий issuedAt, домены, таблица
трансиверов). Опирается на `REQ-radio-device-adapter.md` v0.2 (S2), `REQ-lifecycle.md`, `REQ-core-contracts.md`,
`REQ-link-eval.md`.
Корпус `oracle/vectors/pair_service_v1.json` rev1 (72 случая) заморожен 2026-10-21 (codex #3201, claude #3205),
SHA-256 `fcc9cefa1b8bb46374b67883c239ffc8c6c00343da49a6bf008cf679cac178e1`. До freeze codex пересчитал его своим
checker'ом из этого REQ, не читая эталон: 72/72, расхождений нет (#3200).

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

## 1. Входы, происхождение и домены

Все входы строит сервер. Служба не принимает клиентских значений и не проверяет, откуда пришли объекты.
Все целые — int32, флаги — bool, без неявных преобразований.

**REQ-PS-01. `RICON_TransmissionBinding` — граница, а не экспорт lifecycle.** `RICON_LifecycleLease`
(`RICON_Lifecycle.c:391–402`) не хранит сеть, канал и индекс трансивера (codex #3178). Поэтому вход службы —
отдельный DTO, который будущий серверный код заполняет из PTT-запроса, **принятого** lifecycle, и из выданного
lease. В S3 этот код не пишется; векторы задают DTO явно.

| Поле | Домен | Смысл |
|---|---|---|
| leaseId | 1 … int.MAX | id lease; lifecycle выдаёт и int.MAX (`RICON_Lifecycle.c:804, 856`) |
| sessionEpoch | 1 … int.MAX | epoch, в которой выдан lease |
| txPlayerId | 1 … int.MAX | говорящий игрок |
| txGeneration | 1 … int.MAX | его generation на момент выдачи |
| txDeviceId | 1 … int.MAX−1 | рация отправителя (домен S2) |
| txTransceiverIndex | 0 … int.MAX−1 | трансивер отправителя (домен S2) |
| networkTypeId, channelId | 1 … int.MAX | сеть и канал из принятого PTT-запроса |
| active | bool | lease в состоянии ACTIVE по данным lifecycle |
| issuedAtMs | 0 … int.MAX | время выдачи, ms |
| ttlMs | 1 … int.MAX | срок, ms |

**REQ-PS-02. Слушатель `RICON_PairListener`** — какой игрок слушает какой трансивер какой рации:

| Поле | Домен |
|---|---|
| playerId | 1 … int.MAX |
| generation | 1 … int.MAX — generation игрока, для которой записан слушатель |
| sessionEpoch | 1 … int.MAX — epoch записи |
| deviceId | 1 … int.MAX−1 |
| transceiverIndex | 0 … int.MAX−1 |

**REQ-PS-03. Состояние сервера на момент вызова:**

- `nowMs` (0 … int.MAX) и текущая `sessionEpoch` сервера (1 … int.MAX);
- игроки `RICON_PairPlayer`: playerId (1 … int.MAX), generation (1 … int.MAX), state — одно из значений
  `RICON_EPlayerLifecycleState`;
- рации: `RICON_LifecycleDevice`, ключ — его deviceId; `IsValid()` обязано быть истинным;
- трансиверы `RICON_PairEndpoint`: пара (`RICON_RadioObservation`, `RICON_RadioRfProfile`) S2. Ключ — deviceId и
  transceiverIndex **наблюдения**; так одна рация может иметь несколько трансиверов в одном вызове (codex #3187);
- `RICON_RadioQueryContext` S2 и один evaluator — объект базового класса `RICON_LinkEvaluator`
  (`RICON_LinkEvaluator.c:5`), в векторах — `RICON_WirelessLinkEvaluator`. Одна полоса и один контекст на вызов —
  явная граница;
- лимиты `RICON_PairServiceLimits`: maxBindings, maxListeners, maxPlayers, maxDevices, maxEndpoints — каждый
  1 … 4096; maxPairs — 1 … 65536. Тогда `bindings × listeners` ≤ 4096² помещается в int32.

## 2. Отказ всего вызова (preflight)

**REQ-PS-04.** До первого обращения к S2 или evaluator вызов проверяет вход целиком. При любом отказе результат —
код вызова и **пустой** список пар; S2 и evaluator не вызываются. Порядок нормативный, срабатывает первый
применимый код:

| Код вызова | Условие |
|---|---|
| NULL_INPUT | отсутствует любой массив, контекст, evaluator или лимиты |
| INVALID_LIMITS | любой лимит вне домена §1 |
| LIMIT_EXCEEDED | длина любого массива больше своего лимита, либо `bindings × listeners > maxPairs` |
| INVALID_ELEMENT | null-элемент; `nowMs` или текущая epoch вне домена; поле binding, слушателя или игрока вне домена; `RICON_LifecycleDevice.IsValid()` ложно; у трансивера null-часть или ключ (deviceId, transceiverIndex наблюдения) вне домена S2 |
| DUPLICATE | повтор leaseId; повтор txDeviceId среди bindings; повтор playerId среди игроков; повтор deviceId среди раций; повтор (deviceId, transceiverIndex) среди трансиверов; повтор слушателя (playerId, deviceId, transceiverIndex) |
| OK | вход принят; пар ровно `bindings × listeners` |

Лимит пар считается до фильтров: число кандидатов известно заранее, работа ограничена сверху. Обрезки нет.
Домены полей S2 внутри трансивера (координаты, RF-поля) preflight не проверяет: это делает сборщик S2 на шаге 8.

## 3. Код пары

**REQ-PS-05.** Каждая пара (binding, listener) получает ровно один код. Порядок нормативный:

| № | Код пары | Первая применимая причина |
|---|---|---|
| 1 | BINDING_INACTIVE | `active` ложно; epoch binding ≠ текущей; `nowMs < issuedAtMs`; `nowMs − issuedAtMs ≥ ttlMs` |
| 2 | TX_PLAYER_INVALID | говорящего нет среди игроков; его state не BOUND_ACTIVE; его generation ≠ txGeneration |
| 3 | TX_DEVICE_INVALID | рации отправителя нет; трансивера (txDeviceId, txTransceiverIndex) нет; `!CanAccess(txPlayerId)`; `!Allows(networkTypeId, channelId)` |
| 4 | SELF_PAIR | deviceId слушателя равен txDeviceId (любой индекс трансивера) |
| 5 | RX_PLAYER_INVALID | epoch слушателя ≠ текущей; слушателя нет среди игроков; его state не BOUND_ACTIVE; его generation ≠ generation слушателя |
| 6 | RX_DEVICE_INVALID | рации слушателя нет; трансивера (deviceId, transceiverIndex) нет; `!CanAccess(playerId)`; `!Allows(networkTypeId, channelId)` |
| 7 | STATE_MISMATCH | у любой стороны питание рации в lifecycle ≠ питанию наблюдения S2 |
| 8 | DEVICE_QUERY | S2 Build вернул не READY; код S2 сохраняется в паре |
| 9 | EVALUATED | S2 READY; бюджет посчитан evaluator |

Шаг 1: разность `nowMs − issuedAtMs` вычисляется только при `nowMs ≥ issuedAtMs`, оба значения ≥ 0, поэтому
переполнения int32 нет. Сумма `issuedAtMs + ttlMs` не используется.

**REQ-PS-06. S2 вызывается так:** TX — наблюдение и профиль трансивера отправителя, RX — трансивера слушателя,
контекст — общий. Сборщик S2 по-прежнему проверяет домены, профиль, питание, настройку, частоту и ключ. Служба этих
правил не дублирует.

**REQ-PS-07. Бюджет** — результат существующего `evaluator.Evaluate(query)` без изменений: `valid`, `reason`,
`state`, `intelligibility`, `sinrDb`. EVALUATED с невалидным бюджетом — нормальный исход: холм, дальность или модель
могли отказать. Служба его не скрывает и не превращает в другой код.

## 4. Выход

**REQ-PS-08.** Список из `bindings × listeners` записей при OK. Порядок по ключу:
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

**REQ-PS-09.** Одинаковый вход в любом порядке элементов массивов даёт одинаковый выход. S2 вызывается не больше
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
| CT-PS-004 | время: `nowMs = issuedAtMs − 1` и `= issuedAtMs + ttlMs` неактивны, `= issuedAtMs + ttlMs − 1` активен; issuedAtMs и ttlMs у int.MAX |
| CT-PS-005 | SELF_PAIR при другом индексе трансивера той же рации |
| CT-PS-006 | два слушателя на двух трансиверах одной рации в одном вызове |
| CT-PS-007 | устаревший слушатель: другая generation или epoch → RX_PLAYER_INVALID |
| CT-PS-008 | DEVICE_QUERY для каждого не-READY кода S2, достижимого после фильтров службы |
| CT-PS-009 | EVALUATED с невалидным бюджетом (слишком далеко или вне модели) остаётся EVALUATED |
| CT-PS-010 | асимметрия: обмен ролями двух раций меняет бюджет согласно их профилям |
| CT-PS-011 | счётчики вызовов S2 и evaluator совпадают с числом пар на шагах 8–9 |
| CT-PS-012 | leaseId = int.MAX принимается; границы лимитов 4096 и 65536 |

Приёмка S3: согласованный корпус, независимый код, Enforce parity с отрицательными контролями, общая регрессия.

## 7. S3b — следующий шаг, не часть S3

linkId для логической пары talker → listener → network, linkSeq и epoch через существующий
`RICON_CommsManagerComponent.NextLinkSequence` (`:290`), снимки `RICON_LinkSnapshot`, TTL и ротация
(REQ-core §8). Требования S3b — отдельным документом после закрытия S3.

## 8. Открытые вопросы

- Взаимные помехи нескольких передач на одном канале и захват канала. В S3 помеха задаётся только контекстом.
- Одна полоса и один контекст на вызов — граница S3; несколько полос — позже.
- Стоимость при 30/60 игроках — Э10, runtime.
