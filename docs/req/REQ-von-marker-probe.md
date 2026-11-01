# REQ-von-marker-probe — опровергающий опыт AI-event VoN

- Тип: спецификация исследовательского TOOL_ONLY probe, **DRAFT**, v0.4, 2026-11-02. v0.2 уточнила terminal binding/provenance, происхождение callback, seal и N3; v0.3 добавила D0 communication-event discovery. v0.4 добавляет только отдельный V0 native VoN input-shape preflight по поручению Lead message:355522fa-1163-491c-903b-0b8a7f9c00be; V0 — DESIGN_REVIEW / NOT_RUN, не разрешение реализации/запуска и не packet/serializer квалификация. Вход V0: `ec2d363465394950840be5921ff27fbd42c11770`; эта новая редакция пока не записана в Git.
- Автор: Реверсинженер / Ghidra. Техническое ревью: VoN, Enforce, ядро, QA; координатор и единственный будущий оператор — Lead. Утверждающая роль — владелец проекта.
- Источник истины редакций: Git RICON-Reforger. Исходный вход Design: `170e9c02e2160e0d4f7140f37c4ff57f43c54663`; вход новой V0-редакции указан выше. Ранний D0 записан и проверен отдельно; новая V0-редакция ещё не закоммичена и не утверждена.
- Цель: [INTENT v0.2](<INTENT-local-radio-slice.md>), включая поправку владельца: допустима информация вместо звука только по доказанному тому же штатному пути и протоколу. Повторное утверждение цели не требуется.
- Процесс: **FRESH_SESSION_WAIVED**, не строгая clean-session compliance. После сообщения о FRESH_SESSION_REQUIRED владелец прямо ответил «Действующий самостоятельно до завершения данной задачи» (рабочий Thread, событие 799); применительно к заявленному блокеру это записано как исключение для продолжения Design, не как доказательство свежести. Уведомление новой роли в 04:02:31 само по себе переход между фазами не доказывает.
- Team — решения/разрешения, Git — редакции. Решения: thread:9511c510-9ddd-4a5d-83e0-79760fd2e088, сообщения message:c683bd28-a3fb-4b15-8854-840d534fd6fc (scope), message:b1d30a13-f863-4192-967e-463f728ae067 (блокер), message:3c3be7c4-5f55-4449-9e88-1cc434fb15cb (продолжение с исключением).
- Применены: `ai-native-sdlc-cycle` и artifact-contracts, `code-quality-loop`, `docs-manage`; версии навыков не объявлены. Docs connector недоступен; артефакт — локальный Markdown. `graphify` прочитан, граф отсутствует; установка/генерация исключены scope. Корневой и вложенные AGENTS отсутствуют; прочитан родительский [AGENTS](<../../../AGENTS.md>), преимущественно об Arma 3/SQF: его PBO-процесс не переносится на Reforger.
- Write-scope автора новой V0 Design-редакции — только данный файл. Прежний отдельный D0 gate и [bounded plan](<../plan/PLAN-communication-discovery-D0.md>) не открывают V0: его implementation/compile/runtime остаются закрыты до собственных recorded gates и технического ревью. D0 live-результат приведён в §5/V0; production, P0–P4 и запрещённые setters не открыты.

## 1. Контекст и граница решения

**Сейчас безопасный валидированный механизм не установлен: BLOCKED_NO_QUALIFIED_RX_AND_EQUIVALENCE.** Это не доказательство невозможности VoN AI-events. Документ задаёт проверяемую гипотезу и ранние опровержения, не выдаёт разрешение на её исполнение.

Прочитаны целиком входы на указанной редакции: [оценка кандидатов](<../evidence/E-von-marker-feasibility-lead.md>), [манифест источников](<../evidence/receipts/von-marker-feasibility-20261102-lead.json>), [REQ-von-seam v0.2](<REQ-von-seam.md>), [REQ-core-contracts v0.9](<REQ-core-contracts.md>), [аудиопротокол](<../plan/LOCAL_AUDIO_VALIDATION_RU.md>) и [границы контроля](<../analysis/AUDIO_CHAIN_CONTROL_RU.md>). Рабочие входы сверены с `170e9c0`; исходный Git был чистым. Состояние цели прочитано в [state](<../../.opencode/goal/state.json>); оно предшествует исключению свежести, поэтому не является доказательством снятия этого gate.

Гипотеза H-AI: вызов AI-event именно через VoN A даёт на B отличимый нативный результат, который можно независимо декодировать, привязать к единственному A→B radio-binding и квалифицировать относительно player-voice пути и radio-only gate. Каждое звено проверяется отдельно. Внешние DLL, новая бинарная RE/Ghidra, установка, код/сборка, Play, SetCapture, UI и Windows-настройки в текущую фазу не входят. Production seam MUST оставаться no-op с `BLOCKED_NO_ACTUATOR`.

## 2. Факты, не обещания API

