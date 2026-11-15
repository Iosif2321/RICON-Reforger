//! Bounded file-backed playback candidate for the qualified Windows little-endian runtime.
class RICON_Pcm16WaveFile
{
	protected static const int SAMPLE_RATE = 48000;
	protected static const int HEADER_BYTES = 44;
	protected static const string PATH_PREFIX = "$profile:ricon-";
	protected static const string PATH_SUFFIX = ".wav";

	//! Caller owns a unique path and serializes access. FileMode.WRITE has no exclusive-create flag.
	static bool WriteNew(string path, array<int> samples)
	{
		if (!IsOwnedWavePath(path) || !samples || samples.Count() < 1 || samples.Count() > SAMPLE_RATE) return false;
		foreach (int sample : samples)
		{
			if (sample < -32768 || sample > 32767) return false;
		}

		int dataBytes = 2 * samples.Count();
		// RIFF/WAVE/fmt/data packed as little-endian words, with PCM tag 1 and one channel.
		array<int> header = {1179011410, 0, 1163280727, 544501094, 16, 65537, SAMPLE_RATE, 96000, 1048578, 1635017060, 0};
		header[1] = HEADER_BYTES - 8 + dataBytes;
		header[10] = dataBytes;

		if (FileIO.FileExists(path)) return false;
		FileHandle handle = FileIO.OpenFile(path, FileMode.WRITE);
		if (!handle) return false;
		bool complete = handle.WriteArray(header, 4) == HEADER_BYTES;
		if (complete) complete = handle.WriteArray(samples, 2) == dataBytes;
		handle.Close();

		if (complete)
		{
			handle = FileIO.OpenFile(path, FileMode.READ);
			if (handle)
			{
				complete = handle.GetLength() == HEADER_BYTES + dataBytes;
				handle.Close();
			}
			else complete = false;
		}
		if (!complete) FileIO.DeleteFile(path);
		return complete;
	}

	protected static bool IsOwnedWavePath(string path)
	{
		int length = path.Length();
		int prefixLength = PATH_PREFIX.Length();
		int suffixLength = PATH_SUFFIX.Length();
		if (length <= prefixLength + suffixLength || length > 128) return false;
		if (path.Substring(0, prefixLength) != PATH_PREFIX || path.Substring(length - suffixLength, suffixLength) != PATH_SUFFIX)
			return false;

		for (int index = prefixLength; index < length - suffixLength; index++)
		{
			int symbol = path.ToAscii(index);
			bool letter = (symbol >= 65 && symbol <= 90) || (symbol >= 97 && symbol <= 122);
			bool digit = symbol >= 48 && symbol <= 57;
			if (!letter && !digit && symbol != 45) return false;
		}
		return true;
	}
}
