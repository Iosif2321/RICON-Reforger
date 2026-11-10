//! TOOL_ONLY: один outbound loopback GET; guarded ingress только по exact opt-in, callback без actions.
class RICON_RestCapabilityGate
{
	static const int NATIVE_TIMEOUT_S = 5;
	static const int CALLBACK_MS = 5000;
	static const int TOTAL_MS = 10000;
	static bool Port(string value, out int port)
	{
		port = 0;
		if (value.IsEmpty() || value.Length() > 5) return false;
		for (int i = 0; i < value.Length(); i++)
		{
			int code = value.ToAscii(i);
			if (code < 48 || code > 57) return false;
			port = port * 10 + code - 48;
		}
		return port >= 1024 && port <= 65535;
	}
	static bool Ids(string run, string token)
	{
		if (!RICON_D0Gate.ValidRunId(run) || !RICON_D0Gate.ValidRunId(token)) return false;
		return run != "." && run != "..";
	}
	static string BaseUrl(int port) { return "http://127.0.0.1:" + port.ToString() + "/"; }
	static string Request(string run) { return "ricon-control-capability/" + run; }
	static bool Timely(int started, int requestAt, int callbackAt, int now)
	{
		if (requestAt < started || callbackAt < requestAt || now < callbackAt) return false;
		if (now - started >= TOTAL_MS) return false;
		return callbackAt - requestAt <= CALLBACK_MS;
	}
	static string CallbackResult(bool success, bool match, int http, ERestResult result)
	{
		if (!success)
		{
			if (result == ERestResult.EREST_ERROR_TIMEOUT) return "NATIVE_TIMEOUT";
			return "ERROR_CALLBACK";
		}
		if (http != 200) return "HTTP_MISMATCH";
		if (result != ERestResult.EREST_SUCCESS) return "REST_RESULT_MISMATCH";
		if (!match) return "TOKEN_MISMATCH";
		return "TOKEN_OBSERVED";
	}
}

//! Только explicit guarded rehearsal меняет бюджеты; обычный capability остаётся 5/10 секунд.
class RICON_RestRehearsalGate
{
	static bool Requested()
	{
		string value;
		return System.GetCLIParam("riconRestRehearsalArm", value) && value == "1";
	}
	static bool Config(bool enabled, bool nativeRx, string mode, string restRun, string nrxRun, string attempt)
	{
		return enabled && nativeRx && mode == "rehearsal" && restRun == nrxRun
			&& RICON_RestCapabilityGate.Ids(restRun, attempt);
	}
	static bool PreparedMatches(int expected, int actual) { return expected > 0 && actual == expected; }
	static int TotalMs(bool rehearsal)
	{
		if (rehearsal) return 55000;
		return RICON_RestCapabilityGate.TOTAL_MS;
	}
	static int CallbackMs(bool rehearsal)
	{
		if (rehearsal) return 50000;
		return RICON_RestCapabilityGate.CALLBACK_MS;
	}
	static int TimeoutS(bool rehearsal)
	{
		if (rehearsal) return 50;
		return RICON_RestCapabilityGate.NATIVE_TIMEOUT_S;
	}
	static bool Timely(bool rehearsal, int started, int requestAt, int callbackAt, int now)
	{
		if (!rehearsal) return RICON_RestCapabilityGate.Timely(started, requestAt, callbackAt, now);
		if (requestAt < started || callbackAt < requestAt || now < callbackAt) return false;
		return now - started < 55000 && callbackAt - requestAt <= 50000;
	}
}

