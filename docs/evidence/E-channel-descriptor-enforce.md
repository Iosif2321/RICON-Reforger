# CT10.1: Enforce-кодек RCFD/1 и Rebase совпадают с замороженным корпусом

Дата: 2026-10-23. Автор кода и стенда: claude. Статус: **ожидает ревью codex** (исходник и повтор пробника).

Уровень: `ENFORCE_PARITY`. Код исполнен в Workbench без мира, Play и сервера; это не Rpl-доставка, не сериализация
движка, не клиентские часы и не голос.

Основание:
- REQ-channel-descriptor v0.1 AGENT-REVIEWED (codex, freeze 2483033);
- корпус `channel_descriptor_v1.json` заморожен по LF SHA `95208dd198cee4680763e6813cb66374832ef241ea1b5590e20799d834f2aa27`
  (agora #3632, #3634);
- до кода claude пересчитал корпус слепым checker по REQ: 54/54, goldenHex, композиция Rebase, continuity 54,
  переворот байтов 48/48 (`f1d338e`, `53185a6`).

## Что сделано

`addons/RICON_Core/Scripts/Game/RICON/Propagation/RICON_ChannelDescriptor.c`:
- `RICON_EDescriptorCode`, значение 0 — INVALID_LIMITS;
- `RICON_DescriptorCodec.Encode`, `Decode`, `Rebase`, `Clone`. Порядок проверок — CD04–CD06; при отказе результат
  содержит только код;
- DTO переиспользован из P1b (`RICON_FadingDescriptor`). Проверка descriptor — общий
  `RICON_FiniteMultipath.ValidDescriptor`, выделенный из прежнего `Valid` без изменения порядка проверок;
- binary32 ↔ IEEE-биты — точным масштабированием на степени 2, без внешних API. Ноль любого знака даёт +0 (CD03);
  экспонента 255 → отказ кадра;
- big-endian int32 собирается как (b0 − 256)·2^24 + … без переполнения;
- ключи пакуются через `ToAscii`, распаковываются через алфавит CF и `Substring`; прочие коды → INVALID_FRAME;
- каноничность Decode — побайтовое сравнение с повторной упаковкой, поэтому вес −0 даёт INVALID_FRAME;
- Rebase: tQ = MulMod((t − anchor) mod 2^30, 1000), фазы через `PhaseAt`; anchor и validFrom := t,
  validUntil сохраняется.

Production-вызовов нет; snapshot v1, producer, receiver и seam не изменены.

Стенд `RICON_ChannelDescriptorFixtureSupport.c`:
- код, descriptor (все поля строго), байты и размер сравниваются строго;
- проверяется неизменность входа; второй вызов даёт новый объект и после порчи первого результата;
- результат не делит descriptor, концы, массив и составляющие с входом;
- 8 контрольных порч сравнения на D01;
- 3 дополнительные проверки:
  1. арифметика: битовые образы 1.0, 150, 0.1, FLT_MAX, 2^-149, 2^-126, 0; Inf и NaN отвергнуты; −1.0;
     int32 0, −1, ±граница, −12345; корневой goldenHex декодируется и пакуется обратно байт в байт;
  2. кадры: round trip всех 12 OK-кадров и переворот младшего бита в байтах 0, 4, 6 и последнем — 48 кадров, ни
     один не дал прежний descriptor;
  3. Rebase (D42): композиция Rebase(Rebase(D, t1), t2) == Rebase(D, t2) на 4 парах; ядро P1b даёт одинаковые
     фазы и коэффициент для старого и нового descriptor при subUs 0/1/999.

Генератор `gen_channel_descriptor_fixture.py`:
- закреплён за LF SHA, 46 native-случаев;
- host-only список совпадает с полем `hostOnly`: D17–D21, D31, D47, D54;
- длинные кадры пишутся hex-строкой, которую разбирает стенд: литерал массива в 5066 элементов превысил лимит
  Enforce «Too many instructions per function».

Тест генератора — 5 случаев. Регрессия: `regress_host.py` (+ слепой checker, + `--check` генератора),
`regress_wb.ps1` (+ пробник).

SHA-256 по LF-содержимому:

| Файл | SHA-256 |
|---|---|
| `RICON_ChannelDescriptor.c` | `cb077b44f5debc9a132c2a21f29ea32de05d5718ef1a14ca876488b010428343` |
| `RICON_ChannelFading.c` (ValidDescriptor) | `21bc98df6713891da5be9dd1719b183be6c77e13d9741422d9a2b001832e9992` |
| `RICON_ChannelDescriptorFixtureSupport.c` | `c3ed3cb1d062886f1bf9374d0f5b680384ec2e4b29bfc6cab1ae87da3dc56ddb` |
| `RICON_ChannelDescriptorFixtures.c` | `9181da803e69054e4d88aa1d825942230ebddb7a37b898bec46a9bf4f1a4cbed` |
| `RICON_ChannelDescriptorProbePlugin.c` | `7136f975e194157779d24ac73644a3d222f43ba5c3ae4df0c3245f276917a514` |
| `gen_channel_descriptor_fixture.py` | `23934963e9d75563f34840754a82dc025386b08f2b22681d74b967c31bac78c9` |
| `test_gen_channel_descriptor_fixture.py` | `6ab458ab31ed3a4bffdf6ed683040ac1218ac4dbede31dadd65fdd3d1d9b2700` |
| `receipts/ct10-1-log-check-claude.py` | `523ec1ff22c114daf10ac4d9fbd69f37fa439aa79d8f43506126260420ef9603` |

## Исполнение

База логов: `%USERPROFILE%/Documents/My Games/ArmaReforgerWorkbench/logs/`, `console.log`.

| Запуск | Итог | SHA-256 |
|---|---|---|
| `ricon-descriptor-claude-20261023-162406` | не скомпилирован модуль Game: в дереве лежала незаконченная P4.2 fixture partner (`Unknown type RICON_OvercanopyRun`); codex её убрал (#3647) | `14282cd409401f0d0aa0ece923e8666f5e183a337549e617b4a894d518ee7951` |
| `ricon-descriptor-claude-20261023-162523` | мой дефект: «Too many instructions per function» на литерале кадра 5066 байт | `1250b06933eb40bb40c5f034cee06170012536f0b203bbf1575fe169beaf97fe` |
| `ricon-descriptor-claude-20261023-162623` | exit 0, SCRIPT(E) 0, один done: PASS; `cases=46 match=46 immutable=46 controls=8 extras=3 hostOnly=8`; frames 12, flips 48 | `7ae8d947bda453059d6bd9079f4779acc180310765fe6dd96d9b7556657d87c1` |
| `ricon-fading-claude-20261023-162642` (P1b после выделения ValidDescriptor) | 65/65, controls 9, extras 3, PASS; `P1B_LOG_CHECK_PASS` | `81589929dafc484b3602b1faab3b94005622f8cd2a6cfa8beb09d44c05dd0719` |

CRC в финальном логе: ChannelDescriptor 5e90176a, ChannelFading c868ed4d, Support a4103485, Fixtures 40517a57,
Probe 5846e73b.

Независимая проверка лога: `python -I docs/evidence/receipts/ct10-1-log-check-claude.py <log>
oracle/vectors/channel_descriptor_v1.json` → `CT10_1_LOG_CHECK_PASS: 46 native, host-only 8 absent, frames 12, SCRIPT(E) 0`.

Host: `TOTAL 44/44 PASS` — `docs/evidence/receipts/ct10-1-host-claude-20261023.txt`, SHA-256 `d80f57c2b4d5c153ec19a7d32ed5b033e2868b32503b57595ba47f506fd1f8b6` (LF).

## Границы

Не сделано и не заявляется:
- Rpl/RPC-доставка и ScriptBitWriter/Reader;
- реальный порядок байтов и накладные расходы движка;
- адресность и происхождение descriptor;
- миграция schema 2 всех потребителей (CT10);
- clock mapping (CT06);
- непрерывность при смене физики;
- голос.
