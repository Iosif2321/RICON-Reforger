# CT10 manual world probe: compile и pure guard evidence

2026-11-10. **READY_FOR_ROOT_REVIEW**. Автор evidence — Documentation (`codex.team.docs`), canonical publisher/operator — Root. Current result: **FULL_LIVEPROBE_ENFORCE_COMPILE_CONFIRMED / SHARED_PURE_GUARDS_175_PASS**. **Manual menu Run, native helper, actual manual-plugin CLI dispatch и world callbacks — NOT_RUN.** Это готовый review package, не заявление о canonical публикации или world permission. Полная Goal ACTIVE.

## Exact installed source и неизменённые parents

Root установил четыре новых файла в existing LiveProbe. Documentation независимо пересчитала pins из `MANUAL_PROBE_INSTALL.json` (SHA `14294f90fd40f78b5033a957026351e45a796e5aac44cf9ed55239dd99aff0c5`). Canonical root: `C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger`.

| Canonical path под `tools/RICON_LiveProbe/` | SHA256 |
|---|---|
| `Scripts/Game/RICON_Live/RICON_LivePhysicalObservationProbe.c` | `b623857a62144c5ef4dd8fa6c4b3798dacf9ae0d8ae69a385512ee301ddd2704` |
| `Scripts/WorkbenchGame/RICON_LivePhysicalObservationPlugin.c` | `c31b496d1af3cb5b58017db0403a829148a90720bbe1b70d60f7d056fc5073fa` |
| `Scripts/Game/RICON_Live/RICON_LivePhysicalObservationFixtures.c` | `e8c61fa5605d1a440e85ec19566942742678b4c2e315d0aa15c08cc7442e418d` |
| `Scripts/WorkbenchGame/RICON_LivePhysicalObservationCheckPlugin.c` | `36dc6a68b1458960e440bb9de19a55fcb15571315fefd748ed873b2d788cd34f` |

Manifest protected pins также совпали: LiveProbe `addon.gproj` `8b47f08ce19736da62a06f25b5af521c6e71828aa087fa63197ca2617acf0e90`; existing PhysicalObservation sampler `23a86d540a31302d51aa383bb19012536623642009dbfec1e62e3784b05319bf`; native adapter `3f0af441fe34e5d7476d838cfb5b90b99478b18f78dfe3dbaa0ae24484efe9ac`. Root сообщил отсутствие edits старого RPL/checker/TestRunner; новый pure checker находится в LiveProbe, а не заменяет старый runner. Неизменность этих дополнительных файлов не выводится только из175checks.

## Свежий actual ScriptEditor run

Log: `C:/Users/Ded/Documents/My Games/ArmaReforgerWorkbench/logs/ricon-physical-manual-guards-20261110-174031/console.log`.

**SHA256 `36bb25b999ae09b2537c0f5bed67beac411cf293171612661cb944ebe1812641`**, независимо прочитан/проверен. Log:10 задаёт **ScriptEditor** и **RICON_LivePhysicalObservationCheckPlugin**; это отдельный pure checker, не manual WorldEditor plugin.

| Наблюдение | Exact result |
|---|---|
| Log:168 | `checks=175 failures=0 first=0 label=`; label пустой |
| Log:169 | Ровно один `done: PASS` |
| Whole log | SCRIPT(E)=0; NULL-pointer reason count=0 |
| WorkbenchGame, :161 | 195 files / 674 classes / CRC32 `c270fc5f` |
| Game, :162 | 5843 files / 11788 classes / CRC32 `faccde44` |
| Отдельный operator receipt | `process_exit=0`, тот же exact log SHA, `native_world_run=false` |

Operator receipt: `ricon-physical-manual-guards-20261110-174031-operator.json`, SHA `e0f11cda3014087d32cb83546cad5cebd19cfd1d953ddf2e525d84cd8ab28805`, scope MANUAL_PROBE_COMPILE_AND_PURE_GUARDS_ONLY. Process exit получен из этого файла; console.log сам по себе его не доказывает. Run wrapper `run_manual_probe_checks.ps1` snapshot-at-read SHA `964e5b4e4eb671dfb01248053ce28761e3d4d016bd75c5faecd597d0e99f2f1c`; Documentation его не запускала.

После PASS есть `PLATFORM(E) Steamworks: RequestCurrentStats failed` (:170) и `RESOURCES(E)` UI font/texture leaks (:190–214). Заявляется SCRIPT(E)0, не отсутствие всех engine diagnostics.

## Уровни проверенного поведения

