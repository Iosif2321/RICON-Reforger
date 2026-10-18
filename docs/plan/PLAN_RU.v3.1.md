# RICON на Arma Reforger — поэтапный план (Stage 2, проектирование)

Версия: **v3.1 — финальная** (раунды 1–15, 2026-10-18). Тема agora: `ricon-reforger-plan`.
Авторы: claude + codex, консенсус в шине. Документ собирает claude, codex сверяет с шиной.
Ссылки вида `id123` — сообщения в шине agora.

---

## 1. Кратко для владельца

- **Ваше решение (2026-10-18):** голос — только встроенный VoN Arma Reforger, изменённый модом.
  Без TeamSpeak, внешних программ и плагинов (id149/150). Внешний голос в этом плане не предлагается
  ни как запасной вариант. Пересмотреть это можно только новым Intent (id286).
- **Что переносим.** Поведение TeamSpeak-ветки A3: кто кого слышит, радио, PTT, прямой голос, HDSP,
  маршруты. Саму TS-реализацию не переносим: плагин, pipe, extension (id153).
- **Как это устроено.** Сервер Reforger на Enforce Script считает всё: модель сигнала HDSP, маршруты,
  рации, ретрансляторы, вышки. Голос переносит движок. Мы управляем им через параметры раций (частота,
  ключ сети, дальность, mute, питание) и через звуковые ассеты VoN.
- **Главный риск.** В публичном API VoN нет громкости и эффектов на каждого говорящего, а прямую речь
  движок выборочно не глушит (§2). Модель будет точно решать, **доходит ли** голос. **Как звучит** плохая
  связь — зависит от результата этапа Э1. Если что-то обязательное сделать нельзя, план не выдаёт это за
  готовность: требование получает статус PARTIAL или BLOCKED, а решение остаётся за вами.
- **Первая точка решения — после Э1.** Цена проверки — минимум 22–37 инженер-дней (Э0+Э1). После неё
  программа идёт одним из путей: GO / LIMITED (вы подписываете отклонение или стоп) / NO_GO (стоп) /
  BLOCKED (пауза) — §2.3.
- **Релизы по частям:**
  - **M1 «Радио»** — прямой голос и носимые рации, полный рабочий цикл;
  - **M2 «Инфраструктура»** — техника, стационарные станции, интерком, ретрансляторы;
  - **M3 «Сотовая сеть»**.
  M3 обязателен для конечного Intent.
- **Объём:** 177–292 инженер-дня всего (≈ 35–58 недель одного разработчика). До M1 — ориентировочно
  87–147 дней по критическому пути. Это диапазоны до результатов Э1, не обещание срока.
- **Что нужно от вас:** §9.

## 2. Голосовой транспорт

### 2.1 Решение и критерий
Единственный транспорт — встроенный VoN + мод. Э1 не выбирает транспорт, а измеряет границы VoN.
Результат Э1 — таблица ADR-001: `требование → API/техника мода → runtime-доказательство → ограничение → вердикт`
(id153, id170).

Release-свойства (id119, id149, id155, id172). У каждого — статус `PASS / PARTIAL / FAIL / NOT_TESTED`
с доказательством:
1. сервер решает, кому доставлен голос, и это обеспечивает движок, а не честность клиента;
2. громкость и эффекты на пару говорящий×слушатель;
3. частоты и каналы;
4. слышимая деградация по качеству связи.

**Статусы приёмки функциональных этапов (id235, id241):**
- `CONTROL_PLANE_PASS` — верны маршруты, устройства, состояния;
- `AUDIO_PASS` — запись и прослушивание несколькими людьми подтверждают доставку и деградацию.

PASS этапа = оба статуса. Логи и UI не заменяют звук.

### 2.2 Что известно о VoN

База ссылок: AR = `community.bistudio.com/wikidata/external-data/arma-reforger/ArmaReforgerScriptAPIPublic/`,
EN = `.../EnfusionScriptAPIPublic/`.

| Рычаг | Источник | Что даёт | Статус |
|---|---|---|---|
| `VoNComponent.SetCapture / SetCommMethod / SetTransmitRadio` (один передатчик) | AR/interfaceVoNComponent.html | вкл./выкл. захват, прямая речь/радио, выбор передатчика | DOC |
| `VoNComponent.OnReceive(playerId, isSenderEditor, receiver, frequency, float quality)` | там же | только уведомление о приёме с качеством 0..1 | DOC |
| `VoNComponent.OnVoNUsed(senderId)` (сервер) | там же | сервер знает, кто говорит | DOC |
| `BaseTransceiver.SetFrequency(кГц)`, `SetRange(м)` («only owner (server) has effect»), `SetMuteState` | AR/interfaceBaseTransceiver.html | серверное управление доставкой радио | DOC |
| `BaseRadioComponent.SetEncryptionKey`, `SetTransceiverFrequency` («sets and syncs with server»), `SetPower`, `TransceiversCount/GetTransceiver` | AR/interfaceBaseRadioComponent.html | ключ сети (группа доступа, не криптография), несколько трансиверов | DOC |
| `RelayTransceiver`, `IntercomTransceiver` | AR/annotated.html | классы есть, поведение не проверено | DOC |
| `SCR_VONController` (entries, `SetActiveChannel` через RPC, `SetVONDisabled`, modded class) | AR/interfaceSCR__VONController.html | UI, каналы, PTT | DOC |
| RplRpc / RplProp / RplChannel / RplRcver / RplCondition.OwnerOnly | EN/group__Replication.html | серверные RPC, реплика только владельцу | DOC |
| громкость/эффекты на говорящего | — | методов нет | DOC (отсутствие) |
| звуковые ассеты VON (`Von.sig`, `VON.acp`) | Workshop 69E059166C863941; ASAPsVON | моды меняют звук VoN; динамическая привязка к паре не доказана | MOD |
| выборочное глушение прямой речи | Workshop 6A325870AC5A13E3 | «no engine-level way» | MOD |
| нативные DLL | Lima-Charlie/docs/how-it-works.md:19 | не загружаются | MOD + DOC (отсутствие) |
| стриминг (игрок вне зоны репликации) | Workshop 6A4539AB94F37633 | клиентские эффекты могут не работать | MOD |

