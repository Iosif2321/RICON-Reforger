//! One receiver and one local clock. The caller owns the output queue and rechecks all live permits before using a window.
class RICON_RelayLocalPlayout
{
	protected ref RICON_RelayWireReceiver m_Receiver;
	protected ref RICON_SharedRxPresentationTimeline m_Timeline;
	protected int m_iDelayMs, m_iPrepareLeadMs, m_iMaxPerPump;
	protected int m_iNextSequence, m_iNextTargetMs, m_iLastNowMs = -1;
	protected bool m_bClosed;

	bool Configure(RICON_RelayWireReceiver receiver, int delayMs, int prepareLeadMs, int maxPerPump)
	{
		if (m_bClosed || m_Receiver || !receiver || !receiver.CanStartLocalPlayout() || delayMs < 4 || delayMs > 100
			|| prepareLeadMs < 0 || prepareLeadMs > delayMs || maxPerPump < 1 || maxPerPump > 16) return false;
		m_Receiver = receiver;
		m_iDelayMs = delayMs;
		m_iPrepareLeadMs = prepareLeadMs;
		m_iMaxPerPump = maxPerPump;
		return true;
	}

	//! Returns windows in local playback order. A gap is a window with no permit and no wire deadline.
	string Pump(int nowMs, float referenceDbm, array<ref RICON_SharedRxMixResult> prepared, out int skipped)
	{
		skipped = 0;
		if (!prepared) return "INVALID";
		prepared.Clear();
		if (m_bClosed || !m_Receiver) return "CLOSED";
		if (nowMs < 0 || nowMs < m_iLastNowMs || nowMs > int.MAX - 128) return "BAD_TIME";
		if (!RICON_Num.IsFinite(referenceDbm)) return "INVALID_RF";
		m_iLastNowMs = nowMs;
		if (!m_Receiver.IsPresentationActive()) return Retire("REVOKED");
		if (!m_Timeline)
		{
			string anchorStatus = Anchor(nowMs);
			if (anchorStatus != "OK") return anchorStatus;
		}

		int windowMs = RICON_SharedRxPresentationTimeline.WINDOW_MS;
		skipped = m_Timeline.SkipLate(nowMs);
		if (skipped > 0)
		{
			if (skipped >= int.MAX - m_iNextSequence) return Retire("END");
			m_iNextSequence += skipped;
			m_iNextTargetMs += skipped * windowMs;
			if (!m_Receiver.AdvanceBeforeSlot(m_Timeline, m_iNextSequence, m_iNextTargetMs, nowMs)) return Retire("INVALID_CURSOR");
		}
		while (prepared.Count() < m_iMaxPerPump && m_iNextTargetMs <= nowMs + m_iPrepareLeadMs)
		{
			if (m_iNextSequence >= int.MAX || m_iNextTargetMs > int.MAX - windowMs)
			{
				if (!prepared.IsEmpty()) return "OK";
				return Retire("END");
			}
			string status = m_Receiver.StagePresentation(m_Timeline, m_iNextSequence, m_iNextTargetMs, nowMs, referenceDbm);
			if (status != "ACCEPTED" && status != "EMPTY")
			{
				prepared.Clear();
				return Retire(status);
			}
			int expiry;
			RICON_SharedRxMixResult mix = m_Timeline.Commit(m_iNextTargetMs, nowMs, 0, null, expiry);
			if (!mix)
			{
				prepared.Clear();
				return Retire("INVALID_CURSOR");
			}
			prepared.Insert(mix);
			m_iNextSequence++;
			m_iNextTargetMs += windowMs;
		}
		return "OK";
	}

	protected string Anchor(int nowMs)
	{
		int sequence, arrivalMs;
		if (!m_Receiver.ReadPresentationAnchor(nowMs, sequence, arrivalMs)) return "WAIT";
		int targetMs = arrivalMs + m_iDelayMs;
		RICON_SharedRxPresentationTimeline timeline = new RICON_SharedRxPresentationTimeline();
		if (!m_Receiver.ConfigurePresentationTimeline(timeline, targetMs)) return Retire("INVALID_CURSOR");
		m_Timeline = timeline;
		m_iNextSequence = sequence;
		m_iNextTargetMs = targetMs;
		return "OK";
	}

	protected string Retire(string status)
	{
		Close();
		return status;
	}

	void Close()
	{
		m_bClosed = true;
		if (m_Timeline) m_Timeline.Close();
		m_Timeline = null;
		m_Receiver = null;
	}
}
