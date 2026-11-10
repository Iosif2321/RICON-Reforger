# Изменения Reforger 1.9: влияние на RICON

2026-11-05. Установленная игра и Tools: 1.9.0.37. Предыдущий статический native baseline: 1.8.0.13. Цель RECEIVER/SERVER, временной канал и отдельная обработка звука сохранены; LOST/Q0 не отключают голос автоматически.

Прочитаны официальные stable/Experimental release notes и два modding update. Публикация changelog 1.9.0.37 датирована 4 ноября, выпуск обновления — 5 ноября. Публикации используются как указатель проверяемых изменений, а не как доказательство работы RICON или нового VoN API.

| Официально описанное изменение | Практическое действие для RICON |
|---|---|
| [1.9.0.37: оптимизация прямого VoN](https://reforger.armaplatform.com/news/changelog-november-4-2026) | Повторно квалифицировать DIRECT на текущей версии; не переносить прежние native адреса и замеры нагрузки. Причина конкретного изменения layout этим пунктом не доказана. |
| [1.9.0.37: исправлена проверка кэша радиомаршрута по позиции отправителя](https://reforger.armaplatform.com/news/changelog-november-4-2026) | После подключения проверить движение TX при неподвижном RX, сохранённую позицию и инвалидирование кэша. Эта запись не задаёт новую RF-модель или дальность. |
| [1.9.0.37: исправлена остановка capture при unpossess чужого персонажа на hosted server](https://reforger.armaplatform.com/news/changelog-november-4-2026) | Добавить этот случай в будущую lifecycle-приёмку; текущий запрет capture/Play сохраняется. |
| [Modding update 24 сентября: ElideText перенесён RichTextWidget → TextWidget](https://reforger.armaplatform.com/news/modding-update-sept-24-2026) | Согласуется с ранее наблюдённой ошибкой смешанных версий Game/Tools. Проектную правку классов интерфейса ради обхода этой ошибки не вводить. |
| [24 сентября: Timestamp, DateTimeView и Duration](https://reforger.armaplatform.com/news/modding-update-sept-24-2026) | Проверять конфликты имён. Wall-clock API не заменяет согласованное время RECEIVER/SERVER или существующий временной контракт. |
| [Modding update 21 октября: CloneContainer и SetObjectVariableMember](https://reforger.armaplatform.com/news/modding-update-oct-21-2026) | Возможное упрощение подготовки конфигураций Workbench. Не считать доступом к живому private VoN instance. |
| [21 октября: property grid и отображение наследуемых конфигураций](https://reforger.armaplatform.com/news/modding-update-oct-21-2026) | Учитывать при работе редактора. Из этих изменений не выводится правило миграции ACP/SIG или значения пропущенных полей. |
| [21 октября: TraceMove поддерживает displacement для End](https://reforger.armaplatform.com/news/modding-update-oct-21-2026) | При подключении world-reader проверить режим координат и численную устойчивость длинных трасс. Не менять End вслепую. |

Отдельно проверен [baseline 1.8.0.13](https://reforger.armaplatform.com/news/update-september-3-2026): исправление блокировки IOP.Reset относится к уже изучавшейся версии, а не является новой возможностью 1.9. Потоковую безопасность сохранять.

## Что удалось прочитать именно о Tools

Отдельный полный changelog Tools 1.9 не обнаружен в проверенных первичных каналах. Developer feeds Steam для Tools 1874910 и Experimental Tools 1890880 вернули по нулю записей; community URL перенаправляют в игровые hubs. Проверены также developer announcements игровых веток и полный HTML официальных modding update. Каналы, redirects, даты и hashes сохранены в [проверке Tools](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-von-attachment-20261104/fixture/version19/changelog-tools/REPORT_RU.md>) и [primary receipt](<C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-von-attachment-20261104/fixture/version19/changelog-tools/primary-sources.json>).

В этих источниках не найдено явного правила ACP v1→v2, исчезновения embedded compiled SIG, пропуска AudioBankSample defaults или обязательного способа их пересборки. Это пробел публикаций, не утверждение об отсутствии audio-изменений. Audio-статьи для 1.6 не используются как свидетельство обновления 1.9.

## Сопоставление с установленным проектом

- Поиск TraceMove, TraceParam, Timestamp, DateTimeView, Duration, CloneContainer и SetObjectVariableMember в `addons` и `tools/RICON_LiveProbe/Scripts` не дал совпадений. Это проверка этих каталогов, не всего диска; код ради новости не менялся.
- Installed source comparison восьми выбранных файлов не дал нового private VoN mutator. Публикации не расширяют эту проверенную поверхность API.
- Current native read отдельно доказал создание свежего AudioSourceTemplate размером 0x88 и сохранение его в VonStream. Это самостоятельный byte/RTTI факт; changelog не доказывает причину отличия от 1.8.
- Own ACP v2 и SIG уже сохранены редактором; явные входы и связи проверены. Неизвестные sample defaults и фактическая native программа остаются UNKNOWN. Changelog не даёт основания подменять их догадкой или вручную создавать compiled IOP.

Ближайшая инженерная работа остаётся прежней: поддерживаемое адресное управление private VoN и квалификация доставки. Из release notes в будущую приёмку добавляются движение отправителя при кэшировании маршрута и переход владения персонажем на hosted server. Автоматический повтор старых scheduler/H исследований, новый backend и APPLIED-заглушка не вводятся.