| Вход/наблюдение | Что подтверждено прочитанным материалом | Чего оно не доказывает |
|---|---|---|
| `VoNComponent.SoundEventPriority(string eventname, array<float> values, int priority, bool ignoreQueue=false)` | Установленная API-страница описывает «Transfer AI sound message through VoN» | Произвольные байты/PCM/Opus, использование player-owned компонента, выбор TX, remote RX, serializer и mute-stage |
| `CommunicationSoundComponent.SoundEventPriority(string eventName, int priority, bool ignoreQueue=false)` | Отдельная сигнатура очереди событий, без массива values | Связь с VoN-вариантом; их нельзя заменять или отождествлять |
| `SCR_CommunicationSoundComponent.GetOnSoundEventStarted()` | Существующий invoker; vanilla override передаёт component, eventName, AudioHandle, priority | Удалённого отправителя, receiver, values и принадлежности к VoN-протоколу callback не содержит |
| `VoNComponent.OnReceive(playerId, isSenderEditor, receiver, frequency, quality)` | Событие получения audio data for playback; SCR invoker имеет отличающуюся сигнатуру, как в observer | Не payload, не seq/frame ID, не слышимость, не гарантированная каденция или положение mute |
| Существующий [LiveObserver](<../../tools/RICON_LiveProbe/Scripts/Game/RICON_Live/RICON_LiveObserver.c#L150-L191>) | Подписки и radio-события; null-transceiver отбрасывается | Handle при записи теряется (`binding=UNKNOWN`); direct и непрерывный binding этим прибором не проверены |
| Campaign `OnDelivery → PlayRadioMsg → RpcDo_PlayRadioMsg` | Для прочитанного campaign-примера есть отдельный RPC | Это не media-путь; вывод не распространяется на все потенциальные scripted messages |

В дополнение к оценке перечитаны установленные разделы VoN API (OnReceive, SoundEventPriority), таблицы Communication API и существующие логи манифеста `von-ai-start`, `von-ai-callback`, `von-ai-reference`. Окно vanilla callback — строки 269–272 исходника, не новая выгрузка. Ноль VoN-совпадений в SCR-файле не устанавливает поведение native-родителя. Имена допустимых событий, значения массива и priority для player-role пока **UNKNOWN**; значения enum communication-компонента не заимствуются как VoN-контракт.

## 3. Требования и владельцы границ

MUST/MUST NOT — требования предлагаемого опыта. MP-01…08 транспортных P0–P4 остаются DRAFT и не утверждены для реализации. MP-D0 имел собственный ограниченный gate; новый MP-V0 (§5/V0) остаётся только DESIGN_REVIEW. Эти предварительные срезы не являются исключением из критериев P0–P4 и не квалифицируют media.

| ID | Требование | Источник intent | Проверка / владелец |
|---|---|---|---|
| MP-D0 | Один штатный CommunicationSound-запрос на собственной живой A, без capture/радиомутаций; настоящие observers и явные пределы результата | §3/5, LS-08; поручение Lead события 988 | D0, VoN + Enforce + QA |
| MP-01 | Использовать только квалифицированный native VoN вход и независимый B-side выход; не вызывать RX/playback callbacks вручную | §3, §5.4–5 | P0/P1, VoN + Enforce |
| MP-02 | До теста фиксировать декодер, алфавит, таймауты, предел попыток, исходники и параметры; nonce неизвестен B | LS-02/08 | P0/P2, QA |
| MP-03 | Один однозначный TX/RX и непрерывная identity; rebind, пропуск или неоднозначность инвалидируют всю попытку | LS-01, §5.6 | P1–P4, ядро + Enforce |
| MP-04 | Доказательство одного пути/протокола отделять от корреляции и переносить только на проверенный участок | LS-03 | P1/P3, VoN + QA |
| MP-05 | Реальные отрицательные контроли, все попытки и отказы сохранять; отсутствие наблюдателя не есть отсутствие доставки | LS-04/08 | P2, QA |
| MP-06 | DESIRED, APPLIED, OBSERVED и MODEL_LOST/ошибка/abort различать; ручной mute не выдавать за действие RICON | LS-05, §5.2–3 | P4, ядро + VoN |
| MP-07 | Не менять direct и чужое mute; восстановление только своих состояний с доказанным владением | LS-06/07, §5.1/7 | P3/P4, VoN + Lead |
| MP-08 | Исследовательский probe не расширяет seam/core; остановка у первого незакрытого gate, без обходного транспорта | §5.2/9, §8–9 | Все фазы, Lead |

## 4. Архитектура наблюдения и формат evidence

Только проект: A-side emitter → **неизвестный native участок** → B-side observer/decoder. Сервер — источник session/player/device/lease/binding; не источник ожидаемых битов для B. Отдельный verifier сравнивает A и B только после запечатывания B-результата. TOOL_ONLY recorder не вставляется в `VoNIntegration` и не меняет RF-модель/seed/quality.

- A MUST записывать до/после вызова: локальное монотонное время и sequence, полную binding identity, точный компонент/метод, eventName, values, priority, ignoreQueue, отсутствие/наличие ошибки. Возврат `void` означает лишь запрос, не отправку; локальный sound-start — лишь дополнительное наблюдение. OnCapture не объявляется обязательным признаком AI-event и не имитируется при его отсутствии.
- B MUST записывать фактический process/role, локальные часы и sequence, component-owner identity, оригинальные аргументы квалифицированного callback/invoker (eventName, handle, priority), lifespan подписки и наблюдателя. Установлен SCR invoker; его native/remote происхождение ещё должно быть доказано P0/P1, не подразумевается названием записи. Binding/subscription generation присваиваются каждой сырой записи до декодирования. Handles — только локальные identity, не общий числовой ключ между процессами.
- Radio RX B записывается отдельно: sender A, local listener B, **живой конкретный receiver**, сырой frequency/quality; receiver сравнивается object-reference с предварительно разрешённым RX. Совпадение частоты/времени с sound-start — только корреляция, не связывание содержимого с RX.
- Для LS-01 нужен server-resolved `(epoch, linkId, talker/listener player+device IDs, TX/RX indices, leaseId)` и проверенная проекция на локальные handles/поколения сущностей. AI-event lease/PTT association неизвестна; не изобретать второй фиктивный lease. Пока её нет, сохранять UNKNOWN и не заявлять полный LS-01/05.
- Предлагаемая запись evidence: schemaVersion, runId, attemptId, phase, sourceRole, sourceClock, localSeq, bindingGeneration, subscriptionGeneration, eventKind, rawArguments, interpretation, reason. Записи `GAP`, rebind, overflow, clock-reset, detach/stop обязательны. UNKNOWN — отдельное значение, не default=0. Ключи шифрования и стороннее аудио не записываются.
- B raw callbacks/decoder result отделены от A TX-log. Verifier хранит SHA-256 каждого сырого файла, commit, build 1.8.0.13 (или явный новый baseline), decoder/config хэши, все попытки в исходном порядке, cleanup receipt. Hash подтверждает неизменность, не происхождение тракта.

### Независимость на одном ПК

Никаких A-log, nonce/seed, расписания битов, переменных emitter, общего файла/памяти, RPC с содержимым, per-symbol управляющих сигналов или ожидаемого результата в B. Общие ресурсы могут содержать **только полный публичный алфавит и алгоритм**, не выбранную последовательность. Управляющий канал ограничен стартом/концом серии, публичным attemptId и режимом контроля; он не должен отражать число/время выбранных символов или исход вызова A.

До серии QA проверяет dataflow emitter/observer/decoder и отсутствие B-чтений A. Одного отдельного каталога мало: host и клиент технически могут читать одни файлы. Если нельзя исключить такое чтение и общий seed из фактического кода/конфигурации, результат BLOCKED_INDEPENDENCE. После завершения окна B фиксирует decoded bytes **либо NO_FRAME/INVALID/INCONCLUSIVE**, сырые данные и terminal record наблюдения окна. Независимый verifier затем проверяет полноту, закрытие и SHA-256 B-артефактов и фиксирует seal до доступа к A nonce; только после этого получает A nonce и сравнивает. Неполный terminal/неподтверждённое закрытие не допускает сравнения с вердиктом PASS. SHA-256 commitment A до передачи допускается хранить у verifier, не передавать B. Все попытки фиксируются, не только совпавшие.

## 5. Последовательность минимального опровержения

Переходы: `DESIGN_DRAFT → REVIEW → RECORDED_SPEC → BOUNDED_PLAN → IMPLEMENTATION/COMPILE → AUTHORIZED_RUNTIME`. Для D0 выполнено техническое ревью и отдельно записан ограниченный implementation plan; его gate не является runtime-разрешением. P0–P4 ниже остаются требованиями будущего исследования, не разрешением на реализацию/запуск.

### D0 — vanilla communication-event discovery, до транспортного P0

**Цель D0:** проверить один стандартный communication-запрос и наблюдаемость события на свежей собственной живой роли A; получить исходные данные для P0, а не объявить AI-event через VoN квалифицированным. Это mutation звуковой очереди с возможным воспроизведением, **не чистое чтение**. В этой редакции опыт только спроектирован, NOT_RUN; реализация/compile/runtime требуют отдельно записанных gates §10.

**Проверенный источник:** [новый WbReader-лог:145–163](<C:/Users/Ded/Documents/My Games/ArmaReforgerWorkbench/logs/ricon-wbreader-von-event-damage-diag-20261101-235430/console.log#L145-L163>) содержит vanilla `SCR_CharacterDamageManagerComponent:581`: вызов `DelayedSoundEventPriority` с `SCR_SoundEvent.SOUND_VOICE_PAIN_RELIEVE`, `SCR_ECommunicationSoundEventPriority.SOUND_PAIN_RELIEVE` и DEFAULT_EVENT_PRIORITY_DELAY. В прочитанном окне проверяется ALIVE; компонент communication проверяется на наличие. [Установленная константа](<C:/Program Files (x86)/Steam/steamapps/common/Arma Reforger Tools/Workbench/docs/ArmaReforgerScriptAPIPublic/html/interfaceSCR__SoundEvent.html#L6796-L6815>) имеет строковое значение `SOUND_VOICE_PAIN_RELIEVE`. [Public CommunicationSound API](<C:/Program Files (x86)/Steam/steamapps/common/Arma Reforger Tools/Workbench/docs/ArmaReforgerScriptAPIPublic/html/interfaceCommunicationSoundComponent.html#L216-L239>) отдельно разрешает `SoundEventPriority(string eventName, int priority, bool ignoreQueue=false)`, возвращает void и ставит событие в очередь по priority. Это **разные direct/delayed входы**; допустимость выбранного event/priority на player-owned direct component остаётся проверяемой гипотезой, не установленной тождественностью с vanilla delayed-цепью или VoN overload.

Provenance входов D0: SHA-256 source-лога `45e8aee51238255fbf7875c4d2242bfc28d587a5548215067afbaba51205f0b8`; страницы SCR_SoundEvent `5e33cfde816c1dd2db2a1e679c8883653469fa6efb85adb2eabf1a0d3f7943c5`; страницы CommunicationSoundComponent `4bf72fcffac5fbdcf1e894acc7fdee6afe12cf0337dbb5553711352aa02bb3bd`. Здесь перечитаны окно source и API, но новый WbReader не запускался; exit 0 source-прогона сообщён Lead, `done`/15 строк видны в логе. Предшествующий bare exit 1 без журнала не считается отрицательным выводом об API.

**Preconditions до запроса:** только две новые собственные роли/профили A/B, текущие identities/entity generations, A ALIVE, однозначный собственный `SCR_CommunicationSoundComponent` A и доступные invokers наблюдения. B-инструментирование должно быть зарегистрировано и привязано до опыта; если B-подписку нельзя квалифицировать, D0 BLOCKED до запроса, а не «нет доставки». В изолированном стенде нет посторонних слушателей, ожидающих/активных событий и foreign writers. Одного отсутствия callbacks для доказательства пустой очереди недостаточно: нужна проверенная fresh-role/setup и отсутствие конкурирующих систем/запросов. Численные окна baseline/post-observation/drain, общий короткий timeout, память и лимит процессов MUST быть заранее зафиксированы в bounded plan; сейчас UNKNOWN, запуск блокируют. Безопасное окончание/очистка собственных тестовых ролей должны быть известны до mutation.

**Единственное действие:** один вызов **на A** public `CommunicationSoundComponent.SoundEventPriority` с указанными event и symbolic priority, `ignoreQueue=false`; не вызывать damage-manager, не наносить урон/лечение и не использовать delayed-метод вместо объявленного direct входа. Никаких retries, неизвестных event, VoNComponent.SoundEventPriority, values/nonce, MP/сигналов-эмуляторов, PlaySound на B или передачи A-события через диагностический RPC. Возврат void — только факт запроса, не ACK отправки/воспроизведения. Отсутствие собственного компонента или соответствующего наблюдателя — BLOCKED до действия.

**Наблюдения A/B:** baseline без запроса, запись REQUEST_BEGIN/END A, затем все настоящие sound-start/finish invokers и VoN callbacks в неизменённых observers до конца post/drain окон; только локальные монотонные часы каждой роли, sequence, run/role/player/entity, component-owner, object handle и binding/subscription generation до интерпретации. Доступные `GetCommMethod`/`GetTransmitRadio`, существующие radio handles и owner metadata читаются без setters; частота/время не заменяют identity. VoN RX записывается отдельно с фактическими sender и receiver, включая null-receiver как отдельную категорию наблюдения, без приписывания ему event payload. Metadata UNKNOWN остаётся UNKNOWN. Нет PCM-записи, SetCapture, PTT, смены comm method/TX entry/transceiver, mute/range/frequency/power, Social или signal mutation. D0 не требует и не запускает микрофон; неожиданная capture/direct/чужая активность приводит к остановке, не к принудительному SetCapture(false).

**Остановка/cleanup:** запросов больше нет; observer остаётся активным до terminal либо безопасной остановки. Rebind/смена entity/component, GAP, clock anomaly, чужое событие/писатель, timeout или неизвестное состояние очистки инвалидируют окно и запрещают повтор. Начавшееся sound event нельзя считать отменённым от прекращения наблюдения. Только штатное завершение собственного события с terminal record либо заранее проверенное завершение собственных изолированных тестовых ролей/процессов; не очищать общую очередь, не останавливать чужие audio handles и не восстанавливать состояние на новом binding. Снять свои подписки, сохранить все сырые события/отказы/terminal и SHA-256; неполный cleanup — отдельный BLOCKED, не успешный D0.

| D0 observation | Допустимый вывод | Недопустимое повышение |
|---|---|---|
| Запрос A и sound-start/finish на её проверенном компоненте | D0_LOCAL_EVENT_OBSERVED в указанном окне | Native TX, remote доставка, слышимость |
| Реальный callback B, корректно привязанный к компоненту/owner | D0_B_EVENT_CORRELATED; время/имя лишь коррелируют с A | Доказанный sender A, VoN/radio protocol, payload |
| Нулевой либо непривязанный B-output при действовавшем наблюдении | INCONCLUSIVE; сохранить отсутствие/UNKNOWN | Отрицание VoN API или PASS тишины |
| Отдельный native radio RX с реальными sender/RX | Наличие RX-события отдельно от communication event | Доставка данного event/параметров |
| Нет компонентов/прав/границ/безопасной очистки до запроса | BLOCKED, запрос не выполняется | Автоматический fallback или retry |

Remote origin допускается назвать доказанным только при отдельной независимой квалификации источника; D0 сам не даёт её по имени/времени. Ни один D0 исход не закрывает LS-02…06, SAME_PATH_PACKET_PASS, H2/direct, AUDIO_PASS или участие RICON; он лишь добавляет диагностическое evidence LS-08 и кандидата следующего транспортного P0. P0/P1 и legal gate нового бинарного исследования остаются незакрытыми.

### V0 — native VoN input-shape preflight, до транспортного P0

**Статус: LIMITED_IMPLEMENTATION_APPROVED / RUNTIME_NOT_OPEN.** После технического VoN/QA review координатор под автономным поручением799 явно открыл только ограниченную реализацию по [V0 bounded plan](<../plan/PLAN-von-input-shape-V0.md>); его числа строже исходных верхних пределов секции. Это не квалификация values/serializer и не автоматическое утверждение P0–P4. Отдельный предварительный исследовательский срез по поручению Lead message:355522fa-1163-491c-903b-0b8a7f9c00be под автономным поручением владельца 799, не транспортный P0 и не квалификация values/serializer. Write-scope этой редакции — только данный spec; changelog ведёт координатор. Открыта только реализация в scope bounded plan после записи редакции; compile после readback/координации, runtime отдельно закрыт. D0 уже дал [D0_LOCAL_EVENT_OBSERVED_A](<../evidence/E-D0-live-001.md>); это квалификация локального CommunicationSound event/config, **не** VoN-входа. Старые NOT_RUN/gate-формулировки раздела D0 описывают его проектную редакцию, не отменяют этот live-факт.

**MP-V0 (intent §3/§5, LS-08; VoN + Enforce + QA):** проверить одну type-compatible форму native входа на новой собственной живой A, честно сохранить native эффекты и ограниченность наблюдения. [Установленный public API:505–533](<C:/Program Files (x86)/Steam/steamapps/common/Arma Reforger Tools/Workbench/docs/ArmaReforgerScriptAPIPublic/html/interfaceVoNComponent.html#L505-L533>) задаёт `proto external void VoNComponent.SoundEventPriority(string eventname, array<float> values, int priority, bool ignoreQueue=false)` и «Transfer AI sound message through VoN». Явного запрета пустого массива в этом описании нет; cardinality/семантика values, допустимость player-role, безопасность и native обработка пустого массива **UNKNOWN**. Type-compatible не означает семантически допустимый вход или заведомо валидное сообщение; D0 это неизвестное не закрывает.

**Единственный кандидат входа:** на A получить фактический собственный live VoN component с проверенными owner/entity/player refs и generations, отдельно от CommunicationSound component. E0 — строка `SOUND_VOICE_PAIN_RELIEVE` из D0; priority — **80**, `ignoreQueue=false`. Эти значения выбраны для preflight, не объявляются документированным VoN-контрактом. Явно создать новый **non-null пустой объект `array<float>`**, удерживать его до возврата вызова, проверить и записать non-null и `Count()==0` до запроса. Не передавать null, не вставлять фиктивные нули/другие элементы, не подбирать values. При null/ненулевом Count — BLOCKED до mutation. Ровно один вызов именно четырёхаргументного VoN overload; CommunicationSound/delayed/PlaySound fallback отсутствует. Нет повторного D0-запроса внутри V0, E1, nonce, декодера, калибровочной серии или retry native вызова даже при ошибке/отсутствии callback.

**Guards и границы:** V0 требует отдельного opt-in/phase/verdict; без этого режима native вызов отсутствует. D0 и его строгая обработка unexpected direct не ослабляются. Наследуются без ослабления [bounded D0 plan v0.3 §§3–5](<../plan/PLAN-communication-discovery-D0.md#L19-L57>), кроме явно заменённого ниже sound-finish условия и трактовки обычного direct-эффекта **только внутри V0**. Ровно две новые собственные роли A/B и раздельные свежие scratch profile/log roots с native readback; старые D0 роли не переиспользовать. A ALIVE, стабильные refs/generations 5 с; наблюдатели A/B зарегистрированы до ARM, callbacks реально включены без setter, quiet baseline 6 с и проверенная fixture без foreign writers/активных/ожидающих событий. Тишина не доказывает пустую очередь. Отсутствие собственного VoN компонента/работающих локальных observers, неизвестное владение или безопасный teardown блокируют запрос. Не требовать от V0 заранее доказанного remote sink: отсутствие его квалификации сохраняется как UNKNOWN, не маскируется локальным положительным контролем или выбором случайного proxy на B.

| Ограничение V0 | Фиксированное значение до будущего bounded plan |
|---|---|
| Native requests | ровно 1 на A, 0 на B; one-use фиксируется до вызова |
| Readiness | initial/fresh challenge с разными ID; ≤20 control attempts/challenge, интервал ≥1500 мс; без сброса t0/deadline; ACK только текущего attempt/generations |
| Граница вызова | не позже 39 с lifetime A; remaining A − call-budget 1 с ≥30 с; reported remaining B − RTT A − receive-age A − call-budget 1 с ≥30 с; RTT и receive-age ≤1 с |
| Наблюдение A | **фиксированные 20 с после REQUEST_END/возврата**, без раннего успеха по callback/finish; не ждать обязательного локального sound-finish как признака remote-only input |
| Наблюдение B | неизменённое окно до собственного t0+70 с; не завершать по исходу/сигналу A |
| Резерв/общий cap | ≥10 с cleanup budget; максимум 70 с от первоначального локального t0 каждой роли, без перезапуска при readiness/rebind |
| Health/поток | heartbeat 1 с, разрыв ≤3 с; ≤4096 строк/роль, из них ≤4000 ordinary и 96 GAP/terminal/cleanup |
| Процессы | один собственный Workbench и один Diag, внешний timeout 300 с / watchdog 330 с |

Часы только локальные monotonic System.GetTickCount; A/B timestamps не сравниваются напрямую. После REQUEST_BEGIN logging и context lookup повторить refs/health/foreign/remaining и non-null/Count==0 проверки непосредственно на mutation boundary; между последним guard и native вызовом не логировать. One-use фиксируется до вызова. Консервативный remaining каждой роли обязан покрывать все 20 с после фактического возврата плюс 10 с cleanup; недостающий остаток блокирует вызов, превышение call-budget после возврата инвалидирует попытку. Control RPC несёт только публичные readiness/run/generations/lifetime, не event, values, outcome или символы. REQUEST_END и elapsed фиксируются после возврата; превышение call-budget или непроверяемый остаток инвалидируют окно, без повтора. Невозврат/исключение/crash — сохраняемый native исход и controlled teardown по внешнему cap, не скрытая удачная попытка. 20 с — предел наблюдения, не доказанный drain или верхняя граница native эффекта.

**Наблюдение, direct и запреты:** до/после запроса записываются точный метод, E0, пустые values, priority/ignoreQueue, actual component/owner/generations, доступные `GetCommMethod`/`GetTransmitRadio` и существующие TX refs **только readback**, без установки режима/TX. Все настоящие sound-start/finish, VoN receive/capture callbacks обеих ролей, включая null receiver и несовпадающие события, сохраняются с сырыми аргументами/refs и health/generations; callbacks/invokers не подделываются. Метаданные, которых callback не даёт, остаются UNKNOWN: component-owner не равен sender, имя/время/частота не доказывают источник или маршрут. Если инструментация не сохраняет null receiver, V0 блокируется до исправленного reviewed TOOL_ONLY observer, а не теряет direct-запись молча. Новый A-proxy sink не угадывается; его добавление требует отдельно проверенного mapping/наблюдения.

Вызов может произвести звук, сетевую активность, поставить событие в очередь или вызвать неизвестное native поведение; это **не чистое чтение**. Direct-настройки, реализация и глушение не меняются, но обычная vanilla emission по direct возможна и отдельно помечается; radio-only не обещается. Для V0 такой direct-эффект сам по себе не является foreign/capture anomaly и не скрывается правилом D0 об unexpected direct. Если будет требоваться запрет самой direct-передачи, а не изменения её настроек, V0 BLOCKED до доказательства radio-only. SetCapture/PTT, comm-method/TX-entry/transceiver setters, mute/range/frequency/power/key/Social, PCM/системный микс, signal mutation, MP-сигналы и emitter на B запрещены. Никакого нового бинарного RE или обхода legal/Windows-policy gate.

**Исходы и остановка:** `void`/REQUEST_END — лишь возврат управления (`V0_CALL_RETURNED`), не ACK/принятие/native TX. Сохранять одновременно наблюдения и отдельные validity/cleanup статусы:

| Реальное наблюдение V0 | Допустимая запись / предел |
|---|---|
| Нет callback при завершённом наблюдении | **INCONCLUSIVE_INPUT_OR_SINK**: неизвестны допустимость input и нужный sink; не API FAIL и не доказательство отсутствия передачи |
| Только локальный A sound-start/finish | RAW_LOCAL_EVENT; не доказательство remote input/delivery; finish не обязателен для самой формы VoN вызова |
| B callback без явного sender/route | RAW_CALLBACK_ORIGIN_UNKNOWN, без назначения A по времени/event/owner |
| B direct/null-receiver callback с явным sender A | **RAW_DIRECT_ROUTE_UNQUALIFIED**; сохранить sender/receiver/route как фактически даны, не radio PASS; null сам по себе не квалифицирует direct transport |
| B callback с явным sender A и конкретным radio RX | RAW_RADIO_RX; не доставка E0/values/пакета, не эквивалентность player voice |
| Foreign sender/writer, rebind, capture anomaly, GAP/clock/overflow, превышенный cap | INVALID/INCONCLUSIVE с точной причиной, запрет новых requests, controlled teardown своих ролей |
| Unknown cleanup/незавершённый native эффект | INCONCLUSIVE_CLEANUP; не PASS, сохранить blocker/сырые данные и выполнить доступный безопасный teardown своих ролей |

После фиксированного окна observer detach **не** считается остановкой native звука/очереди; отсутствие A finish не доказывает ни утечку, ни отсутствие remote эффекта. Заранее проверенное завершение своих изолированных ролей/мира/удержанных процессов обязательно для окончания V0 и неизвестных outstanding effects, без очистки общей очереди, чужих handles или setters восстановления на новом binding. При native anomaly прекращается опыт, не наблюдение ради красивого результата; сохраняются все доступные raw/ошибки/частичные terminal, хэши и отдельный операторский cleanup. Проверить zero subscriptions и фактическое завершение своих процессов; неизвестный результат cleanup остаётся блокером, не стирается watchdog exit.

Ни один исход V0 не открывает P0–P4, LS-02…06, SAME_PATH_PACKET_PASS/AUDIO_PASS, nonce/serializer квалификацию или RICON actuation. Техническое VoN/QA review границы пройдено; Lead явно открыл только limited implementation по отдельному bounded plan под поручением799. До записи редакций код не начинается; после неё разрешены только указанные файлы/проверки. Runtime остаётся закрытым до code/compile/preconditions review. FRESH_SESSION_WAIVED сохраняется, строгая свежесть не заявляется.

### P0 — квалифицировать вход и наблюдаемый алфавит (первый транспортный gate)

VoN/Enforce должны предъявить источник для точного player-role VoN-компонента A, выбора native radio TX, допустимых eventName/values/priority и B-компонента, на котором ожидается выход. Отдельно установить, не является ли путь AI-only и где появляется событие на B. Достаточны существующие законно прочитанные материалы; новую бинарную RE не выполнять. Если источников нет — **BLOCKED_EVENT_CONTRACT**, а не подбор неизвестных eventName во время live.

Для первичного опровержения достаточно двух различимых легальных событий E0/E1, каждого по одному через VoN-вариант A, и контрольного окна без вызова. Имена E0/E1 — обозначения неизвестных ещё событий, не выдуманные API-ассеты. B наблюдает все штатные callbacks без вызова локального sound-event. Если native B-output не появляется при работоспособном квалифицированном наблюдении, гипотеза непригодна в этой конфигурации. Если наблюдение или допустимость входа неизвестны — INCONCLUSIVE/BLOCKED, не опровержение всего API.

До увеличения нагрузки фиксируются реальные eventName, значения, queue policy и предел длительности/памяти; exact значения сейчас UNKNOWN. Нельзя отправлять nonce в values, пока не доказано, что B может читать его именно через квалифицированный тракт. Если остаётся только счётчик/время RX — переход к timing-кандидату требует явно иной классификации (§9), не тихой смены механизма.

### P1 — квалифицировать remote/radio origin и протокол

На B надо независимо отделить локальное воспроизведение, репликацию состояния сущности, direct/AI-broadcast и настоящий radio VoN. Нужны одновременно:

1. Положительный B-output от A при одном непрерывном разрешённом TX/RX binding; никаких local PlaySound/CommunicationSound-вызовов на B для восстановления сигнала.
2. Источниковая цепочка, объясняющая путь вызова VoN A к квалифицированному выходу B и способ связать содержимое с receiver/sender. Native участок допускается UNKNOWN, но тогда **эквивалентность остаётся UNKNOWN**. Совпавшие callbacks или общий RadioManager не заполняют этот пробел. Минимальные предусловия P2 — допустимый вход, проверяемый remote observable, sender/RX attribution и независимость; полная эквивалентность не предполагается до опыта.
3. Контроли P2 затем проверяют зависимость от реального radio-пути, а не только game/Rpl-соединения. Это необходимое, но само по себе недостаточное доказательство одного протокола; итог P1 пересматривается по результату P2/P3, без кругового требования заранее получить PASS.
4. Для сравнения с player-voice явно названы общий транспортный участок, native message/media тип, receiver routing и положение radio gate по источникам/независимому наблюдению. Надпись «through VoN» не обещает одинаковый serializer/voice media protocol. Нельзя объявить SAME_PATH_PACKET_PASS, если AI-event несёт лишь управляющую команду локальному воспроизведению отдельным форматом, не доказанным эквивалентным утверждённому пути.

Неразрешимая sender/RX attribution → BLOCKED_RX_ATTRIBUTION и остановка до P2. Неизвестный serializer/участок протокола блокирует SAME_PATH_PACKET_PASS как BLOCKED_PROTOCOL_EQUIVALENCE, но не сам ограниченный исследовательский P2 при выполненных перечисленных предусловиях и отдельном runtime-разрешении. Это допустимые конкретные итоги probe, а не повод перейти на campaign RPC.

### P2 — скрытый nonce, после P0 и предварительной attribution P1

Предлагаемый **публичный** формат для двухсимвольного алфавита: 16-bit preamble `0xD39A`, version `0x01` (8 bit), attemptId (8 bit), 64-bit свежий nonce как явные 8 octets (не недоказанный 64-bit Enforce integer), CRC-16/CCITT-FALSE (poly `0x1021`, init `0xFFFF`, refin/refout=false, xorout=0) по version+attemptId+nonce; всего 112 символов, MSB-first. E0=0, E1=1. Это формат нашего probe, не заявление о нативных пакетах. Непредсказуемый nonce выбирается только на A после готовности B; способ генерации и гарантия отсутствия общего seed фиксируются в будущем плане.

B восстанавливает только реально увиденные eventName в их нативном порядке. После локального ARM и доказанного drain первым символом начинается единственный кандидат; до конца фиксированного whole-frame окна требуется ровно 112 callbacks с правильными preamble/version/attemptId/CRC, без скользящего поиска подпоследовательностей. Любой чужой/лишний callback, неизвестная привязка или неверный кадр → INVALID/INCONCLUSIVE; нельзя отбрасывать события или подставлять биты из TX. Повтор E0,E0 или E1,E1 — легальные биты, не автоматически duplicate. До nonce нужна квалификация «один запрос → один декодируемый callback»; если native queue сливает повторы или callback multiplicity не установлена, этот алфавит блокируется. Отдельно распознаваемые повторы одного callback также инвалидируют попытку; без native seq нельзя обещать обнаружение всех потерь/дублей. CRC проверяет целостность, но не источник и не протокол; итоговое полное сравнение nonce выполняется после B-seal. Параметры CRC применяются к octets version, attemptId, затем nonce big-endian. Синхронизация — публичный preamble и локальные часы B, не серверное время символов. B не читает values/priority как payload без отдельной квалификации.

Декодер и таймауты замораживаются до nonce-серии. Публичная калибровка отделена: максимум две серии по 16 известных событий, включая одинаковые соседние E0/E1, без скрытого содержимого; по ней выбираются interval/drain/whole-frame timeout с записанным запасом. Безопасные пределы самой калибровки задаются в плане до её запуска. Точные длительности пока UNKNOWN и блокируют запуск. Ровно 3 положительные полные попытки со свежим nonce: baseline и два open-окна вокруг N1; все 3 должны декодироваться точно, без retries внутри попытки. Каждое отрицательное окно N0, N1-closed, N2, N3 выполняется один раз; N3 — последним, его возврат B требует новой квалификации вне этой серии. Одно неудачное окно сохраняется и не заменяется «удачным»; изменение декодера начинает новую редакцию, а не ремонт старых результатов. Это ограниченная воспроизводимость, не статистическая гарантия надёжности канала.

В той же серии обязательны контроли той же длительности, с неизменённым активным B-observer/decoder:

| Контроль | Реальное действие | Ожидаемое / смысл |
|---|---|---|
| N0: без передачи | A не вызывает VoN emitter, остальные условия прежние | Ни одного валидного кадра; выявляет фон/самовоспроизведение |
| N1: radio path open/closed/open | Один реальный разрешённый разрыв native radio-пути при сохранённом Rpl и B-наблюдении, затем восстановление; конкретный способ/authority/readback фиксируются до опыта | На closed нет кадра; до/после свежие положительные кадры. Если разрыв не доказан, контроль INCONCLUSIVE |
| N2: локальная имитация A без отправки | На A тот же генератор/временной рисунок, но native VoN вызов отсутствует; никакой передачи расписания B | На B нет кадра; отсекает управляющий/файловый data-plane |
| N3: B отсутствует | A выполняет ограниченный запрос при реально отключённом B | Никакого B-side receipt; только контроль verifier, не доказательство отрицательной доставки. После возврата — новый run/binding, без приёма старого кадра |

Для N0/N1/N2 отсутствие кадра считается отрицательным контролем только при доказанной непрерывной работоспособности B. Положительный кадр в любом из них отвергает заявленную изоляцию. Очереди должны быть очищены штатно/выдержан доказанный drain до окна; догадки о mute, питании, частоте и задержке недопустимы. Любая новая mutation для N1 требует отдельного плана и ownership; если безопасный разрыв недоступен — BLOCKED_CONTROL, не выполнение опасного fallback.

### P3 — определить положение radio-only mute относительно наблюдения

Сначала требуется установить карту этапов: вход A, native transport/routing, RX callback, playback callback, фактический выход. Их полный порядок, связь callbacks между собой и SetMuteState-stage сейчас UNKNOWN; перечисление не задаёт уже доказанную цепь. Физический разрыв N1 и mute — **разные воздействия**, успех одного не доказывает другое.

H2 `BaseTransceiver.SetMuteState` — только кандидат на отдельной изолированной тестовой приёмной рации B с одним источником. До вызова нужны отдельный контракт прав/RX-TX scope/возврата и доказанное исключительное владение (§7). Social whole-player mute исключён. Оба B-наблюдателя и декодер MUST работать неизменно до/во время/после mute, без локального hide/drop/disable.

- Если raw RX или decoded marker продолжается, а реальный playback меняется ниже него, этот observable расположен до gate и **не годится для LS-05**. Не глушить его программно ради совпадения.
- Если sound-start исчезает, это ещё не доказывает отсутствие последующего слышимого хвоста или порядок native-stage; нужны квалификация callback и независимый post-gate выход либо достаточный источниковый контракт.
- Если marker и player-voice используют разные gates/форматы, packet-заменитель не принят даже при корректном nonce.
- Direct проверяется отдельным неизменённым native direct источником и независимым выходом до/во время/после radio воздействия. Null-receiver callback — максимум наличие direct-события, не сохранность выхода. Если для этой проверки нужен реальный голосовой вход/изолированный аудиовыход, применяются gates аудиопротокола; пакетная ветвь не отменяет их молча.

Нет безопасного H2 ownership или post-gate/direct observable → BLOCKED_MUTE_STAGE / BLOCKED_DIRECT_CONTROL; допускается только более слабый отчёт о доставке, не LS-05/06 PASS.

### P4 — реальное участие RICON, отдельный закрытый gate

Даже квалификация штатного marker не подключает актуатор. До P4 нужны отдельно согласованный engine-adapter/контракт и план, не изменение чистого seam «на месте». Трасса: реальные lifecycle/device/lease → pair evaluation → publication → accepted snapshot/ordering → server binding → проверенный TOOL_ONLY adapter → native readback → независимый B-output.

**Проверяемый provenance:** конкретный accepted `(epoch, linkId, linkSeq)` MUST быть связан с результатом того же вычисления S3 [RICON_PairResult](<../../addons/RICON_Core/Scripts/Game/RICON/Core/RICON_PairService.c#L87-L103>): `m_ePairCode=EVALUATED`, `m_bHasBudget=true`, `m_bValid=true`, точными reason/state/Q и направленным ключом `(leaseId, rxDeviceId, rxTransceiverIndex, rxPlayerId)` с исходным TX binding. MODEL_LOST допустим только при таком valid model результате со state LOST/Q=0 и совпавшей публикации. [BuildPlan](<../../addons/RICON_Core/Scripts/Game/RICON/Core/RICON_LinkPublication.c#L389-L412>) использует LOST/Q=0 и для технических отказов/исчезнувшей пары, а snapshot не содержит причины. Поэтому один LOST, соседний такт или reason от другого seq не подтверждают MODEL_LOST. Нет корреляции — UNKNOWN_PROVENANCE, без модельного вывода. Invalid budget/технический LOST, stale/replay, expiry, abort и rollback — отдельные записи.

**Terminal binding:** перед заменой registry MUST сохранить исходный проверенный binding/старую запись с device/lease/индексами и generation до завершения terminal snapshot и delayed expiry. [Публикация S3b](<../../addons/RICON_Core/Scripts/Game/RICON/Core/RICON_LinkPublication.c#L633-L670>) выдаёт terminal snapshot, но не включает TERMINATE в новый registry; lookup только в новом registry теряет нужную identity. Нельзя восстанавливать её по частоте, новому lease или новому handle. Для delayed expiry quartet `(epoch, talkerPlayerId, talkerDeviceId, leaseId)` сверяется с сохранённым исходным binding по REQ-von-seam §6; mismatch не действует на новую передачу. Adapter отдельно сверяет текущую generation/владение перед любым cleanup: старая запись сама по себе не разрешает mutation нового binding. Ограниченный срок хранения terminal-контекста и завершение/отказ cleanup должны быть заданы в плане, не вечным вторым registry.

DESIRED записывает решение модели; APPLIED — точный вызов и readback, не намерение/`void`-успех; OBSERVED — неизменённые native выходы B. Нужны наблюдаемое подавление и восстановление свежих кадров на том же post-gate участке, причинная связь с RICON, неизменность direct и cleanup. Сейчас P4 = BLOCKED: отдельного утверждённого adapter нет, LS-05 не выполнен. Будущий TOOL_ONLY adapter получает полномочие только из нового контракта, **не из возврата BLOCKED_NO_ACTUATOR**; чистый seam остаётся no-op с закрытым enum, без нового OK/engine-call. Двухролевой H2 опыт не даёт per-flow, 3-client, multiplayer-quality или production PASS.

## 6. Binding, gaps, boundedness и failure semantics

Bindings снимаются перед подпиской, проверяются при каждом callback и на каждой доступной смене entity/controller/radio/power/frequency/lease/epoch. Время polling само по себе не доказывает неизменность между опросами. Если событий/источников для контроля полного интервала нет, интервал помечается unproven; на нём нельзя вынести отрицательный PASS.

Observer health: регистрация на A/B, локальный monotonic heartbeat, счётчик непрерывной записи, overflow/GAP markers и terminal detach. План MUST задать численные heartbeat/queue/time/memory пределы после калибровки и до nonce. Потеря heartbeat, смена компонента/handle/поколения, gap, clock anomaly, истечение окна или неполный STOP инвалидируют попытку; они не означают MODEL_LOST. Смена binding требует нового attemptId/nonce и новой квалификации; старые delayed callbacks не присоединяются к новому кадру.

Вердикты различаются: NOT_RUN (не запускалось), BLOCKED (нет предусловия/прав/метода), INCONCLUSIVE (попытка без достаточной наблюдаемости), FAIL (при исправном контроле нарушено требование), PASS (только названный criterion с evidence). Runtime исключения не скрываются и не повышаются до PASS после автоматического повтора. Abort всегда завершает отправку, сохраняет неполный журнал и запускает только разрешённый cleanup.

## 7. Владение, безопасность и rollback

Lead единолично выполняет будущий запуск; перед ним проверяет текущие полномочия, build/commit/hash, отсутствие чужого Workbench/game и точные лимиты. Изменять только специально созданные тестовые сущности/состояния. Для любого mutable radio свойства нужны recorded baseline, identity/generation, owner token/область, запись собственного изменения и доказательство отсутствия внешнего writer. `IsMuted=false` и успешный setter сами по себе **не доказывают исключительное владение**.

При preexisting mute — не снимать его и не записывать как свой block. При UNKNOWN/BOTH ownership не вызывать unmute/restore по предположению; остановиться, сохранить конфликт и передать Lead явный cleanup blocker. Заранее неизвестное безопасное восстановление блокирует действие **до mutation**. Не использовать Social, capture, power/range/frequency как скрытый fallback к недоказанному актуатору.

После опыта остановить только свой emitter, снять все свои подписки, закрыть/хэшировать raw-журналы и вернуть только доказанно собственные изменения на том же binding; при rebind не переносить rollback на другой handle. Завершить только процессы, созданные runner, через удержанные handles. Чужие процессы/настройки не трогать. Проверить zero subscriptions, terminal records каждой роли, фактическое завершение процессов и результат сверки настроек. Неполный cleanup = LS-07 не PASS, даже при распознанном nonce. Старые B0e receipts не заменяют эту проверку.

## 8. LS-трассировка и допустимые результаты

| Intent | Требования / опыт | Что будет достаточным | Текущий статус |
|---|---|---|---|
| LS-01 | MP-03, P0/P1, §6 | Authority identity + обе локальные проекции handles/lease без gap | PARTIAL по исходному intent, новым опытом не проверялось |
| LS-02 | MP-01/02, P0/P2 | Допустимый VoN запрос A и свежий скрытый nonce; запрос не равен native TX | NOT_RUN; event contract UNKNOWN |
| LS-03 | MP-01/04, P1/P2 | Запечатанный B nonce и независимо доказанный один путь/протокол/участок доставки | BLOCKED_RX_ATTRIBUTION / PROTOCOL_EQUIVALENCE |
| LS-04 | MP-05, P2 | 3/3 положительных; N0–N2 без ложных кадров при исправном B; N3 — только verifier-контроль отсутствия B-receipt, не отрицательная доставка | NOT_RUN |
| LS-05 | MP-06, P3/P4 | Реальное RICON решение → доказанный adapter/gate → изменённая и восстановленная post-gate доставка | BLOCKED_NO_ACTUATOR / MUTE_STAGE |
| LS-06 | MP-07, P3/P4 | Отдельно доказанный native direct выход до/во время/после | BLOCKED_DIRECT_CONTROL |
| LS-07 | MP-07, §7 | Свои состояния/подписки/процессы возвращены, конфликтов нет | NOT_RUN |
| LS-08 | MP-02/05, §4/P2 | Исходники/декодер/параметры/все попытки/хэши/seal-before-reveal/cleanup | NOT_RUN; этот DRAFT не runtime evidence |

`AI_EVENT_REMOTE_OBSERVED` — предлагаемый промежуточный ярлык для квалифицированного remote AI-event, не для одного callback. Он не закрывает эквивалентность. `SAME_PATH_PACKET_PASS` допустим только при совместном LS-02/03/04/08 с доказательством протокола; не означает LS-05/06/07, слышимость, PCM/Opus byte parity или общий AUDIO_PASS. Локальная задача intent закончена только после **всех** LS-01…08. Любое ослабление критерия требует новой явной ревизии, не переименования статуса.

## 9. Альтернативы и открытые решения

| Вопрос/вариант | Решение черновика / stop | Владелец |
|---|---|---|
| Допустимые E0/E1, values/priority, AI-event на player-role | UNKNOWN; P0 до реализации и запуска | VoN + Enforce |
| Где B-output, sender/receiver attribution и native protocol | UNKNOWN; P1, при отсутствии BLOCKED_RX_ATTRIBUTION / PROTOCOL_EQUIVALENCE | VoN, независимая проверка QA |
| Queue/drain/cadence и численные пределы | Не угаданы; отдельная публичная калибровка до nonce, затем freeze | Enforce + Lead |
| Реальная identity/lease для AI-event | Не подменять synthetic lease или guessed frequency | Ядро |
| H2 права, охват, ownership и post-mute observation | UNKNOWN; без контракта не вызывать | VoN + Lead |
| RX timing-marker | Резервный более слабый эксперимент: только native RX, локальные часы B, фиксированный decoder, реальный input/PTT preflight и VAD/cadence; текущим Design не выбран. Класс только VON_RX_TIMING_MARKER, не media PASS/LS-03/05/06 | VoN + QA |
| Scripted campaign RPC | Прочитанный пример отвергнут как заменитель media; не доказывает невозможность всех messages | Lead + VoN |
| Реальный голосовой вход/выход | Допустимая независимая аудиоветвь по существующему протоколу; изоляция B на этом ПК ещё не доказана | Lead |
| Social mute, подавление decoder, общий seed/RPC/file payload | Запрещены: нарушают intent, не альтернативы | Все |

## 10. Gate ревью и самодостаточная передача

Состояние: транспортные P0–P4 — **DRAFT/BLOCKED по известным предусловиям**. D0 прошёл свой отдельный live-срез с результатом D0_LOCAL_EVENT_OBSERVED_A, не transport PASS. Новая V0-редакция — **DESIGN_REVIEW / NOT_RUN**, без реализации/compile/runtime; исходный вход `ec2d363465394950840be5921ff27fbd42c11770`, новая редакция ещё не записана. Следующий владелец Lead получает spec для VoN/QA ревью ограниченного input-shape preflight, затем отдельного записанного bounded plan; никакого автоматического повышения от D0. Production-блокировка не меняется. Автор новой редакции менял только этот файл; changelog ведёт координатор.

Перед спецификационным gate проверить: (1) VoN подтверждает точность API-различий и UNKNOWN native участка; (2) Enforce — осуществимость observer/алфавита без выдуманных API; (3) ядро — identity/lease/provenance; (4) QA — отсутствие обходного канала, фиксированный декодер, корректность негативных контролей/LS; (5) Lead — diff, ссылки и единственный write-scope. Для P0–P4 нельзя подменить этим техническим ревью human-validation механизма. D0 ограниченно открыт явным решением координатора под автономным поручением 799 после закрытия замечаний; это отдельный записанный gate, не автоматическое повышение всего DRAFT и не утверждение VoN-эквивалентности. Другие полномочия проверяются отдельно.

Следующему контексту читать входной `170e9c0`, этот документ **после записи его точной редакции**, intent, seam/core и связанные evidence из §1; parent AGENTS перечитать. Team-событие 799 и зафиксированное здесь FRESH_SESSION_WAIVED сохранять в handoff; строгую свежесть не заявлять и на последующие переходы исключение молча не распространять. Пока редакция спецификации неизвестна/не записана и её gate не пройден, не создавать implementation plan и не писать код.

Выход после ревью — исправленный DRAFT либо конкретный BLOCKED по P0/P1, затем явное решение gate и запись в Git при наличии полномочия. Следующий bounded plan обязан назвать точные файлы TOOL_ONLY, разрешённые вызовы, сроки/память/число процессов, проверки independence/binding/cleanup, CT/compile и текущий runtime scope. Ни этот документ, ни разрешение цели не разрешают новую RE, DLL/драйверы, UI/Windows изменения, SetCapture, Play или production integration сами по себе.
