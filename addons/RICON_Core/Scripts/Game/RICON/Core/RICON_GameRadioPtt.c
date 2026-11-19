//! Public server domain is a namespace, never an authentication token.
class RICON_GameRadioPttContext
{
	string m_sDomain;
	int m_iEpoch, m_iPlayerId, m_iConnectionId, m_iGeneration, m_iEntityId, m_iPolicyRevision;
	bool IsValid()
	{
		string canonical = m_sDomain; canonical.ToLower();
		return canonical == m_sDomain && RICON_GameRadioControlPolicy.IsValidIdentity(m_sDomain)
			&& m_iEpoch > 0 && m_iPlayerId > 0 && m_iConnectionId > 0 && m_iGeneration > 0 && m_iEntityId > 0 && m_iPolicyRevision > 0;
	}
	bool Same(RICON_GameRadioPttContext other)
	{
		return other && m_sDomain == other.m_sDomain && m_iEpoch == other.m_iEpoch && m_iPlayerId == other.m_iPlayerId
			&& m_iConnectionId == other.m_iConnectionId && m_iGeneration == other.m_iGeneration
			&& m_iEntityId == other.m_iEntityId && m_iPolicyRevision == other.m_iPolicyRevision;
	}
	RICON_GameRadioPttContext CloneContext()
	{
		RICON_GameRadioPttContext copy = new RICON_GameRadioPttContext();
		copy.m_sDomain = m_sDomain; copy.m_iEpoch = m_iEpoch; copy.m_iPlayerId = m_iPlayerId;
		copy.m_iConnectionId = m_iConnectionId; copy.m_iGeneration = m_iGeneration;
		copy.m_iEntityId = m_iEntityId; copy.m_iPolicyRevision = m_iPolicyRevision;
		return copy;
	}
}

//! Opaque trusted transport witnesses; no client-supplied player identity is used as the caller.
class RICON_GameRadioPttCaller {}
class RICON_GameRadioPttSelector {}

class RICON_GameRadioPttControlRequest
{
	int m_iRequestId, m_iIndex;
	ref RICON_GameRadioPttContext m_Expected;
	ref RICON_GameRadioPttSelector m_Selector;
}

class RICON_GameRadioPttRequest
{
	int m_iRequestId, m_iHoldToken, m_iDeviceHint, m_iIndex, m_iLeaseId, m_iRequestSeq;
	RICON_ELifecycleMethod m_eMethod;
	ref RICON_GameRadioPttContext m_Expected;
	ref RICON_GameRadioPttSelector m_Selector;
	bool IsValid()
	{
		if (m_iRequestId <= 0 || m_iHoldToken <= 0 || m_iRequestSeq <= 0 || !m_Expected || !m_Expected.IsValid()) return false;
		if (m_eMethod == RICON_ELifecycleMethod.PTT_START)
			return m_Selector && m_iDeviceHint > 0 && m_iIndex >= 0 && m_iLeaseId == 0;
		if (m_eMethod == RICON_ELifecycleMethod.PTT_STOP)
			return m_iLeaseId > 0 && m_iDeviceHint == 0 && m_iIndex == -1;
		return false;
	}
}

//! Separate schema2 negative closure operation; V1 START/STOP frames remain unchanged.
class RICON_GameRadioCancelRequest
{
	int m_iRequestId, m_iHoldToken, m_iRequestSeq, m_iCancelStartSeq;
	ref RICON_GameRadioPttContext m_Expected;
	bool IsValid()
	{
		return m_iRequestId > 0 && m_iHoldToken > 0 && m_iCancelStartSeq > 0 && m_iRequestSeq > m_iCancelStartSeq
			&& m_Expected && m_Expected.IsValid();
	}
	RICON_GameRadioCancelRequest CloneRequest()
	{
		RICON_GameRadioCancelRequest copy = new RICON_GameRadioCancelRequest();
		copy.m_iRequestId = m_iRequestId; copy.m_iHoldToken = m_iHoldToken; copy.m_iRequestSeq = m_iRequestSeq;
		copy.m_iCancelStartSeq = m_iCancelStartSeq;
		if (m_Expected) copy.m_Expected = m_Expected.CloneContext();
		return copy;
	}
}

class RICON_GameRadioCancelReply
{
	int m_iRequestId, m_iHoldToken, m_iRequestSeq, m_iCancelStartSeq;
	ref RICON_GameRadioPttContext m_Context;
	RICON_ELifecycleResult m_eCode = RICON_ELifecycleResult.NOT_READY;
	bool m_bProcessed, m_bHasCachedResult, m_bHasCancelBarrier, m_bHasLease;
	RICON_ELifecycleResult m_eCachedCode;
	int m_iLeaseId;
	RICON_ETransmitLeaseState m_eLeaseState;
	RICON_ELeaseTerminalReason m_eTerminalReason;
	bool Same(RICON_GameRadioCancelReply other)
	{
		if (!other || !m_Context || !other.m_Context || !m_Context.Same(other.m_Context)) return false;
		if (m_iRequestId != other.m_iRequestId || m_iHoldToken != other.m_iHoldToken
			|| m_iRequestSeq != other.m_iRequestSeq || m_iCancelStartSeq != other.m_iCancelStartSeq) return false;
		if (m_eCode != other.m_eCode || m_bProcessed != other.m_bProcessed
			|| m_bHasCachedResult != other.m_bHasCachedResult || m_eCachedCode != other.m_eCachedCode) return false;
		if (m_bHasCancelBarrier != other.m_bHasCancelBarrier || m_bHasLease != other.m_bHasLease
			|| m_iLeaseId != other.m_iLeaseId || m_eLeaseState != other.m_eLeaseState || m_eTerminalReason != other.m_eTerminalReason) return false;
		return true;
	}
	bool HasBarrierProof()
	{
		if (!m_bProcessed || !m_bHasCancelBarrier) return false;
		if (m_eCode == RICON_ELifecycleResult.OK) return true;
		return m_eCode == RICON_ELifecycleResult.DUPLICATE && m_bHasCachedResult && m_eCachedCode == RICON_ELifecycleResult.OK;
	}
	RICON_GameRadioCancelReply CloneReply()
	{
		RICON_GameRadioCancelReply copy = new RICON_GameRadioCancelReply();
		copy.m_iRequestId = m_iRequestId; copy.m_iHoldToken = m_iHoldToken; copy.m_iRequestSeq = m_iRequestSeq;
		copy.m_iCancelStartSeq = m_iCancelStartSeq;
		if (m_Context) copy.m_Context = m_Context.CloneContext();
		copy.m_eCode = m_eCode; copy.m_bProcessed = m_bProcessed; copy.m_bHasCachedResult = m_bHasCachedResult;
		copy.m_eCachedCode = m_eCachedCode; copy.m_bHasCancelBarrier = m_bHasCancelBarrier; copy.m_bHasLease = m_bHasLease;
		copy.m_iLeaseId = m_iLeaseId; copy.m_eLeaseState = m_eLeaseState; copy.m_eTerminalReason = m_eTerminalReason;
		return copy;
	}
}

class RICON_GameRadioPttReply
{
	static const int CONTEXT = 1, RADIO = 2, PTT = 3;
	int m_iKind, m_iRequestId, m_iHoldToken, m_iMethod;
	ref RICON_GameRadioPttContext m_Context;
	int m_iServerNowMs = -1;
	int m_iDeviceId, m_iNetworkId, m_iChannelId, m_iRxGeneration;
	int m_iIndex = -1;
	RICON_ELifecycleResult m_eCode = RICON_ELifecycleResult.NOT_READY;
	bool m_bProcessed, m_bHasCachedResult, m_bHasLease, m_bActive;
	RICON_ELifecycleResult m_eCachedCode;
	int m_iLeaseId, m_iDeadlineMs = -1;
	RICON_ETransmitLeaseState m_eLeaseState;
	RICON_ELeaseTerminalReason m_eTerminalReason;

	RICON_GameRadioPttReply CloneReply()
	{
		RICON_GameRadioPttReply c = new RICON_GameRadioPttReply();
		c.m_iKind = m_iKind; c.m_iRequestId = m_iRequestId; c.m_iHoldToken = m_iHoldToken; c.m_iMethod = m_iMethod;
		if (m_Context) c.m_Context = m_Context.CloneContext();
		c.m_iServerNowMs = m_iServerNowMs; c.m_iDeviceId = m_iDeviceId; c.m_iNetworkId = m_iNetworkId;
		c.m_iChannelId = m_iChannelId; c.m_iRxGeneration = m_iRxGeneration; c.m_iIndex = m_iIndex;
		c.m_eCode = m_eCode; c.m_bProcessed = m_bProcessed; c.m_bHasCachedResult = m_bHasCachedResult;
		c.m_eCachedCode = m_eCachedCode; c.m_bHasLease = m_bHasLease; c.m_iLeaseId = m_iLeaseId;
		c.m_eLeaseState = m_eLeaseState; c.m_eTerminalReason = m_eTerminalReason;
		c.m_bActive = m_bActive; c.m_iDeadlineMs = m_iDeadlineMs;
		return c;
	}
}

class RICON_GameRadioRenewRequest
{
	int m_iRequestId, m_iHoldToken, m_iRequestSeq, m_iLeaseId;
	ref RICON_GameRadioPttContext m_Expected;

	bool IsValid()
	{
		return m_iRequestId > 0 && m_iHoldToken > 0 && m_iRequestSeq > 0 && m_iLeaseId > 0
			&& m_Expected && m_Expected.IsValid();
	}

	RICON_GameRadioRenewRequest CloneRequest()
	{
		RICON_GameRadioRenewRequest copy = new RICON_GameRadioRenewRequest();
		copy.m_iRequestId = m_iRequestId; copy.m_iHoldToken = m_iHoldToken;
		copy.m_iRequestSeq = m_iRequestSeq; copy.m_iLeaseId = m_iLeaseId;
		if (m_Expected) copy.m_Expected = m_Expected.CloneContext();
		return copy;
	}
}

