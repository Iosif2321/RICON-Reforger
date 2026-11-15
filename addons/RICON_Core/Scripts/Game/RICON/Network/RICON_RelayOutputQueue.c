//! Trusted local planner hands over pre-rendered PCM and its original permits. One exact timed take.
class RICON_RelayOutputQueue
{
	protected ref RICON_RelayDestination m_Destination;
	protected ref array<ref RICON_SharedRxMixResult> m_Windows = {};
	protected int m_iEpoch, m_iCapacity, m_iLastNowMs;
	protected int m_iLastTargetMs = -1, m_iLastSampleIndex = -1;
	protected bool m_bClosed;

	bool Configure(int epoch, RICON_RelayDestination destination, int capacity, int nowMs)
	{
		if (m_bClosed || m_Destination || epoch <= 0 || epoch >= int.MAX || !destination || !destination.IsValid()
			|| capacity < 1 || capacity > 16 || nowMs < 0) return false;
		m_iEpoch = epoch;
		m_Destination = destination.CloneDestination();
		m_iCapacity = capacity;
		m_iLastNowMs = nowMs;
		return true;
	}

	string Offer(RICON_SharedRxMixResult window, int nowMs)
	{
		if (m_bClosed || !m_Destination) return "CLOSED";
		if (nowMs < m_iLastNowMs) return "BAD_TIME";
		if (!window || window.m_iEpoch != m_iEpoch || !m_Destination.Same(window.m_Destination)) return "WRONG_RX";
		int width = RICON_SharedRxMixer.WINDOW_SAMPLES;
		int duration = RICON_SharedRxPresentationTimeline.WINDOW_MS;
		if (window.m_iTargetMs < 0 || window.m_iTargetMs > int.MAX - duration
			|| window.m_iFirstSampleIndex < 0 || window.m_iFirstSampleIndex > int.MAX - width
			|| window.m_iFirstSampleIndex % width != 0 || !RICON_SharedRxMixer.IsPcmWindow(window.m_aSamples)
			|| !window.m_aPermits || window.m_aPermits.Count() > RICON_SharedRxMixer.MAX_INPUTS) return "INVALID";
		if (window.m_iTargetMs <= m_iLastTargetMs || window.m_iFirstSampleIndex <= m_iLastSampleIndex) return "RETIRED";
		if (m_iLastTargetMs >= 0)
		{
			int targetDelta = window.m_iTargetMs - m_iLastTargetMs;
			if (targetDelta % duration != 0 || targetDelta / duration != (window.m_iFirstSampleIndex - m_iLastSampleIndex) / width)
				return "OFF_GRID";
		}
		m_iLastNowMs = nowMs;
		Sweep(nowMs);
		if (m_bClosed) return "CLOSED";
		if (nowMs > window.m_iTargetMs) return "LATE";
		if (window.m_aPermits.IsEmpty())
		{
			if (window.m_iWireDeadlineMs != -1) return "INVALID";
			foreach (int sample : window.m_aSamples)
			{
				if (sample != 0) return "INVALID";
			}
		}
		else
		{
			if (window.m_iWireDeadlineMs < window.m_iTargetMs + duration) return "EXPIRED";
			if (!PermitsCurrent(window)) return "REVOKED";
			if (m_bClosed) return "CLOSED";
		}
		if (m_Windows.Count() >= m_iCapacity) return "FULL";

		RICON_SharedRxMixResult queued = new RICON_SharedRxMixResult();
		queued.m_iEpoch = m_iEpoch;
		queued.m_Destination = m_Destination.CloneDestination();
		queued.m_iTargetMs = window.m_iTargetMs;
		queued.m_iFirstSampleIndex = window.m_iFirstSampleIndex;
		queued.m_iWireDeadlineMs = window.m_iWireDeadlineMs;
		if (window.m_Rx) queued.m_Rx = window.m_Rx.CloneRoute();
		queued.m_fTotalPowerRatio = window.m_fTotalPowerRatio;
		queued.m_fPcmPowerRatio = window.m_fPcmPowerRatio;
		queued.m_fEnergyOnlyPowerRatio = window.m_fEnergyOnlyPowerRatio;
		queued.m_fMissingPcmPowerRatio = window.m_fMissingPcmPowerRatio;
		queued.m_fNoisePowerRatio = window.m_fNoisePowerRatio;
		queued.m_aSamples.InsertAll(window.m_aSamples);
		foreach (RICON_MediaOutputPermit permit : window.m_aPermits) queued.m_aPermits.Insert(permit);
		m_Windows.Insert(queued);
		m_iLastTargetMs = queued.m_iTargetMs;
		m_iLastSampleIndex = queued.m_iFirstSampleIndex;
		return "ACCEPTED";
	}

	RICON_SharedRxMixResult Take(int nowMs)
	{
		if (m_bClosed || !m_Destination || nowMs < m_iLastNowMs) return null;
		m_iLastNowMs = nowMs;
		Sweep(nowMs);
		if (m_bClosed || m_Windows.IsEmpty() || m_Windows[0].m_iTargetMs != nowMs) return null;
		ref RICON_SharedRxMixResult window = m_Windows[0];
		m_Windows.RemoveOrdered(0);
		return window;
	}

	protected void Sweep(int nowMs)
	{
		for (int i = m_Windows.Count() - 1; i >= 0; i--)
		{
			ref RICON_SharedRxMixResult window = m_Windows[i];
			bool keep = nowMs <= window.m_iTargetMs && PermitsCurrent(window);
			if (m_bClosed) return;
			if (!keep) m_Windows.RemoveOrdered(i);
		}
	}

	protected static bool PermitsCurrent(RICON_SharedRxMixResult window)
	{
		foreach (RICON_MediaOutputPermit permit : window.m_aPermits)
		{
			if (!permit || !permit.IsCurrent()) return false;
		}
		return true;
	}

	void Close()
	{
		m_bClosed = true;
		m_Windows.Clear();
		m_Destination = null;
	}
}
