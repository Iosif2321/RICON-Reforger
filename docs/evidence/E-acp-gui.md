# Звуковой граф VoN в GUI Audio Editor: структура и числа сигнала качества

Дата: 2026-10-21. Автор: claude. Статус: OWN_CHECKED, ждёт ревью codex. Уровень: WORKBENCH_GUI_STATIC
(ресурс открыт в редакторе, звук не воспроизводился, Play, мир и сервер не запускались).
Основание: владелец в чате claude 2026-10-21 — «если можешь сам используй audio editor или расскажи как это
сделать мне». Закрывает структурную часть пункта 2 заказа №001 (`WB-order-001.md`) и раздела «Сначала Audio Editor»
`WB-order-link-quality.md`. Числа прочитаны 2026-10-22 (раздел «Числа»), кроме частот Biquad HP/LP.

## Как открыто

- `tools/RICON_WbReader/Scripts/WorkbenchGame/RICON_AcpViewPlugin.c`, запуск `-wbModule=ScriptEditor
  -plugin=RICON_AcpViewPlugin -riconAcpView=acp|sig`, видимое окно, один процесс.
- `Workbench.OpenResource("Sounds/VON/von.acp")` → `true`; та же строка с GUID `{B4C3941EC8B2AD20}` → `false`.
  Это наблюдение одной рабочей комбинации (OpenResource, путь без GUID, видимый GUI), а не доказанная причина прежних
  INCOMPLETE: в R26 `SetOpenedResource` уже возвращал true, но корень не получен (`E-audio-api.md`), а R27 читал
  ресурс другим методом, как config (`E-acp-r27.md`). Замечание codex agora #3282.
- Снимки — `PrintWindow` только окон своего PID, хранятся в scratchpad и в репозиторий не попадают (ванильный
  контент). Вид меняли сообщениями `PostMessage` в окна своего PID: колесо — масштаб, стрелки — прокрутка.
  В запусках 2026-10-21 глобальный ввод, щелчки, Save и правка узлов не использовались; щелчки, присланные
  сообщениями, Qt в этих запусках не принял. Настоящая мышь 2026-10-22 — раздел «Числа».
- Чтение полей скриптом не удалось: `AudioEditor.GetNumContainers()` = 0 сразу после открытия. Повторное чтение
  через `GetGame().GetCallqueue().CallLater` в этом запуске не получено; причина и семантика не установлены.
  У `GetContainer`/`GetNumContainers` в Script API есть только сигнатуры, без обещания ACP-контейнеров.

| Запуск (папка логов Workbench) | Что | SHA-256 `console.log` | SCRIPT(E) |
|---|---|---|---|
| `ricon-acpview-20261021-232543` | OpenResource: GUID false, путь true | `81077dee14f9c0fa5fedcb2b7ec7e24a9eac53f1b0c2fe8bf4a9ca7283bf1564` | 0 |
| `ricon-acpview-20261021-232756` | просмотр von.acp (снимки 19–76 %) | `7a868fcb8a6547bbddd0b7ff26c7b2ad5a0cbc2d7d27a231b1e7cf53c9b08fa9` | 0 |
| `ricon-acpview-20261021-233556` | просмотр Von.sig | `73b16f2d3c86cff214d562d3e74017fa230c6e10d5282cfe595a613834c08f92` | 0 |
| `ricon-acpview-20261021-233948` | чтение полей: containers=0, INCOMPLETE | `1bd2e15cfee23763e95dbd5b20db0ce054b99c55a9ed3dfa8a4b680fc4fc957f` | 0 |

Ещё два запуска до них отброшены: в первом OpenResource по GUID дал `false`, во втором была ошибка компиляции
плагина (`set` — имя типа). Итоговый исходник плагина: SHA-256 `ae71a3938092aaac9aad3416d9c302c4020e30520f07efe308c132bd001b4d2a`.

## von.acp: три ветви от одного голоса

Пересказ по снимкам; имена узлов — как в редакторе (`…` — имя обрезано в узле).

- **Общий вход.** `Stream 1` → `Bus Direct Processing` (его DSP — `FilterChain 52` из Filter HP, LP, LS, Compressor).
  Выход этой шины идёт в три ветви.
- **Сырой.** `Shader RAW` → `Sound VON_RAW`; выходы VON_RAW никуда не подключены.
- **Прямой голос.** Шина `…ing When Many Voices` (громкость ← `Variable VON_Direct`) → `Shader Direct`. У шейдера
  входы Spatiality ← `Spatiality`, Amplitude ← `…de -40LUFS to -35LUFS`, Frequency ← `…uency Character Voice`, а у
  того Cutoff ← `Signal Occlusion` (входы Interior, UnderPlayerControl, IsInPlayerVehicle, RoomSize). Дальше
  `Sound VON_DIRECT` → `OutputState.VON`, Aux Out → реверберация.
- **Радио.** `Bus Voice + Noise`: In — тот же обработанный голос плюс `Generator Noise`; Volume ← `Signal 1.Radio_V`;
  DSP ← `FilterChain` из Filter Bitcrusher, HP, Clipper, LP. Wet/Dry Bitcrusher ← `Signal 1.Quality_W`, громкость
  Generator Noise ← `Signal 1.Noise_V`. Порты Wet/Dry у HP, Clipper и LP выглядят свободными. Рядом комментарий
  автора: компрессор здесь не ставить, иначе при смешении DIRECT и RADIO гуляет панорама. Дальше `Shader Radio`
  (только вход In, без Spatiality/Amplitude) → `Sound VON_RADIO` → OutputState.