class RICON_GameRadioRenewReply
{
	int m_iRequestId, m_iHoldToken, m_iRequestSeq, m_iLeaseId;
	ref RICON_GameRadioPttContext m_Context;
	RICON_ELifecycleResult m_eCode = RICON_ELifecycleResult.NOT_READY;
	bool m_bAccepted;
	int m_iServerNowMs = -1, m_iDeadlineMs = -1;

	bool MatchesRequest(RICON_GameRadioRenewRequest request)
	{
		return request && m_Context && m_Context.Same(request.m_Expected)
			&& m_iRequestId == request.m_iRequestId && m_iHoldToken == request.m_iHoldToken
			&& m_iRequestSeq == request.m_iRequestSeq && m_iLeaseId == request.m_iLeaseId;
	}

	bool Same(RICON_GameRadioRenewReply other)
	{
		if (!other || !m_Context || !m_Context.Same(other.m_Context)) return false;
		if (m_iRequestId != other.m_iRequestId || m_iHoldToken != other.m_iHoldToken
			|| m_iRequestSeq != other.m_iRequestSeq || m_iLeaseId != other.m_iLeaseId) return false;
		return m_eCode == other.m_eCode && m_bAccepted == other.m_bAccepted
			&& m_iServerNowMs == other.m_iServerNowMs && m_iDeadlineMs == other.m_iDeadlineMs;
	}

	RICON_GameRadioRenewReply CloneReply()
	{
		RICON_GameRadioRenewReply copy = new RICON_GameRadioRenewReply();
		copy.m_iRequestId = m_iRequestId; copy.m_iHoldToken = m_iHoldToken;
		copy.m_iRequestSeq = m_iRequestSeq; copy.m_iLeaseId = m_iLeaseId;
		if (m_Context) copy.m_Context = m_Context.CloneContext();
		copy.m_eCode = m_eCode; copy.m_bAccepted = m_bAccepted;
		copy.m_iServerNowMs = m_iServerNowMs; copy.m_iDeadlineMs = m_iDeadlineMs;
		return copy;
	}
}

class RICON_GameRadioPttSource
{
	bool IsScope(RICON_LifecycleStateMachine lifecycle) { return false; }
	bool CanConsume(int preEpoch) { return false; }
	UUID Domain() { return UUID.NULL_UUID; }
	int ReadNowMs() { return -1; }
	RICON_GameRadioPttCaller ResolveCaller(RICON_GameRadioPttContext context) { return null; }
	RICON_GameRadioPttContext ReadContext(RICON_GameRadioPttCaller caller) { return null; }
	bool ResolveControl(RICON_GameRadioPttCaller caller, RICON_GameRadioPttContext context,
		RICON_GameRadioPttSelector selector, int index, out int deviceId, out int networkId, out int channelId, out int rxGeneration)
	{
		deviceId = 0; networkId = 0; channelId = 0; rxGeneration = 0;
		return false;
	}
	bool Consume(RICON_LifecycleResult result, int preEpoch) { return false; }
}

//! Original physical selection for an actual active Core lease, not an independent grant or hold-token authority.
class RICON_GameRadioPttBinding
{
	ref RICON_GameRadioPttContext m_Context;
	ref RICON_GameRadioPttSelector m_Selector;
	int m_iRequestSeq, m_iHoldToken, m_iDeviceId, m_iNetworkId, m_iChannelId, m_iRxGeneration, m_iIndex, m_iLeaseId;
	bool m_bGrantConsumed;
	int m_iPublishedUntilMs = -1;
}

//! A server-side read of the current emission. The deadline is already capped by the published PTT grant.
class RICON_GameRadioPttEmission
{
	ref RICON_GameRadioPttContext m_Context;
	int m_iLeaseId, m_iDeviceId, m_iNetworkId, m_iChannelId;
	int m_iTxTransceiverIndex, m_iTxGeneration;
	int m_iIssuedAtMs, m_iServerNowMs, m_iDeadlineMs;
}

//! START/STOP owns no microphone. Lifecycle remains the sole lease/cache/rate/TTL authority.
class RICON_GameRadioPttOwner
{
	protected ref RICON_LifecycleStateMachine m_Lifecycle;
	protected ref RICON_GameRadioPttSource m_Source;
	protected int m_iEpoch, m_iServerTtlMs;
	protected string m_sDomain;
	protected bool m_bBusy, m_bClosed;
	protected ref RICON_GameRadioCancelReply m_LastCancelReceipt, m_LastCancelSnapshot;
	protected ref RICON_GameRadioRenewReply m_LastRenewReceipt, m_LastRenewSnapshot;
	protected ref array<ref RICON_GameRadioPttBinding> m_Bindings = {};
	static const int MAX_SERVER_TTL_MS = 60000;

	bool Configure(RICON_LifecycleStateMachine lifecycle, RICON_GameRadioPttSource source, int serverTtlMs = 1500)
	{
		if (m_bClosed || m_Source || !lifecycle || lifecycle.GetSessionEpoch() <= 0 || lifecycle.GetSessionEpoch() >= int.MAX
			|| !source || serverTtlMs <= 0 || serverTtlMs > MAX_SERVER_TTL_MS
			|| !source.IsScope(lifecycle)) return false;
		string domain = source.Domain();
		if (!RICON_GameRadioControlPolicy.IsValidIdentity(domain) || m_bClosed || !source.IsScope(lifecycle)) return false;
		m_Lifecycle = lifecycle; m_Source = source; m_iEpoch = lifecycle.GetSessionEpoch();
		m_sDomain = domain; m_iServerTtlMs = serverTtlMs;
		return true;
	}

	void Close()
	{
		ref RICON_GameRadioPttOwner held = this;
		m_bClosed = true;
		if (!m_bBusy) { m_Source = null; m_Lifecycle = null; m_Bindings.Clear(); }
	}

	protected bool CurrentScope()
	{
		if (m_bClosed || !m_Source || !m_Lifecycle || m_Lifecycle.GetSessionEpoch() != m_iEpoch) return false;
		ref RICON_GameRadioPttSource source = m_Source;
		ref RICON_LifecycleStateMachine lifecycle = m_Lifecycle;
		bool accepted = source.IsScope(lifecycle);
		return accepted && !m_bClosed && m_Source == source && m_Lifecycle == lifecycle && lifecycle.GetSessionEpoch() == m_iEpoch;
	}

	protected void Finish()
	{
		m_bBusy = false;
		if (m_bClosed) { m_Source = null; m_Lifecycle = null; m_Bindings.Clear(); }
	}

	protected int FreshNow()
	{
		if (!CurrentScope()) return -1;
		int nowMs = m_Source.ReadNowMs();
		if (!CurrentScope() || !m_Lifecycle.CanQueryAt(nowMs)) return -1;
		return nowMs;
	}

	protected bool PureContextCurrent(RICON_GameRadioPttContext context)
	{
		return context && context.IsValid() && m_Lifecycle && context.m_sDomain == m_sDomain && context.m_iEpoch == m_iEpoch
			&& m_Lifecycle.GetSessionEpoch() == m_iEpoch && m_Lifecycle.GetPlayerState(context.m_iPlayerId) == RICON_EPlayerLifecycleState.BOUND_ACTIVE
			&& m_Lifecycle.GetPlayerConnectionId(context.m_iPlayerId) == context.m_iConnectionId
			&& m_Lifecycle.GetPlayerGeneration(context.m_iPlayerId) == context.m_iGeneration
			&& m_Lifecycle.GetPlayerEntityId(context.m_iPlayerId) == context.m_iEntityId;
	}

	protected RICON_GameRadioPttContext CurrentContext(RICON_GameRadioPttCaller caller)
	{
		if (!caller || !CurrentScope()) return null;
		RICON_GameRadioPttContext supplied = m_Source.ReadContext(caller);
		if (!CurrentScope() || !supplied || !PureContextCurrent(supplied)) return null;
		return supplied.CloneContext();
	}

	protected RICON_ELifecycleResult CheckExpected(RICON_GameRadioPttContext expected, RICON_GameRadioPttContext actual)
	{
		if (!expected || !expected.IsValid()) return RICON_ELifecycleResult.MALFORMED;
		if (!actual) return RICON_ELifecycleResult.NOT_READY;
		if (expected.m_sDomain != actual.m_sDomain || expected.m_iEpoch != actual.m_iEpoch) return RICON_ELifecycleResult.INVALID_SESSION;
		if (expected.m_iPlayerId != actual.m_iPlayerId) return RICON_ELifecycleResult.FORGED_OWNER;
		if (expected.m_iConnectionId != actual.m_iConnectionId) return RICON_ELifecycleResult.STALE_CONNECTION;
		if (expected.m_iGeneration != actual.m_iGeneration) return RICON_ELifecycleResult.STALE_GENERATION;
		if (expected.m_iEntityId != actual.m_iEntityId) return RICON_ELifecycleResult.STALE_ENTITY;
		if (expected.m_iPolicyRevision != actual.m_iPolicyRevision) return RICON_ELifecycleResult.INVALID_CATALOG;
		return RICON_ELifecycleResult.OK;
	}

	protected RICON_GameRadioPttReply Reply(int kind, int requestId)
	{
		RICON_GameRadioPttReply reply = new RICON_GameRadioPttReply();
		reply.m_iKind = kind; reply.m_iRequestId = requestId;
		return reply;
	}

	RICON_GameRadioPttReply ReadContext(RICON_GameRadioPttCaller caller, int requestId)
	{
		ref RICON_GameRadioPttOwner held = this;
		RICON_GameRadioPttReply reply = Reply(RICON_GameRadioPttReply.CONTEXT, requestId);
		if (requestId <= 0) { reply.m_eCode = RICON_ELifecycleResult.MALFORMED; return reply; }
		if (m_bBusy || !CurrentScope()) return reply;
		m_bBusy = true;
		reply.m_Context = CurrentContext(caller);
		if (reply.m_Context)
		{
			reply.m_iServerNowMs = FreshNow();
			if (reply.m_iServerNowMs >= 0 && PureContextCurrent(reply.m_Context)) reply.m_eCode = RICON_ELifecycleResult.OK;
		}
		Finish();
		return reply;
	}

