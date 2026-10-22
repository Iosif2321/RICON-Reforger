//! P1: чистая оценка допустимости прекратить вычисление полезного вклада одной передачи на одном участке TX→RX
//! (REQ-channel-cutoff v0.1, корпус заморожен по LF SHA b4ff545d…54d8). Не демодулятор и не правило удаления помех;
//! не меняет LinkEvaluator, RICON_ELinkState, S3/S3b и snapshot v1. Production-вызовы не подключены.
//! Значение 0 каждого enum — безопасный отказ по умолчанию.
enum RICON_ECutoffDecision
{
	UNRESOLVED = 0,
	CONTINUE = 1,
	BOUNDED = 2
}

//! Порядок значений = нормативный порядок проверок CC05.
enum RICON_ECutoffReason
{
	INVALID_INPUT = 0,
	RESOURCE_EXHAUSTED = 1,
	INCOMPLETE_COVERAGE = 2,
	STALE_BOUND = 3,
	EMPTY_PATH_SET = 4,
	MISSING_PATH_BOUND = 5,
	NUMERIC_UNCERTIFIED = 6,
	NUMERIC_FAILURE = 7,
	CUTOFF_DISABLED = 8,
	BELOW_TOLERANCE = 9,
	ABOVE_TOLERANCE = 10
}

//! Строгая верхняя граница конечной мощности одной составляющей на входе RX на всём интервале (CC04).
//! Отсутствие границы — m_bHasUpper=false, а не нулевая мощность.
class RICON_CutoffPathBound
{
	string m_sPathId;
	bool m_bHasUpper;
	float m_fUpperDbm;
}

class RICON_CutoffInput
{
	int m_iNowMs = -1;
	int m_iValidFromMs = -1;
	int m_iValidUntilMs = -1;
	int m_iEnvironmentRevision;
	int m_iBoundRevision;
	bool m_bCoverageComplete;
	bool m_bResourceExhausted;
	bool m_bHasTolerance;
	float m_fToleranceDbm;		//!< верхний предел отбрасываемой полезной мощности, не порог RX
	bool m_bHasNumericalGuard;
	float m_fNumericalGuardDb;	//!< подтверждённый численным профилем запас; производственного значения нет
	ref array<ref RICON_CutoffPathBound> m_aPaths = {};
}

class RICON_CutoffResult
{
	RICON_ECutoffDecision m_eDecision = RICON_ECutoffDecision.UNRESOLVED;
	RICON_ECutoffReason m_eReason = RICON_ECutoffReason.INVALID_INPUT;
	bool m_bUsefulIsZero;
	bool m_bHasUpper;
	float m_fUpperDbm;
}

class RICON_ChannelCutoff
{
	static const int MAX_PATHS = 4096;

	protected static RICON_CutoffResult Make(RICON_ECutoffDecision decision, RICON_ECutoffReason reason)
	{
		RICON_CutoffResult r = new RICON_CutoffResult();
		r.m_eDecision = decision;
		r.m_eReason = reason;
		r.m_bUsefulIsZero = decision == RICON_ECutoffDecision.BOUNDED;
		return r;
	}

	protected static RICON_CutoffResult MakeBound(RICON_ECutoffDecision decision, RICON_ECutoffReason reason, float upperDbm)
	{
		RICON_CutoffResult r = Make(decision, reason);
		r.m_bHasUpper = true;
		r.m_fUpperDbm = upperDbm;
		return r;
	}

