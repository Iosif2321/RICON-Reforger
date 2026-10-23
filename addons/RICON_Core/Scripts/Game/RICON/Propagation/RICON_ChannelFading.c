//! P1b.1: мгновенный коэффициент среды FINITE_MULTIPATH_V1 (REQ-channel-fading v0.2, корпус заморожен по LF SHA
//! cf5030f6…5f61). Чистое ядро: не читает часы, не знает о PTT/epoch, не выдаёт LOST и не является границей P1.
//! Builder коэффициентов, медленная тень и миграция потребителей (CT10) не подключены; production-вызовов нет.
//! Значение 0 enum — безопасный отказ по умолчанию; порядок значений = нормативный порядок проверок CF08.
enum RICON_EFadingCode
{
	INVALID_INPUT = 0,
	UNSUPPORTED_MODEL = 1,
	RESOURCE_EXHAUSTED = 2,
	INCOMPLETE_COVERAGE = 3,
	CONTEXT_MISMATCH = 4,
	STALE_DESCRIPTOR = 5,
	OUT_OF_MODEL_DOMAIN = 6,
	EMPTY_MEDIUM = 7,
	NUMERIC_FAILURE = 8,
	ZERO_AT_SAMPLE = 9,
	OK = 10
}

//! Физический конец (CF03): устройство и индекс приёмопередатчика, не leaseId/linkId.
class RICON_FadingEndpoint
{
	int m_iDeviceId;
	int m_iTransceiverIndex;
}

//! Составляющая среды (CF04): относительный вес мощности, фаза в anchorMs и доплер в 1/2^30 оборота за мкс.
class RICON_FadingComponent
{
	string m_sComponentId;
	float m_fPowerWeight;
	int m_iPhaseQ;
	int m_iDopplerQ;
}

class RICON_FadingDescriptor
{
	string m_sModelId;
	string m_sProfileId;
	int m_iProfileVersion;
	ref RICON_FadingEndpoint m_A;
	ref RICON_FadingEndpoint m_B;
	float m_fFrequencyMHz;
	float m_fBandwidthHz;
	int m_iEnvironmentRevision;
	int m_iAnchorMs = -1;
	int m_iValidFromMs = -1;
	int m_iValidUntilMs = -1;
	bool m_bStaticChannel;
	bool m_bCoverageComplete;
	bool m_bResourceExhausted;
	ref array<ref RICON_FadingComponent> m_aComponents = {};
}

//! Запрос (CF05): серверное время передаётся явно; ожидаемый профиль — с доверенной стороны.
class RICON_FadingQuery
{
	ref RICON_FadingEndpoint m_Tx;
	ref RICON_FadingEndpoint m_Rx;
	float m_fFrequencyMHz;
	float m_fBandwidthHz;
	string m_sProfileId;
	int m_iProfileVersion;
	int m_iEnvironmentRevision;
	int m_iNowMs = -1;
	int m_iSubUs = -1;
}

//! CF09: коэффициент есть при OK, ZERO_AT_SAMPLE (0) и EMPTY_MEDIUM (0); gainDb — только при OK.
class RICON_FadingResult
{
	RICON_EFadingCode m_eCode = RICON_EFadingCode.INVALID_INPUT;
	bool m_bHasCoefficient;
	float m_fReal;
	float m_fImag;
	bool m_bHasGain;
	float m_fGainDb;
	ref array<int> m_aPhasesQ = {};
}

class RICON_FiniteMultipath
{
	static const string MODEL_ID = "FINITE_MULTIPATH_V1";
	static const int MAX_COMPONENTS = 64;
	static const int PHASE_M = 1073741824;		//!< 2^30 долей оборота
	static const int PHASE_MASK = 1073741823;
	static const int QUARTER = 268435456;
	static const int EIGHTH = 134217728;
	static const int HALF_MASK = 32767;			//!< 2^15 − 1, CF06
	static const float PHASE_M_F = 1073741824.0;
	static const float TWO_PI_F32 = 6.2831854820251465;	//!< f32(2π)
	// CF07: f32 от точных дробей; десятичная запись без экспоненты округляется ровно в тот же binary32.
	static const float S3 = -0.1666666716337204;
	static const float S5 = 0.008333333767950535;
	static const float S7 = -0.00019841270113829523;
	static const float S9 = 0.00000275573188446;
	static const float S11 = -0.00000002505210794;
	static const float C2 = -0.5;
	static const float C4 = 0.0416666679084301;
	static const float C6 = -0.0013888889225199819;
	static const float C8 = 0.00002480158764229;
	static const float C10 = -0.00000027557319981;
	static const float C12 = 0.00000000208767559;

