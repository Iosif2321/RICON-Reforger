# Carrier RX-leaf: условная RF-изоляция и граница доверия

2026-11-04 · `codex.goal.carrier` · STATIC_ASM_REUSE / READ_ONLY_CHECK. Владение только `carrier-authority/`. Предыдущие routing/native контракты не изменены. Движок, Play, server, capture, Ghidra, DLL и binary patches не запускались. Дополнительных Ghidra/callee requests: **0**.

**Результат:** ограничение server-side range=1 у обычного RX leaf действительно ограничивает его собственный исходящий первый переход; большая дальность destination/C_TX не делает такое ребро обратимым. Это полезное **условное RF-доказательство**, а не полный server-authoritative carrier contract. Открытый прежде trust-вопрос скрытого чужого TX_ID не устраняется range собственного RX: root/независимый Rpl worker уже установили global raw-ID resolution без peer-policy; wire decoder проверяется отдельным исполнителем. По последнему указанию root дальнейшее расширение incoming Broadcast/RPC/ownership исследования прекращено до этого wire proof. Production candidate здесь не объявлен рабочим.

## 1. Кто задаёт RF range на самом деле

Pinned game EXE1.8.0.13 SHA256 `1d221a990fa16c78fe02f685a7ba1330be115ca93cd3b641e67e5d3f1b40d9c9`, preferred base `0x140000000`, RVA=VA−base. Использованы уже полученные ASM и reviewed native reports; новый indexing/fullscan не выполнялся.

`batch02/export/Radio_ConcreteSlot6.asm`, complete `[0x1405e24a0,0x1405e26b4)`:

- Source pointer находится в R15. `0x1405e24f6` загружает **source** float `[R15+0x40]` и передаёт его spatial broadphase вместе с source origin.
- Candidate radio, равное source owning radio, пропускается (`0x1405e254d..0x1405e2551`). Затем перебираются его transceivers; null/muted кандидат пропускается (`0x1405e257d..0x1405e258a`). Powered-radio broadphase — отдельный предыдущий фильтр.
- `0x1405e25a3..0x1405e25d5` считает XYZ distance²; `0x1405e25bf/0x1405e25d9` читает/возводит в квадрат **source** range; `0x1405e25dd..0x1405e25e0` отвергает distance²>sourceRange². Destination range здесь не читается.
- Graph quality `[0x1405df7f0,0x1405dfc29)`, `Radio_GraphCandidate.asm:195–212`, также читает **current-hop source** range+0x40: `q=1−dXYZ²/Rsource²`. Это не symmetric link test и не максимум двух дальностей.

**Условный контракт для обычного RadioTransceiver S:** при конечных server-side positions, range(S)=1, distance(S,B)>1 и отсутствии расширяющего обходного пути исходящий S→B не включается в этот neighbor list. RX(B).range=1 или TX(C).range=65535 не меняют вывод. На distance=1 equality admitted; q=0 не является hard gate.

Числовой пример: source stolen RX_(C,A) имеет x=0,R=1; RX_(C,B) x=10,R=1; обычный TX_C x=0.5,R=65535. Source→RX_(C,B) не проходит; source→TX_C может попасть в список, но обычный RadioTransceiver.slot5=false, поэтому **сам по себе большой TX_C не пересылает** этот кадр дальше. Нельзя автоматически считать все radios ретрансляторами.

Контракт требует фактических server-side origins, а не только запланированных control-space coordinates или клиентского readback. Уже reviewed source path использует radio-owner GetOrigin. Сохранность attachment/projection transforms, stale cache и placement не проверялись этим заданием.

## 2. Relay-контрпример: почему одной дистанции между leaf недостаточно

Достаточное топологическое условие должно исключать reachable expandable nodes, а не только держать RX разных listeners дальше1м.

Контрпример в тех же метрах/одной matched frequency F и непустом routing key K_C:

| Node | Position X | Native class | Server range |
|---|---:|---|---:|
| S = RX_(C,A), выбранный A как source | 0 | RadioTransceiver | 1 |
| R = посторонний/ошибочно созданный relay | 0.5 | RelayTransceiver | 65535 |
| B = RX_(C,B) | 10 | RadioTransceiver | 1 |

1. S→R проходит собственный range1 gate:0.5≤1.
2. Graph вызывает candidate.slot5 (`0x1405df94a`). Для Relay он true; nonempty message key K_C и relay radio key K_C равны, поэтому unvisited R добавляется в expansion stack (`0x1405df955..0x1405dfa84`).
3. Теперь source текущего перехода — R. Relay neighbor function использует **R.range**, поэтому R→B проходит при9.5≤65535. Same-frequency branch требует receiver tune=relay tune (`0x1405e2c60..0x1405e2c6a`), затем XYZ gate (`0x1405e2c70..0x1405e2ca8`). AnyFrequency=true выбирает другой manager-query branch; это не требуется данному контрпримеру.
4. Base terminal frequency/key tests допускают B; предыдущий weak hop q не перемножается с final-hop q. Поэтому маленький range(S) не ограничивает суммарную дальность маршрута.

Полные источники: `batch02/Radio_GraphCandidate.asm`, `batch05/Relay_FullSlot6.asm`547/547 bytes и RTTI/vtable false/true slots из accepted relay report. Loop visited-set мешает бесконечным повторам, но не этому одноразовому обходу.