	//! CC04: структура, домены, уникальный непустой pathId, интервал [from, until) непуст.
	protected static bool ValidInput(RICON_CutoffInput input)
	{
		if (!input || !input.m_aPaths || input.m_aPaths.Count() > MAX_PATHS)
			return false;
		if (input.m_iNowMs < 0 || input.m_iValidFromMs < 0 || input.m_iValidUntilMs < 0
			|| input.m_iValidFromMs >= input.m_iValidUntilMs)
			return false;
		if (input.m_iEnvironmentRevision <= 0 || input.m_iBoundRevision <= 0)
			return false;
		if (input.m_bHasTolerance && !RICON_Num.IsFinite(input.m_fToleranceDbm))
			return false;
		if (input.m_bHasNumericalGuard && (!RICON_Num.IsFinite(input.m_fNumericalGuardDb) || input.m_fNumericalGuardDb < 0))
			return false;
		map<string, bool> ids = new map<string, bool>();
		foreach (RICON_CutoffPathBound p : input.m_aPaths)
		{
			if (!p || p.m_sPathId.IsEmpty() || ids.Contains(p.m_sPathId))
				return false;
			ids.Set(p.m_sPathId, true);
			if (p.m_bHasUpper && !RICON_Num.IsFinite(p.m_fUpperDbm))
				return false;
		}
		return true;
	}

	//! Вход не меняется; результат — новый объект без ссылок на вход.
	static RICON_CutoffResult Assess(RICON_CutoffInput input)
	{
		// Проверки глобальные: дефект последней записи запрещает принять отсечку по первой (CC05).
		if (!ValidInput(input))
			return Make(RICON_ECutoffDecision.UNRESOLVED, RICON_ECutoffReason.INVALID_INPUT);
		if (input.m_bResourceExhausted)
			return Make(RICON_ECutoffDecision.UNRESOLVED, RICON_ECutoffReason.RESOURCE_EXHAUSTED);
		if (!input.m_bCoverageComplete)
			return Make(RICON_ECutoffDecision.UNRESOLVED, RICON_ECutoffReason.INCOMPLETE_COVERAGE);
		if (input.m_iEnvironmentRevision != input.m_iBoundRevision || input.m_iNowMs < input.m_iValidFromMs
			|| input.m_iNowMs >= input.m_iValidUntilMs)
			return Make(RICON_ECutoffDecision.UNRESOLVED, RICON_ECutoffReason.STALE_BOUND);
		// Подтверждённо полный пустой набор — точный ноль полезного вклада без выдуманного дБм (CC08).
		if (input.m_aPaths.Count() == 0)
			return Make(RICON_ECutoffDecision.BOUNDED, RICON_ECutoffReason.EMPTY_PATH_SET);
		float peak;
		bool first = true;
		foreach (RICON_CutoffPathBound p : input.m_aPaths)
		{
			if (!p.m_bHasUpper)
				return Make(RICON_ECutoffDecision.CONTINUE, RICON_ECutoffReason.MISSING_PATH_BOUND);
			if (first || p.m_fUpperDbm > peak)
				peak = p.m_fUpperDbm;
			first = false;
		}
		if (!input.m_bHasNumericalGuard)
			return Make(RICON_ECutoffDecision.CONTINUE, RICON_ECutoffReason.NUMERIC_UNCERTIFIED);
		// CC06: неравенство треугольника для амплитуд; максимум вынесен, слабые пути до суммы не удаляются.
		float amplitudeSum = 0;
		foreach (RICON_CutoffPathBound q : input.m_aPaths)
			amplitudeSum += Math.Pow(10, (q.m_fUpperDbm - peak) / 20.0);
		float upper = peak + 20.0 * Math.Log10(amplitudeSum) + input.m_fNumericalGuardDb;
		if (!RICON_Num.IsFinite(upper))
			return Make(RICON_ECutoffDecision.UNRESOLVED, RICON_ECutoffReason.NUMERIC_FAILURE);
		if (!input.m_bHasTolerance)
			return MakeBound(RICON_ECutoffDecision.CONTINUE, RICON_ECutoffReason.CUTOFF_DISABLED, upper);
		// Равенство не отсекается.
		if (upper < input.m_fToleranceDbm)
			return MakeBound(RICON_ECutoffDecision.BOUNDED, RICON_ECutoffReason.BELOW_TOLERANCE, upper);
		return MakeBound(RICON_ECutoffDecision.CONTINUE, RICON_ECutoffReason.ABOVE_TOLERANCE, upper);
	}
}
