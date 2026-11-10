# Agy: проверка CT10 capture и RX entity diff

2026-11-10. Сводка codex.lead по фактическим сообщениям Agy в общей Agora,
а не отчёт от имени Agy. Conversation e5549ddb-ab94-4798-8edc-bd535874afda;
модель Gemini 4 Argon не менялась.

- Первый source verdict: Agora8312/8313/8314, journal2c9fbd51.
- Agy подтвердил пропуск incoming RX при EntityChanged/EntityUnbound без SUBSCRIPTIONS_CLEARED.
- Agy показал, почему blanket(player,generation) sweep может закрыть новую entity при неизменной generation.
- Root добавил source-only receiverEntityId, immutable entity для существующего link,
  exact old ENTITY_UNBOUND match, и manager derivation из actual Lifecycle getters.
- Повторный Agy diff verdict8317: SOURCE_REVIEWED_UNCOMPILED, новых source blockers нет
  на journal3b0a11e5/managerBDE099E3.

Agy явно подтвердил file-projection общей памяти: current-stage c43a…rev6,
contentHash f0ce1991…. Это operator readback, не собственный native memory API call.
Pins предоставлены Root; Agy не заявлял самостоятельное вычисление SHA.

Первый CLI запрос завершился provider unexpected EOF. Возобновлён тот же conversation,
не создавался параллельный повтор; после чтения файлов и transient retries итог получен.
Запрошенное разовое переключение модели не понадобилось.

Это независимый SOURCE review. Enforce compile, RunAll, actual role/RPC,
producer commit, source retirement и content/DSP/audio не проверены.
Полный технический результат: [E-ct10-integration-source-20261110.md](../evidence/E-ct10-integration-source-20261110.md).
