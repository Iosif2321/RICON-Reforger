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

//! Trusted gameplay input. Wakeup cadence is independent of the Lifecycle clock and RF observation.
class RICON_OpenRadioTickSource
{
	int ReadNowMs() { return -1; }
	RICON_TemporalProducerTick BuildTick(int nowMs) { return null; }
}

//! Game queue controls wakeups only. The provider supplies authoritative time for each callback.
class RICON_OpenRadioTimer
{
	//! Exactly one owner-side subscriber. Stop may run during notification.
	ref ScriptInvoker<int> m_OnTick = new ScriptInvoker<int>();
	protected ScriptCallQueue m_Queue;
	protected ref RICON_OpenRadioTickSource m_Source;
	protected int m_iGeneration;
	protected bool m_bWaking;

	bool IsWaking() { return m_bWaking; }

	bool Start(ScriptCallQueue queue, RICON_OpenRadioTickSource source, int intervalMs)
	{
		if (m_bWaking || m_Source || !queue || !source || intervalMs <= 0 || m_iGeneration >= int.MAX - 1) return false;
		m_Queue = queue;
		m_Source = source;
		m_iGeneration++;
		m_Queue.CallLater(Wake, intervalMs, true, m_iGeneration);
		return true;
	}

	void Stop()
	{
		if (m_Queue) m_Queue.Remove(Wake);
		m_Queue = null;
		m_Source = null;
	}

	protected void Wake(int generation)
	{
		if (m_bWaking || !m_Source || generation != m_iGeneration) return;
		m_bWaking = true;
		ref RICON_OpenRadioTickSource source = m_Source;
		int nowMs = source.ReadNowMs();
		if (m_Source && generation == m_iGeneration) m_OnTick.Invoke(nowMs);
		m_bWaking = false;
	}
}

class RICON_CommsRelayFanoutSink : RICON_MediaRelayFanoutSink
{
	protected RICON_CommsManagerComponent m_Manager;

	void RICON_CommsRelayFanoutSink(RICON_CommsManagerComponent manager) { m_Manager = manager; }

	override bool Offer(RICON_MediaFanout fanout, RICON_QueuedMediaBlock block, int deadlineMs, int nowMs)
	{
		return m_Manager && m_Manager.StageSharedRelayInput(this, fanout, block, deadlineMs, nowMs);
	}
}

class RICON_CommsManagerComponent : SCR_BaseGameModeComponent
{
	// Process-local generation. It only distinguishes live world/session generations, not all historical servers.
	protected static int s_iProcessEpoch;

	protected int m_iSessionEpoch;
	protected bool m_bRegistriesFrozen;
	protected ref RICON_TemporalAuthorityJournal m_TemporalAuthorityJournal;
	protected ref RICON_GameControllerLifecycle m_GameControllerLifecycle;
	protected ref RICON_MediaReceiverAuthority m_MediaReceiverAuthority;
	protected ref RICON_MediaListenerAuthority m_MediaListenerAuthority;
	protected ref RICON_RelayStationAuthority m_RelayStationAuthority;
	protected ref RICON_StaticLosGeometryAuthority m_RelayGeometry;
	protected ref RICON_StaticLosGeometryAuthority m_RelayInputGeometry;
	protected ref array<ref RICON_OpenRadioPairEndpoint> m_RelayInputEndpoints;
	protected ref RICON_RadioQueryContext m_RelayInputContext;
	protected ref RICON_RelayInputFrame m_RelayInputFrame;
	protected ref RICON_RelayInputSeries m_RelayInputSeries;
	protected int m_iRelayFirstRxSampleIndex, m_iRelayNextOriginSequence;
	protected string m_sRelayMappedTimebaseId;
	protected int m_iRelayGridDeadlineMs;
	protected RICON_MediaFanout m_RelayInputFanout;
	protected ref RICON_CommsRelayFanoutSink m_RelayFanoutSink;
	protected ref RICON_QueuedMediaBlock m_RelaySharedInput;
	protected ref RICON_RelayFrameReservation m_RelayReservation;
	protected int m_iRelayInputSealedAtMs, m_iRelayInputDeadlineMs;
	protected static const int RELAY_INPUT_MAX_AGE_MS = 250;
	protected float m_fRelayInputReferenceDbm;
	protected bool m_bRelayInputTaken;
	protected ref RICON_RelayPcmHandoff m_RelayInput;
	protected ref RICON_RelayRfOutput m_RelayOutput;
	protected ref RICON_RelayPcmPath m_RelayPath;
	protected ref RICON_RelayDestination m_RelayDestination;
	protected ref RICON_RelayWireGrant m_RelayWireGrant;
	protected ref RICON_RelayWirePublication m_RelayWirePublication;
	protected ref RICON_RelayWireReady m_RelayWireReady;
	protected static const int RELAY_READY_TIMEOUT_MS = 500;
	protected static const int RELAY_ACK_TIMEOUT_MS = 500;
	protected bool m_bRelayInstallSubmitted;
	protected RICON_RelayWireGrant m_RelayRevokeInFlight;
	protected SCR_PlayerController m_RelayRevokeRecipient;
	protected RICON_RelayAddressedFrame m_RelayMediaInFlight;
	protected SCR_PlayerController m_RelayMediaRecipient;
	protected array<int> m_RelayMediaBody;
	protected bool m_bRelayMediaSubmitting;
	protected RICON_LifecycleStateMachine m_RelayLifecycle;
	protected bool m_bRelayConfiguring, m_bRelayOutputInstalled;
	protected ref RICON_MediaRpcTransport m_MediaWireTransport;
	protected int m_iOpenRadioNetworkId;
	protected ref RICON_OpenRadioTickSource m_OpenRadioTickSource;
	protected ref RICON_OpenRadioTimer m_OpenRadioTimer;
	protected ref RICON_MediaFanoutTransport m_OpenRadioTransport;
	protected RICON_LifecycleStateMachine m_OpenRadioLifecycle;
	protected RICON_MediaStreamOwner m_OpenRadioSourceOwner;
	protected int m_iOpenRadioVisits, m_iOpenRadioMaxCaptureAgeMs;
	protected int m_iLastOpenRadioAttemptMs = -1;
	protected bool m_bOpenRadioRunning;
	protected bool m_bOpenRadioPcmUplink;
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
	protected ref map<int, SCR_PlayerController> m_mRegisteredControllers = new map<int, SCR_PlayerController>();

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

