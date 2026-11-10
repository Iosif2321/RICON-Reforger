//! Synthetic pure gates; сеть, Game и native callbacks не вызываются.
class RICON_RestCapabilityChecks
{
	protected int m_Checks;
	protected int m_Passed;
	protected void Check(bool ok, string name)
	{
		m_Checks++;
		if (ok) m_Passed++;
		else PrintFormat("[RICON_RESTCHECK] mismatch=%1", name);
	}
	void Run()
	{
		int port;
		Check(RICON_RestCapabilityGate.Port("1024", port) && port == 1024, "port_lower_bound");
		Check(RICON_RestCapabilityGate.Port("65535", port) && port == 65535, "port_upper_bound");
		Check(!RICON_RestCapabilityGate.Port("1023", port), "low_port_refused");
		Check(!RICON_RestCapabilityGate.Port("65536", port), "high_port_refused");
		Check(!RICON_RestCapabilityGate.Port("80/path", port), "port_url_injection_refused");
		Check(!RICON_RestCapabilityGate.Port("-1234", port), "signed_port_refused");
		Check(RICON_RestCapabilityGate.Ids("run-1", "public_token"), "ascii_ids");
		Check(!RICON_RestCapabilityGate.Ids("../escape", "token") && !RICON_RestCapabilityGate.Ids("..", "token"), "path_segments_refused");
		Check(!RICON_RestCapabilityGate.Ids("run", "token with space"), "whitespace_token_refused");
		Check(RICON_RestCapabilityGate.BaseUrl(12345) == "http://127.0.0.1:12345/", "fixed_loopback");
		Check(RICON_RestCapabilityGate.Request("run-1") == "ricon-control-capability/run-1", "fixed_request_path");
		Check(RICON_RestCapabilityGate.Timely(100, 200, 5200, 5201), "callback_5000_boundary");
		Check(!RICON_RestCapabilityGate.Timely(100, 200, 5201, 5201), "late_callback_refused");
		Check(!RICON_RestCapabilityGate.Timely(100, 200, 300, 10100), "total_10000_boundary_refused");
		Check(!RICON_RestCapabilityGate.Timely(100, 200, 199, 300), "callback_before_request_refused");
		Check(!RICON_RestCapabilityGate.Timely(100, 200, 300, 299), "clock_before_callback_refused");
		Check(RICON_RestCapabilityGate.CallbackResult(true, true, 200, ERestResult.EREST_SUCCESS) == "TOKEN_OBSERVED", "success_requires_callback_metadata");
		Check(RICON_RestCapabilityGate.CallbackResult(false, true, 200, ERestResult.EREST_SUCCESS) == "ERROR_CALLBACK", "error_event_never_passes");
		Check(RICON_RestCapabilityGate.CallbackResult(false, false, 0, ERestResult.EREST_ERROR_TIMEOUT) == "NATIVE_TIMEOUT", "native_timeout_classified");
		Check(RICON_RestCapabilityGate.CallbackResult(true, true, 404, ERestResult.EREST_SUCCESS) == "HTTP_MISMATCH", "http_mismatch_refused");
		Check(RICON_RestCapabilityGate.CallbackResult(true, true, 200, ERestResult.EREST_ERROR) == "REST_RESULT_MISMATCH", "rest_error_refused");
		Check(RICON_RestCapabilityGate.CallbackResult(true, false, 200, ERestResult.EREST_SUCCESS) == "TOKEN_MISMATCH", "token_mismatch_refused");
		PrintFormat("[RICON_RESTCHECK] synthetic=1 checks=%1 passed=%2 nativeRequests=0", m_Checks, m_Passed);
		if (m_Checks == 22 && m_Checks == m_Passed) Print("[RICON_RESTCHECK] done: PASS");
		else Print("[RICON_RESTCHECK] done: MISMATCH");
	}
}

