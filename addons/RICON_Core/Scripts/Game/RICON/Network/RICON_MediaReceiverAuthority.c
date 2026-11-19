//! A change invalidates the current admission even if the observed value returns to its original state.
class RICON_MediaRxWatch
{
	protected int m_iVersion = 1;
	protected bool m_bClaimed;

	int GetVersion() { return m_iVersion; }
	bool Claim()
	{
		if (m_bClaimed) return false;
		m_bClaimed = true;
		return true;
	}

	void Invalidate()
	{
		if (m_iVersion < int.MAX) m_iVersion++;
	}

	bool ReadState(out int frequency, out string key)
	{
		frequency = 0;
		key = "";
		return false;
	}

	void Stop() { Invalidate(); }
}

class RICON_MediaNativeRxWatch : RICON_MediaRxWatch
{
	protected IEntity m_Owner;
	protected BaseRadioComponent m_Radio;
	protected BaseTransceiver m_Transceiver;
	protected int m_iIndex;
	protected bool m_bBound;
	protected ref ScriptInvoker<BaseTransceiver, int, int> m_FrequencyEvent;
	protected ref ScriptInvoker<bool> m_PowerEvent;
	protected ref ScriptInvoker<string> m_KeyEvent;

	bool Bind(BaseRadioComponent radio, int index)
	{
		if (!RICON_CommsManagerComponent.IsAuthority() || m_bBound || !radio || index < 0 || index >= radio.TransceiversCount()) return false;
		IEntity owner = radio.GetOwner();
		BaseTransceiver transceiver = radio.GetTransceiver(index);
		if (!owner || !transceiver || transceiver.GetRadio() != radio) return false;
		if (!radio.m_OnTansceiverFrequencyChangedInvoker || !radio.m_OnPowerChanged || !radio.m_OnEncryptionKeyChanged) return false;
		m_Owner = owner;
		m_Radio = radio;
		m_Transceiver = transceiver;
		m_bBound = true;
		m_iIndex = index;
		m_FrequencyEvent = radio.m_OnTansceiverFrequencyChangedInvoker;
		m_PowerEvent = radio.m_OnPowerChanged;
		m_KeyEvent = radio.m_OnEncryptionKeyChanged;
		m_FrequencyEvent.Insert(OnFrequency);
		m_PowerEvent.Insert(OnPower);
		m_KeyEvent.Insert(OnKey);
		return true;
	}

	BaseTransceiver GetTransceiver() { return m_Transceiver; }
	int GetIndex() { return m_iIndex; }

	protected void OnFrequency(BaseTransceiver transceiver, int oldFrequency, int newFrequency)
	{
		if (transceiver == m_Transceiver && oldFrequency != newFrequency) Invalidate();
	}

	protected void OnPower(bool powered) { Invalidate(); }
	protected void OnKey(string key) { Invalidate(); }

	override bool ReadState(out int frequency, out string key)
	{
		frequency = 0;
		key = "";
		if (!RICON_CommsManagerComponent.IsAuthority() || !m_Owner || !m_Radio || !m_Transceiver) return false;
		if (m_Radio.GetOwner() != m_Owner || m_iIndex >= m_Radio.TransceiversCount()
			|| m_Radio.GetTransceiver(m_iIndex) != m_Transceiver || m_Transceiver.GetRadio() != m_Radio) return false;
		if (m_Radio.m_OnTansceiverFrequencyChangedInvoker != m_FrequencyEvent || m_Radio.m_OnPowerChanged != m_PowerEvent
			|| m_Radio.m_OnEncryptionKeyChanged != m_KeyEvent || !m_Radio.IsPowered()) return false;
		frequency = m_Transceiver.GetFrequency();
		key = m_Radio.GetEncryptionKey();
		return true;
	}

	override void Stop()
	{
		if (m_FrequencyEvent) m_FrequencyEvent.Remove(OnFrequency);
		if (m_PowerEvent) m_PowerEvent.Remove(OnPower);
		if (m_KeyEvent) m_KeyEvent.Remove(OnKey);
		m_FrequencyEvent = null;
		m_PowerEvent = null;
		m_KeyEvent = null;
		m_Transceiver = null;
		m_Radio = null;
		m_Owner = null;
		super.Stop();
	}
}

