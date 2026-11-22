class RICON_StockJammerRadio
{
	int m_iDeviceId, m_iPlayerId;
	BaseRadioComponent m_Radio;
}

//! A stock mute belongs to the exact native slot until RICON releases it.
class RICON_StockJammerSlot
{
	int m_iPlayerId;
	BaseRadioComponent m_Radio;
	BaseTransceiver m_Slot;
	ref RICON_PairEndpoint m_Endpoint;

	bool SameSlot(bool checkTune)
	{
		if (!m_Radio || !m_Slot || !m_Radio.GetOwner() || m_Radio.GetOwner().IsDeleted()) return false;
		RICON_RadioObservation rx = m_Endpoint.m_Observation;
		int index = rx.m_iTransceiverIndex;
		return index < m_Radio.TransceiversCount() && m_Radio.GetTransceiver(index) == m_Slot
			&& m_Slot.GetRadio() == m_Radio && (!checkTune || m_Slot.GetFrequency() == rx.m_iTuningValue);
	}

	//! shortcut: the stock API exposes slot mute. Keep the slot open if any potential in-range carrier can beat the jammer.
	bool DominatedBy(RICON_RxEnvironment environment, RICON_RadioQueryContext context, RICON_LinkEvaluator evaluator,
		array<ref RICON_StockJammerSlot> sources, inout int budget)
	{
		RICON_RadioObservation rx = m_Endpoint.m_Observation;
		RICON_LinkQuery query = new RICON_LinkQuery();
		query.m_sBandId = context.m_sBandId;
		query.m_sModelId = context.m_sModelId;
		query.m_fFrequencyMHz = m_Endpoint.m_Profile.m_fFrequencyMHz;
		query.m_vRxPos = rx.m_vAntennaPos;
		query.m_fRxGainDbi = m_Endpoint.m_Profile.m_fRxGainDbi;
		RICON_LinkBudget wanted = new RICON_LinkBudget();
		bool hasCarrier;
		foreach (RICON_StockJammerSlot source : sources)
		{
			RICON_RadioObservation tx = source.m_Endpoint.m_Observation;
			if (source.m_iPlayerId == m_iPlayerId || tx.m_iTuningValue != rx.m_iTuningValue || tx.m_sNetworkKey != rx.m_sNetworkKey) continue;
			if (vector.Distance(tx.m_vAntennaPos, rx.m_vAntennaPos) > source.m_Slot.GetRange()) continue;
			budget--;
			if (budget < 0) return false;
			query.m_vTxPos = tx.m_vAntennaPos;
			query.m_fTxPowerDbm = source.m_Endpoint.m_Profile.m_fTxPowerDbm;
			query.m_fTxGainDbi = source.m_Endpoint.m_Profile.m_fTxGainDbi;
			if (evaluator.FillReceivedPower(query, wanted) != RICON_EReason.OK || environment.m_fInterferenceDbm <= wanted.m_fRxPowerDbm)
				return false;
			hasCarrier = true;
		}
		return hasCarrier;
	}

	void Trace(bool blocked)
	{
		if (!RICON_RadioDev.Enabled()) return;
		RICON_RadioObservation rx = m_Endpoint.m_Observation;
		PrintFormat("[RICON_DEV] STOCK_JAM device=%1 slot=%2 tune=%3 type=%4 blocked=%5", rx.m_iDeviceId,
			rx.m_iTransceiverIndex, rx.m_iTuningValue, typename.EnumToString(RICON_EGameRadioEquipment, RICON_GameRadioEquipment.ReadType(m_Radio)), blocked);
	}
}
