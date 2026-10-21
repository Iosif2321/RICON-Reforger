# S3b — формирование снимков направленной связи

Версия **0.1 DRAFT**, 2026-10-21. Автор codex, проверяет claude. Источник истины — этот репозиторий.
Основание: Intent (сервер решает качество, модульность), owner agora #2936/#2937 и поручение в чате codex
«приступай к работе с партнёром»; миниплан #3242/#3247/#3249/#3250, роли в ROADMAP `accb775`.
Входные контракты: REQ-pair-service v0.2 (S3 `fd2365c`), REQ-core-contracts §8–9,
RICON_LinkSnapshot и RICON_CommsManagerComponent.NextLinkSequence.
Применены Ponytail full и ai-native-sdlc-cycle с существующими именами артефактов проекта.
Ревью, независимый пересчёт корпуса и freeze ещё не выполнены. Enforce-код до freeze не начинается.

## Цель и границы

Из результатов одного такта S3 получить детерминированную партию RICON_LinkSnapshot и новое состояние
реестра. Никаких RPC, отправки пакетов, автоматических тиков, поиска раций, мира, звука и actuator.
Это формирование данных; слово «публикация» ниже означает выдачу вызывающему коду, а не сетевую доставку.
Snapshot receiver, менеджер, lifecycle, S3 и RF-формулы не меняются.

Самый рискованный переход — смена общей эпохи при выдаче seq посреди партии. Частичная партия запрещена.
Альтернативы отклонены: ожидание TTL вместо завершающего LOST; собственный seq allocator в production;
общий routeId DIRECT для всех путей; повторное использование linkId внутри эпохи.

## 1. Вызов и состояние

**REQ-LP-01.** Серверный вызов `Publish(input, registry, source)` возвращает результат и новый реестр.
Входные объекты, включая старый реестр, не меняются. Выходы не ссылаются на входы; выдаваемые снимки и
новый реестр также не разделяют изменяемые объекты. Вызывающий код владеет реестром и заменяет его
возвращённой версией. Служба не хранит глобального/скрытого состояния и не содержит engine handle.

`input`: nowMs, sessionEpoch, ttlMs, qualityDelta, limits, bindings (`RICON_TransmissionBinding` S3),
pairResult (`RICON_PairServiceResult`). Это данные **одного** серверного такта; источник доверия — вызывающий
сервер, а не клиент. S3b проверяет согласованность проекции, не пересчитывает S3/S2 или бюджет.

| Поле | Домен |
|---|---|
| nowMs | int 0..MAX, локальное монотонное время сервера |
| sessionEpoch | int 1..MAX−1, домен snapshot строже S3 |
| ttlMs | int 1..MAX; presentation TTL, не срок lease |
| qualityDelta | конечное float, 0 < значение ≤ 1 |
| limits.maxBindings | int 1..4096 |
| limits.maxPairs, limits.maxActiveLinks | int 1..65536 |

MAX = 2147483647. Все флаги — bool. Целые не принимают bool или дробное число.
Поля binding и их домены — REQ-PS-01 целиком; неактивные и старые epoch допустимы как данные S3.

**REQ-LP-02.** Реестр — объект значений:

- sessionEpoch: 1..MAX−1;
- highWaterLinkId: 0..MAX — наибольший когда-либо выделенный id в этой эпохе, включая невыданную партию;
- lastTickMs: −1 (ещё не было успешного такта) либо 0..MAX;
- needsFullResync: bool;
- entries: список ранее выданных и ещё не завершённых связей, не больше maxActiveLinks.

Начальное состояние: текущая epoch, highWater=0, lastTick=−1, needsFullResync=true, entries=[].
Запись хранит ключ `(leaseId, rxDeviceId, rxTransceiverIndex, rxPlayerId)`, txPlayerId, txDeviceId,
txTransceiverIndex и последние **выданные** linkId, linkSeq, state, quality, ttlMs, publishedAtMs.
Домены id/index — S3; linkId 1..highWater; seq 1..MAX−1; Q конечное [0,1], допустимый state;
LOST требует Q=0; ttl>0; 0≤publishedAt≤lastTick. Если lastTick=−1, entries обязаны быть пусты.
Отсутствующие/завершённые связи удаляются, highWater не уменьшается. Начальное состояние создаёт вызывающий код.

