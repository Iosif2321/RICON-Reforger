# Звуковой граф VoN в GUI Audio Editor: структура прочитана, числа — нет

Дата: 2026-10-21. Автор: claude. Статус: OWN_CHECKED, ждёт ревью codex. Уровень: WORKBENCH_GUI_STATIC
(ресурс открыт в редакторе, звук не воспроизводился, Play, мир и сервер не запускались).
Основание: владелец в чате claude 2026-10-21 — «если можешь сам используй audio editor или расскажи как это
сделать мне». Закрывает структурную часть пункта 2 заказа №001 (`WB-order-001.md`) и раздела «Сначала Audio Editor»
`WB-order-link-quality.md`. Числа кривых не прочитаны.

## Как открыто

- `tools/RICON_WbReader/Scripts/WorkbenchGame/RICON_AcpViewPlugin.c`, запуск `-wbModule=ScriptEditor
  -plugin=RICON_AcpViewPlugin -riconAcpView=acp|sig`, видимое окно, один процесс.
- `Workbench.OpenResource("Sounds/VON/von.acp")` → `true`; та же строка с GUID `{B4C3941EC8B2AD20}` → `false`.
  Отсюда неудача R26/R27: нужен путь без GUID и без `$ArmaReforger:`, и открывать через `Workbench`, а не через модуль.
- Снимки — `PrintWindow` только окон своего PID, хранятся в scratchpad и в репозиторий не попадают (ванильный
  контент). Вид меняли сообщениями `PostMessage` в окна своего PID: колесо — масштаб, стрелки — прокрутка.
  Глобальный ввод, щелчки, Save и правка узлов не использовались. Щелчки, присланные сообщениями, Qt игнорирует.
- Чтение полей скриптом не удалось: `AudioEditor.GetNumContainers()` = 0 сразу после открытия, а
  `CallLater` в Workbench без мира не исполняется.

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

## Вывод для RICON

- **Подтверждено (структура):** в ванильном графе native `TransmissionQuality` управляет радиозвуком тремя
  путями: долей искажения (Bitcrusher), громкостью шума и громкостью самого голоса. Прямой голос от него не
  зависит. Значит, если RICON сможет задавать `TransmissionQuality` для пары, «плохая связь» станет слышна
  средствами ванили. Можно ли задавать его на пару — по-прежнему вопрос S4 и опытов (Э1r).
- **Не установлено:** числа Interpolate (X/Y min/max), значения `Vol Min [dB]`, настройки фильтров, кто и как
  считает native `TransmissionQuality`. Как звучит — не проверено.
- Чтобы получить числа, нужен настоящий щелчок по узлу (панель Item detail). Варианты: владелец щёлкает узлы сам
  (Interpolate 1, Interpolate 1 под ним, Interpolate 14, оба Value) и присылает снимки панели, либо разрешает
  агенту на ~5 минут настоящую мышь, пока сам ПК не трогает. Ресурс не сохранять.
