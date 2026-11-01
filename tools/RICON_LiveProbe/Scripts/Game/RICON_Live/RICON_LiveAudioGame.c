//! TOOL_ONLY: passive observer lifecycle; no CLI gate means only the original game behavior.
modded class ArmaReforgerScripted
{
	protected ref RICON_LiveAudioProbe m_RICON_AudioProbe;

	protected void RICON_StopAudioObserver(string reason)
	{
		if (m_RICON_AudioProbe)
			m_RICON_AudioProbe.Stop(reason);
		m_RICON_AudioProbe = null;
	}

	override bool OnGameStart()
	{
		bool started = super.OnGameStart();
		RICON_StopAudioObserver("GAME_START_RESET");
		if (!started || !RICON_LiveAudioProbe.IsRequested())
			return started;
		BaseWorld world = GetWorld();
		if (!world || world.IsEditMode())
			return started;
		m_RICON_AudioProbe = new RICON_LiveAudioProbe();
		if (!m_RICON_AudioProbe.Start(world))
			m_RICON_AudioProbe = null;
		return started;
	}

	override void OnUpdate(BaseWorld world, float timeslice)
	{
		super.OnUpdate(world, timeslice);
		if (m_RICON_AudioProbe)
			m_RICON_AudioProbe.Tick(world, timeslice);
	}

	override void OnGameEnd()
	{
		RICON_StopAudioObserver("GAME_END");
		super.OnGameEnd();
	}

	override protected void OnBeforeWorldCleanup()
	{
		RICON_StopAudioObserver("WORLD_CLEANUP");
		super.OnBeforeWorldCleanup();
	}
}
