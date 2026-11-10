# Source-radio RplId: raw context decode → global lookup

2026-11-04. `codex.goal.rpl`. **WITH_RAW_DECODE_AND_GLOBAL_LOOKUP / STATIC_CODE_ONLY.** Build 1.8.0.13, EXE SHA256 `1d221a990fa16c78fe02f685a7ba1330be115ca93cd3b641e67e5d3f1b40d9c9`. Только узкое чтение frozen exports и bounded dumpbin; NoPlay/server/capture/SDK/Ghidra/DLL/patch. Сохранён [receipt.json](receipt.json), 8 небольших ASM listings этой папки. Root callee requests: **0**; decoder GAP для этой гипотезы закрыт.

**Вывод:** в обычном voice RPC поле `VonContext+0x0C` передаётся как часть raw 24B аргумента, без peer-local reference remap/streaming validation. В Send это значение поступает в **общий server replication lookup**. Для известного ID живого server radio проверенная цепочка не требует, чтобы radio был streamed клиенту-отправителю, и не сравнивает radio owner с sender peer. Проверка owner вызывающего VoN node остаётся отдельной и сохраняется. Это кодовое доказательство узкой reference-resolution цепочки, не runtime демонстрация отправки голоса и не утверждение, что любой ID проходит остальные radio gates.

## 1. Raw поле не становится peer-local reference при decode

Frozen `batch02/export/VoN_RPC_Registration.asm`: context descriptor задаёт размер `0x18` по `0x14029EA69`, wire writer `0x14029F7C0` по EA70, wire reader `0x14029F680` по EA7C; conversion/injection bindings `0x14029F810` / `0x14029FF60` по EABD/EAC9. Повторный descriptor Receive имеет ту же пару по EDE6/EDF2. Тип контекста сериализуется **целиком**, а не специальным RplId serializer каждого поля. Эти же frozen exports использованы в [CONTEXT_STAGES_RU.md](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/von-re-20261104/transport/CONTEXT_STAGES_RU.md>) и [OUTER_BOUNDARY_RU.md](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/von-re-20261104/transport/OUTER_BOUNDARY_RU.md>).

| Стадия | Exact инструкции / данные | Почему нет peer-reference remap |
|---|---|---|
| Capture initial source ID | `29F565 CALL 1411C4B30`; `29F56A MOV ECX,[RAX]`; `29F56C MOV [context+C],ECX`. Producer `[1411C4B30,1411C4B94)` получает BaseRplComponent ID из node-info+10 | В context кладётся 32-bit ID; это source TX radio ID, а не RPC calling-node ID |
| Wire write `[29F7C0,29F807)` | Переносит `0x18` bytes intermediate→temp; затем `29F7EE MOV R8D,C0`, `29F7FC CALL 8AEAF0` | Все 192 бита идут одной raw bit operation; нет выделенной обработки +C |
| Wire read `[29F680,29F6E1)` | `29F691 MOV R8D,C0`, temp address RDX=`RSP+20`, `29F69C CALL 8AEAF0`; после успеха `29F6AE MOV R8D,18`, temp→intermediate через `8AE180` или `8ADEF0` | Инициализация temp+C=FFFFFFFF перед чтением не фильтрует полученное значение: успешная raw операция записывает весь 24B блок |
| Raw bit dispatcher `[8AEAF0,8AEB05)` | Направление wrapper+8 выбирает writer8AE6D0 либо reader192690; RDX/R8 не заменяются | Только направление и bit-count, нет peer/node identity |
| Полный bit reader `[192690,192855)` | Bounds/cursor, цикл блоками до32bits, mask/shift и raw copy helper1417EC84A. `1927FD ADD RCX,[savedDestination]`; `192805 CALL copy`; `19280E ADD EDI,4` | Context+C — четвёртая скопированная 32-bit группа, без отдельной ветви/lookup. Нет аргумента peer, таблицы references или вызова validation; единственные calls — raw copy helper |
| Intermediate byte read/write | `[8ADEF0,8ADF3B)` bounds→copy→advance; `[8AE180,8AE1ED)` grow→copy→advance | Копируют заданное число bytes, не интерпретируют поле+C |
| Injection `[29FF60,29FF6E)` | `MOV RDX,R8; MOV R8D,18; JMP 8AE0C0` | 24B снова переносятся целиком |
| Полный raw transfer `[8AE0C0,8AE15B)` | Read branch проверяет `cursor+size<=bufferLength`, копирует в переданный destination (`8AE139 MOV RCX,RBP; 8AE13C CALL copy`), увеличивает cursor. Write branch симметрично копирует после grow | Нет RplId/peer semantics. Отказ bounds не является reference admission |

