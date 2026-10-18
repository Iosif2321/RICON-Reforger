# E-web2: модели распространения, сотовая связь, ретрансляторы, TFAR/ACRE2, хранение данных в Reforger (веб-сбор)

Роль: claude.web2. Дата сбора: 2026-10-18. Режим: только чтение веб-источников, ничего не устанавливалось.

Уровни: **STD** = стандарт ITU/3GPP/ETSI; **DOC** = официальная документация Bohemia; **REF** = учебник, университет, производитель, официальный военный наставление; **MOD** = мод или его репозиторий/вики; **FORUM** = форум, хостинговый гайд, блог (добавлен для честной маркировки, аналогично E-web1); **ПРЕДП** = вывод автора этого файла.

**Оговорки о методе.**
- `itu.int` из этой среды недоступен (WebFetch: «Socket is closed», curl: код 000). Поэтому формулы ITU-R взяты из поисковых сниппетов PDF ITU (пометка «STD (сниппет)») или из вторичных источников, которые цитируют рекомендацию (REF). Номера действующих редакций взяты из сниппетов страниц `itu.int/rec/...`.
- Спецификации 3GPP скачаны **напрямую** с `etsi.org/deliver` (PDF → pdftotext) — пометка «STD (загружено)». Версии: TS 22.030/22.090/23.009/23.012/23.018/23.040/23.090/23.038/45.008 V19.0.0 (2025-10); TS 23.003 V19.8.0 (2026-10); TS 24.008 V19.5.0 (2026-02); TS 24.080 V19.4.0; TS 36.331 V19.4.0 (2026-09).
- `community.bistudio.com/wiki/*` отдаёт 403 (Cloudflare). Страницы BI wiki читались через **Wayback Machine** (пометка «DOC (архив, дата правки)») или через поисковые сниппеты («DOC (сниппет)»). Script API (`wikidata/external-data/...-members.html`) частично грузится напрямую («DOC (загружено)»).
- Код модов не копировался. Упоминаются только имена API и пути.

---

## Q28. Модели распространения радиоволн (ITU-R)

### Ответ

**P.525 — свободное пространство.** Действующая редакция **P.525-5 (11/2024)**. Базовые потери между изотропными антеннами:
`L_bf = 20·log10(4πd/λ) = 32.4 + 20·log10 f[МГц] + 20·log10 d[км]` (дБ).
Та же формула в метрах: `−27.55 + 20·log f[МГц] + 20·log d[м]` (её использует ACRE2, см. Q31).

**P.526 — дифракция.** Действующая редакция **P.526-16 (11/2025)**.
- Одиночное ребро (knife-edge, по Буллингтону): `J(ν) = 6.9 + 20·log10( sqrt((ν−0.1)² + 1) + ν − 0.1 )` дБ при ν > −0.78, иначе J = 0. В P.526-16 это уравнение (31).
- Безразмерный параметр (классическое определение P.526, ПРЕДП по памяти, первоисточник не загружен): `ν = h·sqrt( (2/λ)·(1/d1 + 1/d2) )`. Здесь h — высота вершины препятствия над линией прямой видимости, d1 и d2 — расстояния до препятствия.
- Несколько препятствий. В P.526-15 раздел 4.5 «Method for a general terrestrial path» состоит из 4.5.1 «Bullington model» и 4.5.2 «Complete method». Complete method — это **delta-Bullington**: Буллингтон по реальному профилю минус Буллингтон по гладкому профилю плюс дифракция над сферической Землёй. Тот же метод используется в P.452-17, P.1812-6 и P.2001-4. Отдельно в P.526 есть подразделы для двух изолированных рёбер (4.3) и нескольких изолированных цилиндров (4.4).
- Deygout и Epstein-Peterson **не являются** основным методом в актуальных редакциях. Они встречаются в литературе для сравнения. По данным REF, Deygout часто пессимистичен (завышает потери), Bullington может занижать потери (игнорирует промежуточные рёбра), Epstein-Peterson даёт большие ошибки при близко расположенных рёбрах.
- Совпадает ли нумерация разделов в -16 с -15, **не проверено**.

**P.1546 — point-to-area, по кривым.** Действующая редакция P.1546-6 (08/2019). Диапазон 30–4000 МГц, расстояния 1–1000 км (дальность подтверждена для ранних редакций; для -6 SEAMCAT пишет, что диапазоны применимости те же, что в -4/-5). Полный профиль рельефа **не нужен**. Нужна эффективная высота передающей антенны: средний уровень земли на участке 3–15 км в сторону приёмника (для трасс короче 15 км — на участке 0.2d…d). Опционально — поправка на угол просвета (terrain clearance angle) у приёмника. Кривые построены для 1 кВт ЭИИМ на частотах 100/600/2000 МГц.

**P.1812 — path-specific.** Действующая редакция **P.1812-8 (09/2025)**. Диапазон 30 МГц – 6 ГГц, трассы от 0.25 до ~3000 км, обе антенны не выше ~3 км над землёй. **Нужен детальный профиль рельефа** вдоль трассы большого круга. Дифракция считается методом delta-Bullington (разделы 4.3.1–4.3.4 в -8). Для -7 ITU предупреждало, что тропосферное рассеяние вносит ошибки на коротких трассах.

**P.838 — дождь.** Действующая редакция P.838-3 (03/2005). `γ_R = k·R^α` (дБ/км), R в мм/ч. Модель применима **только на 1–1000 ГГц**. На 1 ГГц: kH = 0.0000259, αH = 0.9691. Отсюда при ливне 50 мм/ч γ ≈ 0.0000259·50^0.969 ≈ **0.001 дБ/км** (ПРЕДП, расчёт). Ниже 1 ГГц рекомендация не применяется, а потери пренебрежимо малы. Вывод для КВ/УКВ-радио игры: дождь можно не моделировать (ПРЕДП).

