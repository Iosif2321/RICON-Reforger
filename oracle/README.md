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

## Жизненный цикл: `vectors/lifecycle_v1.json`

Эталон host-модели жизненного цикла по `docs/req/REQ-lifecycle.md` (ревизия в поле `source`): подключение,
регистрация, привязка тела, PTT lease, лимиты, очистка, время и ротация epoch. Это семантика модели, не runtime.

| Файл | Автор | Что содержит |
|---|---|---|
| `vectors/lifecycle_v1.json` | claude | трассы: событие → ожидаемые `resultCode`, `effects` и поля ответа, выписаны вручную по REQ |
| `lifecycle_oracle.py`, `test_lifecycle_oracle.py` | claude | исполнимая модель REQ и тесты (stdlib); тесты портят ожидания и проверяют, что раннер это ловит |
| Enforce state machine Э3 | codex | реализация, независимая от этих файлов |

Трасса — `{id, ct[], req[], note, steps[{event, expect}]}`. `event` — поля §1.2 (`type`, `nowMs`, …). `expect` —
`resultCode`, упорядоченный `effects[{type, …}]` (поля — таблица §1.1) и для PTT: `leaseId`, `leaseState`,
`terminalReason`; у request-`DUPLICATE` ещё `cachedResultCode`. Сравнение точное, включая порядок effects.
Покрыты CT-LC-001…010; CT-LC-004 и CT-LC-009 — только в части host-модели (identity, resync-effect, отказ подделки);
доставка снимков и owner-only транспорт — snapshot oracle и static contract.

