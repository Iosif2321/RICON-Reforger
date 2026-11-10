//! STAGING TEST_ONLY callable fixtures, no autorun/actor. Compile and Enforce execution NOT_RUN.
class RICON_TemporalLifecycleFixtures
{
	static int m_iChecks, m_iFailures, m_iFirstFailure;
	protected static void Check(bool ok)
	{
		m_iChecks++;
		if (!ok) { m_iFailures++; if (m_iFirstFailure == 0) m_iFirstFailure = m_iChecks; }
	}
	static RICON_FadingEndpoint Endpoint(int device)
	{
		RICON_FadingEndpoint e = new RICON_FadingEndpoint(); e.m_iDeviceId = device; e.m_iTransceiverIndex = 0; return e;
	}
	static RICON_TemporalBinding Binding(int link = 1, int rx = 202)
	{
		RICON_TemporalBinding b = new RICON_TemporalBinding();
		b.m_iLeaseId = 10 + link; b.m_iTxGeneration = 1; b.m_iRxGeneration = 1;
		b.m_iTxDeviceId = 101; b.m_iRxDeviceId = rx; b.m_iTxTransceiverIndex = 0; b.m_iRxTransceiverIndex = 0;
		b.m_iNetworkTypeId = 1; b.m_iChannelId = 1; return b;
	}
	static RICON_TemporalSnapshot Snapshot(int link = 1, int rx = 202, int epoch = 7)
	{
		RICON_TemporalSnapshot s = new RICON_TemporalSnapshot();
		s.m_iSchemaVersion = 2; s.m_iSessionEpoch = epoch; s.m_iLinkId = link; s.m_iLinkSeq = 1;
		s.m_iTtlMs = 1000; s.m_iRouteId = link; s.m_iRouteNodeCount = 2; s.m_iTalkerPlayerId = 1; s.m_iListenerPlayerId = 2;
		s.m_eState = RICON_ELinkState.CLEAN; s.m_fQuality = 0.75; s.m_sMode = "RECEIVER"; s.m_sTimebaseId = "TEST_CLOCK";
		s.m_iDescriptorVersion = 1; s.m_Binding = Binding(link, rx);
		s.m_Descriptor = new RICON_FadingDescriptor();
		s.m_Descriptor.m_sModelId = "FINITE_MULTIPATH_V1"; s.m_Descriptor.m_sProfileId = "TEST_ONLY";
		s.m_Descriptor.m_iProfileVersion = 1; s.m_Descriptor.m_A = Endpoint(101); s.m_Descriptor.m_B = Endpoint(rx);
		s.m_Descriptor.m_fFrequencyMHz = 150; s.m_Descriptor.m_fBandwidthHz = 25000; s.m_Descriptor.m_iEnvironmentRevision = 1;
		s.m_Descriptor.m_iAnchorMs = 100; s.m_Descriptor.m_iValidFromMs = 100; s.m_Descriptor.m_iValidUntilMs = 1200;
		s.m_Descriptor.m_bStaticChannel = true; s.m_Descriptor.m_bCoverageComplete = true;
		RICON_FadingComponent c = new RICON_FadingComponent(); c.m_sComponentId = "direct"; c.m_fPowerWeight = 1;
		s.m_Descriptor.m_aComponents.Insert(c); return s;
	}
	static RICON_TemporalExpected Expected(int link = 1, int rx = 202, int epoch = 7)
	{
		RICON_TemporalExpected e = new RICON_TemporalExpected();
		e.m_iSessionEpoch = epoch; e.m_iLinkId = link; e.m_iTalkerPlayerId = 1; e.m_iListenerPlayerId = 2;
		e.m_sMode = "RECEIVER"; e.m_sTimebaseId = "TEST_CLOCK"; e.m_Binding = Binding(link, rx);
		e.m_Channel = new RICON_TemporalChannelContext(); e.m_Channel.m_Tx = Endpoint(101); e.m_Channel.m_Rx = Endpoint(rx);
		e.m_Channel.m_fFrequencyMHz = 150; e.m_Channel.m_fBandwidthHz = 25000; e.m_Channel.m_sProfileId = "TEST_ONLY";
		e.m_Channel.m_iProfileVersion = 1; e.m_Channel.m_iEnvironmentRevision = 1; return e;
	}
	//! Literal independent END receipt; deliberately does not call production FromSnapshot.
	static RICON_TemporalEndRecord End(int link = 1, int rx = 202, int epoch = 7)
	{
		RICON_TemporalEndRecord e = new RICON_TemporalEndRecord();
		e.m_iSessionEpoch = epoch; e.m_iLinkId = link; e.m_iLinkSeq = 2; e.m_iTalkerPlayerId = 1; e.m_iListenerPlayerId = 2;
		e.m_Binding = Binding(link, rx); e.m_sReason = "CLIENT_STOP"; return e;
	}
	static RICON_TemporalProtocolItem ActiveItem(RICON_TemporalSnapshot snapshot)
	{
		RICON_TemporalProtocolItem i = new RICON_TemporalProtocolItem(); i.m_sKind = "ACTIVE"; i.m_Snapshot = snapshot; return i;
	}
	static RICON_TemporalProtocolItem EndItem(RICON_TemporalEndRecord end)
	{
		RICON_TemporalProtocolItem i = new RICON_TemporalProtocolItem(); i.m_sKind = "END"; i.m_End = end; return i;
	}
	static RICON_TemporalProtocolEnvelope Frame(string kind, int seq, int high, int epoch = 7)
	{
		RICON_TemporalProtocolEnvelope f = new RICON_TemporalProtocolEnvelope();
		f.m_sKind = kind; f.m_iOwnerPlayerId = 2; f.m_iSessionEpoch = epoch; f.m_iDeliverySeq = seq; f.m_iLinkIdHighWater = high; return f;
	}
	static RICON_TemporalAuthority Authority(string kind, int seq, int high, int epoch = 7)
	{
		RICON_TemporalAuthority a = new RICON_TemporalAuthority();
		a.m_sKind = kind; a.m_iOwnerPlayerId = 2; a.m_iSessionEpoch = epoch; a.m_iDeliverySeq = seq; a.m_iLinkIdHighWater = high; return a;
	}
	static RICON_TemporalOrderedReceiver Established(int cap = 4)
	{
		RICON_TemporalLimits limits = new RICON_TemporalLimits(); limits.m_iMaxSnapshots = cap; limits.m_iMaxTotalDescriptorBytes = 408;
		RICON_TemporalOrderedReceiver r = new RICON_TemporalOrderedReceiver(2, limits);
		RICON_TemporalProtocolEnvelope f = Frame("FULL", 1, 1); f.m_aItems.Insert(ActiveItem(Snapshot()));
		RICON_TemporalAuthority a = Authority("FULL", 1, 1); a.m_ActiveExpected.Set(1, Expected());
		Check(r.Apply(100, f, a).m_sCode == "ACCEPT_FULL"); return r;
	}

