# Сборка плагина RICON для TeamSpeak 3

Для 0.6.3 использованы Windows x64, Visual Studio 2022 Build Tools с C++, .NET Framework 4.x, PowerShell 7, Python 3 и заголовки официального TeamSpeak Plugin SDK API 26. Исходники аудиокомпонента и его условия распространения находятся в [dsp](dsp/NOTICE.md).

Из корня репозитория выполните команды в PowerShell. Выходной каталог сборки должен быть новым.

```powershell
git clone https://github.com/teamspeak/ts3client-pluginsdk.git external/ts3client-pluginsdk
git -C external/ts3client-pluginsdk checkout --detach 4aa90a53aa150cbf81e13bc97e68c0431b26499f
.\tools\RICON_TS3Probe\native\build.ps1 -OutputDirectory output/ts3-0.6.3 -PluginVersion 0.6.3
python -B tools/RICON_TS3Probe/test_package_manual.py
python -B tools/RICON_TS3Probe/package_manual.py output/ts3-0.6.3 output/RICON.ts3_plugin
```

Если SDK уже установлен, укажите его каталог `include` через `-SdkIncludeDirectory`. Сборка запускает самотесты C/C++ и проверки восстановления обновлятора. Открытый ключ в `updater/alpha-public.xml` нужен клиенту для проверки выпусков. Закрытый ключ издателя нужен только для подписи канала.
