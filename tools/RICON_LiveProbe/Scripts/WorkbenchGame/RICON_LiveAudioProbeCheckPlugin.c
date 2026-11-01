//! TOOL_ONLY: pure gate/journal tests, no world, Core Audio, VoN subscription or capture.
[WorkbenchPluginAttribute(name: "RICON: Check passive audio probe", wbModules: {"ScriptEditor"})]
class RICON_LiveAudioProbeCheckPlugin : WorkbenchPlugin
{
	override void RunCommandline()
	{
		int checks = 10;
		int passed;
		if (!RICON_LiveAudioProbe.ResolveRequested(false, "1")) passed++;
		if (!RICON_LiveAudioProbe.ResolveRequested(true, "")) passed++;
		if (!RICON_LiveAudioProbe.ResolveRequested(true, "0")) passed++;
		if (!RICON_LiveAudioProbe.ResolveRequested(true, "true")) passed++;
		if (RICON_LiveAudioProbe.ResolveRequested(true, "1")) passed++;
		RICON_LiveAudioJournal journal = new RICON_LiveAudioJournal();
		if (!journal.IsStream()) passed++;
		int first = journal.Add(-1, "probe", "NOTE", -1, -1, -1, 0, "synthetic");
		if (first == 0 && journal.Count() == 1 && journal.NextSeq() == 1 && journal.Dropped() == 0) passed++;
		string audioLine = RICON_LiveAudioJournal.AudioFormat(journal.At(0));
		if (audioLine.Contains("[RICON_AUDIO]") && !audioLine.Contains("[RICON_LIVE]")) passed++;
		string baseLine = RICON_LiveJournal.Format(journal.At(0));
		if (baseLine.Contains("[RICON_LIVE]") && !baseLine.Contains("[RICON_AUDIO]")) passed++;
		int second = journal.Add(-1, "probe", "NOTE", -1, -1, -1, 1, "synthetic");
		if (second == 1 && journal.Count() == 2 && journal.NextSeq() == 2) passed++;
		journal.Dump();
		PrintFormat("[RICON_AUDIOCHECK] checks=%1 passed=%2", checks, passed);
		if (passed == checks)
			Print("[RICON_AUDIOCHECK] done: PASS");
		else
			Print("[RICON_AUDIOCHECK] done: MISMATCH");
	}
}
