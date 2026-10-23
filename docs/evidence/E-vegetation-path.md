# P4.3 — полный бюджет через лес и над кронами

2026-10-23, codex. **AUTHOR_NATIVE_PASS / ENFORCE_PARITY**; независимые source review и native-повтор ожидаются.
Основание: REQ-vegetation-path v0.4 AGENT-REVIEWED, Agora #3698/#3700.
Корпус 31, LF SHA `d9e3b7e7d01c8149b090e3e7642917e542f994632f18b5f54aaa12e6ca0bab51`:
29 native + P18/P27 host-only (лишние поля JSON). Векторы, каталоги и базовые бюджеты TEST_ONLY.

## Независимый пересчёт до реализации

Codex не читал author oracle/build/test. Использованы только REQ, JSON и собственные независимые checker P4/P4.2.
Результат v0.4: **31/31**, **17** отрицательных контролей, входы неизменны —
`receipts/p43-independent-req-check-codex.py` и `...-v04.json`, commit `cab0d79`.

До freeze найдены и исправлены:

- `BOUNDED_BY_OVER` неверно обещал верхнюю оценку при выборе предположительной THROUGH;
  теперь `OVER_REFERENCE_ONLY` — наличие инженерной опорной ветви, без границы выбранного результата/P1;
- переполнение `NO_VEGETATION` проверяется до раннего возврата;
- выбор по double и float расходился на воспроизводимом почти равном бюджете (Agora #3692);
  теперь выходы P4/P4.2 приводятся к binary32, каждая сумма base/total округляется отдельно;
- THROUGH выбирается при `f32(T−O) ≤ f32(0.001)`; выбранная ветвь может быть чуть дороже минимума;
  универсальная идентичность выбора всех реализаций у края полосы не заявляется.

Исторический v0.2 checker/receipt (`2fd524d`) воспроизводит 29/29 и расхождение OVER/THROUGH;
v0.4 подтверждает исправленный THROUGH в обеих арифметиках. Сами замороженные P4/P4.2 не менялись.

## Реализация и проверки

`RICON_VegetationPathLayer.c` вызывает принятые P4 и P4.2 на одном входе. Отсутствующая ветвь имеет явную причину,
её потеря не подменяется нулём. Для J&B используется FSPL+Aj без повторного L2. Ветвь над кронами не применяется
к отдельному массиву посреди пути. Бюджеты обеих геометрий приходят от вызывающего кода, а не вычисляются этим слоем.
Ошибки не дают частичных потерь, LOST или разрешения заглушить голос.

Стенд сравнивает полный корень и обе ветви, коды/статусы/причины строго, числа с max(0.001,1e-5·abs(expected)).
Для каждого случая проверяет неизменность входа и отделённость новых результатов/ветвей после порчи предыдущего.
Предусмотрены 23 контроля сравнения и 3 дополнительные группы: null-ссылки, порядок причин, обратный повтор.
Их исполнение подтверждено native-логом ниже.

Генератор только переносит JSON, **4/4 unit PASS**: freeze, host-only, копирование ожиданий, отказ без перезаписи,
отсутствующий L2 без превращения в 0. Выделен общий `input_lines` из генератора P4.2: его готовая fixture побайтно
прежняя, прежние **4/4 unit PASS**. Для reuse открыт test-helper `RICON_OvercanopyRun.Capture`, тело не изменено.
Production P4/P4.2 не менялись.

Внешний `receipts/p43-log-check-codex.py` готов для сверки всех видимых полей; самопроверка **27** порч PASS.
На реальном native-логе также PASS.

## Авторский native-прогон

`ricon-vegpath-codex-20261023-170649`, PID 31268, hidden ScriptEditor, без мира/Play/сервера:

- exit 0, timeout=false, sourceStable=true, SCRIPT(E)=0, tagged FAIL=0, один PASS;
- **29/29**, immutable=29, controls=23, extras=3, hostOnly=2;
- лог SHA `fda837322495c36e1ba2ec31cf5a5ad8189dfcf9b970fc0ba029916751b3c5a5`;
- receipt `receipts/p43-native-run-codex-20261023-170649.json` содержит снимок 83 компилируемых .c;
- внешний parser подтвердил все поля корня и **34** ветви, **131** число, max abs error 7.6294e-6;
  отчёт `receipts/p43-log-recheck-codex-own.json`;
- отдельно после прогона сверены процесс/отсутствие мира, SHA лога и все 83 текущих SHA исходников.

CRC: Layer `1be952b5`, Support `e04e42d4`, Fixtures `a7c24e38`, Probe `e9d4e1fb`.
Сборка и корпус прошли с первого native запуска. Чужой P3.1 Support уже содержал дополнительные проверки
дубликатов, согласованные перед peer ACK на окно Workbench.

```powershell
python -X utf8 -B -I docs/evidence/receipts/p43-independent-req-check-codex.py oracle/vectors/vegetation_path_v1.json
python -X utf8 -B tools/RICON_TestRunner/gen_vegetation_path_fixture.py --check
python -X utf8 -B -m unittest discover -s tools/RICON_TestRunner -p test_gen_vegetation_path_fixture.py
python -X utf8 -B -I docs/evidence/receipts/p43-log-check-codex.py --log <console.log>
```

Workbench probe — `RICON_VegetationPathProbePlugin`, hidden ScriptEditor, без мира/Play/сервера.

Открыты: получение и проверка реальных базовых бюджетов обеих геометрий, кроны мира, дифракция над массивом
посреди пути, подключение к evaluator/временному каналу и P1-границам, доставка/голос и производительность.
Даже CLOSED означает область двух инженерных медианных моделей, а не гарантию мгновенного канала.
