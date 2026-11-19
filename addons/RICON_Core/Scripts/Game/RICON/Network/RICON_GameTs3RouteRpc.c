//! Metadata-only source pull. The selected TS3 tab still requires an independent server match.
class RICON_GameTs3RouteQuery
{
	ref RICON_GameRadioPttContext m_Context;
	int m_iRequestId, m_iHoldToken, m_iStartRequestId, m_iLeaseId;

	bool IsValid()
	{
		return m_Context && m_Context.IsValid() && RICON_GameTs3PendingBinding.IsValidId(m_Context.m_sDomain)
			&& m_iRequestId > 0 && m_iHoldToken > 0 && m_iStartRequestId > 0 && m_iLeaseId > 0;
	}

	RICON_GameTs3RouteQuery CloneQuery()
	{
		RICON_GameTs3RouteQuery copy = new RICON_GameTs3RouteQuery();
		if (m_Context) copy.m_Context = m_Context.CloneContext();
		copy.m_iRequestId = m_iRequestId; copy.m_iHoldToken = m_iHoldToken;
		copy.m_iStartRequestId = m_iStartRequestId; copy.m_iLeaseId = m_iLeaseId;
		return copy;
	}

	bool MatchesHeld(RICON_GameRadioPttReply held)
	{
		if (!IsValid() || !held || !held.m_bProcessed || !held.m_bHasLease || !held.m_bActive) return false;
		if (held.m_iKind != RICON_GameRadioPttReply.PTT || held.m_iMethod != RICON_ELifecycleMethod.PTT_START) return false;
		bool successful = held.m_eCode == RICON_ELifecycleResult.OK;
		if (held.m_eCode == RICON_ELifecycleResult.DUPLICATE)
			successful = held.m_bHasCachedResult && held.m_eCachedCode == RICON_ELifecycleResult.OK;
		return successful && m_Context.Same(held.m_Context) && m_iHoldToken == held.m_iHoldToken
			&& m_iStartRequestId == held.m_iRequestId && m_iLeaseId == held.m_iLeaseId;
	}
}

class RICON_GameTs3RouteReply
{
	ref RICON_GameTs3RouteQuery m_Query;
	int m_iDeviceId, m_iIndex = -1, m_iTxGeneration, m_iNetworkId, m_iChannelId;
	int m_iBindingRequestId, m_iTs3Revision, m_iSourceClientId, m_iBudgetMs;
	string m_sBindingId, m_sServerUid, m_sSourceClientUid;
	ref array<string> m_RecipientUids = {};
	ref array<int> m_RecipientClientIds = {};

	bool IsValid()
	{
		if (!m_Query || !m_Query.IsValid() || m_iDeviceId <= 0 || m_iIndex < 0 || m_iTxGeneration <= 0) return false;
		if (m_iNetworkId <= 0 || m_iChannelId <= 0 || m_iBindingRequestId <= 0 || m_iTs3Revision <= 0) return false;
		if (m_iSourceClientId <= 0 || m_iSourceClientId > 65535 || m_iBudgetMs <= 0 || m_iBudgetMs > RICON_GameTs3RouteWire.MAX_AGE_MS) return false;
		if (!RICON_GameTs3PendingBinding.IsValidId(m_sBindingId) || !RICON_GameTs3BindingClaim.IsValidUid(m_sServerUid)
			|| !RICON_GameTs3BindingClaim.IsValidUid(m_sSourceClientUid)) return false;
		if (!m_RecipientUids || !m_RecipientClientIds || m_RecipientUids.IsEmpty()
			|| m_RecipientUids.Count() != m_RecipientClientIds.Count() || m_RecipientUids.Count() > RICON_GameTs3RouteWire.MAX_RECIPIENTS) return false;
		set<string> uids = new set<string>();
		set<int> clients = new set<int>();
		uids.Insert(m_sSourceClientUid); clients.Insert(m_iSourceClientId);
		for (int index = 0; index < m_RecipientUids.Count(); index++)
		{
			string uid = m_RecipientUids[index];
			int client = m_RecipientClientIds[index];
			if (!RICON_GameTs3BindingClaim.IsValidUid(uid) || client <= 0 || client > 65535) return false;
			if (uids.Contains(uid) || clients.Contains(client)) return false;
			uids.Insert(uid); clients.Insert(client);
		}
		return true;
	}

