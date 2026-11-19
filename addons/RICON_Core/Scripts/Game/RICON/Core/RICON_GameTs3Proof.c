//! Private dedicated-profile exchange. Only the configured server observer may write seed and observation files.
class RICON_GameTs3ProofFiles
{
	static const string DIRECTORY = "$profile:ricon-ts3-proof";
	static const string JOB_HEADER = "RICON_TS3_PROOF_JOB_1";
	static const string SEED_HEADER = "RICON_TS3_PROOF_SEED_2";
	static const string ISSUED_HEADER = "RICON_TS3_PROOF_ISSUED_1";
	static const string OBSERVED_HEADER = "RICON_TS3_PROOF_OBSERVED_2";
	static const int MAX_ROW = 1024;

	static string PathFor(string bindingId, string suffix)
	{
		if (!RICON_GameTs3PendingBinding.IsValidId(bindingId)) return "";
		if (suffix != ".job" && suffix != ".seed" && suffix != ".challenge" && suffix != ".observation") return "";
		return DIRECTORY + "/" + bindingId + suffix;
	}

	static string Body(RICON_GameTs3PendingBinding pending, int issuedMs)
	{
		if (!pending || !pending.m_Claim || !pending.m_Claim.IsValid() || issuedMs < 0) return "";
		if (!RICON_GameTs3PendingBinding.IsValidId(pending.m_sBindingId) || pending.m_iDeadlineMs <= issuedMs
			|| pending.m_iDeadlineMs >= int.MAX || pending.m_iDeadlineMs - issuedMs > RICON_GameTs3BindingStore.TTL_MS) return "";
		RICON_GameTs3BindingClaim claim = pending.m_Claim;
		RICON_GameRadioPttContext context = claim.m_Context;
		string row = context.m_sDomain + " " + context.m_iEpoch.ToString() + " " + context.m_iPlayerId.ToString();
		row += " " + context.m_iConnectionId.ToString() + " " + context.m_iGeneration.ToString() + " " + context.m_iEntityId.ToString();
		row += " " + context.m_iPolicyRevision.ToString() + " " + pending.m_sBindingId + " " + claim.m_iRequestId.ToString();
		row += " " + claim.m_iTs3Revision.ToString() + " " + claim.m_sServerUid + " " + claim.m_sClientUid;
		return row + " " + issuedMs.ToString() + " " + pending.m_iDeadlineMs.ToString();
	}

	static bool ReadRow(string path, out string row)
	{
		row = "";
		if (path.IsEmpty() || !FileIO.FileExists(path)) return false;
		FileHandle input = FileIO.OpenFile(path, FileMode.READ);
		if (!input) return false;
		int length = input.GetLength();
		bool complete = length > 0 && length <= MAX_ROW && input.Read(row, length) == length;
		complete = complete && input.GetLength() == length && row.Length() == length;
		input.Close();
		if (!complete) return false;
		if (row.ToAscii(length - 1) == 10)
		{
			length--;
			if (length > 0 && row.ToAscii(length - 1) == 13) length--;
			if (length == 0) return false;
			row = row.Substring(0, length);
		}
		for (int index = 0; index < length; index++)
		{
			int ch = row.ToAscii(index);
			if (ch < 32 || ch > 126) return false;
		}
		return true;
	}

	static bool WriteNew(string path, string row)
	{
		if (path.IsEmpty() || row.IsEmpty() || row.Length() > MAX_ROW - 2 || FileIO.FileExists(path)) return false;
		FileHandle output = FileIO.OpenFile(path, FileMode.WRITE);
		if (!output) return false;
		row += "\r\n";
		bool complete = output.Write(row, row.Length()) == row.Length();
		output.Close();
		if (!complete) FileIO.DeleteFile(path);
		return complete;
	}

	static string ReadSeed(string body, string row)
	{
		string prefix = SEED_HEADER + " " + body + " ";
		if (body.IsEmpty() || row.Length() != prefix.Length() + 26) return "";
		string nonce = row.Substring(prefix.Length(), 22);
		if (!RICON_GameTs3Challenge.IsValidNonce(nonce) || row != prefix + nonce + " END") return "";
		return nonce;
	}