Этот пример **не утверждает**, что malicious client может создать/переключить server-owned relay или получить K_C. Он доказывает, что гипотезе нужен дополнительный server invariant: для stolen RX-source не существует reachable same-key expandable relay/alternative native class, который открывает запрещённый pair. Ordinary Radio-only type inventory с проверенной slot5=false и server-controlled layout — существенная часть условия. Совпадение класса по скриптовому имени или один setter не доказывает сохранение native dispatch type.

## 3. SetRange: documented owner-only ≠ guard в public wrapper

Pinned generated API `BaseTransceiver.c:46–53` описывает SetRange как effective only on owner/server. Но complete `batch02/Radio_SetRange.asm` `[0x1405e0410,0x1405e04b4)` имеет фактическое тело:

1. VM argument/object resolution; чтение текущего transceiver+0x40 и запрошенного float.
2. Unchanged обычное значение возвращает управление.
3. MAXSS(1,input), MINSS(65535,result), **локальная запись+0x40** (`0x1405e046b`). Для NaN это не finite validator.
4. Range-change callback helper `0x1405ddb10`, optional manager revision increment (`0x1405e0497`), CommonSync `0x1405de520`.

**В этом complete wrapper нет IsOwner/IsProxy/IsMaster/Rpl role branch и нет отдельного SetRange server-request RPC**, в отличие от frequency/power setters. Поэтому фраза «A не может поменять range своего proxy» как локальный факт неверна: wrapper пишет proxy-local значение. Для безопасности важен более узкий вопрос — может ли это изменить **server authoritative instance**, который использует SendVoiceData graph.

Known CommonSync сериализует freq/range и отправляет native `BaseRadioComponent::RPC_SyncTransceiver_BC`, hashFA533DE1. Generic native classifier `[0x140847560,0x140847619)` для receiver selector Broadcast3 и client-role0 не выбирает queue result2: ветви дают local invoke1 либо reject0. Следовательно обычный public SetRange→этот Broadcast submit сам по себе не является доказанным client→server range-write path. Это полезный факт маршрута, но его нельзя раздувать в универсальную packet-admission гарантию.

Accepted transport report отдельно фиксирует incoming RPC owner check: в server-role processing branch node.owner+0x28 сравнивается с incoming sourcePeer+4 до вызова handler (`E67F66..E67F71`). Он относится к **calling replicated RPC node**, а не к любому radio ID внутри чужого owned VoN/RPC context. По указанию root этот check не разворачивался дальше; source-ID worker отдельно замыкает wire resolution. Не заявляется ни обход ownership данного calling node, ни полная неизменяемость server range по одному API-комментарию.

Ключ также не «неизменяем по умолчанию»: previously proven SetEncryptionKey допускается predicate IsOwner(bit1), не IsMaster/Authority-only. При заданном root условии RX действительно server-owned/non-owner у A это отказывает локальной записи, но owner/role должен проверяться на фактическом объекте. Power/frequency имеют другие request paths; свойства нельзя смешивать в единый owner-only запрет.

## 4. Что остаётся от carrier trust-гипотезы

- **Доказанное улучшение:** stolen *ordinary fixed-range RX leaf* не становится дальним TX лишь из-за того, что удалённый intended TX_C имеет большой range. Предыдущий собственный-RX контрпример нельзя сохранять без учёта этого условия.
- **Условие:** source range/position/class/topology должны быть server-controlled именно на graph instance; отсутствие same-key expandable paths и достаточное пространственное разделение нужны во всех актуальных generations/cache states.
- **Независимый ранний trust stop:** server SendVoiceData по-прежнему принимает sourceRadioId/index отдельно от подтверждённого senderId. Выбор **настоящего TX_C**, если его raw ID может разрешиться у client A и на сервере, обходит этот RX-leaf limit. Key/ID secrecy не была признана capability. Root/второй worker уже получили global raw-ID bank lookup без peer-policy; wire-level результат принадлежит их отдельному артефакту.
- Поэтому данный узкий результат не повышает carrier underlay до production/qualified native VoN integration. Положительный RF leaf contract и отрицательный/ожидаемый ID trust result должны оставаться разными доказательствами.

**Следующий шаг для root — только завершить уже назначенный source-ID wire proof и принять stop/continue decision.** Новые hidden-radio spawn/UI/cache/framework работы этим исполнителем не создаются. Дополнительный callee нужен только если второй worker найдёт в decoder реальную peer-bound mapping/capability, меняющую этот verdict; заранее новых targets не запрошено.

## Evidence inventory

Reused existing raw: `C:/Users/Ded/AppData/Local/Temp/ricon-von-re-20261104/`:

- batch02/export/Radio_SetRange.asm; Radio_ConcreteSlot6.asm; Radio_GraphCandidate.asm.
- batch05/export/Relay_FullSlot6.asm; Radio_CommonSync.asm; Radio_GraphSlot8_DFC30_Union.asm.
- batch12/export/RPC_Authorize.asm.
- Accepted [radio contract](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/von-re-20261104/radio/NATIVE_RADIO_CONTRACT.md>), [world report](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/von-re-20261104/world/REPORT.md>), [relay report](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/von-re-20261104/relay/REPORT.md>), [transport incoming admission](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/von-re-20261104/transport/OUTER_SOCKET_CHAIN_RU.md:57>).

API/source docs are navigation/declared contract; specific body/branch claims above use the accepted ASM. Static mathematical examples and bytes are not measurements of native audio, network delivery or live ownership.