### 2.3 Гипотезы Э1 и дерево решений

**Гипотезы:**

| Гипотеза | Что проверяем | Граница / статус |
|---|---|---|
| **H1** групповая доставка радио | сервер выставляет частоту, ключ, дальность, питание | ожидаемо доказуема |
| **H2** mute приёмника (id158, id202, id213) | `SetMuteState` действует на весь RX | точна только при {один активный источник на RX; нет fan-in ретранслятора; сеть сама сериализует PTT}. Busy-channel lease — опциональная политика NetworkType |
| **H3** качество → звук (id158, id182) | динамическая привязка quality к конкретной паре в графе VON | статическая замена ассета не доказательство |
| **H4** подбор `SetRange` (id158, id186) | один скаляр на TX | только радиальная аппроксимация, ошибка по ≥3 слушателям |
| **H5** прямой голос (id158, id180) | выборочная доставка или модификация | если нельзя ни то ни другое — `BLOCKED_INTENT_GAP` |
| **H6** пул скрытых приёмников (id196, id220, id230, id236) | delivery virtualization experiment | кандидат для вызова и интеркома; для ретранслятора отвергнут: рост TX range даёт утечку обычным слушателям частоты |

**Дерево решений после Э1 (id276, id286):**

| Исход | Что дальше |
|---|---|
| `GO` | freeze Э2 → Э3/Э4/Э4T → M1 |
| `LIMITED` | для каждого отсутствующего поведения владелец подписывает датированный `APPROVED_SCOPE_DEVIATION` (требование остаётся PARTIAL/WAIVED), либо STOP |
| `NO_GO_H1` (движок не даёт надёжной серверной доставки) | STOP до Э3/Э4T/Э5; сохранить evidence, ADR, golden, контракты. Демонстратор «HDSP + UI + ванильный VoN» — только отдельным scope, не RICON full и не M1 |
| `BLOCKED_UNVERIFIED` (нет Tools или 3 клиентов) | пауза, без выбора архитектуры |

### 2.4 Отменено
TS-Full, VoN-Lite как отдельный режим, sidecar, whisper, файловый мост `$profile:`, RestApi к localhost,
установщик TS3/TS6, порт `tsplugin/` и `extension/`, общий сменный `VoiceTransport` (id149, id150, id153).

## 3. Архитектура

```
Клиент (ввод)          Сервер Reforger (источник истины, Enforce Script)                  Движок VoN
PTT / тюнинг ──RPC──▶ RICON_CommsManagerComponent (компонент GameMode)
 (owner-bound,        ├─ Registries: Session{epoch} / Endpoint(PlayerId) / Device / Antenna / Site
  валидация, clamp,   ├─ CandidateGraph (пространственная сетка вместо O(P²), fn_netTick.sqf:94-122)
  rate limit)         ├─ LinkEvaluator → LinkBudget (wireless: HDSP L0–L3 + Interference + Antenna; wired: seam)
                      ├─ RoutingService (политики на NetworkType: radio-net / relay / cellular; CallSession)
                      ├─ LinkSnapshot{sessionEpoch, linkId, linkSeq, ttl, route, quality}
                      └─ VoNIntegration ──▶ частота / ключ / range / mute / power (только доказанное Э1) ──▶ доставка
Клиент (презентация) ◀── owner-scoped replication (RplCondition.OwnerOnly): индикация качества, UI
```

- **Реестры расширения (id246, id269):**
  - Band → PropagationModel;
  - NetworkType → RoutingPolicy / CallSession (общий автомат вызова, чтобы спутниковый телефон не стал
    копией сотовой сети);
  - Antenna → Pattern;
  - LinkEvaluator — проводная связь подключается как evaluator без `if wired` в ядре.
- **Сервер:** вся логика. Клиентские RPC — только ввод. Параметры раций — из серверного каталога с clamp
  (урок R-05).
- **VoNIntegration:** единственная точка воздействия на голос. Реализует только то, что доказал ADR-001.
- **Жизненный цикл:** состояние по PlayerId, не по entity (R-02). JIP получает текущий снимок. Целые
  epoch+linkSeq (R-01, R-21).
- **Отказы (id186):**
  - истёк серверный PTT/transmit lease → сервер прекращает передачу и возвращает трансивер к каталожным значениям;
  - устарел UI-снимок у клиента → скрыть качество и сделать resync;
  - RX целиком не мьютится (R-06).
- **Граница (id145):** авторитет над маршрутом ≠ enforcement доставки. Enforcement **доказывается в Э1**.
- **HDSP:** только Enforce + LOD/кэш. DLL нет, внешний процесс запрещён.
- **Топология (id230):** голос в RICON и в A3 не шёл «через» узлы. Fidelity = доставка только при валидном
  многоногом маршруте с качеством min(legs). Задержка ретрансляции — потолок VoN. Ретранслятор = доказанный
  `RelayTransceiver` или gap.

### 3.1 Инварианты расширяемости (id256 + сценарии id246)
1. Data: новая рация/антенна/диапазон = validated prefab/config. Клиент не задаёт параметры вне каталога.
2. Algorithm: новая Propagation/InterferenceModel регистрируется сервером и проходит finite/budget/golden
   контракт. Права менять VoN напрямую у неё нет.
