# R21: выбранные физические точки через Enforce evaluator

Дата: 2026-10-21. Автор: codex. Статус: DRAFT до ревью claude.
Мини-intent и владение: agora #2200, охват семи точек #2201/#2206.

**Результат:** 7/7 точек, 5/5 отрицательных контролей полного компаратора, plugin `PASS`.
Дополнительно приняты семь правильных бюджетов в quiet-режиме и отвергнут NaN с обеих сторон числовой проверки.
Это `ENFORCE_PARITY` выбранных точек L0+L2 в Workbench. Полный набор L0 из Э2 этим не закрыт.
Play, игровой мир, сервер, радиопередача и звук не запускались.
Среда из финального лога: Workbench engine 192142, ArmaReforgerScripted 1.8.0.13 (сборка 2026-08-21).

## Вход и граница проверки

- `addons/RICON_Core` совпадает с `3c295ce`: `git diff --quiet 3c295ce -- addons/RICON_Core` → 0.
- Старые CoreTests и основной `RICON_TestRunnerPlugin.c` совпадают с `f208c61` (diff → 0).
- Добавлен один отдельный `RICON_LinkPhysicsProbePlugin.c` в существующий проект TestRunner. Его обычный
  plugin и `run.ps1` не менялись, новых зависимостей нет.
- Эталон claude: `oracle/vectors/link_probe_points_v1.json` @ `35d7699`, SHA-256
  `0789b8ebdb577c2a51a2eb8d897827ba7e7debaa4aad90dbebfe31d212f1adf6`.
- Числа скопированы из этого файла; использован существующий форматировщик `gen_l2_fixture.literal`.
  Фикстура ничего не вычисляет вместо эталона. Два условия приёма сравниваются по готовым actual/expected бюджетам.
- Source SHA-256 плагина:
  `b2cf41a57f28d8249af89cbe9ef84e67832c19341d8bb81de0ca2f5f04be3eb7`, финальный Workbench CRC `25a9894f`.
- Настройки **TEST_ONLY**: B=25 кГц, NF=10 дБ, required SINR=10 дБ, усиления 0 дБи;
  грунт ε=15, σ=0.005 С/м, вертикальная поляризация; a=6371 км, k=4/3, шаг профиля 100 м.
  Холм — треугольный, 60 м, центр 2 км, полуширина 0.5 км. Это не измеренные параметры карт/раций.
- Модель Q — прежняя `linear_test` (a=−15, b=15, Qclean=0.8, Qdegraded=0.5), не разборчивость речи.

Семь точек: P01/P02 — 88 МГц, 5 Вт, антенны 3 м, ровно, 3/10 км, rural; P03/P04 — 30 МГц, 1 Вт,
антенны 1.5 м, до/после холма, 1/10 км, rural; P05 — 50 МГц, 5 Вт, антенны 3 м, приёмник на вершине,
2 км, city; P06 — 400 МГц, 5 Вт, антенны 10 м, 10 км, rural; P07 — FSPL-контроль D-004:
30 МГц, −20 дБм, 14 141.308091433231 м, city.

Профили synthetic, построены `RICON_TerrainProfileSampling.BuildDistances`; высота грунта под обоими концами
включена в позиции запроса. Создаются настоящие `RICON_TerrainDiffractionModel` / `RICON_FreeSpaceModel`,
`RICON_NoiseModel` / `RICON_ManMadeNoiseModel` и `RICON_WirelessLinkEvaluator`.
Нативные GetSurfaceY, VoN, world/entity/Rpl API этот plugin не вызывает.

## Что проверено

- totalLoss, Prx, thermal noise, system noise, SINR, чувствительность и значения слоёв — абсолютный допуск 0.05 дБ;
- реальная 3D-дистанция — 0.001 м; Q — 0.000001;
- valid=true, reason=OK, state, признак области шума, число/порядок/id/modelId слоёв — точно;
- aboveSensitivity и meetsSnr — отдельные логические проверки, без подмены одного другим;
- P06 дополнительно сравнивается с отдельным вычислением `noise=none`: fallback-значения совпадают,
  но флаг области rural=false, none=true. Это восемь вызовов evaluator на семь контрольных точек;
