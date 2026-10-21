# REQ-radio-device-adapter — наблюдение рации и сборка запроса S2

Версия: **0.2 AGENT-REVIEWED**, 2026-10-21. Автор: codex. Основание: owner agora #2936/#2937,
разделение #2946/#3107, закрытие S1 #3114. Версия 0.1 и корпус rev1 согласованы claude #3127;
v0.2/rev2 уточняет только self-pair по #3127/#3128 и согласована claude #3139 после независимой сверки 68 случаев.
Корпус rev2 заморожен: SHA-256 `1806e7377cc0a68c7fc647ff19a3482841cbba4f5820fd22fbc24b8f8846d960`.
Изменение требований или корпуса требует новой согласованной ревизии. Исполнение Enforce проверяется отдельно.

## Мини-intent

- **Проблема:** ядро получает готовый `RICON_LinkQuery`, но мост от конкретных раций и отдельных параметров
  передатчика/приёмника к запросу отсутствует. Питание рации не является мощностью в дБм.
- **Результат:** чистый сборщик одной направленной пары и тонкая read-only оболочка наблюдения native API.
  Сборщик исполняется без мира; оболочка — compile-only до отдельного опыта.
- **Затронуто:** новые observation/profile/context и builder в одном Enforce-файле, Python-эталон и фикстуры.
  Существующие формулы, `RICON_EReason`, LOST, lifecycle, registries и VoN seam не меняются.
- **Проверка:** OR → peer review → Enforce parity; отдельно цепочка S2 → S1 → L2 → link-eval.
- **Ограничения:** одна пара, без discovery, таймера, RPC, UI, setters, actuator, relay/intercom и регистрации
  реальных устройств. Никакой игровой или акустической приёмки.

## 1. Источники и доверие

Входы строит сервер из своего текущего binding и каталогов (REQ-core §4, REQ-von-seam §4).
Сборщик не принимает клиентские RF-значения, не регистрирует устройства и не доказывает происхождение
переданных ему объектов. Проверка текущей epoch/generation/lease/access остаётся у существующего lifecycle
и будущей службы пар S3. Равенство числового deviceId не заменяет эту проверку.

DOC установленной версии 1.8.0.13:

- `interfaceBaseRadioComponent.html:123–130`: GetOwner → IEntity, TransceiversCount → int,
  GetTransceiver(int) → BaseTransceiver, GetEncryptionKey → string, IsPowered → bool.
