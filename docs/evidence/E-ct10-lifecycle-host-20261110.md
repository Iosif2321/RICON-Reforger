# LINK_END: ordered lifecycle HOST protocol

2026-11-10. **ACCEPT_SCOPED_HOST_ORDERED_LIFECYCLE_PROTOCOL / HOST_PROTOCOL_ONLY**.
Основание: active Goal continuation, Agora8279; Core8292, независимый Reviewer8293/8294.
Большая Goal M1/M2/M3, 21 раздел и этапы0–8 остаётся active.

## Реализованный шаг

Новый отдельный oracle/temporal_lifecycle_oracle.py использует существующие temporal Snapshot2
validators и CF/CD helpers. Snapshot2, raw Receiver.full_resync, schema1, RCFD/1 и Enforce не менялись.
Это HOST-протокол перед будущим producer/receiver adapter, без реального RPC/wire/голосового действия.

- ACTIVE и END — разные tagged records. LOST/Q0, истечение presentation TTL и physical validity не создают END.
- Owner deliverySeq отделён от linkSeq; gap требует свежего FULL, replay не меняет cache/TTL/identity.
- END сохраняет закрытый ID и старую привязку. Даже более высокий linkSeq не открывает этот ID в том же epoch.
- FULL checkpoint H покрывает assigned/reserved IDs; absent IDs<=H остаются закрытыми после pruning tombstones.
- Независимый complete post-state activeExpected проверяется для FULL и DELTA, включая retained links.
- Проверенные observed epoch/seq/H сохраняются отдельно от payload commit при отказе/gap/cap.
  Invalid authority полностью inert; новая pending epoch не стирает старый committed scope.
- Типы, count/byte budgets и вложенные структуры проверяются до копирования непроверенного ввода.
  Accepted inputs/results/state не имеют mutable aliases.

Контракт: [REQ-temporal-lifecycle.md](../req/REQ-temporal-lifecycle.md).
Нормативный HOST код не является новым physical Core или новым voice backend.

## Независимость и RED → GREEN

Core написал contract и staging implementation. Reviewer заморозил30 ручных LC traces
(4c67d051…) и35 прямых tests (1dc9b093…) до чтения implementation.
Встречные LC31–LC39 и их происхождение явно записаны в назначенном review/CONTRACT_COUNTERS_RU.md.
Reviewer никогда не менял implementation; Core не менял reviewer tests.

Независимые проверки выявили:
1. Четыре первоначальных RED на INVALID_AUTHORITY: менялся needsFull вместо inert отказа.
   Исходные четыре ожидания сохранены; исправлен code.
2. Отвергнутый по cap END3/H3 забывался: FULL/H2 мог затем позволить ACTIVE3.
   Добавлены authenticated observed H и deliverySeq; recovery FULL обязан быть после наблюдённой transaction.
3. Bad ACTIVE descriptor в initial/new-epoch FULL мешал удержать pending observations:
   authorization/END-receipt prepass отделён от ACTIVE model domains.
4. Raw deepcopy до caps/validation вызывал RecursionError. Root реально воспроизвёл это на source028e:
   quality graph depth1600 и oversized9items/cap8. Код читает raw input без мутаций и копирует
   только прошедшие проверку значения; limits проверяются до constructor copy.
5. Внутренне foreign/miskeyed authority map могла изменить frontier до отказа.
   Map key/link, epoch, owner и Expected endpoint/binding pins теперь проверяются до observation.

Отдельные API уточнения не скрыты: после observed gap старый FULL/DELTA становится STALE;
H/seq observations — control evidence, не payload commit. Валидный complete authority set,
превышающий локальный cap, даёт RESOURCE_LIMIT, а не INVALID_AUTHORITY.
Правдивость и происхождение самого current-authority input остаются предпосылкой, не результатом тестов.

## Наблюдаемые проверки

