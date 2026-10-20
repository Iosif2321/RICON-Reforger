# R25: порядок снимков и получатель в Workbench

Дата: 2026-10-21. Автор: codex. Статус: AGENT-REVIEWED, claude agora #2534/#2539 (b6ccc7a + c9c58f9).
Мини-intent: agora #2494/#2495; граница authority — #2498.

**Результат:** 43 из 46 векторов, 77 шагов настоящего Core, 43 тихих положительных повтора,
7 повреждённых ожиданий и 3 дополнительных контроля специальных значений отвергнуты. `done: PASS`. Три allocator_trace не исполнялись:
`BLOCKED_NO_AUTHORITY_CONTEXT`. Снимки не доказывают доставку или слышимость голоса.

## Корпус и охват

`oracle/vectors/snapshot_v1.json`, revision 9, SHA-256
`3440982cea0cce21ef857e09a1db3dbe77a5c85561771212dd9ea1199e93721c`.
Ожидания claude написаны из REQ-core §8; Enforce-реализация codex. Генератор принимает только эти байты
до разбора и до записи. Он копирует ожидания и не рассчитывает ordering/TTL/resync вместо Core.

- 26 compare: настоящий `RICON_LinkSnapshot.CompareAgainst`, решения и неизменность обоих входов.
- 17 receiver_trace, 51 шаг: настоящие `ApplyFullResync`, `Receive`, `IsVisible`.
  Сверены decision, REQUEST_RESYNC, DIAGNOSTIC, точное множество linkId/seq, visible, неизменность входов.
  Тестовый наследник receiver только открывает количество/ключи защищённой карты для наблюдения;
  вычислительные методы не переопределяет. Имена параметров конструктора сохранены для inherited prototype.
- TTL — только локальная видимость. Это не время серверной аренды, Rpl/JIP или работа сетевого транспорта.
- CMP-16: NaN создаётся прежним `RICON_L0Run.Special(0)`; конечный guard использует общий
  `RICON_TestSupport.FloatClass(value)==1`. Прежний guard IsFinite=false и value!=value не различал NaN/Infinity
  в этой сборке Workbench; исправление и отдельный отрицательный контроль приведены ниже.
  Infinity в исходной исполненной части корпуса нет, но он участвует в дополнительных контролях.
- CMP-17: неизвестный строковый BROKEN отображён в недопустимый enum -1, `ADAPTER_MAPPED=1`.
  Это явное ограничение типизированного адаптера, не получение такой строки из настоящей сети.

Для сравнения/копий входов повторно используется прежний `RICON_VoNSeamRun` через статический доступ
к защищённым helper; старый helper не изменён. Сами snapshots держатся в локальных переменных на весь случай.
После каждого правильного случая тот же путь проходит в quiet-режиме; quiet подавляет только лог.

Семь контролей повреждают ожидаемые поля: compare decision, REQUEST_RESYNC, diagnostic при отрицательном
elapsed, visible до TTL, число слотов, seq слота, diagnostic INVALID_RESYNC. Все проходят обычные Case-методы
и обычный компаратор; PASS требует точных 43/77/43/7, пустой прогон не принимается.

## Почему 3 allocator остаются открытыми

ALLOC-01/02/03 обращаются к выдаче seq и смене epoch. Production-путь находится в
`RICON_CommsManagerComponent.c`: static IsAuthority (:113–116) → Replication.IsServer();
NextLinkSequence (:290–315) требует authority, RotateSessionEpoch (:277–287) вызывает процессный allocator
(:333–347), который может использовать System.GetUnixTime(). Это game mode component, а не чистый receiver.
Тест не создаёт игровой мир, не меняет authority-проверку и не копирует seq-формулу вместо её исполнения.
На текущем разрешённом стенде эти 3 сценария не проверены. Также fullResync-эффект не экспортируется из
NextLinkSequence; его доставка остаётся отдельной проверкой. Полного `ENFORCE_PARITY 46/46` здесь нет.

## Прогоны

База логов: `Documents/My Games/ArmaReforgerWorkbench/logs/` текущего пользователя.

| Лог console.log | Результат | SHA-256 |
|---|---|---|
| `ricon-snapshot-r25-20261021-012102` | compile FAIL: имя аргумента конструктора schema отличается от supportedSchemaVersion | `3683fd996fa99abd01662e1373f5beeb49dd08293ae5fb2ece12bd5e113e3c4c` |
| `ricon-snapshot-r25-20261021-012118` | compile FAIL: оставшийся вызов удалённого helper CheckSpecial | `c675884c1983c9f38b463660cd1241b1f1c7ab5c063e2fecf7305df45daed6e5` |
| `ricon-snapshot-r25-20261021-012141` | exit 0, SCRIPT(E)=0, FAIL=0; 43/77/43/7; done PASS | `03dae7574479766521866c9f3f291ad2f7ceee82fb85d31651be2b3b8804f7af` |

Оба compileFAIL также вернули exit 0: как и прежде, только код завершения не является PASS.
Отдельный парсер сверил все 77 уникальных (id, step) в конечном логе с исходным JSON:
решения, эффекты, количество слотов и visible. Seq и точный набор linkId проверены самим Enforce-компаратором;
поштучные seq в raw-лог не экспортировались.

Базовые исходники `b6ccc7a` до находки float-сравнений (конечная дельта ниже):

