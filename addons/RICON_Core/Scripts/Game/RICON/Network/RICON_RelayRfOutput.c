//! A typed station emission and its separate R.TX -> B.RX budget.
class RICON_RelayRfOutputFrame
{
	ref RICON_RelayPcmOutputFrame m_Frame;
	ref RICON_LinkBudget m_Budget;
	int m_iModelAtMs;
	int m_iCoveredUntilMs, m_iReleasedAtMs;
	string m_sBandId, m_sTimebaseId;
	float m_fFrequencyMHz, m_fBandwidthHz;
}

class RICON_RelayAddressedFrame
{
	ref RICON_RelayDestination m_Destination;
	ref RICON_RelayRfOutputFrame m_Emission;
}

//! A trusted server caller supplies stationary observations and transfers sole Take ownership of the PCM output.
//! An optional controlled geometry seal qualifies the complete 4 ms emission.
//! Without a seal the diagnostic result carries a point budget and no coverage timebase or end.
class RICON_RelayRfOutput
{
	protected ref RICON_RelayStationAuthority m_Authority;
	protected ref RICON_RelayPcmOutput m_Output;
	protected ref RICON_RelayPcmPath m_Path;
	protected ref RICON_LinkQuery m_Query;
	protected ref RICON_WirelessLinkEvaluator m_Evaluator;
	protected ref RICON_StaticLosGeometryAuthority m_Geometry;
	protected ref RICON_RelayRfOutputFrame m_Pending;
	protected int m_iNextFixedEmitMs = -1;
	protected bool m_bTakeAttempted;
	protected ref RICON_RelayRfOutputFrame m_Completed;
	protected ref RICON_RelayAddressedFrame m_Addressed;
	protected string m_sTimebaseId;
	protected int m_iLastNowMs, m_iValidUntilMs;
	protected bool m_bClosed, m_bEvaluating;

	bool Configure(RICON_RelayStationAuthority authority, RICON_RelayPcmOutput output, RICON_RelayPcmPath path,
		RICON_PairEndpoint stationTx, RICON_PairEndpoint recipientRx, RICON_RadioQueryContext context,
		RICON_WirelessLinkEvaluator evaluator, int nowMs, int validUntilMs,
		RICON_StaticLosGeometryAuthority geometry = null, string timebaseId = "")
	{
		if (m_bClosed || m_bEvaluating || m_Output || nowMs < 0 || validUntilMs <= nowMs || !path) return false;
		if (!authority || authority.Type() != RICON_RelayStationAuthority || !output || output.Type() != RICON_RelayPcmOutput)
			return false;
		if (!stationTx || !recipientRx || !context || context.m_bHasInterference || !evaluator || !evaluator.UsesFixedRfModels())
			return false;
		if (!authority.MatchesOutputEndpoints(path, stationTx.m_Observation, recipientRx.m_Observation)) return false;
		if (geometry)
		{
			if (!RICON_TemporalSnapshots.ValidKey(timebaseId) || !authority.OwnsGeometryScope(geometry)
				|| !geometry.QualifiesFixedPair(stationTx, path.m_iRelayTxGeneration, recipientRx,
					path.m_iDestinationGeneration, timebaseId, nowMs)) return false;
		}
		else if (!timebaseId.IsEmpty()) return false;

		RICON_LinkQuery query;
		if (RICON_RadioQueryBuilder.Build(stationTx.m_Observation, stationTx.m_Profile, recipientRx.m_Observation,
			recipientRx.m_Profile, context, query) != RICON_ERadioQueryCode.READY) return false;

		RICON_RelayPcmPath boundPath = path.ClonePath();
		if (!authority.CanObserveTime(nowMs) || !output.CanTakeAt(nowMs)) return false;
		if (!HasCurrentAuthority(authority, output, boundPath, nowMs)) return false;
		RICON_LinkBudget budget = Evaluate(evaluator, query);
		if (m_bClosed || !budget || !budget.m_bValid || !authority.CanObserveTime(nowMs) || !output.CanTakeAt(nowMs)) return false;
		if (!HasCurrentAuthority(authority, output, boundPath, nowMs)) return false;
		if (geometry && !geometry.IsCurrent()) return false;

		m_Authority = authority;
		m_Output = output;
		m_Path = boundPath;
		m_Query = query;
		m_Evaluator = evaluator;
		m_Geometry = geometry;
		m_sTimebaseId = timebaseId;
		m_iLastNowMs = nowMs;
		m_iValidUntilMs = validUntilMs;
		return true;
	}

