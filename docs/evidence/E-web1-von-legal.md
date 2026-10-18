# E-web1: VoN, ретрансляторы, звуковой граф, стриминг, лицензии и BattlEye (веб-сбор)

Роль: claude.web1. Дата сбора: 2026-10-18. Режим: только чтение веб-источников.

Уровни: **DOC** = официальный источник Bohemia (BI wiki, Script API, официальные условия, официальные changelog); **MOD** = страница мода, репозиторий, автор мода; **FORUM** = форум, Reddit, Steam-гайд, хостинговый блог; **ПРЕДП** = вывод автора этого файла.

**Важная оговорка о методе.** Страницы `community.bistudio.com/wiki/*` (Server_Config, Server_Hosting, Audio:_Voice_over_Network, Audio:_Signals, Scripting:_Performance) отдают **HTTP 403** для WebFetch. Цитаты с этих страниц получены **только через поисковые сниппеты** (WebSearch). Их суть совпадает в нескольких независимых запросах, но дословность не проверена — пометка «DOC (сниппет)». Страницы Script API (`community.bistudio.com/wikidata/external-data/...`) загружались напрямую — пометка «DOC (загружено)». Код модов не копировался.

---

## Q1. RelayTransceiver: ретрансляторы в ванильной игре

### Ответ (кратко)
- `RelayTransceiver` — штатный класс движка в группе Radio Script API. Собственных методов в API нет: всё наследуется от `BaseTransceiver` (частота в кГц, min/max частота, `GetRange`/`SetRange` в метрах, mute, `BeginTransmission`). Отдельных атрибутов «частота входа / частота выхода» в публичном API **нет**.
- В Conflict `RelayTransceiver` добавляют в список Transceivers компонента `SCR_CoverageRadioComponent`. Атрибут — Transmitting Range (по стороннему туториалу по умолчанию 1000, ставят ~2000). `SCR_CoverageRadioComponent` вычисляет «покрытие» сетью базы: радио, помеченное как Source, с тем же ключом шифрования. Это механика захвата и спавна, а не явная ретрансляция голоса.
- **Ретранслирует ли движок голос VoN через relay:** официального подтверждения **не найдено**. Есть косвенные свидетельства MOD-уровня, что в 1.8 доставка радио зависела от relay. По словам автора мода ATG Translator, «передача приходит только через relay в зоне обоих». Тот же автор пишет, что BI признали это багом. Сводка багов 1.8 на трекере BI описывает «Radio Relay Logic Bug». В официальном changelog 1.8.0.13 есть только строка «Radio would not work sometimes». Значит, в 1.8.x на доставку VoN-радио влияла сетевая/relay-логика движка, и это поведение нестабильно между версиями (ПРЕДП).

