//! STAGING typed port of accepted ordered HOST d2c0d648. Compile/runtime NOT_RUN; no actor/RPC/actuator.
class RICON_TemporalEndRecord
{
	int m_iSessionEpoch, m_iLinkId, m_iLinkSeq, m_iTalkerPlayerId, m_iListenerPlayerId;
	ref RICON_TemporalBinding m_Binding;
	string m_sReason;

	static string LeaseReasonName(RICON_ELeaseTerminalReason reason)
	{
		switch (reason)
		{
			case RICON_ELeaseTerminalReason.TTL_EXPIRED: return "TTL_EXPIRED";
			case RICON_ELeaseTerminalReason.CLIENT_STOP: return "CLIENT_STOP";
			case RICON_ELeaseTerminalReason.DISCONNECT: return "DISCONNECT";
			case RICON_ELeaseTerminalReason.RECONNECT: return "RECONNECT";
			case RICON_ELeaseTerminalReason.ENTITY_CHANGED: return "ENTITY_CHANGED";
			case RICON_ELeaseTerminalReason.ENTITY_INACTIVE: return "ENTITY_INACTIVE";
			case RICON_ELeaseTerminalReason.ACCESS_LOST: return "ACCESS_LOST";
			case RICON_ELeaseTerminalReason.POWER_LOST: return "POWER_LOST";
			case RICON_ELeaseTerminalReason.DEVICE_REMOVED: return "DEVICE_REMOVED";
			case RICON_ELeaseTerminalReason.CATALOG_INVALIDATED: return "CATALOG_INVALIDATED";
			case RICON_ELeaseTerminalReason.SESSION_ROTATED: return "SESSION_ROTATED";
			case RICON_ELeaseTerminalReason.ADMIN_REVOKED: return "ADMIN_REVOKED";
			case RICON_ELeaseTerminalReason.TICK_ANOMALY: return "TICK_ANOMALY";
		}
		return "";
	}
	static bool ValidReason(string reason)
	{
		return reason == "TTL_EXPIRED" || reason == "CLIENT_STOP" || reason == "DISCONNECT" || reason == "RECONNECT"
			|| reason == "ENTITY_CHANGED" || reason == "ENTITY_INACTIVE" || reason == "ACCESS_LOST"
			|| reason == "POWER_LOST" || reason == "DEVICE_REMOVED" || reason == "CATALOG_INVALIDATED"
			|| reason == "SESSION_ROTATED" || reason == "ADMIN_REVOKED" || reason == "TICK_ANOMALY" || reason == "LINK_REMOVED";
	}
	bool IsValid()
	{
		return m_iSessionEpoch > 0 && m_iSessionEpoch < int.MAX && m_iLinkId > 0 && m_iLinkSeq > 0 && m_iLinkSeq < int.MAX
			&& m_iTalkerPlayerId > 0 && m_iListenerPlayerId > 0 && m_iTalkerPlayerId != m_iListenerPlayerId
			&& m_Binding && m_Binding.IsValid() && ValidReason(m_sReason);
	}
	bool Same(RICON_TemporalEndRecord other)
	{
		return other && m_iSessionEpoch == other.m_iSessionEpoch && m_iLinkId == other.m_iLinkId
			&& m_iLinkSeq == other.m_iLinkSeq && m_iTalkerPlayerId == other.m_iTalkerPlayerId
			&& m_iListenerPlayerId == other.m_iListenerPlayerId && m_Binding && m_Binding.Same(other.m_Binding)
			&& m_sReason == other.m_sReason;
	}
	RICON_TemporalEndRecord CloneValidated()
	{
		RICON_TemporalEndRecord c = new RICON_TemporalEndRecord();
		c.m_iSessionEpoch = m_iSessionEpoch; c.m_iLinkId = m_iLinkId; c.m_iLinkSeq = m_iLinkSeq;
		c.m_iTalkerPlayerId = m_iTalkerPlayerId; c.m_iListenerPlayerId = m_iListenerPlayerId;
		c.m_Binding = m_Binding.CloneBinding(); c.m_sReason = m_sReason; return c;
	}
	//! End closes old addressing even when its physical descriptor has expired; seq is assigned externally.
	static RICON_TemporalEndRecord FromSnapshot(RICON_TemporalSnapshot snapshot, int endSeq, string reason)
	{
		if (!snapshot || !snapshot.m_Binding) return null;
		RICON_TemporalEndRecord e = new RICON_TemporalEndRecord();
		e.m_iSessionEpoch = snapshot.m_iSessionEpoch; e.m_iLinkId = snapshot.m_iLinkId; e.m_iLinkSeq = endSeq;
		e.m_iTalkerPlayerId = snapshot.m_iTalkerPlayerId; e.m_iListenerPlayerId = snapshot.m_iListenerPlayerId;
		e.m_Binding = snapshot.m_Binding.CloneBinding(); e.m_sReason = reason;
		if (!e.IsValid()) return null;
		return e;
	}
}

class RICON_TemporalProtocolItem
{
	string m_sKind;
	ref RICON_TemporalSnapshot m_Snapshot;
	ref RICON_TemporalEndRecord m_End;
}
class RICON_TemporalProtocolEnvelope
{
	int m_iProtocolVersion = 1;
	string m_sKind;
	int m_iOwnerPlayerId, m_iSessionEpoch, m_iDeliverySeq, m_iLinkIdHighWater;
	ref array<ref RICON_TemporalProtocolItem> m_aItems = {};
	bool ValidHeader()
	{
		return m_iProtocolVersion == 1 && (m_sKind == "FULL" || m_sKind == "DELTA") && m_iOwnerPlayerId > 0
			&& m_iSessionEpoch > 0 && m_iSessionEpoch < int.MAX && m_iDeliverySeq > 0 && m_iDeliverySeq < int.MAX
			&& m_iLinkIdHighWater >= 0;
	}
}
class RICON_TemporalAuthority
{
	int m_iOwnerPlayerId, m_iSessionEpoch, m_iDeliverySeq, m_iLinkIdHighWater;
	string m_sKind;
	ref map<int, ref RICON_TemporalExpected> m_ActiveExpected = new map<int, ref RICON_TemporalExpected>();
	ref map<int, ref RICON_TemporalEndRecord> m_EndedTargets = new map<int, ref RICON_TemporalEndRecord>();
}
class RICON_TemporalProtocolApplyResult
{
	string m_sCode;
	bool m_bNeedsFull;
	int m_iDeliverySeq;
}
class RICON_TemporalProtocolRow
{
	ref RICON_TemporalSnapshot m_Snapshot;
	int m_iReceivedAtMs;
	ref RICON_TemporalExpected m_Expected;
	RICON_TemporalProtocolRow CloneOwned()
	{
		RICON_TemporalProtocolRow c = new RICON_TemporalProtocolRow();
		c.m_Snapshot = m_Snapshot.CloneValidated(); c.m_iReceivedAtMs = m_iReceivedAtMs;
		c.m_Expected = m_Expected.CloneValidated(); return c;
	}
}
class RICON_TemporalProtocolState
{
	int m_iOwnerPlayerId, m_iSessionEpoch, m_iDeliverySeq, m_iCheckpointHighWater, m_iSeenHighWater;
	int m_iObservedEpoch, m_iObservedDeliverySeq, m_iObservedHighWater;
	bool m_bNeedsFull = true;
	ref map<int, ref RICON_TemporalProtocolRow> m_Active = new map<int, ref RICON_TemporalProtocolRow>();
	ref map<int, ref RICON_TemporalEndRecord> m_Closed = new map<int, ref RICON_TemporalEndRecord>();
	RICON_TemporalProtocolState CloneOwned()
	{
		RICON_TemporalProtocolState c = new RICON_TemporalProtocolState();
		c.m_iOwnerPlayerId = m_iOwnerPlayerId; c.m_iSessionEpoch = m_iSessionEpoch; c.m_iDeliverySeq = m_iDeliverySeq;
		c.m_iCheckpointHighWater = m_iCheckpointHighWater; c.m_iSeenHighWater = m_iSeenHighWater;
		c.m_iObservedEpoch = m_iObservedEpoch; c.m_iObservedDeliverySeq = m_iObservedDeliverySeq;
		c.m_iObservedHighWater = m_iObservedHighWater; c.m_bNeedsFull = m_bNeedsFull;
		for (int i = 0; i < m_Active.Count(); i++) { int link = m_Active.GetKey(i); c.m_Active.Set(link, m_Active.Get(link).CloneOwned()); }
		for (int j = 0; j < m_Closed.Count(); j++) { int closed = m_Closed.GetKey(j); c.m_Closed.Set(closed, m_Closed.Get(closed).CloneValidated()); }
		return c;
	}
}

