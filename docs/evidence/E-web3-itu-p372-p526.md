# E-web3 — первоисточники ITU-R P.372 и P.526 (закрытие пробелов REQ-LE)

Дата: 2026-10-19, спринт 10 (тема agora `ricon-reforger-build`, задача C20). Автор: claude. Ревью: codex.
Цель: закрыть открытые вопросы `docs/req/REQ-link-eval.md` (REQ-LE-61/62, раздел 12 п.2) и `E-coverage-matrix.md`
(Q28: L2-дифракция, редакция P.526-16).

## Как получены тексты

- `itu.int` с этого компьютера недоступен: `curl` → «schannel: failed to receive handshake, SSL/TLS connection failed»,
  PowerShell `Invoke-WebRequest` → «Базовое соединение закрыто», WebFetch → «Socket is closed» (2026-10-19).
- Официальные PDF ITU взяты из Internet Archive (копия файла с `itu.int`, без изменений, режим `id_`):

| Документ | Архивный URL | Захват | SHA-256 файла | Объём |
|---|---|---|---|---|
| Rec. ITU-R P.526-16 (11/2025) | `https://web.archive.org/web/20260203052706id_/https://www.itu.int/dms_pubrec/itu-r/rec/p/R-REC-P.526-16-202511-I!!PDF-E.pdf` | 2026-02-03 | `7647c3cb3873dc0389a7007f487728359551ed82468a5677b7cd09430875f23d` | 49 стр. |
| Rec. ITU-R P.372-17 (08/2024) | `https://web.archive.org/web/20260110165631id_/https://www.itu.int/dms_pubrec/itu-r/rec/p/R-REC-P.372-17-202408-I%21%21PDF-E.pdf` | 2026-01-10 | `a4dfc9014475b0656fe4c3e6342c9c4710ab6696e12b6a4a8f33d388630513cb` | 105 стр. |

Тексты стандартов в репозиторий не кладутся (авторское право ITU); ниже — только короткие цитаты по делу.
Уровень доказательств — **STD** (официальный PDF ITU), кроме отмеченного.

## P.526-16: дифракция на общем наземном пути (Q28, REQ-LE L2)

**Редакция.** Обложка: «Recommendation ITU-R P.526-16 (11/2025) … Propagation by diffraction». Сноска на стр. 1:
«Recommendation ITU-R P.526-15 is incorporated by reference in the Radio Regulations, Edition of 2024, Volume 4.»

**Что рекомендовано.** Стр. 1: «recommends that the methods described in Annex 1 should be used for the calculation
of field strengths over diffraction paths…».

**§ 4.5 «Method for a general terrestrial path»** (стр. 27–30):
- «This method is recommended for situations where an automatic process is required to predict diffraction loss for
  any type of path as defined by a profile, whether LoS or trans-horizon, and whether the terrain is rough or smooth.
  This model is based on the Bullington construction, but also makes use of the spherical Earth diffraction model as
  described in § 3.2.»
- «This method should be used when there is no a priori information as to the nature of the propagation path or of
  possible terrain obstructions. This is typical of the case where a computer program is used for profiles selected
  from a terrain height database on a fully automatic basis…»
- Два подметода: «a) the Bullington diffraction method used with a tapered correction…; b) the spherical Earth method
  given in § 3.2.»; Bullington применяется дважды — к реальному профилю (`Lba`) и к сглаженному (`Lbs`).
- Итог, уравнение (66), стр. 30: «The diffraction loss for the general path is now given by:
  `L = Lba + max{Lsph − Lbs, 0}` dB».

**Отсутствие других методов.** В тексте P.526-16 нет ни одного упоминания «Epstein», «Deygout» или «Giovaneli»
(поиск по извлечённому тексту — 0 совпадений).

**Вывод для RICON.**
1. Для L2 (рельеф по профилю из мира Reforger — «computer program … terrain height database … fully automatic») по
   действующей редакции подходит именно § 4.5 — метод на основе Bullington с поправкой на сферическую Землю
   (в литературе — «delta-Bullington»). Вопрос W2:67 «остался ли delta-Bullington в -16» — **закрыт: да**.