	static RICON_GameTs3Observation ReadObservation(RICON_GameTs3Challenge challenge, string body, string row)
	{
		if (!challenge || !challenge.m_Pending || !challenge.m_Pending.m_Claim) return null;
		string prefix = OBSERVED_HEADER + " " + body + " " + challenge.m_sNonce + " ";
		if (body.IsEmpty() || row.Length() <= prefix.Length() + 4) return null;
		array<string> fields = {};
		row.Split(" ", fields, true);
		if (fields.Count() != 18) return null;
		int clientId = PositiveClientId(fields[16]);
		if (clientId <= 0 || row != prefix + clientId.ToString() + " END") return null;
		RICON_GameTs3BindingClaim claim = challenge.m_Pending.m_Claim;
		RICON_GameTs3Observation observation = new RICON_GameTs3Observation();
		observation.m_sBindingId = challenge.m_Pending.m_sBindingId;
		observation.m_iRequestId = claim.m_iRequestId;
		observation.m_iTs3Revision = claim.m_iTs3Revision;
		observation.m_sServerUid = claim.m_sServerUid;
		observation.m_sClientUid = claim.m_sClientUid;
		observation.m_sNonce = challenge.m_sNonce;
		observation.m_iClientId = clientId;
		return observation;
	}

	protected static int PositiveClientId(string value)
	{
		if (value.Length() < 1 || value.Length() > 5 || value.ToAscii(0) == 48) return 0;
		int result;
		for (int index = 0; index < value.Length(); index++)
		{
			int digit = value.ToAscii(index) - 48;
			if (digit < 0 || digit > 9) return 0;
			result = result * 10 + digit;
		}
		if (result < 1 || result > 65535) return 0;
		return result;
	}

	//! Existing public owner-label format, independent of the private observer job.
	static string ChallengeRow(RICON_GameTs3Challenge challenge)
	{
		if (!challenge || !challenge.m_Pending || !challenge.m_Pending.m_Claim
			|| !RICON_GameTs3Challenge.IsValidNonce(challenge.m_sNonce)) return "";
		RICON_GameTs3PendingBinding pending = challenge.m_Pending;
		RICON_GameTs3BindingClaim claim = pending.m_Claim;
		if (!claim.IsValid() || !RICON_GameTs3PendingBinding.IsValidId(pending.m_sBindingId)) return "";
		return "RICON_TS3_PROOF_CHALLENGE_1 " + pending.m_sBindingId + " " + claim.m_iRequestId.ToString()
			+ " " + claim.m_iTs3Revision.ToString() + " " + claim.m_sServerUid + " " + claim.m_sClientUid
			+ " " + challenge.m_sNonce + " END";
	}
}

class RICON_GameTs3ProofJob
{
	ref RICON_GameTs3PendingBinding m_Pending;
	ref RICON_GameTs3Challenge m_Challenge;
	string m_sBody;
	bool m_bMatched, m_bIssuedPublished;
}

//! Native sender context and clock are re-read after every external file operation.
class RICON_GameTs3ProofAdapter
{
	protected ref RICON_LifecycleStateMachine m_Lifecycle;
	protected ref RICON_GameControllerLifecycle m_Bridge;
	protected ref RICON_GameRadioPttNativeSource m_Source;
	protected ref RICON_GameTs3BindingStore m_Store;
	protected ref map<int, ref RICON_GameTs3ProofJob> m_Jobs = new map<int, ref RICON_GameTs3ProofJob>();
	protected string m_sDomain;
	protected int m_iEpoch, m_iMaxPlayers;
	protected bool m_bClosed, m_bPolling;