	//! (x·y) mod 2^30 для x, y ∈ [0, 2^30) без int32-переполнения: наибольшие промежуточные 2147352578 и 2147385345.
	static int MulMod(int x, int y)
	{
		int xl = x & HALF_MASK;
		int xh = x >> 15;
		int yl = y & HALF_MASK;
		int yh = y >> 15;
		int lo = xl * yl;
		int cross = (xl * yh + xh * yl) & HALF_MASK;
		return (lo + (cross << 15)) & PHASE_MASK;
	}

	//! CF06: tQ = (MulMod(Δms mod M, 1000) + subUs) mod M; знак доплера применяется после положительного MulMod.
	static int PhaseAt(int phaseQ, int dopplerQ, int tQ)
	{
		if (dopplerQ >= 0)
			return (phaseQ + MulMod(dopplerQ, tQ)) & PHASE_MASK;
		int back = phaseQ - MulMod(-dopplerQ, tQ);
		if (back < 0)
			back += PHASE_M;
		return back;
	}

	//! CF07: ближайшая четверть, Horner с округлением каждого действия к binary32, точный поворот.
	static void CosSin(int phaseQ, out float cosValue, out float sinValue)
	{
		int k = (phaseQ + EIGHTH) / QUARTER;
		int r = phaseQ - k * QUARTER;
		float c = 1;
		float s = 0;
		if (r != 0)
		{
			float rf = r;
			float frac = rf / PHASE_M_F;
			float x = frac * TWO_PI_F32;
			float z = x * x;
			float ts = z * S11;
			ts = S9 + ts;
			ts = z * ts;
			ts = S7 + ts;
			ts = z * ts;
			ts = S5 + ts;
			ts = z * ts;
			ts = S3 + ts;
			ts = z * ts;
			ts = 1 + ts;
			s = x * ts;
			float tc = z * C12;
			tc = C10 + tc;
			tc = z * tc;
			tc = C8 + tc;
			tc = z * tc;
			tc = C6 + tc;
			tc = z * tc;
			tc = C4 + tc;
			tc = z * tc;
			tc = C2 + tc;
			tc = z * tc;
			c = 1 + tc;
		}
		int turn = k % 4;
		if (turn == 0)
		{
			cosValue = c;
			sinValue = s;
		}
		else if (turn == 1)
		{
			cosValue = -s;
			sinValue = c;
		}
		else if (turn == 2)
		{
			cosValue = -c;
			sinValue = -s;
		}
		else
		{
			cosValue = s;
			sinValue = -c;
		}
	}

	protected static bool ValidKey(string key)
	{
		if (key.Length() < 1 || key.Length() > 64)
			return false;
		for (int i = 0; i < key.Length(); i++)
		{
			int c = key.ToAscii(i);
			if (!((c >= 65 && c <= 90) || (c >= 97 && c <= 122) || (c >= 48 && c <= 57)
				|| c == 95 || c == 46 || c == 58 || c == 45))
				return false;
		}
		return true;
	}

	//! Строгий ASCII-порядок двух допустимых ключей: −1, 0 или 1.
	static int AsciiCompare(string a, string b)
	{
		int n = a.Length();
		if (b.Length() < n)
			n = b.Length();
		for (int i = 0; i < n; i++)
		{
			int ca = a.ToAscii(i);
			int cb = b.ToAscii(i);
			if (ca != cb)
			{
				if (ca < cb)
					return -1;
				return 1;
			}
		}
		if (a.Length() == b.Length())
			return 0;
		if (a.Length() < b.Length())
			return -1;
		return 1;
	}

	protected static bool ValidEndpoint(RICON_FadingEndpoint e)
	{
		return e && e.m_iDeviceId >= 1 && e.m_iTransceiverIndex >= 0;
	}

