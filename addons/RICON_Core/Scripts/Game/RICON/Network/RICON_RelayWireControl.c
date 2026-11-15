//! One fixed station direction per manager epoch. Install and revoke have distinct ordered revisions.
class RICON_RelayWireControl
{
	static const int INSTALL_SEQ = 1;
	static const int REVOKE_SEQ = 2;
	static const int FIELD_COUNT = RICON_RelayWire.ROUTE_VALUES + 2;
	static const int V2_FIELD_COUNT = FIELD_COUNT + 2;

	static array<int> Encode(RICON_RelayWireGrant grant)
	{
		if (!grant || !grant.IsValid()) return null;
		array<int> fields = grant.RouteFields();
		fields.Insert(grant.m_iFirstSequence);
		fields.Insert(grant.m_iFirstSampleIndex);
		if (grant.m_iWireVersion == 2)
		{
			fields.Insert(grant.m_iOriginFirstSequence);
			fields.Insert(grant.m_iOriginFirstSampleIndex);
		}
		return fields;
	}

	static RICON_RelayWireGrant Decode(array<int> fields, string timebaseId)
	{
		if (!fields || fields.IsEmpty()) return null;
		bool v2 = fields[0] == RICON_RelayWire.DISCRIMINATOR_V2;
		if (v2)
		{
			if (fields.Count() != V2_FIELD_COUNT) return null;
		}
		else if (fields[0] != RICON_RelayWire.DISCRIMINATOR || fields.Count() != FIELD_COUNT) return null;

		RICON_RelayWireGrant grant = new RICON_RelayWireGrant();
		grant.m_iEpoch = fields[1];
		grant.m_iEmissionId = fields[2];
		grant.m_iBridgeGeneration = fields[3];
		grant.m_iTransformGeneration = fields[4];
		grant.m_iTxDeviceId = fields[5];
		grant.m_iTxIndex = fields[6];
		grant.m_iTxGeneration = fields[7];
		grant.m_iOriginStreamId = fields[8];
		grant.m_iOriginGeneration = fields[9];
		grant.m_iOriginCaptureEpoch = fields[10];
		grant.m_iFirstSequence = fields[18];
		grant.m_iFirstSampleIndex = fields[19];
		if (v2)
		{
			grant.m_iWireVersion = 2;
			grant.m_iOriginFirstSequence = fields[FIELD_COUNT];
			grant.m_iOriginFirstSampleIndex = fields[FIELD_COUNT + 1];
		}
		grant.m_sTimebaseId = timebaseId;
		RICON_RelayDestination destination = new RICON_RelayDestination();
		destination.m_iPlayerId = fields[11];
		destination.m_iConnectionId = fields[12];
		destination.m_iPlayerGeneration = fields[13];
		destination.m_iEntityId = fields[14];
		destination.m_iRxDeviceId = fields[15];
		destination.m_iRxIndex = fields[16];
		destination.m_iRxGeneration = fields[17];
		grant.m_Destination = destination;
		if (!grant.IsValid()) return null;
		return grant;
	}
}

//! Exact receiver readiness. This body never installs permissions or carries media.
class RICON_RelayWireReady
{
	static const int FIELD_COUNT = 8;

	static array<int> Encode(int epoch, RICON_RelayDestination destination)
	{
		if (epoch <= 0 || epoch >= int.MAX || !destination || !destination.IsValid()) return null;
		array<int> body = {epoch, destination.m_iPlayerId, destination.m_iConnectionId, destination.m_iPlayerGeneration,
			destination.m_iEntityId, destination.m_iRxDeviceId, destination.m_iRxIndex, destination.m_iRxGeneration};
		return body;
	}

	static bool Matches(array<int> body, int epoch, RICON_RelayDestination destination)
	{
		return body && body.Count() == FIELD_COUNT && destination && body[0] == epoch
			&& body[1] == destination.m_iPlayerId && body[2] == destination.m_iConnectionId
			&& body[3] == destination.m_iPlayerGeneration && body[4] == destination.m_iEntityId
			&& body[5] == destination.m_iRxDeviceId && body[6] == destination.m_iRxIndex && body[7] == destination.m_iRxGeneration;
	}

