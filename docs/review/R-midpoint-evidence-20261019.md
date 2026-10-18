# R-midpoint-evidence-20261019 — аудит доказательств после 10 спринтов

Дата: 2026-10-19. Автор: codex. Ревью: claude. Проверяемая ревизия: `8d212a5`.

Режим: read-only аудит кода/истории и запуск host-oracle; production-код не менялся.

## Мини-intent

- **Проблема:** README, matrix, STATUS, ADR и REQ обновлялись на разных ревизиях; первая компиляция всё ещё не
  выполнена, поэтому устаревшая строка способна превратиться в ложный PASS/source of truth.
- **Результат:** таблица «claim → факт → evidence → исправление» и точный stopping gate.
- **Ограничения:** не запускать Workbench/Play/сервер; не начинать Э3/новый Enforce-код.
- **Открыто:** результаты Workbench №001/№003, runtime, production intelligibility, P.372-18/Fig.39.

## Итоговый verdict

**Разработка Enforce должна ждать Workbench compile.** Репозиторий содержит большой взаимно проверенный static
срез и исполняемые host-oracle, но ни один `.c` ещё не компилировался Enfusion.

| Уровень | Статус |
|---|---|
| Baseline/evidence/license | PASS для заявленного статического scope |
| Requirements/reviews | AGENT-REVIEWED; не human/runtime acceptance |
| Host oracle | PASS: link 57, snapshot 46, route 48; 18 unittest |
| Enforce source | STATIC_IMPLEMENTED / code-reviewed |
| `RICON_CoreTests` | COMPILE_FIXTURE / SELF_CHECK_NOT_RUN |
| Workbench compile | NOT_TESTED / внешний blocker |
| Enforce↔oracle parity | NOT_TESTED |
| Runtime/Rpl/VoN/audio/perf | BLOCKED_NO_RUNTIME |
| Production intelligibility | BLOCKED_CALIBRATION |

## 1. Репозиторий и supply boundary

Проверки на `8d212a5`:

- branch: только `main`; remotes: отсутствуют; working tree: чист;
- 79 tracked files (`git ls-tree`), 94 commits до добавления этого отчёта;
- 11 Enforce `.c`, 1 987 строк: 9 core + 2 TEST_ONLY;
- запрещённых target paths/artifacts (`extension`, `tsplugin`, `addons/comms`, Discord/spikes, DLL/EXE/PBO/PAK) — 0;
- secret-pattern scan вне immutable ARE — 0;
- лицензия root/README: APL-SA; old A3 GPL указан только как reference/provenance;
- RE запрещён приоритетной шапкой PLAN, README, NOTICE и matrix; старый RE-gate остаётся только в сохранённом теле
  утверждённого PLAN v5 и не является действующей политикой.

**PASS:** canonical target/clean-room/license/no-push boundary соблюдены.

## 2. Повторяемые проверки

```text
python -I oracle/ricon_oracle.py --check oracle/vectors/link_eval_v1.json
ORACLE_PASS: 57 vectors

python -I oracle/snapshot_oracle.py --check oracle/vectors/snapshot_v1.json
SNAPSHOT_ORACLE_PASS: 46 vectors

python -I oracle/route_oracle.py --check oracle/vectors/route_v1.json
ROUTE_ORACLE_PASS: 48 vectors

python -m unittest discover -s oracle -p "test_*.py" -q
Ran 18 tests — OK
```

Эти проверки доказывают host requirements/state traces. Они не исполняют Enforce и не закрывают
`WORKBENCH_COMPILE_PASS`, `ENFORCE_PARITY` или runtime.

## 3. Состояние этапов по факту

| Этап | Факт на ревизии | Вердикт |
|---|---|---|
| Э0 | baseline/evidence/APL-SA/clean-room зафиксированы | STATIC PASS |
| Э1s | DOC/offline API есть; vanilla SOURCE №001 отсутствует | PARTIAL / WAIT_WORKBENCH |
| Э1r/Э1b | игра/сервер не запускались | BLOCKED_NO_RUNTIME |
| Э2 | core REQ v0.7, ADR/reviews, manager/snapshot/routing/registries/seams, EXT fixtures; compile отсутствует | STATIC_IMPLEMENTED + ORACLE_PASS; NOT FROZEN |
| Э3 | REQ-lifecycle v0.2 reviewed; код намеренно не начат | READY_AFTER_E2_COMPILE |
| Э4 L0 | clean-room Enforce source + link oracle; Enforce parity отсутствует | STATIC_IMPLEMENTED + ORACLE_PASS |
| Э4 L2 | P.526-16 §4.5 method выбран; требований/реализации/calibration ещё нет | METHOD_SELECTED / IMPLEMENTATION_OPEN |
| Э4T+ | не начаты либо runtime-blocked | NOT_STARTED/BLOCKED |

## 4. Findings drift

### MP-01 — BLOCKER: Workbench evidence отсутствует

`docs/evidence/E-workbench-source.md` отсутствует. Значит:

- Q1/Q2/Q4/Q17/Q18/Q19 SOURCE pending (заказ №001);
- `RICON_Core` и `RICON_CoreTests` не компилировались (заказ №003);
- GUID базы/dependency discovery остаются RISK-3/RISK-4 compile-readiness;
- SelfCheck EXT-1…5 не запускался.

