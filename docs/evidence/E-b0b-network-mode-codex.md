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
Меню прочитано, настройка обратимо переключена без Play:
[операторское ревью](E-b0b-operator-review-codex.md). Следующий ограниченный опыт
должен установить режим, процессы и адрес. Исправления PID-reuse/teardown
проверены отдельно; новый runner для Play проходит встречное ревью.
Сетевое соединение, отдельный клиент и голос не проверялись.

## Подготовка чтения меню: inspect

Первый осмотр остановился на неизвестном стартовом заголовке Workbench, второй
дошёл до срока ожидания: без открытого мира целевое окно World Editor не появилось.
Меню не читалось и не активировалось. Добавлен `-riconBenchMode=inspect`: после
SetOpenedResource и GetApi launcher пишет OPEN_ONLY и возвращается до создания
сущностей или SwitchToGameMode. Это открытие edit-мира, не запуск Play.

Изменение скомпилировано в `ricon-b0b-enum-codex-20261025-113048`, SHA
`a1141cdbab13bb46ce92492cda921b54195af2197be102fda28ae5e6f2f853e4`;
0 SCRIPT(E), enum 4/4, исходники стабильны. Затем inspect исполнялся в опытах
чтения меню и roundtrip, сохраняя OPEN_ONLY saved=0 isGameMode=0.

## Возврат после запуска Play через меню

Для следующего опыта inspect с включённым EndpointProbe регистрирует наблюдатель
до OPEN_ONLY и запускает ограниченное ожидание внешнего Play. Сам launcher
Play не запускает. После 30 срезов либо 45 с наблюдения он вызывает SwitchToEditMode.
Общий WaitForStableEdit использует прежнюю проверку одного edit-мира 51 раз за
не менее 5 с; старый тест повторного Play вызывает ту же функцию. Без probe
inspect по-прежнему только открывает мир.

Компиляция `ricon-b0b-enum-codex-20261025-125005`: SHA
`7eba9d4bc5dcf1a6265e434d5ff0b5b649c997faa8aecfb1ce05caafc9bf1942`,
launcher CRC 35ddbc56, enum 4/4, SCRIPT(E)=0, 19 исходных файлов стабильны,
процессов после 0. Новая ветка возврата требует живого опыта; компиляция его
не заменяет.

## Первый запуск серверного режима: частичный результат

Оператор claude, Agora #4713; исходный журнал
`ricon-b0b-serverplay-claude-20261025-125954`, SHA
`e637bf0ac77b742ae7b07be97cc818587d275bdb450a02bb05da8b162d1b8c96`.
Codex независимо сверил SHA и следующие строки/отсутствия:

- registered=1 перед OPEN_ONLY saved=0 isGameMode=0 и WAIT_PLAY.
- Движок сообщил запуск RPL server на ANY:2001 и RplStartupEvent Started.
- Один ENDPOINT_START: isServer=1, isClient=0, rplMode=2.
- RICON_NET: rplName=Listen, listenPresent=1.
- SCRIPT(E)=0; ENDPOINT_SAMPLE/DONE/ABORTED отсутствуют, как и WBMODE START,
  inspect PLAY_ENTERED и EDIT_STABLE.

Это подтверждает **начало Listen в данном запуске**, но не полный жизненный цикл,
доступность порта с клиента или передачу голоса. По операторской квитанции вызов
UIA Invoke не возвращался до ошибки; runner в это время не выполнял Tick и
не наблюдал окна/сетевые точки. Итог запуска — PLAY_INVOKE_ERROR,
SETTING_MAY_REMAIN_ON; собственный процесс завершён. Отсутствие потомков отражает
наблюдение оператора с этим пробелом, а не непрерывную проверку всех процессов.
Гипотеза о модальном запросе пока не доказана.

Отдельный последующий осмотр `ricon-b0b-menu-claude-20261025-130447`, SHA
`5a0c6296a2acb383b3f07eada53be8db6ef977a1d0289c5779c5e7b064c092ed`,
прочитал оба серверных пункта Off и исходные остальные настройки (#4718).
Codex сверил SHA, один OPEN_ONLY, SCRIPT(E)=0 и отсутствие Play; UI-снимок
и равенство реестра/профиля проверены оператором. Восстановительных переключений
не понадобилось. Этот осмотр уточняет состояние после опыта, не превращая
неуспешный запуск в полный PASS.
