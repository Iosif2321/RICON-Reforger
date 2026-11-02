# V0: native VoN input-shape на своём двухролёвом стенде

Дата: 2026-11-02. Оператор: Lead. Launch `0d22d3f`, frozen code `e31adee`, run `v0-native-001-20261102`.

**Вердикт: INCONCLUSIVE_INPUT_OR_SINK. Передача информации, допустимость пустого values и radio-source не установлены.**

## Факты

Использованы новые пустые profile/log roots A=WB listen-host1 и B=Diag2 в штатном MpTest. Создание/фактический native FS mount записаны в raw. Один public native `VoNComponent.SoundEventPriority` вызван с known-D0 event, priority80/false и созданным non-null `array<float>`, Count0 подтверждён на REQUEST_BEGIN. Значение семантически допустимым не объявляется.

[Квитанция](<receipts/v0-live-001-lead.json>) хранит оба непрерывных потока и raw SHA-256; [readiness](<receipts/v0-live-001-readiness.json>) независимо проверяет endpoint-часть, не перенос содержимого.

- A: ровно один REQUEST_BEGIN/END, возврат1мс. Mode до/после равен1, transmitRadio до/после null; прочитан enum numeric, его имя в этом прогоне не регистрировалось. **Готовый radio TX этим не доказан.**
- После actual native return код наблюдал фиксированное20-секундное окно; разность журнальных REQUEST_END→OBSERVATION_END20001мс. Отдельного exact return-tick поле в raw нет, поэтому последний интервал не выдаётся за прямое измерение внутреннего return-anchor.
- A: seq0…57 без GAP/INVALID, terminal/subscriptions0; B: seq0…71, readiness/heartbeat до собственного terminal LIMIT/subscriptions0.
- На наблюдаемых локальных sound/VoN компонентах обеих ролей callbacks0. Native crash и SCRIPT(E) не зарегистрированы. Вызов вернул управление, но не дал подтверждения принятия или доставки.

Нулевые callbacks не различают неизвестный input contract, player-role restriction, невыбранный radio/direct route, геометрию или другой remote/proxy sink. Привязка entity VoN к **активному** `SCR_VONController.GetVONComponent()` в этом прогоне не снималась. Ни API-wide FAIL, ни отсутствие любого remote playback из результата не следуют. Возвращённый void/нет crash не делает вход валидным.

## Завершение и пределы

Runner: DONE/RESTORE_OK, два точных стандартных Arsenal Yes, diff registry/default profiles0, источники стабильны. Host/client/оба watchdog завершены; повторная проверка дала ноль game/Workbench. Источники и ops не менялись в окне опыта. Caller не делал SetCapture/PTT, mode/TX/radio setters, не писал PCM/nonce. Это не доказательство отсутствия всех внутренних native эффектов или завершения произвольной очереди; controlled teardown касается только собственных ролей/процессов.

Sound callback origin по контракту UNKNOWN; в этом прогоне даже таких callbacks не было. Не заявляются SAME_PATH_PACKET_PASS, audible output, native serializer equivalence, H2/direct preservation или RICON actuation. Разовый V0 gate использован и автоматически не переиспользуется.

## Следующий недостающий факт

До нового действия полезен **пассивный mapping**: controller disabled/active component/entry vs entity component, настоящий TX/RX handle, A/B origins в одном мире и native replication identity. Protected script flags можно наблюдать отдельным TOOL_ONLY accessor только как SCRIPT_STATE, не native capture ownership. Пустой values или события не перебираются. При отсутствии qualified source/sink — BLOCKED/UNKNOWN, не подстановка через RPC и не ещё один локальный D0.
