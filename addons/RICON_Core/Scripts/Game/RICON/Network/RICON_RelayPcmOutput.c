class RICON_RelayPcmOutputOrigin
{
	ref RICON_RelayPcmOrigin m_Origin;
	int m_iSequence, m_iRxFirstSampleIndex;

	RICON_RelayPcmOutputOrigin CloneProvenance()
	{
		RICON_RelayPcmOutputOrigin copy = new RICON_RelayPcmOutputOrigin();
		copy.m_Origin = m_Origin.CloneOrigin();
		copy.m_iSequence = m_iSequence;
		copy.m_iRxFirstSampleIndex = m_iRxFirstSampleIndex;
		return copy;
	}
}

//! Local PCM16LE representation of a station emission. The path also identifies the separate original source.
class RICON_RelayPcmOutputFrame
{
	static const int SAMPLE_RATE_HZ = RICON_MediaPcmProfile.SAMPLE_RATE_HZ;
	static const int FRAME_SAMPLES = RICON_MediaPcmProfile.FRAME_SAMPLES;
	static const int GROUP_DELAY_OUTPUT_SAMPLES = RICON_PcmResampler48To8.GROUP_DELAY_OUTPUT_SAMPLES;
	ref RICON_RelayPcmPath m_Path;
	int m_iSequence, m_iFirstSampleIndex, m_iEmitAtMs, m_iDeadlineMs;
	int m_iSourceOriginWindows, m_iOldestSourceSequence;
	bool m_bContinuousInput;
	ref array<int> m_aBytes;
	ref array<ref RICON_RelayPcmOutputOrigin> m_aProvenance = {};
}

//! The caller transfers sole Take ownership of a fresh handoff for one contiguous, synchronously consumed emission.
class RICON_RelayPcmOutput
{
	protected static const int HISTORY_WINDOWS = 2;
	protected ref RICON_RelayPcmHandoff m_Handoff;
	protected ref RICON_RelayPcmPath m_Path;
	protected ref RICON_PcmResampler48To8 m_Decimator = new RICON_PcmResampler48To8();
	protected ref array<ref RICON_RelayPcmOutputOrigin> m_History = {};
	protected int m_iNextRxSampleIndex, m_iNextOutputSampleIndex, m_iNextSequence;
	protected bool m_bClosed;

	bool Configure(RICON_RelayPcmHandoff handoff, RICON_RelayPcmPath expectedPath, int firstRxSampleIndex, int firstOutputSampleIndex)
	{
		if (m_bClosed || m_Handoff || !handoff || !handoff.MatchesPath(expectedPath)
			|| !m_Decimator.Reset(firstRxSampleIndex, firstOutputSampleIndex)) return false;
		m_Handoff = handoff;
		m_Path = expectedPath.ClonePath();
		m_iNextRxSampleIndex = firstRxSampleIndex;
		m_iNextOutputSampleIndex = firstOutputSampleIndex;
		return true;
	}

	bool MatchesAuthority(RICON_RelayPcmPath path, RICON_RelayPcmPolicy policy)
	{
		return !m_bClosed && m_Path && m_Path.SamePath(path) && m_Handoff && m_Handoff.Type() == RICON_RelayPcmHandoff
			&& m_Handoff.MatchesAuthority(path, policy);
	}

	bool CanTakeAt(int nowMs) { return !m_Handoff || m_Handoff.CanTakeAt(nowMs); }

	bool HasCurrentInputPermit() { return !m_bClosed && m_Handoff && m_Handoff.HasCurrentInputPermit(); }
	bool HasSeriesQueue() { return !m_bClosed && m_Handoff && m_Handoff.HasSeriesQueue(); }

