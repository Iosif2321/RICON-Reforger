class RICON_OpenRadioPairScope
{
	int m_iNetworkId, m_iChannelId, m_iRawTune, m_iMaxSpanMs;
	float m_fCarrierMHz;
	ref RICON_PairServiceLimits m_Limits;
	ref RICON_RadioQueryContext m_Context;
}

class RICON_OpenRadioPairEndpoint
{
	BaseRadioComponent m_Radio;
	int m_iGeneration;
	vector m_vLocalAntennaOffset;
	ref RICON_RadioRfProfile m_Profile;
}

class RICON_OpenRadioPairListener
{
	int m_iPlayerId, m_iPlayerGeneration, m_iConnectionId, m_iEntityId, m_iDeviceId, m_iIndex, m_iGeneration;
}

class RICON_OpenRadioPairSample
{
	BaseRadioComponent m_Radio;
	IEntity m_Owner;
	vector m_vLocalAntennaOffset;
	int m_iGeneration, m_iAdmission;
	ref RICON_PairEndpoint m_PairEndpoint;
}

class RICON_OpenRadioProjectionSource
{
	ref RICON_TransmissionBinding m_Binding;
	ref RICON_GameRadioPttContext m_GameContext;
	int m_iPhysicalGeneration;
}

//! Synchronous projection of the server scenario's complete listener scope for one open channel.
//! Publish only to a Temporal owner dedicated to this scope. Model time and descriptors are separate inputs.
class RICON_OpenRadioPairProjector : RICON_PairService
{
	//! The controlled scenario also owns the registration of these built-in models.
	static RICON_OpenRadioPairScope CopyControlledInputs(RICON_OpenRadioPairScope scope,
		array<ref RICON_OpenRadioPairEndpoint> endpoints, array<ref RICON_OpenRadioPairListener> listeners,
		out array<ref RICON_OpenRadioPairEndpoint> endpointCopies, out array<ref RICON_OpenRadioPairListener> listenerCopies)
	{
		endpointCopies = null;
		listenerCopies = null;
		if (!ValidScope(scope) || !endpoints || !listeners) return null;
		if (scope.m_Context.m_sModelId != RICON_FreeSpaceModel.MODEL_ID || scope.m_Context.m_sNoiseEnvironmentId != "none"
			|| scope.m_Context.m_bHasInterference) return null;
		if (endpoints.Count() > scope.m_Limits.m_iMaxEndpoints || listeners.Count() > scope.m_Limits.m_iMaxListeners) return null;

		array<ref RICON_OpenRadioPairEndpoint> stagedEndpoints = {};
		foreach (RICON_OpenRadioPairEndpoint endpoint : endpoints)
		{
			if (!ValidEndpoint(endpoint, scope)) return null;
			RICON_OpenRadioPairEndpoint endpointCopy = new RICON_OpenRadioPairEndpoint();
			endpointCopy.m_Radio = endpoint.m_Radio;
			endpointCopy.m_iGeneration = endpoint.m_iGeneration;
			endpointCopy.m_vLocalAntennaOffset = endpoint.m_vLocalAntennaOffset;
			endpointCopy.m_Profile = CopyProfile(endpoint.m_Profile);
			stagedEndpoints.Insert(endpointCopy);
		}

		array<ref RICON_OpenRadioPairListener> stagedListeners = {};
		foreach (RICON_OpenRadioPairListener listener : listeners)
		{
			if (!listener) return null;
			RICON_OpenRadioPairListener listenerCopy = new RICON_OpenRadioPairListener();
			listenerCopy.m_iPlayerId = listener.m_iPlayerId;
			listenerCopy.m_iPlayerGeneration = listener.m_iPlayerGeneration;
			listenerCopy.m_iConnectionId = listener.m_iConnectionId;
			listenerCopy.m_iEntityId = listener.m_iEntityId;
			listenerCopy.m_iDeviceId = listener.m_iDeviceId;
			listenerCopy.m_iIndex = listener.m_iIndex;
			listenerCopy.m_iGeneration = listener.m_iGeneration;
			stagedListeners.Insert(listenerCopy);
		}

		RICON_OpenRadioPairScope copy = new RICON_OpenRadioPairScope();
		copy.m_iNetworkId = scope.m_iNetworkId;
		copy.m_iChannelId = scope.m_iChannelId;
		copy.m_iRawTune = scope.m_iRawTune;
		copy.m_iMaxSpanMs = scope.m_iMaxSpanMs;
		copy.m_fCarrierMHz = scope.m_fCarrierMHz;
		copy.m_Limits = CopyLimits(scope.m_Limits);
		copy.m_Context = CopyContext(scope.m_Context);
		endpointCopies = stagedEndpoints;
		listenerCopies = stagedListeners;
		return copy;
	}