2. Метод A3 (Epstein–Peterson, `fn_signalCompute.sqf:96-132`) в действующей P.526 отсутствует — подтверждает решение
   R-core P2: только legacy-comparator, не реализация.
3. Для реализации нужны ещё § 3.2 (сферическая Земля) и § 4.5.1 (Bullington) того же документа — они есть в
   полученном PDF; перенос в REQ — отдельная задача Э4 (своими словами, с номерами уравнений).

## P.372-17: внешний шум и коэффициент шума системы (REQ-LE-61/62)

**Редакция.** Обложка: «Recommendation ITU-R P.372-17 (08/2024) … Radio noise».

**Коэффициент шума системы** (стр. 4): после определения `fc`, `ft` (уравнения (3), (4)): «If Tc = Tt = T0,
equation (1) becomes `f = fa − 1 + fc ft fr` (5)». При антенне и линии без потерь (`fc = ft = 1`) это
`f = fa + fr − 1` — **совпадает с выводом REQ-LE-61** (сделанным из определений); теперь есть и прямая STD-ссылка.
Там же (стр. 5): «`Pn = Fa + B − 204` dBW (6)».

**Техногенный шум** (§ 6.1.1, стр. 99–100):
- «In all cases results are consistent with a linear variation of the median value, Fam, expressed in dB, with
  frequency f of the form: `Fam = c − d log f` (17). With f expressed in MHz, c and d take the values given in
  Table 1.»
- **Table 1 «Values of the constants c and d»**: City (curve A) 76.8 / 27.7; Residential (curve B) 72.5 / 27.7;
  Rural (curve C) 67.2 / 27.7; Quiet rural (curve D) 53.6 / 28.6; Galactic noise (curve E) 52.0 / 23.0.
  **Совпадает** со значениями REQ-LE-62 (ранее — только через отчёт FFI, REF).
- Область: «Note that equation (17) is valid in the range 0.3 to 250 MHz for all the environmental categories
  **except those of curves D and E** as indicated on the Figure.» Далее: кривая D (quiet rural) связана с шумом,
  отражённым ионосферой ниже `fxF2`; выше неё доминирует галактический шум (кривая E).

**Вывод для RICON.**
1. REQ-LE-61 и константы REQ-LE-62 подтверждены первоисточником P.372-17 (STD).
2. **Новое уточнение к REQ-LE-63:** область 0.3–250 МГц действует для City/Residential/Rural, но **не для Quiet rural
   (D) и Galactic (E)** — их границы задаются рисунком (Fig. 39), а не единым диапазоном. Сейчас REQ-LE-63 и код
   (`RICON_ManMadeNoiseModel`, `MIN/MAX_FREQUENCY_MHZ` = 0.3/250 для всех категорий) применяют 0.3–250 МГц и к
   quiet rural. Предложение: для `quiet_rural` область — `OPEN` до чтения Fig. 39, а в конфиге среды (спринт
   менеджера) область задаётся на категорию, а не одной константой.
3. **Редакция.** Существует более новая **P.372-18 (09/2026)** (результат поиска на `itu.int`, PDF
   `R-REC-P.372-18-202609-I!!PDF-E.pdf`); в Internet Archive её нет (CDX — пусто, сервис временами недоступен),
   прочитать не удалось. Изменились ли константы в -18 — **не проверено** (OPEN). Пока действует правило REQ-LE-62:
   константы — в версионированном конфиге с указанием источника `P.372-17`.

## Итог по пробелам

| Пробел | Было | Стало |
|---|---|---|
| Q28 / L2: метод для общего пути в P.526-16 | OPEN (подтверждено только для -15) | **CLOSED**: § 4.5, ур. (66), Bullington + сферическая Земля; Epstein–Peterson в -16 нет |
| REQ-LE-61: формула коэффициента шума системы | выведена из определений; цитаты не было | **CLOSED**: P.372-17 ур. (5) |
| REQ-LE-62: константы c/d | REF (через FFI) | **STD** (P.372-17 Table 1) |
| REQ-LE-63: область 0.3–250 МГц | для всех категорий | **уточнить**: не для quiet rural/galactic (P.372-17 § 6.1.1) |
| Действующая редакция P.372 | не известна | -17 прочитана; **-18 (09/2026) существует, не прочитана — OPEN** |
