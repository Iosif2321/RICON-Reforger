//! TEST_ONLY: frozen P2, неизменность входов, отсутствие alias, отрицательные контроли и typed-границы.
class RICON_PathObservationRun
{
	int m_iCases;
	int m_iMatches;
	int m_iImmutable;
	int m_iControls;
	int m_iExtras;
	bool m_bOk = true;

	protected void Require(bool condition, string id, string what)
	{
		m_bOk = RICON_TestSupport.Expect(condition, id, what) && m_bOk;
	}

	protected static bool Close(float actual, float expected)
	{
		return RICON_Num.IsFinite(actual) && RICON_Num.IsFinite(expected)
			&& Math.AbsFloat(actual - expected) <= Math.Max(0.001, 0.00001 * Math.AbsFloat(expected));
	}

	static bool SameResult(RICON_WaterLayerResult actual, RICON_WaterLayerResult expected)
	{
		if (!actual || !expected || actual.m_eCode != expected.m_eCode || actual.m_bHasLoss != expected.m_bHasLoss)
			return false;
		if (actual.m_bHasLoss && !Close(actual.m_fLossDb, expected.m_fLossDb))
			return false;
		if (actual.m_eCode != RICON_EWaterCode.OK)
			return true;
		if (!Close(actual.m_fAlphaDbPerM, expected.m_fAlphaDbPerM) || !Close(actual.m_fInterfaceDb, expected.m_fInterfaceDb)
			|| !Close(actual.m_fWaterLengthM, expected.m_fWaterLengthM) || actual.m_iCrossings != expected.m_iCrossings
			|| !actual.m_aIntervals || !expected.m_aIntervals || actual.m_aIntervals.Count() != expected.m_aIntervals.Count())
			return false;
		for (int i = 0; i < actual.m_aIntervals.Count(); i++)
		{
			RICON_WaterInterval a = actual.m_aIntervals[i];
			RICON_WaterInterval e = expected.m_aIntervals[i];
			if (!a || !e || a.m_fStartM != e.m_fStartM || a.m_fEndM != e.m_fEndM)
				return false;
		}
		return true;
	}

	protected static string EndpointText(RICON_PathEndpoint endpoint, notnull array<float> floats)
	{
		if (!endpoint)
			return "null";
		floats.Insert(endpoint.m_vPosition[0]);
		floats.Insert(endpoint.m_vPosition[1]);
		floats.Insert(endpoint.m_vPosition[2]);
		floats.Insert(endpoint.m_fDepthM);
		return endpoint.m_eMedium.ToString();
	}

	protected static void Capture(RICON_PathObservation input, RICON_WaterProfile profile, out string text, notnull array<float> floats)
	{
		text = "null";
		if (input)
		{
			text = input.m_iEnvironmentRevision.ToString();
			floats.Insert(input.m_fFrequencyMHz);
			text += "|" + EndpointText(input.m_Tx, floats) + "|" + EndpointText(input.m_Rx, floats);
			if (!input.m_aCoverage)
				text += "|nullCoverage";
			else
			{
				text += "|coverage:" + input.m_aCoverage.Count().ToString();
				foreach (int coverage : input.m_aCoverage)
					text += ":" + coverage.ToString();
			}
			if (!input.m_aSegments)
				text += "|nullSegments";
			else
			{
				text += "|segments:" + input.m_aSegments.Count().ToString();
				foreach (RICON_PathSegment segment : input.m_aSegments)
				{
					if (!segment)
					{
						text += "|nullSegment";
						continue;
					}
					text += "|" + segment.m_eFamily.ToString() + "|" + segment.m_sEntityKey + "|" + segment.m_eKnowledge.ToString();
					text += "|" + segment.m_sMaterialId + "|" + segment.m_bOpenings.ToString();
					floats.Insert(segment.m_fStartM);
					floats.Insert(segment.m_fEndM);
				}
			}
		}
		if (profile)
		{
			text += "|profile";
			floats.Insert(profile.m_fTempC);
			floats.Insert(profile.m_fSalinityPpt);
		}
		else
			text += "|nullProfile";
	}