**P.372 — радиошум.** Используемая редакция: таблица из FFI-отчёта ссылается на P.372-11 (09/2013). В найденных источниках упоминаются также -12, -14 и -17; константы c и d в них совпадают с -11.
- Медиана индустриального шума: `F_am = c − d·log10 f[МГц]` дБ (относительно kT0b), применимо **0.3–250 МГц**.
- Значения c/d: город 76.8/27.7; жилая зона 72.5/27.7; сельская 67.2/27.7; тихая сельская 53.6/28.6; галактический шум 52.0/23.0.
- Мощность шума: `f_a = p_n/(k·t0·b)`, t0 = 290 K. Отсюда `P_n[дБВт] = F_a + 10·log10 b − 204` (ПРЕДП, перевод определения в дБ).
- Пример (FFI): на 84.5 МГц F_am составляет 23.4 дБ (город), 19.1 (жилая), 13.8 (сельская), 7.7 (галактический). Выше 200 МГц FFI предлагает альтернативную модель Hagn.

**Two-ray (отражение от земли).** Точная сумма прямого и отражённого лучей с коэффициентом отражения Γ ≈ −1:
`P_r = P_t·G_t·G_r·(λ/4π)²·| 1/d1 − e^(−jΔφ)/d2 |²`, где `Δφ = 2π(d2−d1)/λ`.
- Дальняя зона: `P_r ≈ P_t·G_t·G_r·h_t²·h_r²/d⁴`, то есть `PL ≈ 40·log d − 20·log(h_t·h_r)` (м, G = 1). Наклон 40 дБ/декаду.
- Точка перелома. Gaussianwaves даёт `d_c = 4π·h_t·h_r/λ` (точка равенства асимптот). В учебниках, например у Rappaport, встречается вариант `4·h_t·h_r/λ` (по первой зоне Френеля). Источники **расходятся** в определении.
- Статья о VANET (REF) предупреждает, что упрощённая модель two-ray не лучше free-space, и предлагает точную интерференционную форму.

### Доказательства
1. https://www.itu.int/rec/R-REC-P.525-5-202411-I/en — P.525-5 (11/2024) «In force (Main)»; P.525-4 «incorporated by reference in the Radio Regulations». — STD (сниппет).
2. https://arxiv.org/pdf/2512.22363 — «ITU-R P.525 … 20·log10(4πd/λ) … = 32.4 + 20 log f(MHz) + 20 log d(km) dB». — REF (вторичное цитирование; константа 32.4 проверена расчётом: 20·log(4π·10⁹/3·10⁸) ≈ 32.44).
3. https://www.itu.int/rec/R-REC-P.526-16-202511-I/en — «P.526-16 (11/2025) … In force». https://www.itu.int/dms_pubrec/itu-r/rec/p/R-REC-P.526-16-202511-I!!PDF-E.pdf — «J(ν) … equation (31) … provided ν exceeds −0.78». — STD (сниппет).
4. https://www.itu.int/dms_pubrec/itu-r/rec/p/R-REC-P.526-14-201801-I!!PDF-E.pdf — «for ν greater than −0.78 an approximate value can be obtained» (6.9 + 20 log …). https://link.springer.com/article/10.1007/s44291-025-00075-w — «J(v) ≈ 6.9 + 20 log₁₀(√((v−0.1)² + 1) + v − 0.1) dB … v greater than −0.78». — STD (сниппет) + REF.
5. https://www.itu.int/dms_pubrec/itu-r/rec/p/R-REC-P.526-15-201910-S!!TOC-HTM-E.htm — оглавление: «4.5 Method for a general terrestrial path, 4.5.1 Bullington model, 4.5.2 Complete method»; 4.3 double isolated edges; 4.4 multiple isolated cylinders. — STD (сниппет оглавления).
6. https://www.itu.int/en/ITU-R/study-groups/rsg3/ionotropospheric/Validation%20examples%20for%20the%20delta%20Bullington%20diffraction%20prediction%20method.docx — «This method is used in Recommendations ITU-R P.526-15, P.452-17, P.1812-6 and P.2001-4». — STD (рабочий документ ITU-R SG3, сниппет).
7. http://www.mike-willis.com/Tutorial/diffraction.htm — «the one currently used by the ITU-R for terrestrial links is a modified version of the Deygout model». Старый туториал: он противоречит актуальной редакции (delta-Bullington). — REF (устарело).
8. https://repositorio.ufu.br/bitstream/123456789/18133/4/ComportamentoModelosCalculo.pdf — Bullington «provides the worst results», Deygout «often pessimistic», эталон — Vogler. https://www.its.ntia.gov/publications/download/TR-26-580.pdf — NTIA TR-26-580 (10/2025) сравнивает Bullington, Epstein-Peterson, Deygout и Giovaneli с Vogler на 50 сценариях (сами результаты не прочитаны). — REF.
9. https://www.itu.int/rec/R-REC-P.1546/en — «P.1546-6 (08/2019) … In force»; реализация SEAMCAT: «30 MHz to 4000 MHz». https://help.altair.com/Winprop/topics/winprop/user_guide/proman/propagation_models/proman_prop_model_itup.htm — «between 1-1000 km length». — STD (сниппет) + REF.
10. https://itu.int/dms_pubrec/itu-r/rec/p/R-REC-P.1546-1-200304-S!!PDF-E.pdf — «effective height … over the average level of the ground between distances of 3 and 15 km». — STD (сниппет, ранняя редакция).
11. https://www.itu.int/rec/R-REC-P.1812-7-202308-S/en — «30 MHz to 6 GHz … path lengths from 0.25 km up to about 3 000 km … both terminals within approximately 3 km height»; «users … specify detailed terrain profiles»; «P.1812-8 (09/2025) … In force». https://www.itu.int/dms_pubrec/itu-r/rec/p/R-REC-P.1812-8-202509-I!!TOC-HTM-E.htm — «4.3.4 Complete "delta-Bullington" diffraction loss model». — STD (сниппет).
12. https://itu-rpy.readthedocs.io/en/latest/apidoc/itu838.html — «P.838-3 (03/2005)»; «valid from 1 to 1,000 GHz»; «γ_R = k·R^α». Таблица из копии P.838-3 (Georgia Tech): на 1 ГГц kH = 0.0000259, αH = 0.9691, kV = 0.0000308, αV = 0.8592. — REF (реализация) + STD (сниппет таблицы).
13. https://www.ffi.no/no/Rapporter/16-00869.pdf (загружено, pdftotext) — «F_am = c − d log(f)»; Table 2.1: «City 76,8 27,7; Residential 72,5 27,7; Rural 67,2 27,7; Quiet rural 53,6 28,6; Galactic 52,0 23,0»; «claims a validity from 0,3 to 250 MHz»; ссылка «ITU-R P.372-11 "Radio noise" 09/2013». — REF (FFI цитирует STD).
14. https://www.gaussianwaves.com/?p=18908 — формула two-ray, `P_r = P_t G_t G_r h_t² h_r²/d⁴`, `d_c = 4π h_t h_r/λ`. — REF.
15. https://www.ccs-labs.org/bib/sommer2011using/sommer2011using.pdf — упрощённая модель two-ray не даёт преимущества над free-space. — REF.

