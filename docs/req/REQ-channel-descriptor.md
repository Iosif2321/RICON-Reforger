# CT10.1 — переносимое описание временного канала

Версия **0.1 AGENT-REVIEWED**, 2026-10-23. Автор codex, независимый checker и Enforce — claude.
Основание: `REQ-channel-time.md` CT04–CT10, `REQ-channel-fading.md` v0.2, карта CT10 и Agora #3580/#3591.

## Назначение и границы

**CD01.** Это подготовительный чистый блок CT10: проверка/глубокая копия CF-дескриптора, ограниченный
самодостаточный формат `RCFD/1` и перенос якоря без смены того же процесса. Он не меняет snapshot v1,
producer/registry/receiver/seam, не публикует данные клиентам и не закрывает CT10 целиком.
Будущая миграция schema 2 всех потребителей проводится одним согласованным изменением контракта.

Вход дескриптора поступает от доверенного серверного builder. Кодек проверяет структуру, а не происхождение:
поле «я сервер» внутри сообщения не даёт доверия. Адресность, проверка реального отправителя, CT06-clock mapping,
смена физических параметров и голос остаются задачами интеграции. В RCFD нет leaseId/linkId/epoch/PTT.

**CD02.** Допустимы только дескрипторы CF03/CF04 с `modelId = FINITE_MULTIPATH_V1`. Типизация и порядок ключей
полностью из CF. `resourceExhausted`, `coverageComplete`, нулевые веса/пустой набор и dopplerQ вне физического домена
сохраняются: корректно перенесённое описание может давать отказ вычисления канала. `OK` кодека не означает связь.
Отдельного ПСЧ, словаря компонентов, сжатия/delta и производственных параметров по умолчанию нет.

## Нормальная форма и операции

**CD03.** Все числа сначала проходят CF-типизацию. Все нулевые powerWeight канонизируются к **+0**;
остальные binary32 передаются точно. Это не отсечка малого положительного веса: 2^-149 сохраняется.
Поля/порядок компонентов/ключи не сортируются и не достраиваются — неверный вход отклоняется по CF.
Выходы — новые вложенные объекты; ссылки на вход и между результатами не разделяются.

**CD04. Encode.** Вход `{descriptor, maxBytes}`. maxBytes — строго int32 1..65535; это бюджет вызывающего кода,
не физический порог. Порядок: INVALID_LIMITS → INVALID_DESCRIPTOR (CF) → UNSUPPORTED_MODEL → RESOURCE_LIMIT → OK.
При OK результат `{code, descriptor, bytes, sizeBytes}`; bytes — список целых байтов 0..255 без bool.
При отказе только `{code}`; частичных байтов/дескриптора нет.

**CD05. Decode.** Вход `{bytes, maxBytes}`. Порядок:

1. INVALID_LIMITS: maxBytes вне CD04.
2. INVALID_FRAME: bytes не список.
3. RESOURCE_LIMIT: число байтов больше maxBytes; до обхода элементов/разбора тела.
4. INVALID_FRAME: длина меньше 7 либо больше структурного максимума 5111, любой элемент не int 0..255
   (bool запрещён), неверный magic или длина из заголовка.
5. UNSUPPORTED_VERSION: версия не 1; проверяется после целостности заголовка, до тела.
6. INVALID_FRAME: неполное тело, недопустимые флаги/счётчики/ASCII-строки, trailing bytes, либо неверный CF-дескриптор.
7. UNSUPPORTED_MODEL: корректный по CF ключ другой модели.
8. INVALID_FRAME: нарушена каноничность — повторная Encode не совпадает с входными байтами
   (например, отрицательный нулевой вес). Переформатирование плохого пакета не подменяет отказ.
9. OK: результат как у Encode, с новой копией bytes/descriptor.

Ошибки кодека не означают LOST и не разрешают отсечку P1. Решение о diagnostic/resync принимает будущий receiver.