3. Network: NetworkType/RoutingPolicy строит маршруты. Голос меняет только VoNIntegration.
4. Lifecycle: у каждого device/site есть stable ID, owner/access policy, epoch/seq/TTL, JIP-сериализатор
   и cleanup.
5. Evidence: новая модель или сеть добавляет векторы, adversarial RPC-тесты, perf-профиль и live-сценарий.
6. Compatibility: semver схем и API. ADR-001 привязан к версии Reforger; на неподдерживаемой версии —
   явный статус.

Contract-тесты Э2 (сценарии id246):
- (1) новая УКВ-рация конфигом;
- (2) HF с sky-wave как новая PropagationModel (в A3 только земная волна — `fn_signalCompute.sqf:161-163`);
- (3) спутниковая сеть через NetworkType + общий CallSession;
- (4) направленная антенна через Pattern;
- (5) проводной evaluator-заглушка.

## 4. Этапы

### 4.0 Сводка

| Этап | Суть | Зависит от | Объём, дн. |
|---|---|---|---|
| Э0 | Evidence baseline | — | 7–12 |
| Э1 | Capability lab VoN → дерево решений §2.3 | Э0, разрешение владельца | 15–25 |
| Э2 | Контракты (draft от Э0, freeze после GO/LIMITED Э1) | Э0 / Э1 | 10–15 |
| Э3 | Server-authoritative lifecycle | Э2 | 10–15 |
| Э4 | HDSP на Enforce | Э2, бенчмарки Э1 | 15–25 |
| Э4T-s | VoN integration: scaffold/harness на synthetic LinkSnapshot | Э1, Э2 (параллельно Э4) | ≈ половина Э4T |
| Э4T-f | VoN integration: финальный маппинг и паритет | Э4, Э4T-s | ≈ половина Э4T (всего 10–20) |
| Э5 | Вертикальный срез (3 роли) | Э3, Э4T-f | 10–15 |
| Э6 | Носимое SR/LR — **полный рабочий цикл** | Э5 | 15–25 |
| Э7a | Site / Antenna / Power (основа стационаров и вышек) | Э6 | часть Э7 |
| Э7b | Техника, интерком, динамики, ретрансляторы | Э7a (параллельно Э8) | часть Э7 (всего 20–30) |
| Э8 | Сотовая сеть: control plane + доказанная voice realization | Э7a, interference seam Э4 | 30–50 |
| Э9 | SDK (contract-примеры с Э2; публичный SDK к M2), админка, полировка | Э5–Э8 | 10–15 |
| Э10 | Hardening/scale (повторяемый gate каждого milestone) | — | 15–25 |
| Э11 | Поставка (повторяемый gate каждого milestone) | — | 10–20 |
| **Итого** | | | **177–292** |

**Критический путь до M1** (сверено codex, id295):
- Э0+Э1+Э2 = 32–52;
- параллельно max{Э3, Э4+Э4T-f, Э4T-s} = 20–35;
- Э5 = 10–15;
- Э6 = 15–25;
- **scoped Э10+Э11 для M1 = 10–20** (подбюджет из полных 25–45).

Итого ≈ **87–147 дн.** Если включить полные Э10+Э11, получится 102–172 дн.
Э3 и Э4T-s идут параллельно Э4. Пока нет разрешения на Tools, параллельно идут Э0, черновик Э2 и golden Э4.
Эти затраты учитываются отдельно (id286). Ghidra в план не входит (id129, id143).

### 4.1 Milestones (id244, id266, id272)

| Milestone | Состав |
|---|---|
| **M1 «Радио»** | Э0–Э6 + scoped Э10 (2→20+ игроков, безопасность носимого радио) + scoped Э11 (Workshop, доки) |
| **M2 «Инфраструктура»** | Э7a/Э7b + публичный SDK (Э9) + дельта Э10/Э11 |
| **M3 «Сотовая сеть»** | Э8 + дельта Э10/Э11 |

Каждый milestone проходит:
- установку только через Workshop на чистый клиент;
- rollback;
- security/perf;
- multi-human `AUDIO_PASS`.

M1 с непринятым gap не называется «полной платформой». M3 обязателен для конечного Intent.

### 4.2 Сквозные правила
- UX, клавиши и индикация входят в DoD функционального этапа.
- Gate «dedicated + JIP + respawn + reconnect».
- Бюджет Э2 на synthetic profile.
- `CONTROL_PLANE_PASS` + `AUDIO_PASS`. Без hook VoN — `LIMITED` с датированным `APPROVED_SCOPE_DEVIATION`
  или `BLOCKED_INTENT_GAP`.
- Доказательства по уровням раздельно: STATIC / BUILD / DS-smoke / synthetic / live-human.
- Серверный журнал «ожидалось» и клиентский «наблюдалось» связаны correlation id.
- Обновление Reforger → повтор набора тестов Э1.
- Clean-room: CVON и LC — только идеи; TFAR (APL-SA) — не копировать; ACRE2 (GPL-3.0) — по политике
  NOTICE тоже только идеи.

### Э0 — Evidence baseline (id195, id206, id221)
- **Мини-intent.**
  - *Проблема:* 77 функций A3 (`config.cpp:20-124`); WIKI расходится с кодом (`WIKI_RU.md:926`);
    риски происхождения кода (`fn_signalCompute.sqf:101,171` — ссылки на ACRE2); у CVON и LC нет лицензии.
  - *Результат:* переносимая семантика отделена от механизмов.
  - *Ограничения:* clean-room, GPLv3.
  - *Открыто:* GPLv3 и условия Workshop.
