//! One fixed open station grant, installed by the trusted control plane on B.
class RICON_RelayWireGrant
{
	int m_iWireVersion = 1;
	int m_iEpoch, m_iEmissionId, m_iBridgeGeneration, m_iTransformGeneration;
	int m_iTxDeviceId, m_iTxIndex, m_iTxGeneration;
	int m_iOriginStreamId, m_iOriginGeneration, m_iOriginCaptureEpoch;
	int m_iFirstSequence, m_iFirstSampleIndex;
	int m_iOriginFirstSequence, m_iOriginFirstSampleIndex;
	string m_sTimebaseId;
	ref RICON_RelayDestination m_Destination;

	static RICON_RelayWireGrant FromAddress(RICON_RelayAddressedFrame addressed)
	{
		if (!addressed || !addressed.m_Destination || !addressed.m_Emission || !addressed.m_Emission.m_Frame) return null;
		RICON_RelayPcmOutputFrame frame = addressed.m_Emission.m_Frame;
		if (addressed.m_Emission.m_iCoveredUntilMs <= addressed.m_Emission.m_iModelAtMs
			|| addressed.m_Emission.m_iReleasedAtMs < addressed.m_Emission.m_iCoveredUntilMs) return null;
		return FromPath(frame.m_Path, addressed.m_Destination, frame.m_iSequence, frame.m_iFirstSampleIndex, addressed.m_Emission.m_sTimebaseId);
	}

	static RICON_RelayWireGrant FromPath(RICON_RelayPcmPath path, RICON_RelayDestination destination,
		int firstSequence, int firstSampleIndex, string timebaseId)
	{
		if (!path || !path.IsValid() || !destination || destination.m_iRxDeviceId != path.m_iDestinationEndpointId
			|| destination.m_iRxGeneration != path.m_iDestinationGeneration) return null;
		RICON_MediaStreamBinding origin = path.m_UpstreamRoute.m_Source;
		RICON_RelayWireGrant grant = new RICON_RelayWireGrant();
		grant.m_iEpoch = origin.m_iSessionEpoch;
		grant.m_iEmissionId = path.m_iRelayEmissionId;
		grant.m_iBridgeGeneration = path.m_iBridgeGeneration;
		grant.m_iTransformGeneration = path.m_iTransformGeneration;
		grant.m_iTxDeviceId = path.m_iRelayTxDeviceId;
		grant.m_iTxIndex = path.m_iRelayTxIndex;
		grant.m_iTxGeneration = path.m_iRelayTxGeneration;
		grant.m_iOriginStreamId = origin.m_iStreamId;
		grant.m_iOriginGeneration = origin.m_iStreamGeneration;
		grant.m_iOriginCaptureEpoch = origin.m_iCaptureEpoch;
		grant.m_iFirstSequence = firstSequence;
		grant.m_iFirstSampleIndex = firstSampleIndex;
		grant.m_sTimebaseId = timebaseId;
		grant.m_Destination = destination.CloneDestination();
		if (!grant.IsValid()) return null;
		return grant;
	}

	bool IsValid()
	{
		return ValidWireProfile() && m_iEpoch > 0 && m_iEpoch < int.MAX && m_iEmissionId > 0 && m_iBridgeGeneration > 0 && m_iTransformGeneration > 0
			&& m_iTxDeviceId > 0 && m_iTxIndex >= 0 && m_iTxGeneration > 0 && m_iOriginStreamId > 0
			&& m_iOriginGeneration > 0 && m_iOriginCaptureEpoch > 0 && m_Destination && m_Destination.IsValid()
			&& RICON_MediaWireV1.ValidRange(m_iFirstSequence, m_iFirstSampleIndex, RICON_MediaPcmProfile.FRAME_SAMPLES)
			&& RICON_TemporalSnapshots.ValidKey(m_sTimebaseId);
	}

	protected bool ValidWireProfile()
	{
		if (m_iWireVersion == 1) return m_iOriginFirstSequence == 0 && m_iOriginFirstSampleIndex == 0;
		return m_iWireVersion == 2 && RICON_MediaWireV1.ValidRange(m_iOriginFirstSequence, m_iOriginFirstSampleIndex,
			RICON_MediaPcmProfile.FRAME_SAMPLES);
	}

	bool MatchesSample(int sequence, int firstSampleIndex)
	{
		if (!RICON_MediaWireV1.ValidRange(sequence, firstSampleIndex, RICON_MediaPcmProfile.FRAME_SAMPLES)
			|| sequence < m_iFirstSequence || firstSampleIndex < m_iFirstSampleIndex) return false;
		int delta = firstSampleIndex - m_iFirstSampleIndex;
		return delta % RICON_MediaPcmProfile.FRAME_SAMPLES == 0
			&& delta / RICON_MediaPcmProfile.FRAME_SAMPLES == sequence - m_iFirstSequence;
	}

