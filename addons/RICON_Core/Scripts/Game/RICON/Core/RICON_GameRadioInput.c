//! One physical key hold, not a limit on scenario radios. This layer owns no audio.
class RICON_GameRadioInputSelection
{
	int m_iIndex = -1;
	void Close() {}
}

class RICON_GameRadioInputSource
{
	bool IsCurrent() { return false; }
	int NowMs() { return -1; }
	bool SelectionCurrent(RICON_GameRadioInputSelection selection) { return false; }
	bool ContextCurrent(RICON_GameRadioPttContext context) { return false; }
	bool ScopeLost(RICON_GameRadioPttContext context) { return false; }
	bool LeaseCurrent(RICON_GameRadioPttContext context, int holdToken, int leaseId) { return false; }
	bool RequestContext(int requestId) { return false; }
	bool RequestRadio(int requestId, RICON_GameRadioPttContext context, RICON_GameRadioInputSelection selection) { return false; }
	bool RequestStart(int requestId, int holdToken, int requestSeq, RICON_GameRadioPttContext context,
		RICON_GameRadioInputSelection selection, int deviceHint) { return false; }
	bool RequestStop(int requestId, int holdToken, int requestSeq, RICON_GameRadioPttContext context, int leaseId) { return false; }
	bool SupportsRenewalV3() { return false; }
	bool RequestRenew(int requestId, int holdToken, int leaseId, int requestSeq, RICON_GameRadioPttContext expected) { return false; }
	bool SupportsCancelV2() { return false; }
	bool RequestCancel(int requestId, int holdToken, int requestSeq, int cancelStartSeq, RICON_GameRadioPttContext context) { return false; }
	void Release(int holdToken) {}
}

class RICON_GameRadioInputOwner
{
	protected static const int IDLE = 0, CONTEXT = 1, RADIO = 2, START = 3, ACTIVE = 4;
	protected ref RICON_GameRadioInputSource m_Source;
	protected ref RICON_GameRadioInputSelection m_Selection;
	protected ref RICON_GameRadioPttContext m_Context, m_CanceledContext;
	protected int m_iPhase, m_iPendingRequest, m_iHoldToken, m_iLeaseId, m_iDeviceHint;
	protected int m_iRequestSerial, m_iHoldSerial, m_iSequenceSerial;
	protected int m_iPendingBudget, m_iBeganAt = -1, m_iLastNow = -1, m_iStartSentAt = -1, m_iLeaseBudget;
	protected int m_iRenewConfirmedAt = -1, m_iRenewConfirmedBudget, m_iRenewRequest, m_iRenewSequence, m_iRenewSentAt = -1;
	protected bool m_bRenewAttempted;
	protected int m_iCanceledRequest, m_iCanceledHold, m_iCanceledMethod, m_iCanceledLease;
	protected int m_iStartSequence, m_iStartRequest, m_iCanceledTargetSeq, m_iCanceledStartRequest, m_iCanceledStopRequest;
	protected int m_iCancelSequence, m_iCancelAttempts, m_iCancelBeganAt = -1, m_iCancelLastAt = -1, m_iCleanupBeganAt = -1;
	protected ref array<int> m_CancelRequests = {};
	protected bool m_bKeyDown, m_bWanted, m_bClosed;
	protected string m_sStatus = "IDLE";

	bool Configure(RICON_GameRadioInputSource source, int pendingBudgetMs = 1500)
	{
		if (m_bClosed || m_Source || !source || pendingBudgetMs <= 0 || pendingBudgetMs > 60000) return false;
		m_Source = source; m_iPendingBudget = pendingBudgetMs;
		return true;
	}
	static bool ShouldConsume(bool configured, bool hasOwnHold) { return configured || hasOwnHold; }
	bool OwnsInput() { return m_bKeyDown || m_iPhase != IDLE || m_CanceledContext != null; }
	bool NeedsTick() { return !m_bClosed && (m_iPhase != IDLE || m_CanceledContext != null); }
	string Status() { return m_sStatus; }
	bool ActiveControl()
	{
		ref RICON_GameRadioInputOwner held = this;
		Tick(false); return !m_bClosed && m_iPhase == ACTIVE && m_bWanted;
	}

