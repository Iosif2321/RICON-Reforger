//! STAGING: separate schema2 typed port of REQ-temporal-snapshot. Compile NOT_RUN; no wire/actuator.
class RICON_TemporalBinding
{
	int m_iLeaseId, m_iTxGeneration, m_iRxGeneration, m_iTxDeviceId, m_iRxDeviceId;
	int m_iTxTransceiverIndex = -1;
	int m_iRxTransceiverIndex = -1;
	int m_iNetworkTypeId, m_iChannelId;

	bool IsValid()
	{
		return m_iLeaseId > 0 && m_iTxGeneration > 0 && m_iRxGeneration > 0
			&& m_iTxDeviceId > 0 && m_iRxDeviceId > 0 && m_iTxDeviceId != m_iRxDeviceId
			&& m_iTxTransceiverIndex >= 0 && m_iRxTransceiverIndex >= 0
			&& m_iNetworkTypeId > 0 && m_iChannelId > 0;
	}

	bool Same(RICON_TemporalBinding b)
	{
		return b && m_iLeaseId == b.m_iLeaseId && m_iTxGeneration == b.m_iTxGeneration
			&& m_iRxGeneration == b.m_iRxGeneration && m_iTxDeviceId == b.m_iTxDeviceId
			&& m_iRxDeviceId == b.m_iRxDeviceId && m_iTxTransceiverIndex == b.m_iTxTransceiverIndex
			&& m_iRxTransceiverIndex == b.m_iRxTransceiverIndex
			&& m_iNetworkTypeId == b.m_iNetworkTypeId && m_iChannelId == b.m_iChannelId;
	}

	RICON_TemporalBinding CloneBinding()
	{
		RICON_TemporalBinding c = new RICON_TemporalBinding();
		c.m_iLeaseId = m_iLeaseId; c.m_iTxGeneration = m_iTxGeneration; c.m_iRxGeneration = m_iRxGeneration;
		c.m_iTxDeviceId = m_iTxDeviceId; c.m_iRxDeviceId = m_iRxDeviceId;
		c.m_iTxTransceiverIndex = m_iTxTransceiverIndex; c.m_iRxTransceiverIndex = m_iRxTransceiverIndex;
		c.m_iNetworkTypeId = m_iNetworkTypeId; c.m_iChannelId = m_iChannelId;
		return c;
	}
}

//! CF05 context without time: local receipt time cannot hide in the expected channel.
class RICON_TemporalChannelContext
{
	ref RICON_FadingEndpoint m_Tx;
	ref RICON_FadingEndpoint m_Rx;
	float m_fFrequencyMHz, m_fBandwidthHz;
	string m_sProfileId;
	int m_iProfileVersion, m_iEnvironmentRevision;

	bool IsValid()
	{
		return RICON_TemporalSnapshots.ValidEndpoint(m_Tx) && RICON_TemporalSnapshots.ValidEndpoint(m_Rx)
			&& !RICON_TemporalSnapshots.SameEndpoint(m_Tx, m_Rx)
			&& RICON_Num.IsFinite(m_fFrequencyMHz) && m_fFrequencyMHz > 0
			&& RICON_Num.IsFinite(m_fBandwidthHz) && m_fBandwidthHz > 0
			&& RICON_TemporalSnapshots.ValidKey(m_sProfileId) && m_iProfileVersion > 0 && m_iEnvironmentRevision > 0;
	}

	RICON_FadingQuery Query(int nowMs, int subUs)
	{
		RICON_FadingQuery q = new RICON_FadingQuery();
		q.m_Tx = RICON_TemporalSnapshots.CloneEndpoint(m_Tx); q.m_Rx = RICON_TemporalSnapshots.CloneEndpoint(m_Rx);
		q.m_fFrequencyMHz = m_fFrequencyMHz; q.m_fBandwidthHz = m_fBandwidthHz;
		q.m_sProfileId = m_sProfileId; q.m_iProfileVersion = m_iProfileVersion;
		q.m_iEnvironmentRevision = m_iEnvironmentRevision; q.m_iNowMs = nowMs; q.m_iSubUs = subUs;
		return q;
	}

