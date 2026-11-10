//! TOOL_ONLY: проверка lifetime/share handle, без Arm, capture или голосового payload.
class RICON_FileHandleCapabilityGate
{
	static const int READ_DELAY_MS = 5000;
	static const int TIMEOUT_MS = 10000;
	static bool ValidConfig(string run, string token)
	{
		return RICON_D0Gate.ValidRunId(run) && RICON_D0Gate.ValidRunId(token);
	}
	static string PathFor(string run)
	{
		return "$profile:RICON_FileHandleCapability_" + run + ".txt";
	}
	static string Phase(int now, int last, int ready)
	{
		if (now < last || now < ready)
			return "CLOCK_RESET";
		if (now - ready >= TIMEOUT_MS)
			return "TIMEOUT";
		if (now - ready >= READ_DELAY_MS)
			return "READ";
		return "WAIT";
	}
}

//! ref передаётся из WB до Play; сохранность static/VM пока проверяется, а не предполагается.
class RICON_FileHandleCapability
{
	protected static ref FileHandle s_Handle;
	protected static BaseWorld s_World;
	protected static string s_Run;
	protected static string s_Token;
	protected static bool s_Held;
	protected static bool s_KnownOpen;
	protected static bool s_Started;
	protected static bool s_Finished;
	protected static bool s_ReadUsed;
	protected static int s_ReadyAt;
	protected static int s_Last;
	protected static string Bit(bool value)
	{
		if (value)
			return "1";
		return "0";
	}
	static bool Requested()
	{
		string value;
		return System.GetCLIParam("riconFileHandleCapability", value) && value == "1";
	}
	static bool Config(out string run, out string token)
	{
		return Requested() && System.GetCLIParam("riconFileHandleRun", run)
			&& System.GetCLIParam("riconFileHandleToken", token) && RICON_FileHandleCapabilityGate.ValidConfig(run, token);
	}
	static bool Hold(FileHandle handle, string run, string token)
	{
		if (s_Held || s_Started || s_Finished || !handle || !RICON_FileHandleCapabilityGate.ValidConfig(run, token))
			return false;
		s_Held = true; s_KnownOpen = true; s_Handle = handle; s_Run = run; s_Token = token;
		return true;
	}
	protected static void Emit(string kind, string fields)
	{
		PrintFormat("[RICON_FHC] run=%1 kind=%2 %3", s_Run, kind, fields);
	}
	static void BeginPlay(BaseWorld world)
	{
		if (!Requested() || s_Started || s_Finished || !world || world.IsEditMode())
			return;
		s_Started = true;
		string run; string token;
		if (!Config(run, token)) { Finish("CONFIG_REFUSED"); return; }
		bool binding = s_Held && s_Run == run && s_Token == token;
		s_Run = run;
		Emit("PLAY_CHECK", "holderSeen=" + Bit(s_Held) + ",referencePresent=" + Bit(s_Handle != null));
		if (!binding || !s_Handle) { Finish("HOLDER_UNAVAILABLE_OR_BINDING_MISMATCH"); return; }
		Emit("IS_OPEN_BEGIN", "context=PLAY");
		s_KnownOpen = s_Handle.IsOpen();
		if (!s_KnownOpen) { Finish("HANDLE_CLOSED"); return; }
		s_World = world; s_ReadyAt = System.GetTickCount(); s_Last = s_ReadyAt;
		Emit("READY_IN_PLAY", "readyTickMs=" + s_ReadyAt.ToString()
			+ ",readDelayMs=5000,timeoutMs=10000,externalWrite=ONE_IN_PLACE_TOKEN_LF,media=NOT_TESTED");
	}
	static void Tick(BaseWorld world)
	{
		if (!s_Started || s_Finished)
			return;
		if (!world || world != s_World || world.IsEditMode()) { Finish("WORLD_CHANGED"); return; }
		int now = System.GetTickCount();
		string phase = RICON_FileHandleCapabilityGate.Phase(now, s_Last, s_ReadyAt); s_Last = now;
		if (phase == "CLOCK_RESET" || phase == "TIMEOUT") { Finish(phase); return; }
		if (phase != "READ" || s_ReadUsed)
			return;
		s_ReadUsed = true;
		Emit("READ_ATTEMPT", "elapsedMs=" + (now - s_ReadyAt).ToString() + ",attempt=1,reopen=0");
		s_KnownOpen = s_Handle.IsOpen();
		if (!s_KnownOpen) { Finish("HANDLE_CLOSED_BEFORE_READ"); return; }
		int length = s_Handle.GetLength();
		if (length < 0 || length > 66) { Finish("LENGTH_OUT_OF_CONTRACT"); return; }
		bool eofBefore = s_Handle.IsEOF();
		s_Handle.Seek(0);
		int seekPos = s_Handle.GetPos();
		if (seekPos != 0) { Finish("SEEK_NOT_ZERO"); return; }
		string line;
		int count = s_Handle.ReadLine(line);
		int afterPos = s_Handle.GetPos(); bool eofAfter = s_Handle.IsEOF();
		bool exactMatch = count >= 0 && line == s_Token;
		string fields = "characters=" + count.ToString() + ",lineLength=" + line.Length().ToString();
		fields = fields + ",lengthBeforeRead=" + length.ToString() + ",seekPos=" + seekPos.ToString();
		fields = fields + ",afterPos=" + afterPos.ToString() + ",eofBeforeSeek=" + Bit(eofBefore);
		fields = fields + ",eofAfterRead=" + Bit(eofAfter) + ",exactMatch=" + Bit(exactMatch);
		fields = fields + ",contentLogged=0";
		Emit("READ_RETURN", fields);
		string afterPhase = RICON_FileHandleCapabilityGate.Phase(System.GetTickCount(), s_Last, s_ReadyAt);
		if (afterPhase == "CLOCK_RESET" || afterPhase == "TIMEOUT") { Finish("READ_RETURN_OUTSIDE_WINDOW"); return; }
		if (count < 0) { Finish("EOF_OR_NOT_VISIBLE"); return; }
		if (line != s_Token) { Finish("TOKEN_MISMATCH"); return; }
		Finish("TOKEN_OBSERVED");
	}
	static void Finish(string result)
	{
		if (!s_Started || s_Finished)
			return;
		s_Finished = true;
		Emit("TERMINAL", "result=" + result + ",readUsed=" + Bit(s_ReadUsed)
			+ ",nativeSharingCause=UNKNOWN,authorizationScope=EXTERNAL_OBSERVATION,voice=NOT_TESTED");
		if (s_Handle && s_KnownOpen)
		{
			Emit("CLOSE_BEGIN", "heldReferenceOnly=1");
			s_Handle.Close();
			Emit("CLOSE_RETURN", "nativeReleaseProof=NOT_CLAIMED");
		}
		s_Handle = null; s_World = null; s_Token = "";
	}
}

//! Отдельный default-off lifecycle; существующие NRX/Pulse классы не изменяются.
modded class ArmaReforgerScripted
{
	override bool OnGameStart()
	{
		bool started = super.OnGameStart();
		if (started) RICON_FileHandleCapability.BeginPlay(GetWorld());
		return started;
	}
	override void OnUpdate(BaseWorld world, float timeslice)
	{
		super.OnUpdate(world, timeslice);
		RICON_FileHandleCapability.Tick(world);
	}
	override void OnGameEnd()
	{
		RICON_FileHandleCapability.Finish("GAME_END");
		super.OnGameEnd();
	}
	override protected void OnBeforeWorldCleanup()
	{
		RICON_FileHandleCapability.Finish("WORLD_CLEANUP");
		super.OnBeforeWorldCleanup();
	}
}