	static string Build(RICON_LifecycleStateMachine lifecycle, RICON_MediaStreamOwner streams,
		RICON_MediaReceiverAuthority receivers, BaseWorld world, RICON_OpenRadioPairScope scope,
		array<ref RICON_OpenRadioPairEndpoint> endpoints, array<ref RICON_OpenRadioPairListener> listeners,
		int nowMs, out RICON_PairServiceInput input)
	{
		return BuildProjection(lifecycle, streams, null, receivers, world, scope, endpoints, listeners, nowMs, input);
	}

	//! Server-only input. Emissions must come from the game PTT owner, never a client claim.
	static string BuildFromGameEmissions(RICON_LifecycleStateMachine lifecycle, array<ref RICON_GameRadioPttEmission> emissions,
		RICON_MediaReceiverAuthority receivers, BaseWorld world, RICON_OpenRadioPairScope scope,
		array<ref RICON_OpenRadioPairEndpoint> endpoints, array<ref RICON_OpenRadioPairListener> listeners,
		int nowMs, out RICON_PairServiceInput input)
	{
		return BuildProjection(lifecycle, null, emissions, receivers, world, scope, endpoints, listeners, nowMs, input);
	}

	//! Use the Core witness only while the game PTT owner has confirmed the same effective bound.
	static RICON_TransmissionBinding BindGameEmission(RICON_LifecycleStateMachine lifecycle, RICON_GameRadioPttEmission emission, int nowMs)
	{
		if (!emission || emission.m_iTxTransceiverIndex < 0 || emission.m_iTxGeneration <= 0
			|| emission.m_iServerNowMs < 0 || nowMs < emission.m_iServerNowMs || nowMs >= emission.m_iDeadlineMs
			|| !GameContextCurrent(lifecycle, emission.m_Context)) return null;
		RICON_GameRadioPttContext context = emission.m_Context;
		RICON_LifecycleLease lease = lifecycle.QueryActiveLease(emission.m_iLeaseId, nowMs);
		if (!lease || !lease.MatchesSource(context.m_iPlayerId, context.m_iGeneration, emission.m_iDeviceId,
			emission.m_iNetworkId, emission.m_iChannelId) || lease.m_iIssuedAtMs != emission.m_iIssuedAtMs
			|| lease.EffectiveDeadlineMs() != emission.m_iDeadlineMs) return null;
		return BindLease(lease, context.m_iEpoch, emission.m_iTxTransceiverIndex);
	}

	protected static bool GameContextCurrent(RICON_LifecycleStateMachine lifecycle, RICON_GameRadioPttContext context)
	{
		return lifecycle && context && context.IsValid() && lifecycle.GetSessionEpoch() == context.m_iEpoch
			&& lifecycle.GetPlayerState(context.m_iPlayerId) == RICON_EPlayerLifecycleState.BOUND_ACTIVE
			&& lifecycle.GetPlayerGeneration(context.m_iPlayerId) == context.m_iGeneration
			&& lifecycle.GetPlayerConnectionId(context.m_iPlayerId) == context.m_iConnectionId
			&& lifecycle.GetPlayerEntityId(context.m_iPlayerId) == context.m_iEntityId;
	}