	//! Сетка доступна только при закреплённой геометрии, до первого чтения и для непрерывной очереди.
	bool EnableFixedGrid(int firstEmitMs)
	{
		int duration = RICON_MediaPcmTestProfile.FRAME_DURATION_MS;
		if (m_bClosed || m_bEvaluating || m_bTakeAttempted || m_iNextFixedEmitMs >= 0
			|| !m_Geometry || !m_Output || !m_Output.HasSeriesQueue()
			|| firstEmitMs < m_iLastNowMs || firstEmitMs >= m_iValidUntilMs
			|| m_iValidUntilMs - firstEmitMs <= duration) return false;
		if (!m_Geometry.CoversFixedInterval(firstEmitMs, firstEmitMs + duration)) return false;
		m_iNextFixedEmitMs = firstEmitMs;
		return true;
	}

	string Take(int nowMs, out RICON_RelayRfOutputFrame result)
	{
		result = null;
		if (m_bClosed || m_bEvaluating || !m_Output) return "CLOSED";
		if (nowMs < m_iLastNowMs || !m_Authority.CanObserveTime(nowMs) || !m_Output.CanTakeAt(nowMs)) return "BAD_TIME";
		m_Completed = null;
		m_Addressed = null;
		m_bTakeAttempted = true;
		m_iLastNowMs = nowMs;
		if (m_iNextFixedEmitMs >= 0) return TakeFixedGrid(nowMs, result);
		if (m_Pending) return Complete(nowMs, result);
		if (nowMs >= m_iValidUntilMs) return Retire("STALE_MODEL");
		if (!HasCurrentAuthority(m_Authority, m_Output, m_Path, nowMs)) return Retire("REVOKED");
		int untilMs;
		if (m_Geometry)
		{
			if (m_iValidUntilMs - nowMs < RICON_MediaPcmTestProfile.FRAME_DURATION_MS) return Retire("STALE_MODEL");
			untilMs = nowMs + RICON_MediaPcmTestProfile.FRAME_DURATION_MS;
			if (!m_Geometry.CoversFixedInterval(nowMs, untilMs)) return Retire("RF_UNCOVERED");
		}

		RICON_LinkBudget budget = Evaluate(m_Evaluator, m_Query);
		if (m_bClosed) return "CLOSED";
		if (!m_Authority.CanObserveTime(nowMs) || !m_Output.CanTakeAt(nowMs)) return "BAD_TIME";
		if (!HasCurrentAuthority(m_Authority, m_Output, m_Path, nowMs)) return Retire("REVOKED");
		if (!budget || !budget.m_bValid) return Retire("RF_INVALID");

		RICON_RelayPcmOutputFrame frame;
		string status = m_Output.Take(nowMs, frame);
		if (status == "EMPTY" || status == "WAIT" || status == "BAD_TIME") return status;
		if (status != "OK") return Retire(status);
		if (!HasCurrentAuthority(m_Authority, m_Output, m_Path, nowMs)) return Retire("REVOKED");
		if (!frame || !m_Path.SamePath(frame.m_Path) || frame.m_iEmitAtMs != nowMs || nowMs >= frame.m_iDeadlineMs
			|| !frame.m_aBytes || frame.m_aBytes.Count() != RICON_MediaPcmTestProfile.PAYLOAD_BYTES) return Retire("INVALID");

		RICON_RelayRfOutputFrame emission = new RICON_RelayRfOutputFrame();
		emission.m_Frame = frame;
		emission.m_Budget = budget;
		emission.m_iModelAtMs = nowMs;
		emission.m_sBandId = m_Query.m_sBandId;
		emission.m_fFrequencyMHz = m_Query.m_fFrequencyMHz;
		emission.m_fBandwidthHz = m_Query.m_fBandwidthHz;
		if (m_Geometry)
		{
			if (untilMs >= frame.m_iDeadlineMs) return Retire("EXPIRED");
			if (!m_Geometry.CoversFixedInterval(nowMs, untilMs)) return Retire("RF_UNCOVERED");
			emission.m_iCoveredUntilMs = untilMs;
			emission.m_sTimebaseId = m_sTimebaseId;
			m_Pending = emission;
			return "RF_PENDING";
		}
		emission.m_iReleasedAtMs = nowMs;
		result = emission;
		return "OK";
	}

