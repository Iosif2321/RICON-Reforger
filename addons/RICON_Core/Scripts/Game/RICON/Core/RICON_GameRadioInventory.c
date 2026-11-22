//! Internal trusted-source handles are never accepted from a client RPC.
class RICON_GameRadioInventoryHandle {}

class RICON_GameRadioInventoryObservation
{
	ref RICON_GameRadioInventoryHandle m_Handle;
	int m_iPlayerId, m_iConnectionId, m_iPlayerGeneration, m_iEntityId, m_iTransceiverCount;
	bool m_bPowered;
	string m_sPolicyIdentity;
	int m_iPolicyRevision, m_iNetworkId;
	ref array<int> m_ChannelIds = {};
	ref array<ref RICON_LifecycleCatalogPair> m_AllowedPairs = {};

	bool IsValid()
	{
		if (!m_Handle || m_iPlayerId <= 0 || m_iConnectionId <= 0 || m_iPlayerGeneration <= 0
			|| m_iEntityId <= 0 || m_iTransceiverCount <= 0 || !m_ChannelIds || !m_AllowedPairs) return false;
		if (m_iNetworkId == 0)
			return m_sPolicyIdentity.IsEmpty() && m_iPolicyRevision == 0 && m_ChannelIds.IsEmpty() && m_AllowedPairs.IsEmpty();
		if (m_iNetworkId < 0 || m_iPolicyRevision <= 0 || !RICON_GameRadioControlPolicy.IsValidIdentity(m_sPolicyIdentity)
			|| m_ChannelIds.Count() != m_iTransceiverCount) return false;
		array<int> unique = {};
		foreach (int channelId : m_ChannelIds)
		{
			if (channelId < 0) return false;
			if (channelId == 0) continue; // Native slot without a RICON RF channel, e.g. the vehicle intercom.
			if (!unique.Contains(channelId)) unique.Insert(channelId);
		}
		unique.Sort();
		if (unique.IsEmpty() || m_AllowedPairs.Count() != unique.Count()) return false;
		for (int index = 0; index < unique.Count(); index++)
			if (!m_AllowedPairs[index] || m_AllowedPairs[index].m_iNetworkTypeId != m_iNetworkId
				|| m_AllowedPairs[index].m_iChannelId != unique[index]) return false;
		return true;
	}

	bool DeclareControl(RICON_GameRadioControlPolicy policy, array<int> channelIds)
	{
		if (!policy || !channelIds || channelIds.Count() != m_iTransceiverCount) return false;
		array<int> detached = {}; detached.InsertAll(channelIds);
		m_sPolicyIdentity = policy.Identity(); m_iPolicyRevision = policy.Revision(); m_iNetworkId = policy.NetworkId();
		m_ChannelIds.Clear(); m_ChannelIds.InsertAll(detached); m_AllowedPairs.Clear();
		array<int> unique = {};
		foreach (int channelId : detached)
			if (channelId > 0 && !unique.Contains(channelId)) unique.Insert(channelId);
		unique.Sort();
		foreach (int allowed : unique) m_AllowedPairs.Insert(new RICON_LifecycleCatalogPair(m_iNetworkId, allowed));
		return IsValid();
	}

	RICON_GameRadioInventoryObservation CloneObservation()
	{
		RICON_GameRadioInventoryObservation c = new RICON_GameRadioInventoryObservation();
		c.m_Handle = m_Handle;
		c.m_iPlayerId = m_iPlayerId; c.m_iConnectionId = m_iConnectionId;
		c.m_iPlayerGeneration = m_iPlayerGeneration; c.m_iEntityId = m_iEntityId;
		c.m_iTransceiverCount = m_iTransceiverCount; c.m_bPowered = m_bPowered;
		c.m_sPolicyIdentity = m_sPolicyIdentity; c.m_iPolicyRevision = m_iPolicyRevision; c.m_iNetworkId = m_iNetworkId;
		c.m_ChannelIds.InsertAll(m_ChannelIds);
		foreach (RICON_LifecycleCatalogPair pair : m_AllowedPairs)
			c.m_AllowedPairs.Insert(new RICON_LifecycleCatalogPair(pair.m_iNetworkTypeId, pair.m_iChannelId));
		return c;
	}
}

//! The production implementation reads native server inventory; fixtures replace only this boundary.
class RICON_GameRadioInventorySource
{
	bool IsScope(RICON_LifecycleStateMachine lifecycle, RICON_MediaReceiverAuthority receivers) { return false; }
	int ReadNowMs() { return -1; }
	bool Capture(array<ref RICON_GameRadioInventoryObservation> observations) { return false; }
	string CaptureFailure() { return "INCOMPLETE"; }
	bool IsCurrent(RICON_GameRadioInventoryObservation observation) { return false; }
	bool SamePhysical(RICON_GameRadioInventoryObservation a, RICON_GameRadioInventoryObservation b)
	{
		return a && b && a.m_Handle && a.m_Handle == b.m_Handle;
	}
	bool SameLayout(RICON_GameRadioInventoryObservation a, RICON_GameRadioInventoryObservation b)
	{
		return SamePhysical(a, b);
	}
	int Register(int deviceId, int index, RICON_GameRadioInventoryObservation observation, int nowMs) { return 0; }
	bool UsesOpenRadioAdmissions() { return false; }
	//! Zero excludes this slot by policy; a negative value means the observation cannot be relied on.
	int ReadOpenRadioTune(RICON_GameRadioInventoryObservation observation, int index) { return 0; }
	bool CanConsume(int preEpoch) { return false; }
	bool Consume(RICON_LifecycleResult result, int preEpoch) { return false; }
}

class RICON_GameRadioInventoryRecord
{
	ref RICON_GameRadioInventoryObservation m_Observation;
	ref array<int> m_Generations = {};
	ref array<int> m_WatchVersions = {};
	int m_iDeviceId;
}

class RICON_GameRadioInventoryFailure
{
	ref RICON_GameRadioInventoryObservation m_Observation;
	int m_iAvailableSlots;
	string m_sReason;
}

//! Server-only snapshot row. The radio reference never enters a wire format.
class RICON_GameRadioOpenSlot
{
	ref RICON_OpenRadioPairListener m_Listener;
	BaseRadioComponent m_Radio;
	int m_iAdmission;
}

//! Server catalogue ownership and optional physical admissions; listener and media rights are separate.
class RICON_GameRadioInventory
{
	protected ref RICON_LifecycleStateMachine m_Lifecycle;
	protected ref RICON_MediaReceiverAuthority m_Receivers;
	protected ref RICON_GameRadioInventorySource m_Source;
	protected ref array<ref RICON_GameRadioInventoryRecord> m_Records = {};
	protected ref array<ref RICON_GameRadioInventoryFailure> m_Failures = {};
	protected int m_iEpoch;
	protected int m_iReconcileRevision;
	protected bool m_bBusy, m_bClosed, m_bClosePending;

