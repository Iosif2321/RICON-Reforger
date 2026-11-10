//! SOURCE_CANDIDATE: captures committed publication addressing before Lifecycle events.
//! Pure journal: no clocks, RPC, sequence allocation or voice actions. Enforce compile NOT_RUN.
class RICON_TemporalCaptureInput
{
    ref RICON_TemporalSnapshot m_Snapshot;
    ref RICON_TemporalExpected m_Expected;
    int m_iReceiverEntityId;

    void RICON_TemporalCaptureInput(RICON_TemporalSnapshot snapshot, RICON_TemporalExpected expected, int receiverEntityId)
    {
        m_Snapshot = snapshot; m_Expected = expected; m_iReceiverEntityId = receiverEntityId;
    }
}

class RICON_TemporalCapturedLink
{
    ref RICON_TemporalSnapshot m_Snapshot;
    ref RICON_TemporalExpected m_Expected;
    int m_iDescriptorBytes;
    int m_iReceiverEntityId;
}

//! An intent is NOT a wire END: the producer must independently allocate a newer linkSeq.
class RICON_TemporalClosureIntent
{
    int m_iSessionEpoch, m_iLinkId, m_iCapturedLinkSeq, m_iTalkerPlayerId, m_iListenerPlayerId;
    ref RICON_TemporalBinding m_Binding;
    string m_sReason;

    RICON_TemporalClosureIntent CloneValidated()
    {
        RICON_TemporalClosureIntent c = new RICON_TemporalClosureIntent();
        c.m_iSessionEpoch = m_iSessionEpoch; c.m_iLinkId = m_iLinkId; c.m_iCapturedLinkSeq = m_iCapturedLinkSeq;
        c.m_iTalkerPlayerId = m_iTalkerPlayerId; c.m_iListenerPlayerId = m_iListenerPlayerId;
        c.m_Binding = m_Binding.CloneBinding(); c.m_sReason = m_sReason; return c;
    }

    RICON_TemporalEndRecord ToEnd(int assignedEndSeq)
    {
        if (!m_Binding || m_iCapturedLinkSeq <= 0 || m_iCapturedLinkSeq >= int.MAX
            || assignedEndSeq <= m_iCapturedLinkSeq || assignedEndSeq >= int.MAX) return null;
        RICON_TemporalEndRecord e = new RICON_TemporalEndRecord();
        e.m_iSessionEpoch = m_iSessionEpoch; e.m_iLinkId = m_iLinkId; e.m_iLinkSeq = assignedEndSeq;
        e.m_iTalkerPlayerId = m_iTalkerPlayerId; e.m_iListenerPlayerId = m_iListenerPlayerId;
        e.m_Binding = m_Binding.CloneBinding(); e.m_sReason = m_sReason;
        if (!e.IsValid()) return null;
        return e;
    }
}

class RICON_TemporalCaptureResult
{
    string m_sCode = "INVALID_INPUT";
    bool m_bRequiresFullResync;
    ref array<ref RICON_TemporalClosureIntent> m_aClosures = {};
}

class RICON_TemporalAuthorityJournal
{
    protected int m_iEpoch, m_iTotalBytes, m_iGlobalPrefix;
    protected ref RICON_TemporalLimits m_Limits;
    protected ref map<int, ref RICON_TemporalCapturedLink> m_Active = new map<int, ref RICON_TemporalCapturedLink>();
    protected ref map<int, ref RICON_TemporalClosureIntent> m_Closed = new map<int, ref RICON_TemporalClosureIntent>();
    protected string m_sConfiguration = "INVALID_INPUT";

    void RICON_TemporalAuthorityJournal(int epoch, RICON_TemporalLimits limits)
    {
        if (!limits || !limits.IsValid()) { m_sConfiguration = "INVALID_LIMITS"; return; }
        if (epoch <= 0 || epoch >= int.MAX) return;
        m_iEpoch = epoch; m_Limits = limits.CloneLimits(); m_sConfiguration = "OK";
    }

    static RICON_TemporalCaptureResult Result(string code, bool needsFull = false)
    {
        RICON_TemporalCaptureResult r = new RICON_TemporalCaptureResult();
        r.m_sCode = code; r.m_bRequiresFullResync = needsFull; return r;
    }