	protected static RICON_TransmissionBinding BindLease(RICON_LifecycleLease lease, int epoch, int index)
	{
		RICON_TransmissionBinding binding = new RICON_TransmissionBinding();
		binding.m_iLeaseId = lease.m_iLeaseId;
		binding.m_iSessionEpoch = epoch;
		binding.m_iTxPlayerId = lease.m_iPlayerId;
		binding.m_iTxGeneration = lease.m_iGeneration;
		binding.m_iTxDeviceId = lease.m_iDeviceId;
		binding.m_iTxTransceiverIndex = index;
		binding.m_iNetworkTypeId = lease.m_iNetworkTypeId;
		binding.m_iChannelId = lease.m_iChannelId;
		binding.m_bActive = true;
		binding.m_iIssuedAtMs = lease.m_iIssuedAtMs;
		binding.m_iTtlMs = lease.m_iTtlMs;
		binding.m_iAuthorityUntilMs = lease.m_iRenewedUntilMs;
		return binding;
	}

	protected static string BuildProjection(RICON_LifecycleStateMachine lifecycle, RICON_MediaStreamOwner streams,
		array<ref RICON_GameRadioPttEmission> emissions, RICON_MediaReceiverAuthority receivers, BaseWorld world, RICON_OpenRadioPairScope scope,
		array<ref RICON_OpenRadioPairEndpoint> endpoints, array<ref RICON_OpenRadioPairListener> listeners,
		int nowMs, out RICON_PairServiceInput input)
	{
		input = null;
		if (!WorldCurrent(world) || !ValidScope(scope) || !endpoints || !listeners) return "INVALID_SCOPE";
		if (!lifecycle || (!streams && !emissions) || !receivers || !receivers.CanRead(lifecycle, nowMs)) return "STALE_LIFECYCLE";
		RICON_PairServiceLimits limits = scope.m_Limits;
		if (endpoints.Count() > limits.m_iMaxEndpoints || listeners.Count() > limits.m_iMaxListeners) return "CAPACITY";
		array<ref RICON_MediaStreamBinding> mediaSources = {};
		int sourceCount;
		if (streams)
		{
			if (!streams.ReadActiveRadioSources(lifecycle, scope.m_iNetworkId, scope.m_iChannelId, nowMs, mediaSources)) return "STALE_LIFECYCLE";
			sourceCount = mediaSources.Count();
		}
		else sourceCount = emissions.Count();
		if (sourceCount > limits.m_iMaxBindings || sourceCount * listeners.Count() > limits.m_iMaxPairs) return "CAPACITY";
		array<ref RICON_OpenRadioProjectionSource> sources = {};
		foreach (RICON_MediaStreamBinding mediaSource : mediaSources)
		{
			RICON_LifecycleLease lease = lifecycle.QueryActiveLease(mediaSource.m_iLeaseId, nowMs);
			if (!lease || !lease.MatchesSource(mediaSource.m_iPlayerId, mediaSource.m_iPlayerGeneration, mediaSource.m_iDeviceId,
				scope.m_iNetworkId, scope.m_iChannelId)) return "STALE_LIFECYCLE";
			RICON_OpenRadioProjectionSource source = new RICON_OpenRadioProjectionSource();
			source.m_Binding = BindLease(lease, lifecycle.GetSessionEpoch(), mediaSource.m_iTxTransceiverIndex);
			sources.Insert(source);
		}
		if (emissions)
		{
			set<int> leaseIds = new set<int>();
			foreach (RICON_GameRadioPttEmission emission : emissions)
			{
				ref RICON_TransmissionBinding binding = BindGameEmission(lifecycle, emission, nowMs);
				if (!binding || binding.m_iNetworkTypeId != scope.m_iNetworkId || binding.m_iChannelId != scope.m_iChannelId) return "STALE_LIFECYCLE";
				if (leaseIds.Contains(binding.m_iLeaseId)) return "INVALID_ROSTER";
				leaseIds.Insert(binding.m_iLeaseId);
				RICON_OpenRadioProjectionSource gameSource = new RICON_OpenRadioProjectionSource();
				gameSource.m_Binding = binding;
				gameSource.m_GameContext = emission.m_Context.CloneContext();
				gameSource.m_iPhysicalGeneration = emission.m_iTxGeneration;
				sources.Insert(gameSource);
			}
		}

		map<string, ref RICON_OpenRadioPairEndpoint> registered = new map<string, ref RICON_OpenRadioPairEndpoint>();
		foreach (RICON_OpenRadioPairEndpoint endpoint : endpoints)
		{
			if (!ValidEndpoint(endpoint, scope)) return "INVALID_ROSTER";
			string key = EndpointKey(endpoint.m_Profile.m_iDeviceId, endpoint.m_Profile.m_iTransceiverIndex);
			if (registered.Contains(key)) return "INVALID_ROSTER";
			registered.Set(key, endpoint);
		}

		WorldTimestamp start = world.GetTimestamp();
		RICON_PairServiceInput candidate = new RICON_PairServiceInput();
		candidate.m_iNowMs = nowMs;
		candidate.m_iSessionEpoch = lifecycle.GetSessionEpoch();
		candidate.m_Context = CopyContext(scope.m_Context);
		candidate.m_Limits = CopyLimits(limits);
		map<string, ref RICON_OpenRadioPairSample> samples = new map<string, ref RICON_OpenRadioPairSample>();
		map<int, ref RICON_LifecycleDevice> devices = new map<int, ref RICON_LifecycleDevice>();
		set<int> players = new set<int>();
		set<string> listenerKeys = new set<string>();

		foreach (RICON_OpenRadioProjectionSource projected : sources)
		{
			RICON_TransmissionBinding txBinding = projected.m_Binding;
			string txKey = EndpointKey(txBinding.m_iTxDeviceId, txBinding.m_iTxTransceiverIndex);
			RICON_OpenRadioPairEndpoint tx = registered.Get(txKey);
			if (!tx || (projected.m_iPhysicalGeneration > 0 && tx.m_iGeneration != projected.m_iPhysicalGeneration)
				|| !ProjectEndpoint(candidate, world, lifecycle, receivers, devices, samples, tx,
					scope, txBinding.m_iTxPlayerId, nowMs)) return "RADIO_UNQUALIFIED";
			if (!ProjectPlayer(candidate, players, lifecycle, txBinding.m_iTxPlayerId, txBinding.m_iTxGeneration)) return "CAPACITY";
			candidate.m_aBindings.Insert(txBinding);
		}

		foreach (RICON_OpenRadioPairListener listener : listeners)
		{
			if (!ListenerCurrent(lifecycle, listener)) return "STALE_LIFECYCLE";
			string rxKey = EndpointKey(listener.m_iDeviceId, listener.m_iIndex);
			string listenerKey = rxKey + ":" + listener.m_iPlayerId.ToString();
			if (listenerKeys.Contains(listenerKey)) return "INVALID_ROSTER";
			listenerKeys.Insert(listenerKey);
			RICON_OpenRadioPairEndpoint rx = registered.Get(rxKey);
			if (!rx || rx.m_iGeneration != listener.m_iGeneration || !ProjectEndpoint(candidate, world, lifecycle,
				receivers, devices, samples, rx, scope, listener.m_iPlayerId, nowMs)) return "RADIO_UNQUALIFIED";
			if (!ProjectPlayer(candidate, players, lifecycle, listener.m_iPlayerId, listener.m_iPlayerGeneration)) return "CAPACITY";
			RICON_PairListener row = new RICON_PairListener();
			row.m_iSessionEpoch = candidate.m_iSessionEpoch;
			row.m_iPlayerId = listener.m_iPlayerId;
			row.m_iGeneration = listener.m_iPlayerGeneration;
			row.m_iDeviceId = listener.m_iDeviceId;
			row.m_iTransceiverIndex = listener.m_iIndex;
			row.m_iRxGeneration = rx.m_iGeneration;
			candidate.m_aListeners.Insert(row);
		}

		foreach (RICON_OpenRadioPairSample sample : samples)
		{
			RICON_RadioObservation observation = sample.m_PairEndpoint.m_Observation;
			if (!sample.m_Owner || sample.m_Owner.GetWorld() != world || !sample.m_Radio || sample.m_Radio.GetOwner() != sample.m_Owner
				|| AntennaPosition(sample.m_Owner, sample.m_vLocalAntennaOffset) != observation.m_vAntennaPos) return "RADIO_UNQUALIFIED";
			if (!receivers.MatchesRadio(observation.m_iDeviceId, observation.m_iTransceiverIndex, sample.m_iGeneration, sample.m_Radio)
				|| receivers.ReadOpenEndpointAdmission(observation.m_iDeviceId, observation.m_iTransceiverIndex, sample.m_iGeneration,
					scope.m_iNetworkId, scope.m_iChannelId, scope.m_iRawTune, nowMs) != sample.m_iAdmission) return "RADIO_UNQUALIFIED";
		}
		foreach (RICON_MediaStreamBinding held : mediaSources)
		{
			if (!streams.IsSourceCurrent(held, lifecycle, nowMs)) return "STALE_LIFECYCLE";
		}
		foreach (RICON_OpenRadioProjectionSource current : sources)
		{
			if (!current.m_GameContext) continue;
			RICON_TransmissionBinding bound = current.m_Binding;
			RICON_LifecycleLease live = lifecycle.QueryActiveLease(bound.m_iLeaseId, nowMs);
			if (!GameContextCurrent(lifecycle, current.m_GameContext) || !bound.SameLeaseAuthority(live)
				|| !live.MatchesSource(bound.m_iTxPlayerId, bound.m_iTxGeneration, bound.m_iTxDeviceId,
					bound.m_iNetworkTypeId, bound.m_iChannelId)) return "STALE_LIFECYCLE";
		}
		foreach (RICON_OpenRadioPairListener heldListener : listeners)
		{
			if (!ListenerCurrent(lifecycle, heldListener)) return "STALE_LIFECYCLE";
		}
		foreach (RICON_OpenRadioPairSample captured : samples)
		{
			RICON_RadioObservation heldObservation = captured.m_PairEndpoint.m_Observation;
			if (receivers.ReadRecordedOpenEndpointAdmission(heldObservation.m_iDeviceId, heldObservation.m_iTransceiverIndex,
				captured.m_iGeneration, scope.m_iNetworkId, scope.m_iChannelId, scope.m_iRawTune, nowMs) != captured.m_iAdmission)
				return "RADIO_UNQUALIFIED";
		}
		if (!WorldCurrent(world) || !receivers.CanRead(lifecycle, nowMs)) return "STALE_LIFECYCLE";
		WorldTimestamp end = world.GetTimestamp();
		if (end.Compare(start) < 0) return "CLOCK_ROLLBACK";
		if (end.Compare(start.PlusMilliseconds(scope.m_iMaxSpanMs)) > 0) return "SPAN_EXCEEDED";
		input = candidate;
		return "OK";
	}

