# CT10: сбор и квалификация физических наблюдений

2026-11-10. **OBSERVATION_QUALIFICATION_ONLY / SELECTED_PURE_SCRIPTEDITOR_PASS**. Это контракт существующего Propagation-модуля, а не полный построитель радиоканала. [Evidence](../evidence/E-ct10-physical-observation-20261110.md) и [receipt](../evidence/receipts/ct10-physical-observation-result-20261110.json) разделяют исполнение pure fixtures и компиляцию native-адаптера. [Контракт producer](REQ-temporal-producer.md) сохраняется.

## Вход и доверие

`RICON_PhysicalObservationSampler.Sample(request, source, limits)` принимает два явно заданных физических endpoint, ожидаемые source ID/revision/timebase и отдельные запросы terrain/first-contact. Каждый endpoint содержит device ID, индекс TRX, конечную caller-supplied позицию и antenna source ID. Ключи двух endpoint различны; разные TRX одного устройства допустимы. Совпадающие позиции не принимаются.

`RICON_PhysicalObservationLimits` требует явных значений: `m_iMaxHeightSamples` 3..100000, `m_iMaxTraces` 0..1, `m_iMaxSpanMs` 0..60000. Grid/config/shape/лимиты проверяются до первого source callback. Resource limit означает отказ, а не усечение профиля или физический LOST. Запрошенный terrain использует существующие TerrainProfileSampling/WorldProfileProvider/HeightSource; неуспешные height calls тоже расходуют бюджет.

Source — доверенный локальный интерфейс чтения. Его injection, строковый ID и положительная revision не удостоверяют RPC authority, полноту мира или изменение физической среды. Revision native source — регистрационная; она не является RF environment revision или seed. Request и scalar limits копируются до callbacks, endpoint request — отдельно на каждый callback. Source не получает mutable caller request.

## API и владение

`RICON_PhysicalObservationSource` предоставляет `ReadStamp()`, `ReadEndpoint(endpoint)`, `GetHeightSource()`, `ReadFirstContact(tx,rx,flags,layerMask)` и `MeasureSpan(start,end,maxSpanMs,out elapsedMs)`. Base implementation не выдумывает данные: чтения возвращают null, span — `TIME_UNQUALIFIED`.

Порядок положительного sample: start stamp → TX/RX before → опциональный terrain → опциональный first contact → TX/RX after → end stamp → source MeasureSpan → финальные guards → frame. Ошибка прерывает чтения; принятый частичный frame не возвращается. Sampler удерживает issuing stamps и промежуточные объекты strong refs, немедленно копирует before witnesses и выдаёт detached output. Наружу передаются только primitive stamp headers, без private native timestamp/world/issuer handles.

До/после должны совпасть source/world/timebase/provenance/revision, radio/TRX identity, raw power/tuning/network key/позиция, owner transform, body identity/motion/provenance. Неизменные before/after — проверка наблюдаемого drift, а не доказательство атомарного чтения мира. Raw network key используется только для сравнения; диагностический probe не должен его печатать.

`RICON_PhysicalObservationResult` содержит `m_sCode`, nullable `m_Frame` и фактические counters stamp/endpoint/height/trace. Любой non-OK возвращает frame=null. Коды: `INVALID_LIMITS`, `INVALID_REQUEST`, `RESOURCE_LIMIT`, `SOURCE_UNAVAILABLE`, `CONTEXT_MISMATCH`, `INVALID_SOURCE_RESULT`, `ENDPOINT_UNAVAILABLE`, `TERRAIN_UNAVAILABLE`, `TRACE_UNAVAILABLE`, `SOURCE_CHANGED_DURING_SAMPLE`, `TIME_UNQUALIFIED`, `CLOCK_ROLLBACK`, `SPAN_EXCEEDED`, `OK`.

## Время и native-адаптер

`RICON_BaseWorldPhysicalObservationSource(world,sourceId,sourceRevision,timebaseId,txBinding,rxBinding)` копирует bindings, проверяет существующий authority guard, принадлежность owner миру и radio/TRX друг другу. Два device ID не могут превратить один и тот же actual radio/TRX в два физических endpoint. Native adapter ничего не регистрирует, не публикует и не запускается автоматически.