    bool MatchesLimits(RICON_TemporalLimits limits)
    {
        return m_sConfiguration == "OK" && limits && limits.IsValid()
            && limits.m_iMaxSnapshots == m_Limits.m_iMaxSnapshots
            && limits.m_iMaxDescriptorBytes == m_Limits.m_iMaxDescriptorBytes
            && limits.m_iMaxTotalDescriptorBytes == m_Limits.m_iMaxTotalDescriptorBytes;
    }

    int GetSessionEpoch() { return m_iEpoch; }
    int ActiveCount() { return m_Active.Count(); }
    int PendingCount() { return m_Closed.Count(); }
    int TotalDescriptorBytes() { return m_iTotalBytes; }
    int GetCaptureLimit()
    {
        if (m_sConfiguration != "OK") return 0;
        return m_Limits.m_iMaxSnapshots;
    }

    RICON_TemporalSnapshot ReadActive(int linkId)
    {
        RICON_TemporalCapturedLink row = m_Active.Get(linkId);
        if (!row) return null;
        return row.m_Snapshot.CloneValidated();
    }

    RICON_TemporalClosureIntent ReadPending(int linkId)
    {
        RICON_TemporalClosureIntent row = m_Closed.Get(linkId);
        if (!row) return null;
        return row.CloneValidated();
    }

    //! Caller must be the actual committed producer; these DTO checks alone do not authenticate it.
    RICON_TemporalCaptureResult CapturePublished(RICON_TemporalSnapshot snapshot, RICON_TemporalExpected expected, int receiverEntityId)
    {
        array<ref RICON_TemporalCaptureInput> batch = {new RICON_TemporalCaptureInput(snapshot, expected, receiverEntityId)};
        return CapturePublishedBatch(batch);
    }

    //! Validate against the original journal once; no capacities or live writes in row admission.
    protected RICON_TemporalCaptureResult PrepareCaptureInput(RICON_TemporalCaptureInput input,
        out RICON_TemporalCapturedLink next, out bool duplicate)
    {
        next = null; duplicate = false;
        if (!input) return Result("INVALID_INPUT");
        if (input.m_iReceiverEntityId <= 0) return Result("INVALID_RECEIVER_ENTITY");
        RICON_TemporalValidationResult valid = RICON_TemporalSnapshots.Validate(input.m_Snapshot, input.m_Expected, m_Limits);
        if (valid.m_sCode != "OK") return Result(valid.m_sCode);
        RICON_TemporalSnapshot s = valid.m_Snapshot;
        if (s.m_iSessionEpoch != m_iEpoch) return Result("SCOPE_MISMATCH", true);
        RICON_TemporalCapturedLink old = m_Active.Get(s.m_iLinkId);
        if (m_Closed.Contains(s.m_iLinkId) || (!old && s.m_iLinkId <= m_iGlobalPrefix)) return Result("CLOSED_ID", true);
        if (old)
        {
            RICON_TemporalSnapshot p = old.m_Snapshot;
            if (old.m_iReceiverEntityId != input.m_iReceiverEntityId) return Result("ENTITY_REBIND_REQUIRES_NEW_LINK", true);
            if (!p.m_Binding.Same(s.m_Binding) || p.m_iTalkerPlayerId != s.m_iTalkerPlayerId
                || p.m_iListenerPlayerId != s.m_iListenerPlayerId) return Result("BINDING_MISMATCH", true);
            if (s.m_iLinkSeq < p.m_iLinkSeq) return Result("STALE");
            if (s.m_iLinkSeq == p.m_iLinkSeq)
            {
                if (!RICON_TemporalProtocolRules.SameSnapshot(p, s)) return Result("LINK_SEQUENCE_CONFLICT", true);
                duplicate = true;
            }
        }
        next = new RICON_TemporalCapturedLink();
        next.m_Snapshot = s; next.m_Expected = input.m_Expected.CloneValidated(); next.m_iDescriptorBytes = valid.m_iDescriptorBytes;
        next.m_iReceiverEntityId = input.m_iReceiverEntityId;
        if (duplicate) return Result("DUPLICATE");
        return Result("OK");
    }

