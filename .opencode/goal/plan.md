# Активная цель: локальный радиосрез RICON

Работа ведётся в task:ba901e07-814a-4803-a1ae-3655d281a6b7. Team — источник решений; Git — источник редакций артефактов. Это навигация, не второй roadmap.

- Утверждённая цель с прямой поправкой владельца: [INTENT v0.2](<../../docs/req/INTENT-local-radio-slice.md>).
- Кандидаты и пределы доказуемости: [проверка осуществимости](<../../docs/evidence/E-von-marker-feasibility-lead.md>) и [исходный манифест](<../../docs/evidence/receipts/von-marker-feasibility-20261102-lead.json>).
- Опциональная звуковая ветвь: [локальный аудиопротокол](<../../docs/plan/LOCAL_AUDIO_VALIDATION_RU.md>).
- Наблюдённые аудиомаршруты: [квитанция](<../../docs/evidence/receipts/audio-route-inventory-20261102-lead.json>).
- Машинное состояние: [state.json](<state.json>).

Цель повторно утверждать не нужно: владелец подтвердил v0.1 на 249507a и явно разрешил валидную информацию вместо звука при том же пути/протоколе. Конкретный механизм и SAME_PATH_PACKET_PASS пока не доказаны.

Владелец прямо поручил автономное продолжение до завершения задачи после сообщения о процессном блокере (событие 799). Для текущей задачи записан FRESH_SESSION_WAIVED; строгая clean-session compliance не заявляется и повторного стопа ради смены контекста нет. Техническое ревью, условия безопасности и границы scope сохраняются.

[Probe specification](<../../docs/req/REQ-von-marker-probe.md>) сохраняет DRAFT транспортных фаз. Разовые D0/V0/I0 опыты завершены: [D0](<../../docs/evidence/E-D0-live-001.md>) подтвердил локальное событие A, [V0](<../../docs/evidence/E-V0-live-001.md>) дал INCONCLUSIVE_INPUT_OR_SINK, [I0 receipt](<../../docs/evidence/receipts/i0-live-001-lead.json>) — дискретную карту A и неполное invalid-окно B. QA1642 проверил I0; used gates не переиспользуются. Native TX не квалифицирован, SCRIPT_STATE не native idle/ownership.

[Штатная source-цепочка VONChannel DOWN/UP](<../../docs/evidence/E-standard-radio-ptt-source.md>) статически квалифицирована в восьми окнах; независимый review1838 закрыл только SOURCE_ACTION_MAPPING. Активные блокеры — BLOCKED_RUNTIME_ACTION_DELIVERY, BLOCKED_HOLD_RELEASE_SAFETY и BLOCKED_SELECTOR_OWNERSHIP. Следующий срез — DRAFT J0 non-VoN input-delivery preflight в инертном action/context: public SetActionValue объявлен, но его edge/listener semantics не документированы. Новый PTT опыт потребует отдельного bounded контракта, подтверждённого источника сигнала и независимого приёмного observable. Ни новый runtime, ни selector/capture/values-мутации, ни production actuator этим указателем не разрешаются. Ответственность передана Ядру / Логике / Последовательности событием1647; операторский слот освобождён прежним координатором.

Отдельные блокеры не скрываются: новый бинарный разбор требует применимого правового основания; unsigned reader wrapper не обходится без узкого текущего разрешения. Новая бинарная RE, DLL/драйверы, GUI-мутации и публикация автоматически не разрешаются.
