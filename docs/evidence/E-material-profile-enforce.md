# P3.2b: Enforce-ядро частотного профиля материала совпадает с замороженным корпусом

Дата: 2026-10-23. Автор кода и стенда: claude. Статус: **ENFORCE_PARITY, ожидает повтор codex** (AGENT-REVIEWED
после независимого WB-повтора codex на тех же исходниках; оба независимых парсера лога уже проходят).

Уровень: `ENFORCE_PARITY`. Код исполнен в Workbench без мира, Play и сервера. Это численные примеры Table 3
ITU-R P.2040-4 для явно выбранного профиля, не свойства реальных стен и объектов игры, не назначение материалов
объектам и не геометрия (вход/выход, проёмы — P3.3).

Основание:
- REQ-material-profile v0.2 AGENT_REVIEWED / FROZEN (codex, freeze `e62f479`), LF-SHA REQ
  `2385e37bcb24f8183eaee84e477bf6211a17f557c808d7a011998dbdfb9597dd`;
- корпус `material_profile_v1.json` rev2, LF-SHA `36f854eaa5cc2b9e1dc10d8ee6e158f4db55f2b4ae6bee7ccfcc38042135ebc2`
  (236 случаев: 223 native, 13 JSON-only); каталог `material_profiles_p2040_v1.json` LF-SHA
  `9d5b720157407d2562b5f0f8756052f1cacd1f3d43d3688f34b26c1a679649e9` (38 строк Table 3);
- до кода claude пересчитал корпус слепым checker по REQ и каталогу (без oracle/build/test codex) с независимой прямой
  степенью: 236/236 (`receipts/p3-2b-independent-req-check.{py,json}`, `bc31846`).

## Что сделано

`addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_MaterialProfile.c`:
- `RICON_EMaterialProfileCode` (INVALID_INPUT=0 … OK=5), DTO `RICON_MaterialFitProfile` {a, b, c, d, minMHz, maxMHz,
  maxInclusive, provenance}, `RICON_MaterialProfileInput`, `RICON_MaterialProfileResult` (при отказе значим только код);
- `RICON_MaterialProfileKernel.Evaluate` — порядок кодов MP03, полоса с учётом `maxInclusive`, MHz→GHz, положительность
  по коэффициентам (a > 0, c > 0), а не по результату (MP03.4); пределы MS03 ε′ ∈ [1, 100], σ ≤ 1e8; `material` с
  minMHz = maxMHz = f, provenance копируется (MP05);
- `Power` — только устойчивая форма 10^t, t = log10 coef + exp·log10 f (MP04), точные случаи coef = 0 и exp = 0;
  при t < 0 степень берётся как 10^(t+20)·1e-20, чтобы округление к субнормальному или к нулю выполняло умножение
  IEEE (см. историю прогонов); порогов классификации нет: не конечный результат → переполнение, ноль → underflow.

Стенд (`addons/RICON_CoreTests/.../RICON_MaterialProfileFixtureSupport.c`, `RICON_MaterialProfileFixtures.c`,
`tools/RICON_TestRunner/gen_material_profile_fixture.py`, `.../WorkbenchGame/RICON_MaterialProfileProbePlugin.c`):
- генератор привязан к LF-SHA REQ и корпуса, ожидания только копируются; входы и ожидания — битовые образы binary32
  через `RICON_DescriptorCodec.BitsToFloat` (`t.F(bits)`): вход MP02 округляется к binary32 до любой проверки;
  недопустимые provenance корпуса → `BAD_ENUM = 99`; host-only (13, все `native=false` в корпусе: JSON-типы, лишние
  и отсутствующие поля, строки вместо чисел, bool/NaN/Inf) в Enforce не воспроизводятся;
- на каждый случай: совпадение кода, provenance, minMHz/maxMHz (точно), ε′ и σ (rel ≤ 2e-5 для положительного
  ожидания, ноль точно — MP07), неизменность входа (снимок битами), повторный результат и material — новые объекты;
  на первом OK-случае с σ > 0 — 5 контролей сравнения (null, код, provenance, ε′ вне допуска, σ ноль вместо
  положительного);
- лог печатает числа битами binary32 (codex #3978/#3982: десятичная печать движка теряет субнормальные σ и не печатает
  1e7): `[RICON_MPROF] id= code= epsBits= sigmaBits= minBits= maxBits= provenance= match= immutable=`;
