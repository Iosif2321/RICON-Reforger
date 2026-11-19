enum RICON_EPcmWaveClipState
{
	PREPARED,
	ACTIVE,
	RETIRED
}

class RICON_PcmWaveClip
{
	int m_iId, m_iExpiresAtMs, m_iDurationMs, m_iNominalEndMs, m_iDeleteAfterMs;
	string m_sPath;
	RICON_EPcmWaveClipState m_eState;
	AudioHandle m_Handle = AudioHandle.Invalid;
	bool m_bRequireCompleteDeadline;
	ref array<ref RICON_MediaOutputPermit> m_Permits;
	protected bool m_bFiniteRx;
	protected RICON_LocalRxSlot m_FiniteRxSlot;

	void BindFiniteRxSlot(RICON_LocalRxSlot slot)
	{
		m_bFiniteRx = true;
		m_FiniteRxSlot = slot;
	}

	bool HasCurrentRxSlot()
	{
		return !m_bFiniteRx || (m_FiniteRxSlot && m_FiniteRxSlot.IsOpen());
	}
}

//! Finite local WAV playback. The caller serializes access and never reuses an owner ID in this process/profile.
class RICON_PcmWavePlaybackOwner
{
	protected int m_iEpoch, m_iOwnerId, m_iCapacity, m_iGraceMs, m_iNowMs;
	protected int m_iClipSerial, m_iNextSampleIndex, m_iOutputFenceMs;
	protected bool m_bClosed, m_bCheckingPermits;
	protected ref array<ref RICON_PcmWaveClip> m_aClips = {};
	protected RICON_PcmWaveClip m_Active;

	bool Configure(int epoch, int ownerId, int capacity, int retireGraceMs, int nowMs)
	{
		if (m_iEpoch || epoch <= 0 || epoch >= int.MAX || ownerId <= 0) return false;
		if (capacity < 1 || capacity > 8 || retireGraceMs < 250 || retireGraceMs > 2000) return false;
		if (nowMs < 0 || nowMs > int.MAX - 1000 - retireGraceMs) return false;
		m_iEpoch = epoch;
		m_iOwnerId = ownerId;
		m_iCapacity = capacity;
		m_iGraceMs = retireGraceMs;
		m_iNowMs = nowMs;
		m_aClips.Reserve(capacity);
		return true;
	}

	string Prepare(RICON_PcmBatch batch, int nowMs, out int clipId)
	{
		return PrepareClip(batch, nowMs, null, clipId);
	}

	string PrepareWire(RICON_PcmBatch batch, int nowMs, RICON_MediaOutputPermit permit, out int clipId)
	{
		clipId = 0;
		if (!permit) return "ROUTE_REVOKED";
		array<ref RICON_MediaOutputPermit> permits = {permit};
		return PrepareClip(batch, nowMs, permits, clipId);
	}

	//! The clip depends on every distinct permit of its PCM contributions.
	string PrepareWireMix(RICON_PcmBatch batch, int nowMs, array<ref RICON_MediaOutputPermit> permits, out int clipId)
	{
		clipId = 0;
		if (!permits || permits.Count() < 1 || permits.Count() > 16) return "ROUTE_REVOKED";
		return PrepareClip(batch, nowMs, permits, clipId);
	}

	//! Finite diagnostic sink for a committed 4 ms shared RX window.
	string PrepareSharedRx(RICON_SharedRxMixResult window, int nowMs, out int clipId)
	{
		clipId = 0;
		if (!window || !window.m_Destination || !window.m_Destination.IsValid()
			|| !RICON_SharedRxTestMixer.IsPcmWindow(window.m_aSamples) || window.m_iWireDeadlineMs < 0) return "BAD_BATCH";
		if (!window.m_aPermits || window.m_aPermits.Count() < 1 || window.m_aPermits.Count() > 16) return "ROUTE_REVOKED";
		RICON_PcmBatch batch = new RICON_PcmBatch();
		batch.m_iEpoch = window.m_iEpoch;
		batch.m_iFirstSampleIndex = window.m_iFirstSampleIndex;
		batch.m_iBlockCount = 1;
		batch.m_iExpiresAtMs = window.m_iWireDeadlineMs;
		batch.m_aSamples = window.m_aSamples;
		return PrepareClip(batch, nowMs, window.m_aPermits, clipId, true);
	}

