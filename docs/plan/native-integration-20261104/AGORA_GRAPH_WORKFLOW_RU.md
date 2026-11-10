# Agora: работа с графом логики и кода RICON

Срез 2026-11-07. [Анкета агента](../../../AGENT_CONTEXT_RU.md) · [JSON графа](../../analysis/RICON_KNOWLEDGE_GRAPH.json) · [интерактивная карта](../../analysis/RICON_KNOWLEDGE_GRAPH.html).

## Назначение

Граф сокращает повторное чтение больших журналов: проблема → связанные узлы → owning source/evidence → один следующий факт. Он не заменяет прямое поручение владельца, текущий код, полный план или независимую приёмку. Нативные пробы и обычные source-модули различены явными статусами.

## Роли и владение

| Роль | Ответственность | Граница |
|---|---|---|
| Lead/root | Scope, назначения, объединение проверенных кандидатов и canonical публикация | Единственный native operator; разрешения берутся из текущего поручения |
| Documentation / Картотека | Назначенная карта задач и решений, JSON графа, ссылки, анкета и согласованные документы | Только назначенные файлы; сохраняет происхождение и scope доказательств; не подменяет независимый review |
| Core mapper | Реальные классы/методы/callers и contract gaps | Один назначенный fragment; runtime не додумывает |
| Native/evidence mapper | Current version, identity, metadata/cache и границы опытов | Не называет cached value DSP и не переносит historical offsets |
| Reviewer/хранитель цели | Уровень доказательств, ссылки, полнота требований, риск | Отдельный файл review; реализацию не принимает за весь Goal |

Роли публикации графа зарегистрированы в теме `ricon-reforger-plan`: `codex`, `codex.graph.core`, `codex.graph.native`, `codex.graph.review`; при перепроверке документации назначен `codex.documentation`. Assignment ledger и fragment/review artifacts публикации сохранены в соседнем `Cellular System/output/ricon-knowledge-graph-20261107`, кандидаты перепроверки — в `Cellular System/output/ricon-documentation-recheck-20261107`. Постоянный Host member `35cd977b-25ba-453e-965e-3234da01739d` («Картотека / Документация») является внешней ссылкой на состав команды; текущая runtime delegation не означает запуск его DSH session. Профиль, модель, история и задачи постоянного Host не меняются.

## Рабочий цикл

1. Lead выбирает problem ID и выполняет `python -B tools/knowledge/query_graph.py <ID>` из RICON-Reforger.
2. Назначение записывается в durable ledger задачи и Agora `note`: исполнитель, файлы, запрещённые действия, вопрос, ожидаемое доказательство и reviewer. Сообщение в prompt без записи владения недостаточно.
3. Исполнитель открывает связанные owning sources. Перед claim проверяет факт и публикует `think`, затем `claim/evidence` с problem/node IDs и `файл:строка`/receipt. Исторический protocol/intent не переопределяет новое прямое поручение владельца.
4. Reviewer проверяет actual code/evidence и один существенный возможный контрпример. Он не пишет тот же implementation-файл.
5. Documentation обновляет назначенные JSON, карту задач/решений, ссылки, пояснения и анкету по проверенным evidence с receipt и уровнем доказательства; отдельный reviewer проверяет кандидаты. Lead объединяет их, выполняет validation и canonical публикацию scoped итога. Если код не менялся, runtime-тест не требуется ради документационного PASS.

Один файл имеет одного исполнителя. Общей игровой средой управляет root. Завершённому агенту новое поручение передаётся через `followup_task`; `send_message` само по себе его не запускает. Агент не объявляется фоновым монитором после окончания его turn.

Lead назначает Documentation конкретный scope и reviewer в durable ledger и Agora. Передача evidence специалистом добавляет материал картотеки; она не передаёт право менять scope, создавать задачи или выполнять native-операции. Documentation не объявляет Goal завершённой по факту обновления документов.

## Шаблон назначения по графу

```text
Проблема: P_RT_RADIO_DESCRIPTOR
Узлы: N_RT_CONTROL_DESCRIPTOR, N_RT_OUTPUT_CACHE, C_VON_SEAM
Владение: точные файлы/артефакты; чужие изменения сохранить
Неизвестность: один identity/selector join
Источники: graph source refs → owning source и pinned evidence
Действие: минимальная проверка; scope source/runtime указан отдельно
Выход: новый факт, предел, hash/receipt и один следующий шаг
Reviewer: отдельная роль/файл
```

## Правила обновления знаний

- JSON — единый индекс узлов/рёбер/проблем/requirements. Mermaid и HTML являются его представлениями.
- Для source edges указывать метод/caller и файл:строка. `contract_input`/`requires` не превращать в actual call.
- Для native edges указывать version/image pin, attempt, identity и scope. Версия1.8 хранится как historical.
- Различать script index75, native parameter k1, input record18, numeric output candidate12 и source class30733.
- Capture10 даёт пять дискретных cache observations. SOURCE readback1, найденный output0, pointer или GUID не дают automatic Radio/DSP/content PASS.
- Смена source SHA/версии/identity требует проверки затронутых рёбер; не повторять неизменный failed опыт без новой переменной или диагностики.
- Все 21 раздел/этапы0–8, A/B, RECEIVER/SERVER, DIRECT/sharedRX, no-LOST-mute, калибровка/lifecycle/32users остаются в requirements. Документационная задача может быть завершена отдельно от полной native Goal.

## Оценка проблем

Сначала найдите границу между фактом и требованием. `C_P_ORCHESTRATION` показывает source islands без заявленной production-цепочки; `C_P_CT10` — отсутствующую миграцию temporal descriptor через snapshot v1; `P_RT_RADIO_DESCRIPTOR` — незакрытый native identity join; `P_MEDIA_DELIVERY_A` — отсутствие приёмки содержания/достаточной доставки. Machine-card status рассматривается отдельно: системный blocked не является доказательством дефекта VoN или отмены поручения владельца.

Новый успешный результат закрывает только свою проблему и scope. До закрытия полной цели требуется совокупная native-приёмка основного плана.