### Доказательства
1. https://community.bistudio.com/wikidata/external-data/arma-reforger/ArmaReforgerScriptAPIPublic/group__Radio.html — классы группы Radio: «BaseRadioMessage, BaseTransceiver, EditorFactionTransceiver, EditorTransceiver, IntercomTransceiver, RadioTransceiver, RelayTransceiver, ScriptedRadioMessage, VonRadioMessage»; описания нет. — DOC (загружено, Doxygen 1.13.2, версия игры не указана).
2. https://community.bistudio.com/wikidata/external-data/arma-reforger/ArmaReforgerScriptAPIPublic/interfaceRelayTransceiver.html — собственных членов нет, всё «Inherited from BaseTransceiver»; исходник `Game/generated/Radio/RelayTransceiver.c`. — DOC (загружено).
3. https://community.bistudio.com/wikidata/external-data/arma-reforger/ArmaReforgerScriptAPIPublic/interfaceBaseTransceiver.html — `GetRange()`: «Gets maximum transmitting range of radio» (метры); `SetRange`: «Has effect only on owner (server)», значения ≤1 м трактуются как 1 м; `SetFrequency(int freq)`: частота в кГц; `BeginTransmissionFreq`: «Makes transmission of message on given frequency (kHz), ignoring the tuned one». Описания Min/Max frequency в API перепутаны местами. — DOC (загружено).
4. https://community.bistudio.com/wikidata/external-data/arma-reforger/ArmaReforgerScriptAPIPublic/interfaceRadioManagerEntity.html — единственный метод `GetTransceiversInRange(vector position, float range, out array<BaseTransceiver>)`: «Gets all transceivers in given range that are in power ON state». — DOC (загружено).
5. https://community.bistudio.com/wikidata/external-data/arma-reforger/ArmaReforgerScriptAPIPublic/interfaceSCR__CoverageRadioComponent.html — наследник `ScriptedRadioComponent` → `BaseRadioComponent`. Определяет, покрыт ли владелец сигналом «the component marked as Source with the same encryption key». Методы: `GetRadiosInRangeCount`, `GetRadiosInRangeOfCount`, `ConnectedRadiosContain`, а также недокументированные `Ping(encryptionKey, signalSignature, reverse)`, `PrepareCoverageUpdate`. — DOC (загружено).
6. https://community.bistudio.com/wikidata/external-data/arma-reforger/ArmaReforgerScriptAPIPublic/interfaceBaseRadioComponent.html — `SetEncryptionKey`/`GetEncryptionKey`, `SetPower`, `TransceiversCount`/`GetTransceiver(idx)`, события `OnTansceiverRangeChanged` (срабатывает на клиенте и сервере). — DOC (загружено).
7. https://www.tsanvr.com/2026/08/blog-post.html (2026-08-17, на японском) — в `SCR_CoverageRadioComponent` → Transceivers «add one RelayTransceiver»; Transmitting Range ≈2000, по умолчанию 1000, описано как «дистанция, на которую ретранслируется радио» (перевод). — FORUM (блог-туториал).
8. https://reforger.armaplatform.com/workshop/69D30616C31092F7 (ATG Translator, Smudge202, игра 1.8.0.13, изм. 23.09.2026) — «Arma Reforger 1.8 changed radio delivery so that a transmission only arrives through a relay in range of both people»; «Bohemia have confirmed that behaviour is a bug»; «The 1.8.0.13 radio fix does NOT restore this device». — MOD (подтверждение от BI не найдено).
9. https://report.bistudio.com/issues/ARGF-374 (игра 1.8.0.10, создано 17.08.2026, статус Duplicate) — «Squad members in close physical proximity but outside relay range experience complete failure of short-range radios». — FORUM (пользовательский отчёт на официальном трекере; официальной реакции по существу нет).
10. https://reforger.armaplatform.com/workshop/69C566706ABD5A3C-RecondosRadioRework/changelog — 1.0.7 (15.08.2026): «Fixed bug introduced in 1.8 that mutated the Radio's Receiver Registration afer it's VON entry gets added. Temporary solution until BI fixes this bug.»; 1.0.11 (03.09.2026): «Removal of temporary fixes needed after the 1.8 update». — MOD.
11. https://reforger.armaplatform.com/news/update-september-3-2026 (1.8.0.13) — «Fixed: Radio would not work sometimes». — DOC (загружено).
12. https://reforger.armaplatform.com/news/changelog-august-13-2026 (1.8.0.10) — «Fixed: Arland, Everon, and Kolguyev statically placed antennas should not have enabled radio components or service»; «Fixed: Factions which are not meant to capture bases, like FIA, were still able to capture radio relays». — DOC (загружено).
13. https://steamcommunity.com/sharedfiles/filedetails/?id=2814452333 (Steam-гайд «RADIO 101», обновлён к 1.8, сент. 2026) — relay towers are «real transceivers, the engine class RelayTransceiver sits right next to the one your handheld uses»; влияние на голос «still unconfirmed». — FORUM.
14. https://github.com/ViVi141/Ground-based-radar-station (Apache-2.0, аддон 1.1.8) — README: ретрансляция через включённый RelayTransceiver (GM-вышки, антенны, командные машины), «最多 24» (до 24 хопов), «手台本身不跳转» (носимая рация сама не ретранслирует). Значения помечены «需局内核对» (нужна проверка в игре). Не ясно, движок это или логика мода. — MOD.
15. https://reforger.armaplatform.com/workshop/656A504DB72C4723-MilsimRadioNetwork (игра 1.8.0.13, изм. 17.10.2026, APL) — «SatCom antenna has a 10,000 meter range, and is set to relay on 240-400 MHz»; механизм не описан. — MOD.

### Пробелы/противоречия
- Нет официальной статьи BI о механике relay для VoN. Неизвестно, есть ли у RelayTransceiver отдельные частоты входа и выхода: в публичном API у него только одна частота от BaseTransceiver.
- Число хопов («до 24») подтверждено только README мода.
- Поведение 1.8.0.10 и 1.8.0.13 различается. Текущий статус в 1.9 (экспериментальная ветка 15.09.2026) не проверен: SteamDB отдаёт 403. По сниппету там добавлены «base radio range circles», про VoN-relay — ничего.
- Запросы: «"RelayTransceiver" Reforger»; «Arma Reforger RelayTransceiver radio relay VoN»; «forums.bohemia.net Reforger RelayTransceiver»; «reddit arma reforger radio relay extend voice range»; «Arma Reforger patch notes VON radio relay».