	RICON_GameTs3RouteReply CloneReply()
	{
		RICON_GameTs3RouteReply copy = new RICON_GameTs3RouteReply();
		if (m_Query) copy.m_Query = m_Query.CloneQuery();
		copy.m_iDeviceId = m_iDeviceId; copy.m_iIndex = m_iIndex; copy.m_iTxGeneration = m_iTxGeneration;
		copy.m_iNetworkId = m_iNetworkId; copy.m_iChannelId = m_iChannelId;
		copy.m_iBindingRequestId = m_iBindingRequestId; copy.m_iTs3Revision = m_iTs3Revision;
		copy.m_iSourceClientId = m_iSourceClientId; copy.m_iBudgetMs = m_iBudgetMs;
		copy.m_sBindingId = m_sBindingId; copy.m_sServerUid = m_sServerUid; copy.m_sSourceClientUid = m_sSourceClientUid;
		if (m_RecipientUids) copy.m_RecipientUids.InsertAll(m_RecipientUids);
		if (m_RecipientClientIds) copy.m_RecipientClientIds.InsertAll(m_RecipientClientIds);
		return copy;
	}
}

class RICON_GameTs3RouteWire
{
	static const int MAX_RECIPIENTS = 64, MAX_ROW_BYTES = 12288, MAX_AGE_MS = 500;
	static const string QUERY_HEADER = "RICON_TS3_ROUTE_QUERY_1";
	static const string RESPONSE_HEADER = "RICON_TS3_ROUTE_1";

	static array<int> EncodeQuery(RICON_GameTs3RouteQuery query)
	{
		if (!query || !query.IsValid()) return null;
		RICON_GameRadioPttContext c = query.m_Context;
		array<int> fields = {1, query.m_iRequestId, c.m_iEpoch, c.m_iPlayerId, c.m_iConnectionId, c.m_iGeneration,
			c.m_iEntityId, c.m_iPolicyRevision, query.m_iHoldToken, query.m_iStartRequestId, query.m_iLeaseId};
		return fields;
	}

	static RICON_GameTs3RouteQuery DecodeQuery(string domain, array<int> fields)
	{
		if (!fields || fields.Count() != 11 || fields[0] != 1) return null;
		RICON_GameTs3RouteQuery query = new RICON_GameTs3RouteQuery();
		query.m_Context = RICON_GameRadioPttWire.ReadContext(domain, fields, 2);
		query.m_iRequestId = fields[1]; query.m_iHoldToken = fields[8];
		query.m_iStartRequestId = fields[9]; query.m_iLeaseId = fields[10];
		if (!query.IsValid()) return null;
		return query;
	}

	static bool EncodeReply(RICON_GameTs3RouteReply reply, out array<string> names, out array<int> fields)
	{
		names = null; fields = null;
		if (!reply || !reply.IsValid()) return false;
		fields = EncodeQuery(reply.m_Query);
		fields.Insert(reply.m_iDeviceId); fields.Insert(reply.m_iIndex); fields.Insert(reply.m_iTxGeneration);
		fields.Insert(reply.m_iNetworkId); fields.Insert(reply.m_iChannelId);
		fields.Insert(reply.m_iBindingRequestId); fields.Insert(reply.m_iTs3Revision); fields.Insert(reply.m_iSourceClientId);
		fields.Insert(reply.m_iBudgetMs); fields.Insert(reply.m_RecipientUids.Count());
		fields.InsertAll(reply.m_RecipientClientIds);
		names = {reply.m_Query.m_Context.m_sDomain, reply.m_sBindingId, reply.m_sServerUid, reply.m_sSourceClientUid};
		names.InsertAll(reply.m_RecipientUids);
		return true;
	}

	static RICON_GameTs3RouteReply DecodeReply(array<string> names, array<int> fields)
	{
		if (!names || !fields || fields.Count() < 22 || names.Count() < 5) return null;
		int count = fields[20];
		if (count <= 0 || count > MAX_RECIPIENTS || fields.Count() != 21 + count || names.Count() != 4 + count) return null;
		array<int> header = {};
		for (int index = 0; index < 11; index++) header.Insert(fields[index]);
		RICON_GameTs3RouteReply reply = new RICON_GameTs3RouteReply();
		reply.m_Query = DecodeQuery(names[0], header);
		reply.m_iDeviceId = fields[11]; reply.m_iIndex = fields[12]; reply.m_iTxGeneration = fields[13];
		reply.m_iNetworkId = fields[14]; reply.m_iChannelId = fields[15];
		reply.m_iBindingRequestId = fields[16]; reply.m_iTs3Revision = fields[17]; reply.m_iSourceClientId = fields[18];
		reply.m_iBudgetMs = fields[19]; reply.m_sBindingId = names[1]; reply.m_sServerUid = names[2]; reply.m_sSourceClientUid = names[3];
		for (int receiver = 0; receiver < count; receiver++)
		{
			reply.m_RecipientUids.Insert(names[4 + receiver]); reply.m_RecipientClientIds.Insert(fields[21 + receiver]);
		}
		if (!reply.IsValid()) return null;
		return reply;
	}

