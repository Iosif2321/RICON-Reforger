# ADR-002 — Контракты единого ядра связи (черновик)

Статус: **DRAFT** (PLAN v5, Э2). Freeze — только после исхода Э1r.
Код: `STATIC_IMPLEMENTED/COMPILE_ONLY`. Автор: claude; ревью: codex.

## Контекст
- PLAN v5 §3: одно ядро вычисляет связь для всех сетей — радиосеть, ретрансляторы, позже сотовая.
- Решения по маршрутам принимает сервер. Голоса касается только `RICON_VoNIntegration` (ADR-001).

## Решения

| Сущность | Файл | Назначение |
|---|---|---|
| `RICON_LinkQuery` / `RICON_LinkBudget` | `Core/RICON_LinkTypes.c` | вход и выход одной оценки линка. Бюджет хранит **каждый слой потерь отдельно** — для послойного паритета с oracle |
| `RICON_ELinkState` | там же | LOST / WEAK / DEGRADED / CLEAN (семантика A3 HDSP) |
| `RICON_LinkSnapshot` | там же | owner-scoped маршрут talker→listener: целые `sessionEpoch`, `linkSeq` (R-01, R-21), `IsNewerThan` отбрасывает stale/replay |
| `RICON_Num.IsFinite` | там же | в Enforce нет IsNaN/IsInf (`Math` в EN docs); NaN ловится через `v != v` (урок R-22) |
| `RICON_PropagationModel` | `Propagation/RICON_PropagationModel.c` | точка расширения: модель заполняет слои потерь |
| `RICON_FreeSpaceModel` | там же | L0: FSPL по ITU-R P.525; d < λ ограничено λ |
| `RICON_LinkEvaluator` | `Propagation/RICON_LinkEvaluator.c` | потери → Rx → шум (kTB + NF ⊕ интерференция) → SNR → разборчивость → состояние |
| `RICON_Route` / `RICON_RouteLeg` / `RICON_RoutingPolicy` | `Network/RICON_NetworkContracts.c` | маршрут из ног; качество = min(legs), состояние = худшая нога; политика на NetworkType |
| `RICON_VoNIntegration` | там же | узкий seam к VoN. Тела пустые до ADR-001 |
| `RICON_CommsManagerComponent` | `Core/RICON_CommsManagerComponent.c` | компонент GameMode: epoch, реестр Band→Model, игроки по PlayerId |

Разборчивость считается в линейной мощности: `intel = snrLin / (snrLin + 10^(10/10))` (PLAN v5 Э4).

## Проверенные по офлайн-API сигнатуры (DOC_EVIDENCE)
- `Math.Log10`, `Math.Pow`, `Math.Clamp`, `Math.Max`, `Math.Min` — `EN/interfaceMath.html`.
- `vector.Distance` — `EN/interfacevector.html`.
- `map.Set/Get`, `set.RemoveItem` — EN docs.
- `Replication.IsServer` — `EN/interfaceReplication.html`.
- `ChimeraGame.GetGameMode`, `IEntity.FindComponent` — AR/EN docs.
- `SCR_BaseGameModeComponent`: `OnWorldPostProcess`, `OnPlayerConnected` («Server-only»),
  `OnPlayerDisconnected(int, KickCauseCode, int)` — `AR/interfaceSCR__BaseGameModeComponent.html`.

## Не решено (до Э1r / ревью)
1. Сериализация `RICON_LinkSnapshot` для Rpl (RplProp vs RPC) — Э3.
2. Хранение состояния игрока: по PlayerId или через компонент на PlayerController — Э3.
3. Использовать ли `RadioManagerEntity.GetTransceiversInRange` как CandidateGraph — зависит от
   бенчмарка Э1r.
4. Компиляция в Workbench ещё не выполнялась (игра ставится) → даже `WORKBENCH_COMPILE_PASS` пока нет.