	RICON_TemporalChannelContext CloneContext()
	{
		RICON_TemporalChannelContext c = new RICON_TemporalChannelContext();
		c.m_Tx = RICON_TemporalSnapshots.CloneEndpoint(m_Tx); c.m_Rx = RICON_TemporalSnapshots.CloneEndpoint(m_Rx);
		c.m_fFrequencyMHz = m_fFrequencyMHz; c.m_fBandwidthHz = m_fBandwidthHz;
		c.m_sProfileId = m_sProfileId; c.m_iProfileVersion = m_iProfileVersion;
		c.m_iEnvironmentRevision = m_iEnvironmentRevision;
		return c;
	}
}

class RICON_TemporalExpected
{
	int m_iSessionEpoch, m_iLinkId, m_iTalkerPlayerId, m_iListenerPlayerId;
	string m_sMode, m_sTimebaseId;
	ref RICON_TemporalBinding m_Binding;
	ref RICON_TemporalChannelContext m_Channel;

	bool IsValid()
	{
		return m_iSessionEpoch > 0 && m_iSessionEpoch < int.MAX && m_iLinkId > 0
			&& m_iTalkerPlayerId > 0 && m_iListenerPlayerId > 0 && m_iTalkerPlayerId != m_iListenerPlayerId
			&& (m_sMode == "SERVER" || m_sMode == "RECEIVER") && RICON_TemporalSnapshots.ValidKey(m_sTimebaseId)
			&& m_Binding && m_Binding.IsValid() && m_Channel && m_Channel.IsValid();
	}

	RICON_TemporalExpected CloneValidated()
	{
		RICON_TemporalExpected c = new RICON_TemporalExpected();
		c.m_iSessionEpoch = m_iSessionEpoch; c.m_iLinkId = m_iLinkId;
		c.m_iTalkerPlayerId = m_iTalkerPlayerId; c.m_iListenerPlayerId = m_iListenerPlayerId;
		c.m_sMode = m_sMode; c.m_sTimebaseId = m_sTimebaseId;
		c.m_Binding = m_Binding.CloneBinding(); c.m_Channel = m_Channel.CloneContext();
		return c;
	}
}

class RICON_TemporalLimits
{
	int m_iMaxSnapshots = 65536;
	int m_iMaxDescriptorBytes = 5111;
	int m_iMaxTotalDescriptorBytes = int.MAX;
	bool IsValid()
	{
		return m_iMaxSnapshots >= 1 && m_iMaxSnapshots <= 65536 && m_iMaxDescriptorBytes >= 1
			&& m_iMaxDescriptorBytes <= 65535 && m_iMaxTotalDescriptorBytes >= 1;
	}
	RICON_TemporalLimits CloneLimits()
	{
		RICON_TemporalLimits c = new RICON_TemporalLimits();
		c.m_iMaxSnapshots = m_iMaxSnapshots; c.m_iMaxDescriptorBytes = m_iMaxDescriptorBytes;
		c.m_iMaxTotalDescriptorBytes = m_iMaxTotalDescriptorBytes;
		return c;
	}
}

//! Deliberately not derived from legacy RICON_LinkSnapshot: no implicit v1 admission.
class RICON_TemporalSnapshot
{
	int m_iSchemaVersion, m_iSessionEpoch, m_iLinkId, m_iLinkSeq, m_iTtlMs, m_iRouteId, m_iRouteNodeCount;
	int m_iTalkerPlayerId, m_iListenerPlayerId, m_iDescriptorVersion;
	RICON_ELinkState m_eState;
	float m_fQuality;
	string m_sMode, m_sTimebaseId;
	ref RICON_TemporalBinding m_Binding;
	ref RICON_FadingDescriptor m_Descriptor;

