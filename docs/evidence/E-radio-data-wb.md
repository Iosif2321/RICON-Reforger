# claude.radio_data — игровые данные радиосвязи Arma Reforger (префабы, конфиги, звук VoN)

Статус: DRAFT — отчёт субагента claude (owner agora #2960), ревью codex ожидается. Выборочная сверка claude с сырыми логами и HTML пройдена.

Дата: 2026-10-21. Build 1.8.0.13 (по #2988 codex.radio_api). Инструмент: `tools/RICON_WbReader/run.ps1`. Поиск — по подстроке, не regex.
Play, мир, сервер, распаковка .pak, память, сеть не использовались. Все находки — **WORKBENCH_STATIC**: это значения
в файлах ресурсов, а не поведение в игре. Значения по умолчанию классов (native) в префабах не видны.
Логи: `%USERPROFILE%\Documents\My Games\ArmaReforgerWorkbench\logs\<папка>\console.log`. Ниже `rdNN` — имя прогона.
Префикс путей `$ArmaReforger:` опущен. Дополняет `docs/evidence/E-workbench-source.md` (пункты 1 и 6), не заменяет его.

## 1. Префабы раций, ретрансляторов и интеркомов

**Носимые рации** — `Prefabs/Items/Equipment/Radios/`, 10 файлов (rd04, rd11 — все 10 открыты). Наследование (rd11, строка 1):
- `Prefabs/Items/Core/Radio_base.et` ← `Item_Base.et` (rd01 :1);
- от `Radio_base`: `Radio_ANPRC68`, `Radio_R148`, `Radio_R148_FIA`, `Radio_Deployable_Base`;
- от `Radio_Deployable_Base`: `Radio_ANPRC77`, `Radio_R107M`, `Radio_R107M_FIA`, `Radio_RF10/Radio_RF10_base`;
- `Radio_ANPRC77_COK` ← `Radio_ANPRC77`; `Radio_RF10/Radio_RF10` ← `Radio_RF10_base`.
- `Radio_Deployable_Base` использует модель и имя ANPRC-77 (`:28`, `:36`), то есть фактически это база ANPRC-77 (rd07).

**Ретрансляторы (`RelayTransceiver`)** — объявления в префабах:
- техника (rd23, все 1071 префаба `Prefabs/Vehicles` открыты): `Vehicles/Wheeled/Ural4320/Ural4320_command.et:13`, `Ural4320_repair.et:6`,
  `M923A1/M923A1_command.et:13`;
- сооружения и системы (rd25, `Prefabs/S*`, все 7336 кандидатов открыты, полный список):
  `Systems/MilitaryBase/{ConflictRelayRadio.et:14, ConflictMilitaryBase.et:31, ConflictBase_Base.et:30, ConflictControlPoint.et:6, ConflictSourceBase.et:10}`,
  `Structures/Infrastructure/Towers/TransmitterTower_01/TransmitterTower_01_base.et:94`, `_medium_base.et:82`, `_small_base.et:56`.
  `ConflictSourceBase.et` — новое относительно E-workbench-source.md.
- вне этих корней (из прежнего i1, не перепроверялось): `Prefabs/Props/Military/Compositions/Antenna_0{1,2}_base.et`, `Prefabs/MP/Campaign/…MobileAssembly*.et`.

**Интеркомы (`IntercomTransceiver`)** (rd23): `Vehicles/Wheeled/UAZ452/UAZ452_base.et:312`, `M151A2/M151A2.et:220`, `UAZ469/UAZ469_base.et:266`.
Других объявлений трансиверов в `Prefabs/Vehicles` нет. Наследники этих префабов получают трансиверы от родителя; в списке они не появляются.

## 2. Компоненты и атрибуты раций

Класс радио — `BaseRadioComponent`. Трансиверы — `RadioTransceiver`, `RelayTransceiver` или `IntercomTransceiver` в списке `Transceivers`.
В `Radio_base.et:6–7` блок `BaseRadioComponent` пуст: в базе нет переопределений, значит базовые значения — native-умолчания класса.
Их значения **UNKNOWN**. Рядом стоит `SignalsManagerComponent` (:4) (rd01).

Атрибуты, которые встречаются в префабах:
- у радио — `"Encryption key"` и `"Turned on"`;
- у трансивера — `ChannelFrequency`, `"Transmitting Range"`, `"Min tunable frequency"`, `"Max tunable frequency"`, `"Frequency resolution"`.

Атрибутов мощности передачи, чувствительности, усиления антенны и числа каналов в прочитанных окнах **нет**.
Число трансиверов в списке (`TransceiversCount`, индекс) — не число каналов или пресетов интерфейса: правило UI не
прочитано (codex #3064).

Пометки в таблице: «насл.» — значение унаследовано от родителя (совпадает GUID трансивера); «—» — в цепочке префабов нет, работает native-умолчание (UNKNOWN).

| Рация | Ключ | Трансивер | Частота канала | Диапазон настройки | Шаг | Дальность |
|---|---|---|---|---|---|---|
| ANPRC-68 | `chickenNuggets` :5 | T1 | 32000 :9 | 32000–68000 (i6) | 200 (i6) | 1300 :10 |
| ANPRC-68 | | T2 | 48000 :16 | 32000–68000 (i6) | 200 (i6) | 1300 :17 |
| R-148 | `coldBorscht` :5 | T1 | 38000 :9 | 38000–54000 :11–12 | 200 :13 | 1300 :10 |
| R-148 | | T2 (:15) | — | 38000–54000 :17–18 | 200 :19 | 1300 :16 |
| R-148 FIA | `candleSauce` :5 | T1 / T2 | 38000 :9 / не читалось | не читалось | не читалось | 1300 :10 / :16 |
| Deployable_Base | `chickenNuggets` :17 | T1 (единственный) | 48000 :20 | 30000–76000 :22–23 | — | 2000 :21 |
| ANPRC-77 | насл. `chickenNuggets` | T1 | насл. 48000 | насл. 30000–76000 | 200 :8 | насл. 2000 |
| ANPRC-77 | | T2 (:10) | — | 30000–76000 :12–13 | 200 :14 | 2000 :11 |
| ANPRC-77 COK | насл. | насл. | ключ, частота, дальность не переопределены (rd04/12/13) | | | |
| R-107M | `coldBorscht` :5 | T1 | насл. 48000 | насл. 30000–76000 | 200 :9 | насл. 2000 |
| R-107M | | T2 (:11) | — | 20000–52000 :13–14 | 200 :15 | 2000 :12 |
| R-107M FIA | `candleSauce` :5 | | не читалось | не читалось | не читалось | 2000 :11 |
| RF-10 base | `candleSauce` :10 | T1 | 42000 :14 | насл. 30000–76000 | 200 :15 | насл. 2000 |
| RF-10 base | | T2 (:17) | 39000 :18 | 20000–52000 :20–21 | 200 :22 | 2000 :19 |
| RF-10 | насл. | насл. | ключ, частота, дальность не переопределены | | | |

Строки по прогонам: ANPRC-68 — rd04 и rd13, остальное из i6; R-148 — rd02 и rd03; FIA — rd04, rd12, rd13; Deployable — rd06 и rd07; ANPRC-77 — rd08;
R-107M — rd05; RF-10 — rd09; ключи — rd12; частоты — rd13; дальности — rd04.

**Наблюдения (это вывод, а не измерение):**
- У R-107M и RF-10 T1 сам префаб не задаёт диапазон. По наследованию от `Deployable_Base` получается 30000–76000, у их T2 — 20000–52000.
  Это вывод из цепочки префабов. Итоговые значения в редакторе не проверены.
- Единицы в префабах не подписаны. По codex.radio_api (#2988) `SetFrequency(int)` принимает кГц, по API `SetRange` — метры (#2979).
  Отсюда частоты в кГц (30–76 МГц) и дальность в метрах — DOC + inference.
- У вышки, `ConflictRelayRadio` и `Ural4320_command` дальности 5000, 3000 и 2000, а у `ConflictRelayRadio` шаг 1 — это прежние данные i1.
  Префабы `ConflictSourceBase`, `ConflictControlPoint`, `ConflictBase_Base`, `_medium`/`_small` вышек, `Ural4320_repair` и `M923A1_command` построчно не читались — **UNKNOWN**.
- Интерком M151A2 (`M151A2.et:217–222`, rd24): `BaseRadioComponent`, `"Turned on" 1`, один `IntercomTransceiver` с шагом 1500 и без частоты и дальности.
  Интеркомы UAZ не читались.

**UNKNOWN:**
- native-умолчания `BaseRadioComponent`/`*Transceiver` (дальность, шаг, частота, ключ при отсутствии переопределения);
- мощность и чувствительность — в прочитанных окнах префабов их нет, есть только `Transmitting Range`; это не
  доказательство отсутствия во всех данных игры;
- что «Transmitting Range» значит в игре — граница, затухание или что-то иное;
- поведение ретрансляции (см. E-workbench-source.md, пункт 1).

## 3. VoN на персонаже

- `Prefabs/Characters/Core/Character_Base.et:379–381` (rd14): компонент `SCR_VoNComponent`, задан только `Filename` → `Sounds/VON/von.acp` (:380).
  Атрибутов дальности прямой речи, громкости или радио в префабе нет. В `Prefabs/Characters/Core` 27 префабов, `VoNComponent` есть только в этом.
- Контроллер игрока, Conflict: `Prefabs/Characters/Core/DefaultPlayerControllerMP_Campaign.et:46–51` (rd15) — `SCR_VONController` с `m_VONAutoTune`.
  В нём `m_bEnabled 0` (:48) и конфиг `Configs/Campaign/VONAutoTune/CampaignVONAutoTuneConfig.conf` (:49). Поиск остановился на первом совпадении (MaxFiles 1).
  Другие контроллеры, например не-Campaign, не читались.
- `Configs/Campaign/VONAutoTune/` — 6 файлов `.conf` (rd16). Прочитан один, `CampaignVONAutoTuneLongRangeSquadMember.conf:1–12`.
  Роль `SQUAD_MEMBER`; первый канал (без `m_iChannel`) → `COMMANDER`, `m_iChannel 1` → `OBJECTIVE_PLATOON`.
  Это перечисления категорий частот, а не числа. Остальные 5 файлов не читались.
- **UNKNOWN:** дальность прямой речи. В прочитанном окне `Character_Base.et:379–381` её нет; весь граф ресурсов
  персонажа не читался. Возможно, она в von.acp или в native-коде.

## 4. Звуковые ресурсы VoN

- `Sounds/VON/von.acp` найден (1 кандидат), но `FileIO.OpenFile(READ)` его не открывает: `cannot open`, INCOMPLETE, exit 4 (rd22).
  Это повторяет прежний i2. Поиск `Quality` по нему **недействителен**. `Von.sig` в этот раз не открывался (по i2 — тоже не открывается).
- `Sounds/VON`, расширение `conf`: `SearchResources=false`, exit 3 (rd20). Это **отказ инструмента**, а не «файлов нет».
  Возможно, так поиск отвечает на пустой результат, но это не установлено.
- По всем 196 `.conf` под `Sounds/` строки `TransmissionQuality` нет (rd21). Все кандидаты открыты, поэтому результат отрицательный и валидный.
  Поиск `quality`/`distance`/`frequency` в `.conf` не делался — не хватило бюджета.
- **UNKNOWN:** граф von.acp (куда подключён TransmissionQuality, кривые). Нужен Audio Editor (WB-order-link-quality.md). Отдельно этим занимается codex.native_radio.

## 5. Частоты и ключи фракций

`Configs/Factions/` — 17 файлов `.conf`, все открыты (rd18, rd19):

| Фракция | `m_iFactionRadioFrequency` | `"Radio encryption key"` | Совпадение с рацией |
|---|---|---|---|
| US (`US.conf`) | 48000 :16 | `chickenNuggets` :10 | ключ = ANPRC-68/77; частота = T2 ANPRC-68 и T1 ANPRC-77 |
| USSR (`USSR.conf`) | 42000 :16 | `coldBorscht` :10 | ключ = R-148/R-107M; частота ≠ частоте канала R-148 (38000), но в её диапазоне |
| FIA (`FIA.conf`) | 42000 :18 | `candleSauce` :10 | ключ и частота = RF-10 T1 |
| RNGD_Campaign | 52000 :70 | `prisonRations` :43 | раций с этим ключом в Radios нет |
| CIV (`CIV.conf`) | 58000 :17 | `Sandbox` :9 | раций с этим ключом нет |

- Пресет группы `Configs/GroupPresets/US/GroupPreset_US_Commander.conf:5–6` (rd17): `m_iRadioFrequency 0`, `m_iDefaultActiveRadioChannel 1`.
  Выдача частоты группе во время игры здесь не исследовалась — это зона claude.radio_gameplay. Другие пресеты не читались (лимит вывода).
- Совпадения выше — сравнение значений. Применяет ли игра частоту и ключ фракции к рации и когда — **UNKNOWN** по данным.

## Ограничения

- Всё это WORKBENCH_STATIC. Ни одна строка не доказывает поведение в игре: дальность, слышимость, ретрансляцию, применение ключа.
- Лимит 15 строк на прогон: часть файлов прочитана окнами, итоговые значения после наследования посчитаны вручную.
- В логах есть посторонние (E): `Steamworks RequestCurrentStats failed` и `Resource leaks` UI-текстур, они есть во всех прогонах.
  В rd19 ещё `NETWORK (E): WSAECONNREFUSED` — попытка локального соединения самого Workbench. Плагин её не вызывал, на чтение она не повлияла.
- Окна codex.radio_api в это время не пересекались с моими: его последнее окно завершилось до rd01 (#2989). Одновременно работало одно моё окно.

## Прогоны

Все логи — в `%USERPROFILE%\Documents\My Games\ArmaReforgerWorkbench\logs\`.

| # | Имя | Итог | Папка лога | SHA-256 console.log |
|---|---|---|---|---|
| 1 | rd01-radiobase | PASS | ricon-wbreader-rd01-radiobase-20261021-101849 | 6e8f8537ede609eac2e065e23a2b62020408e742fcb715fbd3cb4441b0319954 |
| 2 | rd02-r148 | PASS | ricon-wbreader-rd02-r148-20261021-101931 | 135db4412eefa4a9fafccc5d5db9d2d54b90dfffffdb2ba0fd22423b8ef9400a |
| 3 | rd03-r148-tail | PASS | ricon-wbreader-rd03-r148-tail-20261021-101952 | 1bdb8227e4dc2b0d94bf870eb0c40d0175955f171bef7c0c8d4a3da86dcd18b0 |
| 4 | rd04-radios-range | PASS | ricon-wbreader-rd04-radios-range-20261021-102016 | fdc076f1e310500fb50d39f2d9ca5d171fb9ca3f6b1f2c2908cb409f98d4c353 |
| 5 | rd05-r107m | PASS | ricon-wbreader-rd05-r107m-20261021-102042 | a931cea61543d06ab1b781b26bd95ad6643279581b96991c26fa666ab8f49030 |
| 6 | rd06-deployable | PASS | ricon-wbreader-rd06-deployable-20261021-102108 | d2f0ef9db9cef3c5ce9b4b853066c1850f1730b2e14da70311ef11c5977d9f84 |
| 7 | rd07-deployable-tail | PASS | ricon-wbreader-rd07-deployable-tail-20261021-102143 | a35fdc0eeb9d4f55a8b34f87971501874703035e11e0438a5defa51f611b6539 |
| 8 | rd08-anprc77 | PASS | ricon-wbreader-rd08-anprc77-20261021-102207 | 1c9692a7064faebd0dcdad64649263b8915e551a901b192b84ca6dcf469237fe |
| 9 | rd09-rf10 | PASS | ricon-wbreader-rd09-rf10-20261021-102233 | ecae827c300ecdfcf9a226998128814404dc0bb486e17e323a22733634ad1b40 |
| 10 | rd10-radios-parents | PASS (0 совпадений: кавычка в -Grep срезана CLI) | ricon-wbreader-rd10-radios-parents-20261021-102257 | 353624adcda6caa8076b071f96f1509c80f4a9417268089f633719c5ff49a9d3 |
| 11 | rd11-radios-parents2 | PASS | ricon-wbreader-rd11-radios-parents2-20261021-102322 | d5a7d762e70a6322aac66256d9638ed9800c2600969c1d5d52082728b14b1ca7 |
| 12 | rd12-radios-keys-freq | PASS | ricon-wbreader-rd12-radios-keys-freq-20261021-102344 | f769a9d5a6e3cf9e419bd34bc2c32df74f065b25b246b16168a7d4d47a9546ed |
| 13 | rd13-radios-chanfreq | PASS | ricon-wbreader-rd13-radios-chanfreq-20261021-102405 | 8fa577b58c0530ed3b4c51b63fbfa88a277d25257d0a3067af92d26d7ef7e734 |
| 14 | rd14-char-von | PASS | ricon-wbreader-rd14-char-von-20261021-102428 | 3ac1c26509d1ea2fdfd5a2a8771c84e8c39267ba6d6b5f2e7ae2db3d1786f9a4 |
| 15 | rd15-voncontroller | PASS | ricon-wbreader-rd15-voncontroller-20261021-102451 | 65796c3bf309ccf098c0d2344f1f4e8476715a840660c365182c26838d313a9d |
| 16 | rd16-autotune-conf | PASS | ricon-wbreader-rd16-autotune-conf-20261021-102512 | ffedfdde8a8a3a4d04644a19aa09bd593c42470d7d52d3c32f3a48de27301399 |
| 17 | rd17-faction-freq-conf | PASS | ricon-wbreader-rd17-faction-freq-conf-20261021-102537 | 7b6f664bc994d24444ecfc46ea85df2bef78da78eaf046dc75ef7bf94be6d726 |
| 18 | rd18-factions-freq-all | PASS | ricon-wbreader-rd18-factions-freq-all-20261021-102605 | 82a9ca250be6ae560b3a741a468604bfd7fc6a8ab5febb222b32a524a16822b8 |
| 19 | rd19-factions-key | PASS | ricon-wbreader-rd19-factions-key-20261021-102630 | 5f123d60ccb979f0d88f857a6e9c9b928e44c77a299862add65b973cb030a2bb |
| 20 | rd20-sounds-von-conf | FAIL (SearchResources=false, exit 3) | ricon-wbreader-rd20-sounds-von-conf-20261021-102705 | 1dacab6bd07dc086b91deb0b81802ea90440c3ad01db7cb726dda4822c76d89d |
| 21 | rd21-sounds-conf-quality | PASS (196 кандидатов, 0 совпадений) | ricon-wbreader-rd21-sounds-conf-quality-20261021-102729 | 25e7dfefedf2c77678c9e7a42f87fad03f0f16f733bf27cc2144e368502e7704 |
| 22 | rd22-von-acp | FAIL (cannot open von.acp, exit 4) | ricon-wbreader-rd22-von-acp-20261021-102800 | 0c79092bed943c635dc8c571fd1c5ef4373066a1740eb65df209df3eea8edab3 |
| 23 | rd23-vehicle-transceivers | PASS | ricon-wbreader-rd23-vehicle-transceivers-20261021-102822 | d13659e684f712f4aa26768a84253dbb4e32838e257f290b321fd112a9d83894 |
| 24 | rd24-intercom-m151 | PASS | ricon-wbreader-rd24-intercom-m151-20261021-102851 | e16ec772393c2cb922cc1fe6e01e4be613f07e543ebc85453439b7c72ca8bd8a |
| 25 | rd25-relay-structsys | PASS | ricon-wbreader-rd25-relay-structsys-20261021-102929 | 46d5fa772b594bcbf9a014c12f77834008898bfd10a629a837ab8c51b4d5c0df |

SHA-256 считались по готовым логам (sha256sum / Get-FileHash). Каждый [WB start]/[WB done] опубликован в agora, сообщения #2997–#3056.
