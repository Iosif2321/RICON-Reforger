# CT10 physical observation: qualification и actual pure SDK evidence

Дата: **2026-11-10**. **ROOT_REVIEWED_READY_FOR_CANONICAL_PUBLICATION**. Автор evidence — Documentation (`codex.team.docs`); canonical publisher — Root. Статус обозначает готовность пакета после Root review. Текущий итог: **285 observation / 2065 total checks PASS; OBSERVATION_QUALIFICATION_ONLY / ENFORCE_COMPILE_LOG_CONFIRMED / PURE_SCRIPTEDITOR_FIXTURES_PASS**. Отдельный historical baseline259/2039 сохранён ниже. Native world wrapper — **COMPILED_ONLY / WORLD_SAMPLING_NOT_RUN**. Полная Goal остаётся ACTIVE.

## Что опубликовано и что исполнялось

Root опубликовал typed request/stamp/witness/frame/result, injected source seam, bounded stateless sampler и отдельный read-only public-native adapter. Actual ScriptEditor выполнил sampler через именованный **TEST_ONLY source DOUBLE**, включая existing terrain sampling/provider helpers; это не чтение реального мира. Observation suite не создаёт `RICON_BaseWorldPhysicalObservationSource`.

Канонический repo: `C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger`. Documentation независимо пересчитала следующие SHA256 при первом чтении после baseline SDK run; все четыре тогда совпали с pins Root/Core. Это exact run pins165356, не утверждение о неизменности текущего fixture:

| Canonical файл | Строк | SHA256 |
|---|---:|---|
| `addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_PhysicalObservation.c` | 681 | `23a86d540a31302d51aa383bb19012536623642009dbfec1e62e3784b05319bf` |
| `addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_BaseWorldPhysicalObservation.c` | 237 | `3f0af441fe34e5d7476d838cfb5b90b99478b18f78dfe3dbaa0ae24484efe9ac` |
| `addons/RICON_CoreTests/Scripts/Game/RICON_Tests/RICON_PhysicalObservationFixtures.c` | 508 | `cac3b6cb71cb4d3bebd6855f5a3c8fe09c102c421f103bb41376b8c2c887f210` |
| `tools/RICON_TestRunner/Scripts/WorkbenchGame/RICON_TemporalIntegrationProbePlugin.c` | 53 | `2ace34bc87e1084d7425ac9136811177c00529e6ba69884a16c30fc0964231b1` |

**Отдельный current переход после baseline:** Root hash-guard опубликовал fixture `562307e16bf0a69f4ca6a3fade4960172daa8798beece87ec7464ea4cb641667`, **584 строки** (+76 insertions only; O19 stable known-zero, O20 ephemeral lifetime). Documentation независимо пересчитала current hash; остальные3source pins сохранились. Run2039 относится только к historical cac3 fixture. Новый fixture имеет собственный свежий SDK gate2065 ниже; результаты двух запусков не смешаны.

Probe:8–14 вызывает семь existing/new pure suites; :34–37 печатает observation counters и требует positive check count; :43–50 печатает done marker и вызывает `Workbench.Exit(0/5)`. Это ScriptEditor pure probe, не Play/world/server/voice flow.

## Baseline SDK: два compile refusal, затем259/2039 PASS

Все log paths ниже находятся в `C:/Users/Ded/Documents/My Games/ArmaReforgerWorkbench/logs/`; последний компонент — `console.log`. Их bytes независимо прочитаны и SHA проверены. Две первые попытки не являются successful execution evidence.

| Log directory | Наблюдение | SCRIPT(E) / donePASS | SHA256 |
|---|---|---:|---|
| `ricon-tests-physical-observation-initial-20261110-165148` | :142–145: override argument `e` отличается от prototype `endpoint`, fixture:89; Game compile failed | 2 / 0 | `4cefac326f0941fb2b170edcca377d2edb184597845a470ca1aa02ca0ae6fc3e` |
| `ricon-tests-physical-observation-override-fix-20261110-165259` | :144–147: `Formula too complex`, sampler:374; затем downstream vanilla diagnostics и Game compile failed | 17 / 0 | `894412dba83c10627be3f1aa92f87839f7153c7d81d9b4d2d869408634145666` |
| `ricon-tests-physical-observation-formula-fix-20261110-165356` | :157–158: WorkbenchGame/Game modules; :164–171: все семь групп и donePASS | 0 / 1 | `8959b94e0fb37df6ad5b96d82303f16ebf6d84b062e9bb5329bfb7b0721be707` |

