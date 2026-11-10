# RICON for Arma Reforger

<!-- VON_NATIVE_CODE_REVIEW_20261104_BEGIN -->
**Native code research, 2026-11-04:** the pinned Windows 1.8.0.13 VoN/radio dataflow, including incoming and outgoing environment, has been statically traced and independently reviewed. See the [readable graph](docs/analysis/VON_RADIO_DATAFLOW_RU.html) and [detailed source map](docs/analysis/VON_RADIO_DATAFLOW_RU.md). This closes the requested static research stage; player-content/runtime, per-pair actuation and production RICON integration remain separate open work.
<!-- VON_NATIVE_CODE_REVIEW_20261104_END -->

Wireless communications simulation platform for Arma Reforger: server-authoritative signal model (HDSP),
radio networks with towers and repeaters, and (later) cellular networks — on top of the built-in VoN voice.

Status: **pre-alpha with bounded runtime evidence** (2026-11-03). Workbench/host checks and local
listen-host + Diag-client experiments have run. A real RICON evaluator controls native generic-radio TX
admission to an exact same-world B transceiver, including wrong-frequency/model-LOST/recovery controls.
VoN AI input executes, but its A-proxy sound event survives detuning every discovered B transceiver;
it does not prove matched-frequency B radio reception. Player-voice content, playback, directed VoN
actuation and production authority remain unproven. See the [dated evidence](docs/evidence/E-agora-radio-native-20261103.md).

Start with the [documentation index](docs/INDEX_RU.md), [current status](docs/plan/STATUS_RU.md) and
[current work order](docs/plan/ROADMAP_RU.md). The approved order is documentation → full VoN-chain
analysis → VoN-radio mechanisms → qualified baseline → actuator → RICON integration → world physics,
lifecycle and load. “Full analysis” records native unknowns and their verification plan; it does not
turn a public API declaration into a runtime guarantee. [PLAN_RU.md](docs/plan/PLAN_RU.md) preserves
the agreed design baseline and dated decisions; it is not the current execution queue.