---

## Q2. Звуковой граф VoN: VON.acp / Von.sig, сигналы, моды

### Ответ (кратко)
- На приёмнике `VoNComponent` декодирует поток, выставляет сигналы и проигрывает его через ACP, указанный в `VoNComponent` получателя. Sound Events зашиты в движок: `VON_DIRECT`, `VON_RADIO`, `VON_RAW`. Источником в графе должен быть узел Stream с ID «Stream».
- Сигналы на приёмнике (по BI wiki): `TransmissionQuality` (0..1), `Distance`, `Interior`, `RoomSize`, `UnderPlayerControl`, `IsInPlayerVehicle`, а также глобальные G-сигналы из GameSignalsManager.
- Как моды меняют звук радио: подменяют `Sounds/VON/Von.sig` (FTA Chat Volume — громкость по каналам), `VON.acp` и `reverb_base.sig` (ASAPsVON — дальность прямой речи, реверберация, громкость радио) или работают поверх нативного VoN на клиенте приёмника (sh_battle radio — шум, статика, деградация). Все такие моды конфликтуют между собой при переопределении одного и того же файла, важен порядок загрузки.

### Доказательства
1. https://community.bistudio.com/wiki/Arma_Reforger:Audio:_Voice_over_Network (по сниппету ~600 дней без правок) — «The audiostream has a hardcoded ID (This simply being "Stream")»; приёмник «decodes … sets signals and plays it back through the ACP specified on the receiver's VoNComponent»; сигналы: «UnderPlayerControl: Whether or not the sender is controlled by the local player (0,1)», «TransmissionQuality … Float, 0..1», а также Distance, Interior, RoomSize, IsInPlayerVehicle. — DOC (сниппет).
2. Там же — условия приёма радио: нужен «RadioManagerEntity» в мире, у отправителя и получателя должен быть предмет с `BaseRadioComponent`; получатель слышит через VON_RADIO, если «within radio range, on the same frequency and using the same encryption key». — DOC (сниппет).
3. https://community.bistudio.com/wiki/Arma_Reforger:Audio:_Sound_Events — VON_DIRECT, VON_RADIO; SOUND_RADIO_TURN_ON/OFF, SOUND_RADIO. — DOC (сниппет).
4. https://community.bistudio.com/wiki/Arma_Reforger:Audio:_Signals и https://community.bistudio.com/wiki/Arma_Reforger:Audio:_Occlusion — Interior: «1 if emitter inside the building (requires SoundWorld on map & buildings with properly set up BSP.)»; RoomSize: «Returns volume in m^3 of room the entity is currently in». — DOC (сниппет).
5. https://community.bistudio.com/wiki/Arma_Reforger:Diag_Menu — в Diag Menu есть опции VON: замена VON тоном 440 Гц, bypass loudness, отключение фильтров, лог starving, воспроизведение записанного VON. — DOC (сниппет).
6. https://community.bistudio.com/wikidata/external-data/arma-reforger/ArmaReforgerScriptAPIPublic/interfaceVoNComponent.html — `OnReceive(int playerId, bool isSenderEditor, BaseTransceiver receiver, int frequency, float quality)`; quality — «<0,1> describing transmission quality»; frequency помечена как «very questionable» и может измениться; также `SetTransmitRadio`, `SetCommMethod`, `SetCapture`, `OnCapture`, серверное `OnVoNUsed`. — DOC (загружено).
7. https://reforger.armaplatform.com/workshop/69E059166C863941 (FTA Chat Volume, Jhon181, игра 1.7.0.54, изм. 20.07.2026, APL) — «Overrides a single base audio file (Sounds/VON/Von.sig)»; ползунки «Radio Channel 1 (Squad)» и «Radio Channel 2 (Platoon)» 0–100 %. — MOD.
8. https://reforger.armaplatform.com/workshop/66EB536657903998 (ASAPsVON 1.0.2, ASAPgaming, игра 1.6.0.95, изм. 13.01.2026, APL) — «Increases VON amplitude outer range value from ~60m to 120m»; изменяемые файлы: VON.acp, reverb_base.sig, frequency_character_voice.conf; «This mod would need to be loaded after any other mod that overrides VON.acp»; громкость радио тоже растёт. — MOD.
9. https://reforger.armaplatform.com/workshop/6A4539AB94F37633 (sh_battle radio 1.0.0, K1REL, игра 1.8.0.10, изм. 02.09.2026, APL) — «Uses Arma Reforger's native radio and VoN systems»; эффект «Distance and native signal-quality interference»; не играет звук на выделенном сервере; обработка на клиенте приёмника. — MOD.
10. https://reforger.armaplatform.com/workshop/672B0395726428B6 (506IRRU Enhanced Radio, Fedora12, игра 1.8.0.10, APL-SA) — «LIKELY NOT COMPATIBLE WITH ANY MODS THAT MODIFY VON COMPONENTS»; экспериментальное распространение «disabled by default». — MOD.
11. https://reforger.armaplatform.com/workshop/6A8A1DB719D54E82 (EasyGames Radio Core, игра 1.8.0.13) — панорамирование и громкость по рации на клиенте; совместим с любыми рациями на `BaseRadioComponent`. — MOD (через сниппет).

