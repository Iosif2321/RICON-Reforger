# P1b.1: Enforce-ядро FINITE_MULTIPATH_V1 совпадает с замороженным корпусом

Дата: 2026-10-23. Автор кода и стенда: claude. Статус: **ожидает ревью codex** (исходник и повтор пробника).

Уровень: `ENFORCE_PARITY`. Код исполнен в Workbench без мира, Play и сервера; это не runtime, не калибровка и не
игровой голос (голос остаётся `BLOCKED_NO_ACTUATOR`).

Основание:
- REQ-channel-fading v0.2 AGENT-REVIEWED (`22a4283`, метка `610a113`);
- корпус `channel_fading_v1.json` заморожен по LF SHA `cf5030f6a4ab5e03b5c02ef63aa4211a012b6a36a059ee2b11b519491a925f61`
  (agora #3542, #3547);
- до кода claude пересчитал корпус слепым checker по REQ: v0.1 — 50/50 (`39dc53f`), v0.2 — 71/71 (`276c6db`);
  codex повторил тот же checker.

## Что сделано

`addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_ChannelFading.c` — чистая
`RICON_FiniteMultipath.Evaluate(descriptor, query)`:
- `RICON_EFadingCode`, где значение 0 — INVALID_INPUT, а порядок совпадает с CF08;
- полный preflight CF03–CF05 до любого кода; ключи проверяются по ASCII, порядок componentId — побайтово;
- фаза CF06 — целочисленно: `MulMod` из двух 15-битных половин, `tQ = MulMod(Δms mod 2^30, 1000) + subUs`, знак
  доплера применяется после положительного MulMod, проверка |dopplerQ| ≤ M/4 обходится без abs(INT_MIN);
- сумма CF07 — в binary32, каждое действие записывается в float-переменную: ближайшая четверть, Horner с
  константами f32 от точных дробей (записаны десятичными литералами без экспоненты), точный поворот;
- `Classify(re, im, phases)`: ZERO решается до hypot только по real == imag == 0; модуль считается масштабированным
  hypot; неположительный или неконечный модуль, как и неконечная часть, даёт NUMERIC_FAILURE;
- нет часов, PTT, epoch, LOST и P1-границы. **Production-вызовов нет**, builder коэффициентов и CT10 не подключены.

Стенд `RICON_ChannelFadingFixtureSupport.c`:
- код, real/imag и фазы сравниваются **строго**, gainDb — с допуском 1e-3 дБ (CF10);
- проверяется неизменность входа, новый объект результата и новый массив фаз на повторный вызов;
- 9 контрольных порч сравнения на F05: null, код, real ±1e-6, imag, gain +0.002, gain NaN, флаг gain, фаза +1,
  лишняя фаза;
- 3 дополнительные проверки:
  1. арифметика: MulMod на 7 крайних парах против точных значений Python, PhaseAt с переносом, кардинальные
     фазы, 11 литералов полинома против одиночного деления binary32 в Enforce;
  2. отказы: null-descriptor, null-query, null-endpoint и null-компонент → INVALID_INPUT; fault-injection
     `Classify` (NaN и −Inf → NUMERIC_FAILURE; 0/0 → ZERO с копией фаз; наименьшая субнормаль не даёт ZERO и
     −Inf);
  3. порядок и частота: серии F04/F05, F40–F50 и F65–F69 дважды в обратном и перемешанном порядке; ZERO на F04
     не мешает ненулевому отсчёту того же descriptor в момент F05.

Генератор `gen_channel_fading_fixture.py`:
- закреплён за LF SHA и пишет LF; 65 native-случаев;
- закрытый host-only список совпадает с полем корпуса `hostOnly`: F27 bool вместо времени, F28 bool вместо веса,
  F29 лишнее поле, F30 пропущенное поле, F57 время вне int32, F70 1000.0 не JSON-int;
- INT_MIN пишется как `(-2147483647 - 1)`;
- недопустимые для REQ ключи («bad/key», пустой, 65 символов) копируются как есть, их отвергает ядро.

Тест генератора — 7 случаев. Регрессия: `regress_host.py` (+ слепой checker, + `--check` генератора) и
`regress_wb.ps1` (+ пробник).

SHA-256 по LF-содержимому:

| Файл | SHA-256 |
|---|---|
| `RICON_ChannelFading.c` | `ae4ea448f61072e4daeb0cdaf668f1800097d487d5c555e90aeff2d6cce6aafe` |
| `RICON_ChannelFadingFixtureSupport.c` | `77a6c05b7fc5ceb713277b34af2480e7921bf01859a83a016fd402fc36e94df7` |
| `RICON_ChannelFadingFixtures.c` | `b4fed18e339469ff7b6c884b5c94e7a7e6b4cc13db51dc4bb307a3daece3c73c` |
| `RICON_ChannelFadingProbePlugin.c` | `ecebd604bad77577513adf4953fe13cfc1200143401a26c85b72a94a0397e26b` |
| `gen_channel_fading_fixture.py` | `ec077e3c9f396bc34e3be60ad2b8c276f64aaa4ecc256e1e9ff6f18e09dabc55` |
| `test_gen_channel_fading_fixture.py` | `edebf6038f198769b1d3eb9eefb2d6ac1340054aee8ad2b363ad6c45837876b0` |
| `receipts/p1b-log-check-claude.py` | `d39a20382186543a46b2826f7f6b3dd8160f995e2a7c4b2a70661f6848f5682a` |

## Исполнение

Лог — `%USERPROFILE%/Documents/My Games/ArmaReforgerWorkbench/logs/ricon-fading-claude-20261023-133705/console.log`,
SHA-256 `240ce6dea188fb80e62b224be7fef1ac77e9155b00b40eb04897eb9103da7b33`. Результат:
- exit 0, SCRIPT(E) 0, единственный `done: PASS`;
- `cases=65 match=65 immutable=65 controls=9 extras=3 hostOnly=6`;
- CRC: `RICON_ChannelFading.c` 83569bbe, FixtureSupport ed656d43, Fixtures 460ab155, Probe 323ffa59.

Это первый и единственный запуск.

Наблюдения:
- **F34** (наименьший положительный binary32-вес) в native совпал строго: OK, h = 1, 0 дБ.
- Fault-injection на наименьшей субнормали: OK, gain −897.069 дБ, то есть FTZ/DAZ на этой платформе не
  обнаружен. Это наблюдение одной машины, не гарантия для сервера.
- Наибольшее расхождение напечатанного gainDb float32 с binary64 эталоном — F08: −243.010284424 против
  −243.010299840 (1.5·10⁻⁵ дБ).

Независимая проверка лога: `python -I docs/evidence/receipts/p1b-log-check-claude.py <log>
oracle/vectors/channel_fading_v1.json` → `P1B_LOG_CHECK_PASS: 65 native, host-only 6 absent, SCRIPT(E) 0`.

Host: `TOTAL 38/38 PASS` — `docs/evidence/receipts/p1b-host-claude-20261023.txt`, SHA-256 `c537e391b9c17f33d681bce02efba644abc32e7ecb6a385ceed822c655407825`.

## Границы

Не сделано и не заявляется:
- builder коэффициентов из положения и движения антенн и отражателей;
- проверка узкополосности и стационарности;
- медленная тень;
- миграция producer/Clone/receiver/seam/serialization (CT10);
- клиентская синхронизация времени (CT06);
- голосовой actuator и реальная тишина в S4;
- вероятностная отсечка хвоста.

ZERO_AT_SAMPLE — свойство одного отсчёта, не LOST и не граница P1.
