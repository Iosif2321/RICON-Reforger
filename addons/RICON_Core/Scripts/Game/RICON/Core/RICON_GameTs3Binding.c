//! A claim names an observed TS3 connection. Its UID strings still require an independent proof.
class RICON_GameTs3BindingClaim
{
	ref RICON_GameRadioPttContext m_Context;
	int m_iRequestId, m_iTs3Revision;
	string m_sServerUid, m_sClientUid;

	static bool IsValidUid(string value)
	{
		int length = value.Length();
		if (length < 1 || length > 127) return false;
		for (int index = 0; index < length; index++)
		{
			int c = value.ToAscii(index);
			if (c >= 65 && c <= 90) continue;   // A-Z
			if (c >= 97 && c <= 122) continue;  // a-z
			if (c >= 48 && c <= 57) continue;   // 0-9
			if (c == 43 || c == 47 || c == 95 || c == 61 || c == 45) continue;  // +/_=-
			return false;
		}
		return true;
	}

	bool IsValid()
	{
		if (!m_Context || !m_Context.IsValid() || m_iRequestId <= 0 || m_iTs3Revision <= 0) return false;
		return IsValidUid(m_sServerUid) && IsValidUid(m_sClientUid);
	}

	bool Same(RICON_GameTs3BindingClaim other)
	{
		if (!other || !m_Context || !m_Context.Same(other.m_Context)) return false;
		return m_iRequestId == other.m_iRequestId && m_iTs3Revision == other.m_iTs3Revision
			&& m_sServerUid == other.m_sServerUid && m_sClientUid == other.m_sClientUid;
	}

	RICON_GameTs3BindingClaim CloneClaim()
	{
		RICON_GameTs3BindingClaim copy = new RICON_GameTs3BindingClaim();
		if (m_Context) copy.m_Context = m_Context.CloneContext();
		copy.m_iRequestId = m_iRequestId; copy.m_iTs3Revision = m_iTs3Revision;
		copy.m_sServerUid = m_sServerUid; copy.m_sClientUid = m_sClientUid;
		return copy;
	}
}

class RICON_GameTs3PendingBinding
{
	ref RICON_GameTs3BindingClaim m_Claim;
	string m_sBindingId;
	int m_iDeadlineMs;

	static bool IsValidId(string value)
	{
		if (value.Length() != 36 || !RICON_GameRadioControlPolicy.IsValidIdentity(value)) return false;
		string canonical = value; canonical.ToLower();
		if (value != canonical) return false;
		int variant = value.ToAscii(19);
		return variant == 56 || variant == 57 || variant == 97 || variant == 98;
	}

	RICON_GameTs3PendingBinding CloneBinding()
	{
		RICON_GameTs3PendingBinding copy = new RICON_GameTs3PendingBinding();
		if (m_Claim) copy.m_Claim = m_Claim.CloneClaim();
		copy.m_sBindingId = m_sBindingId; copy.m_iDeadlineMs = m_iDeadlineMs;
		return copy;
	}
}

//! Detached server-side identity match. It carries no challenge nonce or media permission.
class RICON_GameTs3MatchedBinding
{
	ref RICON_GameTs3BindingClaim m_Claim;
	string m_sBindingId;
	int m_iDeadlineMs, m_iObservedClientId;
}

class RICON_GameTs3Challenge
{
	ref RICON_GameTs3PendingBinding m_Pending;
	string m_sNonce;

	static bool IsValidNonce(string value)
	{
		if (value.Length() != 22) return false;
		for (int index = 0; index < 22; index++)
		{
			int ch = value.ToAscii(index);
			if (ch >= 65 && ch <= 90) continue;
			if (ch >= 97 && ch <= 122) continue;
			if ((ch >= 48 && ch <= 57) || ch == 45 || ch == 95) continue;
			return false;
		}
		return true;
	}
}

//! Supplied only by a trusted server reader of the selected TS3 connection.
class RICON_GameTs3Observation
{
	string m_sBindingId, m_sNonce, m_sServerUid, m_sClientUid;
	int m_iRequestId, m_iTs3Revision, m_iClientId;
}

//! Keep the request watermark for an expired claim while its authenticated context is current.
class RICON_GameTs3BindingRow
{
	ref RICON_GameRadioPttContext m_Context;
	int m_iLastRequest;
	ref RICON_GameTs3PendingBinding m_Pending;
	string m_sChallengeNonce;
	int m_iObservedClientId;
}