class RICON_MediaRxSlot
{
	ref RICON_MediaRxWatch m_Watch;
	int m_iDeviceId, m_iIndex, m_iWriteRevision;
	int m_iGeneration, m_iAdmission, m_iWatchVersion, m_iNetworkId, m_iChannelId, m_iFrequency;
	string m_sKey;

	bool ReadMatches(int frequency, string key, int version)
	{
		int actualFrequency;
		string actualKey;
		if (version >= int.MAX || m_Watch.GetVersion() != version || !m_Watch.ReadState(actualFrequency, actualKey)) return false;
		return m_Watch.GetVersion() == version && actualFrequency == frequency && actualKey == key;
	}
}

//! Session-owned physical RX registry. Product rights remain a separate condition of the managed roster.
class RICON_MediaReceiverAuthority
{
	protected RICON_LifecycleStateMachine m_Lifecycle;
	protected ref map<string, ref RICON_MediaRxSlot> m_Slots = new map<string, ref RICON_MediaRxSlot>();
	protected int m_iEpoch, m_iMaxSlots, m_iSerial;
	protected int m_iLastWriteMs = -1;
	protected bool m_bClosed;

	bool Configure(RICON_LifecycleStateMachine lifecycle, int maxSlots)
	{
		if (m_bClosed || m_iEpoch != 0 || !lifecycle || maxSlots <= 0) return false;
		int epoch = lifecycle.GetSessionEpoch();
		if (epoch <= 0 || epoch >= int.MAX) return false;
		m_Lifecycle = lifecycle;
		m_iEpoch = epoch;
		m_iMaxSlots = maxSlots;
		return true;
	}

	bool IsScope(RICON_LifecycleStateMachine lifecycle)
	{
		return !m_bClosed && lifecycle && lifecycle == m_Lifecycle && lifecycle.GetSessionEpoch() == m_iEpoch;
	}

	bool CanRead(RICON_LifecycleStateMachine lifecycle, int nowMs)
	{
		return IsScope(lifecycle) && nowMs >= m_iLastWriteMs && lifecycle.CanQueryAt(nowMs);
	}

	int LastWriteTimeMs() { return m_iLastWriteMs; }
	//! Resource availability only; current catalogue/admission checks still grant permission.
	int AvailableRegistrationSlots()
	{
		if (!IsScope(m_Lifecycle)) return -1;
		return m_iMaxSlots - m_Slots.Count();
	}

	//! Resource-version witness only; it does not grant catalogue or media permission.
	int ReadRegistrationVersion(int deviceId, int index, int generation)
	{
		if (!IsScope(m_Lifecycle)) return -1;
		string key = SlotKey(deviceId, index);
		ref RICON_MediaRxSlot held = m_Slots.Get(key);
		if (!held || held.m_iGeneration != generation || !held.m_Watch) return -1;
		ref RICON_MediaRxWatch watch = held.m_Watch;
		int version = watch.GetVersion();
		if (!IsScope(m_Lifecycle) || m_Slots.Get(key) != held || held.m_iGeneration != generation
			|| held.m_Watch != watch || version <= 0 || version >= int.MAX) return -1;
		return version;
	}

	protected static string SlotKey(int deviceId, int index) { return string.Format("%1:%2", deviceId, index); }

	protected bool CurrentSlot(RICON_MediaRxSlot slot)
	{
		return slot && slot.m_Watch && m_Slots.Get(SlotKey(slot.m_iDeviceId, slot.m_iIndex)) == slot;
	}

	protected bool CatalogueCurrent(RICON_MediaRxSlot slot, int networkId, int channelId, int nowMs)
	{
		if (!CanRead(m_Lifecycle, nowMs) || !CurrentSlot(slot)) return false;
		RICON_LifecycleDevice device = m_Lifecycle.QueryDevice(slot.m_iDeviceId, nowMs);
		return device && device.m_bPowered && device.Allows(networkId, channelId);
	}

	protected bool SameAdmission(RICON_MediaRxSlot slot, int generation, int admission, int revision, int watchVersion,
		int networkId, int channelId, int frequency, string key)
	{
		return CurrentSlot(slot) && slot.m_iGeneration == generation && slot.m_iAdmission == admission
			&& slot.m_iWriteRevision == revision && slot.m_iWatchVersion == watchVersion && slot.m_iNetworkId == networkId
			&& slot.m_iChannelId == channelId && slot.m_iFrequency == frequency && slot.m_sKey == key;
	}