- **Сигнал качества.** У `Signal 1` один вход `Transmissi…` и выходы Quality_W, Noise_V, Radio_V.
- **Прочее.** `Roger Beep A/B` → `SOUND_ROGER_BEEP`. `Signal Reverb` (Distance, Interior, RoomSize) → три AuxOut
  `Reverb INT Small/Medium/Large` → OutputState `VON Reverb Small/Medium/Large`.

## Von.sig: TransmissionQuality → три выхода

- Вход `TransmissionQuality` идёт в три узла Interpolate (X min, X max, Y min, Y max).
- `Interpolate 1` → выход `Quality_W` (портовые пределы свободны, значения в свойствах узла).
- Второй `Interpolate 1` → `Noise_V`; Y max ← `Db2Gain 10` ← `Value Vol Min [dB]`.
- `Interpolate 14` → `Radio_V`; Y min ← второй `Db2Gain 10` ← второй `Value Vol Min [dB]`.
- Port Ordering: один вход, три выхода.

## Числа (2026-10-22, настоящая мышь)

Основание: владелец в чате claude 2026-10-22 — «Дай подробную инструкцию или сделай сам». Щелчки — настоящей
мышью (`SetCursorPos` + `mouse_event`) только по окнам своего PID, перед каждым щелчком проверялось, что под точкой
окно этого процесса. Узлы выделялись, Item detail читался по снимку. Один случайно вызванный диалог «Reset Default
Layouts» закрыт кнопкой No. После щелчков в заголовке появлялась `*`; Save не вызывался, процесс завершён без
сохранения, `git status` чист.

| Запуск | Что | SHA-256 `console.log` | SCRIPT(E) |
|---|---|---|---|
| `ricon-acpview-20261022-211206` | Von.sig: Interpolate, Value, Db2Gain | `6932f9154598b2d8854809a59ef350d3deb6ae449bd83f4ab16b7caacfb38c0d` | 0 |
| `ricon-acpview-20261022-211551` | von.acp: Signal 1, фильтры, шина, шейдер, звук | `9b913f9884e7da38f7b0d28a7d5358edf049076f2410a856f746b65c9311965c` | 0 |

**Von.sig.** У всех Interpolate включён Clamp. Если к порту подключено значение, считаем, что оно заменяет число в
поле (так выглядит редактор; отдельно не проверялось). Db2Gain: 10^(дБ/20).

| Выход | X min…X max | Y при X min → при X max | Форма |
|---|---|---|---|
| Quality_W | 0 … 1 | 0.85 → 0.20 | Linear |
| Noise_V | 0 … 0.8 | 1.0 → Db2Gain(−60 дБ) = 0.001 (порт Y max) | Power of 1/3 |
| Radio_V | 0 … 1 | Db2Gain(−12 дБ) ≈ 0.251 (порт Y min) → 1.0 | Linear |

**von.acp, радиоветвь.**
- `Signal 1`: ресурс `Von.sig`.
- Filter Bitcrusher: Wet/Dry 0 в поле, на деле его задаёт Quality_W; BitCrusher, Bit Mask `11010101`.
- Filter HP и LP: Biquad, Wet/Dry 1, Volume 2. Частоты не раскрылись в панели.
- Filter Clipper: Distortion HardClipper, Drive 20.0, Wet/Dry 1.
- Generator Noise: BrownianNoise, 1 канал, Volume −14, Infinite.
- FilterChain: 4 порта.
- Bus Voice + Noise: Volume 0, Pitch 0, без ограничения.
- Shader Radio: ITD включён, Reverse Panning и Doppler выключены.
- VON_RADIO: Volume 0, Priority 50, Skip inaudible, Transformation Entity, Trigger Continuous.
  Переменная `VON_Radio` из `GlobalVariables.conf`.

Итог по качеству q = TransmissionQuality (q за пределами 0…1 отсекается):

- голос по радио растёт от −12 дБ при q = 0 до 0 дБ при q = 1;
- шум (броуновский, −14 в генераторе) — от 0 дБ при q = 0 до −60 дБ при q ≥ 0.8, кривая быстро падает у малых q;
- доля Bitcrusher — от 0.85 при q = 0 до 0.20 при q = 1: даже при идеальной связи остаётся 20 % искажения;
- HP, LP и Clipper включены всегда (Wet/Dry 1) — это постоянный «радиотембр», от q не зависит.

## Вывод для RICON

- **Подтверждено (структура):** в ванильном графе native `TransmissionQuality` управляет радиозвуком тремя
  путями: долей искажения (Bitcrusher), громкостью шума и громкостью самого голоса. Прямой голос от него не
  зависит. Значит, если RICON сможет задавать `TransmissionQuality` для пары, «плохая связь» станет слышна
  средствами ванили. Можно ли задавать его на пару — по-прежнему вопрос S4 и опытов (Э1r).
- **Числа** (раздел «Числа»): по ним S4 может перевести Q RICON в TransmissionQuality, если задавать его разрешат.
  Шум пропадает уже к q = 0.8; голос падает не ниже −12 дБ. Значит, без отдельного обрыва «нет связи» при q → 0
  остаётся слышимый, но шумный голос.
- **Не установлено:** частоты Biquad HP/LP, кто и как считает native `TransmissionQuality` в ванили, считается ли
  он на пару, диапазон q на самом деле. Как это звучит, не проверено.
