# R25: корпус маршрутов в Workbench

Дата: 2026-10-21. Автор: claude. Статус: DRAFT до ревью codex.
Мини-intent и владение — agora #2495 (agree codex #2496).

**Результат:** все 48 векторов `oracle/vectors/route_v1.json` исполнены настоящим `RICON_RouteContract.Evaluate` /
`RICON_Route.Evaluate` (REQ-core §9), `done: PASS`. Это `ENFORCE_PARITY` проверки маршрута. Play, мир и сервер не
запускались. Маршрут здесь — контракт проверки цепочки ног и итог «минимум качества, худшее состояние», а не перенос
голоса ретранслятором.

## Вход и граница

- Корпус: `route_v1.json` revision 4, источник `docs/req/REQ-core-contracts.md@f08e20d`, ожидания написаны claude
  вручную по REQ (не по коду). SHA-256 `efccccb536a362cc9b2346a73138b7fc3444f159be19ad24ac172a18e25f43fd`.
- Код маршрутов — codex, `addons/RICON_Core/Scripts/Game/RICON/Network/RICON_NetworkContracts.c`.
  `git diff --quiet 3c295ce -- addons/RICON_Core` → 0.
- `gen_route_fixture.py` принимает только эти байты по SHA, до разбора. Числа копируются через
  `gen_l2_fixture.literal`, ничего не вычисляется. Закрытая проверка ключей, int32, имён причин и состояний.
- Сравнение:
  - `reason` и `state` — точно;
  - `quality` — допуск 1e-6 (`tolerances.ratio` корпуса).
  Плюс входы маршрута (концы, ноги, бюджеты, включая NaN) после вызова не изменены.

**Отображения, которые типизированный API не несёт как есть:**
- `ADAPTER_MAPPED` = 2:
  - R-46 — объект маршрута без ключа `legs` → маршрут без ног;
  - R-23 — неизвестное имя состояния `BROKEN` → enum −1.
- `TEST_SUBCLASS` = 3 (R-15, R-31, R-34): null-нога. Публичный `AddLeg` null отвергает, а REQ-core §9.1 требует
  `NULL_LEG`. Тестовый наследник `RICON_TestRoute` вставляет null через защищённый массив, и проверку делает
  настоящий `Evaluate`.
- NaN в R-22 — настоящий NaN Enforce (`RICON_L2Run.NaN()`).
- `route: null` (R-45) передаётся в `RICON_RouteContract.Evaluate` как null.

**Контроли:** 4 тихих отрицательных на первом векторе. Каждый портит одно поле ожидания (причина, качество + 2·допуск,
состояние, NaN в фактическом качестве), и полное сравнение отвергает каждый. Все 48 векторов дополнительно проходят
то же сравнение в тихом режиме. Пустой прогон PASS не даёт: нужны `cases=48 quietPositive=48 controls=4`.

## Логи и исходники

Папки относительно `Documents/My Games/ArmaReforgerWorkbench/logs/`, файл `console.log`.

| Папка | Факт | SHA-256 |
|---|---|---|
| `ricon-route-r25-20261021-012131` | первый запуск: модуль не скомпилирован из-за черновика snapshot-файла codex в общем рабочем дереве (не мои файлы), exit 0 — ложный | `a82f37db3f26ef57a24a1d8c7380d555882e9c5ef328bc1487c71fc9e65c70b8` |
| `ricon-route-r25-20261021-012312` | 48/48, quietPositive 48, controls 4, adapterMapped 2, testSubclass 3, SCRIPT(E) 0, `done: PASS` | `b28f79f03e8e9ebec2e2b01904153f68716156107fdd322787296d516b7db92e` |
| `ricon-tests-r25-route-regression-20261021-012331` | прежние наборы `run.ps1`: lifecycle 209/209, EXT, VoN seam 103/103, L2 45/45, SCRIPT(E) 0, PASS | `7d672d2eb8e01c0031652e16cd91adf011079a56f5d8865ced11c597d11ff7d5` |

| Файл | SHA-256 | CRC в Workbench |
|---|---|---|
| `tools/RICON_TestRunner/gen_route_fixture.py` | `c67e9eb26d42e16810639b7e6f14bfe82a83123f5c8568078dd102dc4a6d1259` | host |
| `tools/RICON_TestRunner/test_gen_route_fixture.py` | `544b11a9343c204c8cf385a590f40c358de57f98e2a511c8e2bbff13fe4cf9d8` | host |
| `addons/RICON_CoreTests/.../RICON_RouteFixtureSupport.c` | `8149e34904e1d7ba0539dfebce93e7ac87f7e6de35a67a218a77898aba3c6c1e` | `7366c3b1` |
| `addons/RICON_CoreTests/.../RICON_RouteFixtures.c` (сгенерирован) | `dc196fa5e73f81c592f94cffc5b2d6b4d8be636203a7d5202d38c01c9a6b8921` | `388b2982` |
| `tools/RICON_TestRunner/Scripts/WorkbenchGame/RICON_RouteProbePlugin.c` | `87e36bc515c3cb33a8341dd49dca46b8c22a7c9d166a469754158ad4edccb35b` | `3e4196ba` |

Отдельный разбор лога (свой скрипт claude) против `expected` корпуса: 48 строк по 48 разным id, расхождений reason
и state — 0, наибольшее |Δquality| = 2.4·10⁻⁸.

## Как повторить

```
python -B -I tools/RICON_TestRunner/gen_route_fixture.py oracle/vectors/route_v1.json addons/RICON_CoreTests/Scripts/Game/RICON_Tests/RICON_RouteFixtures.c --check
python -B -m unittest discover -s tools/RICON_TestRunner -p "test_gen_route*.py"
```

Workbench — команда из `docs/evidence/E-physics-enforce.md` с `-plugin=RICON_RouteProbePlugin`. В логе обязательны
`source=efccccb5…`, `cases=48 quietPositive=48 controls=4`, CRC трёх Enforce-файлов из таблицы, 0 SCRIPT(E) и
`done: PASS`. Exit 0 сам по себе ничего не доказывает (первый запуск выше).

## Что остаётся открытым

- Поиск маршрута политикой сети (`RICON_RoutingPolicy.FindRoute`) этим не проверен: проверяется готовый маршрут.
- Ретрансляция голоса и доставка — runtime, исполнителя нет.
- Политика доступа здесь — тестовая «разрешить всё»: контекст проверяет только её наличие (REQ-core §9.2).
