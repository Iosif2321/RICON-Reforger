# CT10 — карта миграции временного дескриптора

**Последующий observation-срез 2026-11-10:** [контракт физических наблюдений](REQ-physical-observation.md) и [actual285/2065 pure evidence](../evidence/E-ct10-physical-observation-20261110.md). Read-only public adapter скомпилирован, stateless sampler проверен на явном source DOUBLE. Он не строит descriptor и не доказывает actual world/RF/profile/clock completeness; production physical builder и native world qualification остаются отдельными шагами. Schema1/RCFD1 и физическая validity не расширены.

**Датированное дополнение 2026-11-10:** исходная карта23Oct ниже сохранена как история. Explicit schema2 DTO/Clock/raw+ordered receiver/capture и новый producer/owner реализованы отдельными API без скрытого расширения v1/RCFD1; [контракт producer](REQ-temporal-producer.md) и [фактический SDK1780/legacy106 evidence](../evidence/E-ct10-producer-owner-20261110.md). Manager имеет opt-in source entrypoint с actual PairService и одним private owner, но реальный complete world/exporter, temporal physical builder, калибровка/Receiver orchestration, native manager authority/RPC/seam/audio/load ещё не квалифицированы. Старые таблицы не читать как текущее отсутствие всех v2 классов; полную готовность миграции этот срез не устанавливает.

2026-10-23, codex. **DRAFT / READ_ONLY_INTAKE**, не новый утверждённый контракт.
Основание: `REQ-channel-time.md` v0.2 CT04–CT10 и закрытый kernel `REQ-channel-fading.md` v0.2.
Карта составлена после чтения текущих исходников; Game-код и snapshot v1 не изменены.

## Что уже есть и что придётся менять вместе

| Место | Текущее поведение | Область миграции |
|---|---|---|
| `addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_ChannelFading.c` | Чистая Evaluate по явно заданным descriptor/query; состояния среды не хранит | Глубокая копия/валидация переносимого descriptor; ключ общей среды отделён от lease/link/epoch |
| `addons/RICON_Core/Scripts/Game/RICON/Core/RICON_LinkPublication.c` | Input и registry содержат state/Q/TTL; NeedSnapshot учитывает их и keepalive; публикуется schema 1 | Доверенный вход дескриптора по физической паре и RF-каналу, его копия в registry, публикация при изменении независимо от qualityDelta, полный resync |
| `addons/RICON_Core/Scripts/Game/RICON/Core/RICON_LinkSnapshot.c` | schema 1, скалярная CloneSnapshot, Receive и атомарный ApplyFullResync | Явная версия нового формата, валидация модели/срока/контекста, глубокие копии массивов и endpoint, отклонение неподдерживаемой модели с причиной и resync |
| `addons/RICON_Core/Scripts/Game/RICON/Network/RICON_NetworkContracts.c` | Seam проверяет snapshot/binding, затем BLOCKED_NO_ACTUATOR | Проверка соответствия устройства/индекса/RF-канала и временного контекста; сохранение честного отказа до actuator |
| `oracle/snapshot_oracle.py`, `oracle/link_publication_oracle.py`, `oracle/von_seam_oracle.py` | Модели старого формата, у seam закрытый набор ключей | Новый явно версионированный контракт и трассы без переопределения смысла v1 |
| `tools/RICON_TestRunner/gen_snapshot_fixture.py`, `gen_link_publication_fixture.py`, `gen_von_seam_fixture.py` | Копируют существующие статические поля | Новые поля и вложенные копии, host-only ошибки структуры/типов отдельно от native |
| `addons/RICON_CoreTests/Scripts/Game/RICON_Tests/RICON_{Snapshot,LinkPublication,VoNSeam}{Fixtures,FixtureSupport}.c` | Наборы native-проверок старой цепочки | Все новые причины отказа, последовательности, неизменность вложенных данных и отрицательные контроли сравнения |
| `tools/RICON_TestRunner/Scripts/WorkbenchGame/`: `RICON_SnapshotProbePlugin.c`, `RICON_LinkPublicationProbePlugin.c`, `RICON_TestRunnerPlugin.c` | Первые два вызывают snapshot/publication fixtures; общий TestRunner вызывает VoNSeam fixtures | Обновить все три затронутых пробы вместе с контрактом |
| Сериализация/доставка | В `addons/RICON_Core/Scripts/Game/RICON` не найден codec/RPC snapshot по поиску RplRpc/RplSave/RplLoad/ScriptReadCtx/ScriptWriteCtx/Serialize/Deserialize | Специфицировать формат, размер и round trip; отсутствие codec не считать готовой сетевой доставкой |

Имена семейства fixtures в таблице — сокращение списка файлов, а не буквальный путь с фигурными скобками.
Вызывающие пробы проверены поиском и чтением RunCommandline. `CURRENT_SCHEMA_VERSION = 1` пока сохраняется.

## Решения, которые ещё должны стать требованиями

1. Новый формат и его совместимость: как различаются статическое описание и временная модель, как отказ
   неподдерживаемой версии/модели проходит через single receive, full resync и seam. Молчаливый переход к Q запрещён CT10.
2. Источник доверенного descriptor и его соответствие физическим TX/RX, частоте, полосе, профилю и revision среды.
   Publication не выводит компоненты/доплер из среднего Q. Ключ среды не меняется при PTT, lease и epoch.
3. Правило равенства и изменения descriptor, сохранение непрерывности при обновлении, причина резкого изменения.
   Сетевой linkSeq упорядочивает снимки; он не является фазой или seed среды.
4. Независимые сроки snapshot TTL и descriptor validity. Keepalive не продлевает доказанную область модели.
5. Формат сериализации, лимит размера/количества составляющих и атомарность отказа. Ресурсный лимит не означает LOST.
6. Временной вход клиентского представления: реальное соответствие часов и его неопределённость остаются CT06.
   Прогон с явно заданным синтетическим серверным временем не доказывает синхронизацию Reforger.

## Минимальные проверки согласованной миграции

- Неизменные средние state/Q и новый descriptor всё равно вызывают публикацию; qualityDelta не съедает обновление.
- Первый full resync переносит descriptor; перестановка/дубли не откатывают состояние среды.
- Keepalive меняет срок представления, сохраняя фазовый якорь и срок физической применимости.
- Перерыв PTT и ротация сетевой epoch не создают новую реализацию неизменной среды.
- Обратное направление использует то же состояние среды; разные слушатели имеют свои физические пары.
- Изменение исходного descriptor, копии результата или массива components после вызова не меняет registry/receiver.
- Неподдерживаемая модель/версия и дефект позднего элемента full resync дают согласованный отказ и resync целиком.
- Истёкший descriptor и неподтверждённое соответствие времени не становятся статическим CLEAN/WEAK и не получают LOST по умолчанию.
- Codec сохраняет int32-фазы, binary32-параметры и порядок компонентов; повреждённый/слишком большой пакет отклоняется до применения.
- Краткий ненулевой отсчёт после ZERO сохраняется вычислительной цепочкой; наличие исходного участка речи и управление
  конкретной парой проверяются отдельно. До этого голос остаётся BLOCKED_NO_ACTUATOR.

До согласования конкретных требований, независимого эталона и freeze эта карта не разрешает менять wire contract.
Реализация плана уже поручена владельцем; повторное разрешение на обычные согласованные правки не требуется.
Запуск мира/Play/сервера и игровая приёмка этой картой не выполняются.