	//! Metadata validation for the accepted START receipt, without media authority.
	static bool ValidPttMetadata(RICON_GameRadioPttReply reply)
	{
		if (!reply || reply.m_iKind != RICON_GameRadioPttReply.PTT || reply.m_iMethod != RICON_ELifecycleMethod.PTT_START) return false;
		if (!reply.m_bProcessed || !reply.m_bActive || !reply.m_bHasLease || reply.m_eLeaseState != RICON_ETransmitLeaseState.ACTIVE) return false;
		if (reply.m_eCode != RICON_ELifecycleResult.OK
			&& !(reply.m_eCode == RICON_ELifecycleResult.DUPLICATE && reply.m_bHasCachedResult && reply.m_eCachedCode == RICON_ELifecycleResult.OK)) return false;
		if (!reply.m_Context || reply.m_Context.m_sDomain.Length() != 36 || !reply.m_Context.IsValid()) return false;
		if (reply.m_iHoldToken <= 0 || reply.m_iRequestId <= 0 || reply.m_iLeaseId <= 0 || reply.m_iDeviceId <= 0 || reply.m_iIndex < 0) return false;
		return reply.m_iNetworkId > 0 && reply.m_iChannelId > 0 && reply.m_iRxGeneration > 0;
	}

	//! Report only the locally confirmed START or RENEW window.
	int ReadPttRemainingMs(RICON_GameRadioPttReply reply)
	{
		ref RICON_GameRadioInputOwner held = this;
		if (m_iPhase != ACTIVE || !m_bKeyDown || !ActiveControl()) return 0;
		int nowMs = ReadNow();
		if (nowMs < 0 || m_bClosed || m_iPhase != ACTIVE || !m_bKeyDown || !m_bWanted) return 0;
		if (!ValidPttMetadata(reply) || !SameReplyContext(reply) || !m_Selection) return 0;
		if (reply.m_iHoldToken != m_iHoldToken || reply.m_iRequestId != m_iStartRequest || reply.m_iLeaseId != m_iLeaseId) return 0;
		if (reply.m_iDeviceId != m_iDeviceHint || reply.m_iIndex != m_Selection.m_iIndex) return 0;
		if (ConfirmedLeaseBudget() > 60000) return 0;
		return ConfirmedLeaseRemaining(nowMs);
	}

	protected static int LeaseRemaining(int nowMs, int sentAt, int budget)
	{
		if (sentAt < 0 || nowMs < sentAt || budget <= 0) return 0;
		return Math.Max(0, budget - (nowMs - sentAt));
	}

	protected int ConfirmedLeaseRemaining(int nowMs)
	{
		if (m_iRenewConfirmedAt >= 0) return LeaseRemaining(nowMs, m_iRenewConfirmedAt, m_iRenewConfirmedBudget);
		return LeaseRemaining(nowMs, m_iStartSentAt, m_iLeaseBudget);
	}

	protected int ConfirmedLeaseBudget()
	{
		if (m_iRenewConfirmedAt >= 0) return m_iRenewConfirmedBudget;
		return m_iLeaseBudget;
	}

	protected bool HoldingLease(int holdToken, int leaseId)
	{
		return !m_bClosed && m_iPhase == ACTIVE && m_bWanted && m_bKeyDown && holdToken == m_iHoldToken && leaseId == m_iLeaseId;
	}

	protected void ClearPendingRenew()
	{
		m_iRenewRequest = 0;
		m_iRenewSequence = 0;
		m_iRenewSentAt = -1;
	}