- **Работы:**
  - каталог → требования поведения;
  - матрицы §5a/§5b;
  - реестр R;
  - SPDX/provenance;
  - HDSP-формулы, выведенные заново по первоисточникам (по официальным текстам);
  - golden-векторы (pure-math, аудио-метрики) с хэшами;
  - target-репозиторий из clean commit, `.gitattributes`, CI на всех ветках, pin (R-15, R-35, R-36, R-37).
- **DoD:**
  - CI-проверка §5a: set(CfgFunctions) == set(matrix), 77, дубликатов 0;
  - у каждого R-ID есть этап и тест;
  - нет файлов без статуса лицензии;
  - golden воспроизводимы и покрывают граничные, номинальные и патологические случаи каждого слоя;
  - Discord и TS в target не попали.
- **Риски:** Workshop vs GPLv3.

### Э1 — Capability lab VoN (id164, id170, id172, id182, id191, id204, id286)
- **Мини-intent.**
  - *Проблема:* публичный API не доказывает доставку и DSP на пару.
  - *Результат:* ADR-001, spike-аддон, исход по §2.3.
  - *Ограничения:* только мод, без DLL/TS/sidecar/Ghidra.
- **Работы:**
  1. зафиксировать версии; роли A, B, C; журналы «ожидалось»/«наблюдалось»;
  2. базовая матрица direct/radio;
  3. H1, включая попытки клиента изменить параметры;
  4. H2 как falsifier: A→B LOST и C→B CLEAN одновременно;
  5. H3: динамическая привязка;
  6. H4: кривая quality(d, range) по ≥3 слушателям;
  7. H5;
  8. H6: K, источник, enforcement, утечки;
  9. Relay- и IntercomTransceiver;
  10. одновременный PTT, коллизии, опциональный busy-channel lease;
  11. Rpl/стриминг;
  12. Workshop/BattlEye, официальные правила платформ (консоли);
  13. микро-бенчмарки на DS (median/p95/max): 10^4 FSPL; рельеф и трассировка на профилях 0.1/1/5/15 км;
      Rpl-снимок на 60 synthetic endpoints;
  14. инвентарь клиентов/ПК.
- **DoD:**
  - статусы 4 свойств и H1–H6 с доказательствами;
  - отрицательные результаты воспроизводимы;
  - security-кейсы: подмена частоты/range/mute/PTT, чужой владелец, stale/replay, приём запрещённого
    говорящего;
  - ADR-001 проверен обоими агентами;
  - исход по дереву §2.3.
- **Риски:** §7 R1–R4, R6, R15, R17. Смягчение: сначала falsifiers H2/H3/H5.

### Э2 — Контракты ядра (id195, id206, id269)
- **Работы:**
  - схемы `Endpoint/Device/Antenna/Band/Channel/Network/Site/LinkQuery/LinkBudget/LinkSnapshot`;
  - правила для целых epoch/seq/TTL/version;
  - LinkEvaluator, PropagationModel, InterferenceModel, RoutingPolicy, CallSession, VoNIntegration;
  - валидация и clamp конфигов;
  - threat model RPC;
  - Rpl-контракт;
  - бюджеты из Э1.
- **DoD:**
  - contract-тесты malformed/stale/overflow/NaN;
  - 5 сценариев расширения §3.1;
  - mock сотовой NetworkType;
  - у каждого поля есть владелец и источник;
  - нет VoiceTransport.

### Э3 — Lifecycle (id174, id187, id194)
- **Работы:**
  - `RICON_CommsManagerComponent`: OnPlayerConnected/Disconnected — идея из `CVON_VONGamemodeComponent.c:102-114`;
  - `RICON_PlayerCommsComponent`: `[RplRpc(RplChannel.Reliable, RplRcver.Server)]` — идея из
    `LC_PlayerController.c:20`; проверка владельца, каталога, rate limit;
  - снимок владельцу;
  - серверный PTT lease: TTL, порядок по серверному времени (R-29; не глобальная монополия частоты — id202);
  - respawn: пересвязка; disconnect: отзыв lease (актуатор по ADR-001);
  - серверные устройства на земле и в технике.
- **DoD** (пороги = бюджет Э2):
  - respawn ×3, disconnect посреди PTT, JIP, смена entity при том же PlayerId;
  - поддельные RPC отклонены;
  - подмена снимка клиентом не меняет доставку.

### Э4 — HDSP на Enforce (id165, id184, id192)
- **Порт** из `fn_signalCompute.sqf:72-360` / `signal_core.cpp:133-227`:
  - шумовой пол;
  - FSPL с d≥λ;
  - дифракция Fresnel–Kirchhoff + Epstein–Peterson;
  - потери на объектах по классам (вынести в конфиг);
  - погода с ФНЧ, HF-земная волна;
  - многолучёвость: прямой луч, земля, фасад — с переписанным происхождением;
  - shadowing 50 м; фейдинг K→Nakagami m;
  - `intel = snrLin/(snrLin+10)`, где `snrLin = 10^(SNRdB/10)` → 0.5 при 10 dB;
  - состояния lost/weak/degraded/clean.
- **Новое:** реестр моделей; InterferenceModel seam (полная модель — в Э8).
- **LOD:**
  - L0 — все кандидаты, по событию;
  - L1 — объекты и погода;
  - L2 — рельеф/дифракция, кэш (ячейка tx 50 м, ячейка rx 50 м, диапазон), шаг 7.5–25 м;
  - L3 — параметры фейдинга;
  - приоритет «говорящие сейчас»; тёплые маршруты (R-11).