	bool Configure(RICON_LifecycleStateMachine lifecycle, RICON_MediaReceiverAuthority receivers,
		RICON_GameRadioInventorySource source)
	{
		if (m_bClosed || m_Source || !lifecycle || !receivers || !source || lifecycle.GetSessionEpoch() <= 0
			|| !receivers.IsScope(lifecycle) || !source.IsScope(lifecycle, receivers) || m_bClosed) return false;
		m_Lifecycle = lifecycle; m_Receivers = receivers; m_Source = source;
		m_iEpoch = lifecycle.GetSessionEpoch();
		return true;
	}

	int GetRecordCount() { return m_Records.Count(); }

	//! A stock-VoN RF sample includes slots previously muted by the jammer.
	bool ReadStockRadios(array<ref RICON_StockJammerRadio> radios)
	{
		if (!radios || m_bBusy || m_bClosed || FreshNow() < 0) return false;
		foreach (RICON_GameRadioInventoryRecord record : m_Records)
		{
			RICON_GameRadioInventoryObservation observation = record.m_Observation;
			if (!observation.m_bPowered) continue;
			RICON_GameRadioNativeHandle handle = RICON_GameRadioNativeHandle.Cast(observation.m_Handle);
			if (!handle || !Current(observation) || !handle.Radio()) return false;
			RICON_StockJammerRadio radio = new RICON_StockJammerRadio();
			radio.m_iDeviceId = record.m_iDeviceId;
			radio.m_iPlayerId = observation.m_iPlayerId;
			radio.m_Radio = handle.Radio();
			radios.Insert(radio);
		}
		return SameScope();
	}

	int GetReconcileRevision()
	{
		if (m_bClosed || m_bBusy || m_iReconcileRevision == int.MAX) return 0;
		return m_iReconcileRevision;
	}

	bool OwnsOpenRadioScope(RICON_LifecycleStateMachine lifecycle, RICON_MediaReceiverAuthority receivers)
	{
		return !m_bClosed && lifecycle && lifecycle == m_Lifecycle && receivers && receivers == m_Receivers
			&& m_Source && m_Source.UsesOpenRadioAdmissions() && SameScope() && !m_bClosed;
	}

	string Reconcile()
	{
		ref RICON_GameRadioInventory held = this;
		if (m_bClosed || !m_Source) return "CLOSED";
		if (m_bBusy) return "BUSY";
		m_bBusy = true;
		string status = ReconcileCurrentOperation();
		Finish();
		return status;
	}

	protected string ReconcileCurrentOperation()
	{
		if (m_iReconcileRevision < int.MAX) m_iReconcileRevision++;
		string status = ReconcileHeld();
		if (!m_bClosed && !SameScope()) { m_bClosed = true; m_bClosePending = true; status = "SCOPE_LOST"; }
		if (m_bClosed) status = "CLOSED";
		return status;
	}

	//! A cached owner tuple is insufficient while an item is being transferred between inventories.
	int ReadOwnedDeviceId(int playerId, RICON_GameRadioInventoryHandle handle)
	{
		ref RICON_GameRadioInventory held = this;
		if (m_bClosed || m_bBusy || !m_Source || playerId <= 0 || !handle) return 0;
		m_bBusy = true;
		if (FreshNow() < 0 || m_bClosed) { Close(); Finish(); return 0; }
		ref RICON_GameRadioInventoryRecord record = FindOwnedRecordHeld(playerId, handle);
		int deviceId;
		if (record) deviceId = record.m_iDeviceId;
		if (m_bClosed || !SameScope()) deviceId = 0;
		Finish();
		return deviceId;
	}

	//! Server caller supplies a selected index, never an arbitrary member of the radio's allowed union.
	bool ReadOwnedControl(int playerId, RICON_GameRadioInventoryHandle handle, int index,
		out int deviceId, out int networkId, out int channelId, out int rxGeneration)
	{
		ref RICON_GameRadioInventory held = this;
		deviceId = 0; networkId = 0; channelId = 0; rxGeneration = 0;
		if (m_bClosed || m_bBusy || !m_Source || playerId <= 0 || !handle || index < 0) return false;
		m_bBusy = true;
		if (FreshNow() < 0 || m_bClosed) { Close(); Finish(); return false; }
		ref RICON_GameRadioInventoryRecord record = FindOwnedRecordHeld(playerId, handle);
		bool accepted = record && record.m_Observation.m_bPowered && record.m_Observation.m_iNetworkId > 0
			&& index < record.m_Observation.m_ChannelIds.Count() && index < record.m_Generations.Count()
			&& record.m_Observation.m_ChannelIds[index] > 0;
		if (m_bClosed || !SameScope()) accepted = false;
		if (accepted)
		{
			deviceId = record.m_iDeviceId; networkId = record.m_Observation.m_iNetworkId;
			channelId = record.m_Observation.m_ChannelIds[index]; rxGeneration = record.m_Generations[index];
		}
		Finish();
		return accepted;
	}

	//! Reconcile once and return a complete admitted set for one exact open channel.
	bool CaptureOpenRadioSlots(int networkId, int channelId, int rawTune, int maxSlots,
		array<ref RICON_GameRadioOpenSlot> slots, out int sampleMs)
	{
		ref RICON_GameRadioInventory held = this;
		sampleMs = -1;
		if (!slots) return false;
		slots.Clear();
		if (m_bClosed || m_bBusy || !m_Source || networkId <= 0 || channelId <= 0 || rawTune <= 0 || maxSlots <= 0) return false;
		m_bBusy = true;
		bool captured;
		if (m_Source.UsesOpenRadioAdmissions() && !m_bClosed && ReconcileCurrentOperation() == "OK")
			captured = CollectOpenRadioSlots(networkId, channelId, rawTune, maxSlots, slots, sampleMs);
		Finish();
		if (m_bClosed)
		{
			slots.Clear();
			sampleMs = -1;
			return false;
		}
		return captured;
	}