class RICON_TemporalProtocolRules
{
	static bool ExpectedIntrinsic(RICON_TemporalExpected expected)
	{
		if (!expected || !expected.IsValid()) return false;
		RICON_TemporalChannelContext c = expected.m_Channel;
		RICON_TemporalBinding b = expected.m_Binding;
		return c.m_Tx.m_iDeviceId == b.m_iTxDeviceId && c.m_Tx.m_iTransceiverIndex == b.m_iTxTransceiverIndex
			&& c.m_Rx.m_iDeviceId == b.m_iRxDeviceId && c.m_Rx.m_iTransceiverIndex == b.m_iRxTransceiverIndex;
	}
	static bool AuthorityMapsValid(RICON_TemporalAuthority authority)
	{
		if (!authority || !authority.m_ActiveExpected || !authority.m_EndedTargets
			|| authority.m_ActiveExpected.Count() > 65536 || authority.m_EndedTargets.Count() > 65536) return false;
		for (int i = 0; i < authority.m_ActiveExpected.Count(); i++)
		{
			int link = authority.m_ActiveExpected.GetKey(i);
			RICON_TemporalExpected e = authority.m_ActiveExpected.Get(link);
			if (link <= 0 || !ExpectedIntrinsic(e) || e.m_iLinkId != link || e.m_iSessionEpoch != authority.m_iSessionEpoch
				|| e.m_iListenerPlayerId != authority.m_iOwnerPlayerId) return false;
		}
		for (int j = 0; j < authority.m_EndedTargets.Count(); j++)
		{
			int closed = authority.m_EndedTargets.GetKey(j);
			RICON_TemporalEndRecord end = authority.m_EndedTargets.Get(closed);
			if (closed <= 0 || !end || !end.IsValid() || end.m_iLinkId != closed || end.m_iSessionEpoch != authority.m_iSessionEpoch
				|| end.m_iListenerPlayerId != authority.m_iOwnerPlayerId) return false;
		}
		return true;
	}

	static bool SnapshotBindingSame(RICON_TemporalSnapshot a, RICON_TemporalSnapshot b)
	{
		return a && b && a.m_iSessionEpoch == b.m_iSessionEpoch && a.m_iLinkId == b.m_iLinkId
			&& a.m_iTalkerPlayerId == b.m_iTalkerPlayerId && a.m_iListenerPlayerId == b.m_iListenerPlayerId
			&& a.m_Binding && a.m_Binding.Same(b.m_Binding);
	}
	static bool SnapshotEndBindingSame(RICON_TemporalSnapshot a, RICON_TemporalEndRecord b)
	{
		return a && b && a.m_iSessionEpoch == b.m_iSessionEpoch && a.m_iLinkId == b.m_iLinkId
			&& a.m_iTalkerPlayerId == b.m_iTalkerPlayerId && a.m_iListenerPlayerId == b.m_iListenerPlayerId
			&& a.m_Binding && a.m_Binding.Same(b.m_Binding);
	}
	static bool EndBindingSame(RICON_TemporalEndRecord a, RICON_TemporalEndRecord b)
	{
		return a && b && a.m_iSessionEpoch == b.m_iSessionEpoch && a.m_iLinkId == b.m_iLinkId
			&& a.m_iTalkerPlayerId == b.m_iTalkerPlayerId && a.m_iListenerPlayerId == b.m_iListenerPlayerId
			&& a.m_Binding && a.m_Binding.Same(b.m_Binding);
	}
	//! Validated snapshots only. Canonical RCFD equality preserves exact f32/phase semantics, including +0 normalization.
	//! B's live sound-carrier generation is not a server-snapshot field and is not compared here.
	static bool SameSnapshot(RICON_TemporalSnapshot a, RICON_TemporalSnapshot b)
	{
		if (!SnapshotBindingSame(a, b)) return false;
		if (a.m_iSchemaVersion != b.m_iSchemaVersion || a.m_iLinkSeq != b.m_iLinkSeq || a.m_iTtlMs != b.m_iTtlMs) return false;
		if (a.m_iRouteId != b.m_iRouteId || a.m_iRouteNodeCount != b.m_iRouteNodeCount || a.m_eState != b.m_eState) return false;
		if (a.m_fQuality != b.m_fQuality || a.m_sMode != b.m_sMode) return false;
		if (a.m_sTimebaseId != b.m_sTimebaseId || a.m_iDescriptorVersion != b.m_iDescriptorVersion) return false;
		RICON_DescriptorResult left = RICON_DescriptorCodec.Encode(a.m_Descriptor, 65535);
		RICON_DescriptorResult right = RICON_DescriptorCodec.Encode(b.m_Descriptor, 65535);
		if (left.m_eCode != RICON_EDescriptorCode.OK || right.m_eCode != RICON_EDescriptorCode.OK
			|| left.m_iSizeBytes != right.m_iSizeBytes) return false;
		for (int i = 0; i < left.m_aBytes.Count(); i++) if (left.m_aBytes[i] != right.m_aBytes[i]) return false;
		return true;
	}
}

class RICON_TemporalOrderedReceiver
{
	protected ref RICON_TemporalLimits m_Limits;
	protected ref RICON_TemporalProtocolState m_State = new RICON_TemporalProtocolState();
	protected string m_sConfiguration = "INVALID_INPUT";