| Проверка | Результат |
|---|---|
| Независимый Reviewer run assigned test_temporal_lifecycle.py | 43/43 methods PASS, 0 errors |
| Root повторный run assigned tests | 43/43 PASS |
| В одном meta-test13 заведомо небезопасных изменений | 13/13 обнаружены assertion failure; incidental errors0 |
| Canonical combined temporal lifecycle + snapshot suite | 77/77 PASS = 43 новых +34 прежних |
| Canonical module/test vs author bytes | Совпали |
| git diff --check и видимость новых файлов в Git | PASS |
| Enforce lifecycle protocol / actual RPC/exporter/dispatcher/audio | NOT_RUN |

Команда из canonical корня:
`python -X utf8 -B -m unittest discover -s oracle -p 'test_temporal*oracle.py'`

13 mutants входят в один из43 методов; их не прибавляем к количеству top-level tests.
Проверка77 HOST методов не является77 игровыми/звуковыми сценариями.
Нормативные правила лежат в отдельном REQ; ни code line count, ни document
presence не считаются приёмкой настоящего server authority.

## Сохранённые версии и точные artifacts

| Артефакт | SHA-256 |
|---|---|
| oracle/temporal_lifecycle_oracle.py | d2c0d648b9a986213191a8b3c83bf1e181c71ad48b774cbdffe5f222d678d3e7 |
| oracle/test_temporal_lifecycle_oracle.py | 4c5db4203a4e62200ba0916b7f5e02d89437709922535c494c062402a83636ff |
| oracle/temporal_snapshot_oracle.py — неизменен | 417cd7a561d1d38e3e7c81561bdd7da5634ab05956167d01243148ce696fdb5e |
| oracle/snapshot_oracle.py — неизменен | 98daf7ca987f00264a63b11547bcf0d07d93daa058f4557c60b7c92cb6aafe95 |
| oracle/vectors/snapshot_v1.json — неизменен | 3440982cea0cce21ef857e09a1db3dbe77a5c85561771212dd9ea1199e93721c |

Core author contract128d5cf2… опубликован с root status/provenance overlay, без изменения поведения.
Полная квитанция: [ct10-lifecycle-host-result-20261110.json](receipts/ct10-lifecycle-host-result-20261110.json).
Подробные author/test provenance outputs — соседний Cellular System/output/ct10-lifecycle-20261110.

## Следующая реальная интеграция

Documentation проверила реальные Lifecycle/player/device/lease/manager/S3b sources:
AUTHORITY_FLOW_RU.md (9a2f26e4…),10 joins,3 receipt-группы,6 source pins,35 ссылок.
Lease/effect/PairResult/old registry не содержат полного per-listener binding и RF context.
Old binding/fanout нужен до любого public Lifecycle event: BeginEvent уже sweep/revoke.
Возможный pre-revoke hook — до TerminateLease:1013; готового exporter/hook сейчас нет.
Смешанные old/new epoch effects нельзя stamp одним final manager epoch.
Reserved-ID frontier после source failure нельзя восстанавливать из старого input registry.

Нужны typed Enforce port этого ordered protocol и actual authoritative coordinator/exporter:
capture accepted binding до revoke, source-authenticated current Expected/END receipts,
атомарный FULL checkpoint и согласованный manager/lifecycle epoch; затем dispatcher и wire.
Truthful current-authority/retired-epoch guarantee, реальная RPC origin/serialization/ACK/liveness,
persistent medium continuity, CF/RX/world calibration, compile/runtime/JIP/load и content/DSP/audio OPEN.
При неоднозначной source authority не изобретаем DEFAULT/Q/LOST/mute.

Agy.port предыдущего source-audit: получен ack8270; print observer34168 завершился timeout со
still-in-progress. Дальнейшее состояние backend и итоговый verdict не подтверждены; повторного job
не было. Этот результат независимо принят Core/Reviewer; Agy дополнительная source-приёмка не приписывается.