	protected static bool WorldCurrent(BaseWorld world)
	{
		return world && GetGame() && GetGame().GetWorld() == world && !world.IsEditMode() && RICON_CommsManagerComponent.IsAuthority();
	}

	protected static bool ValidScope(RICON_OpenRadioPairScope scope)
	{
		return scope && scope.m_iNetworkId > 0 && scope.m_iChannelId > 0 && scope.m_iRawTune > 0
			&& RICON_Num.IsFinite(scope.m_fCarrierMHz) && scope.m_fCarrierMHz > 0
			&& scope.m_iMaxSpanMs >= 0 && scope.m_iMaxSpanMs <= 60000 && scope.m_Limits && ValidLimits(scope.m_Limits)
			&& scope.m_Context && RICON_RadioQueryBuilder.IsValidContext(scope.m_Context);
	}

	protected static bool ValidEndpoint(RICON_OpenRadioPairEndpoint endpoint, RICON_OpenRadioPairScope scope)
	{
		return endpoint && endpoint.m_Radio && endpoint.m_iGeneration > 0 && RICON_Num.IsFiniteVector(endpoint.m_vLocalAntennaOffset)
			&& endpoint.m_Profile && RICON_RadioQueryBuilder.IsValidProfile(endpoint.m_Profile)
			&& endpoint.m_Profile.m_iExpectedTuningValue == scope.m_iRawTune && endpoint.m_Profile.m_fFrequencyMHz == scope.m_fCarrierMHz;
	}