	protected void TryRenew(int holdToken, int leaseId, int nowMs)
	{
		// One attempt per confirmed window. A lost or negative ACK leaves that window in force.
		if (!HoldingLease(holdToken, leaseId) || m_bRenewAttempted || !m_Source.SupportsRenewalV3() || !m_Context) return;
		if (ConfirmedLeaseRemaining(nowMs) > Math.Max(1, ConfirmedLeaseBudget() / 2)) return;
		ref RICON_GameRadioPttContext context = m_Context;
		nowMs = ReadNow();
		if (!HoldingLease(holdToken, leaseId) || m_Context != context || ConfirmedLeaseRemaining(nowMs) <= 0) return;
		m_bRenewAttempted = true;
		int requestId = NextRequest(), sequence = NextSequence();
		if (requestId <= 0 || sequence <= 0) return;
		m_iRenewRequest = requestId;
		m_iRenewSequence = sequence;
		m_iRenewSentAt = nowMs;
		bool sent = m_Source.RequestRenew(requestId, holdToken, leaseId, sequence, context);
		if (!sent && m_iRenewRequest == requestId) ClearPendingRenew();
	}

	protected bool MatchesRenewReply(RICON_GameRadioRenewReply reply)
	{
		if (!reply || m_iRenewRequest <= 0 || !HoldingLease(reply.m_iHoldToken, reply.m_iLeaseId)) return false;
		if (reply.m_iRequestId != m_iRenewRequest || reply.m_iRequestSeq != m_iRenewSequence) return false;
		return m_Context && reply.m_Context && m_Context.Same(reply.m_Context);
	}

	void OnRenewReply(RICON_GameRadioRenewReply reply)
	{
		ref RICON_GameRadioInputOwner held = this;
		if (!MatchesRenewReply(reply)) return;
		bool accepted = reply.m_bAccepted && (reply.m_eCode == RICON_ELifecycleResult.OK || reply.m_eCode == RICON_ELifecycleResult.DUPLICATE);
		if (!accepted || reply.m_iServerNowMs < 0 || reply.m_iDeadlineMs <= reply.m_iServerNowMs || reply.m_iDeadlineMs >= int.MAX)
		{
			ClearPendingRenew();
			return;
		}
		int budget = reply.m_iDeadlineMs - reply.m_iServerNowMs;
		if (budget > 60000)
		{
			ClearPendingRenew();
			return;
		}
		bool current = CurrentSelection(reply.m_iHoldToken);
		if (!MatchesRenewReply(reply)) return;
		if (current) current = m_Source.LeaseCurrent(m_Context, reply.m_iHoldToken, reply.m_iLeaseId);
		int nowMs = ReadNow();
		if (!MatchesRenewReply(reply)) return;
		int sentAt = m_iRenewSentAt;
		ClearPendingRenew();
		int previous = ConfirmedLeaseRemaining(nowMs);
		if (!current || previous <= 0 || LeaseRemaining(nowMs, sentAt, budget) <= previous) return;
		m_iRenewConfirmedAt = sentAt;
		m_iRenewConfirmedBudget = budget;
		m_bRenewAttempted = false;
	}