	protected static string Text(float value)
	{
		if (RICON_Num.IsFinite(value) && Math.AbsFloat(value) < 1000000)
			return value.ToString(-1, 9);
		return "out_of_print_range";
	}

	protected void Reject(bool accepted)
	{
		Require(!accepted, "P2 controls", "искажённый результат отвергнут");
		if (!accepted)
			m_iControls++;
	}

	protected void Controls(RICON_WaterLayerResult actual, RICON_WaterLayerResult expected)
	{
		Reject(SameResult(null, expected));
		actual.m_eCode = RICON_EWaterCode.NO_WATER;
		Reject(SameResult(actual, expected));
		actual.m_eCode = RICON_EWaterCode.OK;
		actual.m_bHasLoss = false;
		Reject(SameResult(actual, expected));
		actual.m_bHasLoss = true;
		float loss = actual.m_fLossDb;
		actual.m_fLossDb = loss + 10;
		Reject(SameResult(actual, expected));
		actual.m_fLossDb = RICON_L2Run.NaN();
		Reject(SameResult(actual, expected));
		actual.m_fLossDb = loss;
		float alpha = actual.m_fAlphaDbPerM;
		actual.m_fAlphaDbPerM = alpha + 10;
		Reject(SameResult(actual, expected));
		actual.m_fAlphaDbPerM = alpha;
		float boundary = actual.m_fInterfaceDb;
		actual.m_fInterfaceDb = boundary + 10;
		Reject(SameResult(actual, expected));
		actual.m_fInterfaceDb = boundary;
		float length = actual.m_fWaterLengthM;
		actual.m_fWaterLengthM = length + 10;
		Reject(SameResult(actual, expected));
		actual.m_fWaterLengthM = length;
		actual.m_iCrossings++;
		Reject(SameResult(actual, expected));
		actual.m_iCrossings--;
		float start = actual.m_aIntervals[0].m_fStartM;
		actual.m_aIntervals[0].m_fStartM = start + 1;
		Reject(SameResult(actual, expected));
		actual.m_aIntervals[0].m_fStartM = start;
		RICON_WaterInterval interval = actual.m_aIntervals[0];
		actual.m_aIntervals[0] = null;
		Reject(SameResult(actual, expected));
		actual.m_aIntervals[0] = interval;
	}

	void CheckCase(string id, RICON_PathObservation input, RICON_WaterProfile profile, RICON_WaterLayerResult expected)
	{
		m_iCases++;
		string before;
		array<float> beforeFloats = {};
		Capture(input, profile, before, beforeFloats);
		RICON_WaterLayerResult actual = RICON_WaterLayer.Evaluate(input, profile);
		bool matched = SameResult(actual, expected);
		Require(matched, id, "все поля водного слоя совпадают с эталоном");
		if (matched)
			m_iMatches++;
		if (actual)
		{
			actual.m_fLossDb = -9999;
			if (actual.m_aIntervals && !actual.m_aIntervals.IsEmpty())
				actual.m_aIntervals[0].m_fStartM = -9999;
		}
		RICON_WaterLayerResult again = RICON_WaterLayer.Evaluate(input, profile);
		string after;
		array<float> afterFloats = {};
		Capture(input, profile, after, afterFloats);
		bool immutable = before == after && RICON_ChannelCutoffRun.SameFloats(beforeFloats, afterFloats)
			&& again != actual && SameResult(again, expected);
		Require(immutable, id, "входы неизменны и повторный результат независим");
		if (immutable)
			m_iImmutable++;
		if (id == "W06" && matched && immutable)
		{
			Controls(again, expected);
			Require(SameResult(again, expected), "P2 controls", "результат восстановлен после контролей");
		}
		if (again)
			PrintFormat("[RICON_PATH] case=%1 code=%2 hasLoss=%3 loss=%4 alpha=%5 interface=%6 waterLength=%7 crossings=%8 intervals=%9 immutable=" + immutable.ToString(), id,
				again.m_eCode, again.m_bHasLoss, Text(again.m_fLossDb), Text(again.m_fAlphaDbPerM), Text(again.m_fInterfaceDb),
				Text(again.m_fWaterLengthM), again.m_iCrossings, again.m_aIntervals.Count());
	}