	protected static bool ListenerCurrent(RICON_LifecycleStateMachine lifecycle, RICON_OpenRadioPairListener listener)
	{
		return listener && listener.m_iPlayerId > 0 && listener.m_iPlayerGeneration > 0 && listener.m_iConnectionId > 0
			&& listener.m_iEntityId > 0 && listener.m_iDeviceId > 0 && listener.m_iIndex >= 0 && listener.m_iGeneration > 0
			&& lifecycle.GetPlayerState(listener.m_iPlayerId) == RICON_EPlayerLifecycleState.BOUND_ACTIVE
			&& lifecycle.GetPlayerGeneration(listener.m_iPlayerId) == listener.m_iPlayerGeneration
			&& lifecycle.GetPlayerConnectionId(listener.m_iPlayerId) == listener.m_iConnectionId
			&& lifecycle.GetPlayerEntityId(listener.m_iPlayerId) == listener.m_iEntityId;
	}

	static vector AntennaPosition(IEntity owner, vector offset)
	{
		vector transform[4];
		owner.GetWorldTransform(transform);
		return transform[3] + transform[0] * offset[0] + transform[1] * offset[1] + transform[2] * offset[2];
	}

	protected static bool ProjectEndpoint(RICON_PairServiceInput candidate, BaseWorld world, RICON_LifecycleStateMachine lifecycle,
		RICON_MediaReceiverAuthority receivers, map<int, ref RICON_LifecycleDevice> devices,
		map<string, ref RICON_OpenRadioPairSample> samples, RICON_OpenRadioPairEndpoint endpoint,
		RICON_OpenRadioPairScope scope, int playerId, int nowMs)
	{
		RICON_RadioRfProfile profile = endpoint.m_Profile;
		int deviceId = profile.m_iDeviceId;
		int slotIndex = profile.m_iTransceiverIndex;
		RICON_LifecycleDevice device = devices.Get(deviceId);
		if (!device)
		{
			if (candidate.m_aDevices.Count() >= candidate.m_Limits.m_iMaxDevices) return false;
			device = lifecycle.QueryDevice(deviceId, nowMs);
			if (!device) return false;
			devices.Set(deviceId, device);
			candidate.m_aDevices.Insert(device);
		}
		if (!device.m_bPowered || !device.CanAccess(playerId)
			|| !device.Allows(scope.m_iNetworkId, scope.m_iChannelId)) return false;
		string key = EndpointKey(deviceId, slotIndex);
		if (samples.Contains(key)) return true;
		BaseRadioComponent radio = endpoint.m_Radio;
		if (!radio) return false;
		IEntity owner = radio.GetOwner();
		if (!owner || owner.GetWorld() != world || !receivers.MatchesRadio(deviceId, slotIndex, endpoint.m_iGeneration, radio)) return false;
		int admission = receivers.ReadOpenEndpointAdmission(deviceId, slotIndex, endpoint.m_iGeneration,
			scope.m_iNetworkId, scope.m_iChannelId, scope.m_iRawTune, nowMs);
		if (admission <= 0) return false;
		RICON_RadioObservation observation;
		if (!RICON_BaseRadioObservationReader.TryRead(radio, deviceId, slotIndex,
			AntennaPosition(owner, endpoint.m_vLocalAntennaOffset), observation)) return false;
		if (!observation.m_bPowered || observation.m_iTuningValue != profile.m_iExpectedTuningValue
			|| !observation.m_sNetworkKey.IsEmpty()) return false;
		RICON_PairEndpoint row = new RICON_PairEndpoint();
		row.m_Observation = observation;
		row.m_Profile = CopyProfile(profile);
		RICON_OpenRadioPairSample sample = new RICON_OpenRadioPairSample();
		sample.m_Radio = radio;
		sample.m_Owner = owner;
		sample.m_vLocalAntennaOffset = endpoint.m_vLocalAntennaOffset;
		sample.m_iGeneration = endpoint.m_iGeneration;
		sample.m_iAdmission = admission;
		sample.m_PairEndpoint = row;
		samples.Set(key, sample);
		candidate.m_aEndpoints.Insert(row);
		return true;
	}

