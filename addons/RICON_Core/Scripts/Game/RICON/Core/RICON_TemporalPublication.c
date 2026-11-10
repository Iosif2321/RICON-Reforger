//! STAGING explicit schema2 producer; legacy S3b remains unchanged. No world builder/RPC/voice proof.
class RICON_TemporalChannelInput
{
    ref RICON_FadingQuery m_Query;
    ref RICON_FadingDescriptor m_Descriptor;
}
class RICON_TemporalProducerTick
{
    int m_iSchemaVersion = 2;
    ref RICON_PairServiceInput m_PairInput;
    int m_iTtlMs;
    float m_fQualityDelta;
    string m_sMode;
    string m_sTimebaseId;
    ref map<string, ref RICON_TemporalChannelInput> m_ChannelByPair = new map<string, ref RICON_TemporalChannelInput>();
}
class RICON_TemporalProducerEntry
{
    ref RICON_TemporalSnapshot m_Snapshot;
    ref RICON_TemporalExpected m_Expected;
    int m_iPublishedAtMs;
    int m_iReceiverEntityId;
    int m_iDescriptorBytes;
    RICON_TemporalProducerEntry CloneProducerEntry()
    {
        RICON_TemporalProducerEntry c = new RICON_TemporalProducerEntry();
        c.m_Snapshot = m_Snapshot.CloneValidated();
        c.m_Expected = m_Expected.CloneValidated();
        c.m_iPublishedAtMs = m_iPublishedAtMs;
        c.m_iReceiverEntityId = m_iReceiverEntityId;
        c.m_iDescriptorBytes = m_iDescriptorBytes;
        return c;
    }
}
class RICON_TemporalProducerRegistry
{
    int m_iSessionEpoch;
    int m_iHighWaterLinkId;
    int m_iLastTickMs = -1;
    bool m_bNeedsFullResync = true;
    ref map<string, ref RICON_TemporalProducerEntry> m_Entries = new map<string, ref RICON_TemporalProducerEntry>();
    RICON_TemporalProducerRegistry CloneRegistry2()
    {
        RICON_TemporalProducerRegistry c = new RICON_TemporalProducerRegistry();
        c.m_iSessionEpoch = m_iSessionEpoch;
        c.m_iHighWaterLinkId = m_iHighWaterLinkId;
        c.m_iLastTickMs = m_iLastTickMs;
        c.m_bNeedsFullResync = m_bNeedsFullResync;
        for (int i = 0; i < m_Entries.Count(); i++)
        {
            string key = m_Entries.GetKey(i);
            c.m_Entries.Set(key, m_Entries.Get(key).CloneProducerEntry());
        }
        return c;
    }
}
class RICON_TemporalPublicationResult
{
    string m_sCode = "INVALID_INPUT";
    int m_iSessionEpoch;
    bool m_bRequiresFullResync;
    bool m_bFull;
    int m_iSequenceCalls;
    int m_iEpochReads;
    int m_iReservedHighWater;
    ref array<ref RICON_TemporalSnapshot> m_aSnapshots = {};
    ref array<ref RICON_TemporalEndRecord> m_aEnds = {};
    ref array<int> m_aActiveIds = {};
    ref map<int, ref RICON_TemporalExpected> m_ExpectedByLink = new map<int, ref RICON_TemporalExpected>();
}
class RICON_TemporalPublicationPlanRow
{
    string m_sKey;
    int m_iKind;
    ref RICON_TemporalProducerEntry m_Old;
    ref RICON_TemporalProducerEntry m_Next;
    string m_sEndReason;
    bool m_bNeed;
}
class RICON_TemporalPublicationPlan
{
    string m_sCode = "INVALID_INPUT";
    bool m_bFull;
    int m_iCreateCount;
    ref array<ref RICON_TemporalPublicationPlanRow> m_aRows = {};
}
//! Bridge exposes existing CF validation/context predicates; it implements no new physics.
class RICON_TemporalPublicationChannelCheck : RICON_FiniteMultipath
{
    static bool ValidInput(RICON_FadingDescriptor descriptor, RICON_FadingQuery query)
    {
        return Valid(descriptor, query);
    }
    static bool SameInputContext(RICON_FadingDescriptor descriptor, RICON_FadingQuery query)
    {
        return SameContext(descriptor, query);
    }
}
class RICON_TemporalPairProjectionCheck : RICON_PairService
{
    static RICON_EPairServiceCode CheckStructure(RICON_PairServiceInput input, RICON_LinkEvaluator evaluator)
    {
        return Preflight(input, evaluator);
    }
}

