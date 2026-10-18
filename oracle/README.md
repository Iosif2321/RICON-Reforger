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

- Допуски: величины в дБ/дБм — `db`; разборчивость и доли — `ratio`; расстояния в м — `m`.
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
| `invalid` | `LinkQuery` с одним нарушением | `valid = false`, `reason`, `state = LOST`, `intelligibility = 0`, `allFinite = true` | REQ-LE-80, 81 |

Категории внешнего шума (`noiseEnvironmentId`): `city`, `residential`, `rural`, `quiet_rural`, `none`.

## Статус чисел

- Разборчивость и пороги состояний во всех векторах `budget` — фикстура `linear_test` (**TEST_ONLY**, не модель
  речи; production-кривая — `BLOCKED_CALIBRATION`, REQ-LE-74). Векторы проверяют механику seam, не реализм.
- Константы внешнего шума — ITU-R P.372-11 по вторичному источнику (REF); редакция P.372 не подтверждена.
- Эти векторы дают только `ORACLE_PASS`. Паритет Enforce ↔ oracle (`ENFORCE_PARITY`) требует исполнения
  Enforce-кода и сейчас `NOT_TESTED`.
