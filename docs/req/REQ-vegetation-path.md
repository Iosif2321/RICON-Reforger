# P4.3 — выбор полного бюджета «сквозь лес» или «над кронами»

Версия **0.4 AGENT-REVIEWED**, 2026-10-23. Freeze (agora #3698, #3700): корпус `vegetation_path_v1.json`, 31 случай,
LF SHA-256 `d9e3b7e7d01c8149b090e3e7642917e542f994632f18b5f54aaa12e6ca0bab51`; слепой пересчёт codex 31/31, 17 контролей
(cab0d79). Enforce — codex. v0.4 по codex #3695: сравнение в binary32, итог — выбранная по правилу полосы ветвь,
без универсальной гарантии. Ранее v0.3 по codex #3692: нормативная binary32-арифметика решения и полоса равенства
0.001 дБ (VP06a), случаи P30/P31. Ранее v0.2: v0.2 по codex #3685: статус `OVER_REFERENCE_ONLY` вместо
`BOUNDED_BY_OVER`, проверка конечности для NO_VEGETATION, точный случай равенства. Автор claude, независимая проверка codex. Источник истины — этот репозиторий.

Основание:
- REQ-vegetation v0.4 (P4 v1, корпус `576fed29…ca9`) и REQ-vegetation-overcanopy v0.1 (P4.2, корпус `00d7da2…9cf2`) —
  оба заморожены, этот срез их не меняет;
- ECAC §2.6.2/§3.6 и P.833 ур. (7): потери «сквозь» сравниваются с потерями «поверх», берётся меньшая;
- P.833-10 §2.1: наибольшее затухание Am ограничено распространением над вершинами растительности;
- codex #3637 (опорная высота max(h, R)), #3672 (не подменять отсутствующую ветвь нулём; ASSUMED_EXTENSION не
  закрывает ячейку);
- владелец: закрыть все сценарии, которые одна модель не покрывает.

## 1. Назначение и границы

**VP01.** Чистая функция выбирает между двумя **полными** бюджетами одного пути и говорит, насколько выбор
обоснован. Она не считает L0/L2 сама: базовые потери обеих геометрий приходят числами от существующих вычислителей
(FSPL `RICON_FreeSpaceModel`, L2 `RICON_TerrainDiffractionCalculator.Evaluate` с высотами над землёй). Подключение к
`RICON_WirelessLinkEvaluator`, мир и голос — следующий срез.

**VP02.** Слой вызывает замороженные P4 v1 (`vegetation_layer`) и P4.2 (`over_canopy`) на одном и том же входе.
Ансамбль M1–M5 и поправка M6 не меняются.

## 2. Вход

**VP03.** Корень — ровно `{observation, catalog, polarization, txHeightM, rxHeightM, canopy, budget}`; первые шесть —
как в P4.2 (OC05). `budget` — ровно:

| Поле | Смысл | Домен |
|---|---|---|
| fsplDb | FSPL для реальных положений антенн | binary32 > 0 |
| l2Db | L2 (P.526) для реальных высот над землёй | binary32 ≥ 0 или null — не посчитан |
| fsplOverDb | FSPL для опорной геометрии | binary32 > 0 |
| l2OverDb | L2 для опорных высот | binary32 ≥ 0 или null |

Опорная высота терминала в лесу — `max(h, R)` (OC03); терминал вне леса остаётся на h. FSPL опорной геометрии
считается заново по расстоянию между опорными точками. Это отдельное поле, потому что текущий слой `fspl` берёт
3D-расстояние из положений запроса.

## 3. Ветви

**VP04. Ветвь «сквозь».** Есть, если P4 v1 вернул OK.
- Если хотя бы у одного интервала `excessBaseline = FREE_SPACE` (J&B): `base = fsplDb`, `budgetMode =
  FREE_SPACE_PLUS_JB`. L2 этого пути не прибавляется (REQ-vegetation VG12).
- Иначе `base = fsplDb + l2Db`, `budgetMode = BASE_PLUS_EXCESS`; при `l2Db = null` ветви нет,
  reason `BASE_BUDGET_MISSING`.
- `totalDb = base + lossDb` P4 v1, `closed = allScenariosClosed`.
- P4 v1 `NO_MODEL_FOR_SCENARIO` → ветви нет, reason `NO_MODEL_FOR_SCENARIO`. Отсутствующая ветвь — не 0.

**VP05. Ветвь «поверх».** Есть, если:
- нет растительности посреди пути (объединённый интервал, не касающийся ни одного конца). Иначе reason
  `MID_PATH_VEGETATION`: путь над массивом посреди трассы — срез дифракции над кронами, не P.2108;
- P4.2 вернул OK; иначе reason — код P4.2 (`OUT_OF_MODEL_DOMAIN`, `NO_TERMINAL_IN_VEGETATION`);
- `l2OverDb` не null; иначе reason `BASE_BUDGET_MISSING`.

Тогда `base = fsplOverDb + l2OverDb`, `canopyDb = totalAhDb` P4.2, `totalDb = base + canopyDb`,
`closed = (status P4.2 = SOURCE_DOMAIN)`.

## 4. Выбор и статус

**VP06.**
- Обе ветви есть — выбор по правилу полосы VP06a: THROUGH, если она не дороже OVER больше чем на 0.001 дБ, иначе OVER.
- Одна ветвь — она.
- Ни одной — `UNRESOLVED`, reason `NO_BRANCH`, с описанием обеих отсутствующих ветвей.

