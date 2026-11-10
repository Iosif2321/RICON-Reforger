# CT10: typed Enforce source port и безопасная арифметика часов

2026-11-10. Статус: **SOURCE_REVIEWED_UNCOMPILED**.
Clock: **SOURCE_EXTRACTED_CHECKED_INT32_ALGORITHM_ONLY**.
Полная Goal active; Enforce compile/fixtures/runtime/audio не приняты.

## Изменение и основание

После принятого HOST-контракта (E-ct10-host-contract-20261110.md; Agora8249/8250)
добавлены отдельные typed schema2 DTO, Expected/binding/context/limits, Normalize/Validate/Clone,
atomic receiver, detached Read и отдельно View с clock/physical interval gate.
Это часть существующего RICON_Core. Legacy schema1 DTO/publication/seam и RCFD/1 не менялись.
Новый тип не наследует v1 DTO и не повышает CURRENT_SCHEMA_VERSION старого API.

Canonical source:
- addons/RICON_Core/Scripts/Game/RICON/Core/RICON_TemporalClock.c
- addons/RICON_Core/Scripts/Game/RICON/Core/RICON_TemporalSnapshot.c
- addons/RICON_CoreTests/Scripts/Game/RICON_Tests/RICON_TemporalSnapshotFixtures.c
- tools/RICON_TestRunner/Scripts/WorkbenchGame/RICON_TemporalSnapshotProbePlugin.c
- tools/RICON_TestRunner/check_temporal_clock.ps1

Пять файлов опубликованы byte-identical проверенным staging из соседнего
Cellular System/output/ct10-enforce-20261110. Изменения обратимы, commits не делались.
DTO source пока не является подключённым production producer/receiver dispatcher.

## Встречная проверка

Core автор порта; Reviewer зафиксировал13 MP до чтения DTO staging, а при source review
clock отдельно записал10 ручных MC ожиданий, не вычисляя их через проверяемый алгоритм.
Независимый source review: Agora8271/8272/8274, reviewed snapshot e22b675d…/fixture eabbccaa….
Root нашёл scalar+null reason-order mismatch (8265/8266); Reviewer нашёл null corner публичного ContextMatches.
Core исправил оба в своём staging, добавил fixture, root и Reviewer перечитали исправления.
Это source review, не выполненные Enforce regressions.

Documentation независимо проверила root probe/checker (8273): pure RunAll, checks>0,
checks/failures/firstFailure diagnostics, Print и Workbench.Exit; autorun/Play/world нет.
PowerShell parser0errors. C# harness синтаксически переносит тела actual Clock source,
не переписывает арифметику; независимая long-formula считает точный микросекундный интервал.
Все промежуточные этой формулы при int32 inputs меньше2^43, без потери точности.

Agy получил отдельный read-only plan audit job agy.port, Agora ack8270. CLI observer34168
завершился exit1/DEADLINE_EXCEEDED после4min с reported still-in-progress. Это окончание
наблюдателя, не доказательство остановки backend turn; повторный job не создавался.
Итоговый source verdict Agy на момент публикации этой записи НЕ получен и ему не приписывается.

## Проверено

Из canonical корня:
`& '.\tools\RICON_TestRunner\check_temporal_clock.ps1' -SourcePath '.\addons\RICON_Core\Scripts\Game\RICON\Core\RICON_TemporalClock.c'`

| Проверка | Наблюдаемый результат |
|---|---|
| Механически извлечённый Clock source; C# /checked+ | PASS |
| MapInterval против независимой exact formula | 85907 cases PASS |
| ContainsInterval против exact interval comparison | 34359 cases PASS |
| Намеренные UpperInclusive / BorrowLost defects | 2/2 обнаружены через semantic mismatch |
| Byte equality canonical/staging, все5 файлов | PASS |
| Whitespace новых5 файлов; git diff --check | PASS |
| Canonical source visibility in git status | Все5 показаны как новые файлы |
| Enforce compiler / RunAll source fixtures | NOT_RUN |
| Game/Workbench/server, настоящий clock adapter, DSP/content/audio | NOT_RUN |

Helper использует delta проверенных неотрицательных int32 times, checked anchor+delta,
quotient/remainder errorUs, borrow/carry и guarded bounds. Нет int32 ms*1000,
float clocks или предположения об int64 в Enforce. Правый конец physical interval исключён.
C# execution подтверждает алгоритм source; Enforce parser/typechecker/VM этим не проверены.
Полный HOST suite не повторялся: host417cd7a… и legacy source не менялись; его прежний PASS
не выдаётся за тест новых Enforce файлов.

## Обязательные следующие интеграционные контракты

Независимые воспроизведения на прежнем HOST/source подтвердили:
1. Active LOST/Q0 и TERMINATE выдают одинаковый v1 payload, но registry имеет1или0 entries.
   Нужен отдельный typed END, не новое RF состояние и не null descriptor в ACTIVE.
2. Trusted same-epoch full может откатить seq2→1 и освежить TTL. Реальный transport admission
   обязан иметь согласованную freshness; скрыто менять legacy trusted full нельзя.
3. Expected, скопированный из самого пакета, сам себя подтверждает. Необходим независимый
   authoritative lifecycle/device/lease/generation/RF flow, включая ended-binding receipt.
4. S3b manager epoch/sequence и lifecycle epoch имеют разные границы; coordinator должен
   сохранить consumed IDs, epoch reads и atomic failure. HOST callback этого не заменяет.

Reviewer предложил LINK_END с epoch/link/seq/old binding и ordering floor/tombstone.
Формат ещё не canonical контракт и не реализован. Retention/limits и fresh-full protocol
должны согласовываться вместе. END не сбрасывает физическую среду при паузе PTT.

Следующий шаг — согласовать typed lifecycle event/full freshness/Expected source,
затем реализовывать настоящий producer/publication/receiver adapter с независимыми traces.
Полная карта callers/generators/probes: output/ct10-enforce-20261110/documentation/MIGRATION_OWNERS_RU.md;
counter/MP/MC: review/MIGRATION_COUNTERS_RU.md; исходный порт: core/PORT_DESIGN_RU.md.
Source fixture не представляет JSON bool/float/list в уже typed int: это HOST/future ingress gate.
Wire/Rpl/authentication, builder/sharedRX/calibration, compile/runtime/lifecycle/load и
supported voice actuator остаются OPEN. До Enforce компиляции новый код — source-кандидат.
