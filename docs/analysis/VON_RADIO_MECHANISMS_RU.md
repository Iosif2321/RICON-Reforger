# Радиопередача VoN: механизмы, 32 влияния и проверяемые пробелы

<!-- VON_NATIVE_CODE_REVIEW_20261104_BEGIN -->
**Дополнение native-разбором, 2026-11-04:** текущая формула качества, radio/relay/editor filters, источники параметров и дальнейшая обработка звука установлены в [подробном графе](VON_RADIO_DATAFLOW_RU.html) и [кодовой карте](VON_RADIO_DATAFLOW_RU.md). Таблица ниже сохраняет датированный анализ открытых интерфейсов; прежние native UNKNOWN не заменяют новый адресный результат. Игровые испытания влияний, per-pair и интеграция остаются отдельными этапами.
<!-- VON_NATIVE_CODE_REVIEW_20261104_END -->

Дата: 2026-11-03. Пункт2 фазы Agora #6582. Статус: **CANONICAL_SYNTHESIS / SOURCE_COVERAGE_REVIEWED**. Карта охватывает script/native/device/environment/control границы; она не объявляет private native solver раскрытым или реальный player RADIO baseline полученным. Код и runtime этим документом не меняются.

Общая цепочка и уровни — [VON_PIPELINE_RU](VON_PIPELINE_RU.md); очередь и acceptance — [PLAN_NATIVE_VON_RU](../plan/PLAN_NATIVE_VON_RU.md). Последний измеренный результат — [E-agora-radio-native](../evidence/E-agora-radio-native-20261103.md), не само наличие новой карты.

## 1. Версия и первичные источники

Все Script-Diff ссылки закреплены на official **3d77cc212d5cda9922daf5f45635c7300d2d4cce / 1.8.0.13**. Pin+installed FileVersion и узкие совпавшие окна не означают whole installed source/native ABI parity. Source/EXE/HTML fingerprints и frozen supplemental reports перечислены в плане.