//! NoSend subset: WrongFrequency намеренно unbound/refused до отдельного frequency-only patch.
class RICON_CaptureControlGate
{
	static bool Config(bool noSendConfigured, string noSendValue, bool wrongFrequencyConfigured, string wrongFrequencyValue = "")
	{
		if (noSendConfigured && wrongFrequencyConfigured) return false;
		if (wrongFrequencyConfigured) return wrongFrequencyValue == "1";
		return !noSendConfigured || noSendValue == "1";
	}
	static bool Valid()
	{
		string noSend; string wrong;
		bool configured = System.GetCLIParam("riconCaptureNoSend", noSend);
		bool wrongConfigured = System.GetCLIParam("riconCaptureWrongFrequency", wrong);
		return Config(configured, noSend, wrongConfigured, wrong);
	}
	static bool NoSend()
	{
		string value;
		return System.GetCLIParam("riconCaptureNoSend", value) && value == "1" && Valid();
	}
	static bool NativeAllowed(bool captureMode, bool noSend) { return captureMode && !noSend; }
	static bool WrongFrequency()
	{
		string value;
		return System.GetCLIParam("riconCaptureWrongFrequency", value) && value == "1" && Valid();
	}
	static string Result(bool noSend, string result)
	{
		if (noSend && result == "CAPTURE_REQUEST_CYCLE_OBSERVED") return "NO_SEND_CONTROL_CYCLE_OBSERVED";
		return result;
	}
}

//! Capture opt-in отделён от rehearsal; external Guard/RR admission не удостоверяется Game.
class RICON_RestCaptureGate
{
	static bool Requested()
	{
		string value; string rehearsal;
		if (!RICON_CaptureControlGate.Valid()) return false;
		if (System.GetCLIParam("riconRestRehearsalArm", rehearsal)) return false;
		return System.GetCLIParam("riconRestCapturePulse", value) && value == "1"
			&& RICON_RestControlCapability.Requested() && RICON_LiveNativeRxProbe.IsRequested();
	}
	static bool Config(bool enabled, bool capability, bool nativeRx, bool rehearsal, string mode, string restRun, string nrxRun, string attempt)
	{
		if (!enabled || !capability || !nativeRx || rehearsal || mode != "capture") return false;
		return restRun == nrxRun && RICON_RestCapabilityGate.Ids(restRun, attempt);
	}
	static bool LocalContext(bool world, int player) { return world && player == 1; }
}

//! Единственный typed blind ответ, без произвольных команд и без источника случайности Game.
class RICON_CaptureBlindGate
{
	static bool Config(bool configured, string value, bool serverA, bool separated, bool guardedCapture, bool otherMode)
	{
		if (!configured)
			return true;
		return value == "1" && serverA && separated && guardedCapture && !otherMode;
	}
	static bool Parse(string expectedToken, string response, out int choice)
	{
		choice = -1;
		if (expectedToken.Length() != 32 || response.Length() != 34) return false;
		for (int i = 0; i < response.Length(); i++)
		{
			int code = response.ToAscii(i);
			if (code < 33 || code > 126) return false;
		}
		if (response.Substring(0, 32) != expectedToken || response.Substring(32, 1) != ".") return false;
		string bit = response.Substring(33, 1);
		if (bit == "0") choice = 0;
		else if (bit == "1") choice = 1;
		return choice == 0 || choice == 1;
	}
}