Tie-breakers закреплены в REQ@5993d9e (agora #854–#868), запрет пустого списка каналов — в REQ@9e6ce55 (agora #1058), epoch выдачи в `VON_EXPIRY_INTENDED` — в REQ v0.4 @2bf70a7 (agora #1123, трассы 14, шагов 209); таблица — индекс соответствия эталона:

| # | Решение |
|---|---|
| L1 | rate entry удаляется только вместе с live state игрока (disconnect, reconnect, ротация), по времени — никогда (#854 F1) |
| L2 | позиция 11 `LIMIT_REACHED`: `maxPlayers` (новый CONNECTED), нет места под rate entry, `maxActiveLeases` (PTT_START); проверяется до limiter и owner-проверок (#854 F2) |
| L3 | INIT: структурная ошибка (нет ключа, не int32, каталог не canonical sorted) — `MALFORMED`; корректная структура с лимитом `≤ 0` — `NOT_READY` |
| L4 | INIT у готового менеджера проходит observe/sweep, затем сравнивает `(sessionEpoch, config, catalog)` без `nowMs` |
| L5 | CONNECTED без live state (в т.ч. после tombstone, даже с тем же `connectionId`) — новое подключение: `NONE`, `oldGeneration = 0` |
| L6 | ENTITY_BOUND при `CONNECTED_UNREGISTERED` → `NOT_READY` раньше проверки generation (приоритет 3 < 6) |
| L7 | `INACTIVE_ENTITY` применяется только к PTT_START; PTT_STOP разрешён в любом live-состоянии |
| L8 | ENTITY_CHANGED: `from ≠ current` (в т.ч. повтор уже применённой смены) → `STALE_ENTITY`; `from = to = current` → `DUPLICATE` |
| L9 | DISCONNECTED без live state: совпадение tombstone по generation и connectionId → `DUPLICATE`; иначе generation ≤ tombstone → `STALE_GENERATION`; иначе `NOT_READY` |
| L10 | lease, отозванный аномалией времени, получает `terminalAtMs` = `nowMs` этого события |
| L11 | время без переполнения int32: TTL сравниваются как разность (`nowMs − issuedAtMs ≥ ttlMs`, `nowMs − terminalAtMs ≥ terminalLeaseTtlMs`); `expiresAtMs` tombstone = `min(nowMs + tombstoneTtlMs, int.MAX)` (codex #861) |
| L12 | исчерпан диапазон generation или leaseId (следующий ID > int.MAX) — `LIMIT_REACHED` в позиции 11 без mutation, до `ROTATE_EPOCH` (codex #861) |

Проверка: `python -I oracle/lifecycle_oracle.py --check oracle/vectors/lifecycle_v1.json` → `LIFECYCLE_ORACLE_PASS`.
Тесты всех эталонов: `python -m unittest discover -s oracle -p "test_*.py"`.

## Заготовка VoN seam: `vectors/von_seam_v1.json`

Источник требований — `docs/req/REQ-von-seam.md` (codex). Seam stateless: ни истории, ни счётчиков. Валидный вход
всегда даёт `BLOCKED_NO_ACTUATOR` (§3) — это не доставка голоса и не runtime-доказательство.

| Файл | Автор | Что содержит |
|---|---|---|
| `vectors/von_seam_v1.json` | claude | вызовы `apply`/`expire` и ожидаемое решение, выписанные вручную по REQ |
| `von_seam_oracle.py`, `test_von_seam_oracle.py` | claude | исполнимая модель §5–§6 и тесты (stdlib); домен снимка берётся из `snapshot_oracle.is_valid_snapshot` (REQ-core §8.3) |
| `RICON_VoNIntegration` (Enforce) | codex | реализация, независимая от этих файлов |

Случай — `{id, ct[], req[], note, calls[]}`. Вызов `apply`: `snapshot` (поля LinkSnapshot или `null`), `binding`
(§4 или `null`), `upstreamDecision` (имя решения receiver или произвольное число — неизвестное значение), `expect`.
Вызов `expire`: `sessionEpoch`, `playerId`, `deviceId`, `leaseId`, `binding`, `expect`. `"NaN"`/`"Infinity"` в
`quality` — специальные float, как в snapshot-эталоне. Раннер проверяет и то, что вызов не меняет свои входы (§7).

Покрыты CT-VON-S-001…009. CT-VON-S-010 (в коде нет engine/Rpl/file/world API) — сканирование исходника, не вектор.
Тесты дополнительно перебирают все сочетания ошибок и проверяют глобальный приоритет §5 и §6.

Проверка: `python -I oracle/von_seam_oracle.py --check oracle/vectors/von_seam_v1.json` → `VON_SEAM_ORACLE_PASS`.

## Дифракция на рельефе L2: `vectors/l2_terrain_v1.json`

Источник требований — `docs/req/REQ-L2-terrain.md` (claude) по ITU-R P.526-16 §4.5; формулы сверены по изображениям
страниц PDF. Enforce-реализация — codex (спринт 19), независимо от этих файлов.

| Файл | Что содержит |
|---|---|
| `l2_terrain_oracle.py` | исполнимая модель REQ §2–§5: коды отказа, Bullington (49–57), гладкая поверхность (58–64), сферическая Земля §3.2 через §3.1.1 в практических единицах, итог (66) |
| `vectors/l2_terrain_v1.json` | 28 случаев отказа (порядок кодов) и 17 расчётных; у каждого — `provenance`; флаг точности (19) — во всех трёх значениях |
| `test_l2_terrain_oracle.py` | инварианты, точные границы ветвлений, J(ν), поляризация, перебор домена (`L ≥ 0`, нет `NUMERIC_FAILURE`), враждебный ввод |

Числа расчётных случаев посчитаны **отдельной записью формул** (claude, вне репозитория): одна сплошная процедура,
ν — в геометрической форме (26). Это не код oracle. Совпадение с oracle — до 1e-13 дБ. Это `HAND_DERIVED`, а не
пример из стандарта: числовых примеров для §4.5 в P.526-16 нет. `STD_INVARIANT` — соотношения, которые следуют из
формул: на гладком пути `Lba = Lbs` и `L = max(Lsph, Lbs)`; высокие антенны на коротком пути дают `L = 0`.
`CROSS_CHECKED` ставится, когда codex пересчитает вектор своим кодом.

Векторы держат запас `10⁻³` до порогов ветвлений (REQ §6); каждый расчётный случай проверен на устойчивость веток
при возмущении входов на `10⁻⁵`. Точные границы проверяют тесты. `toleranceDb` = 1e-6 — допуск oracle;
`enforceToleranceDb` = 0.05 — допуск parity для Enforce float32.

Проверка: `python -I oracle/l2_terrain_oracle.py --check oracle/vectors/l2_terrain_v1.json` → `L2_TERRAIN_ORACLE_PASS`.

## Физика линии и матрица сценариев: `vectors/link_scenarios_v1.json` (спринт 20)

Независимый эталон для вопроса владельца «где связь есть, на какой дальности и какого качества» (agora #2020).
Автор — claude; карта цепочки TX→ухо в коде RICON и движке — codex (`docs/analysis/LINK_CHAIN_RU.md`).

| Файл | Что содержит |
|---|---|
| `link_physics_oracle.py` | модели потерь `FSPL`, `TWO_RAY_FLAT`, `P526_GENERAL`, шум, бюджет и поиск дальности на сетке (stdlib) |
| `test_link_physics_oracle.py` | якоря в замкнутой форме: константа FSPL, ур. (2) и (3) Френеля, пределы коэффициентов отражения (нормальное и скользящее падение, угол Брюстера, проводник), асимптота 40 lg d, удвоение поля, интерференционный ноль, kT₀, P.372 Table 1, область шума; независимые числа codex (agora #2110, #2132); закрытый домен входа; сверка векторов и порча ожиданий |
| `vectors/link_scenarios_v1.json` | 48 сценариев: 30/50/88/400 МГц × 1/5 Вт × антенны 1.5/3/10 м × ровно/холм; шум `none`/`rural`/`city` |

Модели не склеиваются (agora #2083), каждая считается отдельно:
- `FSPL` — 20 lg(4πd/λ) между антеннами на их реальной высоте (грунт под концом + антенна);
- `TWO_RAY_FLAT` — прямой и отражённый лучи над плоской землёй. Коэффициенты отражения — P.526-16 ур. (100)/(101),
  η = εr − j·18·10⁹·σ/f; формулы сверены по изображению стр. 41. Применяется только до радиогоризонта (ур. 21).
  Разность хода — 4·h1·h2/(r1 + r2), без вычитания близких чисел. В выборке есть `curvaturePhaseErrorRad` —
  сравнение только фазы пути с высотами над касательной (ур. 22): расходимость, угол и R на сфере в нём не
  учтены, так что это не проверка плоской модели и не порог. Сама плоская модель — отдельный reference, не
  истина для production;
- `P526_GENERAL` — FSPL плюс L2 §4.5 на профиле (`l2_terrain_oracle`), то есть тот же метод, что в коде RICON.

Шум складывается из kT₀B и коэффициента шума системы f = fa + fr − 1 (P.372-17 ур. (5)); fa — медиана
техногенного шума (ур. (17), Table 1). Вне 0.3–250 МГц шум среды не определён:
`NOISE_OUT_OF_MODEL_DOMAIN`, так что 400 МГц считается только без шума среды.

Доступность — два отдельных критерия:
- `aboveSensitivity`: Prx ≥ kT₀B + NF + требуемый SNR — так чувствительность определена в RICON, без шума
  среды. Для среды `none` этот критерий совпадает с `meetsSnr`, и тест это проверяет;
- `meetsSnr`: SNR с шумом среды не ниже требуемого. Считается для каждой среды.

Состояния clean/degraded/lost не моделируются (`snrState = DEFERRED`). Вход закрыт:
- неизвестные ключи, модели, поляризации и среды отвергаются;
- пустые или повторные списки отвергаются;
- нечисла и bool отвергаются;
- проверяются домены и сетка;
- `TWO_RAY_FLAT` допускается только на ровном рельефе.

Статус чисел:
- Параметры сценариев — **TEST_ONLY**: мощность, усиления 0 дБи, полоса 25 кГц, NF 10 дБ, грунт ε 15 / σ 0.005,
  порог SNR 10 дБ, треугольный холм 60 м на 2 км. Это не данные радиостанций.
- Разборчивость — `NOT_CALIBRATED`.
- Дальность — `SAMPLED_ESTIMATE`: узлы сетки с отношением 1.001 (FSPL, two-ray) или 1.01 (P526). Провал между
  узлами может быть пропущен; тест показывает такой ноль.
- Ожидания — `ORACLE_SNAPSHOT` (снимок эталона для регресса и перекрёстной проверки), а не независимый расчёт.
  Независимость дают якоря в тестах и пересчёт выборочных точек codex (будет помечен `CROSS_CHECKED`).

Не моделируются: поверхностная волна (P.368), рассеяние на растительности и застройке, рефракция кроме k-фактора,
многократные отражения. Сотовая связь — в спринтах 21+.

Проверка: `python -I oracle/link_physics_oracle.py --check oracle/vectors/link_scenarios_v1.json` → `LINK_PHYSICS_ORACLE_PASS`.
