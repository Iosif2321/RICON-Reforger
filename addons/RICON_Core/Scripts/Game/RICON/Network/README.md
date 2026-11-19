# Сеть и экспериментальное аудио

Вход PCM → проверка источника и маршрута → отправка / ретрансляция → локальный приём и подготовка вывода. Фактический живой голос и сетевая приёмка не подтверждены.

| Файл | Назначение |
|---|---|
| [`RICON_LocalRxOwner.c`](RICON_LocalRxOwner.c) | Управляет локальными приёмниками и их разрешениями. |
| [`RICON_MediaCaptureClock.c`](RICON_MediaCaptureClock.c) | Связывает интервалы захвата с временными шкалами. |
| [`RICON_MediaEgress.c`](RICON_MediaEgress.c) | Описывает маршруты и очереди исходящих кадров. |
| [`RICON_MediaFanout.c`](RICON_MediaFanout.c) | Распределяет данные по разрешённым маршрутам. |
| [`RICON_MediaIngress.c`](RICON_MediaIngress.c) | Проверяет и принимает входящие медиаданные. |
| [`RICON_MediaListenerAuthority.c`](RICON_MediaListenerAuthority.c) | Описывает допуск слушателя к медиамаршруту. |
| [`RICON_MediaPcmTestProfile.c`](RICON_MediaPcmTestProfile.c) | Ограниченный экспериментальный формат PCM и кодирование кадров. |
| [`RICON_MediaPlayout.c`](RICON_MediaPlayout.c) | Планирует выдачу медиаданных на воспроизведение. |
| [`RICON_MediaReceiverAuthority.c`](RICON_MediaReceiverAuthority.c) | Управляет правами и актуальностью получателя. |
| [`RICON_MediaRfHistory.c`](RICON_MediaRfHistory.c) | Хранит RF-данные, связанные с интервалами кадров. |
| [`RICON_MediaRpcTransport.c`](RICON_MediaRpcTransport.c) | Передаёт сообщения и медиаданные через RPC. |
| [`RICON_MediaStreamOwner.c`](RICON_MediaStreamOwner.c) | Управляет состоянием и входными данными медиапотока. |
| [`RICON_MediaWindow.c`](RICON_MediaWindow.c) | Ограничивает и упорядочивает окна медиаданных. |
| [`RICON_MediaWire.c`](RICON_MediaWire.c) | Формат и приём базовых сетевых медиасообщений. |
| [`RICON_MediaWireControl.c`](RICON_MediaWireControl.c) | Формат управляющих сообщений маршрута. |
| [`RICON_MediaWirePublication.c`](RICON_MediaWirePublication.c) | Публикует и отслеживает состояние маршрутов. |
| [`RICON_MediaWireRfV2.c`](RICON_MediaWireRfV2.c) | Экспериментальный формат кадра с RF-бюджетом. |
| [`RICON_MediaWireRfV3.c`](RICON_MediaWireRfV3.c) | Формат кадра с интервалом захвата и историей RF. |
| [`RICON_NetworkContracts.c`](RICON_NetworkContracts.c) | Общие сетевые и VoN-контракты. |
| [`RICON_PcmResampler48To8.c`](RICON_PcmResampler48To8.c) | Преобразует PCM с 48 кГц на 8 кГц. |
| [`RICON_PcmResampler8To48.c`](RICON_PcmResampler8To48.c) | Преобразует PCM с 8 кГц на 48 кГц. |
| [`RICON_RelayInputFrame.c`](RICON_RelayInputFrame.c) | Проверяет входной кадр ретранслятора. |
| [`RICON_RelayInputSeries.c`](RICON_RelayInputSeries.c) | Собирает последовательность входных кадров станции. |
| [`RICON_RelayLocalPlayout.c`](RICON_RelayLocalPlayout.c) | Планирует локальную выдачу ретранслированного сигнала. |
| [`RICON_RelayOutputQueue.c`](RICON_RelayOutputQueue.c) | Хранит подготовленные окна выхода ретранслятора. |
| [`RICON_RelayPcmHandoff.c`](RICON_RelayPcmHandoff.c) | Описывает передачу PCM между приёмом и станцией. |
| [`RICON_RelayPcmOutput.c`](RICON_RelayPcmOutput.c) | Готовит PCM-представление излучения станции. |
| [`RICON_RelayRfInput.c`](RICON_RelayRfInput.c) | Обрабатывает RF-условия входного плеча ретранслятора. |
| [`RICON_RelayRfOutput.c`](RICON_RelayRfOutput.c) | Рассчитывает RF-данные выходного плеча станции. |
| [`RICON_RelayStationAuthority.c`](RICON_RelayStationAuthority.c) | Проверяет полномочия и адреса станции. |
| [`RICON_RelayWire.c`](RICON_RelayWire.c) | Формат, приём и представление кадров ретранслятора. |
| [`RICON_RelayWireControl.c`](RICON_RelayWireControl.c) | Управляющие сообщения и готовность ретрансляции. |
| [`RICON_SharedRxTestMixer.c`](RICON_SharedRxTestMixer.c) | Экспериментально смешивает вклады в общем приёмнике. |
| [`RICON_SharedRxPresentationTimeline.c`](RICON_SharedRxPresentationTimeline.c) | Упорядочивает окна вывода общего приёмника. |

[← Карта RICON](../README.md)
