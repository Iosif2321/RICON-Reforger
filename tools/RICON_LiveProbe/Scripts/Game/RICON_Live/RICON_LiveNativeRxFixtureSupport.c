//! TOOL_ONLY explicit synthetic self-check; Game-module inheritance is resolved before module sealing. Never auto-started.
//! Synthetic forwarding and pure checks only; no native invocation or receive/delivery PASS.
class RICON_NativeRxSyntheticSink : RICON_LiveNativeRxProbe
{
	int m_iReceive;
	int m_iCapture;
	int m_iUsed;
	int m_iNullRx;
	int m_iNullTx;
	int m_iLastOwner;
	int m_iLastSender;
	override void Receive(RICON_NativeRxTap tap, int sender, BaseTransceiver receiver, int frequency, float quality)
	{
		m_iReceive++; m_iLastOwner = tap.OwnerPlayer(); m_iLastSender = sender;
		if (!receiver) m_iNullRx++;
	}
	override void Capture(RICON_NativeRxTap tap, BaseTransceiver transmitter)
	{
		m_iCapture++; if (!transmitter) m_iNullTx++;
	}
	override void Used(RICON_NativeRxTap tap, int sender) { m_iUsed++; m_iLastSender = sender; }
}

//! Численные test-geometry/freshness guards, без native placement/capture/clock.
class RICON_CaptureSeparatedSelfChecks
{
	protected int m_Checks;
	protected int m_Passed;
	protected void Check(bool ok, string name)
	{
		m_Checks++;
		if (ok) m_Passed++;
		else PrintFormat("[RICON_NRXCHECK] scope=CAPTURE_SEPARATED mismatch=%1", name);
	}
	void Run()
	{
		vector a = Vector(128, 1, 128); vector b = Vector(28, 1, 128);
		Check(RICON_CaptureSeparatedGate.Eligible(a, b, a, 1, 1, 1), "server_geometry_current");
		Check(RICON_CaptureSeparatedGate.Eligible(a, b, b, 2, 1, 1), "b_local_geometry_current");
		Check(!RICON_CaptureSeparatedGate.Eligible(Vector(129, 1, 128), b, a, 1, 1, 1), "a_drift_refused");
		Check(!RICON_CaptureSeparatedGate.Eligible(a, a, a, 1, 1, 1), "b_not_moved_refused");
		Check(RICON_CaptureSeparatedGate.Eligible(a, Vector(30, 1, 128), a, 1, 1, 1), "bounded_search_geometry_edge");
		Check(!RICON_CaptureSeparatedGate.Eligible(a, Vector(30.01, 1, 128), a, 1, 1, 1), "b_outside_search_refused");
		Check(!RICON_CaptureSeparatedGate.Eligible(Vector(128, 2, 128), b, a, 1, 1, 1), "a_surface_drift_refused");
		Check(!RICON_CaptureSeparatedGate.Eligible(a, Vector(28, 0, 128), b, 2, 1, 1), "b_surface_drift_refused");
		Check(!RICON_CaptureSeparatedGate.Eligible(a, b, Vector(129, 1, 128), 1, 1, 1), "a_radio_not_current_refused");
		Check(!RICON_CaptureSeparatedGate.Eligible(a, b, Vector(29, 1, 128), 2, 1, 1), "b_radio_not_current_refused");
		Check(!RICON_CaptureSeparatedGate.Eligible(a, b, a, 0, 1, 1), "unknown_role_refused");
		Check(!RICON_CaptureSeparatedGate.Eligible(Vector(127.5, 1, 128), Vector(30, 1, 128), Vector(127.5, 1, 128), 1, 1, 1), "distance_outside_test_band_refused");
		Check(RICON_CaptureSeparatedGate.Fresh(100, 600), "freshness_boundary");
		Check(!RICON_CaptureSeparatedGate.Fresh(100, 601), "stale_snapshot_refused");
		Check(!RICON_CaptureSeparatedGate.Fresh(100, 99), "future_snapshot_refused");
		Check(!RICON_CaptureSeparatedGate.Fresh(-1, 100), "unknown_snapshot_refused");
		RICON_SeparatedPairGeometry unknown = new RICON_SeparatedPairGeometry();
		Check(!unknown.SameIdentity(unknown), "unknown_identity_not_equivalent");
		PrintFormat("[RICON_NRXCHECK] scope=CAPTURE_SEPARATED synthetic=1 checks=%1 passed=%2 native=NOT_RUN", m_Checks, m_Passed);
		if (m_Checks == 17 && m_Checks == m_Passed) Print("[RICON_NRXCHECK] scope=CAPTURE_SEPARATED done: PASS");
		else Print("[RICON_NRXCHECK] scope=CAPTURE_SEPARATED done: MISMATCH");
	}
}

