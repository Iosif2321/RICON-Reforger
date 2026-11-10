# Большой native range как bounded delivery envelope

2026-11-04 · `codex.access.coverage` · **CONDITIONAL_STATIC_COVERAGE; ACTUAL_WORLD_COVERAGE_UNQUALIFIED**. Прочитаны полная attached objective и текущий temporal PLAN. Никаких Core/probe/framework/native/app изменений, активных передач, SDK/Ghidra/engine запусков. Рассматриваются реальные public-owned radio transceivers, не ранее отклонённый hidden-carrier range1 проект.

**Ответ A:** public SetRange может убрать **distance-only native drop** для заранее известной конечной области, если фактический server-side range покрывает все нужные native radio-owner origins с запасом. **65535 не доказывает охват неизвестной карты, высот, выходов за bounds или произвольного мира.** Это не квалификация client DSP/B и не полная интеграция.

## Условие покрытия и его предел

Пусть на всём проверяемом временном окне каждый нужный native source/receiver radio owner находится в объявленном конечном box со сторонами Lx,Ly,Lz. Тогда D=sqrt(Lx²+Ly²+Lz²) ограничивает native 3D расстояние. Для real geometry выбирается запас на native float arithmetic, coordinate uncertainty и изменение bounds, а не equality как эксплуатационный запас. Если фактический server R≥D+guard и1≤R≤65535, обычный RadioTransceiver не отбрасывает нужную пару **по расстоянию**. Точная машинная проверка — computed distance²≤computed R².

Основание: accepted native Radio6 `[1405E24A0,1405E26B4)`, source range загрузка5E24F6, XYZ gate5E2593..5E25E0; XZ broadphase сначала перечисляет50м cells прямоугольника origin±R, затем exact3D gate. Если d≤R, соответствующий cell попадает в этот broadphase при актуальной регистрации/grid state. d=R admitted, q=0; d>R отбрасывается. Unknown/nonfinite geometry не получает coverage PASS.

Дополнительные необходимые условия **не отменяются**: текущие source binding/права, user power/mute, native frequency/key eligibility, radio registration, concrete transceiver type, actor/generation и актуальный cache. Большой R не снимает их и не добавляет в native voice stream off-channel/защищённое содержимое. Модель энергии помехи отдельно может требовать данные об источниках без доступа к их речи. Relay/alternative native topology имеют собственные правила и не объявлены автоматически покрытыми обычным Radio6 proof.

Если хотя бы D+guard>65535, либо D не известен для всех radio-owner origins/altitudes/перемещений окна, **global world coverage остаётся UNQUALIFIED**. Нет нового «физического RICON запрета» на полезный вклад за65535м: это предел candidate envelope инструмента. Payload нельзя удалять по legacy LOST, Q0 или мгновенной неразборчивости вместо интервального cutoff-контракта temporal Goal.

## Envelope не является physical RICON profile

SetRange — один native source delivery scalar, не tx watts, receiver sensitivity или temporal channel. В RICON остаются настоящие физические TX/RX/антенны/профили/positions и shared receiver interference; большой R не подставляется в них как реальная дальность/мощность. План envelope, effective server readback и physical profile хранятся как разные понятия, без создания нового параллельного Core.

Public API: pinned `BaseTransceiver.c:46–53` объявляет SetRange(float), метры, owner/server и clamp1…65535. Actual setter `[1405E0410,1405E04B4)` сам не содержит role guard: local native field+40 write→range event→manager revision→sync. Поэтому использовать требуется проверенный authority binding, не считать proxy-local getter серверным эффектом. Source server field65535 при sync превращается binary16 в65504 у replica; математическая гарантия привязана к **действительному source instance**, которым сервер строит graph.

Другие штатные consumers GetRange и OnTansceiverRangeChanged также увидят новый scalar. Поэтому такое расширение не объявляется прозрачным для campaign coverage, UX/других модов и чужого управления устройством. Scoped ownership/восстановление и отсутствие competing writer — будущие обязательства применения, не реализованная здесь lease-обёртка.

## Native q и Direct всё ещё действуют

Native graph считает q=1−dXYZ²/R² (`5DFAAA..5DFAF6`), per-transceiver duplicate merge — max current-hop q. На известном D получается q_min=1−D²/R². «Большой range» даёт q≈1 **только при малом D/R**:

| Условный D при R=65535м | Математический q_min | Смысл |
|---:|---:|---|
| 10000м | ≈0.976716 | Не равен1 и не pristine bypass |
| 60000м | ≈0.161784 | Покрытие возможно, но native sound остаётся сильно q-зависимым |
| 65535м | 0 | Distance gate допускает; нулевая intelligibility/тишина из этого не следуют |

Для q_min≥1−ε нужно R≥D/sqrt(ε). При Rmax, q≥.99 требует D≤6553.5м; q≥.999 — D≤≈2072.4м. Эти числа не являются bound какой-либо фактической карты.

Native q передаётся в private TransmissionQuality и уже управляет Radio effects/noise/gain **до будущего custom receiver processing**. Даже native q=1 не означает необработанные отсчёты: vanilla имеет fixed radio HP/clipper/LP и ненулевой configured bitcrusher wet; common voice processing тоже сохраняется. Поздний RICON DSP не может просто восстановить уничтоженную предыдущей обработкой информацию. Следовательно envelope может снять range-drop, но **не доказывает сохранение исходных фрагментов в требуемой точке B** без отдельной квалификации/модернизации radio processing order.

VON_RADIO содержит Radio **и Direct** shader branches; VON_DIRECT — Direct. Большой radio R не меняет physical DIRECT discovery radius или actor positions, однако допустимая radio delivery может заменить direct metadata в общей listener record. Поэтому неизменность DIRECT/render и user state нельзя вывести из одного range readback или q≈1. Весь event mute не является radio-only решением.

## Cache и цена большого envelope

Changed range setter increments available RadioManager revision+148, которую voice cache проверяет перед повторным использованием. Unchanged setter не invalidate. Другие frequency/key/power/cell invalidators сохраняются. Source movement threshold=max(2м,.05R), при65535м —3276.75м; это только один invalidator, **не обещание** stale q на такую дистанцию и не measured update period. Требуется проверка актуальности registration/cell/cache при реальном применении; range update не возвращает уже потерянные прошлые кадры.

Static cost note:50м CellEnumerator не ограничивается числом найденных radios — перечисляет origin±R rectangle. Полный `[1407CE000,1407CE1BD)`: constant50 загружаетсяCE01D; пределы `floor((x±R)/50)` / `floor((z±R)/50)` считаютсяCE02D..CE0A9; запись каждой8-byte coordinate pair и inclusive nested-loop проверки находятсяCE169..CE192. Candidate query `[1407CE370,1407CE4CA)` вызывает enumerator поCE3A7, затем делает hash lookup для каждого pair поCE3D0..CE3E8, прежде чем читать radios.

Для65535м это2622/2623 cells на ось: **6 874 884…6 880 129 coordinate pairs на одно перечисление** до exact3D filter, в зависимости от alignment origin. Это code-derived work count, не measured latency/allocated bytes или performance verdict; cache/hops определяют число самих перечислений. Для малого известного test domain брать минимальный достаточный R с guard, а не вслепую stress65535. В рамках этой проверки запусков и actuator/probe-кода не добавлено.

## Единственный минимальный future capability test

После отдельного разрешения активного voice run: два обычных owned player radio endpoints, физические позиции и real source binding, согласованные key/frequency, user state неизменён. Объявить малую конечную test domain/altitude и исходный stock range R0; разместить B заR0 и вне подтверждённой на этом fixture DIRECT-дальности, но строго внутри выбранногоR1 с guard. Один и тот же реальный голос: положительный near baseline→stock radio negative внеR0→serverSetRange(R1)+actual readback/revision→свежий B-side native receiver/context **и независимое содержимое аудио**→restore R0/recovery controls. Отдельно зафиксировать q_native и DIRECT до/во время/после, чтобы не принять слышимость Direct shader за radio-only qualification. Нет здорового source/sink/binding — UNQUALIFIED; OnReceive один не audio PASS.

Этот test квалифицирует A только в указанной domain/конфигурации и не заменяет будущие full Goal tests A→B/A→C/A+C→B, temporal recoveries и RECEIVER/SERVER parity. Не переносить native/generic LOST admission gates в voice. **Порт B/private writer и управление общим receiver/DSP остаются отдельным обязательным результатом; ApplySnapshot=BLOCKED_NO_ACTUATOR не меняется.**

Evidence reused: canonical temporal PLAN §3/5/6; full attached objective; accepted radio/world contracts и audio `DSP_CURRENT.md`/current dataflow. Exact raw input hashes и математические assertions — `receipt.json`, воспроизводимый `check_envelope.py`. Новых callee requests —0; actual map/world bound не исследовался и не присвоен.