    //! Local capture transaction only, not a producer registry commit or a source checkpoint.
    RICON_TemporalCaptureResult CapturePublishedBatch(array<ref RICON_TemporalCaptureInput> batch)
    {
        if (m_sConfiguration != "OK") return Result(m_sConfiguration);
        if (!batch) return Result("INVALID_INPUT");
        if (batch.Count() > m_Limits.m_iMaxSnapshots) return Result("BATCH_LIMIT");
        if (batch.Count() == 0) return Result("OK");
        set<int> seen = new set<int>();
        array<ref RICON_TemporalCapturedLink> changed = {};
        int incoming, removed, newLinks;
        foreach (RICON_TemporalCaptureInput input : batch)
        {
            if (input && input.m_Snapshot && input.m_Snapshot.m_iLinkId > 0 && seen.Contains(input.m_Snapshot.m_iLinkId))
                return Result("DUPLICATE_LINK");
            RICON_TemporalCapturedLink next;
            bool duplicate;
            RICON_TemporalCaptureResult admitted = PrepareCaptureInput(input, next, duplicate);
            if (admitted.m_sCode != "OK" && admitted.m_sCode != "DUPLICATE") return admitted;
            int link = next.m_Snapshot.m_iLinkId;
            seen.Insert(link);
            if (next.m_iDescriptorBytes > m_Limits.m_iMaxTotalDescriptorBytes - incoming)
                return Result("RESOURCE_LIMIT", true);
            incoming += next.m_iDescriptorBytes;
            if (duplicate) continue;
            RICON_TemporalCapturedLink old = m_Active.Get(link);
            if (old) removed += old.m_iDescriptorBytes;
            else newLinks++;
            changed.Insert(next);
        }
        if (changed.Count() == 0) return Result("DUPLICATE");
        if (m_Active.Count() + m_Closed.Count() + newLinks > m_Limits.m_iMaxSnapshots)
            return Result("RESOURCE_LIMIT", true);
        // Subtract every replaced old row before adding any new row; the final budget is order-independent.
        int retained = m_iTotalBytes - removed;
        foreach (RICON_TemporalCapturedLink replacement : changed)
        {
            if (replacement.m_iDescriptorBytes > m_Limits.m_iMaxTotalDescriptorBytes - retained)
                return Result("RESOURCE_LIMIT", true);
            retained += replacement.m_iDescriptorBytes;
        }
        map<int, ref RICON_TemporalCapturedLink> candidate = new map<int, ref RICON_TemporalCapturedLink>();
        for (int i = 0; i < m_Active.Count(); i++)
        {
            int heldId = m_Active.GetKey(i);
            candidate.Set(heldId, m_Active.Get(heldId));
        }
        foreach (RICON_TemporalCapturedLink row : changed) candidate.Set(row.m_Snapshot.m_iLinkId, row);
        m_Active = candidate; m_iTotalBytes = retained;
        return Result("OK");
    }

    protected RICON_TemporalClosureIntent MakeIntent(RICON_TemporalSnapshot s, string reason)
    {
        RICON_TemporalClosureIntent c = new RICON_TemporalClosureIntent();
        c.m_iSessionEpoch = s.m_iSessionEpoch; c.m_iLinkId = s.m_iLinkId; c.m_iCapturedLinkSeq = s.m_iLinkSeq;
        c.m_iTalkerPlayerId = s.m_iTalkerPlayerId; c.m_iListenerPlayerId = s.m_iListenerPlayerId;
        c.m_Binding = s.m_Binding.CloneBinding(); c.m_sReason = reason; return c;
    }

    protected bool AddIntent(map<int, ref RICON_TemporalClosureIntent> plan, RICON_TemporalSnapshot s, string reason)
    {
        RICON_TemporalClosureIntent prior = plan.Get(s.m_iLinkId);
        if (!prior) { plan.Set(s.m_iLinkId, MakeIntent(s, reason)); return true; }
        if (prior.m_sReason == reason || reason == "LINK_REMOVED") return true;
        if (prior.m_sReason == "LINK_REMOVED") { prior.m_sReason = reason; return true; }
        return false;
    }