### Пробелы
- Первоисточники ITU (PDF) не загружены из-за недоступности itu.int. Дословный текст P.525-5, P.526-16 §4.5 и P.1812-8 не сверен. Нужно скачать вручную с itu.int.
- Не подтверждено, что в P.526-16 метод для общего наземного пути остался delta-Bullington (запросы: «P.526-16 general terrestrial path delta-Bullington»).
- Действующий номер P.372 (-16 или -17) точно не подтверждён.

---

## Q29. Сотовая связь (GSM/UMTS/LTE) — процедуры для упрощённой игровой модели

### Ответ

**Идентификаторы.**
- **IMSI** — не более 15 цифр: MCC (3) + MNC (2–3) + MSIN. Идентифицирует подписку (SIM).
- **MSISDN** — номер E.164, «телефонный номер». Для каждой MS обязателен хотя бы один (кроме GPRS/EPS-подписок без MSISDN).
- **TMSI** — временный идентификатор, который выдаёт VLR.

Игровая модель (ПРЕДП): SIM = {IMSI, MSISDN, баланс}; HLR = таблица IMSI → {MSISDN, текущий VLR/сота, флаги}.

**Attach / location update.**
- Процедура location updating (TS 24.008 §4.4) имеет три назначения: normal location updating, periodic updating (по таймеру **T3212**), IMSI attach.
- Location area (LA) — группа сот. Внутри LA телефон перемещается без повторной регистрации.
- Explicit IMSI detach выполняется при выключении, IMSI attach — при включении.
- Implicit detach: VLR помечает абонента отсоединённым, если не было контакта дольше implicit detach timer (производный от периодического таймера).

Игровая модель (ПРЕДП): телефон регистрируется при включении и при смене «зоны» (группы вышек), периодически подтверждает присутствие. Пропал на N минут → «абонент недоступен».

**Paging и входящий вызов (MT).**
1. GMSC получает вызов и запрашивает HLR (Send Routeing Info).
2. HLR запрашивает у VLR роуминговый номер MSRN (Provide Roaming Number).
3. GMSC маршрутизирует вызов на VMSC.
4. VLR разрешает вызов и просит VMSC выполнить paging.
5. MS отвечает (Page ack), устанавливается канал.

Paging идёт по LA, где MS зарегистрирована (опционально по Paging Area до 5 LA).

**Исходящий вызов (MO).**
1. MS устанавливает сигнальное соединение и шлёт CM service request.
2. VLR проверяет и разрешает (Complete Call).
3. Когда сеть получает ACM от вызываемой стороны, MS получает Alerting (вызов идёт).
4. Когда приходит ANM (ответ), MS получает Connect.

**SMS (store-and-forward).**
- SMS идёт через Service Centre (SC), который хранит сообщение до доставки. Пользовательские данные — до 140 октетов (160 символов 7-bit).
- **TP-Validity-Period** задаёт, сколько SC гарантирует хранение. Относительный формат, 1 октет:
  - 0–143 → (VP+1)·5 мин (до 12 ч);
  - 144–167 → 12 ч + (VP−143)·30 мин;
  - 168–196 → (VP−166) дней;
  - 197–255 → (VP−192) недель (максимум 63 недели).
- При неудаче (absent subscriber, переполнена память) HLR сохраняет адрес SC в списке **MWD** (Messages-Waiting-Data).
- Когда MS снова доступна, сеть посылает SC **Alert-SC**. SC может «подстроить алгоритм повторов» по причине отсутствия. Сам алгоритм повторов **не стандартизован**: это реализация SC (ПРЕДП по тексту).
- Флаг TP-More-Messages-to-Send сообщает MS, что в SC ждут ещё сообщения.

