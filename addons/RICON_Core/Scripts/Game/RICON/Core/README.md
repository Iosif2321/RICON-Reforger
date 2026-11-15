# Ядро и состояние

Жизненный цикл и данные связей → расчёт пары → публикация состояния. Менеджер связывает эти части с устройствами и медиатрактом.

| Файл | Назначение |
|---|---|
| [`RICON_CommsManagerComponent.c`](RICON_CommsManagerComponent.c) | Координирует модели, жизненный цикл и маршруты связи. |
| [`RICON_GameControllerLifecycle.c`](RICON_GameControllerLifecycle.c) | Связывает жизненный цикл с игровым контроллером. |
| [`RICON_Lifecycle.c`](RICON_Lifecycle.c) | Отслеживает игроков, устройства и разрешения передачи. |
| [`RICON_LinkPublication.c`](RICON_LinkPublication.c) | Формирует и публикует данные связей. |
| [`RICON_LinkSnapshot.c`](RICON_LinkSnapshot.c) | Описывает снимок состояния связи. |
| [`RICON_LinkTypes.c`](RICON_LinkTypes.c) | Общие типы запросов, результатов и причин отказа. |
| [`RICON_OpenRadioListenerRoster.c`](RICON_OpenRadioListenerRoster.c) | Хранит состав слушателей открытого радиоканала. |
| [`RICON_OpenRadioStaticLosSource.c`](RICON_OpenRadioStaticLosSource.c) | Готовит серверные данные открытого канала для статической геометрии. |
| [`RICON_PairService.c`](RICON_PairService.c) | Собирает и оценивает пары передатчик–приёмник. |
| [`RICON_Pcm16WaveFile.c`](RICON_Pcm16WaveFile.c) | Работает с PCM16 в формате WAV. |
| [`RICON_PcmBatchQueue.c`](RICON_PcmBatchQueue.c) | Очередь групп PCM-данных. |
| [`RICON_PcmWavePlaybackOwner.c`](RICON_PcmWavePlaybackOwner.c) | Управляет опытным выводом подготовленных PCM-окон. |
| [`RICON_StaticLosTick.c`](RICON_StaticLosTick.c) | Представляет тик статической модели прямой видимости. |
| [`RICON_TemporalAuthorityJournal.c`](RICON_TemporalAuthorityJournal.c) | Журнал авторитетных временных данных. |
| [`RICON_TemporalClock.c`](RICON_TemporalClock.c) | Операции с модельным временем и интервалами. |
| [`RICON_TemporalLifecycle.c`](RICON_TemporalLifecycle.c) | Временное состояние, порядок и актуальность связей. |
| [`RICON_TemporalPublication.c`](RICON_TemporalPublication.c) | Структуры публикации временных данных. |
| [`RICON_TemporalPublicationOwner.c`](RICON_TemporalPublicationOwner.c) | Владеет публикацией и связанными данными RF-модели. |
| [`RICON_TemporalSnapshot.c`](RICON_TemporalSnapshot.c) | Снимки и обработка временных состояний. |

[← Карта RICON](../README.md)