Итог — полная потеря выбранной ветви. Она может превышать минимум не больше чем на 0.001 дБ. Ветви никогда не
складываются.

**VP06a. Арифметика решения.** Числа бюджета уже binary32 (VP03). `lossDb` P4 v1 и `totalAhDb` P4.2 при входе в
P4.3 приводятся к binary32 (ядра P4 и P4.2 не меняются). Каждая сумма base и totalDb округляется к binary32
отдельно, в порядке `base = f32(fspl + l2)`, `total = f32(base + loss)`. «Сквозь» выбирается, если
`f32(T − O) ≤ f32(0.001)`: и разность, и порог — binary32, одинаково в эталоне и native. Полоса равна native-допуску
P4 и P4.2 и снижает чувствительность выбора к их расхождению. Одинаковый выбор у края полосы во всех допустимых
float-реализациях она **не гарантирует**; такой гарантии слой не заявляет. Корпус не содержит случаев на краю:
|f32(T − O) − 0.001| > 0.0009 дБ. Переполнение
binary32 (результат > FLT_MAX после округления) → `NUMERIC_FAILURE`.

Статус обоснованности:
- `CLOSED` — обе ветви есть и обе закрыты: минимум из двух обоснованных полных потерь;
- `OVER_REFERENCE_ONLY` — ветвь «поверх» есть и закрыта, а «сквозь» отсутствует или не закрыта (например, только
  LIN-γ). Итог по-прежнему минимум присутствующих ветвей и может прийти из незакрытой «сквозь» (P03). Закрытая
  `over.totalDb` — лишь инженерная опорная оценка: P.2108 §3.1 даёт медиану, P.833 §2.1 говорит, что Am
  ограничено путём над вершинами. Это **не** граница результата, не строгая граница P1 (CT07) и не CLOSED
  (codex #3685);
- `OPEN` — иначе.

Обязательная ячейка VG10a закрыта только при `CLOSED`. Даже `CLOSED` — минимум двух медианных инженерных
оценок, а не гарантия мгновенного канала: сложение фаз и замирания — P1b. Принимать ли `OVER_REFERENCE_ONLY`
для приёмки — решение владельца. Ни один статус не даёт LOST и строгих границ P1.

## 5. Порядок причин

**VP07.**
1. Лишнее или пропущенное поле корня → `INVALID_INPUT`.
2. Коды P4 v1 `INVALID_OBSERVATION`, `INVALID_INPUT`, `VEGETATION_COVERAGE_INCOMPLETE`, `VEGETATION_UNKNOWN`,
   `VEGETATION_MIXED_PROFILE` передаются как есть; затем те же коды P4.2 (неверный canopy → `INVALID_INPUT`).
3. Неверный `budget` → `INVALID_INPUT`.
4. `NUMERIC_FAILURE` P4 v1 или P4.2 → `NUMERIC_FAILURE`.
5. P4 v1 `NO_VEGETATION` → `NO_VEGETATION` с `totalLossDb = fsplDb + l2Db`; при `l2Db = null` — `UNRESOLVED`,
   reason `BASE_BUDGET_MISSING`; сумма — не конечный binary32 ≤ FLT_MAX → `NUMERIC_FAILURE` (P29).
6. Ветви VP04/VP05; любое значение base или totalDb — не конечный binary32 ≤ FLT_MAX → `NUMERIC_FAILURE`.
7. VP06: `UNRESOLVED` или `OK`.

При OK: `selected`, `totalLossDb`, `status`, `through` и `over`. Каждая ветвь — `{present, reason}`, а при present
ещё `baseDb`, `totalDb`, `closed`; у «сквозь» ещё `vegetationDb` и `budgetMode`, у «поверх» — `canopyDb`.

## 6. Проверка и передача

**VP08.** Файлы:
- эталон `oracle/vegetation_path_oracle.py`;
- сборщик `oracle/build_vegetation_path_vectors.py`;
- корпус `oracle/vectors/vegetation_path_v1.json` — 31 случай P01–P31, host-only P18 и P27 (лишние поля JSON);
  P28 — точное равенство (LIN-γ 64·f32(0.02) против той же суммы при A_h = 0), P29 — переполнение без растительности,
  P30 — близкое равенство codex #3692 (в binary64 OVER, в binary32 равенство → THROUGH), P31 — за полосой → OVER;
- unit-тесты `oracle/test_vegetation_path_oracle.py` (7): корпус и LF; все коды, статусы, ветви и причины;
  итог = выбранная по полосе ветвь (не дороже минимума больше 0.001 дБ), не сумма; правило статуса; целевые ячейки (P02 глубокий лес → OVER_REFERENCE_ONLY; P28 равенство → THROUGH; P29;
  P04 J&B без L2); порча ловится; вход не меняется.

Базовые бюджеты в корпусе — синтетические TEST_ONLY числа. Native: коды, выбор и статус строго, числа —
max(1e-3 дБ, 1e-5·|x|).

Команды:
- `python oracle/vegetation_path_oracle.py --check oracle/vectors/vegetation_path_v1.json`;
- `python -m unittest discover -s oracle -p test_vegetation_path_oracle.py`.

Сначала codex проверяет REQ и пересчитывает корпус своим checker, не читая эталон; затем freeze и Enforce codex.

Не сделано:
- вызов L2 для опорной геометрии и пересчёт FSPL — интеграция;
- путь над массивом посреди трассы;
- кроны мира;
- решение владельца о приёмке `OVER_REFERENCE_ONLY`.