**USSD (*100#).**
- Два режима. MMI-mode — прозрачная передача введённой строки в сеть и текстового ответа на экран. Application mode — данные для приложений.
- Работает через короткие **диалоги (сессии)** по сигнальным каналам: ~600 бит/с вне вызова, ~1000 бит/с во время вызова.
- Строка `*100#` подпадает под **Case a)** TS 22.090: «1–3 символа из (*, #), затем 1X(Y) с X = 0–4 … заканчивается #». Это зарезервировано для **HPLMN**: в роуминге запрос передаётся в домашнюю сеть.
- Сеть **обязана ответить** текстовой строкой с результатом или ошибкой. Сеть может запросить ответ пользователя (меню). Тогда следующая введённая строка не интерпретируется как MMI. Сеть обычно сама освобождает соединение.
- Маршрутизацию к приложению выполняет «USSD handler». HLR держит транзакцию и может закрыть её по таймеру приложения.
- Длина — до **182 символов** 7-bit (160 октетов).
- Отличие от SMS (ПРЕДП по 22.090/23.040): USSD — сессионный обмен в реальном времени без хранения в SC; при недоступности абонента сообщение не откладывается. SMS — store-and-forward с validity period и повторами.

**Хендовер.**
- TS 23.009 описывает процедуры поддержания соединения при переходе между сотами: intra-MSC, inter-MSC и другие, всего пять случаев.
- GSM (TS 45.008 Annex A): критерий power budget `PBGT(n) > HO_MARGIN(n)`. HO_MARGIN — «параметр для предотвращения повторяющегося хендовера между соседними сотами», то есть анти-ping-pong гистерезис 0–24 дБ. Дополнительно используются пороги уровня и качества, критерий дальности MS–BTS и голосование «P из N» усреднённых измерений.
- LTE (TS 36.331), событие **A3** (сосед лучше обслуживающей на offset):
  - вход: `Mn + Ofn + Ocn − Hys > Mp + Ofp + Ocp + Off`;
  - выход: `Mn + Ofn + Ocn + Hys < Mp + Ofp + Ocp + Off`.
  Условие должно держаться **TimeToTrigger** (ms0 … ms5120).
- Игровая модель (ПРЕДП): переключаться на вышку B, если S_B > S_A + Hys дБ непрерывно ≥ TTT секунд. Hys 3–6 дБ, TTT 0.3–1 с — значения для игры, не из стандарта.

### Доказательства
1. ETSI TS 123 003 V19.8.0, §2.2: «IMSI is composed of three parts: … MCC consisting of three digits … MNC consisting of two or three digits»; «Not more than 15 digits»; §3: «the assignment of at least one MSISDN (i.e. E.164 number) to a mobile station is mandatory». https://www.etsi.org/deliver/etsi_ts/123000_123099/123003/19.08.00_60/ts_123003v190800p.pdf — STD (загружено).
2. ETSI TS 123 012 V19.0.0, §2.2: «A location area is an area in which, after having performed a location update once, MSs may roam without being required to perform subsequent location updates»; §2.4.1: «Explicit IMSI attach … the station is powered up»; §2.4.2: «Implicit IMSI detach … no successful contact … for a time determined by the implicit detach timer»; §2.6: «A Paging Area (PgA) is composed of up to 5 Location Areas». https://www.etsi.org/deliver/etsi_ts/123000_123099/123012/19.00.00_60/ts_123012v190000p.pdf — STD (загружено).
3. ETSI TS 124 008 V19.5.0, §4.4.1: «used for the following purposes: normal location updating … periodic updating … IMSI attach»; §4.4.2: «The procedure is controlled by the timer T3212». https://www.etsi.org/deliver/etsi_ts/124000_124099/124008/19.05.00_60/ts_124008v190500p.pdf — STD (загружено).
4. ETSI TS 123 018 V19.0.0, §4.2: «HLRB requests a roaming number from VLRB … VLRB determines that the incoming call is allowed, it requests VMSCB to page the MS … When the MS responds, VMSCB informs VLRB in the Page ack message»; MO: «When the destination exchange returns an ISUP Address Complete Message (ACM), VMSCA sends an Alerting … ANswer Message (ANM), VMSCA sends a Connect». https://www.etsi.org/deliver/etsi_ts/123000_123099/123018/19.00.00_60/ts_123018v190000p.pdf — STD (загружено).
5. ETSI TS 123 040 V19.0.0, Introduction: «a Service Centre, which acts as a store and forward centre for short messages»; §3.2.1: «how long the SC shall guarantee its existence in the SC memory before delivery»; §9.2.3.12.1 (таблица TP-VP); §3.2.7 Alert-SC; MWD: «list of addresses (SC-Addr) of SCs which have made previous unsuccessful delivery attempts»; «so that the SC may adjust any retry algorithm appropriately»; «up to 140 octets». https://www.etsi.org/deliver/etsi_ts/123000_123099/123040/19.00.00_60/ts_123040v190000p.pdf — STD (загружено).
6. ETSI TS 122 090 V19.0.0, §4: «two modes of USSD: MMI-mode and application mode»; «short dialogues with peak data throughput … approximately 600 bits/s outside of a call»; §5.1.2 Case a) (цитата выше) «reserved for HPLMN use»; «The network shall terminate the mobile initiated operation by responding … with either an error signal, or a text string»; §5.3: «Release of the connection used for an unstructured dialogue is normally the responsibility of the network». https://www.etsi.org/deliver/etsi_ts/122000_122099/122090/19.00.00_60/ts_122090v190000p.pdf — STD (загружено).
7. ETSI TS 123 090 V19.0.0, §1: «routing of messages to the correct application is carried out by the USSD handler»; §5.2.1: «The HLR may also release the transaction … (e.g. if an application timer expires)». https://www.etsi.org/deliver/etsi_ts/123000_123099/123090/19.00.00_60/ts_123090v190000p.pdf — STD (загружено).
8. ETSI TS 123 038 V19.0.0: «The message can then consist of up to 182 user characters»; «in 160 octets, is it possible to pack (160*8)/7 = 182.8». https://www.etsi.org/deliver/etsi_ts/123000_123099/123038/19.00.00_60/ts_123038v190000p.pdf — STD (загружено).
9. ETSI TS 123 009 V19.0.0, §1: «to ensure that the connection to the Mobile Station … is maintained as it moves from one cell or radio network to another»; «five handover cases». https://www.etsi.org/deliver/etsi_ts/123000_123099/123009/19.00.00_60/ts_123009v190000p.pdf — STD (загружено).
10. ETSI TS 145 008 V19.0.0, Annex A.3.2 e): «If PBGT(n) > O and PBGT(n) > HO_MARGIN(n) a handover, cause PBGT(n), might be required»; таблица параметров: «HO_MARGIN(n) A parameter used in order to prevent repetitive handover between adjacent cells … Range (0, 24 dB)». https://www.etsi.org/deliver/etsi_ts/145000_145099/145008/19.00.00_60/ts_145008v190000p.pdf — STD (загружено).
11. ETSI TS 136 331 V19.4.0, §5.5.4.4 Event A3: «Mn + Ofn + Ocn - Hys > Mp + Ofp + Ocp + Off»; TimeToTrigger: «the time during which specific criteria for the event needs to be met in order to trigger a measurement report … ms0, ms40, … ms5120». https://www.etsi.org/deliver/etsi_ts/136300_136399/136331/19.04.00_60/ts_136331v190400p.pdf — STD (загружено).

### Пробелы
- LTE/EPS attach и paging (TS 23.401) не загружены: в каталоге ETSI для 123401 не нашёлся PDF. Для игры хватает модели GSM.
- TS 24.090 (USSD stage 3) не читался. Таймеры USSD-сессии на стороне сети в стандарте не заданы, это таймер приложения.
- Явное слово «ping-pong» в загруженных текстах 3GPP не найдено. Анти-ping-pong механизм — это HO_MARGIN / Hys + TTT.

---

## Q30. УКВ-ретрансляторы (голосовые репитеры) и военная ретрансляция

### Ответ

