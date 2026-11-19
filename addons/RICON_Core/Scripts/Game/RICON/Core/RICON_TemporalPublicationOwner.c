//! Detached address of a committed direct pair for the server's route planner.
class RICON_MediaDirectCandidate
{
    int m_iLinkId, m_iListenerPlayerId, m_iRxDeviceId, m_iRxTransceiverIndex;
}

//! Detached receiver environment qualified for a half-open interval on one RF timebase.
class RICON_TemporalRxEnvironment
{
    int m_iEpoch, m_iRxDeviceId, m_iRxIndex = -1, m_iRxGeneration, m_iRevision;
    int m_iFromMs = -1, m_iUntilMs = -1;
    float m_fNoisePlusInterferenceDbm;
    string m_sTimebaseId;

    RICON_TemporalRxEnvironment CloneEnvironment()
    {
        RICON_TemporalRxEnvironment copy = new RICON_TemporalRxEnvironment();
        copy.m_iEpoch = m_iEpoch;
        copy.m_iRxDeviceId = m_iRxDeviceId;
        copy.m_iRxIndex = m_iRxIndex;
        copy.m_iRxGeneration = m_iRxGeneration;
        copy.m_iRevision = m_iRevision;
        copy.m_iFromMs = m_iFromMs;
        copy.m_iUntilMs = m_iUntilMs;
        copy.m_fNoisePlusInterferenceDbm = m_fNoisePlusInterferenceDbm;
        copy.m_sTimebaseId = m_sTimebaseId;
        return copy;
    }

    bool Same(RICON_TemporalRxEnvironment other)
    {
        return other && m_iEpoch == other.m_iEpoch && m_iRxDeviceId == other.m_iRxDeviceId
            && m_iRxIndex == other.m_iRxIndex && m_iRxGeneration == other.m_iRxGeneration && m_iRevision == other.m_iRevision
            && m_iFromMs == other.m_iFromMs && m_iUntilMs == other.m_iUntilMs
            && m_fNoisePlusInterferenceDbm == other.m_fNoisePlusInterferenceDbm && m_sTimebaseId == other.m_sTimebaseId;
    }

    bool IsValidFor(int epoch, int deviceId, int index, string timebaseId, float noiseDbm, int rxGeneration = 0)
    {
        return epoch > 0 && m_iEpoch == epoch && deviceId > 0 && m_iRxDeviceId == deviceId && index >= 0 && m_iRxIndex == index
            && m_iRxGeneration > 0 && (rxGeneration == 0 || m_iRxGeneration == rxGeneration)
            && m_iRevision > 0 && m_sTimebaseId == timebaseId && RICON_TemporalSnapshots.ValidKey(timebaseId)
            && Covers(m_iFromMs, m_iUntilMs) && RICON_Num.IsFinite(noiseDbm) && RICON_Num.IsFinite(m_fNoisePlusInterferenceDbm)
            && m_fNoisePlusInterferenceDbm >= noiseDbm;
    }

    bool Covers(int fromMs, int untilMs)
    {
        return m_iFromMs >= 0 && fromMs >= m_iFromMs && untilMs > fromMs && untilMs <= m_iUntilMs;
    }
}

//! Detached mean received power and receiver noise from one committed server RF calculation.
class RICON_TemporalRfBudget
{
    float m_fRxPowerDbm, m_fNoiseDbm, m_fBandwidthHz;
    int m_iLinkId, m_iTickMs, m_iModelMs, m_iModelSubUs;
    string m_sTimebaseId;
    ref RICON_TemporalRxEnvironment m_RxEnvironment;

    RICON_TemporalRfBudget CloneBudget()
    {
        RICON_TemporalRfBudget copy = new RICON_TemporalRfBudget();
        copy.m_fRxPowerDbm = m_fRxPowerDbm;
        copy.m_fNoiseDbm = m_fNoiseDbm;
        copy.m_fBandwidthHz = m_fBandwidthHz;
        copy.m_iLinkId = m_iLinkId;
        copy.m_iTickMs = m_iTickMs;
        copy.m_iModelMs = m_iModelMs;
        copy.m_iModelSubUs = m_iModelSubUs;
        copy.m_sTimebaseId = m_sTimebaseId;
        if (m_RxEnvironment) copy.m_RxEnvironment = m_RxEnvironment.CloneEnvironment();
        return copy;
    }

