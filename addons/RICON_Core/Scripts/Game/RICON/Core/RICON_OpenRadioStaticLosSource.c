//! One authoritative server epoch with a complete, read-only scenario roster.
//! Lifecycle and model time share the anchored process counter at millisecond resolution.
class RICON_OpenRadioStaticLosSource : RICON_OpenRadioTickSource
{
	protected BaseWorld m_World;
	protected RICON_LifecycleStateMachine m_Lifecycle;
	protected RICON_MediaStreamOwner m_Streams;
	protected RICON_MediaReceiverAuthority m_Receivers;
	protected ref RICON_OpenRadioPairScope m_Scope;
	protected ref array<ref RICON_OpenRadioPairEndpoint> m_Endpoints;
	protected ref array<ref RICON_OpenRadioPairListener> m_Listeners;
	protected ref RICON_StaticLosGeometryAuthority m_Geometry;
	protected ref RICON_OpenRadioListenerRoster m_ListenerRoster;
	protected RICON_TemporalProducerTick m_LastBuiltRfTick;
	protected int m_iEpoch, m_iProcessAnchorMs, m_iLastProcessMs, m_iTtlMs;
	protected int m_iLastNowMs = -1;
	protected float m_fQualityDelta;
	protected bool m_bClockFailed, m_bSamplePending, m_bReadClock;
	protected bool m_bControlledRfContinuity;
	protected int m_iLastBuiltRfMs = -1;
	protected int m_iLastIssuedRfMs = -1;
	protected string m_sLastBuildCode = "NOT_BUILT";

	//! Legacy callers keep their projection DTOs unchanged. Controlled coverage takes detached copies.
	bool Configure(BaseWorld world, RICON_LifecycleStateMachine lifecycle, RICON_MediaStreamOwner streams,
		RICON_MediaReceiverAuthority receivers, RICON_OpenRadioPairScope scope, array<ref RICON_OpenRadioPairEndpoint> endpoints,
		array<ref RICON_OpenRadioPairListener> listeners, RICON_StaticLosGeometryAuthority geometry,
		int processAnchorMs, int ttlMs, float qualityDelta, bool controlledStaticContinuity = false)
	{
		if (m_iEpoch || !world || !GetGame() || GetGame().GetWorld() != world || world.IsEditMode()
			|| !RICON_CommsManagerComponent.IsAuthority() || !lifecycle || !streams || !receivers
			|| !streams.IsScope(lifecycle) || !receivers.IsScope(lifecycle)) return false;
		if (!scope || !scope.m_Limits || !scope.m_Context || !endpoints || !listeners || !geometry
			|| !geometry.IsScope(world, lifecycle, receivers) || !geometry.IsCurrent()) return false;
		if (processAnchorMs < 0 || ttlMs <= 0 || !RICON_Num.IsFinite(qualityDelta) || qualityDelta <= 0 || qualityDelta > 1)
			return false;
		int rawMs = ReadProcessTickMs();
		if (rawMs < processAnchorMs || rawMs == int.MAX) return false;
		int nowMs = rawMs - processAnchorMs;
		if (!lifecycle.CanQueryAt(nowMs) || nowMs < streams.LastObservedTimeMs()) return false;

		RICON_OpenRadioPairScope storedScope = scope;
		array<ref RICON_OpenRadioPairEndpoint> storedEndpoints = endpoints;
		array<ref RICON_OpenRadioPairListener> storedListeners = listeners;
		if (controlledStaticContinuity)
		{
			if (!geometry.UsesControlledRfContinuity()) return false;
			storedScope = RICON_OpenRadioPairProjector.CopyControlledInputs(scope, endpoints, listeners, storedEndpoints, storedListeners);
			if (!storedScope) return false;
		}

		m_World = world;
		m_Lifecycle = lifecycle;
		m_Streams = streams;
		m_Receivers = receivers;
		m_Scope = storedScope;
		m_Endpoints = storedEndpoints;
		m_Listeners = storedListeners;
		m_Geometry = geometry;
		m_bControlledRfContinuity = controlledStaticContinuity;
		m_iEpoch = lifecycle.GetSessionEpoch();
		m_iProcessAnchorMs = processAnchorMs;
		m_iLastProcessMs = rawMs;
		m_iTtlMs = ttlMs;
		m_fQualityDelta = qualityDelta;
		return true;
	}

	//! Set once before the first clock read. The roster supplies detached complete membership on every tick.
	bool UseListenerRoster(RICON_OpenRadioListenerRoster roster)
	{
		if (!m_iEpoch || m_ListenerRoster || m_bReadClock || !roster
			|| !roster.IsScope(m_Lifecycle, m_Receivers, m_Scope.m_iNetworkId, m_Scope.m_iChannelId)) return false;
		m_ListenerRoster = roster;
		return true;
	}

