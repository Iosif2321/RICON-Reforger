# Матрица покрытия вопросов (фаза «Сбор информации»)

Дата: 2026-10-18. Реестр Q1–Q34 — шина `ricon-reforger-dev`, id340. 15 разделов — Codex dossier,
шина `ricon-reforger-plan`, id344.

Статус проверки:
- **самопроверка claude выполнена** (столбец «Проверка»);
- **перекрёстная проверка выполнена обоими агентами**: Claude dossier/аудит — `ricon-reforger-plan`
  id354–361, Codex dossier/аудит — id344/346/362–365, поправки приняты — id366, итог — id367.
  Взаимная проверка не превращает открытые вопросы в закрытые: статусы ниже остаются честными.

Источники:
- **[E1s]** — `E1s-von-api.md`;
- **[OFF]** — `E-offline-api.md`;
- **[W1]** — `E-web1-von-legal.md`;
- **[W2]** — `E-web2-standards-cellular.md`;
- **[CD]** — `CLAUDE_DOSSIER.md` (неизменяемая копия, SHA-256 `42860640…afa9`);
- **[CX]** — `codex-are/` (неизменяемые снимки Codex ARE, циклы 000–004; манифест
  `codex-are.MANIFEST.sha256`); итог — `codex-are/notes/Cycle_004-mutual-research-verdict.md`;
- **[PLAN]** — `docs/plan/PLAN_RU.md` (v5);
- **[E-web3]** — `E-web3-itu-p372-p526.md` (ITU-R P.526-16, P.372-17) и `E-web3-intelligibility.md` (разборчивость: универсальной SINR→Q не найдено);
- **[NR]** — `C:\Users\Ded\duo\work\ricon-reforger-plan\notes_ricon.md`, `notes_refs.md`
  (архив, вне репозитория; Codex проаудировал в id346).

Уровни: DOC — официальная документация; DOC-сниппет — фрагмент официальной страницы без доступа
к полному тексту (BI-вики за Cloudflare, 403); SOURCE — ванильный скрипт/ассет через Workbench;
MOD — чужой мод; STD — стандарт; REF — справочник; ПРЕДП — предположение.

Критерии:
- **П** — полнота;
- **Ц** — целостность (нет противоречий);
- **В** — валидность (первоисточник и уровень);
- **К** — качество (версия, однозначность).

## Решения владельца, учтённые в матрице (2026-10-18)

| # | Решение | Следствие для матрицы |
|---|---|---|
| 1 | Единственная цель — репозиторий `RICON-Reforger` | раздел 1: конфликт двух target снят |
| 2 | Лицензия нового мода — APL-SA; старый A3 — GPLv3; GPL-код не копируется; двойная лицензия не заявляется | Q24: выбор лицензии закрыт; совместимость чужих лицензий — нет |
| 3 | Обратная разработка запрещена; ванильные файлы — только через Workbench силами ассистента владельца по заказу `[Workbench]` | Q26 и раздел 7: RE снят с пути; SOURCE получен инструментом Workbench `tools/RICON_WbReader` (по разрешению владельца, `E-workbench-source.md` №001); звуковой граф — OPEN_UNREADABLE |
| 4 | Игру (Play / сервер) не запускать без отдельного разрешения | всё runtime — `BLOCKED_NO_RUNTIME` |

## Матрица Q1–Q34

### A. VoN и радио

