//! STAGING TEST_ONLY: callable pure fixture; no autorun, engine objects or synthetic PASS claim. Compile NOT_RUN.
class RICON_TemporalSnapshotFixtures
{
	static int m_iChecks, m_iFailures, m_iFirstFailure;
	protected static void Check(bool ok)
	{
		m_iChecks++;
		if (!ok) { m_iFailures++; if (m_iFirstFailure == 0) m_iFirstFailure = m_iChecks; }
	}
	protected static RICON_FadingEndpoint Endpoint(int device, int index = 0)
	{
		RICON_FadingEndpoint e = new RICON_FadingEndpoint(); e.m_iDeviceId = device; e.m_iTransceiverIndex = index; return e;
	}
	protected static RICON_TemporalBinding Binding(int rx = 202)
	{
		RICON_TemporalBinding b = new RICON_TemporalBinding();
		b.m_iLeaseId = 11; b.m_iTxGeneration = 1; b.m_iRxGeneration = 1;
		b.m_iTxDeviceId = 101; b.m_iRxDeviceId = rx; b.m_iTxTransceiverIndex = 0; b.m_iRxTransceiverIndex = 0;
		b.m_iNetworkTypeId = 1; b.m_iChannelId = 1; return b;
	}
	protected static RICON_FadingDescriptor Descriptor(int rx = 202)
	{
		RICON_FadingDescriptor d = new RICON_FadingDescriptor();
		d.m_sModelId = "FINITE_MULTIPATH_V1"; d.m_sProfileId = "TEST_ONLY"; d.m_iProfileVersion = 1;
		d.m_A = Endpoint(101); d.m_B = Endpoint(rx); d.m_fFrequencyMHz = 150; d.m_fBandwidthHz = 25000;
		d.m_iEnvironmentRevision = 1; d.m_iAnchorMs = 100; d.m_iValidFromMs = 100; d.m_iValidUntilMs = 1200;
		d.m_bStaticChannel = true; d.m_bCoverageComplete = true;
		RICON_FadingComponent c = new RICON_FadingComponent();
		c.m_sComponentId = "direct"; c.m_fPowerWeight = 1; c.m_iPhaseQ = 0; c.m_iDopplerQ = 0;
		d.m_aComponents.Insert(c); return d;
	}
	protected static RICON_TemporalSnapshot Snapshot(int linkId = 1, int rx = 202, int listener = 2)
	{
		RICON_TemporalSnapshot s = new RICON_TemporalSnapshot();
		s.m_iSchemaVersion = 2; s.m_iSessionEpoch = 7; s.m_iLinkId = linkId; s.m_iLinkSeq = 1;
		s.m_iTtlMs = 1000; s.m_iRouteId = linkId; s.m_iRouteNodeCount = 2;
		s.m_iTalkerPlayerId = 1; s.m_iListenerPlayerId = listener; s.m_eState = RICON_ELinkState.CLEAN; s.m_fQuality = 0.75;
		s.m_sMode = "RECEIVER"; s.m_sTimebaseId = "TEST_CLOCK"; s.m_iDescriptorVersion = 1;
		s.m_Binding = Binding(rx); s.m_Descriptor = Descriptor(rx); return s;
	}
	protected static RICON_TemporalExpected Expected(int linkId = 1, int rx = 202, int listener = 2)
	{
		RICON_TemporalExpected e = new RICON_TemporalExpected();
		e.m_iSessionEpoch = 7; e.m_iLinkId = linkId; e.m_iTalkerPlayerId = 1; e.m_iListenerPlayerId = listener;
		e.m_sMode = "RECEIVER"; e.m_sTimebaseId = "TEST_CLOCK"; e.m_Binding = Binding(rx);
		e.m_Channel = new RICON_TemporalChannelContext();
		e.m_Channel.m_Tx = Endpoint(101); e.m_Channel.m_Rx = Endpoint(rx);
		e.m_Channel.m_fFrequencyMHz = 150; e.m_Channel.m_fBandwidthHz = 25000;
		e.m_Channel.m_sProfileId = "TEST_ONLY"; e.m_Channel.m_iProfileVersion = 1; e.m_Channel.m_iEnvironmentRevision = 1;
		return e;
	}
	protected static RICON_TemporalMapping Mapping(int errorUs = 0)
	{
		RICON_TemporalMapping m = new RICON_TemporalMapping();
		m.m_sMappingId = "SYNTHETIC_TEST_ONLY"; m.m_sTimebaseId = "TEST_CLOCK";
		m.m_iLocalAnchorMs = 100; m.m_iServerAnchorMs = 100;
		m.m_iValidFromLocalMs = 0; m.m_iValidUntilLocalMs = 2000; m.m_iMaxErrorUs = errorUs; return m;
	}
	protected static map<int, ref RICON_TemporalExpected> Contexts()
	{
		map<int, ref RICON_TemporalExpected> all = new map<int, ref RICON_TemporalExpected>();
		all.Set(1, Expected()); all.Set(2, Expected(2, 203, 3)); return all;
	}

