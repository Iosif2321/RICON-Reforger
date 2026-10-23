//! Сгенерировано gen_object_rf_profile_fixture.py; ожидания только копируются; TEST_ONLY.
//! Host-only (недостижимы в типизированном Enforce): R31 — лишнее поле корня, R32 — лишнее поле объекта, R33 — bool вместо ID профиля, R34 — null вместо строки назначения, R35 — bool вместо родителя, R36 — число вместо назначения, R41 — число вместо родителя.
class RICON_ObjectRfProfileFixtures
{
	static const string SOURCE_SHA = "b0b93be3238b49c223c859ac14e22f045ae708ce03f1cae2ed731eaf0531e4c5";
	static const int CASES = 40;
	static const int HOST_ONLY = 7;

	protected static void Case0(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "rf.metal";
		input.m_Object.m_sSurfaceProfileId = "rf.wood";
		input.m_Object.m_sPrefabId = "mod.shed";
		input.m_Object.m_sClassId = "ModShed";
		input.m_Object.m_sBallisticHint = "ballistic.metal";
		input.m_aProfiles.Insert("rf.wood");
		input.m_aProfiles.Insert("rf.metal");
		input.m_aProfiles.Insert("rf.concrete");
		input.m_aProfiles.Insert("rf.glass");
		t.AddNode(input.m_aPrefabParents, "mod.shed", true, "base.shed");
		t.AddNode(input.m_aPrefabParents, "base.shed", true, "base.entity");
		t.AddNode(input.m_aPrefabParents, "base.entity", false, "");
		t.AddNode(input.m_aClassParents, "ModShed", true, "Building");
		t.AddNode(input.m_aClassParents, "Building", true, "Entity");
		t.AddNode(input.m_aClassParents, "Entity", false, "");
		t.AddRule(input.m_aPrefabRules, "base.shed", "rf.wood");
		t.AddRule(input.m_aPrefabRules, "base.entity", "rf.metal");
		t.AddRule(input.m_aClassRules, "Building", "rf.concrete");
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.RESOLVED;
		expected.m_eKnowledge = RICON_EPathKnowledge.PROFILE;
		expected.m_sMaterialId = "rf.metal";
		expected.m_eSource = RICON_EObjectProfileSource.COMPONENT;
		expected.m_sMatchedKey = "rf.metal";
		t.CheckCase("R01", input, expected);
	}

	protected static void Case1(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "";
		input.m_Object.m_sSurfaceProfileId = "rf.wood";
		input.m_Object.m_sPrefabId = "mod.shed";
		input.m_Object.m_sClassId = "ModShed";
		input.m_Object.m_sBallisticHint = "ballistic.metal";
		input.m_aProfiles.Insert("rf.wood");
		input.m_aProfiles.Insert("rf.metal");
		input.m_aProfiles.Insert("rf.concrete");
		input.m_aProfiles.Insert("rf.glass");
		t.AddNode(input.m_aPrefabParents, "mod.shed", true, "base.shed");
		t.AddNode(input.m_aPrefabParents, "base.shed", true, "base.entity");
		t.AddNode(input.m_aPrefabParents, "base.entity", false, "");
		t.AddNode(input.m_aClassParents, "ModShed", true, "Building");
		t.AddNode(input.m_aClassParents, "Building", true, "Entity");
		t.AddNode(input.m_aClassParents, "Entity", false, "");
		t.AddRule(input.m_aPrefabRules, "base.shed", "rf.wood");
		t.AddRule(input.m_aPrefabRules, "base.entity", "rf.metal");
		t.AddRule(input.m_aClassRules, "Building", "rf.concrete");
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.RESOLVED;
		expected.m_eKnowledge = RICON_EPathKnowledge.PROFILE;
		expected.m_sMaterialId = "rf.wood";
		expected.m_eSource = RICON_EObjectProfileSource.MATERIAL;
		expected.m_sMatchedKey = "rf.wood";
		t.CheckCase("R02", input, expected);
	}

	protected static void Case2(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "";
		input.m_Object.m_sSurfaceProfileId = "";
		input.m_Object.m_sPrefabId = "mod.shed";
		input.m_Object.m_sClassId = "ModShed";
		input.m_Object.m_sBallisticHint = "ballistic.metal";
		input.m_aProfiles.Insert("rf.wood");
		input.m_aProfiles.Insert("rf.metal");
		input.m_aProfiles.Insert("rf.concrete");
		input.m_aProfiles.Insert("rf.glass");
		t.AddNode(input.m_aPrefabParents, "mod.shed", true, "base.shed");
		t.AddNode(input.m_aPrefabParents, "base.shed", true, "base.entity");
		t.AddNode(input.m_aPrefabParents, "base.entity", false, "");
		t.AddNode(input.m_aClassParents, "ModShed", true, "Building");
		t.AddNode(input.m_aClassParents, "Building", true, "Entity");
		t.AddNode(input.m_aClassParents, "Entity", false, "");
		t.AddRule(input.m_aPrefabRules, "base.shed", "rf.wood");
		t.AddRule(input.m_aPrefabRules, "base.entity", "rf.metal");
		t.AddRule(input.m_aPrefabRules, "mod.shed", "rf.glass");
		t.AddRule(input.m_aClassRules, "Building", "rf.concrete");
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.RESOLVED;
		expected.m_eKnowledge = RICON_EPathKnowledge.RULE;
		expected.m_sMaterialId = "rf.glass";
		expected.m_eSource = RICON_EObjectProfileSource.PREFAB;
		expected.m_sMatchedKey = "mod.shed";
		t.CheckCase("R03", input, expected);
	}

	protected static void Case3(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "";
		input.m_Object.m_sSurfaceProfileId = "";
		input.m_Object.m_sPrefabId = "mod.shed";
		input.m_Object.m_sClassId = "ModShed";
		input.m_Object.m_sBallisticHint = "ballistic.metal";
		input.m_aProfiles.Insert("rf.wood");
		input.m_aProfiles.Insert("rf.metal");
		input.m_aProfiles.Insert("rf.concrete");
		input.m_aProfiles.Insert("rf.glass");
		t.AddNode(input.m_aPrefabParents, "mod.shed", true, "base.shed");
		t.AddNode(input.m_aPrefabParents, "base.shed", true, "base.entity");
		t.AddNode(input.m_aPrefabParents, "base.entity", false, "");
		t.AddNode(input.m_aClassParents, "ModShed", true, "Building");
		t.AddNode(input.m_aClassParents, "Building", true, "Entity");
		t.AddNode(input.m_aClassParents, "Entity", false, "");
		t.AddRule(input.m_aPrefabRules, "base.shed", "rf.wood");
		t.AddRule(input.m_aPrefabRules, "base.entity", "rf.metal");
		t.AddRule(input.m_aClassRules, "Building", "rf.concrete");
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.RESOLVED;
		expected.m_eKnowledge = RICON_EPathKnowledge.RULE;
		expected.m_sMaterialId = "rf.wood";
		expected.m_eSource = RICON_EObjectProfileSource.PREFAB;
		expected.m_sMatchedKey = "base.shed";
		t.CheckCase("R04", input, expected);
	}

