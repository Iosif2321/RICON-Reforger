class RICON_RelayStationComponentClass : ScriptComponentClass {}

//! Explicit prefab classification. A generic WORLD radio is not a relay station.
class RICON_RelayStationComponent : ScriptComponent
{
	protected bool m_bDeleted;
	protected bool m_bRoleRevoked;
	protected bool m_bGmEnabled = true;
	protected float m_fGmInputMHz = 38;
	protected float m_fGmOutputMHz = 38.2;
	protected float m_fGmPowerDbm = 30;

	float ReadGmField(RICON_EGmRfField field)
	{
		switch (field)
		{
			case RICON_EGmRfField.RELAY_ENABLED: return m_bGmEnabled;
			case RICON_EGmRfField.RELAY_INPUT: return m_fGmInputMHz;
			case RICON_EGmRfField.RELAY_OUTPUT: return m_fGmOutputMHz;
			case RICON_EGmRfField.RELAY_POWER: return m_fGmPowerDbm;
		}
		return 0;
	}

	void WriteGmField(RICON_EGmRfField field, float value)
	{
		if (!RICON_CommsManagerComponent.IsAuthority() || m_bDeleted || m_bRoleRevoked || !RICON_Num.IsFinite(value)) return;
		switch (field)
		{
			case RICON_EGmRfField.RELAY_ENABLED: m_bGmEnabled = value >= 0.5; break;
			case RICON_EGmRfField.RELAY_INPUT:
				if (value >= 30 && value <= 80) m_fGmInputMHz = value;
				break;
			case RICON_EGmRfField.RELAY_OUTPUT:
				if (value >= 30 && value <= 80) m_fGmOutputMHz = value;
				break;
			case RICON_EGmRfField.RELAY_POWER:
				if (value >= -30 && value <= 70) m_fGmPowerDbm = value;
				break;
		}
	}
	bool MatchesRadio(BaseRadioComponent radio)
	{
		IEntity owner = GetOwner();
		return m_bGmEnabled && !m_bDeleted && !m_bRoleRevoked && owner && !owner.IsDeleted() && owner.GetWorld() && !owner.GetWorld().IsEditMode()
			&& radio && radio.GetOwner() == owner && radio.IsPowered()
			&& owner.FindComponent(RICON_RelayStationComponent) == this;
	}

	bool RevokeRole()
	{
		if (!RICON_CommsManagerComponent.IsAuthority()) return false;
		m_bRoleRevoked = true;
		return true;
	}

	override void OnDelete(IEntity owner) { m_bDeleted = true; super.OnDelete(owner); }
}

class RICON_JammerComponentClass : ScriptComponentClass {}

//! Only this explicitly classified game object projects an EW source into the pure RF model.
class RICON_JammerComponent : ScriptComponent
{
	protected ref RICON_RfNoiseEmitter m_Settings;
	protected vector m_vSealedOrigin;
	protected bool m_bDeleted;
	protected bool m_bRoleRevoked;

	bool Configure(RICON_RfNoiseEmitter settings)
	{
		if (!RICON_CommsManagerComponent.IsAuthority() || m_Settings || !LiveObject() || !settings) return false;
		if (settings.m_iEmissionId <= 0 || !settings.m_aBands || settings.m_aBands.Count() < 1 || settings.m_aBands.Count() > 64) return false;
		if (!RICON_Num.IsFinite(settings.m_fTotalPowerDbm) || !RICON_Num.IsFinite(settings.m_fTxGainDbi)) return false;
		RICON_RfNoiseEmitter copy = CopySettings(settings);
		if (!copy) return false;
		m_Settings = copy;
		m_vSealedOrigin = GetOwner().GetOrigin();
		return true;
	}

	RICON_RfNoiseEmitter ReadEmission()
	{
		if (!RICON_CommsManagerComponent.IsAuthority() || !m_Settings || !LiveObject() || GetOwner().GetOrigin() != m_vSealedOrigin) return null;
		RICON_RfNoiseEmitter copy = CopySettings(m_Settings);
		copy.m_vAntennaPos = m_vSealedOrigin;
		return copy;
	}

	protected bool LiveObject()
	{
		IEntity owner = GetOwner();
		if (m_bDeleted || m_bRoleRevoked || !owner || owner.IsDeleted() || !owner.GetWorld() || owner.GetWorld().IsEditMode()
			|| owner.FindComponent(RICON_JammerComponent) != this || owner.GetParent()) return false;
		Physics body = owner.GetPhysics();
		return !body || (!body.IsKinematic() && body.GetSimulationState() != SimulationState.SIMULATION);
	}

	bool RevokeRole()
	{
		if (!RICON_CommsManagerComponent.IsAuthority()) return false;
		m_bRoleRevoked = true;
		return true;
	}

	override void OnDelete(IEntity owner) { m_bDeleted = true; m_Settings = null; super.OnDelete(owner); }

	protected static RICON_RfNoiseEmitter CopySettings(RICON_RfNoiseEmitter source)
	{
		RICON_RfNoiseEmitter copy = new RICON_RfNoiseEmitter();
		copy.m_iEmissionId = source.m_iEmissionId;
		copy.m_bActive = source.m_bActive;
		copy.m_fTotalPowerDbm = source.m_fTotalPowerDbm;
		copy.m_fTxGainDbi = source.m_fTxGainDbi;
		foreach (RICON_RfNoiseBand band : source.m_aBands)
		{
			if (!band) return null;
			RICON_RfNoiseBand item = new RICON_RfNoiseBand();
			item.m_fFrequencyMHz = band.m_fFrequencyMHz;
			item.m_fBandwidthHz = band.m_fBandwidthHz;
			copy.m_aBands.Insert(item);
		}
		return copy;
	}
}