	protected bool CollectOpenRadioSlots(int networkId, int channelId, int rawTune, int maxSlots,
		array<ref RICON_GameRadioOpenSlot> slots, out int sampleMs)
	{
		sampleMs = -1;
		int nowMs = FreshNow();
		if (m_bClosed || nowMs < 0) return false;
		array<ref RICON_GameRadioOpenSlot> complete = {};
		foreach (RICON_GameRadioInventoryRecord record : m_Records)
		{
			RICON_GameRadioInventoryObservation observation = record.m_Observation;
			if (!observation.m_bPowered || observation.m_iNetworkId != networkId || !observation.m_ChannelIds.Contains(channelId)) continue;
			if (!RegistrationsCurrent(record)) return false;
			foreach (int index, int generation : record.m_Generations)
			{
				if (observation.m_ChannelIds[index] != channelId) continue;
				int tune = m_Source.ReadOpenRadioTune(observation, index);
				if (tune == 0) continue;
				if (m_bClosed || tune != rawTune || complete.Count() >= maxSlots) return false;
				int admission = m_Receivers.ReadOpenEndpointAdmission(record.m_iDeviceId, index, generation, networkId, channelId, rawTune, nowMs);
				if (admission <= 0) return false;
				RICON_GameRadioOpenSlot slot = new RICON_GameRadioOpenSlot();
				slot.m_Listener = new RICON_OpenRadioPairListener();
				slot.m_Listener.m_iPlayerId = observation.m_iPlayerId;
				slot.m_Listener.m_iPlayerGeneration = observation.m_iPlayerGeneration;
				slot.m_Listener.m_iConnectionId = observation.m_iConnectionId;
				slot.m_Listener.m_iEntityId = observation.m_iEntityId;
				slot.m_Listener.m_iDeviceId = record.m_iDeviceId;
				slot.m_Listener.m_iIndex = index;
				slot.m_Listener.m_iGeneration = generation;
				slot.m_iAdmission = admission;
				RICON_GameRadioNativeHandle handle = RICON_GameRadioNativeHandle.Cast(observation.m_Handle);
				if (handle)
				{
					slot.m_Radio = handle.Radio();
					if (!m_Receivers.MatchesRadio(record.m_iDeviceId, index, generation, slot.m_Radio)) return false;
				}
				complete.Insert(slot);
			}
		}
		int finalNowMs = FreshNow();
		if (m_bClosed || finalNowMs < nowMs) return false;
		foreach (RICON_GameRadioOpenSlot current : complete)
		{
			RICON_OpenRadioPairListener row = current.m_Listener;
			RICON_LifecycleDevice device = m_Lifecycle.QueryDevice(row.m_iDeviceId, finalNowMs);
			if (!device || device.m_eKind != RICON_EDeviceKind.CARRIED || device.m_iOwnerPlayerId != row.m_iPlayerId
				|| m_Lifecycle.GetPlayerState(row.m_iPlayerId) != RICON_EPlayerLifecycleState.BOUND_ACTIVE
				|| m_Lifecycle.GetPlayerConnectionId(row.m_iPlayerId) != row.m_iConnectionId
				|| m_Lifecycle.GetPlayerGeneration(row.m_iPlayerId) != row.m_iPlayerGeneration
				|| m_Lifecycle.GetPlayerEntityId(row.m_iPlayerId) != row.m_iEntityId
				|| m_Receivers.ReadRecordedOpenEndpointAdmission(row.m_iDeviceId, row.m_iIndex, row.m_iGeneration,
					networkId, channelId, rawTune, finalNowMs) != current.m_iAdmission) return false;
		}
		if (!SameScope() || m_bClosed) return false;
		foreach (RICON_GameRadioOpenSlot admitted : complete) slots.Insert(admitted);
		sampleMs = finalNowMs;
		return true;
	}

	protected RICON_GameRadioInventoryRecord FindOwnedRecordHeld(int playerId, RICON_GameRadioInventoryHandle handle)
	{
		RICON_GameRadioInventoryObservation query = new RICON_GameRadioInventoryObservation();
		query.m_Handle = handle;
		foreach (RICON_GameRadioInventoryRecord record : m_Records)
		{
			if (record.m_Observation.m_iPlayerId != playerId) continue;
			bool same = m_Source.SamePhysical(query, record.m_Observation);
			if (m_bClosed || !SameScope()) break;
			if (same && Current(record.m_Observation) && RegistrationsCurrent(record))
				return record;
		}
		return null;
	}

	void Close()
	{
		ref RICON_GameRadioInventory held = this;
		m_bClosed = true;
		m_bClosePending = true;
		if (!m_bBusy) Finish();
	}

	protected bool SameScope()
	{
		return m_Source && m_Lifecycle && m_Receivers && m_Lifecycle.GetSessionEpoch() == m_iEpoch
			&& m_Receivers.IsScope(m_Lifecycle) && m_Source.IsScope(m_Lifecycle, m_Receivers);
	}

	protected int FreshNow(bool teardown = false)
	{
		if (teardown)
		{
			if (!m_Source || !m_Lifecycle || m_Lifecycle.GetSessionEpoch() != m_iEpoch || !m_Source.CanConsume(m_iEpoch)) return -1;
			int cleanupNow = m_Source.ReadNowMs();
			if (!m_Source.CanConsume(m_iEpoch) || !m_Lifecycle.CanQueryAt(cleanupNow)) return -1;
			return cleanupNow;
		}
		if (!SameScope()) return -1;
		int nowMs = m_Source.ReadNowMs();
		if (!SameScope() || !m_Lifecycle.CanQueryAt(nowMs) || !m_Receivers.CanRead(m_Lifecycle, nowMs)) return -1;
		return nowMs;
	}

	protected bool Current(RICON_GameRadioInventoryObservation observation)
	{
		return !m_bClosed && SameScope() && observation && observation.IsValid()
			&& m_Source.IsCurrent(observation) && !m_bClosed && SameScope();
	}

	protected bool SameIncarnation(RICON_GameRadioInventoryObservation a, RICON_GameRadioInventoryObservation b)
	{
		if (!a || !b || a.m_iPlayerId != b.m_iPlayerId || a.m_iConnectionId != b.m_iConnectionId
			|| a.m_iPlayerGeneration != b.m_iPlayerGeneration || a.m_iEntityId != b.m_iEntityId
			|| a.m_bPowered != b.m_bPowered || a.m_iTransceiverCount != b.m_iTransceiverCount
			|| !SameControl(a, b)) return false;
		return m_Source.SamePhysical(a, b) && m_Source.SameLayout(a, b);
	}

	protected bool SameControl(RICON_GameRadioInventoryObservation a, RICON_GameRadioInventoryObservation b)
	{
		if (a.m_sPolicyIdentity != b.m_sPolicyIdentity || a.m_iPolicyRevision != b.m_iPolicyRevision
			|| a.m_iNetworkId != b.m_iNetworkId || a.m_ChannelIds.Count() != b.m_ChannelIds.Count()
			|| a.m_AllowedPairs.Count() != b.m_AllowedPairs.Count()) return false;
		for (int index = 0; index < a.m_ChannelIds.Count(); index++)
			if (a.m_ChannelIds[index] != b.m_ChannelIds[index]) return false;
		for (int pair = 0; pair < a.m_AllowedPairs.Count(); pair++)
			if (a.m_AllowedPairs[pair].m_iNetworkTypeId != b.m_AllowedPairs[pair].m_iNetworkTypeId
				|| a.m_AllowedPairs[pair].m_iChannelId != b.m_AllowedPairs[pair].m_iChannelId) return false;
		return true;
	}

