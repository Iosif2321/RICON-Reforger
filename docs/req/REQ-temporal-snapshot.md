# CT10: temporal snapshot schema 2 — HOST contract candidate

2026-11-10. Status: VERIFIED_HOST_CANDIDATE; scoped review Agora #8246/#8249/#8250. Production migration remains OPEN.
Scope decision: Agora #8234. Author draft: Cellular System/output/ct10-contract-20261110/core/CONTRACT_DRAFT_RU.md.
This document specifies the new host API only. Snapshot v1, RCFD/1, Enforce and the network wire retain their existing meaning.

## Boundary and exact records

Integers are exact Python/JSON integers, excluding bool; all are int32. IDs and TTL are 1..2147483647;
epoch/seq are 1..2147483646; indices and times are 0..2147483647.
Keys, binary32 typing, finite numbers and descriptor structure reuse CF03/CD03 without coercion.
All records below have exactly the listed keys. No hidden v1 extension and no scalar fallback.

Snapshot2 fields:

| Fields | Domain |
|---|---|
| schemaVersion; descriptorVersion | Exactly 2; exactly 1 |
| sessionEpoch, linkSeq | epoch/seq |
| linkId, routeId, routeNodeCount | Direct-radio candidate: routeId=linkId, routeNodeCount=2 |
| ttlMs | Presentation TTL from accepted local receipt |
| talkerPlayerId, listenerPlayerId | Distinct IDs |
| state, quality | Existing diagnostic enum; binary32 [0,1]; LOST requires Q0, grants no mute permission |
| mode, timebaseId | SERVER or RECEIVER; key naming the physical model clock domain |
| binding | Exact record below |
| descriptor | Mandatory complete CF04 record, model FINITE_MULTIPATH_V1, not null |

Binding: leaseId, txGeneration, rxGeneration, txDeviceId, txTransceiverIndex, rxDeviceId,
rxTransceiverIndex, networkTypeId, channelId. IDs except nonnegative indices; distinct devices.
CF supports different transceivers of one device, but this existing radio seam boundary requires
distinct devices. Reverse direction is valid: canonical min(tx,rx)/max(tx,rx) must match descriptor a/b.

ExpectedLink is separate trusted caller input, with exact keys:
sessionEpoch, linkId, talkerPlayerId, listenerPlayerId, mode, timebaseId, binding, channel.
Channel is CF05 query without nowMs/subUs. It pins directed endpoints, frequency, bandwidth,
profile ID/version and environment revision. Comparison occurs after CF binary32 typing.
Matching caller JSON is synthetic host evidence, never proof of authentic RPC, rights or live lease.
Modes name a requested calculation location, not permission. Full producer/RX parity remains open.

Limits: maxSnapshots (1..65536), maxDescriptorBytes (1..65535),
maxTotalDescriptorBytes (1..INT_MAX). Empty batch is valid. Both batch and retained-state caps apply.
Descriptor budgets use existing RCFD/1 bytes, not outer packet bytes. No truncation.

## Admission, copying and publication

oracle/temporal_snapshot_oracle.py is separate from the legacy API. validate returns code plus
a detached normalized snapshot and descriptorBytes only on OK. Structural/scalar errors return
INVALID_SNAPSHOT or INVALID_DESCRIPTOR; unsupported schema/descriptor version/model/mode are explicit;
budget failure is RESOURCE_LIMIT; wrong trusted binding/RF/profile/revision/mode/timebase is CONTEXT_MISMATCH.
List/count limits are checked before traversal; rows are validated in input order and the first error wins.
All rows and retained budgets are checked before the sequence callback. This is not a new wire codec.

Publisher receives active candidates with IDs/epoch already assigned by a trusted caller.
Its registry is keyed by those IDs; it does not replace S3b pair registry, lease lifecycle or allocator.
Publish on first/forced call, directed context/state/TTL change, binary32 Q delta reaching qualityDelta,
half-TTL keepalive (ceil), or different canonical RCFD/1 descriptor bytes regardless of Q.
Signed zero weights normalize to +0, so they do not cause updates. Fast samples alone do not publish.
Successful sequence callback returns a newer seq in range. A reported callback failure leaves
host registry/output unchanged, returns NEED_FULL_RESYNC and counts already attempted calls;
external sequence effects cannot be rolled back. No epoch reads are implemented in this host boundary.
Every input, emitted snapshot, retained snapshot and public read result is detached.