	void RICON_TemporalOrderedReceiver(int ownerPlayerId, RICON_TemporalLimits limits)
	{
		if (ownerPlayerId > 0) m_State.m_iOwnerPlayerId = ownerPlayerId;
		if (!limits || !limits.IsValid()) { m_sConfiguration = "INVALID_LIMITS"; return; }
		m_Limits = limits.CloneLimits();
		if (ownerPlayerId > 0) m_sConfiguration = "OK";
	}
	RICON_TemporalProtocolState State() { return m_State.CloneOwned(); }
	RICON_TemporalSnapshot Read(int linkId)
	{
		if (linkId <= 0) return null;
		RICON_TemporalProtocolRow row = m_State.m_Active.Get(linkId);
		if (!row) return null;
		return row.m_Snapshot.CloneValidated();
	}
	bool IsVisible(int nowMs, int linkId)
	{
		if (m_State.m_bNeedsFull || nowMs < 0 || linkId <= 0) return false;
		RICON_TemporalProtocolRow row = m_State.m_Active.Get(linkId);
		if (!row) return false;
		int elapsed = nowMs - row.m_iReceivedAtMs;
		return elapsed >= 0 && elapsed < row.m_Snapshot.m_iTtlMs;
	}
	protected RICON_TemporalProtocolApplyResult Result(string code)
	{
		RICON_TemporalProtocolApplyResult r = new RICON_TemporalProtocolApplyResult();
		r.m_sCode = code; r.m_bNeedsFull = m_State.m_bNeedsFull; r.m_iDeliverySeq = m_State.m_iDeliverySeq; return r;
	}
	protected RICON_TemporalProtocolApplyResult Reject(string code, bool desync = true)
	{
		if (desync && code != "INVALID_AUTHORITY") m_State.m_bNeedsFull = true;
		return Result(code);
	}
	protected void Observe(RICON_TemporalProtocolEnvelope frame)
	{
		if (m_State.m_iObservedEpoch != frame.m_iSessionEpoch)
		{
			m_State.m_iObservedEpoch = frame.m_iSessionEpoch;
			m_State.m_iObservedDeliverySeq = 0; m_State.m_iObservedHighWater = 0;
		}
		if (frame.m_iDeliverySeq > m_State.m_iObservedDeliverySeq) m_State.m_iObservedDeliverySeq = frame.m_iDeliverySeq;
		if (frame.m_iLinkIdHighWater > m_State.m_iObservedHighWater) m_State.m_iObservedHighWater = frame.m_iLinkIdHighWater;
		if (m_State.m_iSessionEpoch == frame.m_iSessionEpoch && frame.m_iLinkIdHighWater > m_State.m_iSeenHighWater)
			m_State.m_iSeenHighWater = frame.m_iLinkIdHighWater;
	}

	//! Only typed union shape and all END grants here; ACTIVE descriptor domains are checked after Observe.
	protected string AuthorizeItems(RICON_TemporalProtocolEnvelope frame, RICON_TemporalAuthority authority)
	{
		set<int> seen = new set<int>();
		set<int> ended = new set<int>();
		foreach (RICON_TemporalProtocolItem item : frame.m_aItems)
		{
			if (!item) return "INVALID_INPUT";
			int link;
			if (item.m_sKind == "ACTIVE" && !item.m_End)
			{
				if (!item.m_Snapshot || item.m_Snapshot.m_iLinkId <= 0) return "INVALID_SNAPSHOT";
				link = item.m_Snapshot.m_iLinkId;
			}
			else if (item.m_sKind == "END" && !item.m_Snapshot)
			{
				RICON_TemporalEndRecord end = item.m_End;
				if (frame.m_sKind == "FULL" || !end || !end.IsValid()) return "INVALID_END";
				link = end.m_iLinkId;
				RICON_TemporalEndRecord receipt = authority.m_EndedTargets.Get(link);
				if (!receipt || !receipt.IsValid() || !end.Same(receipt)) return "INVALID_AUTHORITY";
				ended.Insert(link);
			}
			else return "INVALID_INPUT";
			if (seen.Contains(link)) return "DUPLICATE_LINK";
			seen.Insert(link);
		}
		if (ended.Count() != authority.m_EndedTargets.Count()) return "INVALID_AUTHORITY";
		for (int i = 0; i < authority.m_EndedTargets.Count(); i++)
			if (!ended.Contains(authority.m_EndedTargets.GetKey(i))) return "INVALID_AUTHORITY";
		return "OK";
	}

	protected string Collect(RICON_TemporalProtocolEnvelope frame, RICON_TemporalAuthority authority,
		out array<ref RICON_TemporalProtocolItem> rows)
	{
		rows = new array<ref RICON_TemporalProtocolItem>();
		foreach (RICON_TemporalProtocolItem item : frame.m_aItems)
		{
			RICON_TemporalProtocolItem normalized = new RICON_TemporalProtocolItem();
			int epoch, listener, link;
			if (item.m_sKind == "ACTIVE")
			{
				RICON_TemporalValidationResult checked = RICON_TemporalSnapshots.Validate(item.m_Snapshot,
					authority.m_ActiveExpected.Get(item.m_Snapshot.m_iLinkId), m_Limits);
				if (checked.m_sCode != "OK") return checked.m_sCode;
				normalized.m_sKind = "ACTIVE"; normalized.m_Snapshot = checked.m_Snapshot;
				epoch = checked.m_Snapshot.m_iSessionEpoch; listener = checked.m_Snapshot.m_iListenerPlayerId;
				link = checked.m_Snapshot.m_iLinkId;
			}
			else
			{
				RICON_TemporalEndRecord end = item.m_End;
				RICON_TemporalEndRecord receipt = authority.m_EndedTargets.Get(end.m_iLinkId);
				if (!end.IsValid() || !receipt || !receipt.IsValid() || !end.Same(receipt)) return "INVALID_AUTHORITY";
				normalized.m_sKind = "END"; normalized.m_End = end.CloneValidated();
				epoch = end.m_iSessionEpoch; listener = end.m_iListenerPlayerId; link = end.m_iLinkId;
			}
			if (epoch != frame.m_iSessionEpoch || listener != m_State.m_iOwnerPlayerId) return "BINDING_MISMATCH";
			if (link > frame.m_iLinkIdHighWater) return "HIGH_WATER_ROLLBACK";
			rows.Insert(normalized);
		}
		return "OK";
	}