	protected bool CatalogueMatches(RICON_GameRadioInventoryRecord record)
	{
		int nowMs = FreshNow();
		if (m_bClosed || nowMs < 0) return false;
		RICON_LifecycleDevice device = m_Lifecycle.QueryDevice(record.m_iDeviceId, nowMs);
		if (!device || device.m_eKind != RICON_EDeviceKind.CARRIED
			|| device.m_iOwnerPlayerId != record.m_Observation.m_iPlayerId || device.m_bPowered != record.m_Observation.m_bPowered
			|| device.m_aAllowed.Count() != record.m_Observation.m_AllowedPairs.Count() || !device.m_aAccessPlayerIds.IsEmpty()) return false;
		for (int pair = 0; pair < device.m_aAllowed.Count(); pair++)
			if (device.m_aAllowed[pair].m_iNetworkTypeId != record.m_Observation.m_AllowedPairs[pair].m_iNetworkTypeId
				|| device.m_aAllowed[pair].m_iChannelId != record.m_Observation.m_AllowedPairs[pair].m_iChannelId) return false;
		return true;
	}

	protected bool RegistrationsCurrent(RICON_GameRadioInventoryRecord record)
	{
		if (!record || record.m_Generations.Count() != record.m_Observation.m_iTransceiverCount || !SameScope()
			|| !CatalogueMatches(record)) return false;
		bool policy = record.m_Observation.m_iNetworkId > 0;
		if (policy && record.m_WatchVersions.Count() != record.m_Generations.Count()) return false;
		for (int index = 0; index < record.m_Generations.Count(); index++)
		{
			if (!m_Receivers.HasRegistration(record.m_iDeviceId, index, record.m_Generations[index])) return false;
			if (policy)
			{
				int version = m_Receivers.ReadRegistrationVersion(record.m_iDeviceId, index, record.m_Generations[index]);
				if (m_bClosed || !SameScope() || version <= 0 || version != record.m_WatchVersions[index]) return false;
			}
		}
		if (!policy) return true;
		return Current(record.m_Observation) && CatalogueMatches(record);
	}

	protected int FindObservation(array<ref RICON_GameRadioInventoryObservation> rows, RICON_GameRadioInventoryObservation expected)
	{
		for (int index = 0; index < rows.Count(); index++)
		{
			bool same = m_Source.SamePhysical(rows[index], expected);
			if (m_bClosed || !SameScope()) return -1;
			if (same) return index;
		}
		return -1;
	}

	protected string ReconcileHeld()
	{
		if (!SameScope()) return "SCOPE_LOST";
		if (FreshNow() < 0 || m_bClosed) { Close(); return "CLOCK_UNAVAILABLE"; }
		array<ref RICON_GameRadioInventoryObservation> supplied = {};
		bool complete = m_Source.Capture(supplied);
		if (m_bClosed || !SameScope()) return "CLOSED";
		if (!complete)
		{
			string reason = m_Source.CaptureFailure();
			if (m_bClosed || !SameScope()) return "CLOSED";
			if (reason.IsEmpty()) return "INCOMPLETE";
			return reason;
		}
		array<ref RICON_GameRadioInventoryObservation> snapshot = {};
		foreach (RICON_GameRadioInventoryObservation input : supplied)
		{
			if (!input || !input.IsValid()) return "INCOMPLETE";
			snapshot.Insert(input.CloneObservation());
		}
		// shortcut: match scenario radios linearly; add an identity index if measured inventory cost warrants it.
		for (int item = 0; item < snapshot.Count(); item++)
		{
			if (!Current(snapshot[item])) return "INCOMPLETE";
			for (int prior = 0; prior < item; prior++)
			{
				bool duplicate = m_Source.SamePhysical(snapshot[item], snapshot[prior]);
				if (m_bClosed || !SameScope()) return "CLOSED";
				if (duplicate) return "CONFLICT";
			}
		}
		for (int old = m_Records.Count() - 1; old >= 0; old--)
		{
			RICON_GameRadioInventoryRecord held = m_Records[old];
			int match = FindObservation(snapshot, held.m_Observation);
			if (m_bClosed || !SameScope()) return "CLOSED";
			bool same = match >= 0 && SameIncarnation(held.m_Observation, snapshot[match]);
			if (m_bClosed || !SameScope()) return "CLOSED";
			if (same && Current(held.m_Observation) && RegistrationsCurrent(held)) continue;
			if (!Retire(held)) return "CONSUME_FAILED";
			if (m_bClosed || !SameScope()) return "CLOSED";
		}
		for (int failure = m_Failures.Count() - 1; failure >= 0; failure--)
			if (FindObservation(snapshot, m_Failures[failure].m_Observation) < 0) m_Failures.RemoveOrdered(failure);
		if (m_bClosed || !SameScope()) return "CLOSED";
		string status = "OK";
		foreach (RICON_GameRadioInventoryObservation observation : snapshot)
		{
			bool heldPhysical = false;
			foreach (RICON_GameRadioInventoryRecord record : m_Records)
			{
				if (m_Source.SamePhysical(observation, record.m_Observation)) { heldPhysical = true; break; }
				if (m_bClosed || !SameScope()) return "CLOSED";
			}
			if (m_bClosed || !SameScope()) return "CLOSED";
			if (heldPhysical) continue;
			string added = AddObservation(observation);
			if (added != "OK") status = added;
			if (m_bClosed || !SameScope()) return "CLOSED";
		}
		if (m_Source.UsesOpenRadioAdmissions())
		{
			foreach (RICON_GameRadioInventoryRecord admitted : m_Records)
			{
				if (!RefreshOpenAdmissions(admitted))
				{
					if (!RevokeOpenAdmissions(admitted)) { Close(); return "CLOSED"; }
					if (status == "OK") status = "RF_ADMISSION_UNAVAILABLE";
				}
			}
		}
		return status;
	}

	protected bool RefreshOpenAdmissions(RICON_GameRadioInventoryRecord record)
	{
		RICON_GameRadioInventoryObservation observation = record.m_Observation;
		if (!observation.m_bPowered || observation.m_iNetworkId <= 0) return true;
		if (!RegistrationsCurrent(record)) return false;
		array<int> tunes = {};
		foreach (int index, int generation : record.m_Generations)
		{
			int tune = m_Source.ReadOpenRadioTune(observation, index);
			if (tune < 0) return false;
			tunes.Insert(tune);
			if (tune == 0) continue;
			int nowMs = FreshNow();
			if (m_bClosed || nowMs < 0) return false;
			int channel = observation.m_ChannelIds[index];
			if (m_Receivers.ReadOpenEndpointAdmission(record.m_iDeviceId, index, generation, observation.m_iNetworkId, channel, tune, nowMs) > 0)
				continue;
			if (!m_Receivers.Admit(record.m_iDeviceId, index, generation, observation.m_iNetworkId, channel, tune, nowMs)) return false;
		}
		if (!RegistrationsCurrent(record)) return false;
		int finalNowMs = FreshNow();
		if (m_bClosed || finalNowMs < 0) return false;
		foreach (int slot, int rawTune : tunes)
		{
			if (rawTune > 0 && m_Receivers.ReadRecordedOpenEndpointAdmission(record.m_iDeviceId, slot, record.m_Generations[slot],
				observation.m_iNetworkId, observation.m_ChannelIds[slot], rawTune, finalNowMs) <= 0) return false;
		}
		return true;
	}

