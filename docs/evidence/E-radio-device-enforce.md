# S2: адаптер рации на Enforce — ENFORCE_PARITY

Дата: 2026-10-21. Автор кода: claude. Требования и эталон: codex (`REQ-radio-device-adapter.md` v0.2,
`oracle/radio_device_oracle.py`, корпус rev2 — `400496d`). Статус: AGENT-REVIEWED — codex #3167; независимый повтор стенда codex #3166
(`ricon-radio-device-codex-s2-20261021-182721/console.log`, SHA-256
`86c373fb6da34f79ff3f9d5e24fab014eef8862215ad2b656ddbac9b544bc846`, те же счётчики).
Уровень: **ENFORCE_PARITY** — код совпал с эталоном при исполнении в Workbench без мира, Play и сервера.

## Файлы

| Файл | Что |
|---|---|
| `addons/RICON_Core/Scripts/Game/RICON/Device/RICON_RadioDeviceAdapter.c` | коды §3, наблюдение, RF-профиль, контекст, `RICON_RadioQueryBuilder.Build`, `RICON_BaseRadioObservationReader.TryRead` |
| `addons/RICON_CoreTests/Scripts/Game/RICON_Tests/RICON_RadioDeviceFixtureSupport.c` | сравнение, немутируемость, контрольные отказы, цепочка до SINR, безопасная ветвь reader |
| `addons/RICON_CoreTests/Scripts/Game/RICON_Tests/RICON_RadioDeviceFixtures.c` | генерируется из корпуса, 68 случаев |
| `tools/RICON_TestRunner/gen_radio_device_fixture.py`, `test_gen_radio_device_fixture.py` | генератор, SHA корпуса `1806e7377cc0a68c7fc647ff19a3482841cbba4f5820fd22fbc24b8f8846d960` закреплён; 6 тестов |
| `tools/RICON_TestRunner/Scripts/WorkbenchGame/RICON_RadioDeviceProbePlugin.c` | стенд |

Генератор только копирует ожидания. Float записывается исходной десятичной записью, её округляет компилятор
Enforce. Переполнение binary32 (R50, 1e40) записывается как бесконечность того же знака, как требует REQ §2.
Underflow R51 (1e-50) `literal()` печатает как `0.0`: результат тот же, что у binary32.

## Что проверяет стенд

- Каждый из 68 случаев: код, полный запрос либо null, все числа запроса конечны. Перед вызовом запрос не null:
  отказ обязан его обнулить.
- Немутируемость в каждом случае. Входы до вызова и после совпадают, с учётом NaN и Inf в неактивных полях. При
  READY выходной запрос искажается, затем второй вызов. Новый объект равен ожиданию, входы не изменились.
- 9 контрольных отказов на R01. Сравнение обязано отвергнуть: сдвиг высоты TX, полосу, мощность, усиление RX,
  NF и порог, взятые не с той стороны; чужой bandId; NaN в ожидании; null вместо запроса.
- Цепочка R01 → S1 (ровный источник высот, 100 м) → L2 → evaluator. Ожидания скопированы из
  `oracle/test_radio_device_oracle.py:155–170` (codex): потери 108.1797261261363 дБ, мощность −66.1797261261363 дБм,
  SINR 58.82636093801125 дБ, допуск 0.05 дБ, состояние CLEAN.
- Ноги: та же пара, антенна TX на высоте поверхности (y = 100). Сборщик даёт READY и не поднимает антенну;
  бюджет не valid, причина `TERRAIN_PROFILE_UNAVAILABLE` (16).
- Reader без рации: отказ, observation = null. В Workbench `IsAuthority()` вернул true, поэтому до проверки
  рации дело дошло. Ветви с настоящей рацией только скомпилированы (`BLOCKED_NO_RUNTIME`).

## Логи

База: `%USERPROFILE%/Documents/My Games/ArmaReforgerWorkbench/logs/`.

- **Стенд:** `ricon-radio-device-claude-s2-20261021-181512/console.log`, SHA-256
  `1a3bf34e5ee60a5c814171c0a06e78d1d2f452501d3bbe39238f7c0ba409d870`. Exit 0, SCRIPT(E) 0, `done: PASS` 1;
  cases 68, quietPositive 68, immutable 68, controls 9, chains 2, reader 1. Цепочка: loss 108.179718,
  rx −66.179718, SINR 58.826370, CLEAN, 21 обращение к источнику; ноги: READY, txY 100, valid 0, reason 16.
- **Ранний FAIL компиляции:** `ricon-radio-device-claude-s2-20261021-181418/console.log`. Две длинные цепочки `&&`
  в support (строки 87 и 132) Enforce отверг: «Formula too complex». Их разбил на пошаговые проверки,
  production-код не менялся. Этот запуск не засчитан.

## Общая регрессия

S2 включён в общие runners: `regress_host.py` — radio_device --check и gen_radio_device --check; `regress_wb.ps1` —
RICON_RadioDeviceProbePlugin. Прогон на дереве с кодом S2 до коммита:

- `regress_host.py` — **26/26 PASS**.
- `regress_wb.ps1` — **9/9 PASS**, папки `ricon-r29-claude-20261021-181645-<plugin>`. У каждого exit 0,
  donePass 1, SCRIPT(E) 0, fails 0. RadioDeviceProbe SHA-256
  `8264c448bdd608f4d53ffe966aeb76df4ce3f1c8acac51e381a2113cf7829b0f`, TestRunner SHA-256
  `b4d5f7264cee9ecbf74f04f3a79d3c8f14df361b0b851564cca2f605a61821d8`.

## Не проверено

- Настоящие рации: binding устройства, `GetFrequency` (единица UNKNOWN), ключ, питание, атомарность снимка.
- Каталог RF-параметров настоящих раций: в корпусе только TEST_ONLY и ASSUMED; калибровки нет.
- Отбор пар, в том числе одно устройство на обоих концах (R68 READY), — это S3.
- READY не означает доставку голоса: actuator нет, `BLOCKED_NO_ACTUATOR` сохраняется.
