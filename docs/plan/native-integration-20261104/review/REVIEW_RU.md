# Независимая проверка плана RICON + native VoN/Radio

Дата: 2026-11-04. Исполнитель: `codex.integration.review`. Владение: только этот файл. Чужие изменения и canonical source сохранены. Основание — TEAM.md и один ограниченный review актуального плана и трёх OPTIONS; завершённый native RE не повторялся.

**Вердикт: ACCEPT — DESIGN_REVIEW_ACCEPTED / ENGINE_CAPABILITY_NOT_QUALIFIED. BLOCK нет.** План пригоден как проект интеграционной границы и следующий набор различающих capability-проверок. Он не объявляет найденный Workshop actuator, собранный SDK adapter, runtime PASS или production-калибровку.

## Проверенная редакция

| Документ | SHA256 |
|---|---|
| PLAN_RICON_NATIVE_INTEGRATION_RU.md | `9958c9bb639a8d81c41f805dfef4b7b1ad2820653cda6f14ca30518cd55d0960` |
| routing/OPTIONS_RU.md | `582e80869aaa52e1a84cbd642770ec0c0dc6394b9fdd6470d640ed93867103e5` |
| dsp/OPTIONS_RU.md | `1c514c9e04b84506086a7e60abe2466bdbe9c600c4b2f58cdd7bf508572e266e` |
| hooks/OPTIONS_RU.md | `42f362203b6908ba13692d25d9560d41a7183a1e38287693849558420ee7b228` |

После технического ACCEPT в routing OPTIONS исправлены только три local link targets: `/C:/` → `C:/`. Обновлённый SHA проверен; это link-only delta без изменения технических выводов. Остальные три reviewed-документа не менялись.

## Результат по обязательным границам

| Вопрос | Проверенный контракт | Результат |
|---|---|---|
| Сохранение DIRECT | §4.B меняет radio contribution до overlay. MODEL_LOST исключает radio candidate, не удаляет общую listener-запись с допустимым DIRECT. §4.D сохраняет Direct shader; весь VON_RADIO не принимается за чисто радиоветвь | ACCEPT. Сетевой overlay и acoustic composition разделены; A/B/C qualification включает DIRECT до/во время/после |
| Серверный sender→TX | §4.A требует authoritative sender/controlled entity/logical TX/native ID/index/generation/lease до использования payload source radio. Native RPC owner/MAC не выдаются за это сравнение. Обычный sender не получает editor/AI права из enum | ACCEPT. Подстановка чужого TX или собственного carrier RX как TX имеет отдельный adversarial критерий; carrier пока cooperative-only |
| Power/mute и terminal filters | §4.B прямо сохраняет discovery/manager power/mute checks при замене population: terminal slot8 их не повторяет. Native frequency/key/self и реальные RX/owner/user restrictions остаются downstream | ACCEPT. Allow RICON не обходит native deny; whole RX mute/whole radio power не предлагаются как per-pair решение |
| Произвольная направленная population | Порт принимает собственных RICON receivers, включая допустимые маршруты вне stock geometry. Filter-only классифицирован как более узкая способность. Выбор RX/route на listener детерминирован существующим routing layer | ACCEPT. Один radius или last-writer-wins не подменяют матрицу A→B/A→C/C→B |
| Per-pair quality/dataflow | §4.C проверяет finite q∈[0,1], различает deny и q=0, запрещает поздний stock distance overwrite. Outgoing context содержит выбранные RX ID/index, а не копию source TX. Revision/expiry/generation действуют и без движения | ACCEPT. При минимальном server port готовая native q-цепочка допускает обычного клиента; отдельный custom signal не требуется |
| Calibration | §2 и §4.C/D отделяют SINR/production-разборчивость от audio-control q, требуют версионированный mapping и явно TEST_ONLY fixture для первых опытов | ACCEPT. Состояние BLOCKED_CALIBRATION не снято проектным документом или существующим DSP |
| Поддержанный API и предлагаемый hook | §§3/5/7 отличают observers/setters/custom RPC от нового server policy port. OnReceive и OnVoNUsed не становятся pre-gate; ScriptedRadioMessage не подменяет microphone VoN. Missing private source-signal binding остаётся GAP, а не «доказанное отсутствие» | ACCEPT. Статический native insertion point не объявлен опубликованным Enforce API; совместимость носителя расширения не выдумана |
| Lifetime/hard revoke | §5 использует существующие epoch/lease/snapshot/generation и technical/stale причины. §7 и hooks отделяют прекращение будущих server dispatch от in-flight/encoded/decoder/PCM/device tails; hard pre-playout/generation correlation требуют отдельного client контракта | ACCEPT. Ни stop-bit, ни callback, ни публикация snapshot не объявляются purge/played ACK или instant silence |

