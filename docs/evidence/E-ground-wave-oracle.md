# Земная волна 20–30 МГц: эталон REQ-ground-wave v0.1 (порт LFMF) совпадает с неизменённой сборкой NTIA

Дата: 2026-10-24. Автор REQ, эталона и корпуса: claude. Статус: **HOST_IMPLEMENTED / DRAFT** — ждёт слепой проверки
codex по `docs/req/REQ-ground-wave.md`, P.368-10 и LFMF v1.1 без авторского эталона. Production-код, L2 и оценщик
не менялись. Это вычислительная приёмка программы, не измерение связи в игре.

## Что сделано

- `docs/req/REQ-ground-wave.md` v0.1: GW01–GW08 — назначение и границы, вход и порядок кодов, метод LFMF без
  изменений, выход (`basicLossDb`, `fieldDbuVm` для 1 кВт, `method`, `kr`, `heightToWavelength`, `provenance`),
  конвенция антенн, условия применимости P.368, приёмка.
- `oracle/ground_wave_oracle.py`: перенос NTIA/ITS LFMF v1.1 на Python stdlib — `wofz` (ACM 680), функции Эйри w1/w1'
  в масштабе Wait, корни `wi_root`, `flat_earth_curve_correction`, `residue_series`, `lfmf`; слой REQ `evaluate` и
  `--check`. Изменения относительно C++ перечислены в шапке файла (перенесены только ветви, которые вызывает LFMF;
  исключения вместо inf/UB; std::round половина от нуля). Таблицы функций Эйри (70 центров разложения Ai/Ai', 15
  коэффициентов асимптотики, корни Ai/Ai') не перепечатаны вручную: их извлекает
  `receipts/ground-wave-airy-tables-claude.py`, режим `--check` сверяет блок в эталоне с исходником (sha256 блока
  `ba6a8dff…`).
- `oracle/build_ground_wave_vectors.py` → `oracle/vectors/ground_wave_v1.json`: 257 случаев (222 OK, 23 отказа по
  GW03 с ручными кодами, 12 JSON-only) и 95 опубликованных примеров NTIA; `oracle/test_ground_wave_oracle.py`
  — 17 тестов.

## Сверка с неизменённой сборкой NTIA

`receipts/ground-wave-port-vs-ntia-claude.py` (результат — `ground-wave-port-vs-ntia-claude.json`) сравнивает
`lfmf()` с exe, собранным из неизменённых исходников LFMF v1.1 (`ground_wave_ntia_build_claude.bat`, driver
`ground_wave_ntia_driver_claude.cpp`, exe sha256 `e3dc0246…f590f8`, печать 9 знаков):

| Набор | Точек | Расхождений rc/метода | Худшее \|Δ\|, дБ |
|---|---|---|---|
| Сетка: f 0.01–30 МГц, d 1 м–10 000 км и порог метода ±1e-9, 7 пар высот, 5 грунтов, H и V, Nₛ 250/400, границы проверок | 16 502 | 0 | 1.15e-9 |
| OK-случаи корпуса (P = 1000 Вт, V) | 222 | 0 | 5.0e-10 |
| Опубликованные примеры NTIA `LFMF_Examples.csv` (1 знак) | 95 | 0 | 0.0475 |

Отклонения на уровне 1e-9 — предел печати exe (5e-10) плюс разный порядок округления комплексных степеней.
NumericFailure и не конечных значений exe на сетке нет. Независимые проверки внутри тестов: `wofz` против
exp(y²)·erfc(y) на мнимой оси и exp(−x²) на вещественной; w1, w1' против собственного ряда Маклорена Ai (DLMF 9.4)
с отн. погрешностью < 1e-6 (табличные значения Ai в LFMF заданы с 7–8 знаками); корни удовлетворяют уравнению.

## Найдено попутно

- **Константа NOTE 1.** В LFMF A + E(1 кВт) − 20·log10 f = 141.98599025116266 дБ, а не 142.0 из P.368 Annex 1
  NOTE 1 (округление; разница 0.014 дБ). В `E-ground-wave-candidates.md` §2 раньше было написано «ровно 142.0» —
  исправлено; на числа сравнения кандидатов это не влияет (receipt хранил сырое A_btl). REQ закрепляет A, а не 142.0.
- **Поведение LFMF вне штатных случаев.** C++ при нулевом поле возвращает SUCCESS с A = +inf, при выходе индекса
  таблицы Эйри — исключение `std::range_error`; REQ переводит оба в `NUMERIC_FAILURE`. На сетке они не возникли.

## Что не доказано

- Физическая достоверность для реальных трасс Arma Reforger: рельеф, смешанные суша/вода, растительность и
  пространственная волна вне REQ. Грунты корпуса — TEST_ONLY.
- Native-реализация: точность функций Эйри и ряда вычетов в binary32 не исследована; v0.1 — только host.
- Конвенция антенн до интеграции (GW06, исправлено по codex #4056): `basicLossDb` — Lb между изотропными
  антеннами (P.341); 4.77 dBi опорного монополя сокращается (при нормированном поле 1 A = FSPL). Усиления реальных
  антенн у земли определяет интеграционный REQ.

## Воспроизведение

```
python -B oracle/ground_wave_oracle.py --check oracle/vectors/ground_wave_v1.json
python -B oracle/build_ground_wave_vectors.py --check
python -B -m unittest discover -s oracle -p test_ground_wave_oracle.py
python -I docs/evidence/receipts/ground-wave-airy-tables-claude.py <LFMF-main> --check oracle/ground_wave_oracle.py
python -B docs/evidence/receipts/ground-wave-port-vs-ntia-claude.py <repo> <ntia exe> oracle/data/lfmf_examples_ntia_v1.csv <out.json>
```
