# Независимый implementation review RICON/native

Дата: 2026-11-04. Исполнитель codex.impl.review. Владение: только review/. Canonical и файлы коллег не изменялись; engine/SDK/Ghidra/binary не запускались. Применён ponytail-review: один targeted pass по transfer, actual apply, Goal и готовящемуся generic-message case; завершённый RE не повторялся.

**Итог: ACCEPT — ACTUAL_SOURCE_APPLY_VERIFIED.** Два shadow source, generic source и первоначальные три Goal файла независимо проверены как фактически применённые. Generic callback→PairService→information receipt canonical побайтно равен reviewed SHA `c0185fa5222bd7b926bf3f81b77c1c9ad1322d228c9ef7e689bb88cd6d679ce1`; его actual immutable backup равен before1495DBCA.... Must-fix не найдено. Scope generic — TOOL_ONLY ONE_RX same-world informational diagnostic, не голосовой actuator. SDK/Enforce/native/audio NOT_RUN.

## Что делает изменение

Root перенёс два ранее принятых default-off TOOL_ONLY shadow source в canonical. Реальные getter observations поступают в существующий PairService с явно тестовыми binding/RF/input DTO; результат только логируется, голос не изменяется. Три project Goal файла сохраняют полную цель и историю, добавляя актуальный implementation phase без объявления общей готовности.

## Проверено независимо

- Canonical helper равен accepted payload побайтно: SHA389F5BA2C2C8047A331D578CAE74B247504B3A769F704191ACF105964168B5AC. Canonical Game hook равен accepted SHA71165C07FD12B2E83E9E81F9D6F56AF431457400E06D724A8B0BFE5E4671F5AD. Все12 pinned dependency hashes совпали; четыре исходных freeze/handoff/patch/checker hashes также сверены.
- Hook diff относительно immutable baseline14CE019A... ограничен ref shadow, его opt-in start, Tick и очисткой на start/end/world cleanup. Старые probe/control вызовы сохранены. Нового native actuator или production Core изменения нет.
- Helper сохраняет source identity, radio/transceiver/catalog и повторно читает наблюдения после Evaluate. completed clock берётся после post-getters и placement binding check; contextCurrent отражает полученную свежесть отдельно от raw результата. Изменение identity прекращает серию; восемь evaluation/односекундный poll ограничивают этот bench. Наблюдения не названы атомарными; index0/fixture generation/profile/antenna origin и lifecycle authority ограничения явно присутствуют в receipt.
- Scope raw результата содержит TEST_ONLY_SHADOW/authority NONE/nativeEffect NONE/perpairActuator NONE/BLOCKED_NO_ACTUATOR. Техническая POWER_OFF/CHANNEL/KEY ошибка не превращается в valid RF MODEL_LOST. Нативная частота, model frequency и fixture профили не выданы за калиброванную речь/физику.
- transfer.py/check_transfer.py/manifest прочитаны. Payload/base/dependencies проверяются до apply, verify сравнивает точные bytes, rollback предварительно требует неизменённый accepted result. manifest targets ограничены repository; не предполагается транзакция при power/process termination. Не запускал apply/rollback по canonical.
- Независимые filesystem fixture checks выполнены в temporary directory только под review/: apply+verify exact, повторный apply отказ, rollback поверх concurrent edit отказ, точный baseline rollback, dependency drift отказ до helper write. PASS5/5, initial-checks.json. Это filesystem selfcheck, не Enforce/native test. Existing8 transfer checks и27 source/DTO/Python checks прочитаны как отдельные подготовительные receipts, не сложены с runtime-доказательствами.
- Actual Goal .opencode/goal/state.json/plan.md/log.md совпадают с подготовленными файлами и after hashes apply-receipt. Backup hashes совпали с base hashes. В state из прежних ключей изменён только activePhase; добавлены complete=false/currentCodexIntegration. Все остальные старые поля, полная goal и historical approval сохранены. Plan/log имеют исходные bytes как префикс и только dated append. Machine Goal не подменён project state и не объявлен resumed/complete.

## Findings

Must-fix по shadow/Goal actual bytes не найдено. Один reporting caveat передан root: READY.md/access RESULT и initial Goal append описывают staged/pre-apply момент. Текущий факт — exact canonical apply подтверждён; итоговый root report должен различать эту хронологию и ещё не применённый generic candidate. Сам подготовительный snapshot не считается ложным runtime PASS.

## Generic — один targeted source review