## 2. Предварительная проверка

**REQ-LP-03.** Проверки идут отдельными глобальными проходами, в порядке таблицы. До их завершения
ни один метод source не вызывается. Любой отказ здесь: snapshots=[], sequenceCalls=0, epochReads=0,
requiresFullResync=false и глубокая копия исходного registry (null остаётся null).
Ошибочный вход не делает прежнюю версию реестра корректной.

| Код (значение) | Первое применимое условие |
|---|---|
| NULL_INPUT (0) | null input, registry, source, limits, bindings, pairResult, pairs или entries |
| INVALID_LIMITS (1) | лимит вне §1 |
| LIMIT_EXCEEDED (2) | bindings>maxBindings, pairs>maxPairs, entries>maxActiveLinks |
| INVALID_ELEMENT (3) | поле input/binding/pair вне домена или null-элемент binding/pair |
| INVALID_REGISTRY (4) | поле/элемент registry вне §1 или null entry |
| DUPLICATE (5) | повтор leaseId или txDeviceId в bindings; повтор ключа пары; повтор ключа или linkId в entries |
| UPSTREAM_REFUSED (6) | pairResult.callCode != OK; известный код S3, но весь такт S3 отказал |
| IDENTITY_MISMATCH (7) | пара не найдена по leaseId; txDeviceId не совпал с binding; у существующего ключа сменились txPlayerId/txDeviceId/txIndex |
| CLOCK_ROLLBACK (8) | registry.epoch=input.epoch и lastTick≥0 и nowMs<lastTick |
| ACTIVE_LIMIT (9) | число сохраняемых и новых связей после §3 больше maxActiveLinks |
| ID_EXHAUSTED (10) | новым связям не хватает MAX−highWater id; wrap/reuse запрещены |
| NEED_FULL_RESYNC (11) | только динамическая граница §5, после успешного preflight |
| OK (12) | партия успешно сформирована |

callCode S3 должен быть известным enum NULL_INPUT..OK. Пара: ключевые поля в доменах S3,
pairCode 1..9, hasDeviceCode/hasBudget/valid bool, deviceCode известный enum S2 (0..11), reason enum 0..16,
state enum LOST..CLEAN, конечные sinrDb и Q∈[0,1]. hasBudget эквивалентен EVALUATED;
hasDeviceCode эквивалентен DEVICE_QUERY или EVALUATED. Для EVALUATED deviceCode=READY;
для DEVICE_QUERY deviceCode≠READY. В остальных случаях deviceCode=NULL_INPUT.
При отсутствии бюджета: valid=false, reason=NUMERIC_FAILURE, state=LOST, Q=0, sinr=0.
При наличии: valid эквивалентен reason=OK; invalid требует LOST/Q=0/sinr=0; LOST всегда Q=0.
Счётчики S3 s2Calls/evaluatorCalls — int 0..MAX; S3b их не использует и не пересчитывает.
Неизвестный enum/структурный дефект имеет INVALID_ELEMENT даже при отказе upstream.

Для ACTIVE_LIMIT и ID_EXHAUSTED учитываются правила §3, а при другой registry.epoch старый реестр
считается пустым, highWater=0. До epoch-reset из §5 смена метаданных старого ключа и CLOCK_ROLLBACK
проверяются только при совпадающей epoch. ID_EXHAUSTED требует, чтобы вызывающий код сменил epoch через
существующий authority manager и пересчитал S3. S3b сама epoch не создаёт.

## 3. Идентичность и выбор снимков

**REQ-LP-04.** Направленный ключ — `(leaseId, rxDeviceId, rxTransceiverIndex, rxPlayerId)`.
Каждый новый lease (новое принятое нажатие PTT) создаёт новую связь; старый ключ завершается.
Два индекса одной рации у слушателя — две связи. Адресат каждого снимка — listenerPlayerId=rxPlayerId.
talkerPlayerId и TX index берутся из binding по leaseId, а не угадываются из id устройства.
Один talkerPlayerId может совпадать с listenerPlayerId: snapshot-контракт этого не запрещает;
это не делает VoN binding пригодным, у seam свои более строгие правила.