	protected string ActiveRow(RICON_TemporalSnapshot snapshot, RICON_TemporalProtocolRow old, int nowMs,
		RICON_TemporalExpected expected, out RICON_TemporalProtocolRow row)
	{
		row = null;
		if (old)
		{
			RICON_TemporalSnapshot prior = old.m_Snapshot;
			if (!RICON_TemporalProtocolRules.SnapshotBindingSame(prior, snapshot)) return "BINDING_MISMATCH";
			if (snapshot.m_iLinkSeq < prior.m_iLinkSeq) return "LINK_SEQUENCE_ROLLBACK";
			if (snapshot.m_iLinkSeq == prior.m_iLinkSeq)
			{
				if (!RICON_TemporalProtocolRules.SameSnapshot(snapshot, prior)) return "LINK_SEQUENCE_CONFLICT";
				row = old.CloneOwned(); return "OK";
			}
		}
		row = new RICON_TemporalProtocolRow();
		row.m_Snapshot = snapshot.CloneValidated(); row.m_iReceivedAtMs = nowMs; row.m_Expected = expected.CloneValidated();
		return "OK";
	}
	protected string Postflight(RICON_TemporalProtocolState candidate, RICON_TemporalAuthority authority)
	{
		if (candidate.m_Active.Count() != authority.m_ActiveExpected.Count()) return "COMPLETENESS_MISMATCH";
		for (int i = 0; i < authority.m_ActiveExpected.Count(); i++)
			if (!candidate.m_Active.Contains(authority.m_ActiveExpected.GetKey(i))) return "COMPLETENESS_MISMATCH";
		if (candidate.m_Active.Count() + candidate.m_Closed.Count() > m_Limits.m_iMaxSnapshots) return "RESOURCE_LIMIT";
		int total = 0;
		for (int j = 0; j < candidate.m_Active.Count(); j++)
		{
			int link = candidate.m_Active.GetKey(j);
			RICON_TemporalProtocolRow row = candidate.m_Active.Get(link);
			RICON_TemporalExpected expected = authority.m_ActiveExpected.Get(link);
			RICON_TemporalValidationResult checked = RICON_TemporalSnapshots.Validate(row.m_Snapshot, expected, m_Limits);
			if (checked.m_sCode != "OK") return checked.m_sCode;
			if (checked.m_iDescriptorBytes > m_Limits.m_iMaxTotalDescriptorBytes - total) return "RESOURCE_LIMIT";
			total += checked.m_iDescriptorBytes;
			row.m_Expected = expected.CloneValidated();
		}
		return "OK";
	}

	RICON_TemporalProtocolApplyResult Apply(int nowMs, RICON_TemporalProtocolEnvelope frame, RICON_TemporalAuthority authority)
	{
		if (m_sConfiguration != "OK") return Reject(m_sConfiguration, false);
		if (!frame || !frame.ValidHeader()) return Reject("INVALID_INPUT", false);
		if (frame.m_iOwnerPlayerId != m_State.m_iOwnerPlayerId || !authority || authority.m_iOwnerPlayerId <= 0
			|| authority.m_iSessionEpoch <= 0 || authority.m_iSessionEpoch >= int.MAX
			|| authority.m_iOwnerPlayerId != m_State.m_iOwnerPlayerId || authority.m_iSessionEpoch != frame.m_iSessionEpoch)
			return Reject("INVALID_AUTHORITY", false);
		bool sameEpoch = m_State.m_iSessionEpoch == frame.m_iSessionEpoch;
		if (sameEpoch && frame.m_iDeliverySeq <= m_State.m_iDeliverySeq) return Result("STALE");
		bool observedScope = m_State.m_iObservedEpoch == frame.m_iSessionEpoch;
		if (observedScope && frame.m_iDeliverySeq <= m_State.m_iObservedDeliverySeq) return Result("STALE");
		if (authority.m_iDeliverySeq <= 0 || authority.m_iDeliverySeq >= int.MAX || authority.m_iLinkIdHighWater < 0
			|| authority.m_iDeliverySeq != frame.m_iDeliverySeq || authority.m_iLinkIdHighWater != frame.m_iLinkIdHighWater
			|| authority.m_sKind != frame.m_sKind) return Reject("INVALID_AUTHORITY", false);
		if (nowMs < 0) return Reject("INVALID_INPUT");
		if (!frame.m_aItems) return Reject("INVALID_INPUT");
		if (frame.m_aItems.Count() > m_Limits.m_iMaxSnapshots) return Reject("BATCH_LIMIT");
		if (!RICON_TemporalProtocolRules.AuthorityMapsValid(authority)) return Reject("INVALID_AUTHORITY");
		string code = AuthorizeItems(frame, authority);
		if (code != "OK") return Reject(code);
		int requiredHigh = 0;
		if (sameEpoch) requiredHigh = m_State.m_iSeenHighWater;
		if (observedScope && m_State.m_iObservedHighWater > requiredHigh) requiredHigh = m_State.m_iObservedHighWater;
		Observe(frame);
		if (frame.m_sKind == "DELTA")
		{
			if (!sameEpoch || m_State.m_bNeedsFull) return Reject("NEED_FULL");
			if (m_State.m_iDeliverySeq >= int.MAX - 1 || frame.m_iDeliverySeq != m_State.m_iDeliverySeq + 1) return Reject("GAP");
		}
		if (sameEpoch && m_State.m_iCheckpointHighWater > requiredHigh) requiredHigh = m_State.m_iCheckpointHighWater;
		if (frame.m_iLinkIdHighWater < requiredHigh) return Reject("HIGH_WATER_ROLLBACK");
		array<ref RICON_TemporalProtocolItem> rows;
		code = Collect(frame, authority, rows);
		if (code != "OK") return Reject(code);
		bool full = frame.m_sKind == "FULL";
		RICON_TemporalProtocolState candidate;
		if (full) candidate = new RICON_TemporalProtocolState();
		else candidate = m_State.CloneOwned();
		foreach (RICON_TemporalProtocolItem item : rows)
		{
			if (item.m_sKind == "ACTIVE")
			{
				RICON_TemporalSnapshot snapshot = item.m_Snapshot;
				int link = snapshot.m_iLinkId;
				RICON_TemporalProtocolRow old;
				if (sameEpoch) old = m_State.m_Active.Get(link);
				if (sameEpoch && (m_State.m_Closed.Contains(link) || (!old && link <= m_State.m_iCheckpointHighWater))) return Reject("CLOSED_ID");
				RICON_TemporalProtocolRow row;
				code = ActiveRow(snapshot, old, nowMs, authority.m_ActiveExpected.Get(link), row);
				if (code != "OK") return Reject(code);
				candidate.m_Active.Set(link, row);
				continue;
			}
			RICON_TemporalEndRecord end = item.m_End;
			int closed = end.m_iLinkId;
			RICON_TemporalProtocolRow oldActive = candidate.m_Active.Get(closed);
			RICON_TemporalEndRecord oldEnd = candidate.m_Closed.Get(closed);
			if (oldActive)
			{
				if (!RICON_TemporalProtocolRules.SnapshotEndBindingSame(oldActive.m_Snapshot, end)) return Reject("BINDING_MISMATCH");
				if (end.m_iLinkSeq < oldActive.m_Snapshot.m_iLinkSeq) return Reject("LINK_SEQUENCE_ROLLBACK");
				if (end.m_iLinkSeq == oldActive.m_Snapshot.m_iLinkSeq) return Reject("LINK_SEQUENCE_CONFLICT");
				candidate.m_Active.Remove(closed);
			}
			else if (oldEnd)
			{
				if (!RICON_TemporalProtocolRules.EndBindingSame(oldEnd, end)) return Reject("BINDING_MISMATCH");
				if (end.m_iLinkSeq < oldEnd.m_iLinkSeq) return Reject("LINK_SEQUENCE_ROLLBACK");
				if (end.m_iLinkSeq == oldEnd.m_iLinkSeq)
				{
					if (!end.Same(oldEnd)) return Reject("LINK_SEQUENCE_CONFLICT");
					continue;
				}
			}
			else if (sameEpoch && closed <= m_State.m_iCheckpointHighWater) continue;
			candidate.m_Closed.Set(closed, end.CloneValidated());
		}
		code = Postflight(candidate, authority);
		if (code != "OK") return Reject(code);
		candidate.m_iOwnerPlayerId = m_State.m_iOwnerPlayerId; candidate.m_iSessionEpoch = frame.m_iSessionEpoch;
		candidate.m_iDeliverySeq = frame.m_iDeliverySeq; candidate.m_iSeenHighWater = m_State.m_iObservedHighWater;
		candidate.m_iObservedEpoch = m_State.m_iObservedEpoch; candidate.m_iObservedDeliverySeq = m_State.m_iObservedDeliverySeq;
		candidate.m_iObservedHighWater = m_State.m_iObservedHighWater; candidate.m_bNeedsFull = false;
		if (full) candidate.m_iCheckpointHighWater = frame.m_iLinkIdHighWater;
		m_State = candidate;
		return Result("ACCEPT_" + frame.m_sKind);
	}
}

