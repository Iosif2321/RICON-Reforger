# Public signal writer: конкретный destination и граница доступа к VoN

2026-11-04 · `codex.access.writer` · **CONCRETE_COMPONENT_WRITER_PROVED / VON_MEMBERSHIP_OPEN / RUNTIME_NOT_RUN**. Запись только в `writer/`; canonical, старые отчёты/exports и чужие изменения сохранены. Root разрешил точный lookup двух имён, две исходные native функции, затем два concrete Snd wrappers и один непосредственный writer callee. Ghidra, SDK, Workbench, игра, Play/server/capture/transmissions не запускались; binary/DLL/pagefile/apps не менялись.

**Результат:** поддерживаемый `SndComponent.SetSignalValue[Str]` действительно доходит до того же numeric writer `A1B690`, который использует private native VoN. Но destination берётся из каталога ресурсов конкретного SndComponent. Положительное тождество этого destination приватному `VonStream+20 = T` не установлено. Поэтому это доказанный component-owned writer, а не квалифицированный per-sender native VoN actuator.

## Регистрация: два разных класса нельзя смешивать

Exact null-terminated строки методов: `SetSignalValue` VA14186DE50 и `SetSignalValueStr` VA1418B2D48. [Lookup](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-von-access-20261104/writer/lookup_two_methods.py>) ограничен двумя именами и RIP/data references именно к этим targets; broader audio/API scan не выполнялся. Сырые совпадения до проверки instruction context отмечены candidates в [receipt](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-von-access-20261104/writer/two-method-registration-candidates.json>).

Первоначальный кандидат14120BF00 — **SndComponent**, не BaseSoundComponent. Это исправлено проверкой method context: GetOwner, SoundEventBegin/End и прочие его methods. **BaseSoundComponent** соответствует registrar141362BF0..141362E45: порядок и все19 имён совпадают с полным установленным generated declaration, включая GetEvents, SetDebug, SetMute, GetGain. MusicManager/SndBaseModule candidates исключены по их собственным именам; значения одинаковых method strings не определяют класс сами. [Method context](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-von-access-20261104/writer/registration-method-contexts.json>), [BaseSound registration ASM](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-von-access-20261104/writer/basesound-registration.dumpbin.asm>) и [Snd registration ASM](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-von-access-20261104/writer/registration-candidate.dumpbin.asm>).

У BaseSound registration обе строки получают один R9 callback **14027AC60**: LEA R9 на1362C78/C96, LEA R8 methodname на1362C84/CA2, calls8A41A0 на1362C91/CAF. Этот же callback используется некоторыми другими BaseSound methods; одинаковый callback не доказывает общий audio target.

## BaseSound callbacks: отдельный no-write результат

`[14027AC60,14027AC83)`,35bytes, одна exact.pdata строка: извлекает argument0 helper88D000, получает self context и tail-jumps88CCB0. Float value/signal index не читаются; ACP writer здесь не вызывается. [ASM](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-von-access-20261104/writer/public-writer-wrapper.dumpbin.asm>).

`88CCB0` оказался object-unwrapping leaf, **не VM dispatcher/interpreter**. Полный прочитанный диапазон `[14088CCB0,14088CD3E)`142bytes имеет три подтверждённых RET, последнийCD3D; далееCC padding. Он проверяет argument tag/type flags и возвращает native pointer вRAX (прямой либо adjusted pointer). Calls и writes в этой функции отсутствуют; сигнал/float не применяется. [Полный ASM](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-von-access-20261104/writer/native-object-unwrap-full.dumpbin.asm>). Первые64 bytes первоначального чтения обрезали instruction; prefix/tail artifacts не используются как полное доказательство вместо этой final ASM.

Таким образом, **эти callbacks BaseSound registration не выполняют signal write**. Это не утверждение, что API вообще не работает или concrete implementations отсутствуют: отдельный SndComponent registrar связывает setters с другими wrappers. Наследование/конкретная runtime method resolution вне этих callbacks не подменяется выводом из одинакового thunk.

## Concrete SndComponent: pointer owner и actual write

Snd registration14120BF00..14120C173 связывает `SetSignalValue` с **14120D7A0**, `SetSignalValueStr` с **14120D7E0**. Обозначим native self после object unwrap как **S**, а принадлежащий ему catalog subobject **L=S+50**. Это адрес, реально передаваемый wrappers; семантический тип L и всех его вспомогательных pointers здесь не выдумывается.

| Wrapper / полный range | Установленное действие |
|---|---|
| `14120D7A0..14120D7DE`,62bytes | Получает S, читает script index и float, делает `RCX=S+50`, `EDX=index`, `XMM2=value`, tail-jumps141209970 |
| `14120D7E0..14120D83E`,94bytes | Получает тот жеL; name→index вызывает1412076F0(L,name); при index!=-1 вызывает141209970(L,index,value), иначе возвращается без записи |

В этих интерфейсах нет AudioHandle, sender/player ID, VoNComponent или privateT аргумента. Это ограничение **аргументов вызова**, не доказательство, что T никогда не может находиться в каталоге L. Name lookup1412076F0 отдельно не раскрывался. [Numeric wrapper](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-von-access-20261104/writer/snd-set-signal-value.dumpbin.asm>), [name wrapper](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-von-access-20261104/writer/snd-set-signal-value-str.dumpbin.asm>).