	bool ReadOriginSpan(int sequence, int firstSampleIndex, out int originSequence, out int originStart, out int windows)
	{
		if (m_iWireVersion != 2 || !IsValid() || !MatchesSample(sequence, firstSampleIndex)) return false;
		int delta = sequence - m_iFirstSequence;
		windows = 1 + Math.Min(delta, 3);
		int oldestDelta = delta - (windows - 1);
		int samples = RICON_MediaPcmProfile.FRAME_SAMPLES;
		if (m_iOriginFirstSequence > int.MAX - windows - oldestDelta
			|| oldestDelta > (int.MAX - windows * samples - m_iOriginFirstSampleIndex) / samples) return false;
		originSequence = m_iOriginFirstSequence + oldestDelta;
		originStart = m_iOriginFirstSampleIndex + oldestDelta * samples;
		return originStart <= int.MAX - windows * samples;
	}

	bool Same(RICON_RelayWireGrant other)
	{
		return SameScope(other) && m_iFirstSequence == other.m_iFirstSequence && m_iFirstSampleIndex == other.m_iFirstSampleIndex;
	}

	bool SameScope(RICON_RelayWireGrant other)
	{
		if (!other || m_iWireVersion != other.m_iWireVersion || m_iOriginFirstSequence != other.m_iOriginFirstSequence
			|| m_iOriginFirstSampleIndex != other.m_iOriginFirstSampleIndex) return false;
		return m_iEpoch == other.m_iEpoch && m_iEmissionId == other.m_iEmissionId
			&& m_iBridgeGeneration == other.m_iBridgeGeneration && m_iTransformGeneration == other.m_iTransformGeneration
			&& m_iTxDeviceId == other.m_iTxDeviceId && m_iTxIndex == other.m_iTxIndex && m_iTxGeneration == other.m_iTxGeneration
			&& m_iOriginStreamId == other.m_iOriginStreamId && m_iOriginGeneration == other.m_iOriginGeneration
			&& m_iOriginCaptureEpoch == other.m_iOriginCaptureEpoch && m_sTimebaseId == other.m_sTimebaseId
			&& m_Destination && m_Destination.Same(other.m_Destination);
	}

	//! Fixed route header. The direct wire formats start with a positive sample count.
	array<int> RouteFields()
	{
		int discriminator = RICON_RelayWire.DISCRIMINATOR;
		if (m_iWireVersion == 2) discriminator = RICON_RelayWire.DISCRIMINATOR_V2;
		array<int> fields = {discriminator, m_iEpoch, m_iEmissionId, m_iBridgeGeneration,
			m_iTransformGeneration, m_iTxDeviceId, m_iTxIndex, m_iTxGeneration, m_iOriginStreamId,
			m_iOriginGeneration, m_iOriginCaptureEpoch, m_Destination.m_iPlayerId, m_Destination.m_iConnectionId,
			m_Destination.m_iPlayerGeneration, m_Destination.m_iEntityId, m_Destination.m_iRxDeviceId,
			m_Destination.m_iRxIndex, m_Destination.m_iRxGeneration};
		return fields;
	}

	RICON_RelayWireGrant CloneGrant()
	{
		RICON_RelayWireGrant copy = new RICON_RelayWireGrant();
		copy.m_iWireVersion = m_iWireVersion;
		copy.m_iOriginFirstSequence = m_iOriginFirstSequence;
		copy.m_iOriginFirstSampleIndex = m_iOriginFirstSampleIndex;
		copy.m_iEpoch = m_iEpoch;
		copy.m_iEmissionId = m_iEmissionId;
		copy.m_iBridgeGeneration = m_iBridgeGeneration;
		copy.m_iTransformGeneration = m_iTransformGeneration;
		copy.m_iTxDeviceId = m_iTxDeviceId;
		copy.m_iTxIndex = m_iTxIndex;
		copy.m_iTxGeneration = m_iTxGeneration;
		copy.m_iOriginStreamId = m_iOriginStreamId;
		copy.m_iOriginGeneration = m_iOriginGeneration;
		copy.m_iOriginCaptureEpoch = m_iOriginCaptureEpoch;
		copy.m_iFirstSequence = m_iFirstSequence;
		copy.m_iFirstSampleIndex = m_iFirstSampleIndex;
		copy.m_sTimebaseId = m_sTimebaseId;
		copy.m_Destination = m_Destination.CloneDestination();
		return copy;
	}
}

class RICON_RelayWireFrame
{
	int m_iLocalDeadlineMs;
	int m_iSequence, m_iFirstSampleIndex, m_iEmitFromMs, m_iEmitUntilMs;
	float m_fRxPowerDbm, m_fNoiseDbm;
	int m_iBandwidthHz, m_iOriginSequence, m_iOriginWindows, m_iOriginFirstSampleIndex, m_iOriginSampleCount;
	ref array<int> m_aBytes;
}

//! One local presentation window. R's emitted sample index and server RF stamps remain provenance.
class RICON_RelayPresentationWindow
{
	ref RICON_RelayWireGrant m_Grant;
	ref RICON_RelayWireFrame m_Emission;
	ref array<int> m_aSamples;
	int m_iTargetMs, m_iFirstSourceOutputIndex, m_iDeadlineMs;
	ref RICON_RelayOutputPermit m_Permit;
}

