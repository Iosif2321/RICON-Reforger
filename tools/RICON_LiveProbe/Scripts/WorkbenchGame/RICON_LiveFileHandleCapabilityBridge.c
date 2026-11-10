//! Единственное READ open выполняется в foreground WB Run до сигнала Play, без Sleep.
class RICON_FileHandleCapabilityBridge
{
	protected static bool s_OpenAttempted;
	static bool Prepare()
	{
		if (!RICON_FileHandleCapability.Requested())
			return true;
		string run; string token;
		if (s_OpenAttempted || !RICON_FileHandleCapability.Config(run, token))
		{
			Print("[RICON_FHC] kind=WB_REFUSED reason=CONFIG_OR_ONE_OPEN_USED"); return false;
		}
		s_OpenAttempted = true;
		PrintFormat("[RICON_FHC] run=%1 kind=WB_OPEN_BEGIN mode=READ", run);
		FileHandle handle = FileIO.OpenFile(RICON_FileHandleCapabilityGate.PathFor(run), FileMode.READ);
		PrintFormat("[RICON_FHC] run=%1 kind=WB_OPEN_RETURN referencePresent=%2", run, handle != null);
		if (!handle)
		{
			PrintFormat("[RICON_FHC] run=%1 kind=WB_REFUSED reason=OPEN_FAILED", run); return false;
		}
		bool open = handle.IsOpen();
		if (!open)
		{
			PrintFormat("[RICON_FHC] run=%1 kind=WB_REFUSED reason=HANDLE_NOT_OPEN", run); return false;
		}
		int length = handle.GetLength();
		PrintFormat("[RICON_FHC] run=%1 kind=WB_INITIAL_LENGTH length=%2", run, length);
		if (length != 0)
		{
			PrintFormat("[RICON_FHC] run=%1 kind=WB_REFUSED reason=INITIAL_NOT_EMPTY", run); handle.Close(); return false;
		}
		string emptyLine;
		int count = handle.ReadLine(emptyLine); bool eof = handle.IsEOF();
		PrintFormat("[RICON_FHC] run=%1 kind=WB_INITIAL_READ characters=%2 eof=%3 lineLength=%4", run, count, eof, emptyLine.Length());
		if (count != -1 || !eof || !emptyLine.IsEmpty())
		{
			PrintFormat("[RICON_FHC] run=%1 kind=WB_REFUSED reason=INITIAL_EOF_UNCONFIRMED", run); handle.Close(); return false;
		}
		if (!RICON_FileHandleCapability.Hold(handle, run, token))
		{
			PrintFormat("[RICON_FHC] run=%1 kind=WB_REFUSED reason=HOLDER_REJECTED", run); handle.Close(); return false;
		}
		PrintFormat("[RICON_FHC] run=%1 kind=WB_HELD_EOF length=0 initialRead=-1 eof=1 openAttempts=1", run);
		return true;
	}
}