| Файл | SHA-256 | Workbench CRC |
|---|---|---|
| `gen_snapshot_fixture.py` | `343cd18e113401b62ae6a901034d6daa0afa3dd5a1ec07c954e266cc1e2ca37a` | host |
| `test_gen_snapshot_fixture.py` | `30cee832a67f6e289d551d54b7a5eac5f0fb2ad6989288de9924d671d50d5e53` | host |
| `RICON_SnapshotFixtureSupport.c` | `e04eaf0ad4c57a624800422dcd3cd2db71a7ea98b30dba9a424e89981fdb1d61` | `bcdb7f2a` |
| `RICON_SnapshotFixtures.c` | `5916e51832dc82d7288881d63efe22e573b32597239e0dab463ffbba5f3c21ec` | `15928c6d` |
| `RICON_SnapshotProbePlugin.c` | `56a585dd3a85a4e346f25040282f09752ef2826ba4c5b0e3e6169781d48f0547` | `11ac72c6` |
| настоящий `RICON_LinkSnapshot.c` | `53d99d9710ddc1a4dc4617046cb2a3496e7768d2d76920a8a69a276caf39724c` | `2c666caa` |

Core не менялся относительно `3c295ce`; старые helpers, runner и исходный JSON не редактировались.
Host: 3/3 новых теста, весь доступный runner-корпус 30/30 в момент проверки (включая параллельный route-блок),
snapshot oracle 46/46; --check точно совпал. Тесты проверяют SHA-отказ, точную генерацию и сохранение файла при отказе.

## Воспроизведение

```powershell
python -B -I oracle/snapshot_oracle.py --check oracle/vectors/snapshot_v1.json
python -B -m unittest discover -s tools/RICON_TestRunner -p 'test_gen_snapshot_fixture.py'
python -B -I tools/RICON_TestRunner/gen_snapshot_fixture.py oracle/vectors/snapshot_v1.json addons/RICON_CoreTests/Scripts/Game/RICON_Tests/RICON_SnapshotFixtures.c --check
```

Workbench — команда из `E-physics-enforce.md`, plugin `RICON_SnapshotProbePlugin`, уникальное имя logsDir.
Требуются CRC из таблицы, 0 SCRIPT(E)/FAIL, cases=43 steps=77 quietPositive=43 controls=7
blockedAllocator=3 adapterMapped=1 и done PASS. Play, мир, сервер и native VoN не вызываются.

Открыто: allocator3, restart/collision epoch, транспорт, authority, Rpl/JIP, звук и несколько игроков.

## Конечная дельта R25: NaN и Infinity

Agora #2505/#2515/#2516/#2521/#2532. Независимый эксперимент маршрутов воспроизвёл особенность проверенных
операторов этой сборки Windows Workbench: value!=value истинно и для ±Infinity. Поэтому IEEE-приём
не доказывал вид специального значения. Core по-прежнему отвергает все nonfinite через IsFinite;
его алгоритмы и исходный JSON не менялись. Исправлена достоверность тестового guard/сравнения входов.

Классификатор и SameFloat находятся в общем `RICON_TestSupport` от claude, `f5e424e`:
сначала IsFinite; только у nonfinite вычисляется 1/value — у Infinity результат конечен, у NaN нет.
Направление Infinity различается по знаку. Строковое представление в алгоритме не используется.
SnapshotInput.Same использует исправленный прежний SameSnapshot; RequireNaN теперь проверяет класс 1.
Три дополнительных контроля проходят полное сравнение снимков/guard: NaN→+Infinity, −Infinity→+Infinity,
Infinity вместо ожидаемого NaN. Равные NaN и настоящий NaN-guard положительно приняты.

Конечный лог `ricon-snapshot-r25-20261021-013325/console.log`, SHA-256
`fba22c4c5af4243af2410ae3e460477f5b66f43b849f2667dc259808942e765c`:
exit 0, 0 SCRIPT(E)/FAIL, specialPositive=1 specialControls=3,
cases=43 steps=77 quietPositive=43 controls=7 blockedAllocator=3 adapterMapped=1, done PASS.
Повторный отдельный разбор всех 77 raw-шагов против исходного JSON — PASS.

| Изменённый или общий файл | SHA-256 | CRC |
|---|---|---|
| `RICON_SnapshotFixtureSupport.c` | `751bec2d946f802bbd1cf50d356503abfe34244d857dc9bc82b14c15fec49209` | `930b4fd0` |
| `RICON_SnapshotProbePlugin.c` | `8fd51f374b3a7ddb932593e3b9db2667059861876d42ff3b7273e5f72a1db108` | `79356e00` |
| `RICON_TestSupport.c` @f5e424e | `d3741f72de00c5e7f67930551d58fbcd221e50e9ad045628d71ebca54448ae5e` | `e20579c3` |
| `RICON_VoNSeamFixtureSupport.c` @f5e424e | `5ddcddb058273e1ccbec5452830ce53bde958e687ee8737e9059f362da8c648d` | `4e5fb58c` |

Остальные исходники/CRC из базовой таблицы не менялись. Конечный PASS дополнительно требует
specialPositive=1 specialControls=3 и новые CRC этой таблицы.

Независимый повтор claude: `ricon-snapshot-claude-review2-20261021-013619/console.log`, SHA-256
`175cb8cf339355ef2ac60a9100762124489055bdf5cc932744d6eb24473f19c6`.
Те же 43/77/43/7, specialPositive=1 specialControls=3, done PASS, 0 SCRIPT(E)/FAIL; exit 0 по отчёту #2539.
Лог и полный SHA перечитаны codex. Финальное agree #2539; независимый raw-парсер 77/77 — #2534.