//! Callback только записывает метаданные. Его thread/context и native cancellation пока UNKNOWN.
class RICON_RestControlCapability
{
	protected RestApi m_Api;
	protected RestContext m_Context;
	protected ref RestCallback m_Callback;
	protected BaseWorld m_World;
	protected string m_Run;
	protected string m_Token;
	protected int m_Port;
	protected int m_Generation;
	protected int m_StartedAt;
	protected int m_Last;
	protected int m_RequestAt;
	protected bool m_GetUsed;
	protected bool m_Finished;
	protected bool m_CallbackSeen;
	protected bool m_CallbackSuccess;
	protected bool m_TokenMatch;
	protected int m_Http;
	protected ERestResult m_Result;
	protected int m_CallbackAt;
	protected bool m_RestRehearsal;
	protected bool m_RestCapture;
	protected bool m_NoSend;
	protected bool m_WrongFrequency;
	protected bool m_BlindTrial;
	protected int m_BlindChoice = -1;
	protected string m_Attempt;
	protected int m_ExpectedPreparedGeneration;
	protected bool m_DispatchUsed;
	protected bool GuardedMode() { return m_RestRehearsal || m_RestCapture; }
	static bool Requested()
	{
		string value;
		return System.GetCLIParam("riconRestCapability", value) && value == "1";
	}
	protected static string Bit(bool value)
	{
		if (value)
			return "1";
		return "0";
	}
	protected void Emit(string kind, string fields)
	{
		PrintFormat("[RICON_REST] run=%1 generation=%2 kind=%3 %4", m_Run, m_Generation, kind, fields);
	}
	bool Start(BaseWorld world, int generation)
	{
		if (!Requested() || !world || world.IsEditMode() || generation < 1) return false;
		m_World = world; m_Generation = generation; m_StartedAt = System.GetTickCount(); m_Last = m_StartedAt;
		string port;
		if (!System.GetCLIParam("riconRestRun", m_Run) || !System.GetCLIParam("riconRestToken", m_Token)
			|| !System.GetCLIParam("riconRestPort", port) || !RICON_RestCapabilityGate.Ids(m_Run, m_Token)
			|| !RICON_RestCapabilityGate.Port(port, m_Port))
		{
			m_Run = "UNKNOWN"; Finish("CONFIG_REFUSED"); return false;
		}
		string armValue; string mode; string nrxRun;
		string captureValue;
		m_RestCapture = System.GetCLIParam("riconRestCapturePulse", captureValue);
		m_RestRehearsal = System.GetCLIParam("riconRestRehearsalArm", armValue);
		if (m_RestCapture && m_RestRehearsal) { Finish("GUARDED_MODES_MUTUALLY_EXCLUSIVE"); return false; }
		if (!RICON_CaptureControlGate.Valid()) { Finish("CAPTURE_CONTROL_INVALID_OR_UNBOUND"); return false; }
		m_NoSend = RICON_CaptureControlGate.NoSend();
		m_WrongFrequency = RICON_CaptureControlGate.WrongFrequency();
		if (m_NoSend && !m_RestCapture) { Finish("NO_SEND_REQUIRES_CAPTURE_INGRESS"); return false; }
		if (m_WrongFrequency && !m_RestCapture) { Finish("WRONG_FREQUENCY_REQUIRES_CAPTURE_INGRESS"); return false; }
		if (m_RestRehearsal)
		{
			System.GetCLIParam("riconPulseMode", mode); System.GetCLIParam("riconNativeRxRun", nrxRun);
			System.GetCLIParam("riconPulseAttempt", m_Attempt);
			if (armValue != "1" || !RICON_RestRehearsalGate.Config(true, RICON_LiveNativeRxProbe.IsRequested(), mode, m_Run, nrxRun, m_Attempt))
			{ Finish("REST_REHEARSAL_CONFIG_REFUSED"); return false; }
		}
		if (m_RestCapture)
		{
			System.GetCLIParam("riconPulseMode", mode); System.GetCLIParam("riconNativeRxRun", nrxRun);
			System.GetCLIParam("riconPulseAttempt", m_Attempt);
			if (!RICON_RestCaptureGate.Config(captureValue == "1", Requested(), RICON_LiveNativeRxProbe.IsRequested(), m_RestRehearsal, mode, m_Run, nrxRun, m_Attempt))
			{ Finish("REST_CAPTURE_CONFIG_REFUSED"); return false; }
		}
		string blindValue; string shortValue;
		bool blindConfigured = System.GetCLIParam("riconCaptureBlindTrial", blindValue);
		bool shortConfigured = System.GetCLIParam("riconCaptureShort400", shortValue);
		if (!RICON_CaptureBlindGate.Config(blindConfigured, blindValue, Replication.IsServer(), RICON_CaptureSeparatedGate.Requested(),
			RICON_RestCaptureGate.Requested(), shortConfigured || m_NoSend || m_WrongFrequency || m_RestRehearsal))
		{ Finish("REST_BLIND_CONFIG_REFUSED"); return false; }
		if (blindConfigured && m_Token.Length() != 32) { Finish("REST_BLIND_TOKEN_LENGTH"); return false; }
		m_BlindTrial = blindConfigured;
		string waiting = "startTickMs=" + m_StartedAt.ToString() + ",totalMs=" + RICON_RestRehearsalGate.TotalMs(GuardedMode()).ToString();
		Emit("WAIT_A_ROLE", waiting + ",defaultOff=1");
		return true;
	}
	protected bool Current(BaseWorld world, int generation)
	{
		return world && world == m_World && !world.IsEditMode()
			&& GetGame().GetWorld() == m_World && generation == m_Generation;
	}
	protected void Record(RestCallback callback, bool success)
	{
		if (m_Finished || m_CallbackSeen || !m_GetUsed || callback != m_Callback || !callback) return;
		m_CallbackSuccess = success;
		if (m_BlindTrial) m_TokenMatch = RICON_CaptureBlindGate.Parse(m_Token, callback.GetData(), m_BlindChoice);
		else m_TokenMatch = callback.GetData() == m_Token;
		m_Http = callback.GetHttpCode(); m_Result = callback.GetRestResult();
		m_CallbackAt = System.GetTickCount();
		m_CallbackSeen = true;
	}
	void SuccessEvent(RestCallback callback) { Record(callback, true); }
	void ErrorEvent(RestCallback callback) { Record(callback, false); }
	protected void DispatchRehearsal(BaseWorld world, int generation)
	{
		if (!m_RestRehearsal || m_DispatchUsed || !Current(world, generation)) { Finish("REST_ARM_CONTEXT_REFUSED"); return; }
		if (!RICON_RestRehearsalGate.Timely(true, m_StartedAt, m_RequestAt, m_CallbackAt, System.GetTickCount()))
		{ Finish("LATE_CALLBACK_OR_OBSERVATION"); return; }
		ArmaReforgerScripted game = ArmaReforgerScripted.Cast(GetGame());
		if (!game) { Finish("REST_ARM_GAME_UNAVAILABLE"); return; }
		m_DispatchUsed = true;
		int beforeTick = System.GetTickCount();
		bool accepted = game.RICON_RequestRestRehearsalArm(m_Run, m_Attempt, m_ExpectedPreparedGeneration, m_World);
		int afterTick = System.GetTickCount();
		string fields = "preparedGeneration=" + m_ExpectedPreparedGeneration.ToString() + " beforeTickMs=" + beforeTick.ToString();
		fields = fields + " afterTickMs=" + afterTick.ToString() + " accepted=" + Bit(accepted);
		PrintFormat("[RICON_REST_ARM] run=%1 attempt=%2 kind=DISPATCH %3 guardVerifiedByGame=0", m_Run, m_Attempt, fields);
		if (!accepted) { Finish("REST_REHEARSAL_ARM_REFUSED"); return; }
		Finish("REST_REHEARSAL_ARM_ACCEPTED");
	}
	protected void DispatchCapture(BaseWorld world, int generation)
	{
		if (!m_RestCapture || m_RestRehearsal || m_DispatchUsed || !Current(world, generation))
		{ Finish("REST_CAPTURE_CONTEXT_REFUSED"); return; }
		if (!m_CallbackSeen || RICON_RestCapabilityGate.CallbackResult(m_CallbackSuccess, m_TokenMatch, m_Http, m_Result) != "TOKEN_OBSERVED")
		{ Finish("REST_CAPTURE_REPLY_UNQUALIFIED"); return; }
		if (!RICON_RestRehearsalGate.Timely(true, m_StartedAt, m_RequestAt, m_CallbackAt, System.GetTickCount()))
		{ Finish("LATE_CALLBACK_OR_OBSERVATION"); return; }
		ArmaReforgerScripted game = ArmaReforgerScripted.Cast(GetGame());
		if (!game) { Finish("REST_CAPTURE_GAME_UNAVAILABLE"); return; }
		m_DispatchUsed = true;
		int beforeTick = System.GetTickCount();
		bool accepted;
		if (m_BlindTrial) accepted = game.RICON_RequestRestBlindCaptureArm(m_Run, m_Attempt, m_ExpectedPreparedGeneration, m_World, m_BlindChoice);
		else accepted = game.RICON_RequestRestCaptureArm(m_Run, m_Attempt, m_ExpectedPreparedGeneration, m_World);
		int afterTick = System.GetTickCount();
		string fields = "preparedGeneration=" + m_ExpectedPreparedGeneration.ToString() + " beforeTickMs=" + beforeTick.ToString();
		fields = fields + " afterTickMs=" + afterTick.ToString() + " accepted=" + Bit(accepted);
		if (RICON_DirectCaptureGate.Requested() && RICON_CaptureBPowerOffGate.Requested()) PrintFormat("[RICON_REST_CAPTURE] run=%1 attempt=%2 kind=DISPATCH %3 guardVerifiedByGame=0 control=DIRECT_B_POWER_OFF", m_Run, m_Attempt, fields);
		else if (RICON_DirectCaptureGate.Requested()) PrintFormat("[RICON_REST_CAPTURE] run=%1 attempt=%2 kind=DISPATCH %3 guardVerifiedByGame=0 control=DIRECT", m_Run, m_Attempt, fields);
		else if (RICON_CaptureBPowerOffGate.Requested()) PrintFormat("[RICON_REST_CAPTURE] run=%1 attempt=%2 kind=DISPATCH %3 guardVerifiedByGame=0 control=B_POWER_OFF", m_Run, m_Attempt, fields);
		else if (m_BlindTrial) PrintFormat("[RICON_REST_CAPTURE] run=%1 attempt=%2 kind=DISPATCH %3 guardVerifiedByGame=0 blindTrial=1", m_Run, m_Attempt, fields);
		else if (m_WrongFrequency) PrintFormat("[RICON_REST_CAPTURE] run=%1 attempt=%2 kind=DISPATCH %3 guardVerifiedByGame=0 control=WRONG_FREQUENCY", m_Run, m_Attempt, fields);
		else if (m_NoSend) PrintFormat("[RICON_REST_CAPTURE] run=%1 attempt=%2 kind=DISPATCH %3 guardVerifiedByGame=0 control=NO_SEND", m_Run, m_Attempt, fields);
		else PrintFormat("[RICON_REST_CAPTURE] run=%1 attempt=%2 kind=DISPATCH %3 guardVerifiedByGame=0", m_Run, m_Attempt, fields);
		if (!accepted)
		{
			if (m_WrongFrequency) Finish("REST_CAPTURE_WRONG_FREQUENCY_ARM_REFUSED");
			else if (m_NoSend) Finish("REST_CAPTURE_NO_SEND_ARM_REFUSED"); else Finish("REST_CAPTURE_ARM_REFUSED");
			return;
		}
		if (m_WrongFrequency) Finish("REST_CAPTURE_WRONG_FREQUENCY_ARM_ACCEPTED");
		else if (m_NoSend) Finish("REST_CAPTURE_NO_SEND_ARM_ACCEPTED"); else Finish("REST_CAPTURE_ARM_ACCEPTED");
	}
	void Tick(BaseWorld world, int generation)
	{
		if (m_Finished) return;
		if (!Current(world, generation)) { Finish("WORLD_OR_GENERATION_CHANGED"); return; }
		int now = System.GetTickCount();
		if (now < m_Last || now < m_StartedAt) { Finish("CLOCK_RESET"); return; }
		m_Last = now;
		if (now - m_StartedAt >= RICON_RestRehearsalGate.TotalMs(GuardedMode())) { Finish("TOTAL_TIMEOUT"); return; }
		if (m_GetUsed)
		{
			if (m_CallbackSeen)
			{
				string observed = "success=" + Bit(m_CallbackSuccess) + ",tokenMatch=" + Bit(m_TokenMatch);
				observed = observed + ",http=" + m_Http.ToString() + ",restResult=" + m_Result.ToString();
				observed = observed + ",callbackTickMs=" + m_CallbackAt.ToString() + ",observedTickMs=" + now.ToString();
				Emit("CALLBACK_OBSERVED", observed);
				if (!Current(world, generation)) { Finish("WORLD_OR_GENERATION_CHANGED"); return; }
				if (!RICON_RestRehearsalGate.Timely(GuardedMode(), m_StartedAt, m_RequestAt, m_CallbackAt, System.GetTickCount()))
				{ Finish("LATE_CALLBACK_OR_OBSERVATION"); return; }
				string result = RICON_RestCapabilityGate.CallbackResult(m_CallbackSuccess, m_TokenMatch, m_Http, m_Result);
				if (m_RestCapture && result == "TOKEN_OBSERVED") DispatchCapture(world, generation);
				else if (m_RestRehearsal && result == "TOKEN_OBSERVED") DispatchRehearsal(world, generation);
				else Finish(result);
				return;
			}
			if (now - m_RequestAt > RICON_RestRehearsalGate.CallbackMs(GuardedMode())) Finish("CALLBACK_TIMEOUT");
			return;
		}
		PlayerController player = GetGame().GetPlayerController();
		if (!player || player.GetPlayerId() == 0) return;
		if (player.GetPlayerId() != 1) { Finish("NOT_A_ROLE"); return; }
		ArmaReforgerScripted game = ArmaReforgerScripted.Cast(GetGame());
		if (m_RestRehearsal)
		{
			if (!game) { Finish("REST_ARM_GAME_UNAVAILABLE"); return; }
			m_ExpectedPreparedGeneration = game.RICON_RestRehearsalPreparedGeneration(m_Run, m_Attempt, m_World);
			if (m_ExpectedPreparedGeneration < 1) return;
		}
		if (m_RestCapture)
		{
			if (!game) { Finish("REST_CAPTURE_GAME_UNAVAILABLE"); return; }
			m_ExpectedPreparedGeneration = game.RICON_RestCapturePreparedGeneration(m_Run, m_Attempt, m_World);
			if (m_ExpectedPreparedGeneration < 1) return;
		}
		m_Api = GetGame().GetRestApi();
		if (!m_Api) { Finish("REST_API_UNAVAILABLE"); return; }
		m_Context = m_Api.GetContext(RICON_RestCapabilityGate.BaseUrl(m_Port));
		if (!m_Context) { Finish("REST_CONTEXT_UNAVAILABLE"); return; }
		m_Callback = new RestCallback();
		m_Callback.SetOnSuccess(SuccessEvent); m_Callback.SetOnError(ErrorEvent);
		m_Context.SetTimeout(RICON_RestRehearsalGate.TimeoutS(GuardedMode()));
		if (GuardedMode()) Emit("READY_IN_PLAY", "nativeTimeoutS=50,totalMs=55000,endpoint=LOOPBACK_ONLY,path=FIXED_RUN_PATH");
		else Emit("READY_IN_PLAY", "nativeTimeoutS=5,totalMs=10000,endpoint=LOOPBACK_ONLY,path=FIXED_RUN_PATH");
		now = System.GetTickCount();
		if (m_Finished || !Current(world, generation) || now < m_Last || now - m_StartedAt >= RICON_RestRehearsalGate.TotalMs(GuardedMode()))
		{ Finish("PRE_GET_BINDING_OR_TIMEOUT"); return; }
		if (GetGame().GetPlayerController() != player || player.GetPlayerId() != 1) { Finish("PRE_GET_A_ROLE_CHANGED"); return; }
		if (m_RestRehearsal && !RICON_RestRehearsalGate.PreparedMatches(m_ExpectedPreparedGeneration,
			game.RICON_RestRehearsalPreparedGeneration(m_Run, m_Attempt, m_World))) { Finish("PRE_GET_PREPARED_CHANGED"); return; }
		if (m_RestCapture && !RICON_RestRehearsalGate.PreparedMatches(m_ExpectedPreparedGeneration,
			game.RICON_RestCapturePreparedGeneration(m_Run, m_Attempt, m_World))) { Finish("PRE_GET_CAPTURE_PREPARED_CHANGED"); return; }
		m_GetUsed = true; m_RequestAt = now;
		int returned = m_Context.GET(m_Callback, RICON_RestCapabilityGate.Request(m_Run));
		Emit("GET_RETURN", "nativeReturn=" + returned.ToString() + ",requestTickMs=" + m_RequestAt.ToString() + ",completion=NOT_PROVEN");
		if (m_RestRehearsal && !m_Finished)
		{
			string pending = "preparedGeneration=" + m_ExpectedPreparedGeneration.ToString() + " worldGeneration=" + m_Generation.ToString();
			pending = pending + " requestTickMs=" + m_RequestAt.ToString();
			PrintFormat("[RICON_REST_ARM] run=%1 attempt=%2 kind=PENDING %3", m_Run, m_Attempt, pending);
		}
		if (m_RestCapture && !m_Finished)
		{
			string capturePending = "preparedGeneration=" + m_ExpectedPreparedGeneration.ToString() + " worldGeneration=" + m_Generation.ToString();
			capturePending = capturePending + " requestTickMs=" + m_RequestAt.ToString();
			if (m_NoSend) capturePending = capturePending + " control=NO_SEND";
			else if (m_WrongFrequency) capturePending = capturePending + " control=WRONG_FREQUENCY";
			if (m_BlindTrial) capturePending = capturePending + " blindTrial=1";
			if (RICON_DirectCaptureGate.Requested() && RICON_CaptureBPowerOffGate.Requested()) capturePending = capturePending + " control=DIRECT_B_POWER_OFF";
			else if (RICON_CaptureBPowerOffGate.Requested()) capturePending = capturePending + " control=B_POWER_OFF";
			else if (RICON_DirectCaptureGate.Requested()) capturePending = capturePending + " control=DIRECT";
			PrintFormat("[RICON_REST_CAPTURE] run=%1 attempt=%2 kind=PENDING %3", m_Run, m_Attempt, capturePending);
		}
	}
	void Finish(string result)
	{
		if (m_Finished) return;
		m_Finished = true;
		string fields = "result=" + result + ",getUsed=" + Bit(m_GetUsed);
		fields = fields + ",callbackSeen=" + Bit(m_CallbackSeen) + ",tokenMatch=" + Bit(m_TokenMatch);
		fields = fields + ",http=" + m_Http.ToString() + ",restResult=" + m_Result.ToString();
		fields = fields + ",qualification=CONTROL_ONLY,media=NOT_TESTED,cancel=UNKNOWN";
		Emit("TERMINAL", fields);
		if (m_WrongFrequency && result != "REST_CAPTURE_WRONG_FREQUENCY_ARM_ACCEPTED") RICON_CaptureFrequencyControl.RestoreOwned();
		m_World = null; m_Token = "";
		// Один bounded tombstone удерживает callback до конца процесса; cancel/reset/delete API не вызываются.
	}
}

