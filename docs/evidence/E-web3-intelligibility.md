# E-web3: можно ли получить production-кривую «SINR → разборчивость речи»

Дата проверки: 2026-10-19. Автор: codex. Режим: read-only web/metadata research.

## Краткий результат

**Не найден первоисточник, который обосновывает универсальную скалярную функцию**
`radio SINR (dB) → intelligibility Q ∈ [0,1]` независимо от спектра речи/шума, модуляции, кодека, обработки и
речевого материала.

- ANSI SII использует акустические спектры речи/шума и вклад частотных полос, а не один global SINR.
- IEC STI — отдельный индекс передачи речи, не синоним radio SINR.
- STOI — intrusive metric чистого и деградированного **waveform**; scalar SINR ему недостаточен.
- PESQ/POLQA/P.563 относятся к objective speech **quality** и также не дают подтверждённую здесь функцию
  `SINR → speech intelligibility`.
- Эксперимент Miller–Nicely подтверждает, что тип искажений меняет характер ошибок речи; одна SNR-ось не описывает
  все условия.

Следствие: `REQ-LE-74/75 = BLOCKED_CALIBRATION` остаётся верным. `linear_test` остаётся только TEST_ONLY.

## Источники и доступ

| Источник | Доступ 2026-10-19 | Уровень | Что подтверждено |
|---|---|---|---|
| ANSI/ASA S3.5-1997, SII, `https://webstore.ansi.org/standards/asa/ansiasas1997r2020` | ANSI webstore → HTTP 403; полный standard не прочитан | `STD_METADATA / PAYWALLED` | полная нормативная формула/актуальная редакция не подтверждены |
| Allen, JASA 2018, «SII—Speech intelligibility index standard: ANSI S3.5 1997», `https://doi.org/10.1121/1.5036206` | Crossref metadata + abstract доступны | `PRIMARY_ABSTRACT` (доклад рабочей группы, не сам standard) | SII считается из acoustic speech/noise measurements; framework включает speech/noise spectra и band transmission |
| BS/IEC 60268-16:2020, `https://doi.org/10.3403/30355599` | IEC webstore: соединение закрыто; Crossref/BSI metadata доступны | `STD_METADATA / PAYWALLED` | название: objective rating через Speech Transmission Index; алгоритм не прочитан |
| Taal et al., IEEE TASLP 2011, `https://doi.org/10.1109/TASL.2011.2114881` | IEEE PDF → HTTP 418; Crossref metadata доступны | `PRIMARY_METADATA / PAYWALLED` | первичная STOI-работа идентифицирована |
| Авторская страница Cees Taal, `https://ceestaal.nl/code/` | HTTP 200, final URL тот же; snapshot HTML SHA-256 `2E2F82531AC779139DB58788F9CE23A172C726611E493DBCEC6F28DF62E3CA67` | `PRIMARY_AUTHOR_DOC` | STOI описан как intrusive function clean+degraded speech; перечислены noise reduction/masking/vocoded conditions |
| Miller & Nicely, JASA 1955, `https://doi.org/10.1121/1.1907526` | publisher PDF → HTTP 403; Crossref abstract доступен | `PRIMARY_ABSTRACT` | 16 consonants тестировались при frequency distortion и random masking noise; ошибки зависели от типа искажения |
| ITU-T P.862/P.863/P.563/P.800 official catalog URLs | `itu.int`: PowerShell connection closed; curl Schannel TLS handshake failed | `OFFICIAL_UNAVAILABLE` | названия/методы нельзя повысить до проверенного full-text evidence |
| TIA TSB-88.1-F (2022), `https://tiaonline.org/standardannouncement/tia-issues-new-wireless-communications-systems/` | official TIA announcement HTTP 200; full standard платный | `STD_METADATA / PAYWALLED` | technology-independent narrowband performance modeling, < 1 GHz; публичных DAQ/C/(I+N) таблиц нет |
| TIA-603-E (2016), `https://tiaonline.org/standard/tia-603/` | official TIA page HTTP 200; full standard платный | `STD_METADATA / PAYWALLED` | measurement/performance для land-mobile FM/PM voice/data ≤ 1 GHz; публичный abstract не содержит 12 dB SINAD |

