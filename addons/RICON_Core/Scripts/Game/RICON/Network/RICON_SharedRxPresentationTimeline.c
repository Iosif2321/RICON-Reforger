class RICON_SharedRxPresentationEntry
{
	ref RICON_SharedRxContribution m_Input;
	int m_iSourceOutputIndex, m_iWireExpiryMs;
}

class RICON_SharedRxPresentationWindow
{
	ref array<ref RICON_SharedRxPresentationEntry> m_Entries = {};
}

//! TEST_ONLY. Trusted playout gives an RX-local target and a prepared PCM window.
class RICON_SharedRxPresentationTimeline
{
	static const int WINDOW_MS = RICON_MediaPcmProfile.FRAME_DURATION_MS;
	static const int HORIZON_WINDOWS = 8;
	protected static const int WINDOW_SAMPLES = RICON_SharedRxMixer.WINDOW_SAMPLES;
	protected ref RICON_MediaEgressRoute m_Rx;
	protected ref RICON_RelayDestination m_Destination;
	protected int m_iEpoch;
	protected ref map<int, ref RICON_SharedRxPresentationWindow> m_Windows = new map<int, ref RICON_SharedRxPresentationWindow>();
	protected int m_iNextTargetMs, m_iNextOutputIndex;
	protected int m_iLastNowMs = -1;
	protected bool m_bClosed;

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
		if (!RICON_SharedRxMixer.TryNormalizePower(relay.m_Emission.m_fRxPowerDbm, relay.m_Emission.m_fNoiseDbm,
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

	protected string InsertContribution(int targetMs, int nowMs, int wireExpiryMs, RICON_SharedRxContribution input)
	{
		string timeStatus = CheckOfferTime(targetMs, nowMs);
		if (timeStatus != "OK") return timeStatus;
		int deltaMs = targetMs - m_iNextTargetMs;
		int offset = deltaMs / WINDOW_MS;
		if (!input || input.m_eKind != RICON_ESharedRxContributionKind.PCM)
			return "INVALID";
		if (!input.MatchesReceiver(m_iEpoch, m_Destination)) return "WRONG_RX";
		if (input.m_iFirstSampleIndex < 0 || input.m_iFirstSampleIndex > int.MAX - WINDOW_SAMPLES
			|| !RICON_SharedRxMixer.IsPcmWindow(input.m_aSamples) || !RICON_SharedRxMixer.IsPowerRatio(input.m_fPowerRatio))
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
					return DuplicateStatus(entry, input, wireExpiryMs);
				held = entry;
				break;
			}
			if (!held && window.m_Entries.Count() >= RICON_SharedRxMixer.MAX_INPUTS) return "RX_RESOURCE_LIMIT";
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
		held.m_iSourceOutputIndex = input.m_iFirstSampleIndex;
		held.m_iWireExpiryMs = wireExpiryMs;
		m_iLastNowMs = nowMs;
		return "ACCEPTED";
	}

	RICON_SharedRxMixResult Commit(int targetMs, int nowMs, float noisePowerRatio, array<int> noiseSamples, out int minWireExpiryMs)
	{
		minWireExpiryMs = -1;
		if (m_bClosed || !m_Destination || targetMs != m_iNextTargetMs || nowMs < 0 || nowMs < m_iLastNowMs || nowMs > targetMs)
			return null;
		array<ref RICON_SharedRxContribution> survivors = {};
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
				survivors.Insert(input);
				if (expiry < 0 || expiry > entry.m_iWireExpiryMs) expiry = entry.m_iWireExpiryMs;
			}
		}
		RICON_SharedRxMixResult result = RICON_SharedRxMixer.MixReceiver(m_iEpoch, m_Destination,
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

	//! Missed targets receive no rendered replacement. Leave the first non-past target pending.
	int SkipLate(int nowMs)
	{
		if (m_bClosed || !m_Destination || nowMs < m_iLastNowMs || nowMs <= m_iNextTargetMs) return 0;
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

	protected static string DuplicateStatus(RICON_SharedRxPresentationEntry held, RICON_SharedRxContribution input, int wireExpiryMs)
	{
		RICON_SharedRxContribution existing = held.m_Input;
		if (!existing.SameEmission(input) || existing.m_Permit != input.m_Permit || existing.m_fPowerRatio != input.m_fPowerRatio
			|| held.m_iSourceOutputIndex != input.m_iFirstSampleIndex || held.m_iWireExpiryMs != wireExpiryMs) return "CONFLICT";
		for (int index = 0; index < WINDOW_SAMPLES; index++)
		{
			if (existing.m_aSamples[index] != input.m_aSamples[index]) return "CONFLICT";
		}
		return "DUPLICATE";
	}
}