	bool Configure(RICON_LifecycleStateMachine lifecycle, RICON_GameControllerLifecycle bridge,
		RICON_GameRadioPttNativeSource nativePttSource, RICON_GameTs3BindingStore bindingStore, string domain, int epoch, int maxPlayers)
	{
		if (m_iEpoch || m_bClosed || !lifecycle || !bridge || !nativePttSource || !bindingStore) return false;
		if (!RICON_GameTs3PendingBinding.IsValidId(domain) || epoch <= 0 || lifecycle.GetSessionEpoch() != epoch || maxPlayers <= 0) return false;
		if (!bridge.MatchesLifecycle(lifecycle) || !nativePttSource.IsScope(lifecycle) || !RICON_CommsManagerComponent.IsAuthority()) return false;
		m_Lifecycle = lifecycle;
		m_Bridge = bridge;
		m_Source = nativePttSource;
		m_Store = bindingStore;
		m_sDomain = domain;
		m_iEpoch = epoch;
		m_iMaxPlayers = maxPlayers;
		// The observer/operator owns this directory and its OS permissions.
		FileIO.MakeDirectory(RICON_GameTs3ProofFiles.DIRECTORY);
		return IsScope();
	}

	protected bool IsScope()
	{
		ref RICON_LifecycleStateMachine lifecycle = m_Lifecycle;
		ref RICON_GameControllerLifecycle bridge = m_Bridge;
		ref RICON_GameRadioPttNativeSource source = m_Source;
		ref RICON_GameTs3BindingStore store = m_Store;
		if (m_bClosed || m_iEpoch <= 0 || !store || !lifecycle || !bridge || !source) return false;
		if (lifecycle.GetSessionEpoch() != m_iEpoch || !bridge.MatchesLifecycle(lifecycle)
			|| !source.IsScope(lifecycle) || !RICON_CommsManagerComponent.IsAuthority()) return false;
		return !m_bClosed && lifecycle == m_Lifecycle && bridge == m_Bridge && source == m_Source && store == m_Store;
	}

	protected RICON_GameRadioPttContext ReadActor(SCR_PlayerController sender, out int nowMs)
	{
		nowMs = -1;
		if (!IsScope() || !sender) return null;
		ref RICON_GameRadioPttNativeSource source = m_Source;
		RICON_GameRadioPttContext context = source.ReadContext(new RICON_GameRadioPttNativeCaller(sender));
		if (!context || !context.IsValid() || context.m_sDomain != m_sDomain || context.m_iEpoch != m_iEpoch) return null;
		if (!IsScope() || source != m_Source) return null;
		nowMs = source.ReadNowMs();
		if (nowMs < 0 || !IsScope() || source != m_Source) return null;
		return context;
	}

	protected bool Held(RICON_GameTs3ProofJob job)
	{
		return IsScope() && job && job.m_Pending && job.m_Pending.m_Claim
			&& m_Jobs.Get(job.m_Pending.m_Claim.m_Context.m_iPlayerId) == job;
	}

	protected RICON_GameRadioPttContext Current(RICON_GameTs3ProofJob job, out int nowMs)
	{
		nowMs = -1;
		if (!Held(job)) return null;
		RICON_GameRadioPttContext expected = job.m_Pending.m_Claim.m_Context;
		ref RICON_GameControllerLifecycle bridge = m_Bridge;
		SCR_PlayerController sender = bridge.ResolveRemoteRecipient(expected.m_iPlayerId,
			expected.m_iConnectionId, expected.m_iGeneration, expected.m_iEntityId);
		RICON_GameRadioPttContext context = ReadActor(sender, nowMs);
		if (!context || !context.Same(expected) || !Held(job)) return null;
		RICON_GameTs3PendingBinding pending = m_Store.Read(context, nowMs);
		if (!SamePending(pending, job.m_Pending) || !Held(job)) return null;
		return context;
	}

	protected static bool SamePending(RICON_GameTs3PendingBinding first, RICON_GameTs3PendingBinding second)
	{
		return first && second && first.m_Claim && first.m_Claim.Same(second.m_Claim)
			&& first.m_sBindingId == second.m_sBindingId && first.m_iDeadlineMs == second.m_iDeadlineMs;
	}