- **DoD:**
  - golden pure-math + scene fixtures, ±0.5 dB по каждому слою;
  - isfinite;
  - ограниченный кэш;
  - бюджет на synthetic profile.

### Э4T — VoN integration (id166, id186, id193, id250)
- **Э4T-s:** harness, synthetic LinkSnapshot, W1 `RICON_DeviceComponent` ↔ трансивер (H1), отказы §3.
- **Э4T-f:**
  - W2 mute — только в доказанной области H2;
  - W3 звук от quality — только при H3, иначе статический эффект; range — H4 с ошибкой;
  - W4 прямая речь — H5;
  - W5 интерком/вызов — H6, если PASS.
- **DoD** (по ADR-001):
  - несовместимая частота/ключ → тишина;
  - lost-маршрут не доставлен в доказанной области;
  - три уровня качества различимы (если H3 GO);
  - ванильная рация работает ванильно;
  - истёкший lease прекращает передачу.
- **Совместимость:** подход LC — ванильные трансиверы сохраняются.

### Э5 — Вертикальный срез (id197, id208, id222)
- **Состав:** 1 DS + 3 роли A/C→B. Э5a — прямой голос; Э5b — одна SR-рация, 2 канала, PTT. HUD, корреляция логов.
- **DoD:**
  - сценарии: близко/далеко/не тот канал/lost, PTT, JIP/respawn/reconnect;
  - аудиозаписи, `CONTROL_PLANE_PASS` + `AUDIO_PASS`;
  - внешних процессов нет.

### Э6 — Носимое радио (id197, id208, id222)
- **Работы:**
  - инвентарь, несколько раций; пресеты и ручная настройка;
  - BW/модуляция: совместимость `fn_radioFrequencyCompatible.sqf:19-22` и шумовой пол;
  - ключи сети; scan/squelch; политика PTT/MAC сети;
  - питание, батарея, повреждение;
  - UI, клавиши, доступность.
- **DoD:**
  - ≥2 SR и ≥1 LR — только конфигами;
  - ≥3 клиента; пересекающиеся сети; одновременный PTT (слышимая интерференция — только при H3);
  - выбросить/поднять рацию; неверная настройка; смерть/JIP;
  - тест первого слова (R-11);
  - побеждает серверный каталог.
- **Закрытие Э6 = «полный рабочий цикл» Intent §Problem.**

### Э7 — Инфраструктура (id198, id216, id224, id230, id250)
- **Э7a:** Site/Antenna/Power — мачты, генераторы, повреждения, диаграммы антенн. Открывает Э8.
- **Э7b:** стойки и сиденья, права доступа; интерком (IntercomTransceiver или H6); динамики; ретранслятор
  (`RelayTransceiver` или `BLOCKED_INTENT_GAP`); циклы проверяются **в графе маршрутов** (TTL пути).
  Аудио-петель и задержек нет.
- **DoD:**
  - сесть/выйти; двухногий бюджет земля↔техника↔ретранслятор;
  - уничтожение питания/антенны;
  - граф без циклов; два одновременных входа; JIP.

### Э8 — Сотовая сеть (id198, id224, id231)
- **Control plane:** сайты/секторы/соты, attach, paging, автомат CallSession, номера и авторизация,
  admission/capacity, хендовер с гистерезисом (идея A3 — `fn_selectBestNode.sqf:4-15`: 3 dB / hold 2 с,
  после теста), SINR, карта покрытия, телефон и UI.
- **Voice realization:** только по ADR-001 (кандидат H6: уникальная частота+ключ вызова, отдельный
  передатчик телефона; нельзя одновременно говорить в телефон и рацию). Качество = min(uplink, downlink).
- **SMS / данные / биллинг:** seam, блок до ответа владельца.
- **DoD:**
  - attach / нет сети; вызов полностью;
  - хендовер без дублей, ping-pong;
  - отказ вышки → handover/re-attach при соседней соте, обрыв только без маршрута или по таймауту;
  - privacy, чужой вызов отклонён; одновременные вызовы, исчерпание пула; JIP.

### Э9 — SDK, админка, полировка (id199, id210, id225)
- **Работы:** документированные префабы/конфиги/события; примеры; политика совместимости;
  админ-диагностика («почему меня не слышно» по correlation id); локализация.
- **DoD:**
  - независимый мододел по инструкции делает sample-аддон за ≤1 день;
  - UI не раскрывает чужие скрытые ссылки.

### Э10 — Hardening/scale (id199, id210, id225)
- **Работы:** профили 2/20/40/60+ (цель — по ответу владельца); 4 ч soak; штормы JIP/respawn; лаг/потери;
  fuzz; подмена владельца/устройства/мощности/частоты/PTT; replay; обход доставки; вытеснение из кэша.
  Боты — только после проверки правил.
- **DoD:**
  - бюджеты на названном железе;
  - нет неограниченного состояния;
  - злонамеренный клиент не меняет доставку;
  - нет залипаний;
  - security review обоими агентами (R-04/R-05).

### Э11 — Поставка (id199, id210, id225)
- **Работы:** Workshop; пресеты сервера; semver; CI (unit / contract / golden / package / DS-smoke);
  подписи/BattlEye; манифест и хэши; LICENSE/NOTICE; миграция WIKI (R-38); rollback; поэтапные живые сессии.
- **DoD:**
  - clean commit → артефакт с манифестом (R-17);
  - установка только через Workshop;
  - приёмка 2 клиента → сессия сообщества;
  - документация;
  - уровни доказательств разделены.

## 5. Матрицы функций

### 5a. Функции A3 (77/77, id252, id265, id280)
Решения: **PORT_BEHAVIOR** (переносим поведение), **REDESIGN** (переделываем), **DROP** / **DROP_BY_OWNER**
(отбрасываем), **ENGINE** (делает движок), **VERIFY** (проверить в Э1).

