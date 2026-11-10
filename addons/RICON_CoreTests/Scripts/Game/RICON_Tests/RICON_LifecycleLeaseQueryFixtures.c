//! TEST_ONLY pure fixtures for the production getter; invoked explicitly, with no autorun.
class RICON_LifecycleLeaseQueryProbeMachine : RICON_LifecycleStateMachine
{
	int LastObservedNowMs() { return m_iLastObservedNowMs; }
	int LeaseAllocator() { return m_iLeaseAllocator; }
	int RateEntries() { return m_aRateEntries.Count(); }
	int RateAttempts()
	{
		int total; foreach (RICON_LifecycleRateEntry entry : m_aRateEntries) total += entry.m_iCount; return total;
	}
	int ResponseCacheSize()
	{
		RICON_LifecyclePlayer player = FindPlayer(1);
		if (!player) return 0;
		return player.m_aCache.Count();
	}
	int LastRequestSeq()
	{
		RICON_LifecyclePlayer player = FindPlayer(1);
		if (!player) return 0;
		return player.m_iLastRequestSeq;
	}
	RICON_LifecycleLease Held(int leaseId) { return FindLease(leaseId); }
	//! Only malformed/edge-case fixture data; normal grants below always use real ProcessPtt.
	void ForceTiming(int issuedAtMs, int ttlMs)
	{
		RICON_LifecycleLease held = FindLease(1);
		if (!held)
			return;
		held.m_iIssuedAtMs = issuedAtMs;
		held.m_iTtlMs = ttlMs;
	}
	void CopyMarkers()
	{
		RICON_LifecycleLease held = FindLease(1);
		if (!held) return;
		held.m_iTerminalAtMs = 777; held.m_eTerminalReason = RICON_ELeaseTerminalReason.ADMIN_REVOKED;
	}
}