	static RICON_GameTs3RouteReply Build(RICON_GameTs3RouteQuery query, RICON_GameTs3RecipientSnapshot snapshot)
	{
		if (!query || !query.IsValid() || !snapshot || !snapshot.m_Source || !snapshot.m_Source.m_Claim) return null;
		RICON_GameTs3MatchedBinding source = snapshot.m_Source;
		if (!query.m_Context.Same(source.m_Claim.m_Context) || query.m_iLeaseId != snapshot.m_iLeaseId) return null;
		if (snapshot.m_iSampleMs < 0 || snapshot.m_iDeadlineMs <= snapshot.m_iSampleMs || snapshot.m_iDeadlineMs >= int.MAX) return null;
		if (!snapshot.m_Recipients || snapshot.m_Recipients.IsEmpty() || snapshot.m_Recipients.Count() > MAX_RECIPIENTS) return null;
		RICON_GameTs3RouteReply reply = new RICON_GameTs3RouteReply();
		reply.m_Query = query.CloneQuery();
		reply.m_iDeviceId = snapshot.m_iTxDeviceId; reply.m_iIndex = snapshot.m_iTxTransceiverIndex;
		reply.m_iTxGeneration = snapshot.m_iTxGeneration; reply.m_iNetworkId = snapshot.m_iNetworkId; reply.m_iChannelId = snapshot.m_iChannelId;
		reply.m_iBindingRequestId = source.m_Claim.m_iRequestId; reply.m_iTs3Revision = source.m_Claim.m_iTs3Revision;
		reply.m_iSourceClientId = source.m_iObservedClientId; reply.m_sBindingId = source.m_sBindingId;
		reply.m_sServerUid = source.m_Claim.m_sServerUid; reply.m_sSourceClientUid = source.m_Claim.m_sClientUid;
		reply.m_iBudgetMs = Math.Min(MAX_AGE_MS, snapshot.m_iDeadlineMs - snapshot.m_iSampleMs);
		foreach (RICON_GameTs3MatchedBinding recipient : snapshot.m_Recipients)
		{
			if (!recipient || !recipient.m_Claim || recipient.m_Claim.m_sServerUid != reply.m_sServerUid) return null;
			reply.m_RecipientUids.Insert(recipient.m_Claim.m_sClientUid); reply.m_RecipientClientIds.Insert(recipient.m_iObservedClientId);
		}
		if (!reply.IsValid()) return null;
		return reply;
	}

	static int Remaining(RICON_GameTs3RouteReply reply, int sentAt, int now)
	{
		if (!reply || !reply.IsValid() || sentAt < 0 || now < sentAt || now - sentAt >= reply.m_iBudgetMs) return 0;
		return reply.m_iBudgetMs - (now - sentAt);
	}

	static string Row(string nonce, int sequence, int session, RICON_GameTs3RouteReply reply, int remainingMs)
	{
		string checkedNonce;
		int checkedSequence;
		if (session <= 0 || !RICON_TS3GameBridge.ParseRequest(QUERY_HEADER + " " + nonce + " " + sequence.ToString() + " END",
			checkedNonce, checkedSequence, QUERY_HEADER)) return "";
		string row = RESPONSE_HEADER + " " + nonce + " " + sequence.ToString() + " " + session.ToString();
		if (!reply || !reply.IsValid() || remainingMs <= 0 || remainingMs > reply.m_iBudgetMs) return row + " 0 END";
		RICON_GameTs3RouteQuery query = reply.m_Query;
		RICON_GameRadioPttContext c = query.m_Context;
		row += " 1 " + c.m_sDomain + " " + c.m_iEpoch.ToString() + " " + c.m_iPlayerId.ToString();
		row += " " + c.m_iConnectionId.ToString() + " " + c.m_iGeneration.ToString() + " " + c.m_iEntityId.ToString();
		row += " " + c.m_iPolicyRevision.ToString() + " " + reply.m_iDeviceId.ToString() + " " + reply.m_iIndex.ToString();
		row += " " + reply.m_iTxGeneration.ToString() + " " + reply.m_iNetworkId.ToString() + " " + reply.m_iChannelId.ToString();
		row += " " + query.m_iHoldToken.ToString() + " " + query.m_iStartRequestId.ToString() + " " + query.m_iLeaseId.ToString();
		row += " " + reply.m_sBindingId + " " + reply.m_iBindingRequestId.ToString() + " " + reply.m_iTs3Revision.ToString();
		row += " " + reply.m_sServerUid + " " + reply.m_sSourceClientUid + " " + reply.m_iSourceClientId.ToString();
		row += " " + remainingMs.ToString() + " " + reply.m_RecipientUids.Count().ToString();
		for (int index = 0; index < reply.m_RecipientUids.Count(); index++)
			row += " " + reply.m_RecipientUids[index] + " " + reply.m_RecipientClientIds[index].ToString();
		row += " END";
		if (row.Length() + 2 > MAX_ROW_BYTES) return "";
		return row;
	}
}

