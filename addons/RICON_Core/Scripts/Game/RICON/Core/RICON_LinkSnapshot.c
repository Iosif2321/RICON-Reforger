// LinkSnapshot identity, ordering, resync and presentation TTL (REQ-core §8).
// Status: STATIC_IMPLEMENTED (not compiled yet).

//! Stable decisions shared by single-snapshot receive and atomic full resync.
enum RICON_ESnapshotDecision
{
	ACCEPT_FIRST = 0,
	ACCEPT_NEWER = 1,
	STALE = 2,
	INVALID_SNAPSHOT = 3,
	INVALID_IDENTITY = 4,
	ACCEPT_RESYNC = 5,
	INVALID_RESYNC = 6
}

//! Decision plus explicit effects. Effects are booleans so callers cannot confuse them with decisions.
class RICON_SnapshotApplyResult
{
	RICON_ESnapshotDecision m_eDecision;
	bool m_bRequestResync;
	bool m_bDiagnostic;

	void RICON_SnapshotApplyResult(RICON_ESnapshotDecision decision, bool requestResync = false, bool diagnostic = false)
	{
		m_eDecision = decision;
		m_bRequestResync = requestResync;
		m_bDiagnostic = diagnostic;
	}
}

//! Server-authoritative, owner-scoped description of one talker -> listener link.
class RICON_LinkSnapshot
{
	static const int CURRENT_SCHEMA_VERSION = 1;

	int m_iSchemaVersion;
	int m_iSessionEpoch;
	int m_iLinkId;
	int m_iLinkSeq;
	int m_iTtlMs;
	int m_iRouteId;
	int m_iRouteNodeCount;
	int m_iTalkerPlayerId;
	int m_iListenerPlayerId;
	float m_fQuality;
	RICON_ELinkState m_eState;

	bool IsValid()
	{
		if (m_iSchemaVersion <= 0 || m_iSessionEpoch <= 0 || m_iSessionEpoch >= int.MAX)
			return false;
		if (m_iLinkId <= 0 || m_iLinkSeq <= 0 || m_iLinkSeq >= int.MAX || m_iTtlMs <= 0)
			return false;
		if (m_iRouteId <= 0 || m_iRouteNodeCount <= 0)
			return false;
		if (!RICON_Num.IsFinite(m_fQuality) || m_fQuality < 0 || m_fQuality > 1)
			return false;
		if (m_eState != RICON_ELinkState.LOST && m_eState != RICON_ELinkState.WEAK
			&& m_eState != RICON_ELinkState.DEGRADED && m_eState != RICON_ELinkState.CLEAN)
			return false;
		return m_eState != RICON_ELinkState.LOST || m_fQuality == 0;
	}

	//! Compare this incoming snapshot with a valid current slot. Identity mismatch is never "newer".
	RICON_ESnapshotDecision CompareAgainst(RICON_LinkSnapshot current)
	{
		if (!IsValid())
			return RICON_ESnapshotDecision.INVALID_SNAPSHOT;
		if (!current)
			return RICON_ESnapshotDecision.ACCEPT_FIRST;
		if (!current.IsValid())
			return RICON_ESnapshotDecision.INVALID_IDENTITY;
		if (m_iSchemaVersion != current.m_iSchemaVersion || m_iSessionEpoch != current.m_iSessionEpoch
			|| m_iLinkId != current.m_iLinkId)
			return RICON_ESnapshotDecision.INVALID_IDENTITY;
		if (m_iLinkSeq > current.m_iLinkSeq)
			return RICON_ESnapshotDecision.ACCEPT_NEWER;
		return RICON_ESnapshotDecision.STALE;
	}

	//! TTL uses elapsed local monotonic time supplied by the receiver, never server UTC.
	bool IsExpiredElapsed(int elapsedMs)
	{
		return elapsedMs < 0 || elapsedMs >= m_iTtlMs;
	}

	RICON_LinkSnapshot CloneSnapshot()
	{
		RICON_LinkSnapshot copy = new RICON_LinkSnapshot();
		copy.m_iSchemaVersion = m_iSchemaVersion;
		copy.m_iSessionEpoch = m_iSessionEpoch;
		copy.m_iLinkId = m_iLinkId;
		copy.m_iLinkSeq = m_iLinkSeq;
		copy.m_iTtlMs = m_iTtlMs;
		copy.m_iRouteId = m_iRouteId;
		copy.m_iRouteNodeCount = m_iRouteNodeCount;
		copy.m_iTalkerPlayerId = m_iTalkerPlayerId;
		copy.m_iListenerPlayerId = m_iListenerPlayerId;
		copy.m_fQuality = m_fQuality;
		copy.m_eState = m_eState;
		return copy;
	}
}

//! Owner-scoped receive state. Session identity can only be installed by an atomic full resync.
class RICON_LinkSnapshotReceiver
{
	protected int m_iSupportedSchemaVersion;
	protected int m_iSessionEpoch;
	protected bool m_bHasIdentity;
	protected ref map<int, ref RICON_LinkSnapshot> m_mSlots = new map<int, ref RICON_LinkSnapshot>();
	protected ref map<int, int> m_mReceivedAtMs = new map<int, int>();