	//! A complete, contiguous clip handed off by one local RX slot with its original permits.
	string PrepareFiniteRxClip(RICON_PcmBatch batch, array<ref RICON_MediaOutputPermit> permits,
		RICON_LocalRxSlot slot, int nowMs, out int clipId)
	{
		clipId = 0;
		if (!slot || !slot.IsOpen() || !permits || permits.Count() < 1 || permits.Count() > 16) return "ROUTE_REVOKED";
		return PrepareClip(batch, nowMs, permits, clipId, true, slot);
	}

	protected string PrepareClip(RICON_PcmBatch batch, int nowMs, array<ref RICON_MediaOutputPermit> permits,
		out int clipId, bool requireCompleteDeadline = false, RICON_LocalRxSlot finiteRxSlot = null)
	{
		clipId = 0;
		if (m_bClosed) return "CLOSED";
		if (m_bCheckingPermits) return "BUSY";
		if (!batch) return "BAD_BATCH";
		int epoch = batch.m_iEpoch;
		if (!m_iEpoch || epoch != m_iEpoch) return "STALE_EPOCH";
		if (!ObserveTime(nowMs)) return "BAD_TIME";
		bool finiteRx = finiteRxSlot != null;
		array<ref RICON_MediaOutputPermit> ownedPermits;
		if (permits)
		{
			ownedPermits = {};
			foreach (RICON_MediaOutputPermit permit : permits)
			{
				if (ownedPermits.Contains(permit)) return "ROUTE_REVOKED";
				ownedPermits.Insert(permit);
			}
		}
		bool permitted = PermitsCurrent(ownedPermits);
		if (m_bClosed) return "CLOSED";
		if (epoch != m_iEpoch || epoch != batch.m_iEpoch) return "STALE_EPOCH";
		if (nowMs != m_iNowMs) return "BAD_TIME";
		if (!permitted || (finiteRx && (!finiteRxSlot || !finiteRxSlot.IsOpen()))) return "ROUTE_REVOKED";
		if (!batch.m_aSamples || batch.m_iBlockCount < 1 || batch.m_iBlockCount > 4 || batch.m_iExpiresAtMs < 0)
			return "BAD_BATCH";
		int count = batch.m_aSamples.Count();
		if (count < batch.m_iBlockCount || count > 48000 || count % batch.m_iBlockCount != 0)
			return "BAD_BATCH";
		int first = batch.m_iFirstSampleIndex;
		int alignment = count / batch.m_iBlockCount;
		if (finiteRx)
		{
			alignment = RICON_SharedRxTestMixer.WINDOW_SAMPLES;
			if (batch.m_iBlockCount != 1 || count < alignment || count > 16 * alignment || count % alignment != 0)
				return "BAD_BATCH";
		}
		if (first < 0 || first > int.MAX - count || first % alignment != 0) return "BAD_BATCH";
		if (first < m_iNextSampleIndex) return "STALE_SAMPLE";
		int durationMs = (count + 47) / 48;
		if (nowMs >= batch.m_iExpiresAtMs || (requireCompleteDeadline && batch.m_iExpiresAtMs - nowMs < durationMs))
		{
			m_iNextSampleIndex = first + count;
			return "LATE";
		}
		if (m_aClips.Count() >= m_iCapacity || m_iClipSerial >= int.MAX - 1) return "FULL";

		RICON_PcmWaveClip clip = new RICON_PcmWaveClip();
		m_iClipSerial++;
		clip.m_iId = m_iClipSerial;
		clip.m_sPath = string.Format("$profile:ricon-owned-pcm-%1-%2.wav", m_iOwnerId, clip.m_iId);
		if (FileIO.FileExists(clip.m_sPath)) return "WRITE_FAILED";
		if (!RICON_Pcm16WaveFile.WriteNew(clip.m_sPath, batch.m_aSamples))
		{
			if (FileIO.FileExists(clip.m_sPath))
			{
				Retire(clip, nowMs + m_iGraceMs);
				m_aClips.Insert(clip);
			}
			return "WRITE_FAILED";
		}

		clip.m_eState = RICON_EPcmWaveClipState.PREPARED;
		clip.m_iDurationMs = durationMs;
		clip.m_iExpiresAtMs = batch.m_iExpiresAtMs;
		clip.m_bRequireCompleteDeadline = requireCompleteDeadline;
		clip.m_Permits = ownedPermits;
		if (finiteRx) clip.BindFiniteRxSlot(finiteRxSlot);
		m_aClips.Insert(clip);
		m_iNextSampleIndex = first + count;
		clipId = clip.m_iId;
		return "OK";
	}