	RICON_GameRadioPttReply ReadRadio(RICON_GameRadioPttCaller caller, RICON_GameRadioPttControlRequest request)
	{
		ref RICON_GameRadioPttOwner held = this;
		RICON_GameRadioPttReply reply = Reply(RICON_GameRadioPttReply.RADIO, 0);
		if (!request || request.m_iRequestId <= 0 || !request.m_Expected || request.m_iIndex < 0 || !request.m_Selector)
		{ reply.m_eCode = RICON_ELifecycleResult.MALFORMED; return reply; }
		reply.m_iRequestId = request.m_iRequestId; reply.m_iIndex = request.m_iIndex;
		if (m_bBusy || !CurrentScope()) return reply;
		m_bBusy = true;
		ref RICON_GameRadioPttContext expected = request.m_Expected.CloneContext();
		reply.m_Context = CurrentContext(caller);
		reply.m_eCode = CheckExpected(expected, reply.m_Context);
		if (reply.m_eCode == RICON_ELifecycleResult.OK)
		{
			bool selected = m_Source.ResolveControl(caller, expected, request.m_Selector, request.m_iIndex,
				reply.m_iDeviceId, reply.m_iNetworkId, reply.m_iChannelId, reply.m_iRxGeneration);
			RICON_GameRadioPttContext current = CurrentContext(caller);
			if (!selected || !current || !current.Same(expected) || reply.m_iDeviceId <= 0 || reply.m_iNetworkId <= 0
				|| reply.m_iChannelId <= 0 || reply.m_iRxGeneration <= 0) reply.m_eCode = RICON_ELifecycleResult.UNKNOWN_DEVICE;
			else
			{
				reply.m_iServerNowMs = FreshNow();
				if (reply.m_iServerNowMs < 0 || !PureContextCurrent(expected)) reply.m_eCode = RICON_ELifecycleResult.NOT_READY;
			}
		}
		if (reply.m_eCode != RICON_ELifecycleResult.OK)
		{ reply.m_iDeviceId = 0; reply.m_iNetworkId = 0; reply.m_iChannelId = 0; reply.m_iRxGeneration = 0; }
		Finish();
		return reply;
	}

	RICON_GameRadioPttReply Process(RICON_GameRadioPttCaller caller, RICON_GameRadioPttRequest request)
	{
		ref RICON_GameRadioPttOwner held = this;
		RICON_GameRadioPttReply reply = Reply(RICON_GameRadioPttReply.PTT, 0);
		if (!request || !request.IsValid()) { reply.m_eCode = RICON_ELifecycleResult.MALFORMED; return reply; }
		reply.m_iRequestId = request.m_iRequestId; reply.m_iHoldToken = request.m_iHoldToken; reply.m_iMethod = request.m_eMethod;
		reply.m_iIndex = request.m_iIndex;
		if (m_bBusy || !CurrentScope()) return reply;
		m_bBusy = true;
		ref RICON_GameRadioPttContext expected = request.m_Expected.CloneContext();
		reply.m_Context = CurrentContext(caller);
		reply.m_eCode = CheckExpected(expected, reply.m_Context);
		if (reply.m_eCode == RICON_ELifecycleResult.OK) ProcessHeld(caller, request, expected, reply);
		Finish();
		return reply;
	}

	RICON_GameRadioCancelReply Cancel(RICON_GameRadioPttCaller caller, RICON_GameRadioCancelRequest supplied)
	{
		ref RICON_GameRadioPttOwner held = this;
		RICON_GameRadioCancelReply reply = new RICON_GameRadioCancelReply();
		if (!supplied || !supplied.IsValid()) { reply.m_eCode = RICON_ELifecycleResult.MALFORMED; return reply; }
		ref RICON_GameRadioCancelRequest request = supplied.CloneRequest();
		reply.m_iRequestId = request.m_iRequestId; reply.m_iHoldToken = request.m_iHoldToken;
		reply.m_iRequestSeq = request.m_iRequestSeq; reply.m_iCancelStartSeq = request.m_iCancelStartSeq;
		if (m_bBusy || !CurrentScope()) return reply;
		m_bBusy = true; m_LastCancelReceipt = null; m_LastCancelSnapshot = null;
		reply.m_Context = CurrentContext(caller);
		reply.m_eCode = CheckExpected(request.m_Expected, reply.m_Context);
		if (reply.m_eCode == RICON_ELifecycleResult.OK)
		{
			int nowMs = FreshNow();
			RICON_GameRadioPttContext current = CurrentContext(caller);
			reply.m_eCode = CheckExpected(request.m_Expected, current);
			if (nowMs < 0 || m_bClosed || !m_Lifecycle || !m_Lifecycle.CanQueryAt(nowMs)
				|| !PureContextCurrent(request.m_Expected)) reply.m_eCode = RICON_ELifecycleResult.NOT_READY;
			if (reply.m_eCode == RICON_ELifecycleResult.OK)
			{
				RICON_LifecyclePttRequest cancel = new RICON_LifecyclePttRequest();
				cancel.m_iSchemaVersion = 2; cancel.m_eMethod = RICON_ELifecycleMethod.PTT_CANCEL;
				cancel.m_iNowMs = nowMs; cancel.m_iCorrelationId = request.m_iHoldToken;
				cancel.m_iAuthenticatedPlayerId = current.m_iPlayerId; cancel.m_iPayloadPlayerId = current.m_iPlayerId;
				cancel.m_iSessionEpoch = m_iEpoch; cancel.m_iGeneration = current.m_iGeneration;
				cancel.m_iRequestSeq = request.m_iRequestSeq; cancel.m_iCancelStartSeq = request.m_iCancelStartSeq;
				cancel.m_iDeviceId = 0; cancel.m_iNetworkTypeId = 0; cancel.m_iChannelId = 0; cancel.m_iLeaseId = 0; cancel.m_iServerTtlMs = 0;
				RICON_LifecycleResult result = m_Lifecycle.ProcessPtt(cancel);
				reply.m_bProcessed = true; reply.m_eCode = result.m_eResultCode;
				reply.m_bHasCachedResult = result.m_bHasCachedResult; reply.m_eCachedCode = result.m_eCachedResultCode;
				reply.m_bHasCancelBarrier = result.m_bHasCancelBarrier && result.m_iCancelStartSeq == request.m_iCancelStartSeq;
				reply.m_bHasLease = result.m_bHasLease; reply.m_iLeaseId = result.m_iLeaseId;
				reply.m_eLeaseState = result.m_eLeaseState; reply.m_eTerminalReason = result.m_eTerminalReason;
				PruneBindings(nowMs);
				// A failed consumer cannot undo an authoritative negative Core barrier.
				ConsumeHeld(result);
			}
		}
		RICON_GameRadioPttContext finalContext = CurrentContext(caller);
		if (!finalContext || !reply.m_Context || !finalContext.Same(reply.m_Context) || !CurrentScope()) reply.m_bHasCancelBarrier = false;
		else { m_LastCancelReceipt = reply; m_LastCancelSnapshot = reply.CloneReply(); }
		Finish();
		return reply;
	}

	//! Only the current synchronous server receipt may be serialized; this is not a new media grant.
	bool AuthorizeCancelReply(RICON_GameRadioPttCaller caller, RICON_GameRadioCancelReply reply)
	{
		ref RICON_GameRadioPttOwner held = this;
		if (m_bBusy || !reply || reply != m_LastCancelReceipt || !m_LastCancelSnapshot || !m_LastCancelSnapshot.Same(reply)) return false;
		if (!reply.m_Context || !CurrentScope()) return false;
		m_bBusy = true;
		RICON_GameRadioPttContext current = CurrentContext(caller);
		bool accepted = current && current.Same(m_LastCancelSnapshot.m_Context) && CurrentScope();
		if (m_bClosed || !m_LastCancelSnapshot.Same(reply)) accepted = false;
		Finish(); return accepted;
	}

	RICON_GameRadioRenewReply Renew(RICON_GameRadioPttCaller caller, RICON_GameRadioRenewRequest supplied)
	{
		ref RICON_GameRadioPttOwner held = this;
		ref RICON_GameRadioRenewReply reply = new RICON_GameRadioRenewReply();
		if (!supplied || !supplied.IsValid()) { reply.m_eCode = RICON_ELifecycleResult.MALFORMED; return reply; }
		ref RICON_GameRadioRenewRequest request = supplied.CloneRequest();
		reply.m_iRequestId = request.m_iRequestId; reply.m_iHoldToken = request.m_iHoldToken;
		reply.m_iRequestSeq = request.m_iRequestSeq; reply.m_iLeaseId = request.m_iLeaseId;
		if (m_bBusy || !CurrentScope()) return reply;
		m_bBusy = true;
		m_LastRenewReceipt = null; m_LastRenewSnapshot = null;
		reply.m_Context = CurrentContext(caller);
		reply.m_eCode = CheckExpected(request.m_Expected, reply.m_Context);
		if (reply.m_eCode == RICON_ELifecycleResult.OK)
			RenewHeld(caller, request, reply);
		if (reply.m_Context && CurrentScope())
		{
			m_LastRenewReceipt = reply;
			m_LastRenewSnapshot = reply.CloneReply();
		}
		Finish();
		return reply;
	}

	protected int ReadBoundNow(RICON_GameRadioPttCaller caller, RICON_GameRadioPttBinding binding)
	{
		if (!binding || !binding.m_bGrantConsumed) return -1;
		int nowMs = FreshNow();
		if (nowMs < 0 || !PureContextCurrent(binding.m_Context)) return -1;
		int deviceId, networkId, channelId, rxGeneration;
		bool selected = m_Source.ResolveControl(caller, binding.m_Context, binding.m_Selector, binding.m_iIndex,
			deviceId, networkId, channelId, rxGeneration);
		if (!selected || deviceId != binding.m_iDeviceId || networkId != binding.m_iNetworkId) return -1;
		if (channelId != binding.m_iChannelId || rxGeneration != binding.m_iRxGeneration) return -1;
		RICON_GameRadioPttContext current = CurrentContext(caller);
		if (!current || !current.Same(binding.m_Context)) return -1;
		int confirmedNow = FreshNow();
		if (confirmedNow < nowMs || !PureContextCurrent(binding.m_Context)) return -1;
		return confirmedNow;
	}