//! Only the issuing RelayWire receiver may authorize this relay generation.
class RICON_RelayOutputPermit : RICON_MediaOutputPermit
{
	protected RICON_RelayWireReceiver m_Relay;

	void BindRelay(RICON_RelayWireReceiver receiver) { m_Relay = receiver; }
	override bool IsCurrent() { return m_Relay && m_Relay.HasPresentationPermit(this); }
	override bool MatchesRoute(RICON_MediaEgressRoute route) { return false; }
	bool MatchesGrant(RICON_RelayWireGrant grant) { return m_Relay && m_Relay.HasPresentationPermit(this, grant); }
	bool ConsumeWindow(RICON_RelayPresentationWindow window, int nowMs)
	{
		return m_Relay && m_Relay.ConsumePresentationWindow(this, window, nowMs);
	}
}

//! TEST_ONLY fixed mono PCM16LE 8 kHz. RF powers are centi-dBm; duration is exactly 4 ms.
class RICON_RelayWire
{
	static const int DISCRIMINATOR = -101;
	static const int DISCRIMINATOR_V2 = -102;
	static const int ROUTE_VALUES = 18;
	static const int HEADER_VALUES = 29;
	static const int BODY_VALUES = HEADER_VALUES + RICON_MediaPcmProfile.PAYLOAD_BYTES;

	static array<int> Encode(RICON_RelayAddressedFrame addressed, RICON_RelayWireGrant installed = null)
	{
		RICON_RelayWireGrant grant = RICON_RelayWireGrant.FromAddress(addressed);
		if (!grant) return null;
		RICON_RelayRfOutputFrame emission = addressed.m_Emission;
		RICON_RelayPcmOutputFrame frame = emission.m_Frame;
		if (installed && !installed.IsValid()) return null;
		bool series = installed && installed.m_iWireVersion == 2;
		if (frame.m_bContinuousInput != series) return null;
		if (series)
		{
			grant.m_iWireVersion = 2;
			grant.m_iOriginFirstSequence = installed.m_iOriginFirstSequence;
			grant.m_iOriginFirstSampleIndex = installed.m_iOriginFirstSampleIndex;
		}
		if (installed && (!installed.SameScope(grant) || !installed.MatchesSample(frame.m_iSequence, frame.m_iFirstSampleIndex))) return null;
		RICON_LinkBudget budget = emission.m_Budget;
		if (!(emission.m_fBandwidthHz >= 1.0 && emission.m_fBandwidthHz <= RICON_MediaWireRfV2.MAX_BANDWIDTH_HZ)) return null;
		int bandwidth = emission.m_fBandwidthHz;
		if (!budget || !budget.m_bValid || !RICON_MediaWireRfV2.ValidPowerDbm(budget.m_fRxPowerDbm)
			|| !RICON_MediaWireRfV2.ValidPowerDbm(budget.m_fNoiseDbm) || emission.m_fBandwidthHz != bandwidth) return null;
		if (frame.m_iEmitAtMs != emission.m_iModelAtMs || emission.m_iReleasedAtMs < emission.m_iCoveredUntilMs
			|| emission.m_iReleasedAtMs >= frame.m_iDeadlineMs) return null;
		if (!frame.m_aBytes || frame.m_aBytes.Count() != RICON_MediaPcmProfile.PAYLOAD_BYTES || !frame.m_aProvenance) return null;
		int windows, originSequence, originStart, originCount;
		if (series)
		{
			if (!installed.ReadOriginSpan(frame.m_iSequence, frame.m_iFirstSampleIndex, originSequence, originStart, windows)
				|| !MatchesSeriesProvenance(installed, frame, originSequence, originStart, windows)) return null;
			originCount = windows * RICON_MediaPcmProfile.FRAME_SAMPLES;
		}
		else if (!ReadLegacyProvenance(frame, originSequence, originStart, windows, originCount)) return null;
		array<int> body = grant.RouteFields();
		body.Reserve(BODY_VALUES);
		body.Insert(frame.m_iSequence);
		body.Insert(frame.m_iFirstSampleIndex);
		body.Insert(frame.m_iEmitAtMs);
		body.Insert(emission.m_iCoveredUntilMs);
		body.Insert(RICON_MediaWireRfV2.QuantizePower(budget.m_fRxPowerDbm));
		body.Insert(RICON_MediaWireRfV2.QuantizePower(budget.m_fNoiseDbm));
		body.Insert(bandwidth);
		body.Insert(originSequence);
		body.Insert(windows);
		body.Insert(originStart);
		body.Insert(originCount);
		if (!ValidMedia(body, installed)) return null;
		foreach (int value : frame.m_aBytes)
		{
			if (value < 0 || value > 255) return null;
			body.Insert(value);
		}
		return body;
	}

