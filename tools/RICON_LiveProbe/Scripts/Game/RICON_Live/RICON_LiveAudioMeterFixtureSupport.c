//! Explicit Game-module pure checks. No engine sound/getter invocation or native callback injection.
class RICON_AudioMeterSelfChecks
{
	protected int m_iChecks;
	protected int m_iPassed;
	protected void Check(bool value) { m_iChecks++; if (value) m_iPassed++; }
	void Run()
	{
		Check(!RICON_AudioMeterGate.Requested(false, "1", false));
		Check(!RICON_AudioMeterGate.Requested(true, "true", false));
		Check(!RICON_AudioMeterGate.Requested(true, "1", true));
		Check(RICON_AudioMeterGate.Requested(true, "1", false));
		Check(!RICON_AudioMeterGate.RoleAllowed(1) && RICON_AudioMeterGate.RoleAllowed(2) && !RICON_AudioMeterGate.RoleAllowed(3));
		Check(!RICON_AudioMeterGate.ClockGap(100, 5100) && RICON_AudioMeterGate.ClockGap(100, 5101));
		Check(RICON_AudioMeterGate.ClockGap(100, 99) && RICON_AudioMeterGate.ClockGap(-1, 0));
		Check(!RICON_AudioMeterGate.Expired(59999, 60000) && RICON_AudioMeterGate.Expired(60000, 60000));
		Check(RICON_AudioMeterGate.Expired(-1, 60000));
		Check(!RICON_AudioMeterGate.Expired(39999, 40000) && RICON_AudioMeterGate.Expired(40000, 40000));
		Check(!RICON_AudioMeterGate.Expired(9999, 10000) && RICON_AudioMeterGate.Expired(10000, 10000));
		Check(RICON_AudioMeterGate.SampleDue(-1, 0));
		Check(!RICON_AudioMeterGate.SampleDue(100, 149) && RICON_AudioMeterGate.SampleDue(100, 150));
		Check(!RICON_AudioMeterGate.SampleDue(100, 99) && !RICON_AudioMeterGate.SampleDue(-1, -1));
		Check(RICON_AudioMeterGate.OrdinaryAllowed(511) && !RICON_AudioMeterGate.OrdinaryAllowed(512));
		Check(!RICON_AudioMeterGate.OrdinaryAllowed(-1));
		RICON_AudioMeterEventState eventState = new RICON_AudioMeterEventState();
		Check(!eventState.Begin(-1) && !eventState.m_bRequested);
		Check(!eventState.Started(RICON_EAudioMeterPhase.EVENT_WINDOW, true, true, 7));
		Check(eventState.Begin(1000) && eventState.Outstanding());
		Check(!eventState.Begin(1001) && eventState.m_iRequestedAt == 1000);
		Check(!eventState.Started(RICON_EAudioMeterPhase.BASELINE, true, true, 7));
		Check(!eventState.Started(RICON_EAudioMeterPhase.EVENT_WINDOW, false, true, 7));
		Check(!eventState.Started(RICON_EAudioMeterPhase.EVENT_WINDOW, true, false, 7));
		Check(!eventState.Started(RICON_EAudioMeterPhase.EVENT_WINDOW, true, true, AudioHandle.Invalid));
		Check(!eventState.Finished(RICON_EAudioMeterPhase.EVENT_WINDOW, 1001, true, true, 7, false));
		// Immediate start+finish after Begin models synchronous ordering, not a native callback.
		Check(eventState.Started(RICON_EAudioMeterPhase.EVENT_WINDOW, true, true, 7));
		Check(!eventState.Started(RICON_EAudioMeterPhase.EVENT_WINDOW, true, true, 8) && eventState.m_Handle == 7);
		Check(!eventState.Finished(RICON_EAudioMeterPhase.EVENT_WINDOW, 999, true, true, 7, false));
		Check(!eventState.Finished(RICON_EAudioMeterPhase.EVENT_WINDOW, 1001, true, true, 8, false));
		Check(!eventState.Finished(RICON_EAudioMeterPhase.EVENT_WINDOW, 1001, true, true, 7, true));
		Check(!eventState.Finished(RICON_EAudioMeterPhase.EVENT_WINDOW, 1001, false, true, 7, false));
		Check(!eventState.Finished(RICON_EAudioMeterPhase.EVENT_WINDOW, 1001, true, false, 7, false));
		Check(eventState.Finished(RICON_EAudioMeterPhase.EVENT_WINDOW, 1001, true, true, 7, false) && !eventState.Outstanding());
		Check(eventState.m_iFinishedAt == 1001 && eventState.m_iStarted == 1 && eventState.m_iFinished == 1);
		Check(!eventState.Finished(RICON_EAudioMeterPhase.EVENT_WINDOW, 1002, true, true, 7, false));
		Check(!eventState.Begin(2000) && !eventState.Outstanding());
		Check(!eventState.DeferStop("OUTSIDE_CALL") && eventState.m_sDeferredStop.IsEmpty());
		eventState.m_bInNativeCall = true;
		Check(eventState.DeferStop("FIRST_REASON") && eventState.m_sDeferredStop == "FIRST_REASON");
		Check(eventState.DeferStop("LATER_REASON") && eventState.m_sDeferredStop == "FIRST_REASON");
		Check(!eventState.NeedsGap(eventState.m_sDeferredStop));
		eventState.MarkOverflow();
		Check(eventState.DeferStop("OVERFLOW") && eventState.m_sDeferredStop == "FIRST_REASON" && eventState.NeedsGap(eventState.m_sDeferredStop));
		eventState.m_bInNativeCall = false;
		Check(!eventState.DeferStop("AFTER_RETURN") && eventState.m_sDeferredStop == "FIRST_REASON");
		Check(eventState.m_bOverflowSeen && eventState.NeedsGap("AFTER_RETURN"));
		RICON_LiveJournal journal = new RICON_LiveJournal(516); journal.SetStream(false);
		for (int i = 0; i < 512; i++) journal.Add(i, "synthetic", "PURE", -1, -1, -1, i, "energy=NOT_TESTED");
		for (int r = 0; r < 4; r++) journal.Add(512 + r, "synthetic", "RESERVED", -1, -1, -1, r, "energy=NOT_TESTED");
		Check(journal.Count() == 516 && journal.Dropped() == 0 && journal.NextSeq() == 516);
		PrintFormat("[RICON_METERCHECK] pure=1 checks=%1 passed=%2 native=NOT_RUN,energy=UNKNOWN,playback=NOT_TESTED", m_iChecks, m_iPassed);
		if (m_iChecks == 44 && m_iPassed == m_iChecks) Print("[RICON_METERCHECK] done: PASS");
		else Print("[RICON_METERCHECK] done: MISMATCH");
	}
}

