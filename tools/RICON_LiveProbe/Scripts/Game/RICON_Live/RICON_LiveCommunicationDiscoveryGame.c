//! TOOL_ONLY D0 lifecycle. Without both CLI values this modded game class executes only original behavior.
modded class ArmaReforgerScripted
{
	protected ref RICON_LiveCommunicationDiscovery m_RICON_CommunicationDiscovery;

	protected void RICON_StopCommunicationDiscovery(string reason)
	{
		if (m_RICON_CommunicationDiscovery)
			m_RICON_CommunicationDiscovery.Stop(reason);
		m_RICON_CommunicationDiscovery = null;
	}

	override bool OnGameStart()
	{
		bool started = super.OnGameStart();
		RICON_StopCommunicationDiscovery("GAME_START_RESET");
		if (!started || !RICON_LiveCommunicationDiscovery.IsRequested())
			return started;
		BaseWorld world = GetWorld();
		if (!world || world.IsEditMode())
			return started;
		m_RICON_CommunicationDiscovery = new RICON_LiveCommunicationDiscovery();
		if (!m_RICON_CommunicationDiscovery.Start(world))
			m_RICON_CommunicationDiscovery = null;
		return started;
	}

	override void OnUpdate(BaseWorld world, float timeslice)
	{
		super.OnUpdate(world, timeslice);
		if (m_RICON_CommunicationDiscovery)
			m_RICON_CommunicationDiscovery.Tick(world, timeslice);
	}

	override void OnGameEnd()
	{
		RICON_StopCommunicationDiscovery("GAME_END");
		super.OnGameEnd();
	}

	override protected void OnBeforeWorldCleanup()
	{
		RICON_StopCommunicationDiscovery("WORLD_CLEANUP");
		super.OnBeforeWorldCleanup();
	}
}