	protected static void Case4(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "";
		input.m_Object.m_sSurfaceProfileId = "";
		input.m_Object.m_sPrefabId = "mod.shed";
		input.m_Object.m_sClassId = "ModShed";
		input.m_Object.m_sBallisticHint = "ballistic.metal";
		input.m_aProfiles.Insert("rf.wood");
		input.m_aProfiles.Insert("rf.metal");
		input.m_aProfiles.Insert("rf.concrete");
		input.m_aProfiles.Insert("rf.glass");
		t.AddNode(input.m_aPrefabParents, "mod.shed", true, "base.shed");
		t.AddNode(input.m_aPrefabParents, "base.shed", true, "base.entity");
		t.AddNode(input.m_aPrefabParents, "base.entity", false, "");
		t.AddNode(input.m_aClassParents, "ModShed", true, "Building");
		t.AddNode(input.m_aClassParents, "Building", true, "Entity");
		t.AddNode(input.m_aClassParents, "Entity", false, "");
		t.AddRule(input.m_aPrefabRules, "base.entity", "rf.metal");
		t.AddRule(input.m_aClassRules, "Building", "rf.concrete");
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.RESOLVED;
		expected.m_eKnowledge = RICON_EPathKnowledge.RULE;
		expected.m_sMaterialId = "rf.metal";
		expected.m_eSource = RICON_EObjectProfileSource.PREFAB;
		expected.m_sMatchedKey = "base.entity";
		t.CheckCase("R05", input, expected);
	}

	protected static void Case5(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "";
		input.m_Object.m_sSurfaceProfileId = "";
		input.m_Object.m_sPrefabId = "mod.shed";
		input.m_Object.m_sClassId = "ModShed";
		input.m_Object.m_sBallisticHint = "ballistic.metal";
		input.m_aProfiles.Insert("rf.wood");
		input.m_aProfiles.Insert("rf.metal");
		input.m_aProfiles.Insert("rf.concrete");
		input.m_aProfiles.Insert("rf.glass");
		t.AddNode(input.m_aPrefabParents, "mod.shed", true, "base.shed");
		t.AddNode(input.m_aPrefabParents, "base.shed", true, "base.entity");
		t.AddNode(input.m_aPrefabParents, "base.entity", false, "");
		t.AddNode(input.m_aClassParents, "ModShed", true, "Building");
		t.AddNode(input.m_aClassParents, "Building", true, "Entity");
		t.AddNode(input.m_aClassParents, "Entity", false, "");
		t.AddRule(input.m_aClassRules, "Building", "rf.concrete");
		t.AddRule(input.m_aClassRules, "ModShed", "rf.glass");
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.RESOLVED;
		expected.m_eKnowledge = RICON_EPathKnowledge.RULE;
		expected.m_sMaterialId = "rf.glass";
		expected.m_eSource = RICON_EObjectProfileSource.CLASS;
		expected.m_sMatchedKey = "ModShed";
		t.CheckCase("R06", input, expected);
	}

	protected static void Case6(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "";
		input.m_Object.m_sSurfaceProfileId = "";
		input.m_Object.m_sPrefabId = "mod.shed";
		input.m_Object.m_sClassId = "ModShed";
		input.m_Object.m_sBallisticHint = "ballistic.metal";
		input.m_aProfiles.Insert("rf.wood");
		input.m_aProfiles.Insert("rf.metal");
		input.m_aProfiles.Insert("rf.concrete");
		input.m_aProfiles.Insert("rf.glass");
		t.AddNode(input.m_aPrefabParents, "mod.shed", true, "base.shed");
		t.AddNode(input.m_aPrefabParents, "base.shed", true, "base.entity");
		t.AddNode(input.m_aPrefabParents, "base.entity", false, "");
		t.AddNode(input.m_aClassParents, "ModShed", true, "Building");
		t.AddNode(input.m_aClassParents, "Building", true, "Entity");
		t.AddNode(input.m_aClassParents, "Entity", false, "");
		t.AddRule(input.m_aClassRules, "Building", "rf.concrete");
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.RESOLVED;
		expected.m_eKnowledge = RICON_EPathKnowledge.RULE;
		expected.m_sMaterialId = "rf.concrete";
		expected.m_eSource = RICON_EObjectProfileSource.CLASS;
		expected.m_sMatchedKey = "Building";
		t.CheckCase("R07", input, expected);
	}

	protected static void Case7(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "";
		input.m_Object.m_sSurfaceProfileId = "";
		input.m_Object.m_sPrefabId = "mod.shed";
		input.m_Object.m_sClassId = "ModShed";
		input.m_Object.m_sBallisticHint = "ballistic.metal";
		input.m_aProfiles.Insert("rf.wood");
		input.m_aProfiles.Insert("rf.metal");
		input.m_aProfiles.Insert("rf.concrete");
		input.m_aProfiles.Insert("rf.glass");
		t.AddNode(input.m_aPrefabParents, "mod.shed", true, "base.shed");
		t.AddNode(input.m_aPrefabParents, "base.shed", true, "base.entity");
		t.AddNode(input.m_aPrefabParents, "base.entity", false, "");
		t.AddNode(input.m_aClassParents, "ModShed", true, "Building");
		t.AddNode(input.m_aClassParents, "Building", true, "Entity");
		t.AddNode(input.m_aClassParents, "Entity", false, "");
		t.AddRule(input.m_aClassRules, "Entity", "rf.wood");
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.RESOLVED;
		expected.m_eKnowledge = RICON_EPathKnowledge.RULE;
		expected.m_sMaterialId = "rf.wood";
		expected.m_eSource = RICON_EObjectProfileSource.CLASS;
		expected.m_sMatchedKey = "Entity";
		t.CheckCase("R08", input, expected);
	}

	protected static void Case8(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "";
		input.m_Object.m_sSurfaceProfileId = "";
		input.m_Object.m_sPrefabId = "mod.shed";
		input.m_Object.m_sClassId = "ModShed";
		input.m_Object.m_sBallisticHint = "ballistic.metal";
		input.m_aProfiles.Insert("rf.wood");
		input.m_aProfiles.Insert("rf.metal");
		input.m_aProfiles.Insert("rf.concrete");
		input.m_aProfiles.Insert("rf.glass");
		t.AddNode(input.m_aPrefabParents, "mod.shed", true, "base.shed");
		t.AddNode(input.m_aPrefabParents, "base.shed", true, "base.entity");
		t.AddNode(input.m_aPrefabParents, "base.entity", false, "");
		t.AddNode(input.m_aClassParents, "ModShed", true, "Building");
		t.AddNode(input.m_aClassParents, "Building", true, "Entity");
		t.AddNode(input.m_aClassParents, "Entity", false, "");
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.UNCLASSIFIED;
		expected.m_eKnowledge = RICON_EPathKnowledge.HINT_ONLY;
		expected.m_sMaterialId = "";
		expected.m_eSource = RICON_EObjectProfileSource.HINT;
		expected.m_sMatchedKey = "ballistic.metal";
		t.CheckCase("R09", input, expected);
	}