    bool Same(RICON_TemporalRfBudget other)
    {
        if (!other || m_iLinkId != other.m_iLinkId || m_iTickMs != other.m_iTickMs || m_sTimebaseId != other.m_sTimebaseId) return false;
        if (m_iModelMs != other.m_iModelMs || m_iModelSubUs != other.m_iModelSubUs) return false;
        if (m_fRxPowerDbm != other.m_fRxPowerDbm || m_fNoiseDbm != other.m_fNoiseDbm || m_fBandwidthHz != other.m_fBandwidthHz) return false;
        if (m_RxEnvironment) return m_RxEnvironment.Same(other.m_RxEnvironment);
        return other.m_RxEnvironment == null;
    }
}

//! STAGING manager-owned source dataflow. Pure tests do not prove native manager/world/RPC authority.
class RICON_TemporalPublicationOwner : RICON_TemporalAuthorityJournal
{
    protected ref RICON_TemporalProducerRegistry m_Registry = new RICON_TemporalProducerRegistry();
    protected ref RICON_LinkPublicationLimits m_PublicationLimits;
    protected RICON_LifecycleStateMachine m_PublishedLifecycle;
    protected bool m_bLifecyclePinned;
    protected int m_iReservedHighWater;
    protected bool m_bProducerNeedsFull = true;
    protected bool m_bPublishing;
    protected bool m_bLastPublishSucceeded;
    protected ref RICON_StaticLosGeometryAuthority m_StaticLosGeometry;
    protected RICON_OpenRadioStaticLosSource m_CommittedStaticRfSource;
    protected bool m_bCommittedStaticRfSource;
    protected int m_iGeometryRevision;
    protected int m_iGeometryModelMs = -1;
    protected int m_iGeometryModelSubUs;
    protected string m_sGeometryTimebaseId;
    protected ref RICON_OpenRadioListenerRoster m_ListenerRoster;
    protected int m_iListenerRosterRevision;
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

    void TraceRadioDevLink(int linkId, RICON_TemporalExpected expected, int tickMs)
    {
        if (!RICON_RadioDev.Enabled() || !expected || !expected.m_Binding || m_Registry.m_iLastTickMs != tickMs) return;
        RICON_TemporalBinding binding = expected.m_Binding;
        string key = RICON_TemporalPublicationService.PairKey(binding.m_iLeaseId, binding.m_iRxDeviceId,
            binding.m_iRxTransceiverIndex, expected.m_iListenerPlayerId);
        RICON_TemporalProducerEntry entry = m_Registry.m_Entries.Get(key);
        if (!entry || !entry.m_Snapshot || entry.m_Snapshot.m_iLinkId != linkId) return;
        RICON_TemporalSnapshot snapshot = entry.m_Snapshot;
        PrintFormat("[RICON_DEV] SERVER LINK tick=%1 link=%2 lease=%3 tx=%4:%5 rx=%6:%7 state=%8 quality=%9",
            tickMs, linkId, binding.m_iLeaseId, binding.m_iTxDeviceId, binding.m_iTxTransceiverIndex,
            binding.m_iRxDeviceId, binding.m_iRxTransceiverIndex, typename.EnumToString(RICON_ELinkState, snapshot.m_eState), snapshot.m_fQuality);
        if (entry.m_bHasRfPowers && entry.m_iRfPowerTickMs == tickMs)
            PrintFormat("[RICON_DEV] SERVER RF_POWER link=%1 rxDbm=%2 noiseDbm=%3 modelMs=%4 environment=%5",
                linkId, entry.m_fRxPowerDbm, entry.m_fNoiseDbm, entry.m_iRfModelMs, entry.m_RxEnvironment != null);
    }
    int GetMediaGeometryRevision() { return m_iGeometryRevision; }

    int GetMediaListenerIncarnation(RICON_MediaEgressRoute route)
    {
        if (!m_ListenerRoster) return 0;
        if (m_ListenerRoster.GetRevision() != m_iListenerRosterRevision) return -1;
        int incarnation = m_ListenerRoster.ReadIncarnation(route);
        if (incarnation <= 0) return -1;
        return incarnation;
    }

