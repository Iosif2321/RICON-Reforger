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

**Контроли:**
- 4 тихих отрицательных на первом векторе. Каждый портит одно поле ожидания (причина, качество + 2·допуск,
  состояние, NaN в фактическом качестве), и полное сравнение отвергает каждый.
- Все 48 векторов дополнительно проходят то же сравнение в тихом режиме.
- Самопроверка снимка входов (codex #2505): NaN = NaN, +Inf = +Inf, но NaN ≠ +Inf, +Inf ≠ −Inf, 0.5 ≠ 0.5000001;
  классы float проверяются отдельно.
- Специальный вход R-22 до `Evaluate` подтверждается как настоящий NaN.

Пустой прогон PASS не даёт: нужны `cases=48 quietPositive=48 controls=4 snapshotControls=3`.

## Находка: сравнения float в Workbench не IEEE

Каждый прогон печатает таблицу (`RICON_RouteRun.PrintComparisonTable`):

| v | v<=0 | v>=0 | v<0 | v>0 | v==v | v!=v | IsFinite | строка |
|---|---|---|---|---|---|---|---|---|
| NaN (`Math.Sqrt(-1)`) | 0 | 1 | 0 | 1 | 0 | 1 | 0 | `-nan(ind)` |
| +Inf | 0 | 1 | 0 | 1 | 0 | 1 | 0 | `inf` |
| −Inf | 1 | 0 | 1 | 0 | 0 | 1 | 0 | `-inf` |
| 0.5 | 0 | 1 | 0 | 1 | 1 | 0 | 1 | `0.5` |

Выводы:
- Настоящий NaN сравнивается ровно как +Inf, а `x != x` истинно для любого неконечного значения. Значит, приём
  «`x != x` ⇒ NaN» в Enforce не различает NaN и бесконечность.
- Сначала я исправил проверку на `a != a && b != b`, как предложил codex в #2505. Самопроверка показала, что и
  этот вариант принимает NaN ↔ +Inf (прогоны `…012516`, `…012639`, `…012731` — FAIL).
- Рабочий способ — арифметика: `1/±Inf = 0` конечно, `1/NaN` остаётся NaN (`RICON_TestSupport.FloatClass`,
  общий помощник для всех тестов, codex #2516).
- `RICON_Num.IsFinite` в Core (`v <= MAX && v >= −MAX`) работает, потому что `NaN <= MAX` ложно. В Core NaN
  отсекается `IsFinite` до проверок диапазонов, поэтому код этой находкой не затронут.
- Тот же слабый приём был в моей seam-фикстуре (`RICON_VoNSeamFixtureSupport.c`, `RequireNonFinite` и
  `SameSnapshot`) — исправлен так же. Seam после правки 103/103 (`ricon-tests-r25-seam-floatclass-20261021-012938`).
  Он же есть в L0- и snapshot-поддержке codex (сообщено в #2515).

## Логи и исходники

Папки относительно `Documents/My Games/ArmaReforgerWorkbench/logs/`, файл `console.log`.

| Папка | Факт | SHA-256 |
|---|---|---|
| `ricon-route-r25-20261021-012131` | первый запуск: модуль не скомпилирован из-за черновика snapshot-файла codex в общем рабочем дереве (не мои файлы), exit 0 — ложный | `a82f37db3f26ef57a24a1d8c7380d555882e9c5ef328bc1487c71fc9e65c70b8` |
| `ricon-route-r25-20261021-012312` | 48/48 PASS, но со слабой проверкой неизменности (NaN = ±Inf) — заменён | `b28f79f03e8e9ebec2e2b01904153f68716156107fdd322787296d516b7db92e` |
| `ricon-route-r25-20261021-012516` | после `a != a && b != b`: самопроверка снимка 1/3 — FAIL | `eb9fbd32e13d5dd2f0df7ea0e3ef90df8252db12c99e9a4c50940669bb6906f1` |
| `ricon-route-r25-debug-20261021-012546` | отладка: NaN/+Inf и +Inf/−Inf «равны» | `c5ee74eeec7a31f7932f2932a485215a3849bb6a0f50171f9fd5ff2a5dd4c0f9` |
| `ricon-route-r25-20261021-012639` | классификатор на `>`/`<` — NaN попал в +Inf, FAIL | `824c0aa67af7d4c843b759767a8f3054620a9cb2b5ca03bbcdf54bf52c4b98ab` |
| `ricon-route-r25-20261021-012731` | классификатор на `<=`/`>=` и таблица сравнений — FAIL | `ec6af69a3fd13d05da589ed903be9cb4398eb048d823c31e5fc6db010e7181a3` |
| `ricon-route-r25-debug-20261021-012804` | таблица со строками значений (`-nan(ind)`, `inf`) | `113e14f5a395ebea6b606952b9f8616462fcc76f39a8458c1cc933c3a95003ea` |
| `ricon-route-r25-20261021-012837` | 48/48, quietPositive 48, controls 4, snapshotControls 3, adapterMapped 2, testSubclass 3, SCRIPT(E) 0, `done: PASS` | `c0d6b4b4f5239274ee1666dc09b411839b22f174c88fc2449a3cee1fc067aca7` |
| `ricon-tests-r25-seam-floatclass-20261021-012938` | прежние наборы после правки seam: lifecycle 209/209, EXT, VoN seam 103/103, L2 45/45, SCRIPT(E) 0, PASS | `f17f2244809018ae75f8c178a12fc3c1da6f3dee646f3a6c6b153748cb00b185` |
| **`ricon-route-r25-20261021-013150`** | **итог** после переноса классификатора в общий `RICON_TestSupport` (codex #2516): 48/48, quietPositive 48, controls 4, snapshotControls 3, SCRIPT(E) 0, `done: PASS` | `a2d5064156f8cd08979d873078a0e1cabb2182eda99ce29d4022eab5764d191a` |
| **`ricon-tests-r25-shared-float-20261021-013207`** | **итог** прежних наборов: lifecycle 209/209, EXT, VoN seam 103/103, L2 45/45, SCRIPT(E) 0, PASS | `f7ee1efc46d13bb85bbdd35bf94a129e508e3b4598bd9a1df9dc37833a7a58b8` |

| Файл | SHA-256 | CRC в Workbench |
|---|---|---|
| `tools/RICON_TestRunner/gen_route_fixture.py` | `40717d8a8283173af13bd19a40991aaa778502b978cb785c1630e1ee3cd3d9cc` | host |
| `tools/RICON_TestRunner/test_gen_route_fixture.py` | `544b11a9343c204c8cf385a590f40c358de57f98e2a511c8e2bbff13fe4cf9d8` | host |
| `addons/RICON_CoreTests/.../RICON_RouteFixtureSupport.c` | `293b7b530540471ff1217d036377ba4b996dfdda18958ca4ec7062dad481381b` | `411293c4` |
| `addons/RICON_CoreTests/.../RICON_TestSupport.c` (общий `FloatClass`/`SameFloat`) | `d3741f72de00c5e7f67930551d58fbcd221e50e9ad045628d71ebca54448ae5e` | `e20579c3` |
| `addons/RICON_CoreTests/.../RICON_RouteFixtures.c` (сгенерирован) | `4ae6ca89ae872ff06e09e85b1c30877fbd6b4d5a3eb4d28a0290f3118c5e2dd4` | `5859a7bf` |
| `tools/RICON_TestRunner/Scripts/WorkbenchGame/RICON_RouteProbePlugin.c` | `3d9479e3890135de3afe832fdcd2bb916824b7eb291f14e66e79c314e9d618eb` | `384f589d` |
| `addons/RICON_CoreTests/.../RICON_VoNSeamFixtureSupport.c` (правка R25) | `5ddcddb058273e1ccbec5452830ce53bde958e687ee8737e9059f362da8c648d` | — |

Отдельный разбор лога (свой скрипт claude) против `expected` корпуса: 48 строк по 48 разным id, расхождений reason
и state — 0, наибольшее |Δquality| = 2.4·10⁻⁸.

## Как повторить

```
python -B -I tools/RICON_TestRunner/gen_route_fixture.py oracle/vectors/route_v1.json addons/RICON_CoreTests/Scripts/Game/RICON_Tests/RICON_RouteFixtures.c --check
python -B -m unittest discover -s tools/RICON_TestRunner -p "test_gen_route*.py"
```

Workbench — команда из `docs/evidence/E-physics-enforce.md` с `-plugin=RICON_RouteProbePlugin`. В логе обязательны
`source=efccccb5…`, `cases=48 quietPositive=48 controls=4 snapshotControls=3`, CRC трёх Enforce-файлов из таблицы, 0 SCRIPT(E) и
`done: PASS`. Exit 0 сам по себе ничего не доказывает (первый запуск выше).

## Что остаётся открытым

- Поиск маршрута политикой сети (`RICON_RoutingPolicy.FindRoute`) этим не проверен: проверяется готовый маршрут.
- Ретрансляция голоса и доставка — runtime, исполнителя нет.
- Политика доступа здесь — тестовая «разрешить всё»: контекст проверяет только её наличие (REQ-core §9.2).