	protected string TakeFixedGrid(int nowMs, out RICON_RelayRfOutputFrame result)
	{
		result = null;
		int emitMs = m_iNextFixedEmitMs;
		int duration = RICON_MediaPcmTestProfile.FRAME_DURATION_MS;
		if (nowMs >= m_iValidUntilMs || emitMs >= m_iValidUntilMs || m_iValidUntilMs - emitMs <= duration)
			return Retire("STALE_MODEL");
		if (!HasCurrentAuthority(m_Authority, m_Output, m_Path, nowMs)) return Retire("REVOKED");
		int untilMs = emitMs + duration;
		if (!m_Geometry.CoversFixedInterval(emitMs, untilMs)) return Retire("RF_UNCOVERED");
		if (nowMs < untilMs) return "WAIT";

		RICON_LinkBudget budget = Evaluate(m_Evaluator, m_Query);
		if (m_bClosed) return "CLOSED";
		if (!m_Authority.CanObserveTime(nowMs) || !m_Output.CanTakeAt(nowMs)) return "BAD_TIME";
		if (!budget || !budget.m_bValid) return Retire("RF_INVALID");
		if (!HasCurrentAuthority(m_Authority, m_Output, m_Path, nowMs)) return Retire("REVOKED");
		if (!m_Geometry.CoversFixedInterval(emitMs, untilMs)) return Retire("RF_UNCOVERED");

		RICON_RelayPcmOutputFrame frame;
		string status = m_Output.Take(nowMs, frame, emitMs);
		if (status == "BAD_TIME") return status;
		if (status == "EMPTY" || status == "WAIT") return Retire("GAP");
		if (status != "OK") return Retire(status);
		if (!frame || !frame.m_bContinuousInput || !m_Path.SamePath(frame.m_Path) || frame.m_iEmitAtMs != emitMs
			|| !frame.m_aBytes || frame.m_aBytes.Count() != RICON_MediaPcmTestProfile.PAYLOAD_BYTES) return Retire("INVALID");
		if (nowMs >= frame.m_iDeadlineMs) return Retire("EXPIRED");
		if (!HasCurrentAuthority(m_Authority, m_Output, m_Path, nowMs) || !m_Output.HasCurrentInputPermit()) return Retire("REVOKED");
		if (!m_Geometry.CoversFixedInterval(emitMs, untilMs)) return Retire("RF_UNCOVERED");

		result = new RICON_RelayRfOutputFrame();
		result.m_Frame = frame;
		result.m_Budget = budget;
		result.m_iModelAtMs = emitMs;
		result.m_sBandId = m_Query.m_sBandId;
		result.m_fFrequencyMHz = m_Query.m_fFrequencyMHz;
		result.m_fBandwidthHz = m_Query.m_fBandwidthHz;
		result.m_iCoveredUntilMs = untilMs;
		result.m_iReleasedAtMs = nowMs;
		result.m_sTimebaseId = m_sTimebaseId;
		m_iNextFixedEmitMs = untilMs;
		m_Completed = result;
		return "OK";
	}

	protected string Complete(int nowMs, out RICON_RelayRfOutputFrame result)
	{
		if (!HasCurrentAuthority(m_Authority, m_Output, m_Path, nowMs) || !m_Output.HasCurrentInputPermit())
			return Retire("REVOKED");
		if (nowMs >= m_Pending.m_Frame.m_iDeadlineMs) return Retire("EXPIRED");
		if (!m_Geometry.CoversFixedInterval(m_Pending.m_iModelAtMs, m_Pending.m_iCoveredUntilMs)) return Retire("RF_UNCOVERED");
		if (nowMs < m_Pending.m_iCoveredUntilMs) return "RF_PENDING";
		m_Pending.m_iReleasedAtMs = nowMs;
		result = m_Pending;
		m_Completed = result;
		m_Pending = null;
		return "OK";
	}

