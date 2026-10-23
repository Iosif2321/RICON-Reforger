# P3.1: Enforce-resolver RF-профиля объекта совпадает с замороженным корпусом

Дата: 2026-10-23. Автор кода и стенда: claude. Статус: **ожидает ревью codex** (исходник и повтор пробника).

Уровень: `ENFORCE_PARITY`. Код исполнен в Workbench без мира, Play и сервера. Это не распознавание объектов
движка, не RF-параметры P.2040, не потери и не слышимость.

Основание:
- REQ-object-rf-profile v0.1 AGENT-REVIEWED (codex, freeze cab0d79);
- корпус `object_rf_profile_v1.json` заморожен по LF SHA `b0b93be3238b49c223c859ac14e22f045ae708ce03f1cae2ed731eaf0531e4c5`
  (agora #3689, #3697);
- до кода claude пересчитал корпус слепым checker по REQ: 47/47, контроли 5/5, цепь 5000 узлов (`7f53c91`).

## Что сделано

`addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_ObjectRfProfile.c` — `RICON_ObjectRfProfileResolver.Resolve`:
- коды `RICON_EObjectProfileCode` (0 — INVALID_INPUT); основание — `RICON_EObjectProfileSource`; knowledge —
  `RICON_EPathKnowledge` из P2 (OP05: совместимо с PO04);
- вход: узел графа `{id, hasParent, parentId}` по согласованию #3694. Так `parent = ""` (R45) отличается от корня,
  а дубль узла или правила при построении map — INVALID_INPUT;
- глобальная проверка до выбора (OP02/OP03): реестр без дублей; ключи по PO04 (`ValidKey` P2); родитель существует
  и не равен узлу; циклы ищутся итеративной раскраской 0/1/2 во всём графе, включая неиспользуемые ветви; правила
  ссылаются на существующие узел и профиль; prefabId/classId объекта есть в своих графах;
- приоритет OP04: компонент → поверхность (неизвестное выбранное назначение — UNRESOLVED_PROFILE без fallback) →
  обход prefab к корню → обход класса → подсказка → UNKNOWN. Обход итеративный.

Production-вызовов нет; P2 `RICON_PathObservation` и адаптеры движка не менялись.

Стенд `RICON_ObjectRfProfileFixtureSupport.c`:
- все пять полей результата сравниваются строго;
- проверяется неизменность входа и новый объект результата на повторный вызов;
- 6 контрольных порч сравнения на R01;
- 2 дополнительные проверки:
  1. порядок: у всех native-случаев с входом графы, правила и реестр переставлены в обратном порядке; результат
     тот же, вход восстановлен;
  2. графы: цепь из 5000 узлов разрешается до правила на корне; цикл, висячий родитель и null-узел в
     неиспользуемой ветви дают INVALID_INPUT при известном профиле компонента.

Генератор `gen_object_rf_profile_fixture.py`:
- закреплён за LF SHA, 40 native-случаев;
- host-only совпадает с полем `hostOnly`: R31–R36, R41;
- null-родитель → `hasParent = false`, строка-родитель → `hasParent = true` с этой строкой;
- null-входы R37–R40, R43 переданы в native как null.

Тест генератора — 5 случаев.

SHA-256 по LF-содержимому:

| Файл | SHA-256 |
|---|---|
| `RICON_ObjectRfProfile.c` | `99184b464d709c3eff29a37346d1e3b1469ed8be59ed0a916888d9a43e96835d` |
| `RICON_ObjectRfProfileFixtureSupport.c` | `c44dfbd1507fac354265158c39f187623f9c6d1a3d280168a0fc025bab3188f1` |
| `RICON_ObjectRfProfileFixtures.c` | `c0670138a99a2ebee7a145f2e627b5b524a73b6703e5139442fb27e4d7a9551d` |
| `RICON_ObjectRfProfileProbePlugin.c` | `f6b607fbd83ce0ad40e094a725bf3f47208e37b287315ec341032fa99dbb8cfd` |
| `gen_object_rf_profile_fixture.py` | `9843b907df2ffaafff2d348105b2136e7df80854f08b09373e9915e9959c876f` |
| `test_gen_object_rf_profile_fixture.py` | `0f6d0fe6949d10e50d5d0167d18de13e6ecb77b63d01dffeed716ad70a571355` |
| `receipts/p3-1-log-check-claude.py` | `cb06747e1ceb7c6122ecee09cfc9a5a11d8b5386dbbf629c96f4c03f008cad37` |

## Исполнение

| Запуск | Итог | SHA-256 |
|---|---|---|
| `ricon-objprofile-claude-20261023-165827` | мой дефект: метод стенда `Node(...)` без `t.` перекрыт глобальным `Node` движка («Too many parameters»); переименован в `AddNode`/`AddRule` | `acc9ea86f296c9b7b3467ee82b4286b7e454d9d0e0617bc75d07190a8af41781` |
| `ricon-objprofile-claude-20261023-165857` | exit 0, SCRIPT(E) 0, один done: PASS; `cases=40 match=40 immutable=40 controls=6 extras=2 hostOnly=7` | `a036b703f2716642728e19eb7db6dd83ce404dea6a33063fa22d28114b7562b3` |

CRC: ObjectRfProfile 8dd3d098, Support 8dbb6564, Fixtures c4556322, Probe b833a706.

Независимая проверка лога: `python -I docs/evidence/receipts/p3-1-log-check-claude.py <log>
oracle/vectors/object_rf_profile_v1.json` → `P3_1_LOG_CHECK_PASS: 40 native, host-only 7 absent, SCRIPT(E) 0`.

Host: `TOTAL 50/50 PASS` — `docs/evidence/receipts/p3-1-host-claude-20261023.txt`, SHA-256 `7735b62cc66b71969cdb0bdf70601e4c22d6c36df0fbd8104d80656135f7063e` (LF).

## Границы

Не сделано и не заявляется:
- реальные API префабов, компонентов и материалов;
- достоверность значений профилей;
- вход, выход и проёмы, составные объекты;
- slab-расчёт P.2040-4 и частотные области материалов;
- производительность адаптера и кеширование.

P3 целиком не закрыт.
