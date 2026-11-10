//! TOOL_ONLY I0 pure checks; no world, getters, subscriptions or mutation.
[WorkbenchPluginAttribute(name: "RICON: Check VoN mapping I0", wbModules: {"ScriptEditor"})]
class RICON_LiveVoNMappingInspectionCheckPlugin : WorkbenchPlugin
{
	override void RunCommandline()
	{
		int checks = 53; int passed;
		if (!RICON_I0Gate.ResolveRequested(false, "1")) passed++;
		if (!RICON_I0Gate.ResolveRequested(true, "0")) passed++;
		if (RICON_I0Gate.ResolveRequested(true, "1")) passed++;
		if (RICON_I0Gate.ModesExclusive(true, false, false)) passed++;
		if (!RICON_I0Gate.ModesExclusive(true, true, false)) passed++;
		if (!RICON_I0Gate.IdentityExpired(20000)) passed++;
		if (RICON_I0Gate.IdentityExpired(20001)) passed++;
		if (!RICON_I0Gate.WindowExpired(44999)) passed++;
		if (RICON_I0Gate.WindowExpired(45000)) passed++;
		if (RICON_I0Gate.CanSnapshot(3000, 0, 0)) passed++;
		if (!RICON_I0Gate.CanSnapshot(2999, 0, 0)) passed++;
		if (!RICON_I0Gate.CanSnapshot(6000, 3000, 16)) passed++;
		if (!RICON_I0Gate.CanSnapshot(2999, 3000, 0)) passed++;
		if (RICON_I0Gate.NonNullMatch(true, true, true)) passed++;
		if (!RICON_I0Gate.NonNullMatch(false, false, true)) passed++;
		if (!RICON_I0Gate.NonNullMatch(true, true, false)) passed++;
		if (RICON_I0Gate.EntriesAllowed(8, 8, 8)) passed++;
		if (!RICON_I0Gate.EntriesAllowed(9, 9, 9)) passed++;
		if (!RICON_I0Gate.EntriesAllowed(4, 5, 5)) passed++;
		if (RICON_I0Gate.CatalogAllowed(4, 16)) passed++;
		if (!RICON_I0Gate.CatalogAllowed(5, 16)) passed++;
		RICON_I0Budget budget = new RICON_I0Budget(); bool ordinary = true;
		for (int i = 0; i < 1024; i++) ordinary = ordinary && budget.TakeOrdinary();
		if (ordinary && !budget.TakeOrdinary()) passed++;
		bool reserve = true; for (int r = 0; r < 64; r++) reserve = reserve && budget.TakeReserved();
		if (reserve && !budget.TakeReserved() && budget.Total() == 1088) passed++;
		RICON_I0Budget reverse = new RICON_I0Budget(); bool reverseOk = true;
		for (int rr = 0; rr < 64; rr++) reverseOk = reverseOk && reverse.TakeReserved();
		for (int oo = 0; oo < 1024; oo++) reverseOk = reverseOk && reverse.TakeOrdinary();
		if (reverseOk && reverse.Total() == 1088 && !reverse.TakeOrdinary()) passed++;
		if (RICON_I0Gate.MatchState(false, false, true) == 0) passed++;
		if (RICON_I0Gate.MatchState(true, true, false) == 2) passed++;
		if (!RICON_I0Gate.IdentityStable(true, false, true, true)) passed++;
		if (RICON_I0Gate.IdentityStable(false, false, false, false)) passed++;
		if (RICON_I0Gate.EntryBindingAllowed(true, true, true)) passed++;
		if (!RICON_I0Gate.EntryBindingAllowed(false, false, false)) passed++;
		if (!RICON_I0Gate.EntryBindingAllowed(true, false, true)) passed++;
		for (int mapping = 0; mapping < 16; mapping++)
		{
			bool ready = RICON_I0Gate.MappingReady((mapping & 1) != 0, (mapping & 2) != 0,
				(mapping & 4) != 0, (mapping & 8) != 0);
			if (ready == (mapping == 15)) passed++;
		}
		if (RICON_I0Gate.SameWorldGeometry(true, true, true)) passed++;
		if (!RICON_I0Gate.SameWorldGeometry(true, true, false)) passed++;
		if (RICON_I0Gate.RplRoundTripAllowed(true, true)) passed++;
		if (!RICON_I0Gate.RplRoundTripAllowed(false, false)) passed++;
		if (!RICON_I0Gate.RplRoundTripAllowed(true, false)) passed++;
		RICON_I0Budget terminal = new RICON_I0Budget();
		if (terminal.TakeReserved() && terminal.Reserved() == 1 && terminal.Total() == 1) passed++;
		PrintFormat("[RICON_I0CHECK] checks=%1 passed=%2", checks, passed);
		if (passed == checks) Print("[RICON_I0CHECK] done: PASS"); else Print("[RICON_I0CHECK] done: MISMATCH");
	}
}
