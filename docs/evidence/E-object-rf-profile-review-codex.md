# P3.1 — независимое ревью и native-повтор codex

2026-10-23. **AGENT-REVIEWED / NATIVE_PARITY_PASS**, финальный повтор Claude после усиления стенда проверен.
Основание: REQ-object-rf-profile v0.1, freeze `cab0d79`, реализация Claude `306670b`.
Корпус 47, LF SHA `b0b93be3238b49c223c859ac14e22f045ae708ce03f1cae2ed731eaf0531e4c5`.

Codex прочитал весь `RICON_ObjectRfProfile.c` и fixture support. Расхождений с OP02–OP06 нет:

- профили, оба графа и все правила проверяются до выбора;
- missing parent, self-cycle, неиспользуемый цикл, неправильный конкретный узел и дубли не проходят;
- hasParent сохраняет отличие null-корня от parent="" (R45);
- обход итеративный, порядок записей не выбирает приоритет;
- компонент/поверхность имеют приоритет над правилами; неизвестное выбранное назначение не скрывается fallback;
- BallisticInfo остаётся подсказкой, knowledge/materialId совместимы с P2;
- каждый результат новый и содержит только скаляры; исходные записи не меняются.

По review codex (#3708) Claude добавил в существующую группу CheckGraphs две проверки native loader:
duplicate nodeId и duplicate rule nodeId. Production guards уже имелись; счётчики controls=6/extras=2 не менялись.
Эта версия стенда входила в согласованный снимок до запуска codex (ACK #3710).

Независимый hidden Workbench ScriptEditor `ricon-objprofile-codex-20261023-170714`, PID 24720:

- exit 0, timeout=false, sourceStable=true, SCRIPT(E)=0, tagged FAIL=0, один PASS;
- **40/40**, immutable=40, controls=6, extras=2, hostOnly=7;
- chain 5000 и новые проверки дубликатов исполнены внутри extras;
- лог SHA `5def3be76271674cfa958b70258bfc0b061ff39448234ff503ca229e74f94de2`;
- receipt `receipts/p3-1-native-run-codex-20261023-170714.json` хранит процесс, путь лога и снимок 83 .c.

Собственный `receipts/p3-1-log-check-codex.py` проверил все **5** полей результата в каждом из **40** случаев,
match/immutable, отсутствие host-only, header/summary/PASS; **14** отрицательных контролей parser PASS.
Parser Claude также PASS на этом логе. Отчёт — `receipts/p3-1-log-recheck-codex-own.json`.
Отдельно сверены exit/timeout/отсутствие мира, SHA лога и все 83 текущих SHA исходников.

Финальный повтор Claude `ricon-objprofile-claude-20261023-170914`, SHA
`05ee2bda755c5ef5519dc6bb7068f54465c40c7dc5c3c86cb7735944bed98302`, 40/40, controls6, extras2, PASS.
Codex повторил свой parser на peer-логе и подтвердил тождество всех 43 строк OBJPROFILE и CRC четырёх файлов:
Resolver `8dd3d098`, Support `99cb6d09`, Fixtures `c4556322`, Probe `b833a706`.
Support с duplicate-контролями сохранён в `d6cf34e`. Совместный receipt —
`receipts/p3-p43-peer-review-codex-20261023.json`, parser-отчёт — `receipts/p3-1-log-recheck-codex-peer.json`.
Общий host после P3.1/P4.3: **54/54 PASS**, включая самостоятельные CLI/сборщик P3.1 и все unit-тесты;
`receipts/p3-p43-host-codex-20261023.txt`.

Это приёмка чистого resolver. Адаптер движка, реальные RF-профили/правила, геометрия стен/проёмов, P.2040,
потери, слышимость и производительность остаются открытыми.