Короткие выдержки (каждая меньше 25 слов на источник):

- SII abstract: “calculated from acoustical measurements of speech and noise”.
- STOI author page: “The STOI-measure is intrusive, i.e., a function of the clean and degraded speech signals.”
- Miller–Nicely abstract: “frequency distortion and ... random masking noise”.

## 1. ANSI SII

Доступный abstract доклада участника working group называет SII физической мерой, коррелирующей с
разборчивостью. Его вход — acoustic measurements speech/noise; framework включает:

- dynamic speech and noise spectra;
- information transmission по разным frequency bands;
- hearing threshold и band weighting.

Это совместимо с идеей вычислять метрику из детального спектра/полос, но **не обосновывает** замену всей процедуры
одним wideband SINR и одной универсальной logistic/linear curve. Полный ANSI S3.5 недоступен, поэтому exact
calculation, transfer functions для речевого материала и актуальная редакция остаются OPEN.

## 2. IEC STI

Публичная библиография BS/IEC 60268-16:2020 подтверждает назначение: objective rating speech intelligibility through
Speech Transmission Index. Полный текст/формулы не доступны в этой сессии. STI нельзя автоматически считать
функцией radio SINR: название и доступные metadata этого не утверждают.

## 3. STOI

Первичная STOI-публикация идентифицирована по DOI. Авторская документация прямо говорит:

- metric intrusive;
- использует clean и degraded speech signals;
- предназначена для degraded/noisy/processed speech, включая nonlinear processing.

RICON server LinkEvaluator не имеет clean/degraded waveform: он знает link budget/SINR. Поэтому STOI — кандидат
для offline audio calibration/evaluation, но не drop-in `IntelligibilityModel(float sinrDb)`.

## 4. PESQ, POLQA, P.563 и subjective methods

Official ITU catalog/full text не загрузился; следовательно, никакие численные mapping из этих рекомендаций здесь
не подтверждены. По их известным catalog titles они оценивают speech quality/listening quality, а не объявляют
универсальную радиоканальную SINR-кривую разборчивости. До чтения full text это остаётся scope-level observation,
не normative evidence.

Quality MOS-LQO и intelligibility/word accuracy — разные target variables. Их нельзя смешивать без отдельного
эмпирического mapping.

## 5. Эмпирические данные не универсальны

Miller–Nicely — первичный listening experiment для consonants при конкретной полосе/фильтрации/маскирующем шуме.
Abstract показывает: паттерны ошибок менялись между noise, low-pass и high-pass. Это доказательство зависимости от
условия, но не готовая кривая для VoN/радио Reforger.

Даже одинаковый scalar SINR может дать разную разборчивость при:

- другом noise spectrum/non-stationarity;
- полосе и modulation/codecs;
- packet loss/time clipping/first-word loss;
- фильтрах/AGC/squelch/engine VoN graph;
- speech material, языке и слушателе.

## 6. Решение для RICON

### 6.0 Land-mobile DAQ/SINAD как кандидаты, не доказанные defaults

Серия TIA TSB-88 наиболее близка предметной области RICON: публичная TIA-страница подтверждает narrowband
performance modeling в noise/interference-limited land-mobile systems ниже 1 ГГц. Однако полные документы платные;
в публичном official metadata не найдены:

- определения шкалы Delivered Audio Quality (DAQ);
- конкретные словесные границы DAQ 1–5;
- таблицы CPC / required `C/(I+N)` для modulation types.

Поэтому DAQ MAY стать nomenclature/target для будущих состояний после законного доступа к TSB-88 и calibration, но
сейчас нельзя привязывать CLEAN/DEGRADED/WEAK к числам DAQ «по памяти».