| Q | Тема | Статус | Лучший уровень | Где | Проверка (П/Ц/В/К) |
|---|---|---|---|---|---|
| Q1 | RelayTransceiver | ЧАСТИЧНО | DOC (класс без членов) + MOD/FORUM (поведение 1.8) + PARTIAL_SOURCE (`E-workbench-source.md` №001 п.1: класс без скриптового API, в префабах — только дальность/шаг) | E1s, W1, CX | пересылки голоса в скриптах/префабах не видно; поведение движка — Э1r/Э7r |
| Q2 | Звуковой граф VON | ЧАСТИЧНО | DOC-сниппет + MOD; SOURCE скриптов (`E-workbench-source.md` №001 п.4: `quality` в скриптах только UI) | W1, CX | `von.acp`/`Von.sig` — OPEN_UNREADABLE через FileIO → Audio Editor (ассистент владельца) |
| Q3 | Права SetMuteState | ЧАСТИЧНО | DOC (метод есть, права не указаны) | OFF | → Э1r |
| Q4 | ECommMethod | ЗАКРЫТ | SOURCE (`E-workbench-source.md` №001 п.3): 5 значений, generated | OFF | публичной точки расширения для нового значения не найдено; текущее сопоставление радио RICON — `SQUAD_RADIO`; своё/native расширение — НЕ УСТАНОВЛЕНО |
| Q5 | Формула quality | ЧАСТИЧНО | DOC (дальности 1,3/2 км; quality 0..1); формула НЕ НАЙДЕНА | W1 | → Э1r (замер) |
| Q6 | Несколько трансиверов | ЗАКРЫТ | DOC | OFF | ок |
| Q7 | Свой радиокомпонент | ЗАКРЫТ | DOC (`ScriptedRadioComponent`) | E1s, OFF | ок |
| Q8 | Поиск трансиверов в радиусе | ЗАКРЫТ | DOC | OFF | стоимость → Э1r |
| Q9 | Стриминг / relevance | ЧАСТИЧНО | DOC (сервер маршрутизирует VoN) + MOD | W1, CX | дальняя связь при streaming-out — OPEN → Э1r |
| Q10 | Ключ шифрования | ЗАКРЫТ | DOC | OFF | эффект на звук → Э1r |

### B. Репликация

| Q | Тема | Статус | Лучший уровень | Где | Проверка (П/Ц/В/К) |
|---|---|---|---|---|---|
| Q11 | Rpl / JIP | ЗАКРЫТ по сигнатурам | DOC | OFF | поведение → Э1r |
| Q12 | Respawn / entity | ЗАКРЫТ | DOC | OFF | ок |
| Q13 | Лимиты трафика | НЕ НАЙДЕНО | — | OFF, W1 | → Э1r |

### C. Входы HDSP

| Q | Тема | Статус | Лучший уровень | Где | Проверка (П/Ц/В/К) |
|---|---|---|---|---|---|
| Q14 | Высота рельефа | ЗАКРЫТ | DOC | OFF | стоимость → Э1r |
| Q15 | Трассировка | ЗАКРЫТ | DOC | OFF | ок |
| Q16 | Погода | ЗАКРЫТ | DOC | OFF | ок |
| Q17 | Материалы | ЗАКРЫТ | SOURCE (`E-workbench-source.md` №001 п.9): `SurfaceProps` → `GameMaterial` → `BallisticInfo.GetDensity/GetThickness` | OFF | единица плотности не подписана; коэффициенты затухания — свой конфиг (Э4) |

### D. Интерфейс и ввод

| Q | Тема | Статус | Лучший уровень | Где | Проверка (П/Ц/В/К) |
|---|---|---|---|---|---|
| Q18 | Ввод / действия | ЗАКРЫТ | SOURCE (`E-workbench-source.md` №001 п.7): `VONContext`, `VONMenuContext` и их действия | OFF, NR | привязки клавиш не читались (не нужны) |
| Q19 | Расширение VON radial | ЧАСТИЧНО | MOD + PARTIAL_SOURCE (`E-workbench-source.md` №001 п.8: публичный `AddEntry`, ванильные внешние вызовы) | NR | своя запись — compile fixture + runtime NOT_TESTED (Э6) |
| Q20 | Свои меню | ЗАКРЫТ (механизм) | DOC | OFF | ок |

### E. Сотовая связь

