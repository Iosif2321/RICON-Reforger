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
| `vectors/link_scenarios_v1.json` | 48 сценариев: 30/50/88/400 МГц × 1/5 Вт × антенны 1.5/3/10 м × ровно/холм; шум `none`/`rural`/`city`. Rev 3: у каждой точки `samples` — потери, Prx, `aboveSensitivity` и по каждой среде SNR и `meetsSnr` |

Модели не склеиваются (agora #2083), каждая считается отдельно:
- `FSPL` — 20 lg(4πd/λ) между антеннами на их реальной высоте (грунт под концом + антенна);
- `TWO_RAY_FLAT` — прямой и отражённый лучи над плоской землёй. Коэффициенты отражения — P.526-16 ур. (100)/(101),
  η = εr − j·18·10⁹·σ/f; формулы сверены по изображению стр. 41. Применяется только до радиогоризонта (ур. 21).
  Разность хода — 4·h1·h2/(r1 + r2), без вычитания близких чисел. В выборке есть `curvaturePhaseErrorRad` —
  сравнение только фазы пути с высотами над касательной (ур. 22): расходимость, угол и R на сфере в нём не
  учтены, так что это не проверка плоской модели и не порог. Сама плоская модель — отдельный reference, не
  истина для production;
- `P526_GENERAL` — FSPL плюс L2 §4.5 на профиле (`l2_terrain_oracle`), то есть тот же метод, что в коде RICON.

Неудачная попытка, вне матрицы — `knife_edge_4ray`. Задумывалась как физическая проверка §4.5 для холма (D-002).
  - Устройство: вершина холма — острый клин над плоской землёй, с отражением по обе стороны. Это 4 пары
    «источник/приёмник или их зеркальные образы».
  - У каждой пары свой ν (ур. 26); комплексное поле Френеля–Кирхгофа суммируется когерентно. Модуль поля
    совпадает с J(ν) по ур. (30); ур. (31) отличается от него не больше чем на 0.124 дБ (тест).
  - Интегралы Френеля: ряд при x ≤ 2, дальше цепная дробь erfc (Abramowitz & Stegun 7.1.14). Совпадение с
    таблицами A&S — 10 знаков.
  - Острый клин теряет меньше округлого холма, поэтому выигрыш от препятствия в этой модели — оценка сверху.
  - Статус **RESEARCH** (agora #2203). Способ сложения зеркальных путей взят не из проверенного первоисточника.
    Предел «без препятствия» не выполняется на земле с потерями при низких антеннах:
    - клин высотой 1 см меняет потери на 7.5 дБ против two-ray (3 м, 50 МГц, 4 км);
    - клин нулевой высоты посередине — на 12.03 дБ (1.5 м, V, 1 км; codex #2214);
    - при антеннах 200 м предел выполняется в пределах 1 дБ.
    Всё это закреплено тестами. В сценарии и выводы модель не входит, D-002 остаётся OPEN (agora #2214).

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

### Точки для Enforce-проверки: `vectors/link_probe_points_v1.json` (спринт 21)

Семь точек, на которых codex исполняет Enforce `RICON_WirelessLinkEvaluator` (agora #2201, #2206):
- P01–P06 — из сценариев rev 3: ровный путь на 3 и 10 км; до холма и за ним; приёмник на вершине холма;
  400 МГц вне области модели шума;
- P07 — пример D-004.

Ожидания (`req_budget`) написаны claude по тексту REQ-LE-40…75, независимо от `RICON_LinkEvaluator.c`:
- слои FSPL (d_eff = max(d, λ)) и рельеф §4.5;
- Prx, kT₀B, шум (вне области f_a = 1 и `noiseModelInDomain = false`), SINR, чувствительность;
- Q и состояние — по фикстуре `linear_test`. Это **TEST_ONLY**: проверка ветки кода, а не модель речи.

Тест сверяет часть «свободное пространство + шум + чувствительность» каждой точки с L0-эталоном codex
(`ricon_oracle.evaluate_budget`).

Проверка: `python -I oracle/link_physics_oracle.py --check oracle/vectors/link_probe_points_v1.json` → `LINK_PHYSICS_ORACLE_PASS`.

## Сотовая связь: `vectors/cellular_scenarios_v1.json` (спринт 22)

Эталон SINR и хендовера для фокуса владельца (agora #2020, п. 2). Автор — claude; проверка формулы A3 и выборочных
векторов — codex.

| Файл | Что содержит |
|---|---|
| `cellular_oracle.py` | мощность от каждой соты (`link_physics_oracle`: P.526-16 §4.5 на ровном профиле), SINR с помехами сот на том же канале, хендовер по событию A3 с TimeToTrigger, счёт ping-pong |
| `test_cellular_oracle.py` | условие A3 (строгое, со смещениями); набор TimeToTrigger; сумма мощностей; погрешность интерполяции таблицы потерь; хендовер в середине между равными сотами; задержка ровно v·TTT; положение при Hys; помехи при повторе частот 1 и 3; шум вне области на 900 МГц; ping-pong и его подавление; закрытый вход; векторы |
| `vectors/cellular_scenarios_v1.json` | 48 сценариев: 3 соты вдоль дороги через 5 км, 900 МГц × скорость 15/30 м/с × повтор частот 1/3 × ошибка измерения 0/4 дБ × Hys 0/3 дБ × TTT 0/320/1280 мс |

Первоисточник — 3GPP TS 36.331 V19.4.0 §5.5.4.4 (`STD`, `docs/evidence/E-web2-standards-cellular.md:129–146`):
вход A3 `Mn + Ofn + Ocn − Hys > Mp + Ofp + Ocp + Off`, условие держится TimeToTrigger.

Допущения (они же записаны в файле векторов, поле `assumptions`):
- По стандарту A3 запускает только отчёт об измерениях. Хендовер в момент срабатывания — допущение игровой модели.
- Измерение — абстрактный TEST_ONLY прокси (полная мощность в дБм), а не RSRP.
- Ping-pong — A→B, затем B→A не позже `pingPongWindowS`. Определения в 3GPP нет.
- Ошибка измерения — независимая гауссова с seed, TEST_ONLY. Это не модель замираний.
- Потери от 10 м интерполируются на сетке 1 м: наибольшее отклонение по выборке 100 точек 10…10 000 м ≤ 0.005 дБ (тест, не глобальная граница). Ближе 10 м считается сама модель; FSPL — с d_eff = max(d, λ) (REQ-LE-41).

Что видно в матрице, при тестовых параметрах:
- При ошибке 4 дБ без Hys и TTT — до 1530 хендоверов за проезд, почти все ping-pong.
- TTT 320 мс подавляет их полностью.
- Длинный TTT (1280 мс) задерживает хендовер. Доля отсчётов (оба конца включены) с SINR ≥ 0 дБ падает с 0.96 до 0.87–0.89, минимальный
  SINR опускается до −8…−13 дБ.
- При повторе частот 1 у границы сот SINR около 0 дБ (помеха равна сигналу). При повторе 3 помех нет.
- На 900 МГц шум среды не определён (P.372-17 — до 250 МГц), берётся опорный фон kT₀B + NF (как D-005).

Проверка: `python -I oracle/cellular_oracle.py --check oracle/vectors/cellular_scenarios_v1.json` → `CELLULAR_ORACLE_PASS`.

## Чувствительность: `vectors/sensitivity_v1.json` (спринт 27)

`sensitivity_oracle.py` (claude) — без новой физики распространения: считает `link_physics_oracle` и
`l2_terrain_oracle` при других грунте, поляризации и шаге профиля. agora #2633, #2636.

- **Грунт.** ITU-R P.527-6 §5.2, модель почвы, ур. (57)–(70) и вода (6)–(13); σ по ур. (3a).
  - Сверена с рисунками 14–16 стандарта при 0.1, 1 и 10 ГГц, чтение на глаз, допуск 2 % шкалы.
  - Ниже 100 МГц сверки нет: `BELOW_FIGURE_RANGE`.
  - Классические грунты B, D и E с Fig. 24 — только для сценариев 30 МГц.
  - Источник — `docs/evidence/E-web6-p527-ground.md`.
- **`groundSweep`.** 48 сценариев матрицы × (грунт матрицы + 3 почвы P.527-6, на 30 МГц ещё 3 класса Fig. 24)
  × 2 поляризации. Для каждого: модели TWO_RAY_FLAT и P526_GENERAL, внешние границы по `aboveSensitivity` и SNR
  в селе, потери в точках `sampleDistancesM`.
- **`stepLoss`.** Треугольный холм 60 м на 2 км, путь 10 км, полуширина 0.5 / 0.1 / 0.03 км, 30 и 400 МГц,
  антенны 1.5 и 10 м.
  - Шаги 0.01–1 км, вершина в 8 положениях внутри шага.
  - Берётся худшее |ΔL| против опоры 2 м (нижняя граница REQ-L2); сходимость опоры — против 2.5 м.
- **`stepRange`.** Граница §4.5 в 4 сценариях с холмом при шагах 0.01–1 км. Это одно положение вершины на
  дистанцию, не худший случай.
- Опорный грунт с вертикальной поляризацией обязан воспроизводить границы `link_scenarios_v1.json` rev 3 (тест).

Проверка:
- `python oracle/sensitivity_oracle.py` — полный пересчёт и сравнение, около 5 мин, `SENSITIVITY_ORACLE_PASS`;
- `--write` перезаписывает вектор.

Все формы, высоты и радиопараметры — TEST_ONLY. Шаг не рекомендуется, выбор грунта для карт — OPEN (D-008).

**Опорные точки для Enforce (спринт 28): `vectors/l2_sensitivity_anchors_v1.json`.** 16 точек L2 для probe codex
(agora #2706, #2737, #2758):
- A01–A08 — грунт и поляризация на ровной земле;
- A09–A16 — шаг и фаза на треугольном холме: вершина на узле, пропущенная вершина, широкая форма, опорный шаг 2 м.

Вход: параметры треугольника и полный конфиг. Профиль строится по `provider_sampling`. В ожиданиях — L, Lba, Lbs,
Lsph, ветки, `profilePointCount` и `peakTerrainM`. Допуск 0.05 дБ, как `enforceToleranceDb` в `l2_terrain_v1`.
Проверка `python oracle/sensitivity_oracle.py --anchors` — секунды, `--anchors --write` перезаписывает файл.

## Профиль из мира: `vectors/world_profile_v1.json` (оболочка S1, agora #2946)

Эталон `world_profile_oracle.py` по `docs/req/REQ-L2-world-profile.md` v0.2: поставщик профиля строит сетку
REQ-L2 §6 из абстрактного источника высот. Источник — синтетическая формула (плоскость и гауссовы холмы),
отказ задаётся номером обращения (`failAtCall`, `nanAtCall`). Вода — NOT_MODELLED, мир не читается.

- `level = "model"` (W01–W22) — модель целиком: причина, слои `fspl` и `terrain_diffraction`, профиль, `htg`,
  `hrg`, число обращений к источнику;
- `level = "provider"` (P01–P17) — прямой вызов поставщика на границах входа: null query/config/источник,
  невалидный config, нечисловые `x`, `y`, `z` у обоих концов (в JSON строками `"NaN"`, `"+Inf"`, `"-Inf"`).

Проверка: `python oracle/world_profile_oracle.py --check oracle/vectors/world_profile_v1.json` →
`WORLD_PROFILE_ORACLE_PASS`; `--write` перезаписывает файл. Все высоты и параметры — TEST_ONLY.

## Служба пар: `vectors/pair_service_v1.json` (S3, agora #3172)

Эталон `pair_service_oracle.py` по `docs/req/REQ-pair-service.md` v0.2. Один вызов: активные передачи
(binding DTO) × слушатели. Сначала отказ всего вызова (C01–C30), затем код каждой пары по нормативному порядку
(B01–B27), сборка запроса S2 (`radio_device_oracle.evaluate`, S01–S09) и бюджет свободного пространства
(`ricon_oracle.evaluate_budget`, E01–E04). P01 — базовая сцена 2×3, P02 — та же сцена с переставленными массивами.

В ожидании каждого случая — код вызова, все пары по ключу `(leaseId, rxDeviceId, rxTransceiverIndex, rxPlayerId)`
и счётчики вызовов S2 и evaluator. Допуск Enforce: SINR 0.05 дБ, Q 0.002; состояния выбраны далеко от порогов.

Проверка: `python oracle/pair_service_oracle.py oracle/vectors/pair_service_v1.json --check` →
`PAIR_SERVICE_ORACLE_PASS`; `--write` перезаписывает файл. Все параметры раций — TEST_ONLY.

Rev1 заморожен 2026-10-21 (codex #3201), SHA-256 `fcc9cefa1b8bb46374b67883c239ffc8c6c00343da49a6bf008cf679cac178e1`.
Независимый пересчёт codex из REQ: 72/72 (#3200). Любое изменение — только rev2 по взаимному согласию.

## Формирование снимков: `vectors/link_publication_v1.json` (S3b, DRAFT)

Эталон `link_publication_oracle.py` по `docs/req/REQ-link-publication.md` v0.2. Вход: результат одного такта S3,
bindings того же такта, предыдущий реестр и TEST_ONLY источник seq. Выход: снимки и новая версия реестра,
реальные счётчики source, необходимость full resync. Доставка по сети и управление звуком отсутствуют.

106 случаев: первый/повторный такт, порог Q и TTL, все девять кодов пары со старой связью и без неё,
завершающий LOST, смена lease/epoch, домены, порядок отказов, лимиты, частичный отказ source и переполнение.
Входы не меняются. Unit-тесты дополнительно проходят цепочки тактов и вход S3 из замороженного корпуса.

Проверки:
- `python oracle/link_publication_oracle.py --check oracle/vectors/link_publication_v1.json`;
- `python -m unittest discover -s oracle -p test_link_publication_oracle.py`.

Корпус независимо пересчитан claude (`8e5d34a`): 106/106 без чтения эталона, codex повторил checker.
Freeze согласован в Agora #3343, SHA-256 `a60ed14590222c103aceea9030fb80ee233eea432b5969b8ed7fe93ad5c10be3`.
Q сравнивается с допуском 1e−6 только в компараторе Enforce; входы и разность Q округляются к float32,
ветвление строгое. Q05/Q06 отдельно проверяют округление разности и десятичных JSON-литералов.

## Отсечка полезного вклада: `vectors/channel_cutoff_v1.json` (P1)

`channel_cutoff_oracle.py` по `docs/req/REQ-channel-cutoff.md`: чистый Assess строгих границ конечного
вклада всех возможных составляющих. Безопасная амплитудная сумма учитывает неизвестные фазы.
Неизвестное покрытие/ресурс/истёкший интервал не дают физический LOST; отсутствие границы стохастического
хвоста запрещает отсечку. Допуск не является чувствительностью приёмника. Вход не меняется.

27 случаев независимо пересчитаны claude (`91d50fa`), codex повторил checker. SHA-256
`c5e7e83460b8008f4fed08ce1669e2f95287f34e1e892cd437678a11ed983684`.
Числа — TEST_ONLY, производственный численный запас не выбран. Реальные модели мира, Enforce и
экономия вычислений этим эталоном не подтверждены. Временной контракт P1b — `REQ-channel-time.md`.

Проверки: `python -B oracle/channel_cutoff_oracle.py --check oracle/vectors/channel_cutoff_v1.json` и
`python -B -m unittest discover -s oracle -p test_channel_cutoff_oracle.py`.