//! Частотные/control gates, без native radio объектов/setters или callback injection.
class RICON_CaptureWrongFrequencyChecks
{
	protected int m_Checks;
	protected int m_Passed;
	protected void Check(bool ok, string name)
	{
		m_Checks++;
		if (ok) m_Passed++;
		else PrintFormat("[RICON_RESTCHECK] scope=CAPTURE_WRONG_FREQUENCY mismatch=%1", name);
	}
	void Run()
	{
		Check(RICON_CaptureControlGate.Config(false, "", true, "1"), "wrong_explicit_on");
		Check(!RICON_CaptureControlGate.Config(true, "1", true, "1"), "controls_mutually_exclusive");
		Check(!RICON_CaptureControlGate.Config(false, "", true, "true"), "wrong_exact_value");
		Check(!RICON_CaptureControlGate.Config(false, "", true, "0"), "wrong_zero_not_fallback");
		Check(RICON_CaptureFrequencyGate.Target(32000, 32000, 32000, 68000, 200) == 32200, "legal_up_step");
		Check(RICON_CaptureFrequencyGate.Target(68000, 68000, 32000, 68000, 200) == 67800, "legal_down_step");
		Check(RICON_CaptureFrequencyGate.Target(32000, 48000, 32000, 68000, 200) == 48000, "already_mismatching_unchanged");
		Check(RICON_CaptureFrequencyGate.Target(32000, 32000, 32000, 68000, 0) == -1, "unknown_step_refused");
		Check(RICON_CaptureFrequencyGate.Target(32000, 32000, 32000, 32000, 200) == -1, "no_legal_alternate_refused");
		Check(RICON_CaptureFrequencyGate.Target(2147483600, 2147483600, 2147483600, 2147483647, 100) == -1, "overflow_avoided");
		array<int> absent;
		array<int> empty = {};
		array<int> wrong = {32200, 48000};
		array<int> leaking = {32200, 32000};
		array<int> unknown = {-1, 48000};
		Check(!RICON_CaptureFrequencyGate.AllMismatch(absent, 32000), "missing_catalog_not_clear");
		Check(!RICON_CaptureFrequencyGate.AllMismatch(empty, 32000), "empty_catalog_not_clear");
		Check(RICON_CaptureFrequencyGate.AllMismatch(wrong, 32000), "all_b_mismatch");
		Check(!RICON_CaptureFrequencyGate.AllMismatch(leaking, 32000), "one_matching_b_refused");
		Check(!RICON_CaptureFrequencyGate.AllMismatch(unknown, 32000), "unknown_frequency_not_clear");
		Check(RICON_CaptureFrequencyGate.CanRestore(true, 32200, 32000, 32200), "own_control_value_restore_allowed");
		Check(RICON_CaptureFrequencyGate.CanRestore(true, 32000, 32000, 32200), "already_original_no_write");
		Check(!RICON_CaptureFrequencyGate.CanRestore(true, 32600, 32000, 32200), "foreign_value_no_clobber");
		Check(!RICON_CaptureFrequencyGate.CanRestore(false, 32200, 32000, 32200), "changed_refs_no_restore");
		Check(RICON_CaptureFrequencyGate.CanRestore(true, 48000, 48000, 48000), "unchanged_extra_restores_by_readback");
		PrintFormat("[RICON_RESTCHECK] scope=CAPTURE_WRONG_FREQUENCY synthetic=1 checks=%1 passed=%2 nativeRequests=0 actualCaptureCalls=0", m_Checks, m_Passed);
		if (m_Checks == 20 && m_Checks == m_Passed) Print("[RICON_RESTCHECK] scope=CAPTURE_WRONG_FREQUENCY done: PASS");
		else Print("[RICON_RESTCHECK] scope=CAPTURE_WRONG_FREQUENCY done: MISMATCH");
	}
}