| Q | Тема | Статус | Лучший уровень | Где | Проверка (П/Ц/В/К) |
|---|---|---|---|---|---|
| Q21 | Persistence | ЗАКРЫТ | DOC (офлайн `PersistenceSystem`) | OFF, W2 | Ц: W2 «FileIO — только MOD, 403» против офлайн `EN/interfaceFileIO.html` (`'$profile:…'`) → снято в пользу DOC |
| Q22 | SIM/телефон в инвентаре | ЗАКРЫТ | DOC | OFF | ок; SIM-префаб без уникальных ID (id323) |
| Q23 | Конфиг мода на сервере | ЧАСТИЧНО | ПРЕДП (свой JSON в `$profile:` или missionHeader) | W2, OFF | → тест Э1r |

### F. Поставка и право

| Q | Тема | Статус | Лучший уровень | Где | Проверка (П/Ц/В/К) |
|---|---|---|---|---|---|
| Q24 | Лицензия для Workshop | **ЗАКРЫТ (решение владельца: APL-SA)** | DOC (Workshop Terms Sec. 3; текст APL-SA — `LICENSE`, bohemia.net) | W1, id347, id363 | выбор лицензии закрыт. **Не закрыто:** совместимость чужих лицензий — код TFAR/ACRE2/CVON/LC по-прежнему не копируется (см. раздел 13); APL-SA «ArmaOnly» называет «existing and future Arma games», Reforger явно не перечислен |
| Q25 | BattlEye | ЧАСТИЧНО | DOC-сниппет | W1, CX | список блокировок НЕ НАЙДЕН; OPEN_NO_PRIMARY_PROOF |
| Q26 | EULA / RE | ЗАКРЫТ | DOC (License.txt Tools — локально; EULA 1874880/1874910) | E1s, W1, CX | RE запрещён (решение владельца №3); RE-gate PLAN §4.2 больше не применяется |
| Q27 | Производительность сервера | ЧАСТИЧНО | DOC-сниппет + FORUM | W1 | Ц: maxPlayers 128 vs 256 — противоречие сниппетов (на цель 30+ не влияет) |

### G. Предметная область

| Q | Тема | Статус | Лучший уровень | Где | Проверка (П/Ц/В/К) |
|---|---|---|---|---|---|
| Q28 | ITU-R | ЗАКРЫТ (кроме P.372-18) | STD_COPY (официальные PDF ITU через Internet Archive + SHA-256) | W2, CX, **E-web3-itu-p372-p526** | обновлено 2026-10-19: P.526-16 § 4.5, ур. (66) — метод для общего пути (Bullington + сферическая Земля), Epstein–Peterson в -16 нет → L2 выбран; P.372-17 ур. (5), (17), Table 1 подтверждают REQ-LE-61/62; область 0.3–250 МГц — не для тихой сельской зоны и галактики; P.372-18 (09/2026) не прочитана. P.838 — только 1–1000 ГГц: дождь на УКВ ≈ 0 |
| Q29 | GSM / 3GPP | ЗАКРЫТ (кроме LTE attach) | STD (etsi.org, Rel-19) | W2, CX | `*100#` — USSD-inspired ServiceRequest, не паритет 3GPP |
| Q30 | Ретрансляторы | ЗАКРЫТ (кроме задержки и петель) | REF | W2 | ок для дизайна |
| Q31 | Функции TFAR / ACRE2 | ЗАКРЫТ (кроме ретрансляции и антенн TFAR) | MOD | W2, NR | перекрёстная проверка выполнена (id346 PASS по notes_refs); статус не меняется |

### H. Наследие A3

| Q | Тема | Статус | Лучший уровень | Где | Проверка (П/Ц/В/К) |
|---|---|---|---|---|---|
| Q32 | Каталог 77 функций | ЗАКРЫТ | машинная проверка codex | PLAN §5a, id330 | ок |
| Q33 | R-ledger | ЗАКРЫТ | — | PLAN §6 | ок |
| Q34 | Provenance HDSP | ЧАСТИЧНО | `fn_signalCompute.sqf:96-173` (A3) + STD | NR §3, W2, CX | перекрёстная проверка выполнена; дождь/HF/материалы/тень/разборчивость A3 не обоснованы стандартами и не откалиброваны (id344 п.8) → проверка в ревью ядра |

