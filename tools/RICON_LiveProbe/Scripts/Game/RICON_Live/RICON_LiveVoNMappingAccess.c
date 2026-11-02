//! TOOL_ONLY I0: read-only access to protected script state. No overrides and no mutation.
modded class SCR_VONController
{
	bool RICON_I0ScriptActive() { return m_bIsActive; }
	bool RICON_I0ToggledDirect() { return m_bIsToggledDirect; }
	string RICON_I0ActiveHoldAction() { return m_sActiveHoldAction; }
	EVONTransmitType RICON_I0TransmitType() { return m_eVONType; }
}
