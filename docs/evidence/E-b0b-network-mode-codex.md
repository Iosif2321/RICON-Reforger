# B0b: штатный режим сети и наблюдение выбора Play

Дата: 2026-10-25. Автор: codex. Разделение работ согласовано claude #4616/#4621.
Статус: **SOURCE + COMPILE + NATIVE_ENUM_SELFTEST; SERVER_NOT_RUN**.

## Что установлено

[Квитанция](receipts/b0b-mode-source-codex.json) содержит пять чтений WbReader
и отдельный исполненный self-check enum. Все без SCRIPT(E), без мира и клиента.

- `$core:scripts/GameLib/generated/RplMode.c:8–14` объявляет None, Client,
  Listen, Dedicated без явных чисел. Самопроверка в ScriptEditor подтвердила
  числа и строки через EnumToString: **0=None, 1=Client, 2=Listen, 3=Dedicated**, 4/4.
  Следовательно, прежний B0a с rplMode=0 наблюдался без сетевой репликации;
  isServer=1 сам по себе не доказывал слушающий сервер.
- `$core:scripts/WorkbenchGameCommon/PeerTool.c:149–164`: PeerPlugin вызывает
  Start только для точной строки `Server localhost + PeerTool`; на окончании
  режима вызывает End. Это не описывает процессы самой C++-ветки Server localhost.
- ExecuteAction найден только в декларации WBModuleDef.c:22. Поиск его вызовов
  в 759 core-скриптах и 5809 игровых скриптах не дал штатного примера menuPath.
  Путь меню не выдумывается; следующее действие — наблюдение интерфейса.

## Минимальные диагностические изменения

Новый `RICON_LivePlayModeObserverPlugin` — WorldEditorPlugin с двумя gated-хуками:
OnGameModeStarted пишет `[RICON_WBMODE] event=START mode=… world=… fromCamera=…`,
OnGameModeEnded — `event=END`. Строки экранируются общим кодеком. Хуки ничего
не выбирают в интерфейсе и не меняют режим. Для ScriptEditor RunCommandline
исполняет только четыре проверки enum и завершает процесс.

Launcher при включённом EndpointProbe проверяет существование этого плагина
через GetPlugin до Play. Отсутствие экземпляра даёт registered=0 и отказ.
EndpointProbe добавляет отдельную строку `[RICON_NET]` после START: cycle,
rplMode, rplName, listenPresent. Сырое значение TryGetListenAddress не печатается.
Схема RICON_LIVE, seq и dump сохранена; новые строки разбираются отдельно.

Source-review черновика — claude #4628. Компиляция/самопроверка:
`ricon-b0b-enum-codex-20261025-111047`, SHA
`b8c168b52c5cbb6627a7dfcf46f3bb26a277bcc6101582fb95d200d5da3e575a`.
CRC: EndpointProbe e655d126, launcher 053a9809, новый plugin 792e74b9;
EndpointGame 45b337e3 не менялся. 19 исходных файлов стабильны, процессов после — 0.

## Что ещё требуется

Нативные вызовы хуков в World Editor и GetPlugin ещё не проверены в этом снимке.
Нужно прочитать фактическое меню, подготовить ограниченный опыт и наблюдать
самостоятельно режим, процессы и адрес. Новый runner claude проходит встречное
ревью: до исправления обнаруженных ошибок PID-reuse/teardown он не применяется
к серверному опыту. Сетевое соединение, отдельный клиент и голос не проверялись.

## Подготовка чтения меню: inspect

Первый осмотр остановился на неизвестном стартовом заголовке Workbench, второй
дошёл до срока ожидания: без открытого мира целевое окно World Editor не появилось.
Меню не читалось и не активировалось. Добавлен `-riconBenchMode=inspect`: после
SetOpenedResource и GetApi launcher пишет OPEN_ONLY и возвращается до создания
сущностей или SwitchToGameMode. Это открытие edit-мира, не запуск Play.

Изменение скомпилировано в `ricon-b0b-enum-codex-20261025-113048`, SHA
`a1141cdbab13bb46ce92492cda921b54195af2197be102fda28ae5e6f2f853e4`;
0 SCRIPT(E), enum 4/4, исходники стабильны. Сам режим inspect ещё не исполнялся.
