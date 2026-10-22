//! Сгенерировано gen_channel_cutoff_fixture.py; ожидания только копируются; TEST_ONLY.
//! Host-only (ошибки JSON-типа, недостижимы в Enforce): C23 — bool вместо времени, C24 — bool вместо мощности, C26 — лишнее поле JSON.
class RICON_ChannelCutoffFixtures
{
	static const string SOURCE_SHA = "b4ff545d34de7859aae3d71678689a727c835c57f46d429df1a722d492c654d8";
	static const int CASES = 24;
	static const int HOST_ONLY = 3;

	protected static void Case0(RICON_ChannelCutoffRun t)
	{
		RICON_CutoffInput input = new RICON_CutoffInput();
		input.m_iNowMs = 100;
		input.m_iValidFromMs = 0;
		input.m_iValidUntilMs = 200;
		input.m_iEnvironmentRevision = 1;
		input.m_iBoundRevision = 1;
		input.m_bCoverageComplete = true;
		input.m_bResourceExhausted = false;
		input.m_bHasTolerance = true;
		input.m_fToleranceDbm = -120.0;
		input.m_bHasNumericalGuard = true;
		input.m_fNumericalGuardDb = 0.0;
		RICON_CutoffPathBound path0 = new RICON_CutoffPathBound();
		path0.m_sPathId = "direct";
		path0.m_bHasUpper = true;
		path0.m_fUpperDbm = -150.0;
		input.m_aPaths.Insert(path0);
		RICON_CutoffResult expected = new RICON_CutoffResult();
		expected.m_eDecision = RICON_ECutoffDecision.BOUNDED;
		expected.m_eReason = RICON_ECutoffReason.BELOW_TOLERANCE;
		expected.m_bUsefulIsZero = true;
		expected.m_bHasUpper = true;
		expected.m_fUpperDbm = -150.0;
		t.CheckCase("C01", input, expected);
	}

	protected static void Case1(RICON_ChannelCutoffRun t)
	{
		RICON_CutoffInput input = new RICON_CutoffInput();
		input.m_iNowMs = 100;
		input.m_iValidFromMs = 0;
		input.m_iValidUntilMs = 200;
		input.m_iEnvironmentRevision = 1;
		input.m_iBoundRevision = 1;
		input.m_bCoverageComplete = true;
		input.m_bResourceExhausted = false;
		input.m_bHasTolerance = true;
		input.m_fToleranceDbm = -120.0;
		input.m_bHasNumericalGuard = true;
		input.m_fNumericalGuardDb = 0.0;
		RICON_CutoffPathBound path0 = new RICON_CutoffPathBound();
		path0.m_sPathId = "direct";
		path0.m_bHasUpper = true;
		path0.m_fUpperDbm = -120.0;
		input.m_aPaths.Insert(path0);
		RICON_CutoffResult expected = new RICON_CutoffResult();
		expected.m_eDecision = RICON_ECutoffDecision.CONTINUE;
		expected.m_eReason = RICON_ECutoffReason.ABOVE_TOLERANCE;
		expected.m_bUsefulIsZero = false;
		expected.m_bHasUpper = true;
		expected.m_fUpperDbm = -120.0;
		t.CheckCase("C02", input, expected);
	}

	protected static void Case2(RICON_ChannelCutoffRun t)
	{
		RICON_CutoffInput input = new RICON_CutoffInput();
		input.m_iNowMs = 100;
		input.m_iValidFromMs = 0;
		input.m_iValidUntilMs = 200;
		input.m_iEnvironmentRevision = 1;
		input.m_iBoundRevision = 1;
		input.m_bCoverageComplete = true;
		input.m_bResourceExhausted = false;
		input.m_bHasTolerance = true;
		input.m_fToleranceDbm = -120.0;
		input.m_bHasNumericalGuard = true;
		input.m_fNumericalGuardDb = 0.0;
		RICON_CutoffPathBound path0 = new RICON_CutoffPathBound();
		path0.m_sPathId = "direct";
		path0.m_bHasUpper = true;
		path0.m_fUpperDbm = -90.0;
		input.m_aPaths.Insert(path0);
		RICON_CutoffResult expected = new RICON_CutoffResult();
		expected.m_eDecision = RICON_ECutoffDecision.CONTINUE;
		expected.m_eReason = RICON_ECutoffReason.ABOVE_TOLERANCE;
		expected.m_bUsefulIsZero = false;
		expected.m_bHasUpper = true;
		expected.m_fUpperDbm = -90.0;
		t.CheckCase("C03", input, expected);
	}