class RICON_LifecycleLeaseQueryFixtures
{
	static int m_iChecks, m_iFailures, m_iFirstFailure;
	protected static bool Check(bool ok)
	{
		m_iChecks++; if (!ok) { m_iFailures++; if (m_iFirstFailure == 0) m_iFirstFailure = m_iChecks; PrintFormat("[RICON_LEASE_QUERY_DIAG] failed check=%1", m_iChecks); }
		return ok;
	}
	protected static RICON_LifecycleLeaseQueryProbeMachine Prepared(int ttlMs = 20)
	{
		RICON_LifecycleLeaseQueryProbeMachine machine = new RICON_LifecycleLeaseQueryProbeMachine();
		RICON_LifecycleDevice device = RICON_LifecycleTraceRun.Device(101, RICON_EDeviceKind.CARRIED, 1, true);
		device.m_aAllowed.Insert(new RICON_LifecycleCatalogPair(1, 1));
		array<ref RICON_LifecycleDevice> devices = {device};
		RICON_LifecycleConfig config = RICON_LifecycleTraceRun.Config(4, 4, 8, 60000, 4, 30000, 4, 8, 1000, 5);
		if (!Check(machine.Initialize(7, 0, config, devices).m_eResultCode == RICON_ELifecycleResult.OK)) return null;
		if (!Check(machine.Connected(1, 1, 501).m_eResultCode == RICON_ELifecycleResult.OK)) return null;
		if (!Check(machine.Registered(2, 1, 1).m_eResultCode == RICON_ELifecycleResult.OK)) return null;
		if (!Check(machine.EntityBound(3, 1, 1, 1001).m_eResultCode == RICON_ELifecycleResult.OK)) return null;
		RICON_LifecyclePttRequest request = RICON_LifecycleTraceRun.Start(100, 1, 1, 1, 1, 7, 1, 1, 101, 1, 1, ttlMs);
		RICON_LifecycleResult granted = machine.ProcessPtt(request);
		if (!Check(granted.m_eResultCode == RICON_ELifecycleResult.OK && granted.m_iLeaseId == 1)) return null;
		return machine;
	}
	protected static bool SameLease(RICON_LifecycleLease a, RICON_LifecycleLease b)
	{
		if (!a || !b) return false;
		if (a.m_iLeaseId != b.m_iLeaseId || a.m_iPlayerId != b.m_iPlayerId) return false;
		if (a.m_iGeneration != b.m_iGeneration || a.m_iDeviceId != b.m_iDeviceId) return false;
		if (a.m_iIssuedAtMs != b.m_iIssuedAtMs || a.m_iTtlMs != b.m_iTtlMs) return false;
		if (a.m_iTerminalAtMs != b.m_iTerminalAtMs || a.m_eState != b.m_eState) return false;
		return a.m_eTerminalReason == b.m_eTerminalReason;
	}
	protected static void DetachedCopyCases()
	{
		RICON_LifecycleLeaseQueryProbeMachine machine = Prepared();
		if (!machine) return;
		// Nonzero terminal markers exercise copying all nine fields; this is not a valid terminal transition.
		machine.CopyMarkers();
		RICON_LifecycleLease queried = machine.QueryActiveLease(1, 105);
		Check(queried && queried != machine.Held(1) && SameLease(queried, machine.Held(1)));
		Check(queried && queried.m_iLeaseId == 1 && queried.m_iPlayerId == 1 && queried.m_iGeneration == 1);
		Check(queried && queried.m_iDeviceId == 101 && queried.m_iIssuedAtMs == 100 && queried.m_iTtlMs == 20);
		Check(queried && queried.m_iTerminalAtMs == 777 && queried.m_eState == RICON_ETransmitLeaseState.ACTIVE
			&& queried.m_eTerminalReason == RICON_ELeaseTerminalReason.ADMIN_REVOKED);
		if (queried)
		{
			queried.m_iLeaseId = 999; queried.m_iPlayerId = 999; queried.m_iGeneration = 999; queried.m_iDeviceId = 999;
			queried.m_iIssuedAtMs = 0; queried.m_iTtlMs = 0; queried.m_iTerminalAtMs = 0;
			queried.m_eState = RICON_ETransmitLeaseState.REVOKED; queried.m_eTerminalReason = RICON_ELeaseTerminalReason.NONE;
		}
		RICON_LifecycleLease again = machine.QueryActiveLease(1, 105);
		Check(again && SameLease(again, machine.Held(1)) && again.m_iPlayerId == 1 && again.m_iTerminalAtMs == 777
			&& again.m_eTerminalReason == RICON_ELeaseTerminalReason.ADMIN_REVOKED && again.m_eState == RICON_ETransmitLeaseState.ACTIVE);
	}
	protected static void ClockAndNoSweepCases()
	{
		RICON_LifecycleLeaseQueryProbeMachine machine = Prepared();
		if (!machine) return;
		Check(machine.Tick(105).m_eResultCode == RICON_ELifecycleResult.OK);
		int leases = machine.GetLeaseCount(); int players = machine.GetPlayerCount(); int tombs = machine.GetTombstoneCount();
		int allocator = machine.LeaseAllocator(); int rates = machine.RateEntries(); int attempts = machine.RateAttempts();
		int cache = machine.ResponseCacheSize(); int requestSeq = machine.LastRequestSeq();
		Check(machine.QueryActiveLease(1, 119) != null);
		Check(machine.QueryActiveLease(1, 120) == null);
		Check(machine.LastObservedNowMs() == 105 && !machine.IsTickAnomalyLatched());
		Check(machine.Held(1) && machine.Held(1).m_eState == RICON_ETransmitLeaseState.ACTIVE
			&& machine.Held(1).m_eTerminalReason == RICON_ELeaseTerminalReason.NONE && machine.Held(1).m_iTerminalAtMs == 0);
		Check(machine.GetLeaseCount() == leases && machine.GetPlayerCount() == players && machine.GetTombstoneCount() == tombs);
		Check(machine.LeaseAllocator() == allocator && machine.RateEntries() == rates && machine.RateAttempts() == attempts);
		Check(machine.ResponseCacheSize() == cache && machine.LastRequestSeq() == requestSeq);
		Check(machine.QueryActiveLease(1, 104) == null && !machine.IsTickAnomalyLatched() && machine.LastObservedNowMs() == 105);
		Check(machine.QueryActiveLease(0, 105) == null && machine.QueryActiveLease(-1, 105) == null
			&& machine.QueryActiveLease(99, 105) == null && machine.QueryActiveLease(1, -1) == null);
		// A future read must not make the next earlier genuine event a clock anomaly.
		Check(machine.Tick(106).m_eResultCode == RICON_ELifecycleResult.OK && machine.LastObservedNowMs() == 106);
		Check(machine.QueryActiveLease(1, 119) != null);
		RICON_LifecycleResult expired = machine.Tick(120);
		Check(expired.m_eResultCode == RICON_ELifecycleResult.OK && expired.m_aEffects.Count() == 2);
		if (expired.m_aEffects.Count() == 2)
		{
			Check(expired.m_aEffects[0].m_eType == RICON_ELifecycleEffectType.LEASE_TERMINATED
				&& expired.m_aEffects[0].m_iLeaseId == 1 && expired.m_aEffects[0].m_eTerminalReason == RICON_ELeaseTerminalReason.TTL_EXPIRED);
			Check(expired.m_aEffects[1].m_eType == RICON_ELifecycleEffectType.VON_EXPIRY_INTENDED
				&& expired.m_aEffects[1].m_iLeaseId == 1 && expired.m_aEffects[1].m_iSessionEpoch == 7);
		}
		Check(machine.Held(1) && machine.Held(1).m_eState == RICON_ETransmitLeaseState.EXPIRED && machine.Held(1).m_iTerminalAtMs == 120);
		Check(machine.QueryActiveLease(1, 120) == null);
	}
	protected static void TerminalAndReadyCases()
	{
		RICON_LifecycleLeaseQueryProbeMachine unready = new RICON_LifecycleLeaseQueryProbeMachine();
		Check(unready.QueryActiveLease(1, 0) == null && unready.GetLeaseCount() == 0);
		RICON_LifecycleLeaseQueryProbeMachine machine = Prepared();
		if (!machine) return;
		Check(machine.QueryActiveLease(1, 100) != null);
		RICON_LifecycleResult stopped = machine.ProcessPtt(RICON_LifecycleTraceRun.Stop(101, 1, 2, 1, 1, 7, 1, 2, 1));
		Check(stopped.m_eResultCode == RICON_ELifecycleResult.OK && machine.Held(1) && machine.Held(1).m_eState == RICON_ETransmitLeaseState.REVOKED);
		Check(machine.QueryActiveLease(1, 101) == null && machine.Held(1) && machine.Held(1).m_eTerminalReason == RICON_ELeaseTerminalReason.CLIENT_STOP);
		machine = Prepared();
		if (!machine) return;
		Check(machine.Tick(99).m_eResultCode == RICON_ELifecycleResult.TICK_ANOMALY && machine.IsTickAnomalyLatched());
		int last = machine.LastObservedNowMs();
		Check(machine.QueryActiveLease(1, 100) == null && machine.IsTickAnomalyLatched() && machine.LastObservedNowMs() == last);
	}
	protected static void TimingDomainCases()
	{
		RICON_LifecycleLeaseQueryProbeMachine machine = Prepared(1);
		if (!machine) return;
		Check(machine.QueryActiveLease(1, 100) != null && machine.QueryActiveLease(1, 101) == null);
		machine = Prepared();
		if (!machine)
			return;
		machine.ForceTiming(-1, 20); Check(machine.QueryActiveLease(1, 105) == null);
		machine.ForceTiming(100, 0); Check(machine.QueryActiveLease(1, 105) == null);
		machine.ForceTiming(100, -1); Check(machine.QueryActiveLease(1, 105) == null);
		machine.ForceTiming(110, 20); Check(machine.QueryActiveLease(1, 109) == null);
		machine.ForceTiming(int.MAX - 5, 10);
		Check(machine.QueryActiveLease(1, int.MAX) != null && machine.QueryActiveLease(1, int.MAX - 6) == null);
		machine.ForceTiming(0, int.MAX);
		Check(machine.QueryActiveLease(1, int.MAX - 1) != null && machine.QueryActiveLease(1, int.MAX) == null);
		machine.ForceTiming(int.MAX, 1); Check(machine.QueryActiveLease(1, int.MAX) != null);
		Check(machine.LastObservedNowMs() == 100 && machine.Held(1) && machine.Held(1).m_eState == RICON_ETransmitLeaseState.ACTIVE);
	}
	static bool RunAll()
	{
		m_iChecks = 0; m_iFailures = 0; m_iFirstFailure = 0;
		DetachedCopyCases(); ClockAndNoSweepCases(); TerminalAndReadyCases(); TimingDomainCases();
		return m_iFailures == 0;
	}
}
