//! Synthetic resolver exercises the real routing boundary; it never invokes SelectRouting or native setters.
class RICON_CapturePulseRoutingSynthetic : RICON_LiveCapturePulseProbe
{
	ref RICON_CapturePulseSource m_Actual;
	int m_iResolves;
	void Prepare()
	{
		m_State = new RICON_CapturePulseState(); m_State.Arm(100);
		m_Source = new RICON_CapturePulseSource(); m_Source.m_sEntityRpl = "SYNTHETIC_A";
		m_Source.m_PreviousMethod = ECommMethod.DIRECT;
		m_Actual = new RICON_CapturePulseSource(); m_Actual.m_sEntityRpl = "SYNTHETIC_A";
		m_Actual.m_PreviousMethod = ECommMethod.DIRECT;
	}
	override protected int Now() { return 110; }
	override protected bool ResolveSource(int now, bool requirePulseBudget, out RICON_CapturePulseSource source, out string reason)
	{
		m_iResolves++; source = m_Actual; reason = "SYNTHETIC_RESOLVER"; return true;
	}
	bool CheckBoundary(ECommMethod method) { return RoutingBoundary(method, null); }
}

//! Game-module pure state checks; no world, microphone, native setters or guard process.
class RICON_CapturePulseSelfChecks
{
	protected int m_iChecks;
	protected int m_iPassed;
	protected void Check(bool value, string name)
	{
		m_iChecks++; if (value) m_iPassed++; else PrintFormat("[RICON_PULSECHECK] mismatch=%1", name);
	}
	void Run()
	{
		Check(RICON_CapturePulseGate.Mode(false, "capture") == RICON_ECapturePulseMode.OFF, "default_off");
		Check(RICON_CapturePulseGate.Mode(true, "true") == RICON_ECapturePulseMode.OFF, "strict_mode");
		Check(RICON_CapturePulseGate.Mode(true, "rehearsal") == RICON_ECapturePulseMode.REHEARSAL, "rehearsal_mode");
		Check(RICON_CapturePulseGate.Mode(true, "capture") == RICON_ECapturePulseMode.CAPTURE, "capture_mode");
		Check(RICON_CapturePulseGate.INTENDED_MS == 1000 && RICON_CapturePulseGate.CUTOFF_MS == 1500
			&& RICON_CapturePulseGate.VALIDATION_MS == 2000 && RICON_CapturePulseGate.MIN_TRIGGER_REMAINING_MS == 1100, "fixed_contract");
		RICON_CapturePulseState normal = new RICON_CapturePulseState();
		Check(!normal.BeginRequest(100, true), "no_auto_start");
		Check(normal.Arm(100) && !normal.Arm(101), "one_arm");
		Check(normal.BeginRequest(120, true) && normal.StartUsed() && normal.InsideNative(), "one_use_before_true");
		Check(!normal.BeginRequest(121, true), "no_second_true");
		normal.StartReturned(125, true);
		Check(!normal.NeedsStop(1119, true) && normal.NeedsStop(1120, true), "intended_1000");
		Check(normal.BeginStop(1120, true) && normal.StopUsed() && normal.InsideNative(), "one_stop");
		normal.StopReturned(1125, true);
		Check(normal.Finished() && normal.Result(false) == "CAPTURE_REQUEST_CYCLE_OBSERVED", "request_cycle_not_release");
		Check(normal.Result(true) == "REHEARSAL_ONLY", "rehearsal_tier");
		Check(!normal.BeginStop(1126, true) && !normal.BeginRequest(1126, true), "no_retry");
		RICON_CapturePulseState rejectedStart = new RICON_CapturePulseState();
		rejectedStart.Arm(100); rejectedStart.BeginRequest(110, true); rejectedStart.StartReturned(111, false);
		Check(rejectedStart.NeedsStop(111, true) && rejectedStart.BeginStop(111, true), "false_start_still_stop");
		rejectedStart.StopReturned(112, true);
		Check(rejectedStart.Result(false) == "START_REQUEST_REJECTED", "start_false_classified");
		RICON_CapturePulseState rejectedStop = new RICON_CapturePulseState();
		rejectedStop.Arm(100); rejectedStop.BeginRequest(110, true); rejectedStop.StartReturned(111, true);
		rejectedStop.RequestStop("SYNTHETIC_CLEANUP"); rejectedStop.BeginStop(120, true); rejectedStop.StopReturned(121, false);
		Check(rejectedStop.Result(false) == "STOP_REQUEST_REJECTED", "stop_false_classified");
		RICON_CapturePulseState stale = new RICON_CapturePulseState();
		stale.Arm(100); stale.BeginRequest(110, true); stale.StartReturned(111, true);
		Check(stale.NeedsStop(112, false) && !stale.BeginStop(112, false), "rebind_never_stops_new_component");
		Check(stale.Invalid() && stale.Finished() && !stale.StopUsed(), "stale_teardown_only");
		RICON_CapturePulseState eligibilityLost = new RICON_CapturePulseState();
		eligibilityLost.Arm(100); eligibilityLost.BeginRequest(110, true); eligibilityLost.StartReturned(111, true);
		Check(eligibilityLost.NeedsStop(112, false) && eligibilityLost.BeginStop(112, true), "eligibility_loss_can_stop_owned_binding");
		eligibilityLost.StopReturned(113, true);
		RICON_CapturePulseState reentrant = new RICON_CapturePulseState();
		reentrant.Arm(100); reentrant.BeginRequest(110, true); reentrant.RequestStop("SYNTHETIC_REENTRANT");
		Check(!reentrant.BeginStop(111, true) && !reentrant.StopUsed(), "defer_nested_false");
		reentrant.StartReturned(112, true);
		Check(reentrant.NeedsStop(112, true) && reentrant.BeginStop(112, true), "stop_after_return");
		reentrant.StopReturned(113, true);
		Check(reentrant.Finished() && !reentrant.Invalid(), "deferred_stop_complete");
		RICON_CapturePulseState late = new RICON_CapturePulseState();
		late.Arm(100);
		Check(!late.CanBegin(201, true) && late.CanBegin(200, true), "pre_routing_late_veto");
		Check(!late.BeginRequest(201, true) && late.Invalid() && !late.StartUsed(), "late_start_no_pulse");
		RICON_CapturePulseState boundary = new RICON_CapturePulseState(); boundary.Arm(100);
		Check(boundary.BeginRequest(200, true), "local_slop_boundary"); boundary.StartReturned(201, true);
		RICON_CapturePulseState stall = new RICON_CapturePulseState();
		stall.Arm(100); stall.BeginRequest(110, true); stall.StartReturned(111, true);
		Check(stall.NeedsStop(1600, true) && stall.Invalid(), "local_stall_invalid");
		stall.BeginStop(1600, true); stall.StopReturned(1601, true);
		Check(stall.Result(false) == "INVALID", "late_stop_not_extended");
		RICON_CapturePulseState clock = new RICON_CapturePulseState(); clock.Arm(100);
		Check(!clock.BeginRequest(99, true) && clock.Invalid() && !clock.StartUsed(), "backward_clock");
		Check(!clock.BeginRequest(110, true), "no_clock_retry");
		RICON_CapturePulseState badContext = new RICON_CapturePulseState(); badContext.Arm(100);
		Check(!badContext.BeginRequest(110, false) && badContext.Finished(), "precall_context_veto");
		RICON_CapturePulseState overflow = new RICON_CapturePulseState(); overflow.Arm(100);
		overflow.RejectBeforeRequest("SYNTHETIC_LOG_OVERFLOW");
		Check(overflow.Invalid() && !overflow.BeginRequest(110, true), "overflow_no_start");
		RICON_CapturePulseRoutingSynthetic routing = new RICON_CapturePulseRoutingSynthetic(); routing.Prepare();
		Check(routing.CheckBoundary(ECommMethod.DIRECT), "fresh_prior_route");
		routing.m_Actual.m_PreviousMethod = ECommMethod.SQUAD_RADIO;
		Check(routing.CheckBoundary(ECommMethod.SQUAD_RADIO), "expected_transitional_route");
		routing.m_Actual.m_sEntityRpl = "SYNTHETIC_REBOUND";
		Check(!routing.CheckBoundary(ECommMethod.SQUAD_RADIO), "rebind_between_routing_steps_vetoed");
		routing.m_Actual.m_sEntityRpl = "SYNTHETIC_A"; routing.m_Actual.m_PreviousMethod = ECommMethod.DIRECT;
		Check(!routing.CheckBoundary(ECommMethod.SQUAD_RADIO), "unexpected_route_vetoed");
		Check(routing.m_iResolves == 4, "each_boundary_resolves_fresh_context");
		Check(RICON_CapturePulseGate.FactionPolicy(true, false, "RADIO", "OTHER") == "CROSS_FACTION", "cross_faction_exemption");
		Check(RICON_CapturePulseGate.FactionPolicy(false, true, "RADIO", "OTHER") == "ADMIN", "admin_exemption");
		Check(RICON_CapturePulseGate.FactionPolicy(false, false, "RADIO", "") == "EMPTY_LOCAL_KEY", "empty_local_key_permitted");
		Check(RICON_CapturePulseGate.FactionPolicy(false, false, "", "") == "EMPTY_LOCAL_KEY", "empty_radio_and_local_keys_permitted");
		Check(RICON_CapturePulseGate.FactionPolicy(false, false, "RADIO", "OTHER") == "LOCAL_KEY_MISMATCH", "mismatch_veto");
		Check(RICON_CapturePulseGate.FactionPolicy(false, false, "", "LOCAL") == "LOCAL_KEY_MISMATCH", "empty_radio_nonempty_local_veto");
		Check(RICON_CapturePulseGate.FactionPolicy(false, false, "MATCH", "MATCH") == "MATCHED_LOCAL_KEY", "matching_key_permitted");
		string retained = RICON_CapturePulseGate.EffectiveLocalKey("CACHED", false, "");
		Check(retained == "CACHED" && RICON_CapturePulseGate.FactionPolicy(false, false, "OTHER", retained) == "LOCAL_KEY_MISMATCH", "null_faction_preserves_nonempty_cache");
		string empty = RICON_CapturePulseGate.EffectiveLocalKey("", false, "");
		Check(RICON_CapturePulseGate.FactionPolicy(false, false, "RADIO", empty) == "EMPTY_LOCAL_KEY", "null_faction_empty_cache_permitted");
		Check(RICON_CapturePulseGate.EffectiveLocalKey("STALE", true, "FACTION") == "FACTION", "known_faction_overrides_stale_cache");
		Check(RICON_CapturePulseGate.EffectiveLocalKey("STALE", true, "").IsEmpty(), "known_empty_faction_key_overrides_cache");
		RICON_CapturePulseSource snapshot = new RICON_CapturePulseSource();
		RICON_CapturePulseSource comparison = new RICON_CapturePulseSource();
		snapshot.m_sPrivateKey = "RADIO"; comparison.m_sPrivateKey = "RADIO";
		snapshot.m_sPrivateEffectiveLocalKey = "LOCAL"; comparison.m_sPrivateEffectiveLocalKey = "LOCAL";
		Check(!snapshot.m_Faction && snapshot.SameCore(comparison), "null_faction_snapshot_can_match");
		comparison.m_sPrivateKey = "CHANGED";
		Check(!snapshot.SameCore(comparison), "radio_key_drift_veto");
		comparison.m_sPrivateKey = "RADIO"; comparison.m_sPrivateEffectiveLocalKey = "CHANGED";
		Check(!snapshot.SameCore(comparison), "effective_local_key_drift_veto");
		snapshot.ClearPrivateKeys();
		Check(snapshot.m_sPrivateKey.IsEmpty() && snapshot.m_sPrivateEffectiveLocalKey.IsEmpty(), "clear_both_private_snapshots");
		PrintFormat("[RICON_PULSECHECK] synthetic=1 checks=%1 passed=%2 nativeCalls=0 content=NOT_TESTED", m_iChecks, m_iPassed);
		if (m_iChecks == 52 && m_iPassed == m_iChecks) Print("[RICON_PULSECHECK] done: PASS");
		else Print("[RICON_PULSECHECK] done: MISMATCH");
	}
}

