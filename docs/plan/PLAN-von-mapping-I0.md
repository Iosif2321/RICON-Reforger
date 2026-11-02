# Bounded plan — I0 passive VoN source mapping

Версия0.1, 2026-11-02. Статус **APPROVED_FOR_LIMITED_I0_IMPLEMENTATION_ONLY** решением координатора под поручением799 после scoped VoN/QA review (события1503/1508) и уточнения bulk-allocation границы; без runtime/нового запроса. Цель — недостающий фактический mapping после [V0 INCONCLUSIVE](<../evidence/E-V0-live-001.md>), не ещё один emitter. Основание: автономное поручение799, I0-секция [spec](<../req/REQ-von-marker-probe.md>) и VoN/QA review секции. FRESH_SESSION_WAIVED, не native idle/ownership proof.

## Наблюдения

На каждой локальной роли записать PC/controlled entity и component refs/generations; `SCR_VONController.IsVONDisabled`, его `GetVONComponent` против entity SCR_VoNComponent (две null не MATCH), active entry и bounded entries; у radio entry public method/usability/active/mute/gadget/transceiver/1-based entry number. Сопоставить non-null refs с native radio.GetTransceiver(i) по ссылке; не частоте. Inactive/delayed entry handle — кандидат, не active TX proof.

Read-only accessor protected controller flags допустим только как SCRIPT_STATE: m_bIsActive, m_bIsToggledDirect, m_sActiveHoldAction, m_eVONType. Никаких override обычных handlers или setters. Native capture-idle/права из script-state не выводятся.

В одном локальном world sample: A/B контролируемые entity origins и дистанция только если обе сущности в нём реально разрешены; иначе UNKNOWN. Native RplId/FindItem round-trip и локальные IsOwner/IsProxy/IsMaster, с exact API/source-check. Не смешивать RplId/PlayerId/deviceId и не считать local owner полномочием на VoN mutation. A proxy из PlayerManager может быть null: это UNKNOWN, не отсутствие репликации; не искать по близости. В первом I0 **нет новых control RPC**: если публичный mapping не разрешился, записать проблему, не подставить entity.

## Scope и пределы

- Enforce: отдельные новые TOOL_ONLY `RICON_LiveVoNMappingInspection.c`, `RICON_LiveVoNMappingInspectionGame.c`, `RICON_LiveVoNMappingAccess.c`, Workbench `RICON_LiveVoNMappingInspectionCheckPlugin.c`. Переиспользовать pure helpers только при byte/default сохранности D0/V0; источники emitter не менять.
- Lead: opt-in runner `-VoNMappingInspection`, exclusive с D0/V0/AudioObserve/ReadAuthOnly; public run-id, fresh profile/log routing как существующая guarded ветвь. Никаких ослаблений guards/lib/teardown.
- CLI `riconVoNMapping=1` + `riconCommunicationRun`. Без флага inspector отсутствует, no emit/source calls. Логи `[RICON_I0]`, отдельный verdict.
- Две own роли A1/B2, новый MpTest, scratch profiles/log roots; один Workbench+один Diag, внешний cap300/watchdog330с, единственный оператор Lead.
- Локальный t0 один раз, GetTickCount; max45000ms, snapshots не чаще3000ms, максимум16/роль плюс terminal. No catch-up и no deadline reset. Первоначальную own identity ожидать в том же t0 до20000ms; при отсутствии — BLOCKED_IDENTITY, без продления. Snapshot при missing remote entity допускается как UNKNOWN.
- Каталоги на snapshot: VON entries≤8; radio gadgets **всего≤4**; transceivers/radio≤4, итого≤16. Entries: сначала read-only GetVONEntryCount и отказ при>8, затем bulk GetVONEntries и проверка фактического Count. Gadgets при наличии только bulk getter могут временно выделить больший native массив: это API-зависимая непроверенная граница, не обещанный memory-cap. Сверхcap/расхождение — CATALOG_LIMIT/UNKNOWN и неполный каталог явно; не обходить/не удерживать лишние элементы, не выдавать часть за полный MATCH.
- Память: собственные retained refs — только текущие, не более16 transceiver и8 entry refs; native bulk временная аллокация заранее не гарантируется. История идёт в поток. Лог≤1024 строки ordinary+64 reserve, всего1088/роль. Reserve/total cap при любом порядке; overflow/GAP/clock reset — INVALID, не потеря скрыто.
- Subscriptions0, no callbacks/Invoke, no queue request, no payload/nonce/MP signals. Никакого SoundEventPriority/PlaySound/SetCapture, comm/TX/entry setters, mute/power/range/key/frequency/flags setters, AssignVONComponent/ResetVON, editor-connect или исправления disabled состояния. Getters только читаются; unavailable/NULL/authority/failure раздельны.

## Gates

1. Технический review точности read-only API и лимитов; фиксация spec/плана. Только после явного limited implementation gate — новые файлы и pure null/ref/index/catalog/deadline/default-off tests.
2. Полный readback/forbidden scan, headless ScriptEditor compile/pure без мира/I0-флага, отдельные raw/hashes; tests не являются live mapping.
3. Independent code/source/ops review → one controlled passive I0. Перед ним busy-process guard/точные хэши/dry-run/START. Неизвестный prompt остановит опыт.
4. После END: refs/gens/clock/каталоги/пути/terminal, процессные handles и восстановление verify. Ни один getter snapshot не означает непрерывный binding/idle/transport; SOURCE_MAPPING_OBSERVED/UNKNOWN/BLOCKED — не SAME_PATH_PACKET_PASS, direct preservation, radio permission или готовность актуатора.

Что может сломаться: getter отсутствует/не компилируется, component/entry не готов, профиль/роль не та, серверный mapping не реплицирован клиенту, enum/index неправильно истолкован. Во всех случаях сохранять UNKNOWN/BLOCKED и точный scope, не лечить состояние или начинать передачу. Этот план не выбирает новый backend/RE/driver и не открывает production.
