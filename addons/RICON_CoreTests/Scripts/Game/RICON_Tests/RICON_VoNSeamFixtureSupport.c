// TEST_ONLY support for the generated VoN seam parity fixture (RICON_VoNSeamFixtures.c). Not for Workshop.
// One RICON_VoNSeamRun replays one case of oracle/vectors/von_seam_v1.json on RICON_VoNIntegration. Each call is
// made here: the decision is compared with the oracle and the inputs are compared with copies taken before the call,
// because REQ-von-seam §7 forbids any mutation. A case stops at its first failed call, like the host oracle runner.

class RICON_VoNSeamRun
{
	protected string m_sCaseId;
	protected int m_iExpectedCalls;
	protected int m_iCall;
	protected int m_iPassedCalls;
	protected bool m_bFailed;
	protected bool m_bQuiet;
	protected ref RICON_VoNIntegration m_Seam = new RICON_VoNIntegration();

	void RICON_VoNSeamRun(string caseId, int expectedCalls, bool quiet = false)
	{
		m_sCaseId = caseId;
		m_iExpectedCalls = expectedCalls;
		m_bQuiet = quiet;
	}

	bool IsPassed()
	{
		return !m_bFailed && m_iPassedCalls == m_iExpectedCalls;
	}

	bool IsFailed()
	{
		return m_bFailed;
	}

	int GetPassedCalls()
	{
		return m_iPassedCalls;
	}

	void Apply(RICON_LinkSnapshot snapshot, RICON_VoNDeviceBinding binding, RICON_ESnapshotDecision upstreamDecision,
		RICON_EVoNSeamDecision expected)
	{
		int call = Begin();
		if (call < 0)
			return;
		RICON_LinkSnapshot snapshotBefore = CopySnapshot(snapshot);
		RICON_VoNDeviceBinding bindingBefore = CopyBinding(binding);
		RICON_EVoNSeamDecision actual = m_Seam.ApplySnapshot(snapshot, binding, upstreamDecision);
		string problem = DecisionProblem(expected, actual);
		if (problem.IsEmpty() && !SameSnapshot(snapshotBefore, snapshot))
			problem = "snapshot changed by the call";
		if (problem.IsEmpty() && !SameBinding(bindingBefore, binding))
			problem = "binding changed by the call";
		Finish(call, "apply", problem);
	}

	void Expire(int sessionEpoch, int playerId, int deviceId, int leaseId, RICON_VoNDeviceBinding binding,
		RICON_EVoNSeamDecision expected)
	{
		int call = Begin();
		if (call < 0)
			return;
		RICON_VoNDeviceBinding bindingBefore = CopyBinding(binding);
		RICON_EVoNSeamDecision actual = m_Seam.OnTransmitLeaseExpired(sessionEpoch, playerId, deviceId, leaseId, binding);
		string problem = DecisionProblem(expected, actual);
		if (problem.IsEmpty() && !SameBinding(bindingBefore, binding))
			problem = "binding changed by the call";
		Finish(call, "expire", problem);
	}

	//! Marks the case failed when a special float could not be constructed, so it can never pass silently.
	void RequireNonFinite(float value, bool mustBeNaN)
	{
		if (m_bFailed)
			return;
		// R25: `value != value` is true for ±Inf in Workbench too; classify NaN by arithmetic (RICON_TestSupport.FloatClass).
		bool ok = !RICON_Num.IsFinite(value);
		if (mustBeNaN)
			ok = ok && RICON_TestSupport.FloatClass(value) == 1;
		if (ok)
			return;
		m_bFailed = true;
		if (!m_bQuiet)
			PrintFormat("[RICON_TEST] FAIL %1: special float was not constructed (NaN expected: %2)", m_sCaseId,
				mustBeNaN);
	}

	protected int Begin()
	{
		int call = m_iCall;
		m_iCall++;
		if (m_bFailed)
			return -1;
		return call;
	}

	protected void Finish(int call, string op, string problem)
	{
		if (problem.IsEmpty())
		{
			m_iPassedCalls++;
			return;
		}
		m_bFailed = true;
		if (!m_bQuiet)
			PrintFormat("[RICON_TEST] FAIL %1 call %2 (%3): %4", m_sCaseId, call, op, problem);
	}

	protected static string DecisionProblem(RICON_EVoNSeamDecision expected, RICON_EVoNSeamDecision actual)
	{
		if (actual == expected)
			return "";
		return "decision expected " + typename.EnumToString(RICON_EVoNSeamDecision, expected) + " actual "
			+ typename.EnumToString(RICON_EVoNSeamDecision, actual);
	}

	protected static RICON_LinkSnapshot CopySnapshot(RICON_LinkSnapshot source)
	{
		if (!source)
			return null;
		RICON_LinkSnapshot copy = new RICON_LinkSnapshot();
		copy.m_iSchemaVersion = source.m_iSchemaVersion;
		copy.m_iSessionEpoch = source.m_iSessionEpoch;
		copy.m_iLinkId = source.m_iLinkId;
		copy.m_iLinkSeq = source.m_iLinkSeq;
		copy.m_iTtlMs = source.m_iTtlMs;
		copy.m_iRouteId = source.m_iRouteId;
		copy.m_iRouteNodeCount = source.m_iRouteNodeCount;
		copy.m_iTalkerPlayerId = source.m_iTalkerPlayerId;
		copy.m_iListenerPlayerId = source.m_iListenerPlayerId;
		copy.m_fQuality = source.m_fQuality;
		copy.m_eState = source.m_eState;
		return copy;
	}