**Гражданский/любительский репитер.**
- Принимает на одной частоте (вход) и одновременно передаёт на другой (выход). Разнос (offset) типично **±600 кГц на 2 м** и **±5 МГц на 70 см**. Без разноса репитер «слышал бы только себя».
- Изоляцию приёмника от передатчика даёт **дуплексер** (резонаторы).
- Доступ часто закрыт субтоном **CTCSS** или DCS: без нужного тона репитер не ретранслирует. Это защита от ложных срабатываний.
- **Hang time**: после отпускания PTT передатчик держится несколько секунд, чтобы убрать «щелчок» шумоподавителя.
- **Courtesy tone**: сигнал о сбросе таймера.
- **Time-out timer** (обычно ~3 мин) отключает передачу при затяжном удержании.
- Абонент работает полудуплексом: не может говорить и слышать одновременно (ПРЕДП из принципа PTT). Две одновременные передачи на входе дают подавление (capture) или интерференцию (ПРЕДП). В ACRE2 это моделируется настройкой «Interference», см. Q31.

**Военная ретрансляция (SINCGARS, FM 11-32).**
- Retrans-станция — **две радиостанции**. Primary слушает командную сеть, secondary обеспечивает канал ретрансляции.
- Три режима:
  1. SC↔SC (одноканальный ↔ одноканальный) требует **разноса f1/f2 ≥ 10 МГц**. Пространственный разнос антенн или снижение мощности уменьшают требуемый частотный разнос.
  2. FH↔SC (ППРЧ ↔ одноканальный) не требует ни частотного, ни пространственного разноса.
  3. FH↔FH требует хотя бы одной отличающейся переменной ППРЧ. Предпочтительно — те же hopset, но разные net ID. «Все операции ретрансляции требуют двух net ID».
- Защищённый трафик ретранслируется даже при незащищённых retrans-радиостанциях. Но оператор не может его слушать без загруженных ключей.
- Для снижения совмещённых помех рекомендуют разносить антенны (≥50 м через выносы) и ставить станцию на возвышенность (ПРЕДП из контекста).

**Цепочки и петли.**
- Связанные репитеры (IRLP/AllStar, связь по DTMF) могут образовывать **«ping-pong» петлю**: линк-радио слышит хвост или courtesy tone репитера и передаёт его обратно.
- Практические решения: репитер передаёт CTCSS только при наличии сигнала на входе; отрицательная задержка rxondelay; выделенный линк-порт.
- Для mesh/daisy-chain топологий существуют алгоритмы предотвращения петель (патент US10033625: сравнение «весов» узлов).
- Игровая модель (ПРЕДП): ретрансляция как граф с TTL/hop-limit и множеством посещённых узлов на каждую передачу. Каждый хоп добавляет задержку (ПРЕДП: 50–200 мс на хоп для «ощущения»; в стандартах задержка аналоговых репитеров пренебрежимо мала и не нормируется). Разнос вход/выход — как правило «вход ≠ выход», иначе петля.

### Доказательства
1. https://amateurradio.com.au/about/repeaters-radio-amateurs-guide — «repeaters employ two frequencies, with the disparity between them termed the offset»; «the repeater would only hear its own transmissions»; «installing a device known as a duplexer»; «tones access enabled … to prevent false triggering»; «inbuilt 3 minute timer»; «courtesy tone that indicates the repeater timer has reset»; «Using DTMF tones you can link to another IRLP enabled repeater». — REF (клуб/гайд; поисковый сниппет добавляет «±0.600 MHz … ±5.000 MHz»).
2. https://hamradioprep.com/?p=33741 и https://www.kb6nu.com/2022-no-nonsense-technician-class-license-study-guide-operating-practices/ (через поиск) — «2-meter repeaters, the offset is typically ±0.600 MHz … 70-centimeter … ±5.000 MHz»; «hang timer is to remove the squelch crunch». — REF (сниппет).
3. https://www.globalsecurity.org/military/library/policy/army/fm/11-32/Ch5.htm (FM 11-32, §5-4) — «The primary radio monitors the command/operations network and the secondary provides the retransmission link»; «10 MHz separation between f1 and f2»; «FH to single-channel … no requirement for frequency or physical separation»; «FH to FH … at least one of the hopping variables … different … same hopsets but with different net identifiers»; «the retransmission operator cannot monitor the communications unless the secure devices are filled». — REF (наставление армии США, старая редакция; актуально ATP 6-02.53, не загружено).
4. https://www.globalsecurity.org/military/library/policy/army/fm/11-32/Ch4.htm — «All retransmission operations require two net identifiers for each side of the retransmission station». Ch9: «separation of 50 meters (165 feet) or more between colocated radios». — REF.
5. https://community.allstarlink.org/t/courtesy-tones-from-node-connected-to-a-repeater/20350 — «It is caused by your simplex node hearing the repeater when it unkeys»; решение — CTCSS только при сигнале на входе, отрицательный rxondelay. — FORUM.
6. https://patents.google.com/patent/US10033625 — «Loops may be avoided by comparing the weights of two nodes in a repeater network»; «a loop between repeaters can be formed» (daisy-chain, star). — REF (патент, истёк).

### Пробелы
- Актуальный ATP 6-02.53 (Tactical Radio Operations) не загружен. Запросы: «ATP 6-02.53 retransmission pdf armypubs».
- Официальная документация ARRL по репитерам не найдена, использованы клубные и учебные источники.
- Числовая задержка аналогового/цифрового репитера (DMR/P25) не найдена. Запрос: «DMR repeater latency ms».

---

## Q31. TFAR и ACRE2 — матрица возможностей (по официальной документации/репозиториям)

### Ответ (матрица)

