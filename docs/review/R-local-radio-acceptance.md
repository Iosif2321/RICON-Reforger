# V5 — независимая матрица локальной radio acceptance

2026-11-02. QA / ревьюер. Task task:7c0d6220-a985-4d6d-b306-5b878944aad5. **MATRIX_READY / FINAL_BLOCKED**. Никакого общего LS PASS или рекомендации закрыть цель.

Входы прочитаны: [intent v0.2 LS-01…08](<../req/INTENT-local-radio-slice.md#L49-L64>), [аудиопротокол](<../plan/LOCAL_AUDIO_VALIDATION_RU.md>), [state](<../../.opencode/goal/state.json>). Наблюдаемый baseline HEAD `a04a14f125fe757ad0038d762e2b44ab4819085b`. Frozen D1/I1/I2/V1/V3/V4 и завершённые sealed X1 серии в этот review ещё не переданы. Исторический B0e указан intent как PARTIAL и не получает нового независимого PASS от данной матрицы. Runtime/raw payload не читались и не создавались.

## Evidence и seal до проверки содержимого

Для каждой попытки нужны exact source/build/Git/SHA, run/role/binding generations, заранее frozen decoder/threshold/window, сырые TX/RX locators и SHA, terminal/cleanup, все failures и отрицательные контроли. Независимый reviewer проверяет raw, а не только строку PASS или CRC. Корректность хэша не доказывает источник либо protocol equivalence.

Пакетная ветвь: B декодирует независимо; его output и terminal закрыты, seal/hash зафиксированы ДО раскрытия A nonce/seed/расписания. Нельзя заранее читать A-содержимое reviewer'ом, обслуживающим B-decode, передавать его B через общий файл/RPC/память или строить результат из известных ожидаемых битов. Порядок seal→reveal должен быть воспроизводим по receipts, а не утверждён задним числом. Если порядок нарушен — независимость не доказана, положительный результат отклоняется.

Аудиоветвь: нужны доказанные physical A input, изолированный B-output и leakage controls. Общий USB-микс со сторонней сессией не записывать и не принимать; выбор имени endpoint не равен изоляции. Пакетная ветвь не обязана проходить аудиомикшер, но требует доказанного того же native пути/протокола.

## Матрица LS

Все статусы относятся к новой итоговой X1 evidence, не ретроспективной переоценке старых опытов.

| ID | Независимое условие и false-positive challenge | Статус / недостающая evidence |
|---|---|---|
| LS-01 | Stable A/B process/player/entity/radio bindings и generations; rebind/gap должен инвалидировать окно, одна частота не заменяет match | BLOCKED: frozen D1/I1 + X1 role/binding raw; intent baseline PARTIAL |
| LS-02 | Отличимый свежий A-сигнал; уникальный marker заранее неизвестен B; локальный PlaySound/request/известный seed не считается передачей | NOT_RUN: sealed series отсутствуют |
| LS-03 | Независимое распознавание B; same native path/protocol для пакета либо isolated B audio; callback-only, separate RPC и local RX injection отклоняются | BLOCKED: доказательство пути и raw B отсутствуют |
| LS-04 | Повторные positives со свежим содержимым; no-transmit, broken-path/absent-B, local-TX-without-send controls дают отсутствие ложного кадра при здоровом observer | NOT_RUN: все X1 попытки/controls отсутствуют |
| LS-05 | Реальный RICON reason→DESIRED→APPLIED→OBSERVED меняет доставку на проверенном участке, затем recovery; synthetic LOST, manual mute и decoder-off не засчитываются | BLOCKED: frozen actuator contract/I2/V4 и X1 отсутствуют |
| LS-06 | Независимый direct control до/во время/после radio change; null-filtered observer или неквалифицированный radio marker не доказывает direct preservation | NOT_RUN: direct raw и квалификация observable отсутствуют |
| LS-07 | Возвращены только owned states, subscriptions detached, процессы exited; unknown ownership запрещает unmute; incomplete terminal/exit не даёт cleanup PASS | NOT_RUN: complete X1 cleanup отсутствует |
| LS-08 | Exact SHA/source/build/raw/decoder/thresholds/все попытки; B seal до A reveal, повторяемая инструкция и независимое сравнение | BLOCKED: complete sealed bundle отсутствует |

## Правила финального вывода

PASS выставляется только конкретному LS и проверенному source/build/raw SHA. FAIL означает наблюдённое нарушение, NOT_RUN — неисполненную проверку, BLOCKED — отсутствующие prerequisite/evidence; их нельзя сворачивать в общий PASS. Потеря binding/observer/log integrity не считается успешным отрицательным контролем. Единственный удачный опыт, exit0, N0 pure, compile или callback-only не закрывают LS-03/05/06. Неуспехи не удаляются и не исправляются подменой decoder/порогов.

Lead передаёт завершённые X1 execution note/receipts и sealed raw locators после зависимых reviews; тогда QA продолжает независимую оценку. Без закрытия любого обязательного LS общую цель не закрывать. Запусков, игры, аудиозахвата, ввода, изменения decoder/thresholds/чужих результатов и commit в этом scope нет.