//! B-local attempt and observation. Not part of server snapshot equality.
//! Epoch/link/seq associate the evidence with the target command. The address fields are the observed branch.
//! EffectObserved stays false: a stored reference is not a measured actuator result.
class RICON_LocalLinkEvidence
{
	int m_iSessionEpoch, m_iLinkId, m_iLinkSeq;
	int m_iTalkerPlayerId, m_iListenerPlayerId;
	int m_iTxDeviceId, m_iTxRadioIndex, m_iRxDeviceId, m_iRxRadioIndex;
	int m_iTxBindingGeneration, m_iRxBindingGeneration, m_iSoundCarrierGeneration;
	int m_iObservedAtMs;
	string m_sBranch, m_sRef;
	bool m_bSharedRxExpectationMatched, m_bWindowEligible;

	RICON_LocalLinkEvidence CloneEvidence()
	{
		RICON_LocalLinkEvidence c = new RICON_LocalLinkEvidence();
		c.m_iSessionEpoch = m_iSessionEpoch; c.m_iLinkId = m_iLinkId; c.m_iLinkSeq = m_iLinkSeq;
		c.m_iTalkerPlayerId = m_iTalkerPlayerId; c.m_iListenerPlayerId = m_iListenerPlayerId;
		c.m_iTxDeviceId = m_iTxDeviceId; c.m_iTxRadioIndex = m_iTxRadioIndex;
		c.m_iRxDeviceId = m_iRxDeviceId; c.m_iRxRadioIndex = m_iRxRadioIndex;
		c.m_iTxBindingGeneration = m_iTxBindingGeneration; c.m_iRxBindingGeneration = m_iRxBindingGeneration;
		c.m_iSoundCarrierGeneration = m_iSoundCarrierGeneration; c.m_iObservedAtMs = m_iObservedAtMs;
		c.m_sBranch = m_sBranch; c.m_sRef = m_sRef;
		c.m_bSharedRxExpectationMatched = m_bSharedRxExpectationMatched; c.m_bWindowEligible = m_bWindowEligible;
		return c;
	}
}

class RICON_LocalLinkView
{
	string m_sCode = "NULL_INPUT";
	bool m_bStored;
	int m_iSessionEpoch, m_iLinkId, m_iLinkSeq;
	int m_iTalkerPlayerId, m_iListenerPlayerId;
	int m_iLeaseId, m_iTxBindingGeneration, m_iRxBindingGeneration;
	int m_iTxDeviceId, m_iTxRadioIndex, m_iRxDeviceId, m_iRxRadioIndex;
	string m_sMode, m_sTimebaseId;
	int m_iCommandT0Ms, m_iCommandT1Ms;
	int m_iPhysicalValidFromMs, m_iPhysicalValidUntilMs;
	int m_iPresentationTtlMs, m_iLocalReceivedAtMs, m_iPresentationExpiryMs = -1;
	int m_iSoundCarrierGeneration;
	bool m_bApplicable, m_bCarrierQualified, m_bAttemptRecorded, m_bTargetWindowEligible;
	string m_sActuatorResult;
	ref RICON_LocalLinkEvidence m_Target, m_ControlCB, m_ControlAD, m_Direct;
	//! Assigned effect for this revision. Not the lease and not the presentation TTL.
	ref RICON_FadingDescriptor m_Recipe;

	bool EffectObserved() { return false; }
}

class RICON_LocalLinkRecord
{
	int m_iSessionEpoch, m_iLinkId, m_iLinkSeq;
	int m_iTalkerPlayerId, m_iListenerPlayerId;
	int m_iLeaseId, m_iTxBindingGeneration, m_iRxBindingGeneration;
	int m_iTxDeviceId, m_iTxRadioIndex, m_iRxDeviceId, m_iRxRadioIndex;
	int m_iNetworkTypeId, m_iChannelId;
	string m_sMode, m_sTimebaseId;
	int m_iCommandT0Ms, m_iCommandT1Ms;
	int m_iPhysicalValidFromMs, m_iPhysicalValidUntilMs;
	int m_iPresentationTtlMs, m_iLocalReceivedAtMs;
	int m_iSoundCarrierGeneration;
	bool m_bAttemptRecorded;
	string m_sActuatorResult;
	ref RICON_LocalLinkEvidence m_Target, m_ControlCB, m_ControlAD, m_Direct;
	ref RICON_FadingDescriptor m_Recipe;
}

class RICON_LocalLinkAdapter
{
	protected ref map<string, ref RICON_LocalLinkRecord> m_Records = new map<string, ref RICON_LocalLinkRecord>();
	protected ref map<string, string> m_Latest = new map<string, string>();
	protected ref array<string> m_Order = new array<string>();
	//! Same retained ceiling this file already applies to the receiver: maxSnapshots, at most 65536.
	protected int m_iRetainedLimit = 65536;

	protected string LinkKey(int sessionEpoch, int linkId) { return sessionEpoch.ToString() + ":" + linkId.ToString(); }
	protected string RevisionKey(RICON_LocalLinkRecord record) { return LinkKey(record.m_iSessionEpoch, record.m_iLinkId) + ":" + record.m_iLinkSeq.ToString(); }

	protected RICON_LocalLinkRecord Current(int sessionEpoch, int linkId)
	{
		string linkKey = LinkKey(sessionEpoch, linkId);
		if (!m_Latest.Contains(linkKey)) return null;
		return m_Records.Get(m_Latest.Get(linkKey));
	}

	protected bool RetireOldestCompleted()
	{
		for (int i = 0; i < m_Order.Count(); i++)
		{
			string revisionKey = m_Order.Get(i);
			RICON_LocalLinkRecord record = m_Records.Get(revisionKey);
			if (!record) { m_Order.Remove(i); return true; }
			if (m_Latest.Get(LinkKey(record.m_iSessionEpoch, record.m_iLinkId)) != revisionKey)
			{
				m_Records.Remove(revisionKey);
				m_Order.Remove(i);
				return true;
			}
		}
		return false;
	}

	protected bool Retain(RICON_LocalLinkRecord created)
	{
		string revisionKey = RevisionKey(created);
		if (m_Records.Contains(revisionKey)) return false;
		while (m_Records.Count() >= m_iRetainedLimit && RetireOldestCompleted()) {}
		if (m_Records.Count() >= m_iRetainedLimit) return false;
		m_Records.Set(revisionKey, created);
		m_Order.Insert(revisionKey);
		m_Latest.Set(LinkKey(created.m_iSessionEpoch, created.m_iLinkId), revisionKey);
		return true;
	}

