# D0: штатное communication-событие на двухролёвом стенде

Дата: 2026-11-02. Оператор: Lead. Launch revision `ec2d363`, код `9e280be`, public run-id `d0-native-001-20261102`.

**Результат: D0_LOCAL_EVENT_OBSERVED_A. Пересылка на B и VoN-протокол не доказаны.**

## Выполненный опыт

Один новый Workbench listen-host A=1 и один Diag-клиент B=2 в штатном MpTest, отдельные свежие profile/log roots. Native FS обеих сторон смонтировал ожидаемые профили. Общий timeout runner 300 с, watchdog 330 с; выполнен один public CommunicationSound queue request с `SOUND_VOICE_PAIN_RELIEVE`, priority80, ignoreQueue=false. No SetCapture/PTT, radio/direct setter, PCM или nonce.

[Машинная квитанция](<receipts/d0-live-001-lead.json>) сохраняет исходные пути/SHA-256 и выбранные записи. [Строгий readiness checker](<receipts/d0-live-001-readiness.json>) независимо проверяет endpoint-часть; это не проверка переноса события.

- A реально приняла initial B readiness на attempt6, затем fresh ACK challenge2/attempt1. Readiness — control-plane, не перенос event/outcome.
- Единственный запрос на A завершился за 1 мс. На том же компоненте зарегистрированы start и finish события с одинаковым AudioHandle268; finish не был forced/terminated. Время между start и finish по часам A — 920 мс.
- После finish A наблюдала ещё 10003 мс. Terminal A: requestUsed1, started1/finished1, invalid0, subscriptions0; поток seq0…49 без пропусков.
- B наблюдала 70011 мс, выдала оба readiness ACK и terminal с subscriptions0/invalid0. Её поток seq0…70 непрерывен, но на **наблюдавшихся локальных B communication/VoN компонентах** sound-start/finish и VoN RX не зарегистрированы.

Последний факт даёт INCONCLUSIVE для приёмного observable: другой remote/proxy sound sink этим не исключён. Нет положительной B-калибровки, sender/RX attribution события и тождества его протокола player voice. Нельзя назвать этот прогон ни сетевым провалом всех AI-events, ни успешным пакетом, ни слышимостью. Локальный CommunicationSound-вход не переименовывается в VoNComponent overload.

## Завершение

Обычный возврат редактора/режима дал RESTORE_OK; оба точных штатных Arsenal запроса подтверждены по одному разу. SCRIPT(E)=0 на обеих сторонах, client dependency errors0; источники стабильны. Registry/default profile diff0. Собственные host/client/оба watchdog завершены, повторная проверка дала ноль game/Workbench процессов. Исходные raw остаются вне Git; никакой аудиофайл не записывался.

Полный post-window не доказывает пустую очередь вообще; D0 не меняет уровни SAME_PATH_PACKET_PASS, AUDIO_PASS, H2/direct, MODEL_LOST или RICON actuation. Production seam по-прежнему fail closed.

## Что изменилось в знании

Подтверждён работоспособный **локальный** event/config на реальном avatar и исполнение readiness механизма на настоящих двух процессах. Следующее необходимое звено — квалифицированный VoN emitter и правильный B-side sink/протокол; повторять локальную очередь как доказательство сети нельзя. Публичные примеры native VoN values/приёмника исследуются отдельно; неизвестное не закрывается произвольным RPC или локальной подстановкой callbacks.