	protected static void Case3(RICON_ChannelCutoffRun t)
	{
		RICON_CutoffInput input = new RICON_CutoffInput();
		input.m_iNowMs = 100;
		input.m_iValidFromMs = 0;
		input.m_iValidUntilMs = 200;
		input.m_iEnvironmentRevision = 1;
		input.m_iBoundRevision = 1;
		input.m_bCoverageComplete = true;
		input.m_bResourceExhausted = false;
		input.m_bHasTolerance = true;
		input.m_fToleranceDbm = -120.0;
		input.m_bHasNumericalGuard = true;
		input.m_fNumericalGuardDb = 0.0;
		RICON_CutoffPathBound path0 = new RICON_CutoffPathBound();
		path0.m_sPathId = "a";
		path0.m_bHasUpper = true;
		path0.m_fUpperDbm = -125.0;
		input.m_aPaths.Insert(path0);
		RICON_CutoffPathBound path1 = new RICON_CutoffPathBound();
		path1.m_sPathId = "b";
		path1.m_bHasUpper = true;
		path1.m_fUpperDbm = -125.0;
		input.m_aPaths.Insert(path1);
		RICON_CutoffResult expected = new RICON_CutoffResult();
		expected.m_eDecision = RICON_ECutoffDecision.CONTINUE;
		expected.m_eReason = RICON_ECutoffReason.ABOVE_TOLERANCE;
		expected.m_bUsefulIsZero = false;
		expected.m_bHasUpper = true;
		expected.m_fUpperDbm = -118.97940008672037;
		t.CheckCase("C04", input, expected);
	}