- extras = 6 (codex #3986): null вход/профиль с восстановлением; группа цепей профиль → существующее ядро пластины
  `RICON_MaterialSlabKernel` для **каждого** native OK-случая с σ > 0 (119: OK и L ≥ 0 на той же частоте,
  OUT_OF_MODEL_DOMAIN на 2f или 0,5f, потому что minMHz = maxMHz = f; толщина 0,1 м до 2000 МГц, выше 200/f, чтобы
  f·d ≤ 200 и 2f·d ≤ 400 внутри MS06; строки `[RICON_MPROF_CHAIN] fBits dBits epsBits sigmaBits slab loss shifted ok`);
  повтор всех случаев в обратном порядке; три контрпримера Power codex #3980/#3981 (a = 4, b = 0, TEST_ONLY):
  c = 9e-36, d = 10, f = 100 → σ = 9e-46 → binary32 bits 1 (OK, не ноль); c = 1e38, d = 15, f = 1 → промежуточная
  степень 1e-45 субнормальна, σ = 9.999999406e-08 (bits 869711764); c = 3.4e38, d = 1, f = 1000 → σ конечен, значит
  NUMERIC_DOMAIN_LIMIT, а не NUMERIC_FAILURE.

## Результат Workbench

Запуск `ArmaReforgerWorkbenchSteamDiag -wbModule=ScriptEditor -plugin=RICON_MaterialProfileProbePlugin -wbSilent
-exitAfterInit -noThrow -logLevel verbose -log-scr-checksum` через `tools/RICON_TestRunner/addon.gproj`
(slot по ACK codex #3980: один основной прогон плюс адресные исправления с явными логами). Финальный прогон:
- лог `ricon-p32b-claude-20261023-211215-RICON_MaterialProfileProbePlugin/console.log`, 930 156 байт, sha256
  `4f1d11b6d0c6379554c6d88109fde1954a7230cb2b38dcd21aa7e88b9438ce7b`; выжимка обоих прогонов со snapshot исходников —
  `receipts/p3-2b-wb-claude-20261023.txt`;
- CRC скриптов движка: `RICON_MaterialProfile.c` 3a597b37, `RICON_MaterialProfileFixtureSupport.c` d99c4a29,
  `RICON_MaterialProfileFixtures.c` 72c5e41b, `RICON_MaterialProfileProbePlugin.c` 9ba4b825; sha256 пяти файлов
  (плюс генератор) до и после прогона идентичны;
- exit 0, SCRIPT (E) 0, `[RICON_MPROF] cases=223 match=223 immutable=223 controls=5 extras=6 hostOnly=13 chains=119
  maxRelativeError=0.000008498`, `[RICON_MPROF_POWER] id=1 … sigmaBits=1 ok=1`, `id=2 … sigmaBits=869711764 ok=1`,
  `id=3 code=NUMERIC_DOMAIN_LIMIT … ok=1`, одна строка `done: PASS`;
- независимый разбор лога claude `receipts/p3-2b-log-check-claude.py` (декодирует биты, сверяет с binary64-ожиданиями
  корпуса, порядок/полноту/host-only, 119 цепей с пересчётом потерь принятым slab-оракулом по MS08, 3 контрпримера,
  сводку; `--self-check`: 44 порчи отвергнуты): `P3_2B_LOG_CHECK_PASS … worst rel 8.5e-06, worst |loss dev| 0.0195 dB`;
- независимый валидатор codex `receipts/p3-2b-log-check-codex.py --log …` (30 отрицательных контролей):
  `NATIVE_LOG_PASS`, `maxRelativeErrorFromBits 8.498e-06`, `maxChainLossErrorDb 0.0195`, тот же sha256 лога.

История: прогон `…-210840-…` (sha256 `5be9b1bb…`, CRC ядра 5391326d) — компиляция чистая, 223/223, 119/119 цепей,
та же maxRelativeError, но контрпример 1 дал `NUMERIC_FAILURE`: движковый `Math.Pow(10, −45.0458)` возвращает 0 вместо
округления вверх к FLT_TRUE_MIN (bits 1), хотя субнормальный σ = 1e-40 корпуса (MP235, c = 1e-30, d = 10) он считает
верно — FTZ/DAZ нет, предел именно на границе наименьшего субнормального. Это подмена положительного σ нулём (MP07),
поэтому ядро исправлено (масштабированная форма при t < 0), `done: FAIL` первого прогона сохранён в receipt.
До прогонов по counter codex #3978 из ядра убраны пороги `LOG10_OVERFLOW = 38.53` / `LOG10_UNDERFLOW = −45` (они
отсекали часть конечных binary32: 3.4e38 → NUMERIC_FAILURE вместо NUMERIC_DOMAIN_LIMIT, 9e-46 → 0) и прямое
произведение с субнормальной промежуточной степенью (c = 1e38, d = 15: f^d = 1e-45 теряет точность).

## Точность native против binary64-ожиданий

| Случай | Что проверяет | native | ожидание | rel | допуск |
|---|---|---|---|---|---|
| MP211 | ε′ = 1e-38·110^19: t = −38 + 38.79, сокращение в логарифме | 6.115960598 | 6.115908623 | 8.5e-6 | 2e-5 |
| MP136 | floorboard 02a, c = 2.22e-17, d = 7.3413, 220 ГГц | σ 3.489649773 | 3.489639997 | 2.8e-6 | 2e-5 |
| MP156 | vinyl 1 ГГц, d = 0.8422 | σ 5.09998761e-3 | 5.10000018e-3 | 2.5e-6 | 2e-5 |
| MP235 | σ = 1e-40 субнормальный | bits 71362 | 9.99994610e-41 | в допуске | 2e-5 |
| Power 1 | σ = 9e-46 → округление вверх к FLT_TRUE_MIN | bits 1 | bits 1 | 0 | точно |
| Power 2 | промежуточная степень 1e-45 | bits 869711764 | bits 869711764 | 0 | 2e-5 |

Все остальные случаи ≤ 2.4e-6. Худший случай MP211 совпал с предсказанием binary32-эмуляции рецепта (корректно
округлённые float32-операции): 8.498369e-6 против напечатанного 0.000008498 — на точках корпуса движковые
`Math.Log10`/`Math.Pow` ведут себя как корректно округлённые binary32-функции.

Цепи к пластине (119, потери native против slab-оракула на тех же битах; допуск MS08 max(0.05 дБ, 2e-6·|L|)):
MP171 металл 1 ГГц, d = 0.1 м, L = 172675.47 дБ — отклонение 0.0195 (допуск 0.35); MP172 металл 10 ГГц, d = 0.02 м,
L = 109225.49 — 0.0145 (0.22); MP202 ε′ = 100, σ = 1e8, L = 545905.4 — 0.0073 (1.09); остальные ≤ 3e-4 дБ;
потери от 4.7e-4 дБ (тонкий лист на низкой частоте) до 545905 дБ.

## Что не доказано

- Физическая достоверность стен/объектов, назначение материалов объектам, геометрия и проёмы — вне P3.2b (P3.3, MS09);
  production-профилей нет, каталог — примеры измерений Table 3.
- Точность движковых `Math.Pow`/`Math.Log10` подтверждена на 223 точках корпуса и 3 контрпримерах, не по всей области;
  в пределах ~1e-5 относительных от границ классификации (ε′ = 1 / 100, σ = 1e8, FLT_MAX) native и host могут
  разойтись из-за binary32-ошибки t — в корпусе таких точек нет.
- Ниже FLT_TRUE_MIN/2 положительное значение в binary32 — ноль и у host, и native (NUMERIC_FAILURE, MP236).

## Воспроизведение

```
python -I tools/RICON_TestRunner/gen_material_profile_fixture.py oracle/vectors/material_profile_v1.json addons/RICON_CoreTests/Scripts/Game/RICON_Tests/RICON_MaterialProfileFixtures.c --check
python -I docs/evidence/receipts/p3-2b-independent-req-check.py oracle/vectors/material_profile_v1.json oracle/data/material_profiles_p2040_v1.json docs/req/REQ-material-profile.md
python -I docs/evidence/receipts/p3-2b-log-check-claude.py <console.log> oracle/vectors/material_profile_v1.json --slab-oracle oracle
python -I docs/evidence/receipts/p3-2b-log-check-codex.py --log <console.log>
tools/RICON_TestRunner/regress_wb.ps1   # включает RICON_MaterialProfileProbePlugin
```