	protected static bool ReadLegacyProvenance(RICON_RelayPcmOutputFrame frame, out int originSequence, out int originStart,
		out int windows, out int originCount)
	{
		windows = frame.m_aProvenance.Count();
		if (windows != Math.Min(frame.m_iSequence + 1, 3)) return false;
		RICON_RelayPcmOutputOrigin first = frame.m_aProvenance[0];
		if (!first || !first.m_Origin) return false;
		originSequence = first.m_Origin.m_iSequence;
		originStart = first.m_Origin.m_iFirstSampleIndex;
		if (originSequence < 0 || originSequence > int.MAX - windows || originStart < 0) return false;
		int originEnd = originStart;
		for (int i = 0; i < windows; i++)
		{
			RICON_RelayPcmOutputOrigin part = frame.m_aProvenance[i];
			if (!part || !part.m_Origin || part.m_Origin.m_bContinuousInput || part.m_Origin.m_PreviousInput || part.m_Origin.m_CaptureEnvelope
				|| part.m_iSequence != frame.m_iSequence - windows + 1 + i
				|| part.m_Origin.m_iSequence != originSequence + i || part.m_Origin.m_iFirstSampleIndex != originEnd
				|| part.m_Origin.m_iSampleCount <= 0 || originEnd > int.MAX - part.m_Origin.m_iSampleCount) return false;
			originEnd += part.m_Origin.m_iSampleCount;
		}
		originCount = originEnd - originStart;
		return true;
	}

	protected static bool MatchesSeriesProvenance(RICON_RelayWireGrant grant, RICON_RelayPcmOutputFrame frame,
		int originSequence, int originStart, int windows)
	{
		int parts = 1 + Math.Min(frame.m_iSequence - grant.m_iFirstSequence, 2);
		if (frame.m_aProvenance.Count() != parts || frame.m_iSourceOriginWindows != windows
			|| frame.m_iOldestSourceSequence != originSequence) return false;
		int samples = RICON_MediaPcmProfile.FRAME_SAMPLES;
		int rateRatio = RICON_PcmResampler8To48.OUTPUT_RATE_HZ / RICON_PcmResampler8To48.INPUT_RATE_HZ;
		int stationFirst = frame.m_iSequence - (parts - 1);
		int nextOriginSequence = originSequence + windows - parts;
		int nextOriginSample = originStart + (windows - parts) * samples;
		RICON_MediaStreamBinding source = frame.m_Path.m_UpstreamRoute.m_Source;
		RICON_RelayPcmOrigin prior;
		for (int i = 0; i < parts; i++)
		{
			RICON_RelayPcmOutputOrigin part = frame.m_aProvenance[i];
			if (!part || part.m_iSequence != stationFirst + i || part.m_iRxFirstSampleIndex < 0
				|| part.m_iRxFirstSampleIndex % rateRatio != 0 || part.m_iRxFirstSampleIndex / rateRatio != nextOriginSample) return false;
			RICON_RelayPcmOrigin origin = part.m_Origin;
			if (!origin || !origin.m_bContinuousInput || !origin.IsValid(frame.m_iEmitAtMs)
				|| origin.m_iSequence != nextOriginSequence || origin.m_iFirstSampleIndex != nextOriginSample
				|| origin.EffectiveDeadlineMs() < frame.m_iDeadlineMs) return false;
			if (origin.m_CaptureEnvelope && (origin.m_CaptureEnvelope.m_sTimebaseId != grant.m_sTimebaseId
				|| !source.Same(origin.m_CaptureEnvelope.m_Binding))) return false;
			RICON_RelayPcmOrigin previous = origin.m_PreviousInput;
			if (nextOriginSequence == grant.m_iOriginFirstSequence)
			{
				if (previous) return false;
			}
			else if (!previous || (prior && !previous.SameWindow(prior))) return false;
			prior = origin;
			nextOriginSequence++;
			nextOriginSample += samples;
		}
		return true;
	}

	static RICON_RelayWireFrame Decode(RICON_RelayWireGrant grant, array<int> body, string timebaseId)
	{
		if (!grant || !grant.IsValid() || grant.m_sTimebaseId != timebaseId || !body || body.Count() != BODY_VALUES) return null;
		array<int> fields = grant.RouteFields();
		for (int i = 0; i < ROUTE_VALUES; i++)
		{
			if (body[i] != fields[i]) return null;
		}
		if (!grant.MatchesSample(body[18], body[19]) || !ValidMedia(body, grant)) return null;
		array<int> bytes = {};
		bytes.Reserve(RICON_MediaPcmProfile.PAYLOAD_BYTES);
		for (int index = HEADER_VALUES; index < BODY_VALUES; index++)
		{
			int value = body[index];
			if (value < 0 || value > 255) return null;
			bytes.Insert(value);
		}
		RICON_RelayWireFrame frame = new RICON_RelayWireFrame();
		frame.m_iSequence = body[18];
		frame.m_iFirstSampleIndex = body[19];
		frame.m_iEmitFromMs = body[20];
		frame.m_iEmitUntilMs = body[21];
		frame.m_fRxPowerDbm = body[22] / 100.0;
		frame.m_fNoiseDbm = body[23] / 100.0;
		frame.m_iBandwidthHz = body[24];
		frame.m_iOriginSequence = body[25];
		frame.m_iOriginWindows = body[26];
		frame.m_iOriginFirstSampleIndex = body[27];
		frame.m_iOriginSampleCount = body[28];
		frame.m_aBytes = bytes;
		return frame;
	}

