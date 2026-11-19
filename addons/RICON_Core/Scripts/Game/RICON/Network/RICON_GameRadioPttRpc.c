//! Additive owner-controller control RPC. It never opens capture or changes stock VON state.
modded class SCR_PlayerController
{
	protected IEntity m_RICON_RadioPttEntity;
	protected ref RICON_GameRadioPttContext m_RICON_RadioPttContext;
	protected ref RICON_GameRadioPttReply m_RICON_RadioContextReply, m_RICON_RadioControlReply, m_RICON_RadioPttReply;
	protected ref ScriptInvoker m_RICON_RadioPttReplies = new ScriptInvoker();
	protected ref RICON_GameRadioPttClient m_RICON_RadioIntent = new RICON_GameRadioPttClient();
	protected ref RICON_GameRadioPttContext m_RICON_RadioPendingContext, m_RICON_RadioCancelContext;
	protected int m_iRICON_RadioSeqPending, m_iRICON_CancelHold, m_iRICON_CancelSeq, m_iRICON_CancelTarget;
	protected ref array<int> m_RICON_CancelRequests = {};
	protected ref ScriptInvoker m_RICON_RadioCancelReplies = new ScriptInvoker();
	protected ref RICON_GameRadioRenewRequest m_RICON_RadioRenewPending;
	protected ref ScriptInvoker m_RICON_RadioRenewReplies = new ScriptInvoker();
	protected int m_iRICON_RadioRenewSentAt = -1;
	protected ref RICON_GameRadioPttContext m_RICON_Ts3GameContext;
	protected int m_iRICON_Ts3ContextLast, m_iRICON_Ts3ContextReady, m_iRICON_Ts3ContextSentAt = -1;
	protected static const int RICON_TS3_CONTEXT_AGE_MS = 5000;
	protected ref RICON_GameTs3BindingClaim m_RICON_Ts3Claim;
	protected ref RICON_GameTs3PendingBinding m_RICON_Ts3Pending;
	protected int m_iRICON_Ts3LastRequest, m_iRICON_Ts3SentAt = -1, m_iRICON_Ts3Budget;
	protected bool m_bRICON_Ts3Retried;
	protected int m_iRICON_RadioLastRequest, m_iRICON_RadioContextPending, m_iRICON_RadioControlPending, m_iRICON_RadioPttPending;
	protected int m_iRICON_RadioHoldPending, m_iRICON_RadioMethodPending, m_iRICON_RadioPttSentAt = -1, m_iRICON_RadioReplySentAt = -1;

	// Local to this modded layer so it does not depend on another layer's protected helper declaration order.
	protected bool RICON_IsLocalRadioPttOwner()
	{
		if (Replication.IsServer() || !Replication.IsClient() || RplSession.Mode() != RplMode.Client
			|| !GetGame() || GetGame().GetPlayerController() != this || GetPlayerId() <= 0) return false;
		RplId item = Replication.FindItemId(this);
		if (!item.IsValid()) return false;
		RplNode node = Replication.FindNode(item);
		return node && node.IsOwner();
	}

	protected void RICON_ClearRadioPttClient()
	{
		m_RICON_RadioIntent.Clear();
		RICON_ClearRadioRenewPending();
		RICON_ClearRadioCancelPending(); m_RICON_RadioPendingContext = null; m_iRICON_RadioSeqPending = 0;
		m_RICON_RadioPttContext = null; m_RICON_RadioContextReply = null; m_RICON_RadioControlReply = null; m_RICON_RadioPttReply = null;
		m_iRICON_RadioContextPending = 0; m_iRICON_RadioControlPending = 0; m_iRICON_RadioPttPending = 0;
		m_iRICON_RadioHoldPending = 0; m_iRICON_RadioMethodPending = 0; m_iRICON_RadioLastRequest = 0;
		m_iRICON_RadioPttSentAt = -1; m_iRICON_RadioReplySentAt = -1;
	}

	protected bool RICON_RefreshRadioPttClient()
	{
		if (!RICON_IsLocalRadioPttOwner())
		{
			RICON_ClearRadioPttClient(); RICON_ClearTs3GameContext();
			m_RICON_RadioPttEntity = null;
			return false;
		}
		IEntity current = GetControlledEntity();
		if (current != m_RICON_RadioPttEntity)
		{
			RICON_ClearRadioPttClient();
			RICON_ClearTs3GameContext();
			m_RICON_RadioPttEntity = current;
		}
		return current != null;
	}

	bool RICON_RequestRadioContext(int requestId)
	{
		if (!RICON_RefreshRadioPttClient() || requestId <= m_iRICON_RadioLastRequest) return false;
		m_iRICON_RadioLastRequest = requestId; m_iRICON_RadioContextPending = requestId;
		Rpc(RpcAsk_RICON_RadioContext, requestId);
		return true;
	}

	protected void RICON_ClearTs3Request()
	{
		m_RICON_Ts3Claim = null;
		m_RICON_Ts3Pending = null;
		m_iRICON_Ts3SentAt = -1;
		m_iRICON_Ts3Budget = 0;
		m_bRICON_Ts3Retried = false;
	}

	void RICON_ClearTs3GameContext()
	{
		m_RICON_Ts3GameContext = null;
		m_iRICON_Ts3ContextReady = 0;
		m_iRICON_Ts3ContextSentAt = -1;
		RICON_ClearTs3Request();
	}

	bool RICON_RequestTs3GameContext(int queryId)
	{
		if (!RICON_RefreshRadioPttClient() || queryId <= m_iRICON_Ts3ContextLast) return false;
		int nowMs = System.GetTickCount();
		if (nowMs < 0) return false;
		RICON_ClearTs3Request();
		m_iRICON_Ts3ContextLast = queryId;
		m_iRICON_Ts3ContextReady = 0;
		m_iRICON_Ts3ContextSentAt = nowMs;
		Rpc(RpcAsk_RICON_Ts3GameContext, queryId);
		return true;
	}

	protected bool RICON_Ts3ContextRequestLive(int queryId)
	{
		int nowMs = System.GetTickCount();
		return queryId > 0 && queryId == m_iRICON_Ts3ContextLast && m_iRICON_Ts3ContextSentAt >= 0
			&& nowMs >= m_iRICON_Ts3ContextSentAt && nowMs - m_iRICON_Ts3ContextSentAt < RICON_TS3_CONTEXT_AGE_MS;
	}

	RICON_GameRadioPttContext RICON_ReadTs3GameContext(int queryId)
	{
		if (!RICON_RefreshRadioPttClient() || !m_RICON_Ts3GameContext || queryId != m_iRICON_Ts3ContextReady
			|| !RICON_Ts3ContextRequestLive(queryId)) return null;
		return m_RICON_Ts3GameContext.CloneContext();
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RICON_Ts3GameContext(int queryId)
	{
		if (!Replication.IsServer() || queryId <= 0) return;
		RICON_GameRadioManagerComponent manager = RICON_GameRadioManagerComponent.Cast(RICON_CommsManagerComponent.GetInstance());
		if (!manager) return;
		ref RICON_GameRadioPttReply reply = manager.ReadGameRadioPttContext(this, queryId);
		if (!reply || !manager.AuthorizeGameRadioPttReply(this, reply) || RICON_CommsManagerComponent.GetInstance() != manager) return;
		array<int> encoded = RICON_GameRadioPttWire.EncodeReply(reply);
		if (!encoded) return;
		string domain;
		if (reply.m_Context) domain = reply.m_Context.m_sDomain;
		Rpc(RpcDo_RICON_Ts3GameContext, domain, encoded);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_RICON_Ts3GameContext(string domain, array<int> fields)
	{
		if (!RICON_RefreshRadioPttClient() || m_iRICON_Ts3ContextReady != 0) return;
		ref RICON_GameRadioPttReply reply = RICON_GameRadioPttWire.DecodeReply(domain, fields);
		if (!reply || reply.m_iKind != RICON_GameRadioPttReply.CONTEXT || !RICON_Ts3ContextRequestLive(reply.m_iRequestId)) return;
		if (reply.m_eCode != RICON_ELifecycleResult.OK || !reply.m_Context || reply.m_Context.m_iPlayerId != GetPlayerId())
		{
			RICON_ClearTs3GameContext();
			return;
		}
		if (!m_RICON_Ts3GameContext || !m_RICON_Ts3GameContext.Same(reply.m_Context)) RICON_ClearTs3Request();
		m_RICON_Ts3GameContext = reply.m_Context.CloneContext();
		m_iRICON_Ts3ContextReady = reply.m_iRequestId;
	}

	bool RICON_RequestTs3Binding(int requestId, int ts3Revision, string serverUid, string clientUid)
	{
		ref RICON_GameRadioPttContext current = RICON_ReadTs3GameContext(requestId);
		if (!current || requestId < m_iRICON_Ts3LastRequest) return false;
		ref RICON_GameTs3BindingClaim claim = new RICON_GameTs3BindingClaim();
		claim.m_Context = current; claim.m_iRequestId = requestId; claim.m_iTs3Revision = ts3Revision;
		claim.m_sServerUid = serverUid; claim.m_sClientUid = clientUid;
		array<int> fields = RICON_GameTs3BindingWire.EncodeClaim(claim);
		int sentAt = System.GetTickCount();
		if (!fields || sentAt < 0) return false;
		if (requestId == m_iRICON_Ts3LastRequest)
		{
			if (!claim.Same(m_RICON_Ts3Claim) || m_bRICON_Ts3Retried || sentAt < m_iRICON_Ts3SentAt) return false;
			int elapsedMs = sentAt - m_iRICON_Ts3SentAt;
			if (elapsedMs < 250 || elapsedMs >= RICON_GameTs3BindingStore.TTL_MS) return false;
			m_bRICON_Ts3Retried = true;
			Rpc(RpcAsk_RICON_Ts3Binding, current.m_sDomain, serverUid, clientUid, fields);
			return true;
		}
		RICON_ClearTs3Request();
		m_iRICON_Ts3LastRequest = requestId;
		m_iRICON_Ts3SentAt = sentAt;
		m_RICON_Ts3Claim = claim;
		Rpc(RpcAsk_RICON_Ts3Binding, current.m_sDomain, serverUid, clientUid, fields);
		return true;
	}

	//! Read the owner's unverified association receipt, with a conservative local TTL.
	RICON_GameTs3PendingBinding RICON_ReadTs3PendingBinding(out int remainingMs)
	{
		remainingMs = 0;
		if (!RICON_RefreshRadioPttClient()) return null;
		ref RICON_GameRadioPttContext current = m_RICON_Ts3GameContext;
		if (!m_RICON_Ts3Pending) return null;
		int nowMs = System.GetTickCount();
		if (!current || !current.Same(m_RICON_Ts3Pending.m_Claim.m_Context)
			|| nowMs < m_iRICON_Ts3SentAt || nowMs - m_iRICON_Ts3SentAt >= m_iRICON_Ts3Budget)
		{
			RICON_ClearTs3Request();
			return null;
		}
		remainingMs = m_iRICON_Ts3Budget - (nowMs - m_iRICON_Ts3SentAt);
		return m_RICON_Ts3Pending.CloneBinding();
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RICON_Ts3Binding(string domain, string serverUid, string clientUid, array<int> fields)
	{
		if (!Replication.IsServer()) return;
		RICON_GameRadioManagerComponent manager = RICON_GameRadioManagerComponent.Cast(RICON_CommsManagerComponent.GetInstance());
		if (!manager) return;
		ref RICON_GameTs3BindingClaim claim = RICON_GameTs3BindingWire.DecodeClaim(domain, serverUid, clientUid, fields);
		if (!claim) return;
		int serverNowMs;
		ref RICON_GameTs3PendingBinding pending = manager.ProcessGameTs3Claim(this, claim, serverNowMs);
		if (!pending || RICON_CommsManagerComponent.GetInstance() != manager) return;
		array<int> encoded = RICON_GameTs3BindingWire.EncodePending(pending, serverNowMs);
		if (encoded) Rpc(RpcDo_RICON_Ts3Binding, domain, serverUid, clientUid, pending.m_sBindingId, encoded);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_RICON_Ts3Binding(string domain, string serverUid, string clientUid, string bindingId, array<int> fields)
	{
		if (!RICON_RefreshRadioPttClient()) return;
		ref RICON_GameRadioPttContext current = m_RICON_Ts3GameContext;
		if (!current || !m_RICON_Ts3Claim || !current.Same(m_RICON_Ts3Claim.m_Context)) return;
		int serverNowMs;
		ref RICON_GameTs3PendingBinding pending = RICON_GameTs3BindingWire.DecodePending(domain, serverUid, clientUid, bindingId, fields, serverNowMs);
		if (!pending || !pending.m_Claim.Same(m_RICON_Ts3Claim)) return;
		m_RICON_Ts3Claim = null;
		int nowMs = System.GetTickCount();
		int budget = pending.m_iDeadlineMs - serverNowMs;
		if (nowMs < m_iRICON_Ts3SentAt || nowMs - m_iRICON_Ts3SentAt >= budget) return;
		m_iRICON_Ts3Budget = budget;
		m_RICON_Ts3Pending = pending;
	}

	bool RICON_RequestRadioControl(int requestId, RplId radioId, int index)
	{
		if (!RICON_RefreshRadioPttClient() || !m_RICON_RadioPttContext || !radioId.IsValid()
			|| requestId <= m_iRICON_RadioLastRequest) return false;
		array<int> fields = RICON_GameRadioPttWire.EncodeControl(requestId, m_RICON_RadioPttContext, index);
		if (!fields) return false;
		m_iRICON_RadioLastRequest = requestId; m_iRICON_RadioControlPending = requestId;
		Rpc(RpcAsk_RICON_RadioControl, m_RICON_RadioPttContext.m_sDomain, fields, radioId);
		return true;
	}

	//! Local cancellation survives failed STOP encoding and preserves pending START correlation.
	void RICON_ReleaseRadioPttHold(int holdToken)
	{
		if (holdToken <= 0) return;
		m_RICON_RadioIntent.Release(holdToken);
		if (m_RICON_RadioRenewPending && m_RICON_RadioRenewPending.m_iHoldToken == holdToken)
			RICON_ClearRadioRenewPending();
		if (m_RICON_RadioPttReply && m_RICON_RadioPttReply.m_iHoldToken == holdToken)
			m_RICON_RadioPttReply = m_RICON_RadioIntent.Read(m_RICON_RadioPttReply, System.GetTickCount(), m_iRICON_RadioReplySentAt);
	}

	bool RICON_RequestRadioPtt(int requestId, int holdToken, int method, int deviceHint, RplId radioId,
		int index, int leaseId, int requestSeq)
	{
		if (method == RICON_ELifecycleMethod.PTT_STOP) RICON_ReleaseRadioPttHold(holdToken);
		if (!RICON_RefreshRadioPttClient() || !m_RICON_RadioPttContext || requestId <= m_iRICON_RadioLastRequest) return false;
		if (m_RICON_RadioCancelContext) return false;
		RICON_GameRadioPttRequest request = new RICON_GameRadioPttRequest();
		request.m_iRequestId = requestId; request.m_iHoldToken = holdToken; request.m_eMethod = method;
		request.m_Expected = m_RICON_RadioPttContext.CloneContext(); request.m_iRequestSeq = requestSeq;
		if (method == RICON_ELifecycleMethod.PTT_STOP)
		{
			request.m_iDeviceHint = 0; request.m_iIndex = -1; request.m_iLeaseId = leaseId;
			radioId = RplId.Invalid();
		}
		else
		{
			if (!radioId.IsValid()) return false;
			request.m_iDeviceHint = deviceHint; request.m_iIndex = index; request.m_iLeaseId = leaseId;
		}
		request.m_Selector = new RICON_GameRadioPttNativeSelector(radioId);
		array<int> fields = RICON_GameRadioPttWire.EncodeRequest(request);
		int sentAt = System.GetTickCount();
		if (!fields || sentAt < 0) return false;
		if (method == RICON_ELifecycleMethod.PTT_START)
		{
			if (!m_RICON_RadioIntent.StartIntent(requestId, holdToken, requestSeq)) return false;
			RICON_ClearRadioRenewPending();
		}
		m_iRICON_RadioLastRequest = requestId; m_iRICON_RadioPttPending = requestId;
		m_iRICON_RadioHoldPending = holdToken; m_iRICON_RadioMethodPending = method; m_iRICON_RadioPttSentAt = sentAt;
		m_iRICON_RadioSeqPending = requestSeq; m_RICON_RadioPendingContext = request.m_Expected.CloneContext();
		Rpc(RpcAsk_RICON_RadioPtt, request.m_Expected.m_sDomain, fields, radioId);
		return true;
	}

	protected void RICON_ClearRadioCancelPending()
	{
		m_RICON_RadioCancelContext = null; m_RICON_CancelRequests.Clear();
		m_iRICON_CancelHold = 0; m_iRICON_CancelSeq = 0; m_iRICON_CancelTarget = 0;
	}

	bool RICON_RequestRadioCancel(int requestId, int holdToken, int requestSeq, int cancelStartSeq, RICON_GameRadioPttContext expected)
	{
		RICON_ReleaseRadioPttHold(holdToken);
		if (!RICON_RefreshRadioPttClient() || !m_RICON_RadioPttContext || !expected || !expected.Same(m_RICON_RadioPttContext)) return false;
		if (requestId <= m_iRICON_RadioLastRequest || m_RICON_CancelRequests.Count() >= 3) return false;
		RICON_GameRadioCancelRequest request = new RICON_GameRadioCancelRequest();
		request.m_iRequestId = requestId; request.m_iHoldToken = holdToken; request.m_iRequestSeq = requestSeq;
		request.m_iCancelStartSeq = cancelStartSeq; request.m_Expected = expected.CloneContext();
		array<int> fields = RICON_GameRadioCancelWire.EncodeRequest(request);
		if (!fields) return false;
		if (m_RICON_RadioCancelContext)
		{
			if (!m_RICON_RadioCancelContext.Same(expected) || m_iRICON_CancelHold != holdToken
				|| m_iRICON_CancelSeq != requestSeq || m_iRICON_CancelTarget != cancelStartSeq) return false;
		}
		else
		{
			m_RICON_RadioCancelContext = expected.CloneContext(); m_iRICON_CancelHold = holdToken;
			m_iRICON_CancelSeq = requestSeq; m_iRICON_CancelTarget = cancelStartSeq;
		}
		m_iRICON_RadioLastRequest = requestId; m_RICON_CancelRequests.Insert(requestId);
		Rpc(RpcAsk_RICON_RadioCancel, expected.m_sDomain, fields);
		return true;
	}
	ScriptInvoker RICON_GetRadioCancelReplyInvoker() { return m_RICON_RadioCancelReplies; }

	protected void RICON_ClearRadioRenewPending()
	{
		m_RICON_RadioRenewPending = null;
		m_iRICON_RadioRenewSentAt = -1;
	}

	bool RICON_RequestRadioRenew(int requestId, int holdToken, int leaseId, int requestSeq, RICON_GameRadioPttContext expected)
	{
		RICON_GameRadioPttReply held = RICON_ReadHeldRadioPttStart();
		if (!held || !held.m_Context.Same(expected) || held.m_iLeaseId != leaseId || held.m_iHoldToken != holdToken) return false;
		if (m_RICON_RadioCancelContext || requestId <= m_iRICON_RadioLastRequest || requestSeq <= m_iRICON_RadioSeqPending) return false;
		if (m_RICON_RadioRenewPending && requestSeq < m_RICON_RadioRenewPending.m_iRequestSeq) return false;
		ref RICON_GameRadioRenewRequest request = new RICON_GameRadioRenewRequest();
		request.m_iRequestId = requestId; request.m_iHoldToken = holdToken; request.m_iLeaseId = leaseId;
		request.m_iRequestSeq = requestSeq; request.m_Expected = expected.CloneContext();
		array<int> fields = RICON_GameRadioRenewWire.EncodeRequest(request);
		int sentAt = System.GetTickCount();
		if (!fields || sentAt < 0) return false;
		m_iRICON_RadioLastRequest = requestId;
		m_RICON_RadioRenewPending = request; m_iRICON_RadioRenewSentAt = sentAt;
		Rpc(RpcAsk_RICON_RadioRenew, request.m_Expected.m_sDomain, fields);
		return true;
	}

	ScriptInvoker RICON_GetRadioRenewReplyInvoker() { return m_RICON_RadioRenewReplies; }

	//! Retain the original acknowledged selection. The input owner checks its current deadline separately.
	RICON_GameRadioPttReply RICON_ReadHeldRadioPttStart()
	{
		if (!RICON_RefreshRadioPttClient()) return null;
		return m_RICON_RadioIntent.ReadHeldStart(m_RICON_RadioPttReply);
	}

	RICON_GameRadioPttContext RICON_ReadRadioPttContext()
	{
		if (!RICON_RefreshRadioPttClient() || !m_RICON_RadioPttContext) return null;
		return m_RICON_RadioPttContext.CloneContext();
	}

	//! Returns a bounded latest snapshot. An old positive result is never a fresh lease permission.
	RICON_GameRadioPttReply RICON_ReadLatestRadioPttReply(int kind)
	{
		if (!RICON_RefreshRadioPttClient()) return null;
		RICON_GameRadioPttReply supplied;
		if (kind == RICON_GameRadioPttReply.CONTEXT) supplied = m_RICON_RadioContextReply;
		else if (kind == RICON_GameRadioPttReply.RADIO) supplied = m_RICON_RadioControlReply;
		else if (kind == RICON_GameRadioPttReply.PTT) supplied = m_RICON_RadioPttReply;
		if (!supplied) return null;
		return m_RICON_RadioIntent.Read(supplied, System.GetTickCount(), m_iRICON_RadioReplySentAt);
	}

	ScriptInvoker RICON_GetRadioPttReplyInvoker() { return m_RICON_RadioPttReplies; }

	void RICON_AppendDevControl(array<string> lines)
	{
		if (!RICON_IsLocalRadioPttOwner() || m_RICON_RadioPttEntity != GetControlledEntity())
		{
			lines.Insert("Control replies: no current local owner binding");
			return;
		}
		lines.Insert(string.Format("Last reply  context=%1  radio=%2  PTT=%3", RICON_RadioDev.ReplyCode(m_RICON_RadioContextReply),
			RICON_RadioDev.ReplyCode(m_RICON_RadioControlReply), RICON_RadioDev.ReplyCode(m_RICON_RadioPttReply)));
		lines.Insert(string.Format("Pending  context=%1  radio=%2  PTT=%3  renew=%4  cancel tries=%5",
			m_iRICON_RadioContextPending, m_iRICON_RadioControlPending, m_iRICON_RadioPttPending,
			m_RICON_RadioRenewPending != null, m_RICON_CancelRequests.Count()));
		if (m_RICON_RadioControlReply)
			lines.Insert(string.Format("Last slot grant  radio=%1  slot=%2  network=%3  channel=%4  RX gen=%5",
				m_RICON_RadioControlReply.m_iDeviceId, m_RICON_RadioControlReply.m_iIndex, m_RICON_RadioControlReply.m_iNetworkId,
				m_RICON_RadioControlReply.m_iChannelId, m_RICON_RadioControlReply.m_iRxGeneration));
		if (m_RICON_RadioPttReply && m_RICON_RadioPttReply.m_bHasLease)
			lines.Insert(string.Format("Last lease %1  hold=%2  active=%3  deadline=%4 server ms", m_RICON_RadioPttReply.m_iLeaseId,
				m_RICON_RadioPttReply.m_iHoldToken, m_RICON_RadioPttReply.m_bActive, m_RICON_RadioPttReply.m_iDeadlineMs));
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RICON_RadioContext(int requestId)
	{
		if (!Replication.IsServer() || requestId <= 0) return;
		RICON_GameRadioManagerComponent manager = RICON_GameRadioManagerComponent.Cast(RICON_CommsManagerComponent.GetInstance());
		if (!manager) return;
		RICON_SendRadioPttReply(manager, manager.ReadGameRadioPttContext(this, requestId));
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RICON_RadioControl(string domain, array<int> fields, RplId radioId)
	{
		if (!Replication.IsServer()) return;
		RICON_GameRadioManagerComponent manager = RICON_GameRadioManagerComponent.Cast(RICON_CommsManagerComponent.GetInstance());
		if (!manager) return;
		RICON_SendRadioPttReply(manager, manager.ReadGameRadioPttControl(this, domain, fields, radioId));
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RICON_RadioPtt(string domain, array<int> fields, RplId radioId)
	{
		if (!Replication.IsServer()) return;
		RICON_GameRadioManagerComponent manager = RICON_GameRadioManagerComponent.Cast(RICON_CommsManagerComponent.GetInstance());
		if (!manager) return;
		RICON_SendRadioPttReply(manager, manager.ProcessGameRadioPtt(this, domain, fields, radioId));
	}

	protected void RICON_SendRadioPttReply(RICON_GameRadioManagerComponent manager, RICON_GameRadioPttReply reply)
	{
		ref RICON_GameRadioPttReply held = reply;
		if (!Replication.IsServer() || !manager || !reply || !manager.AuthorizeGameRadioPttReply(this, reply)
			|| RICON_CommsManagerComponent.GetInstance() != manager) return;
		array<int> fields = RICON_GameRadioPttWire.EncodeReply(reply);
		if (!fields || !reply.m_Context) return;
		RICON_RadioDev.TracePtt("SERVER_TX", reply);
		Rpc(RpcDo_RICON_RadioPttReply, reply.m_Context.m_sDomain, fields);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_RICON_RadioPttReply(string domain, array<int> fields)
	{
		if (!RICON_RefreshRadioPttClient()) return;
		RICON_GameRadioPttReply reply = RICON_GameRadioPttWire.DecodeReply(domain, fields);
		if (!reply || !reply.m_Context || reply.m_Context.m_iPlayerId != GetPlayerId()) return;
		if (reply.m_iKind == RICON_GameRadioPttReply.CONTEXT)
		{
			if (reply.m_iRequestId != m_iRICON_RadioContextPending) return;
			m_iRICON_RadioContextPending = 0; m_RICON_RadioContextReply = reply.CloneReply();
			if (reply.m_eCode == RICON_ELifecycleResult.OK && m_RICON_Ts3GameContext && !m_RICON_Ts3GameContext.Same(reply.m_Context))
				RICON_ClearTs3GameContext();
			bool changed = !m_RICON_RadioPttContext || !m_RICON_RadioPttContext.Same(reply.m_Context);
			if (changed && reply.m_eCode == RICON_ELifecycleResult.OK) RICON_ClearRadioCancelPending();
			if (changed || reply.m_eCode != RICON_ELifecycleResult.OK)
			{
				RICON_ClearRadioRenewPending();
				m_RICON_RadioControlReply = null; m_RICON_RadioPttReply = null;
				m_iRICON_RadioControlPending = 0; m_iRICON_RadioPttPending = 0;
				m_iRICON_RadioHoldPending = 0; m_iRICON_RadioMethodPending = 0;
				m_iRICON_RadioPttSentAt = -1; m_iRICON_RadioReplySentAt = -1;
			}
			if (reply.m_eCode == RICON_ELifecycleResult.OK) m_RICON_RadioPttContext = reply.m_Context.CloneContext();
			else m_RICON_RadioPttContext = null;
			m_RICON_RadioIntent.BindContext(m_RICON_RadioPttContext);
		}
		else
		{
			if (!m_RICON_RadioPttContext || !m_RICON_RadioPttContext.Same(reply.m_Context)) { RICON_ClearRadioPttClient(); return; }
			if (reply.m_iKind == RICON_GameRadioPttReply.RADIO)
			{
				if (reply.m_iRequestId != m_iRICON_RadioControlPending) return;
				m_iRICON_RadioControlPending = 0; m_RICON_RadioControlReply = reply.CloneReply();
			}
			else
			{
				if (reply.m_iRequestId != m_iRICON_RadioPttPending || reply.m_iHoldToken != m_iRICON_RadioHoldPending
					|| reply.m_iMethod != m_iRICON_RadioMethodPending) return;
				m_iRICON_RadioPttPending = 0; m_iRICON_RadioReplySentAt = m_iRICON_RadioPttSentAt;
				m_RICON_RadioPttReply = m_RICON_RadioIntent.Read(reply, System.GetTickCount(), m_iRICON_RadioReplySentAt);
			}
		}
		RICON_GameRadioPttReply exposed = RICON_ReadLatestRadioPttReply(reply.m_iKind);
		RICON_RadioDev.TracePtt("CLIENT_RX", exposed);
		if (exposed) m_RICON_RadioPttReplies.Invoke(exposed);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RICON_RadioCancel(string domain, array<int> fields)
	{
		if (!Replication.IsServer()) return;
		RICON_GameRadioManagerComponent manager = RICON_GameRadioManagerComponent.Cast(RICON_CommsManagerComponent.GetInstance());
		if (!manager) return;
		ref RICON_GameRadioCancelReply reply = manager.ProcessGameRadioCancel(this, domain, fields);
		if (!reply || !manager.AuthorizeGameRadioCancelReply(this, reply) || RICON_CommsManagerComponent.GetInstance() != manager) return;
		array<int> encoded = RICON_GameRadioCancelWire.EncodeReply(reply);
		if (encoded && reply.m_Context) Rpc(RpcDo_RICON_RadioCancelReply, reply.m_Context.m_sDomain, encoded);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_RICON_RadioCancelReply(string domain, array<int> fields)
	{
		if (!RICON_RefreshRadioPttClient()) return;
		ref RICON_GameRadioCancelReply reply = RICON_GameRadioCancelWire.DecodeReply(domain, fields);
		if (!reply || !reply.m_Context || reply.m_Context.m_iPlayerId != GetPlayerId() || !m_RICON_RadioCancelContext) return;
		if (!m_RICON_RadioCancelContext.Same(reply.m_Context) || !m_RICON_CancelRequests.Contains(reply.m_iRequestId)) return;
		if (m_iRICON_CancelHold != reply.m_iHoldToken || m_iRICON_CancelSeq != reply.m_iRequestSeq
			|| m_iRICON_CancelTarget != reply.m_iCancelStartSeq) return;
		if (m_RICON_RadioPttContext && !m_RICON_RadioPttContext.Same(reply.m_Context)) return;
		if (RICON_RadioDev.Enabled())
			PrintFormat("[RICON_DEV] CLIENT_RX CANCEL req=%1 hold=%2 lease=%3 result=%4 barrier=%5", reply.m_iRequestId, reply.m_iHoldToken,
				reply.m_iLeaseId, typename.EnumToString(RICON_ELifecycleResult, reply.m_eCode), reply.HasBarrierProof());
		if (reply.HasBarrierProof())
		{
			// Retire only this operation's original START, never another hold or a newer STOP.
			if (m_iRICON_RadioPttPending > 0 && m_iRICON_RadioMethodPending == RICON_ELifecycleMethod.PTT_START
				&& m_iRICON_RadioHoldPending == reply.m_iHoldToken && m_iRICON_RadioSeqPending == reply.m_iCancelStartSeq
				&& m_RICON_RadioPendingContext && m_RICON_RadioPendingContext.Same(reply.m_Context))
			{
				m_iRICON_RadioPttPending = 0; m_iRICON_RadioHoldPending = 0; m_iRICON_RadioMethodPending = 0;
				m_iRICON_RadioSeqPending = 0; m_iRICON_RadioPttSentAt = -1; m_RICON_RadioPendingContext = null;
			}
			RICON_ClearRadioCancelPending();
		}
		m_RICON_RadioCancelReplies.Invoke(reply.CloneReply());
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RICON_RadioRenew(string domain, array<int> fields)
	{
		if (!Replication.IsServer()) return;
		RICON_GameRadioManagerComponent manager = RICON_GameRadioManagerComponent.Cast(RICON_CommsManagerComponent.GetInstance());
		if (!manager) return;
		ref RICON_GameRadioRenewReply reply = manager.ProcessGameRadioRenew(this, domain, fields);
		if (!reply || !manager.AuthorizeGameRadioRenewReply(this, reply) || RICON_CommsManagerComponent.GetInstance() != manager) return;
		array<int> encoded = RICON_GameRadioRenewWire.EncodeReply(reply);
		if (encoded && reply.m_Context) Rpc(RpcDo_RICON_RadioRenewReply, reply.m_Context.m_sDomain, encoded);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_RICON_RadioRenewReply(string domain, array<int> fields)
	{
		RICON_GameRadioPttReply held = RICON_ReadHeldRadioPttStart();
		if (!held || !m_RICON_RadioRenewPending) return;
		ref RICON_GameRadioRenewReply reply = RICON_GameRadioRenewWire.DecodeReply(domain, fields);
		if (!reply || !reply.MatchesRequest(m_RICON_RadioRenewPending) || reply.m_Context.m_iPlayerId != GetPlayerId()) return;
		if (!held.m_Context.Same(reply.m_Context) || held.m_iHoldToken != reply.m_iHoldToken || held.m_iLeaseId != reply.m_iLeaseId) return;
		int sentAt = m_iRICON_RadioRenewSentAt;
		RICON_ClearRadioRenewPending();
		int localNow = System.GetTickCount();
		if (sentAt < 0 || localNow < sentAt) return;
		if (reply.m_bAccepted && localNow - sentAt >= reply.m_iDeadlineMs - reply.m_iServerNowMs) return;
		if (RICON_RadioDev.Enabled())
			PrintFormat("[RICON_DEV] CLIENT_RX RENEW req=%1 hold=%2 lease=%3 result=%4 accepted=%5 deadlineMs=%6", reply.m_iRequestId,
				reply.m_iHoldToken, reply.m_iLeaseId, typename.EnumToString(RICON_ELifecycleResult, reply.m_eCode), reply.m_bAccepted, reply.m_iDeadlineMs);
		m_RICON_RadioRenewReplies.Invoke(reply);
	}
}
