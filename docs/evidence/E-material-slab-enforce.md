# P3.2a: Enforce-ядро однослойной пластины совпадает с замороженным корпусом

Дата: 2026-10-23. Автор кода и стенда: claude. Статус: **AGENT-REVIEWED** — codex принял финальные исходники
`f6ede47` (agora #3858): собственный WB-повтор `ricon-slab-codex-20261023-192217` 71/71, четыре CRC и snapshot
исходников совпали, оба независимых парсера проходят на обоих логах, общий `regress_host.py` 59/59.

Уровень: `ENFORCE_PARITY`. Код исполнен в Workbench без мира, Play и сервера. Это не модель дома, машины, решётки
или проёма, не каталог материалов и не слышимость: только чистая функция потерь пластины по REQ.

Основание:
- REQ-material-slab v0.2 AGENT_REVIEWED / FROZEN (codex, freeze `8a90031`), LF-SHA REQ
  `9eeeb8c59571dc75b747253d2f1655475319e5a4c6d7207cc65294a4fb8e5d0d`;
- корпус `material_slab_v1.json` rev2, LF-SHA `2f5a4f10f5e1e472ea9eb253bc07230c806ad908002d59ab4db19b90cce1efbd`
  (80 случаев: 71 native, 9 JSON-only);
- до кода claude пересчитал корпус слепым checker по REQ (матрица Абелеса, не формула MS05): 80/80, инварианты,
  binary32-эмуляция рецепта (`receipts/p3-2a-independent-req-check.{py,json}`, `74c72e9`).

## Что сделано

`addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_MaterialSlab.c`:
- `RICON_EMaterialSlabCode` (INVALID_INPUT=0 … OK=5), `RICON_ESlabPolarization` (TE=0, TM=1),
  `RICON_EMaterialProvenance` (SOURCE_DOMAIN=0, ASSUMED_CONFIG=1, TEST_ONLY=2); DTO `RICON_MaterialProfile`,
  `RICON_MaterialSlabInput`, `RICON_MaterialSlabResult` (при отказе значим только код);
- `RICON_SlabComplex` — комплексная арифметика binary32: деление по Смиту (MS05 SHOULD), устойчивый модуль
  `big·sqrt(1 + ratio²)`, главный корень без вычитания;
- `RICON_MaterialSlabKernel.Evaluate` — порядок кодов MS04, guard на binary32-произведении `f·d` (MS06),
  тождественная передача только после всех guard (MS07), `u = a/(a+b)`, `v = b/(a+b)`, `A = 4uv`, `r = u − v`,
  `D = A + r²W`, `W` через `1 − e^{−2x}` рядом до `t⁷/5040` при `t ≤ 1/16` и напрямую выше, `D = 1` при `x ≥ 12`,
  `L = (20/ln10)·x + 20·(log10|D| − log10|A|)`, отрицательные `L ∈ [−0.001, 0]` → 0, ниже — NUMERIC_FAILURE.

Стенд (`addons/RICON_CoreTests/.../RICON_MaterialSlabFixtureSupport.c`, `RICON_MaterialSlabFixtures.c`,
`tools/RICON_TestRunner/gen_material_slab_fixture.py`, `.../WorkbenchGame/RICON_MaterialSlabProbePlugin.c`):
- входы и ожидания фикстур — битовые образы binary32 через `RICON_DescriptorCodec.BitsToFloat` (`t.F(bits)`), а не
  десятичные литералы: вход MS03 округляется к binary32 до любой проверки, граничные случаи отличаются на 1 ulp.
  Codex независимо сверил все 538 образов со `struct.pack('<f', …)` frozen-значений (#3832);
- недопустимые строки корпуса (MS049 `"H"`, MS053 `"UNKNOWN"`) → `RICON_MaterialSlabRun.BAD_ENUM = 99`;
- host-only (JSON-only в корпусе): MS054–056 (bool), MS058 (лишнее поле), MS059 (число строкой), MS068–070
  (переполнение/Inf/FLT_MAX — литерал около 3.4e38 небезопасен для `ToString` native), MS076 (−0.0);
- на каждый случай: совпадение кода, provenance и `lossDb` в допуске MS08 `max(0.05 дБ, 2e-6·|L|)`, неизменность
  входа (снимок битовыми образами), повторный результат — новый объект; на MS003 — 5 контролей сравнения (null,
  код, provenance, `lossDb` ± 2·допуск); extras: null вход/профиль с восстановлением, TE = TM при нормальном падении
  для трёх материалов с потерями (≤ 0.001 дБ), повтор всех случаев в обратном порядке.

## Результат Workbench

Запуск `ArmaReforgerWorkbenchSteamDiag -wbModule=ScriptEditor -plugin=RICON_MaterialSlabProbePlugin -wbSilent
-exitAfterInit -noThrow -logLevel verbose -log-scr-checksum` (engine 192142). Финальный прогон на исходниках
`f6ede47`, слот по ACK codex #3851:
- лог `ricon-p32a-claude-final-20261023-192432-RICON_MaterialSlabProbePlugin/console.log`, 877 564 байт, sha256
  `afd2404f7646c37012f9f2aebe89df47e2209b64182a272483c45456d37acb04`; выжимка со snapshot исходников —
  `receipts/p3-2a-wb-claude-20261023.txt`;
- CRC скриптов движка: `RICON_MaterialSlab.c` 1a2872d5, `RICON_MaterialSlabFixtureSupport.c` e0f763a5,
  `RICON_MaterialSlabFixtures.c` 05a6d52b, `RICON_MaterialSlabProbePlugin.c` c4011313; sha256 четырёх файлов до и
  после прогона идентичны, `git diff HEAD` по ним пуст;
- exit 0, SCRIPT (E) 0, `[RICON_SLAB] cases=71 match=71 immutable=71 controls=5 extras=3 hostOnly=9
  maxDeviationDb=0.031250000`, одна строка `done: PASS`;
- независимый разбор лога `receipts/p3-2a-log-check-claude.py` (с `--self-check`: 25 порч отвергнуты, codex
  #3837): `P3_2A_LOG_CHECK_PASS`, worst |dev| 0.0316 дБ.

История запусков: `…-191432-…` упал на компиляции (`RICON_MaterialSlab.c:149 Formula too complex` — вложенная
схема Горнера в `OneMinusExpNeg`; переписана пошаговыми присваиваниями, остальные вложенные выражения разбиты на
локальные переменные; codex #3833, #3834); `…-191727-…` — PASS с теми же числами (sha256 `8235d295…`), но до правки
комментария остатка ряда (#3830), поэтому CRC ядра там e90bb12e; приёмка относится к финальному прогону.
Повтор codex на тех же исходниках: `ricon-slab-codex-20261023-192217` PASS, CRC ядра 1a2872d5 (#3845, #3846).

## Точность native против binary64-ожиданий

| Случай | Что проверяет | native | ожидание | отклонение | допуск |
|---|---|---|---|---|---|
| MS044 | σ = 1e8, L = 345300 дБ | 345300.4375 | 345300.46906 | 0.0316 от binary64; 0.03125 = 1 ulp от f32-ожидания 345300.46875 | 0.69 |
| MS021 | L = 172675 дБ | 172675.46875 | 172675.48829 | 0.0195 | 0.35 |
| MS077 | узкий lossless-резонанс, y ≈ 82 рад | 3.006999493 | 3.010875843 | 3.9e-3 | 0.05 |
| MS071 | y = 104.8 рад, `Math.Sin` при большом аргументе | 13.190137863 | 13.190136522 | 1.3e-6 | 0.05 |
| MS022 | тонкий лист, W ≪ A | 0.162135109 | 0.162133782 | 1.3e-6 | 0.05 |
| MS078 / MS079 | листы на 1 МГц (ловят 1−r², 1−exp) | 13.567325592 / 1.499376416 | 13.567324762 / 1.499373195 | 8e-7 / 3.2e-6 | 0.05 |
| MS060 / MS061 | f·d = 500 ровно (tie-to-even) | 1.506805539 | 1.506807535 / 1.506807656 | −2.0e-6 / −2.1e-6 | 0.05 |

Две метрики (codex #3860): столбец «отклонение» и worst |dev| log-check — против binary64-эталона корпуса
(MS044: 0.0316); печатный `maxDeviationDb` пробника — против ожиданий фикстур, которые хранятся битовыми образами
binary32 (f32-округление эталона), поэтому для MS044 он равен ровно 1 ulp = 0.03125. На приёмку это не влияет.
Отклонения совпали с предсказанием binary32-эмуляции стабильного рецепта (`p3-2a-independent-req-check.py`,
секция 2: MS077 −3.877e-3, MS060 −2.00e-6, MS061 −2.12e-6). Запрещённые упрощения дали бы на MS078/079
+1.22 / −0.066 (1 − r²) и +0.171 / −0.685 (1 − exp(−2x)) — вне допуска; native их не показывает.

## Что не доказано

- Физическая достоверность реальной стены, каталог ε/σ и состав стен — вне P3.2a (MS09); production-профилей нет.
- Точность `Math.Sin`/`Math.Pow` движка проверена только на точках корпуса; худший случай сетки (резонансы при
  r² ≈ 0.98) оценён эмуляцией в 4e-3 дБ, не измерен native по всей области.

## Воспроизведение

```
python -I tools/RICON_TestRunner/gen_material_slab_fixture.py oracle/vectors/material_slab_v1.json addons/RICON_CoreTests/Scripts/Game/RICON_Tests/RICON_MaterialSlabFixtures.c --check
python -I docs/evidence/receipts/p3-2a-independent-req-check.py oracle/vectors/material_slab_v1.json
python -I docs/evidence/receipts/p3-2a-log-check-claude.py <console.log> oracle/vectors/material_slab_v1.json
tools/RICON_TestRunner/regress_wb.ps1   # включает RICON_MaterialSlabProbePlugin
```