Переход generic CRpc reconstruction/invoke к реальным native аргументам уже замкнут frozen transport proof: CRpc pointer array → holder adapter29DF20 → Send2A0AA0. В контекстном codec и injection нет последующей смены/remap значения +C; Send читает именно полученный context. Generic incoming owner gate ниже авторизует **RPC calling item**, а не каждый opaque int внутри payload.

## 2. Send разрешает source ID через общую server таблицу

Frozen `batch02/VoN_RadioGraphCaller.asm` перед lookup проверяет mode1/4 и ID!=FFFFFFFF: `2A0CC3..2A0CD9`. Затем **`2A0CDF MOV ECX,[R14+C]; 2A0CE3 CALL 5DD350`**. Source ID не заменён owner ID: ordinary sender PlayerId исправляется отдельно в context+8. Direct template с+C=-1 — другая копия, не источник этого radio lookup.

| Callee | Полный проверенный контракт |
|---|---|
| `[1405DD350,1405DD391)` frozen batch11 `Radio_LookupById.asm` | Отвергает FFFFFFFF; вызывает `1411C6A10(id)`; если entity найден, ищет компонент BaseRadio ID`97C5FB8B`, иначе null. Peer/owner/streaming comparison отсутствует |
| `[1411C6A10,1411C6A64)` frozen batch16 `Radio_IdLookup.asm` | `11C6A1B MOV EDX,ECX`; **`11C6A1D MOV RCX,[global142177B60]`**; `11C6A24 CALL 846FC0`. Проверяет BaseRplComponent type`2B0B17E6`, возвращает его owner entity. Аргумента peer не передаёт |
| `[140846FC0,140846FE2)` [global_lookup_wrapper.asm](global_lookup_wrapper.asm) | `ADD RCX,1F0`; `CALL DAA9B0`; null→null, иначе `MOV RAX,[record]`. Wrapper использует registry внутри global manager |
| Leaf `[140DAA9B0,140DAA9EA)` [global_id_slot_lookup.asm](global_id_slot_lookup.asm) | High2bits ID выбирают одну из4 банков; low30bits сравниваются с count банка, затем читается slot pointer. Null/out-of-bounds→null. При record+2C>0 возвращает record; иначе требует nonnull record[0]. Нет внешних вызовов, owner ID, peer map, connection или streaming bit |

Точный смысл счётчика record+2C не назначен по догадке; обе его ветви проверены и ни одна не обращается к peer. Наличие неподходящего типа, отсутствующего entity/radio или неверного transceiver index всё ещё приводит к штатному отказу. `5DC880` после radio resolution отдельно проверяет local catalog index. Последующий graph/cache/filter процесс не отменяет факт глобального разрешения входного source-radio ID.

## 3. Граница authority/visibility и вывод для carrier design

Frozen incoming admission (transport [OUTER_SOCKET_CHAIN_RU.md:57](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/von-re-20261104/transport/OUTER_SOCKET_CHAIN_RU.md:57>)): `E67F66..E67F71` сравнивает owner **вызывающего node** с source peer; затем `E67FA5` вызывает CRpc, VoN adapter и Send. У отправителя A это его VoN item. Поле source radio внутри raw context не становится этим node, не получает отдельной owner/streaming проверки в изученной цепочке. Следовательно, нельзя перенести защиту calling VoN owner на sourceRadio owner.

Поддерживаемый per-node/per-peer `EnableStreamingConNode` может ограничить видимость carrier клиенту A, но эта visibility сама не является авторизацией raw ID внутри voice context. Если ID server-owned carrier C известен/угадывается и slot жив, изученный lookup не отвергает его из-за отсутствия stream-in у A. Отсутствие автоматического получения ID через штатный UI не равно reference enforcement. Это сохраняет carrier-trust blocker без инструкции по runtime изготовлению пакетов.

Отдельный диапазон/владение carrier, source selection legitimacy, key/power/frequency/graph gates и возможность client mutations проверяются другими owning ролями. Настоящий boundary для этой гипотезы — server authorization source-radio reference против authenticated source и разрешённого RICON route, а не только скрытие ID. Новый supported hook из этого доказательства не возникает.

Проверка: заново вычислен EXE SHA256, совпал с frozen build; wrapper exact `.pdata`, slot leaf прочитан до обоих RET и INT3; bit reader прочитан до RET; raw transfer helpers exact `.pdata` полностью. Результат относится к static code path, engine/native delivery не проверялись. Нового следующего callee для снятия **именно этой** неизвестности нет.