class RICON_TemporalPublicationService : RICON_LinkPublicationService
{
    static string PairKey(int leaseId, int rxDeviceId, int rxIndex, int rxPlayerId)
    {
        return Key(leaseId, rxDeviceId, rxIndex, rxPlayerId);
    }
    static RICON_LinkPublicationLimits CopyPublicationLimits(RICON_LinkPublicationLimits limits)
    {
        if (!limits)
        {
            return null;
        }
        RICON_LinkPublicationLimits c = new RICON_LinkPublicationLimits();
        c.m_iMaxBindings = limits.m_iMaxBindings;
        c.m_iMaxPairs = limits.m_iMaxPairs;
        c.m_iMaxActiveLinks = limits.m_iMaxActiveLinks;
        return c;
    }
    static bool ValidPublicationLimits(RICON_LinkPublicationLimits limits)
    {
        return limits && limits.m_iMaxBindings >= 1 && limits.m_iMaxBindings <= 4096
            && limits.m_iMaxPairs >= 1 && limits.m_iMaxPairs <= 65536
            && limits.m_iMaxActiveLinks >= 1 && limits.m_iMaxActiveLinks <= 65536;
    }
    protected static RICON_LinkRegistry ScalarView(RICON_TemporalProducerRegistry registry, int reserved, bool full)
    {
        RICON_LinkRegistry view = new RICON_LinkRegistry();
        view.m_iSessionEpoch = registry.m_iSessionEpoch;
        view.m_iHighWaterLinkId = reserved;
        view.m_iLastTickMs = registry.m_iLastTickMs;
        view.m_bNeedsFullResync = full;
        for (int i = 0; i < registry.m_Entries.Count(); i++)
        {
            RICON_TemporalProducerEntry held = registry.m_Entries.Get(registry.m_Entries.GetKey(i));
            RICON_TemporalSnapshot s = held.m_Snapshot;
            RICON_LinkRegistryEntry e = new RICON_LinkRegistryEntry();
            e.m_iLeaseId = s.m_Binding.m_iLeaseId;
            e.m_iRxDeviceId = s.m_Binding.m_iRxDeviceId;
            e.m_iRxTransceiverIndex = s.m_Binding.m_iRxTransceiverIndex;
            e.m_iRxPlayerId = s.m_iListenerPlayerId;
            e.m_iTxPlayerId = s.m_iTalkerPlayerId;
            e.m_iTxDeviceId = s.m_Binding.m_iTxDeviceId;
            e.m_iTxTransceiverIndex = s.m_Binding.m_iTxTransceiverIndex;
            e.m_iLinkId = s.m_iLinkId;
            e.m_iLinkSeq = s.m_iLinkSeq;
            e.m_eState = s.m_eState;
            e.m_fQuality = s.m_fQuality;
            e.m_iTtlMs = s.m_iTtlMs;
            e.m_iPublishedAtMs = held.m_iPublishedAtMs;
            view.m_aEntries.Insert(e);
        }
        return view;
    }
    protected static string EndReason(RICON_TemporalProducerEntry old, map<int, ref RICON_TemporalClosureIntent> pending)
    {
        RICON_TemporalClosureIntent intent = pending.Get(old.m_Snapshot.m_iLinkId);
        if (intent)
        {
            return intent.m_sReason;
        }
        return "LINK_REMOVED";
    }
    protected static void AddEnd(RICON_TemporalPublicationPlan plan, string key, RICON_TemporalProducerEntry old,
        map<int, ref RICON_TemporalClosureIntent> pending)
    {
        RICON_TemporalPublicationPlanRow row = new RICON_TemporalPublicationPlanRow();
        row.m_sKey = key;
        row.m_iKind = RICON_LinkPublicationItem.TERMINATE;
        row.m_Old = old;
        row.m_sEndReason = EndReason(old, pending);
        row.m_bNeed = true;
        plan.m_aRows.Insert(row);
    }
    protected static RICON_PairListener Listener(RICON_PairServiceInput input, RICON_LinkPublicationItem pair)
    {
        foreach (RICON_PairListener listener : input.m_aListeners)
        {
            if (listener.m_iPlayerId == pair.m_iRxPlayerId && listener.m_iDeviceId == pair.m_iRxDeviceId
                && listener.m_iTransceiverIndex == pair.m_iRxTransceiverIndex)
            {
                return listener;
            }
        }
        return null;
    }
    protected static RICON_RadioRfProfile Profile(RICON_PairServiceInput input, int device, int index)
    {
        foreach (RICON_PairEndpoint endpoint : input.m_aEndpoints)
        {
            if (endpoint.m_Observation.m_iDeviceId == device && endpoint.m_Observation.m_iTransceiverIndex == index)
            {
                return endpoint.m_Profile;
            }
        }
        return null;
    }
    protected static bool DescriptorSame(RICON_FadingDescriptor left, RICON_FadingDescriptor right)
    {
        RICON_DescriptorResult a = RICON_DescriptorCodec.Encode(left, 65535);
        RICON_DescriptorResult b = RICON_DescriptorCodec.Encode(right, 65535);
        if (a.m_eCode != RICON_EDescriptorCode.OK || b.m_eCode != RICON_EDescriptorCode.OK || a.m_iSizeBytes != b.m_iSizeBytes)
        {
            return false;
        }
        for (int i = 0; i < a.m_aBytes.Count(); i++)
        {
            if (a.m_aBytes[i] != b.m_aBytes[i])
            {
                return false;
            }
        }
        return true;
    }
    protected static string ActiveEntry(RICON_TemporalProducerTick tick, RICON_LinkPublicationItem pair,
        RICON_LifecycleStateMachine lifecycle, int nowMs, RICON_TemporalLimits limits, out RICON_TemporalProducerEntry entry)
    {
        entry = null;
        RICON_TemporalChannelInput channel = tick.m_ChannelByPair.Get(PairKey(pair.m_iLeaseId, pair.m_iRxDeviceId, pair.m_iRxTransceiverIndex, pair.m_iRxPlayerId));
        if (!channel || !channel.m_Query || !channel.m_Descriptor)
        {
            return "TEMPORAL_INPUT_UNAVAILABLE";
        }
        RICON_FadingQuery q = channel.m_Query;
        RICON_DescriptorResult encoded = RICON_DescriptorCodec.Encode(channel.m_Descriptor, limits.m_iMaxDescriptorBytes);
        if (encoded.m_eCode != RICON_EDescriptorCode.OK)
        {
            return typename.EnumToString(RICON_EDescriptorCode, encoded.m_eCode);
        }
        if (!RICON_TemporalPublicationChannelCheck.ValidInput(encoded.m_Descriptor, q))
        {
            return "TEMPORAL_INPUT_UNAVAILABLE";
        }
        if (!RICON_TemporalPublicationChannelCheck.SameInputContext(encoded.m_Descriptor, q))
        {
            return "CONTEXT_MISMATCH";
        }
        if (q.m_iNowMs < encoded.m_Descriptor.m_iValidFromMs || q.m_iNowMs >= encoded.m_Descriptor.m_iValidUntilMs)
        {
            return "STALE_DESCRIPTOR";
        }
        RICON_TransmissionBinding b = pair.m_Binding;
        RICON_PairListener listener = Listener(tick.m_PairInput, pair);
        RICON_LifecycleLease lease = lifecycle.QueryActiveLease(b.m_iLeaseId, nowMs);
        if (!listener || !lease || lease.m_iPlayerId != b.m_iTxPlayerId || lease.m_iGeneration != b.m_iTxGeneration
            || lease.m_iDeviceId != b.m_iTxDeviceId || lease.m_iIssuedAtMs != b.m_iIssuedAtMs || lease.m_iTtlMs != b.m_iTtlMs
            || lifecycle.GetPlayerState(b.m_iTxPlayerId) != RICON_EPlayerLifecycleState.BOUND_ACTIVE
            || lifecycle.GetPlayerState(listener.m_iPlayerId) != RICON_EPlayerLifecycleState.BOUND_ACTIVE
            || lifecycle.GetPlayerGeneration(b.m_iTxPlayerId) != b.m_iTxGeneration
            || lifecycle.GetPlayerGeneration(listener.m_iPlayerId) != listener.m_iGeneration)
        {
            return "STALE_LIFECYCLE_CONTEXT";
        }
        int entity = lifecycle.GetPlayerEntityId(listener.m_iPlayerId);
        RICON_RadioRfProfile txProfile = Profile(tick.m_PairInput, b.m_iTxDeviceId, b.m_iTxTransceiverIndex);
        RICON_RadioRfProfile rxProfile = Profile(tick.m_PairInput, listener.m_iDeviceId, listener.m_iTransceiverIndex);
        if (entity <= 0 || !txProfile || !rxProfile || q.m_Tx.m_iDeviceId != b.m_iTxDeviceId
            || q.m_Tx.m_iTransceiverIndex != b.m_iTxTransceiverIndex || q.m_Rx.m_iDeviceId != listener.m_iDeviceId
            || q.m_Rx.m_iTransceiverIndex != listener.m_iTransceiverIndex
            || q.m_fFrequencyMHz != txProfile.m_fFrequencyMHz || q.m_fBandwidthHz != rxProfile.m_fRxBandwidthHz)
        {
            return "CONTEXT_MISMATCH";
        }
        RICON_TemporalExpected expected = new RICON_TemporalExpected();
        expected.m_iSessionEpoch = tick.m_PairInput.m_iSessionEpoch;
        expected.m_iTalkerPlayerId = b.m_iTxPlayerId;
        expected.m_iListenerPlayerId = listener.m_iPlayerId;
        expected.m_sMode = tick.m_sMode;
        expected.m_sTimebaseId = tick.m_sTimebaseId;
        expected.m_Binding = new RICON_TemporalBinding();
        expected.m_Binding.m_iLeaseId = b.m_iLeaseId;
        expected.m_Binding.m_iTxGeneration = b.m_iTxGeneration;
        expected.m_Binding.m_iRxGeneration = listener.m_iGeneration;
        expected.m_Binding.m_iTxDeviceId = b.m_iTxDeviceId;
        expected.m_Binding.m_iRxDeviceId = listener.m_iDeviceId;
        expected.m_Binding.m_iTxTransceiverIndex = b.m_iTxTransceiverIndex;
        expected.m_Binding.m_iRxTransceiverIndex = listener.m_iTransceiverIndex;
        expected.m_Binding.m_iNetworkTypeId = b.m_iNetworkTypeId;
        expected.m_Binding.m_iChannelId = b.m_iChannelId;
        expected.m_Channel = new RICON_TemporalChannelContext();
        expected.m_Channel.m_Tx = RICON_TemporalSnapshots.CloneEndpoint(q.m_Tx);
        expected.m_Channel.m_Rx = RICON_TemporalSnapshots.CloneEndpoint(q.m_Rx);
        expected.m_Channel.m_fFrequencyMHz = q.m_fFrequencyMHz;
        expected.m_Channel.m_fBandwidthHz = q.m_fBandwidthHz;
        expected.m_Channel.m_sProfileId = q.m_sProfileId;
        expected.m_Channel.m_iProfileVersion = q.m_iProfileVersion;
        expected.m_Channel.m_iEnvironmentRevision = q.m_iEnvironmentRevision;
        RICON_TemporalSnapshot s = new RICON_TemporalSnapshot();
        s.m_iSchemaVersion = 2;
        s.m_iSessionEpoch = expected.m_iSessionEpoch;
        s.m_iTtlMs = tick.m_iTtlMs;
        s.m_iRouteNodeCount = 2;
        s.m_iTalkerPlayerId = expected.m_iTalkerPlayerId;
        s.m_iListenerPlayerId = expected.m_iListenerPlayerId;
        s.m_eState = pair.m_eState;
        s.m_fQuality = pair.m_fQuality;
        s.m_sMode = tick.m_sMode;
        s.m_sTimebaseId = tick.m_sTimebaseId;
        s.m_iDescriptorVersion = 1;
        s.m_Binding = expected.m_Binding.CloneBinding();
        s.m_Descriptor = encoded.m_Descriptor;
        // Private provisional identifiers validate the existing Snapshot2 domain; they reserve/issue nothing.
        s.m_iLinkId = 1;
        s.m_iRouteId = 1;
        s.m_iLinkSeq = 1;
        expected.m_iLinkId = 1;
        RICON_TemporalValidationResult normalized = RICON_TemporalSnapshots.Validate(s, expected, limits);
        if (normalized.m_sCode != "OK")
        {
            return normalized.m_sCode;
        }
        normalized.m_Snapshot.m_iLinkId = 0;
        normalized.m_Snapshot.m_iRouteId = 0;
        normalized.m_Snapshot.m_iLinkSeq = 0;
        expected.m_iLinkId = 0;
        entry = new RICON_TemporalProducerEntry();
        entry.m_Snapshot = normalized.m_Snapshot;
        entry.m_Expected = expected;
        entry.m_iPublishedAtMs = tick.m_PairInput.m_iNowMs;
        entry.m_iReceiverEntityId = entity;
        entry.m_iDescriptorBytes = normalized.m_iDescriptorBytes;
        return "OK";
    }

