# Прямой голос: управляющая цепочка и граница доказательств

Дата: 2026-10-21, спринт 23. Автор: codex. Статус: DRAFT до ревью claude.
Мини-intent/владение: agora #2381/#2384. Задача владельца — #2020.

**Результат:** прослежена скриптовая цепочка прямого PTT до native-запроса захвата VoN.
Режим DIRECT настраивается без выбранного радиотрансивера. Численная дальность, затухание звука,
препятствия, шум окружения и слышимость слушателя по этим источникам не установлены.
Уровень — `WORKBENCH_STATIC` и `DOC_EVIDENCE`; Play, мир, сервер и Audio Editor не запускались.

## Цепочка управления

Префикс C: `$ArmaReforger:scripts/Game/VON/SCR_VONController.c`.
Префикс N: `$ArmaReforger:scripts/Game/generated/Components/VoNComponent.c`.

| Звено | Источник | Установлено |
|---|---|---|
| Ввод | C:827–829 | Нажатие/отпускание ACTION_DIRECT вызывает ActionVONProximity; toggle имеет отдельный обработчик |
| PTT | C:420–434 | Обработчик вызывает SetVONProximity с признаком, что действие не UP; отдельно учитывает toggle |
| Допуск direct entry | C:301–315 | Нужны VoN-компонент и usable direct entry; повторная активация уже выбранного DIRECT пропускается; далее ActivateVON/DeactivateVON |
| Выбор entry | C:526, :530, :604–618 | Для DIRECT выбирается m_DirectSpeechEntry и передаётся в основной ActivateVON |
| Выбор типа | C:538–552 | Основной ActivateVON различает radio/direct; проверка INCAPACITATED в показанном условии относится к типу, отличному от DIRECT |
| Настройка native | C:624–638 | Радиоветвь задаёт SQUAD_RADIO и трансивер; другая ветвь задаёт DIRECT и SetTransmitRadio(null) |
| Захват | C:577–579 | После SetActiveTransmit вызывается SetCapture(true), затем m_bIsActive=true |
| Остановка | C:594–596 | Скриптовый флаг сбрасывается и вызывается SetCapture(false) |
| Native-граница | N:25, :27–31 | SetCapture возвращает bool; SetCommMethod и SetTransmitRadio — proto external |
| Уведомление о приёме | E-workbench-source.md, пункт 4; SCR_VoNComponent.c:78–86 | Проверенный в R20 обработчик передаёт данные в display/invoker; решение о слышимости там не видно |

Это управляющая цепочка. Из неё не следует, что событие каждого кадра, индикатор передачи или объект entry
равны слышимому звуку на другом клиенте. Срабатывание direct OnReceive и значения receiver/frequency/quality
нужно проверить отдельно; принадлежность DIRECT к той же акустической кривой, что radio, не предполагается.

## Почему active не доказывает захват и доставку

Офлайн API `interfaceVoNComponent.html`, член SetCapture: булев результат сообщает об успехе **запроса**
запуска/остановки захвата. В C:578–579 возвращаемое значение не проверяется, скриптовый флаг ставится сразу.
Поэтому m_bIsActive сам по себе не подтверждает даже успех этого запроса. Это наблюдение о коде и диагностике,
не утверждение о воспроизведённом отказе ванильного VoN.

Даже успешный запрос не доказывает запись полезной речи, доставку, воспроизведение или разборчивость.
Для будущего опыта следует разделять запрос, события захвата, событие приёма и оценку звука участником.

## Какие настройки доступны в проверенном API

Офлайн API и восемь native-деклараций N проверены независимо. В собственных публичных членах
VoNComponent найдены управление захватом, выбор direct/radio, один transmit transceiver, служебные методы
редактора, AI sound-event и callbacks. Регулятор дальности DIRECT, mute/volume/filter на пару
«говорящий — слушатель» в этом классе не найден.

Это ограничение **обследованного интерфейса**, а не доказательство невозможности во всём движке.
Другие аудиокомпоненты, граф VON.acp, конфиги и runtime остаются отдельным исследованием.
SoundEventPriority описан как перенос AI sound message; управление им голосом игрока не подтверждено.

Источник API: установленный Tools, `Workbench/docs/ArmaReforgerScriptAPIPublic/html/interfaceVoNComponent.html`,
SHA-256 `5b04e19f815b5a1d50a52696f42470bc6bd9cf6228af69487843c02689d4a212`.
Текст API сообщает об успехе запроса, но не даёт здесь отдельного подтверждения доставки/звука.

## Что это означает для RICON

- REQ-von-seam.md:23 сохраняет direct voice без изменений; строка :126 запрещает неявный fallback радио на direct.
- Текущий Core считает радиобюджеты с частотой в МГц и передаёт готовые снимки через fail-closed seam.
  Акустическая модель прямой речи и её исполнитель в этот этап не входят.
- Нельзя переносить BaseTransceiver.SetRange на DIRECT: проверенная ветвь не выбирает радиотрансивер.
  Из этого не выводится ни постоянная дальность прямой речи, ни её отсутствие.