//! Только pure placement guards/one-use; world/teleport/native не вызываются.
class RICON_PairPlacementSelfChecks
{
	protected int m_Checks;
	protected int m_Passed;
	protected void Check(bool ok, string name)
	{
		m_Checks++;
		if (ok) m_Passed++;
		else PrintFormat("[RICON_NRXCHECK] scope=PAIR_PLACEMENT mismatch=%1", name);
	}
	void Run()
	{
		Check(!RICON_PairPlacementGate.Exact(false, "1"), "default_off");
		Check(!RICON_PairPlacementGate.Exact(true, "true"), "exact_value");
		Check(RICON_PairPlacementGate.Exact(true, "1"), "explicit_on");
		Check(RICON_PairPlacementGate.WorldAllowed("{96A8AF57260A7392}worlds/MP/MpTest/MpTest.ent"), "exact_mptest_guid");
		Check(!RICON_PairPlacementGate.WorldAllowed("worlds/MP/Other/MpTest.ent"), "other_world_refused");
		Check(RICON_PairPlacementGate.OwnerAllowed(2, 2, true, false, true, true, true, true), "local_b_owner_context");
		Check(!RICON_PairPlacementGate.OwnerAllowed(1, 1, true, false, true, true, true, true), "a_cannot_move");
		Check(!RICON_PairPlacementGate.OwnerAllowed(2, 1, true, false, true, true, true, true), "local_id_mismatch");
		Check(!RICON_PairPlacementGate.OwnerAllowed(2, 2, false, true, true, true, true, true), "server_observer_only");
		Check(!RICON_PairPlacementGate.OwnerAllowed(2, 2, true, false, false, true, true, true), "changed_refs_refused");
		Check(!RICON_PairPlacementGate.OwnerAllowed(2, 2, true, false, true, false, true, true), "dead_refused");
		Check(!RICON_PairPlacementGate.OwnerAllowed(2, 2, true, false, true, true, false, true), "vehicle_refused");
		Check(!RICON_PairPlacementGate.OwnerAllowed(2, 2, true, false, true, true, true, false), "attached_parent_refused");
		RICON_PairPlacementAttempt attempt = new RICON_PairPlacementAttempt();
		Check(attempt.Begin(true) && attempt.Used, "one_shot_before_setter");
		Check(!attempt.Begin(true), "no_retry_after_attempt");
		RICON_PairPlacementAttempt invalid = new RICON_PairPlacementAttempt();
		Check(!invalid.Begin(false) && !invalid.Begin(true), "final_context_refusal_consumes");
		Check(RICON_PairPlacementGate.Finite(Vector(28, 1, 128)), "finite_candidate");
		Check(RICON_PairPlacementGate.NearCandidateXZ(Vector(28, 1, 128)), "server_candidate_center");
		Check(RICON_PairPlacementGate.NearCandidateXZ(Vector(30, 10, 128)), "server_candidate_xz_boundary");
		Check(!RICON_PairPlacementGate.NearCandidateXZ(Vector(128, 1, 128)), "server_old_spawn_not_post_move");
		Check(RICON_PairPlacementGate.WorldRepresentation("") == "EMPTY", "known_empty_world_representation");
		Check(RICON_PairPlacementGate.WorldRepresentation("{96A8AF57260A7392}worlds/MP/MpTest/MpTest.ent")
			== "{96A8AF57260A7392}worlds/MP/MpTest/MpTest.ent", "known_public_asset_representation");
		Check(RICON_PairPlacementGate.WorldRepresentation("$Synthetic:worlds/MP/MpTest/MpTest.ent")
			== "$Synthetic:worlds/MP/MpTest/MpTest.ent" && !RICON_PairPlacementGate.WorldAllowed("$Synthetic:worlds/MP/MpTest/MpTest.ent"), "synthetic_alias_logged_not_allowed");
		Check(RICON_PairPlacementGate.WorldRepresentation("C:/PRIVATE/worlds/MP/MpTest/MpTest.ent") == "OTHER_MASKED"
			&& RICON_PairPlacementGate.WorldRepresentation("$Alias:../worlds/MP/MpTest/MpTest.ent") == "OTHER_MASKED", "private_absolute_and_traversal_masked");
		Check(RICON_PairPlacementGate.WorldAllowed("$ArmaReforger:worlds/MP/MpTest/MpTest.ent"), "actual_public_root_allowed");
		Check(!RICON_PairPlacementGate.WorldAllowed("$Other:worlds/MP/MpTest/MpTest.ent")
			&& !RICON_PairPlacementGate.WorldAllowed("$ArmaReforger:worlds/MP/Other/Other.ent"), "other_alias_or_asset_refused");
		Check(!RICON_PairPlacementGate.WorldAllowed("$ArmaReforger:../worlds/MP/MpTest/MpTest.ent")
			&& !RICON_PairPlacementGate.WorldAllowed("C:/PRIVATE/worlds/MP/MpTest/MpTest.ent"), "traversal_and_absolute_not_allowed");
		PrintFormat("[RICON_NRXCHECK] scope=PAIR_PLACEMENT synthetic=1 checks=%1 passed=%2 native=NOT_RUN", m_Checks, m_Passed);
		if (m_Checks == 27 && m_Checks == m_Passed) Print("[RICON_NRXCHECK] scope=PAIR_PLACEMENT done: PASS");
		else Print("[RICON_NRXCHECK] scope=PAIR_PLACEMENT done: MISMATCH");
	}
}