//! Synthetic проверяет только выбор таймера и неизменные ограничения; native вызовов нет.
class RICON_CaptureShort400SelfChecks
{
	protected int m_Checks;
	protected int m_Passed;
	protected void Check(bool ok, string name)
	{
		m_Checks++;
		if (ok) m_Passed++;
		else PrintFormat("[RICON_PULSECHECK] scope=SHORT400 fail=%1", name);
	}
	void Run()
	{
		Check(RICON_CaptureShort400Gate.Config(false, "", false, false, false, false, true), "default_unchanged");
		Check(RICON_CaptureShort400Gate.Config(true, "1", true, true, true, true, false), "exact_separated_guarded_A");
		Check(!RICON_CaptureShort400Gate.Config(true, "400", true, true, true, true, false), "no_generic_duration");
		Check(!RICON_CaptureShort400Gate.Config(true, "1", false, true, true, true, false), "B_forbidden");
		Check(!RICON_CaptureShort400Gate.Config(true, "1", true, false, true, true, false), "ordinary_or_passive_forbidden");
		Check(!RICON_CaptureShort400Gate.Config(true, "1", true, true, false, true, false), "rehearsal_forbidden");
		Check(!RICON_CaptureShort400Gate.Config(true, "1", true, true, true, false, false), "guarded_ingress_required");
		Check(!RICON_CaptureShort400Gate.Config(true, "1", true, true, true, true, true), "other_controls_forbidden");
		RICON_CapturePulseState shortPulse = new RICON_CapturePulseState();
		Check(shortPulse.UseShort400() && !shortPulse.UseShort400(), "fixed_selection_once_before_arm");
		shortPulse.Arm(100); shortPulse.BeginRequest(110, true); shortPulse.StartReturned(111, true);
		Check(shortPulse.StartRequestTick() == 110 && shortPulse.LastReturnTick() == 111
			&& !shortPulse.NeedsStop(509, true) && shortPulse.NeedsStop(510, true), "request_based_400_boundary");
		bool stopped = shortPulse.BeginStop(510, true); shortPulse.StopReturned(511, true);
		Check(stopped && shortPulse.LastReturnTick() == 511 && shortPulse.Result(false) == "CAPTURE_REQUEST_CYCLE_OBSERVED", "short_stop_and_real_return_ticks");
		Check(!shortPulse.UseShort400(), "cannot_reconfigure_after_arm");
		RICON_CapturePulseState normal = new RICON_CapturePulseState();
		normal.Arm(100); normal.BeginRequest(110, true); normal.StartReturned(111, true);
		Check(!normal.NeedsStop(1109, true) && normal.NeedsStop(1110, true), "default_1000_boundary");
		RICON_CapturePulseState late = new RICON_CapturePulseState(); late.UseShort400(); late.Arm(100);
		Check(late.CanBegin(200, true) && !late.BeginRequest(201, true), "start_slop_not_extended");
		RICON_CapturePulseState stale = new RICON_CapturePulseState(); stale.UseShort400(); stale.Arm(100);
		stale.BeginRequest(110, true); stale.StartReturned(111, true); stale.NeedsStop(510, true);
		Check(!stale.BeginStop(510, false) && stale.Invalid() && !stale.StopUsed(), "stale_owned_binding_no_false");
		RICON_CapturePulseState stall = new RICON_CapturePulseState(); stall.UseShort400(); stall.Arm(100);
		stall.BeginRequest(110, true); stall.StartReturned(111, true);
		Check(stall.NeedsStop(1600, true) && stall.Invalid(), "guard_overrun_not_short_success");
		RICON_CapturePulseState rollback = new RICON_CapturePulseState(); rollback.UseShort400(); rollback.Arm(100);
		rollback.BeginRequest(110, true); rollback.StartReturned(109, true);
		bool startReturnExact = rollback.LastReturnTick() == 109;
		bool rollbackStop = rollback.BeginStop(112, true); rollback.StopReturned(108, true);
		Check(startReturnExact && rollbackStop && rollback.LastReturnTick() == 108 && rollback.Invalid(), "backward_return_arguments_preserved");
		PrintFormat("[RICON_PULSECHECK] scope=SHORT400 synthetic=1 checks=%1 passed=%2 nativeCalls=0 content=NOT_TESTED", m_Checks, m_Passed);
		if (m_Checks == 17 && m_Passed == m_Checks) Print("[RICON_PULSECHECK] scope=SHORT400 done: PASS");
		else Print("[RICON_PULSECHECK] scope=SHORT400 done: MISMATCH");
	}
}

