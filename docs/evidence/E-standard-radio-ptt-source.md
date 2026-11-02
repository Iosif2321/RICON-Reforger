# Штатный radio-PTT: частичная квалификация source

Статус: **SOURCE_ACTION_MAPPING_STATIC_PASS** (независимый review1838), WORKBENCH_STATIC, 2026-11-02. BLOCKED_RUNTIME_ACTION_DELIVERY / BLOCKED_HOLD_RELEASE_SAFETY. Не runtime/transport PASS.

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
- :350–358: если персонаж не ALIVE, entry отсутствует или unusable, вызывается proximity-ветвь с аргументом activate && !unconscious и затем возврат. **Штатный activating radio-action может уйти в direct при не-unconscious; на UP аргумент false.** Это не доказательство фактического capture/слышимости.
- :361–363: activate-ветвь вызывает ActivateVON(transmitType); первая серия обрывается на else. Stop-call доказан второй серией ниже.

## Вторая серия: source mapping и stop-call

Разрешение владельца1781 в ответ на запрос1775: продолжать необходимое в рамках проекта. В этом слоте выполнены ровно четыре дополнительных read-only окна с тем же неизменённым wrapper, process-local Bypass и без мира/Play. До запуска повторно проверен SHA, чужих Workbench/game не было. Для script-окон общие root/ext/path как в первой серии; для config — `-Root '$ArmaReforger:Configs/System' -Ext conf -PathFilter 'chimeraInputCommon.conf'`. MaxFiles1 во всех случаях. Каждый exit0, SearchResources=true/candidate1/done, SCRIPT(E)=0; Steamworks/resource-leak ошибки присутствуют. Managed jobs собраны, Workbench/game0, дерево перед записью чистое.

| Name / дополнительные параметры | Папка | SHA-256 console.log | Строк |
|---|---|---|---|
| `ptt-release-20261102 -Grep 'SetVONBroadcast' -Line 361` | `ricon-wbreader-ptt-release-20261102-20261102-065244` | `9f76e835bfdc0104f673bc10a22be84be9cfe0a5d5982aad7bc2efc51c2dd6ce` | 15:361–375 |
| `ptt-registration-20261102 -Grep 'ACTION_CHANNEL' -Context 0` | `ricon-wbreader-ptt-registration-20261102-20261102-065303` | `577aeedb05eaa72fe14aaa6a4f99992a846a9b72a4883dd703905a50a3703e4c` | 6 matches |
| `ptt-config-20261102 -Grep 'VONChannel' -Line 8858` | `ricon-wbreader-ptt-config-20261102-20261102-065319` | `db4be8b24cabc59b897fdab884ae6c238a3343d4b77fe2b6bf761c8d67046d8b` | 15:8858–8872 |
| `ptt-config-tail-20261102 -Grep 'VONChannel' -Line 8872` | `ricon-wbreader-ptt-config-tail-20261102-20261102-065342` | `409888a908b6554a2565b7ea6ccce43a0a6440ffce71ce7f3c3e503826135bd2` | 15:8872–8886 |

Новые установленные факты:
- SCR_VONController:40 — ACTION_CHANNEL соответствует строке VONChannel. :830–831 — регистрация ActionVONBroadcast на DOWN и UP; :879–880 — снятие этих же listeners. Условия enclosing lifecycle метода этими шестью совпадениями не исследованы.
- :361–365 — при activate вызывается ActivateVON(transmitType), иначе DeactivateVON(transmitType); метод закрывается на365. UP вычисляет transmitType заново по long-range toggle: это не сохранённый токен владения удержанием.
- chimeraInputCommon.conf:8862–8878 — vanilla VONChannel содержит InputSourceSum с keyboard:KC_CAPITAL и FilterPreset hold, gamepad0:shoulder_left и gamepad:hold; явный InputFilterHold с HoldDuration200 относится к gamepad source. Единицы200 отдельно не доказаны. Это default config, не effective user binding или гарантия активного VONContext.

## Не закрыто и следующий gate

Первая серия независимо проверена Enforce в Team1765; SHA/выводы подтверждены с уточнениями null-only auto-select и conditional proximity fallback. Вторая серия независимо проверена Enforce1838: четыре SHA/диапазоны совпали. Точная source-цепь action→DOWN/UP handler→bool dispatch→typed stop закрывает только SOURCE_ACTION_MAPPING. Новые блокеры: BLOCKED_RUNTIME_ACTION_DELIVERY и BLOCKED_HOLD_RELEASE_SAFETY; native ownership не доказан.

Установленный public API ActionManager.SetActionValue(string,float) объявлен в [ActionManager API:365–383](<C:/Program Files (x86)/Steam/steamapps/common/Arma Reforger Tools/Workbench/docs/EnfusionScriptAPI/html/interfaceActionManager.html#L365-L383>) и унаследован InputManager ([API:174](<C:/Program Files (x86)/Steam/steamapps/common/Arma Reforger Tools/Workbench/docs/EnfusionScriptAPI/html/interfaceInputManager.html#L174>)); блок описания пуст. По одной сигнатуре не доказаны delivery DOWN/UP, hold/release, native capture ownership или возможность безопасной синтетической активации VONChannel. Это кандидат отдельного non-VoN input preflight, не готовый PTT actuator. Пользовательские bindings не читались.

Любой будущий PTT-контракт обязан квалифицировать radio entry/handle, ALIVE/usability, long-range/toggle/hold состояния и отсутствие другого автора ввода; смена этих условий инвалидирует radio-only опыт. Source-проверка не доказывает, что состояние останется стабильным во время удержания. Не создавать protected-wrapper и не обходить fallback догадкой. Реальный input, независимый RX/выход и новый runtime gate остаются отдельными требованиями. D0/V0/I0 gates consumed; selector/capture/production BLOCKED.
