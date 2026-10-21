# S1 — источник высот → профиль → L2 → бюджет связи

Дата: 2026-10-21. Автор: codex. Статус: AGENT-REVIEWED, claude agora #3084; сырые peer-логи проверены codex.
Основание: owner agora #2937, разделение #2946/#2952, REQ v0.2 и эталон `c6acd32` + `a089d82`.
Источник истины: `REQ-L2-world-profile.md`, `REQ-L2-terrain.md` §6 и `REQ-link-eval.md`.
Это продолжение разработки после закрытой серии R1–R30, а не повтор её приёмки.
REQ v0.3 (`2162102`) уточняет только граничные проверки оболочки `BaseWorld`; коды, геометрия и корпус остаются теми же.

## Результат и граница

Добавлен `RICON_WorldProfileProvider`: строит профиль из `RICON_HeightSource`, используя существующий
`RICON_TerrainProfileSampling.BuildDistances`. Первая и последняя выборки получают ровно координаты антенн;
высоты антенн над грунтом вычисляются отдельно. Отсутствие входа, нечисловая позиция, превышение ёмкости,
непригодная выборка или нечисловая высота антенны дают `TERRAIN_PROFILE_UNAVAILABLE` с очищенными выходами.
Конечные значения вне физических доменов проверяет существующий L2, без повторения его правил.

`RICON_BaseWorldHeightSource` — тонкая оболочка `BaseWorld.GetSurfaceY(x,z)`. Отсутствующий мир возвращает
отказ. Метод с настоящим миром **не вызывался**; регистрация в игровом менеджере и рабочий runtime-поставщик
ещё не подтверждены. Оболочка скомпилирована вместе с pure-стендом, без Play/мира/сервера.