	//! Quality counts as unchanged when both are the same class (finite and equal, NaN, +Inf or -Inf). R25: the old
	//! `x != x` test treated NaN and ±Inf alike in Workbench, so the class comes from RICON_TestSupport.FloatClass.
	protected static bool SameSnapshot(RICON_LinkSnapshot a, RICON_LinkSnapshot b)
	{
		if (!a || !b)
			return !a && !b;
		// Short checks: Enforce rejects long boolean chains ("Formula too complex").
		if (!RICON_TestSupport.SameFloat(a.m_fQuality, b.m_fQuality))
			return false;
		if (a.m_iSchemaVersion != b.m_iSchemaVersion || a.m_iSessionEpoch != b.m_iSessionEpoch)
			return false;
		if (a.m_iLinkId != b.m_iLinkId || a.m_iLinkSeq != b.m_iLinkSeq || a.m_iTtlMs != b.m_iTtlMs)
			return false;
		if (a.m_iRouteId != b.m_iRouteId || a.m_iRouteNodeCount != b.m_iRouteNodeCount)
			return false;
		if (a.m_iTalkerPlayerId != b.m_iTalkerPlayerId || a.m_iListenerPlayerId != b.m_iListenerPlayerId)
			return false;
		return a.m_eState == b.m_eState;
	}

	protected static RICON_VoNDeviceBinding CopyBinding(RICON_VoNDeviceBinding source)
	{
		if (!source)
			return null;
		return Binding(source.m_iSessionEpoch, source.m_iLinkId, source.m_iTalkerPlayerId, source.m_iListenerPlayerId,
			source.m_iTalkerDeviceId, source.m_iListenerDeviceId, source.m_iTalkerLeaseId,
			source.m_iTalkerTransceiverIndex, source.m_iListenerTransceiverIndex);
	}

	protected static bool SameBinding(RICON_VoNDeviceBinding a, RICON_VoNDeviceBinding b)
	{
		if (!a || !b)
			return !a && !b;
		if (a.m_iSessionEpoch != b.m_iSessionEpoch || a.m_iLinkId != b.m_iLinkId)
			return false;
		if (a.m_iTalkerPlayerId != b.m_iTalkerPlayerId || a.m_iListenerPlayerId != b.m_iListenerPlayerId)
			return false;
		if (a.m_iTalkerDeviceId != b.m_iTalkerDeviceId || a.m_iListenerDeviceId != b.m_iListenerDeviceId)
			return false;
		if (a.m_iTalkerLeaseId != b.m_iTalkerLeaseId)
			return false;
		return a.m_iTalkerTransceiverIndex == b.m_iTalkerTransceiverIndex
			&& a.m_iListenerTransceiverIndex == b.m_iListenerTransceiverIndex;
	}

	//! Builders in the field order of REQ-core §8 (snapshot) and REQ-von-seam §4 (binding).
	static RICON_LinkSnapshot Snapshot(int schemaVersion, int sessionEpoch, int linkId, int linkSeq, int ttlMs,
		int routeId, int routeNodeCount, float quality, RICON_ELinkState state, int talkerPlayerId,
		int listenerPlayerId)
	{
		RICON_LinkSnapshot snapshot = new RICON_LinkSnapshot();
		snapshot.m_iSchemaVersion = schemaVersion;
		snapshot.m_iSessionEpoch = sessionEpoch;
		snapshot.m_iLinkId = linkId;
		snapshot.m_iLinkSeq = linkSeq;
		snapshot.m_iTtlMs = ttlMs;
		snapshot.m_iRouteId = routeId;
		snapshot.m_iRouteNodeCount = routeNodeCount;
		snapshot.m_fQuality = quality;
		snapshot.m_eState = state;
		snapshot.m_iTalkerPlayerId = talkerPlayerId;
		snapshot.m_iListenerPlayerId = listenerPlayerId;
		return snapshot;
	}

	static RICON_VoNDeviceBinding Binding(int sessionEpoch, int linkId, int talkerPlayerId, int listenerPlayerId,
		int talkerDeviceId, int listenerDeviceId, int talkerLeaseId, int talkerTransceiverIndex,
		int listenerTransceiverIndex)
	{
		RICON_VoNDeviceBinding binding = new RICON_VoNDeviceBinding();
		binding.m_iSessionEpoch = sessionEpoch;
		binding.m_iLinkId = linkId;
		binding.m_iTalkerPlayerId = talkerPlayerId;
		binding.m_iListenerPlayerId = listenerPlayerId;
		binding.m_iTalkerDeviceId = talkerDeviceId;
		binding.m_iListenerDeviceId = listenerDeviceId;
		binding.m_iTalkerLeaseId = talkerLeaseId;
		binding.m_iTalkerTransceiverIndex = talkerTransceiverIndex;
		binding.m_iListenerTransceiverIndex = listenerTransceiverIndex;
		return binding;
	}

	//! Special floats are made at run time; RequireNonFinite() confirms the result before it is used.
	static float NaN()
	{
		return Math.Sqrt(-1.0);
	}

	static float Infinity()
	{
		float big = float.MAX;
		return big * 2.0;
	}
}