	protected bool RevokeOpenAdmissions(RICON_GameRadioInventoryRecord record)
	{
		int nowMs = FreshNow();
		if (m_bClosed || nowMs < 0) return false;
		foreach (int index, int generation : record.m_Generations)
		{
			if (m_Receivers.HasRegistration(record.m_iDeviceId, index, generation)
				&& !m_Receivers.Invalidate(record.m_iDeviceId, index, generation, nowMs)) return false;
		}
		return true;
	}

	protected string AddObservation(RICON_GameRadioInventoryObservation observation)
	{
		if (!Current(observation)) return "INCOMPLETE";
		int available = m_Receivers.AvailableRegistrationSlots();
		RICON_GameRadioInventoryFailure previous;
		foreach (RICON_GameRadioInventoryFailure failure : m_Failures)
		{
			if (m_Source.SamePhysical(observation, failure.m_Observation)) { previous = failure; break; }
			if (m_bClosed || !SameScope()) return "CLOSED";
		}
		if (m_bClosed || !SameScope()) return "CLOSED";
		if (previous && previous.m_iAvailableSlots == available && SameIncarnation(previous.m_Observation, observation))
			return previous.m_sReason;
		if (m_bClosed || !SameScope()) return "CLOSED";
		if (previous) m_Failures.RemoveItem(previous);
		if (available < observation.m_iTransceiverCount)
			return RememberFailure(observation, "RESOURCE_LIMIT");
		int nowMs = FreshNow();
		if (m_bClosed || nowMs < 0) return "CLOCK_UNAVAILABLE";
		int highWater = m_Lifecycle.GetDeviceIdHighWater();
		if (highWater >= int.MAX) return RememberFailure(observation, "RESOURCE_LIMIT");
		RICON_GameRadioInventoryRecord record = new RICON_GameRadioInventoryRecord();
		record.m_Observation = observation.CloneObservation();
		RICON_LifecycleDevice device = new RICON_LifecycleDevice();
		device.m_iDeviceId = highWater + 1;
		device.m_eKind = RICON_EDeviceKind.CARRIED;
		device.m_iOwnerPlayerId = observation.m_iPlayerId;
		device.m_bPowered = observation.m_bPowered;
		foreach (RICON_LifecycleCatalogPair pair : observation.m_AllowedPairs)
			device.m_aAllowed.Insert(new RICON_LifecycleCatalogPair(pair.m_iNetworkTypeId, pair.m_iChannelId));
		RICON_LifecycleResult result = m_Lifecycle.AddDevice(nowMs, m_iEpoch, device);
		if (result && result.m_eResultCode == RICON_ELifecycleResult.OK) record.m_iDeviceId = device.m_iDeviceId;
		bool consumed = ConsumeHeld(result);
		if (!consumed || !record.m_iDeviceId || !Current(observation))
		{
			if (record.m_iDeviceId) Retire(record);
			return RememberFailure(observation, "CATALOGUE_FAILED");
		}
		for (int index = 0; index < observation.m_iTransceiverCount; index++)
		{
			if (!Current(observation)) { Retire(record); return RememberFailure(observation, "INCOMPLETE"); }
			nowMs = FreshNow();
			if (m_bClosed || nowMs < 0) { Retire(record); return RememberFailure(observation, "CLOCK_UNAVAILABLE"); }
			int generation = m_Source.Register(record.m_iDeviceId, index, observation, nowMs);
			if (generation <= 0 || !Current(observation)
				|| !m_Receivers.HasRegistration(record.m_iDeviceId, index, generation))
			{
				Retire(record);
				return RememberFailure(observation, "REGISTRATION_FAILED");
			}
			record.m_Generations.Insert(generation);
		}
		if (observation.m_iNetworkId > 0)
		{
			for (int slot = 0; slot < record.m_Generations.Count(); slot++)
			{
				int version = m_Receivers.ReadRegistrationVersion(record.m_iDeviceId, slot, record.m_Generations[slot]);
				if (m_bClosed || version <= 0 || !SameScope()) { Retire(record); return RememberFailure(observation, "REGISTRATION_FAILED"); }
				record.m_WatchVersions.Insert(version);
			}
			if (!Current(observation) || !RegistrationsCurrent(record)) { Retire(record); return RememberFailure(observation, "REGISTRATION_FAILED"); }
		}
		m_Records.Insert(record);
		return "OK";
	}

	protected string RememberFailure(RICON_GameRadioInventoryObservation observation, string reason)
	{
		if (m_bClosed || !SameScope()) return "CLOSED";
		RICON_GameRadioInventoryFailure failure = new RICON_GameRadioInventoryFailure();
		failure.m_Observation = observation.CloneObservation();
		failure.m_iAvailableSlots = m_Receivers.AvailableRegistrationSlots();
		failure.m_sReason = reason;
		m_Failures.Insert(failure);
		return reason;
	}

	protected bool ConsumeHeld(RICON_LifecycleResult result)
	{
		ref RICON_GameRadioInventorySource source = m_Source;
		if (!result || !source || !m_Lifecycle || m_Lifecycle.GetSessionEpoch() != m_iEpoch || !source.CanConsume(m_iEpoch)) return false;
		// Even a Close callback cannot discard old effects while this exact manager epoch remains current.
		return source.Consume(result, m_iEpoch);
	}

	protected bool Retire(RICON_GameRadioInventoryRecord record)
	{
		if (!record || record.m_iDeviceId <= 0) return true;
		ref RICON_GameRadioInventoryRecord held = record;
		m_Records.RemoveItem(held);
		RICON_LifecycleResult result;
		int nowMs = FreshNow(true);
		if (nowMs >= 0) result = m_Lifecycle.RemoveDevice(nowMs, m_iEpoch, held.m_iDeviceId);
		if (m_Receivers) m_Receivers.DetachDevice(held.m_iDeviceId);
		bool consumed = !result || ConsumeHeld(result);
		held.m_iDeviceId = 0;
		return consumed;
	}

	protected void Finish()
	{
		m_bBusy = false;
		if (!m_bClosePending && !m_bClosed) return;
		m_bBusy = true;
		m_bClosePending = false;
		while (!m_Records.IsEmpty()) Retire(m_Records[0]);
		m_Failures.Clear();
		m_Source = null; m_Lifecycle = null; m_Receivers = null;
		m_bBusy = false; m_bClosePending = false;
	}
}