- SINR/Q радиобюджета, native quality и понятность прямой речи — разные величины. Их связь здесь не измерена.

## Воспроизводимость чтения

Инструмент — существующий `tools/RICON_WbReader/run.ps1`, без изменений. Среда первого нового лога:
ArmaReforgerScripted 1.8.0.13, сборка 2026-08-21. Каждый запуск дал SearchResources=true,
одного кандидата, финальный done, exit 0; SCRIPT(E) не обнаружен. В общих логах присутствуют прежние
Steamworks/resource-leak сообщения, поэтому «ноль всех ошибок движка» не заявляется.

Пример адресного повторения из корня проекта (Steam запущен):

```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass -Force
& .\tools\RICON_WbReader\run.ps1 -Name direct-repeat -Root '$ArmaReforger:scripts' -Ext c -Grep 'SetActiveTransmit' -PathFilter 'SCR_VONController.c' -Line 624 -MaxFiles 1
& .\tools\RICON_WbReader\run.ps1 -Name direct-native-repeat -Root '$ArmaReforger:scripts' -Ext c -Grep 'proto' -PathFilter 'generated/Components/VoNComponent.c' -Context 0 -MaxFiles 1
```

Остальные окна повторяются заменой Name, Grep/Line/Context по таблице ниже. Все папки находятся
относительно `Documents/My Games/ArmaReforgerWorkbench/logs/`, файл console.log.
Контроллер — PathFilter `SCR_VONController.c`, кроме строки native-api.

| Папка | Grep; Line либо Context | SHA-256 console.log |
|---|---|---|
| `ricon-wbreader-r23-proximity-index-20261021-003740` | SetVONProximity; Context=2 | `8cae1ab99796fe7c09dc5db953e61033a6ba8515207574c84ce8c43cce305888` |
| `ricon-wbreader-r23-proximity-body-20261021-003803` | SetVONProximity; Line=301 | `95f66dc139486c7298fac00b2c515b62ae89b3d2d0ff65520a1de51a9362667a` |
| `ricon-wbreader-r23-activate-index-20261021-003905` | ActivateVON(; Context=0 | `863823109a685d596f5e448a012439dec2a692e417dbf6ad7d22a97b70ae867e` |
| `ricon-wbreader-r23-activate-body-20261021-003933` | ActivateVON; Line=538 | `4d3c18cccb070f31359eb6855ae3f6c1acec40c6d8e0eeaf8721c45f8b6739da` |
| `ricon-wbreader-r23-activate-capture-20261021-004131` | ActivateVON; Line=553 | `4d332833ddaf6b7ecc884e30716e8e6f5433cb3b0e46dbc12e8d4aae75417c82` |
| `ricon-wbreader-r23-capture-index-20261021-004210` | SetCapture; Context=1 | `0ab79bad41742644f3d1bdc385460b68a0b0720f1ebe12fbcea0f7e3c90def15` |
| `ricon-wbreader-r23-direct-native-select-20261021-004249` | SetActiveTransmit; Line=624 | `13027aa9a26779713534e1ecea453214d37056cd906681078c091ee267f69e0b` |
| `ricon-wbreader-r23-von-native-api-20261021-004338` | proto; Context=0; generated/Components/VoNComponent.c | `6a44627d6208a59d59d754c8ae8fd4800a8f6816a45a02ae27abb0cacda4e2d3` |
| `ricon-wbreader-r23-direct-input-bindings-20261021-004422` | ActionVONProximity; Context=0 | `413873e97d430842fd5713c73c118f2e311f96d59b8302c04f4380e2642bb51e` |
| `ricon-wbreader-r23-direct-input-body-20261021-004529` | ActionVONProximity; Line=420 | `775058595761d28836c4f0b0409e212e8263db707e395f967fafd65b7d02d024` |
| `ricon-wbreader-r23-direct-entry-index-20261021-004611` | GetEntryByTransmitType; Context=0 | `aba676f6ea96164c6317f1e369d8dd44bfbaefa04d1916a6ca67a93d7ffd0939` |
| `ricon-wbreader-r23-direct-entry-body-20261021-004629` | GetEntryByTransmitType; Line=604 | `c365f7eb37f8eacb24738804b7929263e254cb5675f81d5f631fb45d96b97a7e` |

На запуск напечатано не больше 15 исходных строк. Два окна ActivateVON538–567 ещё не показывали захват:
обоснование дополнительного адресного grep SetCapture записано в agora #2401 (правило #789).
Последовательная выгрузка файла не выполнялась, ванильный код в репозиторий не переносился.

## Следующая проверка

Сначала Audio Editor для direct-ветви VON.acp и её сигналов. Затем после отдельного разрешения владельца —
контролируемый опыт двух участников по `docs/evidence/WB-order-link-quality.md`.
Пока неизвестны дальность/направленность речи, действие стен/шума, соответствие quality звуку,
возможность воздействия на отдельного слушателя и работа на нескольких клиентах.
