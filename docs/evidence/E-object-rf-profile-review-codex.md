# P3.1 — независимое ревью и native-повтор codex

2026-10-23. **SOURCE_REVIEW_PASS / ENFORCE_PARITY**, финальный повтор Claude после усиления стенда ожидается.
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

Это приёмка чистого resolver. Адаптер движка, реальные RF-профили/правила, геометрия стен/проёмов, P.2040,
потери, слышимость и производительность остаются открытыми.