	protected RICON_LifecycleLease ReadBoundLease(RICON_GameRadioPttBinding binding, int nowMs)
	{
		if (!binding || !m_Lifecycle || nowMs < 0) return null;
		RICON_LifecycleLease lease = m_Lifecycle.QueryActiveLease(binding.m_iLeaseId, nowMs);
		if (!lease || !lease.MatchesSource(binding.m_Context.m_iPlayerId, binding.m_Context.m_iGeneration,
			binding.m_iDeviceId, binding.m_iNetworkId, binding.m_iChannelId)) return null;
		if (lease.m_iStartCorrelationId != binding.m_iHoldToken || lease.m_iStartRequestSeq != binding.m_iRequestSeq) return null;
		return lease;
	}

	RICON_GameRadioPttEmission ReadCurrentEmission(RICON_GameRadioPttCaller caller, int leaseId)
	{
		ref RICON_GameRadioPttOwner held = this;
		if (m_bBusy || m_bClosed || !caller || leaseId <= 0) return null;
		m_bBusy = true;
		ref RICON_GameRadioPttEmission emission = ReadEmissionHeld(caller, leaseId);
		Finish();
		return emission;
	}

	//! Server-only complete channel snapshot. Reading never advances or releases a lease.
	bool CaptureChannelEmissions(int networkId, int channelId, array<ref RICON_GameRadioPttEmission> emissions)
	{
		ref RICON_GameRadioPttOwner held = this;
		if (!emissions) return false;
		emissions.Clear();
		if (m_bBusy || m_bClosed || networkId <= 0 || channelId <= 0) return false;
		m_bBusy = true;
		bool captured = CaptureChannelEmissionsHeld(networkId, channelId, emissions);
		Finish();
		return captured;
	}

	protected bool CaptureChannelEmissionsHeld(int networkId, int channelId, array<ref RICON_GameRadioPttEmission> emissions)
	{
		int sampledAt = FreshNow();
		if (sampledAt < 0) return false;
		array<ref RICON_GameRadioPttEmission> staged = {};
		array<ref RICON_GameRadioPttBinding> bindings = {};
		foreach (RICON_GameRadioPttBinding binding : m_Bindings)
		{
			if (!binding.m_bGrantConsumed || binding.m_iNetworkId != networkId || binding.m_iChannelId != channelId
				|| binding.m_iPublishedUntilMs <= sampledAt || !PureContextCurrent(binding.m_Context)
				|| !ReadBoundLease(binding, sampledAt)) continue;
			ref RICON_GameRadioPttCaller caller = m_Source.ResolveCaller(binding.m_Context.CloneContext());
			if (!CurrentScope()) return false;
			if (!caller) continue;
			ref RICON_GameRadioPttEmission emission = ReadEmissionHeld(caller, binding.m_iLeaseId);
			if (!CurrentScope()) return false;
			if (!emission || !emission.m_Context.Same(binding.m_Context)) continue;
			if (emission.m_iServerNowMs < sampledAt) return false;
			sampledAt = emission.m_iServerNowMs;
			staged.Insert(emission);
			bindings.Insert(binding);
		}

		int finalNow = FreshNow();
		if (finalNow < sampledAt) return false;
		foreach (int index, RICON_GameRadioPttEmission candidate : staged)
		{
			RICON_GameRadioPttBinding binding = bindings[index];
			if (!PureContextCurrent(candidate.m_Context) || finalNow >= candidate.m_iDeadlineMs) continue;
			RICON_LifecycleLease lease = ReadBoundLease(binding, finalNow);
			if (!lease || lease.EffectiveDeadlineMs() != candidate.m_iDeadlineMs) continue;
			RICON_LifecycleDevice device = m_Lifecycle.QueryDevice(binding.m_iDeviceId, finalNow);
			if (!device || device.m_eKind != RICON_EDeviceKind.CARRIED || device.m_iOwnerPlayerId != candidate.m_Context.m_iPlayerId
				|| !device.m_bPowered || !device.Allows(networkId, channelId)) continue;
			candidate.m_iServerNowMs = finalNow;
			emissions.Insert(candidate);
		}
		return true;
	}

	protected RICON_GameRadioPttEmission ReadEmissionHeld(RICON_GameRadioPttCaller caller, int leaseId)
	{
		RICON_GameRadioPttContext context = CurrentContext(caller);
		if (!context) return null;
		ref RICON_GameRadioPttBinding binding = FindLeaseBinding(leaseId, context);
		int nowMs = ReadBoundNow(caller, binding);
		RICON_LifecycleLease lease = ReadBoundLease(binding, nowMs);
		if (!lease) return null;
		int deadlineMs = lease.EffectiveDeadlineMs();
		if (deadlineMs > binding.m_iPublishedUntilMs) deadlineMs = binding.m_iPublishedUntilMs;
		if (deadlineMs <= nowMs || deadlineMs >= int.MAX) return null;

		RICON_GameRadioPttEmission emission = new RICON_GameRadioPttEmission();
		emission.m_Context = context;
		emission.m_iLeaseId = leaseId; emission.m_iDeviceId = binding.m_iDeviceId;
		emission.m_iNetworkId = binding.m_iNetworkId; emission.m_iChannelId = binding.m_iChannelId;
		emission.m_iTxTransceiverIndex = binding.m_iIndex; emission.m_iTxGeneration = binding.m_iRxGeneration;
		emission.m_iIssuedAtMs = lease.m_iIssuedAtMs; emission.m_iServerNowMs = nowMs; emission.m_iDeadlineMs = deadlineMs;
		return emission;
	}

	protected void RenewHeld(RICON_GameRadioPttCaller caller, RICON_GameRadioRenewRequest request, RICON_GameRadioRenewReply reply)
	{
		ref RICON_GameRadioPttBinding binding = FindLeaseBinding(request.m_iLeaseId, request.m_Expected);
		reply.m_eCode = RICON_ELifecycleResult.STALE_LEASE;
		if (!binding || !binding.m_bGrantConsumed || binding.m_iHoldToken != request.m_iHoldToken) return;
		if (request.m_iRequestSeq <= binding.m_iRequestSeq) return;
		int nowMs = ReadBoundNow(caller, binding);
		if (nowMs < 0) { reply.m_eCode = RICON_ELifecycleResult.NOT_READY; return; }
		if (nowMs >= binding.m_iPublishedUntilMs || !ReadBoundLease(binding, nowMs)) return;

		RICON_LifecyclePttRequest renew = new RICON_LifecyclePttRequest();
		renew.m_iSchemaVersion = 3; renew.m_eMethod = RICON_ELifecycleMethod.PTT_RENEW; renew.m_iNowMs = nowMs;
		renew.m_iCorrelationId = binding.m_iHoldToken; renew.m_iLeaseId = binding.m_iLeaseId;
		renew.m_iAuthenticatedPlayerId = binding.m_Context.m_iPlayerId;
		renew.m_iPayloadPlayerId = renew.m_iAuthenticatedPlayerId; renew.m_iSessionEpoch = m_iEpoch;
		renew.m_iGeneration = binding.m_Context.m_iGeneration; renew.m_iRequestSeq = request.m_iRequestSeq;
		renew.m_iServerTtlMs = m_iServerTtlMs;
		ref RICON_LifecycleResult result = m_Lifecycle.ProcessPtt(renew);
		PruneBindings(nowMs);
		reply.m_eCode = result.m_eResultCode;
		bool fresh = result.m_eResultCode == RICON_ELifecycleResult.OK;
		bool replay = result.m_eResultCode == RICON_ELifecycleResult.DUPLICATE && result.m_bHasCachedResult
			&& result.m_eCachedResultCode == RICON_ELifecycleResult.OK;
		bool exactLease = result.m_bHasLease && result.m_iLeaseId == binding.m_iLeaseId
			&& result.m_eLeaseState == RICON_ETransmitLeaseState.ACTIVE;
		int deadline = result.m_iRenewedUntilMs;
		if (!ConsumeHeld(result)) { reply.m_eCode = RICON_ELifecycleResult.NOT_READY; return; }
		if (!fresh && !replay) return;
		if (!exactLease) { reply.m_eCode = RICON_ELifecycleResult.STALE_LEASE; return; }

		nowMs = ReadBoundNow(caller, binding);
		RICON_LifecycleLease current = ReadBoundLease(binding, nowMs);
		if (!current || nowMs >= binding.m_iPublishedUntilMs) { reply.m_eCode = RICON_ELifecycleResult.STALE_LEASE; return; }
		int currentDeadline = current.EffectiveDeadlineMs();
		if (deadline > currentDeadline) deadline = currentDeadline;
		if (deadline <= nowMs || deadline >= int.MAX) { reply.m_eCode = RICON_ELifecycleResult.STALE_LEASE; return; }
		if (fresh && deadline > binding.m_iPublishedUntilMs) binding.m_iPublishedUntilMs = deadline;
		if (deadline > binding.m_iPublishedUntilMs) deadline = binding.m_iPublishedUntilMs;
		reply.m_iDeadlineMs = deadline;
		reply.m_iServerNowMs = nowMs;
		reply.m_bAccepted = true;
	}

