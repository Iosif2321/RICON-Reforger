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
  контрольных порч сравнения; 3 доп. проверки (перестановка трёх массивов на P04, базовый источник и адаптер без
  менеджера fail-closed, NEED_FULL_RESYNC через такой адаптер).
- Генератор `tools/RICON_TestRunner/gen_link_publication_fixture.py` только копирует ожидания, закреплён за SHA
  корпуса; тест генератора — 8 случаев. Неизвестный `deviceCode` (C37) кодируется как −1 по REQ-LP §6.
- Общая регрессия: `regress_host.py` (+ CLI эталона S3b и `--check` генератора), `regress_wb.ps1` (+ пробник).

| Файл | SHA-256 |
|---|---|
| `RICON_LinkPublication.c` | `b3b5061a73937763feb7fb1991f8ad6d7938970a009ff79f98e536c41f078deb` |
| `RICON_LinkPublicationFixtureSupport.c` | `cf6fddc5b5be37f52af8c7e72661fbeaa8af3f95f4942220cbf07d7f77496c32` |
| `RICON_LinkPublicationFixtures.c` | `03d7b37832a986d6eb23dad568f41fd90fcf1dbf72348c3711cb87df097c45b0` |
| `RICON_LinkPublicationProbePlugin.c` | `6a8ad09c71d8ad3c0e0f3a172e8880ab4f3ac67260293bbabc944d06e59f0c95` |
| `gen_link_publication_fixture.py` | `a327f1efd2b3eccd1664fbdca0c3047b57238a51498023bc7e9f7e4811513c15` |

## Исполнение

База логов: `%USERPROFILE%/Documents/My Games/ArmaReforgerWorkbench/logs/`, файл `console.log`.

| Запуск | Итог | SHA-256 |
|---|---|---|
| `ricon-linkpub-claude-20261022-221016` (отдельный пробник) | exit 0, `cases=106 match=106 immutable=106 controls=15 extras=3 snapshots=37`, `done: PASS`, SCRIPT(E) 0 | `415d8f0a1a4ea3a8df7f5976f37514c703fe193de18d691cbf06608ab2de13fe` |
| `ricon-r29-claude-20261022-221754-RICON_LinkPublicationProbePlugin` (в полной регрессии) | то же | `d6653992067efd13522e1e96f925ac3fca60546e45bf8c90373fb0a05795fcd7` |

CRC в логе: `RICON_LinkPublication.c` 197aa68a, FixtureSupport 6846dc19, Fixtures 65c76868, Probe 1d84e440.
Отдельная сверка лога claude по каждому id с корпусом (код, число снимков, вызовы source, чтения epoch, флаг full):
106/106, помеченных FAIL нет. Первый запуск до этого (`…-220942`) не скомпилировался: остаток переименования
`next` → исправлен, затем PASS.

Общая регрессия на одном дереве:
- host: `TOTAL 30/30 PASS` — `docs/evidence/receipts/s3b-host-claude-20261022.txt`, SHA-256
  `4d2791f97369379814433c3b984d679266c9fa3cfbf89ef0f39a4822fda18b61`;
- Workbench: `TOTAL 11/11 PASS` — `docs/evidence/receipts/s3b-wb-claude-20261022.txt`, SHA-256
  `72f8ed0c3404201df57bdb8120f95ae032051a64279fb99ec52ab1521a0ace18`.

## Границы

Не проверено: настоящий `RICON_CommsManagerComponent` с authority (адаптер исполнен только без менеджера), реальные
PTT и слушатели из игры, доставка снимков по сети, JIP, нагрузка 30/60 игроков, звук (`BLOCKED_NO_ACTUATOR`).
Дескриптор замираний P1b в снимок не входит: это следующая версия контракта (`REQ-channel-time.md`).
Этап закрывается после независимого ревью исходника и повторного прогона Workbench стороной codex.
