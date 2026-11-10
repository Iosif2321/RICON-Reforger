# Узкая перепроверка official BI API
Дата чтения UTC: 2026-11-04 18:03:49 UTC; installed baseline **1.8.0.13**, без запуска SDK/engine.
**Результат: UNCHANGED_NO_SUPPORTED_PAIR_HOOK в доступной текущей official API документации; CURRENT_HEAD_REVISION_UNVERIFIED.**
Сравнение ограничено четырьмя классами и accepted [hooks report](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-von-integration-20261104/hooks/OPTIONS_RU.md>); полного поиска/RE не было.

| Текущий official класс | Проверенная поверхность и результат |
|---|---|
| [VoNComponent](https://community.bistudio.com/wikidata/external-data/arma-reforger/ArmaReforgerScriptAPIPublic/interfaceVoNComponent.html) | Capture/comm/TX, AI/editor функции; прежние OnCapture/OnReceive/OnVoNUsed. Pre-overlay authorization/population/final-q callback не объявлен. |
| [BaseTransceiver](https://community.bistudio.com/wikidata/external-data/arma-reforger/ArmaReforgerScriptAPIPublic/interfaceBaseTransceiver.html) | Device getters/setters и BeginTransmission/BeginTransmissionFreq для BaseRadioMessage; provider ordinary player voice отсутствует. |
| [ScriptedRadioComponent](https://community.bistudio.com/wikidata/external-data/arma-reforger/ArmaReforgerScriptAPIPublic/interfaceScriptedRadioComponent.html) | Собственные OnPostInit(IEntity), OnInit(IEntity), OnFrame(IEntity,float); inherited radio state methods/observers не принимают пару или mutable voice context. |
| [RadioManagerEntity](https://community.bistudio.com/wikidata/external-data/arma-reforger/ArmaReforgerScriptAPIPublic/interfaceRadioManagerEntity.html) | Только GetTransceiversInRange(vector,float,out notnull array<BaseTransceiver>); запрос списка не регистрирует замену native VoN population. |

Страницы имеют footer Doxygen **1.13.2**: это версия генератора, не доказательство game release/date или совместимости с installed. Дата публикации этих страниц не указана.
Последний локально закреплённый Script-Diff pin — **3d77cc212d5cda9922daf5f45635c7300d2d4cce** ([official manifest](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/von-re-20261104/index/input_sources/official_manifest.json>)); дата commit в manifest не записана. Он не объявляется текущим head.
Текущий GitHub commit/release подтвердить не удалось: web API inaccessible, прямой official API fallback блокирован socket sandbox, official commits/blob-main fallback DisabledError. Не повторяли failed routes; **непроверенный более новый head остаётся неизвестным**, а не отрицательно обследованным исходником.
Нового hook/signature/example/compatible release по доступным первичным источникам нет; недостаёт supported server binding до radio overlay для source-radio authorization, authoritative population и final q. Adapter не создавался.
[receipt.json](receipt.json) содержит URLs, время, method observations, revision limitations и ошибки; SHA256 **B909EFB0E1EF0AB11D6630F30C7C99582581379762EE1B090902F4F065EE5F6C** — хеш локальных нормализованных observations, не HTTP body hash. Нативный host исследует отдельная роль.