	protected void Extra(bool passed, string name)
	{
		Require(passed, name, "typed-граница P2");
		if (passed)
			m_iExtras++;
		PrintFormat("[RICON_PATH] extra=%1 pass=%2", name, passed);
	}

	void CheckBounds()
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 100;
		input.m_iEnvironmentRevision = int.MAX;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_eMedium = RICON_EPathMedium.AIR;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_eMedium = RICON_EPathMedium.AIR;
		input.m_Rx.m_vPosition = "2048 0 0";
		input.m_aCoverage = {2, 2, 2, 2, 2};
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 20;
		profile.m_fSalinityPpt = 0;
		Extra(RICON_WaterLayer.Evaluate(null, profile).m_eCode == RICON_EWaterCode.INVALID_OBSERVATION, "null_input");
		Extra(RICON_WaterLayer.Evaluate(input, null).m_eCode == RICON_EWaterCode.INVALID_PROFILE, "null_profile");
		for (int i = 0; i < 1024; i++)
		{
			RICON_PathSegment segment = new RICON_PathSegment();
			segment.m_eFamily = RICON_EPathFamily.STRUCTURE;
			segment.m_fStartM = i;
			segment.m_fEndM = i + 1;
			segment.m_sEntityKey = "structure";
			input.m_aSegments.Insert(segment);
		}
		Extra(RICON_WaterLayer.Evaluate(input, profile).m_eCode == RICON_EWaterCode.NO_WATER, "segments_1024_revision_MAX");
		RICON_PathSegment overflow = new RICON_PathSegment();
		overflow.m_eFamily = RICON_EPathFamily.STRUCTURE;
		overflow.m_sEntityKey = "structure";
		overflow.m_fStartM = 1024;
		overflow.m_fEndM = 1025;
		input.m_aSegments.Insert(overflow);
		Extra(RICON_WaterLayer.Evaluate(input, profile).m_eCode == RICON_EWaterCode.INVALID_OBSERVATION, "segments_1025");
		input.m_aSegments.Remove(input.m_aSegments.Count() - 1);
		input.m_aSegments[1023].m_fEndM = -1;
		profile.m_fTempC = RICON_L2Run.NaN();
		Extra(RICON_WaterLayer.Evaluate(input, profile).m_eCode == RICON_EWaterCode.INVALID_OBSERVATION, "global_invalid_before_profile");
		profile.m_fTempC = 20;
		input.m_aSegments[1023] = null;
		Extra(RICON_WaterLayer.Evaluate(input, profile).m_eCode == RICON_EWaterCode.INVALID_OBSERVATION, "null_segment");
		input.m_aSegments = null;
		Extra(RICON_WaterLayer.Evaluate(input, profile).m_eCode == RICON_EWaterCode.INVALID_OBSERVATION, "null_segments");
		input.m_aSegments = {};
		array<string> keys = {"-", ".", "0", ":", "A", "_", "a"};
		foreach (string key : keys)
		{
			RICON_PathSegment keyed = new RICON_PathSegment();
			keyed.m_eFamily = RICON_EPathFamily.STRUCTURE;
			keyed.m_sEntityKey = key;
			keyed.m_fStartM = 1;
			keyed.m_fEndM = 2;
			input.m_aSegments.Insert(keyed);
		}
		bool sortedAscii = RICON_WaterLayer.Evaluate(input, profile).m_eCode == RICON_EWaterCode.NO_WATER;
		RICON_PathSegment first = input.m_aSegments[0];
		input.m_aSegments[0] = input.m_aSegments[1];
		input.m_aSegments[1] = first;
		Extra(sortedAscii && RICON_WaterLayer.Evaluate(input, profile).m_eCode == RICON_EWaterCode.INVALID_OBSERVATION, "ascii_sort");
	}
}