	protected bool BranchOk(string branch)
	{
		return branch == "A_TO_B" || branch == "C_TO_B" || branch == "A_TO_D" || branch == "DIRECT";
	}

	protected RICON_LocalLinkView Fail(string code)
	{
		RICON_LocalLinkView view = new RICON_LocalLinkView();
		view.m_sCode = code;
		return view;
	}

	protected void ClearEvidence(RICON_LocalLinkRecord record)
	{
		record.m_Target = null; record.m_ControlCB = null; record.m_ControlAD = null; record.m_Direct = null;
	}

	protected RICON_LocalLinkEvidence CopyEvidence(RICON_LocalLinkEvidence evidence)
	{
		if (!evidence) return null;
		return evidence.CloneEvidence();
	}

	protected RICON_LocalLinkView Expose(RICON_LocalLinkRecord record, string code, bool applicable)
	{
		RICON_LocalLinkView view = new RICON_LocalLinkView();
		view.m_sCode = code; view.m_bStored = true; view.m_bApplicable = applicable;
		view.m_iSessionEpoch = record.m_iSessionEpoch; view.m_iLinkId = record.m_iLinkId; view.m_iLinkSeq = record.m_iLinkSeq;
		view.m_iTalkerPlayerId = record.m_iTalkerPlayerId; view.m_iListenerPlayerId = record.m_iListenerPlayerId;
		view.m_iLeaseId = record.m_iLeaseId;
		view.m_iTxBindingGeneration = record.m_iTxBindingGeneration; view.m_iRxBindingGeneration = record.m_iRxBindingGeneration;
		view.m_iTxDeviceId = record.m_iTxDeviceId; view.m_iTxRadioIndex = record.m_iTxRadioIndex;
		view.m_iRxDeviceId = record.m_iRxDeviceId; view.m_iRxRadioIndex = record.m_iRxRadioIndex;
		view.m_sMode = record.m_sMode; view.m_sTimebaseId = record.m_sTimebaseId;
		view.m_iCommandT0Ms = record.m_iCommandT0Ms; view.m_iCommandT1Ms = record.m_iCommandT1Ms;
		view.m_iPhysicalValidFromMs = record.m_iPhysicalValidFromMs; view.m_iPhysicalValidUntilMs = record.m_iPhysicalValidUntilMs;
		view.m_iPresentationTtlMs = record.m_iPresentationTtlMs; view.m_iLocalReceivedAtMs = record.m_iLocalReceivedAtMs;
		if (record.m_iLocalReceivedAtMs >= 0 && record.m_iPresentationTtlMs > 0
			&& record.m_iPresentationTtlMs <= int.MAX - record.m_iLocalReceivedAtMs)
			view.m_iPresentationExpiryMs = record.m_iLocalReceivedAtMs + record.m_iPresentationTtlMs;
		view.m_iSoundCarrierGeneration = record.m_iSoundCarrierGeneration;
		view.m_bCarrierQualified = applicable && record.m_iSoundCarrierGeneration > 0;
		view.m_bAttemptRecorded = record.m_bAttemptRecorded; view.m_sActuatorResult = record.m_sActuatorResult;
		view.m_Target = CopyEvidence(record.m_Target);
		view.m_ControlCB = CopyEvidence(record.m_ControlCB);
		view.m_ControlAD = CopyEvidence(record.m_ControlAD);
		view.m_Direct = CopyEvidence(record.m_Direct);
		view.m_bTargetWindowEligible = view.m_Target && view.m_Target.m_bWindowEligible;
		if (record.m_Recipe) view.m_Recipe = RICON_DescriptorCodec.Clone(record.m_Recipe);
		return view;
	}

	protected bool Capture(RICON_TemporalSnapshot snapshot, int receivedAtMs, int commandT0Ms, int commandT1Ms, out RICON_LocalLinkRecord record)
	{
		record = null;
		if (!snapshot || !snapshot.m_Binding || !snapshot.m_Binding.IsValid() || !snapshot.m_Descriptor) return false;
		if (snapshot.m_iTalkerPlayerId <= 0 || snapshot.m_iListenerPlayerId <= 0
			|| snapshot.m_iTalkerPlayerId == snapshot.m_iListenerPlayerId) return false;
		if (snapshot.m_sMode != "SERVER" && snapshot.m_sMode != "RECEIVER") return false;
		if (!RICON_TemporalSnapshots.ValidKey(snapshot.m_sTimebaseId)) return false;
		int physicalFrom = snapshot.m_Descriptor.m_iValidFromMs;
		int physicalUntil = snapshot.m_Descriptor.m_iValidUntilMs;
		if (physicalFrom < 0 || physicalUntil <= physicalFrom) return false;
		RICON_TemporalBinding binding = snapshot.m_Binding;
		record = new RICON_LocalLinkRecord();
		record.m_iSessionEpoch = snapshot.m_iSessionEpoch; record.m_iLinkId = snapshot.m_iLinkId; record.m_iLinkSeq = snapshot.m_iLinkSeq;
		record.m_iTalkerPlayerId = snapshot.m_iTalkerPlayerId; record.m_iListenerPlayerId = snapshot.m_iListenerPlayerId;
		record.m_iLeaseId = binding.m_iLeaseId;
		record.m_iTxBindingGeneration = binding.m_iTxGeneration; record.m_iRxBindingGeneration = binding.m_iRxGeneration;
		record.m_iTxDeviceId = binding.m_iTxDeviceId; record.m_iTxRadioIndex = binding.m_iTxTransceiverIndex;
		record.m_iRxDeviceId = binding.m_iRxDeviceId; record.m_iRxRadioIndex = binding.m_iRxTransceiverIndex;
		record.m_iNetworkTypeId = binding.m_iNetworkTypeId; record.m_iChannelId = binding.m_iChannelId;
		record.m_sMode = snapshot.m_sMode; record.m_sTimebaseId = snapshot.m_sTimebaseId;
		record.m_iCommandT0Ms = commandT0Ms; record.m_iCommandT1Ms = commandT1Ms;
		record.m_iPhysicalValidFromMs = physicalFrom; record.m_iPhysicalValidUntilMs = physicalUntil;
		record.m_iPresentationTtlMs = snapshot.m_iTtlMs; record.m_iLocalReceivedAtMs = receivedAtMs;
		record.m_Recipe = RICON_DescriptorCodec.Clone(snapshot.m_Descriptor);
		return true;
	}