| Функция | TFAR | ACRE2 |
|---|---|---|
| Классы раций | SR (карманные), LR (ранцевые/авиа/машинные), подводные (DD) | Конкретные модели: PRC-343/77/148/117F/152, SEM 52 SL/70, BF-888S |
| Модель сигнала | Влияние рельефа (Terrain Interception Coefficient), глобальный коэффициент дальности, табличные дальности (напр. 20 км для ранца, 30 км для машинной) | «ITU terrain loss», многолучевость по карте высот (в C++-расширении), мощность, частота, диаграммы антенн; модели Arcade / LOS Simple / LOS Multipath (по умолчанию) / Longley-Rice (ITM, 20 МГц–20 ГГц, экспериментально) |
| Здания/объекты на радиосигнал | Настройка «Object Interception» | Нет («buildings … do not effect radio signals») |
| Антенны | Отдельный аддон antennas («Communication Tower (TFAR)»); механика не описана | Направленность антенн (ориентация по спине, сгибание), Ground Spike Antenna (до 20 Вт, мачта) |
| Интерференция / полный дуплекс | Настройка Full-Duplex | Настройки Interference (одновременные передачи деструктивны) и Full Duplex |
| Ретрансляция (retrans) | В документации **не найдена** | В документации **не найдена** |
| Интерком | Vehicle intercom (каналы Cargo/Crew), внешний интерком (телефон на корпусе, беспроводная гарнитура, радиус) | Crew/Pax intercom, режимы R/T/RT, голос/PTT, мастер-станция, broadcast, infantry telephone |
| Стойки в технике | LR-рация для водителя/командира/стрелка | Vehicle racks (VRC-64/103/110/111, SEM 90) с повышением мощности до 20–50 Вт, интеграция с интеркомом |
| Громкость голоса | Шёпот / норма / крик | Слайдер из 5 уровней (≈1–15 м «громко», до ~195 м «едва слышно») |
| Окклюзия голоса | Изоляция техники (экспериментально), Directional Voice | Окклюзия объектами и зданиями, ослабление в технике по местам |
| Динамики (speaker) | Режим Speakers у рации; статические рации с динамиком (модуль Static Radio) | Режим speaker; брошенная рация продолжает играть |
| Слышимость ИИ | AI Hearing: ИИ слышит говорящих и активные динамики | «AI Hears you» |
| Языки | Нет | Babel (искажённая речь для чужого языка) |
| Шифрование/стороны | Radio code по сторонам; трофейные рации сохраняют настройки | Конфигурации раций по сторонам; рация — физический объект с настройками |
| Под водой | Рация не работает; речь вблизи неразборчива | Не описано |
| Прочее | Стерео L/R, дополнительный канал, режимы Lightweight/Serious, слышимость спектатора/Zeus | Несколько PTT, внешние рации (чужой инвентарь), God Mode, автопереключение каналов TS3, API своей функции сигнала |

### Доказательства
1. https://github.com/michail-nikolaev/task-force-arma-3-radio/wiki/README_EN — «The quality of radio signal is affected by the nature of surrounding terrain»; «Whispering, Normal or Yelling»; «You can not use radio communication underwater»; закрытая техника изолирует голос; дальности «30–87 MHz /20 km», «30 km for vehicle-mounted». — MOD (официальная вики мода).
2. https://raw.githubusercontent.com/michail-nikolaev/task-force-arma-3-radio/master/addons/core/stringtable.xml (строки настроек, не код) — «Terrain Interception Coefficient»; «Radio range coefficient»; «Experimental Vehicle Isolation»; «Enable vehicle Intercom»; «Enable Object Interception»; «Directional Voice»; «Full-Duplex … Allows Radios to simultaneously receive while transmitting»; «Intercom Ducking»; «Speakers»; «radio code». Аддоны репозитория (GitHub API): `ai_hearing, antennas, backpacks, core, external_intercom, handhelds, static_radios`. — MOD.
3. TFAR `addons/ai_hearing/stringtable.xml` — «If enabled, the ai is able to detect speaking players»; «detect active speakers». `addons/external_intercom` — «Pickup Intercom Phone», «Wireless Intercom», «External Intercom Range». `addons/antennas` — «Communication Tower (TFAR)». `addons/static_radios` — «Static Radio», «Speaker enabled». — MOD.
4. https://acre2.idi-systems.com/wiki/user/feature-list — «Signal loss and degradation is done using REAL WORLD ITU Terrain loss modeling»; «AI direct speaking detection; AI can also hear you talking»; «Vehicle intercom system … Infantry Telephones»; «In-game speakers … If you drop the radio, it keeps playing!»; «Babel». — MOD (официальная вики мода).
5. https://acre2.idi-systems.com/wiki/user/radio-signal-loss — «uses the terrain height map to calculate multiple possible pathways … Constructive and destructive interference»; «buildings and terrain objects do not effect radio signals»; модели «Arcade / LOS Simple / LOS Multipath / Longley-Rice (ITM) … between 20MHz and 20GHz»; Terrain Loss по умолчанию 0.5; Interference; Full Duplex; Ignore Antenna direction. — MOD.
6. https://acre2.idi-systems.com/wiki/frameworks/custom-signal-processing — вход функции: частота (МГц), мощность (мВт), ID рации приёмника и передатчика; выход: [доля мощности 0–1, дБм]; чувствительность «around -100 (AN/PRC343) to -117 (AN/PRC-117F)»; пример Arcade основан на free-space `−27.55 + 20·log f + 20·log d`. — MOD (описание, код не копировался).
7. https://acre2.idi-systems.com/wiki/user/direct-speech (таблица 5 уровней громкости), /wiki/user/vehicle-intercom (режимы R, T, R/T; voice/PTT; master station), /wiki/user/vehicle-racks (повышение мощности), /wiki/user/ground-spike-antenna («increase the transmit power to 20 W»). — MOD.

### Пробелы
- Ретрансляция в TFAR и ACRE2 в документации не найдена. Запросы: «ACRE2 retransmission», «TFAR relay» по GitHub issues. Поиск через GitHub API вернул пусто, вероятно, из-за лимита.
- Механика аддона TFAR antennas (влияние вышки на дальность) не описана в документации. Нужно чтение кода (вне рамок «без кода»).
- Документация TFAR по CBA Settings на вики — заглушка.

---

## Q21/Q23. Reforger: хранение данных и серверная конфигурация мода

### Ответ

