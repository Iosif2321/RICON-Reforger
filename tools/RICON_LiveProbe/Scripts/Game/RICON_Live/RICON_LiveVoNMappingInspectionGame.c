//! TOOL_ONLY I0 lifecycle; absent CLI flag preserves original behavior.
modded class ArmaReforgerScripted
{
	protected ref RICON_LiveVoNMappingInspection m_RICON_VoNMappingInspection;
	protected void RICON_StopVoNMapping(string reason)
	{
		if (m_RICON_VoNMappingInspection) m_RICON_VoNMappingInspection.Stop(reason);
		m_RICON_VoNMappingInspection = null;
	}
	override bool OnGameStart()
	{
		bool started = super.OnGameStart(); RICON_StopVoNMapping("GAME_START_RESET");
		if (!started || !RICON_LiveVoNMappingInspection.IsRequested()) return started;
		BaseWorld world = GetWorld(); if (!world || world.IsEditMode()) return started;
		m_RICON_VoNMappingInspection = new RICON_LiveVoNMappingInspection();
		if (!m_RICON_VoNMappingInspection.Start(world)) m_RICON_VoNMappingInspection = null;
		return started;
	}
	override void OnUpdate(BaseWorld world, float timeslice)
	{
		super.OnUpdate(world, timeslice); if (m_RICON_VoNMappingInspection) m_RICON_VoNMappingInspection.Tick(world, timeslice);
	}
	override void OnGameEnd() { RICON_StopVoNMapping("GAME_END"); super.OnGameEnd(); }
	override protected void OnBeforeWorldCleanup() { RICON_StopVoNMapping("WORLD_CLEANUP"); super.OnBeforeWorldCleanup(); }
}