//! Только pure readiness gates/formatter; никаких native объектов, keys или callback injection.
class RICON_RadioReadinessSelfChecks
{
	protected int m_Checks;
	protected int m_Passed;
	protected void Check(bool ok, string name)
	{
		m_Checks++;
		if (ok) m_Passed++;
		else PrintFormat("[RICON_NRXCHECK] scope=RADIO_READINESS mismatch=%1", name);
	}
	void Run()
	{
		Check(RICON_RadioReadinessGate.View(1, true, false) == "A_SERVER_VIEW", "server_view_label");
		Check(RICON_RadioReadinessGate.View(2, false, true) == "B_OWN_A_PROXY_VIEW", "b_local_proxy_view_label");
		Check(RICON_RadioReadinessGate.View(2, true, true) == "UNQUALIFIED_VIEW", "server_not_b_local");
		Check(RICON_RadioReadinessGate.View(1, false, true) == "UNQUALIFIED_VIEW", "client_not_a_server");
		Check(RICON_RadioReadinessGate.View(0, false, false) == "UNQUALIFIED_VIEW", "unknown_role");
		Check(RICON_RadioReadinessGate.PairKnown(true, true, 1, 1, true), "unique_pair_known");
		Check(!RICON_RadioReadinessGate.PairKnown(false, true, 1, 1, true), "missing_proxy_identity_unknown");
		Check(!RICON_RadioReadinessGate.PairKnown(true, false, 1, 1, true), "missing_catalog_unknown");
		Check(!RICON_RadioReadinessGate.PairKnown(true, true, 0, 1, true), "missing_a_radio_unknown");
		Check(!RICON_RadioReadinessGate.PairKnown(true, true, 1, 0, true), "missing_b_radio_unknown");
		Check(!RICON_RadioReadinessGate.PairKnown(true, true, 2, 1, true), "ambiguous_a_unknown");
		Check(!RICON_RadioReadinessGate.PairKnown(true, true, 1, 2, true), "ambiguous_b_unknown");
		Check(!RICON_RadioReadinessGate.PairKnown(true, true, -1, 1, true), "unobserved_a_unknown");
		Check(!RICON_RadioReadinessGate.PairKnown(true, true, 1, 1, false), "changed_refs_unknown");
		Check(RICON_RadioReadinessGate.BindingAllowed(true, true, 0, true, true), "local_selection_ref_match");
		Check(!RICON_RadioReadinessGate.BindingAllowed(false, true, 0, true, true), "unknown_entry_not_bound");
		Check(!RICON_RadioReadinessGate.BindingAllowed(true, false, 0, true, true), "nonradio_not_bound");
		Check(!RICON_RadioReadinessGate.BindingAllowed(true, true, -1, true, true), "catalog_mismatch_not_bound");
		Check(!RICON_RadioReadinessGate.BindingAllowed(true, true, 0, false, true), "inactive_not_bound");
		Check(!RICON_RadioReadinessGate.BindingAllowed(true, true, 0, true, false), "entry_ref_change_not_bound");
		Check(RICON_RadioReadinessGate.Bit(false, false) == -1, "unknown_not_false");
		Check(RICON_RadioReadinessGate.Bit(false, true) == -1, "unknown_not_true");
		Check(RICON_RadioReadinessGate.Bit(true, false) == 0 && RICON_RadioReadinessGate.Bit(true, true) == 1, "known_bits");
		Check(RICON_RadioReadinessGate.Equality(false, "SYNTHETIC_KEY", "SYNTHETIC_KEY") == -1, "unknown_equal_values_not_known");
		Check(RICON_RadioReadinessGate.Equality(true, "SYNTHETIC_KEY", "SYNTHETIC_KEY") == 1, "known_equal_values");
		Check(RICON_RadioReadinessGate.Equality(true, "SYNTHETIC_A", "SYNTHETIC_B") == 0, "known_unequal_values");
		Check(RICON_RadioReadinessGate.Equality(true, "", "") == 1, "known_empty_equality_is_observation");
		RICON_RadioReadinessSnapshot snapshot = new RICON_RadioReadinessSnapshot();
		Check(snapshot.PairKnown == 0 && snapshot.KeysEqual == -1 && snapshot.AEmpty == -1 && snapshot.BEmpty == -1, "default_pair_unknown");
		Check(snapshot.EntryKnown == 0 && snapshot.EntryIndex == -1 && snapshot.EntryBindingAllowed == -1, "default_entry_unknown");
		string text = snapshot.Text();
		Check(text.IndexOf("SYNTHETIC_KEY") < 0 && text.IndexOf("rrNativeOwnership=UNQUALIFIED") >= 0
			&& text.IndexOf("rrReplication=UNQUALIFIED") >= 0, "formatter_no_key_values_or_authority_claim");
		PrintFormat("[RICON_NRXCHECK] scope=RADIO_READINESS synthetic=1 checks=%1 passed=%2 native=NOT_RUN", m_Checks, m_Passed);
		if (m_Checks == 30 && m_Checks == m_Passed) Print("[RICON_NRXCHECK] scope=RADIO_READINESS done: PASS");
		else Print("[RICON_NRXCHECK] scope=RADIO_READINESS done: MISMATCH");
	}
}

