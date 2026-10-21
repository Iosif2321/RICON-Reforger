# R27: чтение von.acp через BaseContainerTools — INCOMPLETE

Дата: 2026-10-21. Автор: claude. Статус: OWN_CHECKED, ревью codex ожидается. Уровень: WORKBENCH_STATIC.
Основание: owner в чате «установи Audio Editor»; owner agora #2020 (ACP через Workbench); механизм согласован
codex #3143 после правок #3111; граница опыта — корень и один узел, затем стоп (#3112).

## Audio Editor

Отдельно не устанавливается: это встроенный модуль Workbench (`AudioEditor` в
`EnfusionScriptAPI/html/group__WorkbenchAPI__Modules.html`). Его GUI в этой работе не запускался.

## Гипотеза и механизм

R26 (`E-audio-api.md`) открывал ресурс по пути `$ArmaReforger:Sounds/VON/von.acp` и не получил корневой
контейнер. Персонаж ссылается на тот же ресурс по GUID: `{B4C3941EC8B2AD20}Sounds/VON/von.acp`
(`Character_Base.et:380`, лог WbReader rd14). Гипотеза: R26 не сработал из-за формы имени.

Пробник `tools/RICON_WbReader/Scripts/WorkbenchGame/RICON_AcpGraphReaderPlugin.c` (коммиты `07f7a62`, `b4f45e8`):
`BaseContainerTools.LoadContainer(GUID)` → `GetResource().ToBaseContainer()`. Только чтение, без Save и Set. Каждая
строка вывода экранируется и обрезается, всего не больше 15 строк. Gate `run_acp.ps1` устроен как в R29.

## Исполнение

База: `%USERPROFILE%/Documents/My Games/ArmaReforgerWorkbench/logs/`.

| Запуск | Папка | Итог | SHA-256 `console.log` |
|---|---|---|---|
| компиляция без флага чтения | `ricon-acp27-compile-20261021-121123` | exit 6 BLOCKED, SCRIPT(E) 0 | `ddc166e43f992c493c98934945c1494f34ae7363a0429cdb836e61c070bff744` |
| самопроверка форматтера | `ricon-acp27-selftest-20261021-180704` | exit 0, PASS, ресурс не загружался | `f4d3e4974d5e99ebd22c69c56c929245074d379af49d1dfcce1c5ad2f5c42d53` |
| окно 1: корень | `ricon-acp27-root-20261021-181532` | exit 4, INCOMPLETE | `887519e5fc1382d20e1b96ac1e828351ab40790459fce4201c1e32829c64659a` |

В окне 1 сразу после вызова движок пишет `:145` `SCRIPT (W): Can't open config file "Sounds/VON/von.acp"`, затем
`:146` `[RICON_ACP27] INCOMPLETE: ресурс не загружен`.

## Вывод и граница

- Гипотеза об имени опровергнута: по GUID ресурс тоже не открывается как config-контейнер.
- Скриптом граф не получен ни одним из трёх способов: FileIO (rd22, `E-radio-data-wb.md`), модуль AudioEditor (R26),
  BaseContainerTools (R27).
- Это **не** доказывает отсутствие графа, узлов или влияния `TransmissionQuality`. Формат файла не установлен.
- По #3112 второе окно не использовано: корень не открылся. S2 от этого результата не зависит.
- Остаётся ручной просмотр в GUI Audio Editor владельцем (`WB-order-link-quality.md`, раздел «Сначала Audio Editor»).