	protected static void Case4(RICON_ChannelCutoffRun t)
	{
		RICON_CutoffInput input = new RICON_CutoffInput();
		input.m_iNowMs = 100;
		input.m_iValidFromMs = 0;
		input.m_iValidUntilMs = 200;
		input.m_iEnvironmentRevision = 1;
		input.m_iBoundRevision = 1;
		input.m_bCoverageComplete = true;
		input.m_bResourceExhausted = false;
		input.m_bHasTolerance = true;
		input.m_fToleranceDbm = -120.0;
		input.m_bHasNumericalGuard = true;
		input.m_fNumericalGuardDb = 0.0;
		RICON_CutoffPathBound path0 = new RICON_CutoffPathBound();
		path0.m_sPathId = "0";
		path0.m_bHasUpper = true;
		path0.m_fUpperDbm = -140.0;
		input.m_aPaths.Insert(path0);
		RICON_CutoffPathBound path1 = new RICON_CutoffPathBound();
		path1.m_sPathId = "1";
		path1.m_bHasUpper = true;
		path1.m_fUpperDbm = -140.0;
		input.m_aPaths.Insert(path1);
		RICON_CutoffPathBound path2 = new RICON_CutoffPathBound();
		path2.m_sPathId = "2";
		path2.m_bHasUpper = true;
		path2.m_fUpperDbm = -140.0;
		input.m_aPaths.Insert(path2);
		RICON_CutoffPathBound path3 = new RICON_CutoffPathBound();
		path3.m_sPathId = "3";
		path3.m_bHasUpper = true;
		path3.m_fUpperDbm = -140.0;
		input.m_aPaths.Insert(path3);
		RICON_CutoffPathBound path4 = new RICON_CutoffPathBound();
		path4.m_sPathId = "4";
		path4.m_bHasUpper = true;
		path4.m_fUpperDbm = -140.0;
		input.m_aPaths.Insert(path4);
		RICON_CutoffPathBound path5 = new RICON_CutoffPathBound();
		path5.m_sPathId = "5";
		path5.m_bHasUpper = true;
		path5.m_fUpperDbm = -140.0;
		input.m_aPaths.Insert(path5);
		RICON_CutoffPathBound path6 = new RICON_CutoffPathBound();
		path6.m_sPathId = "6";
		path6.m_bHasUpper = true;
		path6.m_fUpperDbm = -140.0;
		input.m_aPaths.Insert(path6);
		RICON_CutoffPathBound path7 = new RICON_CutoffPathBound();
		path7.m_sPathId = "7";
		path7.m_bHasUpper = true;
		path7.m_fUpperDbm = -140.0;
		input.m_aPaths.Insert(path7);
		RICON_CutoffPathBound path8 = new RICON_CutoffPathBound();
		path8.m_sPathId = "8";
		path8.m_bHasUpper = true;
		path8.m_fUpperDbm = -140.0;
		input.m_aPaths.Insert(path8);
		RICON_CutoffPathBound path9 = new RICON_CutoffPathBound();
		path9.m_sPathId = "9";
		path9.m_bHasUpper = true;
		path9.m_fUpperDbm = -140.0;
		input.m_aPaths.Insert(path9);
		RICON_CutoffPathBound path10 = new RICON_CutoffPathBound();
		path10.m_sPathId = "10";
		path10.m_bHasUpper = true;
		path10.m_fUpperDbm = -140.0;
		input.m_aPaths.Insert(path10);
		RICON_CutoffPathBound path11 = new RICON_CutoffPathBound();
		path11.m_sPathId = "11";
		path11.m_bHasUpper = true;
		path11.m_fUpperDbm = -140.0;
		input.m_aPaths.Insert(path11);
		RICON_CutoffPathBound path12 = new RICON_CutoffPathBound();
		path12.m_sPathId = "12";
		path12.m_bHasUpper = true;
		path12.m_fUpperDbm = -140.0;
		input.m_aPaths.Insert(path12);
		RICON_CutoffPathBound path13 = new RICON_CutoffPathBound();
		path13.m_sPathId = "13";
		path13.m_bHasUpper = true;
		path13.m_fUpperDbm = -140.0;
		input.m_aPaths.Insert(path13);
		RICON_CutoffPathBound path14 = new RICON_CutoffPathBound();
		path14.m_sPathId = "14";
		path14.m_bHasUpper = true;
		path14.m_fUpperDbm = -140.0;
		input.m_aPaths.Insert(path14);
		RICON_CutoffPathBound path15 = new RICON_CutoffPathBound();
		path15.m_sPathId = "15";
		path15.m_bHasUpper = true;
		path15.m_fUpperDbm = -140.0;
		input.m_aPaths.Insert(path15);
		RICON_CutoffPathBound path16 = new RICON_CutoffPathBound();
		path16.m_sPathId = "16";
		path16.m_bHasUpper = true;
		path16.m_fUpperDbm = -140.0;
		input.m_aPaths.Insert(path16);
		RICON_CutoffPathBound path17 = new RICON_CutoffPathBound();
		path17.m_sPathId = "17";
		path17.m_bHasUpper = true;
		path17.m_fUpperDbm = -140.0;
		input.m_aPaths.Insert(path17);
		RICON_CutoffPathBound path18 = new RICON_CutoffPathBound();
		path18.m_sPathId = "18";
		path18.m_bHasUpper = true;
		path18.m_fUpperDbm = -140.0;
		input.m_aPaths.Insert(path18);
		RICON_CutoffPathBound path19 = new RICON_CutoffPathBound();
		path19.m_sPathId = "19";
		path19.m_bHasUpper = true;
		path19.m_fUpperDbm = -140.0;
		input.m_aPaths.Insert(path19);
		RICON_CutoffPathBound path20 = new RICON_CutoffPathBound();
		path20.m_sPathId = "20";
		path20.m_bHasUpper = true;
		path20.m_fUpperDbm = -140.0;
		input.m_aPaths.Insert(path20);
		RICON_CutoffPathBound path21 = new RICON_CutoffPathBound();
		path21.m_sPathId = "21";
		path21.m_bHasUpper = true;
		path21.m_fUpperDbm = -140.0;
		input.m_aPaths.Insert(path21);
		RICON_CutoffPathBound path22 = new RICON_CutoffPathBound();
		path22.m_sPathId = "22";
		path22.m_bHasUpper = true;
		path22.m_fUpperDbm = -140.0;
		input.m_aPaths.Insert(path22);
		RICON_CutoffPathBound path23 = new RICON_CutoffPathBound();
		path23.m_sPathId = "23";
		path23.m_bHasUpper = true;
		path23.m_fUpperDbm = -140.0;
		input.m_aPaths.Insert(path23);
		RICON_CutoffPathBound path24 = new RICON_CutoffPathBound();
		path24.m_sPathId = "24";
		path24.m_bHasUpper = true;
		path24.m_fUpperDbm = -140.0;
		input.m_aPaths.Insert(path24);
		RICON_CutoffPathBound path25 = new RICON_CutoffPathBound();
		path25.m_sPathId = "25";
		path25.m_bHasUpper = true;
		path25.m_fUpperDbm = -140.0;
		input.m_aPaths.Insert(path25);
		RICON_CutoffPathBound path26 = new RICON_CutoffPathBound();
		path26.m_sPathId = "26";
		path26.m_bHasUpper = true;
		path26.m_fUpperDbm = -140.0;
		input.m_aPaths.Insert(path26);
		RICON_CutoffPathBound path27 = new RICON_CutoffPathBound();
		path27.m_sPathId = "27";
		path27.m_bHasUpper = true;
		path27.m_fUpperDbm = -140.0;
		input.m_aPaths.Insert(path27);
		RICON_CutoffPathBound path28 = new RICON_CutoffPathBound();
		path28.m_sPathId = "28";
		path28.m_bHasUpper = true;
		path28.m_fUpperDbm = -140.0;
		input.m_aPaths.Insert(path28);
		RICON_CutoffPathBound path29 = new RICON_CutoffPathBound();
		path29.m_sPathId = "29";
		path29.m_bHasUpper = true;
		path29.m_fUpperDbm = -140.0;
		input.m_aPaths.Insert(path29);
		RICON_CutoffResult expected = new RICON_CutoffResult();
		expected.m_eDecision = RICON_ECutoffDecision.CONTINUE;
		expected.m_eReason = RICON_ECutoffReason.ABOVE_TOLERANCE;
		expected.m_bUsefulIsZero = false;
		expected.m_bHasUpper = true;
		expected.m_fUpperDbm = -110.45757490560675;
		t.CheckCase("C05", input, expected);
	}