class RICON_NativeRxSelfChecks
{
	protected int m_iChecks;
	protected int m_iPassed;
	protected void Check(bool value) { m_iChecks++; if (value) m_iPassed++; }
	void Run()
	{
		Check(!RICON_NativeRxGate.Requested(false, "1"));
		Check(!RICON_NativeRxGate.Requested(true, "true"));
		Check(RICON_NativeRxGate.Requested(true, "1"));
		Check(RICON_NativeRxGate.RoleAllowed(1) && RICON_NativeRxGate.RoleAllowed(2));
		Check(!RICON_NativeRxGate.RoleAllowed(0) && !RICON_NativeRxGate.RoleAllowed(3));
		Check(!RICON_NativeRxGate.Expired(59999) && RICON_NativeRxGate.Expired(60000));
		Check(RICON_NativeRxGate.Expired(-1));
		Check(!RICON_NativeRxGate.ClockGap(100, 5100) && RICON_NativeRxGate.ClockGap(100, 5101));
		Check(RICON_NativeRxGate.ClockGap(100, 99));
		Check(RICON_NativeRxGate.QualityValid(0) && RICON_NativeRxGate.QualityValid(1));
		Check(!RICON_NativeRxGate.QualityValid(-0.001) && !RICON_NativeRxGate.QualityValid(1.001));
		Check(RICON_NativeRxGate.CatalogAllowed(4, 16) && !RICON_NativeRxGate.CatalogAllowed(5, 16));
		Check(!RICON_NativeRxGate.CatalogAllowed(4, 17));
		Check(!RICON_NativeRxGate.CatalogAllowed(-1, 0) && !RICON_NativeRxGate.CatalogAllowed(0, -1));
		array<SCR_GadgetComponent> absent;
		array<SCR_GadgetComponent> empty = {};
		array<SCR_GadgetComponent> placeholder = {null};
		Check(RICON_NativeRxGate.CombineCategories(absent, absent).Count() == 0);
		Check(RICON_NativeRxGate.CombineCategories(empty, absent).Count() == 0);
		Check(RICON_NativeRxGate.CombineCategories(absent, placeholder).Count() == 1);
		Check(RICON_NativeRxGate.CombineCategories(placeholder, absent).Count() == 1);
		Check(RICON_NativeRxGate.CombineCategories(placeholder, placeholder).Count() == 2 && placeholder.Count() == 1);
		Check(RICON_NativeRxGate.Route(false, false) == "NULL_OR_UNKNOWN");
		Check(RICON_NativeRxGate.Route(true, true) == "CATALOG_REF_MATCH");
		Check(RICON_NativeRxGate.Route(true, false) == "UNBOUND_RECEIVER");
		Check(RICON_NativeRxGate.OrdinaryAllowed(511) && !RICON_NativeRxGate.OrdinaryAllowed(512));
		Check(!RICON_NativeRxGate.OrdinaryAllowed(-1));
		RICON_LiveJournal bounded = new RICON_LiveJournal(516); bounded.SetStream(false);
		for (int e = 0; e < 512; e++) bounded.Add(e, "synthetic", "NOTE", -1, -1, -1, e, "synthetic=1");
		for (int r = 0; r < 4; r++) bounded.Add(512 + r, "synthetic", "TERMINAL_TEST", -1, -1, -1, r, "synthetic=1");
		Check(bounded.Count() == 516 && bounded.Dropped() == 0 && bounded.NextSeq() == 516);
		RICON_NativeRxSyntheticSink sink = new RICON_NativeRxSyntheticSink();
		RICON_NativeRxTap own = new RICON_NativeRxTap(); RICON_NativeRxTap proxy = new RICON_NativeRxTap();
		ScriptInvoker ownRx = new ScriptInvoker(); ScriptInvoker proxyRx = new ScriptInvoker();
		Check(!own.AttachReceive(sink, null, 2) && own.Subscriptions() == 0);
		Check(!own.AttachReceive(sink, ownRx, 3) && own.Subscriptions() == 0);
		Check(own.AttachReceive(sink, ownRx, 2) && proxy.AttachReceive(sink, proxyRx, 1));
		Check(own.MatchesReceive(ownRx) && !own.MatchesReceive(proxyRx));
		Check(!own.AttachReceive(sink, proxyRx, 1) && own.Subscriptions() == 1);
		// These are private SYNTHETIC invokers, never native component callbacks.
		ownRx.Invoke(1, null, 48000, 0.5);
		Check(sink.m_iReceive == 1 && sink.m_iNullRx == 1 && sink.m_iLastOwner == 2);
		proxyRx.Invoke(7, null, 48200, 0.0001);
		Check(sink.m_iReceive == 2 && sink.m_iNullRx == 2 && sink.m_iLastOwner == 1 && sink.m_iLastSender == 7);
		own.Detach(); proxy.Detach(); own.Detach();
		Check(!own.MatchesReceive(ownRx) && !proxy.MatchesReceive(proxyRx));
		ownRx.Invoke(1, null, 48000, 0.5); proxyRx.Invoke(1, null, 48000, 0.5);
		Check(sink.m_iReceive == 2 && own.Subscriptions() == 0 && proxy.Subscriptions() == 0);
		ScriptInvoker capture = new ScriptInvoker(); ScriptInvokerInt used = new ScriptInvokerInt();
		Check(!own.AttachSource(sink, capture, null) && own.Subscriptions() == 0);
		Check(own.AttachSource(sink, capture, used) && own.Subscriptions() == 2);
		Check(own.MatchesSource(capture, used) && !own.MatchesSource(proxyRx, used));
		capture.Invoke(null); used.Invoke(1);
		Check(sink.m_iCapture == 1 && sink.m_iNullTx == 1 && sink.m_iUsed == 1);
		own.Detach(); capture.Invoke(null); used.Invoke(1);
		Check(sink.m_iCapture == 1 && sink.m_iUsed == 1 && own.Subscriptions() == 0);
		PrintFormat("[RICON_NRXCHECK] synthetic=1 checks=%1 passed=%2 delivery=NOT_TESTED", m_iChecks, m_iPassed);
		if (m_iChecks == 39 && m_iPassed == m_iChecks) Print("[RICON_NRXCHECK] done: PASS");
		else Print("[RICON_NRXCHECK] done: MISMATCH");
	}
}