class RICON_GameTs3BindingStore
{
	static const int TTL_MS = 10000;
	protected ref map<int, ref RICON_GameTs3BindingRow> m_Rows = new map<int, ref RICON_GameTs3BindingRow>();
	protected string m_sDomain;
	protected int m_iEpoch, m_iMaxPlayers, m_iLastNowMs = -1;
	protected bool m_bClosed;

	bool Configure(string domain, int epoch, int maxPlayers)
	{
		if (m_bClosed || epoch <= 0 || epoch >= int.MAX || maxPlayers <= 0) return false;
		if (!RICON_GameTs3PendingBinding.IsValidId(domain)) return false;
		if (m_iEpoch > 0) return m_sDomain == domain && m_iEpoch == epoch && m_iMaxPlayers == maxPlayers;
		m_sDomain = domain; m_iEpoch = epoch; m_iMaxPlayers = maxPlayers;
		return true;
	}

	protected bool InScope(RICON_GameRadioPttContext context)
	{
		return !m_bClosed && m_iEpoch > 0 && context && context.IsValid()
			&& context.m_iEpoch == m_iEpoch && context.m_sDomain == m_sDomain;
	}

	protected bool ObserveTime(int nowMs)
	{
		if (m_bClosed || m_iEpoch <= 0 || nowMs < 0 || nowMs >= int.MAX) return false;
		if (nowMs < m_iLastNowMs)
		{
			Close();
			return false;
		}
		m_iLastNowMs = nowMs;
		return true;
	}

	protected void Expire(RICON_GameTs3BindingRow row, int nowMs)
	{
		if (row.m_Pending && nowMs >= row.m_Pending.m_iDeadlineMs)
		{
			row.m_Pending = null;
			row.m_sChallengeNonce = "";
			row.m_iObservedClientId = 0;
		}
	}

	protected bool IdInUse(string id, int nowMs)
	{
		foreach (int playerId, RICON_GameTs3BindingRow row : m_Rows)
		{
			if (!row.m_Pending || nowMs >= row.m_Pending.m_iDeadlineMs) continue;
			if (row.m_Pending.m_sBindingId == id) return true;
		}
		return false;
	}

	RICON_GameTs3PendingBinding Claim(RICON_GameTs3BindingClaim claim, RICON_GameRadioPttContext authoritative, int nowMs, string freshId)
	{
		if (!claim || !claim.IsValid() || !InScope(authoritative) || !claim.m_Context.Same(authoritative)) return null;
		if (!ObserveTime(nowMs)) return null;
		int playerId = authoritative.m_iPlayerId;
		ref RICON_GameTs3BindingRow row = m_Rows.Get(playerId);
		if (row && row.m_Context.Same(authoritative))
		{
			Expire(row, nowMs);
			if (claim.m_iRequestId < row.m_iLastRequest) return null;
			if (claim.m_iRequestId == row.m_iLastRequest)
			{
				if (!row.m_Pending || !row.m_Pending.m_Claim.Same(claim)) return null;
				return row.m_Pending.CloneBinding();
			}
		}
		if (!row && m_Rows.Count() >= m_iMaxPlayers) return null;
		if (nowMs >= int.MAX - TTL_MS || !RICON_GameTs3PendingBinding.IsValidId(freshId) || IdInUse(freshId, nowMs)) return null;

		ref RICON_GameTs3PendingBinding pending = new RICON_GameTs3PendingBinding();
		pending.m_Claim = claim.CloneClaim();
		pending.m_sBindingId = freshId; pending.m_iDeadlineMs = nowMs + TTL_MS;
		ref RICON_GameTs3BindingRow next = new RICON_GameTs3BindingRow();
		next.m_Context = authoritative.CloneContext(); next.m_iLastRequest = claim.m_iRequestId;
		next.m_Pending = pending;
		m_Rows.Set(playerId, next);
		return pending.CloneBinding();
	}

	RICON_GameTs3PendingBinding Read(RICON_GameRadioPttContext authoritative, int nowMs)
	{
		if (!InScope(authoritative) || !ObserveTime(nowMs)) return null;
		RICON_GameTs3BindingRow row = m_Rows.Get(authoritative.m_iPlayerId);
		if (!row || !row.m_Context.Same(authoritative)) return null;
		Expire(row, nowMs);
		if (!row.m_Pending) return null;
		return row.m_Pending.CloneBinding();
	}