### Итог по Q1–Q34

| Статус | Вопросы | Число |
|---|---|---|
| ЗАКРЫТ | Q6, Q7, Q8, Q10, Q11, Q12, Q14, Q15, Q16, Q20, Q21, Q22, Q24, Q26, Q28, Q29, Q30, Q31, Q32, Q33 | 20 |
| ЧАСТИЧНО | Q1, Q2, Q3, Q4, Q5, Q9, Q17, Q18, Q19, Q23, Q25, Q27, Q34 | 13 |
| НЕ НАЙДЕНО | Q13 | 1 |

## Матрица 15 разделов (объединённый итог id367)

| # | Раздел | Статус | Ключевые источники | Что осталось |
|---|---|---|---|---|
| 1 | Scope / артефакты | CLOSED | PLAN v5; решения владельца id302/310/316 и 2026-10-18 | конфликт target снят решением №1 |
| 2 | VoN / радио-контролы | PARTIAL | E1s §1–3; `interfaceVoNComponent.html:125-149`; `interfaceBaseTransceiver.html` (SetRange 1..65535, только сервер) | runtime authority/latency — BLOCKED_NO_RUNTIME |
| 3 | Аудиограф / per-pair DSP | OPEN | W1 Q2 (DOC-сниппет), CX cycle_001; SOURCE скриптов (`E-workbench-source.md` №001 п.4: `quality` в скриптах только UI) | `von.acp`/`Von.sig` — OPEN_UNREADABLE → Audio Editor (ассистент владельца); привязка к паре — Э1r |
| 4 | Relay / intercom | PARTIAL | `interfaceRelayTransceiver.html` (нет своих членов); PARTIAL_SOURCE (`E-workbench-source.md` №001 п.1, п.5: класс без API, в префабах — дальность/шаг; покрытие без прямой связи с VoN, пользователи — Conflict) | полный перечень префабов/атрибутов; пересылка голоса движком — Э1r/Э7r |
| 5 | Rpl / JIP / стриминг | PARTIAL | OFF Q11–Q12; `interfaceRplComponent.html` (EnableStreamingForConnection «Use with caution») | дальняя связь при стриминге — OPEN |
| 6 | Workbench / статическая сборка | PARTIAL | игра и Tools установлены (`addons\data\ArmaReforger.gproj`); CLI проверки компиляции документирован (`E-workbench-build.md`, `Page_CLI.html`); заказы `WB-order-002/003` | компиляция **выполнена** 2026-10-19 (claude по разрешению владельца): `RICON_Core` и `RICON_CoreTests` — 0 ошибок, 5 конфигураций (`E-workbench-source.md`); ванильные файлы (№001): SOURCE_EVIDENCE — п. 3, 4, 7, 9; PARTIAL_SOURCE — 1, 5, 6, 8; OPEN_UNREADABLE — 2 (`E-workbench-source.md` №001) |
| 7 | Право / Workshop / BattlEye / RE | PARTIAL | Workshop Terms Sec. 3; `LICENSE` (APL-SA); License.txt Tools | лицензия — решена (APL-SA); RE — запрещён; BattlEye — OPEN_NO_PRIMARY_PROOF |
| 8 | HDSP provenance / калибровка | PARTIAL | W2 Q28; E-web3 (P.526-16, P.372-17, STD_COPY); `fn_signalCompute.sqf:96-173`, `signal_core.cpp:133-224` (A3, справочник) | метод L2 выбран: P.526-16 § 4.5, ур. (66); реализация и калибровка L2 — OPEN; P.372-18 не прочитана; калибровочных данных нет |
| 9 | Масштаб 30/60 | OPEN | пары 870/3540, активные 87/177; `GetTransceiversInRange`, `AsyncTraceMove` | бюджеты — BLOCKED_NO_RUNTIME |
| 10 | Общее ядро / расширяемость | PARTIAL | REQ-core v0.7, ADR-002; host oracle 57/46/48 PASS (link-eval, снимки, маршруты); фикстуры EXT-1…5 (`RICON_CoreTests`) | Enforce компилируется (`WORKBENCH_COMPILE_PASS`, EXT-1…5 `COMPILE_FIXTURE_PASS`); SelfCheck фикстур и паритет Enforce↔oracle — NOT_TESTED |
| 11 | Верификация / один ПК | CLOSED (граница) / OPEN (доказательства) | id316, решение №4 | runtime и multi-client — BLOCKED_NO_RUNTIME |
| 12 | Сотовая / SMS | PARTIAL | W2 Q29 (3GPP 23.003/23.040/24.090); `PersistenceSystem` | M3a/M3b не реализованы |
| 13 | Референсы / лицензии | CLOSED (политика) | TFAR `LICENSE.md` APL-SA; `acre2/LICENSE` GPLv3; CVON/LC без лицензии | только идеи и требования; код не копируется |
| 14 | Упаковка / операции | PARTIAL | W1 Q24/Q27; CX fact_011 (Workshop хранит последние 50 версий) | CI, сборка, откат — не реализованы; публикация запрещена |
| 15 | Кросс-аудит | CLOSED | id354–367 | — |

