# RICON + native VoN: временной канал и модель приёмника

Дата: 2026-11-04. Статус: **AMENDED_DESIGN / REVIEW_ACCEPTED / NATIVE_NOT_QUALIFIED / PRODUCTION_NOT_READY**.
Основание: прямое продолжение владельца и [TEAM]; эта полная редакция заменяет текущую server-only/final-q конструкцию, а не добавляет к ней противоречивое исключение.
Исходный canonical PLAN SHA256: `9958c9bb639a8d81c41f805dfef4b7b1ad2820653cda6f14ca30518cd55d0960`. Root сохраняет backup и применяет только проверенную редакцию.

## 1. Решение и неизменные границы

Сохраняем native микрофон, подготовку голоса, Opus, сеть, source authentication, decoder и вывод. RICON описывает физический радиоканал во времени и отдельно работу конкретного приёмника; один средний Q не заменяет эти две модели.
Два разрешённых места расчёта — **RECEIVER и SERVER**, с общими функциями, профилями, входами и временным контрактом. Место расчёта не даёт клиенту права назначать source binding или разрешения.
Существующие проверки владельца native объекта сохраняются; дополнительно проверяется `(sender, entity, device, radio Rpl ID, TX index, generation, lease, comm method)`. Это проверка происхождения/прав, не бинарная RF-модель.
DIRECT, другие пары, пользовательские power/mute, инвентарь и native lifecycle сохраняются. Один VON_RADIO содержит Radio и Direct shaders: управление всем событием не считается radio-only воздействием.
Новый voice backend, второй Core/реестр, DLL/patching и извлечение движка по пункту1.1 не вводятся. Эта редакция не меняет Core и замороженные REQ автоматически.

## 2. Две ступени и два места расчёта

| Ступень | Вход и результат | Разделение ответственности |
|---|---|---|
| 1 — временной канал | Подтверждённые TX/RX/антенны, среда, диапазон/полоса, профили и время → версия описания полезного сигнала, шума, помех, fading/корреляции, окна времени, полноты и применимости | Переиспользуем бюджет и channel-time/fading/descriptor/cutoff. Не выдаём скалярные LOST/Q за всю физику |
| 2 — приёмник и звук | Совместный набор вкладов в один приёмный тракт + профиль реальной рации + состояние органов управления → отклик приёмника и отдельно параметры радиоветви звука | Селективность, шумовой порог, демодуляция, перегрузка/захват, AGC, squelch и слуховая калибровка относятся к приёмнику, а не к транспортному allow/deny |

| Место | Выполнение тех же расчётов | Что остаётся на клиенте-получателе |
|---|---|---|
| RECEIVER | Сервер проверяет происхождение и публикует действительные входы/описание; получатель рассчитывает канал и управляющие параметры приёмника по общему времени и профилю | Обработка PCM/DSP, зависящая от аудио работа приёмника и применение к точной Radio-ветви |
| SERVER | Сервер рассчитывает тот же канал и управляющие параметры приёмника, передаёт версионированное описание/результаты окна; не один конечный q и не удалённый PCM | Те же обработка PCM/DSP, зависящая от аудио работа приёмника, применение параметров и вывод; без повторного внесения тех же потерь |

Сравнение RECEIVER/SERVER делается на одинаковых полных входах, окнах и состоянии модели, с заявленной точностью. Расхождения чисел, временной привязки или полноты видны, а не скрываются сменой расположения.
Выполнение расчёта отделено от серверных прав и фактической доставки; окончательный звуковой вывод всегда происходит у слушателя. Переключение места не создаёт новую физическую реализацию канала. Место расчёта относится к каналу и receiver-control параметрам; фактическое исполнение PCM/DSP и audio-dependent receiver processing в обоих режимах остаётся на listening client. Сервер получает штатный encoded voice; декодировать его на сервере ради ступени2 или создавать удалённый PCM backend этот план не предлагает.

## 3. Контракт ступени1: данные и время

