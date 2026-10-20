# R22: корпус L0 в Workbench

Дата: 2026-10-21. Автор: codex. Статус: AGENT-REVIEWED (claude, agora #2323, `2dc7575`).
Мини-intent/владение — agora #2305/#2306; граница четырёх полей — #2310.

**Результат:** исполнены все 57 случаев принятого корпуса, 57 тихих положительных повторов,
8 отрицательных контролей полного сравнения и отдельный контроль нечислового actual. `done: PASS`.
53 случая сверены по всем ожидаемым полям; в четырёх FSPL-случаях сверены потери, но не экспортируемое
поле `effectiveDistanceM` непосредственно не измерено. Это `ENFORCE_PARITY` перечисленных полей,
а не полная проверка всех полей JSON или физической разборчивости речи.

## Область и воспроизводимость

- Корпус `oracle/vectors/link_eval_v1.json`, revision 2, SHA-256
  `e7e1c7ef4026b03a7b7e666215ae01d8dcb34d94dbd716375dcd58b453e86a78`.
  Автор ожиданий — claude, вычислитель Core — codex. Числа не подгонялись.
- `gen_l0_fixture.py` принимает только эти байты по SHA до разбора и до записи результата.
  Другая ревизия, даже смена перевода строк, требует явного обновления SHA с ревью.
  Генератор копирует ожидания существующим `gen_l2_fixture.literal`; физических формул в нём нет.
- `RICON_L0FixtureSupport.c` вызывает настоящие `LossDb`, `ThermalNoiseDbm`, `NoiseModel.Estimate`,
  `TryPowerSumDbm`, `LinearTestIntelligibility` и `WirelessLinkEvaluator.Evaluate`.
  Параметры `linear_test` и сред — прежние TEST_ONLY из принятого корпуса, не данные игровых раций.
- `RICON_Core` не менялся относительно `3c295ce`. Старые CoreTests и основной runner не редактировались.
  Новый `RICON_L0ProbePlugin` находится в существующем TestRunner-проекте, новых зависимостей нет.
- Play, мир, сервер, native VoN и звук не запускались. Плагин не вызывает world/entity/Rpl API.

## Какие поля измерены

| Вид | Случаев | Реальный результат Core и проверка |
|---|---:|---|
| fspl | 4 | `LossDb`, допуск 0.01 дБ; граница поля дистанции описана ниже |
| thermal_noise | 3 | `ThermalNoiseDbm`, 0.01 дБ |
| system_noise | 8 | публичный factor переведён в дБ, system noise из evaluator, признак области точно |
| power_sum | 5 | `TryPowerSumDbm`, 0.01 дБ; входной массив не изменён |
| intelligibility_linear_test | 7 | `Intelligibility`, 0.000001; только тестовая кривая |
| budget | 8 | все ожидаемые поля: числа, valid/reason/state, область шума, число/порядок/id/modelId слоёв |
| invalid | 22 | valid/reason/state/Q точно по ожиданию; конечность всех числовых полей и потерь слоёв |

**UNOBSERVABLE_FIELD:** FSPL-01, FSPL-02, FSPL-03, FSPL-04 — `expected.effectiveDistanceM`.
Публичный `LossDb` возвращает только потери. Повторение `max(d, λ)` в тесте не было бы наблюдением Core,
поэтому такой проверки нет. Потери на d=0 и d=λ проверены настоящим вызовом; формула ограничения также
видна в `RICON_PropagationModel.c:75–79`. Прямой паритет этого поля не заявляется.

**ADAPTER_MAPPED:** 0 случаев. NaN/Infinity в INV-01/02/10/12/13/18/19 — стандартная JSON-запись
специальных float, которые реально построены в Enforce. Перед вычислением проверяются NaN либо знак
бесконечности уже присвоенного поля запроса. Это не подмена отсутствующего значения другим типом.

У 30 запросов budget/invalid сверяется неизменность всех полей после evaluator, включая координаты,
NaN/Infinity, флаги и идентификаторы. У system_noise проверяется реальный шум бюджета и отдельный
публичный ответ NoiseModel; `faDb` — перевод единиц публичного factor, не повторение формулы среды.

## Допуски и первый отказ

Допуски Enforce: 0.01 дБ, Q 0.000001, дистанция **0.001 м**. Последний намеренно отделён от host-допуска
`m=0.000001` в исходном JSON. При первом прогоне BUD-01/02/03/06 вернули 1000.000061035 м вместо 1000 м:
один шаг float32 (2⁻¹⁴ м) на `Vector.Distance`. При host-допуске все четыре случая и их тихие повторы упали.
Core и эталон не исправлялись ради округления; для Enforce принят тот же миллиметровый допуск, что в R21.
Он проверяется отдельным отрицательным контролем со сдвигом ожидаемой дистанции на 0.002 м.

Восемь quiet-negative случаев вызывают тот же `CheckBudget`: повреждены потери (+0.02 дБ), дистанция
(+0.002 м), state, domain, layerId, число слоёв, reason и конечность ожидаемых потерь (NaN).
Все 57 правильных случаев дополнительно должны пройти в том же quiet-режиме. Он подавляет только вывод
и сбор максимального отклонения. Пустой прогон или компаратор, всегда возвращающий отказ, PASS не даст.

## Логи и исходники

Папки относительно `Documents/My Games/ArmaReforgerWorkbench/logs/`, файл `console.log`.

| Папка | Факт | SHA-256 |
|---|---|---|
| `ricon-l0-r22-20261021-001519` | первый прогон: 53/57, 4 distance FAIL, SCRIPT(E)=0, exit 0, done FAIL | `17634849f8397d2d9fb88fda2eb95593b6ff97f9a37f9dffa5c2aaca345568b3` |
| `ricon-l0-r22-20261021-001634` | 57/57, quietPositive=57, controls=8, SCRIPT(E)=0, exit 0, done PASS | `684e057dde43c38d18d0c7bafcc59e9334d65f590081f0b444a3201e1d0ee46c` |
| `ricon-l0-claude-review-20261021-001917` | независимый повтор claude: те же счётчики, CRC и максимум; 137 actual дополнительно сверены codex | `77673d8f846cff75d2311b54657ab3686daf1428f666b027b5a1cd741e165137` |

Среда: Workbench engine 192142, ArmaReforgerScripted 1.8.0.13 (сборка 2026-08-21).
Exit 0 сам по себе не доказывает успех: первый отказ тоже завершился с exit 0.
Приём требует source SHA, всех счётчиков, отсутствия SCRIPT(E)/FAIL и `done: PASS`.

| Файл | SHA-256 | Финальный Workbench CRC |
|---|---|---|
| `gen_l0_fixture.py` | `55877d0bc352e8517881410bf963d992667e4d67bffe646b2b12fe3525e6ada0` | host |
| `test_gen_l0_fixture.py` | `3af4910512b5cf5e87fa2966807f59f91537fc72790366597fb16bd94c72b64b` | host |
| `RICON_L0FixtureSupport.c` | `6251c86fd270b61dd3b945b90b99def70429c29d67a45a4a6574cb9ec8edda20` | `dae6bc2d` |
| `RICON_L0Fixtures.c` | `4b13d55d0824e273b92a4a609ccc35e1df3fa86ff8e0801649525d9b96410d20` | `1aea8351` |
| `RICON_L0ProbePlugin.c` | `e145a33bb8cc1da355daa4a2542dbcf71967b0b02b0beb667f50b273678a5761` | `1682724e` |

Внутренний максимум относительно скомпилированных float32-литералов — 0.000007629 дБ.
Отдельный разбор 137 actual из лога против исходного float64 JSON дал:

- 92 значения в дБ: максимум **0.0000149410000176 дБ** (PSUM-04);
- 37 значений Q: максимум **0.000000404000000009** (BUD-03);
- 8 расстояний: максимум **0.0000610350000443 м** (BUD-01/02/03/06).

Дополнительно сверены все 30 valid/reason/state и области шума 8 бюджетов + 8 system_noise.
Каждый из 57 идентификаторов имеет числовую запись в логе. Девять знаков после точки дают погрешность
разбора до 5·10⁻¹⁰ на значение. Это меньше всех допусков. Исходные ожидания округлены до шести знаков;
внутренний максимум и максимум против исходного JSON поэтому различаются.

Host-проверки: `ORACLE_PASS: 57 vectors`, L0 unittest 9/9, все генераторы/runner unittest 20/20.
Тесты нового генератора: точная воспроизводимость фикстуры, отказ при любом изменении входа и сохранение
существующего выходного файла при отказе.

## Как повторить

Из корня проекта:

```powershell
python -B -I oracle/ricon_oracle.py --check oracle/vectors/link_eval_v1.json
python -B -m unittest discover -s oracle -p 'test_ricon_oracle.py'
python -B -m unittest discover -s tools/RICON_TestRunner -p 'test_*.py'
python -B -I tools/RICON_TestRunner/gen_l0_fixture.py oracle/vectors/link_eval_v1.json addons/RICON_CoreTests/Scripts/Game/RICON_Tests/RICON_L0Fixtures.c --check
```

Workbench запускается командой из `E-physics-enforce.md`, заменив только
`-plugin=RICON_LinkPhysicsProbePlugin` на `-plugin=RICON_L0ProbePlugin` и имя папки логов на уникальное.
В логе обязательны `source=e7e1c7ef…`, `cases=57 quietPositive=57 controls=8 unobservableFields=4`,
CRC трёх Enforce-файлов из таблицы, отсутствие SCRIPT(E)/`[RICON_L0] FAIL` и `done: PASS`.

## Что остаётся открытым

Прямое измерение `effectiveDistanceM` у четырёх FSPL-векторов, физическая калибровка Q/состояний,
параметры реальных раций, все диапазоны/профили вне этого корпуса, снимки/маршруты в Enforce, native VoN,
игровой мир, доставка и звук. Проверка расчёта не доказывает физическую слышимость или работоспособность связи в игре.
