class RICON_StaticLosFixedEndpoint
{
	BaseRadioComponent m_Radio;
	IEntity m_Owner;
	int m_iDeviceId, m_iIndex, m_iGeneration, m_iRawTune, m_iContinuityAdmission;
	float m_fBandwidthHz;
	vector m_vAxis0, m_vAxis1, m_vAxis2, m_vOrigin, m_vAntennaPoint;
}

//! The trusted scenario qualifies the complete fixed geometry and model interval before sealing.
//! Controlled geometry/environment writes must call Invalidate before mutation. Readback cannot see an unobserved round trip.
class RICON_StaticLosGeometryAuthority
{
	protected static const int MAX_ENDPOINTS = 4096;
	protected BaseWorld m_World;
	protected RICON_LifecycleStateMachine m_Lifecycle;
	protected RICON_MediaReceiverAuthority m_Receivers;
	protected int m_iEpoch;
	protected ref map<string, ref RICON_StaticLosFixedEndpoint> m_Endpoints;
	protected ref RICON_StaticLosProfile m_Profile;
	protected string m_sTimebaseId;
	protected int m_iLastModelMs = -1;
	protected int m_iLastModelSubUs;
	protected RICON_TemporalProducerTick m_LastBuiltTick;
	protected bool m_bInvalidated;
	protected bool m_bControlledStaticContinuity;
	protected bool m_bBuildSucceeded;

	bool Seal(BaseWorld world, RICON_LifecycleStateMachine lifecycle, RICON_MediaReceiverAuthority receivers,
		array<ref RICON_OpenRadioPairEndpoint> endpoints, RICON_StaticLosProfile profile, string timebaseId,
		bool controlledStaticContinuity = false)
	{
		if (m_bInvalidated || m_Profile || !WorldCurrent(world) || !lifecycle || !receivers || !receivers.IsScope(lifecycle)) return false;
		int epoch = lifecycle.GetSessionEpoch();
		if (epoch <= 0 || epoch >= int.MAX || !endpoints || endpoints.Count() < 2 || endpoints.Count() > MAX_ENDPOINTS
			|| !RICON_StaticLosChannel.ValidProfile(profile) || !RICON_TemporalSnapshots.ValidKey(timebaseId)) return false;

		map<string, ref RICON_StaticLosFixedEndpoint> staged = new map<string, ref RICON_StaticLosFixedEndpoint>();
		foreach (RICON_OpenRadioPairEndpoint endpoint : endpoints)
		{
			RICON_StaticLosFixedEndpoint fixed = Snapshot(world, receivers, endpoint, profile.m_fFrequencyMHz);
			if (!fixed) return false;
			if (controlledStaticContinuity)
			{
				if (!IsStationaryRoot(fixed.m_Owner)) return false;
				fixed.m_iContinuityAdmission = receivers.ReadFixedOpenAdmission(fixed.m_iDeviceId, fixed.m_iIndex,
					fixed.m_iGeneration, fixed.m_iRawTune);
				if (fixed.m_iContinuityAdmission <= 0) return false;
			}
			string key = EndpointKey(fixed.m_iDeviceId, fixed.m_iIndex);
			if (staged.Contains(key)) return false;
			staged.Set(key, fixed);
		}
		m_World = world;
		m_Lifecycle = lifecycle;
		m_Receivers = receivers;
		m_iEpoch = epoch;
		m_Endpoints = staged;
		m_Profile = CopyProfile(profile);
		m_sTimebaseId = timebaseId;
		m_bControlledStaticContinuity = controlledStaticContinuity;
		return IsCurrent();
	}

	//! Checks observed geometry only. Expiry is latched when BuildTick receives a trusted model timestamp.
	bool IsScope(BaseWorld world, RICON_LifecycleStateMachine lifecycle, RICON_MediaReceiverAuthority receivers)
	{
		return world && world == m_World && IsLifecycleScope(lifecycle, receivers);
	}