	protected static void Case9(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "";
		input.m_Object.m_sSurfaceProfileId = "";
		input.m_Object.m_sPrefabId = "mod.shed";
		input.m_Object.m_sClassId = "ModShed";
		input.m_Object.m_sBallisticHint = "";
		input.m_aProfiles.Insert("rf.wood");
		input.m_aProfiles.Insert("rf.metal");
		input.m_aProfiles.Insert("rf.concrete");
		input.m_aProfiles.Insert("rf.glass");
		t.AddNode(input.m_aPrefabParents, "mod.shed", true, "base.shed");
		t.AddNode(input.m_aPrefabParents, "base.shed", true, "base.entity");
		t.AddNode(input.m_aPrefabParents, "base.entity", false, "");
		t.AddNode(input.m_aClassParents, "ModShed", true, "Building");
		t.AddNode(input.m_aClassParents, "Building", true, "Entity");
		t.AddNode(input.m_aClassParents, "Entity", false, "");
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.UNCLASSIFIED;
		expected.m_eKnowledge = RICON_EPathKnowledge.UNKNOWN;
		expected.m_sMaterialId = "";
		expected.m_eSource = RICON_EObjectProfileSource.NONE;
		expected.m_sMatchedKey = "";
		t.CheckCase("R10", input, expected);
	}

	protected static void Case10(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "rf.absent";
		input.m_Object.m_sSurfaceProfileId = "rf.wood";
		input.m_Object.m_sPrefabId = "mod.shed";
		input.m_Object.m_sClassId = "ModShed";
		input.m_Object.m_sBallisticHint = "ballistic.metal";
		input.m_aProfiles.Insert("rf.wood");
		input.m_aProfiles.Insert("rf.metal");
		input.m_aProfiles.Insert("rf.concrete");
		input.m_aProfiles.Insert("rf.glass");
		t.AddNode(input.m_aPrefabParents, "mod.shed", true, "base.shed");
		t.AddNode(input.m_aPrefabParents, "base.shed", true, "base.entity");
		t.AddNode(input.m_aPrefabParents, "base.entity", false, "");
		t.AddNode(input.m_aClassParents, "ModShed", true, "Building");
		t.AddNode(input.m_aClassParents, "Building", true, "Entity");
		t.AddNode(input.m_aClassParents, "Entity", false, "");
		t.AddRule(input.m_aPrefabRules, "base.shed", "rf.wood");
		t.AddRule(input.m_aPrefabRules, "base.entity", "rf.metal");
		t.AddRule(input.m_aClassRules, "Building", "rf.concrete");
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.UNRESOLVED_PROFILE;
		expected.m_eKnowledge = RICON_EPathKnowledge.UNKNOWN;
		expected.m_sMaterialId = "";
		expected.m_eSource = RICON_EObjectProfileSource.COMPONENT;
		expected.m_sMatchedKey = "rf.absent";
		t.CheckCase("R11", input, expected);
	}

	protected static void Case11(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "";
		input.m_Object.m_sSurfaceProfileId = "rf.absent";
		input.m_Object.m_sPrefabId = "mod.shed";
		input.m_Object.m_sClassId = "ModShed";
		input.m_Object.m_sBallisticHint = "ballistic.metal";
		input.m_aProfiles.Insert("rf.wood");
		input.m_aProfiles.Insert("rf.metal");
		input.m_aProfiles.Insert("rf.concrete");
		input.m_aProfiles.Insert("rf.glass");
		t.AddNode(input.m_aPrefabParents, "mod.shed", true, "base.shed");
		t.AddNode(input.m_aPrefabParents, "base.shed", true, "base.entity");
		t.AddNode(input.m_aPrefabParents, "base.entity", false, "");
		t.AddNode(input.m_aClassParents, "ModShed", true, "Building");
		t.AddNode(input.m_aClassParents, "Building", true, "Entity");
		t.AddNode(input.m_aClassParents, "Entity", false, "");
		t.AddRule(input.m_aPrefabRules, "base.shed", "rf.wood");
		t.AddRule(input.m_aPrefabRules, "base.entity", "rf.metal");
		t.AddRule(input.m_aClassRules, "Building", "rf.concrete");
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.UNRESOLVED_PROFILE;
		expected.m_eKnowledge = RICON_EPathKnowledge.UNKNOWN;
		expected.m_sMaterialId = "";
		expected.m_eSource = RICON_EObjectProfileSource.MATERIAL;
		expected.m_sMatchedKey = "rf.absent";
		t.CheckCase("R12", input, expected);
	}

	protected static void Case12(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "rf.metal";
		input.m_Object.m_sSurfaceProfileId = "rf.absent";
		input.m_Object.m_sPrefabId = "mod.shed";
		input.m_Object.m_sClassId = "ModShed";
		input.m_Object.m_sBallisticHint = "ballistic.metal";
		input.m_aProfiles.Insert("rf.wood");
		input.m_aProfiles.Insert("rf.metal");
		input.m_aProfiles.Insert("rf.concrete");
		input.m_aProfiles.Insert("rf.glass");
		t.AddNode(input.m_aPrefabParents, "mod.shed", true, "base.shed");
		t.AddNode(input.m_aPrefabParents, "base.shed", true, "base.entity");
		t.AddNode(input.m_aPrefabParents, "base.entity", false, "");
		t.AddNode(input.m_aClassParents, "ModShed", true, "Building");
		t.AddNode(input.m_aClassParents, "Building", true, "Entity");
		t.AddNode(input.m_aClassParents, "Entity", false, "");
		t.AddRule(input.m_aPrefabRules, "base.shed", "rf.wood");
		t.AddRule(input.m_aPrefabRules, "base.entity", "rf.metal");
		t.AddRule(input.m_aClassRules, "Building", "rf.concrete");
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.RESOLVED;
		expected.m_eKnowledge = RICON_EPathKnowledge.PROFILE;
		expected.m_sMaterialId = "rf.metal";
		expected.m_eSource = RICON_EObjectProfileSource.COMPONENT;
		expected.m_sMatchedKey = "rf.metal";
		t.CheckCase("R13", input, expected);
	}

