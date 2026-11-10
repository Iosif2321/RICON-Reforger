# R1 — DRAFT: ограниченный exact-path reader и нейтральная fixture

Версия 0.1, 2026-11-02. Автор: Специалист по VoN; accountable и оператор будущего опыта: Lead. Статус **DRAFT / REVIEW_PENDING / NOT_RECORDED / CODE_BLOCKED / PROCESS_BLOCKED / SOURCE_BLOCKED**. Назначение: проверить возможность безопасного bounded доступа к заранее известной script-точке для I2, не осуществляя сейчас чтение исходников игры. Текущий write-scope — только этот файл; код, fixture, gproj, существующий reader и metadata не меняются.

## 1. Основание и gate

Git — источник редакций; входной HEAD `a04a14f125fe757ad0038d762e2b44ab4819085b`; дерево на старте содержит чужие незакоммиченные документы и metadata, они не являются записанными revision и не меняются. Источник задачи — task:b7877eb8-a9d0-4df0-b73e-47c715d2ae51; Q3 task:da893f87-1bc7-463a-8a03-2dfd06f6d1b1 принят как SOURCE_CANDIDATE_WITH_BLOCKERS. Утверждённый [intent v0.2](<../req/INTENT-local-radio-slice.md>) ссылается на исходную v0.1 `249507a`; новая редакция не считается recorded только по наличию файла. [REQ-local-radio-actuation v0.2](<../req/REQ-local-radio-actuation.md>) — DRAFT, не утверждённый spec данного reader. Утверждённой специализированной reader-spec нет: **BLOCKED_SPEC_RECORD** до отдельного решения о нормативной основе; этот документ нельзя использовать как неявную утверждённую spec.

Применены `ai-native-sdlc-cycle`/artifact-contracts и `docs-manage`, версии не фиксировались. Исторический fresh-session waiver относится к прежней закрытой задаче и не переносится: **FRESH_SESSION_REQUIRED**, пока новый прямой waiver не записан. Review, запись неизменной ревизии, fresh-session и отдельный writing GO — разные условия. Они откроют максимум согласованные code-файлы, но не compile, процесс, source extraction, игру, ввод или аудио. Любой policy/signature refusal — STOP без Bypass, Zone.Identifier changes, другого хоста или замены wrapper как обхода.

