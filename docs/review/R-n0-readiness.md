# V2 — frozen N0 readiness

2026-11-02. Независимый исполнитель: QA / ревьюер. Task task:d8b1e554-35d9-432a-8354-7200bd034b03. Статус: **STATIC_READY_FOR_NEXT_GATED_CHECK**, не разрешение исполнения.

## Проверенная идентичность

При чтении HEAD `a04a14f125fe757ad0038d762e2b44ab4819085b`, `git status --short` пуст. SHA-256 независимо получены Get-FileHash; совпали с [state](<../../.opencode/goal/state.json#L114-L125>) и handoff Team2676/2669:

| Файл | SHA-256 |
|---|---|
| [core](<../../tools/RICON_LiveProbe/ops/k0-neutral-core.ps1>) | a254d93d93b28cae44556bd14a3a5547242139e682445a7e504451e15d69d0e4 |
| [helper](<../../tools/RICON_LiveProbe/ops/k0-neutral-helper.ps1>) | 9da576fe3b5e6f1fc9eb3cc849d700c02632aad2719256d5a18ce4ae2717605c |
| [runner](<../../tools/RICON_LiveProbe/ops/k0-neutral-preflight.ps1>) | d392af9f7735a3130b1521aef4027ffc7ffccec8643fd92d6f7b56bb27e9bc00 |
| [selftest](<../../tools/RICON_LiveProbe/ops/k0-neutral-selftest.ps1>) | d46d393ae65815a62ba0209ae377ad5324be2d993207ebe9517867ab7d76284c |
| [plan v0.3](<../plan/PLAN-standard-ptt-K0.md>) | 13b573056a9b9eb1eda2e49cac4cb377e1f120e3fc3a9545610019cc432301cc |

Четыре скрипта и план прочитаны. Использованы прежние узкие заключения: Team2591 — helper/runner identity, IO, ownership, final audit; Team2663 — conservative tRead и отдельный wake; Team2676 — ASCII, exact negative histories, sticky secondary fault, singleton Boolean. Новых блокеров в этом readiness-scope не выявлено; общий аудит не повторяется. Указанный Workspace AGENTS.md отсутствует при чтении.

## Handoff / последовательность gates

1. Lead фиксирует exact Git/пять SHA и текущий статус дерева перед START. Любое изменение исходников требует нового адресного review, не молчаливого использования этого заключения.
2. G_PARSE: только Lead после точного START, только Parser.ParseFile для четырёх файлов, ожидается ноль parse errors. **NOT_RUN** данным review. Никакого dot-source/выполнения как части AST.
3. G_PURE: отдельный gate после AST; текущая Windows PowerShell 5.1 Desktop, без нового процесса. Exact selftest Mode Pure из плана. Все case outputs должны быть ровно одним Boolean true; массив/truthy object и исключение — FAIL. Дополнительно helper/runner/selftest без EnableN0 и без остальных аргументов должны вернуть только N0_DISABLED, без IO/часов/process effects. **NOT_RUN**; статически guard стоит до side effects, core definitions-only.
4. G_RUN_NORMAL: отдельный exact one-run gate, проверенный существующий local OutRoot вне checkout, без reparse/UNC, fresh CreateNew outputs, только fixed sibling helper и fixed PS5.1 executable. Никакого Bypass, alternate invocation или подмены threshold при policy failure.
5. G_RUN_NEGATIVE: только после Normal END и подтверждённого cleanup; отдельный допуск, DelayEmit затем Hanging последовательно, всего не более трёх helpers серии, без retries. Неполный cleanup останавливает серию.

## Независимые ожидаемые исходы

| Case | Необходимая наблюдаемая история | Допустимый итог |
|---|---|---|
| Normal | HELLO seq0, восемь SAMPLE seq1…8, DONE seq9; каждый однозначный delta >1 tick и ≤250ms; successful stop на неизменном probe deadline и exit | N0_NEUTRAL_OBSERVED только при целостных финальных файлах и всех guards |
| DelayEmit timeout | HELLO-only, Records1/NextSeq1/Samples0/Done=false; INVALID_MISSING_SAMPLE на HelloRead+500ms | N0_NEGATIVE_DETECTED; sample latency NOT_OBSERVED, не подставлять injected600ms |
| DelayEmit threshold | Только HELLO+SAMPLE1, Records2/NextSeq2/Samples1/Done=false, measured delta>250ms и tRead<FirstDeadline | INVALID_SAMPLE_THRESHOLD; command reference=tRead, не post-validation wake |
| Hanging | Только HELLO, Records1/NextSeq1/Samples0/Done=false; исключительно INVALID_MISSING_SAMPLE на FirstDeadline | N0_NEGATIVE_DETECTED при общем строгом command/cleanup predicate |

Для negative исходов обязательны matching StopReason/InvalidReason/deadline, успешная единственная команда, begin≥deadline, return≥begin, 0≤return−reference≤100ms, exit≤5000ms от begin. NaturalExit, late/malformed дополнительная запись, secondary quota/protocol/clock/ownership/command fault исключают detection PASS. Primary reason не заменяет sticky DisqualifyingFault; terminal не очищает latch. Cleanup/log-integrity inconclusive имеет приоритет. Pure histories должны независимо проверять эти случаи, cross-scenario и ложную подмену complete Normal, parser-cost/reference и singleton Boolean — соответствующие cases написаны, но не исполнены.

## Safety и evidence checklist X1

- QPC raw same-host positive equal frequency/high-resolution; Int64 decimal strings, overflow/rollback invalid, ±1 tick ambiguous; не переносить на Enforce clocks.
- Один t0 перед Start: absolute10s, startup5s, First500ms, Probe2s от HELLO; SAMPLE не сбрасывает probe/absolute. Poll10ms и self-cap10s — best-effort, не real-time гарантия.
- Удержанный собственный Process/handle, никаких PID/name/tree recovery; одна зарегистрированная Kill, WaitForExit отдельно. При неизвестном exit/handle — INCONCLUSIVE_CLEANUP и остановка серии.
- Wire16×1024 bytes/16384 total; reader4096+pending1024; summaries≤8. Runner64 ordinary + reserve4/4/8, record≤2048/file≤163840; manifest/receipt≤8192 каждый, aggregate≤196608. Reserve не расходуется ordinary; oversize diagnostic prefix≤1025, rawComplete=false; чтение/хэш oversized потока не продолжаются без bound.
- Проверить manifest/script identity, final wire schema/seq/LF/hash и tail, closed runner log/hash, receipt и его внешний SHA после закрытия. END содержит cleanup и все раздельные verdicts, в том числе ошибки; stdout/exit0 не заменяют raw evidence.
- Ошибки IO/policy/scheduling не исправлять изменением системы, повтором или ослаблением теста. Не удалять evidence.

## Граница вывода

Это readiness X1 для следующего отдельно допущенного AST/pure шага. AST/PURE/IPC/command/cleanup runtime: **NOT_RUN этим review**. В прочитанном входном snapshot execution gates закрыты согласно [state](<../../.opencode/goal/state.json#L43-L50>); актуальное решение перед START обязан проверить Lead. Во время финальной проверки state/spec уже менялись параллельными авторами, поэтому их текущую редакцию этот review не утверждает. Пять frozen N0 SHA при повторной сверке остались неизменны. Никаких игровых/аудио/input/focus действий, запусков файлов, dot-source, AST или runtime evidence не выполнялось. N0 не доказывает K0 safety/provenance, physical release, audio, same-path packet, RICON effect или direct preservation.