    //! preEventEpoch comes from the real Lifecycle BEFORE its call, never from its final epoch.
    //! LEASE_TERMINATED has no meaningful epoch: join the adjacent VON_EXPIRY_INTENDED.
    RICON_TemporalCaptureResult ConsumeLifecycleResult(RICON_LifecycleResult result, int preEventEpoch)
    {
        if (m_sConfiguration != "OK") return Result(m_sConfiguration);
        if (preEventEpoch != m_iEpoch) return Result("SCOPE_MISMATCH", true);
        if (!result || !result.m_aEffects) return Result("INVALID_EFFECTS", true);
        if (result.m_aEffects.Count() > 65536) return Result("RESOURCE_LIMIT", true);
        map<int, ref RICON_TemporalClosureIntent> plan = new map<int, ref RICON_TemporalClosureIntent>();
        bool needsFull;
        // Do not gate on resultCode: BeginEvent can sweep before an error/duplicate response.
        for (int i = 0; i < result.m_aEffects.Count(); i++)
        {
            RICON_LifecycleEffect effect = result.m_aEffects[i];
            if (!effect || effect.m_eType < RICON_ELifecycleEffectType.PLAYER_STATE_CHANGED
                || effect.m_eType > RICON_ELifecycleEffectType.FULL_RESYNC_REQUIRED) return Result("INVALID_EFFECTS", true);
            if (effect.m_eType == RICON_ELifecycleEffectType.LEASE_TERMINATED)
            {
                if (i + 1 >= result.m_aEffects.Count()) return Result("INVALID_EFFECTS", true);
                RICON_LifecycleEffect expiry = result.m_aEffects[i + 1];
                string reason = RICON_TemporalEndRecord.LeaseReasonName(effect.m_eTerminalReason);
                if (!expiry || expiry.m_eType != RICON_ELifecycleEffectType.VON_EXPIRY_INTENDED
                    || expiry.m_iSessionEpoch != preEventEpoch || effect.m_iPlayerId <= 0
                    || effect.m_iGeneration <= 0 || effect.m_iDeviceId <= 0 || effect.m_iLeaseId <= 0
                    || expiry.m_iPlayerId != effect.m_iPlayerId || expiry.m_iDeviceId != effect.m_iDeviceId
                    || expiry.m_iLeaseId != effect.m_iLeaseId || reason.IsEmpty()
                    || (effect.m_eLeaseState != RICON_ETransmitLeaseState.EXPIRED && effect.m_eLeaseState != RICON_ETransmitLeaseState.REVOKED))
                    return Result("INVALID_EFFECTS", true);
                for (int a = 0; a < m_Active.Count(); a++)
                {
                    RICON_TemporalSnapshot s = m_Active.GetElement(a).m_Snapshot;
                    if (s.m_iTalkerPlayerId == effect.m_iPlayerId && s.m_Binding.m_iTxGeneration == effect.m_iGeneration
                        && s.m_Binding.m_iTxDeviceId == effect.m_iDeviceId && s.m_Binding.m_iLeaseId == effect.m_iLeaseId)
                        if (!AddIntent(plan, s, reason)) return Result("CONFLICTING_EFFECTS", true);
                }
                i++;
            }
            else if (effect.m_eType == RICON_ELifecycleEffectType.VON_EXPIRY_INTENDED)
                return Result("INVALID_EFFECTS", true);
            else if (effect.m_eType == RICON_ELifecycleEffectType.ENTITY_UNBOUND)
            {
                if (effect.m_iPlayerId <= 0 || effect.m_iGeneration <= 0 || effect.m_iEntityId <= 0)
                    return Result("INVALID_EFFECTS", true);
                for (int entityIndex = 0; entityIndex < m_Active.Count(); entityIndex++)
                {
                    RICON_TemporalCapturedLink addressed = m_Active.GetElement(entityIndex);
                    RICON_TemporalSnapshot incoming = addressed.m_Snapshot;
                    if (incoming.m_iListenerPlayerId == effect.m_iPlayerId
                        && incoming.m_Binding.m_iRxGeneration == effect.m_iGeneration
                        && addressed.m_iReceiverEntityId == effect.m_iEntityId)
                        if (!AddIntent(plan, incoming, "LINK_REMOVED")) return Result("CONFLICTING_EFFECTS", true);
                }
            }
            else if (effect.m_eType == RICON_ELifecycleEffectType.SUBSCRIPTIONS_CLEARED)
            {
                if (effect.m_iPlayerId <= 0 || effect.m_iGeneration <= 0) return Result("INVALID_EFFECTS", true);
                for (int b = 0; b < m_Active.Count(); b++)
                {
                    RICON_TemporalSnapshot rx = m_Active.GetElement(b).m_Snapshot;
                    if (rx.m_iListenerPlayerId == effect.m_iPlayerId && rx.m_Binding.m_iRxGeneration == effect.m_iGeneration)
                        if (!AddIntent(plan, rx, "LINK_REMOVED")) return Result("CONFLICTING_EFFECTS", true);
                }
            }
            else if (effect.m_eType == RICON_ELifecycleEffectType.FULL_RESYNC_REQUIRED)
            {
                if (effect.m_iPlayerId <= 0 || effect.m_iSessionEpoch <= 0 || effect.m_iSessionEpoch >= int.MAX)
                    return Result("INVALID_EFFECTS", true);
                needsFull = true;
            }
            // ENTITY_UNBOUND pins the old entity; CARRIED_DEVICES_DETACHED alone has no device/entity list.
        }
        RICON_TemporalCaptureResult accepted = Result("OK", needsFull);
        array<int> ids = {};
        for (int key = 0; key < plan.Count(); key++) ids.Insert(plan.GetKey(key));
        ids.Sort();
        // Entire source result was validated before any journal mutation.
        foreach (int linkId : ids)
        {
            RICON_TemporalCapturedLink held = m_Active.Get(linkId);
            RICON_TemporalClosureIntent intent = plan.Get(linkId);
            m_iTotalBytes -= held.m_iDescriptorBytes;
            m_Active.Remove(linkId); m_Closed.Set(linkId, intent);
            accepted.m_aClosures.Insert(intent.CloneValidated());
        }
        return accepted;
    }