	RICON_GameTs3MatchedBinding ReadMatched(RICON_GameRadioPttContext authoritative, int nowMs)
	{
		if (!InScope(authoritative)) return null;
		RICON_GameTs3BindingRow row = m_Rows.Get(authoritative.m_iPlayerId);
		if (!row || !row.m_Context.Same(authoritative) || !ObserveTime(nowMs)) return null;
		Expire(row, nowMs);
		if (!row.m_Pending || row.m_iObservedClientId <= 0) return null;
		RICON_GameTs3MatchedBinding matched = new RICON_GameTs3MatchedBinding();
		matched.m_Claim = row.m_Pending.m_Claim.CloneClaim();
		matched.m_sBindingId = row.m_Pending.m_sBindingId;
		matched.m_iDeadlineMs = row.m_Pending.m_iDeadlineMs;
		matched.m_iObservedClientId = row.m_iObservedClientId;
		return matched;
	}

	protected RICON_GameTs3BindingRow CurrentProofRow(RICON_GameRadioPttContext authoritative, string bindingId, int requestId, int nowMs)
	{
		if (!InScope(authoritative) || requestId <= 0 || !RICON_GameTs3PendingBinding.IsValidId(bindingId)) return null;
		RICON_GameTs3BindingRow row = m_Rows.Get(authoritative.m_iPlayerId);
		if (!row || !row.m_Context.Same(authoritative) || !row.m_Pending) return null;
		if (row.m_Pending.m_sBindingId != bindingId || row.m_iLastRequest != requestId) return null;
		if (!ObserveTime(nowMs)) return null;
		Expire(row, nowMs);
		if (!row.m_Pending) return null;
		return row;
	}

	protected bool ChallengeNonceInUse(string nonce, int nowMs)
	{
		foreach (int playerId, RICON_GameTs3BindingRow row : m_Rows)
		{
			if (row.m_Pending && nowMs < row.m_Pending.m_iDeadlineMs && row.m_sChallengeNonce == nonce) return true;
		}
		return false;
	}

	RICON_GameTs3Challenge IssueChallenge(RICON_GameRadioPttContext authoritative, string bindingId, int requestId, int nowMs, string freshNonce)
	{
		RICON_GameTs3BindingRow row = CurrentProofRow(authoritative, bindingId, requestId, nowMs);
		if (!row || row.m_iObservedClientId > 0) return null;
		if (row.m_sChallengeNonce.IsEmpty())
		{
			if (!RICON_GameTs3Challenge.IsValidNonce(freshNonce) || ChallengeNonceInUse(freshNonce, nowMs)) return null;
			row.m_sChallengeNonce = freshNonce;
		}
		RICON_GameTs3Challenge result = new RICON_GameTs3Challenge();
		result.m_Pending = row.m_Pending.CloneBinding();
		result.m_sNonce = row.m_sChallengeNonce;
		return result;
	}

	protected bool MatchedAddressInUse(RICON_GameTs3BindingClaim claim, int clientId, int nowMs)
	{
		foreach (int playerId, RICON_GameTs3BindingRow row : m_Rows)
		{
			if (row.m_iObservedClientId <= 0 || !row.m_Pending || nowMs >= row.m_Pending.m_iDeadlineMs) continue;
			RICON_GameTs3BindingClaim matched = row.m_Pending.m_Claim;
			if (matched.m_sServerUid == claim.m_sServerUid
				&& (matched.m_sClientUid == claim.m_sClientUid || row.m_iObservedClientId == clientId)) return true;
		}
		return false;
	}

	//! Records the one-use match for this pending request. It grants no route or audio permission.
	bool ConsumeObservation(RICON_GameRadioPttContext authoritative, RICON_GameTs3Observation observation, int nowMs)
	{
		if (!observation || observation.m_iClientId <= 0 || observation.m_iClientId > 65535 || observation.m_iTs3Revision <= 0) return false;
		if (!RICON_GameTs3Challenge.IsValidNonce(observation.m_sNonce)) return false;
		RICON_GameTs3BindingRow row = CurrentProofRow(authoritative, observation.m_sBindingId, observation.m_iRequestId, nowMs);
		if (!row || row.m_iObservedClientId > 0 || row.m_sChallengeNonce != observation.m_sNonce) return false;
		RICON_GameTs3BindingClaim claim = row.m_Pending.m_Claim;
		if (claim.m_sServerUid != observation.m_sServerUid || claim.m_sClientUid != observation.m_sClientUid
			|| claim.m_iTs3Revision != observation.m_iTs3Revision || MatchedAddressInUse(claim, observation.m_iClientId, nowMs)) return false;
		row.m_iObservedClientId = observation.m_iClientId;
		return true;
	}

