class RICON_SharedRxPresentationEntry
{
	ref RICON_SharedRxContribution m_Input;
	ref RICON_MediaCaptureEnvelope m_DirectCapture;
	ref RICON_MediaRfHistoryEntry m_DirectRfHistory;
	float m_fReferenceDbm;
	int m_iSourceOutputIndex, m_iWireExpiryMs;
}

class RICON_SharedRxPresentationWindow
{
	ref array<ref RICON_SharedRxPresentationEntry> m_Entries = {};
}

//! TEST_ONLY. Trusted playout gives an RX-local target and a prepared PCM window.
class RICON_SharedRxPresentationTimeline
{
	static const int WINDOW_MS = RICON_MediaPcmTestProfile.FRAME_DURATION_MS;
	static const int HORIZON_WINDOWS = 8;
	protected static const int WINDOW_SAMPLES = RICON_SharedRxTestMixer.WINDOW_SAMPLES;
	protected ref RICON_MediaEgressRoute m_Rx;
	protected ref RICON_RelayDestination m_Destination;
	protected int m_iEpoch;
	protected ref map<int, ref RICON_SharedRxPresentationWindow> m_Windows = new map<int, ref RICON_SharedRxPresentationWindow>();
	protected int m_iNextTargetMs, m_iNextOutputIndex;
	protected int m_iLastNowMs = -1;
	protected bool m_bClosed;
	protected bool m_bCommittingNoise;

	bool Configure(RICON_MediaEgressRoute rx, int anchorTargetMs, int anchorOutputIndex)
	{
		if (!rx || !rx.IsValid() || !ConfigureReceiver(rx.m_Source.m_iSessionEpoch, RICON_RelayDestination.FromRoute(rx),
			anchorTargetMs, anchorOutputIndex)) return false;
		m_Rx = rx.CloneRoute();
		return true;
	}

	bool ConfigureReceiver(int epoch, RICON_RelayDestination destination, int anchorTargetMs, int anchorOutputIndex)
	{
		if (m_bClosed || m_Destination || !destination || !destination.IsValid() || epoch <= 0 || epoch >= int.MAX) return false;
		if (anchorTargetMs < 0 || anchorTargetMs > int.MAX - WINDOW_MS) return false;
		if (anchorOutputIndex < 0 || anchorOutputIndex > int.MAX - WINDOW_SAMPLES) return false;
		m_Destination = destination.CloneDestination();
		m_iEpoch = epoch;
		m_iNextTargetMs = anchorTargetMs;
		m_iNextOutputIndex = anchorOutputIndex;
		return true;
	}

	bool MatchesReceiver(int epoch, RICON_RelayDestination destination)
	{
		return !m_bClosed && m_Destination && m_iEpoch == epoch && m_Destination.Same(destination);
	}

	bool IsNextTarget(int targetMs) { return !m_bClosed && m_Destination && targetMs == m_iNextTargetMs; }

	string CheckOfferTime(int targetMs, int nowMs)
	{
		if (m_bClosed || !m_Destination) return "CLOSED";
		if (m_bCommittingNoise) return "BUSY";
		if (nowMs < 0 || nowMs < m_iLastNowMs) return "BAD_TIME";
		if (targetMs < m_iNextTargetMs || nowMs > targetMs) return "LATE";
		int deltaMs = targetMs - m_iNextTargetMs;
		if (deltaMs % WINDOW_MS != 0) return "OFF_GRID";
		int offset = deltaMs / WINDOW_MS;
		if (offset >= HORIZON_WINDOWS || offset >= RemainingWindows()) return "OUTSIDE_HORIZON";
		return "OK";
	}