	//! Same validated-only contract as existing DescriptorCodec.Clone; public admission is Validate.
	RICON_TemporalSnapshot CloneValidated()
	{
		RICON_TemporalSnapshot c = new RICON_TemporalSnapshot();
		c.m_iSchemaVersion = m_iSchemaVersion; c.m_iSessionEpoch = m_iSessionEpoch;
		c.m_iLinkId = m_iLinkId; c.m_iLinkSeq = m_iLinkSeq; c.m_iTtlMs = m_iTtlMs;
		c.m_iRouteId = m_iRouteId; c.m_iRouteNodeCount = m_iRouteNodeCount;
		c.m_iTalkerPlayerId = m_iTalkerPlayerId; c.m_iListenerPlayerId = m_iListenerPlayerId;
		c.m_iDescriptorVersion = m_iDescriptorVersion; c.m_eState = m_eState; c.m_fQuality = m_fQuality;
		c.m_sMode = m_sMode; c.m_sTimebaseId = m_sTimebaseId;
		c.m_Binding = m_Binding.CloneBinding(); c.m_Descriptor = RICON_DescriptorCodec.Clone(m_Descriptor);
		return c;
	}
}

class RICON_TemporalValidationResult
{
	string m_sCode = "INVALID_INPUT";
	ref RICON_TemporalSnapshot m_Snapshot;
	int m_iDescriptorBytes;
}

class RICON_TemporalApplyResult
{
	RICON_ESnapshotDecision m_eDecision = RICON_ESnapshotDecision.INVALID_SNAPSHOT;
	string m_sReason;
	bool m_bRequestResync, m_bDiagnostic;
	void RICON_TemporalApplyResult(RICON_ESnapshotDecision decision, string reason, bool resync = false, bool diagnostic = false)
	{
		m_eDecision = decision; m_sReason = reason; m_bRequestResync = resync; m_bDiagnostic = diagnostic;
	}
}

class RICON_TemporalMapping
{
	string m_sMappingId, m_sTimebaseId;
	int m_iLocalAnchorMs = -1;
	int m_iServerAnchorMs = -1;
	int m_iValidFromLocalMs = -1;
	int m_iValidUntilLocalMs = -1;
	int m_iMaxErrorUs = -1;
}

class RICON_TemporalViewResult
{
	string m_sCode = "PRESENTATION_EXPIRED";
	bool m_bVisible;
	ref RICON_FadingResult m_Fading;
}

class RICON_TemporalSnapshots
{
	static bool ValidKey(string key)
	{
		if (key.Length() < 1 || key.Length() > 64) return false;
		for (int i = 0; i < key.Length(); i++)
			if (RICON_DescriptorCodec.KeySymbol(key.ToAscii(i)) == "") return false;
		return true;
	}
	static bool ValidEndpoint(RICON_FadingEndpoint e) { return e && e.m_iDeviceId > 0 && e.m_iTransceiverIndex >= 0; }
	static bool SameEndpoint(RICON_FadingEndpoint a, RICON_FadingEndpoint b)
	{
		return a && b && a.m_iDeviceId == b.m_iDeviceId && a.m_iTransceiverIndex == b.m_iTransceiverIndex;
	}
	static RICON_FadingEndpoint CloneEndpoint(RICON_FadingEndpoint e)
	{
		if (!e) return null;
		RICON_FadingEndpoint c = new RICON_FadingEndpoint();
		c.m_iDeviceId = e.m_iDeviceId; c.m_iTransceiverIndex = e.m_iTransceiverIndex;
		return c;
	}