**CD06. Rebase.** Вход `{descriptor, anchorMs}`. Порядок: INVALID_DESCRIPTOR → UNSUPPORTED_MODEL →
OUT_OF_MODEL_DOMAIN (хотя бы один |dopplerQ| > M/4) →
INVALID_ANCHOR (не int32 либо вне `[validFromMs, validUntilMs)`) → OK.
Новый descriptor копирует старый, меняет anchorMs и validFromMs на anchorMs входа; validUntilMs сохраняется.
Для каждой составляющей новая phaseQ — фаза старой составляющей в этот момент при subUs=0 по CF06;
powerWeight, dopplerQ, идентичность, профиль, revision и флаги сохраняются.

Rebase не требует покрытия/ресурса для вычисления известной фазы, но не расширяет объявленную физическую область.
При успехе `{code, descriptor}`, при отказе только `{code}`. Для любого последующего допустимого времени
старый и новый descriptor дают те же phasesQ и коэффициент. Смена частоты/весов/доплера или резкая смена среды
не является Rebase; её непрерывность/причина пока не реализованы.
Для допустимых t1 ≤ t2 композиция `Rebase(Rebase(D, t1), t2)` даёт тот же descriptor, что `Rebase(D, t2)`.

## Формат RCFD/1

**CD07.** Все многобайтовые значения — big endian; i32 — знаковый two's complement, f32 — IEEE binary32.
Строка key = u8 длина 1..64 + столько же ASCII-байтов из CF. Последовательность строго следующая:

| Порядок | Поле | Размер |
|---|---|---|
| 1 | magic ASCII `RCFD` | 4 байта |
| 2 | версия = 1 | u8 |
| 3 | длина всего сообщения, включая заголовок | u16 |
| 4 | modelId, profileId | две key |
| 5 | profileVersion | i32 |
| 6 | a.deviceId, a.transceiverIndex, b.deviceId, b.transceiverIndex | четыре i32 |
| 7 | frequencyMHz, bandwidthHz | два f32 |
| 8 | environmentRevision | i32 |
| 9 | anchorMs, validFromMs, validUntilMs | три i32 |
| 10 | флаги: bit0 staticChannel, bit1 coverageComplete, bit2 resourceExhausted; остальные = 0 | u8 |
| 11 | число составляющих 0..64 | u8 |
| 12 | для каждой: componentId, powerWeight, phaseQ, dopplerQ | key + f32 + i32 + i32 |

Размер `55 + len(modelId) + len(profileId) + Σ(13 + len(componentId))`.
Структурный максимум с ключами по 64 байта и 64 составляющими — **5111** байт;
для поддерживаемого modelId длины 19 — **5066** байт. Это размер **этого формата**, без Rpl/RPC/сетевых заголовков.
Бюджет задаёт вызывающий код; равенство бюджету допускается. Превышение не приводит к обрезанию списка.

## Проверка, API и незакрытые пункты

**CD08.** Эталон Python stdlib использует принятый CF preflight и struct для binary32/int32.
Обязательны: независимая golden-строка байтов, round trip, минимум/максимум binary32, +0/-0, 64 компонента,
граница бюджета, неверная длина/magic/версия/ASCII/флаги/число компонентов, NaN/Inf, отсутствие алиасов,
Rebase на границах срока/int32 и точное совпадение последовательности при новых якорях.
CF/старые snapshot-векторы не меняются. Ошибки JSON-типа/структуры явно отмечаются host-only.

Реальный адаптер Enforce выбирается и проверяется отдельно. Официальный API подтверждает фиксированные
Read/Write и позиции в битах, но не доказывает интероперабельность RCFD с внутренним ABI/порядком байтов движка:
[ScriptBitWriter](https://community.bistudio.com/wikidata/external-data/arma-reforger/EnfusionScriptAPIPublic/interfaceScriptBitWriter.html),
[ScriptBitReader](https://community.bistudio.com/wikidata/external-data/arma-reforger/EnfusionScriptAPIPublic/interfaceScriptBitReader.html).
Порядок/ширина записи и чтения должны совпадать:
[Multiplayer Scripting](https://community.bohemia.net/wiki/Arma_Reforger%3AMultiplayer_Scripting).
Сжатый half/Float01 не заменяет точную передачу CF-параметров. Скорость/нагрузка сервера и сетевые накладные
расходы пока NOT_VERIFIED. Ни Rpl-доставка, ни клиентские часы, ни аудиопоток этим кодеком не заявляются готовыми.