	//! Лексикографический порядок пары (deviceId, transceiverIndex).
	protected static bool Less(RICON_FadingEndpoint a, RICON_FadingEndpoint b)
	{
		return a.m_iDeviceId < b.m_iDeviceId || (a.m_iDeviceId == b.m_iDeviceId && a.m_iTransceiverIndex < b.m_iTransceiverIndex);
	}

	protected static bool Same(RICON_FadingEndpoint a, RICON_FadingEndpoint b)
	{
		return a.m_iDeviceId == b.m_iDeviceId && a.m_iTransceiverIndex == b.m_iTransceiverIndex;
	}

	protected static bool PositiveFinite(float v)
	{
		return RICON_Num.IsFinite(v) && v > 0;
	}

	//! CF03–CF05 целиком, до любого успешного или иного кода: дефект последней записи запрещает ранний результат.
	protected static bool Valid(RICON_FadingDescriptor d, RICON_FadingQuery q)
	{
		if (!d || !q || !ValidKey(d.m_sModelId) || !ValidKey(d.m_sProfileId) || d.m_iProfileVersion < 1)
			return false;
		if (!ValidEndpoint(d.m_A) || !ValidEndpoint(d.m_B) || !Less(d.m_A, d.m_B))
			return false;
		if (!PositiveFinite(d.m_fFrequencyMHz) || !PositiveFinite(d.m_fBandwidthHz) || d.m_iEnvironmentRevision < 1)
			return false;
		if (d.m_iAnchorMs < 0 || d.m_iAnchorMs > d.m_iValidFromMs || d.m_iValidFromMs >= d.m_iValidUntilMs)
			return false;
		if (!d.m_aComponents || d.m_aComponents.Count() > MAX_COMPONENTS)
			return false;
		RICON_FadingComponent previous;
		foreach (RICON_FadingComponent c : d.m_aComponents)
		{
			if (!c || !ValidKey(c.m_sComponentId) || !RICON_Num.IsFinite(c.m_fPowerWeight))
				return false;
			if (c.m_fPowerWeight < 0 || c.m_fPowerWeight > 1 || c.m_iPhaseQ < 0 || c.m_iPhaseQ >= PHASE_M)
				return false;
			if (d.m_bStaticChannel && c.m_iDopplerQ != 0)
				return false;
			if (previous && AsciiCompare(previous.m_sComponentId, c.m_sComponentId) >= 0)
				return false;
			previous = c;
		}
		if (!ValidEndpoint(q.m_Tx) || !ValidEndpoint(q.m_Rx) || Same(q.m_Tx, q.m_Rx))
			return false;
		if (!PositiveFinite(q.m_fFrequencyMHz) || !PositiveFinite(q.m_fBandwidthHz) || !ValidKey(q.m_sProfileId))
			return false;
		if (q.m_iProfileVersion < 1 || q.m_iEnvironmentRevision < 1 || q.m_iNowMs < 0 || q.m_iSubUs < 0 || q.m_iSubUs > 999)
			return false;
		return true;
	}

	protected static bool SameContext(RICON_FadingDescriptor d, RICON_FadingQuery q)
	{
		RICON_FadingEndpoint low = q.m_Tx;
		RICON_FadingEndpoint high = q.m_Rx;
		if (Less(high, low))
		{
			low = q.m_Rx;
			high = q.m_Tx;
		}
		return Same(low, d.m_A) && Same(high, d.m_B) && q.m_fFrequencyMHz == d.m_fFrequencyMHz
			&& q.m_fBandwidthHz == d.m_fBandwidthHz && q.m_sProfileId == d.m_sProfileId
			&& q.m_iProfileVersion == d.m_iProfileVersion && q.m_iEnvironmentRevision == d.m_iEnvironmentRevision;
	}

	protected static RICON_FadingResult Make(RICON_EFadingCode code)
	{
		RICON_FadingResult r = new RICON_FadingResult();
		r.m_eCode = code;
		return r;
	}