**REQ-LP-05.** На основе текущих пар и старых entries строится объединение ключей в возрастающем порядке.

| Пара | Старой записи нет | Старая запись есть |
|---|---|---|
| EVALUATED и valid budget | создать связь с budget.state/Q, в том числе LOST/Q=0 | сохранить id и взять budget.state/Q |
| EVALUATED и invalid budget; STATE_MISMATCH; DEVICE_QUERY | не создавать id/снимок | сохранить связь, целевое состояние LOST/Q=0 |
| код 1..6 или ключ отсутствует | ничего | один завершающий снимок LOST/Q=0 с новым seq, затем удалить запись |

Завершающий снимок обязателен даже когда старый state уже LOST. При исчезновении binding используются
метаданные старой записи. Новым связям id выдаются по сортировке ключей: highWater+1, без overflow.
Для существующей связи id не меняется. Реестр после OK отсортирован по ключу.

**REQ-LP-06.** Снимок сохраняемой связи нужен, если она новая, needsFullResync=true, state изменился,
`abs(Q−lastPublishedQ) >= qualityDelta`, ttlMs изменился или возраст последней выдачи не меньше
`ceil(ttlMs/2)`. Округление без overflow: `ttlMs/2 + ttlMs%2`, минимум 1.
Порог Q сравнивается с последним **выданным**, не с прошлым рассчитанным значением: малые изменения копятся.
Если снимка нет, запись полностью сохраняет прежние published-поля. Возраст — now−publishedAt после проверки
времени, сумма now+ttl не используется. Изменение одной qualityDelta само по себе не вызывает выдачу.

**REQ-LP-07.** Каждому нужному снимку соответствует ровно один NextLinkSequence. Порядок обращений source и
снимков — общий порядок ключей, включая завершающие. schemaVersion=1, epoch=input.epoch,
routeId=linkId, routeNodeCount=2 (два endpoint, одно звено — явная RICON-конвенция S3b).
ttlMs — текущий input.ttlMs даже у завершающего. Остальные поля — §1 и §3, LOST всегда Q=0.
Полный budget и ключи сети в snapshot не копируются. Результат отдельно не выдаёт RICON_VoNDeviceBinding.

## 4. Источник последовательностей

**REQ-LP-08.** `RICON_LinkSequenceSource` имеет ровно `GetSessionEpoch()` и
`NextLinkSequence(linkId, out seq, out rotated) -> bool`. Production-адаптер только делегирует существующему
manager, null manager даёт epoch=0/false; проверка authority остаётся в manager.NextLinkSequence.
Base source fail-closed (epoch=0/false). Fake source в тестах имеет реальный счётчик вызовов.

После preflight вызывается GetSessionEpoch **один раз**, затем после **каждого** NextLinkSequence ещё один раз,
даже если Next вернул false/rotated/некорректный seq. Иных чтений source нет.
Результат выдаёт sequenceCalls и epochReads по фактическим обращениям, включая неуспешное.
Корректный source при успехе и без rotated выдаёт seq∈1..MAX−1; для старой записи он строго больше lastSeq.
Гарантию отсутствия глобальных races обеспечивает серверный вызывающий код: один writer/тактовый вызов.
Production не использует Python-эталон или тестовый allocator.

## 5. Атомарность и epoch

**REQ-LP-09.** Если первое чтение source не равно input.epoch, либо registry.epoch отличается от input.epoch,
возвращается NEED_FULL_RESYNC: снимков нет, seq-вызовов 0, epochReads=1. Новый пустой реестр имеет
валидную epoch source (если source невалидна — input.epoch), needsFullResync=true, lastTick=−1;
highWater сохраняется только если новая epoch равна прежней registry.epoch, иначе 0.
Это не публикация пустого resync: вызывающий код должен согласовать epoch, пересчитать S3 и повторить вызов.