	protected bool CurrentAdmission(RICON_MediaRxSlot slot, int generation, int admission, int revision, int watchVersion,
		int networkId, int channelId, int frequency, string key, int nowMs)
	{
		return admission > 0 && revision > 0 && revision < int.MAX && watchVersion > 0 && watchVersion < int.MAX
			&& SameAdmission(slot, generation, admission, revision, watchVersion, networkId, channelId, frequency, key)
			&& CatalogueCurrent(slot, networkId, channelId, nowMs) && slot.m_Watch.GetVersion() == watchVersion;
	}

	protected void AdvanceWriteTime(int nowMs)
	{
		if (nowMs > m_iLastWriteMs) m_iLastWriteMs = nowMs;
	}

	protected bool BeginSlotWrite(RICON_MediaRxSlot slot, int nowMs)
	{
		AdvanceWriteTime(nowMs);
		if (slot.m_iWriteRevision <= 0 || slot.m_iWriteRevision >= int.MAX - 1)
		{
			slot.m_iWriteRevision = int.MAX;
			slot.m_iAdmission = 0;
			return false;
		}
		slot.m_iWriteRevision++;
		return true;
	}

	protected void RetireCaptured(RICON_MediaRxSlot slot, int generation, int admission, int revision, int watchVersion,
		int networkId, int channelId, int frequency, string key, int nowMs)
	{
		if (admission <= 0 || !SameAdmission(slot, generation, admission, revision, watchVersion, networkId, channelId, frequency, key)) return;
		BeginSlotWrite(slot, nowMs);
		slot.m_iAdmission = 0;
	}

	protected int ReadCurrentAdmission(RICON_MediaRxSlot slot, int networkId, int channelId, int frequency, string key,
		int nowMs, bool retireMismatch = true)
	{
		if (!slot || slot.m_iAdmission <= 0 || slot.m_iNetworkId != networkId || slot.m_iChannelId != channelId
			|| slot.m_iFrequency != frequency || slot.m_sKey != key) return 0;
		int generation = slot.m_iGeneration, admission = slot.m_iAdmission, revision = slot.m_iWriteRevision;
		int version = slot.m_iWatchVersion;
		bool current = CurrentAdmission(slot, generation, admission, revision, version, networkId, channelId, frequency, key, nowMs);
		if (current && slot.ReadMatches(frequency, key, version)
			&& CurrentAdmission(slot, generation, admission, revision, version, networkId, channelId, frequency, key, nowMs)) return admission;
		// A different slot may have advanced the registry clock during readback.
		if (!CanRead(m_Lifecycle, nowMs)) return 0;
		if (retireMismatch) RetireCaptured(slot, generation, admission, revision, version, networkId, channelId, frequency, key, nowMs);
		return 0;
	}

	//! Resource presence only; current permission must be queried through an admission reader.
	bool HasRegistration(int deviceId, int index, int generation)
	{
		if (!IsScope(m_Lifecycle)) return false;
		RICON_MediaRxSlot slot = m_Slots.Get(SlotKey(deviceId, index));
		return slot && slot.m_iGeneration == generation;
	}

	bool MatchesRadio(int deviceId, int index, int generation, BaseRadioComponent radio)
	{
		if (!radio || index < 0 || index >= radio.TransceiversCount() || !IsScope(m_Lifecycle)) return false;
		RICON_MediaRxSlot slot = m_Slots.Get(SlotKey(deviceId, index));
		if (!slot || slot.m_iGeneration != generation) return false;
		RICON_MediaNativeRxWatch watch = RICON_MediaNativeRxWatch.Cast(slot.m_Watch);
		BaseTransceiver transceiver = radio.GetTransceiver(index);
		return watch && transceiver && watch.GetTransceiver() == transceiver && transceiver.GetRadio() == radio;
	}

	//! Read-only witness for a geometry seal that has already matched the exact radio.
	int ReadFixedOpenAdmission(int deviceId, int index, int generation, int expectedFrequency)
	{
		if (!IsScope(m_Lifecycle)) return 0;
		int nowMs = m_Lifecycle.LastObservedTimeMs();
		if (m_iLastWriteMs > nowMs) nowMs = m_iLastWriteMs;
		if (!CanRead(m_Lifecycle, nowMs)) return 0;
		ref RICON_MediaRxSlot slot = m_Slots.Get(SlotKey(deviceId, index));
		if (!slot || slot.m_iGeneration != generation) return 0;
		return ReadCurrentAdmission(slot, slot.m_iNetworkId, slot.m_iChannelId, expectedFrequency, "", nowMs, false);
	}

