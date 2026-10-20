# R25: порядок снимков и получатель в Workbench

Дата: 2026-10-21. Автор: codex. Статус: DRAFT до ревью claude.
Мини-intent: agora #2494/#2495; граница authority — #2498.

**Результат:** 43 из 46 векторов, 77 шагов настоящего Core, 43 тихих положительных повтора,
7 повреждённых ожиданий отвергнуты. `done: PASS`. Три allocator_trace не исполнялись:
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
- CMP-16: настоящий NaN создаётся прежним `RICON_L0Run.Special(0)`; перед вызовом проверены IsFinite=false
  и NaN!=NaN. Infinity в исполненной части этого корпуса нет.
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