Native source читает public SDK через существующий radio reader, height source, `TraceMove`, owner Physics и `WorldTimestamp`. Installed SDK 1.9 принимает timestamp как `handle64` value. `MeasureSpan` проверяет issuer/world, непротиворечивость порядка и deadline через opaque `Compare`/`PlusMilliseconds`; `DiffMilliseconds` сохраняется как короткий diagnostic. Sampler дополнительно требует finite elapsed в 0..maxSpanMs даже после source OK. Абсолютные float world times не вычитаются. Предел0 требует равенства opaque timestamps, но не доказывает atomic snapshot.

Это не измерение точности часов, не соответствие CF model time и не SERVER/RECEIVER clock mapper. Native positive label — `WORLD_TIMESTAMP_DOMAIN_OBSERVED_PHYSICAL_CLOCK_UNQUALIFIED`; подставной TEST_ONLY source — `TEST_ONLY_SPAN_OBSERVED_PHYSICAL_CLOCK_UNQUALIFIED`. Literal DOUBLE span не доказывает native clock behaviour.

Raw `GetFrequency()` сохраняет непреобразованный tuning value: единица текущего getter не квалифицирована. Исторические три AN/PRC-68 точки другой версии не разрешают универсальный перевод или автоматическое деление на1000.

## Что означает положительный frame

Всегда `m_bAtomicSnapshot=false`, `m_bCompletePhysicalPaths=false`. Terrain COMPLETE относится только к полученному sampled profile. First-contact/no-hit не удостоверяет полноту лучей, отсутствие RF-препятствий, толщину стены или коэффициенты материалов. Trace flags дают только PARTIAL для соответствующих семейств, не COMPLETE. Неразрешённые поля не заменяются чистой средой.

Caller position не становится калиброванным фазовым центром антенны. `m_bHasBodyVelocity=false` вместе с пустым body token и `MOTION_UNAVAILABLE` означает неизвестное движение; vector0 в этом случае — placeholder. True с body witness и `BODY_AT_CALLER_POINT` допускает наблюдённый ноль тела в caller point, но не удостоверяет движение антенны или всей сцены.

Stable missing reasons: `ANTENNA_POSITION_CALLER_SUPPLIED`, `ANTENNA_MOTION_UNVERIFIED`, опциональные TX/RX `BODY_VELOCITY_UNAVAILABLE`, `FULL_PATH_SET_NOT_OBSERVED`, `RF_PROFILE_NOT_SAMPLED`, `CLOCK_MAPPING_NOT_QUALIFIED`. OK может сопровождаться этими пробелами; downstream не вправе объявлять полный физический input.

## Проверки и следующий gate

Manual O01–O20 fixtures проверяют конкретные source DOUBLE success/failure/drift/cap/copy/time/ownership случаи. 10.11 actual ScriptEditor:285 observation checks +1780 прежних =2065 PASS, Script(E)0, process/runner0; native source **COMPILED ONLY**. Предыдущий2039 run и первые два compile failures сохранены отдельно в receipt. Generic script ownership не доказывает lifetime engine handles.

Следующий необходимый факт — controlled actual world sample с двумя явными real radio bindings и source-domain timestamps. Его probe подготавливается и компилируется отдельно; запуск мира/Play требует отдельного допуска. Положительный world sample закроет только фактически наблюдённые поля и условия.

RF/antenna calibration, применимый профиль component amplitude/phase/delay/Doppler, persistent physical process, model validity/clock mapping и shared RX ещё OPEN. Нормировка относительных CF weights не поставляет абсолютное ослабление и не доказывает average power1; mean budget/noise/interference считаются отдельно. Здесь не создаются descriptor, второй Core, codec или voice backend; RCFD1, TTL/lease semantics и штатная голосовая цепочка сохраняются. Полная Goal M1/M2/M3, SERVER/RECEIVER parity, audio/content и нагрузка данным контрактом не закрываются.