	protected int NextRequest()
	{
		if (m_iRequestSerial >= int.MAX) return 0;
		m_iRequestSerial++; return m_iRequestSerial;
	}
	protected int NextSequence()
	{
		if (m_iSequenceSerial >= int.MAX) return 0;
		m_iSequenceSerial++; return m_iSequenceSerial;
	}
	protected int ReadNow()
	{
		if (!m_Source) return -1;
		int nowMs = m_Source.NowMs();
		if (m_bClosed || nowMs < 0 || nowMs < m_iLastNow) return -1;
		m_iLastNow = nowMs; return nowMs;
	}
	protected bool CurrentSelection(int holdToken)
	{
		if (m_bClosed || !m_Source || !m_Selection || holdToken != m_iHoldToken) return false;
		ref RICON_GameRadioInputSelection selection = m_Selection;
		bool current = m_Source.IsCurrent();
		if (!current || m_bClosed || holdToken != m_iHoldToken || m_Selection != selection || !m_bWanted) return false;
		current = m_Source.SelectionCurrent(selection);
		return current && !m_bClosed && holdToken == m_iHoldToken && m_Selection == selection && m_bWanted;
	}
	protected bool Waiting(int phase, int requestId, int holdToken)
	{
		return !m_bClosed && m_iPhase == phase && m_iPendingRequest == requestId && m_iHoldToken == holdToken;
	}
	protected void ClearCanceled()
	{
		m_CanceledContext = null; m_iCanceledRequest = 0; m_iCanceledHold = 0; m_iCanceledMethod = 0; m_iCanceledLease = 0;
		m_iCanceledTargetSeq = 0; m_iCanceledStartRequest = 0; m_iCanceledStopRequest = 0;
		m_iCancelSequence = 0; m_iCancelAttempts = 0; m_iCancelBeganAt = -1; m_iCancelLastAt = -1; m_iCleanupBeganAt = -1;
		m_CancelRequests.Clear();
	}
	protected void PrepareCancel()
	{
		if (!m_Source || !m_Source.SupportsCancelV2()) return;
		if (!m_CanceledContext || m_iCanceledTargetSeq <= 0 || m_iCanceledMethod == RICON_ELifecycleMethod.PTT_CANCEL) return;
		m_iCanceledMethod = RICON_ELifecycleMethod.PTT_CANCEL;
		m_iCancelSequence = NextSequence(); m_iCancelAttempts = 0;
		m_iCancelBeganAt = -1; m_iCancelLastAt = -1; m_CancelRequests.Clear();
		m_sStatus = "CANCEL_PENDING";
	}
	protected void TryCancel()
	{
		if (m_bClosed || !m_Source || !m_CanceledContext || m_iCanceledMethod != RICON_ELifecycleMethod.PTT_CANCEL) return;
		int nowMs = ReadNow();
		if (nowMs < 0) { m_sStatus = "CANCEL_BAD_TIME"; return; }
		if (m_iCancelBeganAt < 0) m_iCancelBeganAt = nowMs;
		if (m_iCancelSequence <= m_iCanceledTargetSeq) { m_sStatus = "CANCEL_COUNTER_LIMIT"; return; }
		if (m_iCancelAttempts >= 3 || nowMs - m_iCancelBeganAt >= m_iPendingBudget) { m_sStatus = "CANCEL_ACK_UNCERTAIN"; return; }
		if (m_iCancelLastAt >= 0 && nowMs - m_iCancelLastAt < 250) return;
		ref RICON_GameRadioPttContext context = m_CanceledContext;
		int holdToken = m_iCanceledHold, sequence = m_iCancelSequence, target = m_iCanceledTargetSeq;
		if (!m_Source.ContextCurrent(context) || m_bClosed) return;
		if (!m_CanceledContext || !m_CanceledContext.Same(context) || m_iCancelSequence != sequence) return;
		int requestId = NextRequest();
		if (requestId <= 0) { m_sStatus = "CANCEL_COUNTER_LIMIT"; return; }
		m_CancelRequests.Insert(requestId); m_iCancelAttempts++; m_iCancelLastAt = nowMs;
		bool sent = m_Source.RequestCancel(requestId, holdToken, sequence, target, context);
		if (!sent && m_CanceledContext && m_iCancelSequence == sequence) m_sStatus = "CANCEL_SEND_FAILED";
	}

