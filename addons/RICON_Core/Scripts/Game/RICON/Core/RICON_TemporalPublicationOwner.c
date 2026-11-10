//! STAGING manager-owned source dataflow. Pure tests do not prove native manager/world/RPC authority.
class RICON_TemporalPublicationOwner : RICON_TemporalAuthorityJournal
{
    protected ref RICON_TemporalProducerRegistry m_Registry = new RICON_TemporalProducerRegistry();
    protected ref RICON_LinkPublicationLimits m_PublicationLimits;
    protected int m_iReservedHighWater;
    protected bool m_bProducerNeedsFull = true;
    protected bool m_bPublishing;
    protected bool m_bInterference;
    protected string m_sProducerConfiguration = "INVALID_LIMITS";

    void RICON_TemporalPublicationOwner(int epoch, RICON_TemporalLimits limits, RICON_LinkPublicationLimits publicationLimits)
    {
        m_Registry.m_iSessionEpoch = epoch;
        if (m_sConfiguration == "OK" && RICON_TemporalPublicationService.ValidPublicationLimits(publicationLimits))
        {
            m_PublicationLimits = RICON_TemporalPublicationService.CopyPublicationLimits(publicationLimits);
            m_sProducerConfiguration = "OK";
        }
    }
    int GetReservedHighWater() { return m_iReservedHighWater; }
    bool ProducerNeedsFull() { return m_bProducerNeedsFull; }
    RICON_TemporalProducerRegistry ReadProducerRegistry() { return m_Registry.CloneRegistry2(); }

