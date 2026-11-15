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

	protected static string SlotKey(int deviceId, int index) { return string.Format("%1:%2", deviceId, index); }

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
		RICON_MediaRxSlot slot = m_Slots.Get(SlotKey(deviceId, index));
		if (!slot || slot.m_iGeneration != generation || slot.m_iAdmission <= 0) return 0;
		if (slot.m_iFrequency != expectedFrequency || !slot.m_sKey.IsEmpty()) return 0;
		if (!slot.ReadMatches(expectedFrequency, "", slot.m_iWatchVersion)) return 0;
		return slot.m_iAdmission;
	}

	//! An infrastructure endpoint does not carry an originating player's stream binding.
	int ReadOpenEndpointAdmission(int deviceId, int index, int generation, int networkId, int channelId, int frequency, int nowMs)
	{
		if (!CanRead(m_Lifecycle, nowMs)) return 0;
		RICON_MediaRxSlot slot = m_Slots.Get(SlotKey(deviceId, index));
		if (!slot || slot.m_iGeneration != generation || slot.m_iNetworkId != networkId || slot.m_iChannelId != channelId) return 0;
		if (slot.m_iFrequency != frequency || !slot.m_sKey.IsEmpty() || slot.m_iAdmission <= 0) return 0;
		if (slot.ReadMatches(frequency, "", slot.m_iWatchVersion)) return slot.m_iAdmission;
		slot.m_iAdmission = 0;
		m_iLastWriteMs = nowMs;
		return 0;
	}

	int Register(int deviceId, int index, RICON_MediaRxWatch watch, int nowMs)
	{
		if (!CanRead(m_Lifecycle, nowMs) || !watch || deviceId <= 0 || index < 0 || m_iSerial >= int.MAX - 1) return 0;
		string key = SlotKey(deviceId, index);
		if (m_Slots.Contains(key) || m_Slots.Count() >= m_iMaxSlots || !m_Lifecycle.QueryDevice(deviceId, nowMs)) return 0;
		RICON_MediaNativeRxWatch nativeWatch = RICON_MediaNativeRxWatch.Cast(watch);
		if (nativeWatch && (!nativeWatch.GetTransceiver() || nativeWatch.GetIndex() != index)) return 0;
		for (int i = 0; i < m_Slots.Count(); i++)
		{
			RICON_MediaRxWatch existing = m_Slots.GetElement(i).m_Watch;
			RICON_MediaNativeRxWatch existingNative = RICON_MediaNativeRxWatch.Cast(existing);
			if (nativeWatch && existingNative && nativeWatch.GetTransceiver() == existingNative.GetTransceiver()) return 0;
		}
		if (!watch.Claim()) return 0;
		RICON_MediaRxSlot slot = new RICON_MediaRxSlot();
		slot.m_Watch = watch;
		m_iSerial++;
		slot.m_iGeneration = m_iSerial;
		m_Slots.Set(key, slot);
		m_iLastWriteMs = nowMs;
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
		slot.m_iAdmission = 0;
		m_iLastWriteMs = nowMs;
		return true;
	}

	bool Admit(int deviceId, int index, int generation, int networkId, int channelId, int expectedFrequency, int nowMs,
		string expectedKey = "")
	{
		if (!CanRead(m_Lifecycle, nowMs)) return false;
		RICON_MediaRxSlot slot = m_Slots.Get(SlotKey(deviceId, index));
		if (!slot || slot.m_iGeneration != generation) return false;
		m_iLastWriteMs = nowMs;
		RICON_LifecycleDevice device = m_Lifecycle.QueryDevice(deviceId, nowMs);
		int version = slot.m_Watch.GetVersion();
		if (!device || !device.m_bPowered || !device.Allows(networkId, channelId) || expectedFrequency <= 0
			|| !slot.ReadMatches(expectedFrequency, expectedKey, version))
		{
			slot.m_iAdmission = 0;
			return false;
		}
		if (slot.m_iAdmission && slot.m_iWatchVersion == version && slot.m_iNetworkId == networkId
			&& slot.m_iChannelId == channelId && slot.m_iFrequency == expectedFrequency && slot.m_sKey == expectedKey) return true;
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
		RICON_MediaRxSlot slot = m_Slots.Get(SlotKey(route.m_iRxDeviceId, route.m_iRxTransceiverIndex));
		if (!slot || slot.m_iGeneration != route.m_iRxGeneration) return 0;
		return ReadSlotAdmission(slot, route.m_Source, nowMs);
	}

	protected int ReadSlotAdmission(RICON_MediaRxSlot slot, RICON_MediaStreamBinding source, int nowMs)
	{
		if (!slot || slot.m_iAdmission <= 0 || slot.m_iNetworkId != source.m_iNetworkTypeId
			|| slot.m_iChannelId != source.m_iChannelId) return 0;
		if (slot.ReadMatches(slot.m_iFrequency, slot.m_sKey, slot.m_iWatchVersion)) return slot.m_iAdmission;
		// A detected mismatch stays revoked until a fresh authority admission.
		slot.m_iAdmission = 0;
		m_iLastWriteMs = nowMs;
		return 0;
	}

	//! Open RADIO uses current, unkeyed physical admissions for both ends. No key leaves this registry.
	int ReadOpenRadioSource(RICON_MediaStreamBinding source, int nowMs)
	{
		if (!source || source.m_iSessionEpoch != m_iEpoch || !CanRead(m_Lifecycle, nowMs)) return 0;
		RICON_MediaRxSlot slot = m_Slots.Get(SlotKey(source.m_iDeviceId, source.m_iTxTransceiverIndex));
		if (!slot || !slot.m_sKey.IsEmpty()) return 0;
		return ReadSlotAdmission(slot, source, nowMs);
	}

	int ReadOpenRadioReceiver(RICON_MediaStreamBinding source, int rxDeviceId, int rxIndex, int txAdmission, int nowMs)
	{
		if (txAdmission <= 0 || ReadOpenRadioSource(source, nowMs) != txAdmission) return 0;
		RICON_MediaRxSlot tx = m_Slots.Get(SlotKey(source.m_iDeviceId, source.m_iTxTransceiverIndex));
		RICON_MediaRxSlot rx = m_Slots.Get(SlotKey(rxDeviceId, rxIndex));
		if (!rx || !rx.m_sKey.IsEmpty() || rx.m_iFrequency != tx.m_iFrequency || ReadSlotAdmission(rx, source, nowMs) <= 0) return 0;
		return rx.m_iGeneration;
	}

	bool Unregister(int deviceId, int index, int generation, int nowMs)
	{
		if (!CanRead(m_Lifecycle, nowMs)) return false;
		if (!Detach(deviceId, index, generation)) return false;
		m_iLastWriteMs = nowMs;
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

	void Close()
	{
		m_bClosed = true;
		for (int i = 0; i < m_Slots.Count(); i++) m_Slots.GetElement(i).m_Watch.Stop();
		m_Slots.Clear();
		m_Lifecycle = null;
	}
}
