//! Default-off lifecycle for passive native metadata; legacy audio/discovery paths are untouched.
modded class ArmaReforgerScripted
{
	protected ref RICON_LivePairShadow m_RICON_PairShadow;
	protected ref RICON_LiveNativeRxProbe m_RICON_NativeRxProbe;
	protected ref RICON_LiveCapturePulseProbe m_RICON_CapturePulse;
	protected ref RICON_LivePairPlacement m_RICON_PairPlacement;
	protected ref RICON_LiveCaptureBPowerOff m_RICON_CaptureBPowerOff;
	protected void RICON_StopCaptureBPowerOff(string reason)
	{
		if (m_RICON_CaptureBPowerOff) m_RICON_CaptureBPowerOff.Stop(reason);
		m_RICON_CaptureBPowerOff = null;
	}
	protected ref RICON_LiveNativeRadioPower m_RICON_NativeRadioPower;
	protected void RICON_StopNativeRadioPower(string reason)
	{
		if (m_RICON_NativeRadioPower) m_RICON_NativeRadioPower.Stop(reason);
		m_RICON_NativeRadioPower = null;
	}
	protected ref RICON_LiveVoNSoundObserve m_RICON_VoNSoundObserve;
	protected void RICON_StopVoNSoundObserve(string reason)
	{
		if (m_RICON_VoNSoundObserve) m_RICON_VoNSoundObserve.Stop("UNQUALIFIED", reason, false);
		m_RICON_VoNSoundObserve = null;
	}
	protected void RICON_StopPairPlacement(string reason)
	{
		if (m_RICON_PairPlacement) m_RICON_PairPlacement.Stop("INCOMPLETE", reason);
		m_RICON_PairPlacement = null;
	}
	protected bool m_RICON_RestRehearsalArmUsed;
	protected bool m_RICON_RestCaptureArmUsed;
	protected bool RICON_StopCapturePulse(string reason)
	{
		if (m_RICON_CapturePulse) m_RICON_CapturePulse.Cancel(reason);
		if (m_RICON_CapturePulse && !m_RICON_CapturePulse.CanRelease()) return false;
		m_RICON_CapturePulse = null;
		return true;
	}
	//! Explicit UI request only. External guard proof remains in the root coordinator receipt.
	bool RICON_RequestCapturePulseArm(string run, string attempt, string mode)
	{
		return m_RICON_CapturePulse && m_RICON_CapturePulse.RequestArm(run, attempt, mode);
	}
	int RICON_RestRehearsalPreparedGeneration(string run, string attempt, BaseWorld expectedWorld)
	{
		if (!RICON_RestRehearsalGate.Requested() || !RICON_LiveNativeRxProbe.IsRequested() || !m_RICON_NativeRxProbe) return 0;
		if (!expectedWorld || expectedWorld != GetWorld() || expectedWorld.IsEditMode() || !m_RICON_CapturePulse) return 0;
		PlayerController player = GetPlayerController();
		if (!player || player.GetPlayerId() != 1) return 0;
		return m_RICON_CapturePulse.RestRehearsalPreparedGeneration(run, attempt, expectedWorld);
	}
	bool RICON_RequestRestRehearsalArm(string run, string attempt, int expectedPreparedGeneration, BaseWorld expectedWorld)
	{
		if (m_RICON_RestRehearsalArmUsed) return false;
		m_RICON_RestRehearsalArmUsed = true;
		int actual = RICON_RestRehearsalPreparedGeneration(run, attempt, expectedWorld);
		if (!RICON_RestRehearsalGate.PreparedMatches(expectedPreparedGeneration, actual)) return false;
		return m_RICON_CapturePulse.RequestRestRehearsalArm(run, attempt, expectedPreparedGeneration, expectedWorld);
	}
	protected void RICON_StopNativeRx(string reason)
	{
		if (m_RICON_NativeRxProbe) m_RICON_NativeRxProbe.Stop(reason);
		m_RICON_NativeRxProbe = null;
	}
	int RICON_RestCapturePreparedGeneration(string run, string attempt, BaseWorld expectedWorld)
	{
		if (!RICON_RestCaptureGate.Requested() || !m_RICON_NativeRxProbe || !m_RICON_CapturePulse) return 0;
		bool worldCurrent = expectedWorld && expectedWorld == GetWorld() && !expectedWorld.IsEditMode();
		PlayerController player = GetPlayerController();
		if (!player || !RICON_RestCaptureGate.LocalContext(worldCurrent, player.GetPlayerId())) return 0;
		return m_RICON_CapturePulse.RestCapturePreparedGeneration(run, attempt, expectedWorld);
	}
	bool RICON_RequestRestCaptureArm(string run, string attempt, int expectedPreparedGeneration, BaseWorld expectedWorld)
	{
		if (m_RICON_RestCaptureArmUsed) return false;
		m_RICON_RestCaptureArmUsed = true;
		int actual = RICON_RestCapturePreparedGeneration(run, attempt, expectedWorld);
		if (!RICON_RestRehearsalGate.PreparedMatches(expectedPreparedGeneration, actual)) return false;
		return m_RICON_CapturePulse.RequestRestCaptureArm(run, attempt, expectedPreparedGeneration, expectedWorld);
	}
	bool RICON_RequestRestBlindCaptureArm(string run, string attempt, int expectedPreparedGeneration, BaseWorld expectedWorld, int choice)
	{
		if (m_RICON_RestCaptureArmUsed) return false;
		m_RICON_RestCaptureArmUsed = true;
		int actual = RICON_RestCapturePreparedGeneration(run, attempt, expectedWorld);
		if (!RICON_RestRehearsalGate.PreparedMatches(expectedPreparedGeneration, actual)) return false;
		return m_RICON_CapturePulse.RequestRestBlindCaptureArm(run, attempt, expectedPreparedGeneration, expectedWorld, choice);
	}
	override bool OnGameStart()
	{
		bool started = super.OnGameStart();
		m_RICON_PairShadow = null;
		RICON_StopCaptureBPowerOff("GAME_START_RESET");
		RICON_StopNativeRadioPower("GAME_START_RESET");
		RICON_StopVoNSoundObserve("GAME_START_RESET");
		RICON_StopPairPlacement("GAME_START_RESET");
		bool released = RICON_StopCapturePulse("GAME_START_RESET");
		RICON_StopNativeRx("GAME_START_RESET");
		if (!started || !released || !RICON_LiveNativeRxProbe.IsRequested()) return started;
		BaseWorld world = GetWorld();
		if (!world || world.IsEditMode()) return started;
		m_RICON_NativeRxProbe = new RICON_LiveNativeRxProbe();
		if (!m_RICON_NativeRxProbe.Start(world)) m_RICON_NativeRxProbe = null;
		if (m_RICON_NativeRxProbe && RICON_LivePairShadow.Requested()) m_RICON_PairShadow = new RICON_LivePairShadow();
		if (RICON_LiveNativeRadioPower.Configured())
		{
			m_RICON_NativeRadioPower = new RICON_LiveNativeRadioPower();
			if (!m_RICON_NativeRadioPower.Start(world, m_RICON_NativeRxProbe)) m_RICON_NativeRadioPower = null;
			return started;
		}
		if (RICON_LivePairPlacement.Configured())
		{
			m_RICON_PairPlacement = new RICON_LivePairPlacement();
			if (!m_RICON_PairPlacement.Start(world, m_RICON_NativeRxProbe)) m_RICON_PairPlacement = null;
			return started;
		}
		if (RICON_CaptureSeparatedGate.Configured())
		{
			m_RICON_PairPlacement = new RICON_LivePairPlacement();
			if (!m_RICON_PairPlacement.StartSeparated(world, m_RICON_NativeRxProbe)) { m_RICON_PairPlacement = null; return started; }
		}
		if (m_RICON_NativeRxProbe)
		{
			if (RICON_CaptureBPowerOffGate.Configured())
			{
				m_RICON_CaptureBPowerOff = new RICON_LiveCaptureBPowerOff();
				if (!m_RICON_CaptureBPowerOff.Start(world, m_RICON_NativeRxProbe)) m_RICON_CaptureBPowerOff = null;
			}
			if (RICON_LiveVoNSoundObserve.Configured())
			{
				m_RICON_VoNSoundObserve = new RICON_LiveVoNSoundObserve();
				if (!m_RICON_VoNSoundObserve.Start(world, m_RICON_NativeRxProbe)) m_RICON_VoNSoundObserve = null;
			}
			m_RICON_CapturePulse = new RICON_LiveCapturePulseProbe();
			if (!m_RICON_CapturePulse.Start(world, m_RICON_NativeRxProbe)) m_RICON_CapturePulse = null;
		}
		return started;
	}
	override void OnUpdate(BaseWorld world, float timeslice)
	{
		super.OnUpdate(world, timeslice);
		if (m_RICON_NativeRxProbe) m_RICON_NativeRxProbe.Tick(world, timeslice);
		if (m_RICON_NativeRadioPower) m_RICON_NativeRadioPower.Tick(world);
		if (m_RICON_PairPlacement) m_RICON_PairPlacement.Tick(world);
		if (m_RICON_CaptureBPowerOff) m_RICON_CaptureBPowerOff.Tick(world);
		if (m_RICON_VoNSoundObserve) m_RICON_VoNSoundObserve.Tick(world);
		RICON_CaptureFrequencyControl.Poll(world, m_RICON_NativeRxProbe);
		if (m_RICON_CapturePulse) m_RICON_CapturePulse.Tick(world);
		if (m_RICON_PairShadow) m_RICON_PairShadow.Tick(world, m_RICON_NativeRxProbe);
	}
	override void OnGameEnd()
	{
		m_RICON_PairShadow = null;
		RICON_StopCaptureBPowerOff("GAME_END");
		RICON_StopNativeRadioPower("GAME_END");
		RICON_StopVoNSoundObserve("GAME_END");
		RICON_StopPairPlacement("GAME_END");
		RICON_CaptureFrequencyControl.ClearReader();
		RICON_StopCapturePulse("GAME_END"); RICON_StopNativeRx("GAME_END"); super.OnGameEnd();
	}
	override protected void OnBeforeWorldCleanup()
	{
		m_RICON_PairShadow = null;
		RICON_StopCaptureBPowerOff("WORLD_CLEANUP");
		RICON_StopNativeRadioPower("WORLD_CLEANUP");
		RICON_StopVoNSoundObserve("WORLD_CLEANUP");
		RICON_StopPairPlacement("WORLD_CLEANUP");
		RICON_CaptureFrequencyControl.ClearReader();
		RICON_StopCapturePulse("WORLD_CLEANUP"); RICON_StopNativeRx("WORLD_CLEANUP"); super.OnBeforeWorldCleanup();
	}
}