//! NoSend scheduler/config только; actual CAPTURE routing проверяется root runtime отдельно.
class RICON_CaptureNoSendChecks
{
	protected int m_Checks;
	protected int m_Passed;
	protected void Check(bool ok, string name)
	{
		m_Checks++;
		if (ok) m_Passed++;
		else PrintFormat("[RICON_RESTCHECK] scope=CAPTURE_NO_SEND mismatch=%1", name);
	}
	void Run()
	{
		Check(RICON_CaptureControlGate.Config(false, "", false), "positive_default_unchanged");
		Check(RICON_CaptureControlGate.Config(true, "1", false), "exact_no_send_on");
		Check(!RICON_CaptureControlGate.Config(true, "true", false), "no_send_wrong_value");
		Check(!RICON_CaptureControlGate.Config(true, "0", false), "no_send_zero_not_positive_fallback");
		Check(!RICON_CaptureControlGate.Config(false, "", true), "wrong_frequency_unbound_refusal");
		Check(!RICON_CaptureControlGate.Config(true, "1", true), "controls_mutually_exclusive");
		Check(RICON_CaptureControlGate.NativeAllowed(true, false), "positive_capture_unchanged");
		Check(!RICON_CaptureControlGate.NativeAllowed(false, false), "rehearsal_stays_non_native");
		Check(!RICON_CaptureControlGate.NativeAllowed(true, true), "no_send_blocks_true_and_false");
		Check(!RICON_CaptureControlGate.NativeAllowed(false, true), "no_send_cannot_enable_capture");
		Check(RICON_CaptureControlGate.Result(false, "CAPTURE_REQUEST_CYCLE_OBSERVED") == "CAPTURE_REQUEST_CYCLE_OBSERVED", "positive_terminal_unchanged");
		Check(RICON_CaptureControlGate.Result(true, "CAPTURE_REQUEST_CYCLE_OBSERVED") == "NO_SEND_CONTROL_CYCLE_OBSERVED", "no_send_separate_terminal");
		Check(RICON_CaptureControlGate.Result(true, "INVALID") == "INVALID", "no_send_does_not_hide_invalid");
		Check(RICON_CaptureControlGate.Result(true, "NOT_REQUESTED") == "NOT_REQUESTED", "no_send_does_not_fake_completed");
		RICON_CapturePulseState window = new RICON_CapturePulseState();
		window.Arm(100); window.BeginRequest(110, true); window.StartReturned(111, true);
		Check(!window.NeedsStop(1109, true) && window.NeedsStop(1110, true), "same_1000ms_window");
		Check(!window.BeginRequest(1110, true) && window.BeginStop(1110, true), "one_scheduler_start_stop");
		window.StopReturned(1111, true);
		Check(RICON_CaptureControlGate.Result(true, window.Result(false)) == "NO_SEND_CONTROL_CYCLE_OBSERVED"
			&& window.Reason() == "INTENDED_STOP", "complete_state_only_cycle");
		RICON_CapturePulseState invalid = new RICON_CapturePulseState();
		invalid.Arm(100); invalid.BeginRequest(110, true); invalid.StartReturned(111, true);
		invalid.NeedsStop(112, false); invalid.BeginStop(112, true); invalid.StopReturned(113, true);
		Check(RICON_CaptureControlGate.Result(true, invalid.Result(false)) == "INVALID", "changed_context_invalid");
		RICON_CapturePulseState late = new RICON_CapturePulseState(); late.Arm(100);
		Check(!late.BeginRequest(201, true) && RICON_CaptureControlGate.Result(true, late.Result(false)) == "INVALID", "late_start_invalid");
		RICON_CapturePulseState rebound = new RICON_CapturePulseState();
		rebound.Arm(100); rebound.BeginRequest(110, true); rebound.StartReturned(111, true);
		Check(!rebound.BeginStop(112, false) && !rebound.StopUsed() && rebound.Invalid(), "rebound_owned_stop_not_weakened");
		PrintFormat("[RICON_RESTCHECK] scope=CAPTURE_NO_SEND synthetic=1 checks=%1 passed=%2 nativeRequests=0 actualCaptureCalls=0", m_Checks, m_Passed);
		if (m_Checks == 20 && m_Checks == m_Passed) Print("[RICON_RESTCHECK] scope=CAPTURE_NO_SEND done: PASS");
		else Print("[RICON_RESTCHECK] scope=CAPTURE_NO_SEND done: MISMATCH");
	}
}

//! Проверяется strict capture wrapper, но synthetic RequestArm никогда не вызывает native Pulse.
class RICON_RestCaptureArmSynthetic : RICON_LiveCapturePulseProbe
{
	int ObservedGeneration;
	int ArmCalls;
	string LastMode;
	override int RestCapturePreparedGeneration(string run, string attempt, BaseWorld expectedWorld)
	{
		return ObservedGeneration;
	}
	override bool RequestArm(string run, string attempt, string mode)
	{
		ArmCalls++; LastMode = mode;
		return mode == "capture";
	}
}