	string Submit(int epoch, int clipId, int nowMs)
	{
		if (m_bClosed) return "CLOSED";
		if (m_bCheckingPermits) return "BUSY";
		if (!m_iEpoch || epoch != m_iEpoch) return "STALE_EPOCH";
		if (!ObserveTime(nowMs)) return "BAD_TIME";
		SweepRevoked(nowMs, clipId);
		// A native stop during the sweep precedes all final submission checks.
		if (m_bClosed) return "CLOSED";
		if (epoch != m_iEpoch) return "STALE_EPOCH";
		if (nowMs != m_iNowMs) return "BAD_TIME";
		ref RICON_PcmWaveClip clip = FindClip(clipId);
		if (!clip || clip.m_eState != RICON_EPcmWaveClipState.PREPARED) return "STALE_CLIP";
		bool permitted = PermitsCurrent(clip.m_Permits);
		if (m_bClosed) return "CLOSED";
		if (epoch != m_iEpoch) return "STALE_EPOCH";
		if (nowMs != m_iNowMs) return "BAD_TIME";
		if (clip.m_eState != RICON_EPcmWaveClipState.PREPARED || FindClip(clipId) != clip) return "STALE_CLIP";
		if (!permitted || !clip.HasCurrentRxSlot())
		{
			Retire(clip, nowMs + m_iGraceMs);
			return "ROUTE_REVOKED";
		}
		if (nowMs >= clip.m_iExpiresAtMs || (clip.m_bRequireCompleteDeadline && clip.m_iExpiresAtMs - nowMs < clip.m_iDurationMs))
		{
			Retire(clip, nowMs + m_iGraceMs);
			return "LATE";
		}
		if (m_Active || nowMs < m_iOutputFenceMs) return "BUSY";
		foreach (RICON_PcmWaveClip older : m_aClips)
		{
			if (older == clip) break;
			if (older.m_eState == RICON_EPcmWaveClipState.PREPARED && nowMs < older.m_iExpiresAtMs) return "BUSY";
		}

		clip.m_iNominalEndMs = nowMs + clip.m_iDurationMs;
		int collectAtMs = clip.m_iNominalEndMs + m_iGraceMs;
		// A submission attempt is irreversible even if the engine returns no usable handle.
		Retire(clip, collectAtMs);
		clip.m_Handle = AudioSystem.PlaySound(clip.m_sPath);
		if (clip.m_Handle == AudioHandle.Invalid)
		{
			m_iOutputFenceMs = collectAtMs;
			return "SUBMIT_UNKNOWN";
		}
		clip.m_eState = RICON_EPcmWaveClipState.ACTIVE;
		m_Active = clip;
		return "OK";
	}

	bool ResetEpoch(int epoch, int nowMs)
	{
		if (m_bClosed || epoch <= m_iEpoch || epoch >= int.MAX || !ObserveTime(nowMs)) return false;
		m_iEpoch = epoch;
		m_iNextSampleIndex = 0;
		RetireCurrent(nowMs);
		return true;
	}

	bool Stop(int nowMs)
	{
		if (!ObserveTime(nowMs)) return false;
		if (m_bClosed) return true;
		m_bClosed = true;
		RetireCurrent(nowMs);
		return true;
	}