	protected static bool SameState(RICON_TemporalProtocolState a, RICON_TemporalProtocolState b)
	{
		if (a.m_iOwnerPlayerId != b.m_iOwnerPlayerId || a.m_iSessionEpoch != b.m_iSessionEpoch || a.m_iDeliverySeq != b.m_iDeliverySeq) return false;
		if (a.m_iCheckpointHighWater != b.m_iCheckpointHighWater || a.m_iSeenHighWater != b.m_iSeenHighWater) return false;
		if (a.m_iObservedEpoch != b.m_iObservedEpoch || a.m_iObservedDeliverySeq != b.m_iObservedDeliverySeq) return false;
		if (a.m_iObservedHighWater != b.m_iObservedHighWater || a.m_bNeedsFull != b.m_bNeedsFull) return false;
		if (a.m_Active.Count() != b.m_Active.Count() || a.m_Closed.Count() != b.m_Closed.Count()) return false;
		for (int i = 0; i < a.m_Active.Count(); i++)
		{
			int link = a.m_Active.GetKey(i); RICON_TemporalProtocolRow left = a.m_Active.Get(link); RICON_TemporalProtocolRow right = b.m_Active.Get(link);
			if (!right || left.m_iReceivedAtMs != right.m_iReceivedAtMs || !RICON_TemporalProtocolRules.SameSnapshot(left.m_Snapshot, right.m_Snapshot)) return false;
		}
		for (int j = 0; j < a.m_Closed.Count(); j++) { int closed = a.m_Closed.GetKey(j); if (!a.m_Closed.Get(closed).Same(b.m_Closed.Get(closed))) return false; }
		return true;
	}
	protected static void FactoryAndCopies()
	{
		RICON_TemporalSnapshot s = Snapshot(); s.m_Descriptor = null;
		RICON_TemporalEndRecord end = RICON_TemporalEndRecord.FromSnapshot(s, 2, "CLIENT_STOP");
		Check(end && end.Same(End())); s.m_Binding.m_iLeaseId = 999; Check(end.m_Binding.m_iLeaseId == 11);
		Check(RICON_TemporalEndRecord.LeaseReasonName(RICON_ELeaseTerminalReason.TTL_EXPIRED) == "TTL_EXPIRED");
		Check(RICON_TemporalEndRecord.LeaseReasonName(RICON_ELeaseTerminalReason.NONE) == "");
		Check(!RICON_TemporalEndRecord.ValidReason("LOST") && !RICON_TemporalEndRecord.ValidReason("NONE"));
		Check(!RICON_TemporalEndRecord.FromSnapshot(Snapshot(), int.MAX, "CLIENT_STOP"));
		RICON_TemporalOrderedReceiver r = Established();
		RICON_TemporalProtocolState state = r.State(); state.m_Active.Get(1).m_Snapshot.m_Binding.m_iLeaseId = 222;
		Check(r.Read(1).m_Binding.m_iLeaseId == 11);
		RICON_TemporalSnapshot read = r.Read(1); read.m_Descriptor.m_aComponents[0].m_iPhaseQ = 25;
		Check(r.Read(1).m_Descriptor.m_aComponents[0].m_iPhaseQ == 0);
	}
	protected static void EndAndReplay()
	{
		RICON_TemporalOrderedReceiver r = Established();
		RICON_TemporalProtocolEnvelope f = Frame("DELTA", 2, 1); f.m_aItems.Insert(EndItem(End()));
		RICON_TemporalAuthority a = Authority("DELTA", 2, 1); a.m_EndedTargets.Set(1, End());
		Check(r.Apply(101, f, a).m_sCode == "ACCEPT_DELTA");
		Check(!r.Read(1) && r.State().m_Closed.Get(1).m_iLinkSeq == 2 && !r.IsVisible(102, 1));
		f = Frame("DELTA", 3, 1); RICON_TemporalSnapshot s = Snapshot(); s.m_iLinkSeq = 7; f.m_aItems.Insert(ActiveItem(s));
		a = Authority("DELTA", 3, 1); a.m_ActiveExpected.Set(1, Expected());
		Check(r.Apply(102, f, a).m_sCode == "CLOSED_ID");
		RICON_TemporalProtocolState before = r.State(); f = Frame("FULL", 1, 1); f.m_aItems = null; a = Authority("FULL", 1, 1);
		Check(r.Apply(999, f, a).m_sCode == "STALE" && SameState(before, r.State()));
	}
	protected static void CheckpointAndCaps()
	{
		RICON_TemporalOrderedReceiver r = Established(2);
		RICON_TemporalProtocolEnvelope f = Frame("DELTA", 2, 2); f.m_aItems.Insert(EndItem(End(2, 203)));
		RICON_TemporalAuthority a = Authority("DELTA", 2, 2); a.m_ActiveExpected.Set(1, Expected()); a.m_EndedTargets.Set(2, End(2, 203));
		Check(r.Apply(101, f, a).m_sCode == "ACCEPT_DELTA");
		f = Frame("DELTA", 3, 3); f.m_aItems.Insert(EndItem(End(3, 204)));
		a = Authority("DELTA", 3, 3); a.m_ActiveExpected.Set(1, Expected()); a.m_EndedTargets.Set(3, End(3, 204));
		Check(r.Apply(102, f, a).m_sCode == "RESOURCE_LIMIT");
		RICON_TemporalProtocolState state = r.State();
		Check(state.m_iDeliverySeq == 2 && state.m_iSeenHighWater == 3 && state.m_iObservedDeliverySeq == 3
			&& state.m_Closed.Count() == 1 && state.m_Closed.Contains(2) && state.m_bNeedsFull && !r.IsVisible(103, 1));
		f = Frame("FULL", 4, 2); f.m_aItems.Insert(ActiveItem(Snapshot())); a = Authority("FULL", 4, 2); a.m_ActiveExpected.Set(1, Expected());
		Check(r.Apply(104, f, a).m_sCode == "HIGH_WATER_ROLLBACK");
		f = Frame("FULL", 5, 3); f.m_aItems.Insert(ActiveItem(Snapshot())); a = Authority("FULL", 5, 3); a.m_ActiveExpected.Set(1, Expected());
		Check(r.Apply(105, f, a).m_sCode == "ACCEPT_FULL"); state = r.State();
		Check(state.m_iCheckpointHighWater == 3 && state.m_Closed.Count() == 0 && state.m_Active.Get(1).m_iReceivedAtMs == 100);
		RICON_TemporalSnapshot delayed = Snapshot(2, 203); delayed.m_iLinkSeq = 7;
		f = Frame("DELTA", 6, 3); f.m_aItems.Insert(ActiveItem(delayed)); a = Authority("DELTA", 6, 3);
		a.m_ActiveExpected.Set(1, Expected()); a.m_ActiveExpected.Set(2, Expected(2, 203));
		Check(r.Apply(106, f, a).m_sCode == "CLOSED_ID");
	}
	protected static void GapPendingAndAtomicPayload()
	{
		RICON_TemporalOrderedReceiver r = Established();
		RICON_TemporalProtocolEnvelope f = Frame("DELTA", 5, 3); f.m_aItems.Insert(EndItem(End(3, 204)));
		RICON_TemporalAuthority a = Authority("DELTA", 5, 3); a.m_ActiveExpected.Set(1, Expected()); a.m_EndedTargets.Set(3, End(3, 204));
		Check(r.Apply(101, f, a).m_sCode == "GAP");
		RICON_TemporalProtocolState before = r.State();
		f = Frame("FULL", 4, 3); f.m_aItems.Insert(ActiveItem(Snapshot())); a = Authority("FULL", 4, 3); a.m_ActiveExpected.Set(1, Expected());
		Check(r.Apply(102, f, a).m_sCode == "STALE" && SameState(before, r.State()));
		f = Frame("FULL", 6, 3); RICON_TemporalSnapshot bad = Snapshot(2, 203); bad.m_Descriptor.m_aComponents[0].m_iPhaseQ = 1073741824;
		f.m_aItems.Insert(ActiveItem(Snapshot())); f.m_aItems.Insert(ActiveItem(bad));
		a = Authority("FULL", 6, 3); a.m_ActiveExpected.Set(1, Expected()); a.m_ActiveExpected.Set(2, Expected(2, 203));
		Check(r.Apply(103, f, a).m_sCode == "INVALID_DESCRIPTOR");
		Check(r.State().m_iDeliverySeq == 1 && r.State().m_iObservedDeliverySeq == 6 && r.State().m_Active.Count() == 1 && !r.Read(2));
		f = Frame("FULL", 1, 2, 8); bad = Snapshot(1, 202, 8); bad.m_Descriptor.m_aComponents[0].m_iPhaseQ = 1073741824;
		f.m_aItems.Insert(ActiveItem(bad)); a = Authority("FULL", 1, 2, 8); a.m_ActiveExpected.Set(1, Expected(1, 202, 8));
		Check(r.Apply(104, f, a).m_sCode == "INVALID_DESCRIPTOR");
		RICON_TemporalProtocolState state = r.State();
		Check(state.m_iSessionEpoch == 7 && state.m_iCheckpointHighWater == 1 && state.m_iSeenHighWater == 3
			&& state.m_iObservedEpoch == 8 && state.m_iObservedDeliverySeq == 1 && state.m_iObservedHighWater == 2);
		f = Frame("FULL", 2, 1, 8); f.m_aItems.Insert(ActiveItem(Snapshot(1, 202, 8)));
		a = Authority("FULL", 2, 1, 8); a.m_ActiveExpected.Set(1, Expected(1, 202, 8));
		Check(r.Apply(105, f, a).m_sCode == "HIGH_WATER_ROLLBACK");
		f = Frame("FULL", 3, 2, 8); f.m_aItems.Insert(ActiveItem(Snapshot(1, 202, 8)));
		a = Authority("FULL", 3, 2, 8); a.m_ActiveExpected.Set(1, Expected(1, 202, 8));
		Check(r.Apply(106, f, a).m_sCode == "ACCEPT_FULL" && r.State().m_iSessionEpoch == 8);
	}
	protected static void AuthorityAndRetainedContext()
	{
		RICON_TemporalOrderedReceiver r = Established(); RICON_TemporalProtocolState before = r.State();
		RICON_TemporalProtocolEnvelope f = Frame("DELTA", 2, 1); f.m_aItems.Insert(EndItem(End()));
		RICON_TemporalAuthority a = Authority("DELTA", 2, 1);
		Check(r.Apply(101, f, a).m_sCode == "INVALID_AUTHORITY" && SameState(before, r.State()));
		a.m_EndedTargets.Set(1, End()); a.m_EndedTargets.Get(1).m_Binding.m_iLeaseId = 99;
		Check(r.Apply(101, f, a).m_sCode == "INVALID_AUTHORITY" && SameState(before, r.State()));
		f = Frame("DELTA", 2, 1); a = Authority("DELTA", 2, 1); a.m_ActiveExpected.Set(1, Expected()); a.m_ActiveExpected.Get(1).m_iLinkId = 2;
		Check(r.Apply(101, f, a).m_sCode == "INVALID_AUTHORITY" && SameState(before, r.State()));
		a = Authority("DELTA", 2, 1); a.m_ActiveExpected.Set(1, Expected()); a.m_ActiveExpected.Get(1).m_Channel.m_Tx.m_iDeviceId = 999;
		Check(r.Apply(101, f, a).m_sCode == "INVALID_AUTHORITY" && SameState(before, r.State()));
		a = Authority("DELTA", 2, 1); a.m_ActiveExpected.Set(1, Expected()); a.m_ActiveExpected.Get(1).m_Binding.m_iTxGeneration = 2;
		Check(r.Apply(101, f, a).m_sCode == "CONTEXT_MISMATCH");
		Check(r.State().m_iDeliverySeq == 1 && r.State().m_bNeedsFull && r.Read(1).m_Binding.m_iTxGeneration == 1);
		r = Established(); f = Frame("DELTA", 2, 1); a = Authority("DELTA", 2, 1);
		Check(r.Apply(102, f, a).m_sCode == "COMPLETENESS_MISMATCH" && r.Read(1));
	}

	static bool RunAll()
	{
		m_iChecks = 0; m_iFailures = 0; m_iFirstFailure = 0;
		FactoryAndCopies(); EndAndReplay(); CheckpointAndCaps(); GapPendingAndAtomicPayload(); AuthorityAndRetainedContext();
		return m_iFailures == 0;
	}
}