	protected static void ClockCases()
	{
		int em, es, lm, ls, um, us;
		Check(RICON_TemporalClock.MapInterval(1199, 999, 100, 100, 0, 2000, 0, em, es, lm, ls, um, us) == 0);
		Check(em == 1199 && es == 999 && lm == 1199 && ls == 999 && um == 1199 && us == 999);
		Check(RICON_TemporalClock.ContainsInterval(lm, ls, um, us, 100, 1200));
		Check(RICON_TemporalClock.MapInterval(1199, 999, 100, 100, 0, 2000, 1, em, es, lm, ls, um, us) == 0);
		Check(lm == 1199 && ls == 998 && um == 1200 && us == 0);
		Check(!RICON_TemporalClock.ContainsInterval(lm, ls, um, us, 100, 1200));
		Check(RICON_TemporalClock.MapInterval(100, 0, 100, 100, 0, 2000, 1, em, es, lm, ls, um, us) == 0);
		Check(lm == 99 && ls == 999 && um == 100 && us == 1);
		Check(!RICON_TemporalClock.ContainsInterval(lm, ls, um, us, 100, 1200));
		Check(RICON_TemporalClock.MapInterval(int.MAX - 1, 999, 0, 1, 0, int.MAX, 0, em, es, lm, ls, um, us) == 0);
		Check(em == int.MAX && es == 999 && um == int.MAX && us == 999);
		Check(RICON_TemporalClock.MapInterval(int.MAX - 1, 999, 0, 1, 0, int.MAX, 1, em, es, lm, ls, um, us) == 2);
		Check(RICON_TemporalClock.MapInterval(int.MAX - 1, 0, 0, 2, 0, int.MAX, 0, em, es, lm, ls, um, us) == 2);
		Check(RICON_TemporalClock.MapInterval(0, 0, int.MAX - 1, int.MAX - 1, 0, int.MAX, 0, em, es, lm, ls, um, us) == 0);
		Check(em == 0 && lm == 0 && um == 0);
		Check(RICON_TemporalClock.MapInterval(0, 0, int.MAX - 1, int.MAX - 1, 0, int.MAX, 1, em, es, lm, ls, um, us) == 2);
		Check(RICON_TemporalClock.MapInterval(100, 1000, 100, 100, 0, 2000, 0, em, es, lm, ls, um, us) == 1);
	}