	bool Track(RICON_GameTs3PendingBinding pending)
	{
		ref RICON_GameTs3ProofAdapter held = this;
		if (!IsScope() || !pending || !pending.m_Claim || !pending.m_Claim.IsValid()) return false;
		RICON_GameRadioPttContext expected = pending.m_Claim.m_Context;
		ref RICON_GameControllerLifecycle bridge = m_Bridge;
		SCR_PlayerController sender = bridge.ResolveRemoteRecipient(expected.m_iPlayerId,
			expected.m_iConnectionId, expected.m_iGeneration, expected.m_iEntityId);
		int nowMs;
		RICON_GameRadioPttContext context = ReadActor(sender, nowMs);
		if (!context || !context.Same(expected) || !SamePending(m_Store.Read(context, nowMs), pending)) return false;
		ref RICON_GameTs3ProofJob prior = m_Jobs.Get(context.m_iPlayerId);
		if (prior && SamePending(prior.m_Pending, pending)) return true;
		if (prior) Retire(prior);
		if (!IsScope() || m_Jobs.Count() >= m_iMaxPlayers) return false;
		ref RICON_GameTs3ProofJob job = new RICON_GameTs3ProofJob();
		job.m_Pending = pending.CloneBinding();
		job.m_sBody = RICON_GameTs3ProofFiles.Body(job.m_Pending, nowMs);
		if (job.m_sBody.IsEmpty()) return false;
		string path = RICON_GameTs3ProofFiles.PathFor(pending.m_sBindingId, ".job");
		if (!RICON_GameTs3ProofFiles.WriteNew(path, RICON_GameTs3ProofFiles.JOB_HEADER + " " + job.m_sBody + " END")) return false;
		if (!IsScope())
		{
			FileIO.DeleteFile(path);
			return false;
		}
		m_Jobs.Set(expected.m_iPlayerId, job);
		if (Current(job, nowMs)) return true;
		Retire(job);
		return false;
	}

	void Poll()
	{
		ref RICON_GameTs3ProofAdapter held = this;
		if (!IsScope() || m_bPolling) return;
		m_bPolling = true;
		array<int> players = {};
		for (int index = 0; index < m_Jobs.Count(); index++) players.Insert(m_Jobs.GetKey(index));
		players.Sort();
		foreach (int playerId : players)
		{
			if (!IsScope()) break;
			ref RICON_GameTs3ProofJob job = m_Jobs.Get(playerId);
			if (job) PollJob(job);
		}
		m_bPolling = false;
	}

	protected void PollJob(RICON_GameTs3ProofJob job)
	{
		int nowMs;
		ref RICON_GameRadioPttContext context = Current(job, nowMs);
		if (!context) { Retire(job); return; }
		if (job.m_bMatched) return;
		string bindingId = job.m_Pending.m_sBindingId;
		string row;
		if (!job.m_Challenge)
		{
			bool readSeed = RICON_GameTs3ProofFiles.ReadRow(RICON_GameTs3ProofFiles.PathFor(bindingId, ".seed"), row);
			context = Current(job, nowMs);
			if (!context) { Retire(job); return; }
			if (!readSeed) return;
			string nonce = RICON_GameTs3ProofFiles.ReadSeed(job.m_sBody, row);
			if (nonce.IsEmpty()) return;
			ref RICON_GameTs3Challenge challenge = m_Store.IssueChallenge(context, bindingId,
				job.m_Pending.m_Claim.m_iRequestId, nowMs, nonce);
			if (!challenge || !SamePending(challenge.m_Pending, job.m_Pending) || !Held(job)) return;
			job.m_Challenge = challenge;
		}
		if (!job.m_bIssuedPublished)
		{
			string issued = RICON_GameTs3ProofFiles.ISSUED_HEADER + " " + job.m_sBody + " " + job.m_Challenge.m_sNonce + " END";
			string path = RICON_GameTs3ProofFiles.PathFor(bindingId, ".challenge");
			bool published = RICON_GameTs3ProofFiles.WriteNew(path, issued);
			context = Current(job, nowMs);
			if (!context) { Retire(job); return; }
			if (!published)
			{
				string existing;
				published = RICON_GameTs3ProofFiles.ReadRow(path, existing) && existing == issued;
				context = Current(job, nowMs);
				if (!context) { Retire(job); return; }
			}
			// A failed write retains the same original nonce and expiry for the next poll.
			if (!published) return;
			job.m_bIssuedPublished = true;
		}
		bool readObservation = RICON_GameTs3ProofFiles.ReadRow(RICON_GameTs3ProofFiles.PathFor(bindingId, ".observation"), row);
		context = Current(job, nowMs);
		if (!context) { Retire(job); return; }
		if (!readObservation) return;
		RICON_GameTs3Observation observation = RICON_GameTs3ProofFiles.ReadObservation(job.m_Challenge, job.m_sBody, row);
		if (!observation || !m_Store.ConsumeObservation(context, observation, nowMs) || !Held(job)) return;
		context = Current(job, nowMs);
		if (!context || !m_Store.IsChallengeMatched(context, bindingId, observation.m_iRequestId, nowMs)) { Retire(job); return; }
		job.m_bMatched = true;
		DeleteFiles(job);
	}

