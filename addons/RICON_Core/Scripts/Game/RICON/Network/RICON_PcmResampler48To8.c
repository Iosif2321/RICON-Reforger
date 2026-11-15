//! Causal mono PCM16 decimation for the fixed synthetic relay profile.
class RICON_PcmResampler48To8
{
	static const int INPUT_RATE_HZ = RICON_PcmResampler8To48.OUTPUT_RATE_HZ;
	static const int INPUT_SAMPLES = RICON_PcmResampler8To48.OUTPUT_SAMPLES;
	static const int OUTPUT_RATE_HZ = RICON_MediaPcmProfile.SAMPLE_RATE_HZ;
	static const int OUTPUT_SAMPLES = RICON_MediaPcmProfile.FRAME_SAMPLES;
	static const int GROUP_DELAY_INPUT_SAMPLES = 192;
	static const int GROUP_DELAY_OUTPUT_SAMPLES = 32;

	protected static const int RATIO = 6;
	protected static const int LAST_TAP = 2 * GROUP_DELAY_INPUT_SAMPLES;
	protected static const int HISTORY_SIZE = LAST_TAP + 1;
	protected static const int Q14_ONE = 16384;
	protected static const int Q14_HALF = 8192;

	// 3600 Hz Hann-windowed sinc at 48 kHz. Normalize DC, quantize to Q14, then add 6 to the center tap.
	protected ref array<int> m_HalfTaps = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, -1, -1, -1, -1, 0, 0, 1, 2, 2,
		2, 1, 0, -1, -2, -3, -3, -2, -1, 1,
		2, 3, 4, 4, 3, 1, -1, -4, -5, -5,
		-5, -3, 0, 3, 6, 7, 7, 6, 3, -1,
		-5, -8, -9, -9, -6, -2, 3, 8, 11, 12,
		10, 6, 0, -6, -12, -15, -15, -11, -5, 3,
		10, 16, 18, 17, 11, 3, -6, -15, -21, -22,
		-19, -11, 0, 11, 21, 26, 26, 20, 9, -5,
		-18, -28, -32, -29, -20, -5, 11, 26, 35, 38,
		32, 18, 0, -19, -35, -44, -43, -33, -15, 8,
		29, 46, 53, 48, 33, 9, -18, -42, -58, -62,
		-52, -30, 0, 31, 58, 72, 71, 54, 24, -13,
		-49, -76, -88, -80, -55, -15, 30, 71, 99, 106,
		89, 51, 0, -55, -101, -127, -126, -97, -44, 23,
		89, 140, 164, 151, 104, 29, -59, -141, -199, -216,
		-186, -109, 0, 122, 229, 297, 305, 242, 114, -62,
		-253, -419, -518, -514, -382, -116, 268, 736, 1239, 1716,
		2109, 2367, 2464
	};
	protected ref array<int> m_History = {};
	protected int m_iNewest;
	protected int m_iNextInputIndex = -1;
	protected int m_iNextOutputIndex;

	//! The caller selects both sample origins and explicitly resets after a gap.
	bool Reset(int firstInputIndex, int firstOutputIndex)
	{
		if (firstInputIndex < 0 || firstInputIndex > int.MAX - INPUT_SAMPLES
			|| firstOutputIndex < 0 || firstOutputIndex > int.MAX - OUTPUT_SAMPLES) return false;
		m_History.Resize(HISTORY_SIZE);
		for (int historyIndex = 0; historyIndex < HISTORY_SIZE; ++historyIndex)
			m_History[historyIndex] = 0;
		m_iNewest = 0;
		m_iNextInputIndex = firstInputIndex;
		m_iNextOutputIndex = firstOutputIndex;
		return true;
	}

	//! Keep one sample per group of six. The signal retains its FIR group delay.
	array<int> Process(int firstInputIndex, array<int> samples, out int firstOutputIndex)
	{
		firstOutputIndex = -1;
		if (firstInputIndex < 0 || firstInputIndex != m_iNextInputIndex
			|| firstInputIndex > int.MAX - INPUT_SAMPLES || m_iNextOutputIndex > int.MAX - OUTPUT_SAMPLES
			|| !samples || samples.Count() != INPUT_SAMPLES) return null;
		foreach (int sample : samples)
		{
			if (sample < -32768 || sample > 32767) return null;
		}

		array<int> outputSamples = {};
		outputSamples.Reserve(OUTPUT_SAMPLES);
		for (int index = 0; index < INPUT_SAMPLES; ++index)
		{
			if (--m_iNewest < 0) m_iNewest = HISTORY_SIZE - 1;
			m_History[m_iNewest] = samples[index];
			if (index % RATIO != 0) continue;

			int historyIndex = m_iNewest;
			// Full tap L1 is 36084, so the PCM16 accumulator and rounding fit signed int32.
			int sum = 0;
			for (int tap = 0; tap <= LAST_TAP; ++tap)
			{
				int halfTap = tap;
				if (halfTap > GROUP_DELAY_INPUT_SAMPLES) halfTap = LAST_TAP - tap;
				sum += m_HalfTaps[halfTap] * m_History[historyIndex];
				if (++historyIndex == HISTORY_SIZE) historyIndex = 0;
			}

			int rounded;
			if (sum >= 0) rounded = (sum + Q14_HALF) / Q14_ONE;
			else rounded = -((-sum + Q14_HALF) / Q14_ONE);
			if (rounded < -32768) rounded = -32768;
			if (rounded > 32767) rounded = 32767;
			outputSamples.Insert(rounded);
		}
		firstOutputIndex = m_iNextOutputIndex;
		m_iNextInputIndex += INPUT_SAMPLES;
		m_iNextOutputIndex += OUTPUT_SAMPLES;
		return outputSamples;
	}
}