Сводка (2026-10-19, после №001): CLOSED — 1, 13, 15 (+ граница 11); PARTIAL — 2, 4, 5, 6, 7, 8, 10, 12, 14; OPEN — 3, 9.

## Что нужно для приоритетов задания

| Приоритет | Чего не хватает | Чем закрыть |
|---|---|---|
| 1. Ревью ядра | production-модель разборчивости и пороги состояний: полные методы первоисточников + эмпирическая калибровка или одобренная владельцем игровая политика (статус `BLOCKED_CALIBRATION`); эталонные числа FSPL/шума — есть (oracle) | `E-web3-intelligibility.md` + калибровочный набор данных (runtime) или решение владельца `[Решение]` |
| 2. Доработка под Reforger | паритет Enforce↔oracle и запуск SelfCheck фикстур (компиляция выполнена, `E-workbench-source.md`) | исполнение — Э1r (нужен runtime) |
| 3. Слои плана | после №001: Q4, Q17, Q18 — SOURCE_EVIDENCE; Q1, Q19 — PARTIAL_SOURCE; Q2 — OPEN_UNREADABLE | Q2 — Audio Editor (ассистент владельца); Q1 — Э7r; Q19 — compile fixture + runtime (Э6) |
| 3. Слои плана (runtime) | Q3, Q5, Q9, Q13, Q23, Q25, Q27 | Э1r — только с разрешения владельца |

## Противоречия

1. FileIO: W2 (MOD) против офлайн-docs (DOC) — снято в пользу DOC.
2. maxPlayers: 128 против 256 — открыто, на план не влияет (цель — 30+).
3. Метод дифракции — **снято** (2026-10-19, E-web3): production — ITU-R P.526-16 § 4.5 (Bullington + сферическая Земля,
   ур. (66)); A3 Epstein–Peterson в -16 отсутствует и остаётся только legacy comparator.
4. PLAN v5 §4.2 и §9.1 п.10 допускают RE через gate; решение владельца №3 запрещает RE. Действует решение
   владельца; отметка — в шапке `PLAN_RU.md` и в `STATUS_RU.md`.

## Вопросы владельцу

Закрыто решениями 2026-10-18: лицензия (APL-SA), target (`RICON-Reforger`), RE (запрещён).
Открыто:
1. Заказ `[Workbench]` №001 — выполнен частично (`E-workbench-source.md` №001); открыт пункт 2: звуковой граф VON в **Audio Editor**.
2. Разрешение на runtime (Э1r) — для Q3, Q5, Q9, Q13, Q23, Q25, Q27.
