//! TEST_ONLY: доступ к существующему сравнению входов, без копирования его логики.
class RICON_SnapshotInput : RICON_VoNSeamRun
{
	static RICON_LinkSnapshot Copy(RICON_LinkSnapshot source) { return CopySnapshot(source); }
	static bool Same(RICON_LinkSnapshot a, RICON_LinkSnapshot b) { return SameSnapshot(a, b); }
}

//! Только наблюдение защищённой карты настоящего получателя; алгоритм не переопределяется.
class RICON_SnapshotProbeReceiver : RICON_LinkSnapshotReceiver
{
	void RICON_SnapshotProbeReceiver(int supportedSchemaVersion) {}
	int SlotCount() { return m_mSlots.Count(); }
	int SlotId(int index) { return m_mSlots.GetKey(index); }
}

class RICON_SnapshotRun
{
	protected string m_sId;
	protected bool m_bQuiet;
	protected bool m_bOk = true;
	protected int m_iCalls;
	protected ref RICON_SnapshotProbeReceiver m_Receiver;

	void RICON_SnapshotRun(string id, bool quiet = false)
	{
		m_sId = id;
		m_bQuiet = quiet;
		m_Receiver = new RICON_SnapshotProbeReceiver(1);
	}

	bool Passed(int expectedCalls) { return m_bOk && m_iCalls == expectedCalls; }
	bool Failed() { return !m_bOk; }
	int Calls() { return m_iCalls; }

	void Check(bool ok, string field)
	{
		m_bOk = ok && m_bOk;
		if (!ok && !m_bQuiet)
			PrintFormat("[RICON_SNAPSHOT] FAIL %1 step=%2 field=%3", m_sId, m_iCalls, field);
	}

	protected void Log(string result)
	{
		if (!m_bQuiet)
			PrintFormat("[RICON_SNAPSHOT] %1 step=%2 %3", m_sId, m_iCalls, result);
	}

	void RequireNaN(RICON_LinkSnapshot snapshot)
	{
		Check(snapshot != null, "снимок с NaN существует");
		if (snapshot)
			Check(RICON_TestSupport.FloatClass(snapshot.m_fQuality) == 1, "настоящий NaN");
	}

	//! Полные снимки должны различать NaN/±Infinity; отдельный guard обязан отвергнуть Infinity вместо NaN.
	static bool SpecialSelfCheck()
	{
		RICON_LinkSnapshot a = new RICON_LinkSnapshot();
		RICON_LinkSnapshot b = new RICON_LinkSnapshot();
		a.m_fQuality = RICON_L0Run.Special(0);
		b.m_fQuality = RICON_L0Run.Special(0);
		bool positive = RICON_SnapshotInput.Same(a, b);
		RICON_SnapshotRun guard = new RICON_SnapshotRun("NaN-positive", true);
		guard.RequireNaN(a);
		positive = !guard.Failed() && positive;
		int rejected;
		b.m_fQuality = RICON_L0Run.Special(1);
		if (!RICON_SnapshotInput.Same(a, b)) rejected++;
		a.m_fQuality = RICON_L0Run.Special(-1);
		if (!RICON_SnapshotInput.Same(a, b)) rejected++;
		RICON_SnapshotRun wrong = new RICON_SnapshotRun("NaN-negative", true);
		wrong.RequireNaN(b);
		if (wrong.Failed()) rejected++;
		PrintFormat("[RICON_SNAPSHOT] specialPositive=%1 specialControls=%2", positive, rejected);
		return positive && rejected == 3;
	}

	void Compare(RICON_LinkSnapshot current, RICON_LinkSnapshot incoming, RICON_ESnapshotDecision expected)
	{
		m_iCalls++;
		RICON_LinkSnapshot beforeCurrent = RICON_SnapshotInput.Copy(current);
		RICON_LinkSnapshot beforeIncoming = RICON_SnapshotInput.Copy(incoming);
		RICON_ESnapshotDecision actual = incoming.CompareAgainst(current);
		Check(actual == expected, "decision");
		Check(RICON_SnapshotInput.Same(current, beforeCurrent), "current не изменён");
		Check(RICON_SnapshotInput.Same(incoming, beforeIncoming), "incoming не изменён");
		Log("decision=" + typename.EnumToString(RICON_ESnapshotDecision, actual));
	}

	protected void Result(RICON_SnapshotApplyResult actual, RICON_ESnapshotDecision expected,
		bool resync, bool diagnostic, array<int> ids, array<int> sequences)
	{
		Check(actual != null, "result существует");
		if (!actual)
			return;
		Check(actual.m_eDecision == expected, "decision");
		Check(actual.m_bRequestResync == resync, "REQUEST_RESYNC");
		Check(actual.m_bDiagnostic == diagnostic, "DIAGNOSTIC");
		Check(ids.Count() == sequences.Count() && m_Receiver.SlotCount() == ids.Count(), "число слотов");
		for (int i = 0; i < ids.Count(); i++)
			Check(m_Receiver.GetLinkSeq(ids[i]) == sequences[i], "seq слота");
		for (int j = 0; j < m_Receiver.SlotCount(); j++)
			Check(ids.Contains(m_Receiver.SlotId(j)), "нет лишнего слота");
		Log(string.Format("decision=%1 resync=%2 diagnostic=%3 slots=%4",
			typename.EnumToString(RICON_ESnapshotDecision, actual.m_eDecision),
			actual.m_bRequestResync, actual.m_bDiagnostic, m_Receiver.SlotCount()));
	}

	void Resync(int nowMs, int epoch, int schema, array<ref RICON_LinkSnapshot> snapshots,
		RICON_ESnapshotDecision expected, bool resync, bool diagnostic, array<int> ids, array<int> sequences)
	{
		m_iCalls++;
		array<ref RICON_LinkSnapshot> copies = {};
		foreach (RICON_LinkSnapshot item : snapshots)
		{
			copies.Insert(RICON_SnapshotInput.Copy(item));
		}
		RICON_SnapshotApplyResult actual = m_Receiver.ApplyFullResync(nowMs, epoch, schema, snapshots);
		Check(snapshots.Count() == copies.Count(), "размер входа resync");
		for (int i = 0; i < snapshots.Count() && i < copies.Count(); i++)
			Check(RICON_SnapshotInput.Same(snapshots[i], copies[i]), "вход resync не изменён");
		Result(actual, expected, resync, diagnostic, ids, sequences);
	}

	void Receive(int nowMs, RICON_LinkSnapshot snapshot, RICON_ESnapshotDecision expected,
		bool resync, bool diagnostic, array<int> ids, array<int> sequences)
	{
		m_iCalls++;
		RICON_LinkSnapshot before = RICON_SnapshotInput.Copy(snapshot);
		RICON_SnapshotApplyResult actual = m_Receiver.Receive(nowMs, snapshot);
		Check(RICON_SnapshotInput.Same(snapshot, before), "вход receive не изменён");
		Result(actual, expected, resync, diagnostic, ids, sequences);
	}

	void Visible(int nowMs, int linkId, bool expected, bool diagnostic)
	{
		m_iCalls++;
		bool actualDiagnostic;
		bool actual = m_Receiver.IsVisible(nowMs, linkId, actualDiagnostic);
		Check(actual == expected, "visible");
		Check(actualDiagnostic == diagnostic, "visible diagnostic");
		Log(string.Format("visible=%1 diagnostic=%2", actual, actualDiagnostic));
	}
}