//! Actual SourceCurrent/SourceAtRoute, только synthetic resolver без native setters.
class RICON_DirectRoutingSynthetic : RICON_CapturePulseRoutingSynthetic
{
	void Direct(bool enabled) { m_bDirect = enabled; }
	bool Selected() { return SourceCurrent(110, true); }
}

class RICON_DirectPulseChecks
{
	protected int m_Checks;
	protected int m_Passed;
	protected void Check(bool ok, string name)
	{
		m_Checks++;
		if (ok) m_Passed++;
		else PrintFormat("[RICON_PULSECHECK] scope=DIRECT mismatch=%1", name);
	}
	void Run()
	{
		Check(RICON_DirectCaptureGate.Route(true, true), "DIRECT_and_null_TX");
		Check(!RICON_DirectCaptureGate.Route(false, true), "null_TX_alone_not_DIRECT");
		Check(!RICON_DirectCaptureGate.Route(true, false), "DIRECT_with_radio_TX_not_selected");
		Check(!RICON_DirectCaptureGate.Route(false, false), "radio_route_not_DIRECT");
		RICON_DirectRoutingSynthetic direct = new RICON_DirectRoutingSynthetic(); direct.Prepare(); direct.Direct(true);
		Check(direct.Selected(), "actual_SourceCurrent_DIRECT_branch");
		direct.m_Actual.m_PreviousMethod = ECommMethod.SQUAD_RADIO;
		Check(!direct.Selected(), "actual_SQUAD_getter_rejected");
		direct.m_Actual.m_PreviousMethod = ECommMethod.DIRECT; direct.m_Actual.m_sEntityRpl = "SYNTHETIC_REBOUND";
		Check(!direct.Selected(), "source_rebind_rejected");
		RICON_DirectRoutingSynthetic normal = new RICON_DirectRoutingSynthetic(); normal.Prepare(); normal.Direct(false);
		Check(!normal.Selected(), "default_still_expects_SQUAD");
		RICON_CapturePulseState ownedState = new RICON_CapturePulseState(); ownedState.Arm(100); ownedState.BeginRequest(110, true); ownedState.StartReturned(111, true);
		Check(ownedState.NeedsStop(120, false) && ownedState.BeginStop(120, true), "B_near_loss_does_not_block_owned_A_stop");
		ownedState.StopReturned(121, true);
		Check(!ownedState.BeginStop(122, true), "owned_stop_once");
		RICON_CapturePulseState rebound = new RICON_CapturePulseState(); rebound.Arm(100); rebound.BeginRequest(110, true); rebound.StartReturned(111, true);
		Check(!rebound.BeginStop(120, false), "foreign_A_binding_no_false");
		Check(RICON_CapturePulseGate.INTENDED_MS == 1000 && RICON_CapturePulseGate.CUTOFF_MS == 1500 && RICON_CapturePulseGate.VALIDATION_MS == 2000, "existing_timing_unchanged");
		PrintFormat("[RICON_PULSECHECK] scope=DIRECT synthetic=1 checks=%1 passed=%2 nativeCaptureCalls=0", m_Checks, m_Passed);
		if (m_Checks == 12 && m_Passed == m_Checks) Print("[RICON_PULSECHECK] scope=DIRECT done: PASS");
		else Print("[RICON_PULSECHECK] scope=DIRECT done: MISMATCH");
	}
}
