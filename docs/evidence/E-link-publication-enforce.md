# S3b: Enforce-код снимков связи совпадает с замороженным корпусом

Дата: 2026-10-22. Автор кода и стенда: claude. Статус: OWN_CHECKED, ждёт независимого ревью codex.
Уровень: `ENFORCE_PARITY` (код исполнен в Workbench без мира, Play и сервера), не runtime.
Основание: REQ-link-publication v0.2 AGENT-REVIEWED (`fc9ad7a`), корпус `link_publication_v1.json` заморожен
по SHA `a60ed14590222c103aceea9030fb80ee233eea432b5969b8ed7fe93ad5c10be3` (agora #3343, #3347).
До кода claude пересчитал корпус своим checker'ом только по REQ: 106/106 (`8e5d34a`).

## Что сделано

- `addons/RICON_Core/Scripts/Game/RICON/Core/RICON_LinkPublication.c` — чистая служба `Publish(input, registry,
  source)`: preflight 0–10 по REQ-LP-03, план ключей REQ-LP-05 (сортировка слиянием, без квадратичной вставки),
  правило выдачи REQ-LP-06 (разность Q во float-переменной, ceil TTL последней выдачи), атомарная партия и сброс
  REQ-LP-09/10. Реестр и снимки — новые объекты; вход и старый реестр не меняются. Источник номеров —
  `RICON_LinkSequenceSource` (fail-closed) и адаптер `RICON_ManagerLinkSequenceSource`, который только делегирует
  `RICON_CommsManagerComponent.GetSessionEpoch/NextLinkSequence`.
- Стенд `RICON_LinkPublicationFixtureSupport.c`: тестовый источник точно по REQ-LP §6 (failAt, rotateAt, invalidAt,
  rotatedEpoch, ротация при seq ≥ MAX−1), его настоящие счётчики сверяются со счётчиками результата; проверка
  неизменности входа и реестра, отсутствие общих объектов между входом, первым и повторным результатом; 15
  контрольных порч сравнения; 4 доп. проверки (перестановка трёх массивов на P04, базовый источник и адаптер без
  менеджера fail-closed, NEED_FULL_RESYNC через такой адаптер; по unit codex #3343 — реестр другой epoch с
  highWater=MAX и полным maxActive: NEED_FULL_RESYNC без ложных ACTIVE_LIMIT/ID_EXHAUSTED, highWater 0).
- Генератор `tools/RICON_TestRunner/gen_link_publication_fixture.py` только копирует ожидания, закреплён за SHA
  корпуса; тест генератора — 8 случаев. Неизвестный `deviceCode` (C37) кодируется как −1 по REQ-LP §6.
- Общая регрессия: `regress_host.py` (+ CLI эталона S3b и `--check` генератора), `regress_wb.ps1` (+ пробник).

SHA-256 файлов репозитория — по содержимому git blob (LF); рабочая копия Windows может хранить CRLF.

| Файл | SHA-256 |
|---|---|
| `RICON_LinkPublication.c` | `b3b5061a73937763feb7fb1991f8ad6d7938970a009ff79f98e536c41f078deb` |
| `RICON_LinkPublicationFixtureSupport.c` | `96fab662465769a70ed4afeca667e5c981d1967290b6db319856e5894cd6d0a5` |
| `RICON_LinkPublicationFixtures.c` | `03d7b37832a986d6eb23dad568f41fd90fcf1dbf72348c3711cb87df097c45b0` |
| `RICON_LinkPublicationProbePlugin.c` | `88a914cf5bede1ad9252b86d3f9d056443723f30d07e9f01ae860ab5c710a4b4` |
| `gen_link_publication_fixture.py` | `a327f1efd2b3eccd1664fbdca0c3047b57238a51498023bc7e9f7e4811513c15` |

## Исполнение

База логов: `%USERPROFILE%/Documents/My Games/ArmaReforgerWorkbench/logs/`, файл `console.log`.

| Запуск | Итог | SHA-256 |
|---|---|---|
| `ricon-linkpub-claude-20261022-221016` (первый срез) | exit 0, `cases=106 match=106 immutable=106 controls=15 extras=3 snapshots=37`, `done: PASS`, SCRIPT(E) 0 | `415d8f0a1a4ea3a8df7f5976f37514c703fe193de18d691cbf06608ab2de13fe` |
| `ricon-r29-claude-20261022-221754-RICON_LinkPublicationProbePlugin` (в полной регрессии) | то же, extras=3 | `d6653992067efd13522e1e96f925ac3fca60546e45bf8c90373fb0a05795fcd7` |
| `ricon-linkpub-claude-20261022-222158` (после #3354, финальный срез) | exit 0, `cases=106 match=106 immutable=106 controls=15 extras=4 snapshots=37`, `done: PASS`, SCRIPT(E) 0 | `fec0d35bb4b1e340d09f0bef20d3ca12dff7c495fcd1c6581587693bb2bb79ea` |

CRC в финальном логе: `RICON_LinkPublication.c` 197aa68a (код не менялся), FixtureSupport 00f4707e, Fixtures 65c76868,
Probe 4f158f9c. Первый срез: FixtureSupport 6846dc19, Probe 1d84e440.
Отдельная сверка лога claude по каждому id с корпусом (код, число снимков, вызовы source, чтения epoch, флаг full):
106/106, помеченных FAIL нет. Первый запуск до этого (`…-220942`) не скомпилировался: остаток переименования
`next` → исправлен, затем PASS.

Общая регрессия на одном дереве:
- host (финальный срез, + CLI channel_cutoff по просьбе codex #3362): `TOTAL 31/31 PASS` —
  `docs/evidence/receipts/s3b-host-claude-20261022.txt`, SHA-256 `0d7c2aa4b20bf86d1f5fcc320ac05c3426a9406a6121c9df5d30349c3081766b`;
- Workbench (первый срез, extras=3): `TOTAL 11/11 PASS` — `docs/evidence/receipts/s3b-wb-claude-20261022.txt`, SHA-256
  `3d9035fa71c681f224bcddeee7872ecd6c3b61d1d27f59f55ae70ec904b32a71`.

## Границы

Не проверено: настоящий `RICON_CommsManagerComponent` с authority (адаптер исполнен только без менеджера), реальные
PTT и слушатели из игры, доставка снимков по сети, JIP, нагрузка 30/60 игроков, звук (`BLOCKED_NO_ACTUATOR`).
Дескриптор замираний P1b в снимок не входит: это следующая версия контракта (`REQ-channel-time.md`).
Этап закрывается после независимого ревью исходника и повторного прогона Workbench стороной codex.
