//! TOOL_ONLY: frozen P2, ожидания скопированы из независимо проверенного JSON.
class RICON_PathObservationFixtures
{
	static const string SOURCE_SHA = "7a12ed92587318108059a213ace1d3f4286ad9a66bf86e8eb63dd4cdc7eea032";
	static const int CASES = 47;
	static const int HOST_ONLY = 4;

	// O16: host-only — лишнее поле JSON.
	// O19: host-only — bool вместо числа.
	// W17: host-only — лишнее поле JSON.
	// W21: host-only — пропущенное поле профиля JSON.
	static void Run(RICON_PathObservationRun t)
	{
		Case0(t);
		Case1(t);
		Case2(t);
		Case3(t);
		Case4(t);
		Case5(t);
		Case6(t);
		Case7(t);
		Case8(t);
		Case9(t);
		Case10(t);
		Case11(t);
		Case12(t);
		Case13(t);
		Case14(t);
		Case15(t);
		Case16(t);
		Case17(t);
		Case18(t);
		Case19(t);
		Case20(t);
		Case21(t);
		Case22(t);
		Case23(t);
		Case24(t);
		Case25(t);
		Case26(t);
		Case27(t);
		Case28(t);
		Case29(t);
		Case30(t);
		Case31(t);
		Case32(t);
		Case33(t);
		Case34(t);
		Case35(t);
		Case36(t);
		Case37(t);
		Case38(t);
		Case39(t);
		Case40(t);
		Case41(t);
		Case42(t);
		Case43(t);
		Case44(t);
		Case45(t);
		Case46(t);
	}

	protected static void Case0(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.NO_WATER;
		expected.m_bHasLoss = true;
		expected.m_fLossDb = 0.0;
		t.CheckCase("O01", input, profile, expected);
	}

	protected static void Case1(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_PathSegment segment0 = new RICON_PathSegment();
		segment0.m_eFamily = 3;
		segment0.m_fStartM = 100.0;
		segment0.m_fEndM = 130.0;
		segment0.m_sEntityKey = "tree.17";
		segment0.m_eKnowledge = 2;
		segment0.m_sMaterialId = "rule.tree_broadleaf";
		segment0.m_bOpenings = false;
		input.m_aSegments.Insert(segment0);
		RICON_PathSegment segment1 = new RICON_PathSegment();
		segment1.m_eFamily = 2;
		segment1.m_fStartM = 120.0;
		segment1.m_fEndM = 128.0;
		segment1.m_sEntityKey = "house.3";
		segment1.m_eKnowledge = 3;
		segment1.m_sMaterialId = "brick_wall_02";
		segment1.m_bOpenings = true;
		input.m_aSegments.Insert(segment1);
		RICON_PathSegment segment2 = new RICON_PathSegment();
		segment2.m_eFamily = 2;
		segment2.m_fStartM = 140.0;
		segment2.m_fEndM = 148.0;
		segment2.m_sEntityKey = "house.3";
		segment2.m_eKnowledge = 3;
		segment2.m_sMaterialId = "brick_wall_02";
		segment2.m_bOpenings = false;
		input.m_aSegments.Insert(segment2);
		RICON_PathSegment segment3 = new RICON_PathSegment();
		segment3.m_eFamily = 4;
		segment3.m_fStartM = 500.0;
		segment3.m_fEndM = 504.0;
		segment3.m_sEntityKey = "veh.ural.9";
		segment3.m_eKnowledge = 1;
		segment3.m_sMaterialId = "";
		segment3.m_bOpenings = false;
		input.m_aSegments.Insert(segment3);
		RICON_PathSegment segment4 = new RICON_PathSegment();
		segment4.m_eFamily = 2;
		segment4.m_fStartM = 700.0;
		segment4.m_fEndM = 712.0;
		segment4.m_sEntityKey = "mod.bunker.1";
		segment4.m_eKnowledge = 0;
		segment4.m_sMaterialId = "";
		segment4.m_bOpenings = false;
		input.m_aSegments.Insert(segment4);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.NO_WATER;
		expected.m_bHasLoss = true;
		expected.m_fLossDb = 0.0;
		t.CheckCase("O02", input, profile, expected);
	}

	protected static void Case2(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_PathSegment segment0 = new RICON_PathSegment();
		segment0.m_eFamily = 3;
		segment0.m_fStartM = 100.0;
		segment0.m_fEndM = 130.0;
		segment0.m_sEntityKey = "tree.17";
		segment0.m_eKnowledge = 2;
		segment0.m_sMaterialId = "rule.tree_broadleaf";
		segment0.m_bOpenings = false;
		input.m_aSegments.Insert(segment0);
		RICON_PathSegment segment1 = new RICON_PathSegment();
		segment1.m_eFamily = 2;
		segment1.m_fStartM = 120.0;
		segment1.m_fEndM = 128.0;
		segment1.m_sEntityKey = "house.3";
		segment1.m_eKnowledge = 3;
		segment1.m_sMaterialId = "brick_wall_02";
		segment1.m_bOpenings = true;
		input.m_aSegments.Insert(segment1);
		RICON_PathSegment segment2 = new RICON_PathSegment();
		segment2.m_eFamily = 2;
		segment2.m_fStartM = 126.0;
		segment2.m_fEndM = 148.0;
		segment2.m_sEntityKey = "house.3";
		segment2.m_eKnowledge = 3;
		segment2.m_sMaterialId = "brick_wall_02";
		segment2.m_bOpenings = false;
		input.m_aSegments.Insert(segment2);
		RICON_PathSegment segment3 = new RICON_PathSegment();
		segment3.m_eFamily = 4;
		segment3.m_fStartM = 500.0;
		segment3.m_fEndM = 504.0;
		segment3.m_sEntityKey = "veh.ural.9";
		segment3.m_eKnowledge = 1;
		segment3.m_sMaterialId = "";
		segment3.m_bOpenings = false;
		input.m_aSegments.Insert(segment3);
		RICON_PathSegment segment4 = new RICON_PathSegment();
		segment4.m_eFamily = 2;
		segment4.m_fStartM = 700.0;
		segment4.m_fEndM = 712.0;
		segment4.m_sEntityKey = "mod.bunker.1";
		segment4.m_eKnowledge = 0;
		segment4.m_sMaterialId = "";
		segment4.m_bOpenings = false;
		input.m_aSegments.Insert(segment4);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.INVALID_OBSERVATION;
		expected.m_bHasLoss = false;
		t.CheckCase("O03", input, profile, expected);
	}

