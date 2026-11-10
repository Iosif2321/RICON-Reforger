# RICON / Agora — ограниченная проверка радиосигнала, 2–3 ноября 2026

Срез выполнен на RICON-Reforger HEAD `a983aaa` с сохранением прежних незакоммиченных изменений. Изменены восемь TOOL_ONLY файлов; production `addons/RICON_Core` не менялся. Новые исходники и текущие проверки перечислены в [манифесте](<../../../Cellular System/output/agora-radio-20261102/verification-manifest.json>).

## Подтверждённые результаты

| Проверка | Результат и граница |
|---|---|
| Две локальные роли | Workbench listen-host и отдельный Diag-клиент с назначенными персонажами/рациями; I0 дождался готовности и сохранил согласованные native-привязки на дискретных срезах. |
| RICON → native radio → отдельный B | Реальный `RICON_WirelessLinkEvaluator` допускает `BaseTransceiver.BeginTransmission`; `ScriptedRadioMessage.OnDelivery` получает точный B-трансивер. Это одна сцена и отдельные физические endpoints, не доказательство cross-process payload. |
| Частотный контроль generic radio | B на 32200 при TX 32000 не получил сообщение; другой receiver продолжал получать. Возврат 32000 восстановил B. |
| Управление моделью | Валидный модельный LOST запретил native TX submission; восстановление вернуло передачу. Мощность модели и линейная разборчивость — TEST_ONLY. Это общий TX admission, не directed receiver gate. |
| Native VoN AI event | `VoNComponent.SoundEventPriority` с CLEAR/Seed реально вызван; отдельный B увидел native sound-start на A-прокси. Собственный B `OnReceive`/radio RX остался 0. |
| Фальсификатор VoN observable | На тех же исходниках A=48000, все два обнаруженных B-трансивера=48200: callback A-прокси сохранился. Его собственный каталог в callback подтвердил mismatch. При свежем восстановлении A/B=48000 callback снова пришёл. Он не является свидетельством matched-frequency B-radio RX. |
| Очистка | У последних контрольных запусков `RESTORE_OK`, собственные game/Workbench/watchdog процессы завершены, registry/profile diff=0, `SOURCES_STABLE`; TX и захваченные radio properties восстановлены с readback. Первый generic attempt обновил два MRU значения; это записано отдельно. |

Native TX end/drain, player voice packet equivalence, слышимость, per-pair VoN gate/DSP и production authority/JIP этим срезом не подтверждены. Нулевой `OnReceive` при неквалифицированном sink не доказывает невозможность native связи.

## Проверки и исправления

- 480 HOST проверок в трёх наборах RICON; отдельная воспроизводимая композиция — 18 сценариев, 36 направлений и 3 отрицательных контроля.
- 199 актуальных Enforce проверок: I0=53, VoN/control=140, generic model gate=6. Старые прогоны 89/94/101/118 не суммируются повторно.
- Duo: 8/8 наборов, 136 именованных PASS и отдельный assert suite Codex2. Исходный dirty-срез Duo сохранён.
- Исправлены преждевременная фиксация пустой I0-привязки, смешивание двух sequential snapshots, неверный вход Seed, выбор частотной обёртки вместо подтверждённого direct setter, startup-only guard и лишняя сериализация quiet/ACK ожиданий. Все численные START/70/37/32/1/6/ACK ограничения сохранены.
- Контроль частоты учитывает полный обнаруженный B-каталог, ограничен восемью трансиверами, снимает baselines до первой записи и восстанавливает только собственные изменения. B-observer продолжает работать после восстановления A.

Дальность, шум/SNR, дифракция, материалы, растительность, вода и fading проверены по существующим моделям. Их области и открытые переходы к миру/VoN перечислены в полном отчёте. Fading не подключён к production budget/PCM; физическая калибровка и погода остаются открытыми.

## Модернизация VoN

Изучены прежние Codex/Agora обсуждения, installed API и первичные документы. Поддержанный путь — Enforce + receiver ACP + диагностика; отдельные `VON_RADIO`/`VON_DIRECT` и источник `Stream` не дают автоматически per-stream RICON setter. InterceptAR — сторонний DS/Workbench proto-мост; клиентский voice hook и точная совместимость с локальным 1.8.0.13 не квалифицированы. Собственные native DLL не загружались.

Следующий capability gate должен различать DESIRED, APPLIED и независимый post-gate OBSERVED, доказать точное связывание radio-only потока до mix и отдельное server recipient permission. Проверка адресности: A→B подавлен при сохранении C→B на том же RX и native DIRECT. Снятие собственного DSP эффекта не является отзывом lease.

## Доказательства

- [Полный отчёт и повторяемые команды](<../../../Cellular System/output/agora-radio-20261102/REPORT_RU.md>)
- [Реестр 12 агентов и независимых Agora-проверок](<../../../Cellular System/output/agora-radio-20261102/AGORA_EVIDENCE.md>)
- [Generic native receipt](<../../../Cellular System/output/agora-radio-20261102/generic-runtime-receipt.json>)
- [VoN wrong-frequency receipt](<../../../Cellular System/output/agora-radio-20261102/v1-wrong-runtime-receipt.json>)
- [VoN recovery receipt](<../../../Cellular System/output/agora-radio-20261102/v1-recovery-runtime-receipt.json>)
- [Сравнение native VoN observations](<../../../Cellular System/output/agora-radio-20261102/v1-frequency-control-comparison.json>)
- [Варианты модернизации и native ABI](<../../../Cellular System/output/agora-radio-20261102/VON_ENGINE_MODERNIZATION_OPTIONS.md>)

Сырые журналы содержат сведения локального окружения и остаются только локально; в Telegram передаётся краткое итоговое сообщение без них и без credentials.