	bool IsLifecycleScope(RICON_LifecycleStateMachine lifecycle, RICON_MediaReceiverAuthority receivers)
	{
		return m_Profile && !m_bInvalidated && lifecycle && lifecycle == m_Lifecycle && lifecycle.GetSessionEpoch() == m_iEpoch
			&& receivers && receivers == m_Receivers && receivers.IsScope(lifecycle);
	}

	bool IsCurrent()
	{
		if (!m_Profile || m_bInvalidated) return false;
		if (!WorldCurrent(m_World) || !m_Lifecycle || m_Lifecycle.GetSessionEpoch() != m_iEpoch
			|| !m_Receivers || !m_Receivers.IsScope(m_Lifecycle))
		{
			Invalidate();
			return false;
		}
		foreach (RICON_StaticLosFixedEndpoint fixed : m_Endpoints)
		{
			if (MatchesPose(fixed) && (!m_bControlledStaticContinuity || MatchesContinuityAdmission(fixed))) continue;
			Invalidate();
			return false;
		}
		return true;
	}

	void Invalidate()
	{
		m_bInvalidated = true;
		m_bBuildSucceeded = false;
		m_LastBuiltTick = null;
	}

	int GetEnvironmentRevision()
	{
		if (!m_Profile) return 0;
		return m_Profile.m_iEnvironmentRevision;
	}

	int GetLastModelNowMs() { return m_iLastModelMs; }
	int GetLastModelSubUs() { return m_iLastModelSubUs; }
	bool UsesControlledRfContinuity() { return m_bControlledStaticContinuity; }

	//! Qualifies an exact fixed pair without the lease and listener projection of a direct radio route.
	bool QualifiesFixedPair(RICON_PairEndpoint tx, int txGeneration, RICON_PairEndpoint rx, int rxGeneration,
		string timebaseId, int nowMs)
	{
		if (!m_bControlledStaticContinuity || !IsCurrent() || m_Endpoints.Count() != 2 || timebaseId != m_sTimebaseId
			|| !MatchesReference(tx) || !MatchesReference(rx)) return false;
		RICON_StaticLosFixedEndpoint txFixed = m_Endpoints.Get(EndpointKey(tx.m_Observation.m_iDeviceId, tx.m_Observation.m_iTransceiverIndex));
		RICON_StaticLosFixedEndpoint rxFixed = m_Endpoints.Get(EndpointKey(rx.m_Observation.m_iDeviceId, rx.m_Observation.m_iTransceiverIndex));
		if (txFixed == rxFixed || txFixed.m_iGeneration != txGeneration || rxFixed.m_iGeneration != rxGeneration) return false;
		RICON_TemporalChannelInput channel;
		return RICON_StaticLosChannel.Build(tx, rx, m_Profile, nowMs, 0, channel) == "OK" && IsCurrent();
	}

	bool CoversFixedInterval(int fromMs, int untilMs)
	{
		return m_bControlledStaticContinuity && m_Profile && fromMs >= m_Profile.m_iValidFromMs
			&& untilMs > fromMs && untilMs <= m_Profile.m_iValidUntilMs && IsCurrent();
	}

	//! Для консервативного интервала захвата верхняя граница входит в проверяемое окно.
	bool CoversFixedCapture(int fromMs, int untilMs)
	{
		return CoversFixedInterval(fromMs, untilMs) && untilMs < m_Profile.m_iValidUntilMs;
	}

	//! The scenario owns all pose and environment writes and invalidates before each change.
	bool CanAttestRfContinuity(int modelNowMs)
	{
		if (!m_bControlledStaticContinuity || !m_bBuildSucceeded || !m_Profile) return false;
		if (modelNowMs != m_iLastModelMs || m_iLastModelSubUs != 0) return false;
		if (modelNowMs < m_Profile.m_iValidFromMs || modelNowMs >= m_Profile.m_iValidUntilMs) return false;
		return IsCurrent();
	}

	//! A newly built tick may be published synchronously with its projection held unchanged.
	bool MatchesTick(RICON_TemporalProducerTick tick, RICON_LifecycleStateMachine lifecycle)
	{
		return tick && tick == m_LastBuiltTick && lifecycle && lifecycle == m_Lifecycle && IsCurrent();
	}