Два narrow SDK fixes: `sdk-fixture-override-name-fix.patch` — alpha-rename `e→endpoint` и одна indentation-only правка; `sdk-witness-formula-fix.patch` — эквивалентная последовательность guards вместо длинной конъюнкции SameWitness. Десять сравнений identity/antenna/owner matrix/body status/velocity/body identity/provenance и их порядок сохранены; предшествующие шесть RadioObservation comparisons не менялись. Core SDK review проверил все1024 Boolean assignments и реконструкцию canonical source; это logical rewrite check, не Enforce interpreter/physics proof. Expected values/case groups не ослаблялись.

Baseline final log :164–171:

| Group | Checks | Failures | First |
|---|---:|---:|---:|
| snapshot | 57 | 0 | 0 |
| ordered | 40 | 0 | 0 |
| capture | 59 | 0 | 0 |
| lease_query | 67 | 0 | 0 |
| capture_batch | 61 | 0 | 0 |
| producer | 1496 | 0 | 0 |
| observation | 259 | 0 | 0 |
| **Total** | **2039** | **0** | все группы0 |

Baseline WorkbenchGame: **195 files / 673 classes / CRC32 `070025ea`**. Game: **5805 files / 11651 classes / CRC32 `5f519ae6`**. SCRIPT(E)=0, NULL-pointer reason count=0, donePASS count=1. **Process exit0 и runner exit0 наблюдены Root и переданы Documentation; один console.log не является самостоятельным process-exit receipt.** Native wrapper принимается компилятором в этом наборе модулей; native callbacks этим PASS не исполнялись.

После donePASS присутствуют `PLATFORM(E) RequestCurrentStats failed` (:172) и `RESOURCES(E)` UI font/texture leaks (:192–216). Утверждение «все категории ошибок в логе нулевые» не делается.

## Отдельный fresh supplemental gate:285/2065 PASS

Current fixture562307e получил Root/Core peer source acceptance и затем **actual fresh ScriptEditor run**, без изменения остальных3source files. Log: `C:/Users/Ded/Documents/My Games/ArmaReforgerWorkbench/logs/ricon-tests-physical-observation-lifetime-20261110-170412/console.log`; независимо прочитанный SHA256 **`34a49a24d19581449d50cf400e34f571f3a2ac0b95a9fa42d3ac86afd93cee5f`**.

