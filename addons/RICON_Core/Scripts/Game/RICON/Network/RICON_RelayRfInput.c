//! Первое плечо фиксированного ретранслятора для одного окна PCM.
//! Серверный владелец подтверждает полномочия, профиль и соответствие бюджета маршруту и кадру.
class RICON_RelayRfInput
{
	static RICON_LinkBudget Evaluate(RICON_PairEndpoint tx, int txGeneration, RICON_PairEndpoint rx, int rxGeneration,
		RICON_RadioQueryContext context, RICON_WirelessLinkEvaluator evaluator, RICON_StaticLosGeometryAuthority geometry,
		string timebaseId, int fromMs, int untilMs)
	{
		if (fromMs < 0 || untilMs <= fromMs || untilMs - fromMs != RICON_MediaPcmProfile.FRAME_DURATION_MS)
			return null;
		if (!tx || txGeneration <= 0 || !rx || rxGeneration <= 0 || !context || context.m_bHasInterference
			|| !evaluator || !evaluator.UsesFixedRfModels() || !geometry || !RICON_TemporalSnapshots.ValidKey(timebaseId))
			return null;
		if (!CoversPair(geometry, tx, txGeneration, rx, rxGeneration, timebaseId, fromMs, untilMs)) return null;

		RICON_LinkQuery query;
		if (RICON_RadioQueryBuilder.Build(tx.m_Observation, tx.m_Profile, rx.m_Observation, rx.m_Profile,
			context, query) != RICON_ERadioQueryCode.READY) return null;
		RICON_LinkBudget budget = evaluator.Evaluate(query);
		if (!ValidBudget(budget) || !CoversPair(geometry, tx, txGeneration, rx, rxGeneration, timebaseId, fromMs, untilMs))
			return null;
		return budget;
	}

	protected static bool CoversPair(RICON_StaticLosGeometryAuthority geometry, RICON_PairEndpoint tx, int txGeneration,
		RICON_PairEndpoint rx, int rxGeneration, string timebaseId, int fromMs, int untilMs)
	{
		return geometry.CoversFixedInterval(fromMs, untilMs)
			&& geometry.QualifiesFixedPair(tx, txGeneration, rx, rxGeneration, timebaseId, fromMs);
	}

	//! На входе уже подготовленное окно 192 отсчётов PCM16 на шкале 48 кГц.
	//! shortcut: инструментальный профиль подавляет моделируемый шум при отсутствии шумовых отсчётов.
	static RICON_SharedRxMixResult Mix(RICON_MediaEgressRoute route, RICON_MediaOutputPermit permit, int firstSampleIndex,
		array<int> samples48, RICON_LinkBudget budget, float referenceDbm, array<int> noiseSamples)
	{
		float signalRatio, noiseRatio;
		if (!ValidBudget(budget) || !RICON_SharedRxMixer.TryNormalizePower(budget.m_fRxPowerDbm, budget.m_fNoiseDbm,
			referenceDbm, signalRatio, noiseRatio)) return null;
		RICON_SharedRxContribution input = new RICON_SharedRxContribution();
		input.m_eKind = RICON_ESharedRxContributionKind.PCM;
		input.m_Route = route;
		input.m_Permit = permit;
		input.m_iFirstSampleIndex = firstSampleIndex;
		input.m_fPowerRatio = signalRatio;
		input.m_aSamples = samples48;
		array<ref RICON_SharedRxContribution> inputs = {input};
		if (!noiseSamples) noiseRatio = 0.0;
		return RICON_SharedRxMixer.Mix(route, firstSampleIndex, inputs, noiseRatio, noiseSamples);
	}

	protected static bool ValidBudget(RICON_LinkBudget budget)
	{
		return budget && budget.m_bValid && RICON_Num.IsFinite(budget.m_fRxPowerDbm) && RICON_Num.IsFinite(budget.m_fNoiseDbm);
	}
}
