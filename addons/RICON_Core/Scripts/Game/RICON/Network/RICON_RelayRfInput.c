//! Первое плечо фиксированного ретранслятора для одного окна PCM.
//! Серверный владелец подтверждает полномочия, профиль и соответствие бюджета маршруту и кадру.
class RICON_RelayRfInput
{
	static RICON_LinkBudget Evaluate(RICON_PairEndpoint tx, int txGeneration, RICON_PairEndpoint rx, int rxGeneration,
		RICON_RadioQueryContext context, RICON_WirelessLinkEvaluator evaluator, RICON_StaticLosGeometryAuthority geometry,
		string timebaseId, int fromMs, int untilMs)
	{
		if (fromMs < 0 || untilMs <= fromMs || untilMs - fromMs != RICON_MediaPcmTestProfile.FRAME_DURATION_MS)
			return null;
		if (!tx || txGeneration <= 0 || !rx || rxGeneration <= 0 || !context || context.m_bHasInterference
			|| !evaluator || !evaluator.UsesFixedRfModels() || !geometry || !RICON_TemporalSnapshots.ValidKey(timebaseId))
			return null;
		if (!CoversPair(geometry, tx, txGeneration, rx, rxGeneration, timebaseId, fromMs, untilMs)) return null;

		return EvaluateQualified(tx, txGeneration, rx, rxGeneration, context, evaluator, geometry, timebaseId, fromMs, untilMs);
	}

	//! Internal common path: a public no-J or environment-qualified entry point must establish the noise contract.
	protected static RICON_LinkBudget EvaluateQualified(RICON_PairEndpoint tx, int txGeneration, RICON_PairEndpoint rx, int rxGeneration,
		RICON_RadioQueryContext context, RICON_WirelessLinkEvaluator evaluator, RICON_StaticLosGeometryAuthority geometry,
		string timebaseId, int fromMs, int untilMs)
	{
		RICON_LinkQuery query;
		if (RICON_RadioQueryBuilder.Build(tx.m_Observation, tx.m_Profile, rx.m_Observation, rx.m_Profile,
			context, query) != RICON_ERadioQueryCode.READY) return null;
		RICON_LinkBudget budget = evaluator.Evaluate(query);
		if (!ValidBudget(budget) || !CoversPair(geometry, tx, txGeneration, rx, rxGeneration, timebaseId, fromMs, untilMs))
			return null;
		return budget;
	}

	//! Pure linear input profile only; the caller still supplies current station authority and modeled noise samples.
	static RICON_RelayRfInputEnvironmentFrame EvaluateEnvironment(RICON_MediaEgressRoute route,
		RICON_PairEndpoint tx, int txGeneration, RICON_PairEndpoint rx, int rxGeneration, RICON_RadioQueryContext baseContext,
		RICON_WirelessLinkEvaluator evaluator, RICON_StaticLosGeometryAuthority geometry, string timebaseId,
		RICON_RelayPcmOrigin origin, int nowMs, RICON_RxEnvironment environment)
	{
		if (!route || !route.IsValid() || !tx || !rx || !tx.m_Observation || !rx.m_Observation || !environment) return null;
		if (!origin || !origin.IsValid(nowMs) || route.m_Source.m_sProfileId != RICON_MediaPcmTestProfile.PROFILE_ID) return null;
		int fromMs = origin.m_iCaptureFromMs, untilMs = origin.m_iCaptureUntilMs;
		if (!baseContext || baseContext.m_bHasInterference || !evaluator || !evaluator.UsesFixedRfModels() || !geometry) return null;
		if (!RICON_TemporalSnapshots.ValidKey(timebaseId) || environment.m_sTimebaseId != timebaseId
			|| environment.m_iExcludedEmissionId != 0 || txGeneration <= 0 || rxGeneration != route.m_iRxGeneration) return null;
		if (route.m_Source.m_iDeviceId != tx.m_Observation.m_iDeviceId || route.m_Source.m_iTxTransceiverIndex != tx.m_Observation.m_iTransceiverIndex
			|| route.m_iRxDeviceId != rx.m_Observation.m_iDeviceId || route.m_iRxTransceiverIndex != rx.m_Observation.m_iTransceiverIndex) return null;
		if (fromMs < 0 || untilMs <= fromMs || untilMs - fromMs != RICON_MediaPcmTestProfile.FRAME_DURATION_MS) return null;
		if (!environment.IsCurrentFor(rx.m_Observation, rx.m_Profile, baseContext, route.m_Source.m_iSessionEpoch, fromMs)
			|| !environment.IsCurrentFor(rx.m_Observation, rx.m_Profile, baseContext, route.m_Source.m_iSessionEpoch, untilMs - 1)
			|| !CoversPair(geometry, tx, txGeneration, rx, rxGeneration, timebaseId, fromMs, untilMs)) return null;
		RICON_RadioQueryContext context = RICON_RxEnvironmentService.CopyContext(environment.m_Context);
		RICON_LinkBudget budget = EvaluateQualified(tx, txGeneration, rx, rxGeneration, context, evaluator, geometry, timebaseId, fromMs, untilMs);
		if (!ValidBudget(budget) || budget.m_eReason != RICON_EReason.OK || !RICON_Num.IsFinite(budget.m_fNoisePlusInterferenceDbm)
			|| Math.AbsFloat(budget.m_fNoiseDbm - environment.m_fNoiseDbm) > 0.01
			|| Math.AbsFloat(budget.m_fNoisePlusInterferenceDbm - environment.m_fNoisePlusInterferenceDbm) > 0.01) return null;
		RICON_RelayRfInputEnvironmentFrame result = new RICON_RelayRfInputEnvironmentFrame();
		result.m_Route = route.CloneRoute();
		result.m_Budget = budget;
		result.m_Geometry = geometry;
		result.m_sTimebaseId = timebaseId;
		result.m_iFromMs = fromMs;
		result.m_iUntilMs = untilMs;
		result.m_iEnvironmentRevision = environment.m_iRevision;
		result.m_Origin = origin.CloneOrigin();
		int rateRatio = RICON_PcmResampler8To48.OUTPUT_RATE_HZ / RICON_PcmResampler8To48.INPUT_RATE_HZ;
		if (origin.m_iSampleCount != RICON_MediaPcmTestProfile.FRAME_SAMPLES
			|| origin.m_iFirstSampleIndex > (int.MAX - RICON_SharedRxTestMixer.WINDOW_SAMPLES) / rateRatio) return null;
		result.m_iFirstSampleIndex = origin.m_iFirstSampleIndex * rateRatio;
		if (!result.Matches(route, nowMs)) return null;
		return result;
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
		if (!ValidBudget(budget)) return null;
		return MixNoisePower(route, permit, firstSampleIndex, samples48, budget, referenceDbm, budget.m_fNoiseDbm, noiseSamples, false);
	}

