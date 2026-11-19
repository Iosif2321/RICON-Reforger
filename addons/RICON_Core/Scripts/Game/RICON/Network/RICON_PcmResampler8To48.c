//! Streaming mono PCM16 interpolation for the synthetic V1 profile.
class RICON_PcmResampler8To48
{
	static const int INPUT_RATE_HZ = RICON_MediaPcmTestProfile.SAMPLE_RATE_HZ;
	static const int INPUT_SAMPLES = RICON_MediaPcmTestProfile.FRAME_SAMPLES;
	static const int OUTPUT_RATE_HZ = 48000;
	static const int OUTPUT_SAMPLES = 192;
	static const int GROUP_DELAY_OUTPUT_SAMPLES = 48;

	protected static const int RATIO = 6;
	protected static const int LAST_TAP = 96;
	protected static const int HISTORY_SIZE = 17;
	protected static const int Q14_ONE = 16384;
	protected static const int Q14_HALF = 8192;

	// For k=-48..48, a[k]=sinc(k/6)*(1+cos(pi*k/48))/2, with sinc(x)=sin(pi*x)/(pi*x).
	// Round 6*16384*a[k]/sum(a), then adjust indices 45..51 by {-1,-2,2,3,2,-2,-1} for exact phase DC gain.
	protected ref array<int> m_HalfTaps = {
		0, 0, -3, -7, -10, -10, 0, 20, 45, 68,
		74, 52, 0, -76, -156, -211, -212, -141, 0, 183,
		359, 466, 453, 292, 0, -362, -696, -890, -853, -544,
		0, 664, 1270, 1622, 1557, 998, 0, -1246, -2429, -3183,
		-3160, -2119, 0, 3045, 6658, 10327, 13487, 15628, 16384
	};
	protected ref array<int> m_History = {};
	protected int m_iNewest;
	protected int m_iNextSourceIndex = -1;

	//! Start or explicitly restart with zero history. The caller owns gap policy and terminal trim.
	bool Reset(int firstSourceIndex)
	{
		if (firstSourceIndex < 0 || firstSourceIndex > (int.MAX - OUTPUT_SAMPLES) / RATIO) return false;
		m_History.Resize(HISTORY_SIZE);
		for (int historyIndex = 0; historyIndex < HISTORY_SIZE; ++historyIndex)
			m_History[historyIndex] = 0;
		m_iNewest = 0;
		m_iNextSourceIndex = firstSourceIndex;
		return true;
	}

	//! Output interval starts at 6*firstSourceIndex. Its signal retains the FIR group delay.
	array<int> Process(int firstSourceIndex, array<int> samples, out int firstOutputIndex)
	{
		firstOutputIndex = -1;
		if (firstSourceIndex < 0 || firstSourceIndex != m_iNextSourceIndex
			|| firstSourceIndex > (int.MAX - OUTPUT_SAMPLES) / RATIO || !samples || samples.Count() != INPUT_SAMPLES) return null;
		foreach (int sample : samples)
		{
			if (sample < -32768 || sample > 32767) return null;
		}

		array<int> outputSamples = {};
		outputSamples.Reserve(OUTPUT_SAMPLES);
		foreach (int sample : samples)
		{
			if (--m_iNewest < 0) m_iNewest = HISTORY_SIZE - 1;
			m_History[m_iNewest] = sample;
			for (int phase = 0; phase < RATIO; ++phase)
			{
				int historyIndex = m_iNewest;
				// The largest phase L1 norm is 33548. Products and their sum fit signed int32.
				int sum = 0;
				for (int tap = phase; tap <= LAST_TAP; tap += RATIO)
				{
					int halfTap = tap;
					if (halfTap > GROUP_DELAY_OUTPUT_SAMPLES) halfTap = LAST_TAP - tap;
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
		}
		firstOutputIndex = firstSourceIndex * RATIO;
		m_iNextSourceIndex += INPUT_SAMPLES;
		return outputSamples;
	}
}
