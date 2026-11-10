# RICON → RICON REFORGER: полное дерево разработки

Полное дерево разработки: от наследия Arma 3 до радио и сотовой сети

Срез: 2026-11-09. Навигационный срез существующих планов, исходников и доказательств. Не новый утверждённый roadmap и не объявление мода готовым.

- Работа, решение, доказательство и важность проблемы — четыре независимые оси.
- Выполненный source-срез, compile или unit test не доказывает работающую радиосвязь, содержание звука или готовность к выпуску.
- Радиоплан: все 21 раздел и этапы 0–8. Сотовая сеть: дальнейшая общая программа; её подробные решения требуют собственной спецификации и проверки.
- Актуальные evidence и checkpoint имеют приоритет над историческими next в старых графах; даты и границы сохраняются.
- Статусы относятся к указанному объёму узла. Статус родителя не означает, что все его дочерние критерии выполнены.
- Карточки DSH, Codex Goal и техническая готовность продукта — разные состояния. Готовность не выводится из активности команды.
- Даты реализации, бюджет и проценты готовности не оцениваются по числу узлов.

## Дерево и критерии

- **Полностью рабочая платформа связи RICON в Reforger** `ricon` — ● СЕЙЧАС · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P0
  Игроки пользуются радиосетью и сотовыми устройствами; физическая модель определяет приём, штатный VoN доставляет голос, обработка звука остаётся у слушателя.
  Проблема: Полная сквозная приёмка радио и сотовой сети ещё не предъявлена.
  Готово, когда: Рабочие радио, инфраструктура и сотовая сеть проходят собственные функциональные, сетевые, звуковые, нагрузочные и lifecycle-критерии; версия и ограничения опубликованы.
  Нужно раньше: `release`
  Источник: [reforger:docs/plan/PLAN_RU.md:1](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:1>).
  Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:9](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:9>).

  - **00 · Исходный RICON / Arma 3** `legacy` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · none
    Сохранить исходный замысел, опыт SQF/HDSP/extension/TeamSpeak и ограничения прежней реализации; использовать как историю и comparator.
    Готово, когда: Выделены переносимые требования, функции и уроки; наличие старого кода отделено от живой приёмки.
    Источник: [legacy:docs/WIKI_RU.md:1](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/docs/WIKI_RU.md:1>).

    - **Arma 3: серверная модель маршрута и тонкий TS-плагин** `d-a3-authority` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · none
      SQF решает маршрут; extension/IPC передают состояние; TS применяет per-talker gate/DSP. Это исходная архитектура, не Reforger-код.
      Проблема: Исходный прототип не равен готовой общей платформе.
      Готово, когда: Историческая архитектура и перечень переносимого поведения зафиксированы отдельно от TS/extension механизмов.
      Источник: [legacy:README.md:17](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/README.md:17>).
      Источник: [legacy:docs/WIKI_RU.md:41](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/docs/WIKI_RU.md:41>).

    - **Arma 3: extension/IPC 1.9 и TS health** `d-a3-bridge` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · P2
      Мост, provision_status, WHOAMI/TALK и диагностика описаны как implemented; live MP остаётся отдельным gate.
      Проблема: Не подтверждена вся цепочка в MP с двумя людьми.
      Готово, когда: Для исторического runtime PASS нужны согласованные RPT/TS traces; в Reforger переносится семантика, не DLL.
      Нужно раньше: `d-a3-authority`
      Источник: [legacy:docs/WIKI_RU.md:585](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/docs/WIKI_RU.md:585>).
      Источник: [legacy:docs/WIKI_RU.md:546](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/docs/WIKI_RU.md:546>).

    - **Arma 3: direct proximity v1** `d-a3-direct` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · runtime metadata / callback · P2
      Есть маршрут по расстоянию и smoke; occlusion, whisper/yell и полноценная пространственность не готовы.
      Проблема: Smoke не доказывает живое слышимое общение двух людей.
      Готово, когда: Исторические apply/clear/ACK и вход/выход из зоны подтверждены на двух клиентах, без переноса этих настроек на native DIRECT Reforger.
      Нужно раньше: `d-a3-bridge`
      Источник: [legacy:docs/WIKI_RU.md:238](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/docs/WIKI_RU.md:238>).
      Источник: [legacy:docs/WIKI_RU.md:543](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/docs/WIKI_RU.md:543>).

    - **Arma 3: PTT и frequency-device skeleton** `d-a3-radio` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · runtime metadata / callback · P2
      SR/LR профили и совместимость частот существуют; inventory, реальные рации и полный UX не завершены.
      Проблема: PTT smoke не означает полный радиопродукт.
      Готово, когда: Два живых клиента: PTT, настоящий голос, signal route и clear; catalogue/inventory критерии проверены отдельно.
      Нужно раньше: `d-a3-bridge`
      Источник: [legacy:docs/WIKI_RU.md:545](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/docs/WIKI_RU.md:545>).
      Источник: [legacy:docs/WIKI_RU.md:307](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/docs/WIKI_RU.md:307>).

    - **Arma 3: per-talker DSP и audioProof** `d-a3-dsp` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · runtime metadata / callback · P2
      Код callback/gate и selfTest существует; audioProof с loaded plugin не закрывает голос двух людей.
      Проблема: Нет общей human-audio приёмки в прочитанных сводках.
      Готово, когда: Callback и применение канала коррелируются с реальным входящим голосом; критерии происхождения/содержания не заменены synthetic proof.
      Нужно раньше: `d-a3-bridge`
      Источник: [legacy:docs/WIKI_RU.md:547](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/docs/WIKI_RU.md:547>).
      Источник: [legacy:docs/WIKI_RU.md:548](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/docs/WIKI_RU.md:548>).

    - **Arma 3: Windows/Linux dist и проверяющие скрипты** `d-a3-packaging` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · source / документ · none
      Windows CLI-рубеж упаковки реализован; Linux требует отдельной живой среды.
      Проблема: Done относится к наличию dist pipeline, не к Linux runtime или parity с ACRE/TFAR.
      Готово, когда: Повторяемые сборка/verify-dist и отдельные platform smoke не смешиваются.
      Нужно раньше: `d-a3-authority`
      Источник: [legacy:README.md:65](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/README.md:65>).
      Источник: [legacy:docs/WIKI_RU.md:549](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/docs/WIKI_RU.md:549>).

    - **Arma 3: инфраструктура и сотовая оставались design-only** `d-a3-unfinished-network` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · P2
      Ретрансляторы, вышки и handover не были готовой реализацией; MIMO также не полноценный стек.
      Проблема: Нельзя обозначить исходный Arma 3 как завершённый radio+cellular мод.
      Готово, когда: Унаследованные идеи имеют отдельный реестр зрелости; planned nodes не засчитываются как перенесённые возможности.
      Нужно раньше: `d-a3-radio`
      Источник: [legacy:docs/WIKI_RU.md:551](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/docs/WIKI_RU.md:551>).
      Источник: [legacy:docs/WIKI_RU.md:1049](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/docs/WIKI_RU.md:1049>).
      Источник: [legacy:docs/WIKI_RU.md:1095](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/docs/WIKI_RU.md:1095>).

  - **01 · Переход и проектирование Reforger** `migration` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · P2
    Архитектура, лицензия, clean-room, границы платформы, требования и доказательства; текущий план сохраняет полную цель.
    Проблема: Документы содержат датированные срезы и старые next; их нельзя читать как одинаково актуальные.
    Готово, когда: Каждая работа связана с действующим требованием, owning source и актуальной проверкой.
    Нужно раньше: `legacy`
    Источник: [reforger:docs/plan/PLAN_RU.md:1](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:1>).
    Источник: [reforger:AGENT_CONTEXT_RU.md:1](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/AGENT_CONTEXT_RU.md:1>).

    - **Выбор Reforger built-in VoN вместо TS-механизмов** `d-migration-transport` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · source / документ · none
      Одобрено переносить поведение, сохраняя native capture/codec/transport; TS plugin/pipe/extension не копируются в целевой runtime.
      Готово, когда: В target отсутствует новый voice transport; альтернативный backend не выдан за исходный approved путь.
      Нужно раньше: `d-a3-authority`
      Источник: [reforger:docs/plan/PLAN_RU.md:81](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:81>).
      Источник: [reforger:docs/plan/PLAN_RU.md:84](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:84>).
      Источник: [reforger:README.md:89](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/README.md:89>).

    - **Clean-room и разделение лицензий A3/Reforger** `d-migration-provenance` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · source / документ · none
      A3 остаётся GPL; новый target APL-SA и самостоятельная кодовая база. Референсы идей не дают права копировать код.
      Готово, когда: NOTICE/происхождение и лицензии отделяют собственные требования от чужого runtime; нет автоматического импорта legacy GPL.
      Нужно раньше: `d-migration-transport`
      Источник: [reforger:README.md:105](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/README.md:105>).
      Источник: [reforger:README.md:116](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/README.md:116>).
      Источник: [reforger:docs/plan/PLAN_RU.md:332](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:332>).

    - **Э0: каталог поведения, рисков и evidence baseline** `d-migration-baseline` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · source / документ · none
      В STATUS зафиксирован reviewed baseline; исходная матрица 77 функций и R-реестр служат входом переноса.
      Готово, когда: Каталог CfgFunctions сопоставим с матрицей; каждый риск имеет этап/тест; сохранены старые hash и исходный смысл.
      Нужно раньше: `d-migration-provenance`
      Источник: [reforger:docs/plan/STATUS_RU.md:617](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/STATUS_RU.md:617>).
      Источник: [reforger:docs/plan/PLAN_RU.md:335](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:335>).
      Источник: [reforger:docs/plan/PLAN_RU.md:351](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:351>).

    - **Э1s: static VoN/Radio/API/asset research** `d-migration-capability` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · P1
      Значительный source-разбор принят; это карта доступных и неизвестных входов, не доказанный порт на каждую пару.
      Проблема: Открытые runtime capabilities не закрываются объёмом RE.
      Готово, когда: У каждого H1–H7/relay требования есть текущий source/compile/runtime scope и способ проверки UNKNOWN.
      Нужно раньше: `d-migration-baseline`
      Источник: [reforger:docs/plan/PLAN_RU.md:358](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:358>).
      Источник: [reforger:docs/plan/ROADMAP_RU.md:29](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/ROADMAP_RU.md:29>).
      Источник: [reforger:README.md:26](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/README.md:26>).

    - **Развести полный Intent, радио Goal и номерные шкалы** `d-migration-plan-scope` — ● СЕЙЧАС · УТВЕРЖДЕНО · source / документ · P1
      M1/M2/M3 остаются общей программой; текущая detailed Goal 0–8 покрывает радиоинтеграцию, не весь cellular выпуск.
      Проблема: Риск скрыть M3 за текущим native blocker или смешать Э8 с радио этапом8.
      Готово, когда: В дереве явно сохранены 12 базовых семейств Э0–Э11/19 подэтапов, prose1–13+3a и отдельная radio0–8 детализация.
      Нужно раньше: `d-migration-baseline`
      Источник: [reforger:docs/plan/PLAN_RU.md:230](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:230>).
      Источник: [reforger:docs/plan/PLAN_RU.md:280](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:280>).
      Источник: [reforger:docs/plan/ROADMAP_RU.md:301](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/ROADMAP_RU.md:301>).
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:62](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:62>).

    - **Полная Goal и единый план 21/0–8** `n-goal-contract` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · source / документ · none
      Полностью прочитаны Goal92 и PLAN539; пять принципов, A/B и обе схемы расчёта сохранены. Это контракт, не реализация.
      Проблема: Нельзя переименовать полную Goal в один успешный capture или source helper.
      Готово, когда: Все21раздел, этапы0–8 и граница дальних сетей явно отображены без нового canonical roadmap.
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:6](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:6>).
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:92](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:92>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:291](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:291>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:512](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:512>).

    - **Датировать старый VoN/Radio 1.8** `n-legacy18-boundary` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · source / документ · none
      Сохранён source baseline1.8.0.13; old offsets/RTTI не считаются current1.9 identity.
      Проблема: Старые addresses и API-кандидаты не доказывают current native port.
      Готово, когда: Для каждого перенесённого факта указана версия; новые границы требуют собственных current refs.
      Нужно раньше: `n-goal-contract`
      Источник: [reforger:docs/evidence/E-von-native-marker-path-reverse.md:41](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/evidence/E-von-native-marker-path-reverse.md:41>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:280](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:280>).

  - **02 · Ядро, контракты и устройства** `core` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · P1
    Реестры, authority, Core, PairService, наблюдения, lifecycle и публикация результатов; готовые блоки требуют общей orchestration.
    Проблема: Наличие отдельных модулей не доказывает замкнутый production-контур.
    Готово, когда: Действительный TX и RX проходят единую исполняемую цепочку до accepted snapshot и native port.
    Нужно раньше: `migration`
    Источник: [reforger:docs/analysis/RICON_KNOWLEDGE_GRAPH_RU.md:1](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/analysis/RICON_KNOWLEDGE_GRAPH_RU.md:1>).

    - **Э2: типы устройств/сетей и registry contracts** `d-core-contracts` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · P1
      Схемы, evaluator/routing/call seams и manager существуют; поведение всей production orchestration не доказано.
      Проблема: Draft/freeze gates ADR и live authority не закрыты одним compile.
      Готово, когда: Owner/source каждого поля, malformed/stale/NaN/overflow и Rpl контракт проверены; production freeze имеет свои gates.
      Нужно раньше: `d-migration-baseline`
      Источник: [reforger:docs/plan/PLAN_RU.md:404](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:404>).
      Источник: [reforger:docs/plan/STATUS_RU.md:621](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/STATUS_RU.md:621>).
      Источник: [reforger:docs/adr/ADR-002-core-contracts.md:3](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/adr/ADR-002-core-contracts.md:3>).

    - **Э3: отдельная lifecycle machine и parity** `d-core-lifecycle-fixture` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · unit / модель / WB · none
      STATUS сообщает 14 трасс/209 шагов Enforce parity без Play; это выполненный вычислительный срез.
      Проблема: Native callback/RPC/JIP порядок отдельно не принят.
      Готово, когда: Сохранены replay/epoch/seq/lease/time/capacity проверки и граница без engine integration.
      Нужно раньше: `d-core-contracts`
      Источник: [reforger:docs/plan/STATUS_RU.md:622](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/STATUS_RU.md:622>).
      Источник: [reforger:docs/plan/PLAN_RU.md:420](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:420>).

    - **S2/S3: read-only devices → PairService** `d-core-pair-service` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · unit / модель / WB · P1
      Device adapter и служба пар имеют reviewed parity корпуса; настоящие PTT/binding/production loop остаются открытыми.
      Проблема: Калькулятор пары и tool callers не равны рабочему per-pair voice.
      Готово, когда: Действительные accepted-origin TX/listeners проходят существующий service, а не второй registry/oracle runtime.
      Нужно раньше: `d-core-contracts`, `d-core-lifecycle-fixture`
      Источник: [reforger:docs/plan/STATUS_RU.md:689](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/STATUS_RU.md:689>).
      Источник: [reforger:docs/plan/STATUS_RU.md:690](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/STATUS_RU.md:690>).
      Источник: [reforger:output/project-study-20261109-v3/STUDY_RU.md:54](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/output/project-study-20261109-v3/STUDY_RU.md:54>).

    - **S3b: snapshots/publication/receiver и production handoff** `d-core-publication` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · unit / модель / WB · P1
      106-case publication parity описана; текущий scalar schema1 ещё не несёт полную temporal/RX схему.
      Проблема: Ordering/TTL scaffold не завершает CT10 или сетевую доставку.
      Готово, когда: Production sender/receiver full/single/resync, epoch/seq и atomic отказ согласованы с новым wire contract.
      Нужно раньше: `d-core-pair-service`
      Источник: [reforger:docs/plan/STATUS_RU.md:691](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/STATUS_RU.md:691>).
      Источник: [reforger:output/project-study-20261109-v3/STUDY_RU.md:56](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/output/project-study-20261109-v3/STUDY_RU.md:56>).
      Источник: [reforger:output/project-study-20261109-v3/STUDY_RU.md:58](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/output/project-study-20261109-v3/STUDY_RU.md:58>).

    - **Единые query/budget и численные инварианты** `c-contracts` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · none
      LinkQuery/LinkBudget и finite power sum отделяют расчёт от типов сети.
      Готово, когда: Единицы, flags, failure precedence и расширение модели проходят contract без скрытого fallback.
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_LinkTypes.c:38](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_LinkTypes.c:38>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_LinkTypes.c:144](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_LinkTypes.c:144>).
      Источник: [reforger:docs/req/REQ-core-contracts.md:3](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/req/REQ-core-contracts.md:3>).

    - **Authority manager и registry** `c-authority` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · none
      Manager хранит модели/bands/network policies и проверяет Replication.IsServer. Наличие компонента не доказывает production wiring.
      Готово, когда: Authority-only регистрация и unknown-ID отказы; integration проверяется отдельно.
      Нужно раньше: `c-contracts`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_CommsManagerComponent.c:59](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_CommsManagerComponent.c:59>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_CommsManagerComponent.c:116](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_CommsManagerComponent.c:116>).

    - **Разрешение зависимостей и freeze registry** `c-registry` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · none
      RegisterBand строит evaluator из разрешённых моделей, FreezeRegistries закрывает mutation; достоверные physical defaults не появляются автоматически.
      Готово, когда: Дубликаты/null/unknown модели отвергаются; после freeze нет незаметной подмены.
      Нужно раньше: `c-authority`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_CommsManagerComponent.c:159](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_CommsManagerComponent.c:159>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_CommsManagerComponent.c:214](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_CommsManagerComponent.c:214>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_CommsManagerComponent.c:315](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_CommsManagerComponent.c:315>).

    - **Production orchestration всей цепи** `c-orchestration` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Callbacks manager выделяют epoch и ведут player set; в production Core не найдено создания PairService/Lifecycle/Publication pipeline. Pure classes не запускаются наличием manager.
      Проблема: Callbacks manager выделяют epoch и ведут player set; в production Core не найдено создания PairService/Lifecycle/Publication pipeline. Pure classes не запускаются наличием manager.
      Готово, когда: Наблюдаемый event/tick поток observations→lifecycle→pairs→physics→publication→receiver с budgets/diagnostics.
      Нужно раньше: `c-registry`, `c-lifecycle`, `c-pair`, `c-publication`, `c-geometry`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_CommsManagerComponent.c:90](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_CommsManagerComponent.c:90>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_CommsManagerComponent.c:101](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_CommsManagerComponent.c:101>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_PairService.c:336](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_PairService.c:336>).

    - **Входы PairService и preflight bounds** `c-transmission` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · none
      Bindings связывают lease/players/devices/endpoints/profiles; preflight отвергает invalid/duplicate IDs до частичного результата.
      Готово, когда: Полный coherent input и resource bounds, failure reasons не маскируются нулевым Q.
      Нужно раньше: `c-lease`, `c-query`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_PairService.c:26](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_PairService.c:26>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_PairService.c:73](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_PairService.c:73>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_PairService.c:164](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_PairService.c:164>).

    - **Направленные пары TX→listener** `c-pair` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · none
      EvaluatePair проверяет lease/TTL/generation/access/self-pair/power, вызывает query builder/evaluator и сохраняет причины.
      Готово, когда: Pure пары соответствуют эталону; actual recipients/audio не выводятся из успешного budget.
      Нужно раньше: `c-transmission`, `c-evaluator`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_PairService.c:274](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_PairService.c:274>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_PairService.c:321](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_PairService.c:321>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_PairService.c:336](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_PairService.c:336>).

    - **Candidate graph и стоимость пар** `c-scale` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P2
      Текущий путь перебирает leaseIds×listeners; insertion sort bounded 4096, но 32-user CPU/network budget не измерен этим кодом.
      Проблема: Текущий путь перебирает leaseIds×listeners; insertion sort bounded 4096, но 32-user CPU/network budget не измерен этим кодом.
      Готово, когда: Публикуются measured TX/devices/RX/candidates и раздельные CPU/network/audio budgets без изменения физического смысла.
      Нужно раньше: `c-pair`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_PairService.c:362](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_PairService.c:362>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_PairService.c:374](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_PairService.c:374>).
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:60](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:60>).

    - **Wireless query→budget pipeline** `c-evaluator` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · none
      Losses→Rx→N/N+I→SINR→sensitivity→Q/state; model contracts и finite outputs проверяются.
      Готово, когда: Invalid models/output не проходят; диагностический budget отделён от mute и voice transport.
      Нужно раньше: `c-contracts`, `c-registry`, `c-fspl`, `c-thermal`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_LinkEvaluator.c:43](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_LinkEvaluator.c:43>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_LinkEvaluator.c:114](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_LinkEvaluator.c:114>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_LinkEvaluator.c:168](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_LinkEvaluator.c:168>).

    - **Free-space propagation kernel** `c-fspl` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · none
      FreeSpaceModel/LossDb дают один базовый loss layer и domain, а не всю игровую среду.
      Готово, когда: Domain/numeric/near-range conventions и extension layer contract проверены отдельно от мира.
      Нужно раньше: `c-contracts`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_PropagationModel.c:33](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_PropagationModel.c:33>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_PropagationModel.c:75](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_PropagationModel.c:75>).

    - **Тепловой шум, interference и SINR** `c-thermal` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · none
      ThermalNoiseDbm и stable power sum различают N0, receiver noise и scalar interference.
      Готово, когда: dBm/linear conversions, extreme finite powers и отсутствие sentinel проходят contract.
      Нужно раньше: `c-contracts`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_LinkEvaluator.c:180](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_LinkEvaluator.c:180>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_LinkEvaluator.c:125](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_LinkEvaluator.c:125>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_LinkTypes.c:144](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_LinkTypes.c:144>).

    - **Production intelligibility и пороги** `c-q-calibration` — ! БЛОКЕР · НЕ ОПРЕДЕЛЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Конкретная linear кривая TEST_ONLY; production calibration отсутствует. Legacy Q0/LOST — budget result, не доказанное правило глушения.
      Проблема: Конкретная linear кривая TEST_ONLY; production calibration отсутствует. Legacy Q0/LOST — budget result, не доказанное правило глушения.
      Готово, когда: Калиброваны слышимость/разборчивость/информативность; LOST/Q0 сами не запрещают voice delivery.
      Нужно раньше: `c-thermal`, `c-rf-profile`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_IntelligibilityModel.c:2](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_IntelligibilityModel.c:2>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_IntelligibilityModel.c:32](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_IntelligibilityModel.c:32>).
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:17](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:17>).

    - **Route legs и network policy seam** `c-route` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · none
      Route агрегирует min quality/худшую ногу, проверяет chain/access/hops; base FindRoute возвращает null.
      Готово, когда: Конкретные radio/relay/cellular policies доказывают маршрут, base seam не считается сетью.
      Нужно раньше: `c-contracts`, `c-evaluator`, `c-device-access`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Network/RICON_NetworkContracts.c:119](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Network/RICON_NetworkContracts.c:119>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Network/RICON_NetworkContracts.c:249](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Network/RICON_NetworkContracts.c:249>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Network/RICON_NetworkContracts.c:266](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Network/RICON_NetworkContracts.c:266>).

    - **Общий RX конкурирующих передач** `c-shared-rx` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Scalar interference в query и per-pair loops не дают полной shared receiver state; защищённый конкурент должен влиять энергией без раскрытия речи.
      Проблема: Scalar interference в query и per-pair loops не дают полной shared receiver state; защищённый конкурент должен влиять энергией без раскрытия речи.
      Готово, когда: Simultaneous TX/capture/interference, background и output разделены и проверены в одном RX.
      Нужно раньше: `c-pair`, `c-thermal`, `c-noise`, `c-geometry`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Device/RICON_RadioDeviceAdapter.c:60](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Device/RICON_RadioDeviceAdapter.c:60>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_PairService.c:374](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_PairService.c:374>).
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:58](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:58>).

    - **RECEIVER/SERVER одна модель и parity** `c-dual-mode` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Server-only manager/reader не реализует RECEIVER. Оба места требуют общей физики/profiles/state, DSP остаётся у слушателя.
      Проблема: Server-only manager/reader не реализует RECEIVER. Оба места требуют общей физики/profiles/state, DSP остаётся у слушателя.
      Готово, когда: Parity на одинаковых inputs/profile/time при разном порядке/шаге/PTT без двойного применения.
      Нужно раньше: `c-orchestration`, `c-ct10`, `c-clock`, `c-shared-rx`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Device/RICON_RadioDeviceAdapter.c:178](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Device/RICON_RadioDeviceAdapter.c:178>).
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:12](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:12>).
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:82](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:82>).

    - **Host/Enforce gates и границы proof** `c-tests` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · none
      Отдельный CoreTests addon/Workbench probes имеют исторические pure parity. TEST_ONLY profiles не являются реальной физикой.
      Готово, когда: Independent oracle/negative controls/hash/compile не смешиваются с миром/MP/audio или текущим rerun.
      Нужно раньше: `c-contracts`, `c-pair`, `c-snapshot`, `c-fading`
      Источник: [reforger:addons/RICON_CoreTests/README.md:1](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_CoreTests/README.md:1>).
      Источник: [reforger:addons/RICON_CoreTests/README.md:16](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_CoreTests/README.md:16>).
      Источник: [reforger:docs/evidence/E-workbench-source.md:335](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/evidence/E-workbench-source.md:335>).

    - **Общий call-session seam для cellular** `c-call-session` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · none
      CallSession/Factory — контракт расширения с base 0/empty/null; cellular state machine/handover в нём не реализованы.
      Готово, когда: Cellular branch получает собственные calls/handover requirements поверх общих endpoint/route/lifecycle contracts.
      Нужно раньше: `c-route`, `c-engine-life`, `c-device-access`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Network/RICON_NetworkContracts.c:275](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Network/RICON_NetworkContracts.c:275>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Network/RICON_NetworkContracts.c:289](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Network/RICON_NetworkContracts.c:289>).
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:62](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:62>).

  - **03 · Физика канала, мир и RF-профили** `world` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · P1
    Рельеф, материалы, вода, растительность, антенны, движение и помехи с единицами, областями применимости и независимыми векторами.
    Проблема: Проверенные численные kernels и TEST_ONLY данные не заменяют реальные world inputs и калибровку.
    Готово, когда: Используются квалифицированные данные мира и профили; отсутствующие данные явно UNKNOWN.
    Нужно раньше: `core`
    Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:204](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:204>).

    - **Э4: FSPL/terrain/path layers в oracle/Enforce** `d-world-pure-layers` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · unit / модель / WB · P1
      Есть L0/L2 и pure слой path/water с датированными parity; реальные world inputs и profile domain не следуют автоматически.
      Проблема: Synthetic terrain не доказывает настоящую геометрию/материалы.
      Готово, когда: У каждого используемого слоя есть domain, units, reference vectors и отдельная world qualification.
      Нужно раньше: `d-core-contracts`
      Источник: [reforger:docs/plan/STATUS_RU.md:623](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/STATUS_RU.md:623>).
      Источник: [reforger:docs/plan/STATUS_RU.md:694](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/STATUS_RU.md:694>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:204](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:204>).

    - **Действительные мир/антенны/материалы/погода** `d-world-live-inputs` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Требуются реальные geometry/terrain/vegetation/material/antenna inputs с причинами отсутствия.
      Проблема: Нет данных нельзя выдавать за воздух/нулевые потери.
      Готово, когда: Source→world reader→qualified domain; неизвестные области UNKNOWN, а графические материалы не RF-калибровка.
      Нужно раньше: `d-world-pure-layers`
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:206](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:206>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:220](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:220>).
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:28](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:28>).

    - **Калибровка RF/RX и содержания по профилям** `d-world-calibration` — ! БЛОКЕР · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      TEST_ONLY коэффициенты и Q/SNR не являются production процентом понятых слов.
      Проблема: Production intelligibility/profile calibration остаётся обязательной.
      Готово, когда: Для реальных устройств заявлены scope, числа, погрешность и независимая слуховая/физическая оценка.
      Нужно раньше: `d-world-live-inputs`, `d-native-actuator`
      Источник: [reforger:docs/adr/ADR-002-core-contracts.md:37](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/adr/ADR-002-core-contracts.md:37>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:54](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:54>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:56](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:56>).
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:17](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:17>).

    - **Идея расширения: HF sky-wave модель** `d-extension-hf-skywave` — ○ ПЛАН · ИДЕЯ · НЕ ПРОВЕРЕНО в исполнении · P3
      PLAN приводит sky-wave как пример новой PropagationModel; это extension scenario, не принятая готовая HF-физика.
      Проблема: Legacy HF earth-wave и contract example не доказывают sky-wave.
      Готово, когда: Отдельный model/domain/primary reference, векторы и calibration; scope должен быть отдельно согласован.
      Нужно раньше: `d-release-sdk`, `d-world-calibration`
      Источник: [reforger:docs/plan/PLAN_RU.md:223](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:223>).
      Источник: [legacy:docs/WIKI_RU.md:594](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/docs/WIKI_RU.md:594>).

    - **Историческая идея: SIMO/MISO/MIMO diversity** `d-extension-diversity` — ○ ПЛАН · ИДЕЯ · НЕ ПРОВЕРЕНО в исполнении · P3
      Wiki различает SISO baseline, SIMO/MISO roadmap и MIMO design-only; throughput не требуется голосовой маршрутизации.
      Проблема: Нет одобренной текущей MIMO/5G product specification или channel-matrix implementation.
      Готово, когда: Отдельные bandwidth/correlation/antenna/channel-matrix предпосылки и бюджеты; без магического множителя дальности.
      Нужно раньше: `d-world-calibration`
      Источник: [legacy:docs/WIKI_RU.md:1072](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/docs/WIKI_RU.md:1072>).
      Источник: [legacy:docs/WIKI_RU.md:1079](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/docs/WIKI_RU.md:1079>).
      Источник: [legacy:docs/WIKI_RU.md:1087](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/docs/WIKI_RU.md:1087>).

    - **RF background profiles** `c-noise` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · none
      ManMadeNoiseModel требует versioned c/d и frequency domain; фон не равен receiver noise figure/output loudness.
      Готово, когда: Проверенные noise profiles и uncertainty, независимость фона и энергии конкурентов.
      Нужно раньше: `c-thermal`, `c-registry`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_NoiseModel.c:37](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_NoiseModel.c:37>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_CommsManagerComponent.c:132](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_CommsManagerComponent.c:132>).
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:58](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:58>).

    - **L2 diffraction calculator** `c-terrain` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · none
      Pure P.526/Bullington/spherical terms и sampling существуют отдельно от настоящего мира.
      Готово, когда: Frozen numeric/negative controls в пределах domain; world applicability проверяется отдельно.
      Нужно раньше: `c-fspl`, `c-terrain-profile`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_TerrainDiffractionModel.c:251](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_TerrainDiffractionModel.c:251>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_TerrainDiffractionModel.c:763](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_TerrainDiffractionModel.c:763>).

    - **World terrain height/profile adapter** `c-terrain-profile` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · none
      GetSurfaceY и endpoint-aware профиль реализованы; вызов настоящего мира не квалифицирован, вода выделена отдельно.
      Готово, когда: Реальные coordinate/height units, exact endpoint samples, AGL и water semantics подтверждены.
      Нужно раньше: `c-contracts`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_WorldProfileProvider.c:11](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_WorldProfileProvider.c:11>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_WorldProfileProvider.c:26](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_WorldProfileProvider.c:26>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_WorldProfileProvider.c:45](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_WorldProfileProvider.c:45>).

    - **Coverage/knowledge наблюдаемого пути** `c-path` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · none
      PathObservation различает medium/coverage/knowledge; UNKNOWN не означает воздух/LOST. Мир не опрашивается.
      Готово, когда: Partial/full segments, overlaps, water boundaries и revisions дают точные unresolved/error outcomes.
      Нужно раньше: `c-contracts`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_PathObservation.c:1](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_PathObservation.c:1>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_PathObservation.c:120](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_PathObservation.c:120>).
      Источник: [reforger:docs/req/REQ-path-observation.md:22](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/req/REQ-path-observation.md:22>).

    - **RF-profile resolver объекта** `c-object-rf` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · none
      Explicit→prefab/class rules→hint с UNKNOWN; не engine recognition и не модель losses.
      Готово, когда: Проверенные profile IDs/inheritance/provenance; texture/BallisticInfo не становятся физическими свойствами.
      Нужно раньше: `c-path`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_ObjectRfProfile.c:2](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_ObjectRfProfile.c:2>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_ObjectRfProfile.c:173](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_ObjectRfProfile.c:173>).

    - **Частотные ε/σ материала** `c-material-profile` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · source / документ · none
      Pure P.2040 fit MHz→GHz; каталог и геометрия вне kernel.
      Готово, когда: Fit/domain/provenance/units и chain numeric parity; реальные material assignments отдельно.
      Нужно раньше: `c-object-rf`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_MaterialProfile.c:1](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_MaterialProfile.c:1>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_MaterialProfile.c:104](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_MaterialProfile.c:104>).

    - **Однослойная пассивная пластина** `c-slab` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · source / документ · none
      Pure power transmission для заданных материала/геометрии; не дом, машина, проём, multipath или звук.
      Готово, когда: Frozen single-slab invariants и numeric/negative controls, без расширения заявленного domain.
      Нужно раньше: `c-material-profile`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_MaterialSlab.c:3](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_MaterialSlab.c:3>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_MaterialSlab.c:181](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_MaterialSlab.c:181>).

    - **Вода на заданном пути** `c-water` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · none
      Усреднённый профиль и интервалы дают loss заданной прямой; refraction и строгий cutoff bound автоматически не следуют.
      Готово, когда: Medium/coverage/units/domain и отдельные геометрические кандидаты сохраняют UNKNOWN честно.
      Нужно раньше: `c-path`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_WaterLayer.c:2](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_WaterLayer.c:2>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_WaterLayer.c:102](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_WaterLayer.c:102>).
      Источник: [reforger:docs/req/REQ-path-observation.md:119](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/req/REQ-path-observation.md:119>).

    - **Растительность: through и over-canopy budgets** `c-vegetation` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · none
      Pure P4/P4.2/P4.3 требуют explicit intervals/profiles. Выбор полного through/over бюджета не является P1 границей или voice gate.
      Готово, когда: Отдельные canopy/through domains, отсутствие удвоения loss и frozen numeric parity; реальные профили отдельно.
      Нужно раньше: `c-path`, `c-object-rf`, `c-terrain`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_VegetationLayer.c:354](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_VegetationLayer.c:354>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_OvercanopyLayer.c:1](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_OvercanopyLayer.c:1>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_VegetationPathLayer.c:1](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_VegetationPathLayer.c:1>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_VegetationPathLayer.c:128](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_VegetationPathLayer.c:128>).

    - **Production geometry/candidate paths** `c-geometry` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Pure kernels не собирают сами здания, thickness, препятствия, антенны и полное покрытие среды из engine.
      Проблема: Pure kernels не собирают сами здания, thickness, препятствия, антенны и полное покрытие среды из engine.
      Готово, когда: Повторяемые real-world traces с units/coverage/revision/provenance; missing geometry остаётся UNKNOWN.
      Нужно раньше: `c-path`, `c-object-rf`, `c-terrain-profile`, `c-antenna`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_PathObservation.c:1](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_PathObservation.c:1>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_MaterialSlab.c:3](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_MaterialSlab.c:3>).
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:85](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:85>).

    - **Реальные RF/material/device profiles** `c-profiles` — ! БЛОКЕР · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Частичный паспортный каталог и TEST_ONLY/ASSUMED параметры не образуют полный production S2 profile.
      Проблема: Частичный паспортный каталог и TEST_ONLY/ASSUMED параметры не образуют полный production S2 profile.
      Готово, когда: Точные prefab variants, источники/units/uncertainty и measured applicability/calibration с версиями.
      Нужно раньше: `c-rf-profile`, `c-units`, `c-material-profile`, `c-geometry`
      Источник: [reforger:docs/req/REQ-radio-model-catalog.md:11](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/req/REQ-radio-model-catalog.md:11>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Device/RICON_RadioDeviceAdapter.c:34](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Device/RICON_RadioDeviceAdapter.c:34>).
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:26](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:26>).

  - **04 · Штатный VoN / адресный Radio-порт** `native` — ● СЕЙЧАС · УТВЕРЖДЕНО · source / документ · P0
    Текущий критический технический путь: происхождение native-объектов, достаточная доставка содержания A и адресное воздействие только на Radio B.
    Проблема: Поддерживаемый writer и фактический адресный DSP-эффект ещё не приняты; callback, указатель и cache этого не доказывают.
    Готово, когда: A и B проходят native-проверку с содержанием, точной адресностью, сохранением DIRECT, чужих потоков и пользовательского состояния.
    Нужно раньше: `migration`, `core`
    Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:32](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:32>).
    Источник: [reforger:output/native-resume-20261109/CHECKPOINT_RU.md:1](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/output/native-resume-20261109/CHECKPOINT_RU.md:1>).

    - **Э1r/Э5: настоящий player-voice baseline** `d-native-baseline` — ! БЛОКЕР · УТВЕРЖДЕНО · runtime metadata / callback · P0
      Runtime/metadata опыты были, но source/sink и content/playback не приняты совместно.
      Проблема: Callbacks и proxy sound не заменяют второго пользователя и слышимый источник.
      Готово, когда: Qualified player A→B известного содержания с positive/negative/recovery и exact identity.
      Нужно раньше: `d-migration-capability`
      Источник: [reforger:README.md:10](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/README.md:10>).
      Источник: [reforger:docs/plan/ROADMAP_RU.md:55](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/ROADMAP_RU.md:55>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:343](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:343>).

    - **B: supported per-source/per-listener Radio-only actuator** `d-native-actuator` — ● СЕЙЧАС · УТВЕРЖДЕНО · source / документ · P0
      Текущий seam сохраняет BLOCKED_NO_ACTUATOR; broad plan требует точного private stream/lifetime управления.
      Проблема: Ключевой текущий port gap; pointer/whole-RX mute не доказывают B.
      Готово, когда: Точная адресность и обратимый эффект только Radio, DIRECT/другие потоки/user state сохранены.
      Нужно раньше: `d-native-baseline`, `d-core-contracts`
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:34](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:34>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:248](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:248>).
      Источник: [reforger:output/project-study-20261109-v3/STUDY_RU.md:55](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/output/project-study-20261109-v3/STUDY_RU.md:55>).

    - **A: достаточная доставка исходных фрагментов** `d-native-delivery-a` — ! БЛОКЕР · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P0
      Native range/discovery/q не должны заранее уничтожать допустимые будущие просветы.
      Проблема: Нужное содержимое нельзя восстановить DSP после необратимого drop.
      Готово, когда: Источник/адресность/время/доставка исходных окон подтверждены независимо от RMS/callback.
      Нужно раньше: `d-native-baseline`
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:32](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:32>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:242](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:242>).

    - **Сохранение DIRECT, соседних потоков и user state** `d-native-direct-controls` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Штатный direct оставлен ванильным; per-pair Radio вмешательство требует независимых сохранённых контролей.
      Проблема: Whole-event/whole-transceiver изменение может затронуть чужой путь.
      Готово, когда: DIRECT до/во время/после, A→B/A→C и A/C→B, mute/power/squelch/rights и restore пройдены.
      Нужно раньше: `d-native-actuator`
      Источник: [reforger:docs/plan/PLAN_RU.md:85](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:85>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:351](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:351>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:395](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:395>).

    - **Поддерживаемость in-mod порта и повторная qualification** `d-native-workshop-surface` — ! БЛОКЕР · НЕ ОПРЕДЕЛЕНО · source / документ · P0
      Офлайн private dataflow не является распространённым Script/Workshop API; новые DLL/backend запрещены текущей Goal.
      Проблема: Не выбран доказанный production surface для нужного actuator.
      Готово, когда: Lawful/safe/Workshop-compatible port на pinned Game context; версия/API и fallback status задокументированы.
      Нужно раньше: `d-native-actuator`
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:252](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:252>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:507](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:507>).
      Источник: [reforger:README.md:89](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/README.md:89>).

    - **VoN identity/device/transceiver binding** `c-binding` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · none
      Pure descriptor не владеет engine handle; проверяет IDs/indices и snapshot epoch/link.
      Готово, когда: Exact current lease/device/transceiver/generation адресует нужный native listening stream без чужих пар.
      Нужно раньше: `c-lifecycle`, `c-snapshot`, `c-device-access`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Network/RICON_NetworkContracts.c:323](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Network/RICON_NetworkContracts.c:323>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Network/RICON_NetworkContracts.c:324](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Network/RICON_NetworkContracts.c:324>).

    - **VoN seam: BLOCKED_NO_ACTUATOR** `c-von-seam` — ! БЛОКЕР · УТВЕРЖДЕНО · source / документ · P0
      Каждый валидный ApplySnapshot/expiry возвращает явный BLOCKED_NO_ACTUATOR; OK в enum отсутствует.
      Проблема: Каждый валидный ApplySnapshot/expiry возвращает явный BLOCKED_NO_ACTUATOR; OK в enum отсутствует.
      Готово, когда: Supported per-source actuator и реальные native/audio/content A/B, а не только валидация/compile.
      Нужно раньше: `c-binding`, `c-receiver`, `c-expiry`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Network/RICON_NetworkContracts.c:347](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Network/RICON_NetworkContracts.c:347>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Network/RICON_NetworkContracts.c:350](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Network/RICON_NetworkContracts.c:350>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Network/RICON_NetworkContracts.c:367](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Network/RICON_NetworkContracts.c:367>).

    - **Штатный voice path и content acceptance** `c-audio` — ! БЛОКЕР · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P0
      Goal сохраняет PTT/capture/Opus/network/decode/output. Metadata/cache/callback/unit не доказывают услышанные исходные фрагменты.
      Проблема: Goal сохраняет PTT/capture/Opus/network/decode/output. Metadata/cache/callback/unit не доказывают услышанные исходные фрагменты.
      Готово, когда: Адресные A/B обоих режимов, DIRECT/user states, source/time, физические просветы без случайного спасения слов.
      Нужно раньше: `c-von-seam`, `c-dual-mode`, `c-shared-rx`, `c-q-calibration`
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:10](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:10>).
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:80](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:80>).
      Источник: [reforger:AGENT_CONTEXT_RU.md:7](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/AGENT_CONTEXT_RU.md:7>).

    - **K0: source/API intake** `n-k0-source` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · source / документ · none
      Исторически квалифицированы только отдельные сигнатуры/mapping; listener lifetime, effective binding, allocation/readiness/direct/remote sink оставались отдельными gaps.
      Проблема: Полная native Goal этим вспомогательным результатом не закрывается.
      Готово, когда: Документированы точные положительные source факты и UNKNOWN; K0 runtime целиком не объявлен выполненным.
      Нужно раньше: `n-legacy18-boundary`
      Источник: [reforger:docs/evidence/E-k0-von-qualification.md:3](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/evidence/E-k0-von-qualification.md:3>).
      Источник: [reforger:docs/evidence/E-k0-von-qualification.md:13](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/evidence/E-k0-von-qualification.md:13>).

    - **K0: effective input, ownership и release** `n-k0-effective-runtime` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · runtime metadata / callback · P1
      Поздние captures подтверждают callback path, но не снимают весь K0 effective-binding/lifecycle/release контракт.
      Проблема: UP/выход процесса и callback count не доказывают native release/ownership или безопасность DIRECT.
      Готово, когда: Подтвердить actual input context/device/entry/generation, независимый RX и наблюдение release с отрицательными контролями.
      Нужно раньше: `n-k0-source`, `n-capture01`
      Источник: [reforger:docs/evidence/E-k0-von-qualification.md:14](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/evidence/E-k0-von-qualification.md:14>).
      Источник: [legacy:output/ricon-goal-plan-20261106/guarded-cli-capture/capture01/review.json:138](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-goal-plan-20261106/guarded-cli-capture/capture01/review.json:138>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:340](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:340>).

    - **Compile native bridge/probes** `n-native-compile` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · compile · none
      Compile01 подтвердил вход59source и Game/WorkbenchGame module summaries с SCRIPT(E)=0, но platform init провалился. Это не общий process PASS.
      Проблема: Полная native Goal этим вспомогательным результатом не закрывается.
      Готово, когда: Сохранить compile scope и platform failure; не считать отсутствующие fixture markers выполненными тестами.
      Нужно раньше: `n-goal-contract`
      Источник: [reforger:docs/evidence/receipts/native-ricon-compile-review-20261104.json:2](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/evidence/receipts/native-ricon-compile-review-20261104.json:2>).
      Источник: [reforger:docs/evidence/receipts/native-ricon-compile-review-20261104.json:180](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/evidence/receipts/native-ricon-compile-review-20261104.json:180>).

    - **Маркер: API-кандидат изучен** `n-marker-feasibility` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · source / документ · none
      SoundEventPriority/OnReceive и campaign radio рассмотрены; generic radio/AI event/control plane не объявлены player voice protocol.
      Проблема: Полная native Goal этим вспомогательным результатом не закрывается.
      Готово, когда: Принять только описанный ограниченный результат; функциональную native-приёмку не приписывать.
      Нужно раньше: `n-k0-source`
      Источник: [reforger:docs/evidence/E-von-marker-feasibility-lead.md:5](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/evidence/E-von-marker-feasibility-lead.md:5>).
      Источник: [reforger:docs/evidence/E-von-marker-feasibility-lead.md:39](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/evidence/E-von-marker-feasibility-lead.md:39>).

    - **Маркер как вспомогательный same-path probe** `n-marker-samepath` — ! БЛОКЕР · ИДЕЯ · UNKNOWN · P2
      Независимое декодирование маркера и положение относительно mute/playback не квалифицированы; это возможный инструмент, не замена audio/content Goal.
      Проблема: Одинаковый трансивер или временной RX не доказывают player-voice payload.
      Готово, когда: Если кандидат используется, B получает данные только по исследуемому native пути; source/radio/mute attribution и negative controls независимы.
      Нужно раньше: `n-marker-feasibility`
      Источник: [reforger:docs/evidence/E-von-marker-feasibility-lead.md:21](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/evidence/E-von-marker-feasibility-lead.md:21>).
      Источник: [reforger:docs/evidence/E-von-native-marker-path-reverse.md:12](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/evidence/E-von-native-marker-path-reverse.md:12>).
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:18](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:18>).

    - **Own receiver prefab: registration и readback** `n-own-prefab-source` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · source / документ · none
      Own inherited resource получил GUID0DE75817F6D87DD2; fresh GUID-only source lookup и stock control сохранили own/stock ACP. Старые3diagnostics остаются UNKNOWN.
      Проблема: Полная native Goal этим вспомогательным результатом не закрывается.
      Готово, когда: Engine source lookup собственного ресурса принят, original stock не изменён; это не live private writer.
      Нужно раньше: `n-native-compile`
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:68](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:68>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:343](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:343>).

    - **Game baseline: bootstrap/read-only** `n-native-baseline` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · runtime metadata / callback · none
      Baseline01 загрузил модули/Plain fixture и квалифицировал passive type/locator scope; V0/T0 partial scans не доказывают отсутствие объектов.
      Проблема: Полная native Goal этим вспомогательным результатом не закрывается.
      Готово, когда: Принять только описанный ограниченный результат; функциональную native-приёмку не приписывать.
      Нужно раньше: `n-native-compile`
      Источник: [legacy:output/ricon-goal-plan-20261106/native-baseline/baseline01/review.json:4](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-goal-plan-20261106/native-baseline/baseline01/review.json:4>).

    - **Два участника и passive own-child pair09** `n-passive-pair` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · runtime metadata / callback · none
      Own B child/control и typed C creation resource own ACP наблюдались; callbacks0, full server numeric radio ownership/permission не квалифицированы.
      Проблема: Полная native Goal этим вспомогательным результатом не закрывается.
      Готово, когда: Принять только описанный ограниченный результат; функциональную native-приёмку не приписывать.
      Нужно раньше: `n-native-baseline`, `n-own-prefab-source`
      Источник: [legacy:output/ricon-goal-plan-20261106/paired-passive/pair09/review.json:3](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-goal-plan-20261106/paired-passive/pair09/review.json:3>).
      Источник: [legacy:output/ricon-goal-plan-20261106/paired-passive/pair09/review.json:68](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-goal-plan-20261106/paired-passive/pair09/review.json:68>).

    - **Watcher b:null publication race** `n-watcher-race` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · unit / модель / WB · none
      Исправлен manifest-before-status.b race; source/fake regression29 не является29native опытами. Не повторять capture02 без новой переменной.
      Проблема: Полная native Goal этим вспомогательным результатом не закрывается.
      Готово, когда: Непубликованный b=null ждёт bounded без target open; published identity проверяется. Сохранён scope fake checks.
      Нужно раньше: `n-native-baseline`
      Источник: [legacy:output/ricon-goal-plan-20261106/native-von-ref-probe/watch-stream-race-review.json:16](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-goal-plan-20261106/native-von-ref-probe/watch-stream-race-review.json:16>).
      Источник: [reforger:output/native-resume-20261109/same-t-initializer/SOURCE_FINDINGS_RU.md:7](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/output/native-resume-20261109/same-t-initializer/SOURCE_FINDINGS_RU.md:7>).

    - **Capture01: native callback path** `n-capture01` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · runtime metadata / callback · none
      Наблюдён штатный A1→B2 capture/used/RX callback path; ни PCM, ни содержание, ни строгий release этим не приняты.
      Проблема: Полная native Goal этим вспомогательным результатом не закрывается.
      Готово, когда: Принять только описанный ограниченный результат; функциональную native-приёмку не приписывать.
      Нужно раньше: `n-native-baseline`
      Источник: [legacy:output/ricon-goal-plan-20261106/guarded-cli-capture/capture01/review.json:4](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-goal-plan-20261106/guarded-cli-capture/capture01/review.json:4>).
      Источник: [legacy:output/ricon-goal-plan-20261106/guarded-cli-capture/capture01/review.json:138](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-goal-plan-20261106/guarded-cli-capture/capture01/review.json:138>).

    - **Capture02: typed stock C→V→T** `n-capture02` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · runtime metadata / callback · none
      RX-correlated ownB/key1 typed candidate ведёт к stock ACP. NonAtomic snapshot и cutoff timing не квалифицируют full lifetime/content.
      Проблема: Полная native Goal этим вспомогательным результатом не закрывается.
      Готово, когда: Принять только описанный ограниченный результат; функциональную native-приёмку не приписывать.
      Нужно раньше: `n-watcher-race`, `n-capture01`
      Источник: [legacy:output/ricon-goal-plan-20261106/guarded-cli-capture/capture02/review.json:4](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-goal-plan-20261106/guarded-cli-capture/capture02/review.json:4>).
      Источник: [reforger:output/native-resume-20261109/CHECKPOINT_RU.md:19](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/output/native-resume-20261109/CHECKPOINT_RU.md:19>).

    - **Capture03: public Snd lookup negative** `n-capture03-public-snd` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · runtime metadata / callback · none
      Два имени на двух public Character SndComponent вернули NOT_FOUND во время valid RX. Отрицание только этих lookup, не всех private writer.
      Проблема: Полная native Goal этим вспомогательным результатом не закрывается.
      Готово, когда: Принять только описанный ограниченный результат; функциональную native-приёмку не приписывать.
      Нужно раньше: `n-capture02`
      Источник: [legacy:output/ricon-goal-plan-20261106/guarded-cli-capture/capture03/review.json:4](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-goal-plan-20261106/guarded-cli-capture/capture03/review.json:4>).
      Источник: [legacy:output/ricon-goal-plan-20261106/guarded-cli-capture/capture03/review.json:135](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-goal-plan-20261106/guarded-cli-capture/capture03/review.json:135>).

    - **Capture04: memory admission остановил запуск** `n-capture04-admission` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · runtime metadata / callback · none
      Guard остановил попытку до запуска A/B; capture.attempted=false и samples0 не являются VoN-negative.
      Проблема: Полная native Goal этим вспомогательным результатом не закрывается.
      Готово, когда: Сохранить отказ по ресурсу и отсутствие опыта; не менять pagefile/чужие apps и не превращать fail в RF-эффект.
      Нужно раньше: `n-passive-pair`
      Источник: [legacy:output/ricon-goal-plan-20261106/guarded-cli-capture/capture04/review.json:4](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-goal-plan-20261106/guarded-cli-capture/capture04/review.json:4>).
      Источник: [reforger:docs/plan/STATUS_RU.md:17](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/STATUS_RU.md:17>).

    - **Capture05: active own ACP chain** `n-capture05-own-chain` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · runtime metadata / callback · none
      OwnB current typed C→key1 V→T→ownACP наблюдалась. Атомарный lifetime, actual private destination, B/DSP/content остаются открытыми.
      Проблема: Полная native Goal этим вспомогательным результатом не закрывается.
      Готово, когда: Принять только описанный ограниченный результат; функциональную native-приёмку не приписывать.
      Нужно раньше: `n-passive-pair`, `n-capture04-admission`, `n-capture03-public-snd`
      Источник: [legacy:output/ricon-goal-plan-20261106/guarded-cli-capture/capture05/review.json:5](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-goal-plan-20261106/guarded-cli-capture/capture05/review.json:5>).

    - **Capture06: exact parameter names** `n-capture06-names` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · runtime metadata / callback · none
      В current own typed T resource найдены точные имена gain/quality. Native comparator execution и адресный writer не квалифицированы.
      Проблема: Полная native Goal этим вспомогательным результатом не закрывается.
      Готово, когда: Принять только описанный ограниченный результат; функциональную native-приёмку не приписывать.
      Нужно раньше: `n-capture05-own-chain`
      Источник: [legacy:output/ricon-goal-plan-20261106/guarded-cli-capture/capture06/review.json:5](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-goal-plan-20261106/guarded-cli-capture/capture06/review.json:5>).

    - **Capture07: один INPUT scalar snapshot** `n-capture07-input` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · runtime metadata / callback · none
      Stored gain0/quality consistency квалифицированы только в одном снимке; baseline1 gate не пройден. Это не DSP0/mute.
      Проблема: Полная native Goal этим вспомогательным результатом не закрывается.
      Готово, когда: Принять только описанный ограниченный результат; функциональную native-приёмку не приписывать.
      Нужно раньше: `n-capture06-names`
      Источник: [legacy:output/ricon-goal-plan-20261106/guarded-cli-capture/capture07/review.json:5](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-goal-plan-20261106/guarded-cli-capture/capture07/review.json:5>).

    - **Capture08: пять дискретных INPUT точек** `n-capture08-timeline` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · runtime metadata / callback · none
      Пять same T/R snapshots с gain0/quality≈1; непрерывный/универсальный ноль, hydration и audio effect не доказаны.
      Проблема: Полная native Goal этим вспомогательным результатом не закрывается.
      Готово, когда: Принять только описанный ограниченный результат; функциональную native-приёмку не приписывать.
      Нужно раньше: `n-capture07-input`
      Источник: [legacy:output/ricon-goal-plan-20261106/guarded-cli-capture/capture08/review.json:5](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-goal-plan-20261106/guarded-cli-capture/capture08/review.json:5>).

    - **Default-off preregistration: source/compile** `n-prereg-compile` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · compile · none
      A6 изменил один source; compile09 SCRIPT(E)=0, platform/resource diagnostics сохранены. Compile не означает SDK execution.
      Проблема: Полная native Goal этим вспомогательным результатом не закрывается.
      Готово, когда: Принять только описанный ограниченный результат; функциональную native-приёмку не приписывать.
      Нужно раньше: `n-capture08-timeline`
      Источник: [legacy:output/ricon-goal-plan-20261106/source-preregister/compile09-review.json:5](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-goal-plan-20261106/source-preregister/compile09-review.json:5>).
      Источник: [legacy:output/ricon-goal-plan-20261106/source-preregister/compile09-review.json:134](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-goal-plan-20261106/source-preregister/compile09-review.json:134>).

    - **Capture09: actual source API=1** `n-capture09-prereg` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · runtime metadata / callback · none
      AddOrFind/Get1 path достигнут до gated release/RX; SGM index75 и пять private gain0 snapshots не доказывают public D==privateT.
      Проблема: Полная native Goal этим вспомогательным результатом не закрывается.
      Готово, когда: Принять только описанный ограниченный результат; функциональную native-приёмку не приписывать.
      Нужно раньше: `n-prereg-compile`
      Источник: [legacy:output/ricon-goal-plan-20261106/guarded-cli-capture/capture09/review.json:5](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-goal-plan-20261106/guarded-cli-capture/capture09/review.json:5>).
      Источник: [legacy:output/ricon-goal-plan-20261106/guarded-cli-capture/capture09/review.json:6](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-goal-plan-20261106/guarded-cli-capture/capture09/review.json:6>).

    - **Capture10: numeric OUTPUT cache** `n-capture10-output` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · runtime metadata / callback · none
      Пять finite snapshots: IDs5/8 ненулевые, ID12 present-zero. Actual RadioID12 mapping, fresh native evaluation и consumer/DSP не квалифицированы.
      Проблема: Полная native Goal этим вспомогательным результатом не закрывается.
      Готово, когда: Не приравнивать present-zero и no-match fallback0; не переименовывать cache values в DSP.
      Нужно раньше: `n-capture09-prereg`
      Источник: [legacy:output/ricon-goal-plan-20261106/guarded-cli-capture/capture10/review.json:5](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-goal-plan-20261106/guarded-cli-capture/capture10/review.json:5>).
      Источник: [reforger:AGENT_CONTEXT_RU.md:38](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/AGENT_CONTEXT_RU.md:38>).

    - **Authored ACP/SIG: Radio и Direct** `n-authored-radio-graph` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · source / документ · none
      Source30733:12→Radio_V/Bus Voice+Noise является authored hint. Whole VON_RADIO содержит также Direct; это не actual private control mapping.
      Проблема: Полная native Goal этим вспомогательным результатом не закрывается.
      Готово, когда: Принять только описанный ограниченный результат; функциональную native-приёмку не приписывать.
      Нужно раньше: `n-own-prefab-source`
      Источник: [reforger:tools/RICON_LiveProbe/Sounds/RICON_VoNProbe/RICON_VoNProbe.acp:3](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/tools/RICON_LiveProbe/Sounds/RICON_VoNProbe/RICON_VoNProbe.acp:3>).
      Источник: [reforger:tools/RICON_LiveProbe/Sounds/RICON_VoNProbe/RICON_VoNProbe.sig:159](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/tools/RICON_LiveProbe/Sounds/RICON_VoNProbe/RICON_VoNProbe.sig:159>).
      Источник: [legacy:output/ricon-goal-plan-20261106/source-preregister/radio-output-mapping.json:2](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-goal-plan-20261106/source-preregister/radio-output-mapping.json:2>).

    - **Current readers/getter: разные entry paths** `n-native-readers` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · source / документ · none
      A3CB20(150) и A3CA00(280) раздельны; getter54→leaf49 читает OUTPUT. Prepare/evaluation opaque; cache snapshot не вызов getter.
      Проблема: Полная native Goal этим вспомогательным результатом не закрывается.
      Готово, когда: Принять только описанный ограниченный результат; функциональную native-приёмку не приписывать.
      Нужно раньше: `n-capture10-output`
      Источник: [reforger:AGENT_CONTEXT_RU.md:41](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/AGENT_CONTEXT_RU.md:41>).
      Источник: [legacy:output/ricon-goal-plan-20261106/source-preregister/output-reader-review.json:61](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-goal-plan-20261106/source-preregister/output-reader-review.json:61>).

    - **Same-T source307: submission** `n-playback307` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · source / документ · none
      V+20 T проходит к A3A830 и opaque service/collector. Array-like submission не назван DSP buffer или RadioBus.
      Проблема: Полная native Goal этим вспомогательным результатом не закрывается.
      Готово, когда: Принять только описанный ограниченный результат; функциональную native-приёмку не приписывать.
      Нужно раньше: `n-native-readers`
      Источник: [legacy:output/ricon-goal-plan-20261106/native-playback-owner-20261107/review.json:94](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-goal-plan-20261106/native-playback-owner-20261107/review.json:94>).

    - **Source519: construction/registration args** `n-construction519` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · source / документ · none
      Current event record и same T передаются E72090/A39930; allocation152 и low-level args не устанавливают Radio selector.
      Проблема: Полная native Goal этим вспомогательным результатом не закрывается.
      Готово, когда: Принять только описанный ограниченный результат; функциональную native-приёмку не приписывать.
      Нужно раньше: `n-playback307`
      Источник: [legacy:output/ricon-goal-plan-20261106/native-playback-owner-20261107/same-t-construction/review.json:64](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-goal-plan-20261106/native-playback-owner-20261107/same-t-construction/review.json:64>).

    - **Source346: original P, P+68=same T** `n-object346` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · source / документ · none
      E72090 сохраняет same T вP+68 и возвращает original allocation P; selector record+1A сохраняется low8. Это только constructor state.
      Проблема: Полная native Goal этим вспомогательным результатом не закрывается.
      Готово, когда: Принять только описанный ограниченный результат; функциональную native-приёмку не приписывать.
      Нужно раньше: `n-construction519`
      Источник: [legacy:output/ricon-goal-plan-20261106/native-playback-owner-20261107/same-t-object/review.json:35](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-goal-plan-20261106/native-playback-owner-20261107/same-t-object/review.json:35>).

    - **Source1084: generic control consumer** `n-consumer1084` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · source / документ · none
      После initializer zero-status same P идёт E737C0(item+80,&floatOut); conversion/clamp→P+94 не gain. Full same-T после aliases не доказан.
      Проблема: Полная native Goal этим вспомогательным результатом не закрывается.
      Готово, когда: Принять только описанный ограниченный результат; функциональную native-приёмку не приписывать.
      Нужно раньше: `n-object346`
      Источник: [legacy:output/ricon-goal-plan-20261106/native-playback-owner-20261107/same-t-nodes/review.json:59](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-goal-plan-20261106/native-playback-owner-20261107/same-t-nodes/review.json:59>).

    - **A3AA40 source2207 принят** `n-initializer2207` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · source / документ · none
      Current pinned e877 owning range и handoff ctx/item/localT-Pbundle→A3B6D0 квалифицированы; mutable aliases не доказывают сохранность P68.
      Проблема: Полная native Goal этим вспомогательным результатом не закрывается.
      Готово, когда: Принять только описанный ограниченный результат; функциональную native-приёмку не приписывать.
      Нужно раньше: `n-consumer1084`
      Источник: [reforger:output/native-resume-20261109/same-t-initializer/review.json:5](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/output/native-resume-20261109/same-t-initializer/review.json:5>).
      Источник: [reforger:output/native-resume-20261109/same-t-initializer/SOURCE_FINDINGS_RU.md:23](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/output/native-resume-20261109/same-t-initializer/SOURCE_FINDINGS_RU.md:23>).

    - **A3B6D0 source838 принят** `n-delegate838` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · source / документ · none
      Map-like state0/1/2, self-recursion, virtual C0/C8 и concrete P-linked bitmap stores доказаны. Не только delegation; не RF/Radio semantics.
      Проблема: Полная native Goal этим вспомогательным результатом не закрывается.
      Готово, когда: Принять только описанный ограниченный результат; функциональную native-приёмку не приписывать.
      Нужно раньше: `n-initializer2207`
      Источник: [reforger:output/native-resume-20261109/same-t-delegate/review.json:5](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/output/native-resume-20261109/same-t-delegate/review.json:5>).
      Источник: [reforger:output/native-resume-20261109/same-t-delegate/SOURCE_FINDINGS_RU.md:27](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/output/native-resume-20261109/same-t-delegate/SOURCE_FINDINGS_RU.md:27>).

    - **E73830 closed entry-CFG66 принят** `n-lookup66` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · source / документ · none
      66contiguous bytes/21instructions/23edges/2RET; pointer-key ordinal lookup, .pdata owner отсутствует. Не полный function/EH/termination extent.
      Проблема: Полная native Goal этим вспомогательным результатом не закрывается.
      Готово, когда: Сохранить counter count0→FFFFFFFF→outer0; return0 не APPLIED и ordinal не Radio selector.
      Нужно раньше: `n-delegate838`
      Источник: [reforger:output/native-resume-20261109/p-item-lookup/review.json:13](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/output/native-resume-20261109/p-item-lookup/review.json:13>).
      Источник: [reforger:output/native-resume-20261109/p-item-lookup/SOURCE_FINDINGS_RU.md:27](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/output/native-resume-20261109/p-item-lookup/SOURCE_FINDINGS_RU.md:27>).

    - **Same effective item C0: provenance inputs frozen** `n-c0-input-freeze` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · source / документ · none
      inputs.json закрепляет accepted inputs и exact callsiteA3B746/slotC0, но cell/target=NULL; прежние root reports21–24 полнотой не охвачены.
      Проблема: Полная native Goal этим вспомогательным результатом не закрывается.
      Готово, когда: Принять freeze/UNKNOWN, не owning identity. Cached completed task5/7 означает завершение изучения, не qualification cell.
      Нужно раньше: `n-lookup66`
      Источник: [reforger:output/native-resume-20261109/item-c0-38e06317/inputs.json:1468](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/output/native-resume-20261109/item-c0-38e06317/inputs.json:1468>).
      Источник: [reforger:output/native-resume-20261109/item-c0-38e06317/inputs.json:1471](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/output/native-resume-20261109/item-c0-38e06317/inputs.json:1471>).
      Источник: [reforger:output/native-resume-20261109/CHECKPOINT_RU.md:96](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/output/native-resume-20261109/CHECKPOINT_RU.md:96>).

    - **Текущий prerequisite: безопасный witness harness** `n-witness-safety` — ● СЕЙЧАС · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P0
      Свежая DSH metadata показывает API/code/safety preparation. Game/cell NO_READY; нужен benign owned-child gate до actual witness.
      Проблема: Operational safety, API contract, bounded cleanup и independent readiness ещё не подтверждены нашим corpus.
      Готово, когда: Официальные API contracts, pure failure tests и benign owned fixture доказывают save/restore/cleanup/stop; reviewer допускает только конкретный следующий witness, без attach/injection/patch.
      Нужно раньше: `n-c0-input-freeze`
      Источник: [legacy:output/ricon-full-development-graph-20261109/native-inventory/process-snapshot.json:1](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-full-development-graph-20261109/native-inventory/process-snapshot.json:1>).
      Источник: [reforger:output/native-resume-20261109/item-c0-38e06317/inputs.json:1471](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/output/native-resume-20261109/item-c0-38e06317/inputs.json:1471>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:504](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:504>).

    - **Получить actual same-item witness** `n-same-item-witness` — ! БЛОКЕР · НЕ ОПРЕДЕЛЕНО · НЕ ПРОВЕРЕНО в исполнении · P0
      Следующая проверка после safety admission связывает V→T/ctxbundle/pathdepth/currentPC/DR6/RAX/RCX/RDX; wrong first hit → UNKNOWN/stop.
      Проблема: Source aliases сами не устанавливают owning type того effective item, который реально попадает наC0.
      Готово, когда: После отдельной readiness получить один bounded owned witness с точной identity; не сканировать все hooks и не смешивать input/record/output IDs.
      Нужно раньше: `n-witness-safety`
      Источник: [legacy:output/ricon-full-development-graph-20261109/native-inventory/process-snapshot.json:1](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-full-development-graph-20261109/native-inventory/process-snapshot.json:1>).
      Источник: [reforger:output/native-resume-20261109/p-item-lookup/review.json:25](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/output/native-resume-20261109/p-item-lookup/review.json:25>).

    - **Owning type/vtable и одна C0 cell** `n-owning-type-cell` — ! БЛОКЕР · НЕ ОПРЕДЕЛЕНО · UNKNOWN · P0
      Exact owning type/vtable provenance того же effective item остаётся UNKNOWN; absolute cell/target не назначены.
      Проблема: Original item vs selected node и mutable aliases нельзя угадывать по shape/соседнему RTTI.
      Готово, когда: Только после exact identity квалифицировать одну current cell/target и её boundary; C8/E74350/A3B2E0 остаются parked без нового основания.
      Нужно раньше: `n-same-item-witness`
      Источник: [reforger:output/native-resume-20261109/p-item-lookup/SOURCE_FINDINGS_RU.md:43](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/output/native-resume-20261109/p-item-lookup/SOURCE_FINDINGS_RU.md:43>).
      Источник: [reforger:output/native-resume-20261109/item-c0-38e06317/inputs.json:1471](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/output/native-resume-20261109/item-c0-38e06317/inputs.json:1471>).

    - **Actual Radio descriptor и same-T после initializer** `n-radio-descriptor-map` — ! БЛОКЕР · УТВЕРЖДЕНО · UNKNOWN · P0
      Нужны positive joins authored30733/state/output12/currentP/control descriptor и sameT/lifetime. Source ordinal/cache ID не подменяют эти связи.
      Проблема: Пока Radio_V — source hint, а mutable hooks могут менять ctx/P/T.
      Готово, когда: Exact sender/listener/device/generation/window связывается с actual Radio branch; current[P+68] после initializer доказан в нужном consumer scope.
      Нужно раньше: `n-owning-type-cell`, `n-authored-radio-graph`, `n-consumer1084`
      Источник: [legacy:output/ricon-goal-plan-20261106/source-preregister/radio-output-mapping.json:2](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-goal-plan-20261106/source-preregister/radio-output-mapping.json:2>).
      Источник: [reforger:output/native-resume-20261109/same-t-delegate/SOURCE_FINDINGS_RU.md:40](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/output/native-resume-20261109/same-t-delegate/SOURCE_FINDINGS_RU.md:40>).
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:34](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:34>).

    - **Поддерживаемый private Radio writer/handle** `n-supported-radio-writer` — ! БЛОКЕР · НЕ ОПРЕДЕЛЕНО · НЕ ПРОВЕРЕНО в исполнении · P0
      Публичный Snd/SGM setter на другом объекте и raw pointer не являются production port. ApplySnapshot всё ещё fail-closed.
      Проблема: Native addressed writable capability не квалифицирована.
      Готово, когда: Подтвердить поддерживаемый writer к точному Radio instance и безопасный обратимый эффект; до этого BLOCKED_NO_ACTUATOR сохраняется.
      Нужно раньше: `n-radio-descriptor-map`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Network/RICON_NetworkContracts.c:367](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Network/RICON_NetworkContracts.c:367>).
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:36](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:36>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:358](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:358>).

    - **A: достаточная доставка исходных фрагментов** `n-a-delivery` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · runtime metadata / callback · P0
      Callbacks/own resource дают частичную native-path опору, но содержание, время и coverage будущих просветов не доказаны.
      Проблема: Range/discovery/q/codec могут необратимо потерять нужный фрагмент доRICON.
      Готово, когда: Независимо подтвердить known-source fragments/identity/time/coverage при native range/q и будущих допустимых улучшениях, без control-plane подмены.
      Нужно раньше: `n-capture05-own-chain`, `n-k0-effective-runtime`
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:32](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:32>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:242](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:242>).
      Источник: [legacy:output/ricon-goal-plan-20261106/guarded-cli-capture/capture10/review.json:5](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-goal-plan-20261106/guarded-cli-capture/capture10/review.json:5>).

    - **B: адресное Radio-only воздействие** `n-b-actuation` — ! БЛОКЕР · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P0
      Актуальный supported actuator отсутствует; whole-event gain не заменяет Radio-only управление и общийRX.
      Проблема: Source/cache/return0 ещё не дают реального воздействия на адресный звук.
      Готово, когда: Малая обратимая последовательность известного управления меняет только нужный Radio поток, сохраняет DIRECT/чужие пары/user states.
      Нужно раньше: `n-supported-radio-writer`, `n-capture05-own-chain`
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:34](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:34>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:248](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:248>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Network/RICON_NetworkContracts.c:367](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Network/RICON_NetworkContracts.c:367>).

    - **DIRECT и чужие потоки не затронуты** `n-direct-safety` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Обязателен контроль до/во время/после Radio-only управления; VON_RADIO содержит обе ветви.
      Проблема: Whole-event setter может затронуть Direct/другую пару или пользовательский выход.
      Готово, когда: Сравнить DIRECT/нецелевые A→C/поколения и user output state при управлении Radio, с восстановлением stock.
      Нужно раньше: `n-b-actuation`
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:254](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:254>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:353](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:353>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:437](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:437>).

    - **Native audio и информативность просветов** `n-audio-info-fades` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Полная цепь clean→mask→Q0+noise→исходный просвет→mask ещё не принята; callbacks/cache/RMS не содержание.
      Проблема: Риск принять декоративное окно/синтетическую речь за восстановленный исходный фрагмент.
      Готово, когда: До опыта задать фразы/окна/попытки/negative controls и независимую оценку содержания, происхождения и времени; измерить ложную информацию.
      Нужно раньше: `n-a-delivery`, `n-b-actuation`, `n-world-rx-calibration`, `n-clock-provenance`, `n-open-rx-background`
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:80](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:80>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:98](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:98>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:104](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:104>).

    - **Шум открытого RX без активного VoN** `n-open-rx-background` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Открытый приёмник имеет собственный фон/lifecycle даже без voice event; путь ещё требует отдельной native qualification.
      Проблема: Отсутствие VoN-события не означает power-off; шум нельзя дублировать на каждый голос или подменять его случайной речью.
      Готово, когда: Квалифицировать supported native background path, start/stop/rebind и независимость device/output; при нескольких TX фон учитывается один раз.
      Нужно раньше: `n-world-rx-calibration`
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:79](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:79>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:202](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:202>).
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:17](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:17>).

  - **05 · Время, общий RX и RECEIVER / SERVER** `temporal` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · P1
    Одна физическая модель и согласованные временные контракты для двух мест расчёта; общий приёмник и звук на слушающем клиенте.
    Проблема: Нужны CT10 migration, clock mapping, parity и общий RX без двойных потерь или шума на каждый голос.
    Готово, когда: Одинаковые входы, профили, состояние и время дают согласованный результат обоих режимов; LOST/Q0 не включают автоматический mute.
    Нужно раньше: `core`, `world`, `native`
    Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:14](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:14>).

    - **Отдельные fading/descriptor/cutoff вычислители** `d-temporal-kernel` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · unit / модель / WB · P1
      Есть изолированные расчётные kernels; они не готовый общий RX и не полный production канал.
      Проблема: Standalone parity нельзя объявить всей физикой сети.
      Готово, когда: Versioned coefficients/process/codec и строгая interval cutoff сохраняют свой ограниченный contract.
      Нужно раньше: `d-world-pure-layers`
      Источник: [reforger:docs/plan/STATUS_RU.md:692](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/STATUS_RU.md:692>).
      Источник: [reforger:output/project-study-20261109-v3/STUDY_RU.md:64](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/output/project-study-20261109-v3/STUDY_RU.md:64>).
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:24](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:24>).

    - **CT10: temporal producer→wire→receiver→port миграция** `d-temporal-ct10` — ○ ПЛАН · УТВЕРЖДЕНО · source / документ · P1
      Согласовать новую схему/совместимость и все consumers; hidden поля в scalar snapshot1 не допускаются.
      Проблема: Temporal descriptor ещё не проходит всю production цепочку.
      Готово, когда: Requirements/reference→producer/codec/copy→full/single receiver/resync→seam/fixtures мигрированы совместно.
      Нужно раньше: `d-core-publication`, `d-temporal-kernel`
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:48](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:48>).
      Источник: [reforger:output/project-study-20261109-v3/STUDY_RU.md:64](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/output/project-study-20261109-v3/STUDY_RU.md:64>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:364](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:364>).

    - **Одна физика RECEIVER/SERVER и общий RX/звук** `d-temporal-shared-rx` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Две схемы расчёта обязательны; PCM/DSP локален, interference и фон RX не умножаются на число голосов.
      Проблема: Server-only старый design не реализует современную receiver parity.
      Готово, когда: Одинаковые полные inputs/profile/time дают parity; channel/RX/device/output и protected interference раздельны.
      Нужно раньше: `d-temporal-ct10`, `d-native-actuator`
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:12](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:12>).
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:20](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:20>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:196](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:196>).

    - **FINITE_MULTIPATH_V1 kernel** `c-fading` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · source / документ · none
      Evaluate вычисляет coefficient из descriptor/time, не читает часы/PTT/epoch; builder/slow shadow/production calls не подключены.
      Готово, когда: Frozen numeric/immutability/domain checks только pure kernel; физическое происхождение coefficients отдельно.
      Нужно раньше: `c-contracts`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_ChannelFading.c:3](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_ChannelFading.c:3>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_ChannelFading.c:311](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_ChannelFading.c:311>).

    - **RCFD/1 codec, deep-copy и rebase** `c-descriptor` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · source / документ · none
      Bounded encode/decode/rebase проверяют структуру, не происхождение; producer/snapshot/Rpl/seam не мигрированы.
      Готово, когда: int32 phase/float32/order/roundtrip/deep-copy/late corruption и unsupported frames проверены атомарно.
      Нужно раньше: `c-fading`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_ChannelDescriptor.c:3](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_ChannelDescriptor.c:3>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_ChannelDescriptor.c:313](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_ChannelDescriptor.c:313>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_ChannelDescriptor.c:389](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_ChannelDescriptor.c:389>).

    - **P1 interval upper-bound cutoff** `c-cutoff` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · source / документ · none
      Ограничивает полезный вклад на интервале, не удаляет помеху, не задаёт LOST; production calls не подключены.
      Готово, когда: Coherent sum/weak paths и U<tolerance на всём interval; resource limit не объявляется physical zero.
      Нужно раньше: `c-contracts`, `c-path`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_ChannelCutoff.c:3](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_ChannelCutoff.c:3>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_ChannelCutoff.c:110](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_ChannelCutoff.c:110>).

    - **CT10 миграция всей цепи** `c-ct10` — ! БЛОКЕР · НЕ ОПРЕДЕЛЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Scalar snapshot v1 не переносит descriptor. Producer/registry/wire/publication/receiver/seam/fixtures меняются согласованно; конкретный wire contract draft.
      Проблема: Scalar snapshot v1 не переносит descriptor. Producer/registry/wire/publication/receiver/seam/fixtures меняются согласованно; конкретный wire contract draft.
      Готово, когда: Descriptor обновляется независимо от mean Q; resync/keepalive/epoch/deep-copy/schema не ломают continuity.
      Нужно раньше: `c-descriptor`, `c-snapshot`, `c-publication`, `c-receiver`, `c-binding`
      Источник: [reforger:docs/req/CT10-migration-map.md:3](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/req/CT10-migration-map.md:3>).
      Источник: [reforger:docs/req/CT10-migration-map.md:12](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/req/CT10-migration-map.md:12>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_LinkSnapshot.c:34](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_LinkSnapshot.c:34>).

    - **CT06 clocks и physical validity** `c-clock` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Synthetic server time и TTL не доказывают clock mapping клиента. Descriptor anchor/validity/uncertainty отделены от keepalive.
      Проблема: Synthetic server time и TTL не доказывают clock mapping клиента. Descriptor anchor/validity/uncertainty отделены от keepalive.
      Готово, когда: Проверенная clock relation/uncertainty, stale/future guards и continuity при PTT/epoch/разном шаге.
      Нужно раньше: `c-fading`, `c-epoch`
      Источник: [reforger:docs/req/REQ-channel-time.md:68](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/req/REQ-channel-time.md:68>).
      Источник: [reforger:docs/req/CT10-migration-map.md:32](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/req/CT10-migration-map.md:32>).
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:82](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:82>).

    - **CT10 и общая RECEIVER/SERVER интеграция** `n-ct10-mode-time` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Существующие Core/PairService/descriptor/codec/publication/receiver переиспользуются; snapshotv1 не получает скрытый новый смысл.
      Проблема: Один server-oriented source и temporal kernel не означают два полноценных режима или parity.
      Готово, когда: Новая typed/versioned схема, независимые векторы, producer→codec/copy→publication→receiver/resync→seam/port/fixtures согласованы; PCM локален.
      Нужно раньше: `n-goal-contract`, `n-supported-radio-writer`
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:48](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:48>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:360](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:360>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:472](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:472>).

    - **Реальный мир, общий RX и калибровка** `n-world-rx-calibration` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Нужны действительные terrain/material/antenna/profile inputs и один sharedRX; TEST_ONLY не production.
      Проблема: Недостающая геометрия не свободный путь; шум не размножается по голосам, защищённая помеха не раскрывается.
      Готово, когда: В утверждённой области проверить signal/noise/interference, AGC/capture/селективность/squelch/openRX и физическую/слуховую калибровку.
      Нужно раньше: `n-ct10-mode-time`, `n-b-actuation`
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:24](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:24>).
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:26](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:26>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:204](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:204>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:373](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:373>).

    - **Interval cutoff без потери будущих фрагментов** `n-cutoff-window` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · P1
      Контракт/ядро временного полезного коэффициента существуют отдельно; полная native integration/coverage не приняты.
      Проблема: LOST/Q0/ZERO_AT_SAMPLE нельзя превратить в drop оставшегося окна; сумма слабых путей значима.
      Готово, когда: Полная coverage/revisions/finite remaining-interval bound/численный guard и U<tolerance; равенство CONTINUE; underflow/INVALID не тишина.
      Нужно раньше: `n-ct10-mode-time`, `n-world-rx-calibration`, `n-clock-provenance`
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:40](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:40>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:268](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:268>).
      Источник: [reforger:AGENT_CONTEXT_RU.md:34](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/AGENT_CONTEXT_RU.md:34>).

    - **Ordering, copies, full resync и технический recovery** `n-fault-recovery` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Invalid/stale/future/unknown/late bad element не должны частично применить новое состояние или стать физическим RF-LOST.
      Проблема: Старый эффект/неполные данные могут затронуть не то поколение или переопределить корректный state.
      Готово, когда: Атомарный отказ/full resync, deep-copy и declared technical recovery проверены для обоих режимов без обхода прав.
      Нужно раньше: `n-ct10-mode-time`, `n-clock-provenance`, `n-b-actuation`
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:83](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:83>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:224](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:224>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:447](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:447>).

    - **Физические часы, native frames и окна** `n-clock-provenance` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Дискретные watcher timestamps/TTL не являются квалифицированным clock mapping или source-fragment timing.
      Проблема: PTT/lease/epoch/keepalive не должны reseed/продлевать physical validity; краткие окна нельзя потерять междуsnapshot.
      Готово, когда: Объявить и проверитьanchor/window/uncertainty/frame mapping/profile revisions и parity при разном порядке/шаге вызовов.
      Нужно раньше: `n-a-delivery`, `n-b-actuation`, `n-capture10-output`
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:42](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:42>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:226](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:226>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:232](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:232>).

  - **06 · Радиоустройства и управление игрока** `devices` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · P1
    Носимые SR/LR, стационарные и встроенные рации, питание, антенны, частоты, ключи, PTT и интерфейс управления.
    Проблема: Паспортные параметры и адаптер наблюдения не равны проверенной RF/RX-модели каждой рации.
    Готово, когда: Каждый поддерживаемый профиль имеет калибровку, работающие controls и lifecycle, проверенные на штатном голосе.
    Нужно раньше: `core`, `temporal`
    Источник: [reforger:docs/plan/ROADMAP_RU.md:288](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/ROADMAP_RU.md:288>).

    - **Device ownership/generation и безопасные каталоги** `d-device-identity` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · P1
      Статические contracts/adapter существуют; реальные equipment bindings/authority требуют приёмки.
      Проблема: PlayerId/entity/device/radio index нельзя смешивать при rebind.
      Готово, когда: Server validates ownership/access/catalogue; stale generation и чужой device не меняют доставку.
      Нужно раньше: `d-core-lifecycle-fixture`
      Источник: [reforger:docs/plan/PLAN_RU.md:210](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:210>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:234](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:234>).
      Источник: [reforger:output/project-study-20261109-v3/STUDY_RU.md:54](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/output/project-study-20261109-v3/STUDY_RU.md:54>).

    - **Э6: несколько носимых SR/LR только конфигами** `d-device-portable` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P2
      Программа требует минимум две SR и одну LR, presets/manual tuning и пересекающиеся сети.
      Проблема: Source adapter не является полным устройством игрока.
      Готово, когда: Конфигурациями добавляются ≥2SR/≥1LR, проверены частоты/ключи/несколько устройств и ≥3 endpoints.
      Нужно раньше: `d-device-identity`, `d-radio-vertical`
      Источник: [reforger:docs/plan/PLAN_RU.md:485](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:485>).
      Источник: [reforger:docs/plan/PLAN_RU.md:493](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:493>).

    - **Радио GUI, PTT и подсказки ошибок** `d-device-radio-ui` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P2
      UX/клавиши/доступность входят в DoD этапа, а не откладываются навсегда после core.
      Проблема: Нет принятого полного пользовательского tuning/PTT цикла.
      Готово, когда: Игрок настраивает каналы/пресеты, видит power/errors/PTT, проходит first-word test без debug console.
      Нужно раньше: `d-device-portable`
      Источник: [reforger:docs/plan/PLAN_RU.md:324](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:324>).
      Источник: [reforger:docs/plan/PLAN_RU.md:491](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:491>).
      Источник: [legacy:docs/WIKI_RU.md:597](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/docs/WIKI_RU.md:597>).

    - **Частота, полоса, модуляция и ключ сети** `d-device-frequency-model` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · P1
      Unified device concepts перенесены в target design; старое SR/LR деление не заменяет frequency compatibility.
      Проблема: Реальные units/native tuning и calibration не следуют из helpers A3.
      Готово, когда: Разрешённые catalogue значения, wrong frequency/key и границы полосы проверены на реальных устройствах.
      Нужно раньше: `d-device-identity`
      Источник: [reforger:docs/plan/PLAN_RU.md:488](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:488>).
      Источник: [legacy:docs/WIKI_RU.md:304](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/docs/WIKI_RU.md:304>).
      Источник: [reforger:docs/plan/STATUS_RU.md:689](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/STATUS_RU.md:689>).

    - **Питание, батарея, неисправность и squelch** `d-device-power` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Радио-состояние и output controls должны иметь отдельные причины, не legacy LOST mute.
      Проблема: Whole-device metadata не подтверждает per-pair DSP или полную модель батареи.
      Готово, когда: Power/mute/squelch/failure ведут себя независимо от физической разборчивости; repair/restore проверены.
      Нужно раньше: `d-device-portable`, `d-native-direct-controls`
      Источник: [reforger:docs/plan/PLAN_RU.md:490](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:490>).
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:18](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:18>).

    - **Inventory: drop/pickup/death/rebind/JIP** `d-device-world-lifecycle` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Физическое устройство и права должны переживать перемещение и смену владельца/тела.
      Проблема: Старый эффект может ошибочно попасть в новое поколение.
      Готово, когда: Drop/pickup, смерть/respawn/JIP и новое владение не оставляют stale voice/control.
      Нужно раньше: `d-device-portable`, `d-core-publication`
      Источник: [reforger:docs/plan/PLAN_RU.md:495](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:495>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:401](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:401>).

    - **Паспортные RF/RX-профили и prefab presets** `d-device-presets-catalog` — ○ ПЛАН · УТВЕРЖДЕНО · source / документ · P2
      Справочники A3 неполны; Reforger production catalogue требует настоящих параметров и calibration.
      Проблема: TEST_ONLY/ASSUMED нельзя подменить паспортным или полевым профилем.
      Готово, когда: Версионированные power/antenna/bandwidth/RX профили с источниками, units, диапазоном применимости.
      Нужно раньше: `d-world-calibration`
      Источник: [legacy:docs/WIKI_RU.md:590](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/docs/WIKI_RU.md:590>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:206](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:206>).
      Источник: [reforger:docs/plan/STATUS_RU.md:689](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/STATUS_RU.md:689>).

    - **EXT-1…5: контракт расширения без второго ядра** `d-device-extension-fixtures` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · unit / модель / WB · none
      Описаны compile/execution fixtures для новых band/model/network/pattern/evaluator; mock сети не означает готовую новую сеть.
      Проблема: Done — только contract/fixture сценарии, не спутниковая или сотовая поставка.
      Готово, когда: Пять независимых extension examples проходят finite/budget/schema правила без прямого доступа к VoN.
      Нужно раньше: `d-core-contracts`
      Источник: [reforger:docs/plan/STATUS_RU.md:621](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/STATUS_RU.md:621>).
      Источник: [reforger:docs/plan/PLAN_RU.md:210](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:210>).
      Источник: [reforger:docs/adr/ADR-002-core-contracts.md:54](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/adr/ADR-002-core-contracts.md:54>).

    - **Read-only native radio observation** `c-observation` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · none
      Reader проверяет authority/owner/transceiver и читает power/tuning/key; getters последовательны, не атомарны.
      Готово, когда: Реальное binding/generation/актуальность и key privacy проверены на настоящем endpoint.
      Нужно раньше: `c-authority`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Device/RICON_RadioDeviceAdapter.c:172](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Device/RICON_RadioDeviceAdapter.c:172>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Device/RICON_RadioDeviceAdapter.c:171](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Device/RICON_RadioDeviceAdapter.c:171>).

    - **Явный RF-профиль TX/RX** `c-rf-profile` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · none
      Поля MHz/Hz/dBm/dBi и sensitivity либо required SINR явные; допустимый provenance пока TEST_ONLY/ASSUMED.
      Готово, когда: Production profile имеет проверенные units/revision/provenance/антенну/полосу, синтетика не выдаётся за паспорт.
      Нужно раньше: `c-contracts`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Device/RICON_RadioDeviceAdapter.c:31](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Device/RICON_RadioDeviceAdapter.c:31>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Device/RICON_RadioDeviceAdapter.c:77](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Device/RICON_RadioDeviceAdapter.c:77>).
      Источник: [reforger:docs/req/REQ-radio-model-catalog.md:11](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/req/REQ-radio-model-catalog.md:11>).

    - **Native tuning units и antenna binding** `c-units` — ! БЛОКЕР · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      m_iTuningValue opaque; deviceId и antennaPos приходят извне. Getter frequency не доказывает единицы RF профиля.
      Проблема: m_iTuningValue opaque; deviceId и antennaPos приходят извне. Getter frequency не доказывает единицы RF профиля.
      Готово, когда: Квалифицированы native tuning→MHz, точка/ориентация антенны и prefab variant без догадок по имени.
      Нужно раньше: `c-observation`, `c-rf-profile`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Device/RICON_RadioDeviceAdapter.c:25](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Device/RICON_RadioDeviceAdapter.c:25>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Device/RICON_RadioDeviceAdapter.c:192](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Device/RICON_RadioDeviceAdapter.c:192>).
      Источник: [reforger:docs/req/REQ-radio-model-catalog.md:9](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/req/REQ-radio-model-catalog.md:9>).

    - **Сборка физического TX→RX query** `c-query` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · none
      Build проверяет profiles/context/power/tuning/channel/key, TX поля берёт у передатчика, RX у слушателя.
      Готово, когда: Mixed profiles и failure precedence соответствуют contract; device validity не превращается в audio cutoff.
      Нужно раньше: `c-observation`, `c-rf-profile`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Device/RICON_RadioDeviceAdapter.c:114](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Device/RICON_RadioDeviceAdapter.c:114>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Device/RICON_RadioDeviceAdapter.c:112](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Device/RICON_RadioDeviceAdapter.c:112>).

    - **Каталог устройств и права доступа** `c-device-access` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · none
      LifecycleDevice описывает device/channel/network/access, а не просто наличие prefab.
      Готово, когда: Drop/pickup/ownership/power не дают stale endpoint или несанкционированный lease.
      Нужно раньше: `c-contracts`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_Lifecycle.c:163](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_Lifecycle.c:163>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_Lifecycle.c:201](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_Lifecycle.c:201>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_Lifecycle.c:208](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_Lifecycle.c:208>).

    - **Диаграмма антенны и геометрическое gain** `c-antenna` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · none
      Pattern seam чистый; server должен разрешить направление и записать TX/RX gain. Isotropic не доказывает реальные устройства.
      Готово, когда: Проверенные orientation/polarization/frequency/gain; нет double-count при составлении query.
      Нужно раньше: `c-rf-profile`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_AntennaPattern.c:4](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_AntennaPattern.c:4>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_AntennaPattern.c:41](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_AntennaPattern.c:41>).

  - **07 · Станции, вышки и ретрансляторы M1** `infrastructure` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
    Стационарные станции, питание и ретрансляторы входят в первый выпуск «Радиосеть» и создают опору дальнейшей сотовой сети. Техника M2 показана отдельной ветвью после радиорелиза.
    Проблема: Штатные возможности ретрансляции и работы без владельца рядом требуют своей квалификации.
    Готово, когда: Для M1 приняты стационарные и релейные сценарии, питание, ownership, отсутствие петель и lifecycle.
    Нужно раньше: `devices`, `temporal`
    Источник: [reforger:docs/plan/ROADMAP_RU.md:288](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/ROADMAP_RU.md:288>).

    - **Э7a: Site/стационарная станция и мачта** `d-infra-site` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P2
      Наземные станции работают как общие объекты с stable ID; это основа relay и cellular sites.
      Проблема: Пользовательская инфраструктура пока не подтверждена runtime.
      Готово, когда: Станция функционирует без игрока рядом; site/config доступны редактору и корректно реплицируются.
      Нужно раньше: `d-device-portable`
      Источник: [reforger:docs/plan/PLAN_RU.md:501](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:501>).
      Источник: [reforger:docs/plan/ROADMAP_RU.md:292](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/ROADMAP_RU.md:292>).

    - **Антенны/направленность/генераторы/повреждения site** `d-infra-antenna-power` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Site имеет физическую антенну и питание; обрыв маршрута обосновывается реальным состоянием.
      Проблема: Магический radius и неуязвимая вышка не соответствуют плану.
      Готово, когда: Повреждение/отключение антенны или питания меняет валидность пути; pattern/profile и JIP согласованы.
      Нужно раньше: `d-infra-site`, `d-device-presets-catalog`
      Источник: [reforger:docs/plan/PLAN_RU.md:501](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:501>).
      Источник: [reforger:docs/plan/PLAN_RU.md:512](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:512>).
      Источник: [legacy:docs/WIKI_RU.md:1062](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/docs/WIKI_RU.md:1062>).

    - **Э7r: native RelayTransceiver capability gate** `d-infra-relay-capability` — ! БЛОКЕР · НЕ ОПРЕДЕЛЕНО · source / документ · P1
      Ретранслятор обязателен для M1; наличие класса не доказывает корректное переизлучение player voice.
      Проблема: Один из критических release gates M1, не удаляется из scope при FAIL.
      Готово, когда: Qualified native relay либо законная in-mod техника с отдельным evidence; при gap явное решение, не скрытый drop.
      Нужно раньше: `d-infra-site`, `d-native-baseline`
      Источник: [reforger:docs/plan/PLAN_RU.md:503](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:503>).
      Источник: [reforger:docs/plan/PLAN_RU.md:93](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:93>).
      Источник: [reforger:docs/plan/PLAN_RU.md:295](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:295>).

    - **Двухногий маршрут relay без утечки и циклов** `d-infra-relay-fidelity` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Сеть должна доставлять только по допустимому маршруту; hidden RX с расширенным TX range отвергнут для relay.
      Проблема: Исторический min(legs) не заменяет новые channel/RX/content критерии.
      Готово, когда: Uplink/downlink, no outsider leakage, route TTL/no cycles, два входа, power failure и JIP; no-LOST-drop согласован с современной Goal.
      Нужно раньше: `d-infra-relay-capability`, `d-infra-antenna-power`, `d-temporal-shared-rx`
      Источник: [reforger:docs/plan/PLAN_RU.md:505](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:505>).
      Источник: [reforger:docs/plan/PLAN_RU.md:508](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:508>).
      Источник: [reforger:docs/plan/PLAN_RU.md:511](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:511>).
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:18](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:18>).

  - **08 · Multiplayer, безопасность и восстановление** `multiplayer` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
    Authority сервера, JIP, respawn, disconnect, смена устройств, ordering, stale, resync и восстановление без утечек чужого содержания.
    Проблема: Полная матрица native/MP/lifecycle и измерение нагрузки ещё требуют приёмки.
    Готово, когда: Контрольные сценарии проходят на нужных клиентах/сервере с раздельными метриками и проверенной очисткой.
    Нужно раньше: `temporal`, `devices`
    Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:386](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:386>).

    - **Э1b/Э5: несколько настоящих endpoints и content controls** `d-mp-radio-acceptance` — ! БЛОКЕР · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Нужен qualified DS/host с A/C→B, совместимостью частоты/ключа и независимым audible/content proof.
      Проблема: Один клиент/AI proxy/synthetic fixture не равен многоклиентской приёмке.
      Готово, когда: Multi-human либо проверенный multi-instance, known source audio и CONTROL/AUDIO/content scopes пройдены отдельно.
      Нужно раньше: `d-native-delivery-a`, `d-native-actuator`
      Источник: [reforger:docs/plan/PLAN_RU.md:274](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:274>).
      Источник: [reforger:docs/plan/PLAN_RU.md:478](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:478>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:347](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:347>).

    - **Реальные JIP/respawn/reconnect/recovery** `d-mp-lifecycle-recovery` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Нужны engine callback/authority проверки поверх уже имеющихся pure lifecycle machines.
      Проблема: Pure parity не подтверждает full resync и владение в реальной сессии.
      Готово, когда: JIP/respawn×3/disconnect during PTT/stale replay/drop/rebind и atomic восстановление без чужого эффекта.
      Нужно раньше: `d-device-world-lifecycle`, `d-mp-radio-acceptance`
      Источник: [reforger:docs/plan/PLAN_RU.md:429](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:429>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:399](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:399>).

    - **Э10: согласованные профили 30/32/60 и soak** `d-mp-scale` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Исторический общий план:30 acceptance/60 stress; актуальная radio Goal требует сценарий32. Результаты отсутствуют.
      Проблема: Численные сценарии разных редакций нельзя незаметно слить в один PASS.
      Готово, когда: Зафиксированы versioned сценарии: обязательные32 радио и scoped30/60 milestones, CPU/network/audio/memory,4h soak/штормы и bounded state.
      Нужно раньше: `d-mp-lifecycle-recovery`, `d-temporal-shared-rx`
      Источник: [reforger:docs/plan/PLAN_RU.md:562](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:562>).
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:60](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:60>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:412](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:412>).

    - **Session epoch и link sequence** `c-epoch` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · none
      Process-local epoch и per-link seq — transport ordering, не часы/seed физической среды.
      Готово, когда: Overflow/restart/stale sequence не сбрасывают физическую фазу; новое поколение различается явно.
      Нужно раньше: `c-authority`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_CommsManagerComponent.c:61](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_CommsManagerComponent.c:61>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_CommsManagerComponent.c:290](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_CommsManagerComponent.c:290>).
      Источник: [reforger:docs/req/REQ-channel-time.md:46](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/req/REQ-channel-time.md:46>).

    - **Player/generation/entity state machine** `c-lifecycle` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · none
      Pure connected/registered/entity/disconnect и tombstones существуют; это не подписка на engine events.
      Готово, когда: Contract traces сохраняют generation/tombstone semantics; MP/JIP/respawn доказываются отдельно.
      Нужно раньше: `c-device-access`, `c-epoch`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_Lifecycle.c:421](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_Lifecycle.c:421>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_Lifecycle.c:534](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_Lifecycle.c:534>).
      Источник: [reforger:docs/req/REQ-lifecycle.md:15](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/req/REQ-lifecycle.md:15>).

    - **PTT и transmit leases** `c-lease` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · none
      ProcessPtt/Start/StopTransmit задают разрешение/жизненный цикл, не fading среды.
      Готово, когда: Start/stop/renew/deny и потерянный stop очищаются TTL без подмены RF качества.
      Нужно раньше: `c-lifecycle`, `c-device-access`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_Lifecycle.c:650](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_Lifecycle.c:650>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_Lifecycle.c:837](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_Lifecycle.c:837>).

    - **Replay/rate/cached responses** `c-replay` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · none
      Fingerprint/cache/rate entries дают idempotence и rejection stale client generations.
      Готово, когда: Дубли, reorder, stale generation, rate bounds и повторный ответ проверены по trace contract.
      Нужно раньше: `c-lifecycle`, `c-lease`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_Lifecycle.c:293](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_Lifecycle.c:293>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_Lifecycle.c:368](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_Lifecycle.c:368>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_Lifecycle.c:800](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_Lifecycle.c:800>).

    - **Expiry и эффект VoN cleanup** `c-expiry` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · none
      Sweep/TerminateLease создают эффекты, включая VON_EXPIRY_INTENDED; effect не означает applied native voice state.
      Готово, когда: TTL/disconnect/rebind очищают только текущий lease, сохраняя unrelated user/stream state.
      Нужно раньше: `c-lease`, `c-replay`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_Lifecycle.c:925](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_Lifecycle.c:925>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_Lifecycle.c:1355](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_Lifecycle.c:1355>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Network/RICON_NetworkContracts.c:370](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Network/RICON_NetworkContracts.c:370>).

    - **Engine lifecycle/RPC/JIP wiring** `c-engine-life` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Pure автомат требует действительных engine adapters/events/Rpl и проверки handle reuse.
      Проблема: Pure автомат требует действительных engine adapters/events/Rpl и проверки handle reuse.
      Готово, когда: Native connect/disconnect/unpossess/respawn/JIP/drop/pickup/rebind не оставляют stale authority/binding.
      Нужно раньше: `c-lifecycle`, `c-expiry`
      Источник: [reforger:docs/req/REQ-lifecycle.md:15](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/req/REQ-lifecycle.md:15>).
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:84](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:84>).

    - **Scalar snapshot v1** `c-snapshot` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · none
      Schema1: epoch/link/sequence/TTL/scalar fields и CloneSnapshot; temporal arrays отсутствуют.
      Готово, когда: v1 compare/clone/TTL воспроизводимы; новая schema не подменяет старый смысл молча.
      Нужно раньше: `c-epoch`, `c-pair`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_LinkSnapshot.c:34](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_LinkSnapshot.c:34>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_LinkSnapshot.c:87](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_LinkSnapshot.c:87>).

    - **Pure publication и registry** `c-publication` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · none
      NeedSnapshot/Publish формируют scalar registry/snapshots по state/Q/TTL/resync; сеть отсутствует в самом сервисе.
      Готово, когда: seq/overflow/full resync/remove/keepalive детерминированы; temporal change triggers добавляются CT10.
      Нужно раньше: `c-snapshot`, `c-pair`, `c-epoch`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_LinkPublication.c:549](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_LinkPublication.c:549>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_LinkPublication.c:566](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_LinkPublication.c:566>).

    - **Snapshot receive и atomic full resync** `c-receiver` — ◐ ЧАСТИЧНО · УТВЕРЖДЕНО · source / документ · none
      Pure consumer проверяет epoch/order/schema/TTL и visibility; не аудио renderer.
      Готово, когда: Late-invalid массив целиком отвергается; stale/duplicate не откатывают принятый state.
      Нужно раньше: `c-snapshot`
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_LinkSnapshot.c:120](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_LinkSnapshot.c:120>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_LinkSnapshot.c:161](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_LinkSnapshot.c:161>).
      Источник: [reforger:addons/RICON_Core/Scripts/Game/RICON/Core/RICON_LinkSnapshot.c:187](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/addons/RICON_Core/Scripts/Game/RICON/Core/RICON_LinkSnapshot.c:187>).

    - **Snapshot Rpl/wire и recovery** `c-wire` — ○ ПЛАН · НЕ ОПРЕДЕЛЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Production snapshot serialization/RPC в прочитанном Core не найден; отдельный RCFD codec не является общей сетевой доставкой.
      Проблема: Production snapshot serialization/RPC в прочитанном Core не найден; отдельный RCFD codec не является общей сетевой доставкой.
      Готово, когда: Versioned bounded wire, destination authority, loss/reorder/JIP/resync и network metrics на engine.
      Нужно раньше: `c-publication`, `c-receiver`, `c-ct10`, `c-engine-life`
      Источник: [reforger:docs/req/CT10-migration-map.md:19](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/req/CT10-migration-map.md:19>).
      Источник: [reforger:docs/req/REQ-lifecycle.md:27](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/req/REQ-lifecycle.md:27>).

    - **A→B/A→C и A/C→B общий RX** `n-addressed-multi-radio` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Нужно независимое качество слушателей и совместная интерференция нескольких TX; текущий один pair не полная MP-приёмка.
      Проблема: Last-writer-wins и независимые шумы по потоку не физический sharedRX.
      Готово, когда: Разные адресные воздействия и protected competitor energy проверены на нескольких устройствах/передачах; нет двойного шума.
      Нужно раньше: `n-a-delivery`, `n-b-actuation`, `n-world-rx-calibration`
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:354](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:354>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:435](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:435>).
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:81](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:81>).

    - **Lifecycle, late join, respawn, rebind** `n-lifecycle-mp` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Нужны native MP и generation/lease/handle reuse; K0/capture cleanup metadata не закрывают lifecycle.
      Проблема: Устаревший эффект или source binding может перейти новому владельцу/устройству.
      Готово, когда: Проверить late join/leave/respawn/drop/pickup/rebind, movingTX route cache и hosted unpossess1.9 без stale effect и потериlocalcapture.
      Нужно раньше: `n-addressed-multi-radio`, `n-fault-recovery`, `n-clock-provenance`
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:84](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:84>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:399](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:399>).

    - **Frequency/device/rights/power/mute/squelch** `n-controls-rights` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Физический канал, device state, права и output state должны различаться в реальных отрицательных случаях.
      Проблема: Плохая разборчивость сама не разрешает mute/drop; технический отказ не RF-качество.
      Готово, когда: Проверить wrong-frequency/device/source/permissions, power/mute/squelch и защищённого конкурента с правильными отдельными причинами.
      Нужно раньше: `n-b-actuation`, `n-world-rx-calibration`
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:17](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:17>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:394](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:394>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:438](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:438>).

  - **09 · Первый рабочий выпуск «Радиосеть»** `radio-release` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P0
    Отдельный проверяемый рубеж: рабочий радио-мод, станции и ретрансляторы M1, расширяемый API, документация, упаковка и известные пределы.
    Проблема: Радиорелиз зависит от native A/B, инфраструктуры M1, звуковой и сетевой приёмки; наличие исходников не закрывает релиз.
    Готово, когда: Все обязательные критерии полной радиоцели и первого выпуска M1 выполнены; поставка воспроизводима и ограничения названы.
    Нужно раньше: `native`, `temporal`, `world`, `devices`, `infrastructure`, `multiplayer`
    Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:512](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:512>).
    Источник: [reforger:docs/plan/ROADMAP_RU.md:288](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/ROADMAP_RU.md:288>).

    - **Э5: первая рабочая носимая рация** `d-radio-vertical` — ! БЛОКЕР · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P0
      Одна SR, два канала, PTT/HUD/логи и сохранённый native direct — ещё не принятый сквозной продукт.
      Проблема: Число source/compile успехов не закрывает end-to-end voice.
      Готово, когда: A/C→B, near/far/wrong channel, physical poor channel/noLOSTmute, voice content и lifecycle прошли на native пути.
      Нужно раньше: `d-core-pair-service`, `d-native-delivery-a`, `d-native-actuator`, `d-temporal-ct10`
      Источник: [reforger:docs/plan/PLAN_RU.md:477](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:477>).
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:78](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:78>).

    - **M1: полная Радиосеть с обязательным relay** `d-radio-release-gate` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Первый релиз включает portable+sites+relay и scoped hardening/Workshop; cellular остаётся следующим обязательным milestone.
      Проблема: M1 без relay не готовая радиосеть; M1 сам не полный radio+cellular продукт.
      Готово, когда: Все части M1, DIRECT/content/calibration и release/rollback gates подтверждены в объявленном домене.
      Нужно раньше: `d-device-portable`, `d-infra-relay-fidelity`, `d-mp-scale`, `d-release-workshop`
      Источник: [reforger:docs/plan/PLAN_RU.md:284](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:284>).
      Источник: [reforger:docs/plan/PLAN_RU.md:295](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:295>).
      Источник: [reforger:docs/plan/PLAN_RU.md:101](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:101>).

    - **Совместная функциональная native-матрица** `n-functional-matrix` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      A/B, речь/шум/фрагменты, DIRECT, оба режима, controls и lifecycle должны пройти совокупно. Один baseline не fullGoal.
      Проблема: Частный удачный опыт может скрыть незакрытые обязательные проверки.
      Готово, когда: Закрыть функциональные строки§15/§20 в заявленной области с независимым review; workload/load остаются отдельным gate.
      Нужно раньше: `n-audio-info-fades`, `n-direct-safety`, `n-addressed-multi-radio`, `n-lifecycle-mp`, `n-cutoff-window`, `n-controls-rights`
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:425](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:425>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:512](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:512>).
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:92](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:92>).

  - **10 · Сотовая сеть: регистрация, SMS, звонки, handover** `cellular` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
    Дальнейшая программа M3a/M3b: абоненты и номера, регистрация, SMS, голосовые вызовы и переход между вышками при движении.
    Проблема: Общий замысел утверждён; подробная спецификация и сквозная native-приёмка сотовой сети пока не предъявлены.
    Готово, когда: Два абонента регистрируются, обмениваются SMS и звонят через сеть; поддерживаемый handover и отказы проходят независимые сценарии.
    Нужно раньше: `infrastructure`, `radio-release`
    Источник: [reforger:docs/plan/ROADMAP_RU.md:301](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/ROADMAP_RU.md:301>).

    - **M3a: SubscriberRegistry и публичные номера** `d-cellular-subscriber` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Общая сотовая цель и номера одобрены; сервер отделяет internal SubscriberId от публичного номера.
      Проблема: Контракт CallSession/mock не является реализованной абонентской сетью.
      Готово, когда: Уникальность номеров, server ownership/access и spoof/replay проверки выполнены без раскрытия internal ID.
      Нужно раньше: `d-core-contracts`
      Источник: [reforger:docs/plan/PLAN_RU.md:517](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:517>).
      Источник: [reforger:docs/plan/PLAN_RU.md:742](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:742>).

    - **M3a: SIM capabilities и профиль оператора** `d-cellular-sim` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P2
      SIM prefab несёт возможности/профиль, но не уникальный subscriber ID; конкретная игровая реализация не проверена.
      Проблема: Нельзя переносить уникальные ID в тиражируемый prefab.
      Готово, когда: Смена/подмена SIM не подделывает серверную identity; capabilities отдельно от уникальной регистрации.
      Нужно раньше: `d-cellular-subscriber`, `d-device-identity`
      Источник: [reforger:docs/plan/PLAN_RU.md:518](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:518>).
      Источник: [reforger:docs/plan/PLAN_RU.md:521](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:521>).

    - **M3a: attach/serving cell и покрытие по бюджету** `d-cellular-attach` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Лучшая сота + hysteresis на общем core; покрытие следует из link budget, не фиксированного круга.
      Проблема: Legacy selectBestNode и примеры 3dB/2s не калиброванный production handover.
      Готово, когда: Attach/no-network/границы покрытия проверены; профили и hysteresis объявлены после qualification.
      Нужно раньше: `d-cellular-sim`, `d-infra-antenna-power`, `d-core-pair-service`
      Источник: [reforger:docs/plan/PLAN_RU.md:522](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:522>).
      Источник: [legacy:docs/WIKI_RU.md:1070](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/docs/WIKI_RU.md:1070>).
      Источник: [reforger:docs/plan/PLAN_RU.md:544](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:544>).

    - **M3a: SMS store-and-forward** `d-cellular-sms` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      SMS явно включены в owner-approved scope; это planned feature, не выдуманная готовая реализация.
      Проблема: Нет предъявленной реализации доставки/очереди телефона.
      Готово, когда: messageId/TTL/лимиты/dedupe, offline coverage→deferred delivery, reconnect, owner-only replication; текст не логируется.
      Нужно раньше: `d-cellular-subscriber`, `d-core-contracts`
      Источник: [reforger:docs/plan/PLAN_RU.md:523](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:523>).
      Источник: [reforger:docs/plan/PLAN_RU.md:537](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:537>).
      Источник: [reforger:docs/plan/PLAN_RU.md:742](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:742>).

    - **M3a: сервисные коды оператора** `d-cellular-service-codes` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P2
      Одобрен *100#→ответная SMS только аутентифицированному запросившему; это не voice call и не billing.
      Проблема: Privacy/code validation и локализация ответа не подтверждены runtime.
      Готово, когда: Server-validated code table/задержка, ответ лишь request owner, неверный код/спам/replay отклонены.
      Нужно раньше: `d-cellular-sms`
      Источник: [reforger:docs/plan/PLAN_RU.md:530](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:530>).
      Источник: [reforger:docs/plan/PLAN_RU.md:533](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:533>).
      Источник: [reforger:docs/plan/PLAN_RU.md:538](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:538>).

    - **M3a: телефон — номер/SMS/набор кода** `d-cellular-phone-ui` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P2
      Телефонный UI является частью этапа, но не отдельной уже готовой подсистемой.
      Проблема: Нет пользовательской поставки SMS/номерного интерфейса.
      Готово, когда: Игрок видит свой номер/сеть, отправляет/получает сообщения и коды без чужих скрытых links.
      Нужно раньше: `d-cellular-service-codes`
      Источник: [reforger:docs/plan/PLAN_RU.md:534](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:534>).
      Источник: [reforger:docs/plan/PLAN_RU.md:560](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:560>).

    - **M3b: paging/CallSession/admission голосового вызова** `d-cellular-calls` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Voice calls обязательны конечному Intent на том же ядре; control plane не равен audio realization.
      Проблема: Нужны capacity и защищённая маршрутизация двух сторон.
      Готово, когда: Вызов от набора до завершения с paging/auth/admission/no-route/timeouts и concurrent limits.
      Нужно раньше: `d-cellular-phone-ui`, `d-core-contracts`
      Источник: [reforger:docs/plan/PLAN_RU.md:542](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:542>).
      Источник: [reforger:docs/plan/PLAN_RU.md:550](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:550>).
      Источник: [reforger:docs/plan/PLAN_RU.md:287](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:287>).

    - **M3b: выбор supported native voice realization** `d-cellular-voice-candidate` — ! БЛОКЕР · ИДЕЯ · source / документ · P1
      H6 уникальная частота/ключ и отдельный phone transceiver — кандидат design, не подтверждённая production техника.
      Проблема: ADR-001 упомянут в плане; отдельный ADR-001 файл в проверенном docs/adr не найден.
      Готово, когда: Текущая native API/asset техника доказывает privacy/capacity/audio; кандидат принят отдельно и без нового backend.
      Нужно раньше: `d-cellular-calls`, `d-native-workshop-surface`, `d-temporal-shared-rx`
      Источник: [reforger:docs/plan/PLAN_RU.md:546](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:546>).
      Источник: [reforger:docs/plan/PLAN_RU.md:159](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:159>).
      Источник: [reforger:docs/plan/PLAN_RU.md:467](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:467>).

    - **M3b: handover/re-attach при движении и отказах** `d-cellular-handover` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Обязательны hysteresis, отсутствие ping-pong/дублей и fallback к соседней соте.
      Проблема: Ни исходная A3 best-node идея, ни radio cache не доказывают seamless call.
      Готово, когда: Смена соты и отказ питания/вышки сохраняют допустимый вызов; обрыв лишь по валидной причине/no-route/timeout.
      Нужно раньше: `d-cellular-voice-candidate`, `d-cellular-attach`
      Источник: [reforger:docs/plan/PLAN_RU.md:544](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:544>).
      Источник: [reforger:docs/plan/PLAN_RU.md:551](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:551>).
      Источник: [reforger:docs/plan/PLAN_RU.md:552](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:552>).

    - **M3a/M3b: privacy, capacity и replay/spam gates** `d-cellular-security` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Серверная модель доступа обязательна для номера, SMS и голоса; details implementation пока не проверены.
      Проблема: Общая идея сети не даёт доказанной защиты чужого вызова/номера.
      Готово, когда: SIM/номер spoof, перебор, replay/spam, чужой call, exhausted pool и reconnect/JIP имеют negative evidence.
      Нужно раньше: `d-cellular-calls`, `d-cellular-sms`
      Источник: [reforger:docs/plan/PLAN_RU.md:539](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:539>).
      Источник: [reforger:docs/plan/PLAN_RU.md:553](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:553>).

    - **Граница M3: не GSM/LTE/5G stack и не billing/data** `d-cellular-scope-boundary` — ✓ ВЫПОЛНЕНО в указанном scope · УТВЕРЖДЕНО · source / документ · none
      Утверждён игровой cellular функционал; data/billing исключены, 5G stack соответствие не заявлено.
      Проблема: Done — фиксация границы scope, а не готовая сотовая сеть.
      Готово, когда: В дереве не появляются придуманные throughput/billing/5G leaf milestones; позже они требуют нового решения.
      Нужно раньше: `d-migration-plan-scope`
      Источник: [reforger:docs/plan/PLAN_RU.md:540](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:540>).
      Источник: [reforger:docs/plan/PLAN_RU.md:548](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:548>).
      Источник: [legacy:docs/WIKI_RU.md:1095](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/docs/WIKI_RU.md:1095>).
      Источник: [legacy:docs/WIKI_RU.md:1100](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/docs/WIKI_RU.md:1100>).

  - **11 · Расширения M2 и полный выпуск платформы** `release` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · none
    Радиосвязь техники M2 после первого M1; затем финальная совместная приёмка радио и мобильной сети, API для других модов, обновления и поддержка.
    Готово, когда: Приняты расширения техники M2 и совокупная матрица радио/сотовой сети, совместимости, нагрузки и восстановления; публикация имеет отдельный допуск владельца.
    Нужно раньше: `cellular`, `infrastructure`, `multiplayer`, `radio-release`, `vehicle-extensions`
    Источник: [reforger:docs/plan/PLAN_RU.md:1](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:1>).
    Источник: [reforger:docs/plan/ROADMAP_RU.md:301](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/ROADMAP_RU.md:301>).

    - **M2 · Радиосвязь техники, интерком и динамики** `vehicle-extensions` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · none
      После первого выпуска «Радиосеть»: радиостанции в машинах, роли мест, внутренняя связь экипажа и динамики. Это отдельный последующий рубеж, не условие первого M1.
      Готово, когда: Проверены сценарии техники, интеркома и локальной слышимости, lifecycle и API расширения.
      Нужно раньше: `radio-release`
      Источник: [reforger:docs/plan/ROADMAP_RU.md:301](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/ROADMAP_RU.md:301>).

      - **M2: рации в технике, стойки и права мест** `d-infra-vehicle-rack` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P2
        После радиосети запланированы seat/rack и связь техника↔земля на том же core.
        Проблема: Оболочка radio device не подтверждает vehicle UX и access.
        Готово, когда: Сесть/выйти, сменить место/владельца, связь с землёй и JIP без stale ownership.
        Нужно раньше: `d-infra-site`, `d-device-world-lifecycle`, `radio-release`
        Источник: [reforger:docs/plan/PLAN_RU.md:510](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:510>).
        Источник: [reforger:docs/plan/PLAN_RU.md:514](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:514>).

      - **M2: интерком экипажа** `d-infra-intercom` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
        IntercomTransceiver или иной законный in-mod путь требует собственного capability proof.
        Проблема: H6 остаётся кандидатом, не принятой реализацией intercom.
        Готово, когда: Crew-only full call/audio path, seat lifecycle, privacy и concurrency подтверждены.
        Нужно раньше: `d-infra-vehicle-rack`, `d-native-workshop-surface`
        Источник: [reforger:docs/plan/PLAN_RU.md:510](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:510>).
        Источник: [reforger:docs/plan/PLAN_RU.md:386](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:386>).
        Источник: [reforger:docs/plan/PLAN_RU.md:467](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:467>).

      - **M2: динамики и локальная слышимость техники** `d-infra-speakers` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P2
        Динамики перечислены отдельной частью vehicle milestone; собственный output lifecycle ещё нужен.
        Проблема: Нельзя считать динамик реализованным по наличию intercom interface.
        Готово, когда: Выход в заданной области/громкости и доступ не раскрывают закрытый канал; seat/power/JIP корректны.
        Нужно раньше: `d-infra-intercom`, `d-native-direct-controls`
        Источник: [reforger:docs/plan/PLAN_RU.md:510](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:510>).
        Источник: [reforger:docs/plan/ROADMAP_RU.md:303](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/ROADMAP_RU.md:303>).

    - **Э9: публичный SDK и sample addon** `d-release-sdk` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P2
      Документированные prefab/config/event APIs и compatibility входят в M2/M3, не только внутренние fixture seams.
      Проблема: Проверенный EXT mock не доказывает пригодность SDK стороннему мододелу.
      Готово, когда: Независимый мододел добавляет sample по инструкции; schema/API semver и полномочия интерфейса определены.
      Нужно раньше: `d-device-extension-fixtures`, `d-radio-vertical`
      Источник: [reforger:docs/plan/PLAN_RU.md:555](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:555>).
      Источник: [reforger:docs/plan/PLAN_RU.md:559](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:559>).

    - **Админ-диагностика, локализация и error объяснения** `d-release-admin-ux` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P2
      Нужны correlation-id reason paths, admin controls и UI без раскрытия чужих скрытых links.
      Проблема: Debug console и технические logs не готовый пользовательский UX.
      Готово, когда: Админ объясняет почему не слышно, игрок получает локализованную причину; приватные связи не раскрываются.
      Нужно раньше: `d-release-sdk`, `d-device-radio-ui`
      Источник: [reforger:docs/plan/PLAN_RU.md:557](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:557>).
      Источник: [reforger:docs/plan/PLAN_RU.md:560](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:560>).

    - **Э11: Workshop/semver/manifests и clean install** `d-release-workshop` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Поставка только через Workshop, server presets, CI/package/DS-smoke, LICENSE/NOTICE и rollback ещё требуют release evidence.
      Проблема: Сборка source и локальный addon не равны публичному release.
      Готово, когда: Clean commit→versioned artifact/hash manifest→clean client install, rollback и живые этапные сессии; публикация отдельно разрешена.
      Нужно раньше: `d-migration-provenance`, `d-radio-vertical`
      Источник: [reforger:docs/plan/PLAN_RU.md:573](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:573>).
      Источник: [reforger:docs/plan/PLAN_RU.md:579](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:579>).
      Источник: [reforger:docs/plan/ROADMAP_RU.md:299](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/ROADMAP_RU.md:299>).

    - **Э10: RPC fuzz, hostile client и bounded recovery** `d-release-security` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      R-04/R-05, forged owner/device/power/PTT/replay и delivery bypass требуют самостоятельного hardening.
      Проблема: Server authority declaration не доказательство защищённой доставки.
      Готово, когда: Негативные RPC/fuzz и cache storms не раскрывают голос и не создают неограниченное состояние; review/rollback есть.
      Нужно раньше: `d-device-identity`, `d-mp-radio-acceptance`
      Источник: [reforger:docs/plan/PLAN_RU.md:563](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:563>).
      Источник: [reforger:docs/plan/PLAN_RU.md:569](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:569>).

    - **Полностью рабочая radio+cellular платформа** `d-release-full-platform` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P1
      Конечный Intent включает M1+M2+M3a+M3b на общем расширяемом ядре; текущая native radio Goal — необходимая часть, не весь финал.
      Проблема: Нельзя закрыть общую программу одним Radio port или количеством выполненных study tasks.
      Готово, когда: Радиосеть/relay, техника, номера/SMS/servicecodes, voice calls/handover, SDK/security/perf/Workshop прошли отдельные gates; непроверенные details помечены.
      Нужно раньше: `d-radio-release-gate`, `d-infra-intercom`, `d-infra-speakers`, `d-cellular-handover`, `d-cellular-security`, `d-release-sdk`, `d-release-admin-ux`, `d-release-workshop`, `d-release-security`
      Источник: [reforger:docs/plan/PLAN_RU.md:98](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:98>).
      Источник: [reforger:docs/plan/PLAN_RU.md:280](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:280>).
      Источник: [reforger:docs/plan/PLAN_RU.md:742](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:742>).

    - **Идея расширения: satellite NetworkType/CallSession** `d-extension-satellite` — ○ ПЛАН · ИДЕЯ · НЕ ПРОВЕРЕНО в исполнении · P3
      Спутниковая сеть указана как контрактный пример общего ядра; не включена как обязательный готовый M3 продукт.
      Проблема: Mock extension нельзя обозначить satellite service implementation.
      Готово, когда: Отдельные network/device/physics/voice/authorization требования и приёмка при новом принятом scope.
      Нужно раньше: `d-release-sdk`, `d-cellular-calls`
      Источник: [reforger:docs/plan/PLAN_RU.md:191](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:191>).
      Источник: [reforger:docs/plan/PLAN_RU.md:224](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:224>).

    - **Идея расширения: проводной evaluator** `d-extension-wired` — ○ ПЛАН · ИДЕЯ · НЕ ПРОВЕРЕНО в исполнении · P3
      Wired seam и fixture-заглушка предусмотрены архитектурой; проводная сеть не условие нынешней полной radio Goal.
      Проблема: Поддержка интерфейса не доказывает работающую проводную связь.
      Готово, когда: Свой согласованный device/network/lifecycle contract и живой сценарий, без if-wired веток в общем ядре.
      Нужно раньше: `d-release-sdk`
      Источник: [reforger:docs/plan/PLAN_RU.md:194](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:194>).
      Источник: [reforger:docs/plan/PLAN_RU.md:226](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/PLAN_RU.md:226>).
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:62](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:62>).

    - **Нагрузка 32 пользователя и раздельные бюджеты** `n-load32` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P2
      Измерения CPU/channel/RX/control network/nativevoice/PCM/memory ещё требуются; source/model fixtures не load proof.
      Проблема: Изменять физический смысл/выключать расчёт ради нагрузки нельзя.
      Готово, когда: Воспроизводимый32-user сценарий с объявленной долейTX/числомRX/кандидатов и метриками; без pagefile/закрытиячужихapps.
      Нужно раньше: `n-functional-matrix`
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:60](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:60>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:412](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:412>).

    - **Поставка, rollback, диагностика и extension API** `n-release-package` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P2
      Нужны версииGame/Tools/mod/protocol, capability limits, выключение/возвратstock и повторнаяqualificationпослеupdate.
      Проблема: Неподтверждённая возможность не должна скрываться за APPLIED или фиксироваться как универсальный PASS.
      Готово, когда: Зафиксировать поддерживаемую область, integration/profile/nativeport API, recovery/rollback и наблюдаемые ограничения после полной матрицы/нагрузки.
      Нужно раньше: `n-load32`, `n-functional-matrix`
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:415](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:415>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:417](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:417>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:478](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:478>).

    - **После текущей Goal: SR/LR, relay, cellular/telephony** `n-future-networks` — ○ ПЛАН · УТВЕРЖДЕНО · НЕ ПРОВЕРЕНО в исполнении · P3
      Сотовые вышки, handover, телефонные соединения и проводная связь — отдельная дальняя программа. Не условие completion текущейVoN Goal и не её замена.
      Проблема: Нельзя сделать будущую телефонию готовой по текущим callback/source результатам или потерять её из полного product tree.
      Готово, когда: Отдельные требования/архитектура/приёмка на общих контрактах без второго Core/transport; детали проектирования пока не приняты.
      Нужно раньше: `n-release-package`, `n-ct10-mode-time`
      Источник: [reforger:docs/plan/GOAL_RICON_TEMPORAL_RU.md:62](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/GOAL_RICON_TEMPORAL_RU.md:62>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:418](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:418>).
      Источник: [reforger:docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:419](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md:419>).