### Пробелы/противоречия
- Точное содержимое ванильных VON.acp и Von.sig (узлы, кривые) в вебе не опубликовано: смотреть в Workbench (это уже не веб-источник).
- Страница VoN wiki старая (~600 дней), список сигналов мог измениться. Кроме того, моды пишут «~60m» для прямой речи, а хостинг-блоги — «~50 m».
- Запросы: «Arma Reforger VON.acp Von sound signal radio quality modding»; «"Audio: Voice over Network" … TransmissionQuality»; «"FTA" Chat Volume Arma Reforger».

---

## Q5. Как движок вычисляет quality и дальность радио VoN

### Ответ (кратко)
- Официально известно только следующее. Дальность — это атрибут передатчика `BaseTransceiver` (`SetRange`, метры, авторитетно на сервере). Приём возможен «within radio range» при совпадении частоты и ключа. Quality — float 0..1, передаётся в `OnReceive` и в сигнал `TransmissionQuality`. **Формула quality и влияние рельефа официально НЕ описаны.**
- Ванильные дальности по официальной статье: носимые AN/PRC-68 и R-148 — 1,3 км, ранцевые AN/PRC-77, R-107M, RF-10 — 2 км.
- Наблюдения игроков: шум растёт с расстоянием. Есть единичное наблюдение о резкой смене качества на склоне холма (возможен LOS или рельеф), но оно не подтверждено. Автор Realism Overhaul пишет, что VoN почти не модифицируется и планируемое «terrain interception» недоступно. Значит, рельеф в ванили, вероятно, не учитывается или не доступен моддингу (ПРЕДП, низкая уверенность).

### Доказательства
1. https://reforger.armaplatform.com/news/boot-camp-communications (20.09.2024) — AN/PRC-68 и R-148: «Maximum transmission range of 1.3 kilometers»; AN/PRC-77, R-107M, RF-10: «Maximum transmission range of two kilometers». — DOC (загружено).
2. Script API BaseTransceiver (см. Q1 п.3) — «Gets maximum transmitting range of radio» (метры). — DOC (загружено).
3. Script API VoNComponent (см. Q2 п.6) — quality «<0,1>». — DOC (загружено).
4. https://community.bistudio.com/wiki/Arma_Reforger:Audio:_Voice_over_Network — «TransmissionQuality … Float, 0..1»; формула не приводится. — DOC (сниппет).
5. https://steamcommunity.com/sharedfiles/filedetails/?id=2814452333 — «Range is measured from you to the person you're talking to»; «the further you get from your intended receiver the more static and noise»; на склоне: «a flux of great comm quality to terrible static comm»; LOS «seems to be the most plausible besides radios just being distance-based» — но это не подтверждено. — FORUM.
6. https://reforger.armaplatform.com/workshop/632137C83DA94DD5 (Realism Overhaul - Radios, Maisteri) — «Unfortunately not much can be changed about the radios or the VON system»; Reforger «very limited in modding this area». — MOD.

### Пробелы/противоречия
- Нет ни официальной формулы quality(distance/range), ни измерений мододелов с таблицами. Опубликованных замеров quality по дистанции **НЕ НАЙДЕНО**.
- Неизвестно, считается ли quality по отношению distance/range передатчика, по range приёмника или по их минимуму.
- Запросы: «Arma Reforger VoN radio quality terrain line of sight distance how calculated»; «feedback.bistudio.com Reforger radio VON transmission quality terrain».

---

## Q9. Стриминг/relevance: слышно ли радио от игроков вне зоны репликации