**Сохранение данных.**
1. **Штатная система persistence (с 1.6).**
   - Persistence — это WorldSystem, регистрируется через конфиг world systems (пример: `SCR_PersistenceSystem` в `GameMasterSystems.conf`). Конфиги лежат в `Configs/Systems/Persistence`.
   - Поддерживается сохранение и загрузка пользовательских сущностей, компонентов и скриптовых экземпляров.
   - API `PersistenceSystem`: `Save`, `StartTracking/StopTracking`, `GetPersistentState(TypeName)`, `CommitStorage/ClearStorage(TypeName storageType)`, `FindById(UUID)`, `RequestLoad/RequestSpawn`, `WhenAvailable`, хуки `OnBeforeSave/OnAfterSave/OnAfterLoad`.
   - В server config есть блок `persistence`. По сторонним гайдам в нём: `autoSaveInterval` (мин, 0 — выкл), `hiveId` (0–16383), `databases`, `storages`. Хранилища делятся на session storage (состояние мира) и game mode storage (прогресс).
   - Старт с сохранения — параметр `-loadSessionSave`.
2. **EPF/EDF (Arkensor)** — **устарели с 1.6**: «spiritual successor was adapted into the base game». EDF хранил данные в `$profile:/.db` (JSON/Binary-драйверы через `FileIO.MakeDirectory`, `SaveToFile`/`LoadFromFile`). Это подтверждает, что моды пишут файлы в `$profile:` (MOD).
3. **Сериализация:**
   - `SCR_JsonSaveContext`/`SCR_JsonLoadContext` и `SCR_BinSaveContext`/`SCR_BinLoadContext` с `SaveToFile`/`LoadFromFile`;
   - `JsonApiStruct` (наследование, `RegV()`, `Pack`/`Expand`, `SaveToFile`/`PackToFile`/`LoadFromFile`/`ExpandFromRAW`).
4. **`$profile:`.**
   - `-profile` задаёт каталог профиля; сервер пишет туда логи, туда ставятся аддоны.
   - `-backendLocalStorage` переводит сохранение «player and general data» бэкенда на **локальные JSON-файлы в папке профиля**. На форуме виден путь `$profile:.backend\campaign.json`.
   - `-scriptAuthorizeAll` отключает запрос безопасности для FileIO-операций «**outside of profile directory**». Отсюда ПРЕДП: FileIO внутри профиля разрешён без запроса.
5. **`SCR_DataCollectorComponent`** — штатный сборщик статистики сессии и игроков: `GetPlayerData(playerID, createNew, requestFromBackend)`, модули (`FindModule`), статистика фракций. Регистрируется через `ArmaReforgerScripted.RegisterDataCollector`. Это не хранилище произвольных данных мода (ПРЕДП).

**Как мод может читать серверный конфиг.**
- Официального API для чтения **произвольных** ключей server config JSON **не найдено**.
- Документированные варианты:
  - (a) `gameProperties.missionHeader` «overwrites the scenario's SCR_MissionHeaderCampaign». Скрипт получает заголовок через `ChimeraGame.GetMissionHeader()`. ПРЕДП: мод с собственным наследником SCR_MissionHeader может принимать свои поля из `missionHeader` (нужна проверка: принимаются ли поля modded-класса и что происходит на voting-серверах, где, по данным модов Workshop, missionHeader не передаётся в выбранный сценарий).
  - (b) Собственный JSON в `$profile:` (например `$profile:RICON/server.json`), чтение через `JsonApiStruct.LoadFromFile` или `SCR_JsonLoadContext.LoadFromFile`. Это паттерн, который подтверждают DOC (API) и MOD (EDF) (ПРЕДП как рекомендация).
- `ArmaReforgerScripted.ReadGameConfig(string sResource)` существует, но описания нет.
- Из штатного server config к радио относится `VONCanTransmitCrossFaction` (передача на рациях чужой фракции), а также `VONDisableUI` и `VONDisableDirectSpeechUI`.

