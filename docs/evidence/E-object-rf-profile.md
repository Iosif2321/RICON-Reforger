# P3.1 — авторский контракт определения RF-профиля

2026-10-23, codex. **AGENT-REVIEWED / BLIND_CHECK_PASS**; native ещё ожидается.

REQ-object-rf-profile v0.1 задаёт чистый resolver явных назначений и проверенных правил prefab/class.
Графы наследования валидируются целиком: отсутствующий родитель или неиспользуемый цикл не скрываются
правильным компонентом. Приоритет — компонент, поверхность, ближайшее правило prefab, ближайшее правило класса,
подсказка/UNKNOWN. Неизвестное выбранное назначение даёт UNRESOLVED_PROFILE, без fallback.
knowledge/materialId совместимы с P2. Реальный адаптер и радиофизика этим срезом не реализованы.

Авторские файлы: `oracle/object_rf_profile_oracle.py`, `build_object_rf_profile_vectors.py`,
`test_object_rf_profile_oracle.py`, `vectors/object_rf_profile_v1.json`.

Корпус **47**, LF SHA-256 `b0b93be3238b49c223c859ac14e22f045ae708ce03f1cae2ed731eaf0531e4c5`.
Host-only **R31, R32, R33, R34, R35, R36, R41** — ошибки JSON-полей или типов; остальные **40** выразимы
в типизированном стенде, включая null-ссылки. Значения synthetic TEST_ONLY, профили не включены в production.

Результат: CLI **47/47**, builder --check и **9/9 unit PASS**.
Отдельные тесты проверяют неизвестное назначение без fallback, подсказку без материала, ближайшего предка,
неиспользуемый плохой граф, детерминизм при перестановке map/реестра, неизменность входов и отделённые результаты.
Цепочка из **1500** узлов подтверждает завершение без Python recursion.
Корпус построен авторским эталоном; 47/47 не является независимой приёмкой.

Независимый checker Claude (`receipts/p3-1-independent-req-check.py/.json`, `7f53c91`) создан только по REQ/JSON:
47/47, все 4 кода и 6 source, неизменность всех входов, 5/5 дополнительных дефектов конфигурации отвергнуты;
цепь 5000 узлов разрешена без рекурсии. Codex прочитал checker и повторил его CLI: PASS.
Freeze корпуса принят без изменения его байтов. Для native согласован Node DTO с hasParent: R45 остаётся
различим от null-корня, дубликат ID не пропадает при построении map.

```powershell
python -X utf8 -B oracle/build_object_rf_profile_vectors.py --check
python -X utf8 -B oracle/object_rf_profile_oracle.py --check oracle/vectors/object_rf_profile_v1.json
python -X utf8 -B -m unittest discover -s oracle -p test_object_rf_profile_oracle.py
```

Перед Enforce Claude должен пересчитать только REQ и JSON собственным checker. Код движка/мира не менялся.
Открыты: подтверждённое отображение ResourceName/материалов/классов движка на логические ID, загрузчик с отказом
на дубликатах, реальные правила и RF-профили, геометрия стен/проёмов, составные конструкции, расчёт по P.2040
в области конкретного материала, пределы численной отсечки, интеграция, производительность и звук.
