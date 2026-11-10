//! Только pure проверки; FileIO/FileHandle и мир не используются.
class RICON_FileHandleCapabilityChecks
{
	protected int m_Checks;
	protected int m_Passed;
	protected void Check(bool ok, string name)
	{
		m_Checks++;
		if (ok) m_Passed++;
		else PrintFormat("[RICON_FHCCHECK] mismatch=%1", name);
	}
	void Run()
	{
		Check(RICON_FileHandleCapabilityGate.ValidConfig("public-run.1", "public_token-1"), "public_ascii_config");
		Check(!RICON_FileHandleCapabilityGate.ValidConfig("", "token"), "empty_run_refused");
		Check(!RICON_FileHandleCapabilityGate.ValidConfig("../private", "token"), "path_injection_refused");
		Check(!RICON_FileHandleCapabilityGate.ValidConfig("run", "two words"), "whitespace_token_refused");
		Check(RICON_FileHandleCapabilityGate.PathFor("run") == "$profile:RICON_FileHandleCapability_run.txt", "fixed_profile_path");
		Check(RICON_FileHandleCapabilityGate.Phase(5099, 100, 100) == "WAIT", "no_early_read");
		Check(RICON_FileHandleCapabilityGate.Phase(5100, 5099, 100) == "READ", "read_at_5000");
		Check(RICON_FileHandleCapabilityGate.Phase(10099, 5100, 100) == "READ", "last_valid_read_window");
		Check(RICON_FileHandleCapabilityGate.Phase(10100, 5100, 100) == "TIMEOUT", "timeout_before_read");
		Check(RICON_FileHandleCapabilityGate.Phase(99, 100, 100) == "CLOCK_RESET", "backward_clock_refused");
		PrintFormat("[RICON_FHCCHECK] synthetic=1 checks=%1 passed=%2 nativeCalls=0", m_Checks, m_Passed);
		if (m_Checks == 10 && m_Checks == m_Passed) Print("[RICON_FHCCHECK] done: PASS");
		else Print("[RICON_FHCCHECK] done: MISMATCH");
	}
}