### Доказательства
1. https://github.com/Arkensor/EnfusionPersistenceFramework/blob/armareforger/README.md — «This framework is deprecated since game version 1.6 in which a spiritual successor was adapted into the base game»; ссылки на `Arma_Reforger:Persistence_System`, `Server_Config#persistence`, `Startup_Parameters#worldSystemsConfig`. — MOD (загружено, репозиторий обновлялся 2026-04-18).
2. https://community.bistudio.com/wiki/Arma_Reforger:Persistence_System — «Persistence is a WorldSystem, defined through the world systems config»; `SCR_PersistenceSystem` в `GameMasterSystems.conf`; `Configs/Systems/Persistence`. — DOC (сниппет; страница 403, в Wayback нет).
3. https://community.bistudio.com/wikidata/external-data/arma-reforger/ArmaReforgerScriptAPIPublic/interfacePersistenceSystem-members.html — список членов (Save, StartTracking, GetPersistentState, CommitStorage(TypeName storageType…), FindById(UUID), RequestLoad, WhenAvailable, OnBeforeSave…). — DOC (загружено).
4. https://loafhosts.com/guides/arma-reforger-persistence-and-save-games/ и https://xgamingserver.com/docs/arma-reforger/server-config-reference — `persistence.autoSaveInterval` («0 to disable», по умолчанию 10), `hiveId` «0–16383», `databases`, `storages`; session storage и game mode storage; гайд «written against … 1.8.0.10». — FORUM.
5. https://raw.githubusercontent.com/Arkensor/EnfusionDatabaseFramework/armareforger/src/Scripts/Game/Drivers/LocalFile/EDF_FileDbDriverBase.c — `DB_BASE_DIR = "$profile:/.db"`, `FileIO.MakeDirectory`; JsonFile-драйвер: «Local .json files for workbench development and *small* data volumes». — MOD (имена API, код не копировался).
6. Wayback-копия https://community.bistudio.com/wiki/Arma_Reforger:Serialisation (правка 21.02.2024) — «serialisation using SCR_JsonSaveContext/SCR_JsonLoadContext and SCR_BinSaveContext/SCR_BinLoadContext»; «See also JsonApiStruct Usage». — DOC (архив).
7. Wayback-копия https://community.bistudio.com/wiki/Arma_Reforger:JsonApiStruct_Usage (правка 16.07.2024) — «JsonApiStruct is scripted object with support to: Encode … Decode … Import/export from/to file»; «RegV() adds the provided variable name to the defined object's list»; методы `ExpandFromRAW`, `SaveToFile`, `PackToFile`, `LoadFromFile`. — DOC (архив).
8. https://community.bistudio.com/wikidata/external-data/arma-reforger/ArmaReforgerScriptAPIPublic/interfaceJsonSaveContext.html — «proto external bool JsonSaveContext.SaveToFile(string fileName)». — DOC (сниппет).
9. Wayback-копия https://community.bistudio.com/wiki/Arma_Reforger:Startup_Parameters (правка 14.02.2025) — «-profile defines the path to the profile directory»; «-backendLocalStorage forces Save & Load of player and general data … to work with local JSON files in profile folder instead»; «-loadSessionSave allows the game to load a previous game session»; «-scriptAuthorizeAll disables the security popup about RunCmd, RunProcess, KillProcess and (outside of profile directory) FileIO operations»; «Mods are searched in profile/addons». — DOC (архив; раздел `worldSystemsConfig` в этой копии отсутствует).
10. Wayback-копия https://community.bistudio.com/wiki/Arma_Reforger:Server_Config (правка 13.01.2025) — «missionHeader: This property overwrites the scenario's SCR_MissionHeaderCampaign»; «VONCanTransmitCrossFaction … allow players to transmit on other factions radios»; «mods: The list of mods required by the client that will automatically be downloaded». Раздела persistence в этой копии нет: он появился позже, в 1.6+. — DOC (архив).
11. Wayback-копия https://community.bistudio.com/wiki/Arma_Reforger:Server_Hosting (правка 09.09.2024) — «The Server exe uses the -config startup parameter to target the configuration file»; «Console logs etc can be found in above mentioned profile folder». — DOC (архив).
12. https://community.bistudio.com/wikidata/external-data/arma-reforger/ArmaReforgerScriptAPIPublic/interfaceArmaReforgerScripted-members.html — `ReadGameConfig(string sResource)`, `RegisterDataCollector(SCR_DataCollectorComponent)`, `GetDataCollector()`, `GetMissionHeader()` (ChimeraGame), `GetSaveGameManager()` (ChimeraGame). — DOC (загружено; описаний нет).
13. https://community.bistudio.com/wikidata/external-data/arma-reforger/ArmaReforgerScriptAPIPublic/interfaceSCR__DataCollectorComponent-members.html — `GetPlayerData(int playerID, bool createNew=true, bool requestFromBackend=true)`, `FindModule(TypeName)`, `GetFactionStats`, `StartDataCollectorSession`. — DOC (загружено).
14. Форум BI (через поиск) — лог «RequestSave: $profile:.backend\campaign.json … JsonApiStruct :: Object name="m_EditorStruct", not initialized». — FORUM (сниппет).
15. https://reforger.armaplatform.com/workshop/AA4441197C834870 (Paprika COE2 Mission Defaults) — на voting-серверах заголовок миссии выделенного сервера не переносится в выбранный сценарий. — MOD (сниппет).

### Пробелы
- Текущие страницы `Arma_Reforger:Server_Config#persistence` и `Arma_Reforger:Persistence_System` дословно не прочитаны: 403, в Wayback нет. Нужно открыть в браузере.
- Страница класса `FileIO` в Script API не найдена (403 на угаданных URL). Подтверждение сигнатур FileIO — только через MOD (EDF). Запросы: «ArmaReforgerScriptAPIPublic FileIO», «EnfusionScriptAPIPublic FileIO».
- Нет официального описания того, как мод добавляет собственный блок в server config. Это **НЕ НАЙДЕНО**; рабочие варианты — missionHeader или собственный JSON в `$profile:` (требуют теста на 1.8.x).
- Не найдено, как устроены сохранения штатной persistence на диске (формат, путь).

---

## Сводная таблица

| Q | Тема | Статус | Уровень |
|---|---|---|---|
| Q28 | P.525 (формула, -5) | ЗАКРЫТ | STD (сниппет) + REF |
| Q28 | P.526 J(ν), редакция -16, метод для нескольких препятствий | ЧАСТИЧНО (общий путь подтверждён для -15) | STD (сниппет) + REF |
| Q28 | P.1546 / P.1812 (область, частоты, рельеф) | ЗАКРЫТ | STD (сниппет) + REF |
| Q28 | P.838 (дождь, <1 ГГц) | ЗАКРЫТ | REF + STD (сниппет) + ПРЕДП (расчёт) |
| Q28 | P.372 (шум) | ЗАКРЫТ (номер актуальной редакции — ЧАСТИЧНО) | REF (цитирует STD) |
| Q28 | Two-ray | ЗАКРЫТ (точка перелома: источники расходятся) | REF |
| Q29 | IMSI/MSISDN, attach/LU, paging, MO/MT | ЗАКРЫТ | STD (загружено) |
| Q29 | SMS store-and-forward, VP, повторы | ЗАКРЫТ (алгоритм повторов не стандартизован) | STD (загружено) + ПРЕДП |
| Q29 | USSD *100# | ЗАКРЫТ | STD (загружено) |
| Q29 | Хендовер (критерии, гистерезис, TTT, ping-pong) | ЗАКРЫТ | STD (загружено) |
| Q29 | LTE attach/paging (23.401) | НЕ НАЙДЕНО (не загружено) | — |
| Q30 | Голосовой репитер (offset, CTCSS, hang time, TOT) | ЗАКРЫТ | REF |
| Q30 | SINCGARS retrans | ЗАКРЫТ (по FM 11-32, не по ATP 6-02.53) | REF |
| Q30 | Цепочки и петли | ЧАСТИЧНО | FORUM + REF + ПРЕДП |
| Q30 | Задержка репитера | НЕ НАЙДЕНО | — |
| Q31 | Матрица TFAR / ACRE2 | ЗАКРЫТ (кроме ретрансляции и механики антенн TFAR) | MOD |
| Q21/Q23 | Persistence (штатная, с 1.6), EPF устарел | ЧАСТИЧНО (официальная страница — только сниппет) | DOC (сниппет/загружено) + MOD |
| Q21/Q23 | JSON / FileIO / `$profile:` | ЗАКРЫТ (FileIO — только MOD) | DOC (архив) + MOD |
| Q21/Q23 | Чтение серверного конфига модом | НЕ НАЙДЕНО (официально); варианты — ПРЕДП | DOC (архив) + ПРЕДП |