	bool AuthorizeRenewReply(RICON_GameRadioPttCaller caller, RICON_GameRadioRenewReply reply)
	{
		ref RICON_GameRadioPttOwner held = this;
		if (m_bBusy || !reply || reply != m_LastRenewReceipt || !m_LastRenewSnapshot || !m_LastRenewSnapshot.Same(reply)) return false;
		m_bBusy = true;
		ref RICON_GameRadioRenewReply snapshot = m_LastRenewSnapshot;
		m_LastRenewReceipt = null; m_LastRenewSnapshot = null;
		RICON_GameRadioPttContext current = CurrentContext(caller);
		bool allowed = current && current.Same(snapshot.m_Context);
		int nowMs = -1;
		if (allowed && snapshot.m_bAccepted)
		{
			ref RICON_GameRadioPttBinding binding = FindLeaseBinding(snapshot.m_iLeaseId, snapshot.m_Context);
			nowMs = ReadBoundNow(caller, binding);
			RICON_LifecycleLease lease = ReadBoundLease(binding, nowMs);
			allowed = lease && snapshot.m_iHoldToken == binding.m_iHoldToken && nowMs < snapshot.m_iDeadlineMs
				&& snapshot.m_iDeadlineMs <= binding.m_iPublishedUntilMs && snapshot.m_iDeadlineMs <= lease.EffectiveDeadlineMs();
		}
		allowed = allowed && CurrentScope() && snapshot.Same(reply);
		if (allowed && reply.m_bAccepted) reply.m_iServerNowMs = nowMs;
		Finish();
		return allowed;
	}

	protected void ProcessHeld(RICON_GameRadioPttCaller caller, RICON_GameRadioPttRequest request,
		RICON_GameRadioPttContext expected, RICON_GameRadioPttReply reply)
	{
		if (request.m_eMethod == RICON_ELifecycleMethod.PTT_START)
		{
			bool selected = m_Source.ResolveControl(caller, expected, request.m_Selector, request.m_iIndex,
				reply.m_iDeviceId, reply.m_iNetworkId, reply.m_iChannelId, reply.m_iRxGeneration);
			if (!CurrentScope() || !selected || reply.m_iDeviceId <= 0 || reply.m_iNetworkId <= 0
				|| reply.m_iChannelId <= 0 || reply.m_iRxGeneration <= 0) { reply.m_eCode = RICON_ELifecycleResult.UNKNOWN_DEVICE; return; }
			if (request.m_iDeviceHint != reply.m_iDeviceId) { reply.m_eCode = RICON_ELifecycleResult.REQUEST_CONFLICT; return; }
			foreach (RICON_GameRadioPttBinding binding : m_Bindings)
			{
				if (binding.m_iRequestSeq != request.m_iRequestSeq || !binding.m_Context.Same(expected)) continue;
				if (binding.m_iHoldToken != request.m_iHoldToken || binding.m_iDeviceId != reply.m_iDeviceId
					|| binding.m_iNetworkId != reply.m_iNetworkId || binding.m_iChannelId != reply.m_iChannelId
					|| binding.m_iIndex != request.m_iIndex || binding.m_iRxGeneration != reply.m_iRxGeneration)
				{ reply.m_eCode = RICON_ELifecycleResult.REQUEST_CONFLICT; return; }
			}
		}
		RICON_GameRadioPttContext current = CurrentContext(caller);
		reply.m_eCode = CheckExpected(expected, current);
		if (reply.m_eCode != RICON_ELifecycleResult.OK) return;
		int nowMs = FreshNow();
		if (nowMs < 0 || !PureContextCurrent(expected)) { reply.m_eCode = RICON_ELifecycleResult.NOT_READY; return; }
		current = CurrentContext(caller);
		reply.m_eCode = CheckExpected(expected, current);
		if (reply.m_eCode != RICON_ELifecycleResult.OK) return;
		if (request.m_eMethod == RICON_ELifecycleMethod.PTT_START)
		{
			int deviceId, networkId, channelId, rxGeneration;
			bool selectedAgain = m_Source.ResolveControl(caller, expected, request.m_Selector, request.m_iIndex, deviceId, networkId, channelId, rxGeneration);
			if (!CurrentScope() || !selectedAgain || deviceId != reply.m_iDeviceId || networkId != reply.m_iNetworkId
				|| channelId != reply.m_iChannelId || rxGeneration != reply.m_iRxGeneration)
			{ reply.m_eCode = RICON_ELifecycleResult.UNKNOWN_DEVICE; return; }
			current = CurrentContext(caller);
			reply.m_eCode = CheckExpected(expected, current);
			if (reply.m_eCode != RICON_ELifecycleResult.OK) return;
		}
		if (!m_Lifecycle.CanQueryAt(nowMs) || !PureContextCurrent(expected)) { reply.m_eCode = RICON_ELifecycleResult.NOT_READY; return; }
		RICON_LifecyclePttRequest ptt = new RICON_LifecyclePttRequest();
		ptt.m_eMethod = request.m_eMethod; ptt.m_iNowMs = nowMs;
		ptt.m_iCorrelationId = request.m_iHoldToken; ptt.m_iAuthenticatedPlayerId = current.m_iPlayerId;
		ptt.m_iPayloadPlayerId = current.m_iPlayerId; ptt.m_iSessionEpoch = m_iEpoch;
		ptt.m_iGeneration = current.m_iGeneration; ptt.m_iRequestSeq = request.m_iRequestSeq;
		ptt.m_iDeviceId = reply.m_iDeviceId; ptt.m_iNetworkTypeId = reply.m_iNetworkId;
		ptt.m_iChannelId = reply.m_iChannelId; ptt.m_iServerTtlMs = m_iServerTtlMs; ptt.m_iLeaseId = request.m_iLeaseId;
		RICON_LifecycleResult result = m_Lifecycle.ProcessPtt(ptt);
		PruneBindings(nowMs);
		reply.m_bProcessed = true; reply.m_iServerNowMs = nowMs;
		reply.m_eCode = result.m_eResultCode; reply.m_bHasCachedResult = result.m_bHasCachedResult;
		reply.m_eCachedCode = result.m_eCachedResultCode; reply.m_bHasLease = result.m_bHasLease;
		reply.m_iLeaseId = result.m_iLeaseId; reply.m_eLeaseState = result.m_eLeaseState; reply.m_eTerminalReason = result.m_eTerminalReason;
		if (result.m_bHasLease)
		{
			RICON_LifecycleLease lease = m_Lifecycle.QueryActiveLease(result.m_iLeaseId, nowMs);
			if (lease) reply.m_iDeadlineMs = Deadline(lease);
		}
		ref RICON_GameRadioPttBinding grantedBinding;
		if (request.m_eMethod == RICON_ELifecycleMethod.PTT_START && result.m_eResultCode == RICON_ELifecycleResult.OK
			&& result.m_bHasLease && result.m_eLeaseState == RICON_ETransmitLeaseState.ACTIVE)
		{
			RICON_GameRadioPttBinding selectedBinding = new RICON_GameRadioPttBinding();
			selectedBinding.m_Context = expected.CloneContext(); selectedBinding.m_Selector = request.m_Selector;
			selectedBinding.m_iRequestSeq = request.m_iRequestSeq; selectedBinding.m_iHoldToken = request.m_iHoldToken;
			selectedBinding.m_iDeviceId = reply.m_iDeviceId; selectedBinding.m_iNetworkId = reply.m_iNetworkId;
			selectedBinding.m_iChannelId = reply.m_iChannelId; selectedBinding.m_iRxGeneration = reply.m_iRxGeneration;
			selectedBinding.m_iIndex = request.m_iIndex; selectedBinding.m_iLeaseId = result.m_iLeaseId;
			m_Bindings.Insert(selectedBinding);
			grantedBinding = selectedBinding;
		}
		bool consumed = ConsumeHeld(result);
		if (!consumed) { reply.m_bActive = false; return; }
		if (grantedBinding && CurrentScope())
		{
			grantedBinding.m_bGrantConsumed = true;
			grantedBinding.m_iPublishedUntilMs = reply.m_iDeadlineMs;
		}
		QualifyLease(caller, reply);
	}

	protected void PruneBindings(int nowMs)
	{
		if (!m_Lifecycle || !m_Lifecycle.CanQueryAt(nowMs)) return;
		for (int index = m_Bindings.Count() - 1; index >= 0; index--)
			if (!m_Lifecycle.QueryActiveLease(m_Bindings[index].m_iLeaseId, nowMs)) m_Bindings.RemoveOrdered(index);
	}

	protected RICON_GameRadioPttBinding LeaseBinding(RICON_GameRadioPttReply reply)
	{
		return FindLeaseBinding(reply.m_iLeaseId, reply.m_Context);
	}

	protected RICON_GameRadioPttBinding FindLeaseBinding(int leaseId, RICON_GameRadioPttContext context)
	{
		foreach (RICON_GameRadioPttBinding binding : m_Bindings)
			if (binding.m_iLeaseId == leaseId && binding.m_Context.Same(context)) return binding;
		return null;
	}

	protected int Deadline(RICON_LifecycleLease lease)
	{
		if (lease.m_iIssuedAtMs > int.MAX - lease.m_iTtlMs) return int.MAX;
		return lease.m_iIssuedAtMs + lease.m_iTtlMs;
	}

	protected bool ConsumeHeld(RICON_LifecycleResult result)
	{
		ref RICON_GameRadioPttSource source = m_Source;
		return result && source && m_Lifecycle && m_Lifecycle.GetSessionEpoch() == m_iEpoch
			&& source.CanConsume(m_iEpoch) && source.Consume(result, m_iEpoch);
	}