Описание содержит физическую пару `(device,index)` и RF-канал, model/profile IDs и версии, средний бюджет и временные составляющие, полезный сигнал, шум и помехи раздельно, полосу/единицы, anchor/window, environment revision, validity/completeness/resource reasons.
Существующий FINITE_MULTIPATH_V1 описывает только полезный коэффициент среды; RCFD/1 переносит только этот дескриптор. Это не готовая схема всех signal/noise/interference полей и не рабочая доставка клиенту [CF][CD].
Композиция/новая версия интеграционного контракта проектируется явно по [CT][CT10]; snapshot v1/RCFD1 не получают скрытого нового смысла. Нельзя вывести временные составляющие или помехи из одного среднего Q.
Общая физическая среда не привязана к lease/linkSeq/PTT/epoch. Повторный PTT/resync не перебрасывает seed/фазу. Взаимность применяется только в области модели; направленные RF-тракты и конечные SINR/Q могут различаться.
Полный одинаковый вход даёт одинаковый результат независимо от порядка слушателей, числа вызовов и кадров. При полностью неизменном полезном канале часы сами не создают его дрейф; noise/interference моделируются отдельно [CT] (CT01–06).
Нужны временное разрешение минимальных событий и соответствие времени расчёта времени доставленного звука с оценкой неопределённости. Локальный TTL, world delta и частота аудиокадров не объявляются одним clock.
Snapshot TTL и срок физической применимости независимы; keepalive не продлевает descriptor validity. Новая среда/версия требует нового доказательства; Rebase сохраняет ту же последовательность, а не создаёт новые просветы.

## 4. Приёмник, шум и конкурирующие передачи

**LOWQ/LOST/Q=0 не означают тишину, выключение устройства или transport deny.** Числовой ноль полезного коэффициента в одном отсчёте не доказывает ноль на всём интервале; EMPTY_MEDIUM kernel также не сертифицирует cutoff [CF] (CF09).
У открытого приёмника шумовой фон сохраняется при замаскированной речи и отсутствии полезного вклада. Закрытие squelch, пользовательские power/mute и физическая невозможность полезного сигнала — отдельные состояния с отдельными причинами.
AWGN допустим как частный проверочный случай; он не заменяет fading, цветной шум, селективность, импульсные помехи, перегрузку, демодуляционные ошибки и память приёмника. Stock Brownian/bitcrusher — окраска звука, не калиброванный RF-приёмник.
Одновременные источники на одной частоте/в полосе селективности поступают **совместно в один front-end**: полезный сигнал, конкуренты и шум взаимодействуют по профилю RX. Нельзя независимо добавить каждому голосу несвязанную noise дорожку и назвать это общей помехой.
Пересчёт агрегированного I не дублирует уже учтённые вклады. Учёт энергии помехи не даёт права раскрывать содержимое чужой защищённой передачи. Выбор/захват источника, близкий сильный конкурент и смена состояния проверяются совместно.
Профили привязаны к точному типу/варианту рации, полосе, антенне, RF-тракту, демодулятору, squelch и критерию калибровки. [RC] — частичный каталог, не готовый S2-профиль; TEST_ONLY и linear_test не становятся production разборчивостью [LE74–75].
Неизвестные/устаревшие данные дают явный technical/stale/UNQUALIFIED и resync; не CLEAN, не RF-LOST и не выдуманный чистый сигнал/тишину. Политика технического отказа должна быть отдельно задана и проверена до включения managed mode.

## 5. Native доставка: сохранить возможные фрагменты

Нельзя удалить payload по среднему LOST/мгновенному Q=0, а затем «восстановить» речь шумом или seed. Для физически возможных окон нужные исходные фрагменты должны доходить до места применения модели [CT] (CT08).
Покрытие кандидатов native доставки должно включать допустимые моделью полезные и значимые конкурирующие вклады, в том числе за stock range. Настройки stock range/q не должны преждевременно исключать их или необратимо искажать до RICON-приёмника.
Штатные права, source binding и platform/social checks сохраняются, но не подменяют физическую оценку канала. Приёмник/RX metadata выбираются однозначно; last-writer-wins stock map не считается правилом лучшей связи.
Полезный вклад можно пропустить только по существующему [CC]: полное покрытие/все пути, строгие конечные границы на **весь оставшийся `[from,until)`**, актуальные revisions и сертифицированный numerical guard; `U<tolerance`, равенство продолжается.
Среднее, перцентиль, чувствительность RX, низкий Q и пропуск ресурса не заменяют эту границу. Нет строгой границы — CONTINUE/UNRESOLVED, не случайная/пороговая отсечка. После expiry/invalidation повторная оценка обязательна.
BOUNDED относится только к объявленному полезному вкладу/ошибке, не удаляет шум открытого RX, другие источники или их помехи; фон и ограничения пользователя живут по своим контрактам.
Техническая недоступность порта, stale descriptor и отказ аутентификации не перекрашиваются в физический channel cutoff. Transport/auth denial отдельно от рассчитанного приёмного состояния.

## 6. Две независимые квалификации native-порта