//! Synthetic clock policy; native timestamps появляются только в runtime observer.
class RICON_BlindBObserveChecks
{
	protected int m_Checks;
	protected int m_Passed;
	protected void Check(bool ok, string name)
	{
		m_Checks++;
		if (ok) m_Passed++;
		else PrintFormat("[RICON_NRXCHECK] scope=B_OBSERVE mismatch=%1", name);
	}
	void Run()
	{
		Check(RICON_BlindBObserveWindow.Config(false, "", false, false, true, false), "default_unchanged");
		Check(RICON_BlindBObserveWindow.Config(true, "1", true, true, false, true), "exact_B_client_config");
		Check(!RICON_BlindBObserveWindow.Config(true, "true", true, true, false, true), "exact_flag");
		Check(!RICON_BlindBObserveWindow.Config(true, "1", false, true, false, true), "native_rx_required");
		Check(!RICON_BlindBObserveWindow.Config(true, "1", true, false, false, true), "separated_required");
		Check(!RICON_BlindBObserveWindow.Config(true, "1", true, true, true, true), "A_server_forbidden");
		Check(!RICON_BlindBObserveWindow.Config(true, "1", true, true, false, false), "client_required");
		RICON_BlindBObserveWindow window = new RICON_BlindBObserveWindow();
		Check(!window.Active() && window.PollMs() == 500 && window.HeartbeatMs() == 1000, "no_RX_no_window");
		Check(!window.FirstRx(1000, false) && !window.FirstRx(-1, true) && !window.Active(), "nonqualifying_or_unknown_clock");
		Check(window.FirstRx(1000, true) && window.Active() && window.PollMs() == 100 && window.HeartbeatMs() == 100, "first_own_RX_fast_cadence");
		Check(!window.FirstRx(1500, true), "subsequent_RX_cannot_move_origin");
		Check(!window.HeartbeatWritten(999) && window.Active(), "backward_stamp_not_coverage");
		Check(!window.HeartbeatWritten(2099) && window.Active(), "insufficient_actual_horizon");
		Check(window.HeartbeatWritten(2100) && !window.Active() && window.PollMs() == 500 && window.HeartbeatMs() == 1000, "actual_end_resets_old_cadence");
		Check(!window.FirstRx(2300, true) && !window.HeartbeatWritten(3400), "one_window_no_reopen");
		RICON_BlindBObserveWindow bounded = new RICON_BlindBObserveWindow(); bounded.FirstRx(1000, true);
		int beats;
		for (int tick = 1000; tick <= 2100; tick += 100)
		{
			if (bounded.Active()) { beats++; bounded.HeartbeatWritten(tick); }
		}
		Check(beats == 12 && !bounded.Active(), "at_most_12_fast_actual_beats");
		RICON_BlindBObserveWindow earlyExit = new RICON_BlindBObserveWindow(); earlyExit.FirstRx(1000, true);
		Check(!earlyExit.HeartbeatWritten(2000) && earlyExit.Active(), "early_process_stop_does_not_invent_end");
		RICON_BlindBObserveWindow delayed = new RICON_BlindBObserveWindow(); delayed.FirstRx(1000, true);
		Check(delayed.HeartbeatWritten(2400) && !delayed.Active(), "actual_late_stamp_not_synthesized_2100");
		Check(!RICON_NativeRxGate.Expired(59990) && RICON_NativeRxGate.Expired(60020), "actual_getter_return_after_lifetime_veto");
		PrintFormat("[RICON_NRXCHECK] scope=B_OBSERVE synthetic=1 checks=%1 passed=%2 nativeCallbacks=0", m_Checks, m_Passed);
		if (m_Checks == 19 && m_Checks == m_Passed) Print("[RICON_NRXCHECK] scope=B_OBSERVE done: PASS");
		else Print("[RICON_NRXCHECK] scope=B_OBSERVE done: MISMATCH");
	}
}

//! Synthetic: наблюдаемый lifetime не является native VoN stream binding.
class RICON_VoNSoundObserveChecks
{
	protected int m_Checks;
	protected int m_Passed;
	protected void Check(bool ok, string name)
	{
		m_Checks++;
		if (ok) m_Passed++;
		else PrintFormat("[RICON_NRXCHECK] scope=VON_SOUND mismatch=%1", name);
	}
	void Run()
	{
		Check(!RICON_VoNSoundObserveGate.Config(false, "1", true, true, false, true), "default_off");
		Check(RICON_VoNSoundObserveGate.Config(true, "1", true, true, false, true), "B_only_config");
		Check(!RICON_VoNSoundObserveGate.Config(true, "true", true, true, false, true), "exact_flag");
		Check(!RICON_VoNSoundObserveGate.Config(true, "1", true, true, true, true), "A_server_refused");
		Check(!RICON_VoNSoundObserveGate.Config(true, "1", false, true, false, true), "NRX_required");
		Check(!RICON_VoNSoundObserveGate.Config(true, "1", true, false, false, true), "separated_required");
		Check(!RICON_VoNSoundObserveGate.Config(true, "1", true, true, false, false), "client_required");
		RICON_VoNSoundLifetime life = new RICON_VoNSoundLifetime(); int duration;
		Check(!life.Active && life.Starts == 0 && life.Finishes == 0, "initial_empty");
		Check(life.Started("event", AudioHandle.Invalid, 1, 100) == 0 && !life.Active, "unknown_handle_raw_unmatched");
		Check(life.Finished("event", 7, 1, 110, duration) == 0 && duration == -1, "finish_without_start");
		Check(life.Started("event", 7, 1, 100) == 1 && life.Active, "known_start_ordinal");
		Check(life.Started("event", 7, 1, 101) == 0 && life.StartTick == 100, "duplicate_no_overwrite");
		Check(life.Started("other", 8, 2, 102) == 0 && life.Handle == 7, "overlap_preserved_as_ambiguous");
		Check(life.Finished("event", 8, 1, 120, duration) == 0 && life.Active, "foreign_handle_no_join");
		Check(life.Finished("other", 7, 1, 120, duration) == 0 && life.Active, "event_name_no_join");
		Check(life.Finished("event", 7, 2, 120, duration) == 0 && life.Active, "priority_no_join");
		Check(life.Finished("event", 7, 1, 99, duration) == 0 && life.Active, "backward_lifetime_no_join");
		Check(life.Finished("event", 7, 1, 150, duration) == 1 && duration == 50 && !life.Active, "matching_finish");
		Check(life.Finished("event", 7, 1, 151, duration) == 0, "duplicate_finish");
		Check(life.Started("event", 7, 1, 200) == 2, "reused_handle_new_observation");
		RICON_VoNSoundLifetime other = new RICON_VoNSoundLifetime();
		Check(other.Started("event", 7, 1, 200) == 1 && life.Ordinal == 2, "same_handle_other_component_not_joint_token");
		RICON_VoNSoundLifetime unknown = new RICON_VoNSoundLifetime();
		Check(unknown.Started("event", 7, 1, -1) == 0 && !unknown.Active, "unknown_clock");
		Check(unknown.Started("non_von_event", 7, 1, 100) == 1, "all_event_names_observed_no_route_claim");
		RICON_VoNSoundTap tap = new RICON_VoNSoundTap(); tap.Detach(); tap.Detach();
		Check(tap.Subscriptions() == 0 && tap.Generation == 0, "unattached_detach_idempotent");
		PrintFormat("[RICON_NRXCHECK] scope=VON_SOUND synthetic=1 checks=%1 passed=%2 nativeCallbacks=0", m_Checks, m_Passed);
		if (m_Checks == 24 && m_Passed == m_Checks) Print("[RICON_NRXCHECK] scope=VON_SOUND done: PASS");
		else Print("[RICON_NRXCHECK] scope=VON_SOUND done: MISMATCH");
	}
}