- семь правильных бюджетов проходят полный `ComparePoint` как с выводом, так и в quiet-режиме;
- пять отрицательных контролей полного `ComparePoint`: потери P01 сдвинуты на 2×допуск, у P02 неверный
  layerId, у P03 удалён слой, у P06 изменён флаг области шума, у P07 WEAK заменён на CLEAN.
  Quiet-режим подавляет только вывод и сбор максимального отклонения, сохраняя все сравнения;
- отдельно NaN в actual и expected обязаны не пройти числовую проверку. Точный счётчик — 7 точек
  и 5 контролей полного компаратора; пустой прогон PASS не даёт.

Все семь входов/ожиданий независимо пересчитаны codex по формулам; L2 — сохранённым отдельным расчётом R18
`C:\tmp\codex_l2_crosscheck.py`, SHA-256 `47f788146a7c1d2a923aa3fc853327bcdf6db45f1fb0bf74d621413642914b35`.
Он не импортирует oracle claude. Максимальная разница этого пересчёта с JSON — 1.4211·10⁻¹⁴, agora #2237.
Это дополнительная архивная проверка: файл вне checkout в этой сессии только читался и не поставляется
с репозиторием, поэтому пересчёт **не воспроизводится из одного checkout**. Основная проверка Enforce
против сохранённого JSON использует файлы репозитория и повторяется командой ниже.

## Итоговые логи

Папки относительно `Documents/My Games/ArmaReforgerWorkbench/logs/`, файл `console.log`.

| Папка | Назначение | SHA-256 |
|---|---|---|
| `ricon-physics-r21-controls-20261021-000500` | 7 точек, 5 контролей полного компаратора, exit 0, SCRIPT(E)=0, done PASS | `533e321e7a04b18bd1f6880af62ba06ea3e327814c18c9315b1106e73abb4560` |
| `ricon-tests-r21-regression-20261020-235119` | прежние наборы, exit 0, SCRIPT(E)=0, done PASS | `c9fff2c9e362a53b74c110c8ec614ff2fb3a38cfab1b2417d6d73dd4ee2aa8d8` |

Вторым прогоном подтверждены lifecycle 209/209, EXT PASS, VoN seam 103/103 и L2 45/45 (прежний максимум
0.000102997 дБ). Полный host-набор — 109/109; CLI контрольного JSON — `LINK_PHYSICS_ORACLE_PASS`.
Уровень выбранных физических точек не расширяет прежние runtime-гейты.

Первый успешный прогон до уточнения формата логов: `ricon-physics-r21-20261020-234845`, SHA
`f7b411c80e898626af729591c272767a6673f158448f273c465610b61a7f7ef8`, CRC `7aeb5dd2`.
Промежуточный прогон с девятью знаками: `ricon-physics-r21-final-20261020-235015`, SHA
`0d61df0c1c8e81fb164bb4dc893e5741a1aae69d36fa5a3f075c6c2d4fb07f9d`, CRC `d3a19d33`.
Оба прежних прогона имели только три вспомогательных контроля `Within`; по ревью agora #2259 они заменены
пятью контролями полного `ComparePoint`. Финальный лог в таблице относится к усиленной проверке.
Предварительная компиляция helper-методов была отдельной проверкой синтаксиса и не считалась исполнением точек.

## Числа из финального Enforce-лога

Потери — дБ; мощности/шум — дБм; SINR — дБ. Состояние и Q только TEST_ONLY.

| Точка | Потери | Prx | Шум | SINR | Состояние | Q | noiseModelInDomain |
|---|---:|---:|---:|---:|---|---:|---|
| P01 | 118.317733765 | −81.328033447 | −115.143310547 | 33.815277100 | CLEAN | 1.000000000 | true |
| P02 | 140.814834595 | −103.825134277 | −115.143310547 | 11.318176270 | CLEAN | 0.877272546 | true |
| P03 | 87.331947327 | −57.331947327 | −103.621040344 | 46.289093018 | CLEAN | 1.000000000 | true |
| P04 | 113.819442749 | −83.819442749 | −103.621040344 | 19.801597595 | CLEAN | 1.000000000 | true |
| P05 | 81.168861389 | −44.179161072 | −100.215942383 | 56.036781311 | CLEAN | 1.000000000 | true |
| P06 | 120.021240234 | −83.031539917 | −119.995788574 | 36.964248657 | CLEAN | 1.000000000 | false |
| P07 | 85.000000000 | −105.000000000 | −94.101974487 | −10.898025513 | WEAK | 0.136732489 | true |

