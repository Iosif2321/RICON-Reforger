//! P2: наблюдение заданной прямой TX→RX (REQ-path-observation v0.3). Мир здесь не опрашивается.
enum RICON_EPathFamily
{
	TERRAIN = 0,
	WATER = 1,
	STRUCTURE = 2,
	VEGETATION = 3,
	VEHICLE = 4
}

enum RICON_EPathCoverage
{
	NOT_OBSERVED = 0,
	PARTIAL = 1,
	COMPLETE = 2
}

enum RICON_EPathMedium
{
	UNKNOWN = 0,
	AIR = 1,
	WATER = 2
}

enum RICON_EPathKnowledge
{
	UNKNOWN = 0,
	HINT_ONLY = 1,
	RULE = 2,
	PROFILE = 3
}

class RICON_PathEndpoint
{
	vector m_vPosition;
	RICON_EPathMedium m_eMedium;
	float m_fDepthM;
}

class RICON_PathSegment
{
	RICON_EPathFamily m_eFamily;
	float m_fStartM;
	float m_fEndM;
	string m_sEntityKey;
	RICON_EPathKnowledge m_eKnowledge;
	string m_sMaterialId;
	bool m_bOpenings;
}

class RICON_PathObservation
{
	float m_fFrequencyMHz;
	int m_iEnvironmentRevision;
	ref RICON_PathEndpoint m_Tx;
	ref RICON_PathEndpoint m_Rx;
	//! Индексы RICON_EPathFamily; ровно пять записей, включая TERRAIN.
	ref array<int> m_aCoverage = {};
	ref array<ref RICON_PathSegment> m_aSegments = {};
}

class RICON_WaterInterval
{
	float m_fStartM;
	float m_fEndM;
}

class RICON_PathObservationModel
{
	static const int MAX_SEGMENTS = 1024;

	static bool ValidKey(string key)
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

	protected static bool ValidEndpoint(RICON_PathEndpoint endpoint)
	{
		if (!endpoint || !RICON_Num.IsFiniteVector(endpoint.m_vPosition) || !RICON_Num.IsFinite(endpoint.m_fDepthM))
			return false;
		if (endpoint.m_eMedium < RICON_EPathMedium.UNKNOWN || endpoint.m_eMedium > RICON_EPathMedium.WATER || endpoint.m_fDepthM < 0)
			return false;
		return endpoint.m_eMedium == RICON_EPathMedium.WATER || endpoint.m_fDepthM == 0;
	}

	//! PO03: нормативный порядок binary32-операций, без квадрата большой мировой координаты.
	protected static bool Length(RICON_PathEndpoint tx, RICON_PathEndpoint rx, out float lengthM)
	{
		float dx = rx.m_vPosition[0] - tx.m_vPosition[0];
		float dy = rx.m_vPosition[1] - tx.m_vPosition[1];
		float dz = rx.m_vPosition[2] - tx.m_vPosition[2];
		if (!RICON_Num.IsFinite(dx) || !RICON_Num.IsFinite(dy) || !RICON_Num.IsFinite(dz))
			return false;
		float scale = Math.Max(Math.AbsFloat(dx), Math.Max(Math.AbsFloat(dy), Math.AbsFloat(dz)));
		if (scale == 0)
			return false;
		float sx = dx / scale;
		float sy = dy / scale;
		float sz = dz / scale;
		float xx = sx * sx;
		float yy = sy * sy;
		float zz = sz * sz;
		float xy = xx + yy;
		float q = xy + zz;
		float root = Math.Sqrt(q);
		lengthM = scale * root;
		return RICON_Num.IsFinite(lengthM) && lengthM > 0;
	}

	//! Проверка глобальная. Выходные интервалы новые, ссылок на входные сегменты нет.
	static bool Validate(RICON_PathObservation input, out float lengthM, out array<ref RICON_WaterInterval> water)
	{
		lengthM = 0;
		water = {};
		if (!input || !input.m_aCoverage || input.m_aCoverage.Count() != 5 || !input.m_aSegments
			|| input.m_aSegments.Count() > MAX_SEGMENTS || input.m_iEnvironmentRevision <= 0)
			return false;
		if (!RICON_Num.IsFinite(input.m_fFrequencyMHz) || input.m_fFrequencyMHz <= 0
			|| !ValidEndpoint(input.m_Tx) || !ValidEndpoint(input.m_Rx) || !Length(input.m_Tx, input.m_Rx, lengthM))
			return false;
		foreach (int coverage : input.m_aCoverage)
		{
			if (coverage < RICON_EPathCoverage.NOT_OBSERVED || coverage > RICON_EPathCoverage.COMPLETE)
				return false;
		}
		map<string, float> lastEnds = new map<string, float>();
		RICON_PathSegment previous;
		foreach (RICON_PathSegment segment : input.m_aSegments)
		{
			if (!segment || segment.m_eFamily < RICON_EPathFamily.WATER || segment.m_eFamily > RICON_EPathFamily.VEHICLE
				|| !ValidKey(segment.m_sEntityKey) || !RICON_Num.IsFinite(segment.m_fStartM) || !RICON_Num.IsFinite(segment.m_fEndM)
				|| segment.m_fStartM < 0 || segment.m_fStartM >= segment.m_fEndM || segment.m_fEndM > lengthM)
				return false;
			if (previous && (segment.m_fStartM < previous.m_fStartM || (segment.m_fStartM == previous.m_fStartM
				&& (segment.m_eFamily < previous.m_eFamily || (segment.m_eFamily == previous.m_eFamily
				&& segment.m_sEntityKey.Compare(previous.m_sEntityKey, true) < 0)))))
				return false;
			if (lastEnds.Contains(segment.m_sEntityKey) && segment.m_fStartM < lastEnds.Get(segment.m_sEntityKey))
				return false;
			lastEnds.Set(segment.m_sEntityKey, segment.m_fEndM);
			if (segment.m_eKnowledge < RICON_EPathKnowledge.UNKNOWN || segment.m_eKnowledge > RICON_EPathKnowledge.PROFILE)
				return false;
			if (segment.m_eKnowledge == RICON_EPathKnowledge.RULE || segment.m_eKnowledge == RICON_EPathKnowledge.PROFILE)
			{
				if (!ValidKey(segment.m_sMaterialId))
					return false;
			}
			else if (!segment.m_sMaterialId.IsEmpty())
				return false;
			previous = segment;
			if (segment.m_eFamily != RICON_EPathFamily.WATER)
				continue;
			int count = water.Count();
			if (count > 0 && segment.m_fStartM <= water[count - 1].m_fEndM)
			{
				water[count - 1].m_fEndM = Math.Max(water[count - 1].m_fEndM, segment.m_fEndM);
				continue;
			}
			RICON_WaterInterval interval = new RICON_WaterInterval();
			interval.m_fStartM = segment.m_fStartM;
			interval.m_fEndM = segment.m_fEndM;
			water.Insert(interval);
		}
		if (input.m_aCoverage[RICON_EPathFamily.WATER] == RICON_EPathCoverage.COMPLETE
			&& input.m_Tx.m_eMedium != RICON_EPathMedium.UNKNOWN && input.m_Rx.m_eMedium != RICON_EPathMedium.UNKNOWN)
		{
			bool txWater = water.Count() > 0 && water[0].m_fStartM == 0;
			bool rxWater = water.Count() > 0 && water[water.Count() - 1].m_fEndM == lengthM;
			if ((input.m_Tx.m_eMedium == RICON_EPathMedium.WATER) != txWater || (input.m_Rx.m_eMedium == RICON_EPathMedium.WATER) != rxWater)
				return false;
		}
		return true;
	}
}