	protected bool SameAddress(RICON_LocalLinkRecord record, RICON_TemporalSnapshot snapshot)
	{
		if (!record || !snapshot || !snapshot.m_Binding) return false;
		RICON_TemporalBinding binding = snapshot.m_Binding;
		return record.m_iTalkerPlayerId == snapshot.m_iTalkerPlayerId
			&& record.m_iListenerPlayerId == snapshot.m_iListenerPlayerId
			&& record.m_iLeaseId == binding.m_iLeaseId
			&& record.m_iTxBindingGeneration == binding.m_iTxGeneration
			&& record.m_iRxBindingGeneration == binding.m_iRxGeneration
			&& record.m_iTxDeviceId == binding.m_iTxDeviceId && record.m_iTxRadioIndex == binding.m_iTxTransceiverIndex
			&& record.m_iRxDeviceId == binding.m_iRxDeviceId && record.m_iRxRadioIndex == binding.m_iRxTransceiverIndex
			&& record.m_sMode == snapshot.m_sMode && record.m_sTimebaseId == snapshot.m_sTimebaseId;
	}

	RICON_LocalLinkView AcceptCommand(RICON_TemporalOrderedReceiver receiver, int linkId, int commandT0Ms, int commandT1Ms, int nowMs, RICON_TemporalMapping mapping)
	{
		if (!receiver || linkId <= 0) return Fail("NULL_INPUT");
		if (commandT0Ms < 0 || commandT1Ms <= commandT0Ms || commandT1Ms >= int.MAX) return Fail("INVALID_WINDOW");
		RICON_TemporalProtocolState state = receiver.State();
		if (!state || state.m_bNeedsFull) return Fail("NEEDS_FULL");
		RICON_TemporalProtocolRow row = state.m_Active.Get(linkId);
		if (!row || !row.m_Snapshot || row.m_Snapshot.m_iLinkId != linkId) return Fail("NOT_FOUND");
		RICON_LocalLinkRecord created;
		if (!Capture(row.m_Snapshot, row.m_iReceivedAtMs, commandT0Ms, commandT1Ms, created)) return Fail("INVALID_SNAPSHOT");
		RICON_LocalLinkRecord current = Current(created.m_iSessionEpoch, created.m_iLinkId);
		if (current)
		{
			if (created.m_iLinkSeq < current.m_iLinkSeq) return Fail("STALE");
			if (created.m_iLinkSeq == current.m_iLinkSeq)
			{
				if (!SameAddress(current, row.m_Snapshot)) return Fail("IDENTITY_MISMATCH");
				current.m_iPresentationTtlMs = created.m_iPresentationTtlMs;
				current.m_iLocalReceivedAtMs = created.m_iLocalReceivedAtMs;
				return Expose(current, "OK", Applicable(receiver, current, nowMs, mapping));
			}
		}
		if (!Retain(created)) return Fail("RESOURCE_LIMIT");
		return Expose(created, "OK", Applicable(receiver, created, nowMs, mapping));
	}

	protected bool CurrentIdentity(RICON_TemporalOrderedReceiver receiver, RICON_LocalLinkRecord record)
	{
		if (!receiver || !record) return false;
		RICON_TemporalProtocolState state = receiver.State();
		if (!state || state.m_bNeedsFull || state.m_iSessionEpoch != record.m_iSessionEpoch) return false;
		RICON_TemporalProtocolRow row = state.m_Active.Get(record.m_iLinkId);
		if (!row || !row.m_Snapshot) return false;
		RICON_TemporalSnapshot snapshot = row.m_Snapshot;
		return snapshot.m_iSessionEpoch == record.m_iSessionEpoch && snapshot.m_iLinkId == record.m_iLinkId
			&& snapshot.m_iLinkSeq == record.m_iLinkSeq
			&& SameAddress(record, snapshot);
	}

	protected bool CommandIntersectsPhysical(RICON_LocalLinkRecord record)
	{
		return record && record.m_iCommandT0Ms < record.m_iPhysicalValidUntilMs
			&& record.m_iPhysicalValidFromMs < record.m_iCommandT1Ms;
	}

	protected bool Applicable(RICON_TemporalOrderedReceiver receiver, RICON_LocalLinkRecord record, int nowMs, RICON_TemporalMapping mapping)
	{
		return CurrentIdentity(receiver, record) && receiver.IsVisible(nowMs, record.m_iLinkId)
			&& WindowEligible(record, nowMs, mapping);
	}

	protected bool SameRadioBinding(RICON_LocalLinkRecord record, RICON_LocalLinkEvidence evidence)
	{
		return evidence.m_iTalkerPlayerId == record.m_iTalkerPlayerId
			&& evidence.m_iListenerPlayerId == record.m_iListenerPlayerId
			&& evidence.m_iTxDeviceId == record.m_iTxDeviceId && evidence.m_iTxRadioIndex == record.m_iTxRadioIndex
			&& evidence.m_iRxDeviceId == record.m_iRxDeviceId && evidence.m_iRxRadioIndex == record.m_iRxRadioIndex
			&& evidence.m_iTxBindingGeneration == record.m_iTxBindingGeneration
			&& evidence.m_iRxBindingGeneration == record.m_iRxBindingGeneration;
	}

	protected bool ObservedAddressValid(RICON_LocalLinkEvidence evidence)
	{
		return evidence && evidence.m_iTalkerPlayerId > 0 && evidence.m_iListenerPlayerId > 0
			&& evidence.m_iTalkerPlayerId != evidence.m_iListenerPlayerId
			&& evidence.m_iTxDeviceId > 0 && evidence.m_iRxDeviceId > 0 && evidence.m_iTxDeviceId != evidence.m_iRxDeviceId
			&& evidence.m_iTxRadioIndex >= 0 && evidence.m_iRxRadioIndex >= 0
			&& evidence.m_iTxBindingGeneration > 0 && evidence.m_iRxBindingGeneration > 0
			&& evidence.m_iSoundCarrierGeneration > 0 && evidence.m_iSoundCarrierGeneration < int.MAX;
	}

	//! Empty result accepts the observed branch. A relabelled target address is LABEL_ONLY.
	protected string DirectAddress(RICON_LocalLinkRecord record, RICON_LocalLinkEvidence evidence)
	{
		if (!evidence || evidence.m_iSoundCarrierGeneration <= 0 || evidence.m_iSoundCarrierGeneration >= int.MAX)
			return "INVALID_BINDING";
		if (evidence.m_iTalkerPlayerId != record.m_iTalkerPlayerId || evidence.m_iListenerPlayerId != record.m_iListenerPlayerId)
			return "IDENTITY_MISMATCH";
		if (evidence.m_iTxBindingGeneration != record.m_iTxBindingGeneration
			|| evidence.m_iRxBindingGeneration != record.m_iRxBindingGeneration)
			return "IDENTITY_MISMATCH";
		if (evidence.m_iTxDeviceId != 0 || evidence.m_iRxDeviceId != 0 || evidence.m_iTxRadioIndex != -1 || evidence.m_iRxRadioIndex != -1)
			return "IDENTITY_MISMATCH";
		return "";
	}