Внутреннее сравнение Enforce использует ожидаемые литералы, которые компилятор тоже округляет в float32:
максимум `0.000013351 дБ`. Поэтому числа actual из лога дополнительно сверены отдельным Python-разбором
с исходными float64-значениями JSON (55 значений в дБ, 7 расстояний, 7 Q и точные состояния/флаги):

- максимум в дБ: **0.0000142337097166** (P04, SINR);
- максимум 3D-дистанции: **0.000474245231 м** (P07; идеальная координата JSON округлена в float32);
- максимум TEST_ONLY Q: **0.000000282125213** (P02).

Девять десятичных знаков в логе ограничивают точность этого дополнительного разбора примерно 5·10⁻¹⁰
на значение. Это существенно меньше допусков. Не следует называть внутренние 0.000013351 сравнением
с неокруглённым исходным JSON.

## Как повторить

Из корня проекта, Steam запущен. Используется прежний проект TestRunner, но отдельное имя plugin:

```powershell
$steam = 'C:\Program Files (x86)\Steam\steamapps\common'
$wb = "$steam\Arma Reforger Tools\Workbench\ArmaReforgerWorkbenchSteamDiag.exe"
$gp = Join-Path (Get-Location) 'tools\RICON_TestRunner\addon.gproj'
$addons = "$steam\Arma Reforger\addons,$steam\Arma Reforger Tools\Workbench\addons,$(Join-Path (Get-Location) 'addons')"
$logs = 'ricon-physics-repeat-' + (Get-Date -Format 'yyyyMMdd-HHmmss')
$argv = @('-gproj', "`"$gp`"", '-addonsDir', "`"$addons`"", '-noBackend', '-wbModule=ScriptEditor',
  '-plugin=RICON_LinkPhysicsProbePlugin', '-wbSilent', '-exitAfterInit', '-noThrow',
  '-logLevel', 'verbose', '-logsDir', $logs, '-log-scr-checksum')
$process = Start-Process -FilePath $wb -ArgumentList $argv -WindowStyle Hidden -PassThru
$null = $process.Handle
if (-not $process.WaitForExit(300000)) { Stop-Process -Id $process.Id; throw 'Таймаут Workbench' }
$process.ExitCode
$log = "$env:USERPROFILE\Documents\My Games\ArmaReforgerWorkbench\logs\$logs\console.log"
Select-String -LiteralPath $log -Pattern 'RICON_PHYSICS|RICON_TEST.*FAIL|SCRIPT.*\(E\)'
```

PASS требует exit 0, отсутствие SCRIPT(E), правильный source SHA/CRC, `cases=7 controls=5` и `done: PASS`.
Один exit 0 недостаточен. Старые наборы: `tools/RICON_TestRunner/run.ps1 -Name r21-regression`.
Host: `python -B -m unittest discover -s oracle -p 'test_*.py'`;
`python -B -I oracle/link_physics_oracle.py --check oracle/vectors/link_probe_points_v1.json`.

## Что остаётся открытым

Это семь выбранных точек, а не весь диапазон, не полный старый набор Э2 и не все ветки LOST/DEGRADED.
Матрица 48 сценариев остаётся host ORACLE. Two-ray в Core отсутствует, её Enforce-паритет не заявляется.
Реальный профиль из мира, параметры грунта, ошибки моделей относительно реальной радиосвязи,
native quality, доставка голоса, звук, authority/Rpl/JIP и нагрузка этим тестом не проверены.
P06 демонстрирует политику при отсутствии модели шума, а не правильное значение внешнего шума 400 МГц.
Фиксированные ожидания относятся к указанному SHA JSON; изменение JSON требует обновления/ревью литералов.
