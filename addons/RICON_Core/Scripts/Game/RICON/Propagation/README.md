# Распространение сигнала

Наблюдение пути и параметры среды → модель потерь и шума → бюджет радиолинии и оценка её состояния.

| Файл | Назначение |
|---|---|
| [`RICON_AntennaPattern.c`](RICON_AntennaPattern.c) | Диаграмма направленности антенны. |
| [`RICON_BaseWorldPhysicalObservation.c`](RICON_BaseWorldPhysicalObservation.c) | Адаптер наблюдения физических данных мира. |
| [`RICON_ChannelCutoff.c`](RICON_ChannelCutoff.c) | Отсечение полезного вклада канала по интервалам. |
| [`RICON_ChannelDescriptor.c`](RICON_ChannelDescriptor.c) | Описание и обработка параметров канала. |
| [`RICON_ChannelFading.c`](RICON_ChannelFading.c) | Временные изменения коэффициента канала. |
| [`RICON_IntelligibilityModel.c`](RICON_IntelligibilityModel.c) | Интерфейс оценки разборчивости; производственная модель не задана. |
| [`RICON_LinkEvaluator.c`](RICON_LinkEvaluator.c) | Оценивает бюджет радиолинии и её состояние. |
| [`RICON_MaterialProfile.c`](RICON_MaterialProfile.c) | Профили радиосвойств материалов. |
| [`RICON_MaterialSlab.c`](RICON_MaterialSlab.c) | Модель потерь при прохождении материала. |
| [`RICON_NoiseModel.c`](RICON_NoiseModel.c) | Модели шума и его оценки. |
| [`RICON_ObjectRfProfile.c`](RICON_ObjectRfProfile.c) | RF-параметры объектов на пути сигнала. |
| [`RICON_OvercanopyLayer.c`](RICON_OvercanopyLayer.c) | Модель распространения над растительностью. |
| [`RICON_PathObservation.c`](RICON_PathObservation.c) | Данные о пути и пересекаемых средах. |
| [`RICON_PhysicalObservation.c`](RICON_PhysicalObservation.c) | Контракты и проверка физических наблюдений. |
| [`RICON_PropagationModel.c`](RICON_PropagationModel.c) | Базовый контракт и модель распространения. |
| [`RICON_StaticLosChannel.c`](RICON_StaticLosChannel.c) | Данные канала для статической прямой видимости. |
| [`RICON_TerrainDiffractionModel.c`](RICON_TerrainDiffractionModel.c) | Модель дифракции и профиль рельефа. |
| [`RICON_VegetationLayer.c`](RICON_VegetationLayer.c) | Потери, связанные с растительностью. |
| [`RICON_VegetationPathLayer.c`](RICON_VegetationPathLayer.c) | Участки растительности вдоль пути сигнала. |
| [`RICON_WaterLayer.c`](RICON_WaterLayer.c) | Модель влияния воды на путь сигнала. |
| [`RICON_WorldProfileProvider.c`](RICON_WorldProfileProvider.c) | Получает и формирует профиль высот мира. |

[← Карта RICON](../README.md)