	protected ref RICON_RelayDestination m_Destination;
	protected int m_iEpoch, m_iFromMs, m_iDeadlineMs;
	protected bool m_bConsumed;

	bool Begin(int epoch, RICON_RelayDestination destination, int nowMs, int timeoutMs)
	{
		if (m_Destination || epoch <= 0 || epoch >= int.MAX || !destination || !destination.IsValid()
			|| nowMs < 0 || timeoutMs <= 0 || timeoutMs > 10000
			|| nowMs > int.MAX - timeoutMs) return false;
		m_Destination = destination.CloneDestination();
		m_iEpoch = epoch;
		m_iFromMs = nowMs;
		m_iDeadlineMs = nowMs + timeoutMs;
		return true;
	}

	bool MatchesReceiver(array<int> body)
	{
		return !m_bConsumed && Matches(body, m_iEpoch, m_Destination);
	}

	bool Accept(array<int> body, int nowMs)
	{
		if (!MatchesReceiver(body) || nowMs < m_iFromMs || nowMs >= m_iDeadlineMs) return false;
		m_bConsumed = true;
		return true;
	}

	int Remaining(int nowMs)
	{
		if (!m_Destination || m_bConsumed || nowMs < m_iFromMs) return -1;
		return m_iDeadlineMs - nowMs;
	}
}

//! One local server publication. The caller supplies the live, authenticated recipient tuple and server time.
class RICON_RelayWirePublication
{
	protected ref RICON_RelayWireGrant m_Grant;
	protected int m_iAckDeadlineMs, m_iLastNowMs;
	protected bool m_bConfirmed, m_bClosed;

	bool Begin(RICON_RelayWireGrant grant, int nowMs, int timeoutMs)
	{
		if (m_bClosed || m_Grant || !grant || !grant.IsValid() || nowMs < 0 || timeoutMs < 1
			|| timeoutMs > 10000 || nowMs > int.MAX - timeoutMs) return false;
		m_Grant = grant.CloneGrant();
		m_iLastNowMs = nowMs;
		m_iAckDeadlineMs = nowMs + timeoutMs;
		return true;
	}

	bool Confirm(int controlSeq, RICON_RelayWireGrant echo, RICON_RelayDestination recipient, int nowMs)
	{
		if (m_bClosed || !m_Grant || controlSeq != RICON_RelayWireControl.INSTALL_SEQ || nowMs < m_iLastNowMs
			|| !m_Grant.Same(echo) || !m_Grant.m_Destination.Same(recipient)) return false;
		if (m_bConfirmed) return true;
		if (nowMs >= m_iAckDeadlineMs) return false;
		m_iLastNowMs = nowMs;
		m_bConfirmed = true;
		return true;
	}

	bool Expired(int nowMs) { return m_Grant && !m_bConfirmed && nowMs >= m_iAckDeadlineMs; }

	int AckWaitRemaining(int nowMs)
	{
		if (!m_Grant || m_bConfirmed || nowMs < m_iLastNowMs) return -1;
		return m_iAckDeadlineMs - nowMs;
	}

	bool Ready(RICON_RelayDestination recipient, int nowMs)
	{
		return !m_bClosed && m_bConfirmed && nowMs >= m_iLastNowMs && m_Grant.m_Destination.Same(recipient);
	}

	bool CheckMedia(int sequence, int firstSampleIndex, RICON_RelayDestination recipient, int nowMs)
	{
		if (!Ready(recipient, nowMs) || !m_Grant.MatchesSample(sequence, firstSampleIndex)) return false;
		m_iLastNowMs = nowMs;
		return true;
	}

	//! Capture the exact outbound revoke before retiring server-side access.
	RICON_RelayWireGrant Revoke()
	{
		RICON_RelayWireGrant revoked;
		if (m_Grant) revoked = m_Grant.CloneGrant();
		Close();
		return revoked;
	}

	void Close()
	{
		m_bClosed = true;
		m_Grant = null;
	}
}