	//! An infrastructure endpoint does not carry an originating player's stream binding.
	int ReadOpenEndpointAdmission(int deviceId, int index, int generation, int networkId, int channelId, int frequency, int nowMs)
	{
		if (!CanRead(m_Lifecycle, nowMs)) return 0;
		ref RICON_MediaRxSlot slot = m_Slots.Get(SlotKey(deviceId, index));
		if (!slot || slot.m_iGeneration != generation) return 0;
		return ReadCurrentAdmission(slot, networkId, channelId, frequency, "", nowMs);
	}

	//! Check the recorded admit after an inventory batch without a fresh ReadState callback.
	int ReadRecordedOpenEndpointAdmission(int deviceId, int index, int generation, int networkId, int channelId, int frequency, int nowMs)
	{
		ref RICON_MediaRxSlot slot = m_Slots.Get(SlotKey(deviceId, index));
		if (!slot || slot.m_iGeneration != generation) return 0;
		int admission = slot.m_iAdmission;
		int revision = slot.m_iWriteRevision, watchVersion = slot.m_iWatchVersion;
		if (!CurrentAdmission(slot, generation, admission, revision, watchVersion,
			networkId, channelId, frequency, "", nowMs)) return 0;
		if (!CatalogueCurrent(slot, networkId, channelId, nowMs)
			|| !SameAdmission(slot, generation, admission, revision, watchVersion, networkId, channelId, frequency, "")) return 0;
		return admission;
	}

	protected bool WatchRegistered(RICON_MediaRxWatch watch)
	{
		for (int i = 0; i < m_Slots.Count(); i++)
			if (m_Slots.GetElement(i).m_Watch == watch) return true;
		return false;
	}

	protected bool CanRegister(int deviceId, int index, RICON_MediaRxWatch watch, int nowMs)
	{
		if (!CanRead(m_Lifecycle, nowMs) || !watch || deviceId <= 0 || index < 0 || m_iSerial >= int.MAX - 1) return false;
		if (m_Slots.Contains(SlotKey(deviceId, index)) || m_Slots.Count() >= m_iMaxSlots || !m_Lifecycle.QueryDevice(deviceId, nowMs)) return false;
		RICON_MediaNativeRxWatch nativeWatch = RICON_MediaNativeRxWatch.Cast(watch);
		if (nativeWatch && (!nativeWatch.GetTransceiver() || nativeWatch.GetIndex() != index)) return false;
		for (int i = 0; i < m_Slots.Count(); i++)
		{
			RICON_MediaRxWatch existing = m_Slots.GetElement(i).m_Watch;
			if (existing == watch) return false;
			RICON_MediaNativeRxWatch existingNative = RICON_MediaNativeRxWatch.Cast(existing);
			if (nativeWatch && existingNative && nativeWatch.GetTransceiver() == existingNative.GetTransceiver()) return false;
		}
		return true;
	}

	int Register(int deviceId, int index, RICON_MediaRxWatch watch, int nowMs)
	{
		if (!CanRegister(deviceId, index, watch, nowMs) || !watch.Claim()) return 0;
		if (!CanRegister(deviceId, index, watch, nowMs))
		{
			if (!WatchRegistered(watch)) watch.Stop();
			return 0;
		}
		RICON_MediaRxSlot slot = new RICON_MediaRxSlot();
		slot.m_Watch = watch;
		slot.m_iDeviceId = deviceId;
		slot.m_iIndex = index;
		slot.m_iWriteRevision = 1;
		m_iSerial++;
		slot.m_iGeneration = m_iSerial;
		m_Slots.Set(SlotKey(deviceId, index), slot);
		AdvanceWriteTime(nowMs);
		return slot.m_iGeneration;
	}

	int RegisterRadio(int deviceId, int index, BaseRadioComponent radio, int nowMs)
	{
		RICON_MediaNativeRxWatch watch = new RICON_MediaNativeRxWatch();
		if (!watch.Bind(radio, index)) return 0;
		int generation = Register(deviceId, index, watch, nowMs);
		if (!generation) watch.Stop();
		return generation;
	}