	int Collect(int nowMs)
	{
		if (m_bCheckingPermits || !ObserveTime(nowMs)) return 0;
		SweepRevoked(nowMs);
		if (nowMs != m_iNowMs) return 0;
		int removed = 0;
		for (int index = m_aClips.Count() - 1; index >= 0; index--)
		{
			RICON_PcmWaveClip clip = m_aClips[index];
			if (clip.m_eState == RICON_EPcmWaveClipState.PREPARED && nowMs >= clip.m_iExpiresAtMs) Retire(clip, nowMs);
			if (clip.m_eState != RICON_EPcmWaveClipState.RETIRED || nowMs < clip.m_iDeleteAfterMs) continue;
			if (FileIO.FileExists(clip.m_sPath)) FileIO.DeleteFile(clip.m_sPath);
			if (FileIO.FileExists(clip.m_sPath)) continue;
			m_aClips.RemoveOrdered(index);
			removed++;
		}
		return removed;
	}

	int Count() { return m_aClips.Count(); }

	int ActiveCount()
	{
		if (m_Active) return 1;
		return 0;
	}

	int GetEpoch() { return m_iEpoch; }

	string PathForClip(int clipId)
	{
		RICON_PcmWaveClip clip = FindClip(clipId);
		if (!clip) return "";
		return clip.m_sPath;
	}

	protected RICON_PcmWaveClip FindClip(int clipId)
	{
		foreach (RICON_PcmWaveClip clip : m_aClips)
		{
			if (clip.m_iId == clipId) return clip;
		}
		return null;
	}

	protected bool ObserveTime(int nowMs)
	{
		if (!m_iEpoch || nowMs < m_iNowMs || nowMs > int.MAX - 1000 - m_iGraceMs) return false;
		m_iNowMs = nowMs;
		if (m_Active && nowMs >= m_Active.m_iDeleteAfterMs)
		{
			Retire(m_Active, m_Active.m_iDeleteAfterMs);
			m_Active = null;
		}
		return true;
	}

	protected void RetireCurrent(int nowMs)
	{
		foreach (RICON_PcmWaveClip clip : m_aClips)
		{
			if (clip.m_eState == RICON_EPcmWaveClipState.PREPARED) Retire(clip, nowMs + m_iGraceMs);
		}
		if (m_Active) RetireActive(nowMs);
	}

	protected void SweepRevoked(int nowMs, int exceptClipId = 0)
	{
		foreach (RICON_PcmWaveClip clip : m_aClips)
		{
			if (clip.m_iId == exceptClipId || clip.m_eState != RICON_EPcmWaveClipState.PREPARED) continue;
			bool permitted = PermitsCurrent(clip.m_Permits);
			if (nowMs != m_iNowMs || m_bClosed) return;
			if (clip.m_eState == RICON_EPcmWaveClipState.PREPARED && (!permitted || !clip.HasCurrentRxSlot()))
				Retire(clip, nowMs + m_iGraceMs);
		}
		ref RICON_PcmWaveClip active = m_Active;
		if (!active) return;
		bool activePermitted = PermitsCurrent(active.m_Permits);
		if (nowMs == m_iNowMs && m_Active == active && (!activePermitted || !active.HasCurrentRxSlot())) RetireActive(nowMs);
	}

	protected bool PermitsCurrent(array<ref RICON_MediaOutputPermit> permits)
	{
		if (!permits) return true;
		m_bCheckingPermits = true;
		bool permitted = true;
		foreach (RICON_MediaOutputPermit permit : permits)
		{
			if (permit && permit.IsCurrent()) continue;
			permitted = false;
			break;
		}
		m_bCheckingPermits = false;
		return permitted;
	}

	protected void RetireActive(int nowMs)
	{
		int collectAtMs = m_Active.m_iDeleteAfterMs;
		AudioHandle handle = AudioHandle.Invalid;
		// The nominal end does not mean the engine has started this guarded clip.
		if (m_Active.m_bRequireCompleteDeadline || nowMs < m_Active.m_iNominalEndMs)
		{
			handle = m_Active.m_Handle;
			collectAtMs = nowMs + m_iGraceMs;
		}
		m_iOutputFenceMs = collectAtMs;
		Retire(m_Active, collectAtMs);
		m_Active = null;
		if (handle != AudioHandle.Invalid) AudioSystem.TerminateSound(handle);
	}

	protected void Retire(RICON_PcmWaveClip clip, int deleteAfterMs)
	{
		clip.m_eState = RICON_EPcmWaveClipState.RETIRED;
		clip.m_Handle = AudioHandle.Invalid;
		clip.m_iDeleteAfterMs = deleteAfterMs;
	}
}
