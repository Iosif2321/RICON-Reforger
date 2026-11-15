class RICON_PcmBatch
{
	int m_iEpoch;
	int m_iFirstSampleIndex;
	int m_iBlockCount;
	int m_iExpiresAtMs;
	ref array<int> m_aSamples = {};
}

class RICON_PendingPcmBlock
{
	int m_iFirstSampleIndex;
	int m_iExpiresAtMs;
	ref array<int> m_aSamples = {};
}

//! Local mono PCM16 at 48 kHz. The caller supplies its monotonic playback clock in milliseconds.
class RICON_PcmBatchQueue
{
	protected int m_iEpoch;
	protected int m_iBlockSamples;
	protected int m_iCapacity;
	protected int m_iMaxBatchBlocks;
	protected int m_iLastNowMs;
	protected int m_iNextSampleIndex;
	protected int m_iExpiredCount;
	protected ref array<ref RICON_PendingPcmBlock> m_aPending = {};

	bool Configure(int epoch, int blockSamples, int capacity, int maxBatchBlocks, int nowMs)
	{
		if (m_iEpoch > 0 || epoch <= 0 || epoch >= int.MAX || nowMs < 0) return false;
		if (blockSamples < 1 || blockSamples > 4800 || capacity < 1 || capacity > 8) return false;
		if (maxBatchBlocks < 1 || maxBatchBlocks > 4 || maxBatchBlocks > capacity) return false;
		m_iEpoch = epoch;
		m_iBlockSamples = blockSamples;
		m_iCapacity = capacity;
		m_iMaxBatchBlocks = maxBatchBlocks;
		m_iLastNowMs = nowMs;
		m_aPending.Reserve(capacity);
		return true;
	}

	bool ResetEpoch(int epoch, int nowMs)
	{
		if (m_iEpoch <= 0 || epoch <= m_iEpoch || epoch >= int.MAX || nowMs < m_iLastNowMs) return false;
		m_aPending.Clear();
		m_iEpoch = epoch;
		m_iLastNowMs = nowMs;
		m_iNextSampleIndex = 0;
		return true;
	}

	string Offer(int epoch, int firstSampleIndex, int expiresAtMs, array<int> samples, int nowMs)
	{
		if (m_iEpoch <= 0 || epoch != m_iEpoch) return "STALE_EPOCH";
		if (nowMs < m_iLastNowMs) return "BAD_TIME";
		ObserveTime(nowMs);
		if (!samples || samples.Count() != m_iBlockSamples || expiresAtMs < 0) return "BAD_BLOCK";
		if (firstSampleIndex < 0 || firstSampleIndex > int.MAX - m_iBlockSamples) return "BAD_BLOCK";
		if (firstSampleIndex % m_iBlockSamples != 0) return "BAD_BLOCK";
		if (firstSampleIndex < m_iNextSampleIndex) return "STALE_SAMPLE";
		foreach (int sample : samples)
		{
			if (sample < -32768 || sample > 32767) return "BAD_BLOCK";
		}
		if (nowMs >= expiresAtMs)
		{
			// A rejected late interval cannot re-enter under a later deadline.
			m_iNextSampleIndex = firstSampleIndex + m_iBlockSamples;
			CountExpiry();
			return "LATE";
		}
		if (m_aPending.Count() == m_iCapacity) return "FULL";
		RICON_PendingPcmBlock block = new RICON_PendingPcmBlock();
		block.m_iFirstSampleIndex = firstSampleIndex;
		block.m_iExpiresAtMs = expiresAtMs;
		block.m_aSamples.Copy(samples);
		m_aPending.Insert(block);
		m_iNextSampleIndex = firstSampleIndex + m_iBlockSamples;
		return "OK";
	}

	RICON_PcmBatch Take(int epoch, int maxBlocks, int nowMs)
	{
		if (m_iEpoch <= 0 || epoch != m_iEpoch || nowMs < m_iLastNowMs || maxBlocks <= 0) return null;
		ObserveTime(nowMs);
		if (m_aPending.IsEmpty()) return null;

		int limit = Math.Min(m_aPending.Count(), Math.Min(maxBlocks, m_iMaxBatchBlocks));
		RICON_PcmBatch batch = new RICON_PcmBatch();
		batch.m_iEpoch = epoch;
		batch.m_iFirstSampleIndex = m_aPending[0].m_iFirstSampleIndex;
		batch.m_iExpiresAtMs = m_aPending[0].m_iExpiresAtMs;
		batch.m_aSamples.Reserve(limit * m_iBlockSamples);
		int nextSampleIndex = batch.m_iFirstSampleIndex;
		for (int index = 0; index < limit; ++index)
		{
			RICON_PendingPcmBlock block = m_aPending[index];
			if (block.m_iFirstSampleIndex != nextSampleIndex) break;
			batch.m_aSamples.InsertAll(block.m_aSamples);
			batch.m_iExpiresAtMs = Math.Min(batch.m_iExpiresAtMs, block.m_iExpiresAtMs);
			batch.m_iBlockCount++;
			nextSampleIndex += m_iBlockSamples;
		}
		int remaining = m_aPending.Count() - batch.m_iBlockCount;
		for (int index = 0; index < remaining; ++index)
			m_aPending[index] = m_aPending[index + batch.m_iBlockCount];
		m_aPending.Resize(remaining);
		return batch;
	}

	int Count() { return m_aPending.Count(); }
	int GetExpiredCount() { return m_iExpiredCount; }

	protected void ObserveTime(int nowMs)
	{
		m_iLastNowMs = nowMs;
		int kept = 0;
		for (int index = 0; index < m_aPending.Count(); ++index)
		{
			RICON_PendingPcmBlock block = m_aPending[index];
			if (nowMs >= block.m_iExpiresAtMs)
			{
				CountExpiry();
				continue;
			}
			if (kept != index) m_aPending[kept] = block;
			kept++;
		}
		if (kept < m_aPending.Count()) m_aPending.Resize(kept);
	}

	protected void CountExpiry()
	{
		if (m_iExpiredCount < int.MAX) m_iExpiredCount++;
	}
}