//! Native observation snapshot. Layout and source references are held, never reconstructed from numeric guesses.
class RICON_GameRadioNativeHandle : RICON_GameRadioInventoryHandle
{
	protected BaseRadioComponent m_Radio;
	protected IEntity m_Item, m_Entity;
	protected SCR_InventoryStorageManagerComponent m_Inventory;
	protected BaseCompartmentSlot m_DriverSeat;
	protected BaseWorld m_World;
	protected ref array<BaseTransceiver> m_Transceivers = {};
	protected ref array<int> m_RawTunes = {};
	protected string m_sControlKey;
	protected bool m_bControlCaptured;
	protected bool m_bPowered, m_bReady;

	bool Capture(BaseRadioComponent radio, IEntity item, IEntity entity, SCR_InventoryStorageManagerComponent inventory,
		bool controlPolicy = false, BaseCompartmentSlot driverSeat = null)
	{
		if (m_Radio || !radio || !item || !entity || radio.GetOwner() != item
			|| item.IsDeleted() || item.GetWorld() != entity.GetWorld()) return false;
		if (driverSeat)
		{
			BaseCompartmentSlot currentSeat;
			if (RICON_GameRadioEquipment.ReadDriverRadio(entity, currentSeat) != radio || currentSeat != driverSeat) return false;
		}
		else if (!inventory || inventory.GetOwner() != entity || !inventory.Contains(item)) return false;
		int count = radio.TransceiversCount();
		if (count <= 0) return false;
		for (int index = 0; index < count; index++)
		{
			BaseTransceiver transceiver = radio.GetTransceiver(index);
			if (!transceiver || transceiver.GetRadio() != radio) return false;
			m_Transceivers.Insert(transceiver);
			if (controlPolicy)
			{
				int tune;
				if (!driverSeat || RadioTransceiver.Cast(transceiver))
				{
					tune = transceiver.GetFrequency();
					if (tune <= 0) return false;
				}
				m_RawTunes.Insert(tune);
			}
		}
		m_bControlCaptured = controlPolicy;
		if (controlPolicy) m_sControlKey = radio.GetEncryptionKey();
		m_Radio = radio; m_Item = item; m_Entity = entity; m_Inventory = inventory; m_DriverSeat = driverSeat;
		m_World = entity.GetWorld(); m_bPowered = radio.IsPowered(); m_bReady = true;
		return true;
	}

	bool Query(BaseRadioComponent radio)
	{
		if (m_Radio || !radio) return false;
		m_Radio = radio; m_Item = radio.GetOwner();
		return m_Item != null;
	}

	BaseRadioComponent Radio() { return m_Radio; }
	int TransceiverCount() { return m_Transceivers.Count(); }
	bool Powered() { return m_bPowered; }
	int RawTune(int index)
	{
		if (!m_bControlCaptured || index < 0 || index >= m_RawTunes.Count()) return 0;
		return m_RawTunes[index];
	}
	bool MatchesKey(string key) { return m_bControlCaptured && key == m_sControlKey; }
	bool MatchesDeclaration(RICON_GameRadioControlPolicy policy, int epoch, RICON_GameRadioInventoryObservation observation)
	{
		if (!m_bControlCaptured) return !policy && observation.m_iNetworkId == 0;
		if (!policy || !policy.Matches(epoch, observation.m_sPolicyIdentity, observation.m_iPolicyRevision, observation.m_iNetworkId)
			|| m_RawTunes.Count() != observation.m_ChannelIds.Count()) return false;
		for (int index = 0; index < m_RawTunes.Count(); index++)
		{
			int expected;
			if (m_RawTunes[index] > 0)
			{
				expected = policy.ReadChannel(epoch, m_RawTunes[index], m_sControlKey);
				if (expected <= 0) return false;
			}
			if (expected != observation.m_ChannelIds[index]) return false;
		}
		return true;
	}
	bool SameRadio(RICON_GameRadioNativeHandle other)
	{
		return other && m_Radio && m_Radio == other.m_Radio && m_Item == other.m_Item;
	}
	bool SameTransceivers(RICON_GameRadioNativeHandle other)
	{
		if (!SameRadio(other) || m_Transceivers.Count() != other.m_Transceivers.Count()) return false;
		for (int index = 0; index < m_Transceivers.Count(); index++)
			if (m_Transceivers[index] != other.m_Transceivers[index]) return false;
		return true;
	}

	bool IsCurrent(RICON_GameRadioInventoryObservation observation, RICON_GameControllerLifecycle bridge, BaseWorld world)
	{
		if (!m_bReady || !observation || observation.m_Handle != this || !bridge || !world || world != m_World
			|| observation.m_iTransceiverCount != m_Transceivers.Count() || observation.m_bPowered != m_bPowered
			|| !m_Item || m_Item.IsDeleted() || !m_Entity || m_Entity.IsDeleted() || !m_Radio || !GetGame()) return false;
		int connection, generation, entityId;
		if (!bridge.ReadBoundPlayer(observation.m_iPlayerId, connection, generation, entityId)
			|| connection != observation.m_iConnectionId || generation != observation.m_iPlayerGeneration
			|| entityId != observation.m_iEntityId) return false;
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players || players.GetPlayerControlledEntity(observation.m_iPlayerId) != m_Entity
			|| m_Entity.GetWorld() != world || m_Item.GetWorld() != world || m_Radio.GetOwner() != m_Item
			|| m_Radio.IsPowered() != m_bPowered
			|| m_Radio.TransceiversCount() != m_Transceivers.Count()) return false;
		for (int index = 0; index < m_Transceivers.Count(); index++)
			if (!m_Transceivers[index] || m_Radio.GetTransceiver(index) != m_Transceivers[index]
				|| m_Transceivers[index].GetRadio() != m_Radio) return false;
		if (m_bControlCaptured)
		{
			if (m_Radio.GetEncryptionKey() != m_sControlKey || m_RawTunes.Count() != m_Transceivers.Count()) return false;
			for (int tune = 0; tune < m_RawTunes.Count(); tune++)
				if (m_RawTunes[tune] > 0 && m_Transceivers[tune].GetFrequency() != m_RawTunes[tune]) return false;
		}
		if (m_DriverSeat)
		{
			BaseCompartmentSlot currentSeat;
			if (RICON_GameRadioEquipment.ReadDriverRadio(m_Entity, currentSeat) != m_Radio || currentSeat != m_DriverSeat) return false;
		}
		else if (m_Item.FindComponent(BaseRadioComponent) != m_Radio || !m_Inventory
			|| m_Entity.FindComponent(SCR_InventoryStorageManagerComponent) != m_Inventory
			|| m_Inventory.GetOwner() != m_Entity || !m_Inventory.Contains(m_Item)) return false;
		return bridge.ReadBoundPlayer(observation.m_iPlayerId, connection, generation, entityId)
			&& connection == observation.m_iConnectionId && generation == observation.m_iPlayerGeneration
			&& entityId == observation.m_iEntityId && players.GetPlayerControlledEntity(observation.m_iPlayerId) == m_Entity;
	}
}