	protected static bool ProjectPlayer(RICON_PairServiceInput candidate, set<int> players, RICON_LifecycleStateMachine lifecycle,
		int playerId, int generation)
	{
		if (players.Contains(playerId)) return true;
		if (candidate.m_aPlayers.Count() >= candidate.m_Limits.m_iMaxPlayers) return false;
		RICON_PairPlayer player = new RICON_PairPlayer();
		player.m_iPlayerId = playerId;
		player.m_iGeneration = generation;
		player.m_eState = lifecycle.GetPlayerState(playerId);
		candidate.m_aPlayers.Insert(player);
		players.Insert(playerId);
		return true;
	}

	protected static RICON_PairServiceLimits CopyLimits(RICON_PairServiceLimits source)
	{
		RICON_PairServiceLimits copy = new RICON_PairServiceLimits();
		copy.m_iMaxBindings = source.m_iMaxBindings;
		copy.m_iMaxListeners = source.m_iMaxListeners;
		copy.m_iMaxPlayers = source.m_iMaxPlayers;
		copy.m_iMaxDevices = source.m_iMaxDevices;
		copy.m_iMaxEndpoints = source.m_iMaxEndpoints;
		copy.m_iMaxPairs = source.m_iMaxPairs;
		return copy;
	}

	protected static RICON_RadioQueryContext CopyContext(RICON_RadioQueryContext source)
	{
		RICON_RadioQueryContext copy = new RICON_RadioQueryContext();
		copy.m_sBandId = source.m_sBandId;
		copy.m_sModelId = source.m_sModelId;
		copy.m_sNoiseEnvironmentId = source.m_sNoiseEnvironmentId;
		copy.m_sIntelligibilityModelId = source.m_sIntelligibilityModelId;
		copy.m_bHasInterference = source.m_bHasInterference;
		copy.m_fInterferenceDbm = source.m_fInterferenceDbm;
		return copy;
	}