| # | Функции | Поведение A3 | Решение | Цель на Reforger | Этап | Тест |
|---|---|---|---|---|---|---|
| 1 | `log` | логирование | REDESIGN | correlation-логгер | Э3/Э9 | корреляция expected/observed |
| 2 | `pluginProvision` | установка DLL плагина TS | DROP_BY_OWNER | — | — | отсутствие в target |
| 3 | `initSettings` | CBA-настройки, клавиши | REDESIGN | настройки сервера, клавиши Reforger | Э2/Э9 | UI/конфиг-тесты |
| 4–7 | `skeletonInit`, `skeletonTick`, `skeletonRequestState`, `setEligible` | boot, walking skeleton | REDESIGN (boot-функция → lifecycle GameMode) / DROP (остальные три) | CommsManager | Э3 | lifecycle gate |
| 8–16 | `graphInit`, `genId`, `registerNode`, `registerEndpoint`, `registerLink`, `selectBestNode`, `routeResolve`, `routeTick`, `localizeEndpoint` | граф узлов, лучший узел, «сотовый» маршрут | REDESIGN | реестры, Routing, Э8 (3 dB / hold — после теста; локализация — опционально) | Э3/Э8 | хендовер/attach |
| 17–21 | `signalCompute`, `signalDiag`, `signalCompare`, `terrainH`, `weatherState` | HDSP | PORT_BEHAVIOR (clean-room) | PropagationService | Э0/Э4 | golden |
| 22–23 | `applyChannel`, `channelTest` | применение канала / тест | REDESIGN | VoNIntegration, self-test | Э1/Э4T | lab |
| 24–36 | `gameSessionApply`, `applyDiscordVoice`, `gameCoordinatorInit`, `gameSessionCreate`, `gameSessionClose`, `gameIdentityTick`, `gameSessionRequest`, `gameIdentityReport`, `gameRosterApply`, `gameRosterRefresh`, `gameDiscordPlayersUpdate`, `gameLeaseApply`, `gamePolicyEmit` | Discord | DROP | — | — | отсутствие |
| 37–38 | `identityInit`, `registerIdentity` | TS client id ↔ игрок | REDESIGN | PlayerId-реестр | Э3 | respawn/JIP |
| 39–42 | `netCreate`, `netJoin`, `netLeave`, `netList` | debug-сети | REDESIGN | admin/test NetworkType | Э9 | админ-тест |
| 43–48 | `netTick`, `applyTalkerChannel`, `clearTalkerChannel`, `routeApplyAck`, `routeFullResync`, `routeExpireStale` | O(P²) резолвер, доставка, ACK | REDESIGN | CandidateGraph + LinkSnapshot + Rpl epoch/TTL | Э2–Э4T | stale/JIP/скорость |
| 49 | `directRoute` | прямая речь по дистанции | REDESIGN / VERIFY | H5 | Э1/Э5 | Э5a |
| 50–60 | `radioRoute`, `radioFrequencyCompatible`, `radioBuildChannelState`, `radioRegisterDevice`, `radioSetFrequency`, `radioBeginTransmission`, `radioEndTransmission`, `radioInit`, `pttStart`, `pttStop`, `setPttState` | радио, PTT | PORT_BEHAVIOR | компоненты, префабы, серверный PTT lease | Э3/Э6 | Э5b/Э6 |
| 61–62 | `applyLipState`, `lipTick` | губы | ENGINE / VERIFY | движок VoN; своё — только при gap | Э1 | lab |
| 63–68 | `tsChannelApply`, `tsChannelRelease`, `tsChannelStatus`, `tsChannelEnsure`, `tsPluginDiagnosis`, `tsPluginMonitor` | TS-канал | DROP_BY_OWNER | — | — | отсутствие |
| 69–70 | `vadTick`, `setTalking` | VAD через TS | REDESIGN / ENGINE | OnVoNUsed + PTT-события + lease | Э1/Э3 | PTT-тест |
| 71–73 | `pingTick`, `pingEcho`, `pingResult` | пинг | REDESIGN | телеметрия задержек | Э9/Э10 | перф |
| 74–77 | `selfTest`, `audioProof`, `reportMembership`, `showServerReply` | диагностика | REDESIGN | contract/golden/live evidence, админка | Э1/Э9/Э10 | — |

### 5b. Возможности: RICON A3 / TFAR / ACRE2 / CVON / LC (id243, id279, id280)

Источники:
- A3 — `notes_ricon.md`;
- TFAR — `task-force-arma-3-radio/addons/*`, лицензия **APL-SA** (`LICENSE.md`): с GPLv3 несовместима;
- ACRE2 — `acre2/addons/sys_*`, лицензия **GPL-3.0**: по политике NOTICE берём только идеи;
- CVON / LC — `notes_refs.md`, **без лицензии**: только идеи.

Authority locus референсов TFAR/ACRE2/CVON-звук/LC — клиент (TS-плагин), если не указано иное.

