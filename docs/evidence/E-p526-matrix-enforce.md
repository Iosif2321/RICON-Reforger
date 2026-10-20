# R24: P526 samples в Workbench

Дата: 2026-10-21. Автор: codex. Статус: DRAFT до ревью claude.
Мини-intent/владение — agora #2445/#2447.

**Результат:** 336 точек × 3 среды = 1008 расчётов настоящего Core, 1008 quiet-positive,
8 отрицательных контролей полного компаратора, `done: PASS`.
Из них 840 сравниваются с SNR в области принятого эталона, 168 — только с программным fallback RICON.
Это `ENFORCE_PARITY` перечисленных полей в выбранных точках. Дальности сеточного sweep,
все значения между узлами, физическая калибровка раций и слышимость этим не проверены.

## Вход и код

- `oracle/vectors/link_scenarios_v1.json`, revision 3, SHA-256
  `c325463b82cbeeabfc2366a7165070bf43a7bd7a7cfc1d18421d8b0bb3da2e7f`.
- Генератор принимает только эти байты до разбора и записи. Он копирует samples.lossDb/rxPowerDbm,
  noise[env].snrDb и логические признаки; физических формул в генераторе нет.
- Все 48 сценариев, по 7 горизонтальных дистанций: 100, 1000, 2000, 3000, 4000, 10000, 30000 м.
  Среды: none/rural/city. Частоты 30/50/88/400 МГц, мощности 1/5 Вт, высоты 1.5/3/10 м, ровно/холм.
- Новый `RICON_P526MatrixProbePlugin` наследует проверенный построитель EvaluatePoint из R21.
  `RICON_LinkPhysicsProbePlugin.c` не изменён относительно `570c7ef`; Core — относительно `3c295ce`.
  Новый data-класс содержит только сгенерированные вызовы. Старый runner и фикстуры не менялись.
- Построитель создаёт настоящие TerrainDiffractionModel / NoiseModel / WirelessLinkEvaluator, synthetic-профиль
  через BuildDistances и позиции с высотами грунта под обоими концами. Game/World/VoN API не вызываются.
- Параметры TEST_ONLY: B=25 кГц, NF=10 дБ, required SNR=10 дБ, усиления 0 дБи, ε=15, σ=0.005 С/м,
  вертикальная поляризация, Земля 6371 км, k=4/3, шаг профиля 0.1 км. Холм 60 м, центр 2 км,
  полуширина 0.5 км. Они совпадают с фиксированным построителем R21.

Поле samples.distanceM — **горизонтальная** дистанция, а не getter 3D-дистанции бюджета.
Верхняя точка холма при 2000 м поэтому получает корректную 3D-дистанцию между антеннами.
Прямой числовой паритет поля дистанции здесь не заявляется: в этой матрице сравниваются другие поля.

## Поля и контроли

Проверяются valid=true/reason=OK, конечность числовых полей бюджета, потери, Prx, SINR (0.05 дБ),
noiseModelInDomain, aboveSensitivity и meetsSnr (точно там, где эталон определяет SNR).
aboveSensitivity — порог приёмника, meetsSnr — условие SINR≥10 дБ; они не объединены в «голос доставлен».

168 случаев — 400 МГц, rural/city: физический эталон помечает шум среды как
NOISE_OUT_OF_MODEL_DOMAIN. Для них ожидаемый SINR **копируется из noise.none той же точки**:
это проверка реализации REQ-fallback `f_a=1`. noiseModelInDomain обязан быть false;
физический meetsSnr не определён и не сравнивается. Поле счётчика physicalSnr=840 означает только наличие
SNR-эталона при заданных модельных предположениях, а не подтверждение реальной физики.

State/Q речи не сравниваются: эта физическая матрица оставляет их DEFERRED/NOT_CALIBRATED.
Two-ray в Core не добавлялась и здесь не исполняется.