	protected static void Case3(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_PathSegment segment0 = new RICON_PathSegment();
		segment0.m_eFamily = 2;
		segment0.m_fStartM = 700.0;
		segment0.m_fEndM = 712.0;
		segment0.m_sEntityKey = "mod.bunker.1";
		segment0.m_eKnowledge = 0;
		segment0.m_sMaterialId = "";
		segment0.m_bOpenings = false;
		input.m_aSegments.Insert(segment0);
		RICON_PathSegment segment1 = new RICON_PathSegment();
		segment1.m_eFamily = 4;
		segment1.m_fStartM = 500.0;
		segment1.m_fEndM = 504.0;
		segment1.m_sEntityKey = "veh.ural.9";
		segment1.m_eKnowledge = 1;
		segment1.m_sMaterialId = "";
		segment1.m_bOpenings = false;
		input.m_aSegments.Insert(segment1);
		RICON_PathSegment segment2 = new RICON_PathSegment();
		segment2.m_eFamily = 2;
		segment2.m_fStartM = 140.0;
		segment2.m_fEndM = 148.0;
		segment2.m_sEntityKey = "house.3";
		segment2.m_eKnowledge = 3;
		segment2.m_sMaterialId = "brick_wall_02";
		segment2.m_bOpenings = false;
		input.m_aSegments.Insert(segment2);
		RICON_PathSegment segment3 = new RICON_PathSegment();
		segment3.m_eFamily = 2;
		segment3.m_fStartM = 120.0;
		segment3.m_fEndM = 128.0;
		segment3.m_sEntityKey = "house.3";
		segment3.m_eKnowledge = 3;
		segment3.m_sMaterialId = "brick_wall_02";
		segment3.m_bOpenings = true;
		input.m_aSegments.Insert(segment3);
		RICON_PathSegment segment4 = new RICON_PathSegment();
		segment4.m_eFamily = 3;
		segment4.m_fStartM = 100.0;
		segment4.m_fEndM = 130.0;
		segment4.m_sEntityKey = "tree.17";
		segment4.m_eKnowledge = 2;
		segment4.m_sMaterialId = "rule.tree_broadleaf";
		segment4.m_bOpenings = false;
		input.m_aSegments.Insert(segment4);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.INVALID_OBSERVATION;
		expected.m_bHasLoss = false;
		t.CheckCase("O04", input, profile, expected);
	}

	protected static void Case4(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_PathSegment segment0 = new RICON_PathSegment();
		segment0.m_eFamily = 2;
		segment0.m_fStartM = 990.0;
		segment0.m_fEndM = 1000.5;
		segment0.m_sEntityKey = "x";
		segment0.m_eKnowledge = 0;
		segment0.m_sMaterialId = "";
		segment0.m_bOpenings = false;
		input.m_aSegments.Insert(segment0);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.INVALID_OBSERVATION;
		expected.m_bHasLoss = false;
		t.CheckCase("O05", input, profile, expected);
	}

	protected static void Case5(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_PathSegment segment0 = new RICON_PathSegment();
		segment0.m_eFamily = 2;
		segment0.m_fStartM = 10.0;
		segment0.m_fEndM = 10.0;
		segment0.m_sEntityKey = "x";
		segment0.m_eKnowledge = 0;
		segment0.m_sMaterialId = "";
		segment0.m_bOpenings = false;
		input.m_aSegments.Insert(segment0);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.INVALID_OBSERVATION;
		expected.m_bHasLoss = false;
		t.CheckCase("O06", input, profile, expected);
	}

	protected static void Case6(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_PathSegment segment0 = new RICON_PathSegment();
		segment0.m_eFamily = 2;
		segment0.m_fStartM = 1.0;
		segment0.m_fEndM = 2.0;
		segment0.m_sEntityKey = "x";
		segment0.m_eKnowledge = 0;
		segment0.m_sMaterialId = "brick";
		segment0.m_bOpenings = false;
		input.m_aSegments.Insert(segment0);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.INVALID_OBSERVATION;
		expected.m_bHasLoss = false;
		t.CheckCase("O07", input, profile, expected);
	}

	protected static void Case7(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_PathSegment segment0 = new RICON_PathSegment();
		segment0.m_eFamily = 2;
		segment0.m_fStartM = 1.0;
		segment0.m_fEndM = 2.0;
		segment0.m_sEntityKey = "x";
		segment0.m_eKnowledge = 3;
		segment0.m_sMaterialId = "";
		segment0.m_bOpenings = false;
		input.m_aSegments.Insert(segment0);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.INVALID_OBSERVATION;
		expected.m_bHasLoss = false;
		t.CheckCase("O08", input, profile, expected);
	}