	protected static void Case5(RICON_ChannelCutoffRun t)
	{
		RICON_CutoffInput input = new RICON_CutoffInput();
		input.m_iNowMs = 100;
		input.m_iValidFromMs = 0;
		input.m_iValidUntilMs = 200;
		input.m_iEnvironmentRevision = 1;
		input.m_iBoundRevision = 1;
		input.m_bCoverageComplete = true;
		input.m_bResourceExhausted = false;
		input.m_bHasTolerance = true;
		input.m_fToleranceDbm = -120.0;
		input.m_bHasNumericalGuard = true;
		input.m_fNumericalGuardDb = 0.0;
		RICON_CutoffPathBound path0 = new RICON_CutoffPathBound();
		path0.m_sPathId = "direct";
		path0.m_bHasUpper = false;
		input.m_aPaths.Insert(path0);
		RICON_CutoffResult expected = new RICON_CutoffResult();
		expected.m_eDecision = RICON_ECutoffDecision.CONTINUE;
		expected.m_eReason = RICON_ECutoffReason.MISSING_PATH_BOUND;
		expected.m_bUsefulIsZero = false;
		expected.m_bHasUpper = false;
		t.CheckCase("C06", input, expected);
	}

	protected static void Case6(RICON_ChannelCutoffRun t)
	{
		RICON_CutoffInput input = new RICON_CutoffInput();
		input.m_iNowMs = 100;
		input.m_iValidFromMs = 0;
		input.m_iValidUntilMs = 200;
		input.m_iEnvironmentRevision = 1;
		input.m_iBoundRevision = 1;
		input.m_bCoverageComplete = false;
		input.m_bResourceExhausted = false;
		input.m_bHasTolerance = true;
		input.m_fToleranceDbm = -120.0;
		input.m_bHasNumericalGuard = true;
		input.m_fNumericalGuardDb = 0.0;
		RICON_CutoffPathBound path0 = new RICON_CutoffPathBound();
		path0.m_sPathId = "direct";
		path0.m_bHasUpper = true;
		path0.m_fUpperDbm = -150.0;
		input.m_aPaths.Insert(path0);
		RICON_CutoffResult expected = new RICON_CutoffResult();
		expected.m_eDecision = RICON_ECutoffDecision.UNRESOLVED;
		expected.m_eReason = RICON_ECutoffReason.INCOMPLETE_COVERAGE;
		expected.m_bUsefulIsZero = false;
		expected.m_bHasUpper = false;
		t.CheckCase("C07", input, expected);
	}

	protected static void Case7(RICON_ChannelCutoffRun t)
	{
		RICON_CutoffInput input = new RICON_CutoffInput();
		input.m_iNowMs = 100;
		input.m_iValidFromMs = 0;
		input.m_iValidUntilMs = 200;
		input.m_iEnvironmentRevision = 1;
		input.m_iBoundRevision = 1;
		input.m_bCoverageComplete = true;
		input.m_bResourceExhausted = true;
		input.m_bHasTolerance = true;
		input.m_fToleranceDbm = -120.0;
		input.m_bHasNumericalGuard = true;
		input.m_fNumericalGuardDb = 0.0;
		RICON_CutoffPathBound path0 = new RICON_CutoffPathBound();
		path0.m_sPathId = "direct";
		path0.m_bHasUpper = true;
		path0.m_fUpperDbm = -150.0;
		input.m_aPaths.Insert(path0);
		RICON_CutoffResult expected = new RICON_CutoffResult();
		expected.m_eDecision = RICON_ECutoffDecision.UNRESOLVED;
		expected.m_eReason = RICON_ECutoffReason.RESOURCE_EXHAUSTED;
		expected.m_bUsefulIsZero = false;
		expected.m_bHasUpper = false;
		t.CheckCase("C08", input, expected);
	}

	protected static void Case8(RICON_ChannelCutoffRun t)
	{
		RICON_CutoffInput input = new RICON_CutoffInput();
		input.m_iNowMs = 100;
		input.m_iValidFromMs = 0;
		input.m_iValidUntilMs = 200;
		input.m_iEnvironmentRevision = 2;
		input.m_iBoundRevision = 1;
		input.m_bCoverageComplete = true;
		input.m_bResourceExhausted = false;
		input.m_bHasTolerance = true;
		input.m_fToleranceDbm = -120.0;
		input.m_bHasNumericalGuard = true;
		input.m_fNumericalGuardDb = 0.0;
		RICON_CutoffPathBound path0 = new RICON_CutoffPathBound();
		path0.m_sPathId = "direct";
		path0.m_bHasUpper = true;
		path0.m_fUpperDbm = -150.0;
		input.m_aPaths.Insert(path0);
		RICON_CutoffResult expected = new RICON_CutoffResult();
		expected.m_eDecision = RICON_ECutoffDecision.UNRESOLVED;
		expected.m_eReason = RICON_ECutoffReason.STALE_BOUND;
		expected.m_bUsefulIsZero = false;
		expected.m_bHasUpper = false;
		t.CheckCase("C09", input, expected);
	}