	protected static void Case13(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "";
		input.m_Object.m_sSurfaceProfileId = "";
		input.m_Object.m_sPrefabId = "mod.shed";
		input.m_Object.m_sClassId = "ModShed";
		input.m_Object.m_sBallisticHint = "ballistic.metal";
		input.m_aProfiles.Insert("rf.wood");
		input.m_aProfiles.Insert("rf.metal");
		input.m_aProfiles.Insert("rf.concrete");
		input.m_aProfiles.Insert("rf.glass");
		t.AddNode(input.m_aPrefabParents, "base.entity", false, "");
		t.AddNode(input.m_aPrefabParents, "base.shed", true, "base.entity");
		t.AddNode(input.m_aPrefabParents, "mod.shed", true, "base.shed");
		t.AddNode(input.m_aClassParents, "ModShed", true, "Building");
		t.AddNode(input.m_aClassParents, "Building", true, "Entity");
		t.AddNode(input.m_aClassParents, "Entity", false, "");
		t.AddRule(input.m_aPrefabRules, "base.entity", "rf.metal");
		t.AddRule(input.m_aPrefabRules, "base.shed", "rf.wood");
		t.AddRule(input.m_aClassRules, "Building", "rf.concrete");
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.RESOLVED;
		expected.m_eKnowledge = RICON_EPathKnowledge.RULE;
		expected.m_sMaterialId = "rf.wood";
		expected.m_eSource = RICON_EObjectProfileSource.PREFAB;
		expected.m_sMatchedKey = "base.shed";
		t.CheckCase("R14", input, expected);
	}

	protected static void Case14(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "";
		input.m_Object.m_sSurfaceProfileId = "";
		input.m_Object.m_sPrefabId = "";
		input.m_Object.m_sClassId = "";
		input.m_Object.m_sBallisticHint = "";
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.UNCLASSIFIED;
		expected.m_eKnowledge = RICON_EPathKnowledge.UNKNOWN;
		expected.m_sMaterialId = "";
		expected.m_eSource = RICON_EObjectProfileSource.NONE;
		expected.m_sMatchedKey = "";
		t.CheckCase("R15", input, expected);
	}

	protected static void Case15(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "rf.metal";
		input.m_Object.m_sSurfaceProfileId = "rf.wood";
		input.m_Object.m_sPrefabId = "";
		input.m_Object.m_sClassId = "";
		input.m_Object.m_sBallisticHint = "ballistic.metal";
		input.m_aProfiles.Insert("rf.wood");
		input.m_aProfiles.Insert("rf.metal");
		input.m_aProfiles.Insert("rf.concrete");
		input.m_aProfiles.Insert("rf.glass");
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.RESOLVED;
		expected.m_eKnowledge = RICON_EPathKnowledge.PROFILE;
		expected.m_sMaterialId = "rf.metal";
		expected.m_eSource = RICON_EObjectProfileSource.COMPONENT;
		expected.m_sMatchedKey = "rf.metal";
		t.CheckCase("R16", input, expected);
	}

	protected static void Case16(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "";
		input.m_Object.m_sSurfaceProfileId = "";
		input.m_Object.m_sPrefabId = "mod.shed.extra";
		input.m_Object.m_sClassId = "ModShed";
		input.m_Object.m_sBallisticHint = "ballistic.metal";
		input.m_aProfiles.Insert("rf.wood");
		input.m_aProfiles.Insert("rf.metal");
		input.m_aProfiles.Insert("rf.concrete");
		input.m_aProfiles.Insert("rf.glass");
		t.AddNode(input.m_aPrefabParents, "mod.shed", true, "base.shed");
		t.AddNode(input.m_aPrefabParents, "base.shed", true, "base.entity");
		t.AddNode(input.m_aPrefabParents, "base.entity", false, "");
		t.AddNode(input.m_aPrefabParents, "mod.shed.extra", true, "base.entity");
		t.AddNode(input.m_aClassParents, "ModShed", true, "Building");
		t.AddNode(input.m_aClassParents, "Building", true, "Entity");
		t.AddNode(input.m_aClassParents, "Entity", false, "");
		t.AddRule(input.m_aPrefabRules, "mod.shed", "rf.glass");
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.UNCLASSIFIED;
		expected.m_eKnowledge = RICON_EPathKnowledge.HINT_ONLY;
		expected.m_sMaterialId = "";
		expected.m_eSource = RICON_EObjectProfileSource.HINT;
		expected.m_sMatchedKey = "ballistic.metal";
		t.CheckCase("R17", input, expected);
	}

	protected static void Case17(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "";
		input.m_Object.m_sSurfaceProfileId = "";
		input.m_Object.m_sPrefabId = "mod.shed";
		input.m_Object.m_sClassId = "modshed";
		input.m_Object.m_sBallisticHint = "ballistic.metal";
		input.m_aProfiles.Insert("rf.wood");
		input.m_aProfiles.Insert("rf.metal");
		input.m_aProfiles.Insert("rf.concrete");
		input.m_aProfiles.Insert("rf.glass");
		t.AddNode(input.m_aPrefabParents, "mod.shed", true, "base.shed");
		t.AddNode(input.m_aPrefabParents, "base.shed", true, "base.entity");
		t.AddNode(input.m_aPrefabParents, "base.entity", false, "");
		t.AddNode(input.m_aClassParents, "ModShed", true, "Building");
		t.AddNode(input.m_aClassParents, "Building", true, "Entity");
		t.AddNode(input.m_aClassParents, "Entity", false, "");
		t.AddNode(input.m_aClassParents, "modshed", true, "Entity");
		t.AddRule(input.m_aClassRules, "ModShed", "rf.glass");
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.UNCLASSIFIED;
		expected.m_eKnowledge = RICON_EPathKnowledge.HINT_ONLY;
		expected.m_sMaterialId = "";
		expected.m_eSource = RICON_EObjectProfileSource.HINT;
		expected.m_sMatchedKey = "ballistic.metal";
		t.CheckCase("R18", input, expected);
	}

