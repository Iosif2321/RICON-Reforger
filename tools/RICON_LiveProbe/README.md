# RICON_LiveProbe — прибор live-проверки (TOOL_ONLY)

Отдельный аддон для опытов по `docs/plan/LIVE-order-frequency-mute.md` и `docs/analysis/AUDIO_CHAIN_CONTROL_RU.md`
(трек 1). Не для Workshop, не зависит от `RICON_Core`; подключается только на время сессии через `-addonsDir`.
Запуски — в рамках разрешения владельца 2026-10-23, с START/END и доказательствами на шине.
Статус среза: **проверки наблюдателя в Workbench пройдены; окончательное ревью ожидается**.
Продолжение без ожидания ACK Claude разрешено владельцем 2026-10-23, Agora #4011; окончательная совместная приёмка
оставлена открытой. После возвращения партнёра действует обычный START → ACK → run → END.

## Состав

- `Scripts/Game/RICON_Live/RICON_LiveJournal.c` — чистый журнал событий: кольцевой буфер с последовательным номером,
  явной записью `GAP` при переполнении (счётчик потерь и время последнего потерянного) и строгим форматом строки
  `[RICON_LIVE] seq=… t=… side=… kind=… player=… device=… tr=… value=… text=…`. Engine-вызовов нет.
- `Scripts/WorkbenchGame/RICON_LiveJournalProbePlugin.c` — самопроверка журнала в Workbench (ScriptEditor, без
  мира): порядок, вытеснение, учёт потерь сразу после первого переполнения, ёмкость 2, формат, отсутствие слова FAIL.
- `Scripts/Game/RICON_Live/RICON_LiveObserver.c` — подписки и отписки от штатных invokers:
  `SCR_BaseGameMode.GetOnPlayerSpawned()`, `SCR_VoNComponent.GetOnVoNUsed()`, `m_OnCaptureVON`, `m_OnReceivedVON`.
  Пара клиентских подписок устанавливается целиком либо отклоняется. Привязка к устройствам остаётся UNKNOWN.
- `Scripts/Game/RICON_Live/RICON_LiveProbeComponent.c` — компонент запуска на GameMode тестового мира:
  `Start()`, `Dump()`, `Stop()`, переподключение к VoN управляемого персонажа раз в секунду, очистка в `OnDelete`,
  остановка через 900 секунд игрового времени. В режиме редактирования не подписывается.
- `Scripts/WorkbenchGame/RICON_LiveObserverProbePlugin.c` — 30 проверок наблюдателя: подписки, отказы, игнор
  прямой речи, сохранение качества меньше одной промилле, частичная подписка, отписка и исчезнувший invoker.
  Компонент запуска скомпилирован; его жизненный цикл в Play ещё не проверялся.

## Запуск самопроверки (Workbench, без мира)

```
ArmaReforgerWorkbenchSteamDiag.exe -gproj "<repo>\tools\RICON_LiveProbe\addon.gproj"
  -addonsDir "<Steam>\Arma Reforger\addons,<Steam>\Arma Reforger Tools\Workbench\addons" -noBackend
  -wbModule=ScriptEditor -plugin=RICON_LiveJournalProbePlugin -wbSilent -exitAfterInit -noThrow
  -logLevel verbose -log-scr-checksum -logsDir ricon-livejournal-<stamp>
```

PASS — ровно одна строка `[RICON_LIVEJ] done: PASS`, `SCRIPT (E)` = 0 (строки «Resource leaks» и Steamworks
`RequestCurrentStats` — штатный шум). Exit 0 сам по себе ничего не доказывает.

Для наблюдателя в той же команде заменить плагин на `RICON_LiveObserverProbePlugin`, а каталог журнала — на новый
`ricon-liveobserver-<stamp>`. Требуются `checks=30 passed=30`, один `[RICON_LIVEO] done: PASS`, отсутствие
`mismatch` и `SCRIPT (E)`. Доказательства и ограничения: [E-live-observer.md](../../docs/evidence/E-live-observer.md).

## Подготовка опыта в Play

Рабочий сценарий пока не менялся. В отдельной копии тестового мира добавить ровно один `RICON_LiveProbeComponent`
на `SCR_BaseGameMode`. После запуска Play компонент создаёт ограниченный журнал и устанавливает доступные
подписки. `state=RUNNING` обозначает запуск прибора, а `state=CLIENT_BINDING … attached=1` — подключение к найденному
VoN-компоненту; ни то ни другое не подтверждает слышимость.

`Dump()` выводит текущий буфер в `console.log`; `Stop()` отписывается и выводит буфер, повторный вызов безопасен.
Для ручного управления в Script Console получить экземпляр через `FindComponent(RICON_LiveProbeComponent)`
на текущем GameMode и вызвать нужный публичный метод. Этот способ управления и выполнение компонента в Play
ещё требуют отдельного опыта. В течение сессии контролировать время по внешним часам: предел 900 секунд
в компоненте относится к игровому времени и не заменяет ограничение длительности опыта.

Опрос привязки раз в секунду оставляет ненаблюдаемое окно при появлении/смене персонажа. Ветка активного игрового
редактора, где ванильный контроллер использует иной VoN-компонент, не проверена. Нет сопоставления с epoch/linkId/
lease ядра, передачи журнала между клиентами, команды адресного mute или доступа к PCM.

## Правила журнала

- Теги не содержат слова FAIL (gate `regress_wb.ps1`); ключи шифрования не пишутся.
- Времена — источника события (сторона указана), общей аудиошкалы нет; разрешение не заявляется тоньше каденции
  native-событий.
- `TX_CAPTURE` — колбэк `OnCapture` каждый кадр записи; `TX_START`/`TX_STOP` — только после отдельной детекции фронта.
- `TX_SEEN` сервера не сообщает тип передачи: `channel=UNKNOWN`, даже при известном говорящем.
- В `RX_EVENT` поле `player` — говорящий, `listener` в тексте — локальный слушатель; неизвестные ID равны −1.
  `q` печатается с девятью знаками после запятой, без округления к промилле; это формат вывода, не аудиошкала.
- Частота сохраняется как сырое значение движка без установленной единицы. `device`/`tr` пока −1.
- Отсутствие события не означает тишину. Прибор не вызывает актуаторы звука, не меняет рации и пользовательский mute.