**Непосредственный callee141209970** прочитан полностью: `[141209970,141209A0B)`,155bytes,3 contiguous.pdata chunks 9970:99B7,99B7:99FB,99FB:9A0B; последнийRET9A0A. Он делает следующее:

1. Unsigned index проверяется против count `[L+14]`. Вне диапазона — выход.
2. Если pointer `[L+50]` ненулевой, вызывает его virtual slot+20 с `RCX=hook`, `RDX=L`, `R8D=index`, `XMM3=value`. ЕслиAL!=0, дальнейшая запись не выполняется. Это частный interception interface с неизвестным здесь installer/типом, **не доказанный public script extension point**.
3. Иначе выбирает16-byte catalog record `entry=[L+8]+index*10`. Dwords entry+8/+C задают start/count в packed array `[L+18]` с4-byte stride.
4. Каждый packed record содержит signed16 parameter index и signed8 resource index по+2. Из resource array `[L+28]` с24-byte stride выбирается **D=QWORD[resourceArray + resourceIndex*18]**.
5. На1412099E8 вызывает **A1B690(D,parameterIndex,value)**. Ни transformation float, ни поиск sender manager в этом callee не выполняются. Value передаётся unchanged XMM2; downstream setter behavior повторно не экспортировался.

[Полная writer ASM](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-von-access-20261104/writer/snd-owned-signal-write.dumpbin.asm>) устанавливает concrete destination/fanout, а не только наличие public метода.

## Сравнение с приватным VoN T

Уже frozen native receive path CFAC0 передаёт **T=QWORD[VonStream+20]** непосредственно в A1B690 вместе с cached TransmissionQuality index и полученным float. Current typed private instance — AudioSourceTemplate; A1B690→E9DFB0→9FAB40 обновляет его compiled scalar state. Прежний getter/Input-kernel reverse не повторялся.

Новый concrete writer использует тот же A1B690, но выбирает **D из resource catalog L конкретного SndComponent**. Поэтому недостающий identity join теперь точен:

```
SndComponent native self S
  → catalog L=S+50
  → indexed packed mapping
  → resource entry D
  ? D == T данного sender у данного listener/generation
```

Ни совпадение helperA1B690, ни одинаковое имя Input не устанавливают `D==T`. Одновременно разные owner offsets и отсутствие sender argument **не доказывают D!=T**: позднее включение privateT в collection или отдельный native interception/attachment остаются возможностью, которую эти functions не проверяли. Catalog population/attachment и hook installer не исследуются дальше в этой bounded задаче.

Публичный BaseSoundComponent declaration наследует GenericComponent, VoNComponent — GameComponent; concrete SndComponent declaration также имеет собственную component surface. Эти declarations помогают различать API receivers, но сами не являются доказательством запрета внешнего native attachment. В прочитанных public signatures нет метода передачи arbitrary existing AudioSourceTemplate/VoNStream вSnd catalog. Private pointerD/T или VA native helper не предлагается вызывать из script.

## Следующая capability и честный оставшийся пробел

**Готовая поддерживаемая способность:** существующий SndComponent setter меняет свои mapped audio-resource inputs и имеет реальный fanout доA1B690. Для native VoN не хватает поддерживаемого способа выбрать/получить ownerS, каталог которого содержит нужныйT, либо официального attachment contract. Это более конкретный interface gap, чем «не найден AudioHandle».

Минимальный будущий test, только после отдельного разрешённого runtime этапа: в собственном VoN ACP duplicate добавить уникальный Input probe, связанный **только с radio gain**, со значениями0.5/1 и default1; native events/Stream сохранить. На B для sourceA proxy получить script-visible sound component, проверить GetSignalNames/GetSignalIndex и менять probe через его реальный setter при непрерывном native A→B voice. Наличие индекса/успешный вызов не PASS: фактический radio output A→B должен следоватьprobe, а C→B на том жеRX, A→C и A Direct→B сохраняться; после respawn старый owner/index не действует. Это квалификация фактического catalog membership/forwarding, не отдельный WAV/PlayEvent и не server delivery gate. Никакой automatic silence из LOST/zero intelligibility не добавляется.

Если подходящего owner не даёт поддерживаемая surface или native output не меняется, остаётся **NO_QUALIFIED_VON_CATALOG_ACCESS**, а не общий вывод о невозможности любого будущего bridge. Глобальный volume/OutputState и подмена Interior/RoomSize не закрывают этот source-specific contract. Product adapter stub не создаётся, полный Goal не объявляется завершённым.

## Проверка

Текущий установленный EXE SHA256 снова проверен: `1d221a990fa16c78fe02f685a7ba1330be115ca93cd3b641e67e5d3f1b40d9c9`. Все final bounded ASM сверяются byte-for-byte с соответствующими raw spans; exact.pdata фиксируется где есть, unwrap leaf имеет явные RET/padding boundaries. [Initial receipt](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-von-access-20261104/writer/initial-read.receipt.json>), [concrete wrappers/helper receipt](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-von-access-20261104/writer/concrete-writer.receipt.json>) и [owned writer receipt](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-von-access-20261104/writer/snd-owned-signal-write.receipt.json>) сохраняют commands, raw bytes/hashes, output hashes и bounds. Это source/static proof, не engine execution, SDK compile или слышимость.