	RICON_GameTs3Challenge ReadChallenge(SCR_PlayerController sender, string bindingId, int requestId, out int serverNowMs)
	{
		ref RICON_GameTs3ProofAdapter held = this;
		serverNowMs = -1;
		int nowMs;
		ref RICON_GameRadioPttContext context = ReadActor(sender, nowMs);
		if (!context) return null;
		ref RICON_GameTs3ProofJob job = m_Jobs.Get(context.m_iPlayerId);
		if (!job || !job.m_Challenge || !job.m_bIssuedPublished || job.m_Pending.m_sBindingId != bindingId
			|| job.m_Pending.m_Claim.m_iRequestId != requestId) return null;
		ref RICON_GameRadioPttContext current = Current(job, nowMs);
		if (!current || !current.Same(context)) return null;
		RICON_GameTs3Challenge challenge = new RICON_GameTs3Challenge();
		challenge.m_Pending = job.m_Challenge.m_Pending.CloneBinding();
		challenge.m_sNonce = job.m_Challenge.m_sNonce;
		serverNowMs = nowMs;
		return challenge;
	}

	RICON_GameTs3MatchedBinding ReadMatched(SCR_PlayerController sender, string bindingId, int requestId, out int serverNowMs)
	{
		ref RICON_GameTs3ProofAdapter held = this;
		serverNowMs = -1;
		int nowMs;
		ref RICON_GameRadioPttContext context = ReadActor(sender, nowMs);
		if (!context) return null;
		ref RICON_GameTs3ProofJob job = m_Jobs.Get(context.m_iPlayerId);
		if (!job || !job.m_bMatched || job.m_Pending.m_sBindingId != bindingId
			|| job.m_Pending.m_Claim.m_iRequestId != requestId) return null;
		ref RICON_GameRadioPttContext current = Current(job, nowMs);
		if (!current || !current.Same(context)) return null;
		RICON_GameTs3MatchedBinding matched = m_Store.ReadMatched(current, nowMs);
		if (!matched || matched.m_sBindingId != bindingId || matched.m_Claim.m_iRequestId != requestId) return null;
		serverNowMs = nowMs;
		return matched;
	}

	protected static void DeleteFiles(RICON_GameTs3ProofJob job)
	{
		if (!job || !job.m_Pending) return;
		array<string> suffixes = {".job", ".seed", ".challenge", ".observation"};
		foreach (string suffix : suffixes)
		{
			string path = RICON_GameTs3ProofFiles.PathFor(job.m_Pending.m_sBindingId, suffix);
			if (!path.IsEmpty() && FileIO.FileExists(path)) FileIO.DeleteFile(path);
		}
	}

	protected void Retire(RICON_GameTs3ProofJob job)
	{
		ref RICON_GameTs3ProofJob held = job;
		if (!held || !held.m_Pending || !held.m_Pending.m_Claim) return;
		int playerId = held.m_Pending.m_Claim.m_Context.m_iPlayerId;
		if (m_Jobs.Get(playerId) == held) m_Jobs.Remove(playerId);
		DeleteFiles(held);
	}

	void Close()
	{
		ref RICON_GameTs3ProofAdapter held = this;
		m_bClosed = true;
		foreach (int playerId, RICON_GameTs3ProofJob job : m_Jobs) DeleteFiles(job);
		m_Jobs.Clear();
		m_Source = null; m_Bridge = null; m_Lifecycle = null; m_Store = null;
	}
}
