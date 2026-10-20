//! GENERATED TEST_ONLY: gen_snapshot_fixture.py, ожидания копируются из rev9.
//! 3 allocator_trace — BLOCKED_NO_AUTHORITY_CONTEXT; BROKEN -> enum -1 (ADAPTER_MAPPED=1).
class RICON_SnapshotFixtures
{
	static const string SOURCE_SHA = "3440982cea0cce21ef857e09a1db3dbe77a5c85561771212dd9ea1199e93721c";

	//! CMP-01
	protected static void Case0(RICON_SnapshotRun t)
	{
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Compare(null, s0, RICON_ESnapshotDecision.ACCEPT_FIRST);
	}

	//! CMP-02
	protected static void Case1(RICON_SnapshotRun t)
	{
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 6.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Compare(s0, s1, RICON_ESnapshotDecision.ACCEPT_NEWER);
	}

	//! CMP-03
	protected static void Case2(RICON_SnapshotRun t)
	{
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Compare(s0, s1, RICON_ESnapshotDecision.STALE);
	}

	//! CMP-04
	protected static void Case3(RICON_SnapshotRun t)
	{
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 4.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Compare(s0, s1, RICON_ESnapshotDecision.STALE);
	}

	//! CMP-05
	protected static void Case4(RICON_SnapshotRun t)
	{
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(1.0, 8.0, 10.0, 1.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Compare(s0, s1, RICON_ESnapshotDecision.INVALID_IDENTITY);
	}

	//! CMP-06
	protected static void Case5(RICON_SnapshotRun t)
	{
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(2.0, 7.0, 10.0, 6.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Compare(s0, s1, RICON_ESnapshotDecision.INVALID_IDENTITY);
	}

	//! CMP-07
	protected static void Case6(RICON_SnapshotRun t)
	{
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 11.0, 6.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Compare(s0, s1, RICON_ESnapshotDecision.INVALID_IDENTITY);
	}

	//! CMP-08
	protected static void Case7(RICON_SnapshotRun t)
	{
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(0.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Compare(s0, s1, RICON_ESnapshotDecision.INVALID_SNAPSHOT);
	}

	//! CMP-09
	protected static void Case8(RICON_SnapshotRun t)
	{
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(1.0, 0.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Compare(s0, s1, RICON_ESnapshotDecision.INVALID_SNAPSHOT);
	}

	//! CMP-25
	protected static void Case9(RICON_SnapshotRun t)
	{
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(1.0, -5.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Compare(s0, s1, RICON_ESnapshotDecision.INVALID_SNAPSHOT);
	}

	//! CMP-26
	protected static void Case10(RICON_SnapshotRun t)
	{
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(1.0, 2147483647.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Compare(s0, s1, RICON_ESnapshotDecision.INVALID_SNAPSHOT);
	}

	//! CMP-10
	protected static void Case11(RICON_SnapshotRun t)
	{
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 0.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Compare(s0, s1, RICON_ESnapshotDecision.INVALID_SNAPSHOT);
	}

	//! CMP-11
	protected static void Case12(RICON_SnapshotRun t)
	{
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 0.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Compare(s0, s1, RICON_ESnapshotDecision.INVALID_SNAPSHOT);
	}

	//! CMP-12
	protected static void Case13(RICON_SnapshotRun t)
	{
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 0.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Compare(s0, s1, RICON_ESnapshotDecision.INVALID_SNAPSHOT);
	}

	//! CMP-13
	protected static void Case14(RICON_SnapshotRun t)
	{
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 0.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Compare(s0, s1, RICON_ESnapshotDecision.INVALID_SNAPSHOT);
	}

	//! CMP-14
	protected static void Case15(RICON_SnapshotRun t)
	{
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 0.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Compare(s0, s1, RICON_ESnapshotDecision.INVALID_SNAPSHOT);
	}

	//! CMP-15
	protected static void Case16(RICON_SnapshotRun t)
	{
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 1.5, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Compare(s0, s1, RICON_ESnapshotDecision.INVALID_SNAPSHOT);
	}

	//! CMP-16
	protected static void Case17(RICON_SnapshotRun t)
	{
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, RICON_L0Run.Special(0), RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.RequireNaN(s1);
		t.Compare(s0, s1, RICON_ESnapshotDecision.INVALID_SNAPSHOT);
	}

	//! CMP-17
	protected static void Case18(RICON_SnapshotRun t)
	{
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, -1, 1.0, 2.0);
		t.Compare(s0, s1, RICON_ESnapshotDecision.INVALID_SNAPSHOT);
	}

	//! CMP-18
	protected static void Case19(RICON_SnapshotRun t)
	{
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.2, RICON_ELinkState.LOST, 1.0, 2.0);
		t.Compare(s0, s1, RICON_ESnapshotDecision.INVALID_SNAPSHOT);
	}

	//! CMP-19
	protected static void Case20(RICON_SnapshotRun t)
	{
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 6.0, 1000.0, 3.0, 1.0, 0.0, RICON_ELinkState.LOST, 1.0, 2.0);
		t.Compare(s0, s1, RICON_ESnapshotDecision.ACCEPT_NEWER);
	}

	//! CMP-20
	protected static void Case21(RICON_SnapshotRun t)
	{
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(1.0, 0.0, 11.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Compare(s0, s1, RICON_ESnapshotDecision.INVALID_SNAPSHOT);
	}

	//! CMP-21
	protected static void Case22(RICON_SnapshotRun t)
	{
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, -5.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Compare(null, s0, RICON_ESnapshotDecision.INVALID_SNAPSHOT);
	}

	//! CMP-22
	protected static void Case23(RICON_SnapshotRun t)
	{
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 2147483646.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 2147483647.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Compare(s0, s1, RICON_ESnapshotDecision.INVALID_SNAPSHOT);
	}

	//! CMP-23
	protected static void Case24(RICON_SnapshotRun t)
	{
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 2147483646.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 2147483646.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Compare(s0, s1, RICON_ESnapshotDecision.STALE);
	}

	//! CMP-24
	protected static void Case25(RICON_SnapshotRun t)
	{
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 2147483645.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 2147483646.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Compare(s0, s1, RICON_ESnapshotDecision.ACCEPT_NEWER);
	}

	//! RX-01
	protected static void Case26(RICON_SnapshotRun t)
	{
		array<int> ids0 = {};
		array<int> seq0 = {};
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Receive(0, s0, RICON_ESnapshotDecision.INVALID_IDENTITY, true, false, ids0, seq0);
	}

	//! RX-02
	protected static void Case27(RICON_SnapshotRun t)
	{
		array<int> ids0 = {};
		array<int> seq0 = {};
		array<ref RICON_LinkSnapshot> entries0 = {};
		t.Resync(0, 7, 1, entries0, RICON_ESnapshotDecision.ACCEPT_RESYNC, false, false, ids0, seq0);
		array<int> ids1 = {10};
		array<int> seq1 = {5};
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Receive(10, s0, RICON_ESnapshotDecision.ACCEPT_FIRST, false, false, ids1, seq1);
		array<int> ids2 = {10};
		array<int> seq2 = {6};
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 6.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Receive(20, s1, RICON_ESnapshotDecision.ACCEPT_NEWER, false, false, ids2, seq2);
		array<int> ids3 = {10};
		array<int> seq3 = {6};
		RICON_LinkSnapshot s2 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 6.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Receive(30, s2, RICON_ESnapshotDecision.STALE, false, false, ids3, seq3);
		array<int> ids4 = {10};
		array<int> seq4 = {6};
		RICON_LinkSnapshot s3 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 2.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Receive(40, s3, RICON_ESnapshotDecision.STALE, false, false, ids4, seq4);
	}

	//! RX-03
	protected static void Case28(RICON_SnapshotRun t)
	{
		array<int> ids0 = {10, 12};
		array<int> seq0 = {5, 9};
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 12.0, 9.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		array<ref RICON_LinkSnapshot> entries0 = {s0, s1};
		t.Resync(0, 7, 1, entries0, RICON_ESnapshotDecision.ACCEPT_RESYNC, false, false, ids0, seq0);
		array<int> ids1 = {};
		array<int> seq1 = {};
		RICON_LinkSnapshot s2 = RICON_VoNSeamRun.Snapshot(1.0, 8.0, 10.0, 1.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Receive(10, s2, RICON_ESnapshotDecision.INVALID_IDENTITY, true, false, ids1, seq1);
		array<int> ids2 = {};
		array<int> seq2 = {};
		RICON_LinkSnapshot s3 = RICON_VoNSeamRun.Snapshot(1.0, 8.0, 10.0, 2.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Receive(20, s3, RICON_ESnapshotDecision.INVALID_IDENTITY, true, false, ids2, seq2);
		array<int> ids3 = {10};
		array<int> seq3 = {3};
		RICON_LinkSnapshot s4 = RICON_VoNSeamRun.Snapshot(1.0, 8.0, 10.0, 3.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		array<ref RICON_LinkSnapshot> entries3 = {s4};
		t.Resync(30, 8, 1, entries3, RICON_ESnapshotDecision.ACCEPT_RESYNC, false, false, ids3, seq3);
		array<int> ids4 = {10};
		array<int> seq4 = {4};
		RICON_LinkSnapshot s5 = RICON_VoNSeamRun.Snapshot(1.0, 8.0, 10.0, 4.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Receive(40, s5, RICON_ESnapshotDecision.ACCEPT_NEWER, false, false, ids4, seq4);
	}

	//! RX-04
	protected static void Case29(RICON_SnapshotRun t)
	{
		array<int> ids0 = {};
		array<int> seq0 = {};
		array<ref RICON_LinkSnapshot> entries0 = {};
		t.Resync(0, 7, 1, entries0, RICON_ESnapshotDecision.ACCEPT_RESYNC, false, false, ids0, seq0);
		array<int> ids1 = {10};
		array<int> seq1 = {5};
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Receive(10, s0, RICON_ESnapshotDecision.ACCEPT_FIRST, false, false, ids1, seq1);
		array<int> ids2 = {10};
		array<int> seq2 = {5};
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 6.0, 1000.0, 3.0, 1.0, 2.0, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Receive(20, s1, RICON_ESnapshotDecision.INVALID_SNAPSHOT, false, true, ids2, seq2);
	}

	//! RX-05
	protected static void Case30(RICON_SnapshotRun t)
	{
		array<int> ids0 = {};
		array<int> seq0 = {};
		array<ref RICON_LinkSnapshot> entries0 = {};
		t.Resync(0, 7, 1, entries0, RICON_ESnapshotDecision.ACCEPT_RESYNC, false, false, ids0, seq0);
		array<int> ids1 = {10};
		array<int> seq1 = {5};
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Receive(10, s0, RICON_ESnapshotDecision.ACCEPT_FIRST, false, false, ids1, seq1);
		array<int> ids2 = {10};
		array<int> seq2 = {5};
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(2.0, 7.0, 10.0, 6.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Receive(20, s1, RICON_ESnapshotDecision.INVALID_IDENTITY, true, false, ids2, seq2);
	}

	//! RX-06
	protected static void Case31(RICON_SnapshotRun t)
	{
		array<int> ids0 = {};
		array<int> seq0 = {};
		array<ref RICON_LinkSnapshot> entries0 = {};
		t.Resync(0, 7, 1, entries0, RICON_ESnapshotDecision.ACCEPT_RESYNC, false, false, ids0, seq0);
		array<int> ids1 = {10};
		array<int> seq1 = {5};
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Receive(100, s0, RICON_ESnapshotDecision.ACCEPT_FIRST, false, false, ids1, seq1);
		array<int> ids2 = {10};
		array<int> seq2 = {5};
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Receive(1050, s1, RICON_ESnapshotDecision.STALE, false, false, ids2, seq2);
		t.Visible(1099, 10, true, false);
		t.Visible(1100, 10, false, false);
	}

	//! RX-07
	protected static void Case32(RICON_SnapshotRun t)
	{
		array<int> ids0 = {};
		array<int> seq0 = {};
		array<ref RICON_LinkSnapshot> entries0 = {};
		t.Resync(0, 7, 1, entries0, RICON_ESnapshotDecision.ACCEPT_RESYNC, false, false, ids0, seq0);
		array<int> ids1 = {10};
		array<int> seq1 = {5};
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Receive(0, s0, RICON_ESnapshotDecision.ACCEPT_FIRST, false, false, ids1, seq1);
		array<int> ids2 = {10};
		array<int> seq2 = {6};
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 6.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Receive(900, s1, RICON_ESnapshotDecision.ACCEPT_NEWER, false, false, ids2, seq2);
		t.Visible(1500, 10, true, false);
		t.Visible(1900, 10, false, false);
	}

	//! RX-08
	protected static void Case33(RICON_SnapshotRun t)
	{
		array<int> ids0 = {};
		array<int> seq0 = {};
		array<ref RICON_LinkSnapshot> entries0 = {};
		t.Resync(0, 7, 1, entries0, RICON_ESnapshotDecision.ACCEPT_RESYNC, false, false, ids0, seq0);
		array<int> ids1 = {10};
		array<int> seq1 = {5};
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Receive(500, s0, RICON_ESnapshotDecision.ACCEPT_FIRST, false, false, ids1, seq1);
		t.Visible(400, 10, false, true);
	}

	//! RX-09
	protected static void Case34(RICON_SnapshotRun t)
	{
		array<int> ids0 = {10};
		array<int> seq0 = {2};
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 2.0, 500.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		array<ref RICON_LinkSnapshot> entries0 = {s0};
		t.Resync(1000, 7, 1, entries0, RICON_ESnapshotDecision.ACCEPT_RESYNC, false, false, ids0, seq0);
		t.Visible(1499, 10, true, false);
		t.Visible(1500, 10, false, false);
		t.Visible(1600, 99, false, false);
	}

	//! RX-10
	protected static void Case35(RICON_SnapshotRun t)
	{
		array<int> ids0 = {10};
		array<int> seq0 = {5};
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		array<ref RICON_LinkSnapshot> entries0 = {s0};
		t.Resync(0, 7, 1, entries0, RICON_ESnapshotDecision.ACCEPT_RESYNC, false, false, ids0, seq0);
		array<int> ids1 = {};
		array<int> seq1 = {};
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(2.0, 8.0, 10.0, 1.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		array<ref RICON_LinkSnapshot> entries1 = {s1};
		t.Resync(10, 8, 2, entries1, RICON_ESnapshotDecision.INVALID_RESYNC, true, true, ids1, seq1);
		array<int> ids2 = {};
		array<int> seq2 = {};
		RICON_LinkSnapshot s2 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 6.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Receive(20, s2, RICON_ESnapshotDecision.INVALID_IDENTITY, true, false, ids2, seq2);
	}

	//! RX-11
	protected static void Case36(RICON_SnapshotRun t)
	{
		array<int> ids0 = {10};
		array<int> seq0 = {5};
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		array<ref RICON_LinkSnapshot> entries0 = {s0};
		t.Resync(0, 7, 1, entries0, RICON_ESnapshotDecision.ACCEPT_RESYNC, false, false, ids0, seq0);
		array<int> ids1 = {};
		array<int> seq1 = {};
		array<ref RICON_LinkSnapshot> entries1 = {};
		t.Resync(10, 0, 1, entries1, RICON_ESnapshotDecision.INVALID_RESYNC, true, true, ids1, seq1);
	}

	//! RX-15
	protected static void Case37(RICON_SnapshotRun t)
	{
		array<int> ids0 = {10};
		array<int> seq0 = {5};
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		array<ref RICON_LinkSnapshot> entries0 = {s0};
		t.Resync(0, 7, 1, entries0, RICON_ESnapshotDecision.ACCEPT_RESYNC, false, false, ids0, seq0);
		array<int> ids1 = {};
		array<int> seq1 = {};
		array<ref RICON_LinkSnapshot> entries1 = {};
		t.Resync(10, 2147483647, 1, entries1, RICON_ESnapshotDecision.INVALID_RESYNC, true, true, ids1, seq1);
	}

	//! RX-17
	protected static void Case38(RICON_SnapshotRun t)
	{
		array<int> ids0 = {10};
		array<int> seq0 = {5};
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		array<ref RICON_LinkSnapshot> entries0 = {s0};
		t.Resync(0, 7, 1, entries0, RICON_ESnapshotDecision.ACCEPT_RESYNC, false, false, ids0, seq0);
		array<int> ids1 = {};
		array<int> seq1 = {};
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(1.0, 8.0, 10.0, 1.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Receive(10, s1, RICON_ESnapshotDecision.INVALID_IDENTITY, true, false, ids1, seq1);
		array<int> ids2 = {};
		array<int> seq2 = {};
		RICON_LinkSnapshot s2 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 4.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Receive(20, s2, RICON_ESnapshotDecision.INVALID_IDENTITY, true, false, ids2, seq2);
		array<int> ids3 = {};
		array<int> seq3 = {};
		RICON_LinkSnapshot s3 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 12.0, 1.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Receive(30, s3, RICON_ESnapshotDecision.INVALID_IDENTITY, true, false, ids3, seq3);
	}

	//! RX-16
	protected static void Case39(RICON_SnapshotRun t)
	{
		array<int> ids0 = {10};
		array<int> seq0 = {5};
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		array<ref RICON_LinkSnapshot> entries0 = {s0};
		t.Resync(0, 7, 1, entries0, RICON_ESnapshotDecision.ACCEPT_RESYNC, false, false, ids0, seq0);
		array<int> ids1 = {};
		array<int> seq1 = {};
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(1.0, -5.0, 10.0, 1.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		array<ref RICON_LinkSnapshot> entries1 = {s1};
		t.Resync(10, -5, 1, entries1, RICON_ESnapshotDecision.INVALID_RESYNC, true, true, ids1, seq1);
	}

	//! RX-12
	protected static void Case40(RICON_SnapshotRun t)
	{
		array<int> ids0 = {10};
		array<int> seq0 = {5};
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		array<ref RICON_LinkSnapshot> entries0 = {s0};
		t.Resync(0, 7, 1, entries0, RICON_ESnapshotDecision.ACCEPT_RESYNC, false, false, ids0, seq0);
		array<int> ids1 = {};
		array<int> seq1 = {};
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(1.0, 8.0, 10.0, 1.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		RICON_LinkSnapshot s2 = RICON_VoNSeamRun.Snapshot(1.0, 8.0, 12.0, 1.0, 1000.0, 3.0, 1.0, 7.0, RICON_ELinkState.CLEAN, 1.0, 2.0);
		array<ref RICON_LinkSnapshot> entries1 = {s1, s2};
		t.Resync(10, 8, 1, entries1, RICON_ESnapshotDecision.INVALID_RESYNC, true, true, ids1, seq1);
	}

	//! RX-13
	protected static void Case41(RICON_SnapshotRun t)
	{
		array<int> ids0 = {};
		array<int> seq0 = {};
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 8.0, 10.0, 1.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 12.0, 1.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		array<ref RICON_LinkSnapshot> entries0 = {s0, s1};
		t.Resync(10, 8, 1, entries0, RICON_ESnapshotDecision.INVALID_RESYNC, true, true, ids0, seq0);
	}

	//! RX-14
	protected static void Case42(RICON_SnapshotRun t)
	{
		array<int> ids0 = {};
		array<int> seq0 = {};
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 8.0, 10.0, 1.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(1.0, 8.0, 10.0, 2.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		array<ref RICON_LinkSnapshot> entries0 = {s0, s1};
		t.Resync(10, 8, 1, entries0, RICON_ESnapshotDecision.INVALID_RESYNC, true, true, ids0, seq0);
	}

	//! X-1
	protected static void Case43(RICON_SnapshotRun t)
	{
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Compare(null, s0, RICON_ESnapshotDecision.STALE);
	}

	//! X-2
	protected static void Case44(RICON_SnapshotRun t)
	{
		array<int> ids0 = {};
		array<int> seq0 = {};
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Receive(0, s0, RICON_ESnapshotDecision.INVALID_IDENTITY, false, false, ids0, seq0);
	}

	//! X-3
	protected static void Case45(RICON_SnapshotRun t)
	{
		array<int> ids0 = {};
		array<int> seq0 = {};
		array<ref RICON_LinkSnapshot> entries0 = {};
		t.Resync(0, 7, 1, entries0, RICON_ESnapshotDecision.ACCEPT_RESYNC, false, false, ids0, seq0);
		array<int> ids1 = {10};
		array<int> seq1 = {5};
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Receive(500, s0, RICON_ESnapshotDecision.ACCEPT_FIRST, false, false, ids1, seq1);
		t.Visible(400, 10, false, false);
	}

	//! X-4
	protected static void Case46(RICON_SnapshotRun t)
	{
		array<int> ids0 = {10};
		array<int> seq0 = {2};
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 2.0, 500.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		array<ref RICON_LinkSnapshot> entries0 = {s0};
		t.Resync(1000, 7, 1, entries0, RICON_ESnapshotDecision.ACCEPT_RESYNC, false, false, ids0, seq0);
		t.Visible(1499, 10, false, false);
		t.Visible(1500, 10, false, false);
		t.Visible(1600, 99, false, false);
	}

	//! X-5
	protected static void Case47(RICON_SnapshotRun t)
	{
		array<int> ids0 = {10};
		array<int> seq0 = {5};
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 12.0, 9.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		array<ref RICON_LinkSnapshot> entries0 = {s0, s1};
		t.Resync(0, 7, 1, entries0, RICON_ESnapshotDecision.ACCEPT_RESYNC, false, false, ids0, seq0);
		array<int> ids1 = {};
		array<int> seq1 = {};
		RICON_LinkSnapshot s2 = RICON_VoNSeamRun.Snapshot(1.0, 8.0, 10.0, 1.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Receive(10, s2, RICON_ESnapshotDecision.INVALID_IDENTITY, true, false, ids1, seq1);
		array<int> ids2 = {};
		array<int> seq2 = {};
		RICON_LinkSnapshot s3 = RICON_VoNSeamRun.Snapshot(1.0, 8.0, 10.0, 2.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Receive(20, s3, RICON_ESnapshotDecision.INVALID_IDENTITY, true, false, ids2, seq2);
		array<int> ids3 = {10};
		array<int> seq3 = {3};
		RICON_LinkSnapshot s4 = RICON_VoNSeamRun.Snapshot(1.0, 8.0, 10.0, 3.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		array<ref RICON_LinkSnapshot> entries3 = {s4};
		t.Resync(30, 8, 1, entries3, RICON_ESnapshotDecision.ACCEPT_RESYNC, false, false, ids3, seq3);
		array<int> ids4 = {10};
		array<int> seq4 = {4};
		RICON_LinkSnapshot s5 = RICON_VoNSeamRun.Snapshot(1.0, 8.0, 10.0, 4.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Receive(40, s5, RICON_ESnapshotDecision.ACCEPT_NEWER, false, false, ids4, seq4);
	}

	//! X-6
	protected static void Case48(RICON_SnapshotRun t)
	{
		array<int> ids0 = {};
		array<int> seq0 = {};
		array<ref RICON_LinkSnapshot> entries0 = {};
		t.Resync(0, 7, 1, entries0, RICON_ESnapshotDecision.ACCEPT_RESYNC, false, false, ids0, seq0);
		array<int> ids1 = {10};
		array<int> seq1 = {5};
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Receive(10, s0, RICON_ESnapshotDecision.ACCEPT_FIRST, false, false, ids1, seq1);
		array<int> ids2 = {10};
		array<int> seq2 = {5};
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 6.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Receive(20, s1, RICON_ESnapshotDecision.ACCEPT_NEWER, false, false, ids2, seq2);
		array<int> ids3 = {10};
		array<int> seq3 = {6};
		RICON_LinkSnapshot s2 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 6.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Receive(30, s2, RICON_ESnapshotDecision.STALE, false, false, ids3, seq3);
		array<int> ids4 = {10};
		array<int> seq4 = {6};
		RICON_LinkSnapshot s3 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 2.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Receive(40, s3, RICON_ESnapshotDecision.STALE, false, false, ids4, seq4);
	}

	//! X-7
	protected static void Case49(RICON_SnapshotRun t)
	{
		array<int> ids0 = {10};
		array<int> seq0 = {5};
		RICON_LinkSnapshot s0 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 5.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		array<ref RICON_LinkSnapshot> entries0 = {s0};
		t.Resync(0, 7, 1, entries0, RICON_ESnapshotDecision.ACCEPT_RESYNC, false, false, ids0, seq0);
		array<int> ids1 = {};
		array<int> seq1 = {};
		RICON_LinkSnapshot s1 = RICON_VoNSeamRun.Snapshot(2.0, 8.0, 10.0, 1.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		array<ref RICON_LinkSnapshot> entries1 = {s1};
		t.Resync(10, 8, 2, entries1, RICON_ESnapshotDecision.INVALID_RESYNC, true, false, ids1, seq1);
		array<int> ids2 = {};
		array<int> seq2 = {};
		RICON_LinkSnapshot s2 = RICON_VoNSeamRun.Snapshot(1.0, 7.0, 10.0, 6.0, 1000.0, 3.0, 1.0, 0.9, RICON_ELinkState.CLEAN, 1.0, 2.0);
		t.Receive(20, s2, RICON_ESnapshotDecision.INVALID_IDENTITY, true, false, ids2, seq2);
	}

	static bool RunAll()
	{
		Print("[RICON_SNAPSHOT] source=" + SOURCE_SHA);
		int cases;
		int steps;
		int quietPositive;
		int rejected;
		RICON_SnapshotRun t;
		t = new RICON_SnapshotRun("CMP-01");
		Case0(t);
		if (t.Passed(1)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("CMP-01", true);
		Case0(t);
		if (t.Passed(1)) quietPositive++;
		t = new RICON_SnapshotRun("CMP-02");
		Case1(t);
		if (t.Passed(1)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("CMP-02", true);
		Case1(t);
		if (t.Passed(1)) quietPositive++;
		t = new RICON_SnapshotRun("CMP-03");
		Case2(t);
		if (t.Passed(1)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("CMP-03", true);
		Case2(t);
		if (t.Passed(1)) quietPositive++;
		t = new RICON_SnapshotRun("CMP-04");
		Case3(t);
		if (t.Passed(1)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("CMP-04", true);
		Case3(t);
		if (t.Passed(1)) quietPositive++;
		t = new RICON_SnapshotRun("CMP-05");
		Case4(t);
		if (t.Passed(1)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("CMP-05", true);
		Case4(t);
		if (t.Passed(1)) quietPositive++;
		t = new RICON_SnapshotRun("CMP-06");
		Case5(t);
		if (t.Passed(1)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("CMP-06", true);
		Case5(t);
		if (t.Passed(1)) quietPositive++;
		t = new RICON_SnapshotRun("CMP-07");
		Case6(t);
		if (t.Passed(1)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("CMP-07", true);
		Case6(t);
		if (t.Passed(1)) quietPositive++;
		t = new RICON_SnapshotRun("CMP-08");
		Case7(t);
		if (t.Passed(1)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("CMP-08", true);
		Case7(t);
		if (t.Passed(1)) quietPositive++;
		t = new RICON_SnapshotRun("CMP-09");
		Case8(t);
		if (t.Passed(1)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("CMP-09", true);
		Case8(t);
		if (t.Passed(1)) quietPositive++;
		t = new RICON_SnapshotRun("CMP-25");
		Case9(t);
		if (t.Passed(1)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("CMP-25", true);
		Case9(t);
		if (t.Passed(1)) quietPositive++;
		t = new RICON_SnapshotRun("CMP-26");
		Case10(t);
		if (t.Passed(1)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("CMP-26", true);
		Case10(t);
		if (t.Passed(1)) quietPositive++;
		t = new RICON_SnapshotRun("CMP-10");
		Case11(t);
		if (t.Passed(1)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("CMP-10", true);
		Case11(t);
		if (t.Passed(1)) quietPositive++;
		t = new RICON_SnapshotRun("CMP-11");
		Case12(t);
		if (t.Passed(1)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("CMP-11", true);
		Case12(t);
		if (t.Passed(1)) quietPositive++;
		t = new RICON_SnapshotRun("CMP-12");
		Case13(t);
		if (t.Passed(1)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("CMP-12", true);
		Case13(t);
		if (t.Passed(1)) quietPositive++;
		t = new RICON_SnapshotRun("CMP-13");
		Case14(t);
		if (t.Passed(1)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("CMP-13", true);
		Case14(t);
		if (t.Passed(1)) quietPositive++;
		t = new RICON_SnapshotRun("CMP-14");
		Case15(t);
		if (t.Passed(1)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("CMP-14", true);
		Case15(t);
		if (t.Passed(1)) quietPositive++;
		t = new RICON_SnapshotRun("CMP-15");
		Case16(t);
		if (t.Passed(1)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("CMP-15", true);
		Case16(t);
		if (t.Passed(1)) quietPositive++;
		t = new RICON_SnapshotRun("CMP-16");
		Case17(t);
		if (t.Passed(1)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("CMP-16", true);
		Case17(t);
		if (t.Passed(1)) quietPositive++;
		t = new RICON_SnapshotRun("CMP-17");
		Case18(t);
		if (t.Passed(1)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("CMP-17", true);
		Case18(t);
		if (t.Passed(1)) quietPositive++;
		t = new RICON_SnapshotRun("CMP-18");
		Case19(t);
		if (t.Passed(1)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("CMP-18", true);
		Case19(t);
		if (t.Passed(1)) quietPositive++;
		t = new RICON_SnapshotRun("CMP-19");
		Case20(t);
		if (t.Passed(1)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("CMP-19", true);
		Case20(t);
		if (t.Passed(1)) quietPositive++;
		t = new RICON_SnapshotRun("CMP-20");
		Case21(t);
		if (t.Passed(1)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("CMP-20", true);
		Case21(t);
		if (t.Passed(1)) quietPositive++;
		t = new RICON_SnapshotRun("CMP-21");
		Case22(t);
		if (t.Passed(1)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("CMP-21", true);
		Case22(t);
		if (t.Passed(1)) quietPositive++;
		t = new RICON_SnapshotRun("CMP-22");
		Case23(t);
		if (t.Passed(1)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("CMP-22", true);
		Case23(t);
		if (t.Passed(1)) quietPositive++;
		t = new RICON_SnapshotRun("CMP-23");
		Case24(t);
		if (t.Passed(1)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("CMP-23", true);
		Case24(t);
		if (t.Passed(1)) quietPositive++;
		t = new RICON_SnapshotRun("CMP-24");
		Case25(t);
		if (t.Passed(1)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("CMP-24", true);
		Case25(t);
		if (t.Passed(1)) quietPositive++;
		t = new RICON_SnapshotRun("RX-01");
		Case26(t);
		if (t.Passed(1)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("RX-01", true);
		Case26(t);
		if (t.Passed(1)) quietPositive++;
		t = new RICON_SnapshotRun("RX-02");
		Case27(t);
		if (t.Passed(5)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("RX-02", true);
		Case27(t);
		if (t.Passed(5)) quietPositive++;
		t = new RICON_SnapshotRun("RX-03");
		Case28(t);
		if (t.Passed(5)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("RX-03", true);
		Case28(t);
		if (t.Passed(5)) quietPositive++;
		t = new RICON_SnapshotRun("RX-04");
		Case29(t);
		if (t.Passed(3)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("RX-04", true);
		Case29(t);
		if (t.Passed(3)) quietPositive++;
		t = new RICON_SnapshotRun("RX-05");
		Case30(t);
		if (t.Passed(3)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("RX-05", true);
		Case30(t);
		if (t.Passed(3)) quietPositive++;
		t = new RICON_SnapshotRun("RX-06");
		Case31(t);
		if (t.Passed(5)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("RX-06", true);
		Case31(t);
		if (t.Passed(5)) quietPositive++;
		t = new RICON_SnapshotRun("RX-07");
		Case32(t);
		if (t.Passed(5)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("RX-07", true);
		Case32(t);
		if (t.Passed(5)) quietPositive++;
		t = new RICON_SnapshotRun("RX-08");
		Case33(t);
		if (t.Passed(3)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("RX-08", true);
		Case33(t);
		if (t.Passed(3)) quietPositive++;
		t = new RICON_SnapshotRun("RX-09");
		Case34(t);
		if (t.Passed(4)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("RX-09", true);
		Case34(t);
		if (t.Passed(4)) quietPositive++;
		t = new RICON_SnapshotRun("RX-10");
		Case35(t);
		if (t.Passed(3)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("RX-10", true);
		Case35(t);
		if (t.Passed(3)) quietPositive++;
		t = new RICON_SnapshotRun("RX-11");
		Case36(t);
		if (t.Passed(2)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("RX-11", true);
		Case36(t);
		if (t.Passed(2)) quietPositive++;
		t = new RICON_SnapshotRun("RX-15");
		Case37(t);
		if (t.Passed(2)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("RX-15", true);
		Case37(t);
		if (t.Passed(2)) quietPositive++;
		t = new RICON_SnapshotRun("RX-17");
		Case38(t);
		if (t.Passed(4)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("RX-17", true);
		Case38(t);
		if (t.Passed(4)) quietPositive++;
		t = new RICON_SnapshotRun("RX-16");
		Case39(t);
		if (t.Passed(2)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("RX-16", true);
		Case39(t);
		if (t.Passed(2)) quietPositive++;
		t = new RICON_SnapshotRun("RX-12");
		Case40(t);
		if (t.Passed(2)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("RX-12", true);
		Case40(t);
		if (t.Passed(2)) quietPositive++;
		t = new RICON_SnapshotRun("RX-13");
		Case41(t);
		if (t.Passed(1)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("RX-13", true);
		Case41(t);
		if (t.Passed(1)) quietPositive++;
		t = new RICON_SnapshotRun("RX-14");
		Case42(t);
		if (t.Passed(1)) cases++;
		steps += t.Calls();
		t = new RICON_SnapshotRun("RX-14", true);
		Case42(t);
		if (t.Passed(1)) quietPositive++;
		t = new RICON_SnapshotRun("X-1", true);
		Case43(t);
		if (t.Failed()) rejected++;
		t = new RICON_SnapshotRun("X-2", true);
		Case44(t);
		if (t.Failed()) rejected++;
		t = new RICON_SnapshotRun("X-3", true);
		Case45(t);
		if (t.Failed()) rejected++;
		t = new RICON_SnapshotRun("X-4", true);
		Case46(t);
		if (t.Failed()) rejected++;
		t = new RICON_SnapshotRun("X-5", true);
		Case47(t);
		if (t.Failed()) rejected++;
		t = new RICON_SnapshotRun("X-6", true);
		Case48(t);
		if (t.Failed()) rejected++;
		t = new RICON_SnapshotRun("X-7", true);
		Case49(t);
		if (t.Failed()) rejected++;
		PrintFormat("[RICON_SNAPSHOT] cases=%1 steps=%2 quietPositive=%3 controls=%4 blockedAllocator=3 adapterMapped=1",
			cases, steps, quietPositive, rejected);
		return cases == 43 && steps == 77 && quietPositive == 43 && rejected == 7;
	}
}
