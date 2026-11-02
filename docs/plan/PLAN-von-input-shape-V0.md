# Bounded plan — V0 native VoN input-shape preflight

Версия 0.1, 2026-11-02. Статус: **APPROVED_FOR_LIMITED_V0_IMPLEMENTATION_ONLY** явным решением координатора под автономным поручением владельца 799 после VoN/QA технического ревью (события 1344/1348); не runtime и не qualified payload. Вход: код `9e280be`, D0 результаты `d2b58fb`, утверждённая цель с поправкой владельца. Подготовка ведётся в автономном scope события 799, FRESH_SESSION_WAIVED; строгая свежесть не заявляется. Полная [probe spec](<../req/REQ-von-marker-probe.md>) остаётся DRAFT для транспортных P0–P4; V0 — отдельное раннее исследование формы входа.

## 1. Один точный неизвестный

В одной свежей собственной A проверить возвращение/наблюдаемые эффекты **одного** public `VoNComponent.SoundEventPriority(eventname, values, priority, false)` с `SOUND_VOICE_PAIN_RELIEVE`, priority80 и явно созданным non-null пустым `array<float>` с Count=0. Документация даёт типы и «through VoN», но не семантику/cardinality/player-role: это **type-compatible гипотеза**, не заведомо безопасное или валидное сообщение. D0 квалифицирует иной overload, не этот.

Никаких переборов values, nonce, повтора после неизвестного исхода, capture или selectors/setters. Не обещается radio-only: обычная native emission по текущему direct/иному пути возможна. Запрет относится к изменению/подавлению direct и его настроек, не к заранее объявленному использованию vanilla emission в изолированном тесте; сохранность direct этим не доказана.

## 2. Scope реализации после review/фиксации

Enforce — только экспериментальные CommunicationDiscovery Game/controller/lifecycle/pure-check файлы; явный отдельный mode, сохраняющий D0 default/его строгую callback-политику. Source call-site ровно один на выбранный mode, old D0 не ослабляется. Общие guards/ready control допускается переиспользовать. Lead — отдельный mutually exclusive runner switch и публичный run-id/CLI mode обеим ролям. Core/seam/RF/установленная игра/Windows/Script-policy не меняются.

Отдельный log verdict/prefix V0, чтобы D0 и V0 не смешивались. Readiness/control RPC не несёт event/values/outcome, только run/mode/identity/generations/lifetime. Все code/compile/preconditions отдельно проверяются до live.

## 3. Пределы первого V0

- Только два своих свежих участника: A=host/local1 и B=Diag/local2 в новом MpTest; новые пустые scratch profile/log roots, raw вне Git. Контролируемое setup фиксирует build/addon snapshot, новые server-resolved refs/roles и отсутствие внешнего игрового ввода/дополнительных emitters. Это отдельное основание изоляции, **не вывод из тишины/совпадения callback** и не универсальная гарантия пустой native очереди. При фактической чужой активности/неустановленном владении — BLOCKED/INVALID, без вызова по предположению.
- Stable binding 5 с, quiet baseline 6 с, heartbeat1с/gap3с; baseline не заменяет ownership.
- Initial/fresh readiness с challenge+attempt, не более20 attempts/challenge через≥1500ms, t0/deadline не сбрасываются; fresh ACK не может быть устаревшим или другого binding.
- Перед native вызовом: новый Now после всех lookups/logging; A remaining и `B reported − RTT − age` **после 1000ms call-budget ≥32000ms**; RTT/age A≤1000ms. V0 deadline A **37000ms** от своего t0. Отдельный запас для fixed-window/tick scheduling; D0 числа не меняются.
- Массив non-null/Count0 проверяется непосредственно на mutation boundary; refs/mode/transmitRadio/generations сверяются read-only. Любой invalid/reserve/context/B identity veto запрещает вызов.
- Ровно1 call, ignoreQueue=false, `void` — возврат управления, не ACK; call-budget1000ms, overrun/crash/hang — отдельный INCONCLUSIVE.
- После фактического возврата **фиксированное окно 20000ms**, без требования A sound-start/finish, плюс ≥10000ms budget на controlled cleanup. Observer cap70000ms от первоначального t0; на возврате заново проверить current remaining обеих ролей ≥30000ms с actual RTT/age (без повторного вычитания уже прошедшего call-budget). Невыполнение инвалидирует окно и требует teardown, не нового вызова. Резерв до запроса оставляет запас scheduling. Detach/истечение окна не доказывают завершение native эффекта.
- Буфер ordinary4000+reserved96, всего4096; GAP/overflow/clock reset/rebind инвалидирует окно. Source counters, REQUEST_BEGIN/RETURN и native callbacks сохраняются без подстановки.
- Внешний процессный bound: runner300с/watchdog330с. При неизвестном поведении/cleanup — контролируемое завершение **только своих изолированных** ролей/процессов; не пытаться отменять чужое capture/очередь. Неизвестный prompt — stop.

## 4. Таблица callback-граней

| Событие | D0 | V0 после собственного native request |
|---|---|---|
| до вызова unexpected sound/capture/voice | abort без queue | такой же abort |
| sound-start/finish своего компонента | строгий известный D0 event/handle | **origin=UNKNOWN**, только raw collection/correlation, не доказанный результат A request и не отсутствие другого writer; A-finish не обязателен, wrong component/generation invalid |
| B native RX sender A, receiver известный local radio | ограниченное raw наблюдение | RAW_RADIO_ROUTE_UNQUALIFIED; не payload/radio PASS |
| B native RX sender A, null receiver | D0 не ослабляется | RAW_DIRECT_ROUTE_UNQUALIFIED; не изменение direct и не proof его сохранности |
| foreign sender/unbound non-null RX/rebind/gap | INVALID | INVALID |
| capture после native запроса | D0 строгий abort | raw UNCLASSIFIED_CAPTURE/INVALID и безопасный teardown; не назвать AI-recording или микрофоном без доказательства |
| нет callbacks, crash, timeout | по текущему D0 | INCONCLUSIVE_INPUT_OR_SINK / NATIVE_CRASH / NATIVE_TIMEOUT; не API-impossibility |

Readback comm method/TX до и после не доказывает отсутствие транзитных изменений. Не восстанавливать unknown/чужое состояние setters; при изменении/сомнении только сохранять evidence и завершать свои роли. Правильный remote/proxy sink ещё не доказан — нулевые локальные B наблюдения не означают отсутствие любой доставки.

## 5. Gates и результат

1. Техническое VoN/QA review этого узкого плана и V0 spec; точная запись редакции. Только затем явный limited implementation gate под поручением799.
2. Полный code readback/diff; default-D0 regression, отдельная V0 callback policy, non-null/count0/one-call/deadline/remaining/reserve/identity tests; headless compile/pure без мира/флага. Нет игрового P0 от одних tests.
3. Independent code/evidence review, pinned source/ops hashes, fresh roots, отсутствие чужого runtime и START/END. Единственный оператор Lead. Только один live V0.
4. Вердикты: REQUEST_RETURNED_ONLY / RAW_V0_OBSERVABLE / INCONCLUSIVE / NATIVE_FAILURE плюс отдельный cleanup. **Не SAME_PATH_PACKET_PASS, audibility, serializer, H2/direct preservation, MODEL_LOST или RICON actuation.**

Что может сломаться: values пуст не подходит native контракту, event/project несовместим, player-owned компонент не поддержан, выбран не тот B sink, native изменит неизвестное состояние или зависнет. Это причины ограниченного исследования, не основание скрыть риск или фальсифицировать приём. До последующих квалификаций B-content/протокола/решения RICON задача не завершена.