	protected static void Case9(RICON_ChannelCutoffRun t)
	{
		RICON_CutoffInput input = new RICON_CutoffInput();
		input.m_iNowMs = 0;
		input.m_iValidFromMs = 0;
		input.m_iValidUntilMs = 200;
		input.m_iEnvironmentRevision = 1;
		input.m_iBoundRevision = 1;
		input.m_bCoverageComplete = true;
		input.m_bResourceExhausted = false;
		input.m_bHasTolerance = true;
		input.m_fToleranceDbm = -120.0;
		input.m_bHasNumericalGuard = true;
		input.m_fNumericalGuardDb = 0.0;
		RICON_CutoffPathBound path0 = new RICON_CutoffPathBound();
		path0.m_sPathId = "direct";
		path0.m_bHasUpper = true;
		path0.m_fUpperDbm = -150.0;
		input.m_aPaths.Insert(path0);
		RICON_CutoffResult expected = new RICON_CutoffResult();
		expected.m_eDecision = RICON_ECutoffDecision.BOUNDED;
		expected.m_eReason = RICON_ECutoffReason.BELOW_TOLERANCE;
		expected.m_bUsefulIsZero = true;
		expected.m_bHasUpper = true;
		expected.m_fUpperDbm = -150.0;
		t.CheckCase("C10", input, expected);
	}

	protected static void Case10(RICON_ChannelCutoffRun t)
	{
		RICON_CutoffInput input = new RICON_CutoffInput();
		input.m_iNowMs = 200;
		input.m_iValidFromMs = 0;
		input.m_iValidUntilMs = 200;
		input.m_iEnvironmentRevision = 1;
		input.m_iBoundRevision = 1;
		input.m_bCoverageComplete = true;
		input.m_bResourceExhausted = false;
		input.m_bHasTolerance = true;
		input.m_fToleranceDbm = -120.0;
		input.m_bHasNumericalGuard = true;
		input.m_fNumericalGuardDb = 0.0;
		RICON_CutoffPathBound path0 = new RICON_CutoffPathBound();
		path0.m_sPathId = "direct";
		path0.m_bHasUpper = true;
		path0.m_fUpperDbm = -150.0;
		input.m_aPaths.Insert(path0);
		RICON_CutoffResult expected = new RICON_CutoffResult();
		expected.m_eDecision = RICON_ECutoffDecision.UNRESOLVED;
		expected.m_eReason = RICON_ECutoffReason.STALE_BOUND;
		expected.m_bUsefulIsZero = false;
		expected.m_bHasUpper = false;
		t.CheckCase("C11", input, expected);
	}

	protected static void Case11(RICON_ChannelCutoffRun t)
	{
		RICON_CutoffInput input = new RICON_CutoffInput();
		input.m_iNowMs = 100;
		input.m_iValidFromMs = 101;
		input.m_iValidUntilMs = 200;
		input.m_iEnvironmentRevision = 1;
		input.m_iBoundRevision = 1;
		input.m_bCoverageComplete = true;
		input.m_bResourceExhausted = false;
		input.m_bHasTolerance = true;
		input.m_fToleranceDbm = -120.0;
		input.m_bHasNumericalGuard = true;
		input.m_fNumericalGuardDb = 0.0;
		RICON_CutoffPathBound path0 = new RICON_CutoffPathBound();
		path0.m_sPathId = "direct";
		path0.m_bHasUpper = true;
		path0.m_fUpperDbm = -150.0;
		input.m_aPaths.Insert(path0);
		RICON_CutoffResult expected = new RICON_CutoffResult();
		expected.m_eDecision = RICON_ECutoffDecision.UNRESOLVED;
		expected.m_eReason = RICON_ECutoffReason.STALE_BOUND;
		expected.m_bUsefulIsZero = false;
		expected.m_bHasUpper = false;
		t.CheckCase("C12", input, expected);
	}

	protected static void Case12(RICON_ChannelCutoffRun t)
	{
		RICON_CutoffInput input = new RICON_CutoffInput();
		input.m_iNowMs = 100;
		input.m_iValidFromMs = 0;
		input.m_iValidUntilMs = 200;
		input.m_iEnvironmentRevision = 1;
		input.m_iBoundRevision = 1;
		input.m_bCoverageComplete = true;
		input.m_bResourceExhausted = false;
		input.m_bHasTolerance = true;
		input.m_fToleranceDbm = -120.0;
		input.m_bHasNumericalGuard = true;
		input.m_fNumericalGuardDb = 0.0;
		RICON_CutoffResult expected = new RICON_CutoffResult();
		expected.m_eDecision = RICON_ECutoffDecision.BOUNDED;
		expected.m_eReason = RICON_ECutoffReason.EMPTY_PATH_SET;
		expected.m_bUsefulIsZero = true;
		expected.m_bHasUpper = false;
		t.CheckCase("C13", input, expected);
	}

