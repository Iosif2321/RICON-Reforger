enum RICON_EGmRfField
{
	JAMMER_ENABLED,
	JAMMER_FREQUENCY,
	JAMMER_BANDWIDTH,
	JAMMER_POWER,
	JAMMER_GAIN,
	JAMMER_RADIUS,
	RELAY_ENABLED,
	RELAY_INPUT,
	RELAY_OUTPUT,
	RELAY_POWER
}

// One server-side role per selected entity. The entity remains the live position/ownership witness.
class RICON_GmJammerRole
{
	int m_iId;
	bool m_bEnabled;
	float m_fFrequencyMHz = 38;
	float m_fBandwidthKHz = 250;
	float m_fPowerDbm = 30;
	float m_fGainDbi;
	float m_fRadiusM;

	void RICON_GmJammerRole(IEntity target)
	{
		if (target && target.FindComponent(RICON_JammerComponent)) m_fRadiusM = 500;
	}

	static bool IsTarget(IEntity entity)
	{
		return entity && !entity.IsDeleted() && entity.GetWorld() && !entity.GetWorld().IsEditMode()
			&& !ChimeraCharacter.Cast(entity) && !SCR_AIGroup.Cast(entity)
			&& (Vehicle.Cast(entity) || SCR_DestructibleBuildingEntity.Cast(entity) || !entity.FindComponent(InventoryItemComponent));
	}

	bool SetField(RICON_EGmRfField field, float value)
	{
		if (!RICON_Num.IsFinite(value)) return false;
		switch (field)
		{
			case RICON_EGmRfField.JAMMER_ENABLED: m_bEnabled = value >= 0.5; return true;
			case RICON_EGmRfField.JAMMER_FREQUENCY:
				if (value < 20 || value > 500) return false;
				m_fFrequencyMHz = value; return true;
			case RICON_EGmRfField.JAMMER_BANDWIDTH:
				if (value < 5 || value > 2000) return false;
				m_fBandwidthKHz = value; return true;
			case RICON_EGmRfField.JAMMER_POWER:
				if (value < -30 || value > 70) return false;
				m_fPowerDbm = value; return true;
			case RICON_EGmRfField.JAMMER_GAIN:
				if (value < -20 || value > 30) return false;
				m_fGainDbi = value; return true;
			case RICON_EGmRfField.JAMMER_RADIUS:
				if (value < 0 || value > 10000) return false;
				m_fRadiusM = value; return true;
		}
		return false;
	}

	float ReadField(RICON_EGmRfField field)
	{
		switch (field)
		{
			case RICON_EGmRfField.JAMMER_ENABLED: return m_bEnabled;
			case RICON_EGmRfField.JAMMER_FREQUENCY: return m_fFrequencyMHz;
			case RICON_EGmRfField.JAMMER_BANDWIDTH: return m_fBandwidthKHz;
			case RICON_EGmRfField.JAMMER_POWER: return m_fPowerDbm;
			case RICON_EGmRfField.JAMMER_GAIN: return m_fGainDbi;
			case RICON_EGmRfField.JAMMER_RADIUS: return m_fRadiusM;
		}
		return 0;
	}

	RICON_RfNoiseEmitter ReadEmission(vector origin)
	{
		RICON_RfNoiseEmitter emission = new RICON_RfNoiseEmitter();
		emission.m_iEmissionId = m_iId;
		emission.m_bActive = m_bEnabled;
		emission.m_vAntennaPos = origin;
		emission.m_fTotalPowerDbm = m_fPowerDbm;
		emission.m_fTxGainDbi = m_fGainDbi;
		RICON_RfNoiseBand band = new RICON_RfNoiseBand();
		band.m_fFrequencyMHz = m_fFrequencyMHz;
		band.m_fBandwidthHz = m_fBandwidthKHz * 1000;
		emission.m_aBands.Insert(band);
		return emission;
	}
}

[BaseContainerProps()]
class RICON_GmRfEditorAttribute : SCR_BaseValueListEditorAttribute
{
	[Attribute("0", UIWidgets.ComboBox, "RICON field", enums: ParamEnumArray.FromEnum(RICON_EGmRfField))]
	protected RICON_EGmRfField m_eField;

	override bool IsServer() { return true; }

	protected IEntity ReadEntity(Managed item)
	{
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(item);
		if (!editable) return null;
		return editable.GetOwner();
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!manager) return null;
		IEntity entity = ReadEntity(item);
		if (!RICON_GmJammerRole.IsTarget(entity)) return null;
		RICON_RelayStationComponent station = RICON_RelayStationComponent.Cast(entity.FindComponent(RICON_RelayStationComponent));
		if (m_eField >= RICON_EGmRfField.RELAY_ENABLED)
		{
			if (!station) return null;
			return SCR_BaseEditorAttributeVar.CreateFloat(station.ReadGmField(m_eField));
		}
		RICON_GameRadioManagerComponent radio = RICON_GameRadioManagerComponent.FindGmManager();
		if (!radio) return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(radio.ReadGmJammerField(entity, m_eField));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var || !manager || !RICON_CommsManagerComponent.IsAuthority()) return;
		SCR_EditorModeEntity mode = SCR_EditorModeEntity.Cast(manager.GetOwner());
		if (!mode || mode.GetModeType() != EEditorMode.EDIT || !mode.GetManager() || mode.GetManager().GetPlayerID() != playerID) return;
		IEntity entity = ReadEntity(item);
		if (!RICON_GmJammerRole.IsTarget(entity)) return;
		if (m_eField >= RICON_EGmRfField.RELAY_ENABLED)
		{
			RICON_RelayStationComponent station = RICON_RelayStationComponent.Cast(entity.FindComponent(RICON_RelayStationComponent));
			if (station) station.WriteGmField(m_eField, var.GetFloat());
			return;
		}
		RICON_GameRadioManagerComponent radio = RICON_GameRadioManagerComponent.FindGmManager();
		if (radio) radio.WriteGmJammerField(entity, m_eField, var.GetFloat());
	}
}

[BaseContainerProps()]
class RICON_GmJammerEnabledAttribute : RICON_GmRfEditorAttribute {}
[BaseContainerProps()]
class RICON_GmJammerFrequencyAttribute : RICON_GmRfEditorAttribute {}
[BaseContainerProps()]
class RICON_GmJammerBandwidthAttribute : RICON_GmRfEditorAttribute {}
[BaseContainerProps()]
class RICON_GmJammerPowerAttribute : RICON_GmRfEditorAttribute {}
[BaseContainerProps()]
class RICON_GmJammerGainAttribute : RICON_GmRfEditorAttribute {}
[BaseContainerProps()]
class RICON_GmJammerRadiusAttribute : RICON_GmRfEditorAttribute {}
[BaseContainerProps()]
class RICON_GmRelayEnabledAttribute : RICON_GmRfEditorAttribute {}
[BaseContainerProps()]
class RICON_GmRelayInputAttribute : RICON_GmRfEditorAttribute {}
[BaseContainerProps()]
class RICON_GmRelayOutputAttribute : RICON_GmRfEditorAttribute {}
[BaseContainerProps()]
class RICON_GmRelayPowerAttribute : RICON_GmRfEditorAttribute {}
