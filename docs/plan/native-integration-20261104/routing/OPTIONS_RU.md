# RICON → штатный radio routing: варианты и условия пригодности

2026-11-04 · `codex.integration.routing` · ownership только `routing/` · **STATIC_FEASIBILITY, runtime NOT_RUN**. Игра, сервер, Workbench, Ghidra, spawn и capture не запускались. Существующие PairService/Evaluator/LinkPublication/SnapshotReceiver и `BLOCKED_NO_ACTUATOR` не менялись.

**Результат:** штатные scalar setters полезны для состояния устройства и грубой native сети, но не выражают произвольную направленную матрицу RICON. В опубликованном API исследованной 1.8.0.13 нет найденного server pre-route recipient/source-binding hook. Tagged carrier underlay — отдельная содержательная архитектурная гипотеза, однако с неизменным SendVoiceData у неё есть **ранний server-trust stop condition**: source radioID/index не привязаны к подтверждённому voiceSender. Пока это не исправлено поддерживаемой серверной точкой вмешательства, underlay нельзя считать production server-authoritative решением.

## 1. Что требуется от актуатора

Один и тот же кадр/период должен обеспечить `A→B RADIO=LOST`, сохранив `C→B RADIO`, `A→C RADIO` и физический `A DIRECT near B` до/во время/после воздействия. У B используется один и тот же реальный receiver для A/C. Сохраняются настройки/права пользователя, отсутствие актёра и смена generation не превращаются в разрешение или устаревший звук. Receiver selection на сервере и per-source q у клиента — разные задачи.

Короткий контрпример для общих property setters: A=(0,0,0), B=(20,0,0), C=(0,0,20), по одному трансиверу, одинаковые частота/непустой ключ, все power ON/mute OFF. C→B также достижимо. Для A расстояния до B/C равны20м, а RICON может разрешить A→C и запретить A→B по своей физической модели. DIRECT A→B находится внутри текущего vanilla68м; это отдельный контроль, не radio PASS.

## 2. Переиспользование текущих свойств и relay

| Кандидат | Поддерживаемый доступ и фактический scope | A/B/C предел; cache/DIRECT | Вердикт |
|---|---|---|---|
| Scalar range | `BaseTransceiver.SetRange(float)` [P1:49–58]; один source/hop scalar, native q=`1−dXYZ²/R²`, native finite clamp1…65535 | При равных dAB=dAC один R не исключает B, сохраняя C; qAB=qAC. Для близкого B<1м min clamp дополнительно ограничивает hard exclusion. Changed range bumps manager revision и sync; unchanged value не invalidate. q=0 на границе всё ещё допускается геометрическим gate | Использовать как собственную физическую характеристику/грубый envelope; **не per-pair gate/quality** |
| Power | `BaseRadioComponent.SetPower(bool)` [P2:39–45], весь radio/catalog; proxy request либо native local store/registration/revision | B OFF убирает также C→B; A OFF затрагивает A→C. Сохранение DIRECT само по себе не спасает матрицу. Нельзя захватывать пользовательское питание без ownership/lifetime | Device-state reuse, **не per-pair** |
| Mute | `BaseTransceiver.SetMuteState(bool)` [P1:36–43], receiver-wide byte; обычный Radio6 фильтрует mute при graph build, local decoder проверяет его независимо | B mute блокирует также C→B. Setter не bumps graph revision и не входит в transceiver sync. При уже выбранной radio record local mute обходит весь новый packet audio, включая потерю отдельного direct-record fallback после overwrite | Пользовательский RX control, **не radio-only per-pair gate** |
| Frequency / key | Frequency per transceiver [P1:26–29]; key per whole radio [P2:31–37] | В однотрансиверном примере A→C и C→B требуют совпадающих каналов/непустых ключей; это также допускает A→B. Пустой message key bypass не создаёт произвольную матрицу. Key/frequency writers и autotune могут перезаписать управление; source controller дополнительно проверяет faction-key policy | Реальные каналы/права устройства; **не матрица направленных links** |
| Relay | Нативный RelayTransceiver работает как graph expansion; существуют штатные prefab/config + inherited setters | Добавление relay не удаляет уже допустимое прямое A→B ребро. OFF общего relay удаляет и другие обслуживаемые links. Native q — max по transceiver/current hop, не HDSP route product; затем map может перезаписать listener record | Переиспользовать штатную инфраструктуру при подходящей топологии; **не произвольный PairService actuator** |

Для всех вариантов надо сохранять исходные значения/автора и generation; бездоказательный restore поверх ручки/menu/autotune/другого мода недопустим. Никакая lease-обёртка не превращает общий scalar/bool в отсутствующий per-pair механизм.

## 3. Расширяемость: что действительно доступно