### Ответ (кратко)
- Официально: VoN маршрутизирует **сервер** — он определяет, каким клиентам отдать поток. Стриминг сущностей (relevance) решает серверный scheduler, в основном по дистанции. Радиус задаётся `networkViewDistance` (500..5000, по умолчанию 1500). **Официального утверждения о связи VoN-радио со стримингом НЕ НАЙДЕНО.**
- Косвенно (MOD): sh_battle radio пишет, что если «the sender is outside the replication area, vanilla radio continues». То есть ванильное радио доходит от отправителя вне зоны репликации, но сущность отправителя у приёмника не существует (ПРЕДП: данные об отправителе — позиция, машина, сигналы Distance и IsInPlayerVehicle — на приёмнике могут быть недоступны).
- Рекомендация RICON: не полагаться на наличие сущности отправителя на клиенте и проверить в игре дистанцию выше `networkViewDistance` (ПРЕДП).

### Доказательства
1. https://community.bistudio.com/wiki/Arma_Reforger:Audio:_Voice_over_Network — сервер определяет, какие клиенты должны слышать аудиопоток. — DOC (сниппет).
2. https://community.bistudio.com/wikidata/external-data/arma-reforger/EnfusionScriptAPIPublic/Page_Replication_Overview.html — «Scheduler determines whether a node hierarchy is relevant for a client»; «Most of the relevancy and prioritization is based on distance of an object in the game world from the player»; «closer objects are updated more often, while distant objects are updated less often or streamed out completely». VoN на странице не упоминается. — DOC (загружено).
3. https://community.bistudio.com/wiki/Arma_Reforger:Server_Config — `networkViewDistance`: «maximum network streaming range of replicated entities», 500..5000, по умолчанию 1500. — DOC (сниппет).
4. https://community.bistudio.com/wiki/Arma_Reforger:Multiplayer_Scripting — broadcast, отправленный до стриминга предмета клиенту, до этого клиента не дойдёт. — DOC (сниппет).
5. https://community.bistudio.com/wiki/Arma_Reforger:Startup_Parameters — `-nds` (Network Dynamic Simulation) «only streams in relevant replicated entities for each client»; больший диаметр означает больший сетевой обзор и меньшую производительность сервера. — DOC (сниппет).
6. https://reforger.armaplatform.com/workshop/6A4539AB94F37633 (sh_battle radio) — «If detection fails or the sender is outside the replication area, vanilla radio continues». — MOD.

### Пробелы/противоречия
- Нет официального ответа и измерений. Конфликт с моделью ванили: при дальности ранцевой рации 2 км и `networkViewDistance` 1500 по умолчанию радио должно работать вне стриминга, если оно вообще работает на 2 км. Это согласуется с MOD-свидетельством (ПРЕДП).
- Запросы: «Arma Reforger VoN radio sender outside network streaming distance replication»; «Arma Reforger replication streaming networkViewDistance wiki».

---

## Q24. Arma Reforger Workshop Terms и GPLv3

### Ответ (кратко)
- Workshop Terms требуют лицензировать загружаемый контент для других пользователей **только** под APL, APL-SA, APL-ND, ADPL-SA или под кастомной лицензией. Кастомная лицензия **обязана** явно запрещать коммерческое использование и монетизацию (в ряде формулировок — и использование вне продуктов BI).
- GPLv3 разрешает коммерческое использование и запрещает дополнительные ограничения (§7, §10). Поэтому **GPLv3 не удовлетворяет требованию к кастомной лицензии**, и загрузить мод в Workshop «под GPLv3» как лицензию Workshop нельзя (ПРЕДП, юридическая интерпретация).
- Передачи прав (assignment) в Workshop Terms нет. FAQ BI: «content creators do not relinquish ownership rights». Однако EULA игры даёт BI «irrevocable permission to use, copy, modify and adapt» — по FAQ это касается контента, созданного в самой игре. Tools EULA ограничивает инструменты и созданный ими контент некоммерческим использованием.
- О требованиях к исходникам, GPL и open source в Workshop Terms **ничего нет**.
- Прецедент двойной схемы: репозиторий BohemiaInteractive/CWR — код под GPL-3.0-or-later, игровые данные отдельно под APL-SA. Для RICON возможен вариант: исходники в git под GPLv3 (как отдельное распространение автором), публикация в Workshop под APL-SA или APL. Совместимость такого дуального лицензирования для производного от игровых скриптов BI кода требует юридической проверки, потому что моды Reforger наследуют и модифицируют скрипты BI (ПРЕДП).