//! Pure policy: OFF не является identity drift; native setter не вызывается.
class RICON_RadioPowerMirrorSynthetic : RICON_LiveNativeRadioPower
{
	void Role(int player) { m_Player = player; }
	void Getter(bool powered, int now) { ObserveMirrorGetter(powered, now); }
	bool OffSeen() { return m_OffObserved; }
	int OnTick() { return m_MirrorOnAt; }
}

class RICON_NativeRadioPowerChecks
{
	protected int m_Checks;
	protected int m_Passed;
	protected void Check(bool ok, string name)
	{
		m_Checks++;
		if (ok) m_Passed++;
		else PrintFormat("[RICON_NRXCHECK] scope=RADIO_POWER mismatch=%1", name);
	}
	void Run()
	{
		Check(!RICON_RadioPowerGate.Config(false, "1", true, false), "default_off");
		Check(RICON_RadioPowerGate.Config(true, "1", true, false), "exact_dedicated_mode");
		Check(!RICON_RadioPowerGate.Config(true, "true", true, false), "strict_value");
		Check(!RICON_RadioPowerGate.Config(true, "1", false, false), "NRX_required");
		Check(!RICON_RadioPowerGate.Config(true, "1", true, true), "other_modes_forbidden");
		Check(RICON_RadioPowerGate.Restore(false, true, true, false) == -1, "unowned_OFF_no_restore");
		Check(RICON_RadioPowerGate.Restore(false, true, true, true) == -1, "no_attempt_no_ownership_claim");
		Check(RICON_RadioPowerGate.Restore(true, false, true, false) == -1, "identity_drift_no_restore");
		Check(RICON_RadioPowerGate.Restore(true, true, false, false) == -1, "protected_drift_no_restore");
		Check(RICON_RadioPowerGate.Restore(true, false, false, true) == -1, "already_ON_does_not_hide_ref_drift");
		Check(RICON_RadioPowerGate.Restore(true, true, true, true) == 0, "already_original_no_setter");
		Check(RICON_RadioPowerGate.Restore(true, true, true, false) == 1, "owned_OFF_is_expected_restore_state");
		Check(RICON_NativeRxGate.ClockGap(100, 99), "backward_clock_invalid");
		Check(RICON_NativeRxGate.Expired(60000), "bounded_lifetime");
		RICON_RadioPowerMirrorSynthetic mirror = new RICON_RadioPowerMirrorSynthetic(); mirror.Role(1);
		mirror.Getter(false, 100); mirror.Getter(true, 120); mirror.Getter(true, 130);
		Check(mirror.OffSeen() && mirror.OnTick() == 120, "bound_OFF_then_event_ON_before_next_poll");
		RICON_RadioPowerMirrorSynthetic owner = new RICON_RadioPowerMirrorSynthetic(); owner.Role(2);
		owner.Getter(false, 100); owner.Getter(true, 120);
		Check(!owner.OffSeen() && owner.OnTick() == -1, "mirror_tracking_does_not_change_B_writer");
		PrintFormat("[RICON_NRXCHECK] scope=RADIO_POWER synthetic=1 checks=%1 passed=%2 nativePowerCalls=0 captureCalls=0", m_Checks, m_Passed);
		if (m_Checks == 16 && m_Passed == m_Checks) Print("[RICON_NRXCHECK] scope=RADIO_POWER done: PASS");
		else Print("[RICON_NRXCHECK] scope=RADIO_POWER done: MISMATCH");
	}
}

//! Только pure pending observation. Payload события не передаётся в Observe.
class RICON_RadioRestoreObservationChecks
{
	protected int m_Checks;
	protected int m_Passed;
	protected void Check(bool ok, string name)
	{
		m_Checks++;
		if (ok) m_Passed++;
		else PrintFormat("[RICON_NRXCHECK] scope=RADIO_RESTORE_OBSERVE mismatch=%1", name);
	}
	void Run()
	{
		RICON_RadioRestoreObservation delayed = new RICON_RadioRestoreObservation();
		Check(delayed.Begin(100), "anchor_actual_ON_call");
		delayed.Observe(100, true, false);
		Check(!delayed.Drift && !delayed.Qualified(), "immediate_getter_OFF_pending_not_failure");
		delayed.Observe(150, true, true);
		Check(delayed.Qualified() && delayed.FirstOnAt == 150 && !delayed.Due(1099), "delayed_getter_ON_within_original_deadline");
		delayed.Observe(900, true, true);
		Check(delayed.FirstOnAt == 150 && delayed.CallAt == 100 && delayed.Due(1100), "no_reset_no_extra1000_after_qualification");
		Check(!delayed.Begin(900) && delayed.CallAt == 100, "anchor_one_use");
		RICON_RadioRestoreObservation payloadOnly = new RICON_RadioRestoreObservation(); payloadOnly.Begin(100);
		payloadOnly.Observe(200, true, false); payloadOnly.Observe(1100, true, false);
		Check(payloadOnly.Due(1100) && !payloadOnly.Qualified() && !payloadOnly.Drift, "payload_ON_getter_OFF_is_timeout");
		RICON_RadioRestoreObservation late = new RICON_RadioRestoreObservation(); late.Begin(100); late.Observe(1101, true, true);
		Check(late.LateSeen && !late.Qualified() && late.FirstOnAt == -1, "late_ON_recorded_not_success");
		RICON_RadioRestoreObservation edge = new RICON_RadioRestoreObservation(); edge.Begin(100); edge.Observe(1100, true, true);
		Check(edge.Qualified() && edge.FirstOnAt == 1100 && edge.Due(1100), "exact_deadline_boundary");
		RICON_RadioRestoreObservation refs = new RICON_RadioRestoreObservation(); refs.Begin(100); refs.Observe(200, false, true);
		Check(refs.Drift && !refs.Qualified(), "reference_or_protected_conflict_no_qualification");
		RICON_RadioRestoreObservation drift = new RICON_RadioRestoreObservation(); drift.Begin(100);
		drift.Observe(200, true, true); drift.Observe(300, true, false);
		Check(drift.Drift && !drift.Qualified(), "OFF_after_qualified_ON_is_drift");
		RICON_RadioRestoreObservation reset = new RICON_RadioRestoreObservation(); reset.Begin(100);
		reset.Observe(200, true, false); reset.Observe(199, true, true);
		Check(reset.Drift && !reset.Qualified(), "backward_observation_clock");
		Check(RICON_RadioPowerGate.Restore(true, true, true, true) == 0, "already_original_ON_no_extra_setter");
		Check(RICON_RadioPowerGate.Restore(true, false, true, false) == -1, "lost_owner_no_widened_restore");
		RICON_RadioRestoreObservation absent = new RICON_RadioRestoreObservation(); absent.Observe(500, true, true);
		Check(!absent.Due(500) && !absent.Qualified() && !absent.Begin(-1), "no_call_no_synthetic_anchor");
		PrintFormat("[RICON_NRXCHECK] scope=RADIO_RESTORE_OBSERVE synthetic=1 checks=%1 passed=%2 nativePowerCalls=0", m_Checks, m_Passed);
		if (m_Checks == 14 && m_Passed == m_Checks) Print("[RICON_NRXCHECK] scope=RADIO_RESTORE_OBSERVE done: PASS");
		else Print("[RICON_NRXCHECK] scope=RADIO_RESTORE_OBSERVE done: MISMATCH");
	}
}