Receiver reuses legacy ordering/TTL after v2 admission. Single accepted updates deepcopy state;
duplicates/older seq are STALE without TTL refresh. Initial single needs full resync.
A valid different epoch clears identity/slots and requests resync. Unsupported single version/model
keeps previous state within its own validity and returns an explicit reason plus REQUEST_RESYNC.
Invalid full resync clears identity and all slots, including old state; no valid prefix survives.
Full success commits all copies atomically, with strict header types. Full resync is a trusted control call:
same-epoch stale full replay protection requires a separate resync protocol, still OPEN.
read returns detached state; internal entries/legacy.slots are test inspections, not trusted ingress APIs.

Storage does not require current clock mapping or current physical applicability.
An expired or incomplete descriptor can be carried diagnostically; it does not become CLEAN/Q or mute.
Producer continuity, trustworthy revision/reason and physical changes remain an external gate.
This candidate accepts a structurally valid trusted descriptor update at the same revision (I01/I06);
it does not certify that a producer may create an unphysical phase jump.
Unchanged physical input across PTT/lease/epoch and exact CD06 rebase retain the same process.

## Independent presentation and physical sampling

visible uses local elapsed time: 0 <= nowLocalMs-receivedAtLocalMs < ttlMs.
view additionally requires a trusted ClockMapping and the whole error interval inside physical validity.
No mapping is needed to store a snapshot. Keepalive never extends anchor/validUntil or resets phases.

ClockMapping has exact keys: mappingId, timebaseId (keys); localAnchorMs, serverAnchorMs,
validFromLocalMs, validUntilLocalMs, maxErrorUs (nonnegative int32).
Require validFromLocalMs <= localAnchorMs < validUntilLocalMs and current local time within that window.
The candidate rate is exactly 1; adapter must conservatively cover drift/error over the declared window.

estimateUs = 1000*(serverAnchorMs+nowLocalMs-localAnchorMs)+subUs, subUs in 0..999.
The closed interval estimateUs +/- maxErrorUs must be in [0,INT_MAX*1000+999] and
[1000*validFromMs,1000*validUntilMs). Touching the right boundary fails.
Host intermediates are exact integers; a future Enforce adapter needs its own overflow proof.
Expired presentation returns PRESENTATION_EXPIRED; missing/bad mapping CLOCK_UNVERIFIED;
out-of-domain estimate CLOCK_OUT_OF_RANGE; physical interval overlap failure STALE_DESCRIPTOR.
Only eligible sampling calls existing CF.evaluate. EMPTY_MEDIUM, ZERO_AT_SAMPLE,
INCOMPLETE_COVERAGE, RESOURCE_EXHAUSTED and other CF reasons retain their own meanings.
Containment is necessary, but phase accuracy under nonzero clock error remains a separate CT06 gate.
Synthetic zero-error mapping does not prove an actual Reforger clock adapter.

## Verification and open migration

Independent manual scenarios and direct unittest assertions are authored by codex.team.review,
before reading this implementation. The original manual traces remain semantic source material;
direct tests bind their recorded values/branches to the host APIs, without generating expectations
from implementation. The executable tests and evidence must state their real coverage and gaps.

Still OPEN: trusted producer/world/RF inputs, transition/revision policy, shared RX and calibration,
server/receiver orchestration, S3b pair/lease/ID/end-event migration, same-epoch full replay protocol,
actual clock mapping, outer serialization/Rpl/transport sizing, coordinated Enforce consumers/generators/
fixtures/probes, compile/runtime/lifecycle/load, supported voice actuator, content/DSP/audio.
No new terminal LOST event is specified. ApplySnapshot remains BLOCKED_NO_ACTUATOR.
M1/M2/M3 and the full Goal stay active. Sources: REQ-channel-time CT04-CT10,
REQ-channel-fading CF03-CF09, REQ-channel-descriptor CD01-CD08, REQ-core-contracts section 8,
CT10-migration-map, full plan sections 1-21 and stages 0-8.