	protected static void ValidateCases()
	{
		RICON_TemporalLimits limits = new RICON_TemporalLimits();
		limits.m_iMaxDescriptorBytes = 102; limits.m_iMaxTotalDescriptorBytes = 408;
		RICON_TemporalSnapshot s = Snapshot();
		RICON_TemporalExpected e = Expected();
		RICON_TemporalValidationResult r = RICON_TemporalSnapshots.Validate(s, e, limits);
		Check(r.m_sCode == "OK" && r.m_iDescriptorBytes == 102 && r.m_Snapshot != s);
		s.m_Descriptor.m_aComponents[0].m_iPhaseQ = 25;
		Check(r.m_Snapshot.m_Descriptor.m_aComponents[0].m_iPhaseQ == 0);
		s = Snapshot(); limits.m_iMaxDescriptorBytes = 101;
		Check(RICON_TemporalSnapshots.Validate(s, e, limits).m_sCode == "RESOURCE_LIMIT");
		limits.m_iMaxDescriptorBytes = 102; s.m_iSchemaVersion = 1;
		Check(RICON_TemporalSnapshots.Validate(s, e, limits).m_sCode == "UNSUPPORTED_SCHEMA");
		s.m_iSchemaVersion = 2; s.m_iDescriptorVersion = 2;
		Check(RICON_TemporalSnapshots.Validate(s, e, limits).m_sCode == "UNSUPPORTED_DESCRIPTOR_VERSION");
		s = Snapshot(); s.m_sMode = "OTHER";
		Check(RICON_TemporalSnapshots.Validate(s, e, limits).m_sCode == "UNSUPPORTED_MODE");
		s = Snapshot(); s.m_Descriptor = null;
		Check(RICON_TemporalSnapshots.Validate(s, e, limits).m_sCode == "INVALID_DESCRIPTOR");
		s.m_iSchemaVersion = 0;
		Check(RICON_TemporalSnapshots.Validate(s, e, limits).m_sCode == "INVALID_SNAPSHOT");
		s = Snapshot(); s.m_eState = RICON_ELinkState.LOST; s.m_fQuality = 0;
		Check(RICON_TemporalSnapshots.Validate(s, e, limits).m_sCode == "OK");
		s.m_Descriptor.m_aComponents.Clear();
		Check(RICON_TemporalSnapshots.Validate(s, e, limits).m_sCode == "OK");
		s = Snapshot(); s.m_Descriptor.m_sModelId = "OTHER_MODEL";
		Check(RICON_TemporalSnapshots.Validate(s, e, limits).m_sCode == "UNSUPPORTED_MODEL");
		s = Snapshot(); s.m_Descriptor.m_fFrequencyMHz = 151;
		limits.m_iMaxDescriptorBytes = 5111;
		Check(RICON_TemporalSnapshots.Validate(s, e, limits).m_sCode == "CONTEXT_MISMATCH");
		s = Snapshot(); e = Expected();
		s.m_Binding.m_iTxDeviceId = 202; s.m_Binding.m_iRxDeviceId = 101;
		e.m_Binding.m_iTxDeviceId = 202; e.m_Binding.m_iRxDeviceId = 101;
		e.m_Channel.m_Tx = Endpoint(202); e.m_Channel.m_Rx = Endpoint(101);
		Check(RICON_TemporalSnapshots.Validate(s, e, limits).m_sCode == "OK");
		s = Snapshot();
		for (int i = 0; i < 64; i++) { RICON_FadingComponent extra = new RICON_FadingComponent(); s.m_Descriptor.m_aComponents.Insert(extra); }
		Check(RICON_TemporalSnapshots.Validate(s, Expected(), limits).m_sCode == "INVALID_DESCRIPTOR");
	}