Контекст: [старый plugin](<../../tools/RICON_WbReader/Scripts/WorkbenchGame/RICON_WbReaderPlugin.c#L57-L85>) читает весь source в массив до 15-строчного output; [старый wrapper](<../../tools/RICON_WbReader/run.ps1#L20-L39>) читает весь log и timeout завершает по PID. Эти артефакты не редактировать и не запускать для R1. Предыдущие Q3-команды с `-ExecutionPolicy Bypass` отозваны.

## 2. Proposed manifest: не создавать до отдельного writing GO

Ровно один исполнитель владеет новым каталогом `tools/RICON_BoundedReader/`; независимый reviewer — read-only. Ни один файл ниже пока не существует и название/точный GUID нового project должны быть заморожены до code gate. Чужие файлы и `addons/**` не входят.

| Предлагаемый файл | Обязанность |
|---|---|
| `tools/RICON_BoundedReader/addon.gproj` | Отдельный TOOL_ONLY project, явная зависимость от установленной игры, новый уникальный GUID; не изменять существующий gproj. |
| `tools/RICON_BoundedReader/Scripts/WorkbenchGame/RICON_BoundedReaderPlugin.c` | Default-off: только зафиксированный режим fixture либо source, exact allowlist, byte reader и parser; без поиска ресурсов/произвольного пути. |
| `tools/RICON_BoundedReader/Scripts/WorkbenchGame/RICON_BoundedReaderPureCheck.c` | Детерминированные pure vectors на переданных заранее ограниченных int-значениях; ни `FileIO`, ни Workbench execution как следствие code gate. |
| `tools/RICON_BoundedReader/ops/r1-neutral-fixture.bin` | Предлагаемая собственная неконфиденциальная fixture фиксированных байтов §4, создать только после code GO; никаких vanilla данных. |
| `tools/RICON_BoundedReader/ops/r1-runner.ps1` | Только если отдельно доказан create-time held-handle и ограничение engine-log без запрещённого interop; иначе отсутствует, **BLOCKED_EXECUTOR**, не обход старого runner. |

Никакой dynamic path/filename из пользователя, shell payload, `-Command`, `-EncodedCommand`, вложенного script, server, world, Play, сетевых вводов/выводов, setters или подмены callbacks. При неизвестном флаге/режиме — до любых side effects `DISABLED`/`INVALID_CONFIG`. Переменные, CLI strings, пути и логи не служат независимым media data plane.

## 3. API и условный parser contract

- [FileIO.OpenFile(string name, FileMode mode)](<C:/Program Files (x86)/Steam/steamapps/common/Arma Reforger Tools/Workbench/docs/EnfusionScriptAPI/html/interfaceFileIO.html#L109-L113>) возвращает `ref FileHandle`; только фиксированный путь и READ. Прямое разрешение packed `$ArmaReforger:` path без `SearchResources` **не подтверждено**: проверять exact identity/mount в отдельном source gate, не fallback на recursive search.
- [FileHandle.GetLength()/Read/Close](<C:/Program Files (x86)/Steam/steamapps/common/Arma Reforger Tools/Workbench/docs/EnfusionScriptAPI/html/interfaceFileHandle.html#L230-L246>) и [Read(out void data, int length)](<C:/Program Files (x86)/Steam/steamapps/common/Arma Reforger Tools/Workbench/docs/EnfusionScriptAPI/html/interfaceFileHandle.html#L306-L333>): длина в bytes, число действительно прочитанных bytes; `int` length ограничен 0…4. Кандидат — заранее инициализированный scalar `int` и `Read(value,1)`; позиция байта, sign/representation и native temporary allocation **UNKNOWN**. Float/string Read и [ReadLine](<C:/Program Files (x86)/Steam/steamapps/common/Arma Reforger Tools/Workbench/docs/EnfusionScriptAPI/html/interfaceFileHandle.html#L374-L390>) не применять. `ReadArray` не принимается без отдельного доказательства allocation contract.
- Путь и anchor задаются **скомпилированной allowlist**, не свободным CLI/path traversal или текстом поиска. До открытия проверить режим и точную строку resource ID/build; один путь, один handle, строго READ; no `ResourceDatabase.SearchResources`. При отказе/несовпадении — `INCOMPLETE_OPEN_OR_IDENTITY`, не alternate path.
- До каждого выделения и чтения применять byte/line/output caps §5. `GetLength` должен быть валиден и ≤ cap, но не является доказательством постоянства ресурса; фактический счётчик отдельно ограничен, reread/retry запрещены. Каждый запрос — ровно 1 byte; short/zero до ожидаемого EOF, отрицательный/аномальный ответ или изменение length при контрольном чтении → `INCOMPLETE_IO_OR_CHANGE`. Если snapshot consistency нельзя доказать — результат `INCOMPLETE_IDENTITY`, даже при одинаковом length: равный размер не исключает изменения содержимого.
- Парсер принимает только доказанную fixture-проходом ASCII-подмножество: printable 0x20–0x7e, TAB 0x09, LF 0x0a, CRLF как одна граница; NUL, non-ASCII, bare CR, некорректная граница и неподтверждённое представление байта → `INCOMPLETE_ENCODING`. Никакой normalization/замены символов. Последняя строка без LF допустима только при достоверном EOF и полном чтении. Номер строки считается с 1; anchor — только предварительно заданный numeric start, не grep/пользовательское выражение. Не хранить весь файл: максимум одна ограниченная строка и ограниченный output; неполный/лишний output, отсутствующий anchor/EOF, переполнение и незавершённый close → `INCOMPLETE`, не частичный PASS.
- Одно source окно ≤15 строк фиксируется до запуска. Вывести только разрешённый paraphrase/ограниченное source-окно в собственный log вне репозитория согласно проектным ограничениям; vanilla не переносить в Git. Source receipt включает build, exact path/anchor, bytes/count/limits, captured log path/hash, exit и terminal; не декларировать native writer completeness или радио PASS.

## 4. Нейтральная собственная fixture и pure checks

Fixture не содержит входа игры, создаётся в новом project только после writing GO. Exact bytes в hex: `41 0A 42 0D 0A 43` (6 bytes, строки `A`, `B`, `C`, LF/CRLF/EOF без LF). В отдельном расширенном наборе собственных случаев задаются отдельные фиксированные bytes, а не произвольный shell input: empty; NUL `41 00 42`; bare CR `41 0D 42`; non-ASCII `C3 A9`; 4096 и 4097 printable bytes в одной строке; специально смоделированный short read/change/EOF и превышение caps. Пределы генератора должны быть известны до создания файла, до записи — CREATE_NEW в выделенной fresh области; конфиденциальные данные и vanilla path запрещены. Ожидаемые bytes и длины зафиксировать в tests до запуска; runtime fixture не должна модифицировать game files.

**Pure:** передавать test vectors и fake read counts непосредственно parser-функциям, без `FileIO`, processes и clock. Проверить все варианты выше, точные line/output границы, CRLF на границе чтений, повторный EOF, ошибку открытия, overflow, missing anchor, неверный terminal; negative никогда не даёт PASS. Pure model не доказывает engine byte semantics/allocations.

**Native-neutral:** только после отдельного compile и process gates один Workbench ScriptEditor читает только собственные фиксированные fixture bytes через exact mode, не играет и не открывает `$ArmaReforger:`. Сравнить каждый returned byte count/значение, `GetLength`, EOF и результат parser с заранее frozen ожидаемым значением; отдельные nonzero/short/error cases из approved fixture. Неподдержанная репрезентация, неожиданное состояние, превышение квоты или отсутствие controlled exit = INVALID/INCOMPLETE. Этот конечный тест не доказывает верхнюю границу внутренних native allocations, thread safety, snapshot или mount identity — они остаются `BLOCKED_NATIVE_ALLOCATION` и `BLOCKED_MOUNT_IDENTITY` до иных доказательств.

## 5. Процесс, evidence и ограничения

| Ресурс / действие | Предлагаемый жёсткий cap и остановка |
|---|---|
| Процессы | Один собственный Workbench ScriptEditor, одна попытка, no Play/world; дочерние процессы **0**. Невозможность доказать отсутствие потомков → `INCONCLUSIVE_CLEANUP`, не массовое завершение. |
| Время | Внешний абсолютный deadline 60 s с момента до создания процесса; при превышении containment только своего удержанного процесса, затем до 10 s ожидание фактического exit. Это проектные limits, не гарантия Windows. |
| Чтение | input и cumulative reads ≤262144 bytes; path ≤512 chars; одна строка ≤4096 bytes; window ≤15 строк; output ≤16384 bytes. Проверять до следующего чтения/добавления; первый over-limit не обрезать и не считать доказательством. |
| Внешний log read | Фиксированный buffer ≤4096 bytes, cumulative ≤1048576 bytes; `FileStream.Read(byte[],int,int)` с checked count и short-read handling, никакого whole-log `Get-Content`, `ReadToEnd` или unbounded line reader. Выход за лимит → INVALID и containment. |
| Engine-native log | Требуется доказанный cap записи **до process gate**, включая собственные engine logs и stderr/stdout, не только cap их чтения. При отсутствии безопасного механизма квоты/ownership → `BLOCKED_ENGINE_LOG_BOUND`; наблюдение размера постфактум и удаление не эквивалентны cap. Stdout не redirect либо bounded drain при отдельно доказанной отсутствии deadlock; окружение не гарантирует pipes. |
| Evidence | Fresh собственная область, no overwrite/delete; bounded terminal receipt с mode/build/project hashes, run ID, expected/actual counts, open/close/exit, лимитами и точной причиной. SHA-256 и полный raw только допустимого лог-файла после завершения, если все caps/identity доказаны. |

Не использовать PID/name lookup, `Stop-Process -Id`, process-tree kill, поиск уже работающего Workbench или общий kill. Требуется **create-time** ownership/handle с правами wait/terminate от собственного запуска и удержание без повторного открытия по PID; если API позволяет лишь получить handle после старта с гонкой — `BLOCKED_EXECUTOR`. Win32 [TerminateProcess(HANDLE, UINT)](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-terminateprocess) документирован и асинхронен, но доступность допустимого interop здесь не установлена; `Process.Handle` после Start сам по себе не доказывает atomic creation ([Microsoft](https://learn.microsoft.com/en-us/dotnet/api/system.diagnostics.process.handle?view=netframework-4.8.1)). Не создавать DLL/PInvoke/Add-Type или иной обход ради решения. Ошибка запуска после возможного создания процесса без удержанного handle → `INCONCLUSIVE_CLEANUP`, остановить всю серию и передать Lead, не повторять. Timeout termination обязана дожидаться подтверждённого exit; собственный handle освобождается только после receipt/exit. Policy/signature refusal — STOP без каких-либо изменений политики.

## 6. Точные предлагаемые срезы и acceptance

Все команды ниже **NOT_RUN / NON-AUTHORIZING**. Фиксированный путь runner из §2 не существует, его `-EnableR1`/`-Mode` являются предлагаемым интерфейсом, не существующим API. Их нельзя выполнять до записи, свежей сессии/зафиксированного waiver, code review и отдельных соответствующих gates. Отсутствие безопасного executor/log contract = BLOCKED, а не команда для обхода.

| Срез | Предлагаемый запуск / условия | Acceptance и следующий gate |
|---|---|---|
| G0 design | Нет команд; независимый reviewer проверяет данный документ и конкретный manifest | DESIGN_REVIEW_PASS только при закрытии замечаний; не code GO. |
| G1 code | Нет автоматического запуска; отдельное writing GO и только файлы §2 | readback/diff, SHA и code review, default-off proof; без compile. |
| G2 pure | `C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe -NoProfile -NonInteractive -File .\tools\RICON_BoundedReader\ops\r1-runner.ps1 -EnableR1 -Mode Pure` | Только после точного reviewed implementation: fixed vectors и отрицательные проверки; если такой host не исполняет Enforce pure model без Workbench, **BLOCKED_PURE_HOST**, никакой fake PASS или самовольной альтернативы. |
| G3 compile | Точная команда и log/ownership определяются отдельным reviewed compile plan после code freeze; сейчас **BLOCKED_UNSPECIFIED_COMPILE** | Compile PASS не открывает fixture/source. |
| G4 native-neutral | `C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe -NoProfile -NonInteractive -File .\tools\RICON_BoundedReader\ops\r1-runner.ps1 -EnableR1 -Mode Neutral` | Только собственная fixture и один Workbench; byte comparison, strict negatives, bounded logs, handle и cleanup. Нет exact executable/build/command до решения executor/log blockers — **BLOCKED_PROCESS_COMMAND**, не запуск. |
| G5 source | Не задавать shell command до approved exact path/anchor, mount identity, native allocation, log/handle и отдельного source gate | Только собственный bounded receipt; ни один успешный source-результат не открывает I2/радиоактуатор. |

G2 фиксирует целевой CLI, а не утверждает существование чистого Enforce host в PS5.1. До отдельного решения он не исполним; reviewer должен выбрать проверяемый pure-host или отдельно ограничить scope только engine fixture. G3/G4 намеренно не подменяются догадочными командами: отсутствие конкретного безопасного механизма — блокер для отдельного разрешения, а не оставленное оператору усмотрение. **До каждого фактического процесса** отдельный план обязан записать exact executable/arguments и зависимости, подтверждённую policy, полные byte/time limits, fresh artifact hashes и независимый review.

## 7. Source targets и границы выводов

I2 нуждается в origin server VoN-use, dispatch connection/register/spawn/entity и connection identity/reconnect, не в повторении общего API обзора. Ранее прочитан `$ArmaReforger:scripts/GameCode/Network/SCR_VoNComponent.c:78–92` ([raw evidence](<C:/Users/Ded/Documents/My Games/ArmaReforgerWorkbench/logs/ricon-wbreader-ea4-onreceive-78-20261023-203106/console.log#L152-L166>)); там уже виден script override, но native caller/accepted radio START не доказаны. Повтор окна не добавляет origin. Exact path и source anchors остальных тел не установлены; никаких вымышленных путей/индексов или поиска всего корпуса. До отдельной source qualification — `UNKNOWN`. Если native implementation не доступна публично, bounded reader не сможет доказать native caller/authority или отсутствие других writers; нужен иной документированный contract, не binary RE или mute experiment.

## 8. Риски, альтернативы и handoff

- **Что может сломаться:** ошибочное чтение/декодирование может дать ложное source evidence; неограниченный лог/процесс или неверная cleanup identity — повлиять на чужой процесс или диск; PID reuse делает cleanup опасным. Любой сбой — STOP/INCOMPLETE, без повторов или удаления чужого.
- **Наиболее рискованно:** первый native процесс — одновременно неоднозначны ownership при создании, внутренние allocation и engine-log bound. Отдельный neutral fixture не превращает их в доказанные универсальные пределы.
- **Отклонено:** прежний WbReader с полнофайловым массивом/PID cleanup, `ReadLine`/string или recursive index, process-policy bypass, PID/name recovery, неограниченные logs, предположение о доступности native caller. Новый reader не способ обхода отказа policy.
- Rollout отсутствует. До code gate нет изменений для rollback; после будущего gate tool default-off, deployment/Workshop не входит. Cleanup только собственных объектов с проверенными handle и receipt; артефакты evidence не удалять.

| Критерий | Статус |
|---|---|
| Public API signatures и ограничения изучены | SOURCE_CANDIDATE, не runtime PASS |
| Source/spec/record/fresh | **BLOCKED_SPEC_RECORD / FRESH_SESSION_REQUIRED** |
| Parser, pure host, fixture, code | NOT_IMPLEMENTED / NOT_RUN |
| Byte representation и native allocation | BLOCKED_BYTE_DECODING / BLOCKED_NATIVE_ALLOCATION |
| Create-time process handle, children и engine logs | BLOCKED_EXECUTOR / BLOCKED_ENGINE_LOG_BOUND |
| Mount/path/identity и exact I2 caller bodies | BLOCKED_MOUNT_IDENTITY / UNKNOWN |
| Source, compile, runtime, actuator | NOT_RUN / BLOCKED |

**Следующая сессия/исполнитель:** прочитать текущие AGENTS и этот план в записанной ревизии, Q3 task:da893f87-1bc7-463a-8a03-2dfd06f6d1b1, зафиксированные intent/spec и замечания независимого reviewer. Lead определяет нормативную основу и назначает отдельный review; затем после записи/fresh-session и отдельного writing GO — только утверждённый manifest. Reviewer отдельно проверяет реализуемость pre-allocation, тестовые границы, process/log containment и exact команды. Не запускать старый или новый reader, fixture, Workbench или setters на основании данного DRAFT.