	protected void StopHeld(RICON_GameRadioPttContext context, int holdToken, int leaseId, bool track = true)
	{
		if (!m_Source || !context || holdToken <= 0 || leaseId <= 0) return;
		if (track && !m_bClosed)
		{
			m_CanceledContext = context.CloneContext(); m_iCanceledHold = holdToken;
			m_iCanceledRequest = 0; m_iCanceledMethod = RICON_ELifecycleMethod.PTT_STOP; m_iCanceledLease = leaseId;
			m_iCanceledTargetSeq = m_iStartSequence; m_iCanceledStartRequest = m_iStartRequest;
			m_iCleanupBeganAt = m_iLastNow;
		}
		if (!m_Source.ContextCurrent(context)) { m_sStatus = "CLEANUP_CONTEXT_UNAVAILABLE"; return; }
		int requestId = NextRequest(), sequence = NextSequence();
		if (track && !m_bClosed)
		{
			m_iCanceledRequest = requestId; m_iCanceledStopRequest = requestId;
			if (requestId <= 0 || sequence <= 0) m_iCanceledRequest = int.MAX;
		}
		if (requestId <= 0 || sequence <= 0) { m_sStatus = "CLEANUP_COUNTER_LIMIT"; return; }
		bool sent = m_Source.RequestStop(requestId, holdToken, sequence, context, leaseId);
		if (!sent && m_iCanceledRequest == requestId) m_sStatus = "STOP_SEND_FAILED";
	}
	protected void Cancel(string status, bool keepLateCleanup = true)
	{
		ref RICON_GameRadioInputSelection selection = m_Selection;
		ref RICON_GameRadioPttContext context = m_Context;
		int holdToken = m_iHoldToken, leaseId = m_iLeaseId;
		if (keepLateCleanup && m_iPhase == START && leaseId == 0 && context)
		{
			m_CanceledContext = context.CloneContext(); m_iCanceledRequest = m_iPendingRequest; m_iCanceledHold = holdToken;
			m_iCanceledMethod = RICON_ELifecycleMethod.PTT_START; m_iCanceledLease = 0;
			m_iCanceledTargetSeq = m_iStartSequence; m_iCanceledStartRequest = m_iPendingRequest;
			PrepareCancel();
		}
		if (!keepLateCleanup) ClearCanceled();
		m_bWanted = false; m_iPhase = IDLE; m_iPendingRequest = 0;
		m_Selection = null; m_Context = null; m_iLeaseId = 0; m_iDeviceHint = 0;
		m_iBeganAt = -1; m_iStartSentAt = -1; m_iLeaseBudget = 0; m_sStatus = status;
		m_iRenewConfirmedAt = -1; m_iRenewConfirmedBudget = 0; m_bRenewAttempted = false;
		ClearPendingRenew();
		if (m_Source && holdToken > 0) m_Source.Release(holdToken);
		if (selection) selection.Close();
		if (m_iCanceledMethod == RICON_ELifecycleMethod.PTT_CANCEL) TryCancel();
		else StopHeld(context, holdToken, leaseId, keepLateCleanup);
	}