    RICON_TemporalPublicationPlan Prepare(RICON_TemporalProducerTick tick, RICON_PairServiceResult pairs,
        RICON_TemporalProducerRegistry registry, RICON_LinkPublicationLimits publicationLimits, RICON_TemporalLimits limits,
        RICON_LifecycleStateMachine lifecycle, int nowMs, RICON_LinkSequenceSource source, bool full, int reserved,
        map<int, ref RICON_TemporalClosureIntent> pending)
    {
        RICON_TemporalPublicationPlan plan = new RICON_TemporalPublicationPlan();
        plan.m_bFull = full;
        RICON_LinkPublicationInput input = new RICON_LinkPublicationInput();
        input.m_iNowMs = tick.m_PairInput.m_iNowMs;
        input.m_iSessionEpoch = tick.m_PairInput.m_iSessionEpoch;
        input.m_iTtlMs = tick.m_iTtlMs;
        input.m_fQualityDelta = tick.m_fQualityDelta;
        input.m_Limits = publicationLimits;
        input.m_aBindings = tick.m_PairInput.m_aBindings;
        input.m_PairResult = pairs;
        array<ref RICON_LinkPublicationItem> scalar = {};
        // The plan borrows scalar entries; keep their registry alive through NeedSnapshot.
        ref RICON_LinkRegistry scalarRegistry = ScalarView(registry, reserved, full);
        RICON_ELinkPublicationCode code = Preflight(input, scalarRegistry, source, scalar);
        if (code != RICON_ELinkPublicationCode.OK)
        {
            plan.m_sCode = typename.EnumToString(RICON_ELinkPublicationCode, code);
            return plan;
        }
        int activeCount, total;
        int modelNow = -1;
        int modelSub = -1;
        foreach (RICON_LinkPublicationItem item : scalar)
        {
            string key = PairKey(item.m_iLeaseId, item.m_iRxDeviceId, item.m_iRxTransceiverIndex, item.m_iRxPlayerId);
            RICON_TemporalProducerEntry old = registry.m_Entries.Get(key);
            if (item.m_iKind == RICON_LinkPublicationItem.TERMINATE)
            {
                AddEnd(plan, key, old, pending);
                continue;
            }
            if (!item.m_Pair || item.m_Pair.m_ePairCode != RICON_EPairCode.EVALUATED || !item.m_Pair.m_bValid)
            {
                plan.m_sCode = "UNRESOLVED_PAIR";
                return plan;
            }
            RICON_TemporalProducerEntry next;
            plan.m_sCode = ActiveEntry(tick, item, lifecycle, nowMs, limits, next);
            if (plan.m_sCode != "OK")
            {
                return plan;
            }
            RICON_FadingQuery query = tick.m_ChannelByPair.Get(key).m_Query;
            if (modelNow >= 0 && (query.m_iNowMs != modelNow || query.m_iSubUs != modelSub))
            {
                plan.m_sCode = "CONTEXT_MISMATCH";
                return plan;
            }
            modelNow = query.m_iNowMs;
            modelSub = query.m_iSubUs;
            bool rebind = old && (!old.m_Snapshot.m_Binding.Same(next.m_Snapshot.m_Binding)
                || old.m_iReceiverEntityId != next.m_iReceiverEntityId || pending.Contains(old.m_Snapshot.m_iLinkId));
            if (rebind)
            {
                AddEnd(plan, key, old, pending);
            }
            RICON_TemporalPublicationPlanRow row = new RICON_TemporalPublicationPlanRow();
            row.m_sKey = key;
            row.m_Next = next;
            row.m_iKind = RICON_LinkPublicationItem.CREATE;
            row.m_bNeed = true;
            if (old && !rebind)
            {
                row.m_Old = old;
                row.m_iKind = RICON_LinkPublicationItem.KEEP;
                next.m_Snapshot.m_iLinkId = old.m_Snapshot.m_iLinkId;
                next.m_Snapshot.m_iRouteId = old.m_Snapshot.m_iLinkId;
                next.m_Snapshot.m_iLinkSeq = old.m_Snapshot.m_iLinkSeq;
                next.m_Expected.m_iLinkId = old.m_Snapshot.m_iLinkId;
                row.m_bNeed = NeedSnapshot(item, input, full) || !DescriptorSame(old.m_Snapshot.m_Descriptor, next.m_Snapshot.m_Descriptor)
                    || old.m_Snapshot.m_sMode != tick.m_sMode || old.m_Snapshot.m_sTimebaseId != tick.m_sTimebaseId;
                if (!row.m_bNeed)
                {
                    next.m_Snapshot = old.m_Snapshot.CloneValidated();
                    next.m_iPublishedAtMs = old.m_iPublishedAtMs;
                }
            }
            else
            {
                plan.m_iCreateCount++;
            }
            if (next.m_iDescriptorBytes > limits.m_iMaxTotalDescriptorBytes - total)
            {
                plan.m_sCode = "RESOURCE_LIMIT";
                return plan;
            }
            total += next.m_iDescriptorBytes;
            activeCount++;
            plan.m_aRows.Insert(row);
        }
        if (activeCount != tick.m_ChannelByPair.Count())
        {
            plan.m_sCode = "CONTEXT_MISMATCH";
            return plan;
        }
        if (activeCount > publicationLimits.m_iMaxActiveLinks || activeCount > limits.m_iMaxSnapshots)
        {
            plan.m_sCode = "RESOURCE_LIMIT";
            return plan;
        }
        if (plan.m_iCreateCount > int.MAX - reserved)
        {
            plan.m_sCode = "ID_EXHAUSTED";
            return plan;
        }
        plan.m_sCode = "OK";
        return plan;
    }
}