	string BuildTick(RICON_PairServiceInput input, int modelNowMs, int modelSubUs, int ttlMs, float qualityDelta,
		out RICON_TemporalProducerTick tick)
	{
		tick = null;
		m_bBuildSucceeded = false;
		m_LastBuiltTick = null;
		if (!IsCurrent()) return "LOS_GEOMETRY_INVALID";
		if (modelNowMs < 0 || modelSubUs < 0 || modelSubUs > 999) return "INVALID_TIME";
		if (modelNowMs >= m_Profile.m_iValidUntilMs)
		{
			Invalidate();
			return "STALE_DESCRIPTOR";
		}
		if (modelNowMs < m_Profile.m_iValidFromMs) return "STALE_DESCRIPTOR";
		if (modelNowMs < m_iLastModelMs || (modelNowMs == m_iLastModelMs && modelSubUs < m_iLastModelSubUs)) return "CLOCK_ROLLBACK";
		if (!input || input.m_iSessionEpoch != m_iEpoch || !input.m_aEndpoints || input.m_aEndpoints.Count() > m_Endpoints.Count())
			return "INVALID_SCOPE";
		foreach (RICON_PairEndpoint endpoint : input.m_aEndpoints)
		{
			if (!MatchesReference(endpoint)) return "LOS_REFERENCE_MISMATCH";
		}

		RICON_TemporalProducerTick staged;
		string status = RICON_StaticLosTick.Build(input, m_Profile, modelNowMs, modelSubUs, m_sTimebaseId, ttlMs, qualityDelta, staged);
		if (status != "OK") return status;
		if (!IsCurrent()) return "LOS_GEOMETRY_INVALID";
		m_iLastModelMs = modelNowMs;
		m_iLastModelSubUs = modelSubUs;
		staged.m_StaticLosGeometry = this;
		m_LastBuiltTick = staged;
		m_bBuildSucceeded = true;
		tick = staged;
		return "OK";
	}

	protected static bool WorldCurrent(BaseWorld world)
	{
		return world && GetGame() && GetGame().GetWorld() == world && !world.IsEditMode() && RICON_CommsManagerComponent.IsAuthority();
	}

	protected static string EndpointKey(int deviceId, int index) { return string.Format("%1:%2", deviceId, index); }

	protected static bool IsStationaryRoot(IEntity owner)
	{
		if (owner.GetParent()) return false;
		Physics body = owner.GetPhysics();
		return !body || (!body.IsKinematic() && body.GetSimulationState() != SimulationState.SIMULATION);
	}

	protected bool MatchesContinuityAdmission(RICON_StaticLosFixedEndpoint fixed)
	{
		return IsStationaryRoot(fixed.m_Owner) && m_Receivers.ReadFixedOpenAdmission(fixed.m_iDeviceId,
			fixed.m_iIndex, fixed.m_iGeneration, fixed.m_iRawTune) == fixed.m_iContinuityAdmission;
	}

	protected static RICON_StaticLosFixedEndpoint Snapshot(BaseWorld world, RICON_MediaReceiverAuthority receivers,
		RICON_OpenRadioPairEndpoint endpoint, float carrierMHz)
	{
		if (!endpoint || !endpoint.m_Radio || !endpoint.m_Profile || !RICON_RadioQueryBuilder.IsValidProfile(endpoint.m_Profile)
			|| endpoint.m_iGeneration <= 0 || !RICON_Num.IsFiniteVector(endpoint.m_vLocalAntennaOffset)) return null;
		BaseRadioComponent radio = endpoint.m_Radio;
		RICON_RadioRfProfile rf = endpoint.m_Profile;
		IEntity owner = radio.GetOwner();
		if (!owner || owner.IsDeleted() || owner.GetWorld() != world || rf.m_iExpectedTuningValue <= 0 || rf.m_fFrequencyMHz != carrierMHz
			|| !receivers.MatchesRadio(rf.m_iDeviceId, rf.m_iTransceiverIndex, endpoint.m_iGeneration, radio)) return null;

		vector pose[4];
		owner.GetWorldTransform(pose);
		for (int index = 0; index < 4; index++)
		{
			if (!RICON_Num.IsFiniteVector(pose[index])) return null;
		}
		vector offset = endpoint.m_vLocalAntennaOffset;
		vector point = pose[3] + pose[0] * offset[0] + pose[1] * offset[1] + pose[2] * offset[2];
		if (!RICON_Num.IsFiniteVector(point)) return null;
		RICON_StaticLosFixedEndpoint fixed = new RICON_StaticLosFixedEndpoint();
		fixed.m_Radio = radio;
		fixed.m_Owner = owner;
		fixed.m_iDeviceId = rf.m_iDeviceId;
		fixed.m_iIndex = rf.m_iTransceiverIndex;
		fixed.m_iGeneration = endpoint.m_iGeneration;
		fixed.m_iRawTune = rf.m_iExpectedTuningValue;
		fixed.m_fBandwidthHz = rf.m_fRxBandwidthHz;
		fixed.m_vAxis0 = pose[0];
		fixed.m_vAxis1 = pose[1];
		fixed.m_vAxis2 = pose[2];
		fixed.m_vOrigin = pose[3];
		fixed.m_vAntennaPoint = point;
		return fixed;
	}