//! An admitted slot belongs to the captured inventory, even if the same native item remains alive after a transfer.
class RICON_GameRadioOwnedRxWatch : RICON_MediaNativeRxWatch
{
	protected ref RICON_GameRadioInventoryObservation m_Observation;
	protected RICON_GameControllerLifecycle m_Bridge;
	protected BaseWorld m_World;
	protected bool m_bOwnershipLost;

	bool BindOwned(RICON_GameRadioNativeHandle handle, int index, RICON_GameRadioInventoryObservation observation,
		RICON_GameControllerLifecycle bridge, BaseWorld world)
	{
		if (!handle || !observation || !bridge || !world || !Bind(handle.Radio(), index)) return false;
		m_Observation = observation.CloneObservation();
		m_Bridge = bridge;
		m_World = world;
		return true;
	}

	override int GetVersion()
	{
		if (!m_Observation || m_bOwnershipLost) return int.MAX;
		RICON_GameRadioNativeHandle handle = RICON_GameRadioNativeHandle.Cast(m_Observation.m_Handle);
		if (!handle || !handle.IsCurrent(m_Observation, m_Bridge, m_World))
		{
			m_bOwnershipLost = true;
			return int.MAX;
		}
		return super.GetVersion();
	}

	override void Stop()
	{
		m_Observation = null;
		m_Bridge = null;
		m_World = null;
		super.Stop();
	}
}

class RICON_GameRadioNativeInventorySource : RICON_GameRadioInventorySource
{
	protected RICON_CommsManagerComponent m_Manager;
	protected RICON_LifecycleStateMachine m_Lifecycle;
	protected ref RICON_GameControllerLifecycle m_Bridge;
	protected RICON_MediaReceiverAuthority m_Receivers;
	protected BaseWorld m_World;
	protected int m_iEpoch;
	protected ref RICON_GameRadioControlPolicy m_ControlPolicy;
	protected bool m_bOpenRadioAdmissions;
	protected bool m_bIncludeDriverRadio;
	protected string m_sCaptureFailure = "INCOMPLETE";

	bool Configure(RICON_CommsManagerComponent manager, RICON_LifecycleStateMachine lifecycle,
		RICON_GameControllerLifecycle bridge, RICON_MediaReceiverAuthority receivers, RICON_GameRadioControlPolicy policy = null,
		bool openRadioAdmissions = false, bool includeDriverRadio = false)
	{
		if (m_Manager || !manager || !lifecycle || !bridge || !receivers || !GetGame() || (openRadioAdmissions && !policy)) return false;
		m_Manager = manager; m_Lifecycle = lifecycle; m_Bridge = bridge; m_Receivers = receivers;
		m_World = GetGame().GetWorld(); m_iEpoch = lifecycle.GetSessionEpoch();
		m_ControlPolicy = policy;
		m_bOpenRadioAdmissions = openRadioAdmissions;
		m_bIncludeDriverRadio = includeDriverRadio;
		return IsScope(lifecycle, receivers);
	}

	override bool IsScope(RICON_LifecycleStateMachine lifecycle, RICON_MediaReceiverAuthority receivers)
	{
		if (m_ControlPolicy && !m_ControlPolicy.Matches(m_iEpoch, m_ControlPolicy.Identity(), m_ControlPolicy.Revision(), m_ControlPolicy.NetworkId())) return false;
		RplMode mode = RplSession.Mode();
		return m_Manager && lifecycle && lifecycle == m_Lifecycle && receivers && receivers == m_Receivers
			&& m_Bridge && CanConsume(m_iEpoch)
			&& (mode == RplMode.Dedicated || mode == RplMode.Listen)
			&& m_Bridge.MatchesLifecycle(lifecycle) && receivers.IsScope(lifecycle);
	}

	override int ReadNowMs()
	{
		if (!CanConsume(m_iEpoch) || !m_Bridge) return -1;
		return m_Bridge.ReadWireAuthorityTime(m_Lifecycle);
	}

	override bool CanConsume(int preEpoch)
	{
		RICON_GameRadioManagerComponent manager = RICON_GameRadioManagerComponent.Cast(m_Manager);
		return manager && m_Lifecycle && m_World && GetGame() && preEpoch == m_iEpoch
			&& RICON_CommsManagerComponent.IsAuthority() && RICON_CommsManagerComponent.GetInstance() == manager
			&& GetGame().GetWorld() == m_World && !m_World.IsEditMode() && manager.GetOwner()
			&& manager.GetOwner().GetWorld() == m_World && m_Lifecycle.GetSessionEpoch() == m_iEpoch
			&& manager.OwnsInventoryLifecycle(m_Lifecycle, m_iEpoch);
	}

	protected bool RegisteredRemoteBindingMissing(int playerId, PlayerManager players)
	{
		if (!m_Manager.HasRegisteredPlayerController(playerId)
			|| m_Lifecycle.GetPlayerState(playerId) != RICON_EPlayerLifecycleState.NONE) return false;
		SCR_PlayerController controller = SCR_PlayerController.Cast(players.GetPlayerController(playerId));
		if (!controller || controller.GetPlayerId() != playerId) return false;
		RplIdentity owner = controller.GetRplIdentity();
		if (!owner.IsValid() || owner == RplIdentity.Local()) return false;
		RplId item = Replication.FindItemId(controller);
		return item.IsValid() && Replication.FindOwner(item) == owner;
	}