	string OfferRelay(RICON_RelayPresentationWindow relay, float referenceDbm, int nowMs)
	{
		if (!relay || !relay.m_Emission || !relay.m_Permit || !relay.m_Grant || nowMs > relay.m_iTargetMs) return "INVALID";
		// Неверно выбранный RX или слот не должен расходовать одноразовое окно другого RX.
		if (!MatchesReceiver(relay.m_Grant.m_iEpoch, relay.m_Grant.m_Destination)) return "WRONG_RX";
		string timeStatus = CheckOfferTime(relay.m_iTargetMs, nowMs);
		if (timeStatus != "OK") return timeStatus;
		if (relay.m_iDeadlineMs <= relay.m_iTargetMs || relay.m_iDeadlineMs - relay.m_iTargetMs < WINDOW_MS) return "EXPIRED";
		float signalRatio, noiseRatio;
		if (!RICON_SharedRxTestMixer.TryNormalizePower(relay.m_Emission.m_fRxPowerDbm, relay.m_Emission.m_fNoiseDbm,
			referenceDbm, signalRatio, noiseRatio)) return "INVALID_RF";
		if (!relay.m_Permit.ConsumeWindow(relay, nowMs)) return "REVOKED";
		RICON_SharedRxContribution input = new RICON_SharedRxContribution();
		input.m_eKind = RICON_ESharedRxContributionKind.PCM;
		input.m_RelayGrant = relay.m_Grant;
		input.m_Permit = relay.m_Permit;
		input.m_aSamples = relay.m_aSamples;
		input.m_iFirstSampleIndex = relay.m_iFirstSourceOutputIndex;
		input.m_fPowerRatio = signalRatio;
		return InsertContribution(relay.m_iTargetMs, nowMs, relay.m_iDeadlineMs, input);
	}

	string Offer(int targetMs, int nowMs, int wireExpiryMs, RICON_SharedRxContribution input)
	{
		if (input && input.m_RelayGrant) return "INVALID";
		return InsertContribution(targetMs, nowMs, wireExpiryMs, input);
	}

	string OfferDirect(int targetMs, int nowMs, int wireExpiryMs, RICON_SharedRxContribution input,
		RICON_MediaCaptureEnvelope capture, float referenceDbm, RICON_MediaRfHistoryEntry admittedHistory)
	{
		if (!admittedHistory || !admittedHistory.m_Budget || admittedHistory.m_Budget.m_RxEnvironment) return "INVALID";
		if (!capture || !input || input.m_RelayGrant || !input.m_Route || !input.m_Route.m_Source
			|| !input.m_Route.m_Source.Same(capture.m_Binding) || !RICON_Num.IsFinite(referenceDbm)) return "INVALID";
		ref RICON_MediaCaptureEnvelope heldCapture = capture.CloneEnvelope();
		ref RICON_MediaRfHistoryEntry heldHistory = admittedHistory.CloneEntry();
		return InsertContribution(targetMs, nowMs, wireExpiryMs, input, heldCapture, referenceDbm, heldHistory);
	}

	RICON_MediaCaptureEnvelope ReadDirectCapture(int targetMs, RICON_MediaEgressRoute route, float referenceDbm,
		out RICON_MediaRfHistoryEntry admittedHistory)
	{
		admittedHistory = null;
		if (!IsNextTarget(targetMs) || !route) return null;
		RICON_SharedRxPresentationWindow window = m_Windows.Get(targetMs);
		if (!window) return null;
		foreach (RICON_SharedRxPresentationEntry entry : window.m_Entries)
		{
			if (entry.m_DirectCapture && entry.m_DirectRfHistory && entry.m_fReferenceDbm == referenceDbm
				&& route.Same(entry.m_Input.m_Route))
			{
				admittedHistory = entry.m_DirectRfHistory.CloneEntry();
				return entry.m_DirectCapture.CloneEnvelope();
			}
		}
		return null;
	}

