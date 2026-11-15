class RICON_ManagedRxComponentClass : ScriptComponentClass
{
}

//! Opt-in prefab component. The server scenario supplies IDs from its current Lifecycle catalog.
class RICON_ManagedRxComponent : ScriptComponent
{
	protected RICON_MediaReceiverAuthority m_Authority;
	protected ref array<int> m_Generations = {};
	protected int m_iDeviceId;

	bool Attach(RICON_MediaReceiverAuthority authority, int deviceId, int nowMs)
	{
		if (!RICON_CommsManagerComponent.IsAuthority() || !authority || deviceId <= 0) return false;
		bool attached = !m_Generations.IsEmpty();
		for (int slot = 0; slot < m_Generations.Count(); slot++)
		{
			if (GetGeneration(slot) <= 0) attached = false;
		}
		if (attached) return false;
		Detach();
		IEntity owner = GetOwner();
		if (!owner || !owner.GetWorld() || owner.GetWorld().IsEditMode()) return false;
		BaseRadioComponent radio = BaseRadioComponent.Cast(owner.FindComponent(BaseRadioComponent));
		if (!radio || radio.GetOwner() != owner || radio.TransceiversCount() <= 0) return false;
		m_Authority = authority;
		m_iDeviceId = deviceId;
		int count = radio.TransceiversCount();
		for (int index = 0; index < count; index++)
		{
			int generation = authority.RegisterRadio(deviceId, index, radio, nowMs);
			if (generation <= 0)
			{
				Detach();
				return false;
			}
			m_Generations.Insert(generation);
		}
		return true;
	}

	int GetGeneration(int index)
	{
		if (!m_Authority || index < 0 || index >= m_Generations.Count()) return 0;
		int generation = m_Generations[index];
		if (m_Authority.HasRegistration(m_iDeviceId, index, generation)) return generation;
		return 0;
	}

	//! Managed key changes revoke the whole radio before calling the native setter, which may emit no event.
	bool SetManagedKey(string key, int nowMs)
	{
		if (!RICON_CommsManagerComponent.IsAuthority() || !m_Authority || m_Generations.IsEmpty()) return false;
		IEntity owner = GetOwner();
		if (!owner || !owner.GetWorld() || owner.GetWorld().IsEditMode()) return false;
		BaseRadioComponent radio = BaseRadioComponent.Cast(owner.FindComponent(BaseRadioComponent));
		if (!radio || radio.GetOwner() != owner || radio.TransceiversCount() != m_Generations.Count()) return false;
		for (int index = 0; index < m_Generations.Count(); index++)
		{
			if (!m_Authority.MatchesRadio(m_iDeviceId, index, m_Generations[index], radio)) return false;
		}
		for (int index = 0; index < m_Generations.Count(); index++)
		{
			if (!m_Authority.Invalidate(m_iDeviceId, index, m_Generations[index], nowMs)) return false;
		}
		radio.SetEncryptionKey(key);
		return radio.GetEncryptionKey() == key;
	}

	//! Trusted scenario uses this on transfer and must obtain a new admission before writing settings.
	void Detach()
	{
		if (m_Authority)
		{
			for (int index = 0; index < m_Generations.Count(); index++)
				m_Authority.Detach(m_iDeviceId, index, m_Generations[index]);
		}
		m_Generations.Clear();
		m_Authority = null;
		m_iDeviceId = 0;
	}

	override void OnDelete(IEntity owner)
	{
		Detach();
		super.OnDelete(owner);
	}
}