| Часть | Что подтверждено этим запуском | Что остаётся за границей |
|---|---|---|
| Request/context/distance | Actual shared `ValidateRequest`, `AdmitContext`, `InDistanceDomain`: strict exact opt-in, bounded raw keys, восемь boolean context combinations, horizontal inclusive2/100m и отказы | Настоящие WorldEditor/GetGame/world/Replication semantics и actual owners не вызывались |
| One-attempt latch | Actual shared `TakeAttempt`, direct pure no-reset cases | Menu instance/script lifetime и повторные реальные menu/CLI invocations не исполнялись |
| Receipt/refusal | Actual shared `FormatReceipt`/`Refusal` на TEST_ONLY data: positive11lines/refusal1line, cap≤12, shape failures, clipping/escaping, identity/source sentinels | Не actual native observation receipt; source/world identity или scene audit этим formatter input не удостоверены |
| Manual/native path | Source reviewed, весь LiveProbe Game/WorkbenchGame compiled; manual plugin:21–71 показывает explicit manual Run/latch и отдельный CLI refusal без fallback; helper имеет один sampler call | Manual Run, SampleNamedOwners/native sampler, CLI dispatch manual plugin, engine lookups/body/terrain/trace/time **NOT_RUN** |

W01–W12 зафиксированы Reviewer до чтения implementation; исходный freeze `e1381cc51d43afcf2fc72a2d66a276dad8c570013b8a243cd674abc313151af4` сообщён Root и является historical case-snapshot, не current hash mutable отчёта. Затем Root identity counter потребовал actual radio-local/source header fields; W13 добавил три assertions к original227line fixture без ослабления прежних expected. Current234line e8c fixture выполнен175checks. Второй Root counter добавил immediate pre-Sample context recheck к сохранённому postcheck; native execution этих branches не заявляется.

`core/WORLD_PROBE_CODE_RU.md` snapshot-at-read `9737b7cc3134c7fe216b39077f9976e689f54e3c5863aa3f4e6131f555c35654` описывает final source/fixture peer gates. Historical pre-SDK **ACCEPT_SCOPED_MANUAL_WORLD_PROBE_PREPARATION_UNCOMPILED** и scene-intake peer check теперь сохранены в фактической immutable копии **`review/WORLD_PROBE_REVIEW.source9619.md`**, independently checked SHA **`9619e9f83654195e2222d4a2b93e341aad73135e58aac61beda1c605eceacf6e`**. Этот hash не относится к позднее дополненному original report.

После actual SDK отдельно прочитаны и hash-проверены два phase-frozen отчёта: `core/MANUAL_SDK_REVIEW_RU.md` **`3813a27f47c3984878d50b71b0070634479c1a2bb8a082d46f3158475f2d8661`**, verdict **ACCEPT_ACTUAL_COMPILE_AND_SHARED_PURE_GUARDS_ONLY**; current `review/WORLD_PROBE_REVIEW_RU.md` **`d527dedeb33f268272f2f2c59e539be09b57d054b0fd0fca134ae5ff20f78d88`**, actual175 checkpoint. Оба независимо проверили current4source bytes, fresh log и operator receipt и сохранили native/menu/CLI/world NOT_RUN. Новых compile fixes на этом запуске не потребовалось. Root подтвердил freeze этих pins; дальнейшее QA ведётся отдельным артефактом.

Новые helper/plugin не читают и не выводят network key value/digest; pure formatter tests отдельно проверяют отсутствие secret-marker output. Native underlying observation reader может использовать key внутри существующей before/after проверки; такой DTO не возвращается наружу manual helper. Receipt сохраняет caller entity origin, unverified antenna motion, raw tuning, incomplete paths/RF/time qualifications. SceneAudit=NOT_VERIFIED_BY_PROBE: один FindEntityByName не доказывает глобальную уникальность names.

## Незакрытые gates и история

Предыдущий **[285/2065 observation SDK gate](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/evidence/E-ct10-physical-observation-20261110.md>)** — отдельное evidence; его не суммируют с175 в один запуск и в этой фазе не повторяли. Legacy106 здесь также не запускался. Source/compile/pure success не закрывает M1/full Goal.

Current mounted scene/resources и две exact уникальные native radio-owner instances/TRX0 **MISSING**. [WORLD_FIXTURE_INTAKE_RU.md](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ct10-physical-source-20261110/docs/WORLD_FIXTURE_INTAKE_RU.md>) SHA `49b0bf1c2155f59eef009d17802913d93bf3cfb20ead09729c3cba0025ace2c4`: его шесть source claims отдельно прошли Reviewer bounded peer check; готовая сцена/loaded owners не объявлены. Parent world/GameMode/embedded autorun требуют отдельной проверки.

Текущий допуск — **ScriptEditor only**. Следующий WorldEditor **AUTHORING_ONLY** — предложение Root, одобрения ещё нет; это не Play/world/server/voice permission. Native world authority/context, actual CLI/manual dispatch, resource/scene readiness, physical source/clock/antenna/RF calibration, full paths/commonRX/production SERVER/RECEIVER/RPC/audio/load остаются OPEN. Agy final exact-data approval gate сохраняется, новых передач нет. Documentation изменила только два назначенных evidence/receipt staging files; Root выполняет дальнейший review и canonical публикацию.