class RICON_RestCaptureChecks
{
	protected int m_Checks;
	protected int m_Passed;
	protected void Check(bool ok, string name)
	{
		m_Checks++;
		if (ok) m_Passed++;
		else PrintFormat("[RICON_RESTCHECK] scope=REST_CAPTURE mismatch=%1", name);
	}
	void Run()
	{
		Check(RICON_RestCaptureGate.Config(true, true, true, false, "capture", "run", "run", "attempt"), "explicit_capture_config");
		Check(!RICON_RestCaptureGate.Config(false, true, true, false, "capture", "run", "run", "attempt"), "capture_default_off");
		Check(!RICON_RestCaptureGate.Config(true, false, true, false, "capture", "run", "run", "attempt"), "rest_capability_required");
		Check(!RICON_RestCaptureGate.Config(true, true, false, false, "capture", "run", "run", "attempt"), "native_rx_required");
		Check(!RICON_RestCaptureGate.Config(true, true, true, true, "capture", "run", "run", "attempt"), "rehearsal_exclusive");
		Check(!RICON_RestCaptureGate.Config(true, true, true, false, "rehearsal", "run", "run", "attempt"), "rehearsal_mode_not_capture");
		Check(!RICON_RestCaptureGate.Config(true, true, true, false, "off", "run", "run", "attempt"), "off_mode_refused");
		Check(!RICON_RestCaptureGate.Config(true, true, true, false, "capture", "run", "other", "attempt"), "capture_run_mismatch");
		Check(!RICON_RestCaptureGate.Config(true, true, true, false, "capture", "run", "run", ""), "capture_attempt_required");
		Check(RICON_RestCaptureGate.LocalContext(true, 1), "capture_local_a_world");
		Check(!RICON_RestCaptureGate.LocalContext(false, 1), "changed_capture_world_refused");
		Check(!RICON_RestCaptureGate.LocalContext(true, 2), "capture_b_role_refused");
		Check(!RICON_RestCaptureGate.LocalContext(true, 0), "capture_missing_role_refused");
		Check(RICON_RestRehearsalGate.PreparedMatches(4, 4), "capture_generation_same");
		Check(!RICON_RestRehearsalGate.PreparedMatches(4, 5), "capture_generation_changed");
		Check(!RICON_RestRehearsalGate.PreparedMatches(0, 4), "capture_generation_unknown");
		Check(RICON_RestRehearsalGate.TotalMs(true) == 55000 && RICON_RestRehearsalGate.CallbackMs(true) == 50000
			&& RICON_RestRehearsalGate.TimeoutS(true) == 50, "capture_guarded_budget");
		Check(RICON_RestRehearsalGate.TotalMs(false) == 10000 && RICON_RestRehearsalGate.CallbackMs(false) == 5000
			&& RICON_RestRehearsalGate.TimeoutS(false) == 5, "ordinary_budget_unchanged");
		Check(RICON_RestRehearsalGate.Timely(true, 100, 200, 50200, 50201), "capture_callback_boundary");
		Check(!RICON_RestRehearsalGate.Timely(true, 100, 200, 50201, 50201), "capture_callback_late");
		RICON_RestCaptureArmSynthetic acceptedCapture = new RICON_RestCaptureArmSynthetic(); acceptedCapture.ObservedGeneration = 4;
		Check(acceptedCapture.RequestRestCaptureArm("run", "attempt", 4, null) && acceptedCapture.ArmCalls == 1
			&& acceptedCapture.LastMode == "capture", "strict_capture_literal_mode");
		Check(!acceptedCapture.RequestRestCaptureArm("run", "attempt", 4, null) && acceptedCapture.ArmCalls == 1, "capture_duplicate_refused");
		RICON_RestCaptureArmSynthetic stale = new RICON_RestCaptureArmSynthetic(); stale.ObservedGeneration = 5;
		Check(!stale.RequestRestCaptureArm("run", "attempt", 4, null) && stale.ArmCalls == 0, "capture_stale_no_arm");
		stale.ObservedGeneration = 4;
		Check(!stale.RequestRestCaptureArm("run", "attempt", 4, null) && stale.ArmCalls == 0, "capture_refusal_consumes_once");
		RICON_RestCaptureArmSynthetic missing = new RICON_RestCaptureArmSynthetic();
		Check(!missing.RequestRestCaptureArm("run", "attempt", 4, null) && missing.ArmCalls == 0, "capture_missing_context_no_arm");
		RICON_CapturePulseState safeStop = new RICON_CapturePulseState();
		safeStop.Arm(100); safeStop.BeginRequest(110, true); safeStop.StartReturned(111, true);
		Check(safeStop.NeedsStop(112, false) && safeStop.BeginStop(112, true) && safeStop.StopUsed(), "eligibility_lost_owned_stop_allowed");
		safeStop.StopReturned(113, true);
		Check(!safeStop.BeginStop(114, true), "owned_stop_only_once");
		RICON_CapturePulseState rebound = new RICON_CapturePulseState();
		rebound.Arm(100); rebound.BeginRequest(110, true); rebound.StartReturned(111, true);
		Check(!rebound.BeginStop(112, false) && rebound.Finished() && !rebound.StopUsed(), "rebound_stop_refused");
		Check(!rebound.BeginStop(113, true), "rebound_no_stop_retry");
		RICON_CapturePulseState rejected = new RICON_CapturePulseState();
		rejected.Arm(100); rejected.BeginRequest(110, true); rejected.StartReturned(111, false);
		Check(rejected.NeedsStop(112, true) && rejected.BeginStop(112, true), "rejected_true_still_owned_stop");
		RICON_CapturePulseState nested = new RICON_CapturePulseState();
		nested.Arm(100); nested.BeginRequest(110, true); nested.RequestStop("SYNTHETIC_REENTRANCY");
		Check(!nested.BeginStop(111, true) && !nested.StopUsed(), "capture_no_nested_false");
		nested.StartReturned(112, true);
		Check(nested.NeedsStop(113, true) && nested.BeginStop(113, true), "capture_deferred_false_after_return");
		RICON_CapturePulseState late = new RICON_CapturePulseState(); late.Arm(100);
		Check(late.CanBegin(200, true) && !late.CanBegin(201, true), "capture_existing_start_slop");
		RICON_CapturePulseState reset = new RICON_CapturePulseState(); reset.Arm(100);
		Check(!reset.BeginRequest(99, true) && reset.Invalid() && !reset.StartUsed(), "capture_backward_clock_no_true");
		PrintFormat("[RICON_RESTCHECK] scope=REST_CAPTURE synthetic=1 checks=%1 passed=%2 nativeRequests=0 actualCaptureCalls=0", m_Checks, m_Passed);
		if (m_Checks == 34 && m_Checks == m_Passed) Print("[RICON_RESTCHECK] scope=REST_CAPTURE done: PASS");
		else Print("[RICON_RESTCHECK] scope=REST_CAPTURE done: MISMATCH");
	}
}

