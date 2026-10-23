//! Сгенерировано gen_material_slab_fixture.py; ожидания только копируются; TEST_ONLY.
//! Host-only (JSON-only в корпусе): MS054 — bool вместо frequencyMHz, MS055 — bool вместо thicknessM, MS056 — bool вместо cosIncidence, MS058 — лишнее поле, MS059 — число строкой, MS068 — переполнение binary32 (1e39), MS069 — 3.4028236e38 округляется в Inf, MS070 — FLT_MAX вне области: литерал небезопасен в native, MS076 — отрицательный ноль JSON.
class RICON_MaterialSlabFixtures
{
	static const string SOURCE_SHA = "2f5a4f10f5e1e472ea9eb253bc07230c806ad908002d59ab4db19b90cce1efbd";
	static const int CASES = 71;
	static const int HOST_ONLY = 9;

	protected static void Case0(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(0); // 0
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(0); // 0
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS001", input, expected);
	}

	protected static void Case1(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1045220557); // 0.2
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1065353216); // 1
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(0); // 0
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS002", input, expected);
	}

	protected static void Case2(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1052761599); // 0.3747405725
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1073223410); // 1.938200260161128
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS003", input, expected);
	}

	protected static void Case3(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1061150207); // 0.749481145
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(0); // 0
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS004", input, expected);
	}

	protected static void Case4(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1045220557); // 0.2
		input.m_fCosIncidence = t.F(1055193390); // 0.4472135954999579
		input.m_ePolarization = RICON_ESlabPolarization.TM;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(0); // 0
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS005", input, expected);
	}

	protected static void Case5(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1045220557); // 0.2
		input.m_fCosIncidence = t.F(1065353216); // 1
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1066829088); // 1.1759376792334777
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS006", input, expected);
	}

	protected static void Case6(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1045220557); // 0.2
		input.m_fCosIncidence = t.F(1065353216); // 1
		input.m_ePolarization = RICON_ESlabPolarization.TM;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1066829088); // 1.1759376792334777
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS007", input, expected);
	}

	protected static void Case7(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1045220557); // 0.2
		input.m_fCosIncidence = t.F(1056964608); // 0.5
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1080543651); // 3.621681992758499
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS008", input, expected);
	}

	protected static void Case8(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1045220557); // 0.2
		input.m_fCosIncidence = t.F(1056964608); // 0.5
		input.m_ePolarization = RICON_ESlabPolarization.TM;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1018464303); // 0.02203759276138663
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS009", input, expected);
	}

	protected static void Case9(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1045220557); // 0.2
		input.m_fCosIncidence = t.F(1035108022); // 0.08715574274765814
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1099161117); // 16.483453915273802
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS010", input, expected);
	}

	protected static void Case10(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1045220557); // 0.2
		input.m_fCosIncidence = t.F(1035108022); // 0.08715574274765814
		input.m_ePolarization = RICON_ESlabPolarization.TM;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1085193469); // 5.460569704631374
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS011", input, expected);
	}

	protected static void Case11(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1045220557); // 0.2
		input.m_fCosIncidence = t.F(1028443341); // 0.05000000074505806
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1101657713); // 21.245332907868733
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS012", input, expected);
	}

	protected static void Case12(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1045220557); // 0.2
		input.m_fCosIncidence = t.F(1028443341); // 0.05000000074505806
		input.m_ePolarization = RICON_ESlabPolarization.TM;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1092165528); // 9.570213774104827
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS013", input, expected);
	}

	protected static void Case13(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1101004800); // 20
		input.m_fThicknessM = t.F(953267991); // 0.0001
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1065353216); // 1
		input.m_Material.m_fConductivitySm = t.F(1259902592); // 10000000.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1121960819); // 111.88173600692367
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS014", input, expected);
	}

	protected static void Case14(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1106247680); // 30
		input.m_fThicknessM = t.F(953267991); // 0.0001
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1065353216); // 1
		input.m_Material.m_fConductivitySm = t.F(1259902592); // 10000000.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1122451372); // 115.62436125667064
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS015", input, expected);
	}

	protected static void Case15(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1117257728); // 76
		input.m_fThicknessM = t.F(953267991); // 0.0001
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1065353216); // 1
		input.m_Material.m_fConductivitySm = t.F(1259902592); // 10000000.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1124157537); // 129.28272714718287
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS016", input, expected);
	}

	protected static void Case16(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846080); // 1000
		input.m_fThicknessM = t.F(953267991); // 0.0001
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1065353216); // 1
		input.m_Material.m_fConductivitySm = t.F(1259902592); // 10000000.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1131617490); // 243.11258687208974
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS017", input, expected);
	}

	protected static void Case17(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1161527296); // 3000
		input.m_fThicknessM = t.F(953267991); // 0.0001
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1065353216); // 1
		input.m_Material.m_fConductivitySm = t.F(1259902592); // 10000000.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1136023885); // 364.69765369945594
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS018", input, expected);
	}

	protected static void Case18(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982336); // 100000
		input.m_fThicknessM = t.F(953267991); // 0.0001
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1065353216); // 1
		input.m_Material.m_fConductivitySm = t.F(1259902592); // 10000000.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1155404309); // 1776.565120103419
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS019", input, expected);
	}

	protected static void Case19(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1222359552); // 450000
		input.m_fThicknessM = t.F(953267991); // 0.0001
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1065353216); // 1
		input.m_Material.m_fConductivitySm = t.F(1259902592); // 10000000.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1164416999); // 3705.493855164155
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS020", input, expected);
	}

	protected static void Case20(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846080); // 1000
		input.m_fThicknessM = t.F(1036831949); // 0.1
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1065353216); // 1
		input.m_Material.m_fConductivitySm = t.F(1259902592); // 10000000.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1210622175); // 172675.4882879541
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS021", input, expected);
	}

	protected static void Case21(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1106247680); // 30
		input.m_fThicknessM = t.F(730643660); // 1e-12
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1065353216); // 1
		input.m_Material.m_fConductivitySm = t.F(1287568416); // 100000000.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1042679398); // 0.1621337817835488
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS022", input, expected);
	}

	protected static void Case22(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1); // 1.401298464324817e-45
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(1287568416); // 100000000.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(0); // 0.0
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS023", input, expected);
	}

	protected static void Case23(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1058820542); // 0.6106222867965698
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(1065353216); // 1
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1122456299); // 115.66194895211841
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS024", input, expected);
	}

	protected static void Case24(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1058820544); // 0.6106224060058594
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(1065353216); // 1
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1122456302); // 115.6619693008901
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS025", input, expected);
	}

	protected static void Case25(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1045220557); // 0.2
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1066829088); // 1.1759376792334777
		expected.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MS026", input, expected);
	}

	protected static void Case26(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1045220557); // 0.2
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.ASSUMED_CONFIG;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1066829088); // 1.1759376792334777
		expected.m_eProvenance = RICON_EMaterialProvenance.ASSUMED_CONFIG;
		t.CheckCase("MS027", input, expected);
	}

	protected static void Case27(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1045220557); // 0.2
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1066829088); // 1.1759376792334777
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS028", input, expected);
	}

	protected static void Case28(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1065353215); // 0.9999999403953552
		input.m_fThicknessM = t.F(953267991); // 0.0001
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OUT_OF_MODEL_DOMAIN;
		t.CheckCase("MS029", input, expected);
	}

	protected static void Case29(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1065353216); // 1
		input.m_fThicknessM = t.F(953267991); // 0.0001
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(775734988); // 4.29228182409534e-11
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS030", input, expected);
	}

	protected static void Case30(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1222359552); // 450000
		input.m_fThicknessM = t.F(953267991); // 0.0001
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1071938544); // 1.7850322862585708
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS031", input, expected);
	}

	protected static void Case31(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1222359553); // 450000.03125
		input.m_fThicknessM = t.F(953267991); // 0.0001
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OUT_OF_MODEL_DOMAIN;
		t.CheckCase("MS032", input, expected);
	}

	protected static void Case32(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403455); // 99.99999237060547
		input.m_fThicknessM = t.F(1045220557); // 0.2
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1120403456); // 100
		input.m_Material.m_fMaxMHz = t.F(1120403456); // 100
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OUT_OF_MODEL_DOMAIN;
		t.CheckCase("MS033", input, expected);
	}

	protected static void Case33(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100
		input.m_fThicknessM = t.F(1045220557); // 0.2
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1120403456); // 100
		input.m_Material.m_fMaxMHz = t.F(1120403456); // 100
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1066829088); // 1.1759376792334777
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS034", input, expected);
	}

	protected static void Case34(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403457); // 100.00000762939453
		input.m_fThicknessM = t.F(1045220557); // 0.2
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1120403456); // 100
		input.m_Material.m_fMaxMHz = t.F(1120403456); // 100
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OUT_OF_MODEL_DOMAIN;
		t.CheckCase("MS035", input, expected);
	}

	protected static void Case35(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1045220557); // 0.2
		input.m_fCosIncidence = t.F(1028443340); // 0.04999999701976776
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.NUMERIC_DOMAIN_LIMIT;
		t.CheckCase("MS036", input, expected);
	}

	protected static void Case36(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1045220557); // 0.2
		input.m_fCosIncidence = t.F(1028443341); // 0.05
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1101657713); // 21.245332907868733
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS037", input, expected);
	}

	protected static void Case37(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1045220557); // 0.2
		input.m_fCosIncidence = t.F(1028443342); // 0.05000000447034836
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1101657713); // 21.245332265480187
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS038", input, expected);
	}

	protected static void Case38(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1084227583); // 4.999999523162842
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1069604659); // 1.5068115444175494
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS039", input, expected);
	}

	protected static void Case39(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1084227584); // 5
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1069604634); // 1.5068085067132817
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS040", input, expected);
	}

	protected static void Case40(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1084227585); // 5.000000476837158
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.NUMERIC_DOMAIN_LIMIT;
		t.CheckCase("MS041", input, expected);
	}

	protected static void Case41(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1045220557); // 0.2
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1120403456); // 100
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1095642995); // 12.88658435209346
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS042", input, expected);
	}

	protected static void Case42(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1045220557); // 0.2
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1120403457); // 100.00000762939453
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.NUMERIC_DOMAIN_LIMIT;
		t.CheckCase("MS043", input, expected);
	}

	protected static void Case43(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1045220557); // 0.2
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(1287568416); // 100000000.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1219009167); // 345300.4690598885
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS044", input, expected);
	}

	protected static void Case44(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1045220557); // 0.2
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(1287568417); // 100000008.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.NUMERIC_DOMAIN_LIMIT;
		t.CheckCase("MS045", input, expected);
	}

	protected static void Case45(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(0); // 0
		input.m_fThicknessM = t.F(1045220557); // 0.2
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.INVALID_INPUT;
		t.CheckCase("MS046", input, expected);
	}

	protected static void Case46(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(-1082130432); // -1
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.INVALID_INPUT;
		t.CheckCase("MS047", input, expected);
	}

	protected static void Case47(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1045220557); // 0.2
		input.m_fCosIncidence = t.F(1066192077); // 1.1
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.INVALID_INPUT;
		t.CheckCase("MS048", input, expected);
	}

	protected static void Case48(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1045220557); // 0.2
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_MaterialSlabRun.BAD_ENUM; // 'H'
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.INVALID_INPUT;
		t.CheckCase("MS049", input, expected);
	}

	protected static void Case49(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1045220557); // 0.2
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1056964608); // 0.5
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.INVALID_PROFILE;
		t.CheckCase("MS050", input, expected);
	}

	protected static void Case50(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1045220557); // 0.2
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(-1082130432); // -1
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.INVALID_PROFILE;
		t.CheckCase("MS051", input, expected);
	}

	protected static void Case51(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1045220557); // 0.2
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1128792064); // 200
		input.m_Material.m_fMaxMHz = t.F(1120403456); // 100
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.INVALID_PROFILE;
		t.CheckCase("MS052", input, expected);
	}

	protected static void Case52(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1045220557); // 0.2
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_MaterialSlabRun.BAD_ENUM; // 'UNKNOWN'
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.INVALID_PROFILE;
		t.CheckCase("MS053", input, expected);
	}

	protected static void Case53(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1045220557); // 0.2
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material = null;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.INVALID_PROFILE;
		t.CheckCase("MS057", input, expected);
	}

	protected static void Case54(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1077936128); // 3
		input.m_fThicknessM = t.F(1126607531); // 166.6666717529297
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1069604626); // 1.5068075346460894
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS060", input, expected);
	}

	protected static void Case55(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1117257728); // 76
		input.m_fThicknessM = t.F(1087538877); // 6.5789475440979
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1069604627); // 1.5068076561545352
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS061", input, expected);
	}

	protected static void Case56(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1088421888); // 7
		input.m_fThicknessM = t.F(1116658542); // 71.42857360839844
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1069604626); // 1.5068075346460836
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS062", input, expected);
	}

	protected static void Case57(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(0); // 0
		input.m_fThicknessM = t.F(1045220557); // 0.2
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material = null;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.INVALID_INPUT;
		t.CheckCase("MS063", input, expected);
	}

	protected static void Case58(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1056964608); // 0.5
		input.m_fThicknessM = t.F(1045220557); // 0.2
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1056964608); // 0.5
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.INVALID_PROFILE;
		t.CheckCase("MS064", input, expected);
	}

	protected static void Case59(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1056964608); // 0.5
		input.m_fThicknessM = t.F(1157234688); // 2000
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OUT_OF_MODEL_DOMAIN;
		t.CheckCase("MS065", input, expected);
	}

	protected static void Case60(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1045220557); // 0.2
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1120403457); // 100.00000762939453
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1128792064); // 200
		input.m_Material.m_fMaxMHz = t.F(1133903872); // 300
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OUT_OF_MODEL_DOMAIN;
		t.CheckCase("MS066", input, expected);
	}

	protected static void Case61(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1045220557); // 0.2
		input.m_fCosIncidence = t.F(0); // 0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.NUMERIC_DOMAIN_LIMIT;
		t.CheckCase("MS067", input, expected);
	}

	protected static void Case62(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1084227584); // 5
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1120403456); // 100
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1095961293); // 13.190136521546096
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS071", input, expected);
	}

	protected static void Case63(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1106247680); // 30
		input.m_fThicknessM = t.F(781839999); // 7e-11
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1065353216); // 1
		input.m_Material.m_fConductivitySm = t.F(1287568416); // 100000000.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1089062982); // 7.305697535723499
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS072", input, expected);
	}

	protected static void Case64(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1065353216); // 1
		input.m_fThicknessM = t.F(722255052); // 5e-13
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1065353216); // 1
		input.m_Material.m_fConductivitySm = t.F(1287568416); // 100000000.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1034341564); // 0.08144519095511062
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS073", input, expected);
	}

	protected static void Case65(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1106247680); // 30
		input.m_fThicknessM = t.F(730643660); // 1e-12
		input.m_fCosIncidence = t.F(1028443341); // 0.05
		input.m_ePolarization = RICON_ESlabPolarization.TM;
		input.m_Material.m_fEpsilonReal = t.F(1065353216); // 1
		input.m_Material.m_fConductivitySm = t.F(1287568416); // 100000000.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1007026456); // 0.008178971274354132
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS074", input, expected);
	}

	protected static void Case66(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(897988541); // 1e-06
		input.m_fCosIncidence = t.F(1065353216); // 1.0
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		input.m_Material.m_fConductivitySm = t.F(1065353216); // 1
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(987135187); // 0.001636410522737617
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS075", input, expected);
	}

	protected static void Case67(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100.0
		input.m_fThicknessM = t.F(1081783850); // 3.9173684120178223
		input.m_fCosIncidence = t.F(1028443341); // 0.05
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1120403456); // 100
		input.m_Material.m_fConductivitySm = t.F(0); // 0.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1077981745); // 3.010875843391583
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS077", input, expected);
	}

	protected static void Case68(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1065353216); // 1
		input.m_fThicknessM = t.F(758115327); // 1e-11
		input.m_fCosIncidence = t.F(1028443341); // 0.05
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1065353216); // 1
		input.m_Material.m_fConductivitySm = t.F(1287568416); // 100000000.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1096356803); // 13.567324761796545
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS078", input, expected);
	}

	protected static void Case69(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1065353216); // 1
		input.m_fThicknessM = t.F(722255052); // 5e-13
		input.m_fCosIncidence = t.F(1028443341); // 0.05
		input.m_ePolarization = RICON_ESlabPolarization.TE;
		input.m_Material.m_fEpsilonReal = t.F(1065353216); // 1
		input.m_Material.m_fConductivitySm = t.F(1287568416); // 100000000.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1069542262); // 1.4993731947937448
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS079", input, expected);
	}

	protected static void Case70(RICON_MaterialSlabRun t)
	{
		RICON_MaterialSlabInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1065353216); // 1
		input.m_fThicknessM = t.F(730643660); // 1e-12
		input.m_fCosIncidence = t.F(1028443341); // 0.05
		input.m_ePolarization = RICON_ESlabPolarization.TM;
		input.m_Material.m_fEpsilonReal = t.F(1065353216); // 1
		input.m_Material.m_fConductivitySm = t.F(1287568416); // 100000000.0
		input.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		input.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		input.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult expected = new RICON_MaterialSlabResult();
		expected.m_eCode = RICON_EMaterialSlabCode.OK;
		expected.m_fLossDb = t.F(1007026456); // 0.008178971273966228
		expected.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MS080", input, expected);
	}

	static void Run(RICON_MaterialSlabRun t)
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
		Case47(t);
		Case48(t);
		Case49(t);
		Case50(t);
		Case51(t);
		Case52(t);
		Case53(t);
		Case54(t);
		Case55(t);
		Case56(t);
		Case57(t);
		Case58(t);
		Case59(t);
		Case60(t);
		Case61(t);
		Case62(t);
		Case63(t);
		Case64(t);
		Case65(t);
		Case66(t);
		Case67(t);
		Case68(t);
		Case69(t);
		Case70(t);
	}
}
