# REQ-von-seam — безопасная заготовка интеграции VoN (Э4T-s)

<!-- RICON_TEMPORAL_VON_AMENDMENT_20261104_BEGIN -->
**Уточнение владельца, 2026-11-04:** [план RECEIVER/SERVER и channel→audio](../plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md) заменяет прежнюю runtime-интерпретацию LOST как автоматического прекращения голоса. Pure stateless seam и её текущий BLOCKED_NO_ACTUATOR остаются без изменения кода; историческая server-authoritative область ниже не означает готовый двурежимный временной контракт. Source authorization, user power/mute и receiver squelch отделены от физического Q/LOST. Новый контракт и все потребители мигрируют согласованно; diagnostic generic LOST gate не является правилом обычного VoN.
<!-- RICON_TEMPORAL_VON_AMENDMENT_20261104_END -->

Версия: **0.2 AGENT-REVIEWED**, 2026-10-19. Автор: codex. Ревью и независимый oracle: claude
(agora #1117, #1125, #1150, #1164); oracle cross-review codex #1172, #1183.

Источник истины: утверждённый Intent, `docs/plan/PLAN_RU.md` v5 §4 Э4T,
`docs/plan/ROADMAP_RU.md` часть 1 п.2, `docs/req/REQ-core-contracts.md` §8, §9.3 и §11,
`docs/req/REQ-lifecycle.md` §5.1, §6.3 и §11. Это requirements-артефакт, а не доказательство поведения VoN.

**Implementation gate:** разрешена только чистая Enforce-заготовка без обращения к `VoNComponent`,
`BaseTransceiver`, Rpl, миру, Play или серверу. Валидный вход MUST завершаться
`BLOCKED_NO_ACTUATOR`; этот результат не является доставкой голоса, runtime- или audio-PASS.

## Мини-intent

- **Проблема:** текущий `RICON_VoNIntegration` принимает snapshot через `void` no-op. Он не показывает, прошёл ли
  snapshot ordering, к какому device/transceiver binding он относится и почему actuator не вызван.
- **Результат:** детерминированная stateless-граница, которая fail closed проверяет accepted snapshot и binding,
  но при любом валидном входе явно сообщает, что actuator отсутствует.
- **Затронуто:** `RICON_LinkSnapshot`, `RICON_VoNIntegration`, будущий `RICON_DeviceComponent`, synthetic oracle и
  Workbench fixture.
- **Ограничения:** сервер остаётся источником истины; seam не владеет registries, snapshot receiver или lease state;
  direct voice не меняется; только встроенный VoN/Workshop; APL-SA/clean-room; без Play, сервера и reverse engineering.
- **Открыто:** фактический component→`BaseTransceiver` handle, верхняя граница индекса трансивера, authority/Rpl,
  mute/range/frequency/key/power, доставка, звук и несколько bindings на talker. Всё это — Э4T-f/runtime.

## 1. Нормативный язык и evidence

`MUST`/`MUST NOT` обязательны; `SHOULD` требует причины отклонения; `MAY` — опция будущей policy.

- `CT` — host oracle или contract fixture.
- `WB` — Workbench compile/parity без Play и сервера.
- `RT` — runtime в игре; сейчас `BLOCKED_NO_RUNTIME`.
- `AUDIO` — слышимый эффект; сейчас `BLOCKED_NO_RUNTIME`.

Э4T-s может получить только `CONTRACT_PASS`, `ORACLE_PASS`, `WORKBENCH_COMPILE_PASS` и `ENFORCE_PARITY`.
Ни один из них не повышает статус до `RT` или `AUDIO`.

## 2. Граница ответственности

1. `RICON_LinkSnapshotReceiver` единолично решает full resync, epoch/schema identity, stale/duplicate seq и TTL.
2. Lifecycle Э3 единолично решает PTT lease, его terminal transition и effect `VON_EXPIRY_INTENDED`.
3. Authority service MUST передать в seam только snapshot с decision `ACCEPT_FIRST`, `ACCEPT_NEWER` или
   `ACCEPT_RESYNC` и server-resolved binding.
4. Seam MUST NOT повторно хранить или вычислять epoch/seq/JIP/resync/TTL/lease state. Он не ограничивает число
   bindings и не владеет route/snapshot/device registries.
5. До Э4T-f seam MUST NOT импортировать или вызывать engine VoN/radio API и MUST NOT менять мир либо файлы.

## 3. Stable decision enum

Имена и integer values закрыты для oracle/Enforce parity:

| Значение | Decision | Смысл |
|---:|---|---|
| 0 | `NULL_INPUT` | отсутствует snapshot или binding |
| 1 | `INVALID_SNAPSHOT` | snapshot нарушает seam-level domain |
| 2 | `DECISION_NOT_ACCEPTED` | upstream decision не разрешает применение |
| 3 | `INVALID_BINDING` | binding нарушает field domain |
| 4 | `IDENTITY_MISMATCH` | binding не относится к snapshot либо expiry triple |
| 5 | `INVALID_EXPIRY` | expiry epoch либо player/device/lease ID вне своего domain |
| 6 | `BLOCKED_NO_ACTUATOR` | вход валиден, но actuator намеренно отсутствует |

`BLOCKED_NO_ACTUATOR` — не успех доставки. Enum намеренно не содержит `OK` в Э4T-s.

## 4. Device/transceiver binding descriptor

Binding MUST содержать только stable значения:

| Поле | Domain | Назначение |
|---|---|---|
| `sessionEpoch` | `1..int.MAX-1` | не позволяет binding прошлого session совпасть по повторному linkId |
| `linkId` | positive int | связь со snapshot; linkId уникален только внутри epoch |
| `talkerPlayerId`, `listenerPlayerId` | positive int, различны | server-resolved стороны; равенство запрещено против self-echo |
| `talkerDeviceId`, `listenerDeviceId` | positive int, различны | stable catalog/device identity; одна рация не является обеими сторонами одного radio link |
| `talkerLeaseId` | positive int, включая `int.MAX` | связывает apply и delayed expiry с конкретной передачей; совпадает с доменом lifecycle Э3 |
| `talkerTransceiverIndex`, `listenerTransceiverIndex` | explicit non-negative int; default sentinel `-1` | индекс стороны явно назван; count проверит будущий component |

Binding не содержит engine handle. Верхняя граница индекса без реального `BaseRadioComponent` неизвестна и MUST
проверяться в Э4T-f до actuator call. Несколько bindings на одного talker допустимы на этой stateless-границе;
capacity и uniqueness принадлежат authority manager/catalog.

`sessionEpoch` MUST быть одним canonical authority epoch, общим для lifecycle и `LinkSnapshot`; независимые локальные
счётчики запрещены. Его ротацию и atomic full resync координирует authority service вне stateless seam.

## 5. `ApplySnapshot` — точный порядок

Вход: `snapshot`, `binding`, `upstreamDecision`. Возвращается ровно один decision, без mutation/effects.

1. Если `snapshot == null` или `binding == null` → `NULL_INPUT`.
2. Если `snapshot.IsValid() == false`, player ID неположителен либо talker equals listener → `INVALID_SNAPSHOT`.
   Это seam-level правило сильнее текущего `REQ-core` §8.3; сам receiver в Э4T-s не меняется.
3. Если `upstreamDecision` не `ACCEPT_FIRST`, `ACCEPT_NEWER` или `ACCEPT_RESYNC` → `DECISION_NOT_ACCEPTED`.
4. Если binding нарушает §4 → `INVALID_BINDING`.
5. Если `sessionEpoch`, `linkId`, `talkerPlayerId` либо `listenerPlayerId` binding не равны snapshot →
   `IDENTITY_MISMATCH`.
6. Иначе → `BLOCKED_NO_ACTUATOR`.

Порядок глобальный. Например, invalid snapshot имеет приоритет над stale decision и malformed binding.
Valid `LOST` snapshot с `quality=0` также возвращает `BLOCKED_NO_ACTUATOR`: прекращение доставки — такой же будущий
actuator effect, как её разрешение.

## 6. `OnTransmitLeaseExpired` — точный порядок

Вход: `(sessionEpoch, playerId, deviceId, leaseId, currentBinding)`. Возвращается ровно один decision, без
mutation/effects. `sessionEpoch` — epoch, в котором lease был выдан, а не epoch после возможной ротации.

1. `currentBinding == null` → `NULL_INPUT`.
2. `sessionEpoch` вне `1..int.MAX-1` либо любой из остальных трёх ID `<= 0` → `INVALID_EXPIRY`.
3. Невалидный binding по §4 → `INVALID_BINDING`.
4. Quartet не равен `(binding.sessionEpoch, binding.talkerPlayerId, binding.talkerDeviceId,
   binding.talkerLeaseId)` →
   `IDENTITY_MISMATCH`.
5. Иначе → `BLOCKED_NO_ACTUATOR`.

Сравнение epoch и leaseId обязательно: запоздалый expiry `(epoch 7, lease 1)` MUST NOT остановить новый
`(epoch 8, lease 1)` того же устройства после сброса allocator. Seam не запоминает current binding; authority service
передаёт его явно.

## 7. Инварианты no-op

- Оба метода MUST быть stateless и детерминированными.
- Любой вызов MUST оставить binding, snapshot и все registries неизменными.
- В файле реализации MUST отсутствовать ссылки на `VoNComponent`, `BaseTransceiver`, `BaseRadioComponent`,
  `RplRpc`, `GetGame`, world/entity lookup, FileIO и actuator callbacks.
- Неизвестный/новый decision MUST fail closed как `DECISION_NOT_ACCEPTED`.
- Никакой default binding, implicit device, transceiver index 0 по умолчанию или fallback на direct voice не допускается.

## 8. Acceptance matrix

| Test ID | Что проверяет | Evidence R16/R17 |
|---|---|---|
| CT-VON-S-001 | null snapshot и null binding; общий приоритет | OR, затем WB |
| CT-VON-S-002 | все snapshot field failures, player IDs и self-link | OR, затем WB |
| CT-VON-S-003 | каждый accepted и каждый rejected upstream decision | OR, затем WB |
| CT-VON-S-004 | каждое поле binding на границах, разные device IDs, два независимых transceiver indices | OR, затем WB |
| CT-VON-S-005 | mismatch epoch/link/talker/listener по одному полю | OR, затем WB |
| CT-VON-S-006 | CLEAN/DEGRADED/WEAK и valid LOST дают только `BLOCKED_NO_ACTUATOR` | OR, затем WB |
| CT-VON-S-007 | invalid expiry IDs и приоритеты | OR, затем WB |
| CT-VON-S-008 | mismatch epoch/player/device/lease; delayed old-epoch lease против current binding | OR, затем WB |
| CT-VON-S-009 | повтор вызова даёт тот же decision и не хранит state | OR, затем WB |
| CT-VON-S-010 | dependency scan подтверждает отсутствие engine/Rpl/file/world API | CT |

## 9. Stopping gate

R16 готов только после независимого oracle по этому документу, взаимного review и source revision в vectors.
R17 добавляет Enforce fixture, Workbench compile и parity. Даже после R17 actual component binding, authority,
доставка и звук остаются `BLOCKED_NO_RUNTIME`; следующий переход к ним требует Э1r/Audio Editor evidence и отдельного
разрешения владельца на запуск игры.