	protected string InsertContribution(int targetMs, int nowMs, int wireExpiryMs, RICON_SharedRxContribution input,
		RICON_MediaCaptureEnvelope capture = null, float referenceDbm = 0.0, RICON_MediaRfHistoryEntry admittedHistory = null)
	{
		string timeStatus = CheckOfferTime(targetMs, nowMs);
		if (timeStatus != "OK") return timeStatus;
		int deltaMs = targetMs - m_iNextTargetMs;
		int offset = deltaMs / WINDOW_MS;
		if (!input || input.m_eKind != RICON_ESharedRxContributionKind.PCM)
			return "INVALID";
		if (!input.MatchesReceiver(m_iEpoch, m_Destination)) return "WRONG_RX";
		if (input.m_iFirstSampleIndex < 0 || input.m_iFirstSampleIndex > int.MAX - WINDOW_SAMPLES
			|| !RICON_SharedRxTestMixer.IsPcmWindow(input.m_aSamples) || !RICON_SharedRxTestMixer.IsPowerRatio(input.m_fPowerRatio))
			return "INVALID";
		if (!input.HasCurrentPermit()) return "REVOKED";
		if (m_bClosed) return "CLOSED";
		if (!CoversDeadline(input, targetMs, wireExpiryMs)) return "EXPIRED";

		ref RICON_SharedRxPresentationWindow window = m_Windows.Get(targetMs);
		RICON_SharedRxPresentationEntry held;
		if (window)
		{
			foreach (RICON_SharedRxPresentationEntry entry : window.m_Entries)
			{
				RICON_SharedRxContribution existing = entry.m_Input;
				if (input.TxDeviceId() != existing.TxDeviceId() || input.TxIndex() != existing.TxIndex())
					continue;
				bool current = CoversDeadline(existing, targetMs, entry.m_iWireExpiryMs) && existing.HasCurrentPermit();
				if (m_bClosed) return "CLOSED";
				if (current)
					return DuplicateStatus(entry, input, wireExpiryMs, capture, referenceDbm, admittedHistory);
				held = entry;
				break;
			}
			if (!held && window.m_Entries.Count() >= RICON_SharedRxTestMixer.MAX_INPUTS) return "RX_RESOURCE_LIMIT";
		}
		else
		{
			window = new RICON_SharedRxPresentationWindow();
			m_Windows.Set(targetMs, window);
		}

		RICON_SharedRxContribution detached = new RICON_SharedRxContribution();
		detached.m_eKind = input.m_eKind;
		if (input.m_Route) detached.m_Route = input.m_Route.CloneRoute();
		if (input.m_RelayGrant) detached.m_RelayGrant = input.m_RelayGrant.CloneGrant();
		detached.m_iFirstSampleIndex = m_iNextOutputIndex + offset * WINDOW_SAMPLES;
		detached.m_fPowerRatio = input.m_fPowerRatio;
		detached.m_aSamples = {};
		detached.m_aSamples.InsertAll(input.m_aSamples);
		detached.m_Permit = input.m_Permit;
		if (!held)
		{
			held = new RICON_SharedRxPresentationEntry();
			window.m_Entries.Insert(held);
		}
		held.m_Input = detached;
		held.m_DirectCapture = capture;
		held.m_DirectRfHistory = admittedHistory;
		held.m_fReferenceDbm = referenceDbm;
		held.m_iSourceOutputIndex = input.m_iFirstSampleIndex;
		held.m_iWireExpiryMs = wireExpiryMs;
		m_iLastNowMs = nowMs;
		return "ACCEPTED";
	}

	RICON_SharedRxMixResult Commit(int targetMs, int nowMs, float noisePowerRatio, array<int> noiseSamples, out int minWireExpiryMs)
	{
		minWireExpiryMs = -1;
		if (m_bCommittingNoise) return null;
		return CommitWindow(targetMs, nowMs, noisePowerRatio, noiseSamples, minWireExpiryMs);
	}

	RICON_SharedRxMixResult CommitQualifiedNoise(int targetMs, int nowMs, float noisePowerRatio, array<int> noiseSamples,
		RICON_TemporalRxEnvironment environment, RICON_MediaEgressRoute noiseRoute, float referenceDbm, out int minWireExpiryMs)
	{
		minWireExpiryMs = -1;
		if (m_bCommittingNoise || !environment || !noiseRoute || !RICON_SharedRxTestMixer.IsPcmWindow(noiseSamples)) return null;
		m_bCommittingNoise = true;
		ref RICON_SharedRxMixResult result = CommitWindow(targetMs, nowMs, noisePowerRatio, noiseSamples,
			minWireExpiryMs, environment, noiseRoute, referenceDbm);
		m_bCommittingNoise = false;
		return result;
	}