	string Take(int nowMs, out RICON_RelayPcmOutputFrame frame, int plannedEmitMs = -1)
	{
		frame = null;
		if (m_bClosed || !m_Handoff) return "CLOSED";
		if (m_iNextSequence >= int.MAX || m_iNextRxSampleIndex > int.MAX - RICON_PcmResampler48To8.INPUT_SAMPLES
			|| m_iNextOutputSampleIndex > int.MAX - RICON_RelayPcmOutputFrame.FRAME_SAMPLES) return Retire("CLOSED");

		RICON_RelayPcmEmission emission;
		string status = m_Handoff.Take(nowMs, emission, plannedEmitMs);
		if (status == "BAD_TIME") return status;
		if (status != "OK" && status != "WAIT" && status != "EMPTY") return Retire(status);
		int deadlineMs = int.MAX;
		foreach (RICON_RelayPcmOutputOrigin prior : m_History)
		{
			deadlineMs = Math.Min(deadlineMs, prior.m_Origin.EffectiveDeadlineMs());
		}
		if (nowMs >= deadlineMs) return Retire("EXPIRED");
		if (status != "OK") return status;

		int emitMs = nowMs;
		if (plannedEmitMs >= 0) emitMs = plannedEmitMs;
		if (!emission || !emission.m_Origin || emission.m_iEmitAtMs != emitMs) return Retire("INVALID");
		if (!m_Path.SamePath(emission.m_Path) || emission.m_iSequence != m_iNextSequence
			|| emission.m_iRxFirstSampleIndex != m_iNextRxSampleIndex || !OriginContinues(emission.m_Origin)) return Retire("GAP");
		deadlineMs = Math.Min(deadlineMs, emission.m_Origin.EffectiveDeadlineMs());
		if (nowMs >= deadlineMs) return Retire("EXPIRED");

		int firstOutputSampleIndex;
		array<int> samples = m_Decimator.Process(emission.m_iRxFirstSampleIndex, emission.m_aSamples, firstOutputSampleIndex);
		array<int> bytes = RICON_MediaPcmProfile.Encode(samples);
		if (!bytes || firstOutputSampleIndex != m_iNextOutputSampleIndex) return Retire("INVALID");

		RICON_RelayPcmOutputOrigin current = new RICON_RelayPcmOutputOrigin();
		current.m_Origin = emission.m_Origin;
		current.m_iSequence = emission.m_iSequence;
		current.m_iRxFirstSampleIndex = emission.m_iRxFirstSampleIndex;
		RICON_RelayPcmOutputFrame output = new RICON_RelayPcmOutputFrame();
		output.m_Path = emission.m_Path;
		output.m_iSequence = emission.m_iSequence;
		output.m_iFirstSampleIndex = firstOutputSampleIndex;
		output.m_iEmitAtMs = emitMs;
		output.m_iDeadlineMs = deadlineMs;
		RICON_RelayPcmOrigin oldest = current.m_Origin;
		if (!m_History.IsEmpty()) oldest = m_History[0].m_Origin;
		output.m_iSourceOriginWindows = m_History.Count() + 1;
		if (oldest.m_PreviousInput)
		{
			oldest = oldest.m_PreviousInput;
			output.m_iSourceOriginWindows++;
		}
		output.m_iOldestSourceSequence = oldest.m_iSequence;
		output.m_bContinuousInput = current.m_Origin.m_bContinuousInput;
		output.m_aBytes = bytes;
		foreach (RICON_RelayPcmOutputOrigin contributor : m_History) output.m_aProvenance.Insert(contributor.CloneProvenance());
		output.m_aProvenance.Insert(current.CloneProvenance());

		// The FIR spans up to the previous two RX windows, including its startup response.
		if (m_History.Count() == HISTORY_WINDOWS) m_History.RemoveOrdered(0);
		m_History.Insert(current);
		m_iNextRxSampleIndex += RICON_PcmResampler48To8.INPUT_SAMPLES;
		m_iNextOutputSampleIndex += RICON_RelayPcmOutputFrame.FRAME_SAMPLES;
		m_iNextSequence++;
		frame = output;
		return "OK";
	}

	protected bool OriginContinues(RICON_RelayPcmOrigin origin)
	{
		if (m_History.IsEmpty()) return !origin.m_PreviousInput;
		RICON_RelayPcmOrigin prior = m_History[m_History.Count() - 1].m_Origin;
		if (origin.m_bContinuousInput != prior.m_bContinuousInput || !origin.Follows(prior)) return false;
		if (!origin.m_bContinuousInput) return true;
		RICON_RelayPcmOrigin dependency = origin.m_PreviousInput;
		return dependency && dependency.SameWindow(prior);
	}

	protected string Retire(string status)
	{
		Close();
		return status;
	}

	void Close()
	{
		m_bClosed = true;
		if (m_Handoff) m_Handoff.Close();
		m_Handoff = null;
		m_Path = null;
		m_Decimator = null;
		m_History.Clear();
	}
}