    override RICON_TemporalCaptureResult CapturePublished(RICON_TemporalSnapshot snapshot, RICON_TemporalExpected expected, int receiverEntityId)
    {
        return Result("PRODUCER_OWNER_REQUIRED");
    }
    override RICON_TemporalCaptureResult CapturePublishedBatch(array<ref RICON_TemporalCaptureInput> batch)
    {
        return Result("PRODUCER_OWNER_REQUIRED");
    }
    override RICON_TemporalCaptureResult CommitSourceCheckpoint(int epoch, int highWater, array<int> activeIds)
    {
        return Result("PRODUCER_OWNER_REQUIRED");
    }
    override RICON_TemporalCaptureResult ConsumeLifecycleResult(RICON_LifecycleResult result, int preEventEpoch)
    {
        if (m_bPublishing)
        {
            m_bInterference = true;
            m_bProducerNeedsFull = true;
            return Result("PRODUCER_BUSY", true);
        }
        RICON_TemporalCaptureResult accepted = super.ConsumeLifecycleResult(result, preEventEpoch);
        if (accepted.m_bRequiresFullResync)
        {
            m_bProducerNeedsFull = true;
        }
        return accepted;
    }
    protected RICON_TemporalPublicationResult PublicationResult(string code, int calls = 0, int reads = 0, bool recovery = false)
    {
        RICON_TemporalPublicationResult result = new RICON_TemporalPublicationResult();
        result.m_sCode = code;
        result.m_iSessionEpoch = m_iEpoch;
        result.m_iSequenceCalls = calls;
        result.m_iEpochReads = reads;
        result.m_iReservedHighWater = m_iReservedHighWater;
        result.m_bRequiresFullResync = recovery;
        return result;
    }
    protected RICON_TemporalPublicationResult IssuanceFailure(int calls, int reads)
    {
        m_bProducerNeedsFull = true;
        return PublicationResult("NEED_FULL_RESYNC", calls, reads, true);
    }
    protected bool ExpectedForLease(RICON_TransmissionBinding binding)
    {
        for (int i = 0; i < m_Closed.Count(); i++)
        {
            RICON_TemporalClosureIntent intent = m_Closed.Get(m_Closed.GetKey(i));
            RICON_TemporalBinding b = intent.m_Binding;
            if (b.m_iLeaseId == binding.m_iLeaseId && intent.m_iTalkerPlayerId == binding.m_iTxPlayerId
                && b.m_iTxGeneration == binding.m_iTxGeneration && b.m_iTxDeviceId == binding.m_iTxDeviceId
                && b.m_iTxTransceiverIndex == binding.m_iTxTransceiverIndex
                && b.m_iNetworkTypeId == binding.m_iNetworkTypeId && b.m_iChannelId == binding.m_iChannelId)
            {
                return true;
            }
        }
        return false;
    }
    protected string CheckProjection(RICON_PairServiceInput input, RICON_LifecycleStateMachine lifecycle, int nowMs)
    {
        if (!lifecycle || !lifecycle.CanQueryAt(nowMs) || lifecycle.GetSessionEpoch() != m_iEpoch
            || input.m_iSessionEpoch != m_iEpoch || input.m_iNowMs != nowMs)
        {
            return "STALE_LIFECYCLE_CONTEXT";
        }
        set<int> players = new set<int>();
        set<int> devices = new set<int>();
        foreach (RICON_PairPlayer player : input.m_aPlayers)
        {
            if (lifecycle.GetPlayerState(player.m_iPlayerId) != player.m_eState
                || lifecycle.GetPlayerGeneration(player.m_iPlayerId) != player.m_iGeneration)
            {
                return "STALE_SOURCE_PROJECTION";
            }
            players.Insert(player.m_iPlayerId);
        }
        foreach (RICON_LifecycleDevice device : input.m_aDevices)
        {
            RICON_LifecycleDevice actual = lifecycle.QueryDevice(device.m_iDeviceId, nowMs);
            if (!actual || !device.EqualsDevice(actual))
            {
                return "STALE_SOURCE_PROJECTION";
            }
            devices.Insert(device.m_iDeviceId);
        }
        foreach (RICON_TransmissionBinding binding : input.m_aBindings)
        {
            RICON_LifecycleLease lease = lifecycle.QueryActiveLease(binding.m_iLeaseId, nowMs);
            if (binding.m_bActive)
            {
                if (!players.Contains(binding.m_iTxPlayerId) || !devices.Contains(binding.m_iTxDeviceId)
                    || binding.m_iSessionEpoch != m_iEpoch || !lease || lease.m_iPlayerId != binding.m_iTxPlayerId
                    || lease.m_iGeneration != binding.m_iTxGeneration || lease.m_iDeviceId != binding.m_iTxDeviceId
                    || lease.m_iIssuedAtMs != binding.m_iIssuedAtMs || lease.m_iTtlMs != binding.m_iTtlMs)
                {
                    return "STALE_SOURCE_PROJECTION";
                }
                RICON_LifecycleDevice transmitter = lifecycle.QueryDevice(binding.m_iTxDeviceId, nowMs);
                if (!transmitter || !transmitter.m_bPowered || !transmitter.CanAccess(binding.m_iTxPlayerId)
                    || !transmitter.Allows(binding.m_iNetworkTypeId, binding.m_iChannelId))
                {
                    return "STALE_SOURCE_PROJECTION";
                }
            }
            else if (lease || !ExpectedForLease(binding))
            {
                return "UNRESOLVED_PAIR";
            }
        }
        foreach (RICON_PairListener listener : input.m_aListeners)
        {
            if (!players.Contains(listener.m_iPlayerId) || !devices.Contains(listener.m_iDeviceId)
                || listener.m_iSessionEpoch != m_iEpoch || lifecycle.GetPlayerGeneration(listener.m_iPlayerId) != listener.m_iGeneration)
            {
                return "STALE_SOURCE_PROJECTION";
            }
        }
        foreach (RICON_PairEndpoint endpoint : input.m_aEndpoints)
        {
            RICON_LifecycleDevice held = lifecycle.QueryDevice(endpoint.m_Observation.m_iDeviceId, nowMs);
            if (!devices.Contains(endpoint.m_Observation.m_iDeviceId) || !held
                || held.m_bPowered != endpoint.m_Observation.m_bPowered)
            {
                return "STALE_SOURCE_PROJECTION";
            }
        }
        return "OK";
    }
    protected bool RegistryJournalConsistent()
    {
        if (m_iGlobalPrefix > m_iReservedHighWater || m_Registry.m_iSessionEpoch != m_iEpoch
            || m_Registry.m_iHighWaterLinkId > m_iReservedHighWater)
        {
            return false;
        }
        set<int> heldIds = new set<int>();
        for (int i = 0; i < m_Registry.m_Entries.Count(); i++)
        {
            RICON_TemporalProducerEntry entry = m_Registry.m_Entries.Get(m_Registry.m_Entries.GetKey(i));
            int id = entry.m_Snapshot.m_iLinkId;
            if (id <= 0 || id > m_iReservedHighWater || heldIds.Contains(id))
            {
                return false;
            }
            heldIds.Insert(id);
            RICON_TemporalCapturedLink capture = m_Active.Get(id);
            RICON_TemporalClosureIntent closed = m_Closed.Get(id);
            if (!capture && !closed)
            {
                return false;
            }
            if (capture && (!RICON_TemporalProtocolRules.SameSnapshot(capture.m_Snapshot, entry.m_Snapshot)
                || capture.m_iReceiverEntityId != entry.m_iReceiverEntityId))
            {
                return false;
            }
            if (closed && (closed.m_iCapturedLinkSeq != entry.m_Snapshot.m_iLinkSeq
                || !closed.m_Binding.Same(entry.m_Snapshot.m_Binding)
                || closed.m_iTalkerPlayerId != entry.m_Snapshot.m_iTalkerPlayerId
                || closed.m_iListenerPlayerId != entry.m_Snapshot.m_iListenerPlayerId))
            {
                return false;
            }
        }
        for (int a = 0; a < m_Active.Count(); a++)
        {
            if (!heldIds.Contains(m_Active.GetKey(a)))
            {
                return false;
            }
        }
        for (int c = 0; c < m_Closed.Count(); c++)
        {
            if (m_Closed.GetKey(c) > m_iReservedHighWater)
            {
                return false;
            }
        }
        return true;
    }
    protected RICON_TemporalPublicationOwner ScratchOwner()
    {
        RICON_TemporalPublicationOwner scratch = new RICON_TemporalPublicationOwner(m_iEpoch, m_Limits, m_PublicationLimits);
        scratch.m_iReservedHighWater = m_iReservedHighWater;
        scratch.m_iGlobalPrefix = m_iGlobalPrefix;
        scratch.m_iTotalBytes = m_iTotalBytes;
        for (int i = 0; i < m_Active.Count(); i++)
        {
            int id = m_Active.GetKey(i);
            RICON_TemporalCapturedLink old = m_Active.Get(id);
            RICON_TemporalCapturedLink copy = new RICON_TemporalCapturedLink();
            copy.m_Snapshot = old.m_Snapshot.CloneValidated();
            copy.m_Expected = old.m_Expected.CloneValidated();
            copy.m_iDescriptorBytes = old.m_iDescriptorBytes;
            copy.m_iReceiverEntityId = old.m_iReceiverEntityId;
            scratch.m_Active.Set(id, copy);
        }
        for (int j = 0; j < m_Closed.Count(); j++)
        {
            int closed = m_Closed.GetKey(j);
            scratch.m_Closed.Set(closed, m_Closed.Get(closed).CloneValidated());
        }
        return scratch;
    }
    protected RICON_TemporalCaptureResult ScratchCapture(array<ref RICON_TemporalCaptureInput> rows)
    {
        return super.CapturePublishedBatch(rows);
    }
    protected RICON_TemporalCaptureResult ScratchCheckpoint(int highWater, array<int> activeIds)
    {
        return super.CommitSourceCheckpoint(m_iEpoch, highWater, activeIds);
    }
    protected string Withdraw(RICON_TemporalPublicationPlan plan)
    {
        foreach (RICON_TemporalPublicationPlanRow row : plan.m_aRows)
        {
            if (row.m_iKind != RICON_LinkPublicationItem.TERMINATE)
            {
                continue;
            }
            RICON_TemporalSnapshot snapshot = row.m_Old.m_Snapshot;
            int id = snapshot.m_iLinkId;
            if (m_Closed.Contains(id))
            {
                RICON_TemporalClosureIntent pending = m_Closed.Get(id);
                if (pending.m_iCapturedLinkSeq != snapshot.m_iLinkSeq || !pending.m_Binding.Same(snapshot.m_Binding)
                    || pending.m_sReason != row.m_sEndReason)
                {
                    return "SOURCE_STATE_MISMATCH";
                }
                continue;
            }
            RICON_TemporalCapturedLink held = m_Active.Get(id);
            if (!held || !RICON_TemporalProtocolRules.SameSnapshot(held.m_Snapshot, snapshot))
            {
                return "SOURCE_STATE_MISMATCH";
            }
            m_Closed.Set(id, MakeIntent(snapshot, row.m_sEndReason));
            m_iTotalBytes -= held.m_iDescriptorBytes;
            m_Active.Remove(id);
        }
        return "OK";
    }
    protected string PlanCapacity(RICON_TemporalPublicationPlan plan, RICON_LifecycleStateMachine lifecycle, int nowMs)
    {
        int active, additionalClosed;
        foreach (RICON_TemporalPublicationPlanRow row : plan.m_aRows)
        {
            if (row.m_iKind == RICON_LinkPublicationItem.TERMINATE)
            {
                if (!m_Closed.Contains(row.m_Old.m_Snapshot.m_iLinkId))
                {
                    RICON_LifecycleLease lease = lifecycle.QueryActiveLease(row.m_Old.m_Snapshot.m_Binding.m_iLeaseId, nowMs);
                    if (!lease)
                    {
                        return "UNRESOLVED_PAIR";
                    }
                    additionalClosed++;
                }
            }
            else
            {
                active++;
            }
        }
        if (active + m_Closed.Count() + additionalClosed > m_Limits.m_iMaxSnapshots)
        {
            return "RESOURCE_LIMIT";
        }
        return "OK";
    }
    protected bool EntryCurrent(RICON_TemporalProducerEntry entry, RICON_LifecycleStateMachine lifecycle, int nowMs)
    {
        RICON_TemporalSnapshot s = entry.m_Snapshot;
        RICON_LifecycleLease lease = lifecycle.QueryActiveLease(s.m_Binding.m_iLeaseId, nowMs);
        return lease && lease.m_iPlayerId == s.m_iTalkerPlayerId && lease.m_iGeneration == s.m_Binding.m_iTxGeneration
            && lease.m_iDeviceId == s.m_Binding.m_iTxDeviceId
            && lifecycle.GetPlayerState(s.m_iTalkerPlayerId) == RICON_EPlayerLifecycleState.BOUND_ACTIVE
            && lifecycle.GetPlayerState(s.m_iListenerPlayerId) == RICON_EPlayerLifecycleState.BOUND_ACTIVE
            && lifecycle.GetPlayerGeneration(s.m_iTalkerPlayerId) == s.m_Binding.m_iTxGeneration
            && lifecycle.GetPlayerGeneration(s.m_iListenerPlayerId) == s.m_Binding.m_iRxGeneration
            && lifecycle.GetPlayerEntityId(s.m_iListenerPlayerId) == entry.m_iReceiverEntityId;
    }
    protected RICON_TemporalPublicationResult PublishCore(RICON_TemporalProducerTick tick, RICON_LinkEvaluator evaluator,
        RICON_LifecycleStateMachine lifecycle, int nowMs, RICON_LinkSequenceSource source)
    {
        if (m_sConfiguration != "OK" || m_sProducerConfiguration != "OK")
        {
            return PublicationResult("INVALID_LIMITS");
        }
        if (!tick || tick.m_iSchemaVersion != 2 || !tick.m_PairInput || !tick.m_ChannelByPair || !source
            || !RICON_TemporalSnapshots.ValidKey(tick.m_sTimebaseId) || (tick.m_sMode != "SERVER" && tick.m_sMode != "RECEIVER"))
        {
            return PublicationResult("INVALID_INPUT");
        }
        if (!RegistryJournalConsistent())
        {
            return PublicationResult("SOURCE_STATE_MISMATCH");
        }
        RICON_EPairServiceCode structural = RICON_TemporalPairProjectionCheck.CheckStructure(tick.m_PairInput, evaluator);
        if (structural != RICON_EPairServiceCode.OK)
        {
            return PublicationResult(typename.EnumToString(RICON_EPairServiceCode, structural));
        }
        string code = CheckProjection(tick.m_PairInput, lifecycle, nowMs);
        if (code != "OK")
        {
            return PublicationResult(code);
        }
        RICON_PairService pairs = new RICON_PairService();
        RICON_PairServiceResult evaluated = pairs.Evaluate(tick.m_PairInput, evaluator);
        RICON_TemporalPublicationService service = new RICON_TemporalPublicationService();
        bool full = m_bProducerNeedsFull || m_Registry.m_bNeedsFullResync;
        RICON_TemporalPublicationPlan plan = service.Prepare(tick, evaluated, m_Registry, m_PublicationLimits,
            m_Limits, lifecycle, nowMs, source, full, m_iReservedHighWater, m_Closed);
        if (plan.m_sCode != "OK")
        {
            return PublicationResult(plan.m_sCode);
        }
        code = PlanCapacity(plan, lifecycle, nowMs);
        if (code != "OK")
        {
            return PublicationResult(code);
        }
        int reads = 1;
        int calls = 0;
        if (source.GetSessionEpoch() != m_iEpoch)
        {
            return IssuanceFailure(calls, reads);
        }
        int nextId = m_iReservedHighWater;
        m_iReservedHighWater += plan.m_iCreateCount;
        foreach (RICON_TemporalPublicationPlanRow allocation : plan.m_aRows)
        {
            if (allocation.m_iKind == RICON_LinkPublicationItem.CREATE)
            {
                nextId++;
                allocation.m_Next.m_Snapshot.m_iLinkId = nextId;
                allocation.m_Next.m_Snapshot.m_iRouteId = nextId;
                allocation.m_Next.m_Expected.m_iLinkId = nextId;
            }
        }
        RICON_TemporalPublicationResult result = PublicationResult("OK");
        result.m_bFull = full;
        result.m_bRequiresFullResync = full;
        foreach (RICON_TemporalPublicationPlanRow step : plan.m_aRows)
        {
            if (!step.m_bNeed)
            {
                continue;
            }
            int id;
            if (step.m_iKind == RICON_LinkPublicationItem.TERMINATE)
            {
                id = step.m_Old.m_Snapshot.m_iLinkId;
            }
            else
            {
                id = step.m_Next.m_Snapshot.m_iLinkId;
            }
            int seq;
            bool rotated;
            bool ok = source.NextLinkSequence(id, seq, rotated);
            calls++;
            int epoch = source.GetSessionEpoch();
            reads++;
            if (!ok || rotated || seq <= 0 || seq >= int.MAX || epoch != m_iEpoch
                || (step.m_Old && seq <= step.m_Old.m_Snapshot.m_iLinkSeq))
            {
                return IssuanceFailure(calls, reads);
            }
            if (step.m_iKind == RICON_LinkPublicationItem.TERMINATE)
            {
                RICON_TemporalEndRecord end = RICON_TemporalEndRecord.FromSnapshot(step.m_Old.m_Snapshot, seq, step.m_sEndReason);
                if (!end)
                {
                    return IssuanceFailure(calls, reads);
                }
                result.m_aEnds.Insert(end);
            }
            else
            {
                step.m_Next.m_Snapshot.m_iLinkSeq = seq;
                result.m_aSnapshots.Insert(step.m_Next.m_Snapshot.CloneValidated());
            }
        }
        if (m_bInterference || CheckProjection(tick.m_PairInput, lifecycle, nowMs) != "OK")
        {
            return IssuanceFailure(calls, reads);
        }
        RICON_TemporalProducerRegistry candidate = new RICON_TemporalProducerRegistry();
        candidate.m_iSessionEpoch = m_iEpoch;
        candidate.m_iHighWaterLinkId = m_iReservedHighWater;
        candidate.m_iLastTickMs = nowMs;
        candidate.m_bNeedsFullResync = false;
        array<ref RICON_TemporalCaptureInput> capture = {};
        foreach (RICON_TemporalPublicationPlanRow row : plan.m_aRows)
        {
            if (row.m_iKind == RICON_LinkPublicationItem.TERMINATE)
            {
                continue;
            }
            RICON_TemporalProducerEntry entry = row.m_Next;
            if (!EntryCurrent(entry, lifecycle, nowMs))
            {
                return IssuanceFailure(calls, reads);
            }
            candidate.m_Entries.Set(row.m_sKey, entry.CloneProducerEntry());
            capture.Insert(new RICON_TemporalCaptureInput(entry.m_Snapshot, entry.m_Expected, entry.m_iReceiverEntityId));
            result.m_aActiveIds.Insert(entry.m_Snapshot.m_iLinkId);
            result.m_ExpectedByLink.Set(entry.m_Snapshot.m_iLinkId, entry.m_Expected.CloneValidated());
        }
        result.m_aActiveIds.Sort();
        RICON_TemporalPublicationOwner scratch = ScratchOwner();
        code = scratch.Withdraw(plan);
        if (code != "OK")
        {
            return IssuanceFailure(calls, reads);
        }
        RICON_TemporalCaptureResult captured = scratch.ScratchCapture(capture);
        if (captured.m_sCode != "OK" && captured.m_sCode != "DUPLICATE")
        {
            return IssuanceFailure(calls, reads);
        }
        result.m_iSequenceCalls = calls;
        result.m_iEpochReads = reads;
        result.m_iReservedHighWater = m_iReservedHighWater;
        // No allocations, callbacks or fallible validation beyond this adoption boundary.
        m_Active = scratch.m_Active;
        m_Closed = scratch.m_Closed;
        m_iTotalBytes = scratch.m_iTotalBytes;
        m_iGlobalPrefix = scratch.m_iGlobalPrefix;
        m_Registry = candidate;
        m_bProducerNeedsFull = false;
        return result;
    }
    RICON_TemporalPublicationResult Publish(RICON_TemporalProducerTick tick, RICON_LinkEvaluator evaluator,
        RICON_LifecycleStateMachine lifecycle, int lifecycleNowMs, RICON_LinkSequenceSource source)
    {
        if (m_bPublishing)
        {
            m_bInterference = true;
            return PublicationResult("PRODUCER_BUSY", 0, 0, true);
        }
        m_bPublishing = true;
        m_bInterference = false;
        RICON_TemporalPublicationResult result = PublishCore(tick, evaluator, lifecycle, lifecycleNowMs, source);
        m_bPublishing = false;
        return result;
    }