//! Отдельный lifecycle, один attempt на процесс/VM, без изменения FHC/Pulse.
modded class ArmaReforgerScripted
{
	protected ref RICON_RestControlCapability m_RICON_RestCapability;
	protected static ref RICON_RestControlCapability s_RICON_RestTombstone;
	protected int m_RICON_RestGeneration;
	override bool OnGameStart()
	{
		bool started = super.OnGameStart();
		BaseWorld world = GetWorld();
		if (!started || !RICON_RestControlCapability.Requested() || !world || world.IsEditMode()) return started;
		if (s_RICON_RestTombstone) return started;
		m_RICON_RestGeneration++;
		m_RICON_RestCapability = new RICON_RestControlCapability();
		s_RICON_RestTombstone = m_RICON_RestCapability;
		if (!m_RICON_RestCapability.Start(world, m_RICON_RestGeneration)) m_RICON_RestCapability = null;
		return started;
	}
	override void OnUpdate(BaseWorld world, float timeslice)
	{
		super.OnUpdate(world, timeslice);
		if (m_RICON_RestCapability) m_RICON_RestCapability.Tick(world, m_RICON_RestGeneration);
	}
	override void OnGameEnd()
	{
		if (m_RICON_RestCapability) m_RICON_RestCapability.Finish("GAME_END");
		m_RICON_RestCapability = null;
		super.OnGameEnd();
	}
	override protected void OnBeforeWorldCleanup()
	{
		if (m_RICON_RestCapability) m_RICON_RestCapability.Finish("WORLD_CLEANUP");
		m_RICON_RestCapability = null;
		super.OnBeforeWorldCleanup();
	}
}
