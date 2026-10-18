# oracle — эталонные векторы для оценки линка

Host-side эталон на Python stdlib. Это **спецификация в виде чисел**, а не второй production core
(PLAN v5 §4.2, правило oracle). Требования — `docs/req/REQ-link-eval.md` (физика) и
`docs/req/REQ-core-contracts.md` (поля, коды отказов).

## Разделение ответственности (независимость эталона)

| Файл | Автор | Что содержит |
|---|---|---|
| `vectors/link_eval_v1.json` | claude | ожидаемые значения, посчитанные по REQ-LE независимо |
| `ricon_oracle.py` | codex | вычислитель по REQ-LE (без чтения ожиданий при написании) |
| `test_ricon_oracle.py` | codex | прогон векторов и проверка формата |

Два независимых расчёта сверяются друг с другом. Расхождение — ошибка в одном из них или неоднозначность REQ;
её разбирают в шине, а не подгоняют числа.

## Формат `vectors/link_eval_v1.json`

```text
{
  schema: "ricon.oracle.link_eval", version: 1,
  source: "docs/req/REQ-link-eval.md@<commit>",
  tolerances: { db: 0.01, ratio: 1e-6, m: 1e-6 },
  specialFloats: ["NaN", "Infinity", "-Infinity"],
  fixtures: { linear_test: { a, b, qClean, qDegraded, status: "TEST_ONLY" } },
  vectors: [ { id, kind, req: [REQ-LE-…], input: {…}, expected: {…}, tags: [...] } ]
}
```

- Допуски по имени поля (регистрозависимо): суффиксы `Dbm`, `Db`, `Dbi` — `db`; суффикс `M` (`distanceM`,
  `effectiveDistanceM`) — `m`; `intelligibility` и пороги `q…` — `ratio`.
- `revision` растёт при добавлении векторов без смены формата; `version` — версия формата.
- Стандартный JSON не умеет `NaN`/`Inf`: такие значения записаны строками из `specialFloats`.
- Имена полей — как в `REQ-core-contracts` §4 (вход) и §5.2 (бюджет); слои — `layers: [{layerId, lossDb}]`.

| `kind` | Вход | Ожидание | Требования |
|---|---|---|---|
| `fspl` | `distanceM`, `frequencyMHz` | `lossDb`, `effectiveDistanceM` | REQ-LE-40, 41 |
| `thermal_noise` | `bandwidthHz` | `thermalNoiseDbm` | REQ-LE-03, 60 |
| `system_noise` | `bandwidthHz`, `rxNoiseFigureDb`, `noiseEnvironmentId`, `frequencyMHz` | `faDb` (0 при `f_a = 1`), `noiseDbm`, `noiseModelInDomain` | REQ-LE-61–64 |
| `power_sum` | `powersDbm[]` | `sumDbm` | REQ-LE-65, 66 |
| `intelligibility_linear_test` | `sinrDb`, `params{a,b}` | `intelligibility` | REQ-LE-73, 74 (TEST_ONLY) |
| `budget` | полный `LinkQuery` | все поля бюджета, `state`, `reason = OK` | REQ-LE-40…75 |
| `invalid` | `LinkQuery` с одним нарушением или с двумя (тег `order`: побеждает первая граница в порядке REQ-LE-80) | `valid = false`, `reason`, `state = LOST`, `intelligibility = 0`, `allFinite = true` | REQ-LE-80, 81 |

Категории внешнего шума (`noiseEnvironmentId`): `city`, `residential`, `rural`, `quiet_rural`, `none`.

## Статус чисел

- Разборчивость и пороги состояний во всех векторах `budget` — фикстура `linear_test` (**TEST_ONLY**, не модель
  речи; production-кривая — `BLOCKED_CALIBRATION`, REQ-LE-74). Векторы проверяют механику seam, не реализм.
- Константы внешнего шума — ITU-R P.372-11 по вторичному источнику (REF); редакция P.372 не подтверждена.
- Эти векторы дают только `ORACLE_PASS`. Паритет Enforce ↔ oracle (`ENFORCE_PARITY`) требует исполнения
  Enforce-кода и сейчас `NOT_TESTED`.

## Снимки линка: `vectors/snapshot_v1.json`

Эталон порядка `LinkSnapshot` по `docs/req/REQ-core-contracts.md` §8 (ревизия в поле `source`).

| Файл | Автор | Что содержит |
|---|---|---|
| `vectors/snapshot_v1.json` | claude | ожидания, написанные вручную по тексту §8 |
| `snapshot_oracle.py`, `test_snapshot_oracle.py` | claude | исполнимая модель §8 и тесты (stdlib) |
| `Core/RICON_LinkSnapshot.c` и менеджер | codex | Enforce-реализация, независимая от этих файлов |

| `kind` | Что проверяет |
|---|---|
| `compare` | решение для пары «текущий слот / входящий снимок»: `ACCEPT_FIRST`, `ACCEPT_NEWER`, `STALE`, `INVALID_SNAPSHOT` (домены — до identity), `INVALID_IDENTITY` |
| `receiver_trace` | последовательность событий `full_resync` / `receive` / `visible`: identity только через resync, сброс state при новом epoch, `REQUEST_RESYNC` / `DIAGNOSTIC`, TTL по локальному монотонному времени |
| `allocator_trace` | выдача seq authority: максимум `int.MAX − 1`, затем новый non-zero epoch, full resync и seq 1 у всех линков |

`full_resync` атомарен: принят целиком (`ACCEPT_RESYNC`) или отвергнут целиком (`INVALID_RESYNC`, эффекты
`REQUEST_RESYNC` + `DIAGNOSTIC`, identity и слоты сброшены) — при неподдерживаемой schema, epoch = 0, любой
невалидной записи, записи другого epoch/schema или повторе `linkId`. `ACCEPT_FIRST` возможен только при уже
установленной identity: до первого успешного resync любой снимок → `INVALID_IDENTITY` (RX-01). `visible` для пустого
слота → `false` без эффектов (RX-09). Домены `linkSeq` и `sessionEpoch` — `[1, int.MAX − 1]`; после `int.MAX − 1` epoch продолжается с 1. Проверка:
`python -I oracle/snapshot_oracle.py --check oracle/vectors/snapshot_v1.json` → `SNAPSHOT_ORACLE_PASS`.

## Маршруты: `vectors/route_v1.json`

Эталон проверки маршрута по `docs/req/REQ-core-contracts.md` §9 (ревизия в поле `source`).

| Файл | Автор | Что содержит |
|---|---|---|
| `vectors/route_v1.json` | claude | ожидания, написанные вручную по тексту §9 |
| `route_oracle.py`, `test_route_oracle.py` | claude | исполнимая модель §9 и тесты (stdlib) |
| `Network/RICON_NetworkContracts.c` | codex | Enforce-реализация, независимая от этих файлов |

Вход: `context` (`sessionEpoch`, `serverTimeMs`, `maxHops`, `correlationId`, `hasAccessPolicy`) и `route`
(`talkerEndpointId`, `listenerEndpointId`, `legs[{fromEndpointId, toEndpointId, budget{valid, intelligibility, state}}]`).
Ожидание: `reason`, `quality`, `state`. Для валидного маршрута — `min(Q)` и худшее состояние; для невалидного —
`quality = 0`, `LOST`. Причины — в глобальном порядке §9.1: контекст → пусто → hops → null-нога → endpoint →
бюджет → начало → один проход «разрыв, затем петля» → конечная точка. Векторы с тегом `order` содержат два
нарушения. Проверка: `python -I oracle/route_oracle.py --check oracle/vectors/route_v1.json` → `ROUTE_ORACLE_PASS`.