	protected static bool ValidMedia(array<int> body, RICON_RelayWireGrant grant)
	{
		if (body[20] < 0 || body[21] <= body[20] || body[21] - body[20] != RICON_MediaPcmProfile.FRAME_DURATION_MS
			|| !RICON_MediaWireRfV2.ValidWirePower(body[22]) || !RICON_MediaWireRfV2.ValidWirePower(body[23])
			|| body[24] < 1 || body[24] > RICON_MediaWireRfV2.MAX_BANDWIDTH_HZ) return false;
		if (grant && grant.m_iWireVersion == 2)
		{
			int originSequence, originStart, windows;
			return grant.ReadOriginSpan(body[18], body[19], originSequence, originStart, windows)
				&& body[25] == originSequence && body[26] == windows && body[27] == originStart
				&& body[28] == windows * RICON_MediaPcmProfile.FRAME_SAMPLES;
		}
		return body[26] == Math.Min(body[18] + 1, 3) && body[25] >= 0 && body[25] <= int.MAX - body[26]
			&& body[27] >= 0 && body[28] >= body[26] && body[27] <= int.MAX - body[28];
	}
}

//! One preinstalled grant. Call only after authenticating the control and media sender as the server.
//! The caller owns local playout; local arrival time is independent of relay model time.
class RICON_RelayWireReceiver
{
	protected static const int QUEUE_CAPACITY = 8;
	protected static const int QUEUE_AGE_MS = 120;
	protected int m_iQueueCapacity = QUEUE_CAPACITY;
	protected ref RICON_RelayWireGrant m_Grant;
	protected ref RICON_RelayOutputPermit m_Permit;
	protected SCR_PlayerController m_Controller;
	protected ref RICON_MediaWindow m_Window = new RICON_MediaWindow();
	protected ref array<ref RICON_RelayWireFrame> m_Pending = {};
	protected ref RICON_RelayPresentationWindow m_Prepared;
	protected ref RICON_RelayPresentationWindow m_CommittedPresentation;
	protected int m_iCommittedDeadlineMs;
	protected ref RICON_PcmResampler8To48 m_Resampler;
	protected int m_iPresentedSequence = -1;
	protected int m_iPreparedSequenceFloor = -1;
	protected int m_iPresentedEmitEnd, m_iPresentedTargetMs, m_iPresentedDeadlineMs;
	protected int m_iLastAcceptedAtMs = -1;
	protected int m_iAnchorSequence = -1, m_iAnchorArrivalMs;
	protected int m_iExpectedWireVersion = 1;
	protected bool m_bClosed, m_bWireVersionExpected;

	//! Выбор профиля производится до установки первого допуска.
	bool ExpectWireVersion(int version)
	{
		if (m_bClosed || m_Grant || (version != 1 && version != 2)) return false;
		if (m_bWireVersionExpected) return m_iExpectedWireVersion == version;
		m_iExpectedWireVersion = version;
		m_bWireVersionExpected = true;
		return true;
	}

	bool Install(RICON_RelayWireGrant grant, RICON_RelayDestination localDestination, int currentEpoch, int width, int queueCapacity = QUEUE_CAPACITY)
	{
		if (m_bClosed || m_Grant || !grant || !grant.IsValid() || grant.m_iWireVersion != m_iExpectedWireVersion || currentEpoch != grant.m_iEpoch
			|| !grant.m_Destination.Same(localDestination) || width < 1 || width > 64
			|| queueCapacity < 1 || queueCapacity > 16) return false;
		if (!m_Window.Initialize(grant.m_iTransformGeneration, grant.m_iFirstSequence, width,
			RICON_MediaPcmProfile.PAYLOAD_BYTES)) return false;
		m_Grant = grant.CloneGrant();
		m_iQueueCapacity = queueCapacity;
		m_Permit = new RICON_RelayOutputPermit(null, 0);
		m_Permit.BindRelay(this);
		return true;
	}

	bool BindOutputController(SCR_PlayerController controller)
	{
		if (m_bClosed || m_Controller || !controller) return false;
		m_Controller = controller;
		return true;
	}

	bool HasPresentationPermit(RICON_RelayOutputPermit permit, RICON_RelayWireGrant grant = null)
	{
		if (m_bClosed || !m_Grant || !permit || permit != m_Permit || (grant && !m_Grant.Same(grant))) return false;
		return !m_Controller || m_Controller.RICON_IsRelayOutputReceiverCurrent(this);
	}

