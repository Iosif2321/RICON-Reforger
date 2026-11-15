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
		input = null;
		if (!WorldCurrent(world) || !ValidScope(scope) || !endpoints || !listeners) return "INVALID_SCOPE";
		if (!lifecycle || !streams || !receivers || !receivers.CanRead(lifecycle, nowMs)) return "STALE_LIFECYCLE";
		RICON_PairServiceLimits limits = scope.m_Limits;
		if (endpoints.Count() > limits.m_iMaxEndpoints || listeners.Count() > limits.m_iMaxListeners) return "CAPACITY";
		array<ref RICON_MediaStreamBinding> sources = {};
		if (!streams.ReadActiveRadioSources(lifecycle, scope.m_iNetworkId, scope.m_iChannelId, nowMs, sources)) return "STALE_LIFECYCLE";
		if (sources.Count() > limits.m_iMaxBindings || sources.Count() * listeners.Count() > limits.m_iMaxPairs) return "CAPACITY";

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
		RICON_MediaEgressRoute admissionRoute = new RICON_MediaEgressRoute();
		admissionRoute.m_Source = new RICON_MediaStreamBinding();
		admissionRoute.m_Source.m_iSessionEpoch = candidate.m_iSessionEpoch;
		admissionRoute.m_Source.m_iNetworkTypeId = scope.m_iNetworkId;
		admissionRoute.m_Source.m_iChannelId = scope.m_iChannelId;
		map<string, ref RICON_OpenRadioPairSample> samples = new map<string, ref RICON_OpenRadioPairSample>();
		map<int, ref RICON_LifecycleDevice> devices = new map<int, ref RICON_LifecycleDevice>();
		set<int> players = new set<int>();
		set<string> listenerKeys = new set<string>();

		foreach (RICON_MediaStreamBinding source : sources)
		{
			RICON_LifecycleLease lease = lifecycle.QueryActiveLease(source.m_iLeaseId, nowMs);
			if (!lease || !lease.MatchesSource(source.m_iPlayerId, source.m_iPlayerGeneration, source.m_iDeviceId,
				scope.m_iNetworkId, scope.m_iChannelId)) return "STALE_LIFECYCLE";
			string txKey = EndpointKey(source.m_iDeviceId, source.m_iTxTransceiverIndex);
			RICON_OpenRadioPairEndpoint tx = registered.Get(txKey);
			if (!tx || !ProjectEndpoint(candidate, world, lifecycle, receivers, devices, samples, tx,
				admissionRoute, source.m_iPlayerId, nowMs)) return "RADIO_UNQUALIFIED";
			if (!ProjectPlayer(candidate, players, lifecycle, source.m_iPlayerId, source.m_iPlayerGeneration)) return "CAPACITY";
			RICON_TransmissionBinding binding = new RICON_TransmissionBinding();
			binding.m_iLeaseId = lease.m_iLeaseId;
			binding.m_iSessionEpoch = candidate.m_iSessionEpoch;
			binding.m_iTxPlayerId = lease.m_iPlayerId;
			binding.m_iTxGeneration = lease.m_iGeneration;
			binding.m_iTxDeviceId = lease.m_iDeviceId;
			binding.m_iTxTransceiverIndex = source.m_iTxTransceiverIndex;
			binding.m_iNetworkTypeId = lease.m_iNetworkTypeId;
			binding.m_iChannelId = lease.m_iChannelId;
			binding.m_bActive = true;
			binding.m_iIssuedAtMs = lease.m_iIssuedAtMs;
			binding.m_iTtlMs = lease.m_iTtlMs;
			candidate.m_aBindings.Insert(binding);
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
				receivers, devices, samples, rx, admissionRoute, listener.m_iPlayerId, nowMs)) return "RADIO_UNQUALIFIED";
			if (!ProjectPlayer(candidate, players, lifecycle, listener.m_iPlayerId, listener.m_iPlayerGeneration)) return "CAPACITY";
			RICON_PairListener row = new RICON_PairListener();
			row.m_iSessionEpoch = candidate.m_iSessionEpoch;
			row.m_iPlayerId = listener.m_iPlayerId;
			row.m_iGeneration = listener.m_iPlayerGeneration;
			row.m_iDeviceId = listener.m_iDeviceId;
			row.m_iTransceiverIndex = listener.m_iIndex;
			candidate.m_aListeners.Insert(row);
		}

		foreach (RICON_OpenRadioPairSample sample : samples)
		{
			RICON_RadioObservation observation = sample.m_PairEndpoint.m_Observation;
			if (!sample.m_Owner || sample.m_Owner.GetWorld() != world || !sample.m_Radio || sample.m_Radio.GetOwner() != sample.m_Owner
				|| AntennaPosition(sample.m_Owner, sample.m_vLocalAntennaOffset) != observation.m_vAntennaPos) return "RADIO_UNQUALIFIED";
			if (!receivers.MatchesRadio(observation.m_iDeviceId, observation.m_iTransceiverIndex, sample.m_iGeneration, sample.m_Radio)
				|| ReadAdmission(receivers, admissionRoute, observation.m_iDeviceId, observation.m_iTransceiverIndex,
					sample.m_iGeneration, nowMs) != sample.m_iAdmission) return "RADIO_UNQUALIFIED";
		}
		foreach (RICON_MediaStreamBinding held : sources)
		{
			if (!streams.IsSourceCurrent(held, lifecycle, nowMs)) return "STALE_LIFECYCLE";
		}
		foreach (RICON_OpenRadioPairListener heldListener : listeners)
		{
			if (!ListenerCurrent(lifecycle, heldListener)) return "STALE_LIFECYCLE";
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

	protected static int ReadAdmission(RICON_MediaReceiverAuthority receivers, RICON_MediaEgressRoute route,
		int deviceId, int slotIndex, int generation, int nowMs)
	{
		route.m_iRxDeviceId = deviceId;
		route.m_iRxTransceiverIndex = slotIndex;
		route.m_iRxGeneration = generation;
		return receivers.ReadAdmission(route, nowMs);
	}

	protected static bool ProjectEndpoint(RICON_PairServiceInput candidate, BaseWorld world, RICON_LifecycleStateMachine lifecycle,
		RICON_MediaReceiverAuthority receivers, map<int, ref RICON_LifecycleDevice> devices,
		map<string, ref RICON_OpenRadioPairSample> samples, RICON_OpenRadioPairEndpoint endpoint,
		RICON_MediaEgressRoute admissionRoute, int playerId, int nowMs)
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
			|| !device.Allows(admissionRoute.m_Source.m_iNetworkTypeId, admissionRoute.m_Source.m_iChannelId)) return false;
		string key = EndpointKey(deviceId, slotIndex);
		if (samples.Contains(key)) return true;
		BaseRadioComponent radio = endpoint.m_Radio;
		if (!radio) return false;
		IEntity owner = radio.GetOwner();
		if (!owner || owner.GetWorld() != world || !receivers.MatchesRadio(deviceId, slotIndex, endpoint.m_iGeneration, radio)) return false;
		int admission = ReadAdmission(receivers, admissionRoute, deviceId, slotIndex, endpoint.m_iGeneration, nowMs);
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

	protected static RICON_RadioRfProfile CopyProfile(RICON_RadioRfProfile source)
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