## Покрытие всех 21 раздела радиоплана

- §1 Что должен получить игрок → `n-a-delivery`, `n-b-actuation`, `n-audio-info-fades`, `n-addressed-multi-radio`
- §2 Обязательные понятия: что нельзя смешивать → `n-goal-contract`, `n-radio-descriptor-map`, `n-capture10-output`, `n-controls-rights`
- §3 Качество: обязательная структура результата → `n-world-rx-calibration`, `n-ct10-mode-time`, `n-radio-descriptor-map`
- §4 Слышимость и отсутствие автоматической тишины → `n-open-rx-background`, `n-controls-rights`, `n-b-actuation`, `n-audio-info-fades`
- §5 Информативность и сохранность исходного сообщения → `n-a-delivery`, `n-audio-info-fades`, `n-marker-samepath`
- §6 Замирания звука и краткие улучшения → `n-audio-info-fades`, `n-cutoff-window`, `n-clock-provenance`
- §7 Кто выполняет расчёт → `n-ct10-mode-time`, `n-clock-provenance`, `n-addressed-multi-radio`
- §8 Два этапа обработки → `n-ct10-mode-time`, `n-world-rx-calibration`, `n-open-rx-background`
- §9 Устройства, среда и достоверность физики → `n-world-rx-calibration`, `n-controls-rights`
- §10 Время, данные и жизненный цикл → `n-clock-provenance`, `n-fault-recovery`, `n-lifecycle-mp`
- §11 Штатный VoN: две независимые возможности → `n-a-delivery`, `n-b-actuation`, `n-supported-radio-writer`, `n-owning-type-cell`
- §12 Сохранение фрагментов и допустимая оптимизация → `n-cutoff-window`, `n-a-delivery`, `n-fault-recovery`
- §13 Состояние проекта на момент записи плана → `n-legacy18-boundary`, `n-own-prefab-source`, `n-capture10-output`, `n-initializer2207`, `n-delegate838`, `n-lookup66`, `n-c0-input-freeze`, `n-witness-safety`
- §14 Последовательность разработки: этапы0–8 → `n-goal-contract`, `n-native-compile`, `n-b-actuation`, `n-ct10-mode-time`, `n-world-rx-calibration`, `n-lifecycle-mp`, `n-load32`, `n-release-package`
- §15 Обязательная матрица проверок → `n-functional-matrix`, `n-audio-info-fades`, `n-controls-rights`, `n-lifecycle-mp`, `n-load32`
- §16 Переиспользование существующего проекта → `n-ct10-mode-time`, `n-supported-radio-writer`, `n-fault-recovery`, `n-future-networks`
- §17 Наблюдаемость и доказательства → `n-watcher-race`, `n-capture10-output`, `n-clock-provenance`, `n-functional-matrix`, `n-load32`
- §18 Работа команды Agora → `n-goal-contract`, `n-witness-safety`, `n-c0-input-freeze`, `n-functional-matrix`
- §19 Текущие допуски и правила перехода между этапами → `n-goal-contract`, `n-witness-safety`, `n-same-item-witness`, `n-release-package`
- §20 Условие завершения полной Goal → `n-a-delivery`, `n-b-actuation`, `n-functional-matrix`, `n-world-rx-calibration`, `n-load32`, `n-release-package`
- §21 Основные документы и источники текущих решений → `n-goal-contract`, `n-initializer2207`, `n-delegate838`, `n-lookup66`, `n-c0-input-freeze`

