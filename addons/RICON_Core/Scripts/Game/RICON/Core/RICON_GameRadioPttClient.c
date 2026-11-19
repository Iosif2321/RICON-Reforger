//! Pure local intent fence. A server reply is metadata, never an automatic audio command.
class RICON_GameRadioPttClient
{
	protected ref RICON_GameRadioPttContext m_Context;
	protected int m_iStartRequest, m_iHoldToken, m_iStartSeq;
	protected bool m_bReleased;

	void Clear()
	{
		m_Context = null; m_iStartRequest = 0; m_iHoldToken = 0; m_iStartSeq = 0; m_bReleased = false;
	}

	void BindContext(RICON_GameRadioPttContext context)
	{
		if (!context || !context.IsValid()) { Clear(); return; }
		if (!m_Context || !m_Context.Same(context))
		{
			m_iStartRequest = 0; m_iHoldToken = 0; m_iStartSeq = 0; m_bReleased = false;
		}
		m_Context = context.CloneContext();
	}

	void Release(int holdToken)
	{
		if (holdToken <= 0) return;
		if (m_iHoldToken != 0 && m_iHoldToken != holdToken) return;
		m_iHoldToken = holdToken;
		m_bReleased = true;
	}

	bool StartIntent(int requestId, int holdToken, int requestSeq)
	{
		if (!m_Context || requestId <= 0 || holdToken <= 0 || requestSeq <= 0) return false;
		if (m_bReleased && holdToken == m_iHoldToken)
		{
			if (m_iStartSeq == 0 || requestSeq <= m_iStartSeq) return false;
		}
		m_iStartRequest = requestId; m_iHoldToken = holdToken; m_iStartSeq = requestSeq; m_bReleased = false;
		return true;
	}

	//! Identity of an acknowledged START held by the local input. No expiry or media permission is inferred here.
	RICON_GameRadioPttReply ReadHeldStart(RICON_GameRadioPttReply supplied)
	{
		if (m_bReleased || !m_Context || !supplied || !supplied.m_bActive) return null;
		if (supplied.m_iKind != RICON_GameRadioPttReply.PTT || supplied.m_iMethod != RICON_ELifecycleMethod.PTT_START) return null;
		if (supplied.m_iRequestId != m_iStartRequest || supplied.m_iHoldToken != m_iHoldToken) return null;
		if (!m_Context.Same(supplied.m_Context) || !supplied.m_bHasLease || supplied.m_iLeaseId <= 0) return null;
		return supplied.CloneReply();
	}

	RICON_GameRadioPttReply Read(RICON_GameRadioPttReply supplied, int nowMs, int sentAtMs)
	{
		if (!supplied) return null;
		if (supplied.m_iKind != RICON_GameRadioPttReply.CONTEXT)
		{
			if (!m_Context || !supplied.m_Context || !m_Context.Same(supplied.m_Context)) return null;
		}
		RICON_GameRadioPttReply reply = supplied.CloneReply();
		if (!reply.m_bActive) return reply;
		bool current = reply.m_iKind == RICON_GameRadioPttReply.PTT && !m_bReleased;
		if (reply.m_iRequestId != m_iStartRequest || reply.m_iHoldToken != m_iHoldToken) current = false;
		if (sentAtMs < 0 || nowMs < sentAtMs || reply.m_iServerNowMs < 0) current = false;
		if (reply.m_iDeadlineMs <= reply.m_iServerNowMs) current = false;
		if (current && nowMs - sentAtMs >= reply.m_iDeadlineMs - reply.m_iServerNowMs) current = false;
		if (!current)
		{
			reply.m_bActive = false; reply.m_eLeaseState = RICON_ETransmitLeaseState.NONE;
			reply.m_eTerminalReason = RICON_ELeaseTerminalReason.NONE;
		}
		return reply;
	}
}