//! Synthetic seam проверяет actual strict one-use Pulse wrapper без world/native/capture вызовов.
class RICON_RestPulseArmSynthetic : RICON_LiveCapturePulseProbe
{
	int ObservedGeneration;
	int ArmCalls;
	override int RestRehearsalPreparedGeneration(string run, string attempt, BaseWorld expectedWorld)
	{
		return ObservedGeneration;
	}
	override bool RequestArm(string run, string attempt, string mode)
	{
		ArmCalls++;
		return mode == "rehearsal";
	}
}

class RICON_RestRehearsalChecks
{
	protected int m_Checks;
	protected int m_Passed;
	protected void Check(bool ok, string name)
	{
		m_Checks++;
		if (ok) m_Passed++;
		else PrintFormat("[RICON_RESTCHECK] scope=REST_REHEARSAL mismatch=%1", name);
	}
	void Run()
	{
		Check(RICON_RestRehearsalGate.Config(true, true, "rehearsal", "run", "run", "attempt"), "explicit_rehearsal_config");
		Check(!RICON_RestRehearsalGate.Config(false, true, "rehearsal", "run", "run", "attempt"), "default_off");
		Check(!RICON_RestRehearsalGate.Config(true, false, "rehearsal", "run", "run", "attempt"), "native_rx_required");
		Check(!RICON_RestRehearsalGate.Config(true, true, "capture", "run", "run", "attempt"), "capture_mode_refused");
		Check(!RICON_RestRehearsalGate.Config(true, true, "off", "run", "run", "attempt"), "off_mode_refused");
		Check(!RICON_RestRehearsalGate.Config(true, true, "rehearsal", "run", "other", "attempt"), "run_mismatch_refused");
		Check(!RICON_RestRehearsalGate.Config(true, true, "rehearsal", "run", "run", ""), "attempt_required");
		Check(RICON_RestRehearsalGate.PreparedMatches(4, 4), "same_prepared_generation");
		Check(!RICON_RestRehearsalGate.PreparedMatches(0, 4), "zero_expected_refused");
		Check(!RICON_RestRehearsalGate.PreparedMatches(4, 0), "missing_context_refused");
		Check(!RICON_RestRehearsalGate.PreparedMatches(4, 5), "prepared_rebind_refused");
		Check(RICON_RestRehearsalGate.TotalMs(false) == 10000 && RICON_RestRehearsalGate.CallbackMs(false) == 5000
			&& RICON_RestRehearsalGate.TimeoutS(false) == 5, "capability_budgets_unchanged");
		Check(RICON_RestRehearsalGate.TotalMs(true) == 55000 && RICON_RestRehearsalGate.CallbackMs(true) == 50000
			&& RICON_RestRehearsalGate.TimeoutS(true) == 50, "guarded_pending_budgets");
		Check(RICON_RestRehearsalGate.Timely(true, 100, 200, 50200, 50201), "guarded_callback_boundary");
		Check(!RICON_RestRehearsalGate.Timely(true, 100, 200, 50201, 50201), "guarded_late_callback_refused");
		Check(!RICON_RestRehearsalGate.Timely(true, 100, 200, 300, 55100), "guarded_total_boundary_refused");
		Check(!RICON_RestRehearsalGate.Timely(false, 100, 200, 5201, 5201), "capability_deadline_not_extended");
		RICON_RestPulseArmSynthetic accepted = new RICON_RestPulseArmSynthetic(); accepted.ObservedGeneration = 4;
		Check(accepted.RequestRestRehearsalArm("run", "attempt", 4, null) && accepted.ArmCalls == 1, "strict_wrapper_calls_rehearsal_once");
		Check(!accepted.RequestRestRehearsalArm("run", "attempt", 4, null) && accepted.ArmCalls == 1, "duplicate_no_second_arm");
		RICON_RestPulseArmSynthetic stale = new RICON_RestPulseArmSynthetic(); stale.ObservedGeneration = 5;
		Check(!stale.RequestRestRehearsalArm("run", "attempt", 4, null) && stale.ArmCalls == 0, "stale_generation_no_arm");
		stale.ObservedGeneration = 4;
		Check(!stale.RequestRestRehearsalArm("run", "attempt", 4, null) && stale.ArmCalls == 0, "refusal_consumes_one_use");
		RICON_RestPulseArmSynthetic missing = new RICON_RestPulseArmSynthetic();
		Check(!missing.RequestRestRehearsalArm("run", "attempt", 4, null) && missing.ArmCalls == 0, "invalid_context_no_arm");
		PrintFormat("[RICON_RESTCHECK] scope=REST_REHEARSAL synthetic=1 checks=%1 passed=%2 nativeRequests=0 actualCaptureCalls=0", m_Checks, m_Passed);
		if (m_Checks == 22 && m_Checks == m_Passed) Print("[RICON_RESTCHECK] scope=REST_REHEARSAL done: PASS");
		else Print("[RICON_RESTCHECK] scope=REST_REHEARSAL done: MISMATCH");
	}
}