	bool Down(RICON_GameRadioInputSelection selection)
	{
		ref RICON_GameRadioInputOwner held = this;
		if (m_bClosed || !m_Source || m_bKeyDown || m_iPhase != IDLE)
		{
			if (selection && selection != m_Selection) selection.Close();
			return false;
		}
		if (m_CanceledContext)
		{
			m_bKeyDown = true; m_sStatus = "CLEANUP_PENDING";
			if (selection) selection.Close();
			return false;
		}
		m_bKeyDown = true; m_bWanted = true; m_Selection = selection;
		m_iStartSequence = 0; m_iStartRequest = 0;
		if (m_iHoldSerial >= int.MAX) { Cancel("COUNTER_LIMIT"); return false; }
		m_iHoldSerial++; m_iHoldToken = m_iHoldSerial;
		int nowMs = ReadNow();
		if (nowMs < 0 || !selection || selection.m_iIndex < 0 || !CurrentSelection(m_iHoldToken))
		{
			Cancel("UNAVAILABLE"); return false;
		}
		m_iBeganAt = nowMs;
		int requestId = NextRequest(), holdToken = m_iHoldToken;
		if (requestId <= 0) { Cancel("COUNTER_LIMIT"); return false; }
		m_iPhase = CONTEXT; m_iPendingRequest = requestId; m_sStatus = "CONTEXT_PENDING";
		bool sent = m_Source.RequestContext(requestId);
		if (!sent && Waiting(CONTEXT, requestId, holdToken)) Cancel("CONTEXT_SEND_FAILED");
		return sent && !m_bClosed;
	}
	void Up()
	{
		ref RICON_GameRadioInputOwner held = this;
		m_bKeyDown = false;
		if (m_bClosed || m_iPhase == IDLE) return;
		Cancel("RELEASED");
	}
	void LoseScope()
	{
		ref RICON_GameRadioInputOwner held = this;
		Cancel("SCOPE_LOST", false); m_bKeyDown = false;
	}
	void Close()
	{
		ref RICON_GameRadioInputOwner held = this;
		if (m_bClosed) return;
		m_bClosed = true;
		Cancel("CLOSED", false); m_bKeyDown = false;
	}
	void Tick(bool allowRenewal = true)
	{
		ref RICON_GameRadioInputOwner held = this;
		if (m_bClosed || !m_Source) return;
		if (m_iPhase == IDLE)
		{
			if (m_CanceledContext && m_Source.ScopeLost(m_CanceledContext)) { ClearCanceled(); m_sStatus = "SCOPE_LOST"; return; }
			if (m_iCanceledMethod == RICON_ELifecycleMethod.PTT_CANCEL) { TryCancel(); return; }
			if (m_CanceledContext && m_iCanceledMethod == RICON_ELifecycleMethod.PTT_STOP)
			{
				int nowMs = ReadNow();
				if (m_Source.SupportsCancelV2() && nowMs >= 0 && m_iCleanupBeganAt >= 0 && nowMs - m_iCleanupBeganAt >= 250)
				{
					PrepareCancel(); TryCancel(); return;
				}
				if (m_iCanceledRequest == 0)
				{
					ref RICON_GameRadioPttContext context = m_CanceledContext;
					int token = m_iCanceledHold, lease = m_iCanceledLease;
					if (m_Source.ContextCurrent(context)) StopHeld(context, token, lease);
				}
			}
			return;
		}
		int nowMs = ReadNow();
		if (nowMs < 0) { Cancel("BAD_TIME"); return; }
		if (!CurrentSelection(m_iHoldToken)) { if (!m_bClosed) Cancel("SELECTION_CHANGED"); return; }
		if (m_iPhase == ACTIVE)
		{
			if (ConfirmedLeaseRemaining(nowMs) <= 0)
				{ Cancel("LEASE_EXPIRED"); return; }
			int holdToken = m_iHoldToken, leaseId = m_iLeaseId;
			bool active = m_Source.LeaseCurrent(m_Context, holdToken, leaseId);
			if (!HoldingLease(holdToken, leaseId)) return;
			if (!active) { Cancel("LEASE_NOT_CURRENT"); return; }
			if (allowRenewal) TryRenew(holdToken, leaseId, nowMs);
		}
		else if (m_iBeganAt < 0 || nowMs - m_iBeganAt >= m_iPendingBudget) Cancel("PENDING_TIMEOUT");
	}

	protected void RequestRadio()
	{
		int requestId = NextRequest(), holdToken = m_iHoldToken;
		if (requestId <= 0) { Cancel("COUNTER_LIMIT"); return; }
		m_iPhase = RADIO; m_iPendingRequest = requestId; m_sStatus = "RADIO_PENDING";
		bool sent = m_Source.RequestRadio(requestId, m_Context, m_Selection);
		if (!sent && Waiting(RADIO, requestId, holdToken)) Cancel("RADIO_SEND_FAILED");
	}
	protected void RequestStart()
	{
		int nowMs = ReadNow();
		int requestId = NextRequest(), sequence = NextSequence(), holdToken = m_iHoldToken;
		if (nowMs < 0 || requestId <= 0 || sequence <= 0) { Cancel("COUNTER_LIMIT_OR_TIME"); return; }
		m_iPhase = START; m_iPendingRequest = requestId; m_iStartSentAt = nowMs; m_sStatus = "START_PENDING";
		m_iStartSequence = sequence; m_iStartRequest = requestId;
		bool sent = m_Source.RequestStart(requestId, holdToken, sequence, m_Context, m_Selection, m_iDeviceHint);
		if (!sent && Waiting(START, requestId, holdToken)) Cancel("START_SEND_FAILED");
	}
	protected bool SameReplyContext(RICON_GameRadioPttReply reply)
	{
		return reply && m_Context && reply.m_Context && m_Context.Same(reply.m_Context);
	}
	protected bool CleanupTerminal(RICON_GameRadioPttReply reply)
	{
		if (!reply.m_bProcessed) return false;
		RICON_ELifecycleResult code = reply.m_eCode;
		if (code == RICON_ELifecycleResult.DUPLICATE)
		{
			if (!reply.m_bHasCachedResult) return false;
			code = reply.m_eCachedCode;
		}
		if (code == RICON_ELifecycleResult.UNKNOWN_LEASE) return true;
		if (code != RICON_ELifecycleResult.OK || !reply.m_bHasLease || reply.m_iLeaseId != m_iCanceledLease) return false;
		return reply.m_eLeaseState == RICON_ETransmitLeaseState.REVOKED || reply.m_eLeaseState == RICON_ETransmitLeaseState.EXPIRED;
	}

