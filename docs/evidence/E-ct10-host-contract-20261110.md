# CT10: контракт schema 2 и независимые host-тесты

2026-11-10. Результат: **VERIFIED_HOST_CANDIDATE / HOST_CONTRACT_ONLY**.
Основание: поручение владельца о CT10/независимых тестах и общей Goal; Agora #8223–#8224.
Canonical repo: RICON-Reforger. Общая system Goal в thread 01a14fdf-35c1-7491-b741-c20d764973af остаётся active.

## Что реализовано

- Явный отдельный Snapshot2 HOST envelope: обязательный CF descriptor, независимые версии,
  directed binding и trusted ExpectedLink. Существующие snapshot v1 и RCFD/1 не расширялись.
- Публикация canonical descriptor-only изменения при неизменном Q; signed zero не создаёт update.
  Publisher получает готовые IDs/epoch и callback seq, не заменяет S3b pair/lease/ID lifecycle.
- Detached input/output/read copies; строгие версии/типы/контекст, individual и retained-state budgets.
  Вся партия проверяется до callback; поздний дефект не оставляет prefix или частичный registry.
- Отдельные storage/ordering/локальный TTL и sample eligibility. Хранение не требует clock mapping.
  Keepalive не продлевает physical validity; весь estimate +/- error должен лежать внутри окна.
- LOST/Q0 сохраняет temporal data, EMPTY/ZERO/resource/coverage/clock failure различаются.
  Никакой новой команды mute/power-off/доставки этот HOST API не выдаёт.

Использованы существующие CD.normal/Encode/Rebase, CF preflight/Evaluate и legacy receiver ordering/TTL.
Новых зависимостей, voice backend, production codec или второго Core нет.

## Независимость и встречная проверка

Core написал contract draft отдельно от root implementation. Reviewer вручную зафиксировал
28 semantic scenarios и 12 negative controls до чтения будущего implementation.
Затем Reviewer написал прямые unittest, без universal event adapter и без вычисления expected из oracle.
Исходные semantic traces — не executable wire fixtures; прямые тесты связывают записанные значения с API.

Agy реально работал в общей Agora: ack #8225, counter #8230/#8239 по reverse endpoints,
TTL/physical validity, LOST/PTT и несовпадению первоначальных drafts.
Root/Core/Reviewer/Documentation исправили границы до окончательного результата (#8234):
clock gate только при sample; producer transition/reason и full replay policy остаются OPEN;
независимая phase-only change проверяет перенос доверенных данных, не физическую правоту builder.
Core draft bedb8e2a… согласован с canonical row-first first-error (#8251).

Core дополнительно воспроизвёл три boundary дефекта root-модуля: лишние Expected.channel time keys
молча перезаписывались; ordinary callback KeyError/OSError выходили наружу; bool map key совпадал с ID1.
Исправлены strict channel/map keys и контролируемый отказ callback, с независимыми regression.
Потреблённые внешним source seq не объявляются откатившимися: calls честно учитываются,
host output/registry не коммитятся. Root добавил явную причину unsupported full-resync header.
Одна ошибка API-проекции I15 исправлена Reviewer по ранее frozen INVALID_DESCRIPTOR, manual JSON неизменен.

Core: SCOPED_ACCEPT_HOST_CONTRACT_ONLY (#8246), 25 read-only assertions и отдельные budget checks.
Reviewer: ACCEPT_SCOPED_CT10_HOST_CONTRACT_AND_INDEPENDENT_TESTS (#8249/#8250).
Root повторил canonical тесты после публикации, hashes авторских tests/JSON совпали.
Agy не приписывается module acceptance, пока нет отдельного фактического verdict по этой версии.

## Выполненная проверка

Из canonical корня:

| Команда / набор | Наблюдаемый результат |
|---|---|
| python -X utf8 -B -m unittest discover -s oracle -p test_temporal_snapshot_oracle.py | 34/34 PASS |
| Внутри одного из 34 тестов: NC01–NC12, monkeypatch заведомо неверного поведения | 12/12 обнаружены assertion failure; incidental errors 0 |
| test_channel_descriptor_oracle.py / test_channel_fading_oracle.py | 11/11 и 19/19 PASS |
| test_snapshot_oracle.py / test_link_publication_oracle.py / test_von_seam_oracle.py | 8/8, 17/17, 13/13 PASS |
| snapshot_v1 / channel_descriptor_v1 / channel_fading_v1 | 46 + 54 + 71 = 171 прежний vector PASS |
| git diff --check | PASS |

Итого 34 новых top-level unittest и 68 существующих unit-тестов.
12 negative controls входят в один meta-test; 171 corpus vector — отдельный счётчик.
Не складывать это в число runtime/audio сценариев. Generator checks ранее выполнены prerequisite-срезом,
здесь повторно не запускались: generator/Enforce/v1 sources не менялись.

Проверены reverse links, точные фазы rebase, storage без mapping, TTL после physical expiry,
1199 ms+999 us с error0 против error1, нижняя граница, dups без TTL refresh, nested copy,
late-invalid atomic reset, budgets до callback, строгие full headers и ordinary callback failures.

## Pins

| Артефакт | SHA-256 |
|---|---|
| oracle/temporal_snapshot_oracle.py | 417cd7a561d1d38e3e7c81561bdd7da5634ab05956167d01243148ce696fdb5e |
| oracle/test_temporal_snapshot_oracle.py | 3b1c4225201dee15666af0b94215a2c7e4a27f8ef919eacd8de054506aaea04a |
| oracle/vectors/temporal_snapshot_v2.review.json | 293485e812771f24546e54b6936a7d1c7088410fab51f01aae8bf8a5c7c436c0 |
| REQ-temporal-snapshot.md — reviewed content до status-only отметки приёмки | 78cce5bb40fe180477371806567ed64d91c210d64ed0f947f3fe3fb57842c0d8 |

Manual JSON опубликован byte-identical из назначенного Reviewer output; executable tests также.
Полные текущие hashes и команды сохраняются в receipt:
[ct10-host-result.json](receipts/ct10-host-result-20261110.json).
Назначенные author/testplan/coverage outputs лежат в соседнем Cellular System/output/ct10-contract-20261110.

## Что остаётся открытым

I23 новый temporal seam — NOT_RUN; I28 реальный seam/device binding — NOT_RUN.
I09 проверяет callback и host state; operational S3b allocator/epoch/consumed IDs/lifecycle — NOT_RUN.
Не заявляются RPC authentication, actual clock adapter, producer/world/RF calibration,
shared RX, полноценные SERVER/RECEIVER orchestration, outer wire/Rpl/размер сетевого пакета,
согласованная Enforce/generator/fixtures/probes migration, compile/runtime/JIP/load,
доставка исходной речи, supported actuator, DSP/content/audio acceptance.
ApplySnapshot остаётся BLOCKED_NO_ACTUATOR. Game/Workbench/server не запускались.
Полный CT10, M1/M2/M3 и большая Goal не завершены.

Следующий срез: подготовить согласованную migration specification producer/registry/snapshot/copy/
single+full receiver/seam/fixture по принятому HOST кандидату, с явными lifecycle/end-event,
source trust, clocks и outer serialization gates. Не добавлять temporal смысл в v1 скрыто.