	//! N+J is rendered once as an explicit unit-reference noise realization, not as another voice grant.
	static RICON_SharedRxMixResult MixEnvironment(RICON_RelayRfInputEnvironmentFrame frame, RICON_MediaEgressRoute route,
		RICON_MediaOutputPermit permit, int firstSampleIndex, array<int> samples48, float referenceDbm, array<int> noiseSamples, int nowMs)
	{
		if (!frame || !frame.Matches(route, nowMs) || firstSampleIndex != frame.m_iFirstSampleIndex || !ValidBudget(frame.m_Budget)) return null;
		RICON_SharedRxMixResult result = MixNoisePower(route, permit, firstSampleIndex, samples48, frame.m_Budget, referenceDbm,
			frame.m_Budget.m_fNoisePlusInterferenceDbm, noiseSamples, true);
		if (result) result.m_RelayInputOrigin = frame.m_Origin.CloneOrigin();
		return result;
	}

	protected static RICON_SharedRxMixResult MixNoisePower(RICON_MediaEgressRoute route, RICON_MediaOutputPermit permit, int firstSampleIndex,
		array<int> samples48, RICON_LinkBudget budget, float referenceDbm, float noiseDbm, array<int> noiseSamples, bool requiredNoise)
	{
		if (requiredNoise)
		{
			if (!RICON_SharedRxTestMixer.IsPcmWindow(noiseSamples)) return null;
			bool nonzero;
			foreach (int noiseValue : noiseSamples) { if (noiseValue != 0) nonzero = true; }
			if (!nonzero) return null;
		}
		float signalRatio, noiseRatio;
		if (!RICON_SharedRxTestMixer.TryNormalizePower(budget.m_fRxPowerDbm, noiseDbm,
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
		return RICON_SharedRxTestMixer.Mix(route, firstSampleIndex, inputs, noiseRatio, noiseSamples);
	}

	protected static bool ValidBudget(RICON_LinkBudget budget)
	{
		return budget && budget.m_bValid && RICON_Num.IsFinite(budget.m_fRxPowerDbm) && RICON_Num.IsFinite(budget.m_fNoiseDbm);
	}
}

//! A detached model window, not an authority token. Current PCM permits are still checked by the mixer/handoff.
class RICON_RelayRfInputEnvironmentFrame
{
	ref RICON_MediaEgressRoute m_Route;
	ref RICON_LinkBudget m_Budget;
	ref RICON_StaticLosGeometryAuthority m_Geometry;
	ref RICON_RelayPcmOrigin m_Origin;
	string m_sTimebaseId;
	int m_iFromMs = -1, m_iUntilMs = -1, m_iEnvironmentRevision;
	int m_iFirstSampleIndex;

	bool Matches(RICON_MediaEgressRoute route, int nowMs)
	{
		if (!route || !route.IsValid() || !m_Route || !m_Route.Same(route) || !m_Budget || !m_Budget.m_bValid) return false;
		if (!m_Origin || !m_Origin.IsValid(nowMs) || m_Origin.m_iCaptureFromMs != m_iFromMs || m_Origin.m_iCaptureUntilMs != m_iUntilMs) return false;
		int rateRatio = RICON_PcmResampler8To48.OUTPUT_RATE_HZ / RICON_PcmResampler8To48.INPUT_RATE_HZ;
		if (m_Origin.m_iSampleCount != RICON_MediaPcmTestProfile.FRAME_SAMPLES || m_iFirstSampleIndex < 0
			|| m_iFirstSampleIndex % rateRatio != 0 || m_iFirstSampleIndex / rateRatio != m_Origin.m_iFirstSampleIndex) return false;
		if (m_Origin.m_CaptureEnvelope)
		{
			if (m_Origin.m_CaptureEnvelope.m_sTimebaseId != m_sTimebaseId
				|| !route.m_Source.Same(m_Origin.m_CaptureEnvelope.m_Binding)) return false;
		}
		if (m_iEnvironmentRevision <= 0 || !RICON_TemporalSnapshots.ValidKey(m_sTimebaseId)
			|| m_iFromMs < 0 || m_iUntilMs <= m_iFromMs) return false;
		if (m_iUntilMs - m_iFromMs != RICON_MediaPcmTestProfile.FRAME_DURATION_MS
			|| !m_Geometry || !m_Geometry.CoversFixedInterval(m_iFromMs, m_iUntilMs)) return false;
		return RICON_Num.IsFinite(m_Budget.m_fNoisePlusInterferenceDbm) && m_Budget.m_fNoisePlusInterferenceDbm >= m_Budget.m_fNoiseDbm;
	}
}
