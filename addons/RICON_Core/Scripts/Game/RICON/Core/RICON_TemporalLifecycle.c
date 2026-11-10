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