	protected static void Case13(RICON_ChannelCutoffRun t)
	{
		RICON_CutoffInput input = new RICON_CutoffInput();
		input.m_iNowMs = 100;
		input.m_iValidFromMs = 0;
		input.m_iValidUntilMs = 200;
		input.m_iEnvironmentRevision = 1;
		input.m_iBoundRevision = 1;
		input.m_bCoverageComplete = false;
		input.m_bResourceExhausted = false;
		input.m_bHasTolerance = true;
		input.m_fToleranceDbm = -120.0;
		input.m_bHasNumericalGuard = true;
		input.m_fNumericalGuardDb = 0.0;
		RICON_CutoffResult expected = new RICON_CutoffResult();
		expected.m_eDecision = RICON_ECutoffDecision.UNRESOLVED;
		expected.m_eReason = RICON_ECutoffReason.INCOMPLETE_COVERAGE;
		expected.m_bUsefulIsZero = false;
		expected.m_bHasUpper = false;
		t.CheckCase("C14", input, expected);
	}

	protected static void Case14(RICON_ChannelCutoffRun t)
	{
		RICON_CutoffInput input = new RICON_CutoffInput();
		input.m_iNowMs = 100;
		input.m_iValidFromMs = 0;
		input.m_iValidUntilMs = 200;
		input.m_iEnvironmentRevision = 1;
		input.m_iBoundRevision = 1;
		input.m_bCoverageComplete = true;
		input.m_bResourceExhausted = false;
		input.m_bHasTolerance = false;
		input.m_bHasNumericalGuard = true;
		input.m_fNumericalGuardDb = 0.0;
		RICON_CutoffPathBound path0 = new RICON_CutoffPathBound();
		path0.m_sPathId = "direct";
		path0.m_bHasUpper = true;
		path0.m_fUpperDbm = -150.0;
		input.m_aPaths.Insert(path0);
		RICON_CutoffResult expected = new RICON_CutoffResult();
		expected.m_eDecision = RICON_ECutoffDecision.CONTINUE;
		expected.m_eReason = RICON_ECutoffReason.CUTOFF_DISABLED;
		expected.m_bUsefulIsZero = false;
		expected.m_bHasUpper = true;
		expected.m_fUpperDbm = -150.0;
		t.CheckCase("C15", input, expected);
	}

	protected static void Case15(RICON_ChannelCutoffRun t)
	{
		RICON_CutoffInput input = new RICON_CutoffInput();
		input.m_iNowMs = 100;
		input.m_iValidFromMs = 0;
		input.m_iValidUntilMs = 200;
		input.m_iEnvironmentRevision = 1;
		input.m_iBoundRevision = 1;
		input.m_bCoverageComplete = true;
		input.m_bResourceExhausted = false;
		input.m_bHasTolerance = true;
		input.m_fToleranceDbm = -120.0;
		input.m_bHasNumericalGuard = false;
		RICON_CutoffPathBound path0 = new RICON_CutoffPathBound();
		path0.m_sPathId = "direct";
		path0.m_bHasUpper = true;
		path0.m_fUpperDbm = -150.0;
		input.m_aPaths.Insert(path0);
		RICON_CutoffResult expected = new RICON_CutoffResult();
		expected.m_eDecision = RICON_ECutoffDecision.CONTINUE;
		expected.m_eReason = RICON_ECutoffReason.NUMERIC_UNCERTIFIED;
		expected.m_bUsefulIsZero = false;
		expected.m_bHasUpper = false;
		t.CheckCase("C16", input, expected);
	}

	protected static void Case16(RICON_ChannelCutoffRun t)
	{
		RICON_CutoffInput input = new RICON_CutoffInput();
		input.m_iNowMs = 100;
		input.m_iValidFromMs = 0;
		input.m_iValidUntilMs = 200;
		input.m_iEnvironmentRevision = 1;
		input.m_iBoundRevision = 1;
		input.m_bCoverageComplete = true;
		input.m_bResourceExhausted = false;
		input.m_bHasTolerance = true;
		input.m_fToleranceDbm = -120.0;
		input.m_bHasNumericalGuard = true;
		input.m_fNumericalGuardDb = 31.0;
		RICON_CutoffPathBound path0 = new RICON_CutoffPathBound();
		path0.m_sPathId = "direct";
		path0.m_bHasUpper = true;
		path0.m_fUpperDbm = -150.0;
		input.m_aPaths.Insert(path0);
		RICON_CutoffResult expected = new RICON_CutoffResult();
		expected.m_eDecision = RICON_ECutoffDecision.CONTINUE;
		expected.m_eReason = RICON_ECutoffReason.ABOVE_TOLERANCE;
		expected.m_bUsefulIsZero = false;
		expected.m_bHasUpper = true;
		expected.m_fUpperDbm = -119.0;
		t.CheckCase("C17", input, expected);
	}