Вода — `NOT_MODELLED`, ε/σ не выводятся из поверхности. Сырые высоты нельзя считать доказанным профилем
«суша—море»: смысл `GetSurfaceY` над водой, объектами и за границей карты остаётся вопросом опыта.
API подтверждён установленной `EnfusionScriptAPI/html/interfaceBaseWorld.html:128` и
[официальной страницей BaseWorld](https://community.bistudio.com/wikidata/external-data/arma-reforger/EnfusionScriptAPIPublic/interfaceBaseWorld.html).
Сигнатура не описывает эти семантические свойства.

## Входы и исполняемые файлы

Генератор копирует ожидания, а не вычисляет их; принимает только согласованные байты корпуса:

- `oracle/vectors/world_profile_v1.json`, revision 2: **39 случаев** (22 model, 17 provider);
- SHA-256: `56c6a148236325183af8117d838ed1a22e12316642d9946c3b3ab57be48b5b1e`;
- классы NaN/+Inf/−Inf проверяются в Enforce до использования специальных входов;
- model-случаи вызывают настоящий `RICON_TerrainDiffractionModel.AppendLosses`;
- test-subclass только наблюдает вызов поставщика и выходы, его решение не подменяет;
- profile-случаи вызывают настоящий `GetProfile` напрямую, включая null и нечисловые позиции.

| Файл | SHA-256 | CRC свежего Workbench |
|---|---|---|
| `RICON_WorldProfileProvider.c` | `9958268ac2fbbb44d502c366934be8ea136e79d72f78c2dfd8267411aaf48272` | `4f482ec8` |
| `RICON_WorldProfileFixtureSupport.c` | `a4995f398f106e5c8229fce1afce43debde760375f4c5079eb35df2159398874` | `a9249f93` |
| `RICON_WorldProfileFixtures.c` | `ded05c2bf1863ed9be7fe4a35e08bb6f0c58bae3dbcdedc1a43bac065d522185` | `fbcb3cfc` |
| `RICON_WorldProfileProbePlugin.c` | `19da459c03e553d9f75051b1e43a35bd3bb9b18825dc3919bd71434ad9a09b5b` | `7e5a1335` |
| `gen_world_profile_fixture.py` | `bddd87ceff36be6bc508e274a0741de004f90f50866c4db2db46d237fba03ca8` | — |
| `test_gen_world_profile_fixture.py` | `b41a99c5f50e565d908ed0ecd16d76a291347185aca553b7a00f41666819ef76` | — |

## Собственные проверки codex

| Проверка | Результат |
|---|---|
| Новый oracle checker | `WORLD_PROFILE_ORACLE_PASS: 39 cases` |
| Новые oracle unit tests | 15 OK |
| Генератор и его проверки границ | exact generation 39; 5 тестов OK |
| Все tool unit tests после добавления генератора | 39 OK |
| Существующий L2 oracle до правки | 16 тестов OK |
| Независимый пересчёт геометрии без импорта `world_profile_oracle` | 39 случаев, 489 точек; max `1.27897692437e-13` по координате профиля/высоте/AGL |
| Pure Workbench, финальный стенд | 39/39, quietPositive 39, controls 14, endpoints 1, budget 1 |
| Сырой лог против JSON | 39 заголовков/кодов/счётчиков, 489 точек, 1080 чисел; все в допусках |
| Отдельный полный RF-бюджет | 8 сырых чисел; max `9.300988750737815e-6` дБ по семи значениям в дБ; Q=1 точно |
| Существующий TestRunner | lifecycle, extension, von_seam, l2 — PASS; Core provider тот же SHA/CRC |

Независимый геометрический пересчёт использовал `hypot`, барицентрическую запись внутренних координат,
аналитическую плоскость/гауссовы холмы, явные концы и собственную проверку ёмкости/отказов. Повторного
независимого вывода формул P.526 в S1 нет: эта часть использует ранее проверенные L2/FSPL-эталоны.

Максимальные расхождения **сырых** чисел Enforce с JSON:

- дистанция профиля: `9.60329376909e-7` км; допуск `1e-6·(1+d)` км;
- высота профиля/антенны: `5.30278577173e-5` м; допуск 0.01 м;
- потери слоя: `5.49625292479e-6` дБ; допуск 0.05 дБ.

14 отрицательных контролей обнаруживают неправильные счётчик, причину поставщика/модели, обе высоты антенн,
высоту/дистанцию/размер профиля, потери, факт вызова, null actual/expected, NaN и потерю разрядов координат RX.
Отдельная положительная регрессия: TX `(13000,2,10000)` → RX `(0.01,2,0.1)`, три выборки; скалярные компоненты
концов совпадают точно. Мутант RX `(0.009765625,0,0.099609375)` отвергается тем же проверяющим методом.

Проверка бюджета использует W01: ровная земля 100 м, антенны 101.5 м, дистанция 2 км, 50 МГц; дополнительно
**TEST_ONLY** TX 37 дБм, gain=0, B=12500 Гц, NF=8 дБ, required SINR=10 дБ, среда `none`, без помех.
Ожидания: сумма потерь 108.1797261261363 дБ, Prx −71.1797261261363 дБм, thermal −133.00608706414755 дБм,
noise=N+I=−125.00608706414755 дБм, SINR 53.82636093801125 дБ, sensitivity −115.00608706414755 дБм.
Q=1/CLEAN относится к `linear_test`, а не к измеренной речевой разборчивости.

## Логи и воспроизводимость

База логов: `%USERPROFILE%/Documents/My Games/ArmaReforgerWorkbench/logs/`.

- **Финальный стенд:** `ricon-world-profile-codex-s1-20261021-103027/console.log`,
  SHA-256 `daba7a6d08a6c73758bfe69f7b8c5a77c88f54af061b7e45dda8e62a5bd772b4`.
  Exit 0, SCRIPT(E)=0, tagged FAIL=0, ровно один `[RICON_WORLD] done: PASS`.
- **Существующий TestRunner:** `ricon-main-codex-s1-20261021-102733/console.log`,
  SHA-256 `b27723c28d7424dcf6493cb3084034ea78755b66dc332f1d0ab84266abb46f8e`.
  Он исполнен до добавления дополнительной проверки бюджета в тестовый support; производственный код
  provider и старые suites после этого не менялись. Exit 0, SCRIPT(E)=0, tagged FAIL=0, donePASS=1.
- Ранее успешный промежуточный стенд 39/14/endpoints1:
  `ricon-world-profile-codex-s1-20261021-102632/console.log`,
  SHA-256 `c3a83d3437d652de60e02d4b4ebdbd6846857398e7a5013ec662435b14b63b7d`.
- Ранний compile **FAIL**: `ricon-world-profile-codex-s1-20261021-102122/console.log`,
  SHA-256 `9bf4aeea18eb68e7e1a03e20bc52c661ab9031dfb82881214344d5171a4875e1`.
  `Math.Exp` и `array[index] +=` отвергнуты компилятором. Исправлены только тестовые вычисления/операторы;
  этот запуск не засчитывается как PASS. Даже при compile error Workbench вернул exit 0.

Host-команды из корня:

```powershell
python -B -I oracle/world_profile_oracle.py --check oracle/vectors/world_profile_v1.json
python -B -m unittest discover -s oracle -p test_world_profile_oracle.py
python -B -I tools/RICON_TestRunner/gen_world_profile_fixture.py oracle/vectors/world_profile_v1.json addons/RICON_CoreTests/Scripts/Game/RICON_Tests/RICON_WorldProfileFixtures.c --check
python -B -m unittest discover -s tools/RICON_TestRunner -p 'test_*.py'
```

Workbench: тот же `tools/RICON_TestRunner/addon.gproj` и `-addonsDir`, что в `regress_wb.ps1`; плагин
`RICON_WorldProfileProbePlugin`, только `-wbModule=ScriptEditor -noBackend -wbSilent -exitAfterInit -noThrow`,
`-logLevel verbose -log-scr-checksum`, новая уникальная `-logsDir`. Запуск через `Start-Process -WindowStyle Hidden`,
receipt собственного PID, ожидание не больше 180 с. Критерий: свежие CRC, код 0, SCRIPT(E)=0, tagged FAIL=0,
один donePASS и точные счётчики `cases=39 quietPositive=39 controls=14 endpoints=1 budget=1`.
Параметров запуска мира/Play/сервера нет. Ограниченный процесс без доступа к внешним логам не используется:
native Workbench запускался в обычном разрешённом окружении.

## Независимый повтор claude

Claude повторил проверки на дереве `2b5e4d9` и дал code review без блокирующих замечаний (agora #3084).

- `ricon-world-profile-claude-s1-20261021-115710/console.log`, SHA-256
  `7246aa7f8e14831eae7c3ebf519daa0c77b5587cefdd09a77d3a3798925571ad`.
  Exit 0, SCRIPT(E)=0, tagged FAIL=0, donePASS=1; 39 случаев, quietPositive=39, controls=14, endpoints=1, budget=1.
  CRC всех четырёх Enforce-файлов совпали с таблицей выше. Основной codex отдельно сверил все 1080 чисел
  профиля/AGL/потерь и восемь чисел бюджета с JSON и прежними ожиданиями; максимальные расхождения те же.
- `ricon-tests-main-claude-s1-20261021-115633/console.log`, SHA-256
  `0792f930dcfddc4da218b06b1c9e6d30ba398db952b2fa335d9667dccdd30259`.
  Exit 0, SCRIPT(E)=0, tagged FAIL=0, donePASS=1; lifecycle, extension, von_seam, l2 — PASS.
  Этот повтор выполнен уже после добавления проверки бюджета в support и закрывает оговорку раннего main-прогона.
  Основной codex проверил хэш, ошибки, четыре suite-маркера и единственный donePASS.

Это независимый повтор вычислений и ревью кода; приёмки игрового мира или звука он не даёт.

## Включение в общую регрессию

Коммит `c27207c` добавил S1 oracle CLI, byte-check генератора и WorldProfileProbe в существующие общие runners.
Изменение списков проверено codex, включая синтаксис PowerShell (agora #3113); формулы и production-код не менялись.

- Независимый запуск codex: `regress_host.py` — exit 0, **24/24 PASS**. Receipt:
  `%TEMP%/ricon-s1-host-codex-review-20261021-121035.txt`.
- Полный запуск claude: `regress_wb.ps1` — **8/8 PASS**, папки
  `ricon-r29-claude-20261021-120112-<plugin>/console.log` в прежней базе Workbench logs.
  Codex отдельно разобрал все восемь файлов: в каждом ровно один donePASS, SCRIPT(E)=0, tagged FAIL=0.
  JSON со всеми SHA и счётчиками: `%TEMP%/ricon-s1-wb-codex-review-20261021-1210.json`.
- Новый полный WorldProfile log SHA-256:
  `fe271fb139053c6cedbc7441e165a72011a70f5ea29e0dabd546425086d5a1be`.
  CRC четырёх Enforce-файлов и счётчики 39/39, controls=14, endpoints=1, budget=1 совпали.

S1 закрыт по чистому коду (agora #3114, `STATUS_RU.md` @ `2d03821`). Общая регрессия не является игровым опытом.

## Осталось проверить

- Реальный `GetSurfaceY`, высотный datum, границы карты, вода и фактическая точка антенны.
- Источник профиля в серверном lifecycle, привязка устройства и сбор запросов из настоящих раций.
- Число вызовов ограничено конфигом, но производительность 30/60 игроков не измерена; кэша в S1 нет.
- RPC/JIP, звук, per-listener управление и физическая калибровка. `BLOCKED_NO_ACTUATOR` не снят.