//! Seam исполняет actual strict blind wrapper, заменяя только native source lookup и Arm.
class RICON_BlindPulseArmSynthetic : RICON_LiveCapturePulseProbe
{
	int ObservedGeneration = 4;
	int ArmCalls;
	bool ArmAccepted = true;
	void Prepare()
	{
		m_bBlindTrial = true; m_State = new RICON_CapturePulseState();
		m_sRun = "run"; m_sAttempt = "attempt"; m_sMode = "capture";
		m_Mode = RICON_ECapturePulseMode.CAPTURE; m_bReady = true;
	}
	override int RestCapturePreparedGeneration(string run, string attempt, BaseWorld expectedWorld)
	{
		if (run != "run" || attempt != "attempt") return 0;
		return ObservedGeneration;
	}
	override bool RequestArm(string run, string attempt, string mode)
	{
		ArmCalls++;
		if (!ArmAccepted) return false;
		return mode == "capture" && m_State.Arm(100);
	}
	bool Selected() { return m_bBlindChoiceUsed; }
	bool DirectArm() { return super.RequestArm("run", "attempt", "capture"); }
	void PreArm() { m_State.Arm(100); }
	bool TimerBoundary(int stopAt)
	{
		if (!m_State.BeginRequest(110, true)) return false;
		m_State.StartReturned(111, true);
		return !m_State.NeedsStop(stopAt - 1, true) && m_State.NeedsStop(stopAt, true);
	}
}