### Доказательства
1. https://reforger.armaplatform.com/workshop-terms (без даты и версии, © 2026) — контент должен быть лицензирован «under one of the following license terms: APL license, APL-SA license, APL-ND license, ADPL-SA license» (или кастомной). — DOC (загружено).
2. Там же — «The custom license shall explicitly prohibit the use of the Content for any commercial purposes»; «shall explicitly prohibit any direct or indirect monetization of the Content»; по первому пересказу страницы — также запрет «the use of the Content outside Our own products». — DOC (загружено; третий пункт виден в одном из двух извлечений).
3. Там же — пользователи могут «Download, use, alter and further develop the Content uploaded to ARMA REFORGER Workshop» (если исходная лицензия позволяет); санкция за «operating any Content on monetized servers without necessary license or permission». В тексте нет слов GPL, open source или source code; явной лицензии от автора к BI нет. — DOC (загружено).
4. https://reforger.armaplatform.com/news/eula-faq (19.03.2024) — «content creators do not relinquish ownership rights»; «The Workshop Terms of Use do not negate the rules contained within the Arma Reforger Tools EULA»; «you may NOT create any mods for commercial purposes using our game, Tools, or Workshop». — DOC (загружено).
5. https://www.bohemia.net/community/licenses/arma-public-license-share-alike (обновлено 21/08/2026) — «for NonCommercial and ArmaOnly purposes only»; share-alike: «The Adapter's License You apply must be this Public License, or an Arma Public Share Alike Compatible License». — DOC (загружено).
6. https://github.com/BohemiaInteractive/CWR — «The engine and game executables, licensed under GPL-3.0-or-later with additional terms under Section 7»; данные «released separately by Bohemia Interactive under the Arma Public License Share Alike (APL-SA)». Это прецедент BI, но для другого продукта. — DOC (загружено).

### Пробелы/противоречия
- Тексты ADPL-SA и APL-ND отдельно не загружались.
- Нет официальной позиции BI про GPL-моды в Workshop. Нет ответа, обязан ли мод Reforger, наследующий скрипты BI (`modded class`), считаться производным от контента BI.
- Запросы: «Bohemia Interactive Arma Public License Share Alike APL-SA text»; «"Arma Reforger Tools" EULA license user content».

---

## Q25. BattlEye и моды Reforger: ограничения

### Ответ (кратко)
- BattlEye включается полем `battlEye` (bool, по умолчанию true с 0.9.8.73) в `gameProperties`. Конфиг BattlEye `BEServer_x64.cfg` лежит в папке BattlEye. Можно только **добавлять** строки `RConPort`/`RConPassword`; изменённые значения ломают BattlEye.
- **Официального перечня того, что BattlEye блокирует в скриптовых модах, НЕ НАЙДЕНО.** Ограничения доступа скриптов задаёт сам движок, а не BattlEye. `FileIO`: удаление и копирование разрешены только в `$profile:`, `$logs:`, `$saves:`. Операции RunCmd, RunProcess, KillProcess и FileIO вне профиля вызывают security popup; параметр `-scriptAuthorizeAll` отключает его в debug-режиме. `RestApi` есть в Enfusion API (группа Online, `GetContext(string serverURL)`), но ограничения и политика в документации не описаны.
- Для RICON (только VoN, без внешних программ) эти ограничения некритичны (ПРЕДП).

### Доказательства
1. https://community.bistudio.com/wiki/Arma_Reforger:Server_Config — `battlEye`: true включает, false отключает; по умолчанию true (с 0.9.8.73). — DOC (сниппет).
2. https://community.bistudio.com/wiki/Arma_Reforger:Server_Hosting — RCon BattlEye настраивается добавлением RConPort/RConPassword в `BEServer_x64.cfg`; «modified values will prevent BattlEye from working»; при порче файл удаляют и проверяют целостность через Steam. — DOC (сниппет; по сниппету страница ~17 мес. без правок).
3. https://community.bistudio.com/wikidata/external-data/arma-reforger/EnfusionScriptAPIPublic/interfaceFileIO.html — DeleteFile: «Works only on "$profile:", "$logs:" and "$saves:" locations»; CopyFile — назначение только в этих префиксах; про BattlEye ничего нет. — DOC (загружено).
4. https://community.bistudio.com/wiki/Arma_Reforger:Startup_Parameters — `scriptAuthorizeAll` «disables the security popup about RunCmd, RunProcess, KillProcess and (outside of profile directory) FileIO operations» (debug-опция). — DOC (сниппет).
5. https://community.bistudio.com/wikidata/external-data/arma-reforger/EnfusionScriptAPIPublic/interfaceRestApi.html — `proto external RestContext GetContext(string serverURL)`, без описаний. — DOC (загружено).
6. https://xgamingserver.com/blog/arma-reforger-server-configuration-guide/ — с включённым BattlEye клиенты без BE не допускаются; консольные игроки не заходят на сервер без BE. — FORUM.
7. https://feedback.bistudio.com/T165102 — пользовательский анализ: RCON BattlEye в Reforger — модифицированный протокол с ограниченным набором команд. — FORUM.