	//! This receiver is scoped to a single server publication for its whole lifetime.
	bool ApplyControl(int controlSeq, bool install, array<int> fields, string timebaseId,
		RICON_RelayDestination localDestination, int currentEpoch, int width)
	{
		if (m_bClosed) return false;
		RICON_RelayWireGrant grant = RICON_RelayWireControl.Decode(fields, timebaseId);
		if (!grant || grant.m_iWireVersion != m_iExpectedWireVersion || grant.m_iEpoch != currentEpoch
			|| !grant.m_Destination.Same(localDestination)) return false;
		if (install && controlSeq == RICON_RelayWireControl.INSTALL_SEQ)
		{
			if (m_Grant) return m_Grant.Same(grant);
			return Install(grant, localDestination, currentEpoch, width);
		}
		if (!install && controlSeq == RICON_RelayWireControl.REVOKE_SEQ && (!m_Grant || m_Grant.Same(grant)))
		{
			Close();
			return true;
		}
		return false;
	}

	RICON_EMediaWindowDecision Receive(array<int> body, string timebaseId, RICON_RelayDestination localDestination,
		int currentEpoch, int arrivalMs, out RICON_RelayWireFrame frame)
	{
		frame = null;
		if (m_bClosed) return RICON_EMediaWindowDecision.CLOSED;
		if (!m_Grant) return RICON_EMediaWindowDecision.INVALID_WINDOW;
		if (currentEpoch != m_Grant.m_iEpoch || !m_Grant.m_Destination.Same(localDestination))
		{
			Close();
			return RICON_EMediaWindowDecision.WRONG_GENERATION;
		}
		if (arrivalMs < 0 || arrivalMs < m_iLastAcceptedAtMs) return RICON_EMediaWindowDecision.RETIRED;
		RICON_RelayWireFrame parsed = RICON_RelayWire.Decode(m_Grant, body, timebaseId);
		if (!parsed) return RICON_EMediaWindowDecision.INVALID_PAYLOAD;
		if (parsed.m_iSequence <= m_iPreparedSequenceFloor) return RICON_EMediaWindowDecision.RETIRED;
		string contextKey;
		for (int i = RICON_RelayWire.ROUTE_VALUES + 2; i < RICON_RelayWire.HEADER_VALUES; i++)
			contextKey += "/" + body[i].ToString();
		RICON_EMediaWindowDecision status = m_Window.Accept(m_Grant.m_iTransformGeneration, parsed.m_iSequence,
			parsed.m_aBytes, parsed.m_iFirstSampleIndex, RICON_MediaPcmProfile.FRAME_SAMPLES, contextKey);
		if (status == RICON_EMediaWindowDecision.ACCEPTED)
		{
			m_iLastAcceptedAtMs = arrivalMs;
			frame = parsed;
		}
		return status;
	}

	//! One bounded arrival FIFO. The consuming presentation owner must still check playout time.
	bool Enqueue(array<int> body, string timebaseId, RICON_RelayDestination localDestination, int epoch, int nowMs)
	{
		if (nowMs < 0 || nowMs > int.MAX - QUEUE_AGE_MS || nowMs < m_iLastAcceptedAtMs) return false;
		DiscardExpired(nowMs);
		if (m_Pending.Count() >= m_iQueueCapacity) return false;
		RICON_RelayWireFrame frame;
		if (Receive(body, timebaseId, localDestination, epoch, nowMs, frame) != RICON_EMediaWindowDecision.ACCEPTED) return false;
		frame.m_iLocalDeadlineMs = nowMs + QUEUE_AGE_MS;
		m_Pending.Insert(frame);
		if (m_iAnchorSequence < 0)
		{
			m_iAnchorSequence = frame.m_iSequence;
			// V2 сохраняет начало выданной серии даже при перестановке первых пакетов.
			if (m_Grant.m_iWireVersion == 2) m_iAnchorSequence = m_Grant.m_iFirstSequence;
			m_iAnchorArrivalMs = nowMs;
		}
		return true;
	}

	int PendingCount() { return m_Pending.Count(); }

	bool MatchesBoundDestination(RICON_RelayDestination destination)
	{
		return !m_bClosed && m_Grant && destination && m_Grant.m_Destination.Same(destination);
	}

	//! Закрепляет курсор вывода и запрещает сырой Take забирать уже принятые кадры.
	bool ClaimRawCursor()
	{
		if (m_bClosed || !m_Grant || m_Resampler) return false;
		m_Resampler = new RICON_PcmResampler8To48();
		return true;
	}

	//! First admitted arrival anchors the local clock; V2 takes its sequence origin from the grant.
	bool ReadPresentationAnchor(int nowMs, out int sequence, out int arrivalMs)
	{
		sequence = -1;
		arrivalMs = -1;
		if (!HasPresentationPermit(m_Permit) || nowMs < 0 || nowMs < m_iLastAcceptedAtMs) return false;
		if (m_iAnchorSequence < 0) return false;
		sequence = m_iAnchorSequence;
		arrivalMs = m_iAnchorArrivalMs;
		return true;
	}

	bool ConfigurePresentationTimeline(RICON_SharedRxPresentationTimeline timeline, int targetMs)
	{
		return HasPresentationPermit(m_Permit) && timeline && timeline.ConfigureReceiver(m_Grant.m_iEpoch, m_Grant.m_Destination, targetMs, 0);
	}

