# P4.2 — чистый расчёт поправки терминала под кронами

2026-10-23, codex. **AGENT-REVIEWED / NATIVE_PARITY_PASS**, независимое ревью и повтор Claude пройдены.
Ни мир, ни Play, ни игровой сервер не запускались. Срез не закрывает полный P4 и слышимость в игре.

## Контракт и независимый пересчёт

Основание: `REQ-vegetation-overcanopy.md` v0.1 AGENT-REVIEWED, freeze `5218593`, Agora #3648/#3650.
Корпус `oracle/vectors/vegetation_overcanopy_v1.json`: LF SHA-256
`00d7da2115f5d27445ae40502ef6fa3541e027e85a4275b7ba80264664a99cf2`, **31 случай**.
O28 (лишнее поле корня JSON) — host-only; остальные **30** перенесены в типизированный стенд.

До чтения эталона автора codex написал независимый checker только по REQ, JSON и P.2108-1:
`receipts/p42-independent-req-check-codex.py/.json`, commit `1f4d4e2`.
Результат: **31/31**, **20** контролей искажения результата, **50** сравнений монотонности, **7** проверок
приоритета ошибок. J вычислен через эквивалентную формулу asinh; эталон автора не импортируется.
Формулы (1), (2a), (2c)–(2g) и таблицы 1–3 сверены по официальному ITU PDF и локальному рендеру страниц 4–5.
SHA локального PDF: `69c4138db26e76ac1c35fabdc99119ee3154e0328c23b64e8d931690a67693cb`.

По review codex до freeze уточнено:

- будущая опорная высота терминала в лесу — max(h, R); антенна выше крон не опускается до R;
- при h ≥ R все диагностические поля нулевые; jDb=0 означает неприменение формулы, а не J(0);
- округлённые константы стандарта оставлены: предел J(0)−6.03 ≈ +0.002852 дБ, точная непрерывность у R не заявляется.

## Код и стенд

`RICON_OvercanopyLayer.c`:

- DTO входа наследует существующий VegetationInput; проверка RF-профиля и конечности переиспользуется из P4;
- глобальная проверка каталога и крон, известность/полнота наблюдения, объединение отрезков и причины отказов
  следуют OC05–OC07; UNKNOWN и NUMERIC не превращаются в LOST;
- поправка вычисляется отдельно для TX/RX у концов пути; M1–M5 и P1-границы не вызываются;
- выход содержит новые записи; частичный результат при ошибке не публикуется;
- `integrationClosed=false`; это медианная поправка к будущему полному бюджету над кронами.

`RICON_OvercanopyFixtureSupport.c` сравнивает все поля результата, геометрию строго и расчётные числа с допуском
max(0.001, 1e-5·abs(expected)). Для каждого из 30 случаев проверяет глубокую неизменность входа, повтор после
порчи результата и отсутствие разделяемого массива результатов. **20** контрольных порч охватывают корень и все
поля терминала. **5** дополнительных групп: null, неиспользуемые записи/дубликаты, приоритет причин,
подъём антенны, обратный порядок всех случаев.

Генератор `gen_overcanopy_fixture.py` только копирует frozen ожидания, использует существующие helpers
наблюдения и профиля P4. **4/4 unit-теста PASS**: freeze, отказ без перезаписи, перенос ожиданий, bool вместо числа.
Единственная правка прежнего стенда — `RICON_VegetationRun.Capture` стал public, тело не изменено.
Production-код P4.1 не менялся.
Общие runners дополнены проверкой fixture, независимым checker и Workbench probe.
Итог `python -X utf8 -B tools/RICON_TestRunner/regress_host.py .` после CT10.1 и P4.2: **46/46 PASS**,
включая все oracle/tool unit-тесты; receipt `receipts/p42-ct10-host-codex-20261023.txt`.

## Исполнение

Авторский запуск P4.2: `ricon-overcanopy-codex-20261023-163040`, PID 19652:

