# Установка RICON Core 0.6.4 на Windows

1. Закройте игру и скачайте [RICON-Core-0.6.4.zip](https://github.com/Iosif2321/RICON-Reforger/releases/download/0.6.4/RICON-Core-0.6.4.zip).
2. Распакуйте папку «RICON_Core» в «Документы\My Games\ArmaReforger\addons». Проверьте, что файл «addon.gproj» лежит прямо в «addons\RICON_Core». Если там уже есть предыдущая сборка, сначала перенесите её в отдельную папку.
3. В параметрах запуска Arma Reforger в Steam добавьте «-addons 7C060DB9E8B90AC7». Если «-addons» уже указан, добавьте GUID к существующему списку через запятую.
4. Запустите игру и откройте **Сценарии → RICON Radio Training**. Для сетевой игры у сервера и игроков должна быть одинаковая версия аддона.

Основной канал раций RICON в Radio Training — **38 МГц**. Для связи через ретранслятор возьмите рацию RICON из сценария или используйте рацию с тем же игровым ключом. На стандартной раскладке удерживайте **Caps Lock** для выбранной рации и **T** для ближнего голоса. Если звука нет, проверьте громкость Arma Reforger в микшере Windows.

РЭБ и ретрансляторы настраиваются в Game Master на выделенном сервере. [Инструкция Game Master](GM_RF_INFRASTRUCTURE_RU.md)

Для старой интеграции с TeamSpeak используйте [выпуск 0.6.3](https://github.com/Iosif2321/RICON-Reforger/releases/tag/0.6.3). Для игры с 0.6.4 достаточно штатного VoN.