	protected string BranchAddress(RICON_LocalLinkRecord record, RICON_LocalLinkEvidence evidence)
	{
		if (evidence && evidence.m_sBranch == "DIRECT") return DirectAddress(record, evidence);
		if (!ObservedAddressValid(evidence)) return "INVALID_BINDING";
		bool targetBinding = SameRadioBinding(record, evidence);
		if (evidence.m_sBranch == "A_TO_B")
		{
			if (!targetBinding) return "IDENTITY_MISMATCH";
			if (record.m_iSoundCarrierGeneration <= 0 || evidence.m_iSoundCarrierGeneration != record.m_iSoundCarrierGeneration)
				return "CARRIER_UNQUALIFIED";
			return "";
		}
		if (targetBinding) return "LABEL_ONLY";
		if (evidence.m_sBranch == "C_TO_B")
		{
			if (evidence.m_iListenerPlayerId != record.m_iListenerPlayerId || evidence.m_iTalkerPlayerId == record.m_iTalkerPlayerId)
				return "IDENTITY_MISMATCH";
			if (evidence.m_iRxDeviceId != record.m_iRxDeviceId || evidence.m_iRxRadioIndex != record.m_iRxRadioIndex
				|| evidence.m_iTxDeviceId == record.m_iTxDeviceId)
				return "IDENTITY_MISMATCH";
			if (evidence.m_iRxBindingGeneration != record.m_iRxBindingGeneration) return "IDENTITY_MISMATCH";
			return "";
		}
		if (evidence.m_sBranch == "A_TO_D")
		{
			if (evidence.m_iTalkerPlayerId != record.m_iTalkerPlayerId || evidence.m_iListenerPlayerId == record.m_iListenerPlayerId)
				return "IDENTITY_MISMATCH";
			if (evidence.m_iTxDeviceId != record.m_iTxDeviceId || evidence.m_iTxRadioIndex != record.m_iTxRadioIndex
				|| evidence.m_iRxDeviceId == record.m_iRxDeviceId)
				return "IDENTITY_MISMATCH";
			if (evidence.m_iTxBindingGeneration != record.m_iTxBindingGeneration) return "IDENTITY_MISMATCH";
			return "";
		}
		return "INVALID_BRANCH";
	}

	protected bool WindowEligible(RICON_LocalLinkRecord record, int observedAtMs, RICON_TemporalMapping mapping)
	{
		if (!record || !mapping || mapping.m_sTimebaseId != record.m_sTimebaseId
			|| !RICON_TemporalSnapshots.ValidKey(mapping.m_sMappingId)) return false;
		int estimateMs, estimateSubUs, lowerMs, lowerSubUs, upperMs, upperSubUs;
		int clock = RICON_TemporalClock.MapInterval(observedAtMs, 0, mapping.m_iLocalAnchorMs, mapping.m_iServerAnchorMs,
			mapping.m_iValidFromLocalMs, mapping.m_iValidUntilLocalMs, mapping.m_iMaxErrorUs,
			estimateMs, estimateSubUs, lowerMs, lowerSubUs, upperMs, upperSubUs);
		if (clock != 0 || !CommandIntersectsPhysical(record)) return false;
		int fromMs = record.m_iCommandT0Ms;
		int untilMs = record.m_iCommandT1Ms;
		if (record.m_iPhysicalValidFromMs > fromMs) fromMs = record.m_iPhysicalValidFromMs;
		if (record.m_iPhysicalValidUntilMs < untilMs) untilMs = record.m_iPhysicalValidUntilMs;
		return RICON_TemporalClock.ContainsInterval(lowerMs, lowerSubUs, upperMs, upperSubUs, fromMs, untilMs);
	}

	RICON_LocalLinkView RecordAttempt(RICON_TemporalOrderedReceiver receiver, int nowMs, int sessionEpoch, int linkId, int linkSeq, int rxBindingGeneration, int soundCarrierGeneration, RICON_TemporalMapping mapping)
	{
		if (!receiver) return Fail("NULL_INPUT");
		if (sessionEpoch <= 0 || linkId <= 0 || linkSeq <= 0 || linkSeq >= int.MAX) return Fail("INVALID_INPUT");
		if (rxBindingGeneration <= 0) return Fail("INVALID_BINDING");
		if (soundCarrierGeneration < 0 || soundCarrierGeneration >= int.MAX) return Fail("INVALID_CARRIER");
		RICON_LocalLinkRecord record = Current(sessionEpoch, linkId);
		if (!record) return Fail("NOT_FOUND");
		if (!Applicable(receiver, record, nowMs, mapping)) return Expose(record, "NOT_APPLICABLE", false);
		if (linkSeq != record.m_iLinkSeq) return Fail("STALE");
		if (rxBindingGeneration != record.m_iRxBindingGeneration) return Fail("BINDING_MISMATCH");
		if (record.m_iSoundCarrierGeneration != soundCarrierGeneration)
		{
			record.m_iSoundCarrierGeneration = soundCarrierGeneration;
			ClearEvidence(record);
		}
		record.m_bAttemptRecorded = true;
		record.m_sActuatorResult = "BLOCKED_NO_ACTUATOR";
		return Expose(record, "OK", true);
	}

	RICON_LocalLinkView RecordEvidence(RICON_TemporalOrderedReceiver receiver, int nowMs, RICON_LocalLinkEvidence evidence, RICON_TemporalMapping mapping)
	{
		if (!receiver || !evidence) return Fail("NULL_INPUT");
		if (!BranchOk(evidence.m_sBranch)) return Fail("INVALID_BRANCH");
		if (evidence.m_sRef == "") return Fail("NO_MEASUREMENT");
		if (evidence.m_iObservedAtMs < 0) return Fail("INVALID_INPUT");
		RICON_LocalLinkRecord record = Current(evidence.m_iSessionEpoch, evidence.m_iLinkId);
		if (!record) return Fail("NOT_FOUND");
		if (!CurrentIdentity(receiver, record)) return Expose(record, "NOT_APPLICABLE", false);
		if (evidence.m_iLinkSeq != record.m_iLinkSeq || evidence.m_iSessionEpoch != record.m_iSessionEpoch) return Fail("STALE");
		string branch = BranchAddress(record, evidence);
		if (branch != "") return Fail(branch);
		RICON_LocalLinkEvidence copy = evidence.CloneEvidence();
		copy.m_bWindowEligible = WindowEligible(record, copy.m_iObservedAtMs, mapping);
		if (copy.m_sBranch == "A_TO_B") record.m_Target = copy;
		else if (copy.m_sBranch == "C_TO_B") record.m_ControlCB = copy;
		else if (copy.m_sBranch == "A_TO_D") record.m_ControlAD = copy;
		else record.m_Direct = copy;
		bool applicable = Applicable(receiver, record, nowMs, mapping) && copy.m_bWindowEligible;
		if (!applicable) return Expose(record, "HISTORICAL", false);
		return Expose(record, "RECORDED", true);
	}

	RICON_LocalLinkView Read(RICON_TemporalOrderedReceiver receiver, int nowMs, int sessionEpoch, int linkId, RICON_TemporalMapping mapping)
	{
		if (sessionEpoch <= 0 || linkId <= 0) return Fail("INVALID_INPUT");
		RICON_LocalLinkRecord record = Current(sessionEpoch, linkId);
		if (!record) return Fail("NOT_FOUND");
		bool applicable = Applicable(receiver, record, nowMs, mapping);
		if (!applicable) return Expose(record, "NOT_APPLICABLE", false);
		return Expose(record, "OK", true);
	}

	bool EffectObserved(int sessionEpoch, int linkId)
	{
		if (sessionEpoch > 0 && linkId > 0) return false;
		return false;
	}
}