		CloseMediaReceiverAuthority();
		m_iSessionEpoch = AllocateSessionEpoch();
		m_mLinkSeqById.Clear();
		m_sConnectedPlayers.Clear();
		m_mRegisteredControllers.Clear();
	}

	//! OnPlayerConnected is documented server-only.
	override void OnPlayerConnected(int playerId)
	{
		super.OnPlayerConnected(playerId);
		if (!IsAuthority() || playerId <= 0) return;
		m_sConnectedPlayers.Insert(playerId);
		m_mRegisteredControllers.Remove(playerId);
		if (m_GameControllerLifecycle) m_GameControllerLifecycle.Reconcile(playerId);
	}

	bool HasRegisteredPlayerController(int playerId)
	{
		SCR_PlayerController controller;
		return playerId > 0 && m_mRegisteredControllers.Find(playerId, controller)
			&& controller && controller.GetPlayerId() == playerId;
	}

	override void OnPlayerRegistered(int playerId)
	{
		super.OnPlayerRegistered(playerId);
		if (!IsAuthority() || playerId <= 0 || !GetGame()) return;
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players) return;
		SCR_PlayerController controller = SCR_PlayerController.Cast(players.GetPlayerController(playerId));
		if (!controller || controller.GetPlayerId() != playerId) return;
		m_mRegisteredControllers.Set(playerId, controller);
		if (m_GameControllerLifecycle) m_GameControllerLifecycle.Reconcile(playerId, true);
	}

	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		super.OnPlayerDisconnected(playerId, cause, timeout);
		if (!IsAuthority() || playerId <= 0) return;
		if (m_GameControllerLifecycle)
		{
			m_GameControllerLifecycle.OnDisconnected(playerId);
		ChimeraGame game = GetGame();
		if (!game) return;
		PlayerManager players = game.GetPlayerManager();
			if (!players || players.GetPlayerController(playerId) || players.IsPlayerConnected(playerId)) return;
		}
		else if (m_MediaListenerAuthority) m_MediaListenerAuthority.ForgetPlayer(playerId);
		m_sConnectedPlayers.RemoveItem(playerId);
		m_mRegisteredControllers.Remove(playerId);
	}

	static bool IsAuthority()
	{
		return Replication.IsServer();
	}

	override void OnDelete(IEntity owner)
	{
		CloseMediaReceiverAuthority();
		super.OnDelete(owner);
	}

	override void OnGameEnd()
	{
		CloseMediaReceiverAuthority();
		m_sConnectedPlayers.Clear();
		m_mRegisteredControllers.Clear();
		super.OnGameEnd();
	}

	protected void CloseMediaReceiverAuthority()
	{
		StopOpenRadioLoop();
		InvalidateRelayOutput();
		if (m_RelayStationAuthority) m_RelayStationAuthority.Close();
		m_RelayStationAuthority = null;
		m_RelayLifecycle = null;
		m_bRelayOutputInstalled = false;
		if (m_MediaWireTransport) m_MediaWireTransport.Close();
		m_MediaWireTransport = null;
		if (m_GameControllerLifecycle) m_GameControllerLifecycle.Close();
		m_GameControllerLifecycle = null;
		m_iLastOpenRadioAttemptMs = -1;
		if (m_MediaListenerAuthority) m_MediaListenerAuthority.Close();
		m_MediaListenerAuthority = null;
		m_iOpenRadioNetworkId = 0;
		if (m_MediaReceiverAuthority) m_MediaReceiverAuthority.Close();
		m_MediaReceiverAuthority = null;
	}

	//! Explicit server opt-in. Keep one publisher and sequence space per manager epoch.
	RICON_MediaRpcTransport EnableMediaWireTransport(RICON_LifecycleStateMachine lifecycle, int maxRoutes = 64,
		int ackTimeoutMs = 500, int maxUsedPolicies = 4096, string rfTimebaseId = "", bool historicalRf = false)
	{
		if (!IsAuthority() || m_MediaWireTransport || !m_GameControllerLifecycle || !m_MediaListenerAuthority) return null;
		ref RICON_MediaRpcTransport transport = new RICON_MediaRpcTransport();
		if (!transport.Configure(m_GameControllerLifecycle, lifecycle, maxRoutes, ackTimeoutMs, maxUsedPolicies, rfTimebaseId, historicalRf)) return null;
		m_MediaWireTransport = transport;
		return transport;
	}

	//! Opt-in by the active GameMode after installing its authority journal.
	RICON_GameControllerLifecycle EnableGameControllerLifecycle(RICON_LifecycleStateMachine lifecycle,
		int processAnchorMs, int maxPlayers, bool allowStandaloneLocal = false)
	{
		if (!IsAuthority() || m_GameControllerLifecycle || m_MediaListenerAuthority || !lifecycle) return null;
		if (!m_TemporalAuthorityJournal || m_TemporalAuthorityJournal.GetSessionEpoch() != m_iSessionEpoch) return null;
		if (m_MediaReceiverAuthority && !m_MediaReceiverAuthority.IsScope(lifecycle)) return null;
		ref RICON_GameControllerLifecycle bridge = new RICON_GameControllerLifecycle();
		if (!bridge.Configure(this, lifecycle, processAnchorMs, maxPlayers, allowStandaloneLocal)) return null;
		m_GameControllerLifecycle = bridge;
		PlayerManager players = GetGame().GetPlayerManager();
		if (players)
		{
			foreach (int playerId, SCR_PlayerController registered : m_mRegisteredControllers)
			{
				if (registered && players.GetPlayerController(playerId) == registered) bridge.Reconcile(playerId, true);
			}
		}
		// A failed replay may already have changed Lifecycle. Keep the failed scope closed until the epoch ends.
		if (!bridge.MatchesLifecycle(lifecycle)) return null;
		return bridge;
	}

	//! The server gameplay owner explicitly supplies the same Lifecycle used to publish Temporal pairs.
	RICON_MediaReceiverAuthority ConfigureMediaReceiverAuthority(RICON_LifecycleStateMachine lifecycle, int maxSlots)
	{
		if (!IsAuthority() || m_MediaReceiverAuthority || !lifecycle || lifecycle.GetSessionEpoch() != m_iSessionEpoch) return null;
		if (m_GameControllerLifecycle && !m_GameControllerLifecycle.MatchesLifecycle(lifecycle)) return null;
		RICON_MediaReceiverAuthority authority = new RICON_MediaReceiverAuthority();
		if (!authority.Configure(lifecycle, maxSlots)) return null;
		m_MediaReceiverAuthority = authority;
		return authority;
	}

	RICON_MediaListenerAuthority ConfigureMediaListenerAuthority(RICON_LifecycleStateMachine lifecycle,
		RICON_MediaStreamOwner sourceOwner, int maxGrants, int openRadioNetworkId)
	{
		if (!IsAuthority() || m_MediaListenerAuthority || !m_MediaReceiverAuthority || openRadioNetworkId <= 0) return null;
		RICON_MediaListenerAuthority authority = new RICON_MediaListenerAuthority();
		if (!authority.Configure(lifecycle, sourceOwner, m_MediaReceiverAuthority, maxGrants)) return null;
		m_MediaListenerAuthority = authority;
		m_iOpenRadioNetworkId = openRadioNetworkId;
		return authority;
	}

	//! Explicit bootstrap for one fixed directional station grant in this authority epoch.
	RICON_RelayStationAuthority ConfigureRelayStationAuthority(RICON_RelayPcmPath path, RICON_RelayStationDirection direction,
		RICON_LifecycleStateMachine lifecycle, RICON_MediaStreamOwner sourceOwner)
	{
		if (!IsAuthority() || m_RelayStationAuthority || !m_MediaReceiverAuthority || !m_GameControllerLifecycle
			|| !m_GameControllerLifecycle.MatchesLifecycle(lifecycle) || !m_TemporalAuthorityJournal
			|| m_TemporalAuthorityJournal.GetSessionEpoch() != m_iSessionEpoch || !path || !path.IsValid() || !direction) return null;
		int nowMs = m_GameControllerLifecycle.ReadWireAuthorityTime(lifecycle);
		if (nowMs < 0) return null;
		RICON_MediaStreamBinding origin = path.m_UpstreamRoute.m_Source;
		if (!MatchesRelayPlayer(origin.m_iPlayerId, origin.m_iConnectionId, origin.m_iPlayerGeneration, origin.m_iEntityId)
			|| !MatchesRelayPlayer(direction.m_iListenerPlayerId, direction.m_iListenerConnectionId,
				direction.m_iListenerGeneration, direction.m_iListenerEntityId)) return null;
		RICON_RelayStationAuthority authority = new RICON_RelayStationAuthority();
		if (!authority.Configure(path, direction, lifecycle, sourceOwner, m_MediaReceiverAuthority, nowMs)) return null;
		m_RelayStationAuthority = authority;
		m_RelayLifecycle = lifecycle;
		return authority;
	}

	//! Trusted world bootstrap only. The manager creates the actual seal, handoff and RF output.
	bool ConfigureRelayOutput(RICON_RelayPcmPath path, RICON_OpenRadioPairEndpoint stationTx,
		RICON_OpenRadioPairEndpoint recipientRx, RICON_StaticLosProfile profile, RICON_RadioQueryContext context,
		string timebaseId, int firstRxSampleIndex, int firstRelaySampleIndex)
	{
		if (!IsAuthority() || !m_RelayStationAuthority || m_bRelayConfiguring || m_bRelayOutputInstalled
			|| !m_GameControllerLifecycle || !path || !path.IsValid() || !profile || !context
			|| !stationTx || !recipientRx || !GetGame()) return false;
		int nowMs = m_GameControllerLifecycle.ReadWireAuthorityTime(m_RelayLifecycle);
		if (nowMs < 0 || nowMs >= profile.m_iValidUntilMs
			|| profile.m_iValidUntilMs - nowMs < RICON_MediaPcmProfile.FRAME_DURATION_MS) return false;
		RICON_RelayDestination destination = m_RelayStationAuthority.ReadDestinationAddress(path, nowMs);
		if (!RelayPlayersCurrent(path, destination)) return false;
		RICON_PairEndpoint tx = ObserveRelayEndpoint(stationTx);
		RICON_PairEndpoint rx = ObserveRelayEndpoint(recipientRx);
		RICON_WirelessLinkEvaluator evaluator = GetBandEvaluator(context.m_sBandId);
		if (!tx || !rx || !evaluator || !evaluator.UsesFixedRfModels()) return false;
		RICON_StaticLosGeometryAuthority geometry = new RICON_StaticLosGeometryAuthority();
		array<ref RICON_OpenRadioPairEndpoint> endpoints = {stationTx, recipientRx};
		if (!geometry.Seal(GetGame().GetWorld(), m_RelayLifecycle, m_MediaReceiverAuthority, endpoints, profile, timebaseId, true)) return false;

		RICON_RelayPcmPath boundPath = path.ClonePath();
		RICON_RelayWireGrant grant = RICON_RelayWireGrant.FromPath(boundPath, destination, 0, firstRelaySampleIndex, timebaseId);
		if (!grant) return false;
		RICON_RelayPcmHandoff input = new RICON_RelayPcmHandoff();
		RICON_RelayPcmOutput pcm = new RICON_RelayPcmOutput();
		RICON_RelayRfOutput output = new RICON_RelayRfOutput();
		if (!input.Configure(boundPath, m_RelayStationAuthority)
			|| !pcm.Configure(input, boundPath, firstRxSampleIndex, firstRelaySampleIndex)) return false;
		m_bRelayConfiguring = true;
		m_RelayGeometry = geometry;
		bool configured = output.Configure(m_RelayStationAuthority, pcm, boundPath, tx, rx, context, evaluator,
			nowMs, profile.m_iValidUntilMs, geometry, timebaseId);
		m_bRelayConfiguring = false;
		if (!configured || m_RelayGeometry != geometry || !RelayPlayersCurrent(boundPath, destination))
		{
			output.Close();
			InvalidateRelayOutput();
			return false;
		}
		m_RelayInput = input;
		m_RelayOutput = output;
		m_RelayPath = boundPath;
		m_RelayDestination = destination;
		m_RelayWireGrant = grant;
		m_iRelayFirstRxSampleIndex = firstRxSampleIndex;
		m_bRelayOutputInstalled = true;
		return true;
	}

	//! A nonnegative initial origin sequence opts in to RelayWire V2 before the remote handshake.
	bool ConfigureRelayInputRF(RICON_OpenRadioPairEndpoint sourceTx, RICON_OpenRadioPairEndpoint stationRx,
		RICON_StaticLosProfile profile, RICON_RadioQueryContext context, string timebaseId, float referenceDbm,
		int firstOriginSequence = -1, int firstOriginSampleIndex = 0, bool mappedCapture = false, int seriesCapacity = 2)
	{
		if (!IsAuthority() || !m_RelayPath || !m_RelayWireGrant || m_RelayInputFrame || m_RelayWireReady || m_RelayWirePublication
			|| m_bRelayConfiguring || !RICON_StaticLosChannel.ValidProfile(profile) || !context || timebaseId != m_RelayWireGrant.m_sTimebaseId
			|| !RICON_Num.IsFinite(referenceDbm) || !sourceTx || !stationRx || !sourceTx.m_Profile) return false;
		int nowMs = ReadRelayTime();
		if (nowMs < 0 || profile.m_iValidUntilMs - Math.Max(nowMs, profile.m_iValidFromMs) < RICON_MediaPcmProfile.FRAME_DURATION_MS)
			return false;
		RICON_MediaStreamBinding source = m_RelayPath.m_UpstreamRoute.m_Source;
		if (source.m_sProfileId != RICON_MediaPcmProfile.PROFILE_ID || !m_RelayStationAuthority.CanUseInput(m_RelayPath, nowMs)) return false;
		if (firstOriginSequence < -1 || (firstOriginSequence == -1 && (firstOriginSampleIndex != 0 || mappedCapture))
			|| (seriesCapacity != 2 && !mappedCapture)) return false;

		RICON_OpenRadioPairScope scope = new RICON_OpenRadioPairScope();
		scope.m_iNetworkId = source.m_iNetworkTypeId;
		scope.m_iChannelId = source.m_iChannelId;
		scope.m_iRawTune = sourceTx.m_Profile.m_iExpectedTuningValue;
		scope.m_fCarrierMHz = profile.m_fFrequencyMHz;
		scope.m_Context = context;
		scope.m_Limits = new RICON_PairServiceLimits();
		scope.m_Limits.m_iMaxEndpoints = 2;
		scope.m_Limits.m_iMaxDevices = 2;
		scope.m_Limits.m_iMaxBindings = 1;
		scope.m_Limits.m_iMaxListeners = 1;
		scope.m_Limits.m_iMaxPlayers = 1;
		scope.m_Limits.m_iMaxPairs = 1;
		array<ref RICON_OpenRadioPairEndpoint> endpoints = {sourceTx, stationRx};
		array<ref RICON_OpenRadioPairListener> listeners = {};
		array<ref RICON_OpenRadioPairEndpoint> copies;
		array<ref RICON_OpenRadioPairListener> listenerCopies;
		RICON_OpenRadioPairScope frozen = RICON_OpenRadioPairProjector.CopyControlledInputs(scope, endpoints, listeners, copies, listenerCopies);
		if (!frozen) return false;
		RICON_WirelessLinkEvaluator evaluator = GetBandEvaluator(frozen.m_Context.m_sBandId);
		if (!evaluator || !evaluator.UsesFixedRfModels()) return false;
		RICON_PairEndpoint tx = ObserveRelayEndpoint(copies[0]);
		RICON_PairEndpoint rx = ObserveRelayEndpoint(copies[1]);
		if (!tx || !rx || !m_RelayStationAuthority.MatchesInputEndpoints(m_RelayPath,
			tx.m_Observation, copies[0].m_iGeneration, rx.m_Observation, copies[1].m_iGeneration)) return false;
		RICON_StaticLosGeometryAuthority geometry = new RICON_StaticLosGeometryAuthority();
		if (!geometry.Seal(GetGame().GetWorld(), m_RelayLifecycle, m_MediaReceiverAuthority, copies, profile, timebaseId, true)) return false;
		if (!geometry.QualifiesFixedPair(tx, copies[0].m_iGeneration, rx, copies[1].m_iGeneration, timebaseId,
			Math.Max(nowMs, profile.m_iValidFromMs))) return false;
		string mappedTimebaseId;
		if (mappedCapture) mappedTimebaseId = timebaseId;
		if (firstOriginSequence >= 0 && !ConfigureRelayInputSeries(firstOriginSequence, firstOriginSampleIndex, mappedTimebaseId, seriesCapacity)) return false;
		m_RelayInputGeometry = geometry;
		m_RelayInputEndpoints = copies;
		m_RelayInputContext = frozen.m_Context;
		m_RelayInputFrame = new RICON_RelayInputFrame();
		m_sRelayMappedTimebaseId = mappedTimebaseId;
		m_iRelayInputSealedAtMs = nowMs;
		m_fRelayInputReferenceDbm = referenceDbm;
		m_bRelayInputTaken = false;
		return true;
	}

	protected bool ConfigureRelayInputSeries(int firstOriginSequence, int firstOriginSampleIndex, string mappedTimebaseId, int capacity)
	{
		int ratio = RICON_PcmResampler8To48.OUTPUT_RATE_HZ / RICON_PcmResampler8To48.INPUT_RATE_HZ;
		if (m_iRelayFirstRxSampleIndex % ratio != 0 || m_iRelayFirstRxSampleIndex / ratio != firstOriginSampleIndex) return false;
		RICON_RelayWireGrant grant = m_RelayWireGrant.CloneGrant();
		grant.m_iWireVersion = 2;
		grant.m_iOriginFirstSequence = firstOriginSequence;
		grant.m_iOriginFirstSampleIndex = firstOriginSampleIndex;
		RICON_RelayInputSeries series = new RICON_RelayInputSeries();
		if (!grant.IsValid() || !series.Configure(m_RelayPath.m_UpstreamRoute, firstOriginSampleIndex, mappedTimebaseId)
			|| !m_RelayInput || !m_RelayInput.EnableSeriesQueue(capacity)) return false;
		m_RelayWireGrant = grant;
		RICON_RelayFrameReservation reservation = new RICON_RelayFrameReservation();
		if (!reservation.Open(capacity, firstOriginSequence)) return false;
		m_RelayInputSeries = series;
		m_RelayReservation = reservation;
		m_iRelayNextOriginSequence = firstOriginSequence;
		return true;
	}

	//! Подключить серверного владельца прямого потока до согласования выхода станции.
	bool BindRelayInputFanout(RICON_MediaFanout fanout)
	{
		if (!IsAuthority() || !m_RelayInputFrame || !fanout) return false;
		int nowMs = ReadRelayTime();
		return nowMs >= 0 && BindRelayInputFanoutAt(fanout, nowMs);
	}

	protected bool BindRelayInputFanoutAt(RICON_MediaFanout fanout, int nowMs)
	{
		if (!IsAuthority() || !m_RelayInputFrame || !fanout || m_RelayInputFanout || m_bRelayInputTaken
			|| m_bRelayConfiguring || m_RelayWireReady || m_RelayWirePublication) return false;
		if (!m_sRelayMappedTimebaseId.IsEmpty())
		{
			if (!m_MediaWireTransport || !m_MediaWireTransport.IsCurrent() || m_MediaWireTransport.GetWireVersion() != 3
				|| m_MediaWireTransport.GetRfTimebaseId() != m_sRelayMappedTimebaseId) return false;
		}
		else if (m_MediaWireTransport && m_MediaWireTransport.GetWireVersion() != 1) return false;
		if (!RelayPlayersCurrent(m_RelayPath, m_RelayDestination) || !m_RelayInputGeometry.IsCurrent()
			|| !m_RelayStationAuthority.CanUseSharedInputFanout(m_RelayPath, fanout, nowMs)) return false;
		RICON_CommsRelayFanoutSink sink = new RICON_CommsRelayFanoutSink(this);
		if (!fanout.BindRelaySink(sink, m_RelayPath.m_UpstreamRoute.m_Source, nowMs, m_sRelayMappedTimebaseId)) return false;
		m_RelayInputFanout = fanout;
		m_RelayFanoutSink = sink;
		return true;
	}

	//! Synchronous call from the registered owner. Use the fanout tick so direct dispatch keeps the same clock.
	bool StageSharedRelayInput(RICON_CommsRelayFanoutSink sink, RICON_MediaFanout fanout, RICON_QueuedMediaBlock block,
		int deadlineMs, int nowMs)
	{
		if (!IsAuthority() || !m_RelayInputFrame || m_bRelayConfiguring
			|| !sink || sink != m_RelayFanoutSink || !fanout || fanout != m_RelayInputFanout
			|| !fanout.OwnsRelayOffer(sink, block, deadlineMs, nowMs)) return false;
		if (!m_RelayInputGeometry.IsCurrent() || !RelayPlayersCurrent(m_RelayPath, m_RelayDestination)
			|| !m_RelayStationAuthority.CanUseSharedInputFanout(m_RelayPath, fanout, nowMs) || RelayWireGate(nowMs) != "OK") return false;
		if (!m_sRelayMappedTimebaseId.IsEmpty() && !RelayMappedUpstreamCurrent())
		{
			InvalidateRelayOutput();
			return false;
		}
		if (m_RelayReservation)
		{
			string admit = m_RelayReservation.Admit(block, deadlineMs, m_bRelayInputTaken);
			RICON_OnRelayReservationDecision(admit, block.m_iSequence, block.m_iFirstSampleIndex, nowMs);
			if (admit == "HOLD") return true;
			if (admit != "STORE")
			{
				InvalidateRelayOutput();
				return false;
			}
		}
		else if (m_bRelayInputTaken)
		{
			if (m_RelayInputSeries && block.m_iSequence > m_iRelayNextOriginSequence) InvalidateRelayOutput();
			return false;
		}
		if (!StoreRelayInput(block, deadlineMs, nowMs))
		{
			if (m_RelayInputSeries) InvalidateRelayOutput();
			return false;
		}
		m_bRelayInputTaken = true;
		// Fanout has already detached each direct queue copy and no longer uses this block.
		m_RelaySharedInput = block;
		return true;
	}

	//! Metadata observation only; overrides must not mutate the active fanout or relay state.
	protected void RICON_OnRelayReservationDecision(string decision, int sequence, int firstSample, int nowMs) {}

	//! The returned block is for RX staging; the authoritative copy stays here.
	RICON_QueuedMediaBlock TakeRelayInputBlock()
	{
		if (!IsAuthority() || !m_RelayInputFrame || m_bRelayConfiguring) return null;
		int nowMs = ReadRelayTime();
		if (nowMs < 0 || RelayWireGate(nowMs) != "OK") return null;
		if (RetireExpiredRelayInput(nowMs)) return null;
		if (m_RelayFanoutSink)
		{
			ref RICON_QueuedMediaBlock shared = m_RelaySharedInput;
			m_RelaySharedInput = null;
			if (!shared) return null;
			if (nowMs >= m_iRelayInputDeadlineMs || !m_RelayStationAuthority.CanUseSharedInputFanout(m_RelayPath, m_RelayInputFanout, nowMs))
			{
				if (m_RelayInputSeries) InvalidateRelayOutput();
				return null;
			}
			return shared;
		}
		if (m_bRelayInputTaken || !m_sRelayMappedTimebaseId.IsEmpty()) return null;
		RICON_QueuedMediaBlock block = m_RelayStationAuthority.TakeInputBlock(m_RelayPath, nowMs);
		if (!block) return null;
		m_bRelayInputTaken = true;
		if (!StoreRelayInput(block, int.MAX, nowMs))
		{
			if (m_RelayInputSeries) InvalidateRelayOutput();
			return null;
		}
		return block;
	}

	protected bool StoreRelayInput(RICON_QueuedMediaBlock block, int deadlineMs, int nowMs)
	{
		if (!block || deadlineMs <= nowMs || (m_iRelayGridDeadlineMs > 0 && nowMs >= m_iRelayGridDeadlineMs)
			|| nowMs > int.MAX - RELAY_INPUT_MAX_AGE_MS
			|| (m_RelayInputSeries && block.m_iSequence != m_iRelayNextOriginSequence)
			|| block.m_iCaptureLowerMs < m_iRelayInputSealedAtMs || block.m_iCaptureUpperMs > nowMs
			|| nowMs - block.m_iCaptureLowerMs >= RELAY_INPUT_MAX_AGE_MS
			|| (!m_sRelayMappedTimebaseId.IsEmpty() && !m_RelayInputGeometry.CoversFixedCapture(block.m_iCaptureLowerMs, block.m_iCaptureUpperMs))
			|| !m_RelayInputFrame.Store(m_RelayPath.m_UpstreamRoute, block, m_sRelayMappedTimebaseId)) return false;
		m_iRelayInputDeadlineMs = Math.Min(deadlineMs, block.m_iCaptureLowerMs + RELAY_INPUT_MAX_AGE_MS);
		if (m_iRelayGridDeadlineMs > 0) m_iRelayInputDeadlineMs = Math.Min(m_iRelayInputDeadlineMs, m_iRelayGridDeadlineMs);
		return true;
	}

	protected void PromoteRelayReservation(int nowMs)
	{
		if (!m_RelayReservation || m_bRelayInputTaken || m_RelaySharedInput) return;
		int deadlineMs;
		RICON_QueuedMediaBlock block = m_RelayReservation.TakeHeld(deadlineMs);
		if (!block) return;
		if (!StoreRelayInput(block, deadlineMs, nowMs))
		{
			InvalidateRelayOutput();
			return;
		}
		m_bRelayInputTaken = true;
		m_RelaySharedInput = block;
	}

	//! Вызывается перед извлечением локального решения, чтобы не сдвигать курсор при заполненной очереди.
	bool CanOfferRelaySeriesPCM()
	{
		return IsAuthority() && m_RelayInputSeries && m_RelayInput && m_RelayInput.HasSeriesRoom()
			&& !m_bRelayConfiguring && m_bRelayInputTaken && !m_RelaySharedInput;
	}

	//! Accept only the exact receiver decision for the consumed server block. Never accept caller-supplied PCM or budget.
	string OfferRelayInputPCM(RICON_MediaPlayoutDecision decision)
	{
		ref RICON_RelayInputSeries series = m_RelayInputSeries;
		if (series && !CanOfferRelaySeriesPCM()) return "NOT_READY";
		string status = OfferRelayInputPCMOnce(decision);
		if (series && m_RelayInputSeries == series)
		{
			if (status == "OK")
			{
				// После передачи PCM выходные стадии владеют кадром; голова освобождается, хвост резервации занимает её.
				m_iRelayNextOriginSequence++;
				m_bRelayInputTaken = false;
				if (m_RelayReservation) m_RelayReservation.CompleteHead();
				int promoteMs = ReadRelayTime();
				if (promoteMs >= 0) PromoteRelayReservation(promoteMs);
				else if (m_RelayReservation && m_RelayReservation.Count() > 0) InvalidateRelayOutput();
			}
			else InvalidateRelayOutput();
		}
		return status;
	}

	protected string OfferRelayInputPCMOnce(RICON_MediaPlayoutDecision decision)
	{
		if (!IsAuthority() || !m_RelayInputFrame || !m_RelayInput || !m_bRelayInputTaken
			|| m_RelaySharedInput || m_bRelayConfiguring) return "CLOSED";
		int nowMs = ReadRelayTime();
		if (nowMs < 0) return "REVOKED";
		string gate = RelayWireGate(nowMs);
		if (gate != "OK") return gate;
		if (!m_RelayStationAuthority.CanObserveTime(nowMs) || !m_RelayStationAuthority.CanUseInput(m_RelayPath, nowMs)) return "REVOKED";
		ref RICON_RelayPcmPath path = m_RelayPath;
		RICON_MediaEgressRoute route = path.m_UpstreamRoute;
		RICON_QueuedMediaBlock block = m_RelayInputFrame.Read(route, decision, nowMs);
		if (!block) return "INVALID_INPUT_FRAME";
		int deadlineMs = Math.Min(m_iRelayInputDeadlineMs, decision.m_iWireExpiryMs);
		if (deadlineMs <= nowMs) return "EXPIRED";
		int rxReadyMs = decision.m_iDecidedAtMs;
		ref RICON_MediaOutputPermit permit = decision.m_OutputPermit;

		RICON_PairEndpoint tx = ObserveRelayEndpoint(m_RelayInputEndpoints[0]);
		RICON_PairEndpoint rx = ObserveRelayEndpoint(m_RelayInputEndpoints[1]);
		int txGeneration = m_RelayInputEndpoints[0].m_iGeneration;
		int rxGeneration = m_RelayInputEndpoints[1].m_iGeneration;
		if (!tx || !rx || !m_RelayStationAuthority.MatchesInputEndpoints(path, tx.m_Observation, txGeneration, rx.m_Observation, rxGeneration))
			return "WRONG_INPUT_ENDPOINT";
		RICON_WirelessLinkEvaluator evaluator = GetBandEvaluator(m_RelayInputContext.m_sBandId);
		ref RICON_StaticLosGeometryAuthority geometry = m_RelayInputGeometry;
		ref RICON_RadioQueryContext context = m_RelayInputContext;
		m_bRelayConfiguring = true;
		RICON_LinkBudget budget = RICON_RelayRfInput.Evaluate(tx, txGeneration, rx, rxGeneration, context, evaluator,
			geometry, m_RelayWireGrant.m_sTimebaseId, block.m_iCaptureLowerMs, block.m_iCaptureUpperMs);
		m_bRelayConfiguring = false;
		if (m_RelayPath != path || m_RelayInputGeometry != geometry) return "REVOKED";
		if (!budget) return "INPUT_RF_UNCOVERED";
		int first;
		array<int> pcm;
		RICON_RelayPcmOrigin origin;
		if (m_RelayInputSeries)
		{
			RICON_RelayInputSeriesWindow window;
			string seriesStatus = m_RelayInputSeries.Process(route, block, nowMs, deadlineMs, window);
			if (seriesStatus != "OK") return seriesStatus;
			if (block.m_CaptureEnvelope)
			{
				if (!geometry.CoversFixedCapture(window.m_iOldestCaptureMs, block.m_iCaptureUpperMs)) return "INPUT_RF_UNCOVERED";
			}
			else if (!geometry.CoversFixedInterval(window.m_iOldestCaptureMs, block.m_iCaptureUpperMs)) return "INPUT_RF_UNCOVERED";
			origin = window.m_Origin;
			pcm = window.m_aSamples;
			first = window.m_iFirstSampleIndex;
		}
		else
		{
			RICON_PcmResampler8To48 up = new RICON_PcmResampler8To48();
			if (!up.Reset(block.m_iFirstSampleIndex)) return "INVALID_INPUT_FRAME";
			pcm = up.Process(block.m_iFirstSampleIndex, RICON_MediaPcmProfile.DecodeBytes(block.m_Bytes), first);
			origin = new RICON_RelayPcmOrigin();
			origin.m_iSequence = block.m_iSequence;
			origin.m_iFirstSampleIndex = block.m_iFirstSampleIndex;
			origin.m_iSampleCount = block.m_iSampleCount;
			origin.m_iCaptureFromMs = block.m_iCaptureLowerMs;
			origin.m_iCaptureUntilMs = block.m_iCaptureUpperMs;
			origin.m_iRxReadyMs = rxReadyMs;
			origin.m_iDeadlineMs = deadlineMs;
		}
		RICON_SharedRxMixResult mix = RICON_RelayRfInput.Mix(route, permit, first, pcm, budget,
			m_fRelayInputReferenceDbm, null);
		if (!mix) return "INVALID_INPUT_PCM";

		int commitMs = ReadRelayTime();
		if (commitMs < 0 || m_RelayPath != path || !m_RelayInput) return "REVOKED";
		if (RelayWireGate(commitMs) != "OK" || !m_RelayStationAuthority.CanObserveTime(commitMs)
			|| !m_RelayStationAuthority.CanUseInput(path, commitMs)) return "REVOKED";
		return CommitRelayRx(mix, origin, commitMs);
	}

	//! Called by the trusted world owner before modifying a sealed pose or RF environment.
	void InvalidateRelayOutput()
	{
		if (m_RelayWirePublication && GetGame() && GetGame().GetCallqueue())
			GetGame().GetCallqueue().Remove(CheckRelayInstallTimeout);
		if (m_RelayWireReady && GetGame() && GetGame().GetCallqueue())
			GetGame().GetCallqueue().Remove(CheckRelayReadyTimeout);
		m_RelayWireReady = null;
		SCR_PlayerController recipient;
		RICON_RelayWireGrant revoked;
		if (m_bRelayInstallSubmitted && m_RelayWirePublication)
		{
			recipient = ResolveRelayRecipient();
			revoked = m_RelayWirePublication.Revoke();
		}
		m_bRelayInstallSubmitted = false;
		if (m_RelayGeometry) m_RelayGeometry.Invalidate();
		if (m_RelayInputGeometry) m_RelayInputGeometry.Invalidate();
		if (m_RelayOutput) m_RelayOutput.Close();
		m_RelayGeometry = null;
		m_RelayInputGeometry = null;
		m_RelayInputEndpoints = null;
		m_RelayInputContext = null;
		m_RelayInputFrame = null;
		if (m_RelayInputSeries) m_RelayInputSeries.Close();
		m_RelayInputSeries = null;
		m_sRelayMappedTimebaseId = "";
		m_iRelayGridDeadlineMs = 0;
		if (m_RelayInputFanout) m_RelayInputFanout.UnbindRelaySink(m_RelayFanoutSink);
		m_RelayInputFanout = null;
		m_RelayFanoutSink = null;
		m_RelaySharedInput = null;
		m_bRelayInputTaken = false;
		if (m_RelayReservation) m_RelayReservation.Close();
		m_RelayReservation = null;
		m_RelayOutput = null;
		m_RelayInput = null;
		m_RelayPath = null;
		m_RelayDestination = null;
		m_RelayWireGrant = null;
		if (m_RelayWirePublication) m_RelayWirePublication.Close();
		m_RelayWirePublication = null;
		m_RelayMediaInFlight = null;
		m_RelayMediaRecipient = null;
		m_RelayMediaBody = null;
		if (recipient && revoked)
		{
			m_RelayRevokeInFlight = revoked;
			m_RelayRevokeRecipient = recipient;
			recipient.RICON_SubmitRelayWireRevoke(this, revoked);
			m_RelayRevokeInFlight = null;
			m_RelayRevokeRecipient = null;
		}
	}

	bool AuthorizeRelayWireRevoke(SCR_PlayerController recipient, RICON_RelayWireGrant revoked)
	{
		if (!IsAuthority() || !m_GameControllerLifecycle || !revoked || m_RelayRevokeInFlight != revoked
			|| !recipient || recipient != m_RelayRevokeRecipient) return false;
		RICON_RelayDestination destination = revoked.m_Destination;
		return m_GameControllerLifecycle.ResolveRemoteRecipient(destination.m_iPlayerId, destination.m_iConnectionId,
			destination.m_iPlayerGeneration, destination.m_iEntityId) == recipient;
	}

	//! The world owner starts the handshake after configuring the station.
	bool StartRelayWireHandshake()
	{
		if (!IsAuthority() || !m_RelayOutput || !m_RelayWireGrant || m_RelayWireReady || m_RelayWirePublication || m_bRelayConfiguring) return false;
		if (!m_sRelayMappedTimebaseId.IsEmpty() && (!m_RelayFanoutSink || !RelayMappedUpstreamCurrent())) return false;
		int nowMs = ReadRelayTime();
		if (nowMs < 0 || !m_RelayOutput.CanPublishGrant(nowMs)) return false;
		SCR_PlayerController recipient = ResolveRelayRecipient();
		ScriptCallQueue queue = GetGame().GetCallqueue();
		RICON_RelayWireReady ready = new RICON_RelayWireReady();
		if (!recipient || !queue || !ready.Begin(m_iSessionEpoch, m_RelayDestination, nowMs, RELAY_READY_TIMEOUT_MS)) return false;
		m_RelayWireReady = ready;
		queue.CallLater(CheckRelayReadyTimeout, RELAY_READY_TIMEOUT_MS, false, m_iSessionEpoch);
		if (recipient.RICON_SubmitRelayReadyRequest(this, m_iSessionEpoch)) return true;
		InvalidateRelayOutput();
		return false;
	}

	bool AuthorizeRelayReadyRequest(SCR_PlayerController recipient, int epoch)
	{
		return IsAuthority() && m_RelayWireReady && !m_RelayWirePublication && epoch == m_iSessionEpoch
			&& recipient && ResolveRelayRecipient() == recipient;
	}

	bool ConfirmRelayWireReady(SCR_PlayerController sender, array<int> body)
	{
		if (!IsAuthority() || !m_RelayWireReady || m_RelayWirePublication || !sender || ResolveRelayRecipient() != sender
			|| !m_RelayWireReady.MatchesReceiver(body)) return false;
		int nowMs = ReadRelayTime();
		if (nowMs < 0 || !m_RelayWireReady.Accept(body, nowMs)) return false;
		GetGame().GetCallqueue().Remove(CheckRelayReadyTimeout);
		if (SubmitRelayWireInstall()) return true;
		InvalidateRelayOutput();
		return false;
	}

	protected void CheckRelayReadyTimeout(int epoch)
	{
		if (epoch != m_iSessionEpoch || !m_RelayWireReady || m_RelayWirePublication) return;
		int nowMs = ReadRelayTime();
		if (nowMs < 0) return;
		int remaining = m_RelayWireReady.Remaining(nowMs);
		if (remaining <= 0) InvalidateRelayOutput();
		else GetGame().GetCallqueue().CallLater(CheckRelayReadyTimeout, remaining, false, epoch);
	}

	protected bool SubmitRelayWireInstall()
	{
		if (!IsAuthority() || !m_RelayOutput || !m_RelayWireGrant || m_RelayWirePublication || m_bRelayConfiguring) return false;
		int nowMs = ReadRelayTime();
		if (nowMs < 0 || !m_RelayOutput.CanPublishGrant(nowMs)) return false;
		SCR_PlayerController recipient = ResolveRelayRecipient();
		if (!recipient) return false;
		RICON_RelayWirePublication publication = new RICON_RelayWirePublication();
		ScriptCallQueue queue = GetGame().GetCallqueue();
		if (!queue || !publication.Begin(m_RelayWireGrant, nowMs, RELAY_ACK_TIMEOUT_MS)) return false;
		m_RelayWirePublication = publication;
		m_bRelayInstallSubmitted = true;
		if (recipient.RICON_SubmitRelayWireInstall(this, m_RelayWireGrant))
		{
			queue.CallLater(CheckRelayInstallTimeout, RELAY_ACK_TIMEOUT_MS, false, m_iSessionEpoch);
			return true;
		}
		m_bRelayInstallSubmitted = false;
		InvalidateRelayOutput();
		return false;
	}

	protected void CheckRelayInstallTimeout(int epoch)
	{
		if (epoch != m_iSessionEpoch || !m_RelayWirePublication) return;
		int nowMs = ReadRelayTime();
		if (nowMs < 0) return;
		if (m_RelayWirePublication.Expired(nowMs)) InvalidateRelayOutput();
		else
		{
			int remaining = m_RelayWirePublication.AckWaitRemaining(nowMs);
			if (remaining > 0) GetGame().GetCallqueue().CallLater(CheckRelayInstallTimeout, remaining, false, epoch);
		}
	}

	bool AuthorizeRelayWireInstall(SCR_PlayerController recipient, RICON_RelayWireGrant grant)
	{
		if (!IsAuthority() || !m_bRelayInstallSubmitted || !m_RelayWirePublication || !recipient
			|| m_RelayWireGrant != grant || ResolveRelayRecipient() != recipient) return false;
		int nowMs = ReadRelayTime();
		return nowMs >= 0 && !m_RelayWirePublication.Expired(nowMs) && m_RelayOutput.CanPublishGrant(nowMs);
	}

	//! Server RPC adapter must pass its actual owner controller, never a controller selected from the echo.
	bool ConfirmRelayWireInstall(SCR_PlayerController sender, int controlSeq, array<int> fields, string timebaseId)
	{
		if (!IsAuthority() || !m_bRelayInstallSubmitted || !m_RelayWirePublication || controlSeq != RICON_RelayWireControl.INSTALL_SEQ
			|| !sender || ResolveRelayRecipient() != sender) return false;
		RICON_RelayWireGrant echo = RICON_RelayWireControl.Decode(fields, timebaseId);
		if (!echo || !m_RelayWireGrant.Same(echo)) return false;
		int nowMs = ReadRelayTime();
		if (nowMs < 0 || m_RelayWirePublication.Expired(nowMs) || !m_RelayOutput.CanPublishGrant(nowMs)
			|| !m_RelayWirePublication.Confirm(controlSeq, echo, m_RelayDestination, nowMs)) return false;
		GetGame().GetCallqueue().Remove(CheckRelayInstallTimeout);
		return true;
	}

	string OfferRelayRx(RICON_SharedRxMixResult mix, RICON_RelayPcmOrigin origin)
	{
		if (!IsAuthority() || !m_RelayInput || m_bRelayConfiguring) return "CLOSED";
		if (m_RelayInputFrame) return "INPUT_RF_REQUIRED";
		int nowMs = ReadRelayTime();
		if (nowMs < 0) return "REVOKED";
		string gate = RelayWireGate(nowMs);
		if (gate != "OK") return gate;
		return CommitRelayRx(mix, origin, nowMs);
	}

	protected string CommitRelayRx(RICON_SharedRxMixResult mix, RICON_RelayPcmOrigin origin, int nowMs)
	{
		string status = m_RelayInput.Offer(mix, origin, nowMs);
		if (status == "REVOKED" || status == "CLOSED") InvalidateRelayOutput();
		return status;
	}

	//! Проверяет согласование, не читая кадры у RF-выхода.
	string ReadRelayWireReadiness()
	{
		if (!IsAuthority() || !m_RelayOutput || m_bRelayConfiguring || m_bRelayMediaSubmitting) return "CLOSED";
		int nowMs = ReadRelayTime();
		if (nowMs < 0) return "REVOKED";
		return RelayWireGate(nowMs);
	}

	//! Сервер привязывает начало один раз до первого чтения, после согласования выхода.
	bool EnableMappedRelayGrid(int firstEmitMs, int deadlineMs)
	{
		if (!IsAuthority() || !m_RelayOutput || m_sRelayMappedTimebaseId.IsEmpty() || !m_RelayInputSeries
			|| m_bRelayInputTaken || m_iRelayGridDeadlineMs != 0 || m_bRelayConfiguring || m_bRelayMediaSubmitting) return false;
		int nowMs = ReadRelayTime();
		if (nowMs < 0 || firstEmitMs <= nowMs || deadlineMs <= firstEmitMs
			|| deadlineMs - firstEmitMs <= RICON_MediaPcmProfile.FRAME_DURATION_MS || RelayWireGate(nowMs) != "OK") return false;
		if (!m_RelayOutput.EnableFixedGrid(firstEmitMs)) return false;
		m_iRelayGridDeadlineMs = deadlineMs;
		return true;
	}

	//! Один синхронный вызов адресной отправки; SUBMITTED не означает доставку.
	string SubmitRelayWireMedia()
	{
		if (!IsAuthority() || !m_RelayOutput) return "CLOSED";
		if (m_bRelayMediaSubmitting) return "BUSY";
		m_bRelayMediaSubmitting = true;
		string status = SubmitRelayWireMediaOnce();
		m_RelayMediaInFlight = null;
		m_RelayMediaRecipient = null;
		m_RelayMediaBody = null;
		m_bRelayMediaSubmitting = false;
		return status;
	}

	protected string SubmitRelayWireMediaOnce()
	{
		int gateMs = ReadRelayTime();
		if (gateMs < 0) return "REVOKED";
		if (RetireExpiredRelayInput(gateMs)) return "EXPIRED";
		string gate = RelayWireGate(gateMs);
		if (gate != "OK") return gate;
		SCR_PlayerController recipient;
		RICON_RelayAddressedFrame addressed;
		string status = TakeRelayForRemote(recipient, addressed);
		if (status != "OK") return status;
		array<int> encoded = RICON_RelayWire.Encode(addressed, m_RelayWireGrant);
		if (!encoded)
		{
			InvalidateRelayOutput();
			return "INVALID_WIRE";
		}
		int nowMs = ReadRelayTime();
		if (nowMs < 0 || nowMs >= addressed.m_Emission.m_Frame.m_iDeadlineMs || ResolveRelayRecipient() != recipient)
		{
			InvalidateRelayOutput();
			return "REVOKED";
		}
		m_RelayMediaRecipient = recipient;
		m_RelayMediaInFlight = addressed;
		m_RelayMediaBody = encoded;
		if (recipient.RICON_SubmitRelayWireMedia(this, encoded, m_RelayWireGrant.m_sTimebaseId))
			return "SUBMITTED";
		InvalidateRelayOutput();
		return "REVOKED";
	}

	protected bool RetireExpiredRelayInput(int nowMs)
	{
		if (!m_RelayInputSeries || !m_bRelayInputTaken || nowMs < m_iRelayInputDeadlineMs) return false;
		InvalidateRelayOutput();
		return true;
	}

	//! Called only by the target controller immediately before Rpc.
	bool AuthorizeRelayWireMedia(SCR_PlayerController recipient, array<int> body, string timebaseId)
	{
		if (!IsAuthority() || !m_bRelayMediaSubmitting || !m_RelayMediaInFlight || !m_RelayOutput
			|| !recipient || recipient != m_RelayMediaRecipient || !body || body != m_RelayMediaBody
			|| !m_RelayWireGrant || timebaseId != m_RelayWireGrant.m_sTimebaseId || ResolveRelayRecipient() != recipient) return false;
		ref RICON_RelayAddressedFrame addressed = m_RelayMediaInFlight;
		m_RelayMediaInFlight = null;
		int nowMs = ReadRelayTime();
		if (nowMs < 0 || (m_iRelayGridDeadlineMs > 0 && nowMs >= m_iRelayGridDeadlineMs)
			|| !m_RelayOutput.AuthorizeAddressed(addressed, m_RelayDestination, nowMs)) return false;
		RICON_RelayPcmOutputFrame frame = addressed.m_Emission.m_Frame;
		return m_RelayWirePublication.CheckMedia(frame.m_iSequence, frame.m_iFirstSampleIndex, m_RelayDestination, nowMs)
			&& ResolveRelayRecipient() == recipient;
	}

	protected string RelayWireGate(int nowMs)
	{
		if (m_iRelayGridDeadlineMs > 0 && nowMs >= m_iRelayGridDeadlineMs)
		{
			InvalidateRelayOutput();
			return "EXPIRED";
		}
		if (!m_RelayWirePublication)
		{
			if (m_RelayWireReady) return "READY_PENDING";
			return "INSTALL_REQUIRED";
		}
		if (m_RelayWirePublication.Expired(nowMs))
		{
			InvalidateRelayOutput();
			return "ACK_TIMEOUT";
		}
		if (!m_RelayWirePublication.Ready(m_RelayDestination, nowMs)) return "ACK_PENDING";
		return "OK";
	}

	protected string TakeRelayForRemote(out SCR_PlayerController controller, out RICON_RelayAddressedFrame frame)
	{
		controller = null;
		frame = null;
		if (!IsAuthority() || !m_RelayOutput || m_bRelayConfiguring) return "CLOSED";
		int nowMs = ReadRelayTime();
		if (nowMs < 0) return "REVOKED";
		if (!ResolveRelayRecipient())
		{
			InvalidateRelayOutput();
			return "NO_REMOTE_RECIPIENT";
		}
		ref RICON_RelayRfOutput output = m_RelayOutput;
		RICON_RelayRfOutputFrame emission;
		string status = output.Take(nowMs, emission);
		if (status != "OK")
		{
			if (status != "BAD_TIME" && status != "EMPTY" && status != "WAIT" && status != "RF_PENDING") InvalidateRelayOutput();
			return status;
		}
		int releaseMs = ReadRelayTime();
		if (releaseMs < 0 || m_RelayOutput != output) return "REVOKED";
		SCR_PlayerController remote = ResolveRelayRecipient();
		if (!remote)
		{
			InvalidateRelayOutput();
			return "NO_REMOTE_RECIPIENT";
		}
		RICON_RelayAddressedFrame addressed = output.AddressCompleted(emission, releaseMs);
		if (!addressed || m_RelayOutput != output || ResolveRelayRecipient() != remote)
		{
			InvalidateRelayOutput();
			return "REVOKED";
		}
		controller = remote;
		frame = addressed;
		return "OK";
	}

	protected int ReadRelayTime()
	{
		if (m_GameControllerLifecycle && RelayPlayersCurrent(m_RelayPath, m_RelayDestination)
			&& (!m_RelayInputGeometry || m_RelayInputGeometry.IsCurrent())
			&& (!m_RelayFanoutSink || (m_RelayInputFanout && m_RelayInputFanout.HasRelaySink(m_RelayFanoutSink)
				&& (m_sRelayMappedTimebaseId.IsEmpty() || RelayMappedUpstreamCurrent()))))
		{
			int nowMs = m_GameControllerLifecycle.ReadWireAuthorityTime(m_RelayLifecycle);
			if (nowMs >= 0) return nowMs;
		}
		InvalidateRelayOutput();
		return -1;
	}

	protected bool RelayPlayersCurrent(RICON_RelayPcmPath path, RICON_RelayDestination destination)
	{
		if (!m_GameControllerLifecycle || !path || !destination) return false;
		RICON_MediaStreamBinding origin = path.m_UpstreamRoute.m_Source;
		return MatchesRelayPlayer(origin.m_iPlayerId, origin.m_iConnectionId, origin.m_iPlayerGeneration, origin.m_iEntityId)
			&& MatchesRelayPlayer(destination.m_iPlayerId, destination.m_iConnectionId,
				destination.m_iPlayerGeneration, destination.m_iEntityId);
	}

	protected SCR_PlayerController ResolveRelayRecipient()
	{
		RICON_RelayDestination destination = m_RelayDestination;
		if (!m_GameControllerLifecycle || !destination) return null;
		return m_GameControllerLifecycle.ResolveRemoteRecipient(destination.m_iPlayerId, destination.m_iConnectionId,
			destination.m_iPlayerGeneration, destination.m_iEntityId);
	}

	protected static RICON_PairEndpoint ObserveRelayEndpoint(RICON_OpenRadioPairEndpoint endpoint)
	{
		BaseRadioComponent radio = endpoint.m_Radio;
		RICON_RadioRfProfile profile = endpoint.m_Profile;
		if (!radio || !profile || !RICON_Num.IsFiniteVector(endpoint.m_vLocalAntennaOffset)) return null;
		IEntity owner = radio.GetOwner();
		if (!owner || owner.IsDeleted()) return null;
		RICON_RadioObservation observation;
		if (!RICON_BaseRadioObservationReader.TryRead(radio, profile.m_iDeviceId, profile.m_iTransceiverIndex,
			RICON_OpenRadioPairProjector.AntennaPosition(owner, endpoint.m_vLocalAntennaOffset), observation)) return null;
		RICON_PairEndpoint result = new RICON_PairEndpoint();
		result.m_Observation = observation;
		result.m_Profile = profile;
		return result;
	}

	protected bool MatchesRelayPlayer(int playerId, int connectionId, int generation, int entityId)
	{
		int currentConnection, currentGeneration, currentEntity;
		return m_GameControllerLifecycle.ReadBoundPlayer(playerId, currentConnection, currentGeneration, currentEntity)
			&& currentConnection == connectionId && currentGeneration == generation && currentEntity == entityId;
	}

	RICON_MediaFanout OpenMediaDirectRoutes(RICON_MediaStreamOwner sourceOwner, RICON_LifecycleStateMachine lifecycle,
		RICON_MediaStreamBinding source, int nowMs, int maxBranches, int maxQueuedPerBranch, int maxPayloadBytes)
	{
		if (!IsAuthority() || !sourceOwner || !m_MediaListenerAuthority || m_bTemporalProducerPublishing) return null;
		if (m_GameControllerLifecycle && (!m_GameControllerLifecycle.MatchesLifecycle(lifecycle)
			|| !m_GameControllerLifecycle.AllBindingsCurrent())) return null;
		RICON_TemporalPublicationOwner publication = RICON_TemporalPublicationOwner.Cast(m_TemporalAuthorityJournal);
		RICON_MediaFanout fanout = sourceOwner.OpenDirectRoutes(source, lifecycle, publication, nowMs, maxBranches, maxQueuedPerBranch,
			maxPayloadBytes, m_MediaReceiverAuthority, m_MediaListenerAuthority, m_iOpenRadioNetworkId, m_GameControllerLifecycle);
		if (fanout && m_RelayInputFrame && m_RelayPath && m_RelayPath.m_UpstreamRoute.m_Source.Same(source))
			BindRelayInputFanoutAt(fanout, nowMs);
		return fanout;
	}

	//! Explicit world bootstrap supplies the trusted clock, a complete producer projection and a synchronous transport.
	bool StartOpenRadioLoop(RICON_LifecycleStateMachine lifecycle, RICON_MediaStreamOwner sourceOwner,
		RICON_OpenRadioTickSource tickSource, RICON_MediaFanoutTransport transport, int intervalMs, int maxVisits, int maxCaptureAgeMs)
	{
		if (!IsAuthority() || m_bOpenRadioRunning || m_OpenRadioTickSource || !tickSource || !transport
			|| intervalMs <= 0 || maxVisits <= 0 || maxCaptureAgeMs <= 0) return false;
		if (RICON_MediaRpcTransport.Cast(transport) && transport != m_MediaWireTransport) return false;
		if (m_OpenRadioTimer && m_OpenRadioTimer.IsWaking()) return false;
		IEntity owner = GetOwner();
		if (!owner || !owner.GetWorld() || owner.GetWorld().IsEditMode() || !GetGame()
			|| owner.GetWorld() != GetGame().GetWorld() || !GetGame().GetCallqueue()) return false;
		if (!lifecycle || lifecycle.GetSessionEpoch() != m_iSessionEpoch || !m_MediaListenerAuthority
			|| !m_MediaListenerAuthority.IsScope(lifecycle, sourceOwner, m_MediaReceiverAuthority)
			|| !RICON_TemporalPublicationOwner.Cast(m_TemporalAuthorityJournal)) return false;
		if (m_GameControllerLifecycle && !m_GameControllerLifecycle.MatchesLifecycle(lifecycle)) return false;
		m_OpenRadioLifecycle = lifecycle;
		m_OpenRadioSourceOwner = sourceOwner;
		m_OpenRadioTickSource = tickSource;
		m_OpenRadioTransport = transport;
		m_iOpenRadioVisits = maxVisits;
		m_iOpenRadioMaxCaptureAgeMs = maxCaptureAgeMs;
		m_OpenRadioTimer = new RICON_OpenRadioTimer();
		m_OpenRadioTimer.m_OnTick.Insert(RunOpenRadioTick);
		if (!m_OpenRadioTimer.Start(GetGame().GetCallqueue(), tickSource, intervalMs))
		{
			StopOpenRadioLoop();
			return false;
		}
		return true;
	}

	void StopOpenRadioLoop()
	{
		// Keep the stopped timer alive until any in-flight subscriber has returned.
		if (m_OpenRadioTimer) m_OpenRadioTimer.Stop();
		if (m_OpenRadioSourceOwner) m_OpenRadioSourceOwner.CancelOpenRadioPump();
		m_OpenRadioTickSource = null;
		m_OpenRadioTransport = null;
		m_OpenRadioLifecycle = null;
		m_OpenRadioSourceOwner = null;
		m_bOpenRadioPcmUplink = false;
		if (m_RelayFanoutSink && !m_sRelayMappedTimebaseId.IsEmpty()) InvalidateRelayOutput();
	}

	protected bool RelayMappedUpstreamCurrent()
	{
		return m_OpenRadioLifecycle == m_RelayLifecycle && OpenRadioPcmTransportCurrent()
			&& m_MediaWireTransport.GetRfTimebaseId() == m_sRelayMappedTimebaseId;
	}

	//! Доверенный bootstrap включает вход только для запущенного прямого V3 с RF-историей.
	bool EnableOpenRadioPcmUplink()
	{
		if (m_bOpenRadioPcmUplink || !OpenRadioPcmTransportCurrent()) return false;
		m_bOpenRadioPcmUplink = true;
		return true;
	}

	protected bool OpenRadioPcmTransportCurrent()
	{
		return OpenRadioLoopCurrent() && m_GameControllerLifecycle && m_OpenRadioSourceOwner
			&& m_MediaWireTransport && m_OpenRadioTransport == m_MediaWireTransport
			&& m_MediaWireTransport.IsCurrent() && m_MediaWireTransport.GetWireVersion() == 3;
	}

	void ReceiveOpenRadioPcm(SCR_PlayerController sender, int epoch, int leaseId, int streamId,
		int sequence, int firstSampleIndex, array<int> payload)
	{
		if (!m_bOpenRadioPcmUplink || epoch != m_iSessionEpoch || !OpenRadioPcmTransportCurrent()) return;
		int connectionId, generation, entityId;
		if (!m_GameControllerLifecycle.ReadRemoteSender(sender, connectionId, generation, entityId)) return;
		int nowMs = m_GameControllerLifecycle.ReadWireAuthorityTime(m_OpenRadioLifecycle);
		// При отказе часов bridge может закрыть текущую сессию.
		if (nowMs < 0 || !m_bOpenRadioPcmUplink || !OpenRadioPcmTransportCurrent()) return;
		RICON_EMediaWindowDecision window;
		m_OpenRadioSourceOwner.AcceptRemotePcm(epoch, leaseId, streamId, sender.GetPlayerId(), connectionId, generation,
			entityId, nowMs, sequence, firstSampleIndex, payload, m_iOpenRadioMaxCaptureAgeMs, window, m_MediaWireTransport.GetRfTimebaseId());
	}

	protected void RunOpenRadioTick(int nowMs)
	{
		if (m_bOpenRadioRunning || !m_OpenRadioTickSource) return;
		if (!OpenRadioLoopCurrent()) { StopOpenRadioLoop(); return; }
		m_bOpenRadioRunning = true;
		RunOpenRadioTickCore(nowMs);
		m_bOpenRadioRunning = false;
	}

	protected bool OpenRadioLoopCurrent()
	{
		IEntity owner = GetOwner();
		return IsAuthority() && owner && GetGame() && owner.GetWorld() == GetGame().GetWorld()
			&& m_OpenRadioTickSource && m_OpenRadioTransport && m_OpenRadioLifecycle
			&& (!m_GameControllerLifecycle || m_GameControllerLifecycle.MatchesLifecycle(m_OpenRadioLifecycle))
			&& m_OpenRadioLifecycle.GetSessionEpoch() == m_iSessionEpoch && m_MediaListenerAuthority
			&& m_MediaListenerAuthority.IsScope(m_OpenRadioLifecycle, m_OpenRadioSourceOwner, m_MediaReceiverAuthority);
	}

	protected void RunOpenRadioTickCore(int nowMs)
	{
		ref RICON_OpenRadioTickSource tickSource = m_OpenRadioTickSource;
		if (!OpenRadioLoopCurrent() || nowMs <= m_iLastOpenRadioAttemptMs || !m_OpenRadioLifecycle.CanQueryAt(nowMs)
			|| nowMs < m_OpenRadioSourceOwner.LastObservedTimeMs()) return;
		m_iLastOpenRadioAttemptMs = nowMs;
		if (m_MediaWireTransport && m_OpenRadioTransport == m_MediaWireTransport) m_MediaWireTransport.SweepExpiredInstalls(nowMs);
		if (!OpenRadioLoopCurrent()) return;
		if (m_GameControllerLifecycle && !m_GameControllerLifecycle.ReconcileKnownAt(nowMs)) return;
		if (!OpenRadioLoopCurrent()) return;
		RICON_LifecycleResult lifecycleResult = m_OpenRadioLifecycle.Tick(nowMs);
		RICON_TemporalCaptureResult consumed = ConsumeTemporalLifecycleResult(lifecycleResult, m_iSessionEpoch);
		if (!lifecycleResult || lifecycleResult.m_eResultCode != RICON_ELifecycleResult.OK || !consumed || consumed.m_sCode != "OK") return;
		RICON_TemporalProducerTick tick = tickSource.BuildTick(nowMs);
		if (!OpenRadioLoopCurrent() || !tick) return;
		RICON_TemporalPublicationResult published = PublishTemporalPairs2(tick, m_OpenRadioLifecycle, nowMs);
		if (!published || published.m_sCode != "OK" || !OpenRadioLoopCurrent()) return;
		if (m_GameControllerLifecycle && !m_GameControllerLifecycle.AllBindingsCurrent()) return;
		ref RICON_MediaFanoutTransport transport = m_OpenRadioTransport;
		int frames = 1;
		if (m_RelayReservation) frames = DirectFramesForStationRoom(m_RelayReservation.Room(m_bRelayInputTaken));
		m_OpenRadioSourceOwner.PumpOpenRadio(RICON_TemporalPublicationOwner.Cast(m_TemporalAuthorityJournal), nowMs,
			m_iOpenRadioVisits, m_iOpenRadioMaxCaptureAgeMs, transport, frames);
	}

	//! Прямые копии получают и кадр, который станция отвергнет. Предел 16 совпадает с PumpOpenRadio.
	static int DirectFramesForStationRoom(int room)
	{
		if (room <= 0) return 1;
		int frames = room + 1;
		if (frames > 16) return 16;
		return frames;
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
		int nextEpoch = AllocateSessionEpoch();
		if (nextEpoch <= 0 || nextEpoch >= int.MAX || nextEpoch == m_iSessionEpoch)
			return false;
		CloseMediaReceiverAuthority();
		m_iSessionEpoch = nextEpoch;
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
        if (m_GameControllerLifecycle)
        {
            if (!tick.m_ListenerRoster || !tick.m_ListenerRoster.UsesGameAuthority(m_GameControllerLifecycle,
                m_MediaReceiverAuthority, m_iOpenRadioNetworkId)) return ProducerFailure("CONTROLLER_ROSTER_REQUIRED");
        }
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
            || !sourceLease.MatchesSource(snapshot.m_iTalkerPlayerId, snapshot.m_Binding.m_iTxGeneration,
                snapshot.m_Binding.m_iTxDeviceId, snapshot.m_Binding.m_iNetworkTypeId, snapshot.m_Binding.m_iChannelId))
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
        RICON_TemporalCaptureResult consumed = m_TemporalAuthorityJournal.ConsumeLifecycleResult(result, preEventEpoch);
        if (m_MediaListenerAuthority && consumed && consumed.m_sCode == "OK")
            m_MediaListenerAuthority.ConsumeLifecycle(result, preEventEpoch);
        if (m_RelayStationAuthority)
        {
            if (consumed && consumed.m_sCode == "OK")
            {
                if (m_RelayStationAuthority.ConsumeLifecycle(result, preEventEpoch)) InvalidateRelayOutput();
            }
            else
            {
                InvalidateRelayOutput();
                m_RelayStationAuthority.Close();
            }
        }
        return consumed;
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