	static bool ContextMatches(RICON_TemporalSnapshot s, RICON_TemporalExpected e)
	{
		if (!s || !s.m_Binding || !s.m_Descriptor) return false;
		if (!e || !e.IsValid() || s.m_iSessionEpoch != e.m_iSessionEpoch || s.m_iLinkId != e.m_iLinkId
			|| s.m_iTalkerPlayerId != e.m_iTalkerPlayerId || s.m_iListenerPlayerId != e.m_iListenerPlayerId
			|| s.m_sMode != e.m_sMode || s.m_sTimebaseId != e.m_sTimebaseId || !s.m_Binding.Same(e.m_Binding))
			return false;
		RICON_TemporalChannelContext q = e.m_Channel;
		RICON_TemporalBinding b = e.m_Binding;
		if (q.m_Tx.m_iDeviceId != b.m_iTxDeviceId || q.m_Tx.m_iTransceiverIndex != b.m_iTxTransceiverIndex
			|| q.m_Rx.m_iDeviceId != b.m_iRxDeviceId || q.m_Rx.m_iTransceiverIndex != b.m_iRxTransceiverIndex)
			return false;
		RICON_FadingDescriptor d = s.m_Descriptor;
		bool pair = (SameEndpoint(d.m_A, q.m_Tx) && SameEndpoint(d.m_B, q.m_Rx))
			|| (SameEndpoint(d.m_A, q.m_Rx) && SameEndpoint(d.m_B, q.m_Tx));
		return pair && d.m_fFrequencyMHz == q.m_fFrequencyMHz && d.m_fBandwidthHz == q.m_fBandwidthHz
			&& d.m_sProfileId == q.m_sProfileId && d.m_iProfileVersion == q.m_iProfileVersion
			&& d.m_iEnvironmentRevision == q.m_iEnvironmentRevision;
	}

	static RICON_TemporalValidationResult Normalize(RICON_TemporalSnapshot s, RICON_TemporalLimits limits)
	{
		RICON_TemporalValidationResult r = new RICON_TemporalValidationResult();
		if (!limits || !limits.IsValid()) { r.m_sCode = "INVALID_LIMITS"; return r; }
		if (!s) { r.m_sCode = "INVALID_SNAPSHOT"; return r; }
		if (s.m_iSchemaVersion <= 0 || s.m_iDescriptorVersion <= 0 || s.m_iSessionEpoch <= 0 || s.m_iSessionEpoch >= int.MAX)
		{ r.m_sCode = "INVALID_SNAPSHOT"; return r; }
		if (s.m_iLinkId <= 0 || s.m_iLinkSeq <= 0 || s.m_iLinkSeq >= int.MAX || s.m_iTtlMs <= 0)
		{ r.m_sCode = "INVALID_SNAPSHOT"; return r; }
		if (s.m_iRouteId != s.m_iLinkId || s.m_iRouteNodeCount != 2 || s.m_iTalkerPlayerId <= 0)
		{ r.m_sCode = "INVALID_SNAPSHOT"; return r; }
		if (s.m_iListenerPlayerId <= 0 || s.m_iTalkerPlayerId == s.m_iListenerPlayerId)
		{ r.m_sCode = "INVALID_SNAPSHOT"; return r; }
		if (!s.m_Binding || !s.m_Binding.IsValid() || !ValidKey(s.m_sTimebaseId))
		{ r.m_sCode = "INVALID_SNAPSHOT"; return r; }
		if (!RICON_Num.IsFinite(s.m_fQuality) || s.m_fQuality < 0 || s.m_fQuality > 1)
		{ r.m_sCode = "INVALID_SNAPSHOT"; return r; }
		if (s.m_eState != RICON_ELinkState.LOST && s.m_eState != RICON_ELinkState.WEAK
			&& s.m_eState != RICON_ELinkState.DEGRADED && s.m_eState != RICON_ELinkState.CLEAN)
		{ r.m_sCode = "INVALID_SNAPSHOT"; return r; }
		if (s.m_eState == RICON_ELinkState.LOST && s.m_fQuality != 0)
		{ r.m_sCode = "INVALID_SNAPSHOT"; return r; }
		if (!s.m_Descriptor) { r.m_sCode = "INVALID_DESCRIPTOR"; return r; }
		if (!RICON_FiniteMultipath.ValidDescriptor(s.m_Descriptor))
		{ r.m_sCode = "INVALID_DESCRIPTOR"; return r; }
		if (s.m_iSchemaVersion != 2) { r.m_sCode = "UNSUPPORTED_SCHEMA"; return r; }
		if (s.m_iDescriptorVersion != 1) { r.m_sCode = "UNSUPPORTED_DESCRIPTOR_VERSION"; return r; }
		if (s.m_Descriptor.m_sModelId != RICON_FiniteMultipath.MODEL_ID)
		{ r.m_sCode = "UNSUPPORTED_MODEL"; return r; }
		if (s.m_sMode != "SERVER" && s.m_sMode != "RECEIVER")
		{ r.m_sCode = "UNSUPPORTED_MODE"; return r; }
		RICON_DescriptorResult encoded = RICON_DescriptorCodec.Encode(s.m_Descriptor, limits.m_iMaxDescriptorBytes);
		if (encoded.m_eCode != RICON_EDescriptorCode.OK || encoded.m_iSizeBytes > limits.m_iMaxTotalDescriptorBytes)
		{ r.m_sCode = "RESOURCE_LIMIT"; return r; }
		r.m_Snapshot = s.CloneValidated(); r.m_Snapshot.m_Descriptor = encoded.m_Descriptor;
		r.m_iDescriptorBytes = encoded.m_iSizeBytes; r.m_sCode = "OK";
		return r;
	}

