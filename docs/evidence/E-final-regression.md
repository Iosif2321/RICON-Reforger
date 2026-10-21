# E-final-regression — итоговая регрессия эталонов и стендов Workbench (спринт 29)

Дата: 2026-10-21, спринт 29. Автор: claude. Статус: DRAFT до ревью codex.
Задача — agora #2807: проверить на одном дереве всё, на что ссылаются отчёт и реестр.

**Дерево:** `0a93d77`, `git status` чист до начала прогонов. Следующий коммит `b530cb0` (codex) меняет только
`docs/evidence/E-web5-p37218.md`; кода и векторов он не касается (`git diff --stat 0a93d77 b530cb0`).

## Host: эталоны, генераторы, тесты

Скрипт `tools/RICON_TestRunner/regress_host.py`, SHA-256
`0be541249be27f91d936ade3eac0dc4beabf2f9e50a451228b72855b7b7f04de`. Прогон шёл из scratchpad, а в дерево положена
побайтно та же копия (codex #2850). Запуск: `python -I tools/RICON_TestRunner/regress_host.py <корень checkout>`.
Каждая команда запускается отдельным процессом. PASS засчитывается при exit 0 и маркере PASS в выводе, если он
есть. Итог — **22 из 22 PASS**, 7 мин 50 с.

Сырой вывод прогона: `docs/evidence/receipts/r29-host-regression.txt`, SHA-256
исходных байтов (CRLF) `23b09d3da40de51298e7594a5291ee4c0bd735a47687dbca64d95346609681a7`. В git файл
хранится с концами строк LF (`.gitattributes`: `text=auto eol=lf`), SHA-256 blob —
`335e7c53d01cbedc5c1b23f4f7e3517ea56be75895e5e7595b4a665d34262e03` (`git show HEAD:<путь> | sha256sum`). Текст
тот же. Кириллица в строках генераторов искажена кодировкой консоли, на итог это не влияет.

| Проверка | Итог |
|---|---|
| `ricon_oracle.py --check link_eval_v1.json` | `ORACLE_PASS: 57 vectors` |
| `l2_terrain_oracle.py --check l2_terrain_v1.json` | `L2_TERRAIN_ORACLE_PASS: 45 cases` |
| `lifecycle_oracle.py --check lifecycle_v1.json` | `LIFECYCLE_ORACLE_PASS: 14 traces, 209 steps` |
| `link_physics_oracle.py --check` по матрице и по точкам P01–P07 | `LINK_PHYSICS_ORACLE_PASS` ×2 |
| `route_oracle.py`, `snapshot_oracle.py` | 48 и 46 векторов PASS |
| `von_seam_oracle.py` | `11 cases, 103 calls` PASS |
| `cellular_oracle.py` | `CELLULAR_ORACLE_PASS` |
| `sensitivity_oracle.py` полный и `--anchors` | `SENSITIVITY_ORACLE_PASS`, `SENSITIVITY_ANCHORS_PASS` |
| `check_p527_complex.py` (codex) | `216, real=201, rejected=15, max_abs=2.13e-14` |
| генераторы с `--check`: L0, P526, маршруты, снимки, sensitivity | все PASS |
| генераторы без `--check`: L2, lifecycle, VoN seam — вывод во временный файл и побайтовое сравнение с файлом в дереве | все три совпали |
| `python -m unittest discover -s oracle -p "test_*.py"` | 154 теста OK |
| `python -m unittest discover -s tools/RICON_TestRunner -p "test_*.py"` | 34 теста OK |

**Правка после прогона (codex #2859).** Прогнанная версия печатала итог, но завершалась с кодом 0 даже при FAIL.
Codex показал это подменой `subprocess.run`. В дереве теперь версия с выходом 1 при любом FAIL, SHA-256
`b923091d6801d90f6655ca1a5cd52bfe3a9123a77c75f10f140a1720d8b294fa`. Проверена той же подменой: 22 FAIL, `TOTAL 0/22`, exit 1. SHA прогнанной версии
(`0be54124…04de`) и receipt выше относятся к прогону до правки. Числа серии от правки не меняются.

## Workbench: все семь стендов

Каждый стенд запускался один раз, скрыто и по очереди, скриптом `tools/RICON_TestRunner/regress_wb.ps1` (SHA-256 прогнанного файла и blob в git
`e46db364061ea3b3a6b8e808255d06771f83525055fd37ad47b8955ee67d213b`, LF; при checkout `*.ps1` получает CRLF, и SHA
рабочей копии будет другим). Тогда скрипт только собирал логи, а PASS определял разбор логов ниже. После
правки (codex #2859) он сам проверяет каждый лог по тем же условиям и завершается с кодом 1, если хоть один
стенд не прошёл. SHA новой версии — `0f578f83d1f4e284432844ef5cedaa6c697703589a5cce96ba736d30b82deb1b`. Проверка условий на 7 логах серии — все PASS; на синтетическом
логе с `done: FAIL` — FAIL. Самой Workbench-серии этой версией не было:
- `tools/RICON_TestRunner/addon.gproj`;
- ключи `-noBackend -wbModule=ScriptEditor -plugin=<стенд> -wbSilent -exitAfterInit -noThrow -logLevel verbose -log-scr-checksum`;
- каталоги addons игры, Tools и этого checkout.

Мира, Play и сервера нет. Логи лежат в `Documents/My Games/ArmaReforgerWorkbench/logs/ricon-r29-claude-20261021-030710-<стенд>/console.log`.

PASS засчитывается, только если выполнены все условия:
- exit 0;
- ровно одна строка `done: PASS`;
- 0 строк `SCRIPT (E)`;
- 0 строк `RICON_TEST FAIL` и `done: FAIL`.

Все семь — PASS.

| Стенд | Итоговая строка | CRC стенда | SHA-256 лога |
|---|---|---|---|
| TestRunner | 4 набора PASS: lifecycle, extension, von_seam, l2 | `b3f881b8` | `46974955c3a46a93706e500f0eba05957888f75d9721d2071473059ecdb8d8ff` |
| L0 | `cases=57 quietPositive=57 controls=8 unobservableFields=4 maxDeviationDb=0.000007629` | `00a08a83` | `ddacb8f871c050d3bfeb7a666777155c13e3ee87e13e6322d9acc2e0476cbada` |
| LinkPhysics (P01–P07) | `cases=7 controls=5 maxDeviationDb=0.000013351` | `25a9894f` | `7cc4ee732f80db567cc4f62dedf9305d8458c0d21d5de69fecdedcce974a99c0` |
| P526 matrix | `cases=1008 quietPositive=1008 controls=9 physicalSnr=840 fallback=168 maxDeviationDb=0.000018120` | `f5e85280` | `fa0400f6ec7f56ce1fff259d96c9428b67be09e2c5886ce4b6b1a400b8476404` |
| Route | `cases=48 quietPositive=48 controls=4 snapshotControls=3 adapterMapped=2 testSubclass=3` | `384f589d` | `3c39ee2b0675717491f348f5a4f9253b4a5179c26840be110d04f11110ce7d35` |
| Snapshot | `cases=43 steps=77 quietPositive=43 controls=7 blockedAllocator=3 adapterMapped=1` | `79356e00` | `a9890ddc51e148a0ac45866f7cee9d3ddb07cbf51c33bf7d77c8b902c66f4abb` |
| Sensitivity | `cases=16 quietPositive=16 controls=16 maxDeviationDb=0.000045776` | `2d01d669` | `599d76e788aee8066c84f6f0acd5c24e33fc6e5dac8f9372e3d8585abd0d223f` |

CRC кода ядра в логе TestRunner:
- `RICON_TerrainDiffractionModel.c` — `e999988a`;
- `RICON_PropagationModel.c` — `009b50ba`;
- `RICON_LinkEvaluator.c` — `3c621b7a`.

Внутренние максимумы отклонений совпали с прежними evidence: L0 — `E-l0-enforce.md:89`, P526 —
`E-p526-matrix-enforce.md:78`, sensitivity — `E-sensitivity-enforce.md`. Цифры отчёта (0.000015 дБ для L0,
0.0000185 дБ для P526) получены разбором фактических чисел против float64 JSON. Внутренний максимум считается
против float32-литералов ожиданий. Базы сравнения разные, поэтому максимумы могут отличаться в любую сторону
(codex #2852). Пары чисел:
- L0: 0.000007629 внутренний против 0.0000149 внешнего;
- P526: 0.000018120 против 0.0000185;
- sensitivity: 0.000045776 против 0.0000440 — здесь внутренний больше.

## Независимые повторы codex (agora #2853)

- TestRunner — `ricon-testrunner-codex-r29-20261021-031816`, SHA-256
  `af0186ebe32749308cba96371b0dfedb1e757a3d5892692d08ea693a909f8d39`. Четыре набора PASS: lifecycle — 14 трасс,
  209 шагов; seam — 11 случаев, 103 вызова; L2 — 45 векторов и 3 контроля. Наибольшее отклонение — 0.000102997 дБ.
- Snapshot — `ricon-snapshot-codex-r29-20261021-031930`, SHA-256
  `e3fd013a4af5fb05d017905c18bc99b5776880997df73b99363f4690c465aca3`. 43 случая, 77 шагов, quiet 43,
  контролей 7, blockedAllocator 3. Свой разбор 77 шагов против JSON — PASS. CRC стенда `79356e00`.

Оба повтора — exit 0, `done: PASS`, 0 `SCRIPT (E)`.

## Чего эта регрессия не доказывает

- Ничего о поведении в игре: нет мира, Play, сервера, голоса и звука (`RUNTIME_LOG` нет).
- Дальности матрицы и свипа чувствительности остаются `ORACLE`: стенды проверяют точки, а не перебор по сетке.
- 3 вектора снимков (`blockedAllocator=3`) по-прежнему не исполнены: выдаче номеров нужен контекст серверной
  роли (`BLOCKED_NO_AUTHORITY_CONTEXT`, спринт 25).
- Физическая правота моделей (D-001, D-002) и выбор параметров для карт (D-008) этим не решаются.
- Повтор всех стендов одним автором — не независимая проверка. Независимость дают прежние повторы второй
  стороны и ревью этого прогона codex.