	//! Trusted synchronous caller checks its actual release time after resolving the native recipient.
	RICON_RelayAddressedFrame AddressCompleted(RICON_RelayRfOutputFrame frame, int nowMs)
	{
		if (frame != m_Completed || !CanReleaseCompleted(frame, nowMs)) return null;
		RICON_RelayDestination destination = m_Authority.ReadDestinationAddress(m_Path, nowMs);
		if (!destination) return null;
		RICON_RelayAddressedFrame result = new RICON_RelayAddressedFrame();
		result.m_Destination = destination;
		result.m_Emission = frame;
		m_Completed = null;
		m_Addressed = result;
		return result;
	}

	//! Synchronous one-use check at the point of submission.
	bool AuthorizeAddressed(RICON_RelayAddressedFrame frame, RICON_RelayDestination recipient, int nowMs)
	{
		if (!frame || frame != m_Addressed || !frame.m_Destination || !frame.m_Destination.Same(recipient)) return false;
		m_Addressed = null;
		return CanReleaseCompleted(frame.m_Emission, nowMs);
	}

	protected bool CanReleaseCompleted(RICON_RelayRfOutputFrame frame, int nowMs)
	{
		if (m_bClosed || m_bEvaluating || !m_Geometry || !frame || !frame.m_Frame || !m_Path.SamePath(frame.m_Frame.m_Path)
			|| frame.m_sTimebaseId != m_sTimebaseId || frame.m_iReleasedAtMs < frame.m_iCoveredUntilMs
			|| frame.m_iReleasedAtMs != m_iLastNowMs || nowMs < m_iLastNowMs || !m_Authority.CanObserveTime(nowMs)
			|| !m_Output.CanTakeAt(nowMs) || nowMs >= frame.m_Frame.m_iDeadlineMs) return false;
		if (m_iNextFixedEmitMs >= 0 && nowMs >= m_iValidUntilMs) return false;
		return m_Geometry.CoversFixedInterval(frame.m_iModelAtMs, frame.m_iCoveredUntilMs)
			&& HasCurrentAuthority(m_Authority, m_Output, m_Path, nowMs) && m_Output.HasCurrentInputPermit();
	}

	protected static bool HasCurrentAuthority(RICON_RelayStationAuthority authority, RICON_RelayPcmOutput output,
		RICON_RelayPcmPath path, int nowMs)
	{
		return output.MatchesAuthority(path, authority) && authority.CanUseInput(path, nowMs) && authority.CanUseOutput(path, nowMs);
	}

	bool CanPublishGrant(int nowMs)
	{
		if (m_bClosed || m_bEvaluating || !m_Geometry || !m_Output || nowMs < m_iLastNowMs || nowMs >= m_iValidUntilMs
			|| !m_Authority.CanObserveTime(nowMs) || !m_Output.CanTakeAt(nowMs)) return false;
		int duration = RICON_MediaPcmTestProfile.FRAME_DURATION_MS;
		return m_iValidUntilMs - nowMs >= duration && m_Geometry.CoversFixedInterval(nowMs, nowMs + duration)
			&& HasCurrentAuthority(m_Authority, m_Output, m_Path, nowMs);
	}

	protected RICON_LinkBudget Evaluate(RICON_WirelessLinkEvaluator evaluator, RICON_LinkQuery query)
	{
		// The quality model can be an extension. Do not accept reentrant Configure or Take from it.
		m_bEvaluating = true;
		RICON_LinkBudget budget = evaluator.Evaluate(query);
		m_bEvaluating = false;
		return budget;
	}

	protected string Retire(string status)
	{
		Close();
		return status;
	}

	void Close()
	{
		m_bClosed = true;
		if (m_Output) m_Output.Close();
		m_Authority = null;
		m_Output = null;
		m_Path = null;
		m_Query = null;
		m_Evaluator = null;
		m_Geometry = null;
		m_Pending = null;
		m_Completed = null;
		m_Addressed = null;
	}
}
