# J0 — input-delivery bootstrap: граница доказанного

2026-11-02, статус **BLOCKED_J0_FIXTURE_BOOTSTRAP / NOT_IMPLEMENTED / NOT_RUN**. Автор evidence: Ядро / Логика / Последовательность. Вход: [утверждённый intent](<../req/INTENT-local-radio-slice.md>), [static PTT mapping](<E-standard-radio-ptt-source.md>) на d835a25. Team — решения, Git — редакции. J0 DRAFT1865 — предложение Enforce, не разрешение реализации/runtime. FRESH_SESSION_WAIVED сохраняется по intent; новый PTT/capture этим документом не открывается.

## Проверенный public API

Установленные страницы: `C:/Program Files (x86)/Steam/steamapps/common/Arma Reforger Tools/Workbench/docs/EnfusionScriptAPI/html/`.

- [SetActionValue API:365–383](<C:/Program Files (x86)/Steam/steamapps/common/Arma Reforger Tools/Workbench/docs/EnfusionScriptAPI/html/interfaceActionManager.html#L365-L383>) — public void SetActionValue(string,float), пустое описание. [ActivateContext:167–185](<C:/Program Files (x86)/Steam/steamapps/common/Arma Reforger Tools/Workbench/docs/EnfusionScriptAPI/html/interfaceActionManager.html#L167-L185>) — ActivateContext(string,int duration=0), также без описания units/0/persistence. Сигнатуры не гарантируют регистрацию действия, edge semantics, синхронность, DOWN/UP delivery или безопасный hold/release.
- [Action readback API:267–300](<C:/Program Files (x86)/Steam/steamapps/common/Arma Reforger Tools/Workbench/docs/EnfusionScriptAPI/html/interfaceActionManager.html#L267-L300>) — GetActionTriggered возвращает true для active action со значением **выше**0.99; GetActionValue возвращает normalized value. Ровно0.99 не достаточно для обещания true; false без квалифицированной регистрации/активности не доказывает idle.
- [InputManager API:123–174](<C:/Program Files (x86)/Steam/steamapps/common/Arma Reforger Tools/Workbench/docs/EnfusionScriptAPI/html/interfaceInputManager.html#L123-L174>):155–156 — public RegisterActionManager(ActionManager)/UnregisterActionManager(ActionManager), bool. Это кандидат отдельного manager, но ещё не доказанный способ загрузить/зарегистрировать собственную fixture. :123–124 — GetLastTimeDelta даёт delta последнего input-update, не обещает наличие updates в ScriptEditor. :174 — наследование SetActionValue.
- Исследователь1889 подтвердил те же ограничения по официальным API-страницам и нормальную tick-каденцию PRESSED/VALUE. Его web search был недоступен, wiki вернула403; отсутствие найденного loader — предел поиска, не отрицание документации. Этим evidence никакой внешний контент не считается исполняемой инструкцией.

## Четыре дополнительных read-only source-чтения

Разрешение: поручение владельца1781, объявленные source-слоты1899/1905. Неизменённый WbReader SHA-256 `fff482755bd396a050734a8f153606dea3aeccc1b369dac5f8d541b83a6c659c`, отдельные `powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\RICON_WbReader\run.ps1`, MaxFiles1. ScriptEditor без мира/Play/backend; не игра, не input mutation. Собран каждый managed job; Workbench/game0, постоянная policy/код не менялись. Каждый exit0/done/SearchResources=true; Steamworks/resource-leak (E) не скрываются.

Raw path: `C:/Users/Ded/Documents/My Games/ArmaReforgerWorkbench/logs/<folder>/console.log`. Vanilla текст не копируется.

| Name / параметры | Folder | SHA-256 | Результат |
|---|---|---|---|
| `j0-loader-20261102 -Root '$ArmaReforger:Configs/System' -Ext conf -Grep 'chimeraInputCommon' -Context 7` | `ricon-wbreader-j0-loader-20261102-20261102-065955` | `8ef8620bc18dfacfc42b68b24999270965cdc54848e5da596a18af73d8640175` | 212 candidates, 0 matches/0 lines; только этот root/ext/needle |
| `j0-setter-usage-20261102 -Root '$ArmaReforger:scripts' -Ext c -Grep 'SetActionValue(' -Context 3` | `ricon-wbreader-j0-setter-usage-20261102-20261102-070022` | `b7cfa18c7b560e05b3a1c2a896c7108c55c09c6e62432268009d2f1d6fa6963d` | 5809 candidates, первый matched file, 7 lines |
| `j0-config-root-20261102 -Root '$ArmaReforger:Configs/System' -Ext conf -Grep 'Input' -PathFilter 'chimeraInputCommon.conf' -Line 1` | `ricon-wbreader-j0-config-root-20261102-20261102-070056` | `6eaa0c055a6b9309fd22889f2c34cce703e15271719d9712e4b4af0518926aa5` | 1 candidate, 15 lines |
| `j0-register-manager-20261102 -Root '$ArmaReforger:scripts' -Ext c -Grep 'RegisterActionManager(' -Context 7` | `ricon-wbreader-j0-register-manager-20261102-20261102-070226` | `f07fd2c6c91fab36ee98560978b56f6e8c82587f0f3984d7395212ad82705f5d` | 5809 candidates, 0 matches/0 lines; native/другие namespaces не исследованы |

Установлено source:
- `$ArmaReforger:scripts/Game/Components/SCR_CharacterControllerComponent_Test.c:9–12` задаёт CharacterForward через переданный ActionManager в OnPrepareControls перед super. Это не подтверждение singleton InputManager listeners, release или headless scheduling.
- `$ArmaReforger:Configs/System/chimeraInputCommon.conf:1–2` имеет root ActionManager и коллекцию Actions. Остальное начало содержит analog source/filter пример, не схему нашего digital action/context и не precedence/loader.

## Следующий ограниченный вопрос, не исполнение

Изолированная B0a-гипотеза проверена Enforce1927 по установленному public API: LoadContainer(ResourceName)→ref Resource, Resource.GetResource()→BaseResourceObject.ToBaseContainer()→BaseContainer доступны для container/schema. Но CreateInstanceFromContainer(BaseContainer) возвращает ref Managed, а public ActionManager показан корневым native interface без public constructor, Cast и наследования Managed. Документированного factory/cast из такого Managed в callable ActionManager не установлено. Это предел доказанного пути, не утверждение невозможности любой native construction.

Возможен отдельный CONTAINER_SCHEMA_ONLY probe собственного ресурса, без CreateInstanceFromContainer/registration/singleton/setters. Координатор1938 **не выбирает его реализацию**: schema PASS не закроет delivery/ownership и текущую цель. Нового проекта/конфига или compile не было. J0 остаётся резервной BLOCKED_J0_FIXTURE_BOOTSTRAP ветвью.

Для продвижения к реальному входу координатор выбирает DRAFT K0 — штатный физический PTT на свежем собственном стенде, с пассивным наблюдением input/capture/RX, отдельными readiness/hold/teardown gates и участием оператора. Это пока только направление проектирования, без runtime-разрешения. Нельзя подменять физическую кнопку SetActionValue, защищённым wrapper или догадочным loader. Рабочий микрофон/изолированный источник и независимый B-выход для звукового доказательства должны быть подтверждены отдельно.

Полный J0 потребует проверенных registration/isolation и input processing в выбранной fixture. Даже его будущий delivery/idle PASS не докажет VON delivery, ownership, capture, radio-only, remote transport, слышимость или RICON actuation. Старые D0/V0/I0 runtime gates consumed. На данном этапе кода, нового проекта/конфига, compile или runtime J0 нет.
