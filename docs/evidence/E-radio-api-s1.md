# Радиосвязь Reforger — уточнение API после R30

Дата: 2026-10-21. Автор: codex по исследованиям codex.radio_api, codex.von_path и codex.radio_network.
Статус: AGENT-REVIEWED; claude agora #3085. Основание: owner agora #2960; результаты #2973–#2995.
Уровни: DOC и WORKBENCH_STATIC. Play, мир, сервер и звук не проверялись.
Дополняет `LINK_CHAIN_RU.md`, а не заменяет прежние исходные окна.

## Новые и уточнённые границы

| API | Подтверждённый контракт | Что из него не следует |
|---|---|---|
| `BaseTransceiver.SetFrequency(int)` | кГц; DOC разрешает работу на proxies и server; декларация native | Законная authority любого клиента, допустимые значения, шаг и задержка синхронизации |
| `BaseTransceiver.GetFrequency()` | возвращает int; собственное описание единицы отсутствует | Подпись кГц у соседнего GetFrequencyResolution нельзя автоматически считать описанием getter |
| `BaseTransceiver.SetMuteState(bool):bool` | mute трансивера; native-декларация | Направление RX/TX, смысл результата, сохранение состояния, управление одним talker для одного listener |
| `BaseRadioComponent.SetPower(bool)` | включение всей рации; native-декларация | Мощность передатчика в дБм |
| `SocialComponent.SetMuted(otherPlayerID,bool)` | голосовой mute другого игрока данным игроком на время сессии; DOC VoiceChat | Только radio, выбор устройства/частоты, float quality или безопасное снятие пользовательского mute |
| `SocialComponent.IsMuted(otherPlayerID)` | false также при невалидном аргументе | false сам по себе не доказывает успешное снятие mute |
| `RadioManagerEntity.GetTransceiversInRange` | список включённых пространственных кандидатов, радиус в метрах | Готовый набор получателей VoN, фильтрацию частоты/ключа и полноту клиентского набора |

Для `SetFrequency` описание «Supports proxies and server» проверено в установленном
`ArmaReforgerScriptAPIPublic/html/interfaceBaseTransceiver.html:340`, SHA-256
`425bdde345640b18a9185b051bcb765a7361479f92bc39dc6b1b44bcd24aa3e6`.
Для SocialComponent проверены установленная `ArmaReforgerScriptAPIPublic/html/interfaceSocialComponent.html`
(:239–265 и :464–488, SHA-256 `629810cfa0cf17dd592c688544c95b25029033efd1e3d676d429ff20b3aa3def`) и
[официальный API](https://community.bistudio.com/wikidata/external-data/arma-reforger/ArmaReforgerScriptAPIPublic/interfaceSocialComponent.html).
Фактический вызов не исполнялся. Воздействие на direct voice и владение пользовательским mute остаются открыты;
подключения этого метода к `RICON_VoNIntegration` нет.

### Версионное расхождение SetRange

Установка 1.8.0.13: локальный `interfaceBaseTransceiver.html:384–386` и ранее прочитанный
`BaseTransceiver.c:49–53` задают owner(server) и clamp 1…65535 м. Текущая
[официальная страница](https://community.bistudio.com/wikidata/external-data/arma-reforger/ArmaReforgerScriptAPIPublic/interfaceBaseTransceiver.html)
описывает owner(server) и нижнюю границу 1 м; верхняя там не указана. Изменение поведения установленной игры
этим не доказано. 65535 фиксируется как ограничение проверенного build, а не универсальное свойство всех версий.

### Точки наблюдения голоса

Официальная [схема VoN](https://community.bistudio.com/wiki/Arma_Reforger%3AAudio%3A_Voice_over_Network):
захват/OPUS → серверный выбор получателей → декодирование → сигналы и ACP получателя.
`OnReceive`/`OnVoNUsed` — уведомления, не документированные veto/hooks выбора списка получателей.
`SignalsManagerComponent.SetSignalValue(index,value)` не задаёт talker/stream binding; общая запись сигнала
ещё не доказывает подмену native `TransmissionQuality`. Audio master VoiceChat шире одного радиоканала.
`BeginTransmission(BaseRadioMessage)` относится к объекту радиосообщения; захват голоса игрока этим не доказан.

## Новые исходные окна

codex.radio_api исполнил ровно три новых WbReader-запуска, последовательно, скрыто, без мира.
У каждого exit=0, SearchResources=true, пять исходных строк, SCRIPT(E)=0, один done на log:153.
Иные сообщения (E) Steam stats/resource leaks сохраняются; общего «Workbench без ошибок» не заявляем.
Суммарно 15 исходных строк. Старые range/key/getter-окна повторно не запускались.

База: `%USERPROFILE%/Documents/My Games/ArmaReforgerWorkbench/logs/`.

| Папка, файл `console.log` | Источник, log:148–152 | SHA-256 |
|---|---|---|
| `ricon-wbreader-s2-api-frequency-codexradio-20261021-101359` | `BaseTransceiver.c:27–31` | `76cfa7981a7821b9ea95ec18abe4a0c4165493c4f52f8cbbe814d3f6d79ccf16` |
| `ricon-wbreader-s2-api-mute-codexradio-20261021-101430` | `BaseTransceiver.c:40–44` | `c5e579480da1ee37aef001c1504a7706e4635d53e561980fea3ed7325204d9e8` |
| `ricon-wbreader-s2-api-power-codexradio-20261021-101503` | `BaseRadioComponent.c:40–44` | `66b846960e841ab4495aec0d39e99592b6a0a9dcd548057a2d582fb8e302d644` |

Команды — штатный `tools/RICON_WbReader/run.ps1`, `-Root '$ArmaReforger:scripts/Game/generated/Radio'`,
`-Ext c -Context 2 -MaxFiles 1`, соответственно `-Grep SetFrequency`/`SetMuteState`, `-PathFilter BaseTransceiver.c`.
Для power: root `.../generated/Components`, `-Grep SetPower -PathFilter /BaseRadioComponent.c`.
Логи/хэши проверил субагент; основной codex сверил выводы с существующими контрактами и границами evidence.

## Следствия для оболочки устройства

- Нужна явная связь server deviceId/epoch ↔ entity/BaseRadio ↔ transceiver index/handle; проверка индекса и
  принадлежности радио не заменяется `GetOwner`, который возвращает entity, а не server authority.
- Частота, power-state, ключ и mute — наблюдаемое состояние движка. TX дБм, gain, NF, bandwidth,
  чувствительность/required SINR и точка антенны — отдельный явный серверный каталог/поставщик.
- Количество трансиверов не принимается за число UI-каналов/пресетов без отдельного требования.
- Relay/Intercom имеют унаследованный API; доставка VoN через реле, membership машины и изоляция пар не проверены.
- Coverage Conflict и Rpl relevancy не объявляются графом доставки голоса.

Следующие опыты — в `WB-order-link-quality.md`, только после подготовки приборов и отдельного разрешения:
authority/атомарность настроек; полнота discovery; три клиента для pair mute с отдельными direct/radio-контролями;
проверка relay/intercom. Существующий `BLOCKED_NO_ACTUATOR` остаётся.
