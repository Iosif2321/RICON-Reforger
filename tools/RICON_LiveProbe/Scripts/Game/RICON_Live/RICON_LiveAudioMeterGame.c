//! Default-off own-B calibration lifecycle; legacy NRX/D0 paths are not changed.
modded class ArmaReforgerScripted
{
	protected ref RICON_LiveAudioMeterProbe m_RICON_AudioMeter;
	protected void RICON_StopAudioMeter(string reason)
	{
		if (m_RICON_AudioMeter) m_RICON_AudioMeter.Stop(reason);
		if (m_RICON_AudioMeter && m_RICON_AudioMeter.IsRunning()) return;
		m_RICON_AudioMeter = null;
	}
	override bool OnGameStart()
	{
		bool started = super.OnGameStart();
		RICON_StopAudioMeter("GAME_START_RESET");
		if (!started || !RICON_LiveAudioMeterProbe.IsRequested()) return started;
		BaseWorld world = GetWorld();
		if (!world || world.IsEditMode()) return started;
		m_RICON_AudioMeter = new RICON_LiveAudioMeterProbe();
		if (!m_RICON_AudioMeter.Start(world)) m_RICON_AudioMeter = null;
		return started;
	}
	override void OnUpdate(BaseWorld world, float timeslice)
	{
		super.OnUpdate(world, timeslice);
		if (m_RICON_AudioMeter) m_RICON_AudioMeter.Tick(world, timeslice);
	}
	override void OnGameEnd()
	{
		RICON_StopAudioMeter("GAME_END"); super.OnGameEnd();
	}
	override protected void OnBeforeWorldCleanup()
	{
		RICON_StopAudioMeter("WORLD_CLEANUP"); super.OnBeforeWorldCleanup();
	}
}