	protected bool CleanupReply(RICON_GameRadioPttReply reply)
	{
		if (!m_CanceledContext || reply.m_iKind != RICON_GameRadioPttReply.PTT) return false;
		if (m_iCanceledMethod == RICON_ELifecycleMethod.PTT_CANCEL)
		{
			if (!reply.m_Context || !m_CanceledContext.Same(reply.m_Context) || reply.m_iHoldToken != m_iCanceledHold) return false;
			bool originalStart = reply.m_iMethod == RICON_ELifecycleMethod.PTT_START && reply.m_iRequestId == m_iCanceledStartRequest;
			bool originalStop = reply.m_iMethod == RICON_ELifecycleMethod.PTT_STOP && reply.m_iRequestId == m_iCanceledStopRequest;
			if (!originalStart && !originalStop) return false;
			if (reply.m_bHasLease && reply.m_iLeaseId > 0) m_iCanceledLease = reply.m_iLeaseId;
			return true;
		}
		if (reply.m_iRequestId != m_iCanceledRequest || reply.m_iHoldToken != m_iCanceledHold
			|| reply.m_iMethod != m_iCanceledMethod) return false;
		if (!reply.m_Context || !m_CanceledContext.Same(reply.m_Context)) return false;
		if (m_iCanceledMethod == RICON_ELifecycleMethod.PTT_STOP && !CleanupTerminal(reply))
		{
			m_sStatus = "CLEANUP_NOT_TERMINAL"; return true;
		}
		ref RICON_GameRadioPttContext context = m_CanceledContext;
		int holdToken = m_iCanceledHold, method = m_iCanceledMethod; ClearCanceled();
		if (method == RICON_ELifecycleMethod.PTT_START && reply.m_bHasLease && reply.m_iLeaseId > 0)
			StopHeld(context, holdToken, reply.m_iLeaseId);
		return true;
	}

	protected void StartReply(RICON_GameRadioPttReply reply, int holdToken)
	{
		if (reply.m_iKind != RICON_GameRadioPttReply.PTT || reply.m_iHoldToken != holdToken
			|| reply.m_iMethod != RICON_ELifecycleMethod.PTT_START) return;
		if (!SameReplyContext(reply)) { Cancel("CONTEXT_CHANGED"); return; }
		// Retain this ACK's cleanup lease even when selection or the pending time budget has already expired.
		if (reply.m_bHasLease && reply.m_iLeaseId > 0) m_iLeaseId = reply.m_iLeaseId;
		bool contextCurrent = m_Source.ContextCurrent(m_Context);
		if (!Waiting(START, reply.m_iRequestId, holdToken) || !m_bWanted) return;
		if (!contextCurrent)
		{
			bool scopeLost = m_Source.ScopeLost(m_Context);
			if (!Waiting(START, reply.m_iRequestId, holdToken) || !m_bWanted) return;
			if (scopeLost) LoseScope();
			else Cancel("CONTEXT_UNAVAILABLE");
			return;
		}
		bool selected = CurrentSelection(holdToken);
		if (!Waiting(START, reply.m_iRequestId, holdToken) || !m_bWanted) return;
		int nowMs = ReadNow();
		if (!selected || nowMs < 0 || m_iBeganAt < 0 || nowMs - m_iBeganAt >= m_iPendingBudget)
			{ Cancel("START_CANCELED"); return; }
		if (!reply.m_bActive || m_iLeaseId <= 0 || reply.m_iServerNowMs < 0 || reply.m_iDeadlineMs <= reply.m_iServerNowMs)
			{ Cancel("START_NOT_ACTIVE"); return; }
		m_iLeaseBudget = reply.m_iDeadlineMs - reply.m_iServerNowMs;
		if (m_iStartSentAt < 0 || nowMs - m_iStartSentAt >= m_iLeaseBudget) { Cancel("LEASE_EXPIRED"); return; }
		m_iPhase = ACTIVE; m_iPendingRequest = 0; m_sStatus = "ACTIVE_CONTROL_ONLY";
	}
	void OnCancelReply(RICON_GameRadioCancelReply reply)
	{
		ref RICON_GameRadioInputOwner held = this;
		if (m_bClosed || !reply || !m_CanceledContext || m_iCanceledMethod != RICON_ELifecycleMethod.PTT_CANCEL) return;
		if (!reply.m_Context || !m_CanceledContext.Same(reply.m_Context) || !m_CancelRequests.Contains(reply.m_iRequestId)) return;
		if (reply.m_iHoldToken != m_iCanceledHold || reply.m_iRequestSeq != m_iCancelSequence || reply.m_iCancelStartSeq != m_iCanceledTargetSeq) return;
		if (!reply.HasBarrierProof()) { m_sStatus = "CANCEL_NOT_CONFIRMED"; return; }
		ClearCanceled(); m_sStatus = "CANCEL_CONFIRMED";
	}