	bool IsChallengeMatched(RICON_GameRadioPttContext authoritative, string bindingId, int requestId, int nowMs)
	{
		RICON_GameTs3BindingRow row = CurrentProofRow(authoritative, bindingId, requestId, nowMs);
		return row && row.m_iObservedClientId > 0;
	}

	void ReadPlayerIds(array<int> ids)
	{
		if (!ids) return;
		ids.Clear();
		for (int index = 0; index < m_Rows.Count(); index++) ids.Insert(m_Rows.GetKey(index));
		ids.Sort();
	}

	//! A null context is supplied only by the server after it establishes that the player is no longer bound.
	void ReconcilePlayer(int playerId, RICON_GameRadioPttContext authoritative, int nowMs)
	{
		if (playerId <= 0) return;
		if (authoritative && (!InScope(authoritative) || authoritative.m_iPlayerId != playerId)) return;
		if (!ObserveTime(nowMs)) return;
		RICON_GameTs3BindingRow row = m_Rows.Get(playerId);
		if (!row) return;
		if (!authoritative || !row.m_Context.Same(authoritative))
		{
			m_Rows.Remove(playerId);
			return;
		}
		Expire(row, nowMs);
	}

	void Close()
	{
		m_bClosed = true;
		m_Rows.Clear();
	}
}

//! Separate pending-binding frames: no route, capture or voice grant can be encoded here.
class RICON_GameTs3BindingWire
{
	static const int VERSION = 1, CLAIM_FIELDS = 9, PENDING_FIELDS = 11;

	static array<int> EncodeClaim(RICON_GameTs3BindingClaim claim)
	{
		if (!claim || !claim.IsValid()) return null;
		array<int> fields = {VERSION, claim.m_iRequestId};
		RICON_GameRadioPttWire.AppendContext(fields, claim.m_Context);
		fields.Insert(claim.m_iTs3Revision);
		return fields;
	}

	protected static RICON_GameTs3BindingClaim ReadClaim(string domain, string serverUid, string clientUid, array<int> fields)
	{
		RICON_GameTs3BindingClaim claim = new RICON_GameTs3BindingClaim();
		claim.m_Context = RICON_GameRadioPttWire.ReadContext(domain, fields, 2);
		claim.m_iRequestId = fields[1]; claim.m_iTs3Revision = fields[8];
		claim.m_sServerUid = serverUid; claim.m_sClientUid = clientUid;
		if (!claim.IsValid()) return null;
		return claim;
	}

	static RICON_GameTs3BindingClaim DecodeClaim(string domain, string serverUid, string clientUid, array<int> fields)
	{
		if (!fields || fields.Count() != CLAIM_FIELDS || fields[0] != VERSION) return null;
		return ReadClaim(domain, serverUid, clientUid, fields);
	}

	protected static bool ValidWindow(int serverNowMs, int deadlineMs)
	{
		if (serverNowMs < 0 || deadlineMs <= serverNowMs || deadlineMs >= int.MAX) return false;
		return deadlineMs - serverNowMs <= RICON_GameTs3BindingStore.TTL_MS;
	}

	static array<int> EncodePending(RICON_GameTs3PendingBinding binding, int serverNowMs)
	{
		if (!binding || !RICON_GameTs3PendingBinding.IsValidId(binding.m_sBindingId)) return null;
		if (!ValidWindow(serverNowMs, binding.m_iDeadlineMs)) return null;
		array<int> fields = EncodeClaim(binding.m_Claim);
		if (!fields) return null;
		fields.Insert(serverNowMs); fields.Insert(binding.m_iDeadlineMs);
		return fields;
	}

	static RICON_GameTs3PendingBinding DecodePending(string domain, string serverUid, string clientUid, string bindingId, array<int> fields, out int serverNowMs)
	{
		serverNowMs = -1;
		if (!fields || fields.Count() != PENDING_FIELDS || fields[0] != VERSION) return null;
		if (!RICON_GameTs3PendingBinding.IsValidId(bindingId) || !ValidWindow(fields[9], fields[10])) return null;
		RICON_GameTs3BindingClaim claim = ReadClaim(domain, serverUid, clientUid, fields);
		if (!claim) return null;
		RICON_GameTs3PendingBinding binding = new RICON_GameTs3PendingBinding();
		binding.m_Claim = claim; binding.m_sBindingId = bindingId; binding.m_iDeadlineMs = fields[10];
		serverNowMs = fields[9];
		return binding;
	}
}