| Вопрос | Доказательство возможности | Что не принимается |
|---|---|---|
| A — серверное покрытие доставки | При корректном source binding нужные исходные фрагменты доходят до RX для всей подтверждённой области модели; stock discovery/range/q не уничтожает будущий просвет; DIRECT/прочие пары сохранены | Удаляющий-only фильтр stock списка, callback count, Q metadata, generic message admission или искусственное глобальное OFF |
| B — управление клиентской радиоветвью | Точный sender/receiver/generation/window связан с фактическим private VoN stream; RICON управляет Radio и общим RX front-end при сохранении Direct shader, чужих потоков и пользовательских состояний | Generic SignalsManager на entity, global variable, обычный PlayEvent, весь VON_RADIO event или public declaration без binding |

A нужен и для RECEIVER расчёта; B нужен и для SERVER расчёта. Передача конечного q по native context сама по себе не закрывает B и новую модель. Root/audio продолжает один конкретный provider join; не строим большой фиктивный adapter до результата.
Успех одного вопроса не выдаётся за другой. Пока нет реального порта, `RICON_VoNIntegration.ApplySnapshot` остаётся `BLOCKED_NO_ACTUATOR`, не APPLIED от публикации/переименования enum [VS].

## 7. Переиспользование и согласованная миграция

| Существующее место | Что сохранить / что мигрировать вместе |
|---|---|
| Core PairService (`RICON_PairService.Evaluate`), `RICON_BaseRadioObservationReader`, `RICON_RadioQueryBuilder`, `RICON_LinkEvaluator` | Реальные устройства, binding, generations, доступ и provenance; отделить legacy state/Q и invalid budget от временного канала/приёмника. Не создавать второй реестр/службу пар [PS] |
| `Propagation/RICON_ChannelFading.c`, действующие channel-time/descriptor/cutoff блоки | Переиспользовать чистый kernel/валидацию/копии/кодек/Assess; дополнить физический builder и versioned signal/noise/I/profile контекст без новых произвольных моделей по умолчанию |
| `Core/RICON_LinkPublication.c`, `Core/RICON_LinkSnapshot.c` | Новый явный schema/limits, registry/CloneSnapshot/deep copy, single/full resync, публикация нового descriptor независимо от qualityDelta; TTL не продлевает физическое окно |
| `Network/RICON_NetworkContracts.c`, существующий native shadow bridge | Использовать текущую seam/identity/ordering; добавить location/window/profile validity и реальный port result. SOURCE_ACCEPTED/offline27 не означает SDK/native/production readiness |
| Lifecycle/lease/device catalog и snapshot receiver | Сохранить authority epoch, адресность/индексы, drop/respawn/rebind, late expiry и ресинхронизацию; не выдавать старый эффект новому endpoint |
| Oracle/fixtures/generators/probes по [CT10] | `snapshot_oracle`, `link_publication_oracle`, `von_seam_oracle`, channel vectors, соответствующие gen_* и Snapshot/LinkPublication/VoNSeam fixtures; codec/size/round-trip, time parity и отрицательные контроли мигрируют совместно |
| REQ-LE72/74/75/81, core state/Q, publication terminal LOST, старые generic LOST gates | LE72 threshold/LOST, формулы и legacy tests этой редакцией не переписаны; запрещено использовать LOST/Q0 как автоматическую тишину или voice transport kill. Generic LOST admission — диагностическая фикстура, не правило живого голоса |
| Собственные Radio ACP/SIG и проверенный native binding | Параметры временного канала/приёмника, шум открытого RX и совместный front-end; Direct/окружение/громкость не присваивать адаптеру |

Точные пути семейства Core/Network и перечень затронутых consumer/generator/probe даны в [CT10]; новый формат не реализуется скрытым расширением snapshot v1. Серверная область [PS] и pure snapshot seam [VS] исторические: двурежимное исполнение требует будущего согласованного типизированного контракта, а не объявления существующей seam готовой архитектурой. Первыми согласуются требования/эталон/версия, затем меняются все вызывающие стороны.
Уже подготовленные shadow/lifecycle/publication части продолжаются; старые synthetic/static проверки сохраняют свой уровень. Production-калибровка разборчивости и полный физический builder остаются незакрытыми.

## 8. Порядок продолжения и приёмка

**Текущий gate 2026-11-06: одно разрешённое data-only inspect01 выполнено, изменение Filename остановлено guard.** Clone entity kind проверен; полная component isolation не прошла, выбранный VoN shared отдельно не установлен. Повторный run/guard bypass не выполняется. Следующая разрешённая подготовка — offline inherited-prefab source; native registration/Spawn/Play не разрешены. Machine Goal остаётся blocked, full objective/complete=false неизменны. [Точный результат](../../evidence/native-von-access-20261104/VON_PROJECT_CONFIG_RU.md).

