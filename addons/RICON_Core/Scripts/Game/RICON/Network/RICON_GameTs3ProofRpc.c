//! The local profile carries the owner challenge. Voice permission is decided separately.
class RICON_GameTs3ProofChallengeFile
{
	static const string PATH = "$profile:ricon-ts3-binding-challenge.txt";
	static const int MAX_ROW_BYTES = 384;

	static bool Enabled()
	{
		string value;
		return System.GetCLIParam("riconTS3Proof", value) && value == "1";
	}

	static string Build(RICON_GameTs3PendingBinding pending, string nonce)
	{
		if (!pending || !pending.m_Claim || !pending.m_Claim.IsValid()
			|| !RICON_GameTs3PendingBinding.IsValidId(pending.m_sBindingId)
			|| !RICON_GameTs3Challenge.IsValidNonce(nonce)) return "";
		RICON_GameTs3BindingClaim claim = pending.m_Claim;
		return "RICON_TS3_PROOF_CHALLENGE_1 " + pending.m_sBindingId + " " + claim.m_iRequestId.ToString()
			+ " " + claim.m_iTs3Revision.ToString() + " " + claim.m_sServerUid + " " + claim.m_sClientUid + " " + nonce + " END";
	}

	static bool Write(string row)
	{
		if (row.IsEmpty() || row.Length() > MAX_ROW_BYTES) return false;
		FileHandle file = FileIO.OpenFile(PATH, FileMode.WRITE);
		if (!file) return false;
		bool complete = file.Write(row, row.Length()) == row.Length();
		file.Close();
		return complete;
	}

	static bool Clear(string ownedRow)
	{
		if (ownedRow.IsEmpty() || !FileIO.FileExists(PATH)) return true;
		FileHandle file = FileIO.OpenFile(PATH, FileMode.READ);
		if (!file) return false;
		string row;
		int count = ownedRow.Length();
		bool same = file.GetLength() == count && file.Read(row, count) == count
			&& file.GetLength() == count && row == ownedRow;
		file.Close();
		if (!same) return true;
		FileIO.DeleteFile(PATH);
		return !FileIO.FileExists(PATH);
	}
}

modded class SCR_PlayerController
{
	protected static const int RICON_TS3_PROOF_POLL_MS = 250;
	protected string m_sRICON_Ts3ProofBinding, m_sRICON_Ts3ProofFileRow;
	protected int m_iRICON_Ts3ProofQuery, m_iRICON_Ts3ProofSentAt = -1, m_iRICON_Ts3ProofServerPollAt = -1;
	protected bool m_bRICON_Ts3ProofMatched;

	void RICON_ClearTs3Proof()
	{
		RICON_GameTs3ProofChallengeFile.Clear(m_sRICON_Ts3ProofFileRow);
		m_sRICON_Ts3ProofFileRow = "";
		m_sRICON_Ts3ProofBinding = "";
		m_bRICON_Ts3ProofMatched = false;
		m_iRICON_Ts3ProofSentAt = -1;
	}

	void RICON_PollTs3Proof()
	{
		if (!RICON_GameTs3ProofChallengeFile.Enabled()) return;
		int remainingMs;
		ref RICON_GameTs3PendingBinding pending = RICON_ReadTs3PendingBinding(remainingMs);
		if (!pending || !pending.m_Claim)
		{
			RICON_ClearTs3Proof();
			return;
		}
		if (m_sRICON_Ts3ProofBinding != pending.m_sBindingId)
		{
			RICON_ClearTs3Proof();
			m_sRICON_Ts3ProofBinding = pending.m_sBindingId;
		}
		int nowMs = System.GetTickCount();
		if (m_bRICON_Ts3ProofMatched)
		{
			if (RICON_GameTs3ProofChallengeFile.Clear(m_sRICON_Ts3ProofFileRow)) m_sRICON_Ts3ProofFileRow = "";
			return;
		}
		if (remainingMs <= 0 || m_iRICON_Ts3ProofQuery == int.MAX || nowMs < 0) return;
		if (m_iRICON_Ts3ProofSentAt >= 0 && nowMs - m_iRICON_Ts3ProofSentAt < RICON_TS3_PROOF_POLL_MS) return;
		m_iRICON_Ts3ProofSentAt = nowMs;
		m_iRICON_Ts3ProofQuery++;
		Rpc(RpcAsk_RICON_Ts3Proof, pending.m_sBindingId, pending.m_Claim.m_iRequestId, m_iRICON_Ts3ProofQuery);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RICON_Ts3Proof(string bindingId, int requestId, int queryId)
	{
		if (!Replication.IsServer() || !RICON_GameTs3ProofChallengeFile.Enabled() || requestId <= 0 || queryId <= 0
			|| !RICON_GameTs3PendingBinding.IsValidId(bindingId)) return;
		int nowMs = System.GetTickCount();
		if (nowMs < 0 || (m_iRICON_Ts3ProofServerPollAt >= 0 && nowMs - m_iRICON_Ts3ProofServerPollAt < 100)) return;
		m_iRICON_Ts3ProofServerPollAt = nowMs;
		RICON_GameRadioManagerComponent manager = RICON_GameRadioManagerComponent.Cast(RICON_CommsManagerComponent.GetInstance());
		if (!manager) return;
		int remainingMs;
		bool matched;
		string nonce = manager.ReadGameTs3Proof(this, bindingId, requestId, remainingMs, matched);
		if (remainingMs <= 0 || (!matched && nonce.IsEmpty()) || RICON_CommsManagerComponent.GetInstance() != manager) return;
		Rpc(RpcDo_RICON_Ts3Proof, bindingId, requestId, queryId, remainingMs, matched, nonce);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_RICON_Ts3Proof(string bindingId, int requestId, int queryId, int remainingMs, bool matched, string nonce)
	{
		if (!RICON_GameTs3ProofChallengeFile.Enabled() || m_bRICON_Ts3ProofMatched || remainingMs <= 0
			|| remainingMs > RICON_GameTs3BindingStore.TTL_MS || queryId <= 0 || queryId != m_iRICON_Ts3ProofQuery
			|| m_iRICON_Ts3ProofSentAt < 0 || bindingId != m_sRICON_Ts3ProofBinding) return;
		int localRemainingMs;
		ref RICON_GameTs3PendingBinding pending = RICON_ReadTs3PendingBinding(localRemainingMs);
		int nowMs = System.GetTickCount();
		if (!pending || !pending.m_Claim || pending.m_sBindingId != bindingId || pending.m_Claim.m_iRequestId != requestId
			|| nowMs < m_iRICON_Ts3ProofSentAt || nowMs - m_iRICON_Ts3ProofSentAt >= remainingMs || localRemainingMs <= 0) return;
		if (matched)
		{
			if (!nonce.IsEmpty()) return;
			m_bRICON_Ts3ProofMatched = true;
			if (RICON_GameTs3ProofChallengeFile.Clear(m_sRICON_Ts3ProofFileRow)) m_sRICON_Ts3ProofFileRow = "";
			return;
		}
		string row = RICON_GameTs3ProofChallengeFile.Build(pending, nonce);
		if (!row.IsEmpty() && m_sRICON_Ts3ProofFileRow.IsEmpty() && RICON_GameTs3ProofChallengeFile.Write(row))
			m_sRICON_Ts3ProofFileRow = row;
	}
}