	protected void QualifyLease(RICON_GameRadioPttCaller caller, RICON_GameRadioPttReply reply)
	{
		reply.m_bActive = false;
		int nowMs = FreshNow();
		if (nowMs >= 0) reply.m_iServerNowMs = nowMs;
		if (!reply.m_bHasLease || reply.m_eLeaseState != RICON_ETransmitLeaseState.ACTIVE) return;
		ref RICON_GameRadioPttBinding binding = LeaseBinding(reply);
		RICON_LifecycleLease lease;
		if (nowMs >= 0 && binding && binding.m_bGrantConsumed && reply.m_Context && PureContextCurrent(reply.m_Context))
		{
			int deviceId, networkId, channelId, rxGeneration;
			bool selected = m_Source.ResolveControl(caller, binding.m_Context, binding.m_Selector, binding.m_iIndex,
				deviceId, networkId, channelId, rxGeneration);
			RICON_GameRadioPttContext current = CurrentContext(caller);
			if (selected && current && current.Same(binding.m_Context) && m_Lifecycle.CanQueryAt(nowMs)
				&& deviceId == binding.m_iDeviceId && networkId == binding.m_iNetworkId && channelId == binding.m_iChannelId
				&& rxGeneration == binding.m_iRxGeneration)
				lease = m_Lifecycle.QueryActiveLease(reply.m_iLeaseId, nowMs);
		}
		if (lease && lease.m_iPlayerId == reply.m_Context.m_iPlayerId && lease.m_iGeneration == reply.m_Context.m_iGeneration)
		{
			reply.m_iDeviceId = binding.m_iDeviceId; reply.m_iNetworkId = binding.m_iNetworkId;
			reply.m_iChannelId = binding.m_iChannelId; reply.m_iRxGeneration = binding.m_iRxGeneration; reply.m_iIndex = binding.m_iIndex;
			reply.m_iDeadlineMs = Deadline(lease);
			reply.m_bActive = !m_bClosed && reply.m_iMethod == RICON_ELifecycleMethod.PTT_START
				&& (reply.m_eCode == RICON_ELifecycleResult.OK
					|| (reply.m_eCode == RICON_ELifecycleResult.DUPLICATE && reply.m_eCachedCode == RICON_ELifecycleResult.OK));
			if (lease.m_iRenewedUntilMs != -1 && nowMs >= reply.m_iDeadlineMs) reply.m_bActive = false;
			return;
		}
		if (nowMs >= 0 && reply.m_iDeadlineMs >= 0 && nowMs >= reply.m_iDeadlineMs)
		{ reply.m_eLeaseState = RICON_ETransmitLeaseState.EXPIRED; reply.m_eTerminalReason = RICON_ELeaseTerminalReason.TTL_EXPIRED; }
		else { reply.m_eLeaseState = RICON_ETransmitLeaseState.NONE; reply.m_eTerminalReason = RICON_ELeaseTerminalReason.NONE; }
	}

	//! Materialize expiry even when inventory remains unchanged. This is not lease renewal.
	bool Pump()
	{
		ref RICON_GameRadioPttOwner held = this;
		if (m_bBusy || !CurrentScope()) return false;
		m_bBusy = true;
		int nowMs = FreshNow();
		bool accepted;
		if (nowMs >= 0)
		{
			RICON_LifecycleResult result = m_Lifecycle.Tick(nowMs);
			PruneBindings(nowMs);
			accepted = ConsumeHeld(result);
		}
		Finish();
		return accepted;
	}

	bool AuthorizeReply(RICON_GameRadioPttCaller caller, RICON_GameRadioPttReply reply)
	{
		ref RICON_GameRadioPttOwner held = this;
		if (m_bBusy || !reply || !reply.m_Context || !CurrentScope()) return false;
		m_bBusy = true;
		RICON_GameRadioPttContext current = CurrentContext(caller);
		bool accepted = current && current.Same(reply.m_Context);
		if (accepted && reply.m_iKind == RICON_GameRadioPttReply.PTT)
		{
			bool previouslyActive = reply.m_bActive;
			QualifyLease(caller, reply);
			reply.m_bActive = previouslyActive && reply.m_bActive;
		}
		current = CurrentContext(caller);
		if (!current || !current.Same(reply.m_Context)) accepted = false;
		if (!CurrentScope()) accepted = false;
		Finish();
		return accepted;
	}
}

//! Fixed-arity int frames plus a public domain. No key, native frequency, power or TS3 identity is carried.
class RICON_GameRadioPttWire
{
	static const int VERSION = 1, CONTROL_FIELDS = 9, REQUEST_FIELDS = 14, REPLY_FIELDS = 27;

	static RICON_GameRadioPttContext ReadContext(string domain, array<int> fields, int offset)
	{
		if (!fields || offset < 0 || fields.Count() - offset < 6 || domain.Length() != 36) return null;
		RICON_GameRadioPttContext context = new RICON_GameRadioPttContext();
		context.m_sDomain = domain; context.m_iEpoch = fields[offset]; context.m_iPlayerId = fields[offset + 1];
		context.m_iConnectionId = fields[offset + 2]; context.m_iGeneration = fields[offset + 3];
		context.m_iEntityId = fields[offset + 4]; context.m_iPolicyRevision = fields[offset + 5];
		if (!context.IsValid()) return null;
		return context;
	}

	static void AppendContext(array<int> fields, RICON_GameRadioPttContext context)
	{
		if (!context) { for (int empty = 0; empty < 6; empty++) fields.Insert(0); return; }
		fields.Insert(context.m_iEpoch); fields.Insert(context.m_iPlayerId); fields.Insert(context.m_iConnectionId);
		fields.Insert(context.m_iGeneration); fields.Insert(context.m_iEntityId); fields.Insert(context.m_iPolicyRevision);
	}

	static RICON_GameRadioPttControlRequest DecodeControl(string domain, array<int> fields, RICON_GameRadioPttSelector selector)
	{
		if (!fields || fields.Count() != CONTROL_FIELDS || fields[0] != VERSION || fields[1] <= 0 || fields[8] < 0 || !selector) return null;
		RICON_GameRadioPttContext context = ReadContext(domain, fields, 2);
		if (!context) return null;
		RICON_GameRadioPttControlRequest request = new RICON_GameRadioPttControlRequest();
		request.m_iRequestId = fields[1]; request.m_Expected = context; request.m_Selector = selector; request.m_iIndex = fields[8];
		return request;
	}

	static array<int> EncodeControl(int requestId, RICON_GameRadioPttContext context, int index)
	{
		if (requestId <= 0 || !context || !context.IsValid() || index < 0) return null;
		array<int> fields = {VERSION, requestId}; AppendContext(fields, context); fields.Insert(index);
		return fields;
	}

	static RICON_GameRadioPttRequest DecodeRequest(string domain, array<int> fields, RICON_GameRadioPttSelector selector)
	{
		if (!fields || fields.Count() != REQUEST_FIELDS || fields[0] != VERSION) return null;
		RICON_GameRadioPttRequest request = new RICON_GameRadioPttRequest();
		request.m_iRequestId = fields[1]; request.m_Expected = ReadContext(domain, fields, 2);
		request.m_iHoldToken = fields[8]; request.m_eMethod = fields[9]; request.m_iDeviceHint = fields[10];
		request.m_iIndex = fields[11]; request.m_iLeaseId = fields[12]; request.m_iRequestSeq = fields[13]; request.m_Selector = selector;
		if (!request.IsValid()) return null;
		return request;
	}

	static array<int> EncodeRequest(RICON_GameRadioPttRequest request)
	{
		if (!request || !request.IsValid()) return null;
		array<int> fields = {VERSION, request.m_iRequestId}; AppendContext(fields, request.m_Expected);
		fields.Insert(request.m_iHoldToken); fields.Insert(request.m_eMethod); fields.Insert(request.m_iDeviceHint);
		fields.Insert(request.m_iIndex); fields.Insert(request.m_iLeaseId); fields.Insert(request.m_iRequestSeq);
		return fields;
	}

	protected static int Bit(bool value)
	{
		if (value)
		{
			return 1;
		}
		return 0;
	}

	static array<int> EncodeReply(RICON_GameRadioPttReply reply)
	{
		if (!reply) return null;
		array<int> fields = {VERSION, reply.m_iKind, reply.m_iRequestId, reply.m_iHoldToken, reply.m_iMethod};
		AppendContext(fields, reply.m_Context);
		fields.Insert(reply.m_iServerNowMs); fields.Insert(reply.m_iDeviceId); fields.Insert(reply.m_iNetworkId);
		fields.Insert(reply.m_iChannelId); fields.Insert(reply.m_iRxGeneration); fields.Insert(reply.m_iIndex); fields.Insert(reply.m_eCode);
		fields.Insert(Bit(reply.m_bProcessed)); fields.Insert(Bit(reply.m_bHasCachedResult)); fields.Insert(reply.m_eCachedCode);
		fields.Insert(Bit(reply.m_bHasLease)); fields.Insert(reply.m_iLeaseId); fields.Insert(reply.m_eLeaseState);
		fields.Insert(reply.m_eTerminalReason); fields.Insert(Bit(reply.m_bActive)); fields.Insert(reply.m_iDeadlineMs);
		return fields;
	}