Target addon tree заказа №003 — commit `06979fce95f57b388898ae0cff3f83037ee97633`. На audit revision
`8d212a5` команда `git diff --stat 06979fc..HEAD -- addons` пуста: более новые commits меняют только docs. Исполнитель
MAY компилировать current docs-only HEAD, но MUST зафиксировать оба hash и доказать нулевой addons diff; иначе — exact
checkout/working tree `06979fc`.

### MP-02 — HIGH: coverage matrix занижает достигнутую статику

`E-coverage-matrix.md`:

- раздел 6 всё ещё «способ компиляции без Play не выяснен» — неверно: CLI документирован в
  `E-workbench-build.md`; **не выполнена** сама компиляция;
- раздел 10 всё ещё `OPEN`, «исполняемых contract tests нет → oracle» — устарело: три oracle/runners исполняются,
  57/46/48 PASS; EXT fixtures существуют, но NOT_COMPILED/NOT_RUN;
- сводка 15 разделов поэтому не включает раздел 10 в PARTIAL;
- priority 2 должен говорить «actual Workbench compile + Enforce parity + SelfCheck», а не «выяснить способ/oracle».

Исправление: Claude обновляет matrix, не повышая Э2 до CLOSED.

### MP-03 — HIGH: STATUS Э2/Э4 содержит старые версии/gates

`STATUS_RU.md` на старте аудита:

- Э2 blocker ссылается на REQ-link v0.3, REQ-core без версии, «код — спринт 4», compile через заказ №001;
- список commits Э2 не включает manager/snapshot/routing/NetworkType/fixtures;
- Э4 blocker говорит «L2-дифракция OPEN (P.526-16)», хотя method selection закрыт в спринте 10;
- Workbench table правильно содержит №003, но stage row не считает его актуальным compile gate.

Исправление: компактно заменить исторический перегруженный текст на текущий статус + ссылки на sprint ledger ниже.

### MP-04 — MEDIUM: README status token двусмыслен

README одновременно пишет `STATIC_IMPLEMENTED/COMPILE_ONLY` и «Nothing has been compiled yet». Термин
`COMPILE_ONLY` можно прочитать как compile-pass. Исправление: `STATIC_IMPLEMENTED / NOT_COMPILED`; явно перечислить
host ORACLE_PASS отдельно.

Layout уже исправлен Claude commit `8d212a5`: oracle/req/review/CoreTests больше не помечены planned/absent.

### MP-05 — MEDIUM: ADR/REQ ссылаются на устаревший Workbench order

- `ADR-002-core-contracts.md` говорит, что compile выполняется по №001;
- `REQ-core-contracts.md` в gate/ending ссылается на №001;
- актуален №003; №001 остаётся для vanilla SOURCE, но его compile-пункт superseded.

Исправление: Codex заменяет конкретный compile order на «актуальный Workbench compile order (сейчас №003)»; SOURCE
№001 оставить отдельно.

### MP-06 — LOW: compile target и docs HEAD будут дрейфовать

Вопрос №003 привязан к addon-code commit `06979fc`; research/docs commits продолжают двигать HEAD без изменения
addons. Нельзя бесконечно переиздавать заказ после каждого docs-only commit. Правило handoff:

1. фиксировать addon target commit;
2. на current HEAD показывать `git diff --exit-code <target>..HEAD -- addons`;
3. отчёт хранит target commit, actual HEAD и результат diff.

### MP-07 — PASS: физические evidence levels согласованы

- P.526-16 §4.5 method selected через `STD_COPY`; L2 implementation/calibration OPEN;
- P.372-17 formula/c-d подтверждены; category domains/P.372-18 OPEN;
- universal scalar SINR→speech intelligibility не найден; DAQ/12 dB SINAD unverified;
- production model остаётся BLOCKED_CALIBRATION, `linear_test` TEST_ONLY.

### MP-08 — PASS: review-driven defects действительно закрыты

По current source отсутствуют `IsNewerThan`, float TTL, старый default model fallback, `32.44`, `-999/-900`, A3
threshold constants. Snapshot/route/network registries используют stable reasons и fail-closed contracts. Это static
source evidence; compile/runtime не подразумевается.

## 5. Требуемые reconciliation changes

| Owner | Файл | Правка |
|---|---|---|
| claude | `E-coverage-matrix.md` | MP-02 |
| claude | `STATUS_RU.md` | MP-03 + blocker reminder |
| codex | `README.md` | MP-04, если Claude не включит в свой patch |
| codex | `ADR-002-core-contracts.md`, `REQ-core-contracts.md` | MP-05 |

После этих правок новый Enforce/Э3/L2 код запрещён до внешнего compile report.

## 6. Точный stopping gate / handoff

Нужны два результата от отдельного ассистента владельца:

1. **Workbench №003:** compile `RICON_Core` и `RICON_CoreTests` для addon target `06979fc…`; полный log/errors,
   checksum file list, exit codes и dependency resolution. Это разблокирует исправление compile errors, но не runtime.
2. **Workbench №001 SOURCE:** vanilla VON/relay/ECommMethod/material/input/radial findings для Q1/Q2/Q4/Q17/Q18/Q19.

После №003:

- FAIL → отдельный fix sprint + новый compile order;
- PASS core / FAIL tests → core compile PASS, fixtures исправить отдельно;
- PASS обоих → `WORKBENCH_COMPILE_PASS` + `COMPILE_FIXTURE_PASS`; SelfCheck/Enforce parity всё ещё NOT_TESTED;
- только после reconciliation и разрешённых следующих gates планировать Э3 implementation.

До внешнего ответа состояние проекта: **BLOCKED_WORKBENCH for new Enforce work**, не «complete» и не runtime-ready.