Log :164–171: snapshot57, ordered40, capture59, lease_query67, capture_batch61, producer1496, **observation285** → **2065 checks, все failures0/first0**, SCRIPT(E)0, NULL-pointer reason0, donePASS1. Root наблюдал process/runner0 (session5661); log не подменяет exit receipt. WorkbenchGame195/673 CRC **070025ea**; Game5805/**11652** CRC **ca2c99e1** (:157–158). После PASS снова есть PLATFORM(E) :172 и UI RESOURCES(E) :192–216.

**Lineage:** cac3/508lines/259checks → только +76 inserted lines →562307e/584lines/285checks; **+26 assertions в двух новых cases**, прежние tests не ослаблены по independent source peer review. O19 проверяет stable known-zero body witness, отличный от unavailable, без вывода об antenna motion. O20 возвращает fresh unretained source objects и проверяет завершение callbacks/detached frame без provider roots: это actual pure script ownership/lifetime stress. Native engine handles/WorldTimestamp/radio/body/trace/height этим не исполнялись. Baseline2039 остаётся отдельным checkpoint; свежий2065 относится к current fixture и не включает более поздних кандидатов.

## Что именно квалифицировано

`Sample`:420 onward проверяет request/limits/terrain grid до provider callbacks, делает private input copies, сохраняет detached before witnesses, читает requested profile/first contact и повторно проверяет endpoints/stamps. После source-domain MeasureSpan идут final guards; любой отказ оставляет frame=null. Положительный terrain+trace case проверяет actual issued counts2stamp/4endpoint/3height/1trace. Historical cac3 fixture:495–508 вызывал пять observation groups; current562 fixture:570–584 добавляет KnownZeroBodyAndTransientOwnership к сохранённым positive, preflight, drift/late failure, span/large-clock и copy groups.

Выполненные случаи включают budgets, missing sources/nonfinite height, source/context/endpoint drift, defensive copies, exact source-domain span boundaries и большой raw float clock с независимым literal span. Double не исполняет WorldTimestamp.Compare/PlusMilliseconds. В baseline source outputs удерживаются strongly; current O20 отдельно выполняет unretained pure-source branch. Transient **native engine-object** lifetime по-прежнему не проверен. O19/O20 не приписаны historical259/2039; они учитываются только в fresh285/2065.

OK означает bounded observations: `atomicSnapshot=false`, `completePhysicalPaths=false`; terrain COMPLETE относится только к sampled profile. Один hit/nohit не даёт полного physical path set/EMPTY_MEDIUM. Antenna position остаётся CALLER_SUPPLIED; missing body velocity — MOTION_UNAVAILABLE, не stationary antenna. TEST_ONLY clock label остаётся PHYSICAL_CLOCK_UNQUALIFIED. Source revision — регистрационный pin, local identity strings — не verified Rpl/persistent physical identity.

## Основания review и предел приёмки

Прочитаны exact API, author note, reciprocal fixture review, Core SDK review и независимый Reviewer source review. Их абсолютная staging база: `C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ct10-physical-source-20261110/`.

- `core/API_RU.md`: `1f85ef31e4277a3f4a14c0276ae47823a49cb5a3c3024bb2011a9e8dbac43cc9` — frozen input/order/codes/provenance contract.
- `core/NOTE_RU.md`: historical source-only freeze `bbf8b5efeb0345aaaa6605b8413ceb35275c3b1325bff64f1f9bfb69c6b6f2c0`; не переписан задним числом в SDK acceptance.
- `core/TEST_REVIEW_RU.md`: `314ed51cc3ab432ab5ce2643f0ee6f3e13093ca7da42a02c73ca6dbb74ee73d2`; содержит разрешение historical WORLD/TEST_ONLY label ambiguity без ослабления assertions.
- `core/SDK_REVIEW_RU.md`: **ACCEPT_SCOPED_SDK_FIXES_AND_ACTUAL_PURE_LOG**, `b5d3c6232e8b2260bf5e3921f5284eac219aa47cd42c70d67dcda65f6564c2d2`; exact baseline4pins/165356 log checked.
- `review/REVIEW_RU.md`: independent pre-SDK **ACCEPT_SCOPED_OBSERVATION_QUALIFICATION_UNCOMPILED**, historical snapshot hash на момент чтения `c73b7018e0dc62653499d3947f0b85b0db280bb9829cc4167373d57016d8e1e7`. Он не обозначает current bytes mutable отчёта. Это исторический source verdict, не приписанный ему final SDK verdict; отдельный immutable capture этого старого snapshot здесь не заявлен.

Отдельный **Reviewer checkpoint :104–118 на момент fresh2065 review**: **ACCEPT_SCOPED_OBSERVATION_QUALIFICATION + PURE_SCRIPTEDITOR_FIXTURES_PASS** для fixture562307e/285/2065. Его SHA `61b73efa8c55846235e95c523cd13f8e54f4a8417c6ec302020f48797b9befe9` теперь относится к фактической immutable копии Root **`before/REVIEW_RU.2065-61b73efa.md`**, независимо hash-проверенной Documentation. Original mutable `review/REVIEW_RU.md` позднее дополнен contract review и при проверке имеет другой SHA `9eef4fdcd0b56443b5f8aec1c23ecf8f09b1671a0e87072494d5ac6ba0175b6d`; 61b hash ему не переприсваивается. Frozen checkpoint сохраняет прежние source/baseline verdicts как историю и подтверждает fresh log/pins без native world acceptance. Documentation независимо проверила тот же fresh log; новый Core fresh-SDK verdict сверх указанного baseline отчёта не выдумывается.

Первичные RF-основания и missing input matrix сохранены отдельно: [PRIMARY_INPUTS_RU.md](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ct10-physical-source-20261110/docs/PRIMARY_INPUTS_RU.md>), SHA `cd50ad831a9cf69f5f397f3e3be7a7094224f97b32dac6547cf7fa97301f0a67`. LOS/scalarQ/free-space loss не становятся K, calibrated fading или complete multipath из-за observation PASS.

**OPEN:** реальные radio/height/trace/body/opaque-clock samples; endpoint antenna mapping; frequency/material/RF calibration; full physical paths и multipath builder; persistent physical process/history и CF clock mapping; authoritative world exporter/native manager role; common RX, production SERVER/RECEIVER parity, RPC, voice/audio, load и полный product acceptance. Ни Play, ни world/server/microphone действий Documentation не выполняла.

Предыдущие1780 pure и legacy106 остаются отдельной историей; первые1780 pure checks вновь присутствуют в этом2039 run, **legacy106 в этой фазе не запускался**. Предыдущие evidence неизменны. Финальный Agy producer implementation/data audit остаётся pending explicit exact-data approval; отправки/обхода отказа нет, более ранний design review не повышен до code/SDK acceptance. Пакет evidence/receipt прошёл Root review и готов к canonical публикации; автор — Documentation, publisher — Root.