- exit 0, timeout=false, sourceStable=true, SCRIPT(E)=0, один done: PASS, tagged FAIL=0;
- **30/30**, immutable=30, controls=20, extras=5, hostOnly=1;
- SHA-256 лога `a54f263413877d9b5214c35c5fc41b1e27c62023369da59accd9d7d8a5b28053`;
- полный снимок SHA всех компилируемых исходников, параметры и путь лога —
  `receipts/p42-native-run-codex-20261023-163040.json`.

Внешний `receipts/p42-log-check-codex.py` повторно проверил **30 кодов**, **14 терминалов**, **156 напечатанных
чисел**; наибольшая абсолютная ошибка 2.5363e-6. Самопроверка parser: **27** отрицательных контролей.
Результат — `receipts/p42-log-recheck-codex-own.json`. Parser сверяет содержимое лога; процесс и неизменность
исходников подтверждает отдельный native receipt.

После открытия Capture повтор P4.1 `ricon-vegetation-codex-20261023-163113`: exit 0, stable, SCRIPT(E)=0,
один PASS; **58/58**, controls=13, extras=4. Лог SHA
`c620d7b49d2ec44af1661c1031677cbc971a685fad3aad2af3bef8c6160e2720`.
Независимый parser Claude подтвердил 58 случаев и 202 напечатанных числа.

Независимый повтор Claude: `ricon-overcanopy-claude-20261023-163456`, exit 0, SCRIPT(E)=0, один PASS,
30/30, immutable=30, controls=20, extras=5. SHA-256 лога
`0efad8a1d1a7b8a9f4ac3c28afe5e379905a4a732b9c8f9cb5e71015697507c9`.
Ревью исходника принято в Agora #3664; parser Claude — commit `91aae7a`.
Codex проверил SHA, тождество всех **61** строк RICON_CANOPY*, повторил оба parser на peer-логе:
156 чисел у codex = 138 активных чисел у Claude + 18 нулевых итогов отказов. Оба PASS.
CRC Layer `7b0ff36e`, Support `94012f1f`, Fixtures `bb44c9db`, Probe `3361ba71` совпали у обеих сторон.
Исходники P4.2 сохранились; единственное последующее изменение общего снимка — чужой test-helper CT10.1,
исправленный по отдельному review. Это зафиксировано в `receipts/p42-peer-review-codex-20261023.json`.

Сохранены неудачные попытки:

- до готовности P4.2 generated fixture оказалась в addons и сорвала чужой compile 162406;
  codex удалил её до повторного запуска Claude. Это ошибка порядка подготовки, не успех теста;
- собственный sandbox запуск 163014 завершился −1 до создания штатного каталога лога; fallback error.log
  сообщает `ResourceDB: could not open cache file`. Он не учитывается в native PASS. Fallback console/error/script
  сохранены в receipts с префиксом `p42-sandbox-startup-163014-`. Разрешённый hidden WB повтор выполнен вне sandbox.

## Воспроизведение и границы

```powershell
python -X utf8 -B -I docs/evidence/receipts/p42-independent-req-check-codex.py oracle/vectors/vegetation_overcanopy_v1.json
python -X utf8 -B tools/RICON_TestRunner/gen_overcanopy_fixture.py --check
python -X utf8 -B -m unittest discover -s tools/RICON_TestRunner -p test_gen_overcanopy_fixture.py
python -X utf8 -B -I docs/evidence/receipts/p42-log-check-codex.py --log <console.log>
```

Workbench plugin — `RICON_OvercanopyProbePlugin`, ScriptEditor, hidden, без мира/Play/сервера.

Открыты: реальные высоты крон/распознавание мира, полные L1/L2 бюджеты обеих геометрий и выбор пути,
дифракция над массивом S3/S3+, связь с временным каналом и доказанными границами отсечки, голосовой актюатор,
доставка/игровое исполнение и производительность. Стандартный R=15 допускается только явной записью
STANDARD_DEFAULT; production-каталог этим срезом не включается.