	override bool Capture(array<ref RICON_GameRadioInventoryObservation> observations)
	{
		m_sCaptureFailure = "INCOMPLETE";
		if (!observations || !IsScope(m_Lifecycle, m_Receivers)) return false;
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players) return false;
		array<int> playerIds = {};
		array<int> rawTunes = {};
		array<string> exactKeys = {};
		if (players.GetPlayers(playerIds) != playerIds.Count() || !IsScope(m_Lifecycle, m_Receivers)) return false;
		playerIds.Sort();
		int prior;
		foreach (int playerId : playerIds)
		{
			if (playerId <= prior) return false;
			prior = playerId;
			int connection, generation, entityId;
			bool bound = m_Bridge.ReadBoundPlayer(playerId, connection, generation, entityId);
			if (!IsScope(m_Lifecycle, m_Receivers)) return false;
			if (!bound)
			{
				bool missing = RegisteredRemoteBindingMissing(playerId, players);
				if (!IsScope(m_Lifecycle, m_Receivers) || missing) return false;
				continue;
			}
			IEntity entity = players.GetPlayerControlledEntity(playerId);
			if (!entity || entity.IsDeleted() || entity.GetWorld() != m_World) return false;
			SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(entity.FindComponent(SCR_InventoryStorageManagerComponent));
			array<IEntity> items = {};
			if (inventory && (inventory.GetItems(items, EStoragePurpose.PURPOSE_ANY) != items.Count() || !IsScope(m_Lifecycle, m_Receivers))) return false;
			BaseCompartmentSlot driverSeat;
			IEntity vehicle;
			if (m_bIncludeDriverRadio)
			{
				BaseRadioComponent installed = RICON_GameRadioEquipment.ReadDriverRadio(entity, driverSeat);
				if (installed)
				{
					vehicle = installed.GetOwner();
					if (!items.Contains(vehicle)) items.Insert(vehicle);
				}
			}
			foreach (IEntity item : items)
			{
				if (!item || item.IsDeleted()) return false;
				BaseRadioComponent radio = BaseRadioComponent.Cast(item.FindComponent(BaseRadioComponent));
				if (item == vehicle) radio = RICON_GameRadioEquipment.ReadNativeRadio(item);
				if (!radio) continue;
				if (m_bIncludeDriverRadio && item != vehicle
					&& RICON_GameRadioEquipment.ReadType(radio) == RICON_EGameRadioEquipment.VEHICLE) continue;
				RICON_GameRadioNativeHandle handle = new RICON_GameRadioNativeHandle();
				BaseCompartmentSlot mountedSeat;
				if (item == vehicle) mountedSeat = driverSeat;
				if (!handle.Capture(radio, item, entity, inventory, m_ControlPolicy != null, mountedSeat)) return false;
				RICON_GameRadioInventoryObservation observation = new RICON_GameRadioInventoryObservation();
				observation.m_Handle = handle; observation.m_iPlayerId = playerId; observation.m_iConnectionId = connection;
				observation.m_iPlayerGeneration = generation; observation.m_iEntityId = entityId;
				observation.m_iTransceiverCount = handle.TransceiverCount(); observation.m_bPowered = handle.Powered();
				if (!handle.IsCurrent(observation, m_Bridge, m_World) || !IsScope(m_Lifecycle, m_Receivers)) return false;
				if (m_ControlPolicy)
				{
					string exactKey = radio.GetEncryptionKey();
					if (!handle.MatchesKey(exactKey) || !IsScope(m_Lifecycle, m_Receivers)) return false;
					for (int slot = 0; slot < observation.m_iTransceiverCount; slot++)
					{
						int tune = handle.RawTune(slot);
						if (!tune) continue;
						rawTunes.Insert(tune);
						exactKeys.Insert(exactKey);
					}
				}
				observations.Insert(observation);
			}
		}
		if (m_ControlPolicy)
		{
			array<int> channels = {};
			string resolved = m_ControlPolicy.ResolveBatch(m_iEpoch, rawTunes, exactKeys, channels);
			if (resolved != "OK") { m_sCaptureFailure = resolved; return false; }
			int offset;
			foreach (RICON_GameRadioInventoryObservation declared : observations)
			{
				RICON_GameRadioNativeHandle handle = RICON_GameRadioNativeHandle.Cast(declared.m_Handle);
				if (!handle) return false;
				array<int> perIndex = {};
				for (int index = 0; index < declared.m_iTransceiverCount; index++)
				{
					if (!handle.RawTune(index)) { perIndex.Insert(0); continue; }
					if (offset >= channels.Count()) return false;
					perIndex.Insert(channels[offset]); offset++;
				}
				if (!declared.DeclareControl(m_ControlPolicy, perIndex)) return false;
			}
			if (offset != channels.Count()) return false;
		}
		foreach (RICON_GameRadioInventoryObservation current : observations)
			if (!IsCurrent(current)) return false;
		return IsScope(m_Lifecycle, m_Receivers);
	}

	override string CaptureFailure() { return m_sCaptureFailure; }

	override bool IsCurrent(RICON_GameRadioInventoryObservation observation)
	{
		if (!IsScope(m_Lifecycle, m_Receivers) || !observation || !observation.IsValid()) return false;
		RICON_GameRadioNativeHandle handle = RICON_GameRadioNativeHandle.Cast(observation.m_Handle);
		return handle && handle.MatchesDeclaration(m_ControlPolicy, m_iEpoch, observation)
			&& handle.IsCurrent(observation, m_Bridge, m_World) && IsScope(m_Lifecycle, m_Receivers);
	}

	override bool SamePhysical(RICON_GameRadioInventoryObservation a, RICON_GameRadioInventoryObservation b)
	{
		if (!a || !b) return false;
		RICON_GameRadioNativeHandle first = RICON_GameRadioNativeHandle.Cast(a.m_Handle);
		RICON_GameRadioNativeHandle second = RICON_GameRadioNativeHandle.Cast(b.m_Handle);
		return first && first.SameRadio(second);
	}
	override bool SameLayout(RICON_GameRadioInventoryObservation a, RICON_GameRadioInventoryObservation b)
	{
		if (!a || !b) return false;
		RICON_GameRadioNativeHandle first = RICON_GameRadioNativeHandle.Cast(a.m_Handle);
		RICON_GameRadioNativeHandle second = RICON_GameRadioNativeHandle.Cast(b.m_Handle);
		return first && first.SameTransceivers(second);
	}
	override int Register(int deviceId, int index, RICON_GameRadioInventoryObservation observation, int nowMs)
	{
		if (!IsCurrent(observation)) return 0;
		RICON_GameRadioNativeHandle handle = RICON_GameRadioNativeHandle.Cast(observation.m_Handle);
		int freshNow = ReadNowMs();
		if (freshNow < nowMs || !IsScope(m_Lifecycle, m_Receivers) || !m_Receivers.CanRead(m_Lifecycle, freshNow)) return 0;
		if (!m_bOpenRadioAdmissions) return m_Receivers.RegisterRadio(deviceId, index, handle.Radio(), freshNow);
		RICON_GameRadioOwnedRxWatch watch = new RICON_GameRadioOwnedRxWatch();
		if (!watch.BindOwned(handle, index, observation, m_Bridge, m_World)) return 0;
		int generation = m_Receivers.Register(deviceId, index, watch, freshNow);
		if (generation <= 0) watch.Stop();
		return generation;
	}

	override bool UsesOpenRadioAdmissions() { return m_bOpenRadioAdmissions; }

	override int ReadOpenRadioTune(RICON_GameRadioInventoryObservation observation, int index)
	{
		if (!m_bOpenRadioAdmissions) return 0;
		if (!observation || !IsCurrent(observation)) return -1;
		if (!observation.m_bPowered) return 0;
		RICON_GameRadioNativeHandle handle = RICON_GameRadioNativeHandle.Cast(observation.m_Handle);
		if (!handle) return -1;
		if (!handle.MatchesKey("")) return 0;
		int tune = handle.RawTune(index);
		if (tune == 0 && index >= 0 && index < observation.m_ChannelIds.Count() && observation.m_ChannelIds[index] == 0) return 0;
		if (tune <= 0) return -1;
		return tune;
	}
	override bool Consume(RICON_LifecycleResult result, int preEpoch)
	{
		if (!result || !CanConsume(preEpoch)) return false;
		RICON_TemporalCaptureResult consumed = m_Manager.ConsumeTemporalLifecycleResult(result, preEpoch);
		return consumed && consumed.m_sCode == "OK" && CanConsume(preEpoch);
	}
}