	bool IsPresentationActive() { return HasPresentationPermit(m_Permit); }

	//! The attached scheduler needs an untouched output cursor on this grant.
	bool CanStartLocalPlayout()
	{
		return IsPresentationActive() && !m_Resampler && m_iPreparedSequenceFloor < 0;
	}

	RICON_RelayWireFrame Take(int nowMs)
	{
		if (m_bClosed || m_Resampler || nowMs < 0 || nowMs < m_iLastAcceptedAtMs) return null;
		m_iLastAcceptedAtMs = nowMs;
		DiscardExpired(nowMs);
		if (m_Pending.IsEmpty()) return null;
		ref RICON_RelayWireFrame frame = m_Pending[0];
		m_Pending.RemoveOrdered(0);
		return frame;
	}

	//! The owner supplies a target on the local RX clock. Samples are private until exact commit.
	string PreparePresentation(int targetMs, int nowMs)
	{
		return PreparePresentationSlot(targetMs, nowMs, -1);
	}

	//! The configured scheduler supplies the sequence mapped to this exact local RX slot.
	string StagePresentation(RICON_SharedRxPresentationTimeline timeline, int sequence, int targetMs, int nowMs, float referenceDbm)
	{
		if (!m_Grant) return "CLOSED";
		if (!timeline || !timeline.MatchesReceiver(m_Grant.m_iEpoch, m_Grant.m_Destination)) return "WRONG_RX";
		if (sequence < m_Grant.m_iFirstSequence || sequence >= int.MAX) return "INVALID";
		string timeStatus = timeline.CheckOfferTime(targetMs, nowMs);
		if (timeStatus != "OK") return timeStatus;
		string status = PreparePresentationSlot(targetMs, nowMs, sequence);
		if (status != "PREPARED") return status;
		RICON_RelayPresentationWindow window = ReleasePrepared(nowMs, false);
		if (!window) return "REVOKED";
		status = timeline.OfferRelay(window, referenceDbm, nowMs);
		if (status != "ACCEPTED")
		{
			m_CommittedPresentation = null;
			m_iPresentedDeadlineMs = -1;
		}
		return status;
	}

	//! The scheduler is about to finalize this local slot and has already retired its predecessors.
	//! This cannot retire the named slot itself or change the installed stream generation.
	bool AdvanceBeforeSlot(RICON_SharedRxPresentationTimeline timeline, int sequence, int targetMs, int nowMs)
	{
		if (!HasPresentationPermit(m_Permit) || !timeline || !timeline.MatchesReceiver(m_Grant.m_iEpoch, m_Grant.m_Destination)
			|| sequence < m_Grant.m_iFirstSequence || sequence <= m_iPreparedSequenceFloor || sequence >= int.MAX
			|| nowMs < m_iLastAcceptedAtMs
			|| !timeline.IsNextTarget(targetMs) || timeline.CheckOfferTime(targetMs, nowMs) != "OK") return false;
		m_iLastAcceptedAtMs = nowMs;
		DiscardPreparedIfLate(nowMs);
		if (m_Prepared && m_Prepared.m_Emission.m_iSequence < sequence)
		{
			m_Prepared = null;
			m_iPresentedDeadlineMs = -1;
		}
		if (m_CommittedPresentation && m_CommittedPresentation.m_Emission.m_iSequence < sequence)
		{
			m_CommittedPresentation = null;
			m_iPresentedDeadlineMs = -1;
		}
		RetirePresentationThrough(sequence - 1);
		for (int i = m_Pending.Count() - 1; i >= 0; i--)
		{
			if (m_Pending[i].m_iSequence <= m_iPreparedSequenceFloor) m_Pending.RemoveOrdered(i);
		}
		return true;
	}

