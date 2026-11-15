//! Experimental fixed mono PCM16-LE profile; identity remains TEST_ONLY until qualified.
class RICON_MediaPcmProfile
{
	static const string PROFILE_ID = "TEST_ONLY_PCM_S16LE_8K_V1";
	static const int SAMPLE_RATE_HZ = 8000;
	static const int FRAME_SAMPLES = 32;
	static const int FRAME_DURATION_MS = 4;
	static const int PAYLOAD_BYTES = 64;

	static array<int> Encode(array<int> samples)
	{
		if (!samples || samples.Count() != FRAME_SAMPLES) return null;
		array<int> bytes = {};
		bytes.Reserve(PAYLOAD_BYTES);
		foreach (int sample : samples)
		{
			if (sample < -32768 || sample > 32767) return null;
			bytes.Insert(sample & 255);
			bytes.Insert((sample >> 8) & 255);
		}
		return bytes;
	}

	//! The caller must acquire the decision from its exact authorized RX route.
	static array<int> Decode(RICON_MediaStreamBinding source, RICON_MediaPlayoutDecision decision)
	{
		if (!source || !source.IsValid() || source.m_sProfileId != PROFILE_ID) return null;
		if (!decision || decision.m_sKind != "PLAY" || decision.m_iSlotCount != 1 || decision.m_iSampleCount != FRAME_SAMPLES) return null;
		if (decision.m_iSequence < 0 || decision.m_iSequence >= int.MAX || decision.m_iFirstSampleIndex < 0
			|| decision.m_iFirstSampleIndex > int.MAX - FRAME_SAMPLES) return null;

		RICON_QueuedMediaBlock block = decision.m_Block;
		if (!block || !source.Same(block.m_Binding) || block.m_iSequence != decision.m_iSequence
			|| block.m_iFirstSampleIndex != decision.m_iFirstSampleIndex || block.m_iSampleCount != FRAME_SAMPLES) return null;
		return DecodeBytes(block.m_Bytes);
	}

	static array<int> DecodeBytes(array<int> bytes)
	{
		if (!bytes || bytes.Count() != PAYLOAD_BYTES) return null;
		array<int> samples = {};
		samples.Reserve(FRAME_SAMPLES);
		for (int index = 0; index < PAYLOAD_BYTES; index += 2)
		{
			int low = bytes[index];
			int high = bytes[index + 1];
			if (low < 0 || low > 255 || high < 0 || high > 255) return null;
			int sample = low | (high << 8);
			if (sample >= 32768) sample -= 65536;
			samples.Insert(sample);
		}
		return samples;
	}
}
