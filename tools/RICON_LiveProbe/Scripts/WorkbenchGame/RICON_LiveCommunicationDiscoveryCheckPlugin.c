//! TOOL_ONLY D0 pure checks: no world, RPC, sound, VoN, queue mutation or runtime process.
[WorkbenchPluginAttribute(name: "RICON: Check communication discovery D0", wbModules: {"ScriptEditor"})]
class RICON_LiveCommunicationDiscoveryCheckPlugin : WorkbenchPlugin
{
	override void RunCommandline()
	{
		int checks = 60;
		int passed;
		if (!RICON_D0Gate.ResolveRequested(false, "1")) passed++;
		if (!RICON_D0Gate.ResolveRequested(true, "0")) passed++;
		if (RICON_D0Gate.ResolveRequested(true, "1")) passed++;
		if (!RICON_D0Gate.ValidRunId("")) passed++;
		if (!RICON_D0Gate.ValidRunId("bad run")) passed++;
		if (!RICON_D0Gate.ValidRunId("bad,value")) passed++;
		if (RICON_D0Gate.ValidRunId("D0.public_01")) passed++;
		if (!RICON_D0Gate.ValidRunId("D0-public\n")) passed++;
		string longId = "12345678901234567890123456789012345678901234567890123456789012345";
		if (!RICON_D0Gate.ValidRunId(longId)) passed++;
		if (RICON_D0Gate.Forward(100, 100)) passed++;
		if (RICON_D0Gate.Forward(100, 101)) passed++;
		if (!RICON_D0Gate.Forward(101, 100)) passed++;
		if (RICON_D0Gate.ValidCallbackContext(true, true, true, true, true, true, true, true, 2, 2, 1, 1, 1)) passed++;
		if (!RICON_D0Gate.ValidCallbackContext(true, false, true, true, true, true, true, true, 2, 2, 1, 1, 1)) passed++;
		if (!RICON_D0Gate.ValidCallbackContext(true, true, true, false, true, true, true, true, 2, 2, 1, 1, 1)) passed++;
		if (!RICON_D0Gate.ValidCallbackContext(true, true, true, true, true, true, true, true, 2, 2, 0, 1, 1)) passed++;
		if (!RICON_D0Gate.ValidCallbackContext(false, true, true, true, true, true, true, true, 2, 2, 1, 1, 1)) passed++;
		if (RICON_D0Gate.ValidRadioReceive(true, 2, 2, 1, 1, true, 0)) passed++;
		if (!RICON_D0Gate.ValidRadioReceive(true, 2, 2, 1, 1, false, -1)) passed++;
		if (!RICON_D0Gate.ValidRadioReceive(true, 2, 2, 9, 1, true, 0)) passed++;
		if (!RICON_D0Gate.ValidRadioReceive(true, 1, 2, 1, 1, true, 0)) passed++;
		if (RICON_D0Gate.ValidChallengeSequence(1, 2, 0, 0)) passed++;
		if (RICON_D0Gate.ValidChallengeSequence(1, 3, 1, 2)) passed++;
		if (!RICON_D0Gate.ValidChallengeSequence(1, 2, 1, 2)) passed++;
		if (RICON_D0Gate.ValidChallengeSequence(2, 1, 1, 3)) passed++;
		if (RICON_D0Gate.CanRetryChallenge(true, 1, 1500, 5000)) passed++;
		if (!RICON_D0Gate.CanRetryChallenge(true, 1, 1499, 5000)) passed++;
		if (!RICON_D0Gate.CanRetryChallenge(true, 20, 1500, 30000)) passed++;
		if (!RICON_D0Gate.CanRetryChallenge(true, 1, 1500, 39001)) passed++;
		RICON_D0TraceModel trace = new RICON_D0TraceModel();
		trace.StartA();
		bool lateMiss = !trace.InitialReadiness(false, false, 1, 1, 1) && !trace.IsBlocked();
		bool lateReady = trace.InitialReadiness(true, false, 1, 1, 1);
		bool baseline = trace.BaselineComplete();
		bool fresh = trace.FreshReadiness(true, true, 1, 1, 1);
		bool called = trace.PreCall(true, true, true, true, false);
		if (lateMiss && lateReady && baseline && fresh && called && trace.IsRequestCalled()) passed++;
		if (!trace.PreCall(true, true, true, true, false) && trace.IsRequestUsed()) passed++;
		RICON_D0TraceModel timeoutTrace = new RICON_D0TraceModel();
		timeoutTrace.StartA();
		if (!timeoutTrace.InitialReadiness(false, true, 1, 1, 1) && timeoutTrace.IsBlocked()
			&& !timeoutTrace.IsRequestCalled()) passed++;
		RICON_D0TraceModel swapTrace = new RICON_D0TraceModel();
		swapTrace.StartA(); swapTrace.InitialReadiness(true, false, 1, 1, 1); swapTrace.BaselineComplete();
		if (!swapTrace.FreshReadiness(true, false, 1, 1, 1) && swapTrace.IsBlocked()) passed++;
		RICON_D0TraceModel reserveTrace = new RICON_D0TraceModel();
		reserveTrace.StartA(); reserveTrace.InitialReadiness(true, false, 1, 1, 1); reserveTrace.BaselineComplete();
		reserveTrace.FreshReadiness(true, true, 1, 1, 1);
		if (!reserveTrace.PreCall(false, true, true, true, false) && reserveTrace.IsRequestUsed()
			&& !reserveTrace.IsRequestCalled()) passed++;
		RICON_D0TraceModel delayTrace = new RICON_D0TraceModel();
		delayTrace.StartA(); delayTrace.InitialReadiness(true, false, 1, 1, 1); delayTrace.BaselineComplete();
		delayTrace.FreshReadiness(true, true, 1, 1, 1);
		if (!delayTrace.PreCall(true, false, true, true, false) && !delayTrace.IsRequestCalled()) passed++;
		RICON_D0TraceModel contextTrace = new RICON_D0TraceModel();
		contextTrace.StartA(); contextTrace.InitialReadiness(true, false, 1, 1, 1); contextTrace.BaselineComplete();
		contextTrace.FreshReadiness(true, true, 1, 1, 1);
		if (!contextTrace.PreCall(true, true, false, true, false) && !contextTrace.IsRequestCalled()) passed++;
		RICON_D0TraceModel serverTrace = new RICON_D0TraceModel();
		serverTrace.StartA(); serverTrace.InitialReadiness(true, false, 1, 1, 1); serverTrace.BaselineComplete();
		serverTrace.FreshReadiness(true, true, 1, 1, 1);
		if (!serverTrace.PreCall(true, true, true, false, false) && !serverTrace.IsRequestCalled()) passed++;
		RICON_D0TraceModel invalidTrace = new RICON_D0TraceModel();
		invalidTrace.StartA(); invalidTrace.InitialReadiness(true, false, 1, 1, 1); invalidTrace.BaselineComplete();
		invalidTrace.FreshReadiness(true, true, 1, 1, 1);
		if (!invalidTrace.PreCall(true, true, true, true, true) && !invalidTrace.IsRequestCalled()) passed++;
		if (!RICON_D0Gate.PostWindowComplete(9999, 10000)) passed++;
		if (RICON_D0Gate.PostWindowComplete(10000, 10000)) passed++;

		RICON_ED0RequestGate gate = RICON_D0Gate.CanRequest(false, 39000, 0, 38500, 38900,
			31500, 2, 4, 6, 2, 4, 6);
		if (gate == RICON_ED0RequestGate.OK) passed++;
		gate = RICON_D0Gate.CanRequest(false, 39001, 0, 38500, 38900, 31500, 2, 4, 6, 2, 4, 6);
		if (gate == RICON_ED0RequestGate.A_DEADLINE) passed++;
		gate = RICON_D0Gate.CanRequest(false, 39000, 0, 37900, 38901, 32000, 2, 4, 6, 2, 4, 6);
		if (gate == RICON_ED0RequestGate.ACK_RTT) passed++;
		gate = RICON_D0Gate.CanRequest(false, 39000, 0, 37000, 38000, 33000, 2, 4, 6, 2, 4, 6);
		if (gate == RICON_ED0RequestGate.OK) passed++;
		gate = RICON_D0Gate.CanRequest(false, 39000, 0, 36999, 38000, 33000, 2, 4, 6, 2, 4, 6);
		if (gate == RICON_ED0RequestGate.ACK_RTT) passed++;
		gate = RICON_D0Gate.CanRequest(false, 39000, 0, 38000, 38000, 33000, 2, 4, 6, 2, 4, 6);
		if (gate == RICON_ED0RequestGate.OK) passed++;
		gate = RICON_D0Gate.CanRequest(false, 39001, 0, 38000, 38000, 33000, 2, 4, 6, 2, 4, 6);
		if (gate == RICON_ED0RequestGate.A_DEADLINE) passed++;
		gate = RICON_D0Gate.CanRequest(false, 38001, 0, 37000, 37000, 34000, 2, 4, 6, 2, 4, 6);
		if (gate == RICON_ED0RequestGate.ACK_AGE) passed++;
		gate = RICON_D0Gate.CanRequest(false, 39000, 0, 38500, 38900, 31499, 2, 4, 6, 2, 4, 6);
		if (gate == RICON_ED0RequestGate.B_DEADLINE) passed++;
		gate = RICON_D0Gate.CanRequest(false, 39000, 0, 38500, 38900, 31500, 2, 4, 6, 2, 4, 7);
		if (gate == RICON_ED0RequestGate.GENERATION) passed++;
		gate = RICON_D0Gate.CanRequest(false, 39000, 0, 38500, 38900, 31500, 0, 4, 6, 0, 4, 6);
		if (gate == RICON_ED0RequestGate.GENERATION) passed++;
		gate = RICON_D0Gate.CanRequest(true, 39000, 0, 38500, 38900, 31500, 2, 4, 6, 2, 4, 6);
		if (gate == RICON_ED0RequestGate.ALREADY_USED) passed++;
		gate = RICON_D0Gate.CanRequest(false, 38900, 0, 39000, 38900, 31500, 2, 4, 6, 2, 4, 6);
		if (gate == RICON_ED0RequestGate.CLOCK) passed++;

		RICON_D0LogBudget budget = new RICON_D0LogBudget();
		bool ordinary = true;
		for (int i = 0; i < RICON_D0LogBudget.ORDINARY_LIMIT; i++)
			ordinary = ordinary && budget.TakeOrdinary();
		if (ordinary && !budget.TakeOrdinary() && budget.HasGap() && budget.Ordinary() == 4000) passed++;
		bool reserve = true;
		for (int r = 0; r < 96; r++)
			reserve = reserve && budget.TakeReserved();
		if (reserve && budget.Total() == 4096) passed++;
		if (!budget.TakeReserved() && budget.Total() == 4096) passed++;
		RICON_D0LogBudget reserveFirst = new RICON_D0LogBudget();
		bool firstReserve = true;
		for (int rf = 0; rf < 96; rf++)
			firstReserve = firstReserve && reserveFirst.TakeReserved();
		if (firstReserve && reserveFirst.Reserved() == 96) passed++;
		if (!reserveFirst.TakeReserved() && reserveFirst.Total() == 96) passed++;
		bool afterReserve = true;
		for (int ro = 0; ro < 4000; ro++)
			afterReserve = afterReserve && reserveFirst.TakeOrdinary();
		if (afterReserve && reserveFirst.Total() == 4096 && !reserveFirst.TakeOrdinary()) passed++;
		RICON_D0LogBudget interleaved = new RICON_D0LogBudget();
		bool interleaveOk = true;
		for (int a = 0; a < 48; a++) interleaveOk = interleaveOk && interleaved.TakeReserved();
		for (int b = 0; b < 2000; b++) interleaveOk = interleaveOk && interleaved.TakeOrdinary();
		for (int c = 0; c < 48; c++) interleaveOk = interleaveOk && interleaved.TakeReserved();
		for (int d = 0; d < 2000; d++) interleaveOk = interleaveOk && interleaved.TakeOrdinary();
		if (interleaveOk && interleaved.Total() == 4096 && interleaved.Reserved() == 96) passed++;

		PrintFormat("[RICON_D0CHECK] checks=%1 passed=%2", checks, passed);
		if (passed == checks)
			Print("[RICON_D0CHECK] done: PASS");
		else
			Print("[RICON_D0CHECK] done: MISMATCH");
	}
}