	static RICON_GameRadioPttReply DecodeReply(string domain, array<int> fields)
	{
		if (!fields) return null;
		if (fields.Count() != REPLY_FIELDS) return null;
		if (fields[0] != VERSION) return null;
		if (fields[1] < 1 || fields[1] > 3 || fields[2] <= 0) return null;
		if (fields[11] < -1 || fields[16] < -1 || fields[26] < -1) return null;
		if (fields[17] < 0 || fields[17] > RICON_ELifecycleResult.OK) return null;
		if (fields[20] < 0 || fields[20] > RICON_ELifecycleResult.OK || fields[22] < 0) return null;
		if (fields[23] < 0 || fields[23] > RICON_ETransmitLeaseState.REVOKED) return null;
		if (fields[24] < 0 || fields[24] > RICON_ELeaseTerminalReason.TICK_ANOMALY) return null;
		for (int flag = 18; flag <= 21; flag++)
			if (flag != 20 && (fields[flag] < 0 || fields[flag] > 1)) return null;
		if (fields[25] < 0 || fields[25] > 1) return null;
		RICON_GameRadioPttReply reply = new RICON_GameRadioPttReply();
		reply.m_iKind = fields[1]; reply.m_iRequestId = fields[2]; reply.m_iHoldToken = fields[3]; reply.m_iMethod = fields[4];
		if (domain.IsEmpty())
		{
			for (int empty = 5; empty < 11; empty++) if (fields[empty] != 0) return null;
			if (fields[17] == RICON_ELifecycleResult.OK) return null;
		}
		else { reply.m_Context = ReadContext(domain, fields, 5); if (!reply.m_Context) return null; }
		reply.m_iServerNowMs = fields[11]; reply.m_iDeviceId = fields[12]; reply.m_iNetworkId = fields[13];
		reply.m_iChannelId = fields[14]; reply.m_iRxGeneration = fields[15]; reply.m_iIndex = fields[16]; reply.m_eCode = fields[17];
		reply.m_bProcessed = fields[18] == 1; reply.m_bHasCachedResult = fields[19] == 1; reply.m_eCachedCode = fields[20];
		reply.m_bHasLease = fields[21] == 1; reply.m_iLeaseId = fields[22]; reply.m_eLeaseState = fields[23];
		reply.m_eTerminalReason = fields[24]; reply.m_bActive = fields[25] == 1; reply.m_iDeadlineMs = fields[26];
		if (reply.m_iKind == RICON_GameRadioPttReply.PTT)
		{
			if (reply.m_iHoldToken <= 0 || (reply.m_iMethod != 1 && reply.m_iMethod != 2)) return null;
		}
		else if (reply.m_iHoldToken != 0 || reply.m_iMethod != 0 || reply.m_bProcessed || reply.m_bHasLease || reply.m_bHasCachedResult) return null;
		if (reply.m_iDeviceId < 0 || reply.m_iNetworkId < 0 || reply.m_iChannelId < 0 || reply.m_iRxGeneration < 0) return null;
		if (reply.m_iDeviceId == 0 && (reply.m_iNetworkId != 0 || reply.m_iChannelId != 0 || reply.m_iRxGeneration != 0)) return null;
		if (reply.m_iDeviceId > 0 && (reply.m_iNetworkId <= 0 || reply.m_iChannelId <= 0 || reply.m_iRxGeneration <= 0 || reply.m_iIndex < 0)) return null;
		if (reply.m_iKind == RICON_GameRadioPttReply.CONTEXT && (reply.m_iDeviceId != 0 || reply.m_iIndex != -1)) return null;
		if (reply.m_eCode == RICON_ELifecycleResult.OK && (!reply.m_Context || reply.m_iServerNowMs < 0
			|| (reply.m_iKind == RICON_GameRadioPttReply.RADIO && reply.m_iDeviceId <= 0))) return null;
		if (reply.m_bProcessed && (!reply.m_Context || reply.m_iServerNowMs < 0 || reply.m_iKind != RICON_GameRadioPttReply.PTT)) return null;
		if (reply.m_bHasCachedResult && (!reply.m_bProcessed || reply.m_eCode != RICON_ELifecycleResult.DUPLICATE)) return null;
		if (reply.m_eCode == RICON_ELifecycleResult.DUPLICATE && !reply.m_bHasCachedResult) return null;
		if (reply.m_bHasLease && reply.m_iLeaseId <= 0) return null;
		if (!reply.m_bHasLease && (reply.m_iLeaseId != 0 || reply.m_eLeaseState != RICON_ETransmitLeaseState.NONE || reply.m_eTerminalReason != RICON_ELeaseTerminalReason.NONE)) return null;
		if (reply.m_bActive)
		{
			if (!reply.m_bProcessed || !reply.m_bHasLease || !reply.m_Context) return null;
			if (reply.m_iDeviceId <= 0 || reply.m_iNetworkId <= 0 || reply.m_iChannelId <= 0 || reply.m_iRxGeneration <= 0 || reply.m_iIndex < 0) return null;
			if (reply.m_iMethod != RICON_ELifecycleMethod.PTT_START || reply.m_eLeaseState != RICON_ETransmitLeaseState.ACTIVE) return null;
			if (reply.m_iServerNowMs < 0 || reply.m_iDeadlineMs <= reply.m_iServerNowMs) return null;
			bool effectiveOk = reply.m_eCode == RICON_ELifecycleResult.OK;
			if (reply.m_eCode == RICON_ELifecycleResult.DUPLICATE && reply.m_bHasCachedResult && reply.m_eCachedCode == RICON_ELifecycleResult.OK)
				effectiveOk = true;
			if (!effectiveOk) return null;
		}
		return reply;
	}
}

class RICON_GameRadioPttNativeCaller : RICON_GameRadioPttCaller
{
	protected SCR_PlayerController m_Controller;
	void RICON_GameRadioPttNativeCaller(SCR_PlayerController controller) { m_Controller = controller; }
	SCR_PlayerController Controller() { return m_Controller; }
}

class RICON_GameRadioPttNativeSelector : RICON_GameRadioPttSelector
{
	protected RplId m_Id;
	void RICON_GameRadioPttNativeSelector(RplId id) { m_Id = id; }
	RplId Id() { return m_Id; }
}

class RICON_GameRadioPttNativeSource : RICON_GameRadioPttSource
{
	protected RICON_GameRadioManagerComponent m_Manager;
	protected RICON_LifecycleStateMachine m_Lifecycle;
	protected ref RICON_GameControllerLifecycle m_Bridge;
	protected BaseWorld m_World;
	protected int m_iEpoch;

	bool Configure(RICON_GameRadioManagerComponent manager, RICON_LifecycleStateMachine lifecycle, RICON_GameControllerLifecycle bridge)
	{
		if (m_Manager || !manager || !lifecycle || !bridge || !GetGame()) return false;
		m_Manager = manager; m_Lifecycle = lifecycle; m_Bridge = bridge; m_World = GetGame().GetWorld(); m_iEpoch = lifecycle.GetSessionEpoch();
		return IsScope(lifecycle);
	}
		override bool CanConsume(int preEpoch)
	{
		return m_Manager && m_Lifecycle && GetGame() && m_World && preEpoch == m_iEpoch
			&& RICON_CommsManagerComponent.IsAuthority() && RICON_CommsManagerComponent.GetInstance() == m_Manager
			&& GetGame().GetWorld() == m_World && !m_World.IsEditMode() && m_Manager.GetOwner()
			&& m_Manager.GetOwner().GetWorld() == m_World && m_Manager.OwnsInventoryLifecycle(m_Lifecycle, preEpoch);
	}
		override bool IsScope(RICON_LifecycleStateMachine lifecycle)
	{
		return lifecycle == m_Lifecycle && CanConsume(m_iEpoch) && m_Bridge && m_Bridge.MatchesLifecycle(lifecycle)
			&& m_Manager.HasInventoryPttScope(lifecycle, m_iEpoch);
	}
	override UUID Domain()
	{
		if (!m_Manager) return UUID.NULL_UUID;
		return m_Manager.GetInventoryPublicDomain();
	}
		override int ReadNowMs()
	{
		if (!IsScope(m_Lifecycle)) return -1;
		return m_Bridge.ReadWireAuthorityTime(m_Lifecycle);
	}
	override RICON_GameRadioPttCaller ResolveCaller(RICON_GameRadioPttContext context)
	{
		if (!context || !context.IsValid() || !IsScope(m_Lifecycle) || context.m_iEpoch != m_iEpoch) return null;
		SCR_PlayerController controller = m_Bridge.ResolveRemoteRecipient(context.m_iPlayerId,
			context.m_iConnectionId, context.m_iGeneration, context.m_iEntityId);
		if (!controller || !IsScope(m_Lifecycle)) return null;
		return new RICON_GameRadioPttNativeCaller(controller);
	}
		override RICON_GameRadioPttContext ReadContext(RICON_GameRadioPttCaller caller)
	{
		if (!IsScope(m_Lifecycle)) return null;
		RICON_GameRadioPttNativeCaller nativeCaller = RICON_GameRadioPttNativeCaller.Cast(caller);
		if (!nativeCaller || !nativeCaller.Controller()) return null;
		int connection, generation, entityId;
		if (!m_Bridge.ReadRemoteSender(nativeCaller.Controller(), connection, generation, entityId)) return null;
		RICON_GameRadioPttContext context = new RICON_GameRadioPttContext();
		context.m_sDomain = Domain(); context.m_iEpoch = m_iEpoch; context.m_iPlayerId = nativeCaller.Controller().GetPlayerId();
		context.m_iConnectionId = connection; context.m_iGeneration = generation; context.m_iEntityId = entityId;
		context.m_iPolicyRevision = m_Manager.GetInventoryPolicyRevision();
		if (!context.IsValid() || !IsScope(m_Lifecycle)) return null;
		return context;
	}
		override bool ResolveControl(RICON_GameRadioPttCaller caller, RICON_GameRadioPttContext context,
		RICON_GameRadioPttSelector selector, int index, out int deviceId, out int networkId, out int channelId, out int rxGeneration)
	{
		deviceId = 0; networkId = 0; channelId = 0; rxGeneration = 0;
		RICON_GameRadioPttContext current = ReadContext(caller);
		RICON_GameRadioPttNativeSelector nativeSelector = RICON_GameRadioPttNativeSelector.Cast(selector);
		if (!current || !context || !current.Same(context) || !nativeSelector || !nativeSelector.Id().IsValid() || index < 0) return false;
		RplComponent replication = RplComponent.Cast(Replication.FindItem(nativeSelector.Id()));
		if (!replication || replication.Id() != nativeSelector.Id()) return false;
		IEntity item = replication.GetEntity();
		if (!item || item.IsDeleted() || item.GetWorld() != m_World || item.FindComponent(RplComponent) != replication) return false;
		BaseRadioComponent radio = BaseRadioComponent.Cast(item.FindComponent(BaseRadioComponent));
		if (!radio || radio.GetOwner() != item || index >= radio.TransceiversCount()) return false;
		BaseTransceiver transceiver = radio.GetTransceiver(index);
		if (!transceiver || transceiver.GetRadio() != radio
			|| !m_Manager.ReadInventoryRadioControl(context.m_iPlayerId, radio, index, deviceId, networkId, channelId, rxGeneration)) return false;
		current = ReadContext(caller);
		if (!current || !current.Same(context)) { deviceId = 0; networkId = 0; channelId = 0; rxGeneration = 0; return false; }
		return true;
	}
		override bool Consume(RICON_LifecycleResult result, int preEpoch)
	{
		if (!result || !CanConsume(preEpoch)) return false;
		RICON_TemporalCaptureResult consumed = m_Manager.ConsumeTemporalLifecycleResult(result, preEpoch);
		return consumed && consumed.m_sCode == "OK" && CanConsume(preEpoch);
	}
}