//! Synthetic fixed hold/mode policy; native writes и receive callbacks не вызываются.
class RICON_CaptureBPowerOffChecks
{
	protected int m_Checks;
	protected int m_Passed;
	protected void Check(bool ok, string name)
	{
		m_Checks++;
		if (ok) m_Passed++;
		else PrintFormat("[RICON_NRXCHECK] scope=CAPTURE_B_POWER_OFF mismatch=%1", name);
	}
	void Run()
	{
		Check(!RICON_CaptureBPowerOffGate.Config(false, "1", true, true, false), "default_off");
		Check(RICON_CaptureBPowerOffGate.Config(true, "1", true, true, false), "exact_control");
		Check(!RICON_CaptureBPowerOffGate.Config(true, "true", true, true, false), "strict_flag");
		Check(!RICON_CaptureBPowerOffGate.Config(true, "1", false, true, false), "NRX_required");
		Check(!RICON_CaptureBPowerOffGate.Config(true, "1", true, false, false), "separated_required");
		Check(!RICON_CaptureBPowerOffGate.Config(true, "1", true, true, true), "other_controls_forbidden");
		Check(RICON_CaptureBPowerOffGate.Deadline(100) == 6100 && RICON_CaptureBPowerOffGate.HOLD_MS == 6000, "fixed_preset_from_call");
		Check(RICON_CaptureBPowerOffGate.Deadline(-1) == -1, "unknown_call_no_deadline");
		Check(!RICON_CaptureBPowerOffGate.Expired(6099, 6100), "before_fixed_deadline");
		Check(RICON_CaptureBPowerOffGate.Expired(6100, 6100), "deadline_boundary");
		Check(RICON_CaptureBPowerOffGate.Expired(6200, 6100), "late_tick_no_extension");
		Check(RICON_CaptureBPowerOffGate.Remaining(2000, 6100) == 4100, "actual_remaining");
		Check(RICON_CaptureBPowerOffGate.Remaining(2000, -1) == -1 && !RICON_CaptureBPowerOffGate.Expired(2000, -1), "unknown_no_positive_proof");
		Check(RICON_RadioPowerGate.Restore(true, false, true, false) == -1 && RICON_RadioPowerGate.Restore(true, true, true, true) == 0, "no_blind_restore_no_unnecessary_ON");
		PrintFormat("[RICON_NRXCHECK] scope=CAPTURE_B_POWER_OFF synthetic=1 checks=%1 passed=%2 nativePowerCalls=0 captureCalls=0", m_Checks, m_Passed);
		if (m_Checks == 14 && m_Passed == m_Checks) Print("[RICON_NRXCHECK] scope=CAPTURE_B_POWER_OFF done: PASS");
		else Print("[RICON_NRXCHECK] scope=CAPTURE_B_POWER_OFF done: MISMATCH");
	}
}

//! Near test parameter3м не означает SpeakingRange. Только pure/synthetic.
class RICON_DirectNearChecks
{
	protected int m_Checks;
	protected int m_Passed;
	protected void Check(bool ok, string name)
	{
		m_Checks++;
		if (ok) m_Passed++;
		else PrintFormat("[RICON_NRXCHECK] scope=DIRECT_NEAR mismatch=%1", name);
	}
	void Run()
	{
		Check(!RICON_DirectCaptureGate.Config(false, "1", true, false), "default_off");
		Check(RICON_DirectCaptureGate.Config(true, "1", true, false), "exact_mode");
		Check(!RICON_DirectCaptureGate.Config(true, "true", true, false), "strict_value");
		Check(!RICON_DirectCaptureGate.Config(true, "1", false, false), "NRX_required");
		Check(!RICON_DirectCaptureGate.Config(true, "1", true, true), "other_modes_forbidden");
		Check(RICON_DirectCaptureGate.Near("128 1 128", "128 1 127"), "actual_near_example");
		Check(RICON_DirectCaptureGate.Near("0 1 0", "3 1 0"), "near_boundary3");
		Check(!RICON_DirectCaptureGate.Near("0 1 0", "3.01 1 0"), "outside_test_near");
		Check(!RICON_DirectCaptureGate.Near("128 1 128", "28 1 128"), "old100m_not_near");
		Check(RICON_DirectCaptureGate.Near("0 0 0", "0 0 3"), "near_uses3D");
		Check(!RICON_DirectCaptureGate.Near("0 0 0", "0 3.01 0"), "height_distance_counts");
		RICON_SeparatedPairGeometry unknown = new RICON_SeparatedPairGeometry();
		Check(!unknown.SameIdentity(new RICON_SeparatedPairGeometry()), "unknown_identity_not_known_near");
		PrintFormat("[RICON_NRXCHECK] scope=DIRECT_NEAR synthetic=1 checks=%1 passed=%2 nativeCalls=0", m_Checks, m_Passed);
		if (m_Checks == 12 && m_Passed == m_Checks) Print("[RICON_NRXCHECK] scope=DIRECT_NEAR done: PASS");
		else Print("[RICON_NRXCHECK] scope=DIRECT_NEAR done: MISMATCH");
	}
}

