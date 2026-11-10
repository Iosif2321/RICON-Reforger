# Дополнение: creation caller и config → catalog loader

2026-11-04 · `codex.access.writer` · **CONFIG_LOADER_CONTRACT_PROVED / ITEM_CREATION_ADOPTION_OPEN / RUNTIME_NOT_RUN**.

Основной [PROOF_RU.md](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-von-access-20261104/writer/PROOF_RU.md>) сохранён без изменения, SHA256 `6405135bf7f5aefdfe08e6ada15a432291ca5841d6d61942fc9817e45103e6d9`. Это отдельный узкий шаг по поручению root: найти существующий creation caller каталога L=SndComponent self+50, затем прочитать только один его115-byte loader. Другие callees не раскрывались; canonical/old exports и приложения не менялись.

**Новый результат:** каталог из public concrete signal setter действительно передаётся config-loader из constructor. Но прочитанный loader — wrapper, который делегирует создание/принятие каждого resource element в одну конкретную функцию. Различие «новый component-owned AudioSourceTemplate D» и «принятый existing T» этим wrapper ещё не установлено. Нельзя объявить private VoN T отсутствующим в collection.

## Что дал reuse frozen creation caller

В уже сохранённом [engine_sound_constructor.dumpbin.asm](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/von-re-20261104/world/environment_sources/engine_sound_constructor.dumpbin.asm>) для `[14120A480,14120A6A8)` обнаружен точный вызов:

- НаA4AA–A4AE адрес self+50 передаётся1412071B0 для инициализации subobject; constructor затем устанавливает его interface vtable наA4DF–A4E6.
- НаA5BE–A5F3 из component configuration выбирается array/span, count ограничивается0x20. Название configuration field здесь не переименовывается в ACP/privateT без descriptor proof.
- **Fallback branch при self+C8==0:** A66A–A685 собирает span `{pointer,count}`, делает `RDX=&span`, `R8D=0`, `RCX=self+50`, затем вызывает **141207ED0**. Это ровноL, который получает concrete setter141209970.
- **Альтернативная branch при self+C8!=0:** A63C–A663 передаёт тот же configuration span, self, owner/world context в **141206550**. Ранее тип C8 был независимо квалифицирован как SndSystem. Эта ветвь не исследуется здесь и не считается эквивалентной direct fallback loader по отсутствию своего body.

Итак, положительно установлен loader caller дляL, а не предположение по похожему offset. Никакого нового whole-constructor dump не было: использован frozen ASM. Дополнительно повторно сверены уже существующие VoN_ComponentConstructor и VonStream_ConstructorCandidate: resource загрузка и private T allocation сами по себе не содержат доказанного identity join сL этого SndComponent. Их наличие также не исключает позднее включение T.

## Единственный новый loader: полный `[141207ED0,141207F43)`

Root разрешил115bytes, одна exact `.pdata` строка, последнийRET7F42. [ASM](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-von-access-20261104/writer/catalog-config-loader.dumpbin.asm>) и [receipt](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-von-access-20261104/writer/catalog-config-loader.receipt.json>) подтверждают все115bytes byte-for-byte и sourceStatUnchanged.

1. RCX сохраняется какL; RDX указывает на span. При span.count=[RDX+8]==0 функция возвращается до дальнейших вызовов.
2. Для каждого8-byte элемента span вызывает **141207F50**, с `RCX=L`, `RDX=&element`, `R8D=0`. Callsite **141207F09**, bytes `E8 42 00 00 00`.
3. После обхода вызывает141209EC0 с `RCX=&L+28`, затем141208910(L). Они прямо связаны с resource collection и её последующей обработкой, но их конкретная сортировка/indexing семантика этим wrapper не устанавливается.
4. Затем вызывает virtual slot+20 **самогоL** сRCX=L и возвращаетAL=1. Это другой receiver, чем optional interception object из writer: writer сначала загружает **H=[L+50]**, затем вызывает **H.vslot+20(H,L,index,value)**. Два virtualslot+20 нельзя смешивать в один доказанный hook.

Нет прямой записи resource pointerD вL+28 внутри этого loader body. Это не отсутствие insertion вообще: item-loader141207F50 и postprocessing functions получают работу дальше. Нулевой R8D на вызове не означает, что existing T никогда не может быть принят; семантика этого аргумента не установлена.

## Один точный decision-changing вопрос после wrapper

**Кандидат следующего самостоятельного bounded шага:** item-loader `[141207F50,14120826B)`,795bytes, одна exact frozen `.pdata` строка. Он назван исключительно по callsite141207F09; тело не прочитано в этом дополнении, дополнительного экспорта/самовольного продолжения не было.

Вопрос к нему: откуда получается destination D, добавляемый в resource collectionL+28 — создаётся ли новый template из config element, используется ли shared/existing pointer и какой supported owner/attachment interface для этого существует? Если code только делегирует дальше, это также должно остаться точным scoped результатом. Альтернативная SndSystem branch141206550 и installerH=[L+50] остаются самостоятельными unknowns, не доказанной невозможностью доступа.

Текущий полезный contract уже есть: **public concrete Snd setter → owner catalog mapping → D → A1B690**, и **component constructor → config span → loader(L)**. Missing join теперь локализован в catalog membership/creation/adoption. Один и тот же resource filename или общий numeric helper не доказывают identity D==T; разные offsets и private creation не доказывают D!=T.

Следующее решение/test выбирает root. Runtime/SDK/engine не запускались. Дефолтный temporal channel contract не меняется: no automatic silence из LOST/zero intelligibility; Direct, другие senders, shared receiver behavior и user state сохраняются как требования. Никакого adapter stub или утверждения о полном завершении Goal нет.