- `interfaceBaseTransceiver.html:125,142,198,275`: GetFrequency → int, GetRadio → BaseRadioComponent.
  Единица GetFrequency — UNKNOWN; подпись кГц у SetFrequency/Resolution её не доказывает
  (`E-radio-api-s1.md`, agora #3069/#3070).
- GetOwner даёт entity, не authority. Точка антенны задаётся серверным поставщиком явно (REQ-WP-06),
  не выводится из положения ног или центра радио.

Физическую частоту задаёт каждый RF-профиль вместе с явным ожидаемым `tuningValue`. Эта таблица соответствия
пока TEST_ONLY/ASSUMED; деления native значения на 1000 нет. Возможны разные native значения у двух устройств,
если их явно заданные RF-частоты равны. Значения ванили Transmitting Range в расчёт не входят и в дБм не переводятся.

## 2. Значения входов

### 2.1 RICON_RadioObservation

| Поле JSON | Поле Enforce | Домен |
|---|---|---|
| deviceId | m_iDeviceId | целое 1…int.MAX−1, server-resolved |
| transceiverIndex | m_iTransceiverIndex | целое 0…int.MAX−1; обязательное, default −1 |
| powered | m_bPowered | bool |
| tuningValue | m_iTuningValue | int32, непрозрачное native значение |
| networkKey | m_sNetworkKey | string, не выводится в диагностические логи |
| antennaPos | m_vAntennaPos | три конечных числа в метрах world-space |

### 2.2 RICON_RadioRfProfile — отдельный объект для каждого конца

| Поле JSON | Поле Enforce | Домен / смысл |
|---|---|---|
| profileId | m_sProfileId | непустой stable string ID |
| deviceId | m_iDeviceId | 1…int.MAX−1 |
| transceiverIndex | m_iTransceiverIndex | 0…int.MAX−1, default −1 |
| provenance | m_sProvenance | ровно TEST_ONLY или ASSUMED; реальная калибровка не заявляется |
| expectedTuningValue | m_iExpectedTuningValue | int32; единицы и допустимые native sentinel не угадываются |
| frequencyMHz | m_fFrequencyMHz | конечное >0, явно заданная физическая частота |
| rxBandwidthHz | m_fRxBandwidthHz | конечное >0, шумовая полоса этого приёмника |
| txPowerDbm | m_fTxPowerDbm | конечное, выбранная мощность этого передатчика |
| txGainDbi | m_fTxGainDbi | конечное, усиление передающей антенны для текущего направления |
| rxGainDbi | m_fRxGainDbi | конечное, усиление приёмной антенны для текущего направления |
| rxNoiseFigureDb | m_fRxNoiseFigureDb | конечное ≥0 |
| hasRxSensitivity | m_bHasRxSensitivity | bool |
| rxSensitivityDbm | m_fRxSensitivityDbm | конечное, если выбран этот порог |
| hasRequiredSinr | m_bHasRequiredSinr | bool |
| requiredSinrDb | m_fRequiredSinrDb | конечное, если выбран этот порог |

Ровно один флаг порога true. Оба профиля должны быть пригодны для обеих ролей; при сборке используются
только TX-поля отправителя и RX-поля приёмника (§4). Неактивное значение порога не интерпретируется.
Усиления разрешает существующий серверный сервис AntennaPattern для направления пары; в TEST_ONLY
фикстурах они заданы явно. Новые типы антенн в S2 не вводятся. Никаких неявных RF-значений по умолчанию.

### 2.3 RICON_RadioQueryContext

`bandId`, `modelId`, `noiseEnvironmentId`, `intelligibilityModelId` — непустые строки, те же поля LinkQuery.
`hasInterference` — bool; `interferenceDbm` — конечное, если флаг true. Принадлежность ID реальным frozen
registries проверяет существующий evaluator/manager: READY ниже не означает успешное разрешение ID.

В Python bool не принимается как int/float; целые ограничены int32. Нет неявных преобразований строк в числа,
значений по умолчанию для обязательных полей или подстановки отсутствующего профиля.
Все float-поля и компоненты векторов при входе имеют семантику IEEE754 binary32, как Enforce float.
Эталон округляет их в binary32 до проверки конечности и сравнения частот; overflow становится ±Inf,
underflow — signed zero. Поэтому две разные десятичные записи, округлившиеся в одно binary32, равны.
Только NaN/+Inf/−Inf служат специальным JSON-представлением float; строковые ID и ключи не декодируются.

## 3. Коды и порядок сборки

`RICON_RadioQueryBuilder.Build(txObservation, txProfile, rxObservation, rxProfile, context, out query)`
возвращает отдельный `RICON_ERadioQueryCode`. На любом отказе `query = null`, частичного результата нет.
Существующий `RICON_EReason` не расширяется и не перенумеровывается.

| Код | Имя | Первая применимая причина |
|---|---|---|
| 0 | NULL_INPUT | отсутствует любой из пяти входных объектов |
| 1 | INVALID_OBSERVATION | хотя бы одно наблюдение нарушает §2.1 |
| 2 | INVALID_PROFILE | хотя бы один профиль нарушает §2.2 |
| 3 | INVALID_CONTEXT | контекст нарушает §2.3 |
| 4 | PROFILE_MISMATCH | deviceId либо transceiverIndex профиля не равен соответствующему наблюдению |
| 5 | POWER_OFF | отправитель или приёмник выключен |
| 6 | TUNING_MISMATCH | хотя бы один tuningValue не равен своему expectedTuningValue |
| 7 | CHANNEL_MISMATCH | явно заданные frequencyMHz профилей не равны |
| 8 | KEY_MISMATCH | networkKey двух наблюдений не равны |
| 9 | READY | сформирован новый запрос; его ещё нужно оценить ядром |

Порядок таблицы нормативный, в том числе для нескольких одновременно неверных входов. Проверки TX/RX
внутри одной причины имеют одинаковый код. Частоты сравниваются точно как значения одного каталога;
перекрытие полос, неточная настройка и захват соседнего канала в этой итерации не моделируются.

Сравнение ключей — точное равенство непрозрачных строк. Две пустые строки равны по политике этого сборщика.
Это не доказательство native voice encryption, разрешённой доставки или безопасности сети. Ключи не входят в запрос.
Сборщик не исключает одно и то же устройство или endpoint с обеих сторон. Отбор пар, в том числе исключение
нежелательных self-pair, относится к S3. READY для такой пары не означает разрешение доставки или обратной связи.

## 4. Выход и немутируемость

При READY создаётся **новый** RICON_LinkQuery:

| Выход | Источник |
|---|---|
| txPos / rxPos | antennaPos соответствующих наблюдений, без сдвига по высоте |
| frequencyMHz | txProfile.frequencyMHz, уже равная rxProfile.frequencyMHz |
| bandwidthHz | **rxProfile.rxBandwidthHz** |
| txPowerDbm / txGainDbi | **txProfile** |
| rxGainDbi / rxNoiseFigureDb | **rxProfile** |
| флаги и выбранный RX-порог | **rxProfile**; невыбранное числовое поле = 0 |
| hasInterference / interferenceDbm | context; при false значение = 0 |
| четыре string ID | context |

Все числовые выходы конечны. Входные наблюдения, профили и контекст остаются неизменными, запрос не хранит
ссылки на изменяемые входы. Противоположные, неиспользуемые в этой роли RF-поля не подмешиваются в выход.
READY не равно valid budget, CLEAN, доставке голоса или приёмке ASSUMED-значений. Пример: неизвестный modelId
либо антенна на земле могут пройти сборку и затем получить штатный отказ существующего ядра.

## 5. Тонкая native-оболочка

`RICON_BaseRadioObservationReader.TryRead(radio, deviceId, transceiverIndex, antennaPos, out observation)`:

1. Только authority (`Replication.IsServer()` через существующий RICON_CommsManagerComponent.IsAuthority).
   При false — отказ без вызовов radio API.
2. Проверить radio, deviceId/index и конечность явной antennaPos. GetOwner должен быть ненулевым.
3. Проверить `index < radio.TransceiversCount()`, затем GetTransceiver(index) и его ненулевой результат.
4. `transceiver.GetRadio() == radio`; иначе отказ.
5. Создать полное observation из аргументов и radio.IsPowered(), transceiver.GetFrequency(), radio.GetEncryptionKey().

При отказе observation=null; методы Set*, сохранение ресурсов, RPC и изменение движка запрещены.
Reader не назначает stable ID: deviceId и radio поступают из серверного binding. Он не заменяет проверку
актуальности/доступа из §1 и не обещает атомарный snapshot нескольких native getters.
В Workbench без мира исполняются только безопасные ветви, не требующие настоящего радио; остальное
скомпилировано, но BLOCKED_NO_RUNTIME. На настоящую рацию reader пока не регистрируется.

## 6. Проверки и приёмка

- Положительные пары: разные профили, receiver BW/NF/порог и TX power/gain выбираются с правильной стороны.
- Асимметрия: обратное направление меняет параметры согласно профилям; изменение неиспользуемого TX/RX-поля
  при сохранении пригодности профиля не меняет исходный запрос.
- Разные native tuningValue при одной явной RF-частоте допустимы; никакого скрытого /1000.
- Каждый отказ §3 и несколько нарушений одновременно; NaN/±Inf в координатах и активных RF-полях.
- Неактивные порог/interference могут содержать нечисловое входное значение; на выходе соответствующие поля 0.
- Профиль, identity/index, provenance отсутствуют или неверны; powered=false с любой стороны; tuning/key mismatch.
- На READY запрос новый; после вызова и после изменения выходного запроса входы неизменны.
- Синтетическая полная цепочка: разные TX/RX RF-профили → query → S1 flat-source → L2 → evaluator.
  Вторая точка: adapter сохраняет позицию ног на высоте поверхности; READY, затем L2 отказывает с
  TERRAIN_PROFILE_UNAVAILABLE (AGL=0), не поднимает антенну молча.
- Корпус: schema `ricon.radio_device`, version 1, revision 2; обязательные input и expect,
  специальные float как строки NaN/+Inf/−Inf. В expect — код и полный query либо null.
  R01…R67 сохранены из rev1, R68 фиксирует READY для одного и того же endpoint с обеих сторон.

Критерий S2: согласованный корпус и независимый код, исполненная Enforce parity с отрицательными контролями,
сквозные проверки и повтор общей регрессии. Native reader остаётся compile-only; характеристики реальных раций,
текущее server binding, атомарность наблюдения, RPC/JIP и звук требуют отдельного опыта, а не нового ярлыка PASS.