    //! INTERNAL producer FULL-STATE handoff for ALL owners, not a per-owner network FULL or client ACK.
    //! Every committed publication must already be captured; the real producer/exporter proves this.
    RICON_TemporalCaptureResult CommitSourceCheckpoint(int epoch, int highWater, array<int> activeIds)
    {
        if (m_sConfiguration != "OK") return Result(m_sConfiguration);
        if (epoch != m_iEpoch || highWater < 0 || !activeIds) return Result("INVALID_INPUT");
        if (activeIds.Count() > m_Limits.m_iMaxSnapshots) return Result("RESOURCE_LIMIT", true);
        if (highWater < m_iGlobalPrefix) return Result("HIGH_WATER_ROLLBACK", true);
        set<int> supplied = new set<int>();
        foreach (int id : activeIds)
        {
            if (id <= 0 || id > highWater || supplied.Contains(id)) return Result("INVALID_CHECKPOINT", true);
            supplied.Insert(id);
        }
        int count;
        for (int a = 0; a < m_Active.Count(); a++)
        {
            RICON_TemporalSnapshot s = m_Active.GetElement(a).m_Snapshot;
            if (s.m_iLinkId > highWater || !supplied.Contains(s.m_iLinkId)) return Result("INVALID_CHECKPOINT", true);
            count++;
        }
        if (count != supplied.Count()) return Result("INVALID_CHECKPOINT", true);
        array<int> closedIds = {};
        for (int c = 0; c < m_Closed.Count(); c++)
        {
            RICON_TemporalClosureIntent pending = m_Closed.GetElement(c);
            if (pending.m_iLinkId > highWater) return Result("HIGH_WATER_ROLLBACK", true);
            closedIds.Insert(pending.m_iLinkId);
        }
        m_iGlobalPrefix = highWater;
        foreach (int closedId : closedIds) m_Closed.Remove(closedId);
        return Result("OK");
    }
}