//! Flat mode matrix: ни RPC, ни native capture/power/route calls.
class RICON_DirectBPowerOffChecks
{
	protected int m_Checks;
	protected int m_Passed;
	protected void Check(bool ok, string name)
	{
		m_Checks++;
		if (ok) m_Passed++;
		else PrintFormat("[RICON_NRXCHECK] scope=DIRECT_B_POWER_OFF mismatch=%1", name);
	}
	void Run()
	{
		Check(RICON_DirectCaptureGate.OptionalPower(false, ""), "standalone_DIRECT_no_power");
		Check(RICON_DirectCaptureGate.OptionalPower(true, "1"), "exact_optional_power");
		Check(!RICON_DirectCaptureGate.OptionalPower(true, "0"), "power_invalid_zero");
		Check(!RICON_DirectCaptureGate.OptionalPower(true, "true"), "power_invalid_text");
		Check(RICON_DirectCaptureGate.Config(true, "1", true, false), "standalone_DIRECT_retained");
		Check(RICON_CaptureBPowerOffGate.GeometryMode(false, "", true, true), "standalone_separated_power_retained");
		Check(RICON_CaptureBPowerOffGate.GeometryMode(true, "1", false, false), "combo_uses_DIRECT_near");
		Check(!RICON_CaptureBPowerOffGate.GeometryMode(true, "1", true, true), "combo_forbids_separated");
		Check(!RICON_CaptureBPowerOffGate.GeometryMode(true, "0", false, false), "direct_invalid_zero");
		Check(RICON_CaptureBPowerOffGate.Config(true, "1", true, RICON_CaptureBPowerOffGate.GeometryMode(true, "1", false, false), false), "both_exact_flags_valid");
		Check(!RICON_CaptureBPowerOffGate.Config(true, "0", true, true, false), "invalid_power_stays_rejected");
		Check(!RICON_CaptureBPowerOffGate.Config(false, "1", true, true, false), "no_implicit_power_from_DIRECT");
		PrintFormat("[RICON_NRXCHECK] scope=DIRECT_B_POWER_OFF synthetic=1 checks=%1 passed=%2 nativeCalls=0", m_Checks, m_Passed);
		if (m_Checks == 12 && m_Passed == m_Checks) Print("[RICON_NRXCHECK] scope=DIRECT_B_POWER_OFF done: PASS");
		else Print("[RICON_NRXCHECK] scope=DIRECT_B_POWER_OFF done: MISMATCH");
	}
}

//! Только заранее выбранные presets; native actors/Power/guards не вызываются.
class RICON_DirectPower10sChecks
{
	protected int m_Checks;
	protected int m_Passed;
	protected void Check(bool ok, string name)
	{
		m_Checks++;
		if (ok) m_Passed++;
		else PrintFormat("[RICON_NRXCHECK] scope=DIRECT_POWER_10S mismatch=%1", name);
	}
	void Run()
	{
		Check(RICON_CaptureBPowerOffGate.FixedHold(true) == 10000 && RICON_CaptureBPowerOffGate.Plan(true) == "BPowerOff.Direct10s.1", "combo_preset_before_OFF");
		Check(RICON_CaptureBPowerOffGate.FixedHold(false) == 6000 && RICON_CaptureBPowerOffGate.Plan(false) == "BPowerOff.1", "standalone_unchanged");
		Check(RICON_CaptureBPowerOffGate.Deadline(12062, true) == 22062, "actual_counter_new_DB");
		Check(RICON_CaptureBPowerOffGate.Remaining(15610, 22062) - 500 == 5952, "actual_same_elapsed_counter_not_RXsuccess");
		Check(RICON_CaptureBPowerOffGate.Remaining(15610, RICON_CaptureBPowerOffGate.Deadline(12062)) - 500 == 1952, "old_NO_RELEASE_counter_preserved");
		Check(!RICON_CaptureBPowerOffGate.Expired(22061, 22062) && RICON_CaptureBPowerOffGate.Expired(22062, 22062), "fixed_boundary");
		Check(RICON_CaptureBPowerOffGate.Remaining(16610, 22062) == 5452 && RICON_CaptureBPowerOffGate.Deadline(12062, true) == 22062, "elapsed_does_not_reset_deadline");
		Check(RICON_CaptureBPowerOffGate.Deadline(-1, true) == -1, "unknown_call_not_positive_budget");
		PrintFormat("[RICON_NRXCHECK] scope=DIRECT_POWER_10S synthetic=1 checks=%1 passed=%2 nativeCalls=0", m_Checks, m_Passed);
		if (m_Checks == 8 && m_Passed == m_Checks) Print("[RICON_NRXCHECK] scope=DIRECT_POWER_10S done: PASS");
		else Print("[RICON_NRXCHECK] scope=DIRECT_POWER_10S done: MISMATCH");
	}
}
