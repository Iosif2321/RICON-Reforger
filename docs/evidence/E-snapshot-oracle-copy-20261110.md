# Snapshot v1: изоляция принятых данных в host-эталоне

Дата: 2026-11-10. Автор: `codex.lead`; независимые intake: `codex.team.core` и `codex.team.review`.
Область результата: **HOST_ORACLE_REGRESSION_ONLY**. Поручение/владение — Agora #8203/#8208.

## Причина и минимальная правка

`oracle/snapshot_oracle.py` сохранял словарь вызывающей стороны в `Receiver.slots` без копии на путях
`full_resync` и принятого `receive`. После принятия изменение исходного `linkSeq`, `ttlMs` или `quality`
переписывало состояние эталона. Это расходилось с существующими Enforce `CloneSnapshot()` в
`RICON_LinkSnapshotReceiver.ApplyFullResync` и `Receive`.

Кроме того, `compare({}, validIncoming)` выбрасывал `KeyError`, тогда как Enforce проверяет текущий слот
и возвращает `INVALID_IDENTITY`.

Исправление: стандартный `copy.deepcopy` на обеих границах принятия и валидация `current` до сравнения.
Новых зависимостей нет. Schema v1, JSON-векторы, Enforce и wire-контракт не менялись.

Независимая приёмка: Agora #8219/#8220, `ACCEPT_SCOPED_ORACLE_COPY_FIX`. Reviewer повторил 8 snapshot,
17 Publication и 13 VoNSeam тестов, 46 vectors; отдельно проверил invalid current `{}`/`[]`/seq0/NaN,
приоритет INVALID_SNAPSHOT и RX-12 (плохой последний элемент full resync сбрасывает весь state/identity).
Ошибочное ожидание сохранённого старого слота было обнаружено отрицательным контролем.

Основание: `docs/req/REQ-core-contracts.md` §8; `docs/req/CT10-migration-map.md` — независимость копий.

## RED → GREEN

До исправления новый regression-корпус дал `Ran 8 tests; FAILED (failures=2, errors=1)`:
оба mutation-теста увидели `storedSeq=999` вместо `5`; malformed current вызвал `KeyError: schemaVersion`.
Прежние пять тестов при этом проходили. Core независимо воспроизвёл исходный alias-дефект.

После исправления:

| Команда из canonical корня | Результат |
|---|---|
| `python -B -m unittest discover -s oracle -p test_snapshot_oracle.py -v` | 8/8 PASS; single receive проверяет ACCEPT_FIRST и ACCEPT_NEWER |
| `python -B -I oracle/snapshot_oracle.py --check oracle/vectors/snapshot_v1.json` | 46 vectors PASS |
| `python -B -m unittest discover -s oracle -p test_link_publication_oracle.py` | 17/17 PASS |
| `python -B -m unittest discover -s oracle -p test_von_seam_oracle.py` | 13/13 PASS |
| `python -B -m unittest discover -s tools/RICON_TestRunner -p test_gen_snapshot_fixture.py` | 3/3 PASS |
| `python -B -m unittest discover -s tools/RICON_TestRunner -p test_gen_link_publication_fixture.py` | 8/8 PASS |
| `python -B -I tools/RICON_TestRunner/gen_snapshot_fixture.py oracle/vectors/snapshot_v1.json addons/RICON_CoreTests/Scripts/Game/RICON_Tests/RICON_SnapshotFixtures.c --check` | 43 cases / 77 steps; совпадение сохранённой fixture; 3 allocator по-прежнему blocked |
| `git diff --check -- oracle/snapshot_oracle.py oracle/test_snapshot_oracle.py` | PASS |

Всего 49 host unit-тестов и прежний корпус 46 векторов; это разные счётчики, не 95 runtime-сценариев.

## SHA-256 текущих файлов

| Файл | SHA-256 |
|---|---|
| `oracle/snapshot_oracle.py` | `98daf7ca987f00264a63b11547bcf0d07d93daa058f4557c60b7c92cb6aafe95` |
| `oracle/test_snapshot_oracle.py` | `556435280e0e9aff19cca77aef597448d56248400b0e63f4510dbcc90e7fbcb5` |
| `oracle/vectors/snapshot_v1.json` — без изменений | `3440982cea0cce21ef857e09a1db3dbe77a5c85561771212dd9ea1199e93721c` |
| `RICON_LinkSnapshot.c` — без изменений | `53d99d9710ddc1a4dc4617046cb2a3496e7768d2d76920a8a69a276caf39724c` |

## Граница и следующий срез

CT10/schema2 не закрыта: descriptor отсутствует в snapshot v1. Изменение descriptor при неизменном Q
должно отдельно вызывать публикацию. Нужен согласованный контракт requirements → независимые vectors →
producer/codec/copy/publication/single-full receiver и все consumers. Physical validity не продлевается TTL.

В этой работе не запускались Workbench, Game, сервер, debugger или native-контроллеры. Новый
ENFORCE_PARITY, transport/audio/performance PASS не заявляется. Системная Goal не создавалась.