Canonical analyses: [VoN pipeline](docs/analysis/VON_PIPELINE_RU.md),
[32 radio influences](docs/analysis/VON_RADIO_MECHANISMS_RU.md), and the
[native baseline/control execution plan](docs/plan/PLAN_NATIVE_VON_RU.md). These source-backed
consolidations are SOURCE_COVERAGE_REVIEWED (Agora #6740/#6741); the native voice/output baseline remains open.

The newer [passive NRX fixture](docs/plan/STATUS_RU.md) passed two-role binding/catalog/lifecycle
observation and 39 synthetic checks on compile attempt 4. Capture and microphone input were never
activated; zero callbacks are not a player-radio/content/audio baseline. Source/sink gates remain open.

A later meter experiment passed 44 pure checks, but its runtime failed before B binding; the
meter-disabled control also failed and remained incomplete. Meter calibration and the player-media
baseline are NOT_READY. The later script-policy run reached PREPARED under the observed ADMIN branch
with unknown faction, but remained unarmed and never requested capture. Own-profile native Low was
verified on disk; memory savings were not measured. See the [current status and receipts](docs/plan/STATUS_RU.md).
Координатор применён; меню Plugins в Play и held-READ файл не дали квалифицированного пути активации.
Первый одиночный REST-опыт завершился timeout и сохранён отдельно. Новый опыт подтвердил callback
служебного токена за 684 ms: только single-host control PASS, не arm/capture/VoN/media.
NoPlay зафиксировал остановку собственного target и свежий baseline Off, но не restorative setter или native release.
Предыдущие guarded rehearsal и локальная аудиопроба сохранены как отдельные scopes. Новая короткая
native VoN проба дала A OnCapture и B own VoN OnReceive с локальным catalog match — только metadata candidate.
Отдельное no-send окно пропустило оба capture вызова и не записало raw callbacks; это finite control,
не доказательство глобального microphone-off. Последующие wrong-frequency и fresh matched пробы
теперь завершены, но серия остаётся только metadata pattern: переходный RX не имеет известной tuning,
null не означает DIRECT, RF/content/audio/per-pair PASS нет. Bounded parent text chain прочитана,
но effective Speaking Range и радио baseline остаются открытыми. Fixed-prefab numeric lookup завершился
INCOMPLETE без чтения числовых значений; MpTest text read нашёл ресурс, но не открыл его.
Позднее в Edit прочитаны список сущностей, границы объекта мира и пять высот поверхности; это не
проверка геометрии в игре или Speaking Range. Первый отказ placement сохранён; поздняя диагностика
выявила непринятое public-root представление MpTest. После точечного исправления пассивная проба
подтвердила конечные samples на 100 m: у A восемь, у B семь после первого sample около 1 m.
Это не voice radius или полная проверка standing/physics. Поздняя capture-серия со свежей геометрией
100 m завершена: Positive A/B 96/74, Wrong 98/1, Matched 97/88 — только metadata pattern.
Единственный Wrong RX пришёл после false, его tuning неизвестна; счётчики не измеряют packet loss.
Один известный short400 pilot дал A request/return span 409 ms и B callback span 392 ms; это только
feasibility-наблюдение. Позднее новое known-LONG окно B квалифицировано, а один blind trial дал
совпадение LONG: A actual 1005 ms, B span 1000 ms. При независимом равномерном бинарном выборе
случайное совпадение имеет вероятность 1/2; передача информации и надёжность этим не установлены.
Поздняя sound-проба после исправленного READY admission дала A 92 / B 86 radio-associated metadata,
но ни одного started/finished event в заранее подключённых own-B/A-proxy hooks в конечном окне.
Это не доказательство тишины или отсутствия native stream. Поздняя power-проба наблюдала локальный
OFF/ON cycle всей B рации с своевременным getter ON; исходный APPLIED_UNKNOWN сохранён отдельно.
Позднее controlled OFF дал A 94 / B RX 0, а fresh ON на том же коде — A 91 / B 81 exact metadata.
Это конечная metadata association, не RF-block, DIRECT/audio/per-pair PASS и не restore старого объекта.
Поздний explicit DIRECT опыт дал actual route getter перед capture, A 86 direct/TX-null и B 80
receiver-null rows в текущем own-B context — только combined metadata candidate, не proof по одному null.
Следующий шаг — короткая контрпроверка и один same-DIRECT + whole-B power OFF preservation control.
Core остаётся BLOCKED_NO_ACTUATOR; содержимое, playback и per-pair не квалифицированы.
RF/radio-only, содержимое, playback, per-pair и полный native voice baseline не приняты.
Квитанции и ограничения — в [STATUS](docs/plan/STATUS_RU.md).

## Layout

| Path | Purpose |
|---|---|
| `addons/RICON_Core/` | Reforger addon (Enforce Script): link evaluation, noise/antenna/intelligibility seams, snapshots, routing, comms manager |
| `addons/RICON_CoreTests/` | TEST_ONLY addon: compile fixtures for the five extension scenarios; not published |
| `oracle/` | host-side reference (Python stdlib): link-eval, snapshot and route vectors with runners (`ORACLE_PASS`) |
| `docs/req/` | requirements written from primary sources (link evaluation, core contracts, lifecycle) |
| `docs/review/` | reviews (core, API, compile readiness) |
| `docs/adr/` | architecture decision records (ADR-002: core contracts) |
| `docs/evidence/` | evidence notes with levels DOC/SOURCE/STD_COPY/…; `codex-are/` — immutable research snapshots (SHA-256 manifest); `WB-order-*.md` — Workbench requests |
| `docs/plan/` | agreed plan and live status (`STATUS_RU.md`) |

First Workbench compile: `docs/evidence/E-workbench-source.md` (orders `WB-order-003.md`/`004`).

## Rules

- Built-in VoN is the selected production transport. External/native extension research is a separate,
  explicitly scoped track; no client voice hook or installed DLL is qualified by the current evidence.
- Clean-room: Coalition-VON / Lima-Charlie are ideas-only (no license); TFAR (APL-SA) and ACRE2 (GPL-3.0) —
  ideas-only by project policy, no code or assets copied.
- Read installed vanilla scripts/assets through Workbench and distinguish public documentation, source, host,
  Enforce and native/runtime evidence. Native/binary research follows its explicit approved scope;
  a historical analysis or SDK license does not grant general permission for loading or modifying an engine.
- Preserve native DIRECT and user/platform mute. Per-pair radio effects require exact ownership,
  binding and independent output evidence; `OnReceive` or a proxy sound event alone is not playback.

The owner prohibits enabling, expanding or reconfiguring the pagefile as a workaround in this task.
Only a normal NVIDIA Overlay close was authorized; the signal was accepted but exit was not observed.
Other applications were not targeted. External memory release is not a measured saving from our actions.

## License

RICON-Reforger is licensed under the **Arma Public License Share Alike (APL-SA)** — see `LICENSE`
(official text: <https://www.bohemia.net/en/licenses/arma-public-license-share-alike>).

In short (the full text in `LICENSE` is binding):

- **Attribution** — credit "RICON-Reforger by Iosif2321" and link to this license;
- **Noncommercial** — no commercial use or monetization;
- **Arma Only** — the material may not be converted or adapted for games other than Arma;
- **Share Alike** — adaptations may be distributed only under APL-SA (or a listed compatible license).

This is a new, independent codebase. The earlier RICON for Arma 3 stays under GPL-3.0; no GPL code from it
was copied here — see `NOTICE.md` for provenance and the clean-room statement.
