# От передатчика к слушателю: проверенная цепочка R20

Дата: 2026-10-20. Автор: codex. Статус: DRAFT до перекрёстного ревью.
Основание: Intent, PLAN v5, уточнение владельца agora #2020. Исходный Core: `3c295ce`.

Результат проверки: RICON вычисляет радиобюджет, состояние и допустимость снимка. Подключения этого решения
к реальному голосу пока нет: корректный вызов seam заканчивается `BLOCKED_NO_ACTUATOR`. Ванильный обработчик
приёма передаёт полученное качество в UI и событие; формулу качества и звуковое действие из него вывести нельзя.
Прямой голос остаётся ванильным по PLAN v5; сотовая сеть и доставка через ретранслятор ещё не реализованы.

## RICON: что именно вычисляет код

Пути таблицы относительно `addons/RICON_Core/Scripts/Game/RICON/`.

| Звено | Файл и строки | Решение / формула | Что уже проверено |
|---|---|---|---|
| Каталог и выбор модели | `Core/RICON_CommsManagerComponent.c:120`, `:159`, `:224` | Регистрация, проверка зависимостей, выдача evaluator только после freeze. Это реестр, а не работающая система сбора координат и запуска расчётов | SOURCE; EXT fixtures исполнялись в Workbench |
| Вход одного ребра | `Core/RICON_LinkTypes.c:42`, `:50`; `Propagation/RICON_LinkEvaluator.c:187` | Частота МГц, полоса Гц, мощности дБм; ровно один порог: чувствительность либо требуемый SINR. Невалидный вход возвращает причину, а не физическое LOST | SOURCE; L0 oracle; полного L0 Enforce parity ещё нет |
| Свободное пространство | `Propagation/RICON_PropagationModel.c:75` | `20 log10(4π max(d,λ)/λ)`, `λ=c/f`. Ограничение при `d<λ` — проектная защита формулы дальней зоны | ORACLE; используется в L2 adapter checks |
| Рельеф | `Propagation/RICON_TerrainDiffractionModel.c:256`, `:796` | P.526-16 §4.5: Bullington + поправка сферической Земли; два слоя `fspl` и `terrain_diffraction`. Горизонтальная дистанция профиля отличается от 3D-дистанции FSPL | ENFORCE_PARITY 45/45 на synthetic-профилях |
| Профиль мира | `Propagation/RICON_TerrainDiffractionModel.c:175`, `:192` | Базовый поставщик отказывает; есть synthetic-поставщик. Рабочего чтения высот `BaseWorld.GetSurfaceY` здесь нет | SOURCE; отказ → reason 16, пустые слои, проверено адаптером |
| Мощность приёмника | `Propagation/RICON_LinkEvaluator.c:109` | `P_rx=P_tx+G_tx+G_rx−ΣL` | L0 ORACLE; полный Enforce parity NOT_TESTED |
| Шум и SINR | `Propagation/RICON_LinkEvaluator.c:118`; `Propagation/RICON_NoiseModel.c:65` | `N0=kT0B`; `N=N0+10log10(f_a+f_r−1)`; мощности помех складываются в линейной шкале, затем `SINR=P_rx−(N+I)` | L0 ORACLE; область внешнего шума ограничена конфигом/REQ |
| Приём и состояние | `Propagation/RICON_LinkEvaluator.c:138`, `:145`; `Propagation/RICON_IntelligibilityModel.c:63` | Сначала `P_rx < sensitivity → LOST`; иначе TEST_ONLY кривая SINR→Q и пороги WEAK/DEGRADED/CLEAN. При равенстве чувствительности приём проходит эту проверку | ORACLE; речевая разборчивость NOT_CALIBRATED |
| Несколько ног | `Network/RICON_NetworkContracts.c:119`, `:174` | Проверка структуры маршрута; качество = минимум Q ног, состояние = худшее. Это контракт маршрута, а не перенос аудио ретранслятором | SOURCE; route ORACLE, полный Enforce parity NOT_TESTED |
| Доставка снимка | `Core/RICON_LinkSnapshot.c:161` | Версия, epoch, последовательность, TTL и запрос resync. Наличие pure receiver не доказывает работающие RPC/JIP | ORACLE; runtime NOT_TESTED |
| Голосовая граница | `Network/RICON_NetworkContracts.c:350`, `:367` | Проверки снимка, binding, идентичности; любой допустимый вызов → `BLOCKED_NO_ACTUATOR`. Ни SetRange, ни mute, ни аудиограф здесь не вызываются | ENFORCE_PARITY 103/103 вызова |

