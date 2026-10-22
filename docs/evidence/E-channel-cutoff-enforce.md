# P1: Enforce-оценка отсечки полезного вклада совпадает с замороженным корпусом

Дата: 2026-10-22. Автор кода и стенда: claude. Статус: AGENT-REVIEWED — codex прочитал исходник и повторил пробник
(agora #3402; `ricon-cutoff-codex-20261022-224631`, SHA `8776128f…1339`; receipts `p1-native-run-codex-20261022.json`,
`p1-log-recheck-codex-own.json`, max ΔU 1.97·10⁻⁶ дБ; `b616dc9`, `E-channel-cutoff.md`).
Уровень: `ENFORCE_PARITY` (исполнено в Workbench без мира, Play и сервера), не runtime и не калибровка.
Основание: REQ-channel-cutoff v0.1 AGENT-REVIEWED (`623f094`), корпус `channel_cutoff_v1.json` заморожен по LF SHA
`b4ff545d34de7859aae3d71678689a727c835c57f46d429df1a722d492c654d8` (agora #3348, #3361, #3370). До кода claude
пересчитал корпус по REQ: 27/27 (`91d50fa`). Допуск native и список host-only согласованы в #3383/#3384.

## Что сделано

- `addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_ChannelCutoff.c` — чистая `Assess(input)`: полный
  preflight до решения (CC05), амплитудная граница `U = m + 20·lg Σ10^((u−m)/20) + guard` (CC06), строгое `<`,
  пустой подтверждённый набор → BOUNDED/EMPTY_PATH_SET без дБм. Отсутствие значения — явный флаг `m_bHas*`, не
  числовой sentinel. Вход не меняется. **Production-вызовы не подключены**, LinkEvaluator, S3/S3b и снимки не
  изменены.
- Стенд `RICON_ChannelCutoffFixtureSupport.c`: решение, причина и признак нуля сравниваются строго; `U` — с
  допуском 1e-3 дБ (U во float32 против binary64 эталона; это допуск компаратора, не сертификация численного
  запаса и не ошибка физической модели). Неизменность входа, новый объект на повторный вызов, 7 контрольных порч
  сравнения, 3 доп. проверки: перестановка путей (C04); null-вход, null-путь, пустой pathId и различение
  NaN/+Inf/−Inf самим сравнением; 4096 путей по −200 дБм суммируются (U ≈ −127.75), 4097-й — INVALID_INPUT.
- Генератор `gen_channel_cutoff_fixture.py` закреплён за LF SHA и пишет LF; 24 native-случая и закрытый список
  host-only {C23 bool вместо времени, C24 bool вместо мощности, C26 лишнее поле JSON} — ошибки JSON-типа,
  недостижимые в типизированном Enforce; генератор проверяет, что у них ожидается UNRESOLVED/INVALID_INPUT.
  Тест генератора — 6 случаев.
- Регрессия: `regress_host.py` (+ `--check` генератора), `regress_wb.ps1` (+ пробник).

SHA-256 по содержимому git blob (LF):

| Файл | SHA-256 |
|---|---|
| `RICON_ChannelCutoff.c` | `a6ab53c70ed6a02d5828c2efe851f9f09f576687adb46945d34b0f8b457211e7` |
| `RICON_ChannelCutoffFixtureSupport.c` | `55f472f47f4b7285c9df93e754dc63de7fe2ea0a55538c81625e6f77e6c97a99` |
| `RICON_ChannelCutoffFixtures.c` | `fcb43022fdebfba8a3c97d9a4400034afc81ff391927fba0b62c39710277e35b` |
| `RICON_ChannelCutoffProbePlugin.c` | `65a7e0845241fb5fceac560733e55247f85aa41bea81f25fb39b1b3ffd3e6ccc` |
| `gen_channel_cutoff_fixture.py` | `122a3ec38a6d6b483965a5bc70d37c231a9e41be8d5e52c56b8c6c03fb7021a0` |

## Исполнение

База логов: `%USERPROFILE%/Documents/My Games/ArmaReforgerWorkbench/logs/`, `console.log`.

| Запуск | Итог | SHA-256 |
|---|---|---|
| `ricon-cutoff-claude-20261022-223428` | **crash**: стенд, не Assess. `Capture` форматировал каждый float через `ToString(-1, 9)`; на C27 (3.4e38) VM Enforce упала со stack overflow, Workbench завершился аварийно (минидамп в папке лога, отчёт Bohemia не отправлялся). 22 случая до этого — match/immutable | `6741a172e22994b3f39481c5c662d89021cb225d0827736f735650083255ebdf` |
| `ricon-cutoff-claude-20261022-223608` | Capture на массиве float: `cases=24 match=24 immutable=24 controls=7 extras=3 hostOnly=3`, PASS | `1c4d7ef0619f2a9763461784f99873d3a1d6b1a807143684cb796551430efcac` |
| `ricon-cutoff-claude-20261022-224436` (финальный срез, после codex #3388) | то же, PASS, SCRIPT(E) 0 | `046a936f1eab78cc9b315d08adc9547a935009ff15ca29a53676a2dcff8a06fd` |

CRC в финальном логе: `RICON_ChannelCutoff.c` f881c5cd, FixtureSupport 57c5ed7f, Fixtures 0d7e73ac, Probe e27a7421.
Наибольшее расхождение U float32 с эталоном в напечатанных значениях — C05: −110.457572937 против −110.457574906
(2·10⁻⁶ дБ). Host: `TOTAL 32/32 PASS` — `docs/evidence/receipts/p1-host-claude-20261022.txt`, SHA-256
`98e938902f1722b1668a4bf4ec3a95dad3fe875a926aebeddc5ed89173cd0cce`. Полная WB-регрессия по согласию #3395 не
повторялась: старые модули не менялись, компиляция пробника проверяет весь модуль Game.

## Границы

Не сделано и не заявляется: сертификация численного запаса для float32 (CC07), выдача строгих границ моделями
мира (P2–P5), подключение к LinkEvaluator/S3, экономия вычислений, временная модель P1b, звук.
