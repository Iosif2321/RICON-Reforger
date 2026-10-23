//! Сгенерировано gen_material_profile_fixture.py; ожидания только копируются; TEST_ONLY.
//! Host-only (native=false в корпусе): MP222 — Неверный JSON-тип/диапазон a, MP223 — Неверный JSON-тип/диапазон b, MP224 — Неверный JSON-тип/диапазон c, MP225 — Неверный JSON-тип/диапазон d, MP226 — Неверный JSON-тип/диапазон maxInclusive, MP227 — Неверный JSON-вход частоты, MP228 — Неверный JSON-вход частоты, MP229 — Неверный JSON-вход частоты, MP230 — Неверный JSON-вход частоты, MP231 — Лишнее поле профиля, MP232 — Отсутствующее поле профиля, MP233 — Лишнее поле входа, MP234 — Отсутствующее поле входа.
class RICON_MaterialProfileFixtures
{
	static const string SOURCE_SHA = "36f854eaa5cc2b9e1dc10d8ee6e158f4db55f2b4ae6bee7ccfcc38042135ebc2";
	static const int CASES = 223;
	static const int HOST_ONLY = 13;

	protected static void Case0(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1065353216); // 1
		input.m_Profile.m_fA = t.F(1065353216); // 1
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(0); // 0
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1065353216); // 1.0
		expected.m_Material.m_fConductivitySm = t.F(0); // 0.0
		expected.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		expected.m_Material.m_fMaxMHz = t.F(1065353216); // 1.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP001", input, expected);
	}

	protected static void Case1(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1134435623); // 316.22776601683796
		input.m_Profile.m_fA = t.F(1065353216); // 1
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(0); // 0
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1065353216); // 1.0
		expected.m_Material.m_fConductivitySm = t.F(0); // 0.0
		expected.m_Material.m_fMinMHz = t.F(1134435623); // 316.2277526855469
		expected.m_Material.m_fMaxMHz = t.F(1134435623); // 316.2277526855469
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP002", input, expected);
	}

	protected static void Case2(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fA = t.F(1065353216); // 1
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(0); // 0
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1065353216); // 1.0
		expected.m_Material.m_fConductivitySm = t.F(0); // 0.0
		expected.m_Material.m_fMinMHz = t.F(1203982336); // 100000.0
		expected.m_Material.m_fMaxMHz = t.F(1203982336); // 100000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP003", input, expected);
	}

	protected static void Case3(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1065353215); // 0.9999999403953552
		input.m_Profile.m_fA = t.F(1065353216); // 1
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(0); // 0
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP004", input, expected);
	}

	protected static void Case4(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982337); // 100000.0078125
		input.m_Profile.m_fA = t.F(1065353216); // 1
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(0); // 0
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP005", input, expected);
	}

	protected static void Case5(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fA = t.F(1084730900); // 5.24
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1027423286); // 0.0462
		input.m_Profile.m_fD = t.F(1061699138); // 0.7822
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1084730900); // 5.239999771118164
		expected.m_Material.m_fConductivitySm = t.F(1027423286); // 0.04619999974966049
		expected.m_Material.m_fMinMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_fMaxMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP006", input, expected);
	}

	protected static void Case6(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1176256512); // 10000.0
		input.m_Profile.m_fA = t.F(1084730900); // 5.24
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1027423286); // 0.0462
		input.m_Profile.m_fD = t.F(1061699138); // 0.7822
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1084730900); // 5.239999771118164
		expected.m_Material.m_fConductivitySm = t.F(1049575798); // 0.279796302318573
		expected.m_Material.m_fMinMHz = t.F(1176256512); // 10000.0
		expected.m_Material.m_fMaxMHz = t.F(1176256512); // 10000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP007", input, expected);
	}

	protected static void Case7(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fA = t.F(1084730900); // 5.24
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1027423286); // 0.0462
		input.m_Profile.m_fD = t.F(1061699138); // 0.7822
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1084730900); // 5.239999771118164
		expected.m_Material.m_fConductivitySm = t.F(1071179116); // 1.6945013999938965
		expected.m_Material.m_fMinMHz = t.F(1203982336); // 100000.0
		expected.m_Material.m_fMaxMHz = t.F(1203982336); // 100000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP008", input, expected);
	}

	protected static void Case8(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846079); // 999.9999389648438
		input.m_Profile.m_fA = t.F(1084730900); // 5.24
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1027423286); // 0.0462
		input.m_Profile.m_fD = t.F(1061699138); // 0.7822
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP009", input, expected);
	}

	protected static void Case9(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982337); // 100000.0078125
		input.m_Profile.m_fA = t.F(1084730900); // 5.24
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1027423286); // 0.0462
		input.m_Profile.m_fD = t.F(1061699138); // 0.7822
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP010", input, expected);
	}

	protected static void Case10(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fA = t.F(1084584100); // 5.17
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1013813608); // 0.0145
		input.m_Profile.m_fD = t.F(1066108191); // 1.09
		input.m_Profile.m_fMinMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fMaxMHz = t.F(1218519552); // 330000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1084584100); // 5.170000076293945
		expected.m_Material.m_fConductivitySm = t.F(1075566043); // 2.4349277019500732
		expected.m_Material.m_fMinMHz = t.F(1205262336); // 110000.0
		expected.m_Material.m_fMaxMHz = t.F(1205262336); // 110000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP011", input, expected);
	}

	protected static void Case11(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1211764582); // 190525.5888325765
		input.m_Profile.m_fA = t.F(1084584100); // 5.17
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1013813608); // 0.0145
		input.m_Profile.m_fD = t.F(1066108191); // 1.09
		input.m_Profile.m_fMinMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fMaxMHz = t.F(1218519552); // 330000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1084584100); // 5.170000076293945
		expected.m_Material.m_fConductivitySm = t.F(1083034635); // 4.43115758895874
		expected.m_Material.m_fMinMHz = t.F(1211764582); // 190525.59375
		expected.m_Material.m_fMaxMHz = t.F(1211764582); // 190525.59375
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP012", input, expected);
	}

	protected static void Case12(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1218519552); // 330000
		input.m_Profile.m_fA = t.F(1084584100); // 5.17
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1013813608); // 0.0145
		input.m_Profile.m_fD = t.F(1066108191); // 1.09
		input.m_Profile.m_fMinMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fMaxMHz = t.F(1218519552); // 330000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1084584100); // 5.170000076293945
		expected.m_Material.m_fConductivitySm = t.F(1090586105); // 8.063958168029785
		expected.m_Material.m_fMinMHz = t.F(1218519552); // 330000.0
		expected.m_Material.m_fMaxMHz = t.F(1218519552); // 330000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP013", input, expected);
	}

	protected static void Case13(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1205262335); // 109999.9921875
		input.m_Profile.m_fA = t.F(1084584100); // 5.17
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1013813608); // 0.0145
		input.m_Profile.m_fD = t.F(1066108191); // 1.09
		input.m_Profile.m_fMinMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fMaxMHz = t.F(1218519552); // 330000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP014", input, expected);
	}

	protected static void Case14(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1218519553); // 330000.03125
		input.m_Profile.m_fA = t.F(1084584100); // 5.17
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1013813608); // 0.0145
		input.m_Profile.m_fD = t.F(1066108191); // 1.09
		input.m_Profile.m_fMinMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fMaxMHz = t.F(1218519552); // 330000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP015", input, expected);
	}

	protected static void Case15(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fA = t.F(1081752945); // 3.91
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1019410488); // 0.0238
		input.m_Profile.m_fD = t.F(1042536202); // 0.16
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1193033728); // 40000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1081752945); // 3.9100000858306885
		expected.m_Material.m_fConductivitySm = t.F(1019410488); // 0.023800000548362732
		expected.m_Material.m_fMinMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_fMaxMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP016", input, expected);
	}

	protected static void Case16(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1170580593); // 6324.555320336759
		input.m_Profile.m_fA = t.F(1081752945); // 3.91
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1019410488); // 0.0238
		input.m_Profile.m_fD = t.F(1042536202); // 0.16
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1193033728); // 40000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1081752945); // 3.9100000858306885
		expected.m_Material.m_fConductivitySm = t.F(1023603432); // 0.031969934701919556
		expected.m_Material.m_fMinMHz = t.F(1170580593); // 6324.55517578125
		expected.m_Material.m_fMaxMHz = t.F(1170580593); // 6324.55517578125
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP017", input, expected);
	}

	protected static void Case17(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1193033728); // 40000
		input.m_Profile.m_fA = t.F(1081752945); // 3.91
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1019410488); // 0.0238
		input.m_Profile.m_fD = t.F(1042536202); // 0.16
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1193033728); // 40000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1081752945); // 3.9100000858306885
		expected.m_Material.m_fConductivitySm = t.F(1026549368); // 0.04294440150260925
		expected.m_Material.m_fMinMHz = t.F(1193033728); // 40000.0
		expected.m_Material.m_fMaxMHz = t.F(1193033728); // 40000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP018", input, expected);
	}

	protected static void Case18(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846079); // 999.9999389648438
		input.m_Profile.m_fA = t.F(1081752945); // 3.91
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1019410488); // 0.0238
		input.m_Profile.m_fD = t.F(1042536202); // 0.16
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1193033728); // 40000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP019", input, expected);
	}

	protected static void Case19(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1193033729); // 40000.00390625
		input.m_Profile.m_fA = t.F(1081752945); // 3.91
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1019410488); // 0.0238
		input.m_Profile.m_fD = t.F(1042536202); // 0.16
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1193033728); // 40000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP020", input, expected);
	}

	protected static void Case20(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fA = t.F(1082445005); // 4.15
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(974997842); // 0.0006
		input.m_Profile.m_fD = t.F(1070144789); // 1.5712
		input.m_Profile.m_fMinMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fMaxMHz = t.F(1218519552); // 330000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1082445005); // 4.150000095367432
		expected.m_Material.m_fConductivitySm = t.F(1064805584); // 0.9673585891723633
		expected.m_Material.m_fMinMHz = t.F(1205262336); // 110000.0
		expected.m_Material.m_fMaxMHz = t.F(1205262336); // 110000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP021", input, expected);
	}

	protected static void Case21(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1211764582); // 190525.5888325765
		input.m_Profile.m_fA = t.F(1082445005); // 4.15
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(974997842); // 0.0006
		input.m_Profile.m_fD = t.F(1070144789); // 1.5712
		input.m_Profile.m_fMinMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fMaxMHz = t.F(1218519552); // 330000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1082445005); // 4.150000095367432
		expected.m_Material.m_fConductivitySm = t.F(1074970975); // 2.2930524349212646
		expected.m_Material.m_fMinMHz = t.F(1211764582); // 190525.59375
		expected.m_Material.m_fMaxMHz = t.F(1211764582); // 190525.59375
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP022", input, expected);
	}

	protected static void Case22(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1218519552); // 330000
		input.m_Profile.m_fA = t.F(1082445005); // 4.15
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(974997842); // 0.0006
		input.m_Profile.m_fD = t.F(1070144789); // 1.5712
		input.m_Profile.m_fMinMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fMaxMHz = t.F(1218519552); // 330000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1082445005); // 4.150000095367432
		expected.m_Material.m_fConductivitySm = t.F(1085140917); // 5.435511112213135
		expected.m_Material.m_fMinMHz = t.F(1218519552); // 330000.0
		expected.m_Material.m_fMaxMHz = t.F(1218519552); // 330000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP023", input, expected);
	}

	protected static void Case23(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1205262335); // 109999.9921875
		input.m_Profile.m_fA = t.F(1082445005); // 4.15
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(974997842); // 0.0006
		input.m_Profile.m_fD = t.F(1070144789); // 1.5712
		input.m_Profile.m_fMinMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fMaxMHz = t.F(1218519552); // 330000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP024", input, expected);
	}

	protected static void Case24(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1218519553); // 330000.03125
		input.m_Profile.m_fA = t.F(1082445005); // 4.15
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(974997842); // 0.0006
		input.m_Profile.m_fD = t.F(1070144789); // 1.5712
		input.m_Profile.m_fMinMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fMaxMHz = t.F(1218519552); // 330000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP025", input, expected);
	}

	protected static void Case25(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fA = t.F(1076803666); // 2.73
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1007371158); // 0.0085
		input.m_Profile.m_fD = t.F(1064338194); // 0.9395
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1076803666); // 2.7300000190734863
		expected.m_Material.m_fConductivitySm = t.F(1007371158); // 0.008500000461935997
		expected.m_Material.m_fMinMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_fMaxMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP026", input, expected);
	}

	protected static void Case26(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1176256512); // 10000.0
		input.m_Profile.m_fA = t.F(1076803666); // 2.73
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1007371158); // 0.0085
		input.m_Profile.m_fD = t.F(1064338194); // 0.9395
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1076803666); // 2.7300000190734863
		expected.m_Material.m_fConductivitySm = t.F(1033335137); // 0.07394672185182571
		expected.m_Material.m_fMinMHz = t.F(1176256512); // 10000.0
		expected.m_Material.m_fMaxMHz = t.F(1176256512); // 10000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP027", input, expected);
	}

	protected static void Case27(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fA = t.F(1076803666); // 2.73
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1007371158); // 0.0085
		input.m_Profile.m_fD = t.F(1064338194); // 0.9395
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1076803666); // 2.7300000190734863
		expected.m_Material.m_fConductivitySm = t.F(1059368916); // 0.6433079242706299
		expected.m_Material.m_fMinMHz = t.F(1203982336); // 100000.0
		expected.m_Material.m_fMaxMHz = t.F(1203982336); // 100000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP028", input, expected);
	}

	protected static void Case28(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846079); // 999.9999389648438
		input.m_Profile.m_fA = t.F(1076803666); // 2.73
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1007371158); // 0.0085
		input.m_Profile.m_fD = t.F(1064338194); // 0.9395
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP029", input, expected);
	}

	protected static void Case29(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982337); // 100000.0078125
		input.m_Profile.m_fA = t.F(1076803666); // 2.73
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1007371158); // 0.0085
		input.m_Profile.m_fD = t.F(1064338194); // 0.9395
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP030", input, expected);
	}

	protected static void Case30(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fA = t.F(1076090634); // 2.56
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(953267991); // 0.0001
		input.m_Profile.m_fD = t.F(1071895491); // 1.7799
		input.m_Profile.m_fMinMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fMaxMHz = t.F(1218519552); // 330000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1076090634); // 2.559999942779541
		expected.m_Material.m_fConductivitySm = t.F(1054616006); // 0.430006206035614
		expected.m_Material.m_fMinMHz = t.F(1205262336); // 110000.0
		expected.m_Material.m_fMaxMHz = t.F(1205262336); // 110000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP031", input, expected);
	}

	protected static void Case31(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1211764582); // 190525.5888325765
		input.m_Profile.m_fA = t.F(1076090634); // 2.56
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(953267991); // 0.0001
		input.m_Profile.m_fD = t.F(1071895491); // 1.7799
		input.m_Profile.m_fMinMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fMaxMHz = t.F(1218519552); // 330000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1076090634); // 2.559999942779541
		expected.m_Material.m_fConductivitySm = t.F(1066553727); // 1.143112063407898
		expected.m_Material.m_fMinMHz = t.F(1211764582); // 190525.59375
		expected.m_Material.m_fMaxMHz = t.F(1211764582); // 190525.59375
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP032", input, expected);
	}

	protected static void Case32(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1218519552); // 330000
		input.m_Profile.m_fA = t.F(1076090634); // 2.56
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(953267991); // 0.0001
		input.m_Profile.m_fD = t.F(1071895491); // 1.7799
		input.m_Profile.m_fMinMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fMaxMHz = t.F(1218519552); // 330000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1076090634); // 2.559999942779541
		expected.m_Material.m_fConductivitySm = t.F(1078098888); // 3.0388050079345703
		expected.m_Material.m_fMinMHz = t.F(1218519552); // 330000.0
		expected.m_Material.m_fMaxMHz = t.F(1218519552); // 330000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP033", input, expected);
	}

	protected static void Case33(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1205262335); // 109999.9921875
		input.m_Profile.m_fA = t.F(1076090634); // 2.56
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(953267991); // 0.0001
		input.m_Profile.m_fD = t.F(1071895491); // 1.7799
		input.m_Profile.m_fMinMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fMaxMHz = t.F(1218519552); // 330000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP034", input, expected);
	}

	protected static void Case34(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1218519553); // 330000.03125
		input.m_Profile.m_fA = t.F(1076090634); // 2.56
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(953267991); // 0.0001
		input.m_Profile.m_fD = t.F(1071895491); // 1.7799
		input.m_Profile.m_fMinMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fMaxMHz = t.F(1218519552); // 330000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP035", input, expected);
	}

	protected static void Case35(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fA = t.F(1076468122); // 2.65
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(961656599); // 0.0002
		input.m_Profile.m_fD = t.F(1070369604); // 1.598
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1076468122); // 2.6500000953674316
		expected.m_Material.m_fConductivitySm = t.F(1050725921); // 0.3140726387500763
		expected.m_Material.m_fMinMHz = t.F(1203982336); // 100000.0
		expected.m_Material.m_fMaxMHz = t.F(1203982336); // 100000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP036", input, expected);
	}

	protected static void Case36(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1212370944); // 200000.0
		input.m_Profile.m_fA = t.F(1076468122); // 2.65
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(961656599); // 0.0002
		input.m_Profile.m_fD = t.F(1070369604); // 1.598
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1076468122); // 2.6500000953674316
		expected.m_Material.m_fConductivitySm = t.F(1064527294); // 0.9507712125778198
		expected.m_Material.m_fMinMHz = t.F(1212370944); // 200000.0
		expected.m_Material.m_fMaxMHz = t.F(1212370944); // 200000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP037", input, expected);
	}

	protected static void Case37(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1220759552); // 400000
		input.m_Profile.m_fA = t.F(1076468122); // 2.65
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(961656599); // 0.0002
		input.m_Profile.m_fD = t.F(1070369604); // 1.598
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1076468122); // 2.6500000953674316
		expected.m_Material.m_fConductivitySm = t.F(1077425290); // 2.878206729888916
		expected.m_Material.m_fMinMHz = t.F(1220759552); // 400000.0
		expected.m_Material.m_fMaxMHz = t.F(1220759552); // 400000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP038", input, expected);
	}

	protected static void Case38(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982335); // 99999.9921875
		input.m_Profile.m_fA = t.F(1076468122); // 2.65
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(961656599); // 0.0002
		input.m_Profile.m_fD = t.F(1070369604); // 1.598
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP039", input, expected);
	}

	protected static void Case39(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1220759553); // 400000.03125
		input.m_Profile.m_fA = t.F(1076468122); // 2.65
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(961656599); // 0.0002
		input.m_Profile.m_fD = t.F(1070369604); // 1.598
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP040", input, expected);
	}

	protected static void Case40(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1065353216); // 1
		input.m_Profile.m_fA = t.F(1073657938); // 1.99
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(999948917); // 0.0047
		input.m_Profile.m_fD = t.F(1065955518); // 1.0718
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1073657938); // 1.9900000095367432
		expected.m_Material.m_fConductivitySm = t.F(910169084); // 2.8621861929423176e-06
		expected.m_Material.m_fMinMHz = t.F(1065353216); // 1.0
		expected.m_Material.m_fMaxMHz = t.F(1065353216); // 1.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP041", input, expected);
	}

	protected static void Case41(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1134435623); // 316.22776601683796
		input.m_Profile.m_fA = t.F(1073657938); // 1.99
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(999948917); // 0.0047
		input.m_Profile.m_fD = t.F(1065955518); // 1.0718
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1073657938); // 1.9900000095367432
		expected.m_Material.m_fConductivitySm = t.F(984832582); // 0.0013683519791811705
		expected.m_Material.m_fMinMHz = t.F(1134435623); // 316.2277526855469
		expected.m_Material.m_fMaxMHz = t.F(1134435623); // 316.2277526855469
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP042", input, expected);
	}

	protected static void Case42(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fA = t.F(1073657938); // 1.99
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(999948917); // 0.0047
		input.m_Profile.m_fD = t.F(1065955518); // 1.0718
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1073657938); // 1.9900000095367432
		expected.m_Material.m_fConductivitySm = t.F(1059551334); // 0.6541808843612671
		expected.m_Material.m_fMinMHz = t.F(1203982336); // 100000.0
		expected.m_Material.m_fMaxMHz = t.F(1203982336); // 100000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP043", input, expected);
	}

	protected static void Case43(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1065353215); // 0.9999999403953552
		input.m_Profile.m_fA = t.F(1073657938); // 1.99
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(999948917); // 0.0047
		input.m_Profile.m_fD = t.F(1065955518); // 1.0718
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP044", input, expected);
	}

	protected static void Case44(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982337); // 100000.0078125
		input.m_Profile.m_fA = t.F(1073657938); // 1.99
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(999948917); // 0.0047
		input.m_Profile.m_fD = t.F(1065955518); // 1.0718
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP045", input, expected);
	}

	protected static void Case45(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fA = t.F(1072231875); // 1.82
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(998445679); // 0.004
		input.m_Profile.m_fD = t.F(1065991589); // 1.0761
		input.m_Profile.m_fMinMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fMaxMHz = t.F(1218519552); // 330000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1072231875); // 1.8200000524520874
		expected.m_Material.m_fConductivitySm = t.F(1059132563); // 0.629220187664032
		expected.m_Material.m_fMinMHz = t.F(1205262336); // 110000.0
		expected.m_Material.m_fMaxMHz = t.F(1205262336); // 110000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP046", input, expected);
	}

	protected static void Case46(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1211764582); // 190525.5888325765
		input.m_Profile.m_fA = t.F(1072231875); // 1.82
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(998445679); // 0.004
		input.m_Profile.m_fD = t.F(1065991589); // 1.0761
		input.m_Profile.m_fMinMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fMaxMHz = t.F(1218519552); // 330000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1072231875); // 1.8200000524520874
		expected.m_Material.m_fConductivitySm = t.F(1066497127); // 1.1363648176193237
		expected.m_Material.m_fMinMHz = t.F(1211764582); // 190525.59375
		expected.m_Material.m_fMaxMHz = t.F(1211764582); // 190525.59375
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP047", input, expected);
	}

	protected static void Case47(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1218519552); // 330000
		input.m_Profile.m_fA = t.F(1072231875); // 1.82
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(998445679); // 0.004
		input.m_Profile.m_fD = t.F(1065991589); // 1.0761
		input.m_Profile.m_fMinMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fMaxMHz = t.F(1218519552); // 330000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1072231875); // 1.8200000524520874
		expected.m_Material.m_fConductivitySm = t.F(1073961027); // 2.0522620677948
		expected.m_Material.m_fMinMHz = t.F(1218519552); // 330000.0
		expected.m_Material.m_fMaxMHz = t.F(1218519552); // 330000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP048", input, expected);
	}

	protected static void Case48(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1205262335); // 109999.9921875
		input.m_Profile.m_fA = t.F(1072231875); // 1.82
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(998445679); // 0.004
		input.m_Profile.m_fD = t.F(1065991589); // 1.0761
		input.m_Profile.m_fMinMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fMaxMHz = t.F(1218519552); // 330000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP049", input, expected);
	}

	protected static void Case49(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1218519553); // 330000.03125
		input.m_Profile.m_fA = t.F(1072231875); // 1.82
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(998445679); // 0.004
		input.m_Profile.m_fD = t.F(1065991589); // 1.0761
		input.m_Profile.m_fMinMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fMaxMHz = t.F(1218519552); // 330000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP050", input, expected);
	}

	protected static void Case50(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fA = t.F(1074238010); // 2.1183
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1001666904); // 0.0055
		input.m_Profile.m_fD = t.F(1066286868); // 1.1113
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1074238010); // 2.118299961090088
		expected.m_Material.m_fConductivitySm = t.F(1063981740); // 0.9182536602020264
		expected.m_Material.m_fMinMHz = t.F(1203982336); // 100000.0
		expected.m_Material.m_fMaxMHz = t.F(1203982336); // 100000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP051", input, expected);
	}

	protected static void Case51(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1212370944); // 200000.0
		input.m_Profile.m_fA = t.F(1074238010); // 2.1183
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1001666904); // 0.0055
		input.m_Profile.m_fD = t.F(1066286868); // 1.1113
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1074238010); // 2.118299961090088
		expected.m_Material.m_fConductivitySm = t.F(1073605906); // 1.983797311782837
		expected.m_Material.m_fMinMHz = t.F(1212370944); // 200000.0
		expected.m_Material.m_fMaxMHz = t.F(1212370944); // 200000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP052", input, expected);
	}

	protected static void Case52(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1220759552); // 400000
		input.m_Profile.m_fA = t.F(1074238010); // 2.1183
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1001666904); // 0.0055
		input.m_Profile.m_fD = t.F(1066286868); // 1.1113
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1074238010); // 2.118299961090088
		expected.m_Material.m_fConductivitySm = t.F(1082729799); // 4.285800457000732
		expected.m_Material.m_fMinMHz = t.F(1220759552); // 400000.0
		expected.m_Material.m_fMaxMHz = t.F(1220759552); // 400000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP053", input, expected);
	}

	protected static void Case53(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982335); // 99999.9921875
		input.m_Profile.m_fA = t.F(1074238010); // 2.1183
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1001666904); // 0.0055
		input.m_Profile.m_fD = t.F(1066286868); // 1.1113
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP054", input, expected);
	}

	protected static void Case54(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1220759553); // 400000.03125
		input.m_Profile.m_fA = t.F(1074238010); // 2.1183
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1001666904); // 0.0055
		input.m_Profile.m_fD = t.F(1066286868); // 1.1113
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP055", input, expected);
	}

	protected static void Case55(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100
		input.m_Profile.m_fA = t.F(1086974853); // 6.31
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(996929018); // 0.0036
		input.m_Profile.m_fD = t.F(1068200310); // 1.3394
		input.m_Profile.m_fMinMHz = t.F(1120403456); // 100
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1086974853); // 6.309999942779541
		expected.m_Material.m_fConductivitySm = t.F(959236246); // 0.00016477922326885164
		expected.m_Material.m_fMinMHz = t.F(1120403456); // 100.0
		expected.m_Material.m_fMaxMHz = t.F(1120403456); // 100.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP056", input, expected);
	}

	protected static void Case56(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1162191985); // 3162.2776601683795
		input.m_Profile.m_fA = t.F(1086974853); // 6.31
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(996929018); // 0.0036
		input.m_Profile.m_fD = t.F(1068200310); // 1.3394
		input.m_Profile.m_fMinMHz = t.F(1120403456); // 100
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1086974853); // 6.309999942779541
		expected.m_Material.m_fConductivitySm = t.F(1015666800); // 0.01682683825492859
		expected.m_Material.m_fMinMHz = t.F(1162191985); // 3162.277587890625
		expected.m_Material.m_fMaxMHz = t.F(1162191985); // 3162.277587890625
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP057", input, expected);
	}

	protected static void Case57(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fA = t.F(1086974853); // 6.31
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(996929018); // 0.0036
		input.m_Profile.m_fD = t.F(1068200310); // 1.3394
		input.m_Profile.m_fMinMHz = t.F(1120403456); // 100
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1086974853); // 6.309999942779541
		expected.m_Material.m_fConductivitySm = t.F(1071378873); // 1.718314290046692
		expected.m_Material.m_fMinMHz = t.F(1203982336); // 100000.0
		expected.m_Material.m_fMaxMHz = t.F(1203982336); // 100000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP058", input, expected);
	}

	protected static void Case58(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403455); // 99.99999237060547
		input.m_Profile.m_fA = t.F(1086974853); // 6.31
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(996929018); // 0.0036
		input.m_Profile.m_fD = t.F(1068200310); // 1.3394
		input.m_Profile.m_fMinMHz = t.F(1120403456); // 100
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP059", input, expected);
	}

	protected static void Case59(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982337); // 100000.0078125
		input.m_Profile.m_fA = t.F(1086974853); // 6.31
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(996929018); // 0.0036
		input.m_Profile.m_fD = t.F(1068200310); // 1.3394
		input.m_Profile.m_fMinMHz = t.F(1120403456); // 100
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP060", input, expected);
	}

	protected static void Case60(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1213650944); // 220000
		input.m_Profile.m_fA = t.F(1085884334); // 5.79
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(970045207); // 0.0004
		input.m_Profile.m_fD = t.F(1070872920); // 1.658
		input.m_Profile.m_fMinMHz = t.F(1213650944); // 220000
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1085884334); // 5.789999961853027
		expected.m_Material.m_fConductivitySm = t.F(1078190013); // 3.0605309009552
		expected.m_Material.m_fMinMHz = t.F(1213650944); // 220000.0
		expected.m_Material.m_fMaxMHz = t.F(1213650944); // 220000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP061", input, expected);
	}

	protected static void Case61(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1218028117); // 314642.65445104544
		input.m_Profile.m_fA = t.F(1085884334); // 5.79
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(970045207); // 0.0004
		input.m_Profile.m_fD = t.F(1070872920); // 1.658
		input.m_Profile.m_fMinMHz = t.F(1213650944); // 220000
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1085884334); // 5.789999961853027
		expected.m_Material.m_fConductivitySm = t.F(1085358221); // 5.539129734039307
		expected.m_Material.m_fMinMHz = t.F(1218028117); // 314642.65625
		expected.m_Material.m_fMaxMHz = t.F(1218028117); // 314642.65625
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP062", input, expected);
	}

	protected static void Case62(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1222359552); // 450000
		input.m_Profile.m_fA = t.F(1085884334); // 5.79
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(970045207); // 0.0004
		input.m_Profile.m_fD = t.F(1070872920); // 1.658
		input.m_Profile.m_fMinMHz = t.F(1213650944); // 220000
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1085884334); // 5.789999961853027
		expected.m_Material.m_fConductivitySm = t.F(1092642453); // 10.025044441223145
		expected.m_Material.m_fMinMHz = t.F(1222359552); // 450000.0
		expected.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP063", input, expected);
	}

	protected static void Case63(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1213650943); // 219999.984375
		input.m_Profile.m_fA = t.F(1085884334); // 5.79
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(970045207); // 0.0004
		input.m_Profile.m_fD = t.F(1070872920); // 1.658
		input.m_Profile.m_fMinMHz = t.F(1213650944); // 220000
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP064", input, expected);
	}

	protected static void Case64(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1222359553); // 450000.03125
		input.m_Profile.m_fA = t.F(1085884334); // 5.79
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(970045207); // 0.0004
		input.m_Profile.m_fD = t.F(1070872920); // 1.658
		input.m_Profile.m_fMinMHz = t.F(1213650944); // 220000
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP065", input, expected);
	}

	protected static void Case65(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fA = t.F(1087534164); // 6.5767
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(983386450); // 0.0012
		input.m_Profile.m_fD = t.F(1069293345); // 1.4697
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1087534164); // 6.576700210571289
		expected.m_Material.m_fConductivitySm = t.F(1065719910); // 1.0437133312225342
		expected.m_Material.m_fMinMHz = t.F(1203982336); // 100000.0
		expected.m_Material.m_fMaxMHz = t.F(1203982336); // 100000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP066", input, expected);
	}

	protected static void Case66(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1212370944); // 200000.0
		input.m_Profile.m_fA = t.F(1087534164); // 6.5767
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(983386450); // 0.0012
		input.m_Profile.m_fD = t.F(1069293345); // 1.4697
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1087534164); // 6.576700210571289
		expected.m_Material.m_fConductivitySm = t.F(1077477746); // 2.8907132148742676
		expected.m_Material.m_fMinMHz = t.F(1212370944); // 200000.0
		expected.m_Material.m_fMaxMHz = t.F(1212370944); // 200000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP067", input, expected);
	}

	protected static void Case67(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1220759552); // 400000
		input.m_Profile.m_fA = t.F(1087534164); // 6.5767
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(983386450); // 0.0012
		input.m_Profile.m_fD = t.F(1069293345); // 1.4697
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1087534164); // 6.576700210571289
		expected.m_Material.m_fConductivitySm = t.F(1090525587); // 8.006243705749512
		expected.m_Material.m_fMinMHz = t.F(1220759552); // 400000.0
		expected.m_Material.m_fMaxMHz = t.F(1220759552); // 400000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP068", input, expected);
	}

	protected static void Case68(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982335); // 99999.9921875
		input.m_Profile.m_fA = t.F(1087534164); // 6.5767
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(983386450); // 0.0012
		input.m_Profile.m_fD = t.F(1069293345); // 1.4697
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP069", input, expected);
	}

	protected static void Case69(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1220759553); // 400000.03125
		input.m_Profile.m_fA = t.F(1087534164); // 6.5767
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(983386450); // 0.0012
		input.m_Profile.m_fD = t.F(1069293345); // 1.4697
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP070", input, expected);
	}

	protected static void Case70(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fA = t.F(1076174520); // 2.58
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(953267991); // 0.0001
		input.m_Profile.m_fD = t.F(1070825944); // 1.6524
		input.m_Profile.m_fMinMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fMaxMHz = t.F(1218519552); // 330000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1076174520); // 2.5799999237060547
		expected.m_Material.m_fConductivitySm = t.F(1047646883); // 0.23615507781505585
		expected.m_Material.m_fMinMHz = t.F(1205262336); // 110000.0
		expected.m_Material.m_fMaxMHz = t.F(1205262336); // 110000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP071", input, expected);
	}

	protected static void Case71(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1211764582); // 190525.5888325765
		input.m_Profile.m_fA = t.F(1076174520); // 2.58
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(953267991); // 0.0001
		input.m_Profile.m_fD = t.F(1070825944); // 1.6524
		input.m_Profile.m_fMinMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fMaxMHz = t.F(1218519552); // 330000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1076174520); // 2.5799999237060547
		expected.m_Material.m_fConductivitySm = t.F(1058396075); // 0.5853220820426941
		expected.m_Material.m_fMinMHz = t.F(1211764582); // 190525.59375
		expected.m_Material.m_fMaxMHz = t.F(1211764582); // 190525.59375
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP072", input, expected);
	}

	protected static void Case72(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1218519552); // 330000
		input.m_Profile.m_fA = t.F(1076174520); // 2.58
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(953267991); // 0.0001
		input.m_Profile.m_fD = t.F(1070825944); // 1.6524
		input.m_Profile.m_fMinMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fMaxMHz = t.F(1218519552); // 330000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1076174520); // 2.5799999237060547
		expected.m_Material.m_fConductivitySm = t.F(1069134378); // 1.4507496356964111
		expected.m_Material.m_fMinMHz = t.F(1218519552); // 330000.0
		expected.m_Material.m_fMaxMHz = t.F(1218519552); // 330000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP073", input, expected);
	}

	protected static void Case73(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1205262335); // 109999.9921875
		input.m_Profile.m_fA = t.F(1076174520); // 2.58
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(953267991); // 0.0001
		input.m_Profile.m_fD = t.F(1070825944); // 1.6524
		input.m_Profile.m_fMinMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fMaxMHz = t.F(1218519552); // 330000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP074", input, expected);
	}

	protected static void Case74(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1218519553); // 330000.03125
		input.m_Profile.m_fA = t.F(1076174520); // 2.58
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(953267991); // 0.0001
		input.m_Profile.m_fD = t.F(1070825944); // 1.6524
		input.m_Profile.m_fMinMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fMaxMHz = t.F(1218519552); // 330000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP075", input, expected);
	}

	protected static void Case75(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fA = t.F(1069379748); // 1.48
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(982527456); // 0.0011
		input.m_Profile.m_fD = t.F(1065982362); // 1.075
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1069379748); // 1.4800000190734863
		expected.m_Material.m_fConductivitySm = t.F(982527456); // 0.0010999999940395355
		expected.m_Material.m_fMinMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_fMaxMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP076", input, expected);
	}

	protected static void Case76(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1176256512); // 10000.0
		input.m_Profile.m_fA = t.F(1069379748); // 1.48
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(982527456); // 0.0011
		input.m_Profile.m_fD = t.F(1065982362); // 1.075
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1069379748); // 1.4800000190734863
		expected.m_Material.m_fConductivitySm = t.F(1012281944); // 0.013073526322841644
		expected.m_Material.m_fMinMHz = t.F(1176256512); // 10000.0
		expected.m_Material.m_fMaxMHz = t.F(1176256512); // 10000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP077", input, expected);
	}

	protected static void Case77(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fA = t.F(1069379748); // 1.48
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(982527456); // 0.0011
		input.m_Profile.m_fD = t.F(1065982362); // 1.075
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1069379748); // 1.4800000190734863
		expected.m_Material.m_fConductivitySm = t.F(1042226103); // 0.15537916123867035
		expected.m_Material.m_fMinMHz = t.F(1203982336); // 100000.0
		expected.m_Material.m_fMaxMHz = t.F(1203982336); // 100000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP078", input, expected);
	}

	protected static void Case78(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846079); // 999.9999389648438
		input.m_Profile.m_fA = t.F(1069379748); // 1.48
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(982527456); // 0.0011
		input.m_Profile.m_fD = t.F(1065982362); // 1.075
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP079", input, expected);
	}

	protected static void Case79(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982337); // 100000.0078125
		input.m_Profile.m_fA = t.F(1069379748); // 1.48
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(982527456); // 0.0011
		input.m_Profile.m_fD = t.F(1065982362); // 1.075
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP080", input, expected);
	}

	protected static void Case80(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1213650944); // 220000
		input.m_Profile.m_fA = t.F(1069715292); // 1.52
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(993922541); // 0.0029
		input.m_Profile.m_fD = t.F(1065596486); // 1.029
		input.m_Profile.m_fMinMHz = t.F(1213650944); // 220000
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1069715292); // 1.5199999809265137
		expected.m_Material.m_fConductivitySm = t.F(1061092155); // 0.7460209727287292
		expected.m_Material.m_fMinMHz = t.F(1213650944); // 220000.0
		expected.m_Material.m_fMaxMHz = t.F(1213650944); // 220000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP081", input, expected);
	}

	protected static void Case81(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1218028117); // 314642.65445104544
		input.m_Profile.m_fA = t.F(1069715292); // 1.52
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(993922541); // 0.0029
		input.m_Profile.m_fD = t.F(1065596486); // 1.029
		input.m_Profile.m_fMinMHz = t.F(1213650944); // 220000
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1069715292); // 1.5199999809265137
		expected.m_Material.m_fConductivitySm = t.F(1066008228); // 1.0780835151672363
		expected.m_Material.m_fMinMHz = t.F(1218028117); // 314642.65625
		expected.m_Material.m_fMaxMHz = t.F(1218028117); // 314642.65625
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP082", input, expected);
	}

	protected static void Case82(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1222359552); // 450000
		input.m_Profile.m_fA = t.F(1069715292); // 1.52
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(993922541); // 0.0029
		input.m_Profile.m_fD = t.F(1065596486); // 1.029
		input.m_Profile.m_fMinMHz = t.F(1213650944); // 220000
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1069715292); // 1.5199999809265137
		expected.m_Material.m_fConductivitySm = t.F(1070033648); // 1.5579509735107422
		expected.m_Material.m_fMinMHz = t.F(1222359552); // 450000.0
		expected.m_Material.m_fMaxMHz = t.F(1222359552); // 450000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP083", input, expected);
	}

	protected static void Case83(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1213650943); // 219999.984375
		input.m_Profile.m_fA = t.F(1069715292); // 1.52
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(993922541); // 0.0029
		input.m_Profile.m_fD = t.F(1065596486); // 1.029
		input.m_Profile.m_fMinMHz = t.F(1213650944); // 220000
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP084", input, expected);
	}

	protected static void Case84(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1222359553); // 450000.03125
		input.m_Profile.m_fA = t.F(1069715292); // 1.52
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(993922541); // 0.0029
		input.m_Profile.m_fD = t.F(1065596486); // 1.029
		input.m_Profile.m_fMinMHz = t.F(1213650944); // 220000
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP085", input, expected);
	}

	protected static void Case85(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fA = t.F(1067506572); // 1.2567
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(956846236); // 0.00013
		input.m_Profile.m_fD = t.F(1069161644); // 1.454
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1067506572); // 1.256700038909912
		expected.m_Material.m_fConductivitySm = t.F(1037527528); // 0.1051824688911438
		expected.m_Material.m_fMinMHz = t.F(1203982336); // 100000.0
		expected.m_Material.m_fMaxMHz = t.F(1203982336); // 100000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP086", input, expected);
	}

	protected static void Case86(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1212370944); // 200000.0
		input.m_Profile.m_fA = t.F(1067506572); // 1.2567
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(956846236); // 0.00013
		input.m_Profile.m_fD = t.F(1069161644); // 1.454
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1067506572); // 1.256700038909912
		expected.m_Material.m_fConductivitySm = t.F(1049856599); // 0.28816482424736023
		expected.m_Material.m_fMinMHz = t.F(1212370944); // 200000.0
		expected.m_Material.m_fMaxMHz = t.F(1212370944); // 200000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP087", input, expected);
	}

	protected static void Case87(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1220759552); // 400000
		input.m_Profile.m_fA = t.F(1067506572); // 1.2567
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(956846236); // 0.00013
		input.m_Profile.m_fD = t.F(1069161644); // 1.454
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1067506572); // 1.256700038909912
		expected.m_Material.m_fConductivitySm = t.F(1061821198); // 0.7894753217697144
		expected.m_Material.m_fMinMHz = t.F(1220759552); // 400000.0
		expected.m_Material.m_fMaxMHz = t.F(1220759552); // 400000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP088", input, expected);
	}

	protected static void Case88(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982335); // 99999.9921875
		input.m_Profile.m_fA = t.F(1067506572); // 1.2567
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(956846236); // 0.00013
		input.m_Profile.m_fD = t.F(1069161644); // 1.454
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP089", input, expected);
	}

	protected static void Case89(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1220759553); // 400000.03125
		input.m_Profile.m_fA = t.F(1067506572); // 1.2567
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(956846236); // 0.00013
		input.m_Profile.m_fD = t.F(1069161644); // 1.454
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP090", input, expected);
	}

	protected static void Case90(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fA = t.F(1076174520); // 2.58
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1018283059); // 0.0217
		input.m_Profile.m_fD = t.F(1061662228); // 0.78
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1076174520); // 2.5799999237060547
		expected.m_Material.m_fConductivitySm = t.F(1018283059); // 0.021700000390410423
		expected.m_Material.m_fMinMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_fMaxMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP091", input, expected);
	}

	protected static void Case91(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1176256512); // 10000.0
		input.m_Profile.m_fA = t.F(1076174520); // 2.58
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1018283059); // 0.0217
		input.m_Profile.m_fD = t.F(1061662228); // 0.78
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1076174520); // 2.5799999237060547
		expected.m_Material.m_fConductivitySm = t.F(1040573632); // 0.13075542449951172
		expected.m_Material.m_fMinMHz = t.F(1176256512); // 10000.0
		expected.m_Material.m_fMaxMHz = t.F(1176256512); // 10000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP092", input, expected);
	}

	protected static void Case92(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fA = t.F(1076174520); // 2.58
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1018283059); // 0.0217
		input.m_Profile.m_fD = t.F(1061662228); // 0.78
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1076174520); // 2.5799999237060547
		expected.m_Material.m_fConductivitySm = t.F(1061794421); // 0.7878792881965637
		expected.m_Material.m_fMinMHz = t.F(1203982336); // 100000.0
		expected.m_Material.m_fMaxMHz = t.F(1203982336); // 100000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP093", input, expected);
	}

	protected static void Case93(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846079); // 999.9999389648438
		input.m_Profile.m_fA = t.F(1076174520); // 2.58
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1018283059); // 0.0217
		input.m_Profile.m_fD = t.F(1061662228); // 0.78
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP094", input, expected);
	}

	protected static void Case94(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982337); // 100000.0078125
		input.m_Profile.m_fA = t.F(1076174520); // 2.58
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1018283059); // 0.0217
		input.m_Profile.m_fD = t.F(1061662228); // 0.78
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP095", input, expected);
	}

	protected static void Case95(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fA = t.F(1074412913); // 2.16
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(991345561); // 0.0023
		input.m_Profile.m_fD = t.F(1068364726); // 1.359
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1212370944); // 200000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1074412913); // 2.1600000858306885
		expected.m_Material.m_fConductivitySm = t.F(1067043613); // 1.201511025428772
		expected.m_Material.m_fMinMHz = t.F(1203982336); // 100000.0
		expected.m_Material.m_fMaxMHz = t.F(1203982336); // 100000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP096", input, expected);
	}

	protected static void Case96(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1208621911); // 141421.35623730952
		input.m_Profile.m_fA = t.F(1074412913); // 2.16
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(991345561); // 0.0023
		input.m_Profile.m_fD = t.F(1068364726); // 1.359
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1212370944); // 200000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1074412913); // 2.1600000858306885
		expected.m_Material.m_fConductivitySm = t.F(1073106988); // 1.9243216514587402
		expected.m_Material.m_fMinMHz = t.F(1208621911); // 141421.359375
		expected.m_Material.m_fMaxMHz = t.F(1208621911); // 141421.359375
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP097", input, expected);
	}

	protected static void Case97(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1212370944); // 200000
		input.m_Profile.m_fA = t.F(1074412913); // 2.16
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(991345561); // 0.0023
		input.m_Profile.m_fD = t.F(1068364726); // 1.359
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1212370944); // 200000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1074412913); // 2.1600000858306885
		expected.m_Material.m_fConductivitySm = t.F(1078279909); // 3.0819637775421143
		expected.m_Material.m_fMinMHz = t.F(1212370944); // 200000.0
		expected.m_Material.m_fMaxMHz = t.F(1212370944); // 200000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP098", input, expected);
	}

	protected static void Case98(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982335); // 99999.9921875
		input.m_Profile.m_fA = t.F(1074412913); // 2.16
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(991345561); // 0.0023
		input.m_Profile.m_fD = t.F(1068364726); // 1.359
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1212370944); // 200000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP099", input, expected);
	}

	protected static void Case99(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1212370945); // 200000.015625
		input.m_Profile.m_fA = t.F(1074412913); // 2.16
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(991345561); // 0.0023
		input.m_Profile.m_fD = t.F(1068364726); // 1.359
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1212370944); // 200000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP100", input, expected);
	}

	protected static void Case100(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fA = t.F(1076719780); // 2.71
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1051260355); // 0.33
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1193033728); // 40000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1076719780); // 2.7100000381469727
		expected.m_Material.m_fConductivitySm = t.F(1051260355); // 0.33000001311302185
		expected.m_Material.m_fMinMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_fMaxMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP101", input, expected);
	}

	protected static void Case101(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1170580593); // 6324.555320336759
		input.m_Profile.m_fA = t.F(1076719780); // 2.71
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1051260355); // 0.33
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1193033728); // 40000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1076719780); // 2.7100000381469727
		expected.m_Material.m_fConductivitySm = t.F(1051260355); // 0.33000001311302185
		expected.m_Material.m_fMinMHz = t.F(1170580593); // 6324.55517578125
		expected.m_Material.m_fMaxMHz = t.F(1170580593); // 6324.55517578125
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP102", input, expected);
	}

	protected static void Case102(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1193033728); // 40000
		input.m_Profile.m_fA = t.F(1076719780); // 2.71
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1051260355); // 0.33
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1193033728); // 40000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1076719780); // 2.7100000381469727
		expected.m_Material.m_fConductivitySm = t.F(1051260355); // 0.33000001311302185
		expected.m_Material.m_fMinMHz = t.F(1193033728); // 40000.0
		expected.m_Material.m_fMaxMHz = t.F(1193033728); // 40000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP103", input, expected);
	}

	protected static void Case103(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846079); // 999.9999389648438
		input.m_Profile.m_fA = t.F(1076719780); // 2.71
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1051260355); // 0.33
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1193033728); // 40000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP104", input, expected);
	}

	protected static void Case104(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1193033729); // 40000.00390625
		input.m_Profile.m_fA = t.F(1076719780); // 2.71
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1051260355); // 0.33
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1193033728); // 40000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP105", input, expected);
	}

	protected static void Case105(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fA = t.F(1073238508); // 1.94
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1004243884); // 0.0067
		input.m_Profile.m_fD = t.F(1065323017); // 0.9982
		input.m_Profile.m_fMinMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fMaxMHz = t.F(1218519552); // 330000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1073238508); // 1.940000057220459
		expected.m_Material.m_fConductivitySm = t.F(1060836632); // 0.7307906150817871
		expected.m_Material.m_fMinMHz = t.F(1205262336); // 110000.0
		expected.m_Material.m_fMaxMHz = t.F(1205262336); // 110000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP106", input, expected);
	}

	protected static void Case106(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1211764582); // 190525.5888325765
		input.m_Profile.m_fA = t.F(1073238508); // 1.94
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1004243884); // 0.0067
		input.m_Profile.m_fD = t.F(1065323017); // 0.9982
		input.m_Profile.m_fMinMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fMaxMHz = t.F(1218519552); // 330000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1073238508); // 1.940000057220459
		expected.m_Material.m_fConductivitySm = t.F(1067572134); // 1.2645156383514404
		expected.m_Material.m_fMinMHz = t.F(1211764582); // 190525.59375
		expected.m_Material.m_fMaxMHz = t.F(1211764582); // 190525.59375
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP107", input, expected);
	}

	protected static void Case107(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1218519552); // 330000
		input.m_Profile.m_fA = t.F(1073238508); // 1.94
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1004243884); // 0.0067
		input.m_Profile.m_fD = t.F(1065323017); // 0.9982
		input.m_Profile.m_fMinMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fMaxMHz = t.F(1218519552); // 330000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1073238508); // 1.940000057220459
		expected.m_Material.m_fConductivitySm = t.F(1074530524); // 2.1880407333374023
		expected.m_Material.m_fMinMHz = t.F(1218519552); // 330000.0
		expected.m_Material.m_fMaxMHz = t.F(1218519552); // 330000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP108", input, expected);
	}

	protected static void Case108(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1205262335); // 109999.9921875
		input.m_Profile.m_fA = t.F(1073238508); // 1.94
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1004243884); // 0.0067
		input.m_Profile.m_fD = t.F(1065323017); // 0.9982
		input.m_Profile.m_fMinMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fMaxMHz = t.F(1218519552); // 330000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP109", input, expected);
	}

	protected static void Case109(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1218519553); // 330000.03125
		input.m_Profile.m_fA = t.F(1073238508); // 1.94
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1004243884); // 0.0067
		input.m_Profile.m_fD = t.F(1065323017); // 0.9982
		input.m_Profile.m_fMinMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fMaxMHz = t.F(1218519552); // 330000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP110", input, expected);
	}

	protected static void Case110(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fA = t.F(1074454856); // 2.17
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1003384891); // 0.0063
		input.m_Profile.m_fD = t.F(1065730703); // 1.045
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1074454856); // 2.1700000762939453
		expected.m_Material.m_fConductivitySm = t.F(1061579503); // 0.7750691771507263
		expected.m_Material.m_fMinMHz = t.F(1203982336); // 100000.0
		expected.m_Material.m_fMaxMHz = t.F(1203982336); // 100000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP111", input, expected);
	}

	protected static void Case111(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1212370944); // 200000.0
		input.m_Profile.m_fA = t.F(1074454856); // 2.17
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1003384891); // 0.0063
		input.m_Profile.m_fD = t.F(1065730703); // 1.045
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1074454856); // 2.1700000762939453
		expected.m_Material.m_fConductivitySm = t.F(1070380103); // 1.599251627922058
		expected.m_Material.m_fMinMHz = t.F(1212370944); // 200000.0
		expected.m_Material.m_fMaxMHz = t.F(1212370944); // 200000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP112", input, expected);
	}

	protected static void Case112(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1220759552); // 400000
		input.m_Profile.m_fA = t.F(1074454856); // 2.17
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1003384891); // 0.0063
		input.m_Profile.m_fD = t.F(1065730703); // 1.045
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1074454856); // 2.1700000762939453
		expected.m_Material.m_fConductivitySm = t.F(1079193756); // 3.29984188079834
		expected.m_Material.m_fMinMHz = t.F(1220759552); // 400000.0
		expected.m_Material.m_fMaxMHz = t.F(1220759552); // 400000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP113", input, expected);
	}

	protected static void Case113(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982335); // 99999.9921875
		input.m_Profile.m_fA = t.F(1074454856); // 2.17
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1003384891); // 0.0063
		input.m_Profile.m_fD = t.F(1065730703); // 1.045
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP114", input, expected);
	}

	protected static void Case114(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1220759553); // 400000.03125
		input.m_Profile.m_fA = t.F(1074454856); // 2.17
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1003384891); // 0.0063
		input.m_Profile.m_fD = t.F(1065730703); // 1.045
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP115", input, expected);
	}

	protected static void Case115(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fA = t.F(1088577077); // 7.074
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1001666904); // 0.0055
		input.m_Profile.m_fD = t.F(1064115057); // 0.9262
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1198153728); // 60000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1088577077); // 7.073999881744385
		expected.m_Material.m_fConductivitySm = t.F(1001666904); // 0.005499999970197678
		expected.m_Material.m_fMinMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_fMaxMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP116", input, expected);
	}

	protected static void Case116(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1173491644); // 7745.966692414834
		input.m_Profile.m_fA = t.F(1088577077); // 7.074
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1001666904); // 0.0055
		input.m_Profile.m_fD = t.F(1064115057); // 0.9262
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1198153728); // 60000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1088577077); // 7.073999881744385
		expected.m_Material.m_fConductivitySm = t.F(1024854078); // 0.036628954112529755
		expected.m_Material.m_fMinMHz = t.F(1173491644); // 7745.966796875
		expected.m_Material.m_fMaxMHz = t.F(1173491644); // 7745.966796875
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP117", input, expected);
	}

	protected static void Case117(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1198153728); // 60000
		input.m_Profile.m_fA = t.F(1088577077); // 7.074
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1001666904); // 0.0055
		input.m_Profile.m_fD = t.F(1064115057); // 0.9262
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1198153728); // 60000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1088577077); // 7.073999881744385
		expected.m_Material.m_fConductivitySm = t.F(1048169445); // 0.24394185841083527
		expected.m_Material.m_fMinMHz = t.F(1198153728); // 60000.0
		expected.m_Material.m_fMaxMHz = t.F(1198153728); // 60000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP118", input, expected);
	}

	protected static void Case118(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846079); // 999.9999389648438
		input.m_Profile.m_fA = t.F(1088577077); // 7.074
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1001666904); // 0.0055
		input.m_Profile.m_fD = t.F(1064115057); // 0.9262
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1198153728); // 60000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP119", input, expected);
	}

	protected static void Case119(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1198153729); // 60000.00390625
		input.m_Profile.m_fA = t.F(1088577077); // 7.074
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1001666904); // 0.0055
		input.m_Profile.m_fD = t.F(1064115057); // 0.9262
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1198153728); // 60000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP120", input, expected);
	}

	protected static void Case120(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fA = t.F(1090393211); // 7.94
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(953267991); // 0.0001
		input.m_Profile.m_fD = t.F(1071502066); // 1.733
		input.m_Profile.m_fMinMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fMaxMHz = t.F(1218519552); // 330000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1090393211); // 7.940000057220459
		expected.m_Material.m_fConductivitySm = t.F(1051761392); // 0.34493207931518555
		expected.m_Material.m_fMinMHz = t.F(1205262336); // 110000.0
		expected.m_Material.m_fMaxMHz = t.F(1205262336); // 110000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP121", input, expected);
	}

	protected static void Case121(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1211764582); // 190525.5888325765
		input.m_Profile.m_fA = t.F(1090393211); // 7.94
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(953267991); // 0.0001
		input.m_Profile.m_fD = t.F(1071502066); // 1.733
		input.m_Profile.m_fMinMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fMaxMHz = t.F(1218519552); // 330000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1090393211); // 7.940000057220459
		expected.m_Material.m_fConductivitySm = t.F(1063568674); // 0.8936330080032349
		expected.m_Material.m_fMinMHz = t.F(1211764582); // 190525.59375
		expected.m_Material.m_fMaxMHz = t.F(1211764582); // 190525.59375
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP122", input, expected);
	}

	protected static void Case122(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1218519552); // 330000
		input.m_Profile.m_fA = t.F(1090393211); // 7.94
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(953267991); // 0.0001
		input.m_Profile.m_fD = t.F(1071502066); // 1.733
		input.m_Profile.m_fMinMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fMaxMHz = t.F(1218519552); // 330000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1090393211); // 7.940000057220459
		expected.m_Material.m_fConductivitySm = t.F(1075063784); // 2.3151798248291016
		expected.m_Material.m_fMinMHz = t.F(1218519552); // 330000.0
		expected.m_Material.m_fMaxMHz = t.F(1218519552); // 330000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP123", input, expected);
	}

	protected static void Case123(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1205262335); // 109999.9921875
		input.m_Profile.m_fA = t.F(1090393211); // 7.94
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(953267991); // 0.0001
		input.m_Profile.m_fD = t.F(1071502066); // 1.733
		input.m_Profile.m_fMinMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fMaxMHz = t.F(1218519552); // 330000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP124", input, expected);
	}

	protected static void Case124(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1218519553); // 330000.03125
		input.m_Profile.m_fA = t.F(1090393211); // 7.94
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(953267991); // 0.0001
		input.m_Profile.m_fD = t.F(1071502066); // 1.733
		input.m_Profile.m_fMinMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fMaxMHz = t.F(1218519552); // 330000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP125", input, expected);
	}

	protected static void Case125(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fA = t.F(1091169157); // 8.62
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(993063548); // 0.0027
		input.m_Profile.m_fD = t.F(1066611507); // 1.15
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1091169157); // 8.619999885559082
		expected.m_Material.m_fConductivitySm = t.F(1057614235); // 0.5387207865715027
		expected.m_Material.m_fMinMHz = t.F(1203982336); // 100000.0
		expected.m_Material.m_fMaxMHz = t.F(1203982336); // 100000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP126", input, expected);
	}

	protected static void Case126(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1212370944); // 200000.0
		input.m_Profile.m_fA = t.F(1091169157); // 8.62
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(993063548); // 0.0027
		input.m_Profile.m_fD = t.F(1066611507); // 1.15
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1091169157); // 8.619999885559082
		expected.m_Material.m_fConductivitySm = t.F(1066993157); // 1.1954962015151978
		expected.m_Material.m_fMinMHz = t.F(1212370944); // 200000.0
		expected.m_Material.m_fMaxMHz = t.F(1212370944); // 200000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP127", input, expected);
	}

	protected static void Case127(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1220759552); // 400000
		input.m_Profile.m_fA = t.F(1091169157); // 8.62
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(993063548); // 0.0027
		input.m_Profile.m_fD = t.F(1066611507); // 1.15
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1091169157); // 8.619999885559082
		expected.m_Material.m_fConductivitySm = t.F(1076480588); // 2.6529722213745117
		expected.m_Material.m_fMinMHz = t.F(1220759552); // 400000.0
		expected.m_Material.m_fMaxMHz = t.F(1220759552); // 400000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP128", input, expected);
	}

	protected static void Case128(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982335); // 99999.9921875
		input.m_Profile.m_fA = t.F(1091169157); // 8.62
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(993063548); // 0.0027
		input.m_Profile.m_fD = t.F(1066611507); // 1.15
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP129", input, expected);
	}

	protected static void Case129(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1220759553); // 400000.03125
		input.m_Profile.m_fA = t.F(1091169157); // 8.62
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(993063548); // 0.0027
		input.m_Profile.m_fD = t.F(1066611507); // 1.15
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP130", input, expected);
	}

	protected static void Case130(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1195593728); // 50000
		input.m_Profile.m_fA = t.F(1080704369); // 3.66
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(999304672); // 0.0044
		input.m_Profile.m_fD = t.F(1068301812); // 1.3515
		input.m_Profile.m_fMinMHz = t.F(1195593728); // 50000
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1080704369); // 3.6600000858306885
		expected.m_Material.m_fConductivitySm = t.F(1063175251); // 0.8701831698417664
		expected.m_Material.m_fMinMHz = t.F(1195593728); // 50000.0
		expected.m_Material.m_fMaxMHz = t.F(1195593728); // 50000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP131", input, expected);
	}

	protected static void Case131(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1200233303); // 70710.67811865476
		input.m_Profile.m_fA = t.F(1080704369); // 3.66
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(999304672); // 0.0044
		input.m_Profile.m_fD = t.F(1068301812); // 1.3515
		input.m_Profile.m_fMinMHz = t.F(1195593728); // 50000
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1080704369); // 3.6600000858306885
		expected.m_Material.m_fConductivitySm = t.F(1068625228); // 1.3900542259216309
		expected.m_Material.m_fMinMHz = t.F(1200233303); // 70710.6796875
		expected.m_Material.m_fMaxMHz = t.F(1200233303); // 70710.6796875
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP132", input, expected);
	}

	protected static void Case132(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fA = t.F(1080704369); // 3.66
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(999304672); // 0.0044
		input.m_Profile.m_fD = t.F(1068301812); // 1.3515
		input.m_Profile.m_fMinMHz = t.F(1195593728); // 50000
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1080704369); // 3.6600000858306885
		expected.m_Material.m_fConductivitySm = t.F(1074666710); // 2.2205100059509277
		expected.m_Material.m_fMinMHz = t.F(1203982336); // 100000.0
		expected.m_Material.m_fMaxMHz = t.F(1203982336); // 100000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP133", input, expected);
	}

	protected static void Case133(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1195593727); // 49999.99609375
		input.m_Profile.m_fA = t.F(1080704369); // 3.66
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(999304672); // 0.0044
		input.m_Profile.m_fD = t.F(1068301812); // 1.3515
		input.m_Profile.m_fMinMHz = t.F(1195593728); // 50000
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP134", input, expected);
	}

	protected static void Case134(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982337); // 100000.0078125
		input.m_Profile.m_fA = t.F(1080704369); // 3.66
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(999304672); // 0.0044
		input.m_Profile.m_fD = t.F(1068301812); // 1.3515
		input.m_Profile.m_fMinMHz = t.F(1195593728); // 50000
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP135", input, expected);
	}

	protected static void Case135(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1213650944); // 220000
		input.m_Profile.m_fA = t.F(1084793815); // 5.27
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(600621637); // 2.22e-17
		input.m_Profile.m_fD = t.F(1089137646); // 7.3413
		input.m_Profile.m_fMinMHz = t.F(1213650944); // 220000
		input.m_Profile.m_fMaxMHz = t.F(1217559552); // 300000
		input.m_Profile.m_bMaxInclusive = false;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1084793815); // 5.269999980926514
		expected.m_Material.m_fConductivitySm = t.F(1079989827); // 3.4896399974823
		expected.m_Material.m_fMinMHz = t.F(1213650944); // 220000.0
		expected.m_Material.m_fMaxMHz = t.F(1213650944); // 220000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP136", input, expected);
	}

	protected static void Case136(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1216012842); // 256904.65157330257
		input.m_Profile.m_fA = t.F(1084793815); // 5.27
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(600621637); // 2.22e-17
		input.m_Profile.m_fD = t.F(1089137646); // 7.3413
		input.m_Profile.m_fMinMHz = t.F(1213650944); // 220000
		input.m_Profile.m_fMaxMHz = t.F(1217559552); // 300000
		input.m_Profile.m_bMaxInclusive = false;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1084793815); // 5.269999980926514
		expected.m_Material.m_fConductivitySm = t.F(1093554267); // 10.894618034362793
		expected.m_Material.m_fMinMHz = t.F(1216012842); // 256904.65625
		expected.m_Material.m_fMaxMHz = t.F(1216012842); // 256904.65625
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP137", input, expected);
	}

	protected static void Case137(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1217559552); // 300000
		input.m_Profile.m_fA = t.F(1084793815); // 5.27
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(600621637); // 2.22e-17
		input.m_Profile.m_fD = t.F(1089137646); // 7.3413
		input.m_Profile.m_fMinMHz = t.F(1213650944); // 220000
		input.m_Profile.m_fMaxMHz = t.F(1217559552); // 300000
		input.m_Profile.m_bMaxInclusive = false;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP138", input, expected);
	}

	protected static void Case138(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1213650943); // 219999.984375
		input.m_Profile.m_fA = t.F(1084793815); // 5.27
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(600621637); // 2.22e-17
		input.m_Profile.m_fD = t.F(1089137646); // 7.3413
		input.m_Profile.m_fMinMHz = t.F(1213650944); // 220000
		input.m_Profile.m_fMaxMHz = t.F(1217559552); // 300000
		input.m_Profile.m_bMaxInclusive = false;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP139", input, expected);
	}

	protected static void Case139(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1217559553); // 300000.03125
		input.m_Profile.m_fA = t.F(1084793815); // 5.27
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(600621637); // 2.22e-17
		input.m_Profile.m_fD = t.F(1089137646); // 7.3413
		input.m_Profile.m_fMinMHz = t.F(1213650944); // 220000
		input.m_Profile.m_fMaxMHz = t.F(1217559552); // 300000
		input.m_Profile.m_bMaxInclusive = false;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP140", input, expected);
	}

	protected static void Case140(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1217559552); // 300000
		input.m_Profile.m_fA = t.F(1084793815); // 5.27
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(966609234); // 0.0003
		input.m_Profile.m_fD = t.F(1073866814); // 2.0298
		input.m_Profile.m_fMinMHz = t.F(1217559552); // 300000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = false;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1084793815); // 5.269999980926514
		expected.m_Material.m_fConductivitySm = t.F(1107296872); // 32.002349853515625
		expected.m_Material.m_fMinMHz = t.F(1217559552); // 300000.0
		expected.m_Material.m_fMaxMHz = t.F(1217559552); // 300000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP141", input, expected);
	}

	protected static void Case141(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1219044677); // 346410.16151377547
		input.m_Profile.m_fA = t.F(1084793815); // 5.27
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(966609234); // 0.0003
		input.m_Profile.m_fD = t.F(1073866814); // 2.0298
		input.m_Profile.m_fMinMHz = t.F(1217559552); // 300000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = false;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1084793815); // 5.269999980926514
		expected.m_Material.m_fConductivitySm = t.F(1110141329); // 42.853092193603516
		expected.m_Material.m_fMinMHz = t.F(1219044677); // 346410.15625
		expected.m_Material.m_fMaxMHz = t.F(1219044677); // 346410.15625
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP142", input, expected);
	}

	protected static void Case142(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1220759552); // 400000
		input.m_Profile.m_fA = t.F(1084793815); // 5.27
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(966609234); // 0.0003
		input.m_Profile.m_fD = t.F(1073866814); // 2.0298
		input.m_Profile.m_fMinMHz = t.F(1217559552); // 300000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = false;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP143", input, expected);
	}

	protected static void Case143(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1217559551); // 299999.96875
		input.m_Profile.m_fA = t.F(1084793815); // 5.27
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(966609234); // 0.0003
		input.m_Profile.m_fD = t.F(1073866814); // 2.0298
		input.m_Profile.m_fMinMHz = t.F(1217559552); // 300000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = false;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP144", input, expected);
	}

	protected static void Case144(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1220759553); // 400000.03125
		input.m_Profile.m_fA = t.F(1084793815); // 5.27
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(966609234); // 0.0003
		input.m_Profile.m_fD = t.F(1073866814); // 2.0298
		input.m_Profile.m_fMinMHz = t.F(1217559552); // 300000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = false;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP145", input, expected);
	}

	protected static void Case145(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1220759552); // 400000
		input.m_Profile.m_fA = t.F(1084793815); // 5.27
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1111981451); // 49.8726
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1220759552); // 400000
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = false;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1084793815); // 5.269999980926514
		expected.m_Material.m_fConductivitySm = t.F(1111981451); // 49.87260055541992
		expected.m_Material.m_fMinMHz = t.F(1220759552); // 400000.0
		expected.m_Material.m_fMaxMHz = t.F(1220759552); // 400000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP146", input, expected);
	}

	protected static void Case146(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1221536002); // 424264.0687119285
		input.m_Profile.m_fA = t.F(1084793815); // 5.27
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1111981451); // 49.8726
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1220759552); // 400000
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = false;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1084793815); // 5.269999980926514
		expected.m_Material.m_fConductivitySm = t.F(1111981451); // 49.87260055541992
		expected.m_Material.m_fMinMHz = t.F(1221536002); // 424264.0625
		expected.m_Material.m_fMaxMHz = t.F(1221536002); // 424264.0625
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP147", input, expected);
	}

	protected static void Case147(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1222359552); // 450000
		input.m_Profile.m_fA = t.F(1084793815); // 5.27
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1111981451); // 49.8726
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1220759552); // 400000
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = false;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP148", input, expected);
	}

	protected static void Case148(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1220759551); // 399999.96875
		input.m_Profile.m_fA = t.F(1084793815); // 5.27
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1111981451); // 49.8726
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1220759552); // 400000
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = false;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP149", input, expected);
	}

	protected static void Case149(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1222359553); // 450000.03125
		input.m_Profile.m_fA = t.F(1084793815); // 5.27
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1111981451); // 49.8726
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1220759552); // 400000
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = false;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP150", input, expected);
	}

	protected static void Case150(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fA = t.F(1078596731); // 3.1575
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(987466668); // 0.001675
		input.m_Profile.m_fD = t.F(1068102582); // 1.32775
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1078596731); // 3.1575000286102295
		expected.m_Material.m_fConductivitySm = t.F(1061288607); // 0.7577304244041443
		expected.m_Material.m_fMinMHz = t.F(1203982336); // 100000.0
		expected.m_Material.m_fMaxMHz = t.F(1203982336); // 100000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP151", input, expected);
	}

	protected static void Case151(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1212370944); // 200000.0
		input.m_Profile.m_fA = t.F(1078596731); // 3.1575
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(987466668); // 0.001675
		input.m_Profile.m_fD = t.F(1068102582); // 1.32775
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1078596731); // 3.1575000286102295
		expected.m_Material.m_fConductivitySm = t.F(1072919622); // 1.9019858837127686
		expected.m_Material.m_fMinMHz = t.F(1212370944); // 200000.0
		expected.m_Material.m_fMaxMHz = t.F(1212370944); // 200000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP152", input, expected);
	}

	protected static void Case152(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1220759552); // 400000
		input.m_Profile.m_fA = t.F(1078596731); // 3.1575
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(987466668); // 0.001675
		input.m_Profile.m_fD = t.F(1068102582); // 1.32775
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1078596731); // 3.1575000286102295
		expected.m_Material.m_fConductivitySm = t.F(1083754030); // 4.774191856384277
		expected.m_Material.m_fMinMHz = t.F(1220759552); // 400000.0
		expected.m_Material.m_fMaxMHz = t.F(1220759552); // 400000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP153", input, expected);
	}

	protected static void Case153(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982335); // 99999.9921875
		input.m_Profile.m_fA = t.F(1078596731); // 3.1575
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(987466668); // 0.001675
		input.m_Profile.m_fD = t.F(1068102582); // 1.32775
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP154", input, expected);
	}

	protected static void Case154(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1220759553); // 400000.03125
		input.m_Profile.m_fA = t.F(1078596731); // 3.1575
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(987466668); // 0.001675
		input.m_Profile.m_fD = t.F(1068102582); // 1.32775
		input.m_Profile.m_fMinMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fMaxMHz = t.F(1220759552); // 400000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP155", input, expected);
	}

	protected static void Case155(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fA = t.F(1080536596); // 3.62
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1000807911); // 0.0051
		input.m_Profile.m_fD = t.F(1062705771); // 0.8422
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1193033728); // 40000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1080536596); // 3.619999885559082
		expected.m_Material.m_fConductivitySm = t.F(1000807911); // 0.005100000184029341
		expected.m_Material.m_fMinMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_fMaxMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP156", input, expected);
	}

	protected static void Case156(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1170580593); // 6324.555320336759
		input.m_Profile.m_fA = t.F(1080536596); // 3.62
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1000807911); // 0.0051
		input.m_Profile.m_fD = t.F(1062705771); // 0.8422
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1193033728); // 40000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1080536596); // 3.619999885559082
		expected.m_Material.m_fConductivitySm = t.F(1019576930); // 0.02411002293229103
		expected.m_Material.m_fMinMHz = t.F(1170580593); // 6324.55517578125
		expected.m_Material.m_fMaxMHz = t.F(1170580593); // 6324.55517578125
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP157", input, expected);
	}

	protected static void Case157(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1193033728); // 40000
		input.m_Profile.m_fA = t.F(1080536596); // 3.62
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1000807911); // 0.0051
		input.m_Profile.m_fD = t.F(1062705771); // 0.8422
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1193033728); // 40000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1080536596); // 3.619999885559082
		expected.m_Material.m_fConductivitySm = t.F(1038708187); // 0.11397906392812729
		expected.m_Material.m_fMinMHz = t.F(1193033728); // 40000.0
		expected.m_Material.m_fMaxMHz = t.F(1193033728); // 40000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP158", input, expected);
	}

	protected static void Case158(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846079); // 999.9999389648438
		input.m_Profile.m_fA = t.F(1080536596); // 3.62
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1000807911); // 0.0051
		input.m_Profile.m_fD = t.F(1062705771); // 0.8422
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1193033728); // 40000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP159", input, expected);
	}

	protected static void Case159(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1193033729); // 40000.00390625
		input.m_Profile.m_fA = t.F(1080536596); // 3.62
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1000807911); // 0.0051
		input.m_Profile.m_fD = t.F(1062705771); // 0.8422
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1193033728); // 40000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP160", input, expected);
	}

	protected static void Case160(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fA = t.F(1074077368); // 2.08
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(980151802); // 0.0009
		input.m_Profile.m_fD = t.F(1062333317); // 0.82
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1193033728); // 40000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1074077368); // 2.0799999237060547
		expected.m_Material.m_fConductivitySm = t.F(980151802); // 0.0008999999845400453
		expected.m_Material.m_fMinMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_fMaxMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP161", input, expected);
	}

	protected static void Case161(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1170580593); // 6324.555320336759
		input.m_Profile.m_fA = t.F(1074077368); // 2.08
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(980151802); // 0.0009
		input.m_Profile.m_fD = t.F(1062333317); // 0.82
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1193033728); // 40000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1074077368); // 2.0799999237060547
		expected.m_Material.m_fConductivitySm = t.F(998626095); // 0.004084012936800718
		expected.m_Material.m_fMinMHz = t.F(1170580593); // 6324.55517578125
		expected.m_Material.m_fMaxMHz = t.F(1170580593); // 6324.55517578125
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP162", input, expected);
	}

	protected static void Case162(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1193033728); // 40000
		input.m_Profile.m_fA = t.F(1074077368); // 2.08
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(980151802); // 0.0009
		input.m_Profile.m_fD = t.F(1062333317); // 0.82
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1193033728); // 40000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1074077368); // 2.0799999237060547
		expected.m_Material.m_fConductivitySm = t.F(1016582467); // 0.01853240095078945
		expected.m_Material.m_fMinMHz = t.F(1193033728); // 40000.0
		expected.m_Material.m_fMaxMHz = t.F(1193033728); // 40000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP163", input, expected);
	}

	protected static void Case163(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846079); // 999.9999389648438
		input.m_Profile.m_fA = t.F(1074077368); // 2.08
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(980151802); // 0.0009
		input.m_Profile.m_fD = t.F(1062333317); // 0.82
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1193033728); // 40000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP164", input, expected);
	}

	protected static void Case164(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1193033729); // 40000.00390625
		input.m_Profile.m_fA = t.F(1074077368); // 2.08
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(980151802); // 0.0009
		input.m_Profile.m_fD = t.F(1062333317); // 0.82
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1193033728); // 40000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP165", input, expected);
	}

	protected static void Case165(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fA = t.F(1083871068); // 4.83
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1009840764); // 0.0108
		input.m_Profile.m_fD = t.F(1068682655); // 1.3969
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1193033728); // 40000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1083871068); // 4.829999923706055
		expected.m_Material.m_fConductivitySm = t.F(1009840764); // 0.01080000028014183
		expected.m_Material.m_fMinMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_fMaxMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP166", input, expected);
	}

	protected static void Case166(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1170580593); // 6324.555320336759
		input.m_Profile.m_fA = t.F(1083871068); // 4.83
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1009840764); // 0.0108
		input.m_Profile.m_fD = t.F(1068682655); // 1.3969
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1193033728); // 40000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1083871068); // 4.829999923706055
		expected.m_Material.m_fConductivitySm = t.F(1041330311); // 0.14203082025051117
		expected.m_Material.m_fMinMHz = t.F(1170580593); // 6324.55517578125
		expected.m_Material.m_fMaxMHz = t.F(1170580593); // 6324.55517578125
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP167", input, expected);
	}

	protected static void Case167(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1193033728); // 40000
		input.m_Profile.m_fA = t.F(1083871068); // 4.83
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1009840764); // 0.0108
		input.m_Profile.m_fD = t.F(1068682655); // 1.3969
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1193033728); // 40000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1083871068); // 4.829999923706055
		expected.m_Material.m_fConductivitySm = t.F(1072633250); // 1.8678476810455322
		expected.m_Material.m_fMinMHz = t.F(1193033728); // 40000.0
		expected.m_Material.m_fMaxMHz = t.F(1193033728); // 40000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP168", input, expected);
	}

	protected static void Case168(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846079); // 999.9999389648438
		input.m_Profile.m_fA = t.F(1083871068); // 4.83
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1009840764); // 0.0108
		input.m_Profile.m_fD = t.F(1068682655); // 1.3969
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1193033728); // 40000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP169", input, expected);
	}

	protected static void Case169(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1193033729); // 40000.00390625
		input.m_Profile.m_fA = t.F(1083871068); // 4.83
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1009840764); // 0.0108
		input.m_Profile.m_fD = t.F(1068682655); // 1.3969
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1193033728); // 40000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP170", input, expected);
	}

	protected static void Case170(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fA = t.F(1065353216); // 1
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1259902592); // 10000000
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1065353216); // 1.0
		expected.m_Material.m_fConductivitySm = t.F(1259902592); // 10000000.0
		expected.m_Material.m_fMinMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_fMaxMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP171", input, expected);
	}

	protected static void Case171(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1176256512); // 10000.0
		input.m_Profile.m_fA = t.F(1065353216); // 1
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1259902592); // 10000000
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1065353216); // 1.0
		expected.m_Material.m_fConductivitySm = t.F(1259902592); // 10000000.0
		expected.m_Material.m_fMinMHz = t.F(1176256512); // 10000.0
		expected.m_Material.m_fMaxMHz = t.F(1176256512); // 10000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP172", input, expected);
	}

	protected static void Case172(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fA = t.F(1065353216); // 1
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1259902592); // 10000000
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1065353216); // 1.0
		expected.m_Material.m_fConductivitySm = t.F(1259902592); // 10000000.0
		expected.m_Material.m_fMinMHz = t.F(1203982336); // 100000.0
		expected.m_Material.m_fMaxMHz = t.F(1203982336); // 100000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP173", input, expected);
	}

	protected static void Case173(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846079); // 999.9999389648438
		input.m_Profile.m_fA = t.F(1065353216); // 1
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1259902592); // 10000000
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP174", input, expected);
	}

	protected static void Case174(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982337); // 100000.0078125
		input.m_Profile.m_fA = t.F(1065353216); // 1
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1259902592); // 10000000
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1203982336); // 100000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP175", input, expected);
	}

	protected static void Case175(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fA = t.F(1077936128); // 3
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(958220626); // 0.00015
		input.m_Profile.m_fD = t.F(1075922862); // 2.52
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1176256512); // 10000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1077936128); // 3.0
		expected.m_Material.m_fConductivitySm = t.F(958220626); // 0.0001500000071246177
		expected.m_Material.m_fMinMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_fMaxMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP176", input, expected);
	}

	protected static void Case176(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1162191985); // 3162.2776601683795
		input.m_Profile.m_fA = t.F(1077936128); // 3
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(958220626); // 0.00015
		input.m_Profile.m_fD = t.F(1075922862); // 2.52
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1176256512); // 10000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1077936128); // 3.0
		expected.m_Material.m_fConductivitySm = t.F(993190469); // 0.0027295511681586504
		expected.m_Material.m_fMinMHz = t.F(1162191985); // 3162.277587890625
		expected.m_Material.m_fMaxMHz = t.F(1162191985); // 3162.277587890625
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP177", input, expected);
	}

	protected static void Case177(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1176256512); // 10000
		input.m_Profile.m_fA = t.F(1077936128); // 3
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(958220626); // 0.00015
		input.m_Profile.m_fD = t.F(1075922862); // 2.52
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1176256512); // 10000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1077936128); // 3.0
		expected.m_Material.m_fConductivitySm = t.F(1028354668); // 0.049669668078422546
		expected.m_Material.m_fMinMHz = t.F(1176256512); // 10000.0
		expected.m_Material.m_fMaxMHz = t.F(1176256512); // 10000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP178", input, expected);
	}

	protected static void Case178(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846079); // 999.9999389648438
		input.m_Profile.m_fA = t.F(1077936128); // 3
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(958220626); // 0.00015
		input.m_Profile.m_fD = t.F(1075922862); // 2.52
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1176256512); // 10000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP179", input, expected);
	}

	protected static void Case179(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1176256513); // 10000.0009765625
		input.m_Profile.m_fA = t.F(1077936128); // 3
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(958220626); // 0.00015
		input.m_Profile.m_fD = t.F(1075922862); // 2.52
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1176256512); // 10000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP180", input, expected);
	}

	protected static void Case180(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fA = t.F(1097859072); // 15
		input.m_Profile.m_fB = t.F(-1110651699); // -0.1
		input.m_Profile.m_fC = t.F(1024416809); // 0.035
		input.m_Profile.m_fD = t.F(1070638039); // 1.63
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1176256512); // 10000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1097859072); // 15.0
		expected.m_Material.m_fConductivitySm = t.F(1024416809); // 0.03500000014901161
		expected.m_Material.m_fMinMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_fMaxMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP181", input, expected);
	}

	protected static void Case181(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1162191985); // 3162.2776601683795
		input.m_Profile.m_fA = t.F(1097859072); // 15
		input.m_Profile.m_fB = t.F(-1110651699); // -0.1
		input.m_Profile.m_fC = t.F(1024416809); // 0.035
		input.m_Profile.m_fD = t.F(1070638039); // 1.63
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1176256512); // 10000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1096148597); // 13.36876392364502
		expected.m_Material.m_fConductivitySm = t.F(1047139581); // 0.22859568893909454
		expected.m_Material.m_fMinMHz = t.F(1162191985); // 3162.277587890625
		expected.m_Material.m_fMaxMHz = t.F(1162191985); // 3162.277587890625
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP182", input, expected);
	}

	protected static void Case182(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1176256512); // 10000
		input.m_Profile.m_fA = t.F(1097859072); // 15
		input.m_Profile.m_fB = t.F(-1110651699); // -0.1
		input.m_Profile.m_fC = t.F(1024416809); // 0.035
		input.m_Profile.m_fD = t.F(1070638039); // 1.63
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1176256512); // 10000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1094624135); // 11.914923667907715
		expected.m_Material.m_fConductivitySm = t.F(1069489037); // 1.4930282831192017
		expected.m_Material.m_fMinMHz = t.F(1176256512); // 10000.0
		expected.m_Material.m_fMaxMHz = t.F(1176256512); // 10000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP183", input, expected);
	}

	protected static void Case183(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846079); // 999.9999389648438
		input.m_Profile.m_fA = t.F(1097859072); // 15
		input.m_Profile.m_fB = t.F(-1110651699); // -0.1
		input.m_Profile.m_fC = t.F(1024416809); // 0.035
		input.m_Profile.m_fD = t.F(1070638039); // 1.63
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1176256512); // 10000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP184", input, expected);
	}

	protected static void Case184(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1176256513); // 10000.0009765625
		input.m_Profile.m_fA = t.F(1097859072); // 15
		input.m_Profile.m_fB = t.F(-1110651699); // -0.1
		input.m_Profile.m_fC = t.F(1024416809); // 0.035
		input.m_Profile.m_fD = t.F(1070638039); // 1.63
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1176256512); // 10000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP185", input, expected);
	}

	protected static void Case185(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fA = t.F(1106247680); // 30
		input.m_Profile.m_fB = t.F(-1093874483); // -0.4
		input.m_Profile.m_fC = t.F(1041865114); // 0.15
		input.m_Profile.m_fD = t.F(1067869798); // 1.3
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1176256512); // 10000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1106247680); // 30.0
		expected.m_Material.m_fConductivitySm = t.F(1041865114); // 0.15000000596046448
		expected.m_Material.m_fMinMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_fMaxMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP186", input, expected);
	}

	protected static void Case186(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1162191985); // 3162.2776601683795
		input.m_Profile.m_fA = t.F(1106247680); // 30
		input.m_Profile.m_fB = t.F(-1093874483); // -0.4
		input.m_Profile.m_fC = t.F(1041865114); // 0.15
		input.m_Profile.m_fD = t.F(1067869798); // 1.3
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1176256512); // 10000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1100443141); // 18.928720474243164
		expected.m_Material.m_fConductivitySm = t.F(1059817160); // 0.6700253486633301
		expected.m_Material.m_fMinMHz = t.F(1162191985); // 3162.277587890625
		expected.m_Material.m_fMaxMHz = t.F(1162191985); // 3162.277587890625
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP187", input, expected);
	}

	protected static void Case187(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1176256512); // 10000
		input.m_Profile.m_fA = t.F(1106247680); // 30
		input.m_Profile.m_fB = t.F(-1093874483); // -0.4
		input.m_Profile.m_fC = t.F(1041865114); // 0.15
		input.m_Profile.m_fD = t.F(1067869798); // 1.3
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1176256512); // 10000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1094653801); // 11.943215370178223
		expected.m_Material.m_fConductivitySm = t.F(1077906320); // 2.9928932189941406
		expected.m_Material.m_fMinMHz = t.F(1176256512); // 10000.0
		expected.m_Material.m_fMaxMHz = t.F(1176256512); // 10000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP188", input, expected);
	}

	protected static void Case188(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846079); // 999.9999389648438
		input.m_Profile.m_fA = t.F(1106247680); // 30
		input.m_Profile.m_fB = t.F(-1093874483); // -0.4
		input.m_Profile.m_fC = t.F(1041865114); // 0.15
		input.m_Profile.m_fD = t.F(1067869798); // 1.3
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1176256512); // 10000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP189", input, expected);
	}

	protected static void Case189(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1176256513); // 10000.0009765625
		input.m_Profile.m_fA = t.F(1106247680); // 30
		input.m_Profile.m_fB = t.F(-1093874483); // -0.4
		input.m_Profile.m_fC = t.F(1041865114); // 0.15
		input.m_Profile.m_fD = t.F(1067869798); // 1.3
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1176256512); // 10000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP190", input, expected);
	}

	protected static void Case190(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fA = t.F(1082130432); // 4
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1048576000); // 0.25
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		expected.m_Material.m_fConductivitySm = t.F(1048576000); // 0.25
		expected.m_Material.m_fMinMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_fMaxMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.SOURCE_DOMAIN;
		t.CheckCase("MP191", input, expected);
	}

	protected static void Case191(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fA = t.F(1082130432); // 4
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1048576000); // 0.25
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.ASSUMED_CONFIG;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		expected.m_Material.m_fConductivitySm = t.F(1048576000); // 0.25
		expected.m_Material.m_fMinMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_fMaxMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.ASSUMED_CONFIG;
		t.CheckCase("MP192", input, expected);
	}

	protected static void Case192(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fA = t.F(1082130432); // 4
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1048576000); // 0.25
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		expected.m_Material.m_fConductivitySm = t.F(1048576000); // 0.25
		expected.m_Material.m_fMinMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_fMaxMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MP193", input, expected);
	}

	protected static void Case193(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1157234688); // 2000
		input.m_Profile.m_fA = t.F(1065353216); // 1
		input.m_Profile.m_fB = t.F(1065353216); // 1
		input.m_Profile.m_fC = t.F(1040187392); // 0.125
		input.m_Profile.m_fD = t.F(1073741824); // 2
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1073741824); // 2.0
		expected.m_Material.m_fConductivitySm = t.F(1056964608); // 0.5
		expected.m_Material.m_fMinMHz = t.F(1157234688); // 2000.0
		expected.m_Material.m_fMaxMHz = t.F(1157234688); // 2000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MP194", input, expected);
	}

	protected static void Case194(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1157234688); // 2000
		input.m_Profile.m_fA = t.F(1082130432); // 4
		input.m_Profile.m_fB = t.F(-1082130432); // -1
		input.m_Profile.m_fC = t.F(1048576000); // 0.25
		input.m_Profile.m_fD = t.F(-1073741824); // -2
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1073741824); // 2.0
		expected.m_Material.m_fConductivitySm = t.F(1031798784); // 0.0625
		expected.m_Material.m_fMinMHz = t.F(1157234688); // 2000.0
		expected.m_Material.m_fMaxMHz = t.F(1157234688); // 2000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MP195", input, expected);
	}

	protected static void Case195(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1157234688); // 2000
		input.m_Profile.m_fA = t.F(1065353216); // 1
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(0); // 0
		input.m_Profile.m_fD = t.F(2137108966); // 3e+38
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1065353216); // 1.0
		expected.m_Material.m_fConductivitySm = t.F(0); // 0.0
		expected.m_Material.m_fMinMHz = t.F(1157234688); // 2000.0
		expected.m_Material.m_fMaxMHz = t.F(1157234688); // 2000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MP196", input, expected);
	}

	protected static void Case196(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fA = t.F(1082130432); // 4
		input.m_Profile.m_fB = t.F(2137108966); // 3e+38
		input.m_Profile.m_fC = t.F(1048576000); // 0.25
		input.m_Profile.m_fD = t.F(-10374682); // -3e+38
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		expected.m_Material.m_fConductivitySm = t.F(1048576000); // 0.25
		expected.m_Material.m_fMinMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_fMaxMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MP197", input, expected);
	}

	protected static void Case197(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fA = t.F(1082130432); // 4
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1048576000); // 0.25
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1148846080); // 1000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		expected.m_Material.m_fConductivitySm = t.F(1048576000); // 0.25
		expected.m_Material.m_fMinMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_fMaxMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MP198", input, expected);
	}

	protected static void Case198(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fA = t.F(1082130432); // 4
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1048576000); // 0.25
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fMaxMHz = t.F(1148846080); // 1000
		input.m_Profile.m_bMaxInclusive = false;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.INVALID_PROFILE);
		t.CheckCase("MP199", input, expected);
	}

	protected static void Case199(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1056964608); // 0.5
		input.m_Profile.m_fA = t.F(1082130432); // 4
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1048576000); // 0.25
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP200", input, expected);
	}

	protected static void Case200(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1223959552); // 500000
		input.m_Profile.m_fA = t.F(1082130432); // 4
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1048576000); // 0.25
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(1232348160); // 1000000.0
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		t.CheckCase("MP201", input, expected);
	}

	protected static void Case201(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fA = t.F(1120403456); // 100
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1287568416); // 100000000.0
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1120403456); // 100.0
		expected.m_Material.m_fConductivitySm = t.F(1287568416); // 100000000.0
		expected.m_Material.m_fMinMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_fMaxMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MP202", input, expected);
	}

	protected static void Case202(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fA = t.F(1056964608); // 0.5
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1048576000); // 0.25
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.NUMERIC_DOMAIN_LIMIT);
		t.CheckCase("MP203", input, expected);
	}

	protected static void Case203(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fA = t.F(1120403457); // 100.00000762939453
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1048576000); // 0.25
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.NUMERIC_DOMAIN_LIMIT);
		t.CheckCase("MP204", input, expected);
	}

	protected static void Case204(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fA = t.F(1082130432); // 4
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1287568417); // 100000008.0
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.NUMERIC_DOMAIN_LIMIT);
		t.CheckCase("MP205", input, expected);
	}

	protected static void Case205(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fA = t.F(1082130432); // 4
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1); // 1.401298464324817e-45
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		expected.m_Material.m_fConductivitySm = t.F(1); // 1.401298464324817e-45
		expected.m_Material.m_fMinMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_fMaxMHz = t.F(1148846080); // 1000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MP206", input, expected);
	}

	protected static void Case206(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1065353216); // 1
		input.m_Profile.m_fA = t.F(1082130432); // 4
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(228737632); // 1e-30
		input.m_Profile.m_fD = t.F(1120403456); // 100
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.NUMERIC_FAILURE);
		t.CheckCase("MP207", input, expected);
	}

	protected static void Case207(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fA = t.F(1082130432); // 4
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1065353216); // 1
		input.m_Profile.m_fD = t.F(1120403456); // 100
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.NUMERIC_FAILURE);
		t.CheckCase("MP208", input, expected);
	}

	protected static void Case208(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1065353216); // 1
		input.m_Profile.m_fA = t.F(1065353216); // 1
		input.m_Profile.m_fB = t.F(1120403456); // 100
		input.m_Profile.m_fC = t.F(1048576000); // 0.25
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.NUMERIC_FAILURE);
		t.CheckCase("MP209", input, expected);
	}

	protected static void Case209(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fA = t.F(1065353216); // 1
		input.m_Profile.m_fB = t.F(1120403456); // 100
		input.m_Profile.m_fC = t.F(1048576000); // 0.25
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.NUMERIC_FAILURE);
		t.CheckCase("MP210", input, expected);
	}

	protected static void Case210(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1205262336); // 110000
		input.m_Profile.m_fA = t.F(7136238); // 1e-38
		input.m_Profile.m_fB = t.F(1100480512); // 19
		input.m_Profile.m_fC = t.F(1048576000); // 0.25
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1086567814); // 6.115908622741699
		expected.m_Material.m_fConductivitySm = t.F(1048576000); // 0.25
		expected.m_Material.m_fMinMHz = t.F(1205262336); // 110000.0
		expected.m_Material.m_fMaxMHz = t.F(1205262336); // 110000.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MP211", input, expected);
	}

	protected static void Case211(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1203982336); // 100000
		input.m_Profile.m_fA = t.F(1056964608); // 0.5
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1048576000); // 0.25
		input.m_Profile.m_fD = t.F(1120403456); // 100
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.NUMERIC_FAILURE);
		t.CheckCase("MP212", input, expected);
	}

	protected static void Case212(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1056964608); // 0.5
		input.m_Profile.m_fA = t.F(0); // 0
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1048576000); // 0.25
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.INVALID_PROFILE);
		t.CheckCase("MP213", input, expected);
	}

	protected static void Case213(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(0); // 0
		input.m_Profile.m_fA = t.F(0); // 0
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1048576000); // 0.25
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.INVALID_INPUT);
		t.CheckCase("MP214", input, expected);
	}

	protected static void Case214(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fA = t.F(0); // 0
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1048576000); // 0.25
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.INVALID_PROFILE);
		t.CheckCase("MP215", input, expected);
	}

	protected static void Case215(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fA = t.F(1082130432); // 4
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(-1082130432); // -1
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.INVALID_PROFILE);
		t.CheckCase("MP216", input, expected);
	}

	protected static void Case216(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fA = t.F(1082130432); // 4
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1048576000); // 0.25
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(0); // 0
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.INVALID_PROFILE);
		t.CheckCase("MP217", input, expected);
	}

	protected static void Case217(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fA = t.F(1082130432); // 4
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1048576000); // 0.25
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(0); // 0
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.INVALID_PROFILE);
		t.CheckCase("MP218", input, expected);
	}

	protected static void Case218(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fA = t.F(1082130432); // 4
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1048576000); // 0.25
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_MaterialProfileRun.BAD_ENUM; // 'UNKNOWN'
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.INVALID_PROFILE);
		t.CheckCase("MP219", input, expected);
	}

	protected static void Case219(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846080); // 1000
		input.m_Profile.m_fA = t.F(1082130432); // 4
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(1048576000); // 0.25
		input.m_Profile.m_fD = t.F(0); // 0
		input.m_Profile.m_fMinMHz = t.F(1157234688); // 2000
		input.m_Profile.m_fMaxMHz = t.F(1148846080); // 1000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.INVALID_PROFILE);
		t.CheckCase("MP220", input, expected);
	}

	protected static void Case220(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1148846080); // 1000
		input.m_Profile = null;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.INVALID_PROFILE);
		t.CheckCase("MP221", input, expected);
	}

	protected static void Case221(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100
		input.m_Profile.m_fA = t.F(1082130432); // 4
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(228737632); // 1e-30
		input.m_Profile.m_fD = t.F(1092616192); // 10
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.OK);
		expected.m_Material.m_fEpsilonReal = t.F(1082130432); // 4.0
		expected.m_Material.m_fConductivitySm = t.F(71362); // 9.99994610111476e-41
		expected.m_Material.m_fMinMHz = t.F(1120403456); // 100.0
		expected.m_Material.m_fMaxMHz = t.F(1120403456); // 100.0
		expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		t.CheckCase("MP235", input, expected);
	}

	protected static void Case222(RICON_MaterialProfileRun t)
	{
		RICON_MaterialProfileInput input = t.NewInput();
		input.m_fFrequencyMHz = t.F(1120403456); // 100
		input.m_Profile.m_fA = t.F(1082130432); // 4
		input.m_Profile.m_fB = t.F(0); // 0
		input.m_Profile.m_fC = t.F(228737632); // 1e-30
		input.m_Profile.m_fD = t.F(1098907648); // 16
		input.m_Profile.m_fMinMHz = t.F(1065353216); // 1
		input.m_Profile.m_fMaxMHz = t.F(1222359552); // 450000
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialProfileResult expected = t.NewExpected(RICON_EMaterialProfileCode.NUMERIC_FAILURE);
		t.CheckCase("MP236", input, expected);
	}

	static void Run(RICON_MaterialProfileRun t)
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
		Case71(t);
		Case72(t);
		Case73(t);
		Case74(t);
		Case75(t);
		Case76(t);
		Case77(t);
		Case78(t);
		Case79(t);
		Case80(t);
		Case81(t);
		Case82(t);
		Case83(t);
		Case84(t);
		Case85(t);
		Case86(t);
		Case87(t);
		Case88(t);
		Case89(t);
		Case90(t);
		Case91(t);
		Case92(t);
		Case93(t);
		Case94(t);
		Case95(t);
		Case96(t);
		Case97(t);
		Case98(t);
		Case99(t);
		Case100(t);
		Case101(t);
		Case102(t);
		Case103(t);
		Case104(t);
		Case105(t);
		Case106(t);
		Case107(t);
		Case108(t);
		Case109(t);
		Case110(t);
		Case111(t);
		Case112(t);
		Case113(t);
		Case114(t);
		Case115(t);
		Case116(t);
		Case117(t);
		Case118(t);
		Case119(t);
		Case120(t);
		Case121(t);
		Case122(t);
		Case123(t);
		Case124(t);
		Case125(t);
		Case126(t);
		Case127(t);
		Case128(t);
		Case129(t);
		Case130(t);
		Case131(t);
		Case132(t);
		Case133(t);
		Case134(t);
		Case135(t);
		Case136(t);
		Case137(t);
		Case138(t);
		Case139(t);
		Case140(t);
		Case141(t);
		Case142(t);
		Case143(t);
		Case144(t);
		Case145(t);
		Case146(t);
		Case147(t);
		Case148(t);
		Case149(t);
		Case150(t);
		Case151(t);
		Case152(t);
		Case153(t);
		Case154(t);
		Case155(t);
		Case156(t);
		Case157(t);
		Case158(t);
		Case159(t);
		Case160(t);
		Case161(t);
		Case162(t);
		Case163(t);
		Case164(t);
		Case165(t);
		Case166(t);
		Case167(t);
		Case168(t);
		Case169(t);
		Case170(t);
		Case171(t);
		Case172(t);
		Case173(t);
		Case174(t);
		Case175(t);
		Case176(t);
		Case177(t);
		Case178(t);
		Case179(t);
		Case180(t);
		Case181(t);
		Case182(t);
		Case183(t);
		Case184(t);
		Case185(t);
		Case186(t);
		Case187(t);
		Case188(t);
		Case189(t);
		Case190(t);
		Case191(t);
		Case192(t);
		Case193(t);
		Case194(t);
		Case195(t);
		Case196(t);
		Case197(t);
		Case198(t);
		Case199(t);
		Case200(t);
		Case201(t);
		Case202(t);
		Case203(t);
		Case204(t);
		Case205(t);
		Case206(t);
		Case207(t);
		Case208(t);
		Case209(t);
		Case210(t);
		Case211(t);
		Case212(t);
		Case213(t);
		Case214(t);
		Case215(t);
		Case216(t);
		Case217(t);
		Case218(t);
		Case219(t);
		Case220(t);
		Case221(t);
		Case222(t);
	}
}