`valid=true` означает корректно рассчитанный бюджет, а не слышимый голос. `LOST` может быть корректным
физическим результатом. Q из `linear_test` не является долей распознанных слов; её параметры не калиброваны.
Для двухлучевой модели дальность может иметь провалы между доступными участками: одна бисекция без
доказанной монотонности не устанавливает непрерывную зону приёма (agora #2072, #2083).

## Ваниль: где видна граница движка

Проверено штатным `tools/RICON_WbReader`, максимум 15 исходных строк за запуск, без Play/мира/сервера.
`proto external` означает, что реализация в прочитанном скрипте отсутствует. Это не доказательство
конкретной формулы потерь, влияния рельефа или соответствия реальной радиофизике.

| Звено | Проверенный источник | Установлено | Осталось неизвестным |
|---|---|---|---|
| Передача | `$ArmaReforger:scripts/Game/generated/Radio/BaseTransceiver.c:49–58` | Дальность задаётся передатчику; SetRange только owner/server, ограничение 1…65535 м; BeginTransmission использует настроенную частоту, BeginTransmissionFreq явно задаёт кГц | Закон ослабления и точная граница доставки native |
| Поиск | `$ArmaReforger:scripts/Game/generated/Entities/RadioManagerEntity.c:24`; offline `interfaceRadioManagerEntity.html:146` | Пространственный поиск трансиверов; документация ограничивает результат включёнными устройствами | Это не проверка совпадения частоты/ключа и не подтверждение приёма голоса |
| Ключ сети | `$ArmaReforger:scripts/Game/generated/Components/BaseRadioComponent.c:34`, `:38` | Есть native Set/GetEncryptionKey; событие изменения ключа приходит клиенту и серверу (:65) | Алгоритм фильтрации/звучание чужого ключа этим чтением не установлены |
| Выбор канала | `docs/evidence/E-workbench-source.md:145–155` (источник заказа №001) | `SCR_VONController.SetActiveTransmit` выбирает SQUAD_RADIO + трансивер либо DIRECT + null | Реальная синхронизация и её задержка |
| Приём | `$ArmaReforger:scripts/GameCode/Network/SCR_VoNComponent.c:78–86` | Обработчик передаёт `quality`/частоту в display и invoker. Он не меняет звук и не возвращает решение о приёме | Откуда native quality; как движок решает доставку |
| Единицы приёма | offline `interfaceVoNComponent.html:401–408` | Событие о полученных аудиоданных; quality 0…1; частота в кГц может отличаться от настроенной | Нельзя считать quality равной RICON Q или SINR; отсутствие callback само по себе не объясняет причину потери |
| Звук | `docs/evidence/E-workbench-source.md:202–208` | ACP через FileIO не прочитан. Запись об источнике TransmissionQuality в прежнем web evidence относится к документации, не к проверенному графу | Узлы/кривые Audio Editor, реальная слышимая деградация |

Скриптовый контракт подтверждает управляемую дальность TX. Один scalar SetRange не кодирует произвольную
матрицу TX×RX с разными препятствиями у разных слушателей. Это ограничение представления; возможность
другого механизма управления внутри мода и его безопасность ещё предстоит проверить на Э1r/Э4T-f.

## Воспроизводимость

В PowerShell для текущего процесса разрешено выполнение собственных скриптов. Виртуальный путь должен
включать `$ArmaReforger:`; относительный `scripts/Game/generated/Radio` дал SearchResources=false.

```powershell
& .\tools\RICON_WbReader\run.ps1 -Name r20-range-qualified -Root '$ArmaReforger:scripts/Game/generated/Radio' -Ext c -Grep 'SetRange' -PathFilter 'BaseTransceiver.c' -Context 5 -MaxFiles 1
& .\tools\RICON_WbReader\run.ps1 -Name r20-radio-manager-all -Root '$ArmaReforger:scripts' -Ext c -Grep 'GetTransceiversInRange' -PathFilter 'RadioManagerEntity.c' -Context 4 -MaxFiles 1
& .\tools\RICON_WbReader\run.ps1 -Name r20-radio-key -Root '$ArmaReforger:scripts' -Ext c -Grep 'Encryption' -PathFilter '/BaseRadioComponent.c' -Context 1 -MaxFiles 1
& .\tools\RICON_WbReader\run.ps1 -Name r20-von-body -Root '$ArmaReforger:scripts' -Ext c -Grep 'OnReceive' -PathFilter 'SCR_VoNComponent.c' -Line 78 -MaxFiles 1
python -B -m unittest discover -s oracle -p 'test_*.py'
python -B -m unittest discover -s tools/RICON_TestRunner -p 'test_*.py'
python -B -I oracle/l2_terrain_oracle.py --check oracle/vectors/l2_terrain_v1.json
```

Последние три команды на исходном R19: 61/61, 17/17 и 45 cases PASS. Полные логи reader лежат вне репозитория
в `Documents/My Games/ArmaReforgerWorkbench/logs/<папка>/console.log`; это WORKBENCH_STATIC.

| Папка | SHA-256 console.log |
|---|---|
| `ricon-wbreader-r20-range-qualified-20261020-225719` | `7c4b730889d86a3ca6420c36e90fca1555d196857636f095b6ec7e710c2c69e5` |
| `ricon-wbreader-r20-radio-manager-all-20261020-225812` | `f272c851d009f58d87ee71b6ca310d41483fcde2072723efe5422ab152b39223` |
| `ricon-wbreader-r20-radio-key-20261020-225854` | `f22b0fbc9b5691c0baff8cd763487789b6040dd9fe17534779fa9c4185aef611` |
| `ricon-wbreader-r20-von-body-20261020-225916` | `449ac2d0b4aebec4b6dec68ca99e12fa0c1a8041cee0d5ecaa99f8b7735f1b4f` |

У каждого успешного запуска: exit 0, SearchResources=true, один кандидат и финальный `done` с 9–15 строками.
Это успешное чтение ресурсов. В общих логах также есть Steamworks RequestCurrentStats/resource-leak сообщения;
проверку компиляции или runtime по этим запускам не заявляем. Неудачные/пустые поиски не приняты как отсутствие API.
Следующий измерительный шаг описан в `docs/evidence/WB-order-link-quality.md`.