	protected string PreparePresentationSlot(int targetMs, int nowMs, int sequence)
	{
		if (!HasPresentationPermit(m_Permit)) return "CLOSED";
		if (nowMs < 0 || nowMs < m_iLastAcceptedAtMs || targetMs < nowMs
			|| targetMs > int.MAX - RICON_MediaPcmProfile.FRAME_DURATION_MS) return "BAD_TIME";
		if (m_CommittedPresentation && nowMs > m_iPresentedTargetMs) m_CommittedPresentation = null;
		m_iLastAcceptedAtMs = nowMs;
		DiscardPreparedIfLate(nowMs);
		if (m_Prepared) return "FULL";
		int durationMs = RICON_MediaPcmProfile.FRAME_DURATION_MS;
		if (m_iPresentedSequence >= 0 && targetMs < m_iPresentedTargetMs + durationMs) return "BEFORE_TARGET";
		DiscardExpired(nowMs);
		int first = FindPresentationFrame(targetMs, sequence);
		if (first < 0)
		{
			RetirePresentationThrough(sequence);
			return "EMPTY";
		}
		if (!m_Resampler) m_Resampler = new RICON_PcmResampler8To48();
		ref RICON_RelayWireFrame frame = m_Pending[first];
		m_Pending.RemoveOrdered(first);
		RetirePresentationThrough(frame.m_iSequence);
		bool consecutive = m_iPresentedSequence >= 0 && frame.m_iSequence == m_iPresentedSequence + 1
			&& frame.m_iEmitFromMs == m_iPresentedEmitEnd && targetMs == m_iPresentedTargetMs + durationMs
			&& targetMs + durationMs <= m_iPresentedDeadlineMs;
		if (!consecutive && !m_Resampler.Reset(frame.m_iFirstSampleIndex)) return "INVALID";
		array<int> decoded = RICON_MediaPcmProfile.DecodeBytes(frame.m_aBytes);
		int outputIndex;
		array<int> samples = m_Resampler.Process(frame.m_iFirstSampleIndex, decoded, outputIndex);
		if (!samples) return "INVALID";
		RICON_RelayPresentationWindow window = new RICON_RelayPresentationWindow();
		window.m_Grant = m_Grant.CloneGrant();
		window.m_Emission = frame;
		window.m_aSamples = samples;
		window.m_iTargetMs = targetMs;
		window.m_iFirstSourceOutputIndex = outputIndex;
		window.m_iDeadlineMs = frame.m_iLocalDeadlineMs;
		if (consecutive) window.m_iDeadlineMs = Math.Min(window.m_iDeadlineMs, m_iPresentedDeadlineMs);
		window.m_Permit = m_Permit;
		m_Prepared = window;
		return "PREPARED";
	}

	protected void RetirePresentationThrough(int sequence)
	{
		if (sequence <= m_iPreparedSequenceFloor) return;
		m_iPreparedSequenceFloor = sequence;
		m_Window.RetireThrough(m_Grant.m_iTransformGeneration, sequence);
	}

	protected int FindPresentationFrame(int targetMs, int sequence)
	{
		int endMs = targetMs + RICON_MediaPcmProfile.FRAME_DURATION_MS;
		int first = -1;
		for (int index = m_Pending.Count() - 1; index >= 0; index--)
		{
			RICON_RelayWireFrame queued = m_Pending[index];
			if (queued.m_iSequence <= m_iPreparedSequenceFloor || (sequence >= 0 && queued.m_iSequence < sequence))
			{
				m_Pending.RemoveOrdered(index);
				if (first >= 0) first--;
			}
			else if ((sequence < 0 || queued.m_iSequence == sequence) && endMs <= queued.m_iLocalDeadlineMs
				&& (first < 0 || queued.m_iSequence < m_Pending[first].m_iSequence))
				first = index;
		}
		return first;
	}

	RICON_RelayPresentationWindow CommitPresentation(int nowMs)
	{
		return ReleasePrepared(nowMs, true);
	}

	protected RICON_RelayPresentationWindow ReleasePrepared(int nowMs, bool exact)
	{
		if (!HasPresentationPermit(m_Permit) || nowMs < 0 || nowMs < m_iLastAcceptedAtMs) return null;
		m_iLastAcceptedAtMs = nowMs;
		DiscardPreparedIfLate(nowMs);
		if (!m_Prepared || (exact && nowMs < m_Prepared.m_iTargetMs)) return null;
		ref RICON_RelayPresentationWindow window = m_Prepared;
		m_Prepared = null;
		m_iPresentedSequence = window.m_Emission.m_iSequence;
		m_iPresentedEmitEnd = window.m_Emission.m_iEmitUntilMs;
		m_iPresentedTargetMs = window.m_iTargetMs;
		m_iPresentedDeadlineMs = window.m_Emission.m_iLocalDeadlineMs;
		m_CommittedPresentation = window;
		m_iCommittedDeadlineMs = window.m_iDeadlineMs;
		return window;
	}

	bool ConsumePresentationWindow(RICON_RelayOutputPermit permit, RICON_RelayPresentationWindow window, int nowMs)
	{
		if (!window || window != m_CommittedPresentation || !HasPresentationPermit(permit, window.m_Grant)
			|| nowMs < m_iLastAcceptedAtMs || window.m_iTargetMs != m_iPresentedTargetMs || window.m_iDeadlineMs != m_iCommittedDeadlineMs) return false;
		m_CommittedPresentation = null;
		return true;
	}

	protected void DiscardPreparedIfLate(int nowMs)
	{
		if (!m_Prepared || (nowMs <= m_Prepared.m_iTargetMs && nowMs < m_Prepared.m_iDeadlineMs)) return;
		m_Prepared = null;
		m_Resampler = new RICON_PcmResampler8To48();
		m_iPresentedDeadlineMs = -1;
	}

	protected void DiscardExpired(int nowMs)
	{
		while (!m_Pending.IsEmpty() && nowMs >= m_Pending[0].m_iLocalDeadlineMs) m_Pending.RemoveOrdered(0);
	}

	void Close()
	{
		m_bClosed = true;
		m_Grant = null;
		m_Permit = null;
		m_Controller = null;
		m_Window.Close();
		m_Pending.Clear();
		m_Prepared = null;
		m_CommittedPresentation = null;
		m_Resampler = null;
	}
}