modded class SCR_PlayerController
{
	protected ref RICON_GameTs3RouteQuery m_RICON_Ts3RouteQuery, m_RICON_Ts3RouteServerQuery;
	protected ref RICON_GameTs3RouteReply m_RICON_Ts3RouteReply;
	protected int m_iRICON_Ts3RouteCounter, m_iRICON_Ts3RouteServerLast, m_iRICON_Ts3RouteSentAt = -1;
	protected int m_iRICON_Ts3RouteServerAt = -1;

	protected bool RICON_IsTs3RouteLocalOwner()
	{
		if (!GetGame() || GetGame().GetPlayerController() != this || GetPlayerId() <= 0) return false;
		if (Replication.IsServer() || !Replication.IsClient() || RplSession.Mode() != RplMode.Client) return false;
		RplId actor = Replication.FindItemId(this);
		if (!actor.IsValid()) return false;
		RplNode node = Replication.FindNode(actor);
		return node && node.IsOwner();
	}

	protected RICON_GameRadioPttReply RICON_Ts3RouteHeld(out int remaining)
	{
		remaining = 0;
		if (!RICON_TS3GameBridge.RouteRequested() || !RICON_IsTs3RouteLocalOwner()) return null;
		SCR_VONController hook = SCR_VONController.Cast(FindComponent(SCR_VONController));
		if (!hook) return null;
		return hook.RICON_ReadPttMetadata(remaining);
	}

	int RICON_RequestTs3Route()
	{
		int remaining;
		RICON_GameRadioPttReply held = RICON_Ts3RouteHeld(remaining);
		if (!held || remaining <= 0 || m_iRICON_Ts3RouteCounter == int.MAX) return 0;
		int now = System.GetTickCount();
		if (now < 0) return 0;
		if (m_RICON_Ts3RouteQuery && now >= m_iRICON_Ts3RouteSentAt && now - m_iRICON_Ts3RouteSentAt < 100) return 0;
		RICON_GameTs3RouteQuery query = new RICON_GameTs3RouteQuery();
		query.m_Context = held.m_Context.CloneContext(); query.m_iRequestId = m_iRICON_Ts3RouteCounter + 1;
		query.m_iHoldToken = held.m_iHoldToken; query.m_iStartRequestId = held.m_iRequestId; query.m_iLeaseId = held.m_iLeaseId;
		array<int> fields = RICON_GameTs3RouteWire.EncodeQuery(query);
		if (!fields || !query.MatchesHeld(held)) return 0;
		m_iRICON_Ts3RouteCounter = query.m_iRequestId; m_iRICON_Ts3RouteSentAt = now;
		m_RICON_Ts3RouteQuery = query; m_RICON_Ts3RouteReply = null;
		Rpc(RpcAsk_RICON_Ts3Route, query.m_Context.m_sDomain, fields);
		return query.m_iRequestId;
	}

	RICON_GameTs3RouteReply RICON_ReadTs3Route(int requestId, out int remaining)
	{
		remaining = 0;
		int pttRemaining;
		RICON_GameRadioPttReply held = RICON_Ts3RouteHeld(pttRemaining);
		if (!m_RICON_Ts3RouteReply || !held || pttRemaining <= 0) return null;
		RICON_GameTs3RouteReply reply = m_RICON_Ts3RouteReply;
		if (reply.m_Query.m_iRequestId != requestId || !reply.m_Query.MatchesHeld(held)) return null;
		if (held.m_iDeviceId != reply.m_iDeviceId || held.m_iIndex != reply.m_iIndex || held.m_iRxGeneration != reply.m_iTxGeneration
			|| held.m_iNetworkId != reply.m_iNetworkId || held.m_iChannelId != reply.m_iChannelId) return null;
		int pendingRemaining;
		RICON_GameTs3PendingBinding pending = RICON_ReadTs3PendingBinding(pendingRemaining);
		if (!pending || pendingRemaining <= 0 || !pending.m_Claim || !pending.m_Claim.m_Context.Same(reply.m_Query.m_Context)) return null;
		if (pending.m_sBindingId != reply.m_sBindingId || pending.m_Claim.m_iRequestId != reply.m_iBindingRequestId
			|| pending.m_Claim.m_iTs3Revision != reply.m_iTs3Revision || pending.m_Claim.m_sServerUid != reply.m_sServerUid
			|| pending.m_Claim.m_sClientUid != reply.m_sSourceClientUid) return null;
		remaining = Math.Min(pttRemaining, Math.Min(pendingRemaining,
			RICON_GameTs3RouteWire.Remaining(reply, m_iRICON_Ts3RouteSentAt, System.GetTickCount())));
		if (remaining <= 0) return null;
		return reply.CloneReply();
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RICON_Ts3Route(string domain, array<int> fields)
	{
		if (!Replication.IsServer() || !RICON_TS3GameBridge.RouteRequested()) return;
		ref RICON_GameTs3RouteQuery query = RICON_GameTs3RouteWire.DecodeQuery(domain, fields);
		if (!query || query.m_iRequestId <= m_iRICON_Ts3RouteServerLast) return;
		RICON_GameRadioManagerComponent manager = RICON_GameRadioManagerComponent.Cast(RICON_CommsManagerComponent.GetInstance());
		int now;
		if (!manager || !manager.RICON_AcceptTs3RouteQuery(this, query, now)) return;
		m_iRICON_Ts3RouteServerLast = query.m_iRequestId; m_iRICON_Ts3RouteServerAt = now;
		m_RICON_Ts3RouteServerQuery = query;
	}

	void RICON_FulfillTs3Route(RICON_GameRadioManagerComponent manager, int leaseId, int now)
	{
		if (!Replication.IsServer() || !m_RICON_Ts3RouteServerQuery || !RICON_TS3GameBridge.RouteRequested()) return;
		if (m_RICON_Ts3RouteServerQuery.m_iLeaseId != leaseId) return;
		ref RICON_GameTs3RouteQuery query = m_RICON_Ts3RouteServerQuery;
		m_RICON_Ts3RouteServerQuery = null;
		if (now < m_iRICON_Ts3RouteServerAt || now - m_iRICON_Ts3RouteServerAt >= RICON_GameTs3RouteWire.MAX_AGE_MS) return;
		ref RICON_GameTs3RecipientSnapshot snapshot = manager.RICON_ReadTs3RouteQuery(this, query, now);
		ref RICON_GameTs3RouteReply reply = RICON_GameTs3RouteWire.Build(query, snapshot);
		if (RICON_RadioDev.Enabled())
		{
			int receivers, budget;
			if (reply) { receivers = reply.m_RecipientClientIds.Count(); budget = reply.m_iBudgetMs; }
			PrintFormat("[RICON_DEV] SERVER TS3_METADATA player=%1 req=%2 lease=%3 recipients=%4 budgetMs=%5",
				GetPlayerId(), query.m_iRequestId, leaseId, receivers, budget);
		}
		array<string> names;
		array<int> fields;
		if (RICON_GameTs3RouteWire.EncodeReply(reply, names, fields)) Rpc(RpcDo_RICON_Ts3Route, names, fields);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_RICON_Ts3Route(array<string> names, array<int> fields)
	{
		if (!RICON_IsTs3RouteLocalOwner() || !m_RICON_Ts3RouteQuery) return;
		ref RICON_GameTs3RouteReply reply = RICON_GameTs3RouteWire.DecodeReply(names, fields);
		if (!reply || reply.m_Query.m_iRequestId != m_RICON_Ts3RouteQuery.m_iRequestId) return;
		if (!reply.m_Query.m_Context.Same(m_RICON_Ts3RouteQuery.m_Context) || reply.m_Query.m_iHoldToken != m_RICON_Ts3RouteQuery.m_iHoldToken
			|| reply.m_Query.m_iStartRequestId != m_RICON_Ts3RouteQuery.m_iStartRequestId || reply.m_Query.m_iLeaseId != m_RICON_Ts3RouteQuery.m_iLeaseId) return;
		if (m_RICON_Ts3RouteReply || RICON_GameTs3RouteWire.Remaining(reply, m_iRICON_Ts3RouteSentAt, System.GetTickCount()) <= 0) return;
		m_RICON_Ts3RouteReply = reply;
		int remaining;
		if (!RICON_ReadTs3Route(reply.m_Query.m_iRequestId, remaining)) m_RICON_Ts3RouteReply = null;
	}
}
