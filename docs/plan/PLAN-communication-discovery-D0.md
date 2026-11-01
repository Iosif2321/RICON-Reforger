# Bounded plan — D0 communication-event discovery

Версия: 0.3, 2026-11-02. Уточнение v0.3 фиксирует control retry пределы, финальную проверку после логирования, post-observation и собственный host profile; это не изменение транспортной гипотезы. Статус: **APPROVED_FOR_D0_IMPLEMENTATION_ONLY** по явному решению координатора в пределах автономного поручения владельца 799 после технического VoN/QA ревью и внесения обязательных guards; runtime пока закрыт. Scope: только D0 из [probe spec v0.3](<../req/REQ-von-marker-probe.md>), не транспортные P0–P4. Входной код/цель: `170e9c0`; фактическая редакция spec записывается в том же наборе изменений. Автор/координатор: Lead.

Полномочие: прямое поручение владельца продолжать автономно до завершения задачи, событие 799 рабочего Thread; процесс FRESH_SESSION_WAIVED, не clean-session compliance. Это явное ограниченное решение о следующем безопасном срезе, **не автоматическое утверждение всего DRAFT или native VoN механизма**. Техническое ревью D0 VoN/QA выполнено в событиях 1018/1023. Реализация разрешается только в перечисленном TOOL_ONLY scope; runtime — после отдельной проверки исходников, компиляции и подтверждения предусловий ниже.

## 1. Что проверяется

Один public `CommunicationSoundComponent.SoundEventPriority` на собственной свежей живой роли A с `SCR_SoundEvent.SOUND_VOICE_PAIN_RELIEVE`, symbolic priority `SCR_ECommunicationSoundEventPriority.SOUND_PAIN_RELIEVE`, `ignoreQueue=false`. Это гипотеза прямого queue-входа, не отождествление с прочитанным delayed-вызовом или VoNComponent overload.

Сохраняются настоящие start/finish communication callbacks и VoN callbacks локальных A/B, binding/owner/handle/generation, heartbeat и read-only comm metadata. Запрос может воспроизвести штатный звук и меняет очередь A. Никакого захвата микрофона, записи PCM, nonce или декодера. Совпадение события B с A — только корреляция.

## 2. Точные write-scopes

- Enforce: новые `RICON_LiveCommunicationDiscovery.c`, `RICON_LiveCommunicationDiscoveryGame.c` в TOOL_ONLY Game и чистый `RICON_LiveCommunicationDiscoveryCheckPlugin.c` в WorkbenchGame. При необходимости отдельный TOOL_ONLY player-controller readiness helper; его RPC несёт только readiness/run/generation, не event/payload/время символов. Любой helper предварительно показывается в diff и проверяется.
- Lead: opt-in `-CommunicationDiscovery`, публичный run-id обеим ролям и только для D0 свежий host profile через существующий NewPeerProfile в [runner](<../../tools/RICON_LiveProbe/ops/wb-serverplay-claude.ps1>); клиентский профиль тоже свежий. Публичный CLI `-profile` задаёт отдельный scratch-root, не меняет обычный режим. Guards, библиотека, worlds allowlist и teardown не ослабляются; до live нужен readback эффективного profile root.
- Production Core, RF-модели, snapshot/seam, обычный observer, установленная игра и Windows-настройки не меняются. Без флага новый прибор не создаётся; исходное поведение сохраняется.

## 3. Фиксированные пределы первого опыта

| Параметр | Значение |
|---|---|
| Роли | ровно host/local A=1 и Diag/local B=2; новый собственный мир и отдельные свежие scratch-профили host/client |
| Запросы | ровно 1 на A; B никогда не вызывает sound/VoN emitter |
| Стабильность binding | 5 с до ARM; каждое событие проверяется по текущим refs/generation |
| Readiness B | до запроса подтверждены run/entity/component/subscription generation, включённые callbacks и подписки B; перед REQUEST требуется свежий control challenge/ACK без event/payload |
| Quiet baseline | 6 с после готовности A/B; никакой посторонней capture/voice/communication активности; baseline не доказывает пустую очередь сам по себе |
| Первый запрос | не раньше baseline и свежего ACK; не позже 39 с локального lifetime A, remaining A минус call-budget 1 с ≥30 с |
| Свежесть ACK | RTT challenge→ACK по одним монотонным часам A ≤1 с; непосредственно на REQUEST: reported remaining B − RTT A − текущий receive-age A − call-budget 1 с ≥30 с; ACK связан с текущими generations, receive-age A ≤1 с |
| Control retry | initial/fresh имеют разные challenge; ≤20 идентифицированных attempts на challenge с интервалом ≥1500 ms, без сброса t0/deadline. ACK только текущего attempt; retry не переносит event/outcome и не разрешает queue повтор |
| Ожидание start/finish | максимум 20 с после запроса; не доказанный drain/очистка очереди |
| Post-observation A | после совпавшего finish ещё 10 с неизменённого наблюдения в прежнем cap; B сохраняет своё bounded окно. Это не доказательство очищенной очереди |
| Резерв | минимум 10 с cleanup/drain budget после 20 с ожидания; это запас планирования, не гарантия native queue |
| Наблюдение | максимум 70 с от первоначального старта локального probe t0; deadline никогда не сбрасывается при readiness/rebind; heartbeat 1 с, допустимый разрыв ≤3 с |
| Поток | максимум 4096 строк на роль: до 4000 ordinary/data, 96 зарезервированы под GAP/terminal/cleanup; переполнение инвалидирует окно |
| Процессы | один Workbench и один Diag; внешний предел 300 с + watchdog 330 с |

