# Штатный radio-PTT: частичная квалификация source

Статус: **BLOCKED_SOURCE_ACTION_MAPPING**, WORKBENCH_STATIC, 2026-11-02. Не runtime/transport PASS.

## Метод и границы

Разрешение владельца: Team event1724, в ответ на узкое предложение1710. Четыре процесса Windows PowerShell с `-NoProfile -ExecutionPolicy Bypass -File .\tools\RICON_WbReader\run.ps1`; только существующий read-only wrapper SHA-256 `fff482755bd396a050734a8f153606dea3aeccc1b369dac5f8d541b83a6c659c`. Постоянная policy/Zone.Identifier и код не менялись. Первый вызов имени `pwsh` не нашёл executable; Workbench при этой ошибке не создавался.

Общие параметры всех четырёх успешных команд: `-Root '$ArmaReforger:scripts/Game/VON' -Ext c -PathFilter 'SCR_VONController.c' -MaxFiles 1`. ScriptEditor скрытый, без мира/Play; wrapper удерживает свой process handle, таймаут300с. Каждое окно: exit0, SearchResources=true, candidate1, done, 15 исходных строк. В журналах есть Steamworks RequestCurrentStats и resource-leak (E); это не основание объявлять журналы свободными от ошибок. Все четыре managed jobs собраны; после серии Workbench/game процессов не обнаружено, git status был чистым.

Логи находятся вне репозитория: `C:/Users/Ded/Documents/My Games/ArmaReforgerWorkbench/logs/<папка>/console.log`. Vanilla текст не копируется в репозиторий.

| Name / дополнительные параметры | Папка | SHA-256 console.log | Прочитано |
|---|---|---|---|
| `ptt-handler-20261102 -Grep 'void ActionVONBroadcast' -Context 7` | `ricon-wbreader-ptt-handler-20261102-20261102-064408` | `7afcdaa2b30b59ef6c89405559c29005e2dbd3c203884c06b40b3393d6ed35ff` | 443–457 |
| `ptt-handler-tail-20261102 -Grep 'ActionVONBroadcast' -Line 457` | `ricon-wbreader-ptt-handler-tail-20261102-20261102-064427` | `3f04752118b5bb953ff0b45e76984a8e11d24aff177a4bd7ed1850b1882354e2` | 457–471 |
| `ptt-dispatch-20261102 -Grep 'void SetVONBroadcast' -Context 7` | `ricon-wbreader-ptt-dispatch-20261102-20261102-064454` | `14610ec488deba3e8c8d50f745aff9b09c1ceb880e6ce2129fb906890b4aae40` | 334–348 |
| `ptt-dispatch-tail-20261102 -Grep 'SetVONBroadcast' -Line 349` | `ricon-wbreader-ptt-dispatch-tail-20261102-20261102-064515` | `5bcd05c789e415194e9fecfa25109e928f6a2c1767a3b5535507fd6d999deb34` | 349–363 |

## Установлено (пересказ, не копия vanilla)

Источник всех строк — `$ArmaReforger:scripts/Game/VON/SCR_VONController.c`.

- :450–453: protected ActionVONBroadcast, reason по умолчанию UP, выход без VoN component.
- :457–465: transmitType выбирается LONG_RANGE при long-range toggle, иначе CHANNEL. Для не-UP и уже выбранного типа ранний выход; иначе SetVONBroadcast получает bool «reason не UP» и этот тип.
- :467–468: при не-UP сохраняется active hold action ACTION_CHANNEL. Из этого не следует native capture ownership.
- :341–348: protected SetVONBroadcast; выход без VoN component; без active entry пытается выбрать первую unmuted entry и заменяет transmitType результатом выбора. Это не read-only операция и не гарантия сохранения radio selector.
- :350–358: если персонаж не ALIVE, entry отсутствует или unusable, вызывается proximity-ветвь с учётом activate/unconscious и затем возврат. **Штатный radio-action может уйти в direct.**
- :361–363: activate-ветвь вызывает ActivateVON(transmitType); виден else, но его тело за пределом окна и не объявляется доказанным.

## Не закрыто и следующий gate

Лимит четырёх окон исчерпан продолжениями handler/dispatch. Регистрация VONChannel/ACTION_CHANNEL с точными trigger types и config binding не читались в этой серии; тело else после363 не прочитано. Ранее подтверждённая нижняя Activate/Deactivate-цепочка не заменяет эти пробелы. Следующий шаг — независимый review сохранённых окон и минимальные адресные дочитки; дополнительные обходящие policy процессы не разрешены текущим четырёхоконным исключением.

Любой будущий PTT-контракт обязан квалифицировать radio entry/handle, ALIVE/usability, long-range/toggle/hold состояния и отсутствие другого автора ввода; смена этих условий инвалидирует radio-only опыт. Source-проверка не доказывает, что состояние останется стабильным во время удержания. Не создавать protected-wrapper и не обходить fallback догадкой. Реальный input, независимый RX/выход и новый runtime gate остаются отдельными требованиями. D0/V0/I0 gates consumed; selector/capture/production BLOCKED.