    RICON_TemporalPublicationResult BuildFullSourceHandoff()
    {
        if (m_bPublishing)
        {
            m_bInterference = true;
            return PublicationResult("PRODUCER_BUSY", 0, 0, true);
        }
        if (m_sConfiguration != "OK" || m_sProducerConfiguration != "OK" || !RegistryJournalConsistent())
        {
            return PublicationResult("SOURCE_STATE_MISMATCH");
        }
        RICON_TemporalPublicationResult result = PublicationResult("OK");
        result.m_bFull = true;
        result.m_bRequiresFullResync = true;
        RICON_TemporalProducerRegistry candidate = new RICON_TemporalProducerRegistry();
        candidate.m_iSessionEpoch = m_iEpoch;
        candidate.m_iHighWaterLinkId = m_iReservedHighWater;
        candidate.m_iLastTickMs = m_Registry.m_iLastTickMs;
        candidate.m_bNeedsFullResync = true;
        for (int i = 0; i < m_Registry.m_Entries.Count(); i++)
        {
            string key = m_Registry.m_Entries.GetKey(i);
            RICON_TemporalProducerEntry row = m_Registry.m_Entries.Get(key);
            int id = row.m_Snapshot.m_iLinkId;
            if (m_Closed.Contains(id))
            {
                continue;
            }
            candidate.m_Entries.Set(key, row.CloneProducerEntry());
            result.m_aSnapshots.Insert(row.m_Snapshot.CloneValidated());
            result.m_ExpectedByLink.Set(id, row.m_Expected.CloneValidated());
            result.m_aActiveIds.Insert(id);
        }
        result.m_aActiveIds.Sort();
        RICON_TemporalPublicationOwner scratch = ScratchOwner();
        RICON_TemporalCaptureResult seal = scratch.ScratchCheckpoint(m_iReservedHighWater, result.m_aActiveIds);
        if (seal.m_sCode != "OK")
        {
            return PublicationResult(seal.m_sCode, 0, 0, true);
        }
        // All-owner output was constructed before pruning/adoption; no receiver ACK is inferred.
        m_Active = scratch.m_Active;
        m_Closed = scratch.m_Closed;
        m_iTotalBytes = scratch.m_iTotalBytes;
        m_iGlobalPrefix = scratch.m_iGlobalPrefix;
        m_Registry = candidate;
        m_bProducerNeedsFull = true;
        return result;
    }
}