	protected RICON_SharedRxMixResult CommitWindow(int targetMs, int nowMs, float noisePowerRatio, array<int> noiseSamples,
		out int minWireExpiryMs, RICON_TemporalRxEnvironment environment = null, RICON_MediaEgressRoute noiseRoute = null,
		float referenceDbm = 0.0)
	{
		minWireExpiryMs = -1;
		if (m_bClosed || !m_Destination || targetMs != m_iNextTargetMs || nowMs < 0 || nowMs < m_iLastNowMs || nowMs > targetMs)
			return null;
		array<ref RICON_SharedRxContribution> survivors = {};
		bool hasNoiseRoute = environment == null;
		int expiry = -1;
		ref RICON_SharedRxPresentationWindow window = m_Windows.Get(targetMs);
		if (window)
		{
			foreach (RICON_SharedRxPresentationEntry entry : window.m_Entries)
			{
				RICON_SharedRxContribution input = entry.m_Input;
				bool current = CoversDeadline(input, targetMs, entry.m_iWireExpiryMs) && input.HasCurrentPermit();
				if (m_bClosed) return null;
				if (!current) continue;
				if (environment && !MatchesNoiseWindow(entry, environment, referenceDbm)) return null;
				if (!hasNoiseRoute && noiseRoute.Same(input.m_Route)) hasNoiseRoute = true;
				survivors.Insert(input);
				if (expiry < 0 || expiry > entry.m_iWireExpiryMs) expiry = entry.m_iWireExpiryMs;
			}
		}
		if (!hasNoiseRoute) return null;
		RICON_SharedRxMixResult result = RICON_SharedRxTestMixer.MixReceiver(m_iEpoch, m_Destination,
			m_iNextOutputIndex, survivors, noisePowerRatio, noiseSamples, m_Rx);
		if (m_bClosed || !result) return null;
		minWireExpiryMs = expiry;
		result.m_iWireDeadlineMs = expiry;
		result.m_iTargetMs = targetMs;
		m_Windows.Remove(targetMs);
		m_iLastNowMs = nowMs;
		Advance(1);
		return result;
	}

	protected static bool MatchesNoiseWindow(RICON_SharedRxPresentationEntry entry,
		RICON_TemporalRxEnvironment environment, float referenceDbm)
	{
		RICON_MediaCaptureEnvelope capture = entry.m_DirectCapture;
		if (!capture || capture.m_sTimebaseId != environment.m_sTimebaseId || entry.m_fReferenceDbm != referenceDbm) return false;
		return RICON_TemporalClock.ContainsInterval(capture.m_iLowerMs, capture.m_iLowerSubUs,
			capture.m_iUpperMs, capture.m_iUpperSubUs, environment.m_iFromMs, environment.m_iUntilMs);
	}

	//! Missed targets receive no rendered replacement. Leave the first non-past target pending.
	int SkipLate(int nowMs)
	{
		if (m_bClosed || m_bCommittingNoise || !m_Destination || nowMs < m_iLastNowMs || nowMs <= m_iNextTargetMs) return 0;
		int skipped = Math.Min((nowMs - m_iNextTargetMs - 1) / WINDOW_MS + 1, RemainingWindows());
		m_iLastNowMs = nowMs;
		Advance(skipped);
		if (!m_bClosed)
		{
			array<int> stale = {};
			foreach (int target, RICON_SharedRxPresentationWindow window : m_Windows)
			{
				if (target < m_iNextTargetMs) stale.Insert(target);
			}
			foreach (int target : stale) m_Windows.Remove(target);
		}
		return skipped;
	}