Выбранные числа — инженерные ограничения первого discovery, не измеренные network/voice latency. Корректировать их можно только новой записанной редакцией, не под результат.

Часы локальны и не считаются синхронизированными. t0/deadline не перезапускаются. Источник времени — документированный System.GetTickCount() из установленного Enfusion API (миллисекунды с запуска игры); использовать локальные значения/разности и инвалидировать clock-reset/отрицательную разность, не сравнивать A с B. Game-world time или сумма timeslice не заменяют этот clock. До runtime проверить доступность/поведение в compile/preflight; недоказанный clock блокирует запрос. Control challenge имеет публичный ID и несёт только readiness/lifetime/generations; он не передаёт событие, исход A или расписание. Сохраняются RTT и receive-time A. На самом REQUEST remaining B консервативно уменьшается на весь RTT, текущий receive-age и 1 с call-budget; remaining A пересчитывается тогда же с тем же call-budget. Дополнительная секунда — запас, не обещание длительности native вызова; превышение бюджета после возврата инвалидирует окно и требует controlled teardown. Перед REQUEST повторно проверяются health/generations/остатки обеих ролей; stale/mismatch блокирует вызов, после вызова инвалидирует окно. При любом неизвестном bounded остатке — BLOCKED без запроса.

## 4. Предусловия и отказ до mutation

Проверить свежую собственную ALIVE entity A, ровно два известных участника, стабильные local IDs/component refs, фактические invokers и `IsScriptedMethodsCallEnabled()` на каждом наблюдаемом компоненте. Не включать флаг callbacks или менять unknown baseline автоматически: при false/UNKNOWN — BLOCKED_OBSERVER. Вызовы getters/подписки не заменяют доказанную работоспособность; native/remote origin не объявляется по одному имени invoker.

Нужна проверенная изолированная fixture/setup без чужих писателей/ожидающих событий. При неожиданном sound/capture/PTT до запроса — abort, без очистки очереди или SetCapture(false). Любая readiness-связь явно отделяется от data-plane; B не получает event outcome A через RPC. Если достаточная собственность/наблюдение не устанавливается, запрос не выполняется, фиксируется BLOCKED.

## 5. Выполнение и cleanup

1. Зарегистрировать callbacks до ARM, показать A/B readiness и generation, выдержать quiet baseline.
2. Перед единственным запросом повторно проверить current A refs/flags, server-resolved B controller/entity против initial и fresh адресата, foreign activity и поколения. После context lookup и REQUEST_BEGIN logging заново взять Now и весь RTT/age/remaining gate непосредственно на mutation boundary; invalid/reserve veto/изменение B запрещают очередь. Не логировать между последним guard и единственным native вызовом; one-use фиксируется до него, без принятия pre-call чужого callback как своего. Записать REQUEST_END после возврата; `void` — не ACK.
3. Неизменённо писать все фактические callbacks, включая посторонние, и read-only native radio RX отдельно. Не вызывать invokers/callbacks вручную. Не присваивать sound-start sender A/RX binding по времени или частоте.
4. При rebind/gap/clock anomaly/foreign writer запретить дальнейшие requests и инвалидировать результат. Не останавливать чужие или неопределённые audio handles.
5. Для собственного начавшегося события нужен настоящий finish/terminal либо заранее проверенное уничтожение собственных тестовых ролей при завершении мира/процессов. Detach наблюдателя сам по себе не останавливает звук. Истечение 20 с даёт INCONCLUSIVE_OUTSTANDING_EVENT и требует полного controlled teardown, не успешного D0.
6. Detach всех собственных подписок, вывести terminal с остатком 0 и незавершёнными событиями, затем обычный runner возвратит edit/настройки и завершит только удержанные свои процессы. Сохранить raw/хэши и отдельный операторский cleanup. Неполный terminal/cleanup не повышается до PASS.

Запрещены SetCapture/PTT, comm-method/TX-entry setters, mute/range/frequency/power/key/Social, ручные MP signals, PlaySound или emitter на B, восстановление чужих настроек и unknown-event retries. Есть только обычный подтверждённый queue-вызов на A; никакого скрытого fallback.

## 6. Verification и stopping gates

- До runtime: полный diff/readback, dependency/forbidden-call scan, pure opt-in/one-request/generation/limits tests; Workbench ScriptEditor compile без мира по log (не только exit 0), регрессия существующих checker.
- Перед запуском: зафиксировать commit и хэши, отдельные raw-пути, отсутствие чужого runtime, dry-run args; единственный оператор Lead. При неизвестном authorization prompt — stop.
- Результаты: D0_REQUEST_ONLY, D0_LOCAL_EVENT_OBSERVED, D0_B_EVENT_CORRELATED, BLOCKED/INCONCLUSIVE и отдельный CLEANUP. **Никогда SAME_PATH_PACKET_PASS, Native TX PASS, AUDIO_PASS, H2/direct или LS-02…06 PASS.**
- После опыта: независимое QA по источникам/границам/подпискам/cleanup. Только затем решается, есть ли пригодный вход для транспортного P0. Один успешный D0 не открывает production и не квалифицирует VoN serializer.

Что может сломаться: input-event не подходит конкретному sound project; callbacks выключены; queue не завершится; B наблюдает другой объект или не тот канал. Все эти исходы допустимы как отрицательное/неполное discovery evidence, не обходятся подстановкой данных. Самый рискованный шаг — один queue request; он не выполняется до всех guards. Отклонены захват микрофона без владения, неизвестный VoN values-массив, guessed event, общий USB recording и binary/PowerShell-policy обход.