## Сохранённые инварианты

- {"id":"inv.goal_scope","statement":"Все 21 раздел основного плана и этапы0–8 остаются обязательными для полной Goal; документационный граф не является новым roadmap или runtime-приёмкой.","source":{"root":"repo","path":"docs/plan/GOAL_RICON_TEMPORAL_RU.md","line":6,"sha256":"5d1ae1a7e830de0e5da8d70e6d8a9d01d689b6a9517bdefdcf67b65857ab4e10"}}
- {"id":"inv.no_lost_mute","statement":"LOST/LOWQ/Q0/нулевая разборчивость не создают mute, deny или запрет доставки; legacy scalar terminal state не переносится в live audio.","source":{"root":"repo","path":"docs/plan/GOAL_RICON_TEMPORAL_RU.md","line":17,"sha256":"5d1ae1a7e830de0e5da8d70e6d8a9d01d689b6a9517bdefdcf67b65857ab4e10"}}
- {"id":"inv.parity_local_pcm","statement":"RECEIVER/SERVER используют одну модель с одинаковыми полными входами и временем; authority прав серверная, PCM/DSP у слушателя.","source":{"root":"repo","path":"docs/plan/GOAL_RICON_TEMPORAL_RU.md","line":20,"sha256":"5d1ae1a7e830de0e5da8d70e6d8a9d01d689b6a9517bdefdcf67b65857ab4e10"}}
- {"id":"inv.direct_shared_rx","statement":"Radio-only воздействие сохраняет DIRECT, другие пары и user states; общий RX учитывает несколько источников и не умножает шум.","source":{"root":"repo","path":"docs/plan/GOAL_RICON_TEMPORAL_RU.md","line":26,"sha256":"5d1ae1a7e830de0e5da8d70e6d8a9d01d689b6a9517bdefdcf67b65857ab4e10"}}
- {"id":"inv.no_reseed","statement":"PTT/lease/linkSeq/epoch/resync/частота вызовов не задают физические фазы; snapshot TTL не продлевает physical validity.","source":{"root":"repo","path":"docs/plan/GOAL_RICON_TEMPORAL_RU.md","line":42,"sha256":"5d1ae1a7e830de0e5da8d70e6d8a9d01d689b6a9517bdefdcf67b65857ab4e10"}}
- {"id":"inv.cutoff_strict","statement":"Отсечка только при полной актуальной границе U<tolerance на весь интервал; равенство и отсутствие доказательства продолжают расчёт.","source":{"root":"repo","path":"docs/plan/GOAL_RICON_TEMPORAL_RU.md","line":40,"sha256":"5d1ae1a7e830de0e5da8d70e6d8a9d01d689b6a9517bdefdcf67b65857ab4e10"}}
- {"id":"inv.core_partial","statement":"FINITE_MULTIPATH_V1 — чистый полезный temporal kernel; текущий source прямо сообщает отсутствие builder/медленной тени/CT10/production вызовов. Это не полная RF/RX интеграция.","source":{"root":"repo","path":"addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_ChannelFading.c","line":1,"sha256":"21bc98df6713891da5be9dd1719b183be6c77e13d9741422d9a2b001832e9992"}}
- {"id":"inv.schema_partial","statement":"Текущий snapshot schema1 содержит scalar quality/state; объявленная CT10 migration не считается уже доставленной temporal/RX схемой.","source":{"root":"repo","path":"addons/RICON_Core/Scripts/Game/RICON/Core/RICON_LinkSnapshot.c","line":34,"sha256":"53d99d9710ddc1a4dc4617046cb2a3496e7768d2d76920a8a69a276caf39724c"}}
- {"id":"inv.no_actuator","statement":"В существующем VoN seam валидный ApplySnapshot заканчивается BLOCKED_NO_ACTUATOR; source/callback/cache наблюдения не дают APPLIED.","source":{"root":"repo","path":"addons/RICON_Core/Scripts/Game/RICON/Network/RICON_NetworkContracts.c","line":367,"sha256":"69f9fdaf905f962b65f35dffa599cb3dc90b614f9320306339c8c17cee4475d1"}}
- {"id":"inv.capture10_boundary","statement":"Capture10: пять bounded numeric OUTPUT-cache точек, ID5/8 ненулевые и ID12 присутствует со значением0; имя Radio_V остаётся source hint. Не доказаны mapping/инициализация/актуальная evaluation/consumer/DSP/mute/content/DIRECT/sharedRX/lifetime.","source":{"root":"repo","path":"docs/plan/STATUS_RU.md","line":5,"sha256":"9f91ad7bc9d6ed72081ec8743fa1d4c4b5d7b7b50875bf3a1a0d00f8d4f8ad47"}}
- {"id":"inv.input_output_distinct","statement":"Source API index, native parameter k, packed selection, INPUT record и cached OUTPUT ID — разные сущности. INPUT0 не означает AudioMute/LOST; present-zero отличается от отсутствия записи.","source":{"root":"repo","path":"docs/plan/STATUS_RU.md","line":5,"sha256":"9f91ad7bc9d6ed72081ec8743fa1d4c4b5d7b7b50875bf3a1a0d00f8d4f8ad47"}}
- {"id":"inv.machine_goal_separate","statement":"Последнее наблюдение machine Goal: blocked, причина UNKNOWN; user continue сохраняется. Project-state/историческая формулировка active не заменяют системное состояние и не создают разрешений.","source":{"root":"repo","path":"docs/plan/STATUS_RU.md","line":3,"sha256":"9f91ad7bc9d6ed72081ec8743fa1d4c4b5d7b7b50875bf3a1a0d00f8d4f8ad47"}}
- {"id":"inv.owner_and_scope","statement":"Baseline1 — условие probe, не запрос нового owner-допуска; положительный public source readback не доказывает propagation в private T.","source":{"root":"repo","path":"docs/plan/STATUS_RU.md","line":9,"sha256":"9f91ad7bc9d6ed72081ec8743fa1d4c4b5d7b7b50875bf3a1a0d00f8d4f8ad47"}}
- {"id":"inv.graph_navigation","statement":"Узел/ребро — навигация по указанному source и его scope; граф не переопределяет AGENTS, память, owner instructions, Goal или доказательство приёмки. Процент готовности без методики не вычисляется.","source":{"root":"repo","path":"docs/plan/native-integration-20261104/PLAN_RICON_NATIVE_INTEGRATION_RU.md","line":304,"sha256":"491d374180c3882b379a3bffa14481c2dddf810c29612c88b7af0974efd57290"}}

## Пределы карты

Исходники и evidence прочитаны для карты; прежние unit/Workbench/runtime результаты атрибутированы своим источникам и не перезапускались при построении. Это полный навигационный охват найденных этапов, а не утверждение, что в проекте не осталось неизвестной работы.

Цвета — важность проблем/рисков, а не проценты готовности. Полная продуктовая приёмка остаётся отдельным требованием.
