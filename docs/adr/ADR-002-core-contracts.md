# ADR-002 — Контракты единого ядра связи (черновик)

Статус: **DRAFT, AGENT-REVIEWED C13; Э4T-s delta C16 REVIEW PENDING** (PLAN v5, Э2).
Freeze запрещён до обязательных gate ниже.
Код L0: `STATIC_IMPLEMENTED`; `RICON_Core` — `WORKBENCH_COMPILE_PASS` (2026-10-19,
`docs/evidence/E-workbench-source.md`). Автор исходного ADR: claude; актуализация: codex; взаимное code review
C13 — agora `ricon-reforger-build` id544.

Нормативные документы: `docs/req/REQ-link-eval.md` и `docs/req/REQ-core-contracts.md`. При расхождении они
имеют приоритет над этим описательным ADR.

## Контекст
- PLAN v5 §3: одно ядро вычисляет связь для всех сетей — радиосеть, ретрансляторы, позже сотовая.
- Решения по маршрутам принимает сервер. Голоса касается только `RICON_VoNIntegration` (ADR-001).
- Черновик L0 из baseline `598e4e8` не прошёл clean-room review и заменён независимой реализацией C13 по
  agent-reviewed REQ. История baseline остаётся только audit trail (`NOTICE.md`).

## Решения

| Сущность | Файл | Назначение |
|---|---|---|
| `RICON_LinkQuery` | `Core/RICON_LinkTypes.c` | server-built physical input; optional sensitivity/required-SINR/interference только через явные flags; stable Band/Model/Noise/Intelligibility IDs |
| `RICON_EReason` | там же | стабильные reason values; первая нарушенная граница по `REQ-core` §4.1; model-output contract имеет отдельный reason |
| `RICON_LossLayer` / `RICON_LinkBudget` | там же | расширяемые именованные слои и раздельные Rx, N0, N, N+I, SINR, sensitivity, Q/state; invalid budget полностью finite |
| `RICON_ELinkState` | там же | LOST / WEAK / DEGRADED / CLEAN — продуктовая семантика; production Q/пороги пока не калиброваны |
| `RICON_LinkSnapshot` | там же | **baseline scaffold, не соответствует REQ-core §8**; переработка отдельным срезом |
| `RICON_Num` | там же | finite через `float.MAX`; стабильная сумма мощностей `TryPowerSumDbm`, без sentinel |
| `RICON_PropagationModel` / `RICON_FreeSpaceModel` | `Propagation/RICON_PropagationModel.c` | модель добавляет слои и объявляет domain; L0 FSPL напрямую `20 lg(4πd/λ)`, `d_eff=max(d,λ)` |
| `RICON_NoiseModel` / `RICON_ManMadeNoiseModel` | `Propagation/RICON_NoiseModel.c` | внешний шум seam; P.372 coefficients приходят из versioned manager config, вне domain `f_a=1` + flag |
| `RICON_IntelligibilityModel` | `Propagation/RICON_IntelligibilityModel.c` | Q/state seam с обязательным config contract; concrete `linear_test` только TEST_ONLY, production calibration blocked |
| `RICON_LinkEvaluator` / `RICON_WirelessLinkEvaluator` | `Propagation/RICON_LinkEvaluator.c` | pure `query→budget`; strong refs на уже разрешённые authority-manager dependencies, без собственных registries; порядок losses→Rx→N/N+I→SINR→threshold→Q/state |
| `RICON_Route` / `RICON_RouteLeg` / `RICON_RoutingPolicy` | `Network/RICON_NetworkContracts.c` | маршрут из ног; качество = min(legs), состояние = худшая нога; политика на NetworkType |
| `RICON_VoNIntegration` | там же | узкий stateless seam к VoN; Э4T-s валидирует accepted snapshot/binding, но всегда возвращает `BLOCKED_NO_ACTUATOR` до runtime evidence (`REQ-von-seam`) |
| `RICON_CommsManagerComponent` | `Core/RICON_CommsManagerComponent.c` | **baseline scaffold**: manager registries/epoch/null-safe access ещё не реализуют `REQ-core`; следующий срез |

Production-модель разборчивости и пороги состояний — `BLOCKED_CALIBRATION`; в коде нет неявных значений.

## Проверенные по офлайн-API сигнатуры (DOC_EVIDENCE)
- `Math.Log10`, `Math.Pow`, `Math.Clamp`, `Math.Max`, `Math.Min` — `EN/interfaceMath.html`.
- `vector.Distance` — `EN/interfacevector.html`.
- `array.Insert/Clear/IsEmpty/Count`, `string.Empty/IsEmpty`, `float.MAX`, `notnull array<ref T>` — EN docs.
- `Replication.IsServer` — `EN/interfaceReplication.html`.
- `ChimeraGame.GetGameMode`, `IEntity.FindComponent` — AR/EN docs.
- `SCR_BaseGameModeComponent`: `OnWorldPostProcess`, `OnPlayerConnected` («Server-only»),
  `OnPlayerDisconnected(int, KickCauseCode, int)` — `AR/interfaceSCR__BaseGameModeComponent.html`.

## Доказательства и границы

- Host oracle: `ORACLE_PASS: 57 vectors`, 9 unittest. Это проверяет REQ/числа, не Enforce.
- Static code review C13: взаимный agree id544; legacy fingerprints отсутствуют; API surface сверена офлайн.
- Авторский `MIRROR_PASS 57` — только вспомогательное static evidence, **не ENFORCE_PARITY**.
- `WORKBENCH_COMPILE_PASS = PASS`: `RICON_Core` @ `06979fc`; EXT-1…5 @ `1c72797` —
  `COMPILE_FIXTURE_PASS` (`docs/evidence/E-workbench-source.md`). После любого изменения `addons/` compile gate
  требуется пройти заново. Заказ №001 выполнен частично как отдельная SOURCE-проверка vanilla-файлов; открытые пункты
  перечислены в `docs/evidence/E-workbench-source.md`.
- Runtime, Rpl, performance и audio — `BLOCKED_NO_RUNTIME`.

## Обязательные gate до freeze Э2

1. `RICON_LinkSnapshot` по `REQ-core` §8: schema/epoch/link identity/seq overflow/int TTL.
2. Authority manager registries/config: Band→resolved evaluator dependencies; no fallback; versioned P.372 source.
3. Routing/Evaluator extension CT для EXT-1…5 и model-contract negative tests (`CT-CORE-011`).
4. **PASS 2026-10-19:** Workbench compile с полным log/checksum и устранением ошибок
   (`docs/evidence/E-workbench-source.md`); повторить после любого изменения `addons/`.
5. Enforce execution parity с oracle — только когда разрешён соответствующий запуск; до этого `NOT_TESTED`.
6. Production intelligibility/calibration и актуальный primary source P.372 остаются отдельными evidence gates.

Дополнительно открыто: Rpl-сериализация snapshot; PlayerId↔entity lifecycle; CandidateGraph/performance; ADR-001
актуаторы VoN. Эти вопросы не блокируют статическую проверку L0, но блокируют freeze/functional PASS.