    bool CanReadMediaLinks(RICON_LifecycleStateMachine lifecycle, int nowMs)
    {
        return !m_bPublishing && !m_bProducerNeedsFull && !m_Registry.m_bNeedsFullResync
            && m_sConfiguration == "OK" && m_sProducerConfiguration == "OK" && m_Registry.m_iSessionEpoch == m_iEpoch
            && lifecycle && lifecycle == m_PublishedLifecycle && lifecycle.GetSessionEpoch() == m_iEpoch && lifecycle.CanQueryAt(nowMs)
            && nowMs >= m_Registry.m_iLastTickMs && CanReadMediaGeometry(nowMs)
            && (!m_ListenerRoster || (m_ListenerRoster.GetRevision() == m_iListenerRosterRevision
                && m_ListenerRoster.MatchesRfReadTime(nowMs)));
    }

    protected bool CanReadMediaGeometry(int nowMs)
    {
        if (!m_StaticLosGeometry) return true;
        return m_StaticLosGeometry.IsCurrent() && m_bLastPublishSucceeded && nowMs == m_Registry.m_iLastTickMs;
    }

    bool HasCommittedMediaTick(RICON_LifecycleStateMachine lifecycle, int nowMs)
    {
        return m_bLastPublishSucceeded && m_Registry.m_iLastTickMs == nowMs && CanReadMediaLinks(lifecycle, nowMs);
    }

    bool CanAttestRfCoverage(RICON_OpenRadioStaticLosSource source, RICON_LifecycleStateMachine lifecycle, int nowMs)
    {
        if (!source || m_CommittedStaticRfSource != source || !HasCommittedMediaTick(lifecycle, nowMs)) return false;
        return source.CanAttestRfCoverage(nowMs);
    }

    protected bool CurrentMediaSource(RICON_MediaStreamBinding source, RICON_LifecycleStateMachine lifecycle, int nowMs)
    {
        if (!source || !source.IsValid() || source.m_iSessionEpoch != m_iEpoch || !CanReadMediaLinks(lifecycle, nowMs)) return false;
        int player = source.m_iPlayerId;
        if (lifecycle.GetPlayerState(player) != RICON_EPlayerLifecycleState.BOUND_ACTIVE
            || lifecycle.GetPlayerGeneration(player) != source.m_iPlayerGeneration
            || lifecycle.GetPlayerConnectionId(player) != source.m_iConnectionId
            || lifecycle.GetPlayerEntityId(player) != source.m_iEntityId) return false;
        RICON_LifecycleLease lease = lifecycle.QueryActiveLease(source.m_iLeaseId, nowMs);
        return lease && lease.MatchesSource(player, source.m_iPlayerGeneration, source.m_iDeviceId,
            source.m_iNetworkTypeId, source.m_iChannelId);
    }

    protected bool CurrentDirectMediaCapture(int linkId, RICON_TemporalCapturedLink capture, RICON_MediaStreamBinding source,
        RICON_LifecycleStateMachine lifecycle, int nowMs, int physicalGeneration = 0)
    {
        if (linkId <= 0 || !capture || !capture.m_Snapshot || !capture.m_Snapshot.m_Binding) return false;
        RICON_TemporalSnapshot snapshot = capture.m_Snapshot;
        RICON_TemporalBinding binding = snapshot.m_Binding;
        if (snapshot.m_iLinkId != linkId || snapshot.m_iRouteNodeCount != 2 || snapshot.m_iSessionEpoch != m_iEpoch
            || snapshot.m_iTalkerPlayerId != source.m_iPlayerId || binding.m_iLeaseId != source.m_iLeaseId) return false;
        if (binding.m_iTxGeneration != source.m_iPlayerGeneration || binding.m_iTxDeviceId != source.m_iDeviceId
            || binding.m_iTxTransceiverIndex != source.m_iTxTransceiverIndex || binding.m_iNetworkTypeId != source.m_iNetworkTypeId
            || binding.m_iChannelId != source.m_iChannelId || m_Closed.Contains(linkId)) return false;

        int listener = snapshot.m_iListenerPlayerId;
        int connection = lifecycle.GetPlayerConnectionId(listener);
        if (listener <= 0 || listener == source.m_iPlayerId || connection <= 0 || connection == source.m_iConnectionId
            || binding.m_iRxDeviceId <= 0 || binding.m_iRxDeviceId == source.m_iDeviceId || binding.m_iRxTransceiverIndex < 0
            || capture.m_iReceiverEntityId <= 0) return false;
        if (m_ListenerRoster && !m_ListenerRoster.AllowsDirectCaptureListener(listener, binding.m_iRxDeviceId,
            binding.m_iRxTransceiverIndex, nowMs, physicalGeneration)) return false;
        string key = RICON_TemporalPublicationService.PairKey(source.m_iLeaseId, binding.m_iRxDeviceId,
            binding.m_iRxTransceiverIndex, listener);
        RICON_TemporalProducerEntry entry = m_Registry.m_Entries.Get(key);
        return entry && entry.m_iReceiverEntityId == capture.m_iReceiverEntityId
            && RICON_TemporalProtocolRules.SnapshotBindingSame(snapshot, entry.m_Snapshot) && ListenerCurrent(entry, lifecycle);
    }