Каждая правильная строка проходит тот же компаратор ещё раз в quiet-режиме. В первой строке затем
намеренно повреждаются ожидания: loss +0.1 дБ, Prx +0.1, SINR +0.1, domain, aboveSensitivity,
meetsSnr, NaN в expected; отдельно NaN временно помещается в actual.totalLoss и после проверки восстанавливается.
Все восемь повреждений обязаны быть отвергнуты. Quiet подавляет вывод/сбор максимума, но не сравнения.
PASS требует всех 1008 положительных проверок и всех точных счётчиков, пустой прогон не проходит.

## Результат и воспроизводимость

Лог: `Documents/My Games/ArmaReforgerWorkbench/logs/ricon-p526-matrix-r24-20261021-010321/console.log`,
SHA-256 `57e54d5df5a2ceb369b3f121d464e9008ee014c6a2449f14290bfc05a852c393`.
exit 0, SCRIPT(E)=0, строк FAIL нет, cases=1008, quietPositive=1008, controls=8,
physicalSnr=840, fallback=168, done PASS. Play, мир и сервер не запускались.

Внутренний максимум против float32-литералов ожиданий — **0.000018120 дБ**.
Отдельный разбор **3024 actual** против исходного float64 JSON — **0.0000184858620376 дБ**
(S-30MHz-5W-h10-hill, 30000 м, rural, SINR). Девять десятичных знаков дают погрешность чтения до 5·10⁻¹⁰.
Все 1008 id уникальны; дополнительно сверены domain/aboveSensitivity и 840 meetsSnr.
У 168 fallback отдельно проверены false-domain и отсутствие физического SNR-сравнения.

| Файл | SHA-256 | Workbench CRC |
|---|---|---|
| `gen_p526_matrix.py` | `a1192e02f444eb37fd6399c34d84c539690c6c0c97a35dccd287d72c2e5505e3` | host |
| `test_gen_p526_matrix.py` | `4035980361dee81f2c168f80d87d0ee2416b999ed4473e6d391066f43b99d89f` | host |
| `RICON_P526MatrixProbePlugin.c` | `22e4a31a15bcd67085d59e2ec0a6b8566236de2471c7b03a13159ea105dd3625` | `9f6436d7` |
| `RICON_P526MatrixPoints.c` | `75b71b25b1ea7b301842d1b0a46d88098c8ddd4a49bffa0dee535ee5551bc551` | `3fb4fb58` |
| прежний `RICON_LinkPhysicsProbePlugin.c` | `b2cf41a57f28d8249af89cbe9ef84e67832c19341d8bb81de0ca2f5f04be3eb7` | `25a9894f` |

Host: 3/3 новых теста генератора, все runner/generator тесты 23/23; --check совпал с сохранённой фикстурой.
Проверены копирование ожиданий, ветвь none-reference, отказ изменённого SHA и сохранение прежнего файла при отказе.

Из корня проекта:

```powershell
python -B -m unittest discover -s tools/RICON_TestRunner -p 'test_*.py'
python -B -I tools/RICON_TestRunner/gen_p526_matrix.py oracle/vectors/link_scenarios_v1.json tools/RICON_TestRunner/Scripts/WorkbenchGame/RICON_P526MatrixPoints.c --check
```

Workbench запускается командой из `E-physics-enforce.md`, заменив plugin на
`RICON_P526MatrixProbePlugin`, а имя папки логов — на уникальное. Для PASS нужны полный счётчик/финальная строка,
CRC из таблицы, отсутствие SCRIPT(E)/FAIL. Exit 0 отдельно не принимается (см. первый отказ L0 в E-l0-enforce.md).
Числа печатает унаследованный CheckDb с тегом RICON_PHYSICS и id сценария, флаги и итог — RICON_P526.

## Что остаётся открытым

Enforce range-sweep; невидимые провалы между узлами; методы FSPL/two-ray вне выбранного покрытия;
реальные высоты мира, физическая корректность D-002, production-шум UHF, речевая калибровка, native качество,
доставка, звук, authority/Rpl/JIP и нагрузка в игре. Совпадение чисел не подтверждает слышимую связь.