### Пробелы/противоречия
- Нет официального документа «BattlEye vs моды Reforger». Не найдено, блокирует ли BE RestApi или FileIO на клиенте.
- Запросы: «Arma Reforger mod RestApi RestContext GetRestApi … BattlEye»; «Arma Reforger BattlEye workshop mods scripts kicked … filters»; «Arma Reforger FileIO "$profile"».

---

## Q26. EULA Arma Reforger: reverse engineering, модификации, моды

### Ответ (кратко)
- EULA игры (Steam app 1874880 и reforger.armaplatform.com/eula): запрет «hack, modify or reverse engineer the game or any game files, its data or network Services»; только некоммерческое использование; запрет читов. Пользовательский контент не должен нарушать чужие права. BI получает «irrevocable permission to use, copy, modify and adapt…».
- Ссылка из задания `store.steampowered.com/eula/1874910_eula_0` — это **Tools**-EULA (app 1874910 = Arma Reforger Tools; `eula_1` озаглавлен «ARMA REFORGER TOOLS EULA»). В ней: запрет «hack, modify or reverse engineer the software or any software files»; инструменты и созданный ими контент — «for non-commercial purposes only»; назначение — «producing non-commercial game content for computer games developed by Us».
- Слово «mods» в EULA не используется. Моды регулирует Workshop Terms (см. Q24). Даты и версии в EULA нет; BI оставляет право менять лицензию.
- Для RICON: использовать только публичный Script API и Workbench, никакого реверса бинарников или протокола VoN. Внешние программы исключены также и по проектному ограничению (ПРЕДП).

### Доказательства
1. https://store.steampowered.com/eula/1874880_eula_0 («ARMA REFORGER EULA») — «You are not allowed to hack, modify or reverse engineer the game or any game files, its data or network Services.»; «You give us an irrevocable permission to use, copy, modify and adapt anything You create»; «You may play and use the game for non-commercial purposes only.»; «You are also not allowed to cheat, create cheats or promote cheats in the game.» — DOC (загружено).
2. https://reforger.armaplatform.com/eula (© 2026, без даты) — те же положения, но формулировка разрешения: «You give us an irrevocable permission to use, copy, modify and adapt the game content». — DOC (загружено).
3. https://store.steampowered.com/eula/1874910_eula_0 — «You are not allowed to hack, modify or reverse engineer the software or any software files»; «You may use the software and the content you create using the software for non-commercial purposes only.»; «We reserve the right to make changes to the license from time to time». — DOC (загружено).
4. https://store.steampowered.com/eula/1874910_eula_1 («ARMA REFORGER TOOLS EULA») — то же плюс назначение: «designing, developing, testing, and producing non-commercial game content for computer games developed by Us». — DOC (загружено).
5. https://reforger.armaplatform.com/news/eula-faq (19.03.2024) — FAQ уточняет, что «irrevocable permission» относится к контенту, созданному в самой игре; Tools EULA «regulates the use of Tools and the content produced by these tools». — DOC (загружено).