    //! A committed pair is a prerequisite for a media grant. Quality and linkSeq do not grant access.
    bool MatchesCurrentMediaLink(int linkId, RICON_MediaEgressRoute route, RICON_LifecycleStateMachine lifecycle, int nowMs)
    {
        if (linkId <= 0 || !route || !route.IsValid() || !CurrentMediaSource(route.m_Source, lifecycle, nowMs)) return false;
        RICON_TemporalCapturedLink capture = m_Active.Get(linkId);
        if (!CurrentDirectMediaCapture(linkId, capture, route.m_Source, lifecycle, nowMs, route.m_iRxGeneration)) return false;
        RICON_TemporalSnapshot snapshot = capture.m_Snapshot;
        RICON_TemporalBinding binding = snapshot.m_Binding;
        return snapshot.m_iListenerPlayerId == route.m_iListenerPlayerId && binding.m_iRxDeviceId == route.m_iRxDeviceId
            && binding.m_iRxTransceiverIndex == route.m_iRxTransceiverIndex && binding.m_iRxGeneration == route.m_iListenerGeneration
            && capture.m_iReceiverEntityId == route.m_iListenerEntityId
            && lifecycle.GetPlayerConnectionId(route.m_iListenerPlayerId) == route.m_iListenerConnectionId;
    }

    //! Server pair evidence; the fanout separately verifies the physical RX and the current route grant.
    RICON_TemporalRfBudget ReadMediaRfBudget(int linkId, RICON_MediaEgressRoute route,
        RICON_LifecycleStateMachine lifecycle, int nowMs)
    {
        if (!HasCommittedMediaTick(lifecycle, nowMs) || !MatchesCurrentMediaLink(linkId, route, lifecycle, nowMs)) return null;
        string key = RICON_TemporalPublicationService.PairKey(route.m_Source.m_iLeaseId, route.m_iRxDeviceId,
            route.m_iRxTransceiverIndex, route.m_iListenerPlayerId);
        RICON_TemporalProducerEntry entry = m_Registry.m_Entries.Get(key);
        if (!entry || entry.m_Expected.m_sMode != "SERVER" || !entry.m_bHasRfPowers || entry.m_iRfPowerTickMs != nowMs || entry.m_iRfModelMs < 0
            || entry.m_iRfModelSubUs < 0 || entry.m_iRfModelSubUs >= 1000) return null;

        RICON_TemporalRfBudget budget = new RICON_TemporalRfBudget();
        budget.m_fRxPowerDbm = entry.m_fRxPowerDbm;
        budget.m_fNoiseDbm = entry.m_fNoiseDbm;
        budget.m_fBandwidthHz = entry.m_Expected.m_Channel.m_fBandwidthHz;
        budget.m_iLinkId = linkId;
        budget.m_iTickMs = entry.m_iRfPowerTickMs;
        budget.m_iModelMs = entry.m_iRfModelMs;
        budget.m_iModelSubUs = entry.m_iRfModelSubUs;
        budget.m_sTimebaseId = entry.m_Snapshot.m_sTimebaseId;
        if (entry.m_RxEnvironment)
        {
            if (!entry.m_RxEnvironment.IsValidFor(route.m_Source.m_iSessionEpoch, route.m_iRxDeviceId,
                route.m_iRxTransceiverIndex, budget.m_sTimebaseId, budget.m_fNoiseDbm, route.m_iRxGeneration)) return null;
            if (nowMs == int.MAX || !entry.m_RxEnvironment.Covers(nowMs, nowMs + 1)) return null;
            budget.m_RxEnvironment = entry.m_RxEnvironment.CloneEnvironment();
        }
        return budget;
    }