	void OnReply(RICON_GameRadioPttReply reply)
	{
		ref RICON_GameRadioInputOwner held = this;
		if (m_bClosed || !m_Source || !reply) return;
		if (CleanupReply(reply)) return;
		if (m_iPhase == IDLE || !m_bWanted || reply.m_iRequestId != m_iPendingRequest) return;
		int phase = m_iPhase, holdToken = m_iHoldToken;
		if (phase == START) { StartReply(reply, holdToken); return; }
		bool selected = CurrentSelection(holdToken);
		if (!Waiting(phase, reply.m_iRequestId, holdToken) || !m_bWanted) return;
		if (!selected) { Cancel("SELECTION_CHANGED"); return; }
		int nowMs = ReadNow();
		if (nowMs < 0 || m_iBeganAt < 0 || nowMs - m_iBeganAt >= m_iPendingBudget)
			{ Cancel("REPLY_TIMEOUT"); return; }
		if (phase == CONTEXT)
		{
			if (reply.m_iKind != RICON_GameRadioPttReply.CONTEXT || reply.m_eCode != RICON_ELifecycleResult.OK
				|| !reply.m_Context || !reply.m_Context.IsValid()) { Cancel("CONTEXT_DENIED"); return; }
			m_Context = reply.m_Context.CloneContext();
			bool contextCurrent = m_Source.ContextCurrent(m_Context);
			if (!Waiting(CONTEXT, reply.m_iRequestId, holdToken) || !m_bWanted) return;
			if (!contextCurrent) { Cancel("CONTEXT_CHANGED"); return; }
			RequestRadio(); return;
		}
		if (!SameReplyContext(reply)) { Cancel("CONTEXT_CHANGED"); return; }
		if (phase == RADIO)
		{
			if (reply.m_iKind != RICON_GameRadioPttReply.RADIO || reply.m_eCode != RICON_ELifecycleResult.OK
				|| reply.m_iDeviceId <= 0 || reply.m_iIndex != m_Selection.m_iIndex)
				{ Cancel("RADIO_DENIED"); return; }
			if (reply.m_iNetworkId <= 0 || reply.m_iChannelId <= 0 || reply.m_iRxGeneration <= 0)
				{ Cancel("RADIO_DENIED"); return; }
			bool contextCurrent = m_Source.ContextCurrent(m_Context);
			if (!Waiting(RADIO, reply.m_iRequestId, holdToken) || !m_bWanted) return;
			if (!contextCurrent) { Cancel("CONTEXT_CHANGED"); return; }
			m_iDeviceHint = reply.m_iDeviceId; RequestStart(); return;
		}
	}
}