	protected bool IsCurrent()
	{
		return m_iEpoch > 0 && m_World && GetGame() && GetGame().GetWorld() == m_World && !m_World.IsEditMode()
			&& RICON_CommsManagerComponent.IsAuthority() && m_Lifecycle && m_Lifecycle.GetSessionEpoch() == m_iEpoch
			&& m_Streams && m_Streams.IsScope(m_Lifecycle) && m_Receivers && m_Receivers.IsScope(m_Lifecycle)
			&& m_Geometry.IsScope(m_World, m_Lifecycle, m_Receivers);
	}

	protected int ReadProcessTickMs() { return System.GetTickCount(); }

	override int ReadNowMs()
	{
		m_bReadClock = true;
		m_bSamplePending = false;
		m_iLastNowMs = -1;
		m_LastBuiltRfTick = null;
		m_iLastBuiltRfMs = -1;
		if (m_bClockFailed || !IsCurrent()) return -1;
		int rawMs = ReadProcessTickMs();
		// Both operands are nonnegative and ordered before subtraction. A reset or rollover ends the epoch's clock.
		if (rawMs < m_iLastProcessMs || rawMs == int.MAX)
		{
			m_bClockFailed = true;
			m_Geometry.Invalidate();
			return -1;
		}
		m_iLastProcessMs = rawMs;
		m_iLastNowMs = rawMs - m_iProcessAnchorMs;
		m_bSamplePending = true;
		return m_iLastNowMs;
	}

	override RICON_TemporalProducerTick BuildTick(int nowMs)
	{
		m_LastBuiltRfTick = null;
		m_iLastBuiltRfMs = -1;
		if (m_bClockFailed || !m_bSamplePending || nowMs != m_iLastNowMs)
		{
			m_sLastBuildCode = "STALE_CLOCK_SAMPLE";
			return null;
		}
		m_bSamplePending = false;
		if (m_bControlledRfContinuity && nowMs <= m_iLastIssuedRfMs)
		{
			m_sLastBuildCode = "RF_TICK_ALREADY_ISSUED";
			return null;
		}
		if (!IsCurrent() || !m_Lifecycle.CanQueryAt(nowMs))
		{
			m_sLastBuildCode = "STALE_LIFECYCLE";
			return null;
		}
		RICON_PairServiceInput input;
		array<ref RICON_OpenRadioPairListener> listeners = m_Listeners;
		int rosterRevision;
		if (m_ListenerRoster)
		{
			listeners = {};
			rosterRevision = m_ListenerRoster.GetRevision();
			if (rosterRevision <= 0 || !m_ListenerRoster.Read(listeners))
			{
				m_sLastBuildCode = "LISTENER_ROSTER_UNAVAILABLE";
				return null;
			}
		}
		m_sLastBuildCode = RICON_OpenRadioPairProjector.Build(m_Lifecycle, m_Streams, m_Receivers, m_World, m_Scope,
			m_Endpoints, listeners, nowMs, input);
		if (m_sLastBuildCode != "OK") return null;
		RICON_TemporalProducerTick tick;
		m_sLastBuildCode = m_Geometry.BuildTick(input, nowMs, 0, m_iTtlMs, m_fQualityDelta, tick);
		if (tick && m_ListenerRoster)
		{
			if (m_ListenerRoster.GetRevision() != rosterRevision)
			{
				m_sLastBuildCode = "STALE_LISTENER_ROSTER";
				return null;
			}
			tick.m_ListenerRoster = m_ListenerRoster;
			tick.m_iListenerRosterRevision = rosterRevision;
		}
		if (tick && m_bControlledRfContinuity)
		{
			if (!m_Geometry.CanAttestRfContinuity(nowMs))
			{
				m_sLastBuildCode = "RF_CONTINUITY_UNAVAILABLE";
				return null;
			}
			tick.m_StaticRfSource = this;
			m_LastBuiltRfTick = tick;
			m_iLastBuiltRfMs = nowMs;
			m_iLastIssuedRfMs = nowMs;
		}
		return tick;
	}

	bool UsesControlledRfContinuity() { return m_bControlledRfContinuity; }

	//! A trusted scenario owns stationary entities and invalidates its seal before external RF changes.
	bool CanAttestRfCoverage(int nowMs)
	{
		if (!m_bControlledRfContinuity || nowMs < 0 || m_iLastBuiltRfMs != nowMs || m_iLastNowMs != nowMs) return false;
		return !m_bClockFailed && IsCurrent() && m_Geometry.CanAttestRfContinuity(nowMs);
	}

	bool MatchesBuiltRfTick(RICON_TemporalProducerTick tick, int nowMs)
	{
		return tick && tick == m_LastBuiltRfTick && tick.m_StaticRfSource == this && CanAttestRfCoverage(nowMs);
	}

	string GetLastBuildCode() { return m_sLastBuildCode; }
	int GetLastNowMs() { return m_iLastNowMs; }
	bool IsClockFailed() { return m_bClockFailed; }
}