	protected bool MatchesPose(RICON_StaticLosFixedEndpoint fixed)
	{
		IEntity owner = fixed.m_Owner;
		BaseRadioComponent radio = fixed.m_Radio;
		if (!owner || owner.IsDeleted() || owner.GetWorld() != m_World || !radio || radio.GetOwner() != owner
			|| !m_Receivers.MatchesRadio(fixed.m_iDeviceId, fixed.m_iIndex, fixed.m_iGeneration, radio)) return false;
		vector pose[4];
		owner.GetWorldTransform(pose);
		return pose[0] == fixed.m_vAxis0 && pose[1] == fixed.m_vAxis1 && pose[2] == fixed.m_vAxis2 && pose[3] == fixed.m_vOrigin;
	}

	protected bool MatchesReference(RICON_PairEndpoint endpoint)
	{
		if (!endpoint || !endpoint.m_Observation || !endpoint.m_Profile) return false;
		RICON_RadioObservation observation = endpoint.m_Observation;
		RICON_RadioRfProfile rf = endpoint.m_Profile;
		RICON_StaticLosFixedEndpoint fixed = m_Endpoints.Get(EndpointKey(observation.m_iDeviceId, observation.m_iTransceiverIndex));
		return fixed && observation.m_bPowered && observation.m_sNetworkKey.IsEmpty() && observation.m_iTuningValue == fixed.m_iRawTune
			&& observation.m_vAntennaPos == fixed.m_vAntennaPoint && rf.m_iDeviceId == fixed.m_iDeviceId
			&& rf.m_iTransceiverIndex == fixed.m_iIndex && rf.m_iExpectedTuningValue == fixed.m_iRawTune
			&& rf.m_fFrequencyMHz == m_Profile.m_fFrequencyMHz && rf.m_fRxBandwidthHz == fixed.m_fBandwidthHz;
	}

	static RICON_StaticLosProfile CopyProfile(RICON_StaticLosProfile source)
	{
		RICON_StaticLosProfile copy = new RICON_StaticLosProfile();
		copy.m_sProfileId = source.m_sProfileId;
		copy.m_iProfileVersion = source.m_iProfileVersion;
		copy.m_iEnvironmentRevision = source.m_iEnvironmentRevision;
		copy.m_iValidFromMs = source.m_iValidFromMs;
		copy.m_iValidUntilMs = source.m_iValidUntilMs;
		copy.m_fFrequencyMHz = source.m_fFrequencyMHz;
		copy.m_fBandwidthHz = source.m_fBandwidthHz;
		copy.m_fMinimumDistanceM = source.m_fMinimumDistanceM;
		copy.m_fMaximumDistanceM = source.m_fMaximumDistanceM;
		copy.m_fDistanceErrorM = source.m_fDistanceErrorM;
		copy.m_fMaxPhaseErrorTurns = source.m_fMaxPhaseErrorTurns;
		copy.m_bSinglePathQualified = source.m_bSinglePathQualified;
		return copy;
	}
}