Изучены весь staged RICON_LiveGenericRadioProbe.c, immutable before.c, generic.patch, checker и README, а также настоящий caller StepGenericRadio в существующем bench: fresh probe создаётся для двух принадлежащих стенду radios, Start вызывается один раз, Step terminal приводит к Finish/Stop и удалению собственных fixture. Legacy mode сохранён. Canonical generic на момент source-check равен before SHA1495DBCA6E655033F86D5A344BBE1B876C14EF9AAB6DBAE0C32F11F44F07E160.

Проверенные свойства:

- Только явный CLI `riconGenericPair=1` выбирает новый режим, отсутствующий флаг оставляет старую phase machine; иное значение возвращаетfalse до прежних mutations. Сам новый флаг не запускает bench.
- Message OnDelivery сначала увеличивает прежние native raw counters и печатает прежние строки. Условный вызов receipt добавлен после них. Нельзя получить ожидаемый negative от замалчивания callbacks.
- В новом режиме PairRequest всегда вызывает BeginTransmission, в том числе при TEST_ONLY model power−200. Все четыре попытки требуют реальный valid raw callback ожидаемого RX; вторая требует valid MODEL_LOST без accepted information, остальные требуют recovery admission. Это отличается от старого TX gate, который вообще не передавал LOST case.
- Receipt использует конкретные выбранные refs TX/RX/radio/owner/world/index0/catalog, nonempty key, power/mute/tuning и сохранённый generation. Перечитывание после Evaluate проверяет те же actual observations и позиции; completed clock взят после Read/Same. TTL2500ms и post-query окно≤500ms проверяются до action. Другой receiver/duplicate/closed/stale callback не создаёт полезной квитанции.
- Вызван настоящий существующий RICON_PairService.Evaluate с WirelessLinkEvaluator и accepted CandidateInput/TestProfile helper; никакого второго registry/oracle в Enforce не добавлено. Guard проверяет одну exact lease/TX device/RX player/device/index row, call/pair/device codes, evaluator count, hasBudget/valid/reason, finite q∈[0,1] и явный допустимый WEAK/DEGRADED/CLEAN state.
- Полезное действие — только фиксированная **in-process diagnostic** строка GENERIC_PAIR_INFO_V1 для accepted row. Она не сериализуется как переданный payload, не является remote ACK и не объявляет player lifecycle authority. Native quality и RICON q отдельны; receipt не меняет native q, RX mute/power/frequency/key или VoN seam.
- m_Seen не позволяет повторный evaluator/action; Close на смене фаз, End и Stop отключает contexts и очищает references. Четыре retained messages и total20s bound ограничивают bench; raw поздние callbacks не замалчиваются. Это не native queue-drain guarantee.

Независимый повтор check_generic_offline.py прошёл37/37 с двумя writes перенаправленными только в review/: generic-offline-independent.json и generic-reviewed.patch. Source-preservation checks сравнили legacy methods/raw observables с immutable before, а существующий Python S3 oracle проверил valid LOST/recovery и негативные DTO cases. В oracle native callback явно assumed input; ни эти37 checks, ни scope assertions не названы исполнением Enforce или callback runtime. Первоначальные5 filesystem fixture checks, исходные27 shadow checks и37 generic checks имеют разные scopes и не складываются в native PASS.

Canonical публикацию generic делает только root: revalidate exact base, записать только accepted staged bytes, сравнить actual bytes/SHA и сохранить before/after receipt. Pre-apply checker содержит canonical_untouched предпосылку, поэтому после apply его нельзя повторно выдавать за актуальную post-apply проверку; postapply сравнение должно использовать immutable before и reviewed payload.

**Финальная post-apply проверка выполнена:** generic/apply-receipt.json указывает actual canonical source C0185FA5... и backup canonical-before.c1495DBCA.... Reviewer независимо сравнил оба с reviewed staged payload/immutable before.c побайтно, hashes совпали. generic-postapply-independent.json сохраняет этот факт; pre-apply checker после публикации не перезапускался. Прежние staged-only фразы README/offline-receipt — датированные подготовительные snapshots, actual apply подтверждён отдельной проверкой. Root source далее frozen; дальнейшее отражение progress в документах/Goal не превращает scope в runtime.

Не проверены SDK/Enforce compile и игра/native runtime; голосовой effect по-прежнему BLOCKED_NO_ACTUATOR, calibration open. ACCEPT разрешает root принять reviewed generic source в текущем implementation scope, не создаёт нового approval и не подтверждает A/B/C isolation, remote payload transport, DIRECT preservation или production voice actuator.