| Код | Источник | Что видно |
|---|---|---|
| C,V,NV | [Controller](https://github.com/BohemiaInteractive/Arma-Reforger-Script-Diff/blob/3d77cc212d5cda9922daf5f45635c7300d2d4cce/scripts/Game/VON/SCR_VONController.c#L341), [SCR callbacks](https://github.com/BohemiaInteractive/Arma-Reforger-Script-Diff/blob/3d77cc212d5cda9922daf5f45635c7300d2d4cce/scripts/GameCode/Network/SCR_VoNComponent.c#L78), [native declarations](https://github.com/BohemiaInteractive/Arma-Reforger-Script-Diff/blob/3d77cc212d5cda9922daf5f45635c7300d2d4cce/scripts/Game/generated/Components/VoNComponent.c#L19) | Dispatch/eligibility/TX/capture и notifications; source catalog/blob identities в pipeline |
| G | [Gadget manager](https://github.com/BohemiaInteractive/Arma-Reforger-Script-Diff/blob/3d77cc212d5cda9922daf5f45635c7300d2d4cce/scripts/Game/Components/Gadgets/SCR_GadgetManagerComponent.c#L940), blob `c659ad364843ab7c5501bf955755d5e26093a63a` | RADIO/RADIO_BACKPACK → entry на каждый native transceiver |
| E | [Radio entry](https://github.com/BohemiaInteractive/Arma-Reforger-Script-Diff/blob/3d77cc212d5cda9922daf5f45635c7300d2d4cce/scripts/Game/VON/SCR_VONEntryRadio.c#L17), blob `dbe3220ca3162278dc8bcf3b30e9b0428e07da7a` | Usability/power/mute/tune и native index vs menu number |
| A | [Autotune](https://github.com/BohemiaInteractive/Arma-Reforger-Script-Diff/blob/3d77cc212d5cda9922daf5f45635c7300d2d4cce/scripts/Game/VON/SCR_VONAutoTune.c#L33), blob `1d450c0e85735a66f8e34856256c641ca64cf5ab` | Faction/group/role writer; delayed update0.5s |
| R | [Radio gadget](https://github.com/BohemiaInteractive/Arma-Reforger-Script-Diff/blob/3d77cc212d5cda9922daf5f45635c7300d2d4cce/scripts/Game/Components/Gadgets/SCR_RadioComponent.c#L64), blob `8ca97e0b36fe043aa6fd1c90550a57e88d6cca68` | Whole-radio power и knob frequency writer |
| NR | [BaseRadioComponent](https://github.com/BohemiaInteractive/Arma-Reforger-Script-Diff/blob/3d77cc212d5cda9922daf5f45635c7300d2d4cce/scripts/Game/generated/Components/BaseRadioComponent.c#L16), blob `f2977d545a8bdde5fdc7628d81aeebb3a69eea38` | Radio-wide key/power/catalog/change notifications |
| NT | [BaseTransceiver](https://github.com/BohemiaInteractive/Arma-Reforger-Script-Diff/blob/3d77cc212d5cda9922daf5f45635c7300d2d4cce/scripts/Game/generated/Radio/BaseTransceiver.c#L12), blob `8e29df5ccc0c33ab8c53131ecbb6778f1ab98c12` | Frequency/mute/range/generic messages; native тела не раскрыты |
| NM | [RadioManager](https://github.com/BohemiaInteractive/Arma-Reforger-Script-Diff/blob/3d77cc212d5cda9922daf5f45635c7300d2d4cce/scripts/Game/generated/Entities/RadioManagerEntity.c), blob `3925a258518279d273950f688c4b7a79f804850f` | Powered spatial candidates, не список реально услышанных recipients |
| CV | [Campaign coverage](https://github.com/BohemiaInteractive/Arma-Reforger-Script-Diff/blob/3d77cc212d5cda9922daf5f45635c7300d2d4cce/scripts/Game/Systems/SCR_RadioCoverageSystem.c#L223), blob `4e3e201f445cea0bb4c27a536213725086ea81ea` | DistanceSqXZ239 ≤ range²241; обратное ребро со своим range; UPDATE_DELAY22=2000ms |
| MB | [Campaign base](https://github.com/BohemiaInteractive/Arma-Reforger-Script-Diff/blob/3d77cc212d5cda9922daf5f45635c7300d2d4cce/scripts/Game/Components/Locations/SCR_CampaignMilitaryBaseComponent.c#L2377), blob `35b0cddb7f6286882462c8190d2de989f0f9db24` | Antenna service scalar range → base Relay; service radio выключается |

Native API [BaseTransceiver](https://community.bistudio.com/wikidata/external-data/arma-reforger/ArmaReforgerScriptAPIPublic/interfaceBaseTransceiver.html) / [BaseRadioComponent](https://community.bistudio.com/wikidata/external-data/arma-reforger/ArmaReforgerScriptAPIPublic/interfaceBaseRadioComponent.html) и [BI VoN](https://community.bistudio.com/wiki/Arma_Reforger%3AAudio%3A_Voice_over_Network) читаются как DOC/API_DECL. Acoustic [Occlusion](https://community.bistudio.com/wiki/Arma_Reforger%3AAudio%3A_Occlusion) не объявляется RF propagation. Некоторые wiki direct-open возвращали403; использованы primary indexed extracts, не новый runtime.

## 2. От инвентаря к source и receiver

G548–641/666–687/940–997 связывает add/remove/local control с RADIO/BACKPACK entries, по одному на native transceiver. Меню нумерует i+1, native catalog — i; exact ref comparison нужен даже при совпавшем номере. RADIO_BACKPACK и LRR entry — категория/выбор, не измеренная дальность. C748–757 различает controller-selected VoN и компонент controlled entity; changing entity/editor/respawn инвалидирует старый binding.

C341–365/538–580 проверяет life state/usable entry/type, C623–638 выбирает SQUAD_RADIO+exact TX либо DIRECT+null TX, затем native capture. **Точная faction-policy для non-DIRECT:** `crossFaction || admin || effectiveLocalKey.IsEmpty() || radioKey == effectiveLocalKey`. Faction-known и непустой radio key не обязательны во всех ветвях. InitEncryptionKey801–809 при unknown faction сохраняет прежний cache; известная faction задаёт prospective effective key, включая пустой. Смена controlled entity сбрасывает cache отдельно. Только non-exempt непустой effective key с mismatch приводит к faction fallback; actual method/TX всё равно фиксируются.

[Source triage](<../../../Cellular System/output/agora-native-von-20261103/pulse-local-faction-triage.md>) отделяет наш LOCAL_FACTION_UNKNOWN guard от native policy. Readonly getter cache не меняет faction/key/flags. Same-faction — baseline-вариант, не универсальный запрет unknown faction; PREPARED по ADMIN при faction UNKNOWN не является voice/server permission PASS.

NV50–58 описывает OnReceive(sender,isSenderEditor,receiver,frequency,quality) как data-for-playback. Частота callback может отличаться от настроенной; quality0…1 не SNR/dBm/BER/RICON intelligibility. V78–85 пересылает sender/RX/freq/q в invoker, опуская editor flag. Это notification, не veto/PCM/final output. Засчитывается **B-process** native callback на positively qualified component owner, с sender=A и exact B-catalog receiver; own-B и A-proxy VoN наблюдаются раздельно. A-proxy SoundStarted не заменяет native VoN OnReceive. Старый own-B RX0 не доказывает ноль всех B-process receive.

## 3. Матрица32 влияний

Writer ниже — известный API/script caller, не доказательство отсутствия других native/user writers. В колонке probe указано минимальное исследование после qualified baseline; таблица не выдаёт его за выполненное. R01–R32 включают отдельную границу RICON, чтобы не подменять ею native модель.

| ID / влияние | Контракт, единицы и известный writer | Observable / текущий предел | Specific probe |
|---|---|---|---|
| R01 Run/epoch/PlayerId/entity generation | Server lifecycle + local refs; opaque IDs | I0 discrete mapping, не continuous ownership | Reconnect/respawn → fresh generations, old callbacks отвергнуты |
| R02 Active VoN component | C Assign/component reset; local ref | Controller getter и entity component различать | Own-B/A-proxy subscriptions tagged; editor/control switch отдельно |
| R03 Inventory membership | G RADIO/BACKPACK add/remove/local control | Полный owned gadget catalog; nested/vehicle scope явный | Add/drop1 own radio, snapshot count/order/refs до и после |
| R04 Prefab/model | Loadout/spawn/prefab inheritance | MpTest может выбрать US/USSR и разные catalogs | Deterministic loadout, class/resource+actual properties |
| R05 Entry number vs native index | G965–971: menu i+1/native i | Номер не exact handle identity | Ref-match entry transceiver с current GetTransceiver(i), stale negative |
| R06 Active/selected/usable/UI mute | C/E/menu отдельные bools | UI opacity/selected не TX/RX media | Снять все UI/native flags и actual TX при одном action |
| R07 Long-range selection | E56–58 backpack; C separate entry/toggle | IsUsingLRR ref equality не toggle readback/дальность | SR/LR source switch с exact TX и captured generation |
| R08 Frequency | NT setter kHz, proxies/server; menu/R/A writers | Native GetFrequency vs cached UI vs callback freq | Full-catalog all-B detune→matched recovery с healthy B sink |
| R09 Frequency domain/step | Per-device min/max/resolution | API min/max prose reversed; BI prefab Hz vs API kHz drift | Read numeric legal bounds/step и successful direct-set readback, не слепой×1000 |
| R10 ANPRC68/R148 fixture config | I0 ANPRC68 32000…68000/200; R148 38000…54000/200 | Игровые настройки, не реальные radio calibration | Выбрать numeric overlap38000…54000 и проверить все current RX |
| R11 Autotune | A33–103/202–231 role/group/faction,0.5s delayed writer | Setter snapshot не исключает later tune | Passive event/tune после role/group change, без собственного setter duel |
| R12 Radio encryption key | NR32–38 opaque radio-wide string | Equality private; author token/ACK/algorithm UNKNOWN | Legit key same/mismatch при fixed frequency; generic/player отдельно |
| R13 Local faction/cross-faction/admin | C555–575/801–809; effective key из cache либо известной faction | Cross/admin/empty effective key/match допускают faction-policy; unknown faction не обнуляет cache; не native media permission | Exempt/empty/match/mismatch, stale cache при null faction; actual method/TX и прочие guards обязательны |
| R14 Whole-radio power | NR SetPower bool, R/E callers | ON/OFF не watts/dBm; active-stream drain UNKNOWN | TX OFF/ON и RX OFF/ON отдельно, callbacks+output+late events |
| R15 Transceiver mute | NT bool; E234–260 UI mirror | Нет sender/lease/owner token; RX/TX/direct/order UNKNOWN | Owned baseline→mute→restore; A→B/C→B SAME RX+DIRECT controls |
| R16 Max TX range | NT49–53 owner(server), metres; installed clamp1…65535 | Web contract не указывает upper bound; law UNKNOWN | Server range sweep вокруг measured separation, B readback, reverse direction |
| R17 Authority/proxy role | Contract каждого метода свой; generations | Frequency authority не обещает key/mute/range semantics | A/server/B mirrors, actual changed property/role; неизвестный writer veto restore |
| R18 Comm method | C623–638 ECommMethod+exact TX | Nonnull TX не capture или media ACK | Log actual SQUAD_RADIO/DIRECT до/во время/после request |
| R19 Capture/held input | Native request+real DOWN/UP+clock | Return/m_bIsActive/500ms recency не input samples | Qualified microphone/route; capture-positive/stop/no-capture/leak controls |
| R20 Direct fallback/toggle | C350–358/555–567/598–600 | Missing entry/key/life may proximity; UP не universal silence | Preserve raw null/direct; distinct DIRECT before/DURING/after held radio gate |
| R21 Life state | C761–797 ALIVE/INCAPACITATED/DEAD | Life policy может запретить source без RF причины | Same binding, state changes, record dispatch vs capture vs RX |
| R22 Editor route | ED own component and native editor connection | isSenderEditor lost by SCR invoker | Character↔editor отдельный run/owner tags, не editor attach ради PASS |
| R23 RX source/receiver/component owner | NV50–58 native sender+local RX ref | B-process qualified owner, не own-B-only; sound handle иной | Observe both VoN owners, exact intended B receiver, null/unknown сохранять |
| R24 RX frequency/quality | NV raw kHz/q0…1, native writer | Frequency may differ; q formula/cadence/direction UNKNOWN | Callback+current tune отдельно; native quality vs independent output across controlled cases |
| R25 Geometry/antenna position | World metres, parent/device transforms | Body origin не antenna phase centre; XZ vs3D UNKNOWN | Same-XZ/synchronous-Y vs distance controls, exact transform/AGL |
| R26 Terrain/materials/forest/water/weather/interference | Native public RF law не раскрыт; RICON отдельные kernels | Acoustic occlusion≠RF; UNKNOWN≠0dB/LOST/нетэффекта | One-variable paired geometry/medium/weather; capture eligibility+native RX+output отдельно |
| R27 Vehicle/compartments/intercom | Native classes/prefabs+occupant topology | Intercom class empty API; passenger→radio-media route UNKNOWN | Enter/exit/seat, carried vs vehicle path, no alternate route; qualified sink |
| R28 Relay | Native RelayTransceiver, campaign range caller | Hop/filter/loop/key/concurrency law UNKNOWN | A/B вне прямой reach, one known relay ON/OFF, no alternate route+recovery |
| R29 Campaign coverage | CV XZ²≤range² directed edges, update2000ms | Не native quality/LOS/audio; NM candidates не final recipients | Log graph separately from native RX/output; не переносить campaign tick |
| R30 Generic radio message | NT BeginTransmission/OnDelivery separate native path | Same-world B+model TX admission PASS; not voice/payload/audio | Reuse as independent diagnostic control, не player baseline substitute |
| R31 AI event/proxy sound | Native event/Seed pairs/priority/AudioHandle | All-B wrongRF proxy persisted; not matched radio RX | Ordinary player input and B-process OnReceive/output; AI channel separately tagged |
| R32 RICON outputs | MHz/dB/Q/model state по собственным REQ | Native q/range не равны RICON parameters; h(t) не PCM | Accepted-origin chain + concrete qualified actuator; valid MODEL_LOST отдельно technical error |

## 4. Native, campaign, акустика и RICON не одна модель

**Range:** documented scalar maximum TX range в метрах не RF power. SetPower(bool) не dBm; native sensitivity/NF/bandwidth/squelch/SNR/interference setters не найдены в просмотренных interfaces. Один generic q≈1 при2m не идентифицирует формулу; GetRange того опыта не был записан. RICON fixture37→−200→37dBm меняла только собственный evaluator/admission, не native RF watts.

**Campaign:** CV223–258 использует DistanceSqXZ owner origins, directed adjacency по каждому range; repeating CallLater2000ms — campaign cadence. Отсутствие raycast в этом методе не доказывает отсутствия terrain в native VoN. Render culling/scene occlusion и Rpl streamout тоже не RF solver.

**Environment:** native diffraction/Fresnel/curvature/antenna gain/polarization/building/foliage/water/weather/jamming laws UNKNOWN. Их наличие и отсутствие не выводятся из class names. MB antenna deployment меняет scalar relay range и выключает service radios, а не доказывает геометрический RF gain. Indoor/vehicle/RoomSize/portal signals описывают acoustic rendering. RICON L2/P.2040/P.527/vegetation kernels имеют собственные domains/provenance/world-provider gaps; на native solver они не переносятся. Даже RICON L2 composite может дать ridge<flat; строгая монотонность любой преграды не universal acceptance.

**Quality→audio:** native0…1, notification q и ACP TransmissionQuality не объявлены одной синхронной frame variable; mapping к RICON intelligibility не калиброван. Brownian noise/bitcrusher в asset — radio coloration, не RF noise/BER. Fading kernel не подключён к production budget/PCM. Подробнее — [audio boundary в pipeline](VON_PIPELINE_RU.md#6-звук-и-возможное-воздействие).

## 5. Граница положительного радио-наблюдения

`RAW_NATIVE_RADIO_RX_OBSERVED` требует настоящего player input на A, actual RADIO+exact TX, native callback **в B процессе**, qualified component owner, sender=A и exact intended B receiver из полного bounded catalog. Reference strings сравниваются только внутри процесса/поколения; межпроцессный mapping — stable replicated IDs и validated binding. Нельзя угадать RX по частоте/времени/index.

Нужно сохранить raw null/editor/direct, частоту callback отдельно от current tune, original q, refs/generations/clock/GAP/terminal. Positive до negative обязателен; healthy source/sink, no-TX/all-RX mismatch/power отдельно и fresh recovery. Source-A-proxy VoN может оказаться допустимым owner после qualification; proxy CommunicationSound.SoundStarted не становится OnReceive. Текущий probe подписан на local-role VoN и A-proxy Sound; source-A-proxy VoN receive ещё не наблюдался этим прибором.

Этот tier не доказывает слышимость, post-mute ordering, fresh content, common player/AI protocol, per-pair gate, DIRECT preservation или production authority. Их проверяет [план](../plan/PLAN_NATIVE_VON_RU.md), а [старый falsifier](../evidence/E-agora-radio-native-20261103.md) остаётся неизменным.

## 6. Минимальный порядок специальных исследований

| Gate | Исследование | Принятие / отказ |
|---|---|---|
| G0 | Ordinary player RADIO, eligible по actual cross/admin/effective-key policy; exact RX+independent B sink | Нет input/eligible entry/source/receiver/output → конкретный UNQUALIFIED, не native impossibility |
| G1 | Frequency/key/power/range/mute по одной переменной | All relevant B receive paths, readbacks и unchanged source; recovery. Silence без healthy positive не gate PASS |
| G2 | Full path inventory | Handheld/backpack/deployed/vehicle/intercom/relay/editor отдельно; cap/lifetime, source generations; иной путь не скрывать |
| G3 | Environment pairs | Distance/range first; same-XZ/Y/clear-ridge/one wall/forest/water/weather отдельно. Native q/receiver/output и RICON prediction разные столбцы |
| G4 | Multi-source/lifecycle | Held A→B gate при C→B SAME RX и A→C/DIRECT healthy; reverse/second RX; JIP/respawn/rebind/expiry/late media |

Для distance sweep нужны реальные GetRange/positions, legal transforms и samples ниже/около/выше threshold с recovery; cached/probe tick не engine update rate. Сначала источник/receiver qualified, потом непрерывные native guarantees могут стать предметом отдельного эксперимента. Никакой новый media transport, physics law, native signature или code implementation этим документом не создан.