### Пробелы/противоречия
- Формулировка «irrevocable permission» расходится: Steam — «anything You create», сайт — «the game content». FAQ трактует узко. Это юридический риск для GPL-публикации (ПРЕДП).
- Отдельно есть Server EULA (https://store.steampowered.com/eula/1874900_eula_1) — не загружалась.

---

## Q27. Производительность сервера Reforger

### Ответ (кратко)
- Официально: с 0.9.8 «heavily recommended» запускать сервер с `-maxFPS` в диапазоне 60..120, иначе сервер может занять все ресурсы (пример `-maxFPS 60`). Официального числа «тикрейта» отдельно от FPS сервера **не найдено**.
- `maxPlayers` (до 0.9.8.73 назывался `playerCountLimit`), по умолчанию 64. Диапазон в источниках **расходится**: 1..128 или 1..256 (см. ниже). Хостинг-гайды: типично 32–64, 128 — практический максимум.
- Сетевые параметры: `networkViewDistance` 500..5000 (по умолчанию 1500), `-nds`, `-streamingBudget`, `-streamsDelta`.
- Скрипты: BI wiki рекомендует разносить тяжёлую работу по кадрам, кэшировать, использовать таймеры через `CallLater` вместо per-frame, экономно использовать EOnFrame, делать дешёвые проверки до вызовов. Профилирование — Diag Menu: Script profiler (off/frame/average), external profiler, порог медленных скриптов 1..10 мс; `-scrNoInline` только на diag-exe.

### Доказательства
1. https://community.bistudio.com/wiki/Arma_Reforger:Server_Hosting — с 0.9.8 «heavily recommended» `-maxFPS` 60..120, иначе сервер «can consume all available resources»; пример `ArmaReforgerServer.exe -maxFPS 60`. — DOC (сниппет).
2. https://community.bistudio.com/wiki/Arma_Reforger:Startup_Parameters — `-maxFPS` задаёт лимит FPS, пример 30; `-scrNoInline` только для diag-exe. — DOC (сниппет; по сниппету страница правилась ~8 дней назад).
3. https://community.bistudio.com/wiki/Arma_Reforger:Server_Config — `maxPlayers`: в одном сниппете «range 1..128, default: 64», в другом «range 1..256, default: 64»; «was playerCountLimit before 0.9.8.73»; совет держать `fastValidation` true и ограничивать FPS. — DOC (сниппет; страница правилась ~26 дней назад — возможно, лимит недавно меняли).
4. https://community.bistudio.com/wiki/Arma_Reforger:Scripting:_Performance — разносить вычисления по кадрам, кэшировать результат, периодическая работа через CallLater. — DOC (сниппет).
5. https://community.bistudio.com/wiki/Arma_Reforger:Scripting:_Best_Practices и https://community.bistudio.com/wiki/Arma_Reforger:Scripting:_Keywords — дешёвые проверки, не вызывать Count() в цикле; в игре использовать `GetGame().GetCallQueue().CallLater()`, а не `thread`. — DOC (сниппет).
6. https://community.bistudio.com/wiki/Arma_Reforger:Diag_Menu — script profiler (off/frame/average), external profiler, slow script threshold 1..10 ms. — DOC (сниппет).
7. https://reforger.armaplatform.com/news/changelog-august-13-2026 — «Tweaked: Improved VoN Performance»; «Fixed: Possible crash in VoN system» (1.8.0.10); по сниппету SteamDB 1.9.0.5 exp — «Improved VoN direct-voice performance». — DOC (загружено) / FORUM (SteamDB-сниппет).
8. https://xgamingserver.com/blog/arma-reforger-server-requirements/ — до 128 игроков, «sweet spot» 32–48; сервер упирается в однопоточную производительность CPU; AI — главная нагрузка. — FORUM.

### Пробелы/противоречия
- Диапазон `maxPlayers` (128 или 256) — проверить на живой странице (403 для WebFetch).
- Нет официальных лимитов на время скриптов в кадре, на число RPC в секунду и на нагрузку VoN на сервер.

---

## Сводная таблица

| Q | Статус | Лучшее доказательство |
|---|---|---|
| Q1 RelayTransceiver / ретрансляция голоса | ЧАСТИЧНО (класс и атрибуты — да; ретрансляция голоса движком официально не подтверждена; в 1.8 есть MOD и FORUM данные о relay-зависимой доставке и баге) | DOC (Script API) для класса; MOD для поведения |
| Q2 Звуковой граф VoN / сигналы / моды | ЗАКРЫТ (кроме содержимого ванильных VON.acp и Von.sig) | DOC (сниппет BI wiki VoN) + DOC (Script API) + MOD |
| Q5 Формула quality и дальности | ЧАСТИЧНО (дальности и диапазон quality есть, формулы и рельефа нет) | DOC (Boot Camp, Script API); формула — НЕ НАЙДЕНО |
| Q9 Радио вне зоны репликации | ЧАСТИЧНО (сервер маршрутизирует VoN; стриминг по дистанции; мод утверждает, что радио работает вне репликации) | DOC (общая репликация) + MOD (sh_battle radio) |
| Q24 Workshop Terms и GPLv3 | ЗАКРЫТ (по фактам); вывод о несовместимости — ПРЕДП | DOC (Workshop Terms, APL-SA, EULA FAQ) |
| Q25 BattlEye и ограничения скриптов | ЧАСТИЧНО (конфиг BE и ограничения FileIO есть; список блокировок BE — НЕ НАЙДЕНО) | DOC (Server Config/Hosting сниппет, FileIO API) |
| Q26 EULA: reverse engineering, моды | ЗАКРЫТ | DOC (Steam EULA 1874880 / 1874910, сайт BI) |
| Q27 Производительность сервера | ЧАСТИЧНО (maxFPS, профайлер, практики есть; тикрейт и лимиты скриптов нет; maxPlayers противоречив) | DOC (сниппеты BI wiki) |