	//! A controlled writer invalidates before calling a native setter, and re-admits only after readback.
	bool Invalidate(int deviceId, int index, int generation, int nowMs)
	{
		if (!CanRead(m_Lifecycle, nowMs)) return false;
		RICON_MediaRxSlot slot = m_Slots.Get(SlotKey(deviceId, index));
		if (!slot || slot.m_iGeneration != generation) return false;
		if (!BeginSlotWrite(slot, nowMs)) return false;
		slot.m_iAdmission = 0;
		return true;
	}

	bool Admit(int deviceId, int index, int generation, int networkId, int channelId, int expectedFrequency, int nowMs,
		string expectedKey = "")
	{
		if (!CanRead(m_Lifecycle, nowMs)) return false;
		ref RICON_MediaRxSlot slot = m_Slots.Get(SlotKey(deviceId, index));
		if (!slot || slot.m_iGeneration != generation || !BeginSlotWrite(slot, nowMs)) return false;
		int admission = slot.m_iAdmission, revision = slot.m_iWriteRevision, priorVersion = slot.m_iWatchVersion;
		int priorNetwork = slot.m_iNetworkId, priorChannel = slot.m_iChannelId, priorFrequency = slot.m_iFrequency;
		string priorKey = slot.m_sKey;
		int version = slot.m_Watch.GetVersion();
		bool matches = CatalogueCurrent(slot, networkId, channelId, nowMs) && expectedFrequency > 0
			&& slot.ReadMatches(expectedFrequency, expectedKey, version);
		if (!matches || !SameAdmission(slot, generation, admission, revision, priorVersion, priorNetwork, priorChannel, priorFrequency, priorKey)
			|| !CatalogueCurrent(slot, networkId, channelId, nowMs) || slot.m_Watch.GetVersion() != version)
		{
			RetireCaptured(slot, generation, admission, revision, priorVersion, priorNetwork, priorChannel, priorFrequency, priorKey, nowMs);
			return false;
		}
		if (admission && priorVersion == version && priorNetwork == networkId && priorChannel == channelId
			&& priorFrequency == expectedFrequency && priorKey == expectedKey) return true;
		slot.m_iAdmission = 0;
		if (m_iSerial >= int.MAX - 1) return false;
		m_iSerial++;
		slot.m_iAdmission = m_iSerial;
		slot.m_iWatchVersion = version;
		slot.m_iNetworkId = networkId;
		slot.m_iChannelId = channelId;
		slot.m_iFrequency = expectedFrequency;
		slot.m_sKey = expectedKey;
		return true;
	}

	int ReadAdmission(RICON_MediaEgressRoute route, int nowMs)
	{
		if (!route || !route.m_Source || route.m_Source.m_iSessionEpoch != m_iEpoch || !CanRead(m_Lifecycle, nowMs)) return 0;
		ref RICON_MediaRxSlot slot = m_Slots.Get(SlotKey(route.m_iRxDeviceId, route.m_iRxTransceiverIndex));
		if (!slot || slot.m_iGeneration != route.m_iRxGeneration) return 0;
		return ReadSlotAdmission(slot, route.m_Source, nowMs);
	}

	//! Fence an OPEN radio admission after readbacks that may have revoked another slot.
	bool MatchesRecordedOpenAdmission(RICON_MediaStreamBinding source, int deviceId, int index, int generation, int admission, int nowMs)
	{
		if (!source || source.m_iSessionEpoch != m_iEpoch || !CanRead(m_Lifecycle, nowMs)) return false;
		RICON_MediaRxSlot slot = m_Slots.Get(SlotKey(deviceId, index));
		// The TX binding carries the unique admission number; a receiver also carries its generation.
		if (!slot || (generation > 0 && slot.m_iGeneration != generation)) return false;
		return CurrentAdmission(slot, slot.m_iGeneration, admission, slot.m_iWriteRevision, slot.m_iWatchVersion,
			source.m_iNetworkTypeId, source.m_iChannelId, slot.m_iFrequency, "", nowMs);
	}

	protected int ReadSlotAdmission(RICON_MediaRxSlot slot, RICON_MediaStreamBinding source, int nowMs)
	{
		if (!source || source.m_iSessionEpoch != m_iEpoch || !slot) return 0;
		return ReadCurrentAdmission(slot, source.m_iNetworkTypeId, source.m_iChannelId, slot.m_iFrequency, slot.m_sKey, nowMs);
	}