Исторический provider join на 1.8.0.13 (для native кода 1.9 требуется отдельная квалификация): async queue producer и direct config loader создают новые templates; одинаковые filename/Input не адресуют существующий private VoN T. Далее проверяется только конкретный поддерживаемый external attachment/mutator/interception contract. Повторение config path и общего scheduler/mixer не требуется. [Текущий результат](../../evidence/native-von-access-20261104/ASSET_PREPARATION_RU.md).

1. Root фиксирует amended Goal/план, base SHA/backup и независимое review; честно отделяет состояние Goal tool от разрешённого продолжения. Никакого фиктивного нового Goal/complete.
2. Квалифицировать A/B отдельными маленькими проверками, начиная с текущего provider join. Разрешены static/compile scope, подготовка и пересборка собственных ACP/SIG через Resource Manager/Audio Editor и Computer Use Workbench по отдельным ответам владельца. Play/server/preview/microphone/capture/voice прогоны не разрешены. Текущий срез: own SIG сохранена и source graph проверен на 1.9.0.37, own ACP source binding применён; scoped API 1.9 не дал нового private mutator, editor Open/Save завершён и проверен; native compiled program, delivery coverage и private VoN attachment не квалифицированы; см. [подготовку ресурсов](../../evidence/native-von-access-20261104/ASSET_PREPARATION_RU.md).
3. Зафиксировать versioned двухступенчатый контракт, реальные входы/профиль и независимые векторы; проверить общее время/его неопределённость. Не менять Core только ради снятия LOST.
4. Выполнить совместную CT10-миграцию producer→descriptor/codec→receiver/resync→seam→port→fixtures. Сначала oracle/pure/compile parity, затем отдельно квалифицированная доставка и звук.
5. Выбранное место расчёта исполняет те же функции и данные; переключение RECEIVER↔SERVER сверяется на одинаковых временах/входах, не по похожему среднему Q.

| Обязательная проверка | Точное принятие / отрицательный контроль |
|---|---|
| Одна физическая последовательность | **Почти неискажённая речь → замаскированная речь → Q0 с шумом открытого RX → физические короткие просветы → вновь маскировка**. Нет независимого RNG, случайно «спасающего слова»; профиль/условия/время порождают просветы |
| Доставка и участок речи | В каждом просвете существует соответствующий исходный фрагмент; заранее выбран независимый критерий содержимого/аудио, а не OnReceive/RMS/Q. Параметр звука не восстанавливает ранее удалённый payload |
| Общий приёмник | A→B и C→B на одном RX совместно влияют по профилю; близкий сильный конкурент/селективность/AGC/захват не равны двум несвязанным шумовым дорожкам. A→C и соседние пары сохраняются |
| Устройства/Direct | DIRECT до/во время/после радио, user power/mute/squelch, inventory/drop/respawn/rebind, radio types; чужое состояние не снимается, старый эффект не касается нового поколения |
| Время/отказы | Короткое окно между медленными snapshot, PTT-пауза, взаимный канал, разные порядки вызовов/шаги, stale/unknown/time uncertainty, дубли/перестановки/full resync; причины не становятся тишиной или CLEAN |
| Отсечка | Весь интервал и revisions/coverage/guard, U ниже/равно/выше tolerance, поздний выигрыш, сумма слабых путей, UNKNOWN/resource/no-bound; ZERO_AT_SAMPLE не удаляет будущие фрагменты |
| Два места расчёта | Одинаковые полные входы → согласованная channel/receiver последовательность при заявленной точности и привязке времени; физическая реализация не зависит от места/частоты вызова |

Ближайший результат — доказанный доступ A/B и согласованный temporal contract, не новый voice backend и не обещание production. Корректный модельный ноль, работа receiver squelch и технический отказ имеют разные причины и проверяются раздельно.

## Локальные первичные источники

[TEAM]: <C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-von-temporal-20261104/TEAM.md>
[CT]: <C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/req/REQ-channel-time.md>
[CF]: <C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/req/REQ-channel-fading.md>
[CD]: <C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/req/REQ-channel-descriptor.md>
[CC]: <C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/req/REQ-channel-cutoff.md>
[CT10]: <C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/req/CT10-migration-map.md>
[PS]: <C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/req/REQ-pair-service.md>
[VS]: <C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/req/REQ-von-seam.md>
[RC]: <C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/req/REQ-radio-model-catalog.md>
[LE74–75]: <C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/docs/req/REQ-link-eval.md>


### Уточнение по changelog 1.9

[Карта официальных изменений](../../evidence/native-von-access-20261104/CHANGELOG_19_RU.md): в будущую приёмку входят движение TX при cached route и unpossess чужого персонажа на hosted server. Это не допуск Play/capture и не private VoN actuator. Current owner/init и следующая Input-kernel qualification описаны в ASSET_PREPARATION. Общая temporal цель и полная приёмка сохраняются.
