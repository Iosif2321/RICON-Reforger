//! TOOL_ONLY: actual pure RICON model and TX admission checks, no world or native radio calls.
[WorkbenchPluginAttribute(name: "RICON: Check generic radio model gate", wbModules: {"ScriptEditor"})]
class RICON_LiveGenericRadioCheckPlugin : WorkbenchPlugin
{
	override void RunCommandline()
	{
		int passed;
		int checks = 6;
		if (!RICON_LiveGenericRadioProbe.Admits(null)) passed++;
		RICON_LinkBudget allow = RICON_LiveGenericRadioProbe.Evaluate("0 0 0", "2 0 0", 37);
		if (allow.m_bValid && allow.m_eReason == RICON_EReason.OK && RICON_LiveGenericRadioProbe.Admits(allow)) passed++;
		RICON_LinkBudget lost = RICON_LiveGenericRadioProbe.Evaluate("0 0 0", "2 0 0", -200);
		if (lost.m_bValid && lost.m_eReason == RICON_EReason.OK && lost.m_eState == RICON_ELinkState.LOST && !RICON_LiveGenericRadioProbe.Admits(lost)) passed++;
		RICON_LinkBudget recovery = RICON_LiveGenericRadioProbe.Evaluate("0 0 0", "2 0 0", 37);
		if (RICON_LiveGenericRadioProbe.Admits(recovery) && recovery.m_fRxPowerDbm == allow.m_fRxPowerDbm) passed++;
		RICON_LinkBudget invalid = new RICON_LinkBudget();
		invalid.MarkInvalid(RICON_EReason.INVALID_FREQUENCY);
		if (!RICON_LiveGenericRadioProbe.Admits(invalid)) passed++;
		RICON_LinkBudget unknownState = RICON_TestSupport.NewValidBudget(1, RICON_ELinkState.CLEAN);
		unknownState.m_eReason = RICON_EReason.OK;
		unknownState.m_eState = -1;
		if (!RICON_LiveGenericRadioProbe.Admits(unknownState)) passed++;
		PrintFormat("[RICON_GENERICCHECK] checks=%1 passed=%2", checks, passed);
		if (passed == checks) Print("[RICON_GENERICCHECK] done: PASS");
		else Print("[RICON_GENERICCHECK] done: MISMATCH");
	}
}