	protected static void Case8(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_PathSegment segment0 = new RICON_PathSegment();
		segment0.m_eFamily = 0;
		segment0.m_fStartM = 1.0;
		segment0.m_fEndM = 2.0;
		segment0.m_sEntityKey = "hill";
		segment0.m_eKnowledge = 0;
		segment0.m_sMaterialId = "";
		segment0.m_bOpenings = false;
		input.m_aSegments.Insert(segment0);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.INVALID_OBSERVATION;
		expected.m_bHasLoss = false;
		t.CheckCase("O09", input, profile, expected);
	}

	protected static void Case9(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.INVALID_OBSERVATION;
		expected.m_bHasLoss = false;
		t.CheckCase("O10", input, profile, expected);
	}

	protected static void Case10(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(5.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(5.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.INVALID_OBSERVATION;
		expected.m_bHasLoss = false;
		t.CheckCase("O11", input, profile, expected);
	}

	protected static void Case11(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 1.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.INVALID_OBSERVATION;
		expected.m_bHasLoss = false;
		t.CheckCase("O12", input, profile, expected);
	}

	protected static void Case12(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 2;
		input.m_Tx.m_fDepthM = -0.5;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_PathSegment segment0 = new RICON_PathSegment();
		segment0.m_eFamily = 1;
		segment0.m_fStartM = 0.0;
		segment0.m_fEndM = 0.5;
		segment0.m_sEntityKey = "sea";
		segment0.m_eKnowledge = 0;
		segment0.m_sMaterialId = "";
		segment0.m_bOpenings = false;
		input.m_aSegments.Insert(segment0);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.INVALID_OBSERVATION;
		expected.m_bHasLoss = false;
		t.CheckCase("O13", input, profile, expected);
	}

	protected static void Case13(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_PathSegment segment0 = new RICON_PathSegment();
		segment0.m_eFamily = 1;
		segment0.m_fStartM = 300.0;
		segment0.m_fEndM = 310.0;
		segment0.m_sEntityKey = "z.pond";
		segment0.m_eKnowledge = 0;
		segment0.m_sMaterialId = "";
		segment0.m_bOpenings = false;
		input.m_aSegments.Insert(segment0);
		RICON_PathSegment segment1 = new RICON_PathSegment();
		segment1.m_eFamily = 2;
		segment1.m_fStartM = 300.0;
		segment1.m_fEndM = 305.0;
		segment1.m_sEntityKey = "a.wall";
		segment1.m_eKnowledge = 0;
		segment1.m_sMaterialId = "";
		segment1.m_bOpenings = false;
		input.m_aSegments.Insert(segment1);
		RICON_PathSegment segment2 = new RICON_PathSegment();
		segment2.m_eFamily = 2;
		segment2.m_fStartM = 300.0;
		segment2.m_fEndM = 302.0;
		segment2.m_sEntityKey = "b.wall";
		segment2.m_eKnowledge = 0;
		segment2.m_sMaterialId = "";
		segment2.m_bOpenings = false;
		input.m_aSegments.Insert(segment2);
		RICON_PathSegment segment3 = new RICON_PathSegment();
		segment3.m_eFamily = 4;
		segment3.m_fStartM = 300.0;
		segment3.m_fEndM = 301.0;
		segment3.m_sEntityKey = "a.car";
		segment3.m_eKnowledge = 0;
		segment3.m_sMaterialId = "";
		segment3.m_bOpenings = false;
		input.m_aSegments.Insert(segment3);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.OK;
		expected.m_bHasLoss = true;
		expected.m_fLossDb = 163.1034273996756;
		expected.m_fAlphaDbPerM = 15.415600130655974;
		expected.m_fInterfaceDb = 4.473713046557934;
		expected.m_fWaterLengthM = 10.0;
		expected.m_iCrossings = 2;
		RICON_WaterInterval interval0 = new RICON_WaterInterval();
		interval0.m_fStartM = 300.0;
		interval0.m_fEndM = 310.0;
		expected.m_aIntervals.Insert(interval0);
		t.CheckCase("O14", input, profile, expected);
	}

	protected static void Case14(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_PathSegment segment0 = new RICON_PathSegment();
		segment0.m_eFamily = 4;
		segment0.m_fStartM = 300.0;
		segment0.m_fEndM = 301.0;
		segment0.m_sEntityKey = "a.car";
		segment0.m_eKnowledge = 0;
		segment0.m_sMaterialId = "";
		segment0.m_bOpenings = false;
		input.m_aSegments.Insert(segment0);
		RICON_PathSegment segment1 = new RICON_PathSegment();
		segment1.m_eFamily = 1;
		segment1.m_fStartM = 300.0;
		segment1.m_fEndM = 310.0;
		segment1.m_sEntityKey = "z.pond";
		segment1.m_eKnowledge = 0;
		segment1.m_sMaterialId = "";
		segment1.m_bOpenings = false;
		input.m_aSegments.Insert(segment1);
		RICON_PathSegment segment2 = new RICON_PathSegment();
		segment2.m_eFamily = 2;
		segment2.m_fStartM = 300.0;
		segment2.m_fEndM = 305.0;
		segment2.m_sEntityKey = "a.wall";
		segment2.m_eKnowledge = 0;
		segment2.m_sMaterialId = "";
		segment2.m_bOpenings = false;
		input.m_aSegments.Insert(segment2);
		RICON_PathSegment segment3 = new RICON_PathSegment();
		segment3.m_eFamily = 2;
		segment3.m_fStartM = 300.0;
		segment3.m_fEndM = 302.0;
		segment3.m_sEntityKey = "b.wall";
		segment3.m_eKnowledge = 0;
		segment3.m_sMaterialId = "";
		segment3.m_bOpenings = false;
		input.m_aSegments.Insert(segment3);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.INVALID_OBSERVATION;
		expected.m_bHasLoss = false;
		t.CheckCase("O15", input, profile, expected);
	}

	protected static void Case15(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 0.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.INVALID_OBSERVATION;
		expected.m_bHasLoss = false;
		t.CheckCase("O17", input, profile, expected);
	}

	protected static void Case16(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 0;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.INVALID_OBSERVATION;
		expected.m_bHasLoss = false;
		t.CheckCase("O18", input, profile, expected);
	}

	protected static void Case17(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(RICON_L0Run.Special(1), 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.INVALID_OBSERVATION;
		expected.m_bHasLoss = false;
		t.CheckCase("O20", input, profile, expected);
	}

	protected static void Case18(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(100.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_PathSegment segment0 = new RICON_PathSegment();
		segment0.m_eFamily = 2;
		segment0.m_fStartM = 10.0;
		segment0.m_fEndM = 20.0;
		segment0.m_sEntityKey = "h";
		segment0.m_eKnowledge = 0;
		segment0.m_sMaterialId = "";
		segment0.m_bOpenings = false;
		input.m_aSegments.Insert(segment0);
		RICON_PathSegment segment1 = new RICON_PathSegment();
		segment1.m_eFamily = 2;
		segment1.m_fStartM = 20.0;
		segment1.m_fEndM = 30.0;
		segment1.m_sEntityKey = "h";
		segment1.m_eKnowledge = 0;
		segment1.m_sMaterialId = "";
		segment1.m_bOpenings = false;
		input.m_aSegments.Insert(segment1);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.NO_WATER;
		expected.m_bHasLoss = true;
		expected.m_fLossDb = 0.0;
		t.CheckCase("O21", input, profile, expected);
	}

	protected static void Case19(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_PathSegment segment0 = new RICON_PathSegment();
		segment0.m_eFamily = 2;
		segment0.m_fStartM = 1.0;
		segment0.m_fEndM = 2.0;
		segment0.m_sEntityKey = "x\"); injected(";
		segment0.m_eKnowledge = 0;
		segment0.m_sMaterialId = "";
		segment0.m_bOpenings = false;
		input.m_aSegments.Insert(segment0);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.INVALID_OBSERVATION;
		expected.m_bHasLoss = false;
		t.CheckCase("O23", input, profile, expected);
	}

	protected static void Case20(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(20000000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_PathSegment segment0 = new RICON_PathSegment();
		segment0.m_eFamily = 2;
		segment0.m_fStartM = 16777216.0;
		segment0.m_fEndM = 16777217.0;
		segment0.m_sEntityKey = "long";
		segment0.m_eKnowledge = 0;
		segment0.m_sMaterialId = "";
		segment0.m_bOpenings = false;
		input.m_aSegments.Insert(segment0);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.INVALID_OBSERVATION;
		expected.m_bHasLoss = false;
		t.CheckCase("O24", input, profile, expected);
	}

	protected static void Case21(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 2;
		input.m_Tx.m_fDepthM = 0.5;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_PathSegment segment0 = new RICON_PathSegment();
		segment0.m_eFamily = 1;
		segment0.m_fStartM = 0.0;
		segment0.m_fEndM = 0.5;
		segment0.m_sEntityKey = "sea";
		segment0.m_eKnowledge = 0;
		segment0.m_sMaterialId = "";
		segment0.m_bOpenings = false;
		input.m_aSegments.Insert(segment0);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.OK;
		expected.m_bHasLoss = true;
		expected.m_fLossDb = 12.18151311188592;
		expected.m_fAlphaDbPerM = 15.415600130655974;
		expected.m_fInterfaceDb = 4.473713046557934;
		expected.m_fWaterLengthM = 0.5;
		expected.m_iCrossings = 1;
		RICON_WaterInterval interval0 = new RICON_WaterInterval();
		interval0.m_fStartM = 0.0;
		interval0.m_fEndM = 0.5;
		expected.m_aIntervals.Insert(interval0);
		t.CheckCase("W01", input, profile, expected);
	}

	protected static void Case22(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 2;
		input.m_Rx.m_fDepthM = 2.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_PathSegment segment0 = new RICON_PathSegment();
		segment0.m_eFamily = 1;
		segment0.m_fStartM = 998.0;
		segment0.m_fEndM = 1000.0;
		segment0.m_sEntityKey = "lake.1";
		segment0.m_eKnowledge = 0;
		segment0.m_sMaterialId = "";
		segment0.m_bOpenings = false;
		input.m_aSegments.Insert(segment0);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.OK;
		expected.m_bHasLoss = true;
		expected.m_fLossDb = 35.30491330786988;
		expected.m_fAlphaDbPerM = 15.415600130655974;
		expected.m_fInterfaceDb = 4.473713046557934;
		expected.m_fWaterLengthM = 2.0;
		expected.m_iCrossings = 1;
		RICON_WaterInterval interval0 = new RICON_WaterInterval();
		interval0.m_fStartM = 998.0;
		interval0.m_fEndM = 1000.0;
		expected.m_aIntervals.Insert(interval0);
		t.CheckCase("W02", input, profile, expected);
	}

	protected static void Case23(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 2;
		input.m_Tx.m_fDepthM = 1.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(800.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 2;
		input.m_Rx.m_fDepthM = 1.5;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_PathSegment segment0 = new RICON_PathSegment();
		segment0.m_eFamily = 1;
		segment0.m_fStartM = 0.0;
		segment0.m_fEndM = 4.0;
		segment0.m_sEntityKey = "sea";
		segment0.m_eKnowledge = 0;
		segment0.m_sMaterialId = "";
		segment0.m_bOpenings = false;
		input.m_aSegments.Insert(segment0);
		RICON_PathSegment segment1 = new RICON_PathSegment();
		segment1.m_eFamily = 1;
		segment1.m_fStartM = 790.0;
		segment1.m_fEndM = 800.0;
		segment1.m_sEntityKey = "lake.1";
		segment1.m_eKnowledge = 0;
		segment1.m_sMaterialId = "";
		segment1.m_bOpenings = false;
		input.m_aSegments.Insert(segment1);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.OK;
		expected.m_bHasLoss = true;
		expected.m_fLossDb = 224.7658279222995;
		expected.m_fAlphaDbPerM = 15.415600130655974;
		expected.m_fInterfaceDb = 4.473713046557934;
		expected.m_fWaterLengthM = 14.0;
		expected.m_iCrossings = 2;
		RICON_WaterInterval interval0 = new RICON_WaterInterval();
		interval0.m_fStartM = 0.0;
		interval0.m_fEndM = 4.0;
		expected.m_aIntervals.Insert(interval0);
		RICON_WaterInterval interval1 = new RICON_WaterInterval();
		interval1.m_fStartM = 790.0;
		interval1.m_fEndM = 800.0;
		expected.m_aIntervals.Insert(interval1);
		t.CheckCase("W03", input, profile, expected);
	}

	protected static void Case24(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_PathSegment segment0 = new RICON_PathSegment();
		segment0.m_eFamily = 1;
		segment0.m_fStartM = 400.0;
		segment0.m_fEndM = 412.5;
		segment0.m_sEntityKey = "pond.4";
		segment0.m_eKnowledge = 0;
		segment0.m_sMaterialId = "";
		segment0.m_bOpenings = false;
		input.m_aSegments.Insert(segment0);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.OK;
		expected.m_bHasLoss = true;
		expected.m_fLossDb = 201.64242772631556;
		expected.m_fAlphaDbPerM = 15.415600130655974;
		expected.m_fInterfaceDb = 4.473713046557934;
		expected.m_fWaterLengthM = 12.5;
		expected.m_iCrossings = 2;
		RICON_WaterInterval interval0 = new RICON_WaterInterval();
		interval0.m_fStartM = 400.0;
		interval0.m_fEndM = 412.5;
		expected.m_aIntervals.Insert(interval0);
		t.CheckCase("W04", input, profile, expected);
	}

	protected static void Case25(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 2;
		input.m_Tx.m_fDepthM = 3.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(5.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 2;
		input.m_Rx.m_fDepthM = 3.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_PathSegment segment0 = new RICON_PathSegment();
		segment0.m_eFamily = 1;
		segment0.m_fStartM = 0.0;
		segment0.m_fEndM = 5.0;
		segment0.m_sEntityKey = "lake.1";
		segment0.m_eKnowledge = 0;
		segment0.m_sMaterialId = "";
		segment0.m_bOpenings = false;
		input.m_aSegments.Insert(segment0);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.OK;
		expected.m_bHasLoss = true;
		expected.m_fLossDb = 77.07800065327987;
		expected.m_fAlphaDbPerM = 15.415600130655974;
		expected.m_fInterfaceDb = 4.473713046557934;
		expected.m_fWaterLengthM = 5.0;
		expected.m_iCrossings = 0;
		RICON_WaterInterval interval0 = new RICON_WaterInterval();
		interval0.m_fStartM = 0.0;
		interval0.m_fEndM = 5.0;
		expected.m_aIntervals.Insert(interval0);
		t.CheckCase("W05", input, profile, expected);
	}

	protected static void Case26(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_PathSegment segment0 = new RICON_PathSegment();
		segment0.m_eFamily = 1;
		segment0.m_fStartM = 10.0;
		segment0.m_fEndM = 20.0;
		segment0.m_sEntityKey = "a.pond";
		segment0.m_eKnowledge = 0;
		segment0.m_sMaterialId = "";
		segment0.m_bOpenings = false;
		input.m_aSegments.Insert(segment0);
		RICON_PathSegment segment1 = new RICON_PathSegment();
		segment1.m_eFamily = 1;
		segment1.m_fStartM = 15.0;
		segment1.m_fEndM = 25.0;
		segment1.m_sEntityKey = "b.pond";
		segment1.m_eKnowledge = 0;
		segment1.m_sMaterialId = "";
		segment1.m_bOpenings = false;
		input.m_aSegments.Insert(segment1);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.OK;
		expected.m_bHasLoss = true;
		expected.m_fLossDb = 240.18142805295548;
		expected.m_fAlphaDbPerM = 15.415600130655974;
		expected.m_fInterfaceDb = 4.473713046557934;
		expected.m_fWaterLengthM = 15.0;
		expected.m_iCrossings = 2;
		RICON_WaterInterval interval0 = new RICON_WaterInterval();
		interval0.m_fStartM = 10.0;
		interval0.m_fEndM = 25.0;
		expected.m_aIntervals.Insert(interval0);
		t.CheckCase("W06", input, profile, expected);
	}

	protected static void Case27(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_PathSegment segment0 = new RICON_PathSegment();
		segment0.m_eFamily = 1;
		segment0.m_fStartM = 10.0;
		segment0.m_fEndM = 20.0;
		segment0.m_sEntityKey = "a.pond";
		segment0.m_eKnowledge = 0;
		segment0.m_sMaterialId = "";
		segment0.m_bOpenings = false;
		input.m_aSegments.Insert(segment0);
		RICON_PathSegment segment1 = new RICON_PathSegment();
		segment1.m_eFamily = 1;
		segment1.m_fStartM = 20.0;
		segment1.m_fEndM = 30.0;
		segment1.m_sEntityKey = "a.pond";
		segment1.m_eKnowledge = 0;
		segment1.m_sMaterialId = "";
		segment1.m_bOpenings = false;
		input.m_aSegments.Insert(segment1);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.OK;
		expected.m_bHasLoss = true;
		expected.m_fLossDb = 317.2594287062354;
		expected.m_fAlphaDbPerM = 15.415600130655974;
		expected.m_fInterfaceDb = 4.473713046557934;
		expected.m_fWaterLengthM = 20.0;
		expected.m_iCrossings = 2;
		RICON_WaterInterval interval0 = new RICON_WaterInterval();
		interval0.m_fStartM = 10.0;
		interval0.m_fEndM = 30.0;
		expected.m_aIntervals.Insert(interval0);
		t.CheckCase("W07", input, profile, expected);
	}

	protected static void Case28(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 2;
		input.m_Tx.m_fDepthM = 1.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_PathSegment segment0 = new RICON_PathSegment();
		segment0.m_eFamily = 1;
		segment0.m_fStartM = 0.0;
		segment0.m_fEndM = 6.0;
		segment0.m_sEntityKey = "a.sea";
		segment0.m_eKnowledge = 0;
		segment0.m_sMaterialId = "";
		segment0.m_bOpenings = false;
		input.m_aSegments.Insert(segment0);
		RICON_PathSegment segment1 = new RICON_PathSegment();
		segment1.m_eFamily = 1;
		segment1.m_fStartM = 6.0;
		segment1.m_fEndM = 9.0;
		segment1.m_sEntityKey = "b.bay";
		segment1.m_eKnowledge = 0;
		segment1.m_sMaterialId = "";
		segment1.m_bOpenings = false;
		input.m_aSegments.Insert(segment1);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.OK;
		expected.m_bHasLoss = true;
		expected.m_fLossDb = 143.21411422246172;
		expected.m_fAlphaDbPerM = 15.415600130655974;
		expected.m_fInterfaceDb = 4.473713046557934;
		expected.m_fWaterLengthM = 9.0;
		expected.m_iCrossings = 1;
		RICON_WaterInterval interval0 = new RICON_WaterInterval();
		interval0.m_fStartM = 0.0;
		interval0.m_fEndM = 9.0;
		expected.m_aIntervals.Insert(interval0);
		t.CheckCase("W08", input, profile, expected);
	}

	protected static void Case29(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 2;
		input.m_Tx.m_fDepthM = 1.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.INVALID_OBSERVATION;
		expected.m_bHasLoss = false;
		t.CheckCase("W09", input, profile, expected);
	}

	protected static void Case30(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_PathSegment segment0 = new RICON_PathSegment();
		segment0.m_eFamily = 1;
		segment0.m_fStartM = 0.0;
		segment0.m_fEndM = 3.0;
		segment0.m_sEntityKey = "pond";
		segment0.m_eKnowledge = 0;
		segment0.m_sMaterialId = "";
		segment0.m_bOpenings = false;
		input.m_aSegments.Insert(segment0);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.INVALID_OBSERVATION;
		expected.m_bHasLoss = false;
		t.CheckCase("W10", input, profile, expected);
	}

	protected static void Case31(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(1);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.WATER_COVERAGE_INCOMPLETE;
		expected.m_bHasLoss = false;
		t.CheckCase("W11", input, profile, expected);
	}

	protected static void Case32(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 0;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.WATER_COVERAGE_INCOMPLETE;
		expected.m_bHasLoss = false;
		t.CheckCase("W12", input, profile, expected);
	}

	protected static void Case33(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 1500000.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_PathSegment segment0 = new RICON_PathSegment();
		segment0.m_eFamily = 1;
		segment0.m_fStartM = 400.0;
		segment0.m_fEndM = 412.5;
		segment0.m_sEntityKey = "pond.4";
		segment0.m_eKnowledge = 0;
		segment0.m_sMaterialId = "";
		segment0.m_bOpenings = false;
		input.m_aSegments.Insert(segment0);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.OUT_OF_MODEL_DOMAIN;
		expected.m_bHasLoss = false;
		t.CheckCase("W13", input, profile, expected);
	}

	protected static void Case34(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 30.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_PathSegment segment0 = new RICON_PathSegment();
		segment0.m_eFamily = 1;
		segment0.m_fStartM = 400.0;
		segment0.m_fEndM = 412.5;
		segment0.m_sEntityKey = "pond.4";
		segment0.m_eKnowledge = 0;
		segment0.m_sMaterialId = "";
		segment0.m_bOpenings = false;
		input.m_aSegments.Insert(segment0);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.OK;
		expected.m_bHasLoss = true;
		expected.m_fLossDb = 182.05725443377446;
		expected.m_fAlphaDbPerM = 13.791156645501767;
		expected.m_fInterfaceDb = 4.833898182501189;
		expected.m_fWaterLengthM = 12.5;
		expected.m_iCrossings = 2;
		RICON_WaterInterval interval0 = new RICON_WaterInterval();
		interval0.m_fStartM = 400.0;
		interval0.m_fEndM = 412.5;
		expected.m_aIntervals.Insert(interval0);
		t.CheckCase("W14", input, profile, expected);
	}

	protected static void Case35(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 2000.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_PathSegment segment0 = new RICON_PathSegment();
		segment0.m_eFamily = 1;
		segment0.m_fStartM = 400.0;
		segment0.m_fEndM = 412.5;
		segment0.m_sEntityKey = "pond.4";
		segment0.m_eKnowledge = 0;
		segment0.m_sMaterialId = "";
		segment0.m_bOpenings = false;
		input.m_aSegments.Insert(segment0);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.OK;
		expected.m_bHasLoss = true;
		expected.m_fLossDb = 2764.275212908683;
		expected.m_fAlphaDbPerM = 220.43037923579234;
		expected.m_fInterfaceDb = 4.447736230639289;
		expected.m_fWaterLengthM = 12.5;
		expected.m_iCrossings = 2;
		RICON_WaterInterval interval0 = new RICON_WaterInterval();
		interval0.m_fStartM = 400.0;
		interval0.m_fEndM = 412.5;
		expected.m_aIntervals.Insert(interval0);
		t.CheckCase("W15", input, profile, expected);
	}

	protected static void Case36(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(300000000000000012135895401846682943488.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_PathSegment segment0 = new RICON_PathSegment();
		segment0.m_eFamily = 1;
		segment0.m_fStartM = 4999999999999999769381329101060571136.0;
		segment0.m_fEndM = 30000000000000002158062836758597337088.0;
		segment0.m_sEntityKey = "ocean";
		segment0.m_eKnowledge = 0;
		segment0.m_sMaterialId = "";
		segment0.m_bOpenings = false;
		input.m_aSegments.Insert(segment0);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.NUMERIC_FAILURE;
		expected.m_bHasLoss = false;
		t.CheckCase("W16", input, profile, expected);
	}

	protected static void Case37(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 0.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 2;
		input.m_Rx.m_fDepthM = 0.4;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_PathSegment segment0 = new RICON_PathSegment();
		segment0.m_eFamily = 1;
		segment0.m_fStartM = 2.0;
		segment0.m_fEndM = 2.2360680103302;
		segment0.m_sEntityKey = "puddle";
		segment0.m_eKnowledge = 0;
		segment0.m_sMaterialId = "";
		segment0.m_bOpenings = false;
		input.m_aSegments.Insert(segment0);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.OK;
		expected.m_bHasLoss = true;
		expected.m_fLossDb = 8.112843097447865;
		expected.m_fAlphaDbPerM = 15.415600130655974;
		expected.m_fInterfaceDb = 4.473713046557934;
		expected.m_fWaterLengthM = 0.2360680103302002;
		expected.m_iCrossings = 1;
		RICON_WaterInterval interval0 = new RICON_WaterInterval();
		interval0.m_fStartM = 2.0;
		interval0.m_fEndM = 2.2360680103302;
		expected.m_aIntervals.Insert(interval0);
		t.CheckCase("W25", input, profile, expected);
	}

	protected static void Case38(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 0.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1.0, 1.0, 0.0);
		input.m_Rx.m_eMedium = 2;
		input.m_Rx.m_fDepthM = 0.4;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_PathSegment segment0 = new RICON_PathSegment();
		segment0.m_eFamily = 1;
		segment0.m_fStartM = 1.0;
		segment0.m_fEndM = 1.4142135623730951;
		segment0.m_sEntityKey = "puddle";
		segment0.m_eKnowledge = 0;
		segment0.m_sMaterialId = "";
		segment0.m_bOpenings = false;
		input.m_aSegments.Insert(segment0);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.OK;
		expected.m_bHasLoss = true;
		expected.m_fLossDb = 10.859063319688714;
		expected.m_fAlphaDbPerM = 15.415600130655974;
		expected.m_fInterfaceDb = 4.473713046557934;
		expected.m_fWaterLengthM = 0.41421353816986084;
		expected.m_iCrossings = 1;
		RICON_WaterInterval interval0 = new RICON_WaterInterval();
		interval0.m_fStartM = 1.0;
		interval0.m_fEndM = 1.4142135381698608;
		expected.m_aIntervals.Insert(interval0);
		t.CheckCase("W26", input, profile, expected);
	}

	protected static void Case39(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 2;
		input.m_Tx.m_fDepthM = 1.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(1);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.WATER_COVERAGE_INCOMPLETE;
		expected.m_bHasLoss = false;
		t.CheckCase("W28", input, profile, expected);
	}

	protected static void Case40(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_PathSegment segment0 = new RICON_PathSegment();
		segment0.m_eFamily = 1;
		segment0.m_fStartM = 400.0;
		segment0.m_fEndM = 412.5;
		segment0.m_sEntityKey = "pond.4";
		segment0.m_eKnowledge = 0;
		segment0.m_sMaterialId = "";
		segment0.m_bOpenings = false;
		input.m_aSegments.Insert(segment0);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 41.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.OUT_OF_MODEL_DOMAIN;
		expected.m_bHasLoss = false;
		t.CheckCase("W18", input, profile, expected);
	}

	protected static void Case41(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_PathSegment segment0 = new RICON_PathSegment();
		segment0.m_eFamily = 1;
		segment0.m_fStartM = 400.0;
		segment0.m_fEndM = 412.5;
		segment0.m_sEntityKey = "pond.4";
		segment0.m_eKnowledge = 0;
		segment0.m_sMaterialId = "";
		segment0.m_bOpenings = false;
		input.m_aSegments.Insert(segment0);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 15.0;
		profile.m_fSalinityPpt = 40.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.OUT_OF_MODEL_DOMAIN;
		expected.m_bHasLoss = false;
		t.CheckCase("W19", input, profile, expected);
	}

	protected static void Case42(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_PathSegment segment0 = new RICON_PathSegment();
		segment0.m_eFamily = 1;
		segment0.m_fStartM = 400.0;
		segment0.m_fEndM = 412.5;
		segment0.m_sEntityKey = "pond.4";
		segment0.m_eKnowledge = 0;
		segment0.m_sMaterialId = "";
		segment0.m_bOpenings = false;
		input.m_aSegments.Insert(segment0);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = -4.0;
		profile.m_fSalinityPpt = 40.0;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.OK;
		expected.m_bHasLoss = true;
		expected.m_fLossDb = 4047.4690328096085;
		expected.m_fAlphaDbPerM = 322.4893211257719;
		expected.m_fInterfaceDb = 8.17625936872986;
		expected.m_fWaterLengthM = 12.5;
		expected.m_iCrossings = 2;
		RICON_WaterInterval interval0 = new RICON_WaterInterval();
		interval0.m_fStartM = 400.0;
		interval0.m_fEndM = 412.5;
		expected.m_aIntervals.Insert(interval0);
		t.CheckCase("W20", input, profile, expected);
	}

	protected static void Case43(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 99.0;
		profile.m_fSalinityPpt = 0.5;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.NO_WATER;
		expected.m_bHasLoss = true;
		expected.m_fLossDb = 0.0;
		t.CheckCase("W22", input, profile, expected);
	}

	protected static void Case44(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_PathSegment segment0 = new RICON_PathSegment();
		segment0.m_eFamily = 1;
		segment0.m_fStartM = 400.0;
		segment0.m_fEndM = 412.5;
		segment0.m_sEntityKey = "pond.4";
		segment0.m_eKnowledge = 0;
		segment0.m_sMaterialId = "";
		segment0.m_bOpenings = false;
		input.m_aSegments.Insert(segment0);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 20.0;
		profile.m_fSalinityPpt = 35.0;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.OK;
		expected.m_bHasLoss = true;
		expected.m_fLossDb = 5459.6713036633755;
		expected.m_fAlphaDbPerM = 435.2829022405236;
		expected.m_fInterfaceDb = 9.317512828415186;
		expected.m_fWaterLengthM = 12.5;
		expected.m_iCrossings = 2;
		RICON_WaterInterval interval0 = new RICON_WaterInterval();
		interval0.m_fStartM = 400.0;
		interval0.m_fEndM = 412.5;
		expected.m_aIntervals.Insert(interval0);
		t.CheckCase("W23", input, profile, expected);
	}

	protected static void Case45(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 150.0;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_PathSegment segment0 = new RICON_PathSegment();
		segment0.m_eFamily = 1;
		segment0.m_fStartM = 400.0;
		segment0.m_fEndM = 412.5;
		segment0.m_sEntityKey = "pond.4";
		segment0.m_eKnowledge = 0;
		segment0.m_sMaterialId = "";
		segment0.m_bOpenings = false;
		input.m_aSegments.Insert(segment0);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 20.0;
		profile.m_fSalinityPpt = 0.0;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.OK;
		expected.m_bHasLoss = true;
		expected.m_fLossDb = 21.448713693458025;
		expected.m_fAlphaDbPerM = 1.0091815129961668;
		expected.m_fInterfaceDb = 4.41697239050297;
		expected.m_fWaterLengthM = 12.5;
		expected.m_iCrossings = 2;
		RICON_WaterInterval interval0 = new RICON_WaterInterval();
		interval0.m_fStartM = 400.0;
		interval0.m_fEndM = 412.5;
		expected.m_aIntervals.Insert(interval0);
		t.CheckCase("W24", input, profile, expected);
	}

	protected static void Case46(RICON_PathObservationRun t)
	{
		RICON_PathObservation input = new RICON_PathObservation();
		input.m_fFrequencyMHz = 0.00000000000000000000000000000000001;
		input.m_iEnvironmentRevision = 1;
		input.m_Tx = new RICON_PathEndpoint();
		input.m_Tx.m_vPosition = Vector(0.0, 2.0, 0.0);
		input.m_Tx.m_eMedium = 1;
		input.m_Tx.m_fDepthM = 0.0;
		input.m_Rx = new RICON_PathEndpoint();
		input.m_Rx.m_vPosition = Vector(1000.0, 2.0, 0.0);
		input.m_Rx.m_eMedium = 1;
		input.m_Rx.m_fDepthM = 0.0;
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		input.m_aCoverage.Insert(2);
		RICON_PathSegment segment0 = new RICON_PathSegment();
		segment0.m_eFamily = 1;
		segment0.m_fStartM = 400.0;
		segment0.m_fEndM = 412.5;
		segment0.m_sEntityKey = "pond.4";
		segment0.m_eKnowledge = 0;
		segment0.m_sMaterialId = "";
		segment0.m_bOpenings = false;
		input.m_aSegments.Insert(segment0);
		RICON_WaterProfile profile = new RICON_WaterProfile();
		profile.m_fTempC = 20.0;
		profile.m_fSalinityPpt = 35.0;
		RICON_WaterLayerResult expected = new RICON_WaterLayerResult();
		expected.m_eCode = RICON_EWaterCode.NUMERIC_FAILURE;
		expected.m_bHasLoss = false;
		t.CheckCase("W27", input, profile, expected);
	}

}
