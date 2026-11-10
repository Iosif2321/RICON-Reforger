# Item-loader: создание destination D из component configuration

2026-11-04 · `codex.access.writer` · **FRESH_TEMPLATE_CREATION_BRANCH_PROVED / PRIVATE_VON_ADOPTION_NOT_QUALIFIED / RUNTIME_NOT_RUN**.

Основной PROOF и115-byte loader supplement остаются frozen. Root отдельно разрешил только `[141207F50,14120826B)`,795 raw bytes, чтобы различить создание новогоD и принятие существующего instance. [ASM](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-von-access-20261104/writer/catalog-item-loader.dumpbin.asm>) и [receipt](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-von-access-20261104/writer/catalog-item-loader.receipt.json>) фиксируют exact.pdata, raw/output hashes и byte equality. SourceStatUnchanged; Ghidra/engine/SDK/runtime не запускались. Последний байт826A — CC padding; последний code instruction8265 — JMP к собственному cleanup8032; RET общего выхода807D находится внутри этого же span. Все прямые conditional/unconditional jumps остаются внутри диапазона.

**Новый decision-changing факт:** в ветке добавления нового configuration element создаётся **новый AudioSourceTemplate D**, а не передаётся existing native VoN instanceT. При этом не делается общий вывод, что T никогда не мог быть включён другим путём: early existing-item branch, альтернативный SndSystem loader и поздние attachments сохраняют свои границы.

## Как получается D

Входы используются как `RCX=L`, `RDX=&configElement`, low byteR8 — mode/flag, сохраняемый вR15B. Смысл mode не выводится из caller zero. ExistingT pointer не является прочитанным аргументом этой функции.

1. **Ранние ветви:** нулевой qword config element приводит кfalse. Если `[L+34]>31`, выполняется diagnostic иfalse. Helper141207770 получаетL иэлемент; при результате!=-1 функция возвращаетtrue **до allocation**. Его body не раскрывался, поэтому эта ветвь обозначается existing-item/lookup success, а конкретные правила совпадения identity не предполагаются.
2. **Resource identity, не instance identity:** 141208007..14120801E вызывает уже известный loader8EDB30 через engine audio-system pointer, передавая адрес configuration element и адрес output resource handle. Результат — resource descriptor, передаваемый далее конструктору. Предыдущий static type proof квалифицировал этот загрузчик AudioProjectRes; совпадение такого descriptor/filename с VoN resource не означает равенства AudioSourceTemplate instances.
3. **Свежая allocation:** на14120807E задаётся размер0x98, на141208083 вызывается allocator. При успешной allocation на**1412080BB вызываетсяA17E00** сRCX=newMemory, RDX=resource descriptor. Возвращённый pointer сохраняется какD. Это тот же квалифицированный constructor AudioSourceTemplate, который используется private VonStream, но этот вызов получает новый allocation result, не старыйT.
4. **Dependencies:** R8B получает сохранённый input mode, R9=0; stack arguments5/6/7 передаются изL+38/L+40/L+48. По уже frozen constructor mapping это соответственно D+68,D+80,D+88. TypeD+68 — AudioEmitter, typeD+88 — ObstructionData; типD+80 остаётся не квалифицированным variable/provider object. Общие dependencies component resources не следует переименовывать в sender SignalsManager binding. НулевойR9 инициирует default SourceSet внутриconstructor; он не доказывает отсутствие последующих attachments.
5. **Init:** на1412080F8 вызываетсяA19930(D,0). Если он возвращаетfalse, вновь созданныйD освобождается/очищается и function завершаетfalse через общий cleanup. Сам init уже frozen, его builders повторно не экспортировались.

Таким образом, **ветка успешного нового элемента использует fresh D**. Одновременный liveT другого VonStream не становится D просто из-за общего AudioProjectRes/filename. Это stronger local result по allocation provenance, чем сравнение разных owner offsets, но не аудит всех возможных путей изменения collectionL.

## Collection handoff: где именно появляется запись с D

После успешного init функция читает providerD+80 и при необходимости обновляет shared provider slotL+40 сrefcounts. Это показанный обмен provider dependency, а не доказанная передача sender identity или privateT.

На1412081B9..141208208 вызываются три metadata gettersD; получаются два float иbyte. Float значения при положительности возводятся в квадрат, иначе используются defaults. Их семантические названия здесь не требуются и не выдумываются.

Затем собирается stack record:

| Поле record | Значение / инструкция |
|---|---|
|+0|PointerD, запись141208208|
|+8,+C|Два computed float, записи141208225/22B|
|+10|Byte metadata, запись141208231|

На141208236 RDX получает&record, на14120823B RCX получает**&L+28**, на**14120823F CALL141209F60** передаёт их collection helper. Это конкретный insertion handoff того же24-byte resource-entry layout, из которого public writer выбираетD. Сам helper141209F60 не раскрывался: его внутренний memory append/refcount/ошибки не выдаются за прочитанный ASM. Непосредственно перед handoff первыйpointer record — именно новыйD; existingT не извлекается и не подставляется в прочитанном теле.

## Обе важные границы остаются явными

- **Existing-item lookup success:** до новой allocation helper141207770 может вернуть!=-1 и завершить функциюtrue. Здесь не доказано, чем создан ранее найденный entry. Это не положительный adoption proof, но и не повод объявлять все entries созданными только этой allocation branch.
- **Альтернативный SndSystem construction path:** frozen constructor14120A663 вызывает141206550 вместо direct config loader141207ED0, когдаself+C8nonnull. Эта ветвь не раскрывалась в этом шаге. Нельзя распространять вывод оfreshD этой branch на любую SndSystem-managed collection.
- **Later attachments/interceptor:** текущий probe не нашёл и не исследовал installer private hookH=[L+50], другие collection mutators или API adoption существующегоT. Их отсутствие не утверждается. Different offsets, constructor zeros, Input metadata и отсутствие sender argument не заменяют положительную identity проверку.

## Текущий usable-interface вывод

Public concrete Snd setter способен менять **component-mapped template instances D** через тот же numericA1B690. Прочитанный default configuration creation path даёт ему **собственные вновь созданныеD**. Это не переиспользует автоматически decoded Stream/private runtime state existing native VoN T: создание ACP с тем же Stream name/resource не является PCM/instance bridge.

Следовательно, остаётся точный gap: поддерживаемый **existingT adoption/catalog membership** либо supported **native VoN instance writer/attachment**. Создание собственногоD и запись вего Input уже не следует считать доказательством эффекта на native incoming A voice. Для положительной квалификации нужен native output того жеT при source/listener/generation isolation; обычный WAV/новыйPlayEvent/успешныйsetter недостаточны.

На этом item-loader reading завершается. Collection helper141209F60, lookup141207770, sort/notification и альтернативныйSndSystem body не запрашиваются. Root выбирает следующий конкретный test/доказательство. Product adapter не добавляется; temporal channel contract и no automatic silence из LOST/zero intelligibility сохраняются.