//! Отдельные pure local boundaries; прежние44 checks выше не изменены.
class RICON_AudioMeterLocalSelfChecks
{
	protected int m_Checks;
	protected int m_Passed;
	protected void Check(bool ok, string name)
	{
		m_Checks++;
		if (ok) m_Passed++;
		else PrintFormat("[RICON_METERCHECK] scope=LOCAL_CALIBRATION mismatch=%1", name);
	}
	void Run()
	{
		Check(!RICON_AudioMeterLocalGate.Requested(false, true, "1"), "main_meter_required");
		Check(!RICON_AudioMeterLocalGate.Requested(true, false, "1"), "local_default_off");
		Check(!RICON_AudioMeterLocalGate.Requested(true, true, "true"), "local_exact_value");
		Check(!RICON_AudioMeterLocalGate.Requested(true, true, "0"), "local_zero_off");
		Check(RICON_AudioMeterLocalGate.Requested(true, true, "1"), "local_explicit_on");
		Check(!RICON_AudioMeterLocalGate.RoleAllowed(false, 1, 0, true, false)
			&& RICON_AudioMeterLocalGate.RoleAllowed(false, 2, 0, true, false), "legacy_role_stays_b_only");
		Check(RICON_AudioMeterLocalGate.RoleAllowed(true, 1, 0, true, false), "real_local_standalone");
		Check(!RICON_AudioMeterLocalGate.RoleAllowed(true, 2, 0, true, false), "local_player2_refused");
		Check(!RICON_AudioMeterLocalGate.RoleAllowed(true, 0, 0, true, false), "local_missing_player_refused");
		Check(!RICON_AudioMeterLocalGate.RoleAllowed(true, 1, 1, true, false), "local_network_server_refused");
		Check(!RICON_AudioMeterLocalGate.RoleAllowed(true, 1, 2, true, false), "local_network_client_refused");
		Check(!RICON_AudioMeterLocalGate.RoleAllowed(true, 1, 0, false, false), "local_server_flag_required");
		Check(!RICON_AudioMeterLocalGate.RoleAllowed(true, 1, 0, true, true), "local_client_flag_refused");
		Check(RICON_AudioMeterLocalGate.RplAllowed(true, false, false), "local_unknown_rpl_uses_refs");
		Check(!RICON_AudioMeterLocalGate.RplAllowed(false, false, true), "network_unknown_rpl_refused");
		Check(RICON_AudioMeterLocalGate.RplAllowed(false, true, true), "network_rpl_proof_preserved");
		Check(!RICON_AudioMeterLocalGate.RplAllowed(false, true, false), "network_rpl_change_refused");
		Check(RICON_AudioMeterLocalGate.RefsCurrent(true, true, true, true, true), "exact_local_refs");
		Check(!RICON_AudioMeterLocalGate.RefsCurrent(false, true, true, true, true), "changed_world_refused");
		Check(!RICON_AudioMeterLocalGate.RefsCurrent(true, false, true, true, true), "changed_controller_refused");
		Check(!RICON_AudioMeterLocalGate.RefsCurrent(true, true, false, true, true), "changed_entity_refused");
		Check(!RICON_AudioMeterLocalGate.RefsCurrent(true, true, true, false, true), "changed_communication_refused");
		Check(!RICON_AudioMeterLocalGate.RefsCurrent(true, true, true, true, false), "changed_invokers_refused");
		PrintFormat("[RICON_METERCHECK] scope=LOCAL_CALIBRATION pure=1 checks=%1 passed=%2 native=NOT_RUN", m_Checks, m_Passed);
		if (m_Checks == 23 && m_Checks == m_Passed) Print("[RICON_METERCHECK] scope=LOCAL_CALIBRATION done: PASS");
		else Print("[RICON_METERCHECK] scope=LOCAL_CALIBRATION done: MISMATCH");
	}
}