| # | Возможность | A3 | TFAR | ACRE2 | CVON | LC | Authority у референсов | Вердикт | Этап / gate |
|---|---|---|---|---|---|---|---|---|---|
| 1 | Сервер решает, кто слышит | `fn_netTick` (сервер, gate у слушателя) | нет | нет | сервер выбирает получателей, данные частично от клиента | нет (слушатель) | client / mixed | REDESIGN (сервер + движок) | Э3/Э4T, R-05 |
| 2 | Per-pair качество звука | `audio_chain.c` | плагин | плагин | плагин | плагин | listener client | REDESIGN, только поведение; WAIVED_IF_OWNER при H3 FAIL | Э4T / H3 |
| 3 | Прямой голос | дистанция (`fn_directRoute`) | есть | есть | есть + окклюзия | есть + room-модель | client | REDESIGN / VERIFY H5 | Э5a |
| 4 | Уровни голоса (шёпот/крик) | нет | есть | есть | есть | есть | client | NEW, VERIFY H5 | Э5/Э6 |
| 5 | Окклюзия прямой речи | нет | частично | есть | `TraceMove` | трассы + room | client | NEW, VERIFY H5 | Э5 |
| 6 | SR/LR рации, частоты, шаг, BW | 3 вирт. устройства | handhelds, backpacks | sys_prc* | своя модель | ванильные трансиверы | client / engine (LC) | PORT_BEHAVIOR → префабы | Э6 |
| 7 | PTT | без lease | есть | есть | есть | есть | client | REDESIGN, серверный lease | Э3/Э6, R-29 |
| 8 | Модель распространения | HDSP (полная) | упрощённая | sys_signal, signalmap | только дистанция | рельеф (порт TFAR) | client | PORT_BEHAVIOR (clean-room) | Э4 |
| 9 | Антенны | частично | antennas | sys_antenna | — | — | client | NEW (Pattern) | Э4/Э7a |
| 10 | Рации в технике / стойки | нет | есть | sys_rack | есть | — | client | NEW | Э7b |
| 11 | Интерком | нет | external_intercom | sys_intercom | есть | — | client | NEW (IntercomTransceiver/H6) | Э7b |
| 12 | Стационарные станции | нет | static_radios | есть | — | — | client | NEW | Э7a |
| 13 | Ретрансляторы | только граф | нет | нет | нет | нет | — | NEW: `RelayTransceiver` или gap | Э7b |
| 14 | Сотовая сеть | граф без звонков | нет | нет | нет | нет | — | NEW | Э8 |
| 15 | Ключ сети (совместимость, не криптография) | нет | частично | — | ключ фракции | ключ движка | client / engine | NEW (`SetEncryptionKey`) | Э6 |
| 16 | Батарея / повреждение | поля есть | — | частично | — | — | — | PORT_BEHAVIOR | Э6/Э7 |
| 17 | Динамики / громкоговоритель | нет | есть | есть | есть | — | client | NEW, по ADR | Э7b |
| 18 | GUI раций | нет | есть | sys_gui | есть | надстройка над ванильным radial | client UI | NEW | Э6 |
| 19 | AI слышит радио | нет | ai_hearing | — | — | есть | — | DEFER (нет в Intent), seam | — |
| 20 | Zeus / спектатор | нет | частично | sys_zeus, sys_spectator | — | — | — | DEFER, seam | Э9+ |
| 21 | TS-канал / provisioning / pipe / extension | есть | есть | есть | есть | есть | — | DROP_BY_OWNER | — |
| 22 | Debug-сети / self-test / audioProof | есть | — | — | — | — | — | REDESIGN → админка | Э9 |
| 23 | Анимация губ | `fn_lipTick` | есть | есть | — | — | — | ENGINE / VERIFY Э1 | Э1 |
| 24 | Проводная связь | ACREWT | — | — | — | — | — | SEAM (LinkEvaluator) | Э2 |
| 25 | Приватность и ёмкость сот | — | — | — | — | — | — | NEW | Э8/Э10 |
| 26 | Хендовер | `selectBestNode` (идея) | — | — | — | — | — | NEW | Э8 |
| 27 | Modder API / SDK | — | API | api | — | — | — | NEW | Э2/Э9 |

## 6. Судьба R-01…R-40

Категории (id121, id153):
- **drop** — вне рамок;
- **vanish** — исчезает вместе с механизмом;
- **invariant** — урок переезжает как требование;
- **gate** — обязательный критерий этапа.

| R | Суть | Категория | Куда |
|---|---|---|---|
| R-01 | несовместимые seq → вечный mute | vanish + invariant | Э2: целые epoch+linkSeq |
| R-02 | нет respawn/JIP/disconnect | invariant | Э3: PlayerId |
| R-03 | path traversal Activity | drop | — |
| R-04 | server→client RPC без проверки | gate | Э3/Э10 |
| R-05 | клиент глушит сам; параметры от клиента | gate | Э1 (enforcement) / Э3 (каталог+clamp) / Э10; не закрывается H2 автоматически |
| R-06 | strict без TTL | vanish + invariant | Э2/Э4T: TTL/lease, RX не мьютить |
| R-07 | вкладка TS | vanish | — |
| R-08, R-09 | pipe | vanish | — |
| R-10 | Discord-backend | drop | — |
| R-11 | маршрут после VAD | gate | Э4/Э6: тёплые маршруты |
| R-12–R-14 | discord-voice/activity | drop | — |
| R-15 | CI только на main | gate | Э0/Э11 |
| R-16 | лицензии | gate | Э0 |
| R-17 | невоспроизводимые релизы | gate | Э11 |
| R-18–R-20 | extension/IPC | vanish | — |
| R-21 | научная нотация/epoch | invariant | Э2 |
| R-22 | NaN в DSP | vanish + invariant | Э4/Э4T: isfinite |
| R-23, R-24 | версии/ACK | vanish + invariant | Э2: версия схемы |
| R-25 | O(P²), кэши | gate | Э4/Э10 |
| R-26 | CfgRemoteExec | vanish | — |
| R-27, R-28 | плагин | vanish | — |
| R-29 | PTT без lease | gate | Э3: серверный порядок + TTL каждого PTT |
| R-30, R-31 | provision/core_launcher | vanish | — |
| R-32–R-34 | discord-voice | drop | — |
| R-35 | supply chain | invariant | Э0/Э11 |
| R-36 | .gitattributes | invariant | Э0 |
| R-37 | смешанное дерево | invariant | Э0 |
| R-38 | нет единого источника правды | gate | Э11 |
| R-39 | подписи dist | vanish + invariant | Э11 |
| R-40 | Activity | drop | (идея: golden считает сервер) |

