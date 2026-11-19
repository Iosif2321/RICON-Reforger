enum RICON_EGameRadioEquipment
{
	POCKET = 1,
	MANPACK = 2,
	VEHICLE = 3
}

class RICON_GameRadioEquipmentComponentClass : ScriptComponentClass {}

//! Scenario classification on the entity that owns the native radio.
class RICON_GameRadioEquipmentComponent : ScriptComponent
{
	[Attribute("1", UIWidgets.EditBox, "Radio equipment: 1 pocket, 2 manpack, 3 vehicle")]
	protected RICON_EGameRadioEquipment m_eEquipment;

	RICON_EGameRadioEquipment ReadEquipment() { return m_eEquipment; }
}

class RICON_GameRadioEquipment
{
	static BaseRadioComponent ReadNativeRadio(IEntity owner)
	{
		if (!owner || owner.IsDeleted()) return null;
		return BaseRadioComponent.Cast(owner.FindComponent(BaseRadioComponent));
	}

	static RICON_EGameRadioEquipment ReadType(BaseRadioComponent radio)
	{
		if (!radio) return 0;
		IEntity owner = radio.GetOwner();
		if (ReadNativeRadio(owner) != radio) return 0;
		RICON_GameRadioEquipmentComponent authored = RICON_GameRadioEquipmentComponent.Cast(owner.FindComponent(RICON_GameRadioEquipmentComponent));
		if (authored) return authored.ReadEquipment();
		EntityPrefabData prefab = owner.GetPrefabData();
		if (prefab)
		{
			ResourceName name = prefab.GetPrefabName();
			if (name == "{9B6B61BB3FE3DFB0}Prefabs/Items/Equipment/Radios/Radio_ANPRC77.et"
				|| name == "{54C68E438DD34265}Prefabs/Items/Equipment/Radios/Radio_R107M.et") return RICON_EGameRadioEquipment.MANPACK;
		}
		return RICON_EGameRadioEquipment.POCKET;
	}

	static BaseRadioComponent ReadDriverRadio(IEntity character, out BaseCompartmentSlot seat)
	{
		seat = null;
		if (!character || character.IsDeleted()) return null;
		CompartmentAccessComponent access = CompartmentAccessComponent.Cast(character.FindComponent(CompartmentAccessComponent));
		if (!access) return null;
		BaseCompartmentSlot occupied = access.GetCompartment();
		if (!PilotCompartmentSlot.Cast(occupied) || occupied.GetOccupant() != character) return null;
		IEntity vehicle = occupied.GetVehicle();
		if (!vehicle || vehicle.IsDeleted() || vehicle.GetWorld() != character.GetWorld()) return null;
		BaseVehicleControllerComponent controller = BaseVehicleControllerComponent.Cast(vehicle.FindComponent(BaseVehicleControllerComponent));
		if (!controller) return null;
		PilotCompartmentSlot designated = controller.GetPilotCompartmentSlot();
		if (designated && designated != occupied) return null;
		BaseRadioComponent radio = ReadNativeRadio(vehicle);
		if (ReadType(radio) != RICON_EGameRadioEquipment.VEHICLE || RadioSlotIndex(radio, 0) < 0) return null;
		seat = occupied;
		return radio;
	}

	static int RadioSlotIndex(BaseRadioComponent radio, int ordinal)
	{
		if (!radio || ordinal < 0) return -1;
		for (int index = 0; index < radio.TransceiversCount(); index++)
		{
			if (!RadioTransceiver.Cast(radio.GetTransceiver(index))) continue;
			if (ordinal == 0) return index;
			ordinal--;
		}
		return -1;
	}

	//! Scenario assumptions for the hardware only. The channel owns frequency and bandwidth.
	static bool ApplyHardware(RICON_EGameRadioEquipment equipment, RICON_RadioRfProfile profile)
	{
		if (!profile) return false;
		switch (equipment)
		{
			case RICON_EGameRadioEquipment.POCKET:
				profile.m_sProfileId = "demo_pocket_v1";
				profile.m_fTxPowerDbm = 30; profile.m_fTxGainDbi = -3; profile.m_fRxGainDbi = -3;
				profile.m_fRxNoiseFigureDb = 8; profile.m_fRxSensitivityDbm = -105;
				break;
			case RICON_EGameRadioEquipment.MANPACK:
				profile.m_sProfileId = "demo_manpack_v1";
				profile.m_fTxPowerDbm = 37; profile.m_fTxGainDbi = 0; profile.m_fRxGainDbi = 0;
				profile.m_fRxNoiseFigureDb = 6; profile.m_fRxSensitivityDbm = -112;
				break;
			case RICON_EGameRadioEquipment.VEHICLE:
				profile.m_sProfileId = "demo_vehicle_v1";
				profile.m_fTxPowerDbm = 43; profile.m_fTxGainDbi = 2; profile.m_fRxGainDbi = 2;
				profile.m_fRxNoiseFigureDb = 5; profile.m_fRxSensitivityDbm = -115;
				break;
			default: return false;
		}
		profile.m_sProvenance = RICON_RadioRfProfile.PROVENANCE_ASSUMED;
		profile.m_bHasRxSensitivity = true;
		profile.m_bHasRequiredSinr = false;
		profile.m_fRequiredSinrDb = 0;
		return RICON_RadioQueryBuilder.IsValidProfile(profile);
	}
}