	void Close()
	{
		m_bClosed = true;
		m_Rx = null;
		m_Destination = null;
		m_Windows.Clear();
	}

	protected static bool CoversDeadline(RICON_SharedRxContribution input, int targetMs, int expiryMs)
	{
		if (input.m_RelayGrant) return expiryMs > targetMs && expiryMs - targetMs >= WINDOW_MS;
		return expiryMs > targetMs;
	}

	protected int RemainingWindows()
	{
		return Math.Min((int.MAX - m_iNextTargetMs) / WINDOW_MS, (int.MAX - m_iNextOutputIndex) / WINDOW_SAMPLES);
	}

	protected void Advance(int windows)
	{
		if (windows >= RemainingWindows())
		{
			Close();
			return;
		}
		m_iNextTargetMs += windows * WINDOW_MS;
		m_iNextOutputIndex += windows * WINDOW_SAMPLES;
	}

	//! Compare only the RF metadata represented by V3. The receiver environment is qualified separately.
	static bool MatchesV3History(RICON_MediaRfHistoryEntry admitted, RICON_MediaRfHistoryEntry selected)
	{
		if (!admitted || !admitted.m_Budget || !selected || !selected.m_Budget) return false;
		if (admitted.m_iValidFromMs != selected.m_iValidFromMs || admitted.m_iValidUntilMs != selected.m_iValidUntilMs) return false;
		RICON_TemporalRfBudget wire = admitted.m_Budget;
		RICON_TemporalRfBudget source = selected.m_Budget;
		if (wire.m_sTimebaseId != source.m_sTimebaseId || wire.m_fBandwidthHz != source.m_fBandwidthHz) return false;
		if (wire.m_iLinkId != source.m_iLinkId || wire.m_iTickMs != source.m_iTickMs
			|| wire.m_iModelMs != source.m_iModelMs || wire.m_iModelSubUs != source.m_iModelSubUs) return false;
		if (!RICON_MediaWireRfV2.ValidPowerDbm(wire.m_fRxPowerDbm) || !RICON_MediaWireRfV2.ValidPowerDbm(source.m_fRxPowerDbm)
			|| !RICON_MediaWireRfV2.ValidPowerDbm(wire.m_fNoiseDbm) || !RICON_MediaWireRfV2.ValidPowerDbm(source.m_fNoiseDbm)) return false;
		return RICON_MediaWireRfV2.QuantizePower(wire.m_fRxPowerDbm) == RICON_MediaWireRfV2.QuantizePower(source.m_fRxPowerDbm)
			&& RICON_MediaWireRfV2.QuantizePower(wire.m_fNoiseDbm) == RICON_MediaWireRfV2.QuantizePower(source.m_fNoiseDbm);
	}

	protected static string DuplicateStatus(RICON_SharedRxPresentationEntry held, RICON_SharedRxContribution input,
		int wireExpiryMs, RICON_MediaCaptureEnvelope capture, float referenceDbm, RICON_MediaRfHistoryEntry admittedHistory)
	{
		RICON_SharedRxContribution existing = held.m_Input;
		if (!existing.SameEmission(input) || existing.m_Permit != input.m_Permit || existing.m_fPowerRatio != input.m_fPowerRatio
			|| held.m_iSourceOutputIndex != input.m_iFirstSampleIndex || held.m_iWireExpiryMs != wireExpiryMs) return "CONFLICT";
		if (capture)
		{
			if (!held.m_DirectCapture || !held.m_DirectCapture.SameEnvelope(capture) || held.m_fReferenceDbm != referenceDbm
				|| !held.m_DirectRfHistory || !held.m_DirectRfHistory.SameEntry(admittedHistory)) return "CONFLICT";
		}
		else if (held.m_DirectCapture) return "CONFLICT";
		for (int index = 0; index < WINDOW_SAMPLES; index++)
		{
			if (existing.m_aSamples[index] != input.m_aSamples[index]) return "CONFLICT";
		}
		return "DUPLICATE";
	}
}