	protected static void ReceiverCases()
	{
		RICON_TemporalLimits limits = new RICON_TemporalLimits(); limits.m_iMaxSnapshots = 4; limits.m_iMaxTotalDescriptorBytes = 408;
		map<int, ref RICON_TemporalExpected> contexts = Contexts();
		RICON_TemporalSnapshotReceiver rx = new RICON_TemporalSnapshotReceiver(contexts, limits);
		RICON_TemporalSnapshot s = Snapshot();
		Check(rx.Receive(100, s).m_eDecision == RICON_ESnapshotDecision.INVALID_IDENTITY);
		array<ref RICON_TemporalSnapshot> batch = {s};
		Check(rx.ApplyFullResync(100, 7, 2, batch).m_eDecision == RICON_ESnapshotDecision.ACCEPT_RESYNC);
		Check(rx.HasIdentity() && rx.SlotCount() == 1 && rx.TotalDescriptorBytes() == 102);
		s.m_Descriptor.m_aComponents[0].m_iPhaseQ = 99; contexts.Get(1).m_Channel.m_fFrequencyMHz = 151;
		limits.m_iMaxSnapshots = 0; limits.m_iMaxDescriptorBytes = 0; limits.m_iMaxTotalDescriptorBytes = 1;
		RICON_TemporalSnapshot read = rx.Read(1);
		Check(read.m_Descriptor.m_aComponents[0].m_iPhaseQ == 0);
		read.m_Descriptor.m_A.m_iDeviceId = 999;
		Check(rx.Read(1).m_Descriptor.m_A.m_iDeviceId == 101);
		Check(rx.View(110, 1, null).m_sCode == "CLOCK_UNVERIFIED");
		Check(rx.View(110, 1, Mapping()).m_sCode == "OK");
		s = Snapshot(); s.m_iLinkSeq = 2; s.m_Descriptor.m_aComponents[0].m_iPhaseQ = 123;
		Check(rx.Receive(600, s).m_eDecision == RICON_ESnapshotDecision.ACCEPT_NEWER);
		Check(rx.Read(1).m_Descriptor.m_aComponents[0].m_iPhaseQ == 123);
		s.m_Descriptor.m_aComponents[0].m_iPhaseQ = 999;
		Check(rx.Read(1).m_Descriptor.m_aComponents[0].m_iPhaseQ == 123);
		read = rx.Read(1); read.m_Descriptor.m_aComponents[0].m_iPhaseQ = 444;
		Check(rx.Read(1).m_Descriptor.m_aComponents[0].m_iPhaseQ == 123);
		Check(rx.Receive(900, s).m_eDecision == RICON_ESnapshotDecision.STALE);
		RICON_TemporalViewResult view = rx.View(1200, 1, Mapping());
		Check(view.m_bVisible && view.m_sCode == "STALE_DESCRIPTOR" && !view.m_Fading);
		bool diagnostic; Check(!rx.IsVisible(1600, 1, diagnostic));
		RICON_TemporalSnapshot bad = Snapshot(2, 203, 3); bad.m_Descriptor.m_aComponents[0].m_iPhaseQ = 1073741824;
		batch = {Snapshot(), bad};
		Check(rx.ApplyFullResync(1000, 7, 2, batch).m_eDecision == RICON_ESnapshotDecision.INVALID_RESYNC);
		Check(!rx.HasIdentity() && rx.SlotCount() == 0 && rx.TotalDescriptorBytes() == 0);
		batch = {Snapshot()};
		Check(rx.ApplyFullResync(1000, 7, 1, batch).m_sReason == "UNSUPPORTED_SCHEMA");
		Check(!rx.HasIdentity());
		Check(rx.ApplyFullResync(1000, 7, 2, batch).m_eDecision == RICON_ESnapshotDecision.ACCEPT_RESYNC);
		s = Snapshot(); s.m_iSessionEpoch = 8;
		Check(rx.Receive(1001, s).m_eDecision == RICON_ESnapshotDecision.INVALID_IDENTITY && !rx.HasIdentity());
	}

	protected static void RetainedLimitsCases()
	{
		RICON_TemporalLimits limits = new RICON_TemporalLimits(); limits.m_iMaxSnapshots = 1;
		RICON_TemporalSnapshotReceiver rx = new RICON_TemporalSnapshotReceiver(Contexts(), limits);
		array<ref RICON_TemporalSnapshot> one = {Snapshot()};
		Check(rx.ApplyFullResync(100, 7, 2, one).m_eDecision == RICON_ESnapshotDecision.ACCEPT_RESYNC);
		Check(rx.Receive(101, Snapshot(2, 203, 3)).m_sReason == "BATCH_LIMIT" && rx.SlotCount() == 1);
		limits.m_iMaxSnapshots = 2; limits.m_iMaxTotalDescriptorBytes = 203;
		rx = new RICON_TemporalSnapshotReceiver(Contexts(), limits);
		Check(rx.ApplyFullResync(100, 7, 2, one).m_eDecision == RICON_ESnapshotDecision.ACCEPT_RESYNC);
		Check(rx.Receive(101, Snapshot(2, 203, 3)).m_sReason == "RESOURCE_LIMIT" && rx.TotalDescriptorBytes() == 102);
		array<ref RICON_TemporalSnapshot> two = {Snapshot(), Snapshot(2, 203, 3)};
		Check(rx.ApplyFullResync(102, 7, 2, two).m_sReason == "RESOURCE_LIMIT" && !rx.HasIdentity());
		array<ref RICON_TemporalSnapshot> empty = {};
		Check(rx.ApplyFullResync(103, 7, 2, empty).m_eDecision == RICON_ESnapshotDecision.ACCEPT_RESYNC && rx.HasIdentity());
	}

	static bool RunAll()
	{
		m_iChecks = 0; m_iFailures = 0; m_iFirstFailure = 0;
		ClockCases(); ValidateCases(); ReceiverCases(); RetainedLimitsCases();
		return m_iFailures == 0;
	}
}