	void RICON_LinkSnapshotReceiver(int supportedSchemaVersion)
	{
		m_iSupportedSchemaVersion = supportedSchemaVersion;
	}

	//! Either installs the entire identity/state or rejects and clears it all (REQ-core §8.3).
	RICON_SnapshotApplyResult ApplyFullResync(
		int nowMs,
		int sessionEpoch,
		int schemaVersion,
		notnull array<ref RICON_LinkSnapshot> snapshots)
	{
		map<int, ref RICON_LinkSnapshot> nextSlots = new map<int, ref RICON_LinkSnapshot>();
		map<int, int> nextTimes = new map<int, int>();
		set<int> seenLinks = new set<int>();

		bool valid = m_iSupportedSchemaVersion > 0 && schemaVersion == m_iSupportedSchemaVersion
			&& sessionEpoch > 0 && sessionEpoch < int.MAX;
		if (valid)
		{
			foreach (RICON_LinkSnapshot snapshot : snapshots)
			{
				if (!snapshot || !snapshot.IsValid() || snapshot.m_iSchemaVersion != schemaVersion
					|| snapshot.m_iSessionEpoch != sessionEpoch || seenLinks.Contains(snapshot.m_iLinkId))
				{
					valid = false;
					break;
				}
				seenLinks.Insert(snapshot.m_iLinkId);
				nextSlots.Set(snapshot.m_iLinkId, snapshot.CloneSnapshot());
				nextTimes.Set(snapshot.m_iLinkId, nowMs);
			}
		}

		if (!valid)
		{
			ResetIdentity();
			return new RICON_SnapshotApplyResult(RICON_ESnapshotDecision.INVALID_RESYNC, true, true);
		}

		m_iSessionEpoch = sessionEpoch;
		m_bHasIdentity = true;
		m_mSlots = nextSlots;
		m_mReceivedAtMs = nextTimes;
		return new RICON_SnapshotApplyResult(RICON_ESnapshotDecision.ACCEPT_RESYNC);
	}

	RICON_SnapshotApplyResult Receive(int nowMs, RICON_LinkSnapshot snapshot)
	{
		if (!snapshot || !snapshot.IsValid())
			return new RICON_SnapshotApplyResult(RICON_ESnapshotDecision.INVALID_SNAPSHOT, false, true);
		if (!m_bHasIdentity || snapshot.m_iSchemaVersion != m_iSupportedSchemaVersion)
			return new RICON_SnapshotApplyResult(RICON_ESnapshotDecision.INVALID_IDENTITY, true);
		if (snapshot.m_iSessionEpoch != m_iSessionEpoch)
		{
			ResetIdentity();
			return new RICON_SnapshotApplyResult(RICON_ESnapshotDecision.INVALID_IDENTITY, true);
		}

		RICON_LinkSnapshot current = m_mSlots.Get(snapshot.m_iLinkId);
		RICON_ESnapshotDecision decision = snapshot.CompareAgainst(current);
		if (decision == RICON_ESnapshotDecision.ACCEPT_FIRST || decision == RICON_ESnapshotDecision.ACCEPT_NEWER)
		{
			m_mSlots.Set(snapshot.m_iLinkId, snapshot.CloneSnapshot());
			m_mReceivedAtMs.Set(snapshot.m_iLinkId, nowMs);
			return new RICON_SnapshotApplyResult(decision);
		}
		if (decision == RICON_ESnapshotDecision.STALE)
			return new RICON_SnapshotApplyResult(decision);
		return new RICON_SnapshotApplyResult(decision, true);
	}

	//! Missing slots are simply invisible. Negative elapsed time fails closed and asks the caller to log diagnostics.
	bool IsVisible(int nowMs, int linkId, out bool diagnostic)
	{
		diagnostic = false;
		RICON_LinkSnapshot snapshot = m_mSlots.Get(linkId);
		if (!snapshot || !m_mReceivedAtMs.Contains(linkId))
			return false;
		int elapsedMs = nowMs - m_mReceivedAtMs.Get(linkId);
		if (elapsedMs < 0)
		{
			diagnostic = true;
			return false;
		}
		return !snapshot.IsExpiredElapsed(elapsedMs);
	}

	int GetLinkSeq(int linkId)
	{
		RICON_LinkSnapshot snapshot = m_mSlots.Get(linkId);
		if (!snapshot)
			return 0;
		return snapshot.m_iLinkSeq;
	}

	bool HasIdentity()
	{
		return m_bHasIdentity;
	}

	int GetSessionEpoch()
	{
		return m_iSessionEpoch;
	}

	protected void ResetIdentity()
	{
		m_bHasIdentity = false;
		m_iSessionEpoch = 0;
		m_mSlots.Clear();
		m_mReceivedAtMs.Clear();
	}
}