Аналогично official page TIA-603 подтверждает область land-mobile FM/PM и measurement standards, но публичный abstract
не подтверждает 12 dB SINAD sensitivity criterion. ETSI EN 300 086 PDF в этой сессии не загрузился. Значение 12 dB
SINAD остаётся `OPEN / UNVERIFIED`, не default RICON.

### 6.1 Что можно реализовывать сейчас

- Сохранять физический `SINR` как измеримый output.
- Сохранять `IntelligibilityModel` seam и versioned config.
- Использовать `linear_test` только для contract/oracle/compile fixtures.
- В UI/логах до калибровки называть Q **test link-quality fixture**, не доказанной разборчивостью речи.

### 6.2 Что запрещено выдавать за production

- A3 logistic curve и thresholds 0.85/0.4.
- Новую линейную/logistic функцию, выбранную только «по правдоподобию».
- PESQ/POLQA/STOI score, вычисленный без требуемого audio input.
- Один порог SINR как универсальную слышимость всех modulation/device/network types.

### 6.3 Минимальная calibration/eval программа

Когда runtime/audio будет разрешён:

1. Зафиксировать Reforger/VoN graph, device/modulation, bandwidth, codec и noise condition.
2. Сгенерировать blinded speech material с несколькими speakers/listeners; измерять keyword/sentence accuracy, а не
   только субъективное «качество».
3. Sweep физического SINR и engine `TransmissionQuality`; сохранять audio/expected/observed с correlation ID.
4. MAY вычислять SII/STI/STOI/POLQA только при наличии их требуемых inputs и законного инструмента; хранить как
   отдельные metrics.
5. Fit monotonic mapping на train set, thresholds CLEAN/DEGRADED/WEAK — по заранее определённым accuracy bands;
   проверить на held-out speakers/material/noise.
6. Версионировать curve вместе с VoN/game/config; повторять после обновления Reforger.
7. После доступа к TSB-88 MAY сопоставить измеренные bands с DAQ, но не подменять empirical word accuracy одной
   нормативной меткой.

До этого production model = `BLOCKED_CALIBRATION`; решение владельца может определить gameplay-quality curve, но она
должна маркироваться как design policy, не физический стандарт.

## 7. Fetch log

- `https://www.itu.int/rec/T-REC-P.863/en`, P.862, P.563, P.800: соединение закрыто; curl `schannel` TLS failure.
- ANSI webstore S3.5: HTTP 403.
- IEC webstore 60268-16: connection unexpectedly closed.
- IEEE Xplore STOI PDF: HTTP 418.
- AIP/ASA PDFs/страницы: HTTP 403.
- TIA official pages TSB-88.1-F и TIA-603-E: HTTP 200; full standards paywalled; DAQ/12 dB SINAD text публично не найден.
- ETSI EN 300 086 official deliver URL: соединение закрыто.
- Wayback CDX по official PDF patterns ITU-T P.862/P.863/P.563: пустые результаты; `STD_COPY` получить не удалось.
- Crossref API: HTTP 200 для DOI metadata/abstract, но это не заменяет full standards.
- Cees Taal author page: HTTP 200.

## Итог для gates

| Gate | Статус |
|---|---|
| Прямая universal `SINR→Q` из primary standard | **НЕ НАЙДЕНО** |
| `linear_test` как test fixture | допустимо, `TEST_ONLY` |
| Production intelligibility/thresholds | `BLOCKED_CALIBRATION` |
| SII/STI/STOI как будущие offline metrics | PARTIAL: нужны full methods и требуемые inputs |
| TSB-88 DAQ как шкала land-mobile состояний | CANDIDATE, full standard + calibration required |
| TIA/ETSI 12 dB SINAD как receiver threshold | OPEN / UNVERIFIED |
| Изменение текущего Enforce production curve | нет: production curve всё ещё отсутствует |