	protected static void Case17(RICON_ChannelCutoffRun t)
	{
		RICON_CutoffInput input = new RICON_CutoffInput();
		input.m_iNowMs = 100;
		input.m_iValidFromMs = 0;
		input.m_iValidUntilMs = 200;
		input.m_iEnvironmentRevision = 1;
		input.m_iBoundRevision = 1;
		input.m_bCoverageComplete = true;
		input.m_bResourceExhausted = false;
		input.m_bHasTolerance = true;
		input.m_fToleranceDbm = -120.0;
		input.m_bHasNumericalGuard = true;
		input.m_fNumericalGuardDb = 0.0;
		RICON_CutoffPathBound path0 = new RICON_CutoffPathBound();
		path0.m_sPathId = "a";
		path0.m_bHasUpper = true;
		path0.m_fUpperDbm = -400.0;
		input.m_aPaths.Insert(path0);
		RICON_CutoffPathBound path1 = new RICON_CutoffPathBound();
		path1.m_sPathId = "b";
		path1.m_bHasUpper = true;
		path1.m_fUpperDbm = 400.0;
		input.m_aPaths.Insert(path1);
		RICON_CutoffResult expected = new RICON_CutoffResult();
		expected.m_eDecision = RICON_ECutoffDecision.CONTINUE;
		expected.m_eReason = RICON_ECutoffReason.ABOVE_TOLERANCE;
		expected.m_bUsefulIsZero = false;
		expected.m_bHasUpper = true;
		expected.m_fUpperDbm = 400.0;
		t.CheckCase("C18", input, expected);
	}

	protected static void Case18(RICON_ChannelCutoffRun t)
	{
		RICON_CutoffInput input = new RICON_CutoffInput();
		input.m_iNowMs = 100;
		input.m_iValidFromMs = 0;
		input.m_iValidUntilMs = 200;
		input.m_iEnvironmentRevision = 1;
		input.m_iBoundRevision = 1;
		input.m_bCoverageComplete = true;
		input.m_bResourceExhausted = false;
		input.m_bHasTolerance = true;
		input.m_fToleranceDbm = -120.0;
		input.m_bHasNumericalGuard = true;
		input.m_fNumericalGuardDb = 0.0;
		RICON_CutoffPathBound path0 = new RICON_CutoffPathBound();
		path0.m_sPathId = "direct";
		path0.m_bHasUpper = true;
		path0.m_fUpperDbm = -150.0;
		input.m_aPaths.Insert(path0);
		RICON_CutoffPathBound path1 = new RICON_CutoffPathBound();
		path1.m_sPathId = "x";
		path1.m_bHasUpper = true;
		path1.m_fUpperDbm = RICON_L0Run.Special(0);
		input.m_aPaths.Insert(path1);
		RICON_CutoffResult expected = new RICON_CutoffResult();
		expected.m_eDecision = RICON_ECutoffDecision.UNRESOLVED;
		expected.m_eReason = RICON_ECutoffReason.INVALID_INPUT;
		expected.m_bUsefulIsZero = false;
		expected.m_bHasUpper = false;
		t.CheckCase("C19", input, expected);
	}

	protected static void Case19(RICON_ChannelCutoffRun t)
	{
		RICON_CutoffInput input = new RICON_CutoffInput();
		input.m_iNowMs = 100;
		input.m_iValidFromMs = 0;
		input.m_iValidUntilMs = 200;
		input.m_iEnvironmentRevision = 1;
		input.m_iBoundRevision = 1;
		input.m_bCoverageComplete = true;
		input.m_bResourceExhausted = false;
		input.m_bHasTolerance = true;
		input.m_fToleranceDbm = -120.0;
		input.m_bHasNumericalGuard = true;
		input.m_fNumericalGuardDb = 0.0;
		RICON_CutoffPathBound path0 = new RICON_CutoffPathBound();
		path0.m_sPathId = "direct";
		path0.m_bHasUpper = true;
		path0.m_fUpperDbm = RICON_L0Run.Special(1);
		input.m_aPaths.Insert(path0);
		RICON_CutoffResult expected = new RICON_CutoffResult();
		expected.m_eDecision = RICON_ECutoffDecision.UNRESOLVED;
		expected.m_eReason = RICON_ECutoffReason.INVALID_INPUT;
		expected.m_bUsefulIsZero = false;
		expected.m_bHasUpper = false;
		t.CheckCase("C20", input, expected);
	}

