# R26: кандидат чтения ACP через документированный API

Дата: 2026-10-21. Автор: codex. Статус: DRAFT до ревью claude.
Основание: owner agora #2020, mini-intent #2551/#2555, границы #2557/#2566/#2567.

**На этом шаге:** кандидат найден и скомпилирован; сам ACP ещё не прочитан.
GetContainer не доказывает, что конкретный ACP выдаст граф. TransmissionQuality, кривые и звук остаются OPEN.
Уровни: DOC_API_CANDIDATE и WORKBENCH_COMPILE_PASS, не чтение узлов и не аудио/runtime.

## Инвентаризация

`tools/RICON_WbReader/audit_audio_api.py` читает только четыре локальных HTML-страницы Script API,
проверяет точные методы и печатает SHA, строку HTML и сигнатуру. Не запускает Workbench и не пишет файлы.
Неполная/изменившаяся страница вызывает отказ. Host self-check2/2: извлечение метода и отказ неполной страницы.
База: `C:/Program Files (x86)/Steam/steamapps/common/Arma Reforger Tools/Workbench/docs/EnfusionScriptAPI/html/`.

| Страница | Строки и методы | SHA-256 |
|---|---|---|
| `interfaceAudioEditor.html` | 123 SetOpenedResource;124 GetContainer;125 GetNumContainers | `54b11f2513dbd327884d5f25520c26aa1e9619213aae5e8679c18809b17ffe9f` |
| `interfaceWBModuleDef.html` | 132–134 те же методы | `ca3377a5fa8aea08913f782ed770fe9421e8adfe6abb40d40c594d6aa13d550d` |
| `interfaceWorkbench.html` | 111 GetModule(TypeName) | `6da66bb00dd8cadb84463605e7272d93a01e179c8bd651dbdc614d3a68bbdf0f` |
| `interfaceBaseContainer.html` | 130 GetClassName;141 GetVarName;142 GetNumVars | `95be347844ef88ace63101ab84658692656b7fa9c848fbf331362a980a45f547` |

AudioEditor наследует методы WBModuleDef. У GetContainer нет описания семантики ACP:
сигнатура не подтверждает доступ к узлам или влияние на VoN.

```powershell
python -B -m unittest discover -s tools/RICON_WbReader -p 'test_audit_audio_api.py'
python -B -I tools/RICON_WbReader/audit_audio_api.py 'C:\Program Files (x86)\Steam\steamapps\common\Arma Reforger Tools\Workbench\docs\EnfusionScriptAPI\html'
```

## Ограниченный плагин

`RICON_AudioContainerProbePlugin.c` — в существующем TOOL_ONLY WbReader:
- без `-riconAcpInspect=yes` завершает работу до открытия модуля/ресурса;
- GetModule(AudioEditor), SetOpenedResource только `$ArmaReforger:Sounds/VON/von.acp`,
  GetNumContainers, GetContainer(0); класс корня и максимум14 имён полей (всего15 строк данных);
- значения и дочерние объекты не читает, другие ресурсы не открывает;
- недоступный модуль/ресурс/корень — INCOMPLETE и exit4, а не «узлов нет»;
- результат возможного успеха — только METADATA_ONLY;
- нет Resource.Load, Save, ExecuteAction, Set полей, FileIO, мира, Play или сетевых вызовов.

Owner #2020 уже разрешил ACP через Workbench. #791/#794 были соглашением агентов о прежнем механизме;
новый ограниченный механизм повторно согласован #2567. Лишний owner-gate не вводится.
Условия: Hidden, -wbSilent, -exitAfterInit, -noBackend, без GUI-взаимодействия/Play/мира.
Если понадобится изменение ресурса или GUI — остановиться и обратиться к владельцу.

## Только компиляция, без выполнения кандидата

Проект `tools/RICON_WbReader/addon.gproj`, `-wbModule=ScriptEditor -validate`, без -plugin.
В успешных логах нет меток [RICON_ACP]. База: `Documents/My Games/ArmaReforgerWorkbench/logs/`, файл console.log.

| Папка | Результат | SHA-256 |
|---|---|---|
| `ricon-audio-api-compile-r26-20261021-014028` | FAIL: AudioEditor.Cast не поддержан, exit0 не принят | `cdf78bde674bd8b2ba8318aaef8399d8bc6894c1ee146ffe1469323dfbd930ab` |
| `ricon-audio-api-compile-r26-20261021-014104` | промежуточный PASS с WBModuleDef до смены CLI-флага | `61ace4fbf320dd79f13911a9a4e02b797213335262882fd7b7cf1c4b4a705dc7` |
| `ricon-audio-api-compile-r26-20261021-014224` | конечный compile PASS, successful,0 SCRIPT(E),CRC096631db | `6bf6b79608cc8210a3e9a8b40c53f85f469fdcfcdd272429d94dda33d066ac4d` |

| Новый файл | SHA-256 |
|---|---|
| `audit_audio_api.py` | `d9599246a8fc1886e5714778a7c03263c9205bf7a66dc43b0003d41f9d3c455c` |
| `test_audit_audio_api.py` | `050968e99bb9e22b7dce24be4586fe087ed599b1a82ddf243ac1296e0c3db2f6` |
| `RICON_AudioContainerProbePlugin.c` | `c77379a706410b1c7b083bb4a3097ce18403857666d1e769f7e8dbc3ad2d2da3` |

После взаимного ревью — один ограниченный скрытый прогон. Команда из E-physics-enforce.md:
проект `tools/RICON_WbReader/addon.gproj`, модуль AudioEditor, plugin RICON_AudioContainerProbePlugin,
`-riconAcpInspect=yes`, уникальный logsDir; без -validate. Таймаут5 минут, завершение только своего PID.
Проверить смысл полного лога, CRC, SCRIPT(E), затем git status; exit0 не заменяет смысл результата.

Для ответа о звуке остаётся `WB-order-link-quality.md`: radio/direct ветви, входы и кривые TransmissionQuality,
источник native quality; затем опыт в игре с отдельным разрешением владельца.