    //! A readable source with no eligible direct pairs returns an empty, successful projection.
    bool ReadDirectMediaCandidates(RICON_MediaStreamBinding source, RICON_LifecycleStateMachine lifecycle, int nowMs,
        array<ref RICON_MediaDirectCandidate> candidates)
    {
        if (!candidates) return false;
        candidates.Clear();
        if (!CurrentMediaSource(source, lifecycle, nowMs) || m_Active.Count() > GetCaptureLimit()) return false;
        array<int> ids = {};
        for (int index = 0; index < m_Active.Count(); index++)
        {
            int linkId = m_Active.GetKey(index);
            if (CurrentDirectMediaCapture(linkId, m_Active.GetElement(index), source, lifecycle, nowMs)) ids.Insert(linkId);
        }
        ids.Sort();
        foreach (int id : ids)
        {
            RICON_TemporalSnapshot snapshot = m_Active.Get(id).m_Snapshot;
            RICON_MediaDirectCandidate candidate = new RICON_MediaDirectCandidate();
            candidate.m_iLinkId = id;
            candidate.m_iListenerPlayerId = snapshot.m_iListenerPlayerId;
            candidate.m_iRxDeviceId = snapshot.m_Binding.m_iRxDeviceId;
            candidate.m_iRxTransceiverIndex = snapshot.m_Binding.m_iRxTransceiverIndex;
            candidates.Insert(candidate);
        }
        return true;
    }

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
        if (m_ListenerRoster && accepted && accepted.m_sCode == "OK") m_ListenerRoster.ConsumeLifecycle(result, preEventEpoch);
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
                    || binding.m_iSessionEpoch != m_iEpoch || !lease
                    || !lease.MatchesSource(binding.m_iTxPlayerId, binding.m_iTxGeneration, binding.m_iTxDeviceId,
                        binding.m_iNetworkTypeId, binding.m_iChannelId)
                    || !binding.SameLeaseAuthority(lease))
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
        RICON_TemporalBinding binding = s.m_Binding;
        RICON_LifecycleLease lease = lifecycle.QueryActiveLease(binding.m_iLeaseId, nowMs);
        return lease && lease.MatchesSource(s.m_iTalkerPlayerId, binding.m_iTxGeneration, binding.m_iTxDeviceId,
            binding.m_iNetworkTypeId, binding.m_iChannelId)
            && lifecycle.GetPlayerState(s.m_iTalkerPlayerId) == RICON_EPlayerLifecycleState.BOUND_ACTIVE
            && lifecycle.GetPlayerGeneration(s.m_iTalkerPlayerId) == binding.m_iTxGeneration
            && ListenerCurrent(entry, lifecycle);
    }

    protected static bool ListenerCurrent(RICON_TemporalProducerEntry entry, RICON_LifecycleStateMachine lifecycle)
    {
        int listener = entry.m_Snapshot.m_iListenerPlayerId;
        return lifecycle.GetPlayerState(listener) == RICON_EPlayerLifecycleState.BOUND_ACTIVE
            && lifecycle.GetPlayerGeneration(listener) == entry.m_Snapshot.m_Binding.m_iRxGeneration
            && lifecycle.GetPlayerEntityId(listener) == entry.m_iReceiverEntityId;
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
        ref RICON_StaticLosGeometryAuthority geometry = tick.m_StaticLosGeometry;
        ref RICON_OpenRadioStaticLosSource staticRfSource = tick.m_StaticRfSource;
        ref RICON_OpenRadioListenerRoster roster = tick.m_ListenerRoster;
        int rosterRevision = tick.m_iListenerRosterRevision;
        if (m_ListenerRoster && m_ListenerRoster != roster) return PublicationResult("LISTENER_ROSTER_REQUIRED");
        if (roster && (rosterRevision <= 0 || roster.GetRevision() != rosterRevision || !roster.MatchesInput(tick.m_PairInput, lifecycle)))
            return PublicationResult("STALE_LISTENER_ROSTER");
        if (m_StaticLosGeometry && !geometry) return PublicationResult("LOS_GEOMETRY_REQUIRED");
        if (m_bCommittedStaticRfSource && geometry == m_StaticLosGeometry && m_CommittedStaticRfSource != staticRfSource)
            return PublicationResult("RF_CONTINUITY_SOURCE_CHANGED");
        if (geometry && !geometry.MatchesTick(tick, lifecycle)) return PublicationResult("LOS_GEOMETRY_INVALID");
        if (staticRfSource)
        {
            if (!geometry || !staticRfSource.MatchesBuiltRfTick(tick, nowMs)) return PublicationResult("RF_CONTINUITY_SOURCE_INVALID");
            RICON_WirelessLinkEvaluator fixedRfEvaluator = RICON_WirelessLinkEvaluator.Cast(evaluator);
            if (!fixedRfEvaluator || !fixedRfEvaluator.UsesFixedRfModels()) return PublicationResult("RF_CONTINUITY_MODELS_UNQUALIFIED");
        }
        if (geometry != m_StaticLosGeometry && m_iGeometryRevision >= int.MAX - 1) return PublicationResult("RESOURCE_LIMIT");
        if (geometry && m_StaticLosGeometry && geometry != m_StaticLosGeometry
            && geometry.GetEnvironmentRevision() <= m_StaticLosGeometry.GetEnvironmentRevision())
            return PublicationResult("LOS_GEOMETRY_REVISION");
        if (geometry && m_StaticLosGeometry && tick.m_sTimebaseId != m_sGeometryTimebaseId)
            return PublicationResult("LOS_TIMEBASE_MISMATCH");
        if (geometry && (geometry.GetLastModelNowMs() < m_iGeometryModelMs
            || (geometry.GetLastModelNowMs() == m_iGeometryModelMs && geometry.GetLastModelSubUs() < m_iGeometryModelSubUs)))
            return PublicationResult("CLOCK_ROLLBACK");
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
        if (tick.m_StaticLosGeometry != geometry || (geometry && !geometry.MatchesTick(tick, lifecycle)))
            return IssuanceFailure(calls, reads);
        if (tick.m_StaticRfSource != staticRfSource || (staticRfSource && !staticRfSource.MatchesBuiltRfTick(tick, nowMs)))
            return IssuanceFailure(calls, reads);
        if (tick.m_ListenerRoster != roster || tick.m_iListenerRosterRevision != rosterRevision
            || (roster && (roster.GetRevision() != rosterRevision || !roster.MatchesInput(tick.m_PairInput, lifecycle))))
            return IssuanceFailure(calls, reads);
        result.m_iSequenceCalls = calls;
        result.m_iEpochReads = reads;
        result.m_iReservedHighWater = m_iReservedHighWater;
        // No allocations, callbacks or fallible validation beyond this adoption boundary.
        m_Active = scratch.m_Active;
        m_Closed = scratch.m_Closed;
        m_iTotalBytes = scratch.m_iTotalBytes;
        m_iGlobalPrefix = scratch.m_iGlobalPrefix;
        m_Registry = candidate;
        if (geometry != m_StaticLosGeometry) m_iGeometryRevision++;
        m_StaticLosGeometry = geometry;
        m_CommittedStaticRfSource = staticRfSource;
        m_bCommittedStaticRfSource = staticRfSource != null;
        m_ListenerRoster = roster;
        m_iListenerRosterRevision = rosterRevision;
        if (geometry)
        {
            m_iGeometryModelMs = geometry.GetLastModelNowMs();
            m_iGeometryModelSubUs = geometry.GetLastModelSubUs();
            m_sGeometryTimebaseId = tick.m_sTimebaseId;
        }
        m_PublishedLifecycle = lifecycle;
        m_bLifecyclePinned = true;
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
        m_bLastPublishSucceeded = false;
        if (m_bLifecyclePinned && (!lifecycle || m_PublishedLifecycle != lifecycle))
            return PublicationResult("LIFECYCLE_MISMATCH");
        m_bPublishing = true;
        m_bInterference = false;
        RICON_TemporalPublicationResult result = PublishCore(tick, evaluator, lifecycle, lifecycleNowMs, source);
        m_bPublishing = false;
        m_bLastPublishSucceeded = result && result.m_sCode == "OK";
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
