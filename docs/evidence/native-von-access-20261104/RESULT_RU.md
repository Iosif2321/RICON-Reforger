# Native VoN: public writer и область доставки

2026-11-04. Полная задача прочитана из owner attachment `e7fc704c-acd3-4d37-bbaf-aa86f22b0e92/goal-objective.md`; её SHA256 `9a1ab8001315126d1d9a85062f009e83d19612d9c79040e722af63f363282ce4`. Машинная Goal active с указателем на этот файл. Чтение файла не принято за завершение интеграции.

## Полученные факты

**B, запись параметров звука.** Concrete `SndComponent.SetSignalValue/Str` выбирает каталог своего native owner и через mapping вызывает тот же `A1B690`, который native VoN использует для изменения параметров собственного AudioSourceTemplate. Это подтверждённый code join публичного метода к реальному scalar writer. [Основное доказательство](writer/PROOF_RU.md).

Нужный resource D выбирается из каталога именно этого SndComponent. Совпадение внутреннего helper и имени Input не доказывает `D == T` голоса A у слушателя B. Проверенный item-loader в ветви нового элемента создаёт собственный AudioSourceTemplate и передаёт его для добавления в каталог, а не принимает готовый live VoN T. Early dedup, альтернативный SndSystem и поздние attachment/interception остаются отдельными границами; универсальное отсутствие будущей связи не утверждается. [Config-loader](writer/CATALOG_SUPPLEMENT_RU.md), [создание нового ресурса](writer/ITEM_LOADER_SUPPLEMENT_RU.md).

Следующая незакрытая возможность — поддерживаемый способ получить owner, каталог которого содержит точный private voice T, либо включить такой T/forwarding через официальный attachment contract. Public method existence не означает этот identity join; private address invocation/DLL не используются.

**A, доставка.** Для известной конечной геометрии `D_native + guard <= actual server R <= 65535` обычный native radio может не отбрасывать подходящие пары только по расстоянию. Неизвестные world bounds/высоты/движение не получают coverage PASS. Охват конкретного мира и фактический voice результат не проверены. [Условия и пределы](coverage/REPORT_RU.md).

Большой native R — candidate envelope, отдельно от физического RF-профиля RICON. Он не делает native q единицей и не отменяет fixed radio effects, cache, права, ключи/частоты, user state или общую Radio/Direct запись. Значение server65535 синхронизируется как half65504 у replica; effect нужно проверять на actual server instance.

При R65535 inclusive50m-grid enumerator создаёт6,874,884…6,880,129 coordinate pairs на одно завершённое перечисление. Это статический work count, не benchmark. Безусловный Rmax всему миру не предлагается: будущая проверка использует минимальный обоснованный R и малую заявленную область.

## Что сохраняется

RECEIVER/SERVER используют общий временной канал и receiver-control; фактический PCM/DSP у слушателя. LOST/Q0 не выключают голос и не удаляют потенциально восстанавливаемые фрагменты. Squelch, питание, mute, source authorization, технический отказ, физические шум/помехи и разборчивость остаются разными состояниями.

Три ранее внесённых исходника, Core, диагностические LOST fixtures и подтверждённая компиляция не менялись. Нового working actuator или APPLIED не объявлено. В этой проверке не запускались SDK/engine/Ghidra/Play/server/capture и передачи; DLL, бинарники, pagefile и другие приложения не изменялись.

Полная native интеграция ещё не квалифицирована: нужны фактический voice-instance binding, delivery coverage, временной receiver/audio contract и вся приёмка A/B/C/DIRECT. Следующий шаг должен проверять именно оставшуюся связь owner/catalog/VoN T, а не повторять Input-name или generic radio demonstration.


## Дополнение 2026-11-05

[Подготовка ресурсов и текущий attachment join](ASSET_PREPARATION_RU.md): Resource Manager/Audio Editor и Computer Use использованы в разрешённом owner scope. Own-resource регистрация/open подтверждены; actual compiled graph и private VoN binding остаются OPEN. Подготовленный context передаёт тот же instance в каталог, но происхождение exact private VoN T не установлено. Сведения выше о no-SDK относятся к прежнему датированному шагу; полной передачи и звуковой приёмки по-прежнему нет.

## Текущий gate2026-11-05

Goal tool подтвердил blocked, задача не завершена. [Текущий результат и manual handoff](ASSET_PREPARATION_RU.md). Reported Build successful и successful own Resource.Load сохранены; typed program/private VoN access не квалифицированы. Source11 не доказывает native-cache failure. Текущий own SIG — reviewed manual-save draft; ACP/meta/Source3 сохранены. Полная temporal-цель и критерии приёмки обязательны.
