//! Отдельный controlled OFF modifier. Passive NativeRadioPower не меняется.
class RICON_CaptureBPowerOffGate
{
	static const int HOLD_MS = 6000;
	static int FixedHold(bool direct)
	{
		if (direct)
			return 10000;
		return HOLD_MS;
	}
	static string Plan(bool direct)
	{
		if (direct)
			return "BPowerOff.Direct10s.1";
		return "BPowerOff.1";
	}
	static bool Configured()
	{
		string value;
		return System.GetCLIParam("riconCaptureBPowerOff", value);
	}
	static bool Config(bool configured, string value, bool nrx, bool separated, bool conflict)
	{
		return configured && value == "1" && nrx && separated && !conflict;
	}
	static bool GeometryMode(bool directConfigured, string directValue, bool separatedConfigured, bool separatedRequested)
	{
		if (directConfigured) return directValue == "1" && !separatedConfigured;
		return separatedRequested;
	}
	static bool Requested()
	{
		string value; bool configured = System.GetCLIParam("riconCaptureBPowerOff", value);
		array<string> forbidden = {"riconNativeRadioPower", "riconPairPlacement", "riconCaptureNoSend", "riconCaptureWrongFrequency", "riconCaptureShort400", "riconCaptureBlindTrial", "riconVoNSoundObserve", "riconBlindBObserve", "riconRestRehearsalArm", "riconAudioMeter", "riconCommunicationDiscovery", "riconVoNInputShape"};
		foreach (string name : forbidden) { string other; if (System.GetCLIParam(name, other)) return false; }
		string directValue; bool directConfigured = System.GetCLIParam("riconCaptureDirect", directValue);
		bool geometry = GeometryMode(directConfigured, directValue, RICON_CaptureSeparatedGate.Configured(), RICON_CaptureSeparatedGate.Requested());
		return Config(configured, value, RICON_LiveNativeRxProbe.IsRequested(), geometry, false);
	}
	static int Deadline(int callAt, bool direct = false)
	{
		if (callAt < 0) return -1;
		return callAt + FixedHold(direct);
	}
	static bool Expired(int now, int deadline) { return deadline >= 0 && now >= deadline; }
	static int Remaining(int now, int deadline)
	{
		if (now < 0 || deadline < 0) return -1;
		return deadline - now;
	}
}

