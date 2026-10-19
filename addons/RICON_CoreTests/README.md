# RICON_CoreTests — фикстуры ядра (TEST_ONLY)

Отдельный аддон с проверками ядра: пять сценариев расширения `docs/req/REQ-core-contracts.md` §10 и сверка
жизненного цикла (`docs/req/REQ-lifecycle.md`) с эталоном. Он зависит от `RICON_Core`, **не публикуется в Workshop
и не входит в сборку мода**. Все числа в фикстурах — тестовые значения, а не физика.

Фикстуры исполняет плагин `tools/RICON_TestRunner` внутри Workbench (ScriptEditor, без Play и без сервера,
разрешение владельца 2026-10-19).

| Набор | Файлы | Статус |
|---|---|---|
| lifecycle | `RICON_LifecycleFixtures.c` (сгенерирован из `oracle/vectors/lifecycle_v1.json`), `RICON_LifecycleFixtureSupport.c` | `ENFORCE_PARITY`: 14/14 трасс, 208/208 шагов, 3/3 контрольных ошибки пойманы |
| extension | `RICON_ExtFixtures.c`, `RICON_TestSupport.c` | `SELF_CHECK_PASS`: EXT-1…5 |

Это не проверка в игре: нет репликации, событий игры и голосового чата.

Сценарии расширения:

| Сценарий | Класс | Что показывает |
|---|---|---|
| EXT-1 новая УКВ-рация | `RICON_Ext1NewVhfRadio` | только конфиг: `RICON_BandConfig` + существующие модели, ни одного нового класса модели |
| EXT-2 КВ, ионосфера | `RICON_Ext2SkyWaveModel` | новая модель распространения со своим слоем `ionosphere_test` и своим диапазоном 3–30 МГц; запрос и бюджет не меняются |
| EXT-3 спутниковая сеть | `RICON_Ext3SatelliteNetwork`, `RICON_Ext3SatelliteCallFactory` | новый тип сети: политика маршрута + общий seam сессии вызова; кода сотовой сети нет |
| EXT-4 направленная антенна | `RICON_Ext4DirectionalAntenna` | новая диаграмма антенны; evaluator видит только итоговое усиление в запросе |
| EXT-5 проводная линия | `RICON_Ext5WiredEvaluator` | другой evaluator с тем же контрактом, без радиошума; в ядре нет `if wired` |

Проверка «ядро не изменилось»: после добавления фикстур `git diff --stat <база> -- addons/RICON_Core` должен быть
пуст.