	static RICON_FadingResult Evaluate(RICON_FadingDescriptor d, RICON_FadingQuery q)
	{
		if (!Valid(d, q))
			return Make(RICON_EFadingCode.INVALID_INPUT);
		if (d.m_sModelId != MODEL_ID)
			return Make(RICON_EFadingCode.UNSUPPORTED_MODEL);
		if (d.m_bResourceExhausted)
			return Make(RICON_EFadingCode.RESOURCE_EXHAUSTED);
		if (!d.m_bCoverageComplete)
			return Make(RICON_EFadingCode.INCOMPLETE_COVERAGE);
		if (!SameContext(d, q))
			return Make(RICON_EFadingCode.CONTEXT_MISMATCH);
		if (q.m_iNowMs < d.m_iValidFromMs || q.m_iNowMs >= d.m_iValidUntilMs)
			return Make(RICON_EFadingCode.STALE_DESCRIPTOR);
		// |dopplerQ| > M/4 без abs(INT_MIN).
		foreach (RICON_FadingComponent domain : d.m_aComponents)
		{
			if (domain.m_iDopplerQ > QUARTER || domain.m_iDopplerQ < -QUARTER)
				return Make(RICON_EFadingCode.OUT_OF_MODEL_DOMAIN);
		}
		float w = 0;
		foreach (RICON_FadingComponent weighed : d.m_aComponents)
		{
			w = w + weighed.m_fPowerWeight;
		}
		if (d.m_aComponents.Count() == 0 || w == 0)
		{
			RICON_FadingResult empty = Make(RICON_EFadingCode.EMPTY_MEDIUM);
			empty.m_bHasCoefficient = true;
			return empty;
		}
		float sw = Math.Sqrt(w);
		if (!PositiveFinite(sw))
			return Make(RICON_EFadingCode.NUMERIC_FAILURE);
		int deltaMs = q.m_iNowMs - d.m_iAnchorMs;
		int tQ = (MulMod(deltaMs % PHASE_M, 1000) + q.m_iSubUs) & PHASE_MASK;
		array<int> phases = {};
		float re = 0;
		float im = 0;
		foreach (RICON_FadingComponent c : d.m_aComponents)
		{
			int phase = PhaseAt(c.m_iPhaseQ, c.m_iDopplerQ, tQ);
			phases.Insert(phase);
			float root = Math.Sqrt(c.m_fPowerWeight);
			float amplitude = root / sw;
			float cosValue;
			float sinValue;
			CosSin(phase, cosValue, sinValue);
			float partReal = amplitude * cosValue;
			float partImag = amplitude * sinValue;
			re = re + partReal;
			im = im + partImag;
			if (!RICON_Num.IsFinite(amplitude) || !RICON_Num.IsFinite(re) || !RICON_Num.IsFinite(im))
				return Make(RICON_EFadingCode.NUMERIC_FAILURE);
		}
		return Classify(re, im, phases);
	}

	//! CF07/CF08 после суммы: ZERO решается до hypot только по real == imag == 0; неконечная часть или
	//! неположительный модуль ненулевого коэффициента (FTZ/underflow) — NUMERIC_FAILURE, не ZERO и не −Inf.
	//! Открыт для fault-injection стенда: валидный вход до этих ветвей не доходит (CF08 v0.2).
	static RICON_FadingResult Classify(float re, float im, notnull array<int> phases)
	{
		if (!RICON_Num.IsFinite(re) || !RICON_Num.IsFinite(im))
			return Make(RICON_EFadingCode.NUMERIC_FAILURE);
		RICON_FadingResult result = Make(RICON_EFadingCode.ZERO_AT_SAMPLE);
		result.m_bHasCoefficient = true;
		result.m_fReal = re;
		result.m_fImag = im;
		foreach (int phase : phases)
		{
			result.m_aPhasesQ.Insert(phase);
		}
		if (re == 0 && im == 0)
			return result;
		// Устойчивый модуль; разбиение hypot ненормативно в пределах 0.001 дБ (CF07 v0.2).
		float ar = Math.AbsFloat(re);
		float ai = Math.AbsFloat(im);
		float big = Math.Max(ar, ai);
		float small = Math.Min(ar, ai);
		float ratio = small / big;
		float modulus = big * Math.Sqrt(1 + ratio * ratio);
		if (!PositiveFinite(modulus))
			return Make(RICON_EFadingCode.NUMERIC_FAILURE);
		float gain = 20 * Math.Log10(modulus);
		if (!RICON_Num.IsFinite(gain))
			return Make(RICON_EFadingCode.NUMERIC_FAILURE);
		result.m_eCode = RICON_EFadingCode.OK;
		result.m_bHasGain = true;
		result.m_fGainDb = gain;
		return result;
	}
}
