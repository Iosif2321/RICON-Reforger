//! One private processed head. Backpressure never repeats source decode, interpolation or RF processing.
class RICON_RelayProcessedInput
{
	protected ref RICON_SharedRxMixResult m_Mix;
	protected ref RICON_RelayPcmOrigin m_Origin;
	protected int m_iLastNowMs = -1;
	protected bool m_bStaging, m_bStageCancelled, m_bCommitting;

	bool HasPending() { return m_Mix != null; }
	bool IsBusy() { return m_bStaging || m_bCommitting; }

	bool Stage(RICON_SharedRxMixResult mix, RICON_RelayPcmOrigin origin, int nowMs)
	{
		if (m_bStaging || m_Mix || !mix || !mix.m_Rx || !mix.m_Rx.IsValid() || !origin || !origin.IsValid(nowMs)) return false;
		if (!RICON_SharedRxTestMixer.IsPcmWindow(mix.m_aSamples) || !mix.m_aPermits || mix.m_aPermits.Count() != 1) return false;
		if (!mix.m_aPermits[0] || origin.EffectiveDeadlineMs() <= nowMs) return false;
		ref RICON_SharedRxMixResult snapshot = new RICON_SharedRxMixResult();
		snapshot.m_Rx = mix.m_Rx.CloneRoute();
		if (mix.m_Destination) snapshot.m_Destination = mix.m_Destination.CloneDestination();
		if (mix.m_RelayInputOrigin) snapshot.m_RelayInputOrigin = mix.m_RelayInputOrigin.CloneOrigin();
		snapshot.m_iEpoch = mix.m_iEpoch;
		snapshot.m_iFirstSampleIndex = mix.m_iFirstSampleIndex;
		snapshot.m_iWireDeadlineMs = mix.m_iWireDeadlineMs;
		snapshot.m_iTargetMs = mix.m_iTargetMs;
		snapshot.m_fTotalPowerRatio = mix.m_fTotalPowerRatio;
		snapshot.m_fPcmPowerRatio = mix.m_fPcmPowerRatio;
		snapshot.m_fEnergyOnlyPowerRatio = mix.m_fEnergyOnlyPowerRatio;
		snapshot.m_fMissingPcmPowerRatio = mix.m_fMissingPcmPowerRatio;
		snapshot.m_fNoisePowerRatio = mix.m_fNoisePowerRatio;
		snapshot.m_aSamples.InsertAll(mix.m_aSamples);
		foreach (RICON_MediaOutputPermit permit : mix.m_aPermits) snapshot.m_aPermits.Insert(permit);
		ref RICON_RelayPcmOrigin frozenOrigin = origin.CloneOrigin();
		m_bStaging = true;
		m_bStageCancelled = false;
		bool allowed = snapshot.m_aPermits[0].MatchesRoute(snapshot.m_Rx);
		m_bStaging = false;
		if (!allowed || m_bStageCancelled || m_Mix) return false;
		m_Mix = snapshot;
		m_Origin = frozenOrigin;
		m_iLastNowMs = nowMs;
		return true;
	}

	string Commit(RICON_RelayPcmHandoff handoff, RICON_RelayPcmPath path, int nowMs)
	{
		if (IsBusy()) return "WAIT";
		m_bCommitting = true;
		string status = CommitOnce(handoff, path, nowMs);
		m_bCommitting = false;
		return status;
	}

	protected string CommitOnce(RICON_RelayPcmHandoff handoff, RICON_RelayPcmPath path, int nowMs)
	{
		if (!m_Mix) return "EMPTY";
		ref RICON_SharedRxMixResult mix = m_Mix;
		ref RICON_RelayPcmOrigin origin = m_Origin;
		if (nowMs < m_iLastNowMs) return "BAD_TIME";
		m_iLastNowMs = nowMs;
		if (!handoff || !path || !handoff.MatchesPath(path) || !path.m_UpstreamRoute.Same(mix.m_Rx)) { Clear(); return "REVOKED"; }
		bool allowed = mix.m_aPermits[0].MatchesRoute(mix.m_Rx);
		if (m_Mix != mix) return "REVOKED";
		if (!allowed) { Clear(); return "REVOKED"; }
		if (nowMs >= origin.EffectiveDeadlineMs()) { Clear(); return "EXPIRED"; }
		string status = handoff.Offer(mix, origin, nowMs);
		if (m_Mix != mix) return "REVOKED";
		if (status == "FULL" || status == "WAIT") return "WAIT";
		if (status != "BAD_TIME") Clear();
		return status;
	}

	void Clear() { if (m_bStaging) m_bStageCancelled = true; m_Mix = null; m_Origin = null; }
}