	//! Open RADIO uses current, unkeyed physical admissions for both ends. No key leaves this registry.
	int ReadOpenRadioSource(RICON_MediaStreamBinding source, int nowMs)
	{
		if (!source || source.m_iSessionEpoch != m_iEpoch || !CanRead(m_Lifecycle, nowMs)) return 0;
		ref RICON_MediaRxSlot slot = m_Slots.Get(SlotKey(source.m_iDeviceId, source.m_iTxTransceiverIndex));
		if (!slot || !slot.m_sKey.IsEmpty()) return 0;
		return ReadSlotAdmission(slot, source, nowMs);
	}

	int ReadOpenRadioReceiver(RICON_MediaStreamBinding source, int rxDeviceId, int rxIndex, int txAdmission, int nowMs)
	{
		if (!source || txAdmission <= 0) return 0;
		int txDeviceId = source.m_iDeviceId, txIndex = source.m_iTxTransceiverIndex;
		int networkId = source.m_iNetworkTypeId, channelId = source.m_iChannelId;
		if (ReadOpenRadioSource(source, nowMs) != txAdmission) return 0;
		ref RICON_MediaRxSlot tx = m_Slots.Get(SlotKey(txDeviceId, txIndex));
		ref RICON_MediaRxSlot rx = m_Slots.Get(SlotKey(rxDeviceId, rxIndex));
		if (!tx || !rx || !tx.m_sKey.IsEmpty() || !rx.m_sKey.IsEmpty() || rx.m_iFrequency != tx.m_iFrequency) return 0;
		int txGeneration = tx.m_iGeneration, txRevision = tx.m_iWriteRevision, txVersion = tx.m_iWatchVersion;
		int frequency = tx.m_iFrequency;
		int rxGeneration = rx.m_iGeneration, rxAdmission = rx.m_iAdmission, rxRevision = rx.m_iWriteRevision, rxVersion = rx.m_iWatchVersion;
		if (ReadCurrentAdmission(rx, networkId, channelId, frequency, "", nowMs) != rxAdmission || rxAdmission <= 0) return 0;
		// RX readback can revoke TX; the final check must not invoke either ReadState again.
		if (!CurrentAdmission(tx, txGeneration, txAdmission, txRevision, txVersion, networkId, channelId, frequency, "", nowMs)
			|| !CurrentAdmission(rx, rxGeneration, rxAdmission, rxRevision, rxVersion, networkId, channelId, frequency, "", nowMs)) return 0;
		if (source.m_iSessionEpoch != m_iEpoch || source.m_iDeviceId != txDeviceId || source.m_iTxTransceiverIndex != txIndex
			|| source.m_iNetworkTypeId != networkId || source.m_iChannelId != channelId) return 0;
		return rxGeneration;
	}

	bool Unregister(int deviceId, int index, int generation, int nowMs)
	{
		if (!CanRead(m_Lifecycle, nowMs)) return false;
		if (!Detach(deviceId, index, generation)) return false;
		AdvanceWriteTime(nowMs);
		return true;
	}

	//! Teardown must remove the exact old registration even after clock or epoch loss.
	bool Detach(int deviceId, int index, int generation)
	{
		string key = SlotKey(deviceId, index);
		RICON_MediaRxSlot slot = m_Slots.Get(key);
		if (!slot || slot.m_iGeneration != generation) return false;
		m_Slots.Remove(key);
		slot.m_Watch.Stop();
		return true;
	}

	//! Remove only the exact registrations captured before any Stop callback runs.
	int DetachDevice(int deviceId)
	{
		array<int> indices = {};
		for (int i = 0; i < m_Slots.Count(); i++)
		{
			RICON_MediaRxSlot slot = m_Slots.GetElement(i);
			if (slot.m_iDeviceId == deviceId) indices.Insert(slot.m_iIndex);
		}
		indices.Sort();
		array<int> generations = {};
		foreach (int index : indices) generations.Insert(m_Slots.Get(SlotKey(deviceId, index)).m_iGeneration);
		int detached;
		for (int item = 0; item < indices.Count(); item++)
			if (Detach(deviceId, indices[item], generations[item])) detached++;
		return detached;
	}

	void Close()
	{
		if (m_bClosed) return;
		m_bClosed = true;
		array<ref RICON_MediaRxSlot> retired = {};
		for (int i = 0; i < m_Slots.Count(); i++) retired.Insert(m_Slots.GetElement(i));
		m_Slots.Clear();
		m_Lifecycle = null;
		foreach (RICON_MediaRxSlot slot : retired) slot.m_Watch.Stop();
	}
}