## Одно неблокирующее уточнение — исправлено

**FLAG-01, CLOSED:** первоначальный последовательный список §8 мог сделать custom-signal DSP опыт обязательной зависимостью перед квалификацией server port. Это лишняя зависимость: минимальный server port передаёт final q штатному клиенту по уже доказанному native полю.

Root добавил точное предложение после таблицы §8; tiny delta проверен в текущей строке166: шаг2 — **параллельный/неблокирующий** для шага3, server q не требует custom signal bridge. Исправление принято. Нового approval gate или повторной стадии исследования из этого замечания не следует. Открытых BLOCK/FLAG к проверенной редакции нет.

## Точечная сверка источников

- `RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Network/RICON_NetworkContracts.c:350–387`: ApplySnapshot и expiry seam после входных/identity checks действительно возвращают BLOCKED_NO_ACTUATOR. План не выдаёт этот seam за уже работающий эффект.
- `Core/RICON_PairService.c:273–334`: текущий EvaluatePair проверяет active epoch/TTL, TX/RX player generation, endpoints, access и powered consistency до BuildQuery/evaluator; модельные row.valid/reason/state/SINR/intelligibility сохраняются отдельно. Проекция native handles и cache-порт должны использовать это, а не строить второй oracle.
- `Device/RICON_RadioDeviceAdapter.c:174–198`: TryRead требует authority, существующий radio owner, допустимый index, соответствие transceiver.GetRadio и finite antenna position. Это проверка observation/handle, **не** готовая полная player→TX authorization; план правильно добавляет последнюю отдельно.
- `docs/req/REQ-link-eval.md:101–102`: production intelligibility/thresholds остаются BLOCKED_CALIBRATION, linear_test — TEST_ONLY. Native TransmissionQuality не подменяет эту недостающую модель речи.
- Frozen shadow `output/ricon-native-shadow-integration-staging/review-handoff.json`: SOURCE_ACCEPTED,27 offline checks, SDK/native NOT_RUN, production Core/canonical не применены, actuator NONE. Уровень доказательств плана соответствует receipt.
- Релевантные завершённые native witnesses повторно использованы адресно: SendVoiceData/source lookup и radio overlay; slot6 discovery против slot8 terminal policy; receive context RX semantics; per-sender VonStream quality setter; VON_RADIO с двумя shader branches; capture/queue/device revoke limits. Новых native targets или SDK/Ghidra/engine запусков для review не было.
- Учтён последний DSP constructor follow-up: null arguments могут создавать default subobjects и вызывать их first-reference hooks. Из них не выводится ни наличие arbitrary sender-signal binding, ни его универсальное отсутствие. Актуальный OPTIONS оставляет именно EXPLICIT_BINDING_GAP.

## Следующий инженерный результат

Продолжать уже принятый shadow/Core bridge и квалифицировать точный поддержанный server population/final-q port с source-binding, native eligibility, cache/lifetime контрактом. Изолированный custom-signal опыт допустим параллельно как самостоятельный DSP-кандидат. Первый actuator PASS должен различать A→B deny/degrade от сохранённых C→B того же RX, A→C и A DIRECT→B; отдельный control RPC/observer/boolean setter такого результата не заменяет.

Не проверены Enforce/SDK compile, доступность предложенного порта в engine, live A/B/C, слышимость, calibration и нагрузка. ACCEPT относится к дизайну и его честным capability-границам, не к production/runtime готовности.
