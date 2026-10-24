//! TOOL_ONLY B0a: один наблюдатель на игровой мир процесса; без CLI-гейта вызывается только штатный код.
modded class ArmaReforgerScripted
{
	protected ref RICON_LiveEndpointProbe m_RICON_EndpointProbe;
	protected int m_iRICON_EndpointCycle;

	protected void RICON_StopEndpoint(string reason)
	{
		if (m_RICON_EndpointProbe)
			m_RICON_EndpointProbe.Stop(reason);
		m_RICON_EndpointProbe = null;
	}

	override bool OnGameStart()
	{
		bool started = super.OnGameStart();
		RICON_StopEndpoint("GAME_START_RESET");
		if (!started || !RICON_LiveEndpointProbe.IsRequested())
			return started;
		BaseWorld world = GetWorld();
		if (!world || world.IsEditMode())
			return started;
		m_iRICON_EndpointCycle++;
		m_RICON_EndpointProbe = new RICON_LiveEndpointProbe();
		if (!m_RICON_EndpointProbe.Start(world, m_iRICON_EndpointCycle))
			m_RICON_EndpointProbe = null;
		return started;
	}

	override void OnUpdate(BaseWorld world, float timeslice)
	{
		super.OnUpdate(world, timeslice);
		if (m_RICON_EndpointProbe)
			m_RICON_EndpointProbe.Tick(world, timeslice);
	}

	override void OnGameEnd()
	{
		RICON_StopEndpoint("GAME_END");
		super.OnGameEnd();
	}

	override protected void OnBeforeWorldCleanup()
	{
		RICON_StopEndpoint("WORLD_CLEANUP");
		super.OnBeforeWorldCleanup();
	}
}