//! Schema2 cancellation stays separate from the fixed V1 9/14/27 frame set.
class RICON_GameRadioCancelWire
{
	static const int VERSION = 2, REQUEST_FIELDS = 11, REPLY_FIELDS = 20;
	protected static bool IsBit(int value) { return value == 0 || value == 1; }
	protected static int Bit(bool value)
	{
		if (value)
			return 1;
		return 0;
	}
	static array<int> EncodeRequest(RICON_GameRadioCancelRequest request)
	{
		if (!request || !request.IsValid()) return null;
		array<int> fields = {VERSION, request.m_iRequestId}; RICON_GameRadioPttWire.AppendContext(fields, request.m_Expected);
		fields.Insert(request.m_iHoldToken); fields.Insert(request.m_iRequestSeq); fields.Insert(request.m_iCancelStartSeq);
		return fields;
	}
	static RICON_GameRadioCancelRequest DecodeRequest(string domain, array<int> fields)
	{
		if (!fields || fields.Count() != REQUEST_FIELDS || fields[0] != VERSION) return null;
		RICON_GameRadioCancelRequest request = new RICON_GameRadioCancelRequest();
		request.m_iRequestId = fields[1]; request.m_Expected = RICON_GameRadioPttWire.ReadContext(domain, fields, 2);
		request.m_iHoldToken = fields[8]; request.m_iRequestSeq = fields[9]; request.m_iCancelStartSeq = fields[10];
		if (!request.IsValid()) return null;
		return request;
	}
	protected static bool ValidReply(RICON_GameRadioCancelReply reply)
	{
		if (!reply || !reply.m_Context || !reply.m_Context.IsValid()) return false;
		if (reply.m_iRequestId <= 0 || reply.m_iHoldToken <= 0 || reply.m_iCancelStartSeq <= 0 || reply.m_iRequestSeq <= reply.m_iCancelStartSeq) return false;
		if (reply.m_eCode < RICON_ELifecycleResult.MALFORMED || reply.m_eCode > RICON_ELifecycleResult.OK) return false;
		if (reply.m_bHasCachedResult && (reply.m_eCachedCode < RICON_ELifecycleResult.MALFORMED || reply.m_eCachedCode > RICON_ELifecycleResult.OK)) return false;
		if (!reply.m_bHasCachedResult && reply.m_eCachedCode != 0) return false;
		if (reply.m_bHasCachedResult != (reply.m_eCode == RICON_ELifecycleResult.DUPLICATE)) return false;
		if (reply.m_bHasCancelBarrier && !reply.HasBarrierProof()) return false;
		if (reply.m_bHasLease)
		{
			if (reply.m_iLeaseId <= 0 || reply.m_eLeaseState < RICON_ETransmitLeaseState.ACTIVE || reply.m_eLeaseState > RICON_ETransmitLeaseState.REVOKED) return false;
			if (reply.m_eTerminalReason < RICON_ELeaseTerminalReason.NONE || reply.m_eTerminalReason > RICON_ELeaseTerminalReason.TICK_ANOMALY) return false;
			if (reply.m_bHasCancelBarrier && reply.m_eLeaseState == RICON_ETransmitLeaseState.ACTIVE) return false;
		}
		else if (reply.m_iLeaseId != 0 || reply.m_eLeaseState != RICON_ETransmitLeaseState.NONE || reply.m_eTerminalReason != RICON_ELeaseTerminalReason.NONE) return false;
		return true;
	}
	static array<int> EncodeReply(RICON_GameRadioCancelReply reply)
	{
		if (!ValidReply(reply)) return null;
		array<int> fields = {VERSION, reply.m_iRequestId}; RICON_GameRadioPttWire.AppendContext(fields, reply.m_Context);
		fields.Insert(reply.m_iHoldToken); fields.Insert(reply.m_iRequestSeq); fields.Insert(reply.m_iCancelStartSeq);
		fields.Insert(reply.m_eCode); fields.Insert(Bit(reply.m_bProcessed)); fields.Insert(Bit(reply.m_bHasCachedResult));
		fields.Insert(reply.m_eCachedCode); fields.Insert(Bit(reply.m_bHasCancelBarrier)); fields.Insert(Bit(reply.m_bHasLease));
		fields.Insert(reply.m_iLeaseId); fields.Insert(reply.m_eLeaseState); fields.Insert(reply.m_eTerminalReason);
		return fields;
	}
	static RICON_GameRadioCancelReply DecodeReply(string domain, array<int> fields)
	{
		if (!fields || fields.Count() != REPLY_FIELDS || fields[0] != VERSION) return null;
		if (!IsBit(fields[12]) || !IsBit(fields[13]) || !IsBit(fields[15]) || !IsBit(fields[16])) return null;
		RICON_GameRadioCancelReply reply = new RICON_GameRadioCancelReply();
		reply.m_iRequestId = fields[1]; reply.m_Context = RICON_GameRadioPttWire.ReadContext(domain, fields, 2);
		reply.m_iHoldToken = fields[8]; reply.m_iRequestSeq = fields[9]; reply.m_iCancelStartSeq = fields[10];
		reply.m_eCode = fields[11]; reply.m_bProcessed = fields[12] == 1; reply.m_bHasCachedResult = fields[13] == 1;
		reply.m_eCachedCode = fields[14]; reply.m_bHasCancelBarrier = fields[15] == 1; reply.m_bHasLease = fields[16] == 1;
		reply.m_iLeaseId = fields[17]; reply.m_eLeaseState = fields[18]; reply.m_eTerminalReason = fields[19];
		if (!ValidReply(reply)) return null;
		return reply;
	}
}

class RICON_GameRadioRenewWire
{
	static const int VERSION = 3, REQUEST_FIELDS = 11, REPLY_FIELDS = 15;

	static array<int> EncodeRequest(RICON_GameRadioRenewRequest request)
	{
		if (!request || !request.IsValid()) return null;
		array<int> fields = {VERSION, request.m_iRequestId};
		RICON_GameRadioPttWire.AppendContext(fields, request.m_Expected);
		fields.Insert(request.m_iHoldToken); fields.Insert(request.m_iRequestSeq); fields.Insert(request.m_iLeaseId);
		return fields;
	}

	static RICON_GameRadioRenewRequest DecodeRequest(string domain, array<int> fields)
	{
		if (!fields || fields.Count() != REQUEST_FIELDS || fields[0] != VERSION) return null;
		RICON_GameRadioRenewRequest request = new RICON_GameRadioRenewRequest();
		request.m_iRequestId = fields[1]; request.m_Expected = RICON_GameRadioPttWire.ReadContext(domain, fields, 2);
		request.m_iHoldToken = fields[8]; request.m_iRequestSeq = fields[9]; request.m_iLeaseId = fields[10];
		if (!request.IsValid()) return null;
		return request;
	}

	protected static bool ValidReply(RICON_GameRadioRenewReply reply)
	{
		if (!reply || !reply.m_Context || !reply.m_Context.IsValid()) return false;
		if (reply.m_iRequestId <= 0 || reply.m_iHoldToken <= 0 || reply.m_iRequestSeq <= 0 || reply.m_iLeaseId <= 0) return false;
		if (reply.m_eCode < RICON_ELifecycleResult.MALFORMED || reply.m_eCode > RICON_ELifecycleResult.OK) return false;
		if (!reply.m_bAccepted) return reply.m_eCode != RICON_ELifecycleResult.OK && reply.m_iServerNowMs == -1 && reply.m_iDeadlineMs == -1;
		if (reply.m_eCode != RICON_ELifecycleResult.OK && reply.m_eCode != RICON_ELifecycleResult.DUPLICATE) return false;
		if (reply.m_iServerNowMs < 0 || reply.m_iDeadlineMs <= reply.m_iServerNowMs || reply.m_iDeadlineMs >= int.MAX) return false;
		return reply.m_iDeadlineMs - reply.m_iServerNowMs <= RICON_GameRadioPttOwner.MAX_SERVER_TTL_MS;
	}

	static array<int> EncodeReply(RICON_GameRadioRenewReply reply)
	{
		if (!ValidReply(reply)) return null;
		array<int> fields = {VERSION, reply.m_iRequestId};
		RICON_GameRadioPttWire.AppendContext(fields, reply.m_Context);
		fields.Insert(reply.m_iHoldToken); fields.Insert(reply.m_iRequestSeq); fields.Insert(reply.m_iLeaseId);
		fields.Insert(reply.m_eCode);
		int accepted = 0;
		if (reply.m_bAccepted) accepted = 1;
		fields.Insert(accepted); fields.Insert(reply.m_iServerNowMs); fields.Insert(reply.m_iDeadlineMs);
		return fields;
	}

	static RICON_GameRadioRenewReply DecodeReply(string domain, array<int> fields)
	{
		if (!fields || fields.Count() != REPLY_FIELDS || fields[0] != VERSION) return null;
		if (fields[12] != 0 && fields[12] != 1) return null;
		RICON_GameRadioRenewReply reply = new RICON_GameRadioRenewReply();
		reply.m_iRequestId = fields[1]; reply.m_Context = RICON_GameRadioPttWire.ReadContext(domain, fields, 2);
		reply.m_iHoldToken = fields[8]; reply.m_iRequestSeq = fields[9]; reply.m_iLeaseId = fields[10];
		reply.m_eCode = fields[11]; reply.m_bAccepted = fields[12] == 1;
		reply.m_iServerNowMs = fields[13]; reply.m_iDeadlineMs = fields[14];
		if (!ValidReply(reply)) return null;
		return reply;
	}
}