## 7. Реестр рисков (id254, id261)
Колонки: P/I — вероятность/влияние (В — высокая, С — средняя, Н — низкая).

| # | Риск | P/I | Триггер | Смягчение | Contingency | Этап | Остаток |
|---|---|---|---|---|---|---|---|
| R1 | VoN не даёт слышимого качества на пару (H3/H4) | В/В | H3 FAIL | falsifier первым | LIMITED → датированное отклонение или STOP | Э1/Э4T | звук беднее HDSP |
| R2 | Прямую речь нельзя выборочно доставлять (H5) | В/В | H5 FAIL | поиск точки в графе VON | BLOCKED_INTENT_GAP → владелец | Э1/Э5 | ванильный proximity |
| R3 | `RelayTransceiver` не работает | С/С | тест 9 Э1 | — | relay = gap, вопрос владельцу | Э1/Э7b | нет ретрансляторов |
| R4 | Нет Tools или <3 клиентов | С/В | нет разрешения | ранний запрос | BLOCKED_UNVERIFIED, пауза | Э1 | — |
| R5 | Производительность Enforce для L2 | С/В | бенчмарки Э1 | LOD, кэш, события | грубее L2 | Э1/Э4/Э10 | ниже детализация |
| R6 | Стриминг / зона репликации | С/С | дальний игрок | тест 11 Э1 | серверные устройства | Э1/Э6 | — |
| R7 | Лицензии референсов | С/В | копирование кода | clean-room, SPDX | переписать | Э0 | — |
| R8 | Конфликт с другими VON-модами | С/С | override ассетов | матрица совместимости | объявить несовместимость | Э4T/Э11 | — |
| R9 | Обновление Reforger меняет VoN | С/В | новая версия | pin, повтор Э1 | откат версии мода | все | — |
| R10 | H6: инъекция/перенастройка клиентом, утечка ключа, один передатчик, рост репликации | С/В | тест 8 Э1 | server-only назначение, ограниченный K | вызов/интерком = gap | Э1/Э8 | — |
| R11 | Приватность и ёмкость сот | С/С | исчерпание пула, гонка хендовера | admission, тесты | ограничение числа вызовов | Э8/Э10 | — |
| R12 | Валидность HDSP: паритет кода ≠ реализм, нет калибровки | С/С | расхождение с ожиданиями | референсные данные, полосы неопределённости | честная маркировка | Э4 | — |
| R13 | Rpl: трафик, JIP-штормы | С/В | перегрузка | бюджеты, backpressure, дельты | реже обновлять | Э3/Э10 | — |
| R14 | Нет приёмки людьми | С/В | только боты/логи | multi-human blind-сценарии | — | Э5/Э11 | — |
| R15 | Ассеты VON или консоли запрещены платформой | С/С | правила BI | проверка официальных правил | ограничить платформы | Э1/Э11 | — |
| R16 | Объём 177–292 дн., открытый scope сот | В/С | — | milestones, SMS/данные/биллинг вне scope | отложить M3 (defer, не drop — M3 обязателен для Intent) | владелец | — |
| R17 | Дрейф доказательств после обновлений | С/С | версия Reforger | pin + повтор набора Э1 | — | каждый релиз | — |
| R18 | Расхождение PlayerId ↔ entity при стриминге устройств | С/С | смена entity | тесты Э3 | серверные устройства | Э3 | — |

## 8. Спорное
Технических разногласий между агентами нет (id281, id289). Сняты в ходе спора:

| Вопрос | Как решено | Где |
|---|---|---|
| два режима транспорта | сняли (VoN-only) | id104, id149 |
| sidecar / whisper | сняли | id133, id139, id153 |
| H2 single-talker | сняли | id202, id213 |
| H6 для relay | отвергнута | id230 |
| альтернативный DoD | заменён на CONTROL+AUDIO PASS | id235, id241 |
| ветка «внешний голос» при NO_GO | убрана | id286, id291 |

Остаются только решения владельца (§9).

## 9. Открытые вопросы владельцу `[Intent]`
1. Разрешить установку Arma Reforger + Reforger Tools (Workbench, локальный DS) для Э1?
   Сколько лицензий/ПК доступно для 3 одновременных клиентов? (id90, id136, id204)
2. Сотовая сеть: только RF/соты/attach/голосовой вызов/хендовер или также SMS, данные, биллинг? (id90, id98)
3. Если VoN не даёт слышимой деградации на пару — принимаете приближение как датированное отклонение
   (`APPROVED_SCOPE_DEVIATION`) или STOP? (id160, id268)
4. Если прямую речь нельзя выборочно доставлять или модифицировать (H5) — оставить ванильный proximity
   как отклонение? (id180)
5. Если ретранслятор невозможен без `RelayTransceiver` — допустимо ли отсутствие ретрансляторов? (id224, id230)
6. Целевое число игроков на сервере для бюджета производительности (60 / 100 / 128)? (id247)
7. Нужна ли поддержка консолей (Xbox/PS5)? (id247, id268)
8. Согласны ли на milestones M1 «Радио» / M2 «Инфраструктура» / M3 «Сотовая сеть»? (id244, id266)
9. При исходе Э1 = NO_GO_H1: подтверждаете STOP программы с сохранением артефактов? (id276, id286)