	protected static void Case20(RICON_ChannelCutoffRun t)
	{
		RICON_CutoffInput input = new RICON_CutoffInput();
		input.m_iNowMs = 100;
		input.m_iValidFromMs = 0;
		input.m_iValidUntilMs = 200;
		input.m_iEnvironmentRevision = 1;
		input.m_iBoundRevision = 1;
		input.m_bCoverageComplete = true;
		input.m_bResourceExhausted = false;
		input.m_bHasTolerance = true;
		input.m_fToleranceDbm = -120.0;
		input.m_bHasNumericalGuard = true;
		input.m_fNumericalGuardDb = -1.0;
		RICON_CutoffPathBound path0 = new RICON_CutoffPathBound();
		path0.m_sPathId = "direct";
		path0.m_bHasUpper = true;
		path0.m_fUpperDbm = -150.0;
		input.m_aPaths.Insert(path0);
		RICON_CutoffResult expected = new RICON_CutoffResult();
		expected.m_eDecision = RICON_ECutoffDecision.UNRESOLVED;
		expected.m_eReason = RICON_ECutoffReason.INVALID_INPUT;
		expected.m_bUsefulIsZero = false;
		expected.m_bHasUpper = false;
		t.CheckCase("C21", input, expected);
	}

	protected static void Case21(RICON_ChannelCutoffRun t)
	{
		RICON_CutoffInput input = new RICON_CutoffInput();
		input.m_iNowMs = 100;
		input.m_iValidFromMs = 0;
		input.m_iValidUntilMs = 200;
		input.m_iEnvironmentRevision = 1;
		input.m_iBoundRevision = 1;
		input.m_bCoverageComplete = true;
		input.m_bResourceExhausted = false;
		input.m_bHasTolerance = true;
		input.m_fToleranceDbm = -120.0;
		input.m_bHasNumericalGuard = true;
		input.m_fNumericalGuardDb = 0.0;
		RICON_CutoffPathBound path0 = new RICON_CutoffPathBound();
		path0.m_sPathId = "direct";
		path0.m_bHasUpper = true;
		path0.m_fUpperDbm = -150.0;
		input.m_aPaths.Insert(path0);
		RICON_CutoffPathBound path1 = new RICON_CutoffPathBound();
		path1.m_sPathId = "direct";
		path1.m_bHasUpper = true;
		path1.m_fUpperDbm = -150.0;
		input.m_aPaths.Insert(path1);
		RICON_CutoffResult expected = new RICON_CutoffResult();
		expected.m_eDecision = RICON_ECutoffDecision.UNRESOLVED;
		expected.m_eReason = RICON_ECutoffReason.INVALID_INPUT;
		expected.m_bUsefulIsZero = false;
		expected.m_bHasUpper = false;
		t.CheckCase("C22", input, expected);
	}

	protected static void Case22(RICON_ChannelCutoffRun t)
	{
		RICON_CutoffInput input = new RICON_CutoffInput();
		input.m_iNowMs = 100;
		input.m_iValidFromMs = 0;
		input.m_iValidUntilMs = 0;
		input.m_iEnvironmentRevision = 1;
		input.m_iBoundRevision = 1;
		input.m_bCoverageComplete = true;
		input.m_bResourceExhausted = false;
		input.m_bHasTolerance = true;
		input.m_fToleranceDbm = -120.0;
		input.m_bHasNumericalGuard = true;
		input.m_fNumericalGuardDb = 0.0;
		RICON_CutoffPathBound path0 = new RICON_CutoffPathBound();
		path0.m_sPathId = "direct";
		path0.m_bHasUpper = true;
		path0.m_fUpperDbm = -150.0;
		input.m_aPaths.Insert(path0);
		RICON_CutoffResult expected = new RICON_CutoffResult();
		expected.m_eDecision = RICON_ECutoffDecision.UNRESOLVED;
		expected.m_eReason = RICON_ECutoffReason.INVALID_INPUT;
		expected.m_bUsefulIsZero = false;
		expected.m_bHasUpper = false;
		t.CheckCase("C25", input, expected);
	}

	protected static void Case23(RICON_ChannelCutoffRun t)
	{
		RICON_CutoffInput input = new RICON_CutoffInput();
		input.m_iNowMs = 100;
		input.m_iValidFromMs = 0;
		input.m_iValidUntilMs = 200;
		input.m_iEnvironmentRevision = 1;
		input.m_iBoundRevision = 1;
		input.m_bCoverageComplete = true;
		input.m_bResourceExhausted = false;
		input.m_bHasTolerance = true;
		input.m_fToleranceDbm = -120.0;
		input.m_bHasNumericalGuard = true;
		input.m_fNumericalGuardDb = 340282346638528859811704183484516925440.0;
		RICON_CutoffPathBound path0 = new RICON_CutoffPathBound();
		path0.m_sPathId = "a";
		path0.m_bHasUpper = true;
		path0.m_fUpperDbm = 340282346638528859811704183484516925440.0;
		input.m_aPaths.Insert(path0);
		RICON_CutoffResult expected = new RICON_CutoffResult();
		expected.m_eDecision = RICON_ECutoffDecision.UNRESOLVED;
		expected.m_eReason = RICON_ECutoffReason.NUMERIC_FAILURE;
		expected.m_bUsefulIsZero = false;
		expected.m_bHasUpper = false;
		t.CheckCase("C27", input, expected);
	}

	static void Run(RICON_ChannelCutoffRun t)
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
	}
}