**REQ-LP-10.** До первого Next выделяются все id новых связей (только локальная новая версия реестра).
Если любой Next вернул false, rotated=true, невалидный seq, seq≤lastSeq либо прочитанная после него epoch
отличается от input.epoch, вся партия отбрасывается: NEED_FULL_RESYNC, snapshots=[], requiresFullResync=true.
Дальнейшие Next не вызываются. Выходной реестр пуст, needsFullResync=true, lastTick=−1.
При прежней epoch сохраняется highWater с **всеми** подготовленными id; при новой epoch highWater=0.
Невалидная epoch source заменяется input.epoch. Source уже мог изменить менеджер: пропуски seq не откатываются.
Старый registry всё равно неизменен; вызывающий код обязан принять возвращённый reset-state при этом коде.

**REQ-LP-11.** При OK новый registry.epoch=input.epoch, lastTick=nowMs, needsFullResync=false.
requiresFullResync равен входному registry.needsFullResync. Если true, выданная успешная партия содержит все
оставшиеся связи (и возможные завершающие старых); для **полного resync** вызывающий код исключает
завершающие снимки по отсутствию linkId в новом реестре. Для обычной delta-партии завершающие нужны.
При начальном пустом такте OK с requiresFullResync=true и snapshots=[] позволяет установить пустую identity.
Генерация полной партии не доказывает её доставку; ошибки отправки — ответственность будущего транспорта.

NEED_FULL_RESYNC всегда требует полного обновления независимо от входного флага. При preflight-отказах
requiresFullResync=false, реестр не продвигается. Никакая не-OK партия не отправляется получателям.

## 6. Проверка и передача партнёру

**REQ-LP-12.** Корпус `oracle/vectors/link_publication_v1.json` хранит независимые случаи одного вызова:
input, registry, сценарий fake source (epoch, seq по linkId, failAt/rotateAt/invalidAt), ожидаемые
callCode, snapshots, новый registry, sequenceCalls, epochReads, requiresFullResync и состояние fake source.
Цепочки между тактами дополнительно проверяются unit-тестами. Все времена/Q/идентичности — TEST_ONLY.
Числа Q в корпусе представлены точно в float32; рядом с порогами используются двоично точные доли.
Коды/ключи/поля snapshot/state/counters сравниваются точно; Q допускает 1e−6 при сравнении Enforce,
но ветвление по qualityDelta — строго, без tolerance.

| Проверка | Обязательное покрытие |
|---|---|
| CT-LP-001 | первый full resync, пустой такт, повтор без выдачи, порядок/перестановка |
| CT-LP-002 | накопление Q, равенство порогу, state/ttl change, нечётный TTL, TTL=1/MAX |
| CT-LP-003 | все девять кодов пары с/без старой записи, invalid budget, valid LOST |
| CT-LP-004 | пропавшая связь, новое PTT, два индекса, owner-scope, отсутствие повторного id |
| CT-LP-005 | все preflight-коды и приоритет нескольких дефектов, source calls=0 |
| CT-LP-006 | внешняя смена epoch, mismatch/невалидный source, ротация после первого готового снимка |
| CT-LP-007 | false/невалидный/stale seq, границы MAX−1/MAX, ID exhaustion, предел active |
| CT-LP-008 | немутируемость и отсутствие alias, реальные счётчики source, контрольные порчи компаратора |

Команды: `python oracle/link_publication_oracle.py --check oracle/vectors/link_publication_v1.json`;
`python -m unittest discover -s oracle -p test_link_publication_oracle.py`.
Сначала claude проверяет REQ, затем пересчитывает корпус по нему, не читая реализацию эталона.
После согласования фиксируем SHA; далее claude делает Enforce/fixture/generator/probe и расширяет regression.
Codex независимо читает исходник и повторяет Workbench. До этого этап не закрыт.
STATUS/ROADMAP ведёт claude. Игровая приёмка, реальный manager authority, доставка, JIP и нагрузка остаются NOT_RUN.