	static RICON_TemporalValidationResult Validate(RICON_TemporalSnapshot s, RICON_TemporalExpected expected, RICON_TemporalLimits limits)
	{
		RICON_TemporalValidationResult r = Normalize(s, limits);
		if (r.m_sCode == "OK" && !ContextMatches(r.m_Snapshot, expected))
		{
			r.m_sCode = "CONTEXT_MISMATCH"; r.m_Snapshot = null; r.m_iDescriptorBytes = 0;
		}
		return r;
	}
}

class RICON_TemporalSnapshotReceiver
{
	protected ref RICON_TemporalLimits m_Limits;
	protected ref map<int, ref RICON_TemporalExpected> m_Expected = new map<int, ref RICON_TemporalExpected>();
	protected ref map<int, ref RICON_TemporalSnapshot> m_Slots = new map<int, ref RICON_TemporalSnapshot>();
	protected ref map<int, int> m_ReceivedAt = new map<int, int>();
	protected string m_sConfigReason = "INVALID_INPUT";
	protected int m_iSessionEpoch, m_iTotalBytes;
	protected bool m_bHasIdentity;

	void RICON_TemporalSnapshotReceiver(map<int, ref RICON_TemporalExpected> expected, RICON_TemporalLimits limits)
	{
		if (!limits || !limits.IsValid()) { m_sConfigReason = "INVALID_LIMITS"; return; }
		m_Limits = limits.CloneLimits();
		if (!expected) return;
		for (int i = 0; i < expected.Count(); i++)
		{
			int linkId = expected.GetKey(i);
			RICON_TemporalExpected e = expected.Get(linkId);
			if (linkId <= 0 || !e || !e.IsValid() || e.m_iLinkId != linkId) return;
			m_Expected.Set(linkId, e.CloneValidated());
		}
		m_sConfigReason = "OK";
	}

	protected void ResetIdentity()
	{
		m_iSessionEpoch = 0; m_bHasIdentity = false; m_iTotalBytes = 0;
		m_Slots.Clear(); m_ReceivedAt.Clear();
	}

	protected string Preflight(array<ref RICON_TemporalSnapshot> snapshots, out array<ref RICON_TemporalSnapshot> copies, out int total)
	{
		copies = new array<ref RICON_TemporalSnapshot>(); total = 0;
		if (m_sConfigReason != "OK") return m_sConfigReason;
		if (!snapshots) return "INVALID_INPUT";
		if (snapshots.Count() > m_Limits.m_iMaxSnapshots) return "BATCH_LIMIT";
		set<int> seen = new set<int>();
		foreach (RICON_TemporalSnapshot s : snapshots)
		{
			RICON_TemporalExpected expected;
			if (s) expected = m_Expected.Get(s.m_iLinkId);
			RICON_TemporalValidationResult r = RICON_TemporalSnapshots.Validate(s, expected, m_Limits);
			if (r.m_sCode != "OK") return r.m_sCode;
			if (seen.Contains(s.m_iLinkId)) return "DUPLICATE_LINK";
			seen.Insert(s.m_iLinkId);
			if (r.m_iDescriptorBytes > m_Limits.m_iMaxTotalDescriptorBytes - total) return "RESOURCE_LIMIT";
			total += r.m_iDescriptorBytes;
			copies.Insert(r.m_Snapshot);
		}
		return "OK";
	}