| Поверхность | Exact evidence | Что позволяет / чего не доказывает |
|---|---|---|
| Свой ScriptedRadioComponent | Installed SDK [D1:123–128,172–239]: OnPostInit/OnInit/OnFrame; наследует P2 | Lifecycle/config/UI интеграция и штатные свойства. Нет опубликованного callback с sender/listener list, q или bool veto |
| RadioManager query | [D2:118–143] `GetTransceiversInRange(position,range,out array)` | Запрос кандидатов, не writer native recipient graph. Обычный native Radio6 непосредственно использует native spatial/query chain; вызов своего script query не изменяет этот chain |
| BaseTransceiver subclass / «scripted transceiver» | Полное P1:12–59 не содержит GetNeighbors/FilterRecipients/OnBuildRoutes; native graph вызывает private primary slot6 и slot8 | Можно исследовать обычную script/config композицию, но **нет доказанного public bridge для замены native virtual graph method**. Наследование или новый script method с придуманным именем не является таким bridge |
| ScriptedRadioMessage.OnDelivery | [D3:123–151], generic `BeginTransmission/BeginTransmissionFreq` в P1:56–58 | Штатные generic/control messages. Player VoN создаёт private VonRadioMessage и вызывает graph/delivery непосредственно; он не передаёт voice frame вашему ScriptedRadioMessage и не вызывает public BeginTransmission wrapper |
| OnReceive / OnVoNUsed | P3:50–69; native RE подтверждает порядок | OnReceive после audio processing либо muted bypass, OnVoNUsed после dispatch final map. Это наблюдение, не предварительный veto и не изменение уже отправленного q |

Итог ограничен pinned declarations/native path, а не заявлением «никакой будущий API невозможен». Для настоящей modernized graph extension нужна **поддерживаемая** точка перед native graph/map dispatch, где сервер может связать voiceSender с разрешённым TX и применить pair policy. Название/сигнатура такой существующей точки в проверенных API не найдены; новый SDK hook не выдумывается.

## 4. Tagged carrier underlay: сначала доверие, затем механика

Гипотеза сохраняет реальные logical devices/keys/positions в RICON; native capture/Opus/network/decoder работают через internal TX_A и RX_(A,B), а control-space geometry кодирует q. Это не отклоняется просто за отдельное control-space: native RF и actor render берут разные входы. Но необходимые условия надо доказать.

### 4.1 Ранний A/B/C source-spoof контрпример

Native server связывает обычный senderId с владельцем VoN (`2A0ADF..2A0AE6`). **Независимо** он принимает `ctx+0x0c=sourceRadioId`, вызывает lookup (`2A0CDF..2A0CE3`), берёт key выбранного radio (`2A0D0D`) и `ctx+0x14=TXindex` (`2A0D3B..2A0D41`). В выбранном chain нет сравнения этого radio/tagged-TX с sender A. Native SetTransmitRadio `[2A2590,2A262E)` лишь сохраняет weak ref и не выполняет inventory-owner/TX-role validation.

Пусть RICON A→B запрещён, C→B разрешён. RX_(A,B) выключен; RX_(C,B) с K_C включён. A использует собственный корректно owned VoN, но выбирает source TX_C. Сервер всё равно ставит **voiceSender=A**, строя carrier graph по C, с K_C; enabled RX_(C,B) принимает голос A. Native peer/VoN-owner checks и packet MAC не исправляют эту независимую application binding. Скрытие RplID/ключа — не authorization proof.

Более того, для обычного RadioTransceiver одной inventory ownership проверки мало: при разрешённом C→A у A может быть собственный RX_(C,A) с K_C, который также способен стать source. P1 не предоставляет RX-only переключатель. Специальный RX-only subclass мог бы исключить именно эту попытку, но такая carrier-конструкция не квалифицирована и не исправляет выбор чужого TX_C. Нужна проверка **(confirmed sender A, current logical device, разрешённый tagged TX_A, generation/lease)** перед routing, а не доверие client selection/hidden UI.

**Статус: cooperative-only при неизменном native handler.** Серверный SetTransmitRadio mirror не меняет того, что handler читает incoming ctx. Local OnCapture/controller mapping остаются клиентским поведением; OnVoNUsed/OnReceive слишком поздние. Production candidate возобновляется только после доказанного supported server pre-route binding/gate. RICON ApplySnapshot не переводится в PASS.

### 4.2 Что останется проверить только после trusted port

Раннего trust blocker достаточно для текущего production-решения; полный spawn/UI/projection prototype сейчас не нужен. Последующие незакрытые условия сохранены как obligations:

- **Carrier ownership/UI:** GadgetManager [P4:940–972] добавляет VON entries только для RADIO/RADIO_BACKPACK gadgets. Это не доказывает hidden inventory prefab, public insertion, вместимость, независимый transform и native OnDelivery→physical VoN owner. Объектов не создавали.
- **Projection/cache:** математически `d=R*sqrt(1−q)`, но q=0 ещё admitted. Cache использует physical source position/cells, RF — radio origins; changed power/key/range/frequency дают revision signal, mute и unchanged setter не дают. Атомарность projection/gate и публичный invalidate не квалифицированы.
- **DIRECT/user mapping:** private local RX mute может убить overwritten direct fallback; server power/key exclusion до map — другой механизм. Native controller [P5:555–575] сравнивает carrier key с faction cache и может перейти в DIRECT. Real user power/mute/frequency/key и policy должны оставаться у LogicalDevice; admin/editor/cross-faction fakery не допускается.
- **Record/lifecycle:** последняя eligible delivery одного VoN component перезаписывает q/freq/radio/index/mode. Требуется одна выбранная RX record на pair и отсутствие competing paths; stale/missing actor, respawn/epoch/expiry обрабатываются существующим core lifecycle, без дублирующего registry. VON_RADIO имеет Direct+Radio render branches.
- **Budget:** N TX + N(N−1) RX = N² radios: 100 игроков→10000, 200→40000. Sparse N*T сокращает объекты ценой lifecycle latency. Replication/inventory/transform budget и global revision/graph churn не измерялись; capacity PASS отсутствует.

## 5. Минимальная следующая capability-проверка

**Только при появлении trusted server-policy port**, который проектирует root; кода effect сейчас нет. Один binding fixture, без hidden spawn/O(N²): A/B/C с реальными owned VoN и здоровым capture/sink, B вне DIRECT range A. Разрешить C→B, запретить A→B. Client A передаёт корректный A-owned VoN, но source C_TX/index, затем собственный RX_(C,A) как TX. Port обязан отклонить обе попытки **до native graph/map dispatch**, не меняя senderId и не подавляя legitimate C→B/A→C. Чистый отказ из-за отсутствующего reference/input/sink — UNQUALIFIED, не binding PASS. Для разрешённых случаев требуется свежий B-side output; OnReceive один недостаточен. DIRECT near B до/во время/после — обязательный следующий контроль того же контракта.

**Рекомендация:** сохранить native capture/Opus/UDP/decoder и текущий RICON shadow/core. Следующий результат — конкретный trusted pre-route source/pair gate с указанным adversarial binding test. При отсутствии поддерживаемой точки нужен точный engine/API extension request; production carrier adapter, spawn/UI/projection/нагрузка до этого не разрабатываются. `ApplySnapshot` остаётся `BLOCKED_NO_ACTUATOR`.

## Evidence anchors

- P1 — [BaseTransceiver.c, pinned official source](https://github.com/BohemiaInteractive/Arma-Reforger-Script-Diff/blob/3d77cc212d5cda9922daf5f45635c7300d2d4cce/scripts/Game/generated/Radio/BaseTransceiver.c), полный public surface 12–59; локально canonical `.re/notes/native-von-20261104/research/index/input_sources/official/scripts/Game/generated/Radio/BaseTransceiver.c`.
- P2 — [BaseRadioComponent.c](https://github.com/BohemiaInteractive/Arma-Reforger-Script-Diff/blob/3d77cc212d5cda9922daf5f45635c7300d2d4cce/scripts/Game/generated/Components/BaseRadioComponent.c), key/power/catalog и notification callbacks.
- P3 — installed source snapshot `canonical .re/notes/native-von-20261104/research/index/output_consumers/source_matches/scripts/Game/generated/Components/VoNComponent.c:21–84`; SDK HTML `interfaceVoNComponent.html:125–148`.
- P4/P5 — pinned [GadgetManager](https://github.com/BohemiaInteractive/Arma-Reforger-Script-Diff/blob/3d77cc212d5cda9922daf5f45635c7300d2d4cce/scripts/Game/Components/Gadgets/SCR_GadgetManagerComponent.c#L940), [VONController](https://github.com/BohemiaInteractive/Arma-Reforger-Script-Diff/blob/3d77cc212d5cda9922daf5f45635c7300d2d4cce/scripts/Game/VON/SCR_VONController.c#L538); локальные frozen source snapshots прочитаны.
- D1/D2/D3 — установленные BI SDK HTML: `Workbench/docs/ArmaReforgerScriptAPIPublic/html/interfaceScriptedRadioComponent.html`, `interfaceRadioManagerEntity.html`, `interfaceScriptedRadioMessage.html`; точные signatures/line windows выше. Это installed API documentation, а не новый runtime. Бounded raw GitHub fetch этих трёх generated файлов вернул cache miss; поведение не выводилось из отсутствующего web ответа.
- Native source/body evidence reused: [canonical dataflow](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/analysis/VON_RADIO_DATAFLOW_RU.md>), [radio contract](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/von-re-20261104/radio/NATIVE_RADIO_CONTRACT.md>), [transport](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/von-re-20261104/transport/FINAL_TRANSPORT_RU.md>), world/relay/lifecycle/audio owning reports. Game1.8.0.13 SHA256 `1d221a990fa16c78fe02f685a7ba1330be115ca93cd3b641e67e5d3f1b40d9c9`.
- Exact source-selection proof: TMP `batch02/export/VoN_RadioGraphCaller.asm` VA`1402A0ADF..AE6`, `1402A0CDF..D41`; `VoN_SetTransmitRadio.asm` full`1402A2590..262E`; OnVoNUsed at end of SendVoiceData after final recipient dispatch. No engine/Ghidra/packet test run in this integration phase.
