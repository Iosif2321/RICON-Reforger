# R28: выбранные грунты и фазы профиля в Enforce

Дата: 2026-10-21. Автор: codex. Статус: DRAFT до ревью и независимого повтора claude.
Мини-intent: agora #2757/#2758; корпус согласован codex #2766, кодовый fix oracle — #2774.

**Результат:** 16 выбранных входов `RICON_TerrainDiffractionCalculator` совпали с эталоном при исполнении
в Workbench без мира, Play и сервера. Это `ENFORCE_PARITY` на указанных входах. Параметры грунта не привязаны
к материалам карт, слышимость и физическая точность не проверены. Ядро не менялось.

## Входы и проверка

- Golden: `oracle/vectors/l2_sensitivity_anchors_v1.json`, 16 случаев, SHA-256
  `8e80dabe4c9450aa76e4fed5b947072dd319b327f0838b829bbbc752fc136271`.
- A01–A08: плоская земля, грунты P.527/Fig.24 и матрицы, вертикальная/горизонтальная поляризация.
- A09–A16: треугольные холмы; вершина на узле, между узлами и полностью пропущена сеткой; полуширина 30 м,
  100 м, 500 м и 1.9 км; опорный шаг 2 м. Путь 10 км, симметричные высоты антенн.
- Генератор только копирует данные принятого SHA. `pathKm` переводится в метры; плоский профиль — нулевая
  высота при ненулевой полуширине. Числа ожиданий не вычисляются генератором.
- Профиль строит настоящий `RICON_TerrainProfileSampling.BuildDistances`, высоты — заданный треугольник.
  `RICON_L2Run.Evaluate` проверяет неизменность профиля и конфига; `CheckOk` — код OK, L/Lba/Lbs/Lsph
  с допуском 0.05 дБ и четыре поля ветвей точно. Дополнительно: число точек точно, пик с допуском 0.001 м.
- У всех 16 есть повторное сравнение в quiet-режиме. 15 отрицательных контролей должны отказать:
  четыре потери +0.1 дБ, четыре неверных enum, неверный код, число точек +1, пик +0.01 м,
  NaN в фактической/ожидаемой L, null фактического/ожидаемого результата. Ожидаемые NaN отдельно отсекаются
  до общего helper. Перед обоими NaN-контролями общий `FloatClass` обязан вернуть класс 1 (NaN), иначе
  весь запуск FAIL. Для проверки этих NaN не используется сравнение `x != x`.

## Выполненные проверки

```powershell
python -B -m unittest discover -s tools/RICON_TestRunner -p 'test_*.py' -q
python -B -I tools/RICON_TestRunner/gen_sensitivity_points.py oracle/vectors/l2_sensitivity_anchors_v1.json tools/RICON_TestRunner/Scripts/WorkbenchGame/RICON_SensitivityPoints.c --check
python -B oracle/sensitivity_oracle.py --anchors
```

34 теста инструментов PASS, включая 4 новых теста генератора; oracle sensitivity — 24 PASS и
`SENSITIVITY_ANCHORS_PASS`. Все 16 golden-входов независимо пересчитаны codex старым калькулятором R19
(вне checkout, `C:/tmp/codex_l2_crosscheck.py`, SHA
`47f788146a7c1d2a923aa3fc853327bcdf6db45f1fb0bf74d621413642914b35`): 64 числа, расхождение ≤ 7.11e-15 дБ,
ветви совпали точно. Это независимая сверка, но внешняя ссылка не делает её переносимой командой из checkout.

Команда Workbench: обычный `tools/RICON_TestRunner/addon.gproj`, `-noBackend -wbModule=ScriptEditor
-plugin=RICON_SensitivityProbePlugin -wbSilent -exitAfterInit -noThrow -logLevel verbose -log-scr-checksum`.
Запуск скрытый, путь логов уникален; добавляются штатные каталоги addons игры, Tools и этого checkout.

Финальный лог: `ricon-sensitivity-r28-20261021-025636/console.log`, полный путь под
`C:/Users/Ded/Documents/My Games/ArmaReforgerWorkbench/logs/`.
SHA-256: `01de13966b342f29a1c00660f92a4f30867ee24949fd016ec29c29a8c7df04e9`.

- exit 0; `cases=16 quietPositive=16 controls=15`, `done: PASS` ровно один раз;
- 0 `SCRIPT (E)`, 0 неожиданных `RICON_TEST FAIL`;
- внутренний максимум: 0.000045776 дБ (сравнение float32);
- независимый разбор 64 фактических чисел против JSON: максимум 0.0000439903063 дБ;
- 16 счётчиков профиля точны, максимум расхождения пика: 0.00009346 м;
- все 16 строк с четырьмя полями ветвей совпали; ожидаемые числа лога сверены с float32-округлением JSON.

| Файл | SHA-256 | CRC в логе |
|---|---|---|
| RICON_SensitivityProbePlugin.c | `bd0a0bc6c18f6620e0d6ec621d7c96c27af0fbb9f6daa6f3462eb1c4ca473ca1` | `ca8695bc` |
| RICON_SensitivityPoints.c | `6d982a6414fbc2a05565a6a04e6e9bddf6779e49085ef091fc832d551e9b180c` | `1d750a3f` |

Общий `RICON_L2FixtureSupport.c` — CRC `1072ff8c`, Core `RICON_TerrainDiffractionModel.c` — `e999988a`.
Первый PASS до усиления проверки создания NaN сохранён отдельно: `ricon-sensitivity-r28-20261021-025412`,
SHA `0e909f941383d070a8d429db76e1c99eaf2e904f76a6e60324ef6eabaea10976`, CRC probe `107fe950`.
Независимый повтор claude пока не выполнен. Весь свип чувствительности и границы дальности этим стендом
не проверяются: здесь только 16 опорных точек и диагностика построенного профиля.
