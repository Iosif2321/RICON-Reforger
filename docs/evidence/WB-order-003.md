# Заказ `[Workbench]` №003 — компиляция `RICON_Core` и `RICON_CoreTests`

Дата: 2026-10-19. Заказчики: claude + codex (тема agora `ricon-reforger-build`, спринт 8).
Исполнитель: ассистент владельца в Workbench. Результат — в `docs/evidence/E-workbench-source.md`, раздел
«Компиляция №003».

**Заменяет заказ №002**: с его ревизии код изменился (маршруты, реестр типов сетей, seam антенны, новый тестовый
аддон). Если №002 уже выполнен — его результат всё равно нужен (он покажет ошибки старой ревизии), но
актуальна проверка по этому заказу. Пункты №001 про ванильные файлы остаются в силе.

## Правила

Те же, что в `WB-order-002.md`: только компиляция скриптов; без Play, без сервера, без запуска мира, без
распаковщиков `.pak`; код не исправлять; агенты проверку сами не запускают.

## Что компилировать

- Репозиторий: `C:\Users\Ded\Documents\Projects\ARMA 3 Scripts\RICON-Reforger`.
- Ревизия: хэш HEAD указан в вопросе `[Workbench]` №003 в шине. Перед проверкой: `git log --oneline -1` и
  `git status --short` (должен быть пуст) — оба вывода в отчёт.
- Два проекта, по очереди:
  1. `addons\RICON_Core\addon.gproj` — ядро мода.
  2. `addons\RICON_CoreTests\addon.gproj` — тестовый аддон (TEST_ONLY, зависит от `RICON_Core`; в Workshop не
     публикуется). Если Workbench не находит зависимость `RICON_Core`, добавить `-addonsDir` с папкой
     `RICON-Reforger\addons` и указать это в отчёте.

## Как

Как в `WB-order-002.md` и `E-workbench-build.md`, для каждого проекта:

```bat
"C:\Program Files (x86)\Steam\steamapps\common\Arma Reforger Tools\Workbench\ArmaReforgerWorkbenchSteamDiag.exe" ^
  -gproj "<repo>\addons\<проект>\addon.gproj" ^
  -wbModule=ScriptEditor -validate -wbSilent -noThrow ^
  -logLevel warning -logsDir ricon-validate-003-<проект> -log-scr-checksum
echo exit=%ERRORLEVEL%
```

Если CLI не работает как описано, — компиляция из Script Editor; указать, какой способ сработал.

## Что прислать (для каждого проекта)

1. Версии игры и Workbench, дата.
2. Ревизия и `git status`.
3. Точная команда или пункт меню.
4. Код возврата.
5. **Дословно все ошибки и предупреждения** компиляции с файлами и строками.
6. Список файлов `Scripts/Game/RICON/**` и `Scripts/Game/RICON_Tests/**` из `-log-scr-checksum`.

## Как агенты используют результат

- Ядро: код 0, нет ошибок, файлы в списке → `WORKBENCH_COMPILE_PASS` для `RICON_Core` на этой ревизии.
- Тестовый аддон: то же → фикстуры EXT-1…5 получают статус `COMPILE_FIXTURE_PASS` (расширение собирается на
  публичных контрактах ядра). `SelfCheck()` при этом не выполняется — `CONTRACT_PASS` остаётся `NOT_TESTED`.
- Любая ошибка → исправление в следующем спринте и заказ №004.
