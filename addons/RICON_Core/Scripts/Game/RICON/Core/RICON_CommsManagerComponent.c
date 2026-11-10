// Authority lifecycle and registries for the common RICON core (REQ-core §3, §7, §8.1).
// Status: STATIC_IMPLEMENTED (not compiled yet). No production models are registered implicitly.

class RICON_CommsManagerComponentClass : SCR_BaseGameModeComponentClass
{
}

//! Immutable input used to build one band-scoped evaluator after all model ids are resolved.
class RICON_BandConfig
{
	string m_sBandId;
	string m_sPropagationModelId;
	string m_sNoiseEnvironmentId;
	string m_sIntelligibilityModelId;
	int m_iConfigVersion;

	void RICON_BandConfig(
		string bandId,
		string propagationModelId,
		string noiseEnvironmentId,
		string intelligibilityModelId,
		int configVersion)
	{
		m_sBandId = bandId;
		m_sPropagationModelId = propagationModelId;
		m_sNoiseEnvironmentId = noiseEnvironmentId;
		m_sIntelligibilityModelId = intelligibilityModelId;
		m_iConfigVersion = configVersion;
	}

	bool IsValid()
	{
		return !m_sBandId.IsEmpty() && !m_sPropagationModelId.IsEmpty() && !m_sNoiseEnvironmentId.IsEmpty()
			&& !m_sIntelligibilityModelId.IsEmpty() && m_iConfigVersion > 0;
	}
}

//! Noise model plus the provenance of its versioned coefficients (REQ-core §7).
class RICON_NoiseRegistration
{
	ref RICON_NoiseModel m_Model;
	string m_sSourceId;
	int m_iSourceRevision;

	void RICON_NoiseRegistration(RICON_NoiseModel model, string sourceId, int sourceRevision)
	{
		m_Model = model;
		m_sSourceId = sourceId;
		m_iSourceRevision = sourceRevision;
	}

	bool IsValid()
	{
		return m_Model && m_Model.IsConfigValid() && !m_Model.GetEnvironmentId().IsEmpty()
			&& !m_sSourceId.IsEmpty() && m_iSourceRevision > 0;
	}
}

class RICON_CommsManagerComponent : SCR_BaseGameModeComponent
{
	// Process-local generation. It only distinguishes live world/session generations, not all historical servers.
	protected static int s_iProcessEpoch;

	protected int m_iSessionEpoch;
	protected bool m_bRegistriesFrozen;
	protected ref RICON_TemporalAuthorityJournal m_TemporalAuthorityJournal;
    protected string m_sTemporalProducerBandId;
    protected bool m_bTemporalProducerPublishing;
	protected string m_sLastRegistryError;

	protected ref map<string, ref RICON_PropagationModel> m_mPropagationById = new map<string, ref RICON_PropagationModel>();
	protected ref map<string, ref RICON_NoiseRegistration> m_mNoiseByEnvironment = new map<string, ref RICON_NoiseRegistration>();
	protected ref map<string, ref RICON_IntelligibilityModel> m_mIntelligibilityById = new map<string, ref RICON_IntelligibilityModel>();
	protected ref map<string, ref RICON_BandConfig> m_mBandConfigById = new map<string, ref RICON_BandConfig>();
	protected ref map<string, ref RICON_WirelessLinkEvaluator> m_mEvaluatorByBand = new map<string, ref RICON_WirelessLinkEvaluator>();
	protected ref map<string, ref RICON_RoutingPolicy> m_mRoutingPolicyByNetworkType = new map<string, ref RICON_RoutingPolicy>();
	protected ref map<string, ref RICON_CallSessionFactory> m_mCallFactoryByNetworkType = new map<string, ref RICON_CallSessionFactory>();
	protected ref map<int, int> m_mLinkSeqById = new map<int, int>();
	protected ref set<int> m_sConnectedPlayers = new set<int>();