	RICON_TemporalApplyResult ApplyFullResync(int nowMs, int epoch, int schema, array<ref RICON_TemporalSnapshot> snapshots)
	{
		array<ref RICON_TemporalSnapshot> copies;
		int total;
		string reason = Preflight(snapshots, copies, total);
		if (reason == "OK" && schema > 0 && schema != 2) reason = "UNSUPPORTED_SCHEMA";
		if (reason == "OK" && (nowMs < 0 || epoch <= 0 || epoch >= int.MAX || schema <= 0)) reason = "INVALID_INPUT";
		if (reason == "OK")
			foreach (RICON_TemporalSnapshot row : copies)
				if (row.m_iSessionEpoch != epoch || row.m_iSchemaVersion != schema) { reason = "INVALID_IDENTITY"; break; }
		if (reason != "OK")
		{
			ResetIdentity();
			return new RICON_TemporalApplyResult(RICON_ESnapshotDecision.INVALID_RESYNC, reason, true, true);
		}
		map<int, ref RICON_TemporalSnapshot> next = new map<int, ref RICON_TemporalSnapshot>();
		map<int, int> times = new map<int, int>();
		foreach (RICON_TemporalSnapshot fresh : copies) { next.Set(fresh.m_iLinkId, fresh); times.Set(fresh.m_iLinkId, nowMs); }
		m_Slots = next; m_ReceivedAt = times; m_iTotalBytes = total; m_iSessionEpoch = epoch; m_bHasIdentity = true;
		return new RICON_TemporalApplyResult(RICON_ESnapshotDecision.ACCEPT_RESYNC, "OK");
	}

	RICON_TemporalApplyResult Receive(int nowMs, RICON_TemporalSnapshot snapshot)
	{
		if (m_sConfigReason != "OK") return new RICON_TemporalApplyResult(RICON_ESnapshotDecision.INVALID_SNAPSHOT, m_sConfigReason, false, true);
		RICON_TemporalValidationResult r = RICON_TemporalSnapshots.Normalize(snapshot, m_Limits);
		if (r.m_sCode != "OK" || nowMs < 0)
		{
			string reason = r.m_sCode;
			if (reason == "OK") reason = "INVALID_INPUT";
			bool unsupported = reason == "UNSUPPORTED_SCHEMA" || reason == "UNSUPPORTED_DESCRIPTOR_VERSION"
				|| reason == "UNSUPPORTED_MODEL" || reason == "UNSUPPORTED_MODE";
			return new RICON_TemporalApplyResult(RICON_ESnapshotDecision.INVALID_SNAPSHOT, reason, unsupported, !unsupported);
		}
		RICON_TemporalSnapshot s = r.m_Snapshot;
		if (!m_bHasIdentity || s.m_iSessionEpoch != m_iSessionEpoch)
		{
			if (m_bHasIdentity) ResetIdentity();
			return new RICON_TemporalApplyResult(RICON_ESnapshotDecision.INVALID_IDENTITY, "INVALID_IDENTITY", true);
		}
		if (!RICON_TemporalSnapshots.ContextMatches(s, m_Expected.Get(s.m_iLinkId)))
			return new RICON_TemporalApplyResult(RICON_ESnapshotDecision.INVALID_SNAPSHOT, "CONTEXT_MISMATCH", true);
		RICON_TemporalSnapshot old = m_Slots.Get(s.m_iLinkId);
		if (old && s.m_iLinkSeq <= old.m_iLinkSeq)
			return new RICON_TemporalApplyResult(RICON_ESnapshotDecision.STALE, "OK");
		int retained = m_iTotalBytes;
		if (old) retained -= RICON_DescriptorCodec.FrameSize(old.m_Descriptor);
		if (!old && m_Slots.Count() >= m_Limits.m_iMaxSnapshots)
			return new RICON_TemporalApplyResult(RICON_ESnapshotDecision.INVALID_SNAPSHOT, "BATCH_LIMIT", true);
		if (r.m_iDescriptorBytes > m_Limits.m_iMaxTotalDescriptorBytes - retained)
			return new RICON_TemporalApplyResult(RICON_ESnapshotDecision.INVALID_SNAPSHOT, "RESOURCE_LIMIT", true);
		m_Slots.Set(s.m_iLinkId, s); m_ReceivedAt.Set(s.m_iLinkId, nowMs); m_iTotalBytes = retained + r.m_iDescriptorBytes;
		RICON_ESnapshotDecision decision = RICON_ESnapshotDecision.ACCEPT_FIRST;
		if (old) decision = RICON_ESnapshotDecision.ACCEPT_NEWER;
		return new RICON_TemporalApplyResult(decision, "OK");
	}