	protected static void Case18(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
		input.m_Object.m_sSurfaceProfileId = "rf.wood";
		input.m_Object.m_sPrefabId = "mod.shed";
		input.m_Object.m_sClassId = "ModShed";
		input.m_Object.m_sBallisticHint = "ballistic.metal";
		input.m_aProfiles.Insert("rf.wood");
		input.m_aProfiles.Insert("rf.metal");
		input.m_aProfiles.Insert("rf.concrete");
		input.m_aProfiles.Insert("rf.glass");
		input.m_aProfiles.Insert("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
		t.AddNode(input.m_aPrefabParents, "mod.shed", true, "base.shed");
		t.AddNode(input.m_aPrefabParents, "base.shed", true, "base.entity");
		t.AddNode(input.m_aPrefabParents, "base.entity", false, "");
		t.AddNode(input.m_aClassParents, "ModShed", true, "Building");
		t.AddNode(input.m_aClassParents, "Building", true, "Entity");
		t.AddNode(input.m_aClassParents, "Entity", false, "");
		t.AddRule(input.m_aPrefabRules, "base.shed", "rf.wood");
		t.AddRule(input.m_aPrefabRules, "base.entity", "rf.metal");
		t.AddRule(input.m_aClassRules, "Building", "rf.concrete");
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.RESOLVED;
		expected.m_eKnowledge = RICON_EPathKnowledge.PROFILE;
		expected.m_sMaterialId = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
		expected.m_eSource = RICON_EObjectProfileSource.COMPONENT;
		expected.m_sMatchedKey = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
		t.CheckCase("R19", input, expected);
	}

	protected static void Case19(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "rf.metal";
		input.m_Object.m_sSurfaceProfileId = "rf.wood";
		input.m_Object.m_sPrefabId = "mod.shed";
		input.m_Object.m_sClassId = "ModShed";
		input.m_Object.m_sBallisticHint = "ballistic.metal";
		input.m_aProfiles.Insert("rf.wood");
		input.m_aProfiles.Insert("rf.metal");
		input.m_aProfiles.Insert("rf.concrete");
		input.m_aProfiles.Insert("rf.glass");
		input.m_aProfiles.Insert("rf.wood");
		t.AddNode(input.m_aPrefabParents, "mod.shed", true, "base.shed");
		t.AddNode(input.m_aPrefabParents, "base.shed", true, "base.entity");
		t.AddNode(input.m_aPrefabParents, "base.entity", false, "");
		t.AddNode(input.m_aClassParents, "ModShed", true, "Building");
		t.AddNode(input.m_aClassParents, "Building", true, "Entity");
		t.AddNode(input.m_aClassParents, "Entity", false, "");
		t.AddRule(input.m_aPrefabRules, "base.shed", "rf.wood");
		t.AddRule(input.m_aPrefabRules, "base.entity", "rf.metal");
		t.AddRule(input.m_aClassRules, "Building", "rf.concrete");
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.INVALID_INPUT;
		t.CheckCase("R20", input, expected);
	}

	protected static void Case20(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "rf.metal";
		input.m_Object.m_sSurfaceProfileId = "rf.wood";
		input.m_Object.m_sPrefabId = "mod.shed";
		input.m_Object.m_sClassId = "ModShed";
		input.m_Object.m_sBallisticHint = "ballistic.metal";
		input.m_aProfiles.Insert("rf.wood");
		input.m_aProfiles.Insert("rf.metal");
		input.m_aProfiles.Insert("rf.concrete");
		input.m_aProfiles.Insert("rf.glass");
		input.m_aProfiles.Insert("");
		t.AddNode(input.m_aPrefabParents, "mod.shed", true, "base.shed");
		t.AddNode(input.m_aPrefabParents, "base.shed", true, "base.entity");
		t.AddNode(input.m_aPrefabParents, "base.entity", false, "");
		t.AddNode(input.m_aClassParents, "ModShed", true, "Building");
		t.AddNode(input.m_aClassParents, "Building", true, "Entity");
		t.AddNode(input.m_aClassParents, "Entity", false, "");
		t.AddRule(input.m_aPrefabRules, "base.shed", "rf.wood");
		t.AddRule(input.m_aPrefabRules, "base.entity", "rf.metal");
		t.AddRule(input.m_aClassRules, "Building", "rf.concrete");
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.INVALID_INPUT;
		t.CheckCase("R21", input, expected);
	}

	protected static void Case21(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "rf.metal";
		input.m_Object.m_sSurfaceProfileId = "rf.wood";
		input.m_Object.m_sPrefabId = "mod.shed";
		input.m_Object.m_sClassId = "ModShed";
		input.m_Object.m_sBallisticHint = "ballistic.metal";
		input.m_aProfiles.Insert("rf.wood");
		input.m_aProfiles.Insert("rf.metal");
		input.m_aProfiles.Insert("rf.concrete");
		input.m_aProfiles.Insert("rf.glass");
		t.AddNode(input.m_aPrefabParents, "mod.shed", true, "base.shed");
		t.AddNode(input.m_aPrefabParents, "base.shed", true, "base.entity");
		t.AddNode(input.m_aPrefabParents, "base.entity", true, "mod.shed");
		t.AddNode(input.m_aClassParents, "ModShed", true, "Building");
		t.AddNode(input.m_aClassParents, "Building", true, "Entity");
		t.AddNode(input.m_aClassParents, "Entity", false, "");
		t.AddRule(input.m_aPrefabRules, "base.shed", "rf.wood");
		t.AddRule(input.m_aPrefabRules, "base.entity", "rf.metal");
		t.AddRule(input.m_aClassRules, "Building", "rf.concrete");
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.INVALID_INPUT;
		t.CheckCase("R22", input, expected);
	}

	protected static void Case22(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "rf.metal";
		input.m_Object.m_sSurfaceProfileId = "rf.wood";
		input.m_Object.m_sPrefabId = "mod.shed";
		input.m_Object.m_sClassId = "ModShed";
		input.m_Object.m_sBallisticHint = "ballistic.metal";
		input.m_aProfiles.Insert("rf.wood");
		input.m_aProfiles.Insert("rf.metal");
		input.m_aProfiles.Insert("rf.concrete");
		input.m_aProfiles.Insert("rf.glass");
		t.AddNode(input.m_aPrefabParents, "mod.shed", true, "base.shed");
		t.AddNode(input.m_aPrefabParents, "base.shed", true, "base.entity");
		t.AddNode(input.m_aPrefabParents, "base.entity", false, "");
		t.AddNode(input.m_aPrefabParents, "unused.a", true, "unused.b");
		t.AddNode(input.m_aPrefabParents, "unused.b", true, "unused.a");
		t.AddNode(input.m_aClassParents, "ModShed", true, "Building");
		t.AddNode(input.m_aClassParents, "Building", true, "Entity");
		t.AddNode(input.m_aClassParents, "Entity", false, "");
		t.AddRule(input.m_aPrefabRules, "base.shed", "rf.wood");
		t.AddRule(input.m_aPrefabRules, "base.entity", "rf.metal");
		t.AddRule(input.m_aClassRules, "Building", "rf.concrete");
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.INVALID_INPUT;
		t.CheckCase("R23", input, expected);
	}

	protected static void Case23(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "rf.metal";
		input.m_Object.m_sSurfaceProfileId = "rf.wood";
		input.m_Object.m_sPrefabId = "mod.shed";
		input.m_Object.m_sClassId = "ModShed";
		input.m_Object.m_sBallisticHint = "ballistic.metal";
		input.m_aProfiles.Insert("rf.wood");
		input.m_aProfiles.Insert("rf.metal");
		input.m_aProfiles.Insert("rf.concrete");
		input.m_aProfiles.Insert("rf.glass");
		t.AddNode(input.m_aPrefabParents, "mod.shed", true, "base.shed");
		t.AddNode(input.m_aPrefabParents, "base.shed", true, "base.entity");
		t.AddNode(input.m_aPrefabParents, "base.entity", false, "");
		t.AddNode(input.m_aPrefabParents, "unused", true, "absent");
		t.AddNode(input.m_aClassParents, "ModShed", true, "Building");
		t.AddNode(input.m_aClassParents, "Building", true, "Entity");
		t.AddNode(input.m_aClassParents, "Entity", false, "");
		t.AddRule(input.m_aPrefabRules, "base.shed", "rf.wood");
		t.AddRule(input.m_aPrefabRules, "base.entity", "rf.metal");
		t.AddRule(input.m_aClassRules, "Building", "rf.concrete");
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.INVALID_INPUT;
		t.CheckCase("R24", input, expected);
	}

	protected static void Case24(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "rf.metal";
		input.m_Object.m_sSurfaceProfileId = "rf.wood";
		input.m_Object.m_sPrefabId = "mod.shed";
		input.m_Object.m_sClassId = "ModShed";
		input.m_Object.m_sBallisticHint = "ballistic.metal";
		input.m_aProfiles.Insert("rf.wood");
		input.m_aProfiles.Insert("rf.metal");
		input.m_aProfiles.Insert("rf.concrete");
		input.m_aProfiles.Insert("rf.glass");
		t.AddNode(input.m_aPrefabParents, "mod.shed", true, "base.shed");
		t.AddNode(input.m_aPrefabParents, "base.shed", true, "base.entity");
		t.AddNode(input.m_aPrefabParents, "base.entity", false, "");
		t.AddNode(input.m_aClassParents, "ModShed", true, "Building");
		t.AddNode(input.m_aClassParents, "Building", true, "Entity");
		t.AddNode(input.m_aClassParents, "Entity", false, "");
		t.AddNode(input.m_aClassParents, "unused.a", true, "unused.b");
		t.AddNode(input.m_aClassParents, "unused.b", true, "unused.a");
		t.AddRule(input.m_aPrefabRules, "base.shed", "rf.wood");
		t.AddRule(input.m_aPrefabRules, "base.entity", "rf.metal");
		t.AddRule(input.m_aClassRules, "Building", "rf.concrete");
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.INVALID_INPUT;
		t.CheckCase("R25", input, expected);
	}

	protected static void Case25(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "rf.metal";
		input.m_Object.m_sSurfaceProfileId = "rf.wood";
		input.m_Object.m_sPrefabId = "absent";
		input.m_Object.m_sClassId = "ModShed";
		input.m_Object.m_sBallisticHint = "ballistic.metal";
		input.m_aProfiles.Insert("rf.wood");
		input.m_aProfiles.Insert("rf.metal");
		input.m_aProfiles.Insert("rf.concrete");
		input.m_aProfiles.Insert("rf.glass");
		t.AddNode(input.m_aPrefabParents, "mod.shed", true, "base.shed");
		t.AddNode(input.m_aPrefabParents, "base.shed", true, "base.entity");
		t.AddNode(input.m_aPrefabParents, "base.entity", false, "");
		t.AddNode(input.m_aClassParents, "ModShed", true, "Building");
		t.AddNode(input.m_aClassParents, "Building", true, "Entity");
		t.AddNode(input.m_aClassParents, "Entity", false, "");
		t.AddRule(input.m_aPrefabRules, "base.shed", "rf.wood");
		t.AddRule(input.m_aPrefabRules, "base.entity", "rf.metal");
		t.AddRule(input.m_aClassRules, "Building", "rf.concrete");
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.INVALID_INPUT;
		t.CheckCase("R26", input, expected);
	}

	protected static void Case26(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "rf.metal";
		input.m_Object.m_sSurfaceProfileId = "rf.wood";
		input.m_Object.m_sPrefabId = "mod.shed";
		input.m_Object.m_sClassId = "absent";
		input.m_Object.m_sBallisticHint = "ballistic.metal";
		input.m_aProfiles.Insert("rf.wood");
		input.m_aProfiles.Insert("rf.metal");
		input.m_aProfiles.Insert("rf.concrete");
		input.m_aProfiles.Insert("rf.glass");
		t.AddNode(input.m_aPrefabParents, "mod.shed", true, "base.shed");
		t.AddNode(input.m_aPrefabParents, "base.shed", true, "base.entity");
		t.AddNode(input.m_aPrefabParents, "base.entity", false, "");
		t.AddNode(input.m_aClassParents, "ModShed", true, "Building");
		t.AddNode(input.m_aClassParents, "Building", true, "Entity");
		t.AddNode(input.m_aClassParents, "Entity", false, "");
		t.AddRule(input.m_aPrefabRules, "base.shed", "rf.wood");
		t.AddRule(input.m_aPrefabRules, "base.entity", "rf.metal");
		t.AddRule(input.m_aClassRules, "Building", "rf.concrete");
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.INVALID_INPUT;
		t.CheckCase("R27", input, expected);
	}

	protected static void Case27(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "rf.metal";
		input.m_Object.m_sSurfaceProfileId = "rf.wood";
		input.m_Object.m_sPrefabId = "mod.shed";
		input.m_Object.m_sClassId = "ModShed";
		input.m_Object.m_sBallisticHint = "ballistic.metal";
		input.m_aProfiles.Insert("rf.wood");
		input.m_aProfiles.Insert("rf.metal");
		input.m_aProfiles.Insert("rf.concrete");
		input.m_aProfiles.Insert("rf.glass");
		t.AddNode(input.m_aPrefabParents, "mod.shed", true, "base.shed");
		t.AddNode(input.m_aPrefabParents, "base.shed", true, "base.entity");
		t.AddNode(input.m_aPrefabParents, "base.entity", false, "");
		t.AddNode(input.m_aClassParents, "ModShed", true, "Building");
		t.AddNode(input.m_aClassParents, "Building", true, "Entity");
		t.AddNode(input.m_aClassParents, "Entity", false, "");
		t.AddRule(input.m_aPrefabRules, "base.shed", "rf.wood");
		t.AddRule(input.m_aPrefabRules, "base.entity", "rf.metal");
		t.AddRule(input.m_aPrefabRules, "absent", "rf.wood");
		t.AddRule(input.m_aClassRules, "Building", "rf.concrete");
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.INVALID_INPUT;
		t.CheckCase("R28", input, expected);
	}

	protected static void Case28(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "rf.metal";
		input.m_Object.m_sSurfaceProfileId = "rf.wood";
		input.m_Object.m_sPrefabId = "mod.shed";
		input.m_Object.m_sClassId = "ModShed";
		input.m_Object.m_sBallisticHint = "ballistic.metal";
		input.m_aProfiles.Insert("rf.wood");
		input.m_aProfiles.Insert("rf.metal");
		input.m_aProfiles.Insert("rf.concrete");
		input.m_aProfiles.Insert("rf.glass");
		t.AddNode(input.m_aPrefabParents, "mod.shed", true, "base.shed");
		t.AddNode(input.m_aPrefabParents, "base.shed", true, "base.entity");
		t.AddNode(input.m_aPrefabParents, "base.entity", false, "");
		t.AddNode(input.m_aClassParents, "ModShed", true, "Building");
		t.AddNode(input.m_aClassParents, "Building", true, "Entity");
		t.AddNode(input.m_aClassParents, "Entity", false, "");
		t.AddRule(input.m_aPrefabRules, "base.shed", "rf.wood");
		t.AddRule(input.m_aPrefabRules, "base.entity", "rf.metal");
		t.AddRule(input.m_aClassRules, "Building", "rf.concrete");
		t.AddRule(input.m_aClassRules, "Entity", "absent");
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.INVALID_INPUT;
		t.CheckCase("R29", input, expected);
	}

	protected static void Case29(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "rf.metal";
		input.m_Object.m_sSurfaceProfileId = "rf.wood";
		input.m_Object.m_sPrefabId = "mod.shed";
		input.m_Object.m_sClassId = "ModShed";
		input.m_Object.m_sBallisticHint = "ballistic.metal";
		input.m_aProfiles.Insert("rf.wood");
		input.m_aProfiles.Insert("rf.metal");
		input.m_aProfiles.Insert("rf.concrete");
		input.m_aProfiles.Insert("rf.glass");
		t.AddNode(input.m_aPrefabParents, "mod.shed", true, "base.shed");
		t.AddNode(input.m_aPrefabParents, "base.shed", true, "base.entity");
		t.AddNode(input.m_aPrefabParents, "base.entity", false, "");
		t.AddNode(input.m_aClassParents, "ModShed", true, "Building");
		t.AddNode(input.m_aClassParents, "Building", true, "Entity");
		t.AddNode(input.m_aClassParents, "Entity", false, "");
		t.AddRule(input.m_aPrefabRules, "base.shed", "rf.wood");
		t.AddRule(input.m_aPrefabRules, "base.entity", "rf.metal");
		t.AddRule(input.m_aPrefabRules, "bad key", "rf.wood");
		t.AddRule(input.m_aClassRules, "Building", "rf.concrete");
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.INVALID_INPUT;
		t.CheckCase("R30", input, expected);
	}

	protected static void Case30(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "rf.metal";
		input.m_Object.m_sSurfaceProfileId = "rf.wood";
		input.m_Object.m_sPrefabId = "mod.shed";
		input.m_Object.m_sClassId = "ModShed";
		input.m_Object.m_sBallisticHint = "ballistic.metal";
		input.m_aProfiles = null;
		t.AddNode(input.m_aPrefabParents, "mod.shed", true, "base.shed");
		t.AddNode(input.m_aPrefabParents, "base.shed", true, "base.entity");
		t.AddNode(input.m_aPrefabParents, "base.entity", false, "");
		t.AddNode(input.m_aClassParents, "ModShed", true, "Building");
		t.AddNode(input.m_aClassParents, "Building", true, "Entity");
		t.AddNode(input.m_aClassParents, "Entity", false, "");
		t.AddRule(input.m_aPrefabRules, "base.shed", "rf.wood");
		t.AddRule(input.m_aPrefabRules, "base.entity", "rf.metal");
		t.AddRule(input.m_aClassRules, "Building", "rf.concrete");
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.INVALID_INPUT;
		t.CheckCase("R37", input, expected);
	}

	protected static void Case31(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object = null;
		input.m_aProfiles.Insert("rf.wood");
		input.m_aProfiles.Insert("rf.metal");
		input.m_aProfiles.Insert("rf.concrete");
		input.m_aProfiles.Insert("rf.glass");
		t.AddNode(input.m_aPrefabParents, "mod.shed", true, "base.shed");
		t.AddNode(input.m_aPrefabParents, "base.shed", true, "base.entity");
		t.AddNode(input.m_aPrefabParents, "base.entity", false, "");
		t.AddNode(input.m_aClassParents, "ModShed", true, "Building");
		t.AddNode(input.m_aClassParents, "Building", true, "Entity");
		t.AddNode(input.m_aClassParents, "Entity", false, "");
		t.AddRule(input.m_aPrefabRules, "base.shed", "rf.wood");
		t.AddRule(input.m_aPrefabRules, "base.entity", "rf.metal");
		t.AddRule(input.m_aClassRules, "Building", "rf.concrete");
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.INVALID_INPUT;
		t.CheckCase("R38", input, expected);
	}

	protected static void Case32(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "rf.metal";
		input.m_Object.m_sSurfaceProfileId = "rf.wood";
		input.m_Object.m_sPrefabId = "mod.shed";
		input.m_Object.m_sClassId = "ModShed";
		input.m_Object.m_sBallisticHint = "ballistic.metal";
		input.m_aProfiles.Insert("rf.wood");
		input.m_aProfiles.Insert("rf.metal");
		input.m_aProfiles.Insert("rf.concrete");
		input.m_aProfiles.Insert("rf.glass");
		input.m_aPrefabParents = null;
		t.AddNode(input.m_aClassParents, "ModShed", true, "Building");
		t.AddNode(input.m_aClassParents, "Building", true, "Entity");
		t.AddNode(input.m_aClassParents, "Entity", false, "");
		t.AddRule(input.m_aPrefabRules, "base.shed", "rf.wood");
		t.AddRule(input.m_aPrefabRules, "base.entity", "rf.metal");
		t.AddRule(input.m_aClassRules, "Building", "rf.concrete");
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.INVALID_INPUT;
		t.CheckCase("R39", input, expected);
	}

	protected static void Case33(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "rf.metal";
		input.m_Object.m_sSurfaceProfileId = "rf.wood";
		input.m_Object.m_sPrefabId = "mod.shed";
		input.m_Object.m_sClassId = "ModShed";
		input.m_Object.m_sBallisticHint = "ballistic.metal";
		input.m_aProfiles.Insert("rf.wood");
		input.m_aProfiles.Insert("rf.metal");
		input.m_aProfiles.Insert("rf.concrete");
		input.m_aProfiles.Insert("rf.glass");
		t.AddNode(input.m_aPrefabParents, "mod.shed", true, "base.shed");
		t.AddNode(input.m_aPrefabParents, "base.shed", true, "base.entity");
		t.AddNode(input.m_aPrefabParents, "base.entity", false, "");
		t.AddNode(input.m_aClassParents, "ModShed", true, "Building");
		t.AddNode(input.m_aClassParents, "Building", true, "Entity");
		t.AddNode(input.m_aClassParents, "Entity", false, "");
		t.AddRule(input.m_aPrefabRules, "base.shed", "rf.wood");
		t.AddRule(input.m_aPrefabRules, "base.entity", "rf.metal");
		input.m_aClassRules = null;
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.INVALID_INPUT;
		t.CheckCase("R40", input, expected);
	}

	protected static void Case34(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
		input.m_Object.m_sSurfaceProfileId = "rf.wood";
		input.m_Object.m_sPrefabId = "mod.shed";
		input.m_Object.m_sClassId = "ModShed";
		input.m_Object.m_sBallisticHint = "ballistic.metal";
		input.m_aProfiles.Insert("rf.wood");
		input.m_aProfiles.Insert("rf.metal");
		input.m_aProfiles.Insert("rf.concrete");
		input.m_aProfiles.Insert("rf.glass");
		t.AddNode(input.m_aPrefabParents, "mod.shed", true, "base.shed");
		t.AddNode(input.m_aPrefabParents, "base.shed", true, "base.entity");
		t.AddNode(input.m_aPrefabParents, "base.entity", false, "");
		t.AddNode(input.m_aClassParents, "ModShed", true, "Building");
		t.AddNode(input.m_aClassParents, "Building", true, "Entity");
		t.AddNode(input.m_aClassParents, "Entity", false, "");
		t.AddRule(input.m_aPrefabRules, "base.shed", "rf.wood");
		t.AddRule(input.m_aPrefabRules, "base.entity", "rf.metal");
		t.AddRule(input.m_aClassRules, "Building", "rf.concrete");
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.INVALID_INPUT;
		t.CheckCase("R42", input, expected);
	}

	protected static void Case35(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input;
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.INVALID_INPUT;
		t.CheckCase("R43", input, expected);
	}

	protected static void Case36(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "rf.metal";
		input.m_Object.m_sSurfaceProfileId = "rf.wood";
		input.m_Object.m_sPrefabId = "mod.shed";
		input.m_Object.m_sClassId = "ModShed";
		input.m_Object.m_sBallisticHint = "ballistic.metal";
		input.m_aProfiles.Insert("rf.wood");
		input.m_aProfiles.Insert("rf.metal");
		input.m_aProfiles.Insert("rf.concrete");
		input.m_aProfiles.Insert("rf.glass");
		t.AddNode(input.m_aPrefabParents, "mod.shed", true, "base.shed");
		t.AddNode(input.m_aPrefabParents, "base.shed", true, "base.entity");
		t.AddNode(input.m_aPrefabParents, "base.entity", false, "");
		t.AddNode(input.m_aClassParents, "ModShed", true, "Building");
		t.AddNode(input.m_aClassParents, "Building", true, "Entity");
		t.AddNode(input.m_aClassParents, "Entity", true, "Entity");
		t.AddRule(input.m_aPrefabRules, "base.shed", "rf.wood");
		t.AddRule(input.m_aPrefabRules, "base.entity", "rf.metal");
		t.AddRule(input.m_aClassRules, "Building", "rf.concrete");
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.INVALID_INPUT;
		t.CheckCase("R44", input, expected);
	}

	protected static void Case37(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "rf.metal";
		input.m_Object.m_sSurfaceProfileId = "rf.wood";
		input.m_Object.m_sPrefabId = "mod.shed";
		input.m_Object.m_sClassId = "ModShed";
		input.m_Object.m_sBallisticHint = "ballistic.metal";
		input.m_aProfiles.Insert("rf.wood");
		input.m_aProfiles.Insert("rf.metal");
		input.m_aProfiles.Insert("rf.concrete");
		input.m_aProfiles.Insert("rf.glass");
		t.AddNode(input.m_aPrefabParents, "mod.shed", true, "base.shed");
		t.AddNode(input.m_aPrefabParents, "base.shed", true, "base.entity");
		t.AddNode(input.m_aPrefabParents, "base.entity", false, "");
		t.AddNode(input.m_aClassParents, "ModShed", true, "Building");
		t.AddNode(input.m_aClassParents, "Building", true, "Entity");
		t.AddNode(input.m_aClassParents, "Entity", true, "");
		t.AddRule(input.m_aPrefabRules, "base.shed", "rf.wood");
		t.AddRule(input.m_aPrefabRules, "base.entity", "rf.metal");
		t.AddRule(input.m_aClassRules, "Building", "rf.concrete");
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.INVALID_INPUT;
		t.CheckCase("R45", input, expected);
	}

	protected static void Case38(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "rf.metal";
		input.m_Object.m_sSurfaceProfileId = "bad key";
		input.m_Object.m_sPrefabId = "mod.shed";
		input.m_Object.m_sClassId = "ModShed";
		input.m_Object.m_sBallisticHint = "ballistic.metal";
		input.m_aProfiles.Insert("rf.wood");
		input.m_aProfiles.Insert("rf.metal");
		input.m_aProfiles.Insert("rf.concrete");
		input.m_aProfiles.Insert("rf.glass");
		t.AddNode(input.m_aPrefabParents, "mod.shed", true, "base.shed");
		t.AddNode(input.m_aPrefabParents, "base.shed", true, "base.entity");
		t.AddNode(input.m_aPrefabParents, "base.entity", false, "");
		t.AddNode(input.m_aClassParents, "ModShed", true, "Building");
		t.AddNode(input.m_aClassParents, "Building", true, "Entity");
		t.AddNode(input.m_aClassParents, "Entity", false, "");
		t.AddRule(input.m_aPrefabRules, "base.shed", "rf.wood");
		t.AddRule(input.m_aPrefabRules, "base.entity", "rf.metal");
		t.AddRule(input.m_aClassRules, "Building", "rf.concrete");
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.INVALID_INPUT;
		t.CheckCase("R46", input, expected);
	}

	protected static void Case39(RICON_ObjectRfProfileRun t)
	{
		RICON_ObjectProfileInput input = t.NewInput();
		input.m_Object.m_sComponentProfileId = "rf.metal";
		input.m_Object.m_sSurfaceProfileId = "rf.wood";
		input.m_Object.m_sPrefabId = "mod.shed";
		input.m_Object.m_sClassId = "ModShed";
		input.m_Object.m_sBallisticHint = "ballistic.metal";
		t.AddNode(input.m_aPrefabParents, "mod.shed", true, "base.shed");
		t.AddNode(input.m_aPrefabParents, "base.shed", true, "base.entity");
		t.AddNode(input.m_aPrefabParents, "base.entity", false, "");
		t.AddNode(input.m_aClassParents, "ModShed", true, "Building");
		t.AddNode(input.m_aClassParents, "Building", true, "Entity");
		t.AddNode(input.m_aClassParents, "Entity", false, "");
		RICON_ObjectProfileResult expected = new RICON_ObjectProfileResult();
		expected.m_eCode = RICON_EObjectProfileCode.UNRESOLVED_PROFILE;
		expected.m_eKnowledge = RICON_EPathKnowledge.UNKNOWN;
		expected.m_sMaterialId = "";
		expected.m_eSource = RICON_EObjectProfileSource.COMPONENT;
		expected.m_sMatchedKey = "rf.metal";
		t.CheckCase("R47", input, expected);
	}

	static void Run(RICON_ObjectRfProfileRun t)
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
	}
}