	static RICON_CommsManagerComponent GetInstance()
	{
		ChimeraGame game = GetGame();
		if (!game)
			return null;

		BaseGameMode gameMode = game.GetGameMode();
		if (!gameMode)
			return null;
		return RICON_CommsManagerComponent.Cast(gameMode.FindComponent(RICON_CommsManagerComponent));
	}

	override void OnWorldPostProcess(World world)
	{
		super.OnWorldPostProcess(world);
		if (!IsAuthority())
			return;

		m_iSessionEpoch = AllocateSessionEpoch();
		m_mLinkSeqById.Clear();
	}

	//! OnPlayerConnected is documented server-only.
	override void OnPlayerConnected(int playerId)
	{
		super.OnPlayerConnected(playerId);
		m_sConnectedPlayers.Insert(playerId);
	}

	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		super.OnPlayerDisconnected(playerId, cause, timeout);
		if (IsAuthority())
			m_sConnectedPlayers.RemoveItem(playerId);
	}

	static bool IsAuthority()
	{
		return Replication.IsServer();
	}

	//! Register before FreezeRegistries. Duplicate/empty/null ids fail closed; no default fallback.
	bool RegisterPropagation(RICON_PropagationModel model)
	{
		if (!CanMutateRegistries("RegisterPropagation"))
			return false;
		if (!model || model.GetModelId().IsEmpty() || !model.IsConfigValid())
			return RegistryFailure("RegisterPropagation: invalid model, id or config");
		if (m_mPropagationById.Contains(model.GetModelId()))
			return RegistryFailure("RegisterPropagation: duplicate model id");
		m_mPropagationById.Set(model.GetModelId(), model);
		return true;
	}

	bool RegisterNoise(RICON_NoiseModel model, string sourceId, int sourceRevision)
	{
		if (!CanMutateRegistries("RegisterNoise"))
			return false;
		RICON_NoiseRegistration registration = new RICON_NoiseRegistration(model, sourceId, sourceRevision);
		if (!registration.IsValid())
			return RegistryFailure("RegisterNoise: invalid model or source metadata");
		string environmentId = model.GetEnvironmentId();
		if (m_mNoiseByEnvironment.Contains(environmentId))
			return RegistryFailure("RegisterNoise: duplicate environment id");
		m_mNoiseByEnvironment.Set(environmentId, registration);
		return true;
	}

	bool RegisterIntelligibility(RICON_IntelligibilityModel model)
	{
		if (!CanMutateRegistries("RegisterIntelligibility"))
			return false;
		if (!model || model.GetModelId().IsEmpty() || !model.IsConfigValid())
			return RegistryFailure("RegisterIntelligibility: invalid model or config");
		if (m_mIntelligibilityById.Contains(model.GetModelId()))
			return RegistryFailure("RegisterIntelligibility: duplicate model id");
		m_mIntelligibilityById.Set(model.GetModelId(), model);
		return true;
	}

	//! Resolve one immutable Band -> dependencies mapping and create the pure evaluator.
	bool RegisterBand(RICON_BandConfig config)
	{
		if (!CanMutateRegistries("RegisterBand"))
			return false;
		if (!config || !config.IsValid())
			return RegistryFailure("RegisterBand: invalid config");
		if (m_mBandConfigById.Contains(config.m_sBandId))
			return RegistryFailure("RegisterBand: duplicate band id");

		RICON_PropagationModel propagation = m_mPropagationById.Get(config.m_sPropagationModelId);
		RICON_NoiseRegistration noiseRegistration = m_mNoiseByEnvironment.Get(config.m_sNoiseEnvironmentId);
		RICON_IntelligibilityModel intelligibility = m_mIntelligibilityById.Get(config.m_sIntelligibilityModelId);
		if (!propagation || !noiseRegistration || !intelligibility)
			return RegistryFailure("RegisterBand: one or more dependency ids are unknown");

		// Store an internal copy so the caller cannot mutate the registered mapping after validation.
		RICON_BandConfig stored = new RICON_BandConfig(
			config.m_sBandId,
			config.m_sPropagationModelId,
			config.m_sNoiseEnvironmentId,
			config.m_sIntelligibilityModelId,
			config.m_iConfigVersion);
		RICON_WirelessLinkEvaluator evaluator = new RICON_WirelessLinkEvaluator(
			stored.m_sBandId,
			propagation,
			noiseRegistration.m_Model,
			intelligibility);
		m_mBandConfigById.Set(stored.m_sBandId, stored);
		m_mEvaluatorByBand.Set(stored.m_sBandId, evaluator);
		return true;
	}

	//! Register one NetworkType atomically. A call factory is optional unless the policy requires one.
	bool RegisterNetworkType(RICON_RoutingPolicy policy, RICON_CallSessionFactory callFactory = null)
	{
		if (!CanMutateRegistries("RegisterNetworkType"))
			return false;
		if (!policy || policy.GetNetworkTypeId().IsEmpty() || !policy.IsConfigValid())
			return RegistryFailure("RegisterNetworkType: invalid policy, id or config");

		string networkTypeId = policy.GetNetworkTypeId();
		if (m_mRoutingPolicyByNetworkType.Contains(networkTypeId))
			return RegistryFailure("RegisterNetworkType: duplicate network type id");
		if (policy.RequiresCallSessionFactory() && !callFactory)
			return RegistryFailure("RegisterNetworkType: required call-session factory is missing");
		if (callFactory && (!callFactory.IsConfigValid() || callFactory.GetNetworkTypeId() != networkTypeId))
			return RegistryFailure("RegisterNetworkType: factory config or network type mismatch");

		m_mRoutingPolicyByNetworkType.Set(networkTypeId, policy);
		if (callFactory)
			m_mCallFactoryByNetworkType.Set(networkTypeId, callFactory);
		return true;
	}

	//! No registration is allowed after freeze. At least one fully resolved band is required.
	bool FreezeRegistries()
	{
		if (!CanMutateRegistries("FreezeRegistries"))
			return false;
		if (m_mEvaluatorByBand.IsEmpty() && m_mRoutingPolicyByNetworkType.IsEmpty())
			return RegistryFailure("FreezeRegistries: no resolved bands or network types");
		m_bRegistriesFrozen = true;
		return true;
	}

	RICON_WirelessLinkEvaluator GetBandEvaluator(string bandId)
	{
		if (!m_bRegistriesFrozen || bandId.IsEmpty())
			return null;
		return m_mEvaluatorByBand.Get(bandId);
	}

	RICON_RoutingPolicy GetRoutingPolicy(string networkTypeId)
	{
		if (!m_bRegistriesFrozen || networkTypeId.IsEmpty())
			return null;
		return m_mRoutingPolicyByNetworkType.Get(networkTypeId);
	}

	RICON_CallSessionFactory GetCallSessionFactory(string networkTypeId)
	{
		if (!m_bRegistriesFrozen || networkTypeId.IsEmpty())
			return null;
		return m_mCallFactoryByNetworkType.Get(networkTypeId);
	}

	string GetNoiseSourceId(string environmentId)
	{
		RICON_NoiseRegistration registration = m_mNoiseByEnvironment.Get(environmentId);
		if (!registration)
			return string.Empty;
		return registration.m_sSourceId;
	}

	int GetNoiseSourceRevision(string environmentId)
	{
		RICON_NoiseRegistration registration = m_mNoiseByEnvironment.Get(environmentId);
		if (!registration)
			return 0;
		return registration.m_iSourceRevision;
	}

	bool AreRegistriesFrozen()
	{
		return m_bRegistriesFrozen;
	}

	string GetLastRegistryError()
	{
		return m_sLastRegistryError;
	}

	int GetSessionEpoch()
	{
		return m_iSessionEpoch;
	}

	//! Used before linkSeq would reach int.MAX. Full-resync effects are handled by the snapshot service.
	bool RotateSessionEpoch()
	{
        if (!m_sTemporalProducerBandId.IsEmpty() && !m_bTemporalProducerPublishing)
            return false;
		if (!IsAuthority())
			return false;
		int previous = m_iSessionEpoch;
		m_iSessionEpoch = AllocateSessionEpoch();
		if (m_iSessionEpoch <= 0 || m_iSessionEpoch >= int.MAX || m_iSessionEpoch == previous)
			return false;
		m_mLinkSeqById.Clear();
		return true;
	}

	//! Allocate a per-link sequence. Before int.MAX it rotates epoch, clears every link and starts from 1.
	bool NextLinkSequence(int linkId, out int linkSeq, out bool rotated)
	{
		linkSeq = 0;
		rotated = false;
        if (!m_sTemporalProducerBandId.IsEmpty() && !m_bTemporalProducerPublishing)
            return false;
		if (!IsAuthority() || linkId <= 0 || m_iSessionEpoch <= 0 || m_iSessionEpoch >= int.MAX)
			return false;

		int current;
		if (!m_mLinkSeqById.Find(linkId, current))
			current = 0;
		if (current >= int.MAX - 1)
		{
			if (!RotateSessionEpoch())
				return false;
			rotated = true;
			linkSeq = 1;
			m_mLinkSeqById.Set(linkId, linkSeq);
			return true;
		}

		linkSeq = current + 1;
		m_mLinkSeqById.Set(linkId, linkSeq);
		return true;
	}

	protected bool CanMutateRegistries(string operation)
	{
		if (!IsAuthority())
			return RegistryFailure(operation + ": authority required");
		if (m_bRegistriesFrozen)
			return RegistryFailure(operation + ": registries are frozen");
		return true;
	}

	protected bool RegistryFailure(string message)
	{
		m_sLastRegistryError = message;
		PrintFormat("[RICON] registry rejected: %1", message);
		return false;
	}

    protected static RICON_TemporalPublicationResult ProducerFailure(string code, bool resync = false)
    {
        RICON_TemporalPublicationResult result = new RICON_TemporalPublicationResult();
        result.m_sCode = code; result.m_bRequiresFullResync = resync;
        return result;
    }

    //! Explicit fresh-scope opt-in; scalar v1 and manual capture cannot share this ID namespace.
    bool ConfigureTemporalProducerV2(string bandId, RICON_LinkPublicationLimits publicationLimits, RICON_TemporalLimits temporalLimits)
    {
        if (!IsAuthority() || !m_bRegistriesFrozen || m_iSessionEpoch <= 0 || m_iSessionEpoch >= int.MAX)
            return false;
        if (m_TemporalAuthorityJournal || m_mLinkSeqById.Count() != 0 || bandId.IsEmpty())
            return false;
        if (!publicationLimits || !temporalLimits || !temporalLimits.IsValid())
            return false;
        if (publicationLimits.m_iMaxBindings < 1 || publicationLimits.m_iMaxBindings > 4096)
            return false;
        if (publicationLimits.m_iMaxPairs < 1 || publicationLimits.m_iMaxPairs > 65536
            || publicationLimits.m_iMaxActiveLinks < 1 || publicationLimits.m_iMaxActiveLinks > 65536)
            return false;
        if (!GetBandEvaluator(bandId))
            return false;
        RICON_TemporalPublicationOwner owner = new RICON_TemporalPublicationOwner(m_iSessionEpoch, temporalLimits, publicationLimits);
        if (owner.GetSessionEpoch() != m_iSessionEpoch || owner.GetCaptureLimit() != temporalLimits.m_iMaxSnapshots)
            return false;
        m_TemporalAuthorityJournal = owner; m_sTemporalProducerBandId = bandId;
        return true;
    }

    //! Source-only entrypoint: the owner calls the real PairService with this registered evaluator.
    RICON_TemporalPublicationResult PublishTemporalPairs2(RICON_TemporalProducerTick tick, RICON_LifecycleStateMachine lifecycle, int lifecycleNowMs)
    {
        if (!IsAuthority()) return ProducerFailure("AUTHORITY_REQUIRED");
        RICON_TemporalPublicationOwner owner = RICON_TemporalPublicationOwner.Cast(m_TemporalAuthorityJournal);
        if (!owner || m_sTemporalProducerBandId.IsEmpty()) return ProducerFailure("PRODUCER_DISABLED");
        if (m_bTemporalProducerPublishing) return ProducerFailure("PRODUCER_BUSY");
        if (!tick || !tick.m_PairInput || !tick.m_PairInput.m_Context || !lifecycle)
            return ProducerFailure("INVALID_INPUT");
        if (tick.m_PairInput.m_Context.m_sBandId != m_sTemporalProducerBandId)
            return ProducerFailure("BAND_MISMATCH");
        if (tick.m_PairInput.m_iSessionEpoch != m_iSessionEpoch || lifecycle.GetSessionEpoch() != m_iSessionEpoch
            || owner.GetSessionEpoch() != m_iSessionEpoch)
            return ProducerFailure("SCOPE_MISMATCH", true);
        RICON_LinkEvaluator evaluator = GetBandEvaluator(m_sTemporalProducerBandId);
        if (!evaluator) return ProducerFailure("UNKNOWN_BAND");
        RICON_ManagerLinkSequenceSource source = new RICON_ManagerLinkSequenceSource(this);
        m_bTemporalProducerPublishing = true;
        RICON_TemporalPublicationResult result = owner.Publish(tick, evaluator, lifecycle, lifecycleNowMs, source);
        m_bTemporalProducerPublishing = false;
        return result;
    }

    RICON_TemporalProducerRegistry ReadTemporalProducerRegistry()
    {
        if (!IsAuthority()) return null;
        RICON_TemporalPublicationOwner owner = RICON_TemporalPublicationOwner.Cast(m_TemporalAuthorityJournal);
        if (!owner) return null;
        return owner.ReadProducerRegistry();
    }

    //! Internal all-owner source handoff; it makes no network FULL/ACK claim.
    RICON_TemporalPublicationResult BuildTemporalProducerFullHandoff()
    {
        if (!IsAuthority()) return ProducerFailure("AUTHORITY_REQUIRED");
        RICON_TemporalPublicationOwner owner = RICON_TemporalPublicationOwner.Cast(m_TemporalAuthorityJournal);
        if (!owner || m_sTemporalProducerBandId.IsEmpty()) return ProducerFailure("PRODUCER_DISABLED");
        if (m_bTemporalProducerPublishing) return ProducerFailure("PRODUCER_BUSY");
        m_bTemporalProducerPublishing = true;
        RICON_TemporalPublicationResult result = owner.BuildFullSourceHandoff();
        m_bTemporalProducerPublishing = false;
        return result;
    }

    //! Explicit server-only opt-in. Reconfiguration cannot silently drop an old scope.
    bool ConfigureTemporalAuthority(RICON_TemporalLimits limits)
    {
        if (!IsAuthority() || m_iSessionEpoch <= 0 || m_iSessionEpoch >= int.MAX || !limits || !limits.IsValid())
            return false;
        if (m_TemporalAuthorityJournal)
            return m_TemporalAuthorityJournal.GetSessionEpoch() == m_iSessionEpoch && m_TemporalAuthorityJournal.MatchesLimits(limits);
        m_TemporalAuthorityJournal = new RICON_TemporalAuthorityJournal(m_iSessionEpoch, limits);
        return true;
    }

    int GetTemporalCaptureEpoch()
    {
        if (!IsAuthority() || !m_TemporalAuthorityJournal) return 0;
        return m_TemporalAuthorityJournal.GetSessionEpoch();
    }

    //! Read-only admission shared by the single and batch capture paths.
    protected RICON_TemporalCaptureResult PrepareTemporalCapture(RICON_TemporalSnapshot snapshot, RICON_TemporalExpected expected, RICON_LifecycleStateMachine lifecycle, int lifecycleNowMs, out RICON_TemporalCaptureInput row)
    {
        row = null;
        if (!snapshot || !snapshot.m_Binding) return RICON_TemporalAuthorityJournal.Result("INVALID_SNAPSHOT");
        if (m_TemporalAuthorityJournal.GetSessionEpoch() != m_iSessionEpoch || snapshot.m_iSessionEpoch != m_iSessionEpoch)
            return RICON_TemporalAuthorityJournal.Result("SCOPE_MISMATCH", true);
        int issued;
        if (!m_mLinkSeqById.Find(snapshot.m_iLinkId, issued) || issued != snapshot.m_iLinkSeq)
            return RICON_TemporalAuthorityJournal.Result("UNISSUED_SEQUENCE", true);
        if (!lifecycle || lifecycle.GetSessionEpoch() != m_iSessionEpoch
            || lifecycle.GetPlayerState(snapshot.m_iTalkerPlayerId) != RICON_EPlayerLifecycleState.BOUND_ACTIVE
            || lifecycle.GetPlayerState(snapshot.m_iListenerPlayerId) != RICON_EPlayerLifecycleState.BOUND_ACTIVE
            || lifecycle.GetPlayerGeneration(snapshot.m_iTalkerPlayerId) != snapshot.m_Binding.m_iTxGeneration
            || lifecycle.GetPlayerGeneration(snapshot.m_iListenerPlayerId) != snapshot.m_Binding.m_iRxGeneration)
            return RICON_TemporalAuthorityJournal.Result("STALE_LIFECYCLE_CONTEXT", true);
        // Lifecycle time is supplied by the authoritative caller, not a descriptor anchor or presentation TTL.
        RICON_LifecycleLease sourceLease = lifecycle.QueryActiveLease(snapshot.m_Binding.m_iLeaseId, lifecycleNowMs);
        if (!sourceLease || sourceLease.m_iLeaseId != snapshot.m_Binding.m_iLeaseId
            || sourceLease.m_iPlayerId != snapshot.m_iTalkerPlayerId
            || sourceLease.m_iGeneration != snapshot.m_Binding.m_iTxGeneration
            || sourceLease.m_iDeviceId != snapshot.m_Binding.m_iTxDeviceId)
            return RICON_TemporalAuthorityJournal.Result("LEASE_CONTEXT_UNAVAILABLE", true);
        int receiverEntityId = lifecycle.GetPlayerEntityId(snapshot.m_iListenerPlayerId);
        if (receiverEntityId <= 0) return RICON_TemporalAuthorityJournal.Result("INVALID_RECEIVER_ENTITY");
        row = new RICON_TemporalCaptureInput(snapshot, expected, receiverEntityId);
        return RICON_TemporalAuthorityJournal.Result("OK");
    }

    //! Expected provenance and an actual producer commit remain the caller's responsibility.
    RICON_TemporalCaptureResult CaptureTemporalPublication(RICON_TemporalSnapshot snapshot, RICON_TemporalExpected expected, RICON_LifecycleStateMachine lifecycle, int lifecycleNowMs)
    {
        if (!IsAuthority()) return RICON_TemporalAuthorityJournal.Result("AUTHORITY_REQUIRED");
        if (!m_TemporalAuthorityJournal) return RICON_TemporalAuthorityJournal.Result("CAPTURE_DISABLED");
        if (!m_sTemporalProducerBandId.IsEmpty()) return RICON_TemporalAuthorityJournal.Result("PRODUCER_OWNER_REQUIRED");
        RICON_TemporalCaptureInput row;
        RICON_TemporalCaptureResult admission = PrepareTemporalCapture(snapshot, expected, lifecycle, lifecycleNowMs, row);
        if (admission.m_sCode != "OK") return admission;
        return m_TemporalAuthorityJournal.CapturePublished(row.m_Snapshot, row.m_Expected, row.m_iReceiverEntityId);
    }

    //! All source admission completes before the journal can commit any row; no RPC or dispatch.
    RICON_TemporalCaptureResult CaptureTemporalPublications(array<ref RICON_TemporalSnapshot> snapshots, map<int, ref RICON_TemporalExpected> expectedByLink, RICON_LifecycleStateMachine lifecycle, int lifecycleNowMs)
    {
        if (!IsAuthority()) return RICON_TemporalAuthorityJournal.Result("AUTHORITY_REQUIRED");
        if (!m_TemporalAuthorityJournal) return RICON_TemporalAuthorityJournal.Result("CAPTURE_DISABLED");
        if (!m_sTemporalProducerBandId.IsEmpty()) return RICON_TemporalAuthorityJournal.Result("PRODUCER_OWNER_REQUIRED");
        if (!snapshots || !expectedByLink) return RICON_TemporalAuthorityJournal.Result("INVALID_INPUT");
        if (snapshots.Count() > m_TemporalAuthorityJournal.GetCaptureLimit())
            return RICON_TemporalAuthorityJournal.Result("BATCH_LIMIT");
        array<ref RICON_TemporalCaptureInput> rows = {};
        foreach (RICON_TemporalSnapshot snapshot : snapshots)
        {
            RICON_TemporalExpected expected;
            if (snapshot) expected = expectedByLink.Get(snapshot.m_iLinkId);
            RICON_TemporalCaptureInput row;
            RICON_TemporalCaptureResult admission = PrepareTemporalCapture(snapshot, expected, lifecycle, lifecycleNowMs, row);
            if (admission.m_sCode != "OK") return admission;
            rows.Insert(row);
        }
        return m_TemporalAuthorityJournal.CapturePublishedBatch(rows);
    }

    //! Capture preEventEpoch from the actual Lifecycle before calling it.
    //! A rotation result can contain old-epoch expiries even when the manager has already advanced.
    RICON_TemporalCaptureResult ConsumeTemporalLifecycleResult(RICON_LifecycleResult result, int preEventEpoch)
    {
        if (!IsAuthority()) return RICON_TemporalAuthorityJournal.Result("AUTHORITY_REQUIRED");
        if (!m_TemporalAuthorityJournal) return RICON_TemporalAuthorityJournal.Result("CAPTURE_DISABLED");
        return m_TemporalAuthorityJournal.ConsumeLifecycleResult(result, preEventEpoch);
    }

    //! Internal producer handoff only; never feed an untrusted client checkpoint into this method.
    RICON_TemporalCaptureResult CommitTemporalSourceCheckpoint(int sourceEpoch, int highWater, array<int> activeIds)
    {
        if (!IsAuthority()) return RICON_TemporalAuthorityJournal.Result("AUTHORITY_REQUIRED");
        if (!m_TemporalAuthorityJournal) return RICON_TemporalAuthorityJournal.Result("CAPTURE_DISABLED");
        return m_TemporalAuthorityJournal.CommitSourceCheckpoint(sourceEpoch, highWater, activeIds);
    }

    RICON_TemporalClosureIntent ReadTemporalClosure(int linkId)
    {
        if (!IsAuthority() || !m_TemporalAuthorityJournal) return null;
        return m_TemporalAuthorityJournal.ReadPending(linkId);
    }

	//! Starts with Unix seconds and then increments process-locally across world/session generations.
	//! If script state is rebuilt, clients must establish a fresh session; global historical uniqueness is not claimed.
	protected static int AllocateSessionEpoch()
	{
		if (s_iProcessEpoch <= 0 || s_iProcessEpoch >= int.MAX - 1)
		{
			s_iProcessEpoch = System.GetUnixTime();
			if (s_iProcessEpoch <= 0 || s_iProcessEpoch >= int.MAX)
				s_iProcessEpoch = 1;
			return s_iProcessEpoch;
		}

		s_iProcessEpoch++;
		return s_iProcessEpoch;
	}
}