	RICON_TemporalSnapshot Read(int linkId)
	{
		if (linkId <= 0) return null;
		RICON_TemporalSnapshot s = m_Slots.Get(linkId);
		if (!s) return null;
		return s.CloneValidated();
	}
	bool HasIdentity() { return m_bHasIdentity; }
	int SlotCount() { return m_Slots.Count(); }
	int TotalDescriptorBytes() { return m_iTotalBytes; }

	bool IsVisible(int nowMs, int linkId, out bool diagnostic)
	{
		diagnostic = false;
		if (nowMs < 0 || linkId <= 0) { diagnostic = true; return false; }
		RICON_TemporalSnapshot s = m_Slots.Get(linkId);
		if (!s || !m_ReceivedAt.Contains(linkId)) return false;
		int elapsed = nowMs - m_ReceivedAt.Get(linkId);
		if (elapsed < 0) { diagnostic = true; return false; }
		return elapsed < s.m_iTtlMs;
	}

	RICON_TemporalViewResult View(int nowMs, int linkId, RICON_TemporalMapping mapping, int subUs = 0)
	{
		RICON_TemporalViewResult v = new RICON_TemporalViewResult();
		bool diagnostic;
		v.m_bVisible = IsVisible(nowMs, linkId, diagnostic);
		if (!v.m_bVisible) return v;
		RICON_TemporalSnapshot s = m_Slots.Get(linkId);
		v.m_sCode = "CLOCK_UNVERIFIED";
		if (!mapping || !RICON_TemporalSnapshots.ValidKey(mapping.m_sMappingId)
			|| !RICON_TemporalSnapshots.ValidKey(mapping.m_sTimebaseId) || mapping.m_sTimebaseId != s.m_sTimebaseId) return v;
		int estimateMs, estimateSubUs, lowerMs, lowerSubUs, upperMs, upperSubUs;
		int clock = RICON_TemporalClock.MapInterval(nowMs, subUs, mapping.m_iLocalAnchorMs, mapping.m_iServerAnchorMs,
			mapping.m_iValidFromLocalMs, mapping.m_iValidUntilLocalMs, mapping.m_iMaxErrorUs,
			estimateMs, estimateSubUs, lowerMs, lowerSubUs, upperMs, upperSubUs);
		if (clock == 1) return v;
		if (clock == 2) { v.m_sCode = "CLOCK_OUT_OF_RANGE"; return v; }
		if (!RICON_TemporalClock.ContainsInterval(lowerMs, lowerSubUs, upperMs, upperSubUs,
			s.m_Descriptor.m_iValidFromMs, s.m_Descriptor.m_iValidUntilMs))
		{ v.m_sCode = "STALE_DESCRIPTOR"; return v; }
		RICON_TemporalExpected e = m_Expected.Get(linkId);
		v.m_Fading = RICON_FiniteMultipath.Evaluate(s.m_Descriptor, e.m_Channel.Query(estimateMs, estimateSubUs));
		v.m_sCode = typename.EnumToString(RICON_EFadingCode, v.m_Fading.m_eCode);
		return v;
	}
}