	static RICON_RadioRfProfile CopyProfile(RICON_RadioRfProfile source)
	{
		RICON_RadioRfProfile copy = new RICON_RadioRfProfile();
		copy.m_sProfileId = source.m_sProfileId;
		copy.m_iDeviceId = source.m_iDeviceId;
		copy.m_iTransceiverIndex = source.m_iTransceiverIndex;
		copy.m_sProvenance = source.m_sProvenance;
		copy.m_iExpectedTuningValue = source.m_iExpectedTuningValue;
		copy.m_fFrequencyMHz = source.m_fFrequencyMHz;
		copy.m_fRxBandwidthHz = source.m_fRxBandwidthHz;
		copy.m_fTxPowerDbm = source.m_fTxPowerDbm;
		copy.m_fTxGainDbi = source.m_fTxGainDbi;
		copy.m_fRxGainDbi = source.m_fRxGainDbi;
		copy.m_fRxNoiseFigureDb = source.m_fRxNoiseFigureDb;
		copy.m_bHasRxSensitivity = source.m_bHasRxSensitivity;
		copy.m_fRxSensitivityDbm = source.m_fRxSensitivityDbm;
		copy.m_bHasRequiredSinr = source.m_bHasRequiredSinr;
		copy.m_fRequiredSinrDb = source.m_fRequiredSinrDb;
		return copy;
	}
}