class RICON_CaptureBlindChecks
{
	protected int m_Checks;
	protected int m_Passed;
	protected void Check(bool ok, string name)
	{
		m_Checks++;
		if (ok) m_Passed++;
		else PrintFormat("[RICON_RESTCHECK] scope=BLIND_TRIAL mismatch=%1", name);
	}
	void Run()
	{
		string token = "0123456789abcdef0123456789abcdef";
		int choice;
		Check(RICON_CaptureBlindGate.Parse(token, token + ".0", choice) && choice == 0, "strict_short_body");
		Check(RICON_CaptureBlindGate.Parse(token, token + ".1", choice) && choice == 1, "strict_long_body");
		Check(!RICON_CaptureBlindGate.Parse(token, "1123456789abcdef0123456789abcdef.0", choice) && choice == -1, "wrong_token_no_long_fallback");
		Check(!RICON_CaptureBlindGate.Parse(token, token + ":0", choice) && choice == -1, "literal_separator");
		Check(!RICON_CaptureBlindGate.Parse(token, token + ".2", choice) && choice == -1, "unknown_bit_no_long_fallback");
		Check(!RICON_CaptureBlindGate.Parse(token, "", choice), "empty_body");
		Check(!RICON_CaptureBlindGate.Parse(token, token, choice), "token_only_invalid_in_blind");
		Check(!RICON_CaptureBlindGate.Parse(token, token + ".0\n", choice), "no_newline_suffix");
		Check(!RICON_CaptureBlindGate.Parse(token, " " + token + ".0", choice), "no_leading_space");
		Check(!RICON_CaptureBlindGate.Parse(token, token + ".00", choice), "one_bit_only");
		Check(!RICON_CaptureBlindGate.Parse(token.Substring(0, 31), token + ".0", choice), "expected_token_exact32");
		Check(!RICON_CaptureBlindGate.Parse(token, token + ".0" + token, choice), "oversize_refused");
		Check(!RICON_CaptureBlindGate.Parse(token, token + ".\t", choice) && choice == -1, "ASCII_control_refused");
		Check(RICON_CaptureBlindGate.Config(false, "", false, false, false, true), "default_paths_unchanged");
		Check(RICON_CaptureBlindGate.Config(true, "1", true, true, true, false), "explicit_A_separated_guarded");
		Check(!RICON_CaptureBlindGate.Config(true, "true", true, true, true, false), "exact_flag");
		Check(!RICON_CaptureBlindGate.Config(true, "1", false, true, true, false), "B_refused");
		Check(!RICON_CaptureBlindGate.Config(true, "1", true, false, true, false), "ordinary_passive_refused");
		Check(!RICON_CaptureBlindGate.Config(true, "1", true, true, false, false), "rehearsal_unguarded_refused");
		Check(!RICON_CaptureBlindGate.Config(true, "1", true, true, true, true), "known_short_other_controls_refused");
		RICON_BlindPulseArmSynthetic shortPulse = new RICON_BlindPulseArmSynthetic(); shortPulse.Prepare();
		Check(!shortPulse.Selected() && !shortPulse.DirectArm(), "no_startup_choice_or_UI_bypass");
		Check(shortPulse.RequestRestBlindCaptureArm("run", "attempt", 4, null, 0) && shortPulse.ArmCalls == 1, "late_short_applied_before_arm");
		Check(shortPulse.TimerBoundary(510), "short_actual_state_timer");
		Check(!shortPulse.RequestRestBlindCaptureArm("run", "attempt", 4, null, 1) && shortPulse.ArmCalls == 1, "no_replay_or_choice_switch");
		RICON_BlindPulseArmSynthetic longPulse = new RICON_BlindPulseArmSynthetic(); longPulse.Prepare();
		Check(longPulse.RequestRestBlindCaptureArm("run", "attempt", 4, null, 1) && longPulse.TimerBoundary(1110), "long_actual_default_timer");
		RICON_BlindPulseArmSynthetic invalid = new RICON_BlindPulseArmSynthetic(); invalid.Prepare();
		Check(!invalid.RequestRestBlindCaptureArm("run", "attempt", 4, null, -1) && !invalid.Selected() && invalid.ArmCalls == 0, "unknown_choice_no_arm");
		Check(!invalid.RequestRestBlindCaptureArm("run", "attempt", 4, null, 1) && invalid.ArmCalls == 0, "unknown_refusal_consumes_once");
		RICON_BlindPulseArmSynthetic stale = new RICON_BlindPulseArmSynthetic(); stale.Prepare(); stale.ObservedGeneration = 5;
		Check(!stale.RequestRestBlindCaptureArm("run", "attempt", 4, null, 0) && !stale.Selected(), "stale_generation_before_selection");
		stale.ObservedGeneration = 4;
		Check(!stale.RequestRestBlindCaptureArm("run", "attempt", 4, null, 0) && stale.ArmCalls == 0, "stale_refusal_no_retry");
		RICON_BlindPulseArmSynthetic wrongRun = new RICON_BlindPulseArmSynthetic(); wrongRun.Prepare();
		Check(!wrongRun.RequestRestBlindCaptureArm("other", "attempt", 4, null, 0) && !wrongRun.Selected(), "run_binding");
		RICON_BlindPulseArmSynthetic wrongAttempt = new RICON_BlindPulseArmSynthetic(); wrongAttempt.Prepare();
		Check(!wrongAttempt.RequestRestBlindCaptureArm("run", "other", 4, null, 0) && !wrongAttempt.Selected(), "attempt_binding");
		RICON_BlindPulseArmSynthetic prearmed = new RICON_BlindPulseArmSynthetic(); prearmed.Prepare(); prearmed.PreArm();
		Check(!prearmed.RequestRestBlindCaptureArm("run", "attempt", 4, null, 0) && !prearmed.Selected(), "choice_cannot_follow_arm");
		RICON_BlindPulseArmSynthetic refused = new RICON_BlindPulseArmSynthetic(); refused.Prepare(); refused.ArmAccepted = false;
		Check(!refused.RequestRestBlindCaptureArm("run", "attempt", 4, null, 0) && !refused.DirectArm()
			&& !refused.RequestRestBlindCaptureArm("run", "attempt", 4, null, 1) && refused.ArmCalls == 1, "arm_refusal_no_later_UI_retry");
		Check(!RICON_RestRehearsalGate.Timely(true, 100, 200, 50201, 50201), "late_reply_no_dispatch");
		PrintFormat("[RICON_RESTCHECK] scope=BLIND_TRIAL synthetic=1 checks=%1 passed=%2 nativeRequests=0 actualCaptureCalls=0", m_Checks, m_Passed);
		if (m_Checks == 34 && m_Passed == m_Checks) Print("[RICON_RESTCHECK] scope=BLIND_TRIAL done: PASS");
		else Print("[RICON_RESTCHECK] scope=BLIND_TRIAL done: MISMATCH");
	}
}