//! Reuse только exact snapshot, safe restore и pending ON; свой bounded hold lifecycle.
class RICON_LiveCaptureBPowerOff : RICON_LiveNativeRadioPower
{
	protected static RICON_LiveCaptureBPowerOff s_Local;
	protected int m_HoldDeadline = -1;
	protected int m_ProofRows;
	protected bool m_DirectHold;
	protected int m_HoldPreset = 6000;
	protected string m_Plan = "BPowerOff.1";
	override bool Start(BaseWorld world, RICON_LiveNativeRxProbe nrx)
	{
		if (!RICON_CaptureBPowerOffGate.Requested() || !world || world.IsEditMode() || !nrx) return false;
		if (!RICON_PairPlacementGate.WorldAllowed(GetGame().GetWorldFile())) return false;
		if (!System.GetCLIParam("riconNativeRxRun", m_Run) || !RICON_D0Gate.ValidRunId(m_Run)) return false;
		m_World = world; m_Nrx = nrx; m_Started = System.GetTickCount(); m_Last = m_Started; m_Poll = m_Started;
		m_DirectHold = RICON_DirectCaptureGate.Requested();
		m_HoldPreset = RICON_CaptureBPowerOffGate.FixedHold(m_DirectHold); m_Plan = RICON_CaptureBPowerOffGate.Plan(m_DirectHold);
		m_Running = true; s_Local = this;
		Emit("WAITING", "holdMs=" + m_HoldPreset.ToString() + ",deadlineOrigin=OFF_CALL,noReset=1,captureCallsByController=0");
		return true;
	}
	protected bool GeometryCurrent(int now)
	{
		RICON_SeparatedPairGeometry geometry;
		if (RICON_DirectCaptureGate.Requested())
			return m_Nrx && m_Nrx.RICON_ReadDirectNearGeometry(now, m_Run, m_World, geometry) && geometry.Eligible;
		return m_Nrx && m_Nrx.RICON_ReadSeparatedGeometry(now, m_Run, m_World, geometry) && geometry.Eligible;
	}
	//! Только fresh A-server mirror getter; B deadline проверяет external coordinator.
	static bool ReadyForSource(string run, BaseWorld world)
	{
		if (!s_Local || !s_Local.m_Running || !s_Local.m_Bound || s_Local.m_Player != 1 || !Replication.IsServer()) return false;
		if (s_Local.m_Run != run || s_Local.m_World != world || !RICON_CaptureBPowerOffGate.Requested()) return false;
		int now = System.GetTickCount();
		int remaining;
		if (!s_Local.m_Nrx.RICON_PulseHealth(now, run, 1, remaining)) return false;
		int nrxDeadline = now + remaining;
		if (!s_Local.ObservationCurrent(now) || !s_Local.GeometryCurrent(now)) return false;
		bool off = !s_Local.m_Snapshot.Radio.IsPowered();
		int checkAt = System.GetTickCount();
		bool current = s_Local.ObservationCurrent(checkAt);
		int after = System.GetTickCount();
		return off && current && after >= now && after - now <= 500 && after < nrxDeadline
			&& !RICON_NativeRxGate.ClockGap(s_Local.m_Last, after) && !RICON_NativeRxGate.Expired(after - s_Local.m_Started);
	}
	override protected void Emit(string kind, string fields)
	{
		if (m_ProofRows >= 192 && kind != "TERMINAL") { Stop("OVERFLOW"); return; }
		bool readback = kind == "STATE" || kind == "BOUND" || kind == "EVENT_READBACK" || kind == "OFF_READBACK" || kind == "RESTORE_READBACK" || kind == "RESTORE_DEADLINE";
		string original = kind;
		int sampleAt = System.GetTickCount(); int remaining; int start = -1;
		bool known = m_Bound && m_Nrx && m_Nrx.RICON_PulseHealth(sampleAt, m_Run, m_Player, remaining) && Current();
		if (known) start = sampleAt - RICON_NativeRxGate.LIMIT_MS + remaining;
		int powered = -1; string indices = "UNKNOWN"; int count;
		if (known)
		{
			powered = 0; if (m_Snapshot.Radio.IsPowered()) powered = 1;
			count = m_Snapshot.Endpoints.Count(); indices = "";
			for (int i = 0; i < count; i++) { if (i > 0) indices = indices + "|"; indices = indices + i.ToString(); }
		}
		int checkAt = System.GetTickCount();
		bool current = known && ObservationCurrent(checkAt);
		int now = System.GetTickCount();
		known = current && now >= sampleAt && now - sampleAt <= 500 && start >= 0
			&& !RICON_NativeRxGate.ClockGap(m_Last, now) && !RICON_NativeRxGate.Expired(now - m_Started)
			&& !RICON_NativeRxGate.Expired(now - start);
		if (!known) powered = -1;
		if (readback)
		{
			kind = "READBACK";
			fields = "sourceKind=" + original + ",startTickMs=" + start.ToString() + ",bindingGeneration=1";
			fields = fields + ",bEntityRpl=" + m_BRpl + ",radioRefLocal=" + string.Format("%1", m_Snapshot.Radio);
			fields = fields + ",nativeIndices=" + indices + ",trCount=" + count.ToString();
			fields = fields + ",currentRefs=" + Bit(known) + ",catalogSame=" + Bit(known) + ",protectedSame=" + Bit(known);
			fields = fields + ",powerKnown=" + Bit(known) + ",powered=" + powered.ToString() + ",observerStampMs=" + now.ToString();
			fields = fields + ",nativeOwnership=UNKNOWN,authority=UNQUALIFIED,replication=UNQUALIFIED";
		}
		fields = "powerPlanId=" + m_Plan + "," + fields;
		if (m_Player == 2)
		{
			int offCalls; int onCalls; int offAt = -1;
			if (m_Setters > 0) offCalls = 1;
			if (m_RestoreObservation.CallAt >= 0) onCalls = 1;
			if (m_HoldDeadline >= 0) offAt = m_OffAt;
			fields = fields + ",offCallTickMs=" + offAt.ToString() + ",holdDeadlineTickMs=" + m_HoldDeadline.ToString();
			int holdRemaining = RICON_CaptureBPowerOffGate.Remaining(now, m_HoldDeadline);
			fields = fields + ",holdRemainingMs=" + holdRemaining.ToString() + ",holdMs=" + m_HoldPreset.ToString();
			fields = fields + ",offCalls=" + offCalls.ToString() + ",restoreOnCalls=" + onCalls.ToString();
		}
		string view = "UNBOUND";
		if (m_Player == 1) view = "A_SERVER_B_MIRROR";
		else if (m_Player == 2) view = "B_LOCAL";
		string line = "[RICON_CPOWER] run=" + RICON_LiveText.Esc(m_Run) + " view=" + view;
		line = line + " kind=" + kind + " rawTickMs=" + now.ToString() + " text=" + fields;
		m_ProofRows++; PrintFormat("%1", line);
	}
	override protected void Off()
	{
		if (!m_Running || m_Invoked || m_Player != 2) return;
		m_Invoked = true;
		if (!ObservationCurrent(System.GetTickCount()) || !m_Snapshot.Radio.IsPowered()) { Stop("PRE_OFF_DRIFT"); return; }
		Emit("OFF_INTENT", "oneUse=1,returnType=VOID,holdPresetMs=" + m_HoldPreset.ToString());
		if (!m_Running || !ObservationCurrent(System.GetTickCount()) || !m_Snapshot.Radio.IsPowered()) { Stop("PRE_OFF_DRIFT"); return; }
		m_OffAt = System.GetTickCount(); m_HoldDeadline = RICON_CaptureBPowerOffGate.Deadline(m_OffAt, m_DirectHold);
		m_InNative = true; m_Setters++;
		m_Snapshot.Radio.SetPower(false);
		m_InNative = false;
		int returnedAt = System.GetTickCount();
		Emit("OFF_RETURN", "callStartTickMs=" + m_OffAt.ToString() + ",callReturnTickMs=" + returnedAt.ToString() + ",returnType=VOID,effect=UNQUALIFIED");
		if (!m_Deferred.IsEmpty()) { Stop(m_Deferred); return; }
		if (!ObservationCurrent(System.GetTickCount())) { Stop("APPLIED_UNKNOWN"); return; }
		m_OffObserved = !m_Snapshot.Radio.IsPowered(); State("OFF_READBACK");
	}
	override void Tick(BaseWorld world)
	{
		if (!m_Running) return;
		int now = System.GetTickCount();
		if (!world || world != m_World || world.IsEditMode() || RICON_NativeRxGate.ClockGap(m_Last, now) || RICON_NativeRxGate.Expired(now - m_Started)) { Stop("WORLD_CLOCK_LIMIT"); return; }
		m_Last = now;
		if (!m_Deferred.IsEmpty()) { Stop(m_Deferred); return; }
		if (m_Player == 2 && m_RestoreObservation.Drift) { Stop("RESTORE_DRIFT"); return; }
		if (m_Player == 2 && m_RestoreObservation.Due(now))
		{
			if (!ObservationCurrent(now)) { Stop("APPLIED_UNKNOWN"); return; }
			State("RESTORE_DEADLINE");
			if (m_RestoreObservation.Qualified()) Stop("LOCAL_STATE_CYCLE_OBSERVED");
			else Stop("APPLIED_UNKNOWN_RESTORE_TIMEOUT_OR_DRIFT");
			return;
		}
		if (!m_Bound)
		{
			if (GeometryCurrent(now)) Bind(now);
			return;
		}
		if (!ObservationCurrent(now) || !GeometryCurrent(now)) { Stop("CONTEXT_LOST"); return; }
		if (m_Player == 2 && m_Invoked && !m_RestoreUsed && RICON_CaptureBPowerOffGate.Expired(now, m_HoldDeadline))
		{
			if (!Restore()) { Stop("RESTORE_UNKNOWN"); return; }
			if (!m_Deferred.IsEmpty()) { Stop(m_Deferred); return; }
			if (m_RestoreNoop) Stop("ALREADY_ORIGINAL_ON");
			return;
		}
		if (now - m_Poll < 500) return;
		m_Poll = now; State("STATE");
		if (!m_Running || m_Player != 2) return;
		if (!m_Invoked && now - m_BoundAt >= 1000) { Off(); return; }
		if (m_Invoked && !m_RestoreUsed)
		{
			if (!m_Snapshot.Radio.IsPowered()) m_OffObserved = true;
			else if (m_OffObserved) Stop("UNEXPECTED_ON_DURING_HOLD");
		}
	}
	override void Stop(string reason)
	{
		if (!m_Running || m_Stopping) return;
		if (m_InNative || m_InCallback) { m_Deferred = reason; return; }
		if (m_Player == 2 && m_Setters > 0 && !m_RestoreUsed)
		{
			m_RestoreUsed = true;
			Emit("CONDITIONAL_CLEANUP_UNKNOWN", "noBlindRestore=1,fixedDeadlineNotExtended=1");
			reason = "CONDITIONAL_CLEANUP_UNKNOWN_" + reason;
		}
		super.Stop(reason);
		if (!m_Running && s_Local == this) s_Local = null;
	}
}
