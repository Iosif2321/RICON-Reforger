# RICON + native VoN/Radio integration, 2026-11-04

Owner keeps original product goals and asks to find a sound integration that reuses/extends/modernizes standard Reforger VoN/Radio. This is now integration feasibility/design, not repetition of the completed full RE. No game, Workbench, engine, server, capture, debugger attach or live-network tests. No binary patches/DLL installs/OS or pagefile changes. Static Ghidra is still allowed if one exact missing fact requires it; root remains the sole heavy-process operator.

Canonical repo: C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger. Write only your assigned subdirectory in C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/Cellular System/output/ricon-von-integration-20261104. You are not alone; preserve others' edits. GPT6.1 Sol High and Agora topic ricon-reforger-plan, sender codex.integration.<role>. Call protocol before Agora; latest owner task supersedes its historical planning-only text. Follow AGENTS_ACCELERATION_RU.md: one concrete question, one evidence artifact, no busywork/approval queue.

Re-use completed source proof: canonical docs/analysis/VON_RADIO_DATAFLOW_RU.md; research output sibling von-re-20261104/{radio,audio,transport,lifecycle,index}. Raw native exports still TEMP/ricon-von-re-20261104 and durable canonical .re/notes/native-von-20261104. Game EXE1.8.0.13 SHA1d221a990fa16c78fe02f685a7ba1330be115ca93cd3b641e67e5d3f1b40d9c9. No routine repeat indexing/Ghidra/whole source scans. Distinguish original native ability, supported Workshop/script/asset access, and unqualified private-code interventions.

Required outcome: native capture/Opus/UDP/owner checks/decoder remain usable; RICON supplies actual per-pair route/quality/lifetime, radio-only scope, DIRECT and other senders preserved. Native server recipient selection and client per-source quality are separate capabilities. Whole-radio/transceiver changes aren't a per-pair solution. OnReceive is after audio processing (or muted bypass); OnVoNUsed is at end of SendVoiceData after recipient map dispatch (batch02 VoN_RadioGraphCaller.c at end). A callback name/presence cannot become a pre-gate hook.

Already implemented RICON: PairService/Evaluator/Device observations, LinkPublication/SnapshotReceiver/Lifecycle. RICON_VoNIntegration.ApplySnapshot remains BLOCKED_NO_ACTUATOR; don't change it merely for a green result. Native shadow stage output/ricon-native-shadow-integration-staging is source-reviewed/offline27, SDK/native NOT_RUN, canonical not applied. Don't duplicate core registries/oracles/adapters.

Explore independent candidate routes, not one assumed answer. Explicitly test A->B blocked/degraded while C->B same receiver and A->C healthy, plus A DIRECT near B before/during/after. Include cache invalidation, single listener record overwrite, VON_RADIO contains both Direct+Radio shaders, user mute/power ownership, missing actor and stale generation. Don't use GM/editor fakery or reinterpret real Interior/RoomSize to smuggle RICON q. No external voice backend.

Write readable Russian. Each candidate needs exact source/public API and native evidence, what can be reused, supported access, actual scope, counterexample/limit, smallest future capability test and success/failure criterion. End with a concrete recommended next step. No speculative production adapters yet.


<!-- RICON_KNOWLEDGE_GRAPH_20261107 -->
## Навигация Agora по графу, 2026-11-07

Историческая рабочая запись выше сохраняется. Актуальная [анкета агента](../../../AGENT_CONTEXT_RU.md), [граф](../../analysis/RICON_KNOWLEDGE_GRAPH_RU.md) и [процесс Agora](AGORA_GRAPH_WORKFLOW_RU.md) связывают problem ID, owning source, scope доказательства, исполнителя и отдельного reviewer. Новое прямое поручение владельца определяет разрешённый scope; граф сам разрешений не выдаёт. Для текущей документальной задачи runtime-код и постоянный DSH/Host не менялись.

Documentation / Картотека (`codex.documentation` в текущем выполнении) владеет только назначенной Lead картой задач/решений, графом, ссылками и документами. Проверенные evidence передаются с receipt и границами результата; отдельный reviewer проверяет кандидаты, Lead/root публикует canonical изменения. Native-операции остаются за root. Постоянный Host member `35cd977b-25ba-453e-965e-3234da01739d` («Картотека / Документация») — внешняя ссылка на состав команды, не утверждение о запуске DSH session; профиль, модель и история не изменяются.
