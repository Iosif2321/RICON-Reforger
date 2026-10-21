//! Сгенерировано gen_world_profile_fixture.py; TEST_ONLY; ожидания только копируются.
class RICON_WorldProfileFixtures
{
	static const string SOURCE_SHA = "56c6a148236325183af8117d838ed1a22e12316642d9946c3b3ab57be48b5b1e";
	static const int CASES = 39;

	protected static void Case0(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		query.m_vTxPos = Vector(0.0, 101.5, 0.0);
		query.m_vRxPos = Vector(2000.0, 101.5, 0.0);
		RICON_AnalyticHeightSource source;
		source = new RICON_AnalyticHeightSource();
		source.m_fBase = 100.0;
		source.m_fSlopeX = 0.0;
		source.m_fSlopeZ = 0.0;
		source.m_iFailAtCall = -1;
		source.m_iNanAtCall = -1;
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = true;
		expected.m_eProviderReason = RICON_EReason.OK;
		expected.m_iHeightCalls = 21;
		expected.m_bModel = true;
		expected.m_eModelReason = RICON_EReason.OK;
		expected.m_Profile = new RICON_TerrainProfile();
		expected.m_Profile.AddPoint(0.0, 100.0);
		expected.m_Profile.AddPoint(0.1, 100.0);
		expected.m_Profile.AddPoint(0.2, 100.0);
		expected.m_Profile.AddPoint(0.3, 100.0);
		expected.m_Profile.AddPoint(0.4, 100.0);
		expected.m_Profile.AddPoint(0.5, 100.0);
		expected.m_Profile.AddPoint(0.6, 100.0);
		expected.m_Profile.AddPoint(0.7, 100.0);
		expected.m_Profile.AddPoint(0.8, 100.0);
		expected.m_Profile.AddPoint(0.9, 100.0);
		expected.m_Profile.AddPoint(1.0, 100.0);
		expected.m_Profile.AddPoint(1.1, 100.0);
		expected.m_Profile.AddPoint(1.2, 100.0);
		expected.m_Profile.AddPoint(1.3, 100.0);
		expected.m_Profile.AddPoint(1.4, 100.0);
		expected.m_Profile.AddPoint(1.5, 100.0);
		expected.m_Profile.AddPoint(1.6, 100.0);
		expected.m_Profile.AddPoint(1.7, 100.0);
		expected.m_Profile.AddPoint(1.8, 100.0);
		expected.m_Profile.AddPoint(1.9, 100.0);
		expected.m_Profile.AddPoint(2.0, 100.0);
		expected.m_fTxAgl = 1.5;
		expected.m_fRxAgl = 1.5;
		expected.m_bHasL2 = true;
		expected.m_eL2Code = RICON_EL2ResultCode.OK;
		expected.m_aLayers.Insert(new RICON_LossLayer("fspl", 72.44778322188337, RICON_TerrainDiffractionModel.MODEL_ID));
		expected.m_aLayers.Insert(new RICON_LossLayer("terrain_diffraction", 35.731942904252925, RICON_TerrainDiffractionModel.MODEL_ID));
		t.CheckCase("W01", true, query, config, source, expected);
	}

	protected static void Case1(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		query.m_vTxPos = Vector(0.0, 101.5, 0.0);
		query.m_vRxPos = Vector(0.0, 101.5, 2000.0);
		RICON_AnalyticHeightSource source;
		source = new RICON_AnalyticHeightSource();
		source.m_fBase = 100.0;
		source.m_fSlopeX = 0.0;
		source.m_fSlopeZ = 0.0;
		source.m_iFailAtCall = -1;
		source.m_iNanAtCall = -1;
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = true;
		expected.m_eProviderReason = RICON_EReason.OK;
		expected.m_iHeightCalls = 21;
		expected.m_bModel = true;
		expected.m_eModelReason = RICON_EReason.OK;
		expected.m_Profile = new RICON_TerrainProfile();
		expected.m_Profile.AddPoint(0.0, 100.0);
		expected.m_Profile.AddPoint(0.1, 100.0);
		expected.m_Profile.AddPoint(0.2, 100.0);
		expected.m_Profile.AddPoint(0.3, 100.0);
		expected.m_Profile.AddPoint(0.4, 100.0);
		expected.m_Profile.AddPoint(0.5, 100.0);
		expected.m_Profile.AddPoint(0.6, 100.0);
		expected.m_Profile.AddPoint(0.7, 100.0);
		expected.m_Profile.AddPoint(0.8, 100.0);
		expected.m_Profile.AddPoint(0.9, 100.0);
		expected.m_Profile.AddPoint(1.0, 100.0);
		expected.m_Profile.AddPoint(1.1, 100.0);
		expected.m_Profile.AddPoint(1.2, 100.0);
		expected.m_Profile.AddPoint(1.3, 100.0);
		expected.m_Profile.AddPoint(1.4, 100.0);
		expected.m_Profile.AddPoint(1.5, 100.0);
		expected.m_Profile.AddPoint(1.6, 100.0);
		expected.m_Profile.AddPoint(1.7, 100.0);
		expected.m_Profile.AddPoint(1.8, 100.0);
		expected.m_Profile.AddPoint(1.9, 100.0);
		expected.m_Profile.AddPoint(2.0, 100.0);
		expected.m_fTxAgl = 1.5;
		expected.m_fRxAgl = 1.5;
		expected.m_bHasL2 = true;
		expected.m_eL2Code = RICON_EL2ResultCode.OK;
		expected.m_aLayers.Insert(new RICON_LossLayer("fspl", 72.44778322188337, RICON_TerrainDiffractionModel.MODEL_ID));
		expected.m_aLayers.Insert(new RICON_LossLayer("terrain_diffraction", 35.731942904252925, RICON_TerrainDiffractionModel.MODEL_ID));
		t.CheckCase("W02", true, query, config, source, expected);
	}

	protected static void Case2(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		query.m_vTxPos = Vector(100.0, 52.0, 200.0);
		query.m_vRxPos = Vector(1300.0, 59.5, 1100.0);
		RICON_AnalyticHeightSource source;
		source = new RICON_AnalyticHeightSource();
		source.m_fBase = 50.0;
		source.m_fSlopeX = 0.01;
		source.m_fSlopeZ = -0.005;
		source.m_iFailAtCall = -1;
		source.m_iNanAtCall = -1;
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = true;
		expected.m_eProviderReason = RICON_EReason.OK;
		expected.m_iHeightCalls = 16;
		expected.m_bModel = true;
		expected.m_eModelReason = RICON_EReason.OK;
		expected.m_Profile = new RICON_TerrainProfile();
		expected.m_Profile.AddPoint(0.0, 50.0);
		expected.m_Profile.AddPoint(0.1, 50.5);
		expected.m_Profile.AddPoint(0.2, 51.0);
		expected.m_Profile.AddPoint(0.3, 51.5);
		expected.m_Profile.AddPoint(0.4, 52.0);
		expected.m_Profile.AddPoint(0.5, 52.5);
		expected.m_Profile.AddPoint(0.6, 53.0);
		expected.m_Profile.AddPoint(0.7, 53.5);
		expected.m_Profile.AddPoint(0.8, 54.0);
		expected.m_Profile.AddPoint(0.9, 54.5);
		expected.m_Profile.AddPoint(1.0, 55.0);
		expected.m_Profile.AddPoint(1.1, 55.5);
		expected.m_Profile.AddPoint(1.2, 56.0);
		expected.m_Profile.AddPoint(1.3, 56.5);
		expected.m_Profile.AddPoint(1.4, 57.0);
		expected.m_Profile.AddPoint(1.5, 57.5);
		expected.m_fTxAgl = 2.0;
		expected.m_fRxAgl = 2.0;
		expected.m_bHasL2 = true;
		expected.m_eL2Code = RICON_EL2ResultCode.OK;
		expected.m_aLayers.Insert(new RICON_LossLayer("fspl", 69.9491170619807, RICON_TerrainDiffractionModel.MODEL_ID));
		expected.m_aLayers.Insert(new RICON_LossLayer("terrain_diffraction", 32.86845134730008, RICON_TerrainDiffractionModel.MODEL_ID));
		t.CheckCase("W03", true, query, config, source, expected);
	}

	protected static void Case3(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		query.m_vTxPos = Vector(1300.0, 59.5, 1100.0);
		query.m_vRxPos = Vector(100.0, 52.0, 200.0);
		RICON_AnalyticHeightSource source;
		source = new RICON_AnalyticHeightSource();
		source.m_fBase = 50.0;
		source.m_fSlopeX = 0.01;
		source.m_fSlopeZ = -0.005;
		source.m_iFailAtCall = -1;
		source.m_iNanAtCall = -1;
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = true;
		expected.m_eProviderReason = RICON_EReason.OK;
		expected.m_iHeightCalls = 16;
		expected.m_bModel = true;
		expected.m_eModelReason = RICON_EReason.OK;
		expected.m_Profile = new RICON_TerrainProfile();
		expected.m_Profile.AddPoint(0.0, 57.5);
		expected.m_Profile.AddPoint(0.1, 57.0);
		expected.m_Profile.AddPoint(0.2, 56.5);
		expected.m_Profile.AddPoint(0.3, 56.0);
		expected.m_Profile.AddPoint(0.4, 55.5);
		expected.m_Profile.AddPoint(0.5, 55.0);
		expected.m_Profile.AddPoint(0.6, 54.5);
		expected.m_Profile.AddPoint(0.7, 54.0);
		expected.m_Profile.AddPoint(0.8, 53.5);
		expected.m_Profile.AddPoint(0.9, 53.0);
		expected.m_Profile.AddPoint(1.0, 52.5);
		expected.m_Profile.AddPoint(1.1, 52.0);
		expected.m_Profile.AddPoint(1.2, 51.5);
		expected.m_Profile.AddPoint(1.3, 51.0);
		expected.m_Profile.AddPoint(1.4, 50.5);
		expected.m_Profile.AddPoint(1.5, 50.0);
		expected.m_fTxAgl = 2.0;
		expected.m_fRxAgl = 2.0;
		expected.m_bHasL2 = true;
		expected.m_eL2Code = RICON_EL2ResultCode.OK;
		expected.m_aLayers.Insert(new RICON_LossLayer("fspl", 69.9491170619807, RICON_TerrainDiffractionModel.MODEL_ID));
		expected.m_aLayers.Insert(new RICON_LossLayer("terrain_diffraction", 32.86845134730008, RICON_TerrainDiffractionModel.MODEL_ID));
		t.CheckCase("W04", true, query, config, source, expected);
	}

	protected static void Case4(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		query.m_vTxPos = Vector(0.0, 2.0, 0.0);
		query.m_vRxPos = Vector(5000.0, 2.0, 0.0);
		RICON_AnalyticHeightSource source;
		source = new RICON_AnalyticHeightSource();
		source.m_fBase = 0.0;
		source.m_fSlopeX = 0.0;
		source.m_fSlopeZ = 0.0;
		source.m_iFailAtCall = -1;
		source.m_iNanAtCall = -1;
		source.m_aHills.Insert(new RICON_TestHeightHill(2500.0, 0.0, 80.0, 300.0));
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = true;
		expected.m_eProviderReason = RICON_EReason.OK;
		expected.m_iHeightCalls = 51;
		expected.m_bModel = true;
		expected.m_eModelReason = RICON_EReason.OK;
		expected.m_Profile = new RICON_TerrainProfile();
		expected.m_Profile.AddPoint(0.0, 0.00000000000006659);
		expected.m_Profile.AddPoint(0.1, 0.00000000000101313);
		expected.m_Profile.AddPoint(0.2, 0.00000000001379297);
		expected.m_Profile.AddPoint(0.3, 0.00000000016803274);
		expected.m_Profile.AddPoint(0.4, 0.00000000183178788);
		expected.m_Profile.AddPoint(0.5, 0.00000001786905149);
		expected.m_Profile.AddPoint(0.6, 0.00000015598142288);
		expected.m_Profile.AddPoint(0.7, 0.00000121839837958);
		expected.m_Profile.AddPoint(0.8, 0.00000851629712886);
		expected.m_Profile.AddPoint(0.9, 0.00005326689175886);
		expected.m_Profile.AddPoint(1.0, 0.0002981322537662937);
		expected.m_Profile.AddPoint(1.1, 0.0014931575290816456);
		expected.m_Profile.AddPoint(1.2, 0.006691867778378202);
		expected.m_Profile.AddPoint(1.3, 0.026837010232200947);
		expected.m_Profile.AddPoint(1.4, 0.09630879958625649);
		expected.m_Profile.AddPoint(1.5, 0.3092736111578246);
		expected.m_Profile.AddPoint(1.6, 0.8887197230593845);
		expected.m_Profile.AddPoint(1.7, 2.2852400627640344);
		expected.m_Profile.AddPoint(1.8, 5.2582822893224375);
		expected.m_Profile.AddPoint(1.9, 10.826822658929016);
		expected.m_Profile.AddPoint(2.0, 19.948176702183698);
		expected.m_Profile.AddPoint(2.1, 32.888983240574994);
		expected.m_Profile.AddPoint(2.2, 48.52245277701067);
		expected.m_Profile.AddPoint(2.3, 64.05899223334464);
		expected.m_Profile.AddPoint(2.4, 75.67675751254123);
		expected.m_Profile.AddPoint(2.5, 80.0);
		expected.m_Profile.AddPoint(2.6, 75.67675751254123);
		expected.m_Profile.AddPoint(2.7, 64.05899223334464);
		expected.m_Profile.AddPoint(2.8, 48.5224527770106);
		expected.m_Profile.AddPoint(2.9, 32.888983240574994);
		expected.m_Profile.AddPoint(3.0, 19.948176702183698);
		expected.m_Profile.AddPoint(3.1, 10.826822658929016);
		expected.m_Profile.AddPoint(3.2, 5.2582822893224375);
		expected.m_Profile.AddPoint(3.3, 2.2852400627640304);
		expected.m_Profile.AddPoint(3.4, 0.8887197230593806);
		expected.m_Profile.AddPoint(3.5, 0.3092736111578246);
		expected.m_Profile.AddPoint(3.6, 0.09630879958625624);
		expected.m_Profile.AddPoint(3.7, 0.026837010232200947);
		expected.m_Profile.AddPoint(3.8, 0.006691867778378202);
		expected.m_Profile.AddPoint(3.9, 0.0014931575290816456);
		expected.m_Profile.AddPoint(4.0, 0.0002981322537662937);
		expected.m_Profile.AddPoint(4.1, 0.00005326689175886);
		expected.m_Profile.AddPoint(4.2, 0.00000851629712886);
		expected.m_Profile.AddPoint(4.3, 0.00000121839837958);
		expected.m_Profile.AddPoint(4.4, 0.00000015598142288);
		expected.m_Profile.AddPoint(4.5, 0.00000001786905149);
		expected.m_Profile.AddPoint(4.6, 0.00000000183178788);
		expected.m_Profile.AddPoint(4.7, 0.00000000016803274);
		expected.m_Profile.AddPoint(4.8, 0.00000000001379297);
		expected.m_Profile.AddPoint(4.9, 0.00000000000101313);
		expected.m_Profile.AddPoint(5.0, 0.00000000000006659);
		expected.m_fTxAgl = 1.9999999999999334;
		expected.m_fRxAgl = 1.9999999999999334;
		expected.m_bHasL2 = true;
		expected.m_eL2Code = RICON_EL2ResultCode.OK;
		expected.m_aLayers.Insert(new RICON_LossLayer("fspl", 80.40658339532413, RICON_TerrainDiffractionModel.MODEL_ID));
		expected.m_aLayers.Insert(new RICON_LossLayer("terrain_diffraction", 28.108423514041256, RICON_TerrainDiffractionModel.MODEL_ID));
		t.CheckCase("W05", true, query, config, source, expected);
	}

	protected static void Case5(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		query.m_vTxPos = Vector(0.0, 2.0, 0.0);
		query.m_vRxPos = Vector(5000.0, 2.0, 0.0);
		RICON_AnalyticHeightSource source;
		source = new RICON_AnalyticHeightSource();
		source.m_fBase = 0.0;
		source.m_fSlopeX = 0.0;
		source.m_fSlopeZ = 0.0;
		source.m_iFailAtCall = -1;
		source.m_iNanAtCall = -1;
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = true;
		expected.m_eProviderReason = RICON_EReason.OK;
		expected.m_iHeightCalls = 51;
		expected.m_bModel = true;
		expected.m_eModelReason = RICON_EReason.OK;
		expected.m_Profile = new RICON_TerrainProfile();
		expected.m_Profile.AddPoint(0.0, 0.0);
		expected.m_Profile.AddPoint(0.1, 0.0);
		expected.m_Profile.AddPoint(0.2, 0.0);
		expected.m_Profile.AddPoint(0.3, 0.0);
		expected.m_Profile.AddPoint(0.4, 0.0);
		expected.m_Profile.AddPoint(0.5, 0.0);
		expected.m_Profile.AddPoint(0.6, 0.0);
		expected.m_Profile.AddPoint(0.7, 0.0);
		expected.m_Profile.AddPoint(0.8, 0.0);
		expected.m_Profile.AddPoint(0.9, 0.0);
		expected.m_Profile.AddPoint(1.0, 0.0);
		expected.m_Profile.AddPoint(1.1, 0.0);
		expected.m_Profile.AddPoint(1.2, 0.0);
		expected.m_Profile.AddPoint(1.3, 0.0);
		expected.m_Profile.AddPoint(1.4, 0.0);
		expected.m_Profile.AddPoint(1.5, 0.0);
		expected.m_Profile.AddPoint(1.6, 0.0);
		expected.m_Profile.AddPoint(1.7, 0.0);
		expected.m_Profile.AddPoint(1.8, 0.0);
		expected.m_Profile.AddPoint(1.9, 0.0);
		expected.m_Profile.AddPoint(2.0, 0.0);
		expected.m_Profile.AddPoint(2.1, 0.0);
		expected.m_Profile.AddPoint(2.2, 0.0);
		expected.m_Profile.AddPoint(2.3, 0.0);
		expected.m_Profile.AddPoint(2.4, 0.0);
		expected.m_Profile.AddPoint(2.5, 0.0);
		expected.m_Profile.AddPoint(2.6, 0.0);
		expected.m_Profile.AddPoint(2.7, 0.0);
		expected.m_Profile.AddPoint(2.8, 0.0);
		expected.m_Profile.AddPoint(2.9, 0.0);
		expected.m_Profile.AddPoint(3.0, 0.0);
		expected.m_Profile.AddPoint(3.1, 0.0);
		expected.m_Profile.AddPoint(3.2, 0.0);
		expected.m_Profile.AddPoint(3.3, 0.0);
		expected.m_Profile.AddPoint(3.4, 0.0);
		expected.m_Profile.AddPoint(3.5, 0.0);
		expected.m_Profile.AddPoint(3.6, 0.0);
		expected.m_Profile.AddPoint(3.7, 0.0);
		expected.m_Profile.AddPoint(3.8, 0.0);
		expected.m_Profile.AddPoint(3.9, 0.0);
		expected.m_Profile.AddPoint(4.0, 0.0);
		expected.m_Profile.AddPoint(4.1, 0.0);
		expected.m_Profile.AddPoint(4.2, 0.0);
		expected.m_Profile.AddPoint(4.3, 0.0);
		expected.m_Profile.AddPoint(4.4, 0.0);
		expected.m_Profile.AddPoint(4.5, 0.0);
		expected.m_Profile.AddPoint(4.6, 0.0);
		expected.m_Profile.AddPoint(4.7, 0.0);
		expected.m_Profile.AddPoint(4.8, 0.0);
		expected.m_Profile.AddPoint(4.9, 0.0);
		expected.m_Profile.AddPoint(5.0, 0.0);
		expected.m_fTxAgl = 2.0;
		expected.m_fRxAgl = 2.0;
		expected.m_bHasL2 = true;
		expected.m_eL2Code = RICON_EL2ResultCode.OK;
		expected.m_aLayers.Insert(new RICON_LossLayer("fspl", 80.40658339532413, RICON_TerrainDiffractionModel.MODEL_ID));
		expected.m_aLayers.Insert(new RICON_LossLayer("terrain_diffraction", 43.8143346055296, RICON_TerrainDiffractionModel.MODEL_ID));
		t.CheckCase("W06", true, query, config, source, expected);
	}

	protected static void Case6(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		query.m_vTxPos = Vector(0.0, -18.0, 0.0);
		query.m_vRxPos = Vector(3000.0, -18.0, 0.0);
		RICON_AnalyticHeightSource source;
		source = new RICON_AnalyticHeightSource();
		source.m_fBase = -20.0;
		source.m_fSlopeX = 0.0;
		source.m_fSlopeZ = 0.0;
		source.m_iFailAtCall = -1;
		source.m_iNanAtCall = -1;
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = true;
		expected.m_eProviderReason = RICON_EReason.OK;
		expected.m_iHeightCalls = 31;
		expected.m_bModel = true;
		expected.m_eModelReason = RICON_EReason.OK;
		expected.m_Profile = new RICON_TerrainProfile();
		expected.m_Profile.AddPoint(0.0, -20.0);
		expected.m_Profile.AddPoint(0.1, -20.0);
		expected.m_Profile.AddPoint(0.2, -20.0);
		expected.m_Profile.AddPoint(0.3, -20.0);
		expected.m_Profile.AddPoint(0.4, -20.0);
		expected.m_Profile.AddPoint(0.5, -20.0);
		expected.m_Profile.AddPoint(0.6, -20.0);
		expected.m_Profile.AddPoint(0.7, -20.0);
		expected.m_Profile.AddPoint(0.8, -20.0);
		expected.m_Profile.AddPoint(0.9, -20.0);
		expected.m_Profile.AddPoint(1.0, -20.0);
		expected.m_Profile.AddPoint(1.1, -20.0);
		expected.m_Profile.AddPoint(1.2, -20.0);
		expected.m_Profile.AddPoint(1.3, -20.0);
		expected.m_Profile.AddPoint(1.4, -20.0);
		expected.m_Profile.AddPoint(1.5, -20.0);
		expected.m_Profile.AddPoint(1.6, -20.0);
		expected.m_Profile.AddPoint(1.7, -20.0);
		expected.m_Profile.AddPoint(1.8, -20.0);
		expected.m_Profile.AddPoint(1.9, -20.0);
		expected.m_Profile.AddPoint(2.0, -20.0);
		expected.m_Profile.AddPoint(2.1, -20.0);
		expected.m_Profile.AddPoint(2.2, -20.0);
		expected.m_Profile.AddPoint(2.3, -20.0);
		expected.m_Profile.AddPoint(2.4, -20.0);
		expected.m_Profile.AddPoint(2.5, -20.0);
		expected.m_Profile.AddPoint(2.6, -20.0);
		expected.m_Profile.AddPoint(2.7, -20.0);
		expected.m_Profile.AddPoint(2.8, -20.0);
		expected.m_Profile.AddPoint(2.9, -20.0);
		expected.m_Profile.AddPoint(3.0, -20.0);
		expected.m_fTxAgl = 2.0;
		expected.m_fRxAgl = 2.0;
		expected.m_bHasL2 = true;
		expected.m_eL2Code = RICON_EL2ResultCode.OK;
		expected.m_aLayers.Insert(new RICON_LossLayer("fspl", 75.969608402997, RICON_TerrainDiffractionModel.MODEL_ID));
		expected.m_aLayers.Insert(new RICON_LossLayer("terrain_diffraction", 39.09273188864554, RICON_TerrainDiffractionModel.MODEL_ID));
		t.CheckCase("W07", true, query, config, source, expected);
	}

	protected static void Case7(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		query.m_vTxPos = Vector(0.0, 2.0, 0.0);
		query.m_vRxPos = Vector(2000.0, 2.0, 0.0);
		RICON_AnalyticHeightSource source;
		source = new RICON_AnalyticHeightSource();
		source.m_fBase = 0.0;
		source.m_fSlopeX = 0.0;
		source.m_fSlopeZ = 0.0;
		source.m_iFailAtCall = 0;
		source.m_iNanAtCall = -1;
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = true;
		expected.m_eProviderReason = RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
		expected.m_iHeightCalls = 1;
		expected.m_bModel = true;
		expected.m_eModelReason = RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
		t.CheckCase("W08", true, query, config, source, expected);
	}

	protected static void Case8(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		query.m_vTxPos = Vector(0.0, 2.0, 0.0);
		query.m_vRxPos = Vector(2000.0, 2.0, 0.0);
		RICON_AnalyticHeightSource source;
		source = new RICON_AnalyticHeightSource();
		source.m_fBase = 0.0;
		source.m_fSlopeX = 0.0;
		source.m_fSlopeZ = 0.0;
		source.m_iFailAtCall = 10;
		source.m_iNanAtCall = -1;
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = true;
		expected.m_eProviderReason = RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
		expected.m_iHeightCalls = 11;
		expected.m_bModel = true;
		expected.m_eModelReason = RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
		t.CheckCase("W09", true, query, config, source, expected);
	}

	protected static void Case9(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		query.m_vTxPos = Vector(0.0, 2.0, 0.0);
		query.m_vRxPos = Vector(2000.0, 2.0, 0.0);
		RICON_AnalyticHeightSource source;
		source = new RICON_AnalyticHeightSource();
		source.m_fBase = 0.0;
		source.m_fSlopeX = 0.0;
		source.m_fSlopeZ = 0.0;
		source.m_iFailAtCall = -1;
		source.m_iNanAtCall = 20;
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = true;
		expected.m_eProviderReason = RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
		expected.m_iHeightCalls = 21;
		expected.m_bModel = true;
		expected.m_eModelReason = RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
		t.CheckCase("W10", true, query, config, source, expected);
	}

	protected static void Case10(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		query.m_vTxPos = Vector(0.0, 2.0, 0.0);
		query.m_vRxPos = Vector(2000.0, 2.0, 0.0);
		RICON_AnalyticHeightSource source;
		source = new RICON_AnalyticHeightSource();
		source.m_fBase = 0.0;
		source.m_fSlopeX = 0.0;
		source.m_fSlopeZ = 0.0;
		source.m_iFailAtCall = -1;
		source.m_iNanAtCall = 7;
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = true;
		expected.m_eProviderReason = RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
		expected.m_iHeightCalls = 8;
		expected.m_bModel = true;
		expected.m_eModelReason = RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
		t.CheckCase("W11", true, query, config, source, expected);
	}

	protected static void Case11(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 20);
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		query.m_vTxPos = Vector(0.0, 2.0, 0.0);
		query.m_vRxPos = Vector(5000.0, 2.0, 0.0);
		RICON_AnalyticHeightSource source;
		source = new RICON_AnalyticHeightSource();
		source.m_fBase = 0.0;
		source.m_fSlopeX = 0.0;
		source.m_fSlopeZ = 0.0;
		source.m_iFailAtCall = -1;
		source.m_iNanAtCall = -1;
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = true;
		expected.m_eProviderReason = RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
		expected.m_iHeightCalls = 0;
		expected.m_bModel = true;
		expected.m_eModelReason = RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
		t.CheckCase("W12", true, query, config, source, expected);
	}

	protected static void Case12(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		query.m_vTxPos = Vector(0.0, 2.0, 0.0);
		query.m_vRxPos = Vector(501000.0, 2.0, 0.0);
		RICON_AnalyticHeightSource source;
		source = new RICON_AnalyticHeightSource();
		source.m_fBase = 0.0;
		source.m_fSlopeX = 0.0;
		source.m_fSlopeZ = 0.0;
		source.m_iFailAtCall = -1;
		source.m_iNanAtCall = -1;
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = true;
		expected.m_eProviderReason = RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
		expected.m_iHeightCalls = 0;
		expected.m_bModel = true;
		expected.m_eModelReason = RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
		t.CheckCase("W13", true, query, config, source, expected);
	}

	protected static void Case13(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		query.m_vTxPos = Vector(0.0, 10.0, 0.0);
		query.m_vRxPos = Vector(1.0, 30.0, 1.0);
		RICON_AnalyticHeightSource source;
		source = new RICON_AnalyticHeightSource();
		source.m_fBase = 0.0;
		source.m_fSlopeX = 0.0;
		source.m_fSlopeZ = 0.0;
		source.m_iFailAtCall = -1;
		source.m_iNanAtCall = -1;
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = false;
		expected.m_iHeightCalls = 0;
		expected.m_bModel = true;
		expected.m_eModelReason = RICON_EReason.OK;
		expected.m_aLayers.Insert(new RICON_LossLayer("fspl", 32.46944383944845, RICON_TerrainDiffractionModel.MODEL_ID));
		expected.m_aLayers.Insert(new RICON_LossLayer("terrain_diffraction", 0.0, RICON_TerrainDiffractionModel.MODEL_ID));
		t.CheckCase("W14", true, query, config, source, expected);
	}

	protected static void Case14(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		query.m_vTxPos = Vector(0.0, 2.0, 0.0);
		query.m_vRxPos = Vector(2.5, 2.0, 0.0);
		RICON_AnalyticHeightSource source;
		source = new RICON_AnalyticHeightSource();
		source.m_fBase = 0.0;
		source.m_fSlopeX = 0.0;
		source.m_fSlopeZ = 0.0;
		source.m_iFailAtCall = -1;
		source.m_iNanAtCall = -1;
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = true;
		expected.m_eProviderReason = RICON_EReason.OK;
		expected.m_iHeightCalls = 3;
		expected.m_bModel = true;
		expected.m_eModelReason = RICON_EReason.OK;
		expected.m_Profile = new RICON_TerrainProfile();
		expected.m_Profile.AddPoint(0.0, 0.0);
		expected.m_Profile.AddPoint(0.00125, 0.0);
		expected.m_Profile.AddPoint(0.0025, 0.0);
		expected.m_fTxAgl = 2.0;
		expected.m_fRxAgl = 2.0;
		expected.m_bHasL2 = true;
		expected.m_eL2Code = RICON_EL2ResultCode.OK;
		expected.m_aLayers.Insert(new RICON_LossLayer("fspl", 21.984197280441926, RICON_TerrainDiffractionModel.MODEL_ID));
		expected.m_aLayers.Insert(new RICON_LossLayer("terrain_diffraction", 0.0, RICON_TerrainDiffractionModel.MODEL_ID));
		t.CheckCase("W15", true, query, config, source, expected);
	}

	protected static void Case15(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 20.0);
		query.m_vTxPos = Vector(0.0, 2.0, 0.0);
		query.m_vRxPos = Vector(2000.0, 2.0, 0.0);
		RICON_AnalyticHeightSource source;
		source = new RICON_AnalyticHeightSource();
		source.m_fBase = 0.0;
		source.m_fSlopeX = 0.0;
		source.m_fSlopeZ = 0.0;
		source.m_iFailAtCall = -1;
		source.m_iNanAtCall = -1;
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = false;
		expected.m_iHeightCalls = 0;
		expected.m_bModel = true;
		expected.m_eModelReason = RICON_EReason.OUT_OF_MODEL_DOMAIN;
		t.CheckCase("W16", true, query, config, source, expected);
	}

	protected static void Case16(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		query.m_vTxPos = Vector(0.0, -1.0, 0.0);
		query.m_vRxPos = Vector(2000.0, 2.0, 0.0);
		RICON_AnalyticHeightSource source;
		source = new RICON_AnalyticHeightSource();
		source.m_fBase = 0.0;
		source.m_fSlopeX = 0.0;
		source.m_fSlopeZ = 0.0;
		source.m_iFailAtCall = -1;
		source.m_iNanAtCall = -1;
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = true;
		expected.m_eProviderReason = RICON_EReason.OK;
		expected.m_iHeightCalls = 21;
		expected.m_bModel = true;
		expected.m_eModelReason = RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
		expected.m_Profile = new RICON_TerrainProfile();
		expected.m_Profile.AddPoint(0.0, 0.0);
		expected.m_Profile.AddPoint(0.1, 0.0);
		expected.m_Profile.AddPoint(0.2, 0.0);
		expected.m_Profile.AddPoint(0.3, 0.0);
		expected.m_Profile.AddPoint(0.4, 0.0);
		expected.m_Profile.AddPoint(0.5, 0.0);
		expected.m_Profile.AddPoint(0.6, 0.0);
		expected.m_Profile.AddPoint(0.7, 0.0);
		expected.m_Profile.AddPoint(0.8, 0.0);
		expected.m_Profile.AddPoint(0.9, 0.0);
		expected.m_Profile.AddPoint(1.0, 0.0);
		expected.m_Profile.AddPoint(1.1, 0.0);
		expected.m_Profile.AddPoint(1.2, 0.0);
		expected.m_Profile.AddPoint(1.3, 0.0);
		expected.m_Profile.AddPoint(1.4, 0.0);
		expected.m_Profile.AddPoint(1.5, 0.0);
		expected.m_Profile.AddPoint(1.6, 0.0);
		expected.m_Profile.AddPoint(1.7, 0.0);
		expected.m_Profile.AddPoint(1.8, 0.0);
		expected.m_Profile.AddPoint(1.9, 0.0);
		expected.m_Profile.AddPoint(2.0, 0.0);
		expected.m_fTxAgl = -1.0;
		expected.m_fRxAgl = 2.0;
		expected.m_bHasL2 = true;
		expected.m_eL2Code = RICON_EL2ResultCode.INVALID_ANTENNA_HEIGHT;
		t.CheckCase("W17", true, query, config, source, expected);
	}

	protected static void Case17(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		query.m_vTxPos = Vector(0.0, 10502.0, 0.0);
		query.m_vRxPos = Vector(2000.0, 10502.0, 0.0);
		RICON_AnalyticHeightSource source;
		source = new RICON_AnalyticHeightSource();
		source.m_fBase = 10500.0;
		source.m_fSlopeX = 0.0;
		source.m_fSlopeZ = 0.0;
		source.m_iFailAtCall = -1;
		source.m_iNanAtCall = -1;
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = true;
		expected.m_eProviderReason = RICON_EReason.OK;
		expected.m_iHeightCalls = 21;
		expected.m_bModel = true;
		expected.m_eModelReason = RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
		expected.m_Profile = new RICON_TerrainProfile();
		expected.m_Profile.AddPoint(0.0, 10500.0);
		expected.m_Profile.AddPoint(0.1, 10500.0);
		expected.m_Profile.AddPoint(0.2, 10500.0);
		expected.m_Profile.AddPoint(0.3, 10500.0);
		expected.m_Profile.AddPoint(0.4, 10500.0);
		expected.m_Profile.AddPoint(0.5, 10500.0);
		expected.m_Profile.AddPoint(0.6, 10500.0);
		expected.m_Profile.AddPoint(0.7, 10500.0);
		expected.m_Profile.AddPoint(0.8, 10500.0);
		expected.m_Profile.AddPoint(0.9, 10500.0);
		expected.m_Profile.AddPoint(1.0, 10500.0);
		expected.m_Profile.AddPoint(1.1, 10500.0);
		expected.m_Profile.AddPoint(1.2, 10500.0);
		expected.m_Profile.AddPoint(1.3, 10500.0);
		expected.m_Profile.AddPoint(1.4, 10500.0);
		expected.m_Profile.AddPoint(1.5, 10500.0);
		expected.m_Profile.AddPoint(1.6, 10500.0);
		expected.m_Profile.AddPoint(1.7, 10500.0);
		expected.m_Profile.AddPoint(1.8, 10500.0);
		expected.m_Profile.AddPoint(1.9, 10500.0);
		expected.m_Profile.AddPoint(2.0, 10500.0);
		expected.m_fTxAgl = 2.0;
		expected.m_fRxAgl = 2.0;
		expected.m_bHasL2 = true;
		expected.m_eL2Code = RICON_EL2ResultCode.INVALID_PROFILE;
		t.CheckCase("W18", true, query, config, source, expected);
	}

	protected static void Case18(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		query.m_vTxPos = Vector(12000.0, 67.0, 11000.0);
		query.m_vRxPos = Vector(1000.0, 34.5, 500.0);
		RICON_AnalyticHeightSource source;
		source = new RICON_AnalyticHeightSource();
		source.m_fBase = 30.0;
		source.m_fSlopeX = 0.002;
		source.m_fSlopeZ = 0.001;
		source.m_iFailAtCall = -1;
		source.m_iNanAtCall = -1;
		source.m_aHills.Insert(new RICON_TestHeightHill(6000.0, 5500.0, 120.0, 800.0));
		source.m_aHills.Insert(new RICON_TestHeightHill(9000.0, 8000.0, 60.0, 400.0));
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = true;
		expected.m_eProviderReason = RICON_EReason.OK;
		expected.m_iHeightCalls = 154;
		expected.m_bModel = true;
		expected.m_eModelReason = RICON_EReason.OK;
		expected.m_Profile = new RICON_TerrainProfile();
		expected.m_Profile.AddPoint(0.0, 65.0);
		expected.m_Profile.AddPoint(0.09939154461271602, 64.7875816993464);
		expected.m_Profile.AddPoint(0.19878308922543203, 64.57516339869281);
		expected.m_Profile.AddPoint(0.298174633838148, 64.36274509803921);
		expected.m_Profile.AddPoint(0.39756617845086406, 64.15032679738562);
		expected.m_Profile.AddPoint(0.49695772306358005, 63.93790849673203);
		expected.m_Profile.AddPoint(0.596349267676296, 63.72549019607843);
		expected.m_Profile.AddPoint(0.6957408122890121, 63.51307189542483);
		expected.m_Profile.AddPoint(0.7951323569017281, 63.300653594771255);
		expected.m_Profile.AddPoint(0.894523901514444, 63.08823529411768);
		expected.m_Profile.AddPoint(0.9939154461271601, 62.87581699346434);
		expected.m_Profile.AddPoint(1.0933069907398762, 62.66339869281252);
		expected.m_Profile.AddPoint(1.192698535352592, 62.45098039217099);
		expected.m_Profile.AddPoint(1.2920900799653081, 62.23856209159426);
		expected.m_Profile.AddPoint(1.3914816245780242, 62.02614379140082);
		expected.m_Profile.AddPoint(1.4908731691907402, 61.81372549333463);
		expected.m_Profile.AddPoint(1.5902647138034562, 61.60130720634535);
		expected.m_Profile.AddPoint(1.689656258416172, 61.38888897346111);
		expected.m_Profile.AddPoint(1.789047803028888, 61.17647098841934);
		expected.m_Profile.AddPoint(1.8884393476416044, 60.96405406782213);
		expected.m_Profile.AddPoint(1.9878308922543202, 60.75164143225777);
		expected.m_Profile.AddPoint(2.087222436867036, 60.53924495982237);
		expected.m_Profile.AddPoint(2.1866139814797525, 60.326905591991256);
		expected.m_Profile.AddPoint(2.2860055260924685, 60.11475510958527);
		expected.m_Profile.AddPoint(2.385397070705184, 59.90318925748357);
		expected.m_Profile.AddPoint(2.4847886153179, 59.69331560700432);
		expected.m_Profile.AddPoint(2.5841801599306162, 59.48801905792558);
		expected.m_Profile.AddPoint(2.6835717045433323, 59.29428119931604);
		expected.m_Profile.AddPoint(2.7829632491560483, 59.127765945391296);
		expected.m_Profile.AddPoint(2.8823547937687644, 59.02096219325894);
		expected.m_Profile.AddPoint(2.9817463383814804, 59.035926784048286);
		expected.m_Profile.AddPoint(3.0811378829941964, 59.28123057798697);
		expected.m_Profile.AddPoint(3.1805294276069125, 59.9294612290144);
		expected.m_Profile.AddPoint(3.279920972219628, 61.226702170666485);
		expected.m_Profile.AddPoint(3.379312516832344, 63.48053584051246);
		expected.m_Profile.AddPoint(3.47870406144506, 67.01204337066181);
		expected.m_Profile.AddPoint(3.578095606057776, 72.06453568331997);
		expected.m_Profile.AddPoint(3.6774871506704927, 78.67954844769316);
		expected.m_Profile.AddPoint(3.7768786952832087, 86.5749391114673);
		expected.m_Profile.AddPoint(3.8762702398959243, 95.07904578344012);
		expected.m_Profile.AddPoint(3.9756617845086404, 103.1736179972743);
		expected.m_Profile.AddPoint(4.075053329121356, 109.66755992859952);
		expected.m_Profile.AddPoint(4.174444873734072, 113.47015423358542);
		expected.m_Profile.AddPoint(4.2738364183467885, 113.88020651098208);
		expected.m_Profile.AddPoint(4.373227962959505, 110.78620872748292);
		expected.m_Profile.AddPoint(4.472619507572221, 104.69911934905349);
		expected.m_Profile.AddPoint(4.572011052184937, 96.60459145788965);
		expected.m_Profile.AddPoint(4.671402596797653, 87.69339861044621);
		expected.m_Profile.AddPoint(4.770794141410368, 79.0712972595129);
		expected.m_Profile.AddPoint(4.870185686023085, 71.54418793182987);
		expected.m_Profile.AddPoint(4.9695772306358, 65.52986641815292);
		expected.m_Profile.AddPoint(5.068968775248516, 61.091318787774426);
		expected.m_Profile.AddPoint(5.1683603198612325, 58.04618484015094);
		expected.m_Profile.AddPoint(5.267751864473949, 56.095868489493704);
		expected.m_Profile.AddPoint(5.3671434090866645, 54.93142464319774);
		expected.m_Profile.AddPoint(5.466534953699381, 54.297495260257776);
		expected.m_Profile.AddPoint(5.565926498312097, 54.01648060983016);
		expected.m_Profile.AddPoint(5.665318042924812, 53.98603751855931);
		expected.m_Profile.AddPoint(5.764709587537529, 54.16431400508115);
		expected.m_Profile.AddPoint(5.864101132150244, 54.55322809564329);
		expected.m_Profile.AddPoint(5.963492676762961, 55.18483574177727);
		expected.m_Profile.AddPoint(6.062884221375677, 56.11187263037558);
		expected.m_Profile.AddPoint(6.162275765988393, 57.40150301543906);
		expected.m_Profile.AddPoint(6.2616673106011085, 59.13072158288864);
		expected.m_Profile.AddPoint(6.361058855213825, 61.38204074126046);
		expected.m_Profile.AddPoint(6.460450399826541, 64.23854198105477);
		expected.m_Profile.AddPoint(6.559841944439256, 67.77782284111892);
		expected.m_Profile.AddPoint(6.659233489051973, 72.06475743199529);
		expected.m_Profile.AddPoint(6.758625033664688, 77.14331272986689);
		expected.m_Profile.AddPoint(6.858016578277405, 83.0279404759401);
		expected.m_Profile.AddPoint(6.95740812289012, 89.69529555339929);
		expected.m_Profile.AddPoint(7.056799667502837, 97.07719841696544);
		expected.m_Profile.AddPoint(7.156191212115552, 105.05583383441952);
		expected.m_Profile.AddPoint(7.255582756728269, 113.46213364580979);
		expected.m_Profile.AddPoint(7.354974301340985, 122.07811035878319);
		expected.m_Profile.AddPoint(7.4543658459537, 130.64359168743064);
		expected.m_Profile.AddPoint(7.5537573905664175, 138.86737601575837);
		expected.m_Profile.AddPoint(7.653148935179132, 146.4423297841423);
		expected.m_Profile.AddPoint(7.752540479791849, 153.06344243879795);
		expected.m_Profile.AddPoint(7.851932024404564, 158.44741484735434);
		expected.m_Profile.AddPoint(7.951323569017281, 162.35205308977586);
		expected.m_Profile.AddPoint(8.050715113629998, 164.593627136631);
		expected.m_Profile.AddPoint(8.150106658242713, 165.06046374372204);
		expected.m_Profile.AddPoint(8.24949820285543, 163.72137312129462);
		expected.m_Profile.AddPoint(8.348889747468144, 160.62802369195666);
		expected.m_Profile.AddPoint(8.44828129208086, 155.911013088669);
		expected.m_Profile.AddPoint(8.547672836693577, 149.7700515475886);
		expected.m_Profile.AddPoint(8.647064381306294, 142.45928546860068);
		expected.m_Profile.AddPoint(8.74645592591901, 134.2692628878203);
		expected.m_Profile.AddPoint(8.845847470531725, 125.50732008698667);
		expected.m_Profile.AddPoint(8.945239015144441, 116.47822196136084);
		expected.m_Profile.AddPoint(9.044630559757156, 107.466724730288);
		expected.m_Profile.AddPoint(9.144022104369874, 98.72338584982171);
		expected.m_Profile.AddPoint(9.243413648982589, 90.45448309836601);
		expected.m_Profile.AddPoint(9.342805193595305, 82.81639449940548);
		expected.m_Profile.AddPoint(9.44219673820802, 75.91430362313926);
		expected.m_Profile.AddPoint(9.541588282820737, 69.80468951474452);
		expected.m_Profile.AddPoint(9.640979827433453, 64.50077623765151);
		expected.m_Profile.AddPoint(9.74037137204617, 59.97997034973993);
		expected.m_Profile.AddPoint(9.839762916658886, 56.192300467394944);
		expected.m_Profile.AddPoint(9.9391544612716, 53.068969110601394);
		expected.m_Profile.AddPoint(10.038546005884317, 50.53030018300112);
		expected.m_Profile.AddPoint(10.137937550497032, 48.492578864398396);
		expected.m_Profile.AddPoint(10.237329095109748, 46.87349994512641);
		expected.m_Profile.AddPoint(10.336720639722465, 45.5961382097871);
		expected.m_Profile.AddPoint(10.436112184335181, 44.591512213762925);
		expected.m_Profile.AddPoint(10.535503728947898, 43.79992222306407);
		expected.m_Profile.AddPoint(10.634895273560613, 43.17130420315389);
		expected.m_Profile.AddPoint(10.734286818173329, 42.66486077295889);
		expected.m_Profile.AddPoint(10.833678362786046, 42.24821685358949);
		expected.m_Profile.AddPoint(10.933069907398762, 41.89631342239437);
		expected.m_Profile.AddPoint(11.032461452011477, 41.59020773254134);
		expected.m_Profile.AddPoint(11.131852996624193, 41.31590106762354);
		expected.m_Profile.AddPoint(11.23124454123691, 41.06327159485669);
		expected.m_Profile.AddPoint(11.330636085849624, 40.825153707379044);
		expected.m_Profile.AddPoint(11.430027630462341, 40.59657783558614);
		expected.m_Profile.AddPoint(11.529419175075057, 40.37416591631714);
		expected.m_Profile.AddPoint(11.628810719687774, 40.15566640357355);
		expected.m_Profile.AddPoint(11.728202264300489, 39.939607275503846);
		expected.m_Profile.AddPoint(11.827593808913205, 39.72504424468108);
		expected.m_Profile.AddPoint(11.926985353525922, 39.51138278612869);
		expected.m_Profile.AddPoint(12.026376898138638, 39.29825543365111);
		expected.m_Profile.AddPoint(12.125768442751355, 39.08543916935348);
		expected.m_Profile.AddPoint(12.22515998736407, 38.872801063954206);
		expected.m_Profile.AddPoint(12.324551531976786, 38.660263289569535);
		expected.m_Profile.AddPoint(12.4239430765895, 38.447781079922706);
		expected.m_Profile.AddPoint(12.523334621202217, 38.23532913451684);
		expected.m_Profile.AddPoint(12.622726165814933, 38.0228934017783);
		expected.m_Profile.AddPoint(12.72211771042765, 37.81046621181409);
		expected.m_Profile.AddPoint(12.821509255040366, 37.59804344963856);
		expected.m_Profile.AddPoint(12.920900799653081, 37.3856229450284);
		expected.m_Profile.AddPoint(13.020292344265798, 37.17320357276575);
		expected.m_Profile.AddPoint(13.119683888878512, 36.960784759261564);
		expected.m_Profile.AddPoint(13.21907543349123, 36.74836621702084);
		expected.m_Profile.AddPoint(13.318466978103945, 36.53594780434814);
		expected.m_Profile.AddPoint(13.417858522716662, 36.32352945256732);
		expected.m_Profile.AddPoint(13.517250067329377, 36.111111128943776);
		expected.m_Profile.AddPoint(13.616641611942095, 35.89869281813185);
		expected.m_Profile.AddPoint(13.71603315655481, 35.686274513056006);
		expected.m_Profile.AddPoint(13.815424701167526, 35.47385621050732);
		expected.m_Profile.AddPoint(13.91481624578024, 35.261437909054294);
		expected.m_Profile.AddPoint(14.014207790392959, 35.04901960806872);
		expected.m_Profile.AddPoint(14.113599335005674, 34.836601307279416);
		expected.m_Profile.AddPoint(14.212990879618388, 34.62418300657121);
		expected.m_Profile.AddPoint(14.312382424231105, 34.411764705895976);
		expected.m_Profile.AddPoint(14.411773968843823, 34.19934640523393);
		expected.m_Profile.AddPoint(14.511165513456538, 33.9869281045771);
		expected.m_Profile.AddPoint(14.610557058069253, 33.77450980392229);
		expected.m_Profile.AddPoint(14.70994860268197, 33.562091503268235);
		expected.m_Profile.AddPoint(14.809340147294686, 33.34967320261447);
		expected.m_Profile.AddPoint(14.9087316919074, 33.137254901960816);
		expected.m_Profile.AddPoint(15.008123236520117, 32.9248366013072);
		expected.m_Profile.AddPoint(15.107514781132835, 32.7124183006536);
		expected.m_Profile.AddPoint(15.20690632574555, 32.5);
		expected.m_fTxAgl = 2.0;
		expected.m_fRxAgl = 2.0;
		expected.m_bHasL2 = true;
		expected.m_eL2Code = RICON_EL2ResultCode.OK;
		expected.m_aLayers.Insert(new RICON_LossLayer("fspl", 90.06802055936305, RICON_TerrainDiffractionModel.MODEL_ID));
		expected.m_aLayers.Insert(new RICON_LossLayer("terrain_diffraction", 29.57991650777423, RICON_TerrainDiffractionModel.MODEL_ID));
		t.CheckCase("W19", true, query, config, source, expected);
	}

	protected static void Case19(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		query.m_vTxPos = Vector(0.0, 2.0, 0.0);
		query.m_vRxPos = Vector(1000.0, 302.0, 0.0);
		RICON_AnalyticHeightSource source;
		source = new RICON_AnalyticHeightSource();
		source.m_fBase = 0.0;
		source.m_fSlopeX = 0.0;
		source.m_fSlopeZ = 0.0;
		source.m_iFailAtCall = -1;
		source.m_iNanAtCall = -1;
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = true;
		expected.m_eProviderReason = RICON_EReason.OK;
		expected.m_iHeightCalls = 11;
		expected.m_bModel = true;
		expected.m_eModelReason = RICON_EReason.OK;
		expected.m_Profile = new RICON_TerrainProfile();
		expected.m_Profile.AddPoint(0.0, 0.0);
		expected.m_Profile.AddPoint(0.1, 0.0);
		expected.m_Profile.AddPoint(0.2, 0.0);
		expected.m_Profile.AddPoint(0.3, 0.0);
		expected.m_Profile.AddPoint(0.4, 0.0);
		expected.m_Profile.AddPoint(0.5, 0.0);
		expected.m_Profile.AddPoint(0.6, 0.0);
		expected.m_Profile.AddPoint(0.7, 0.0);
		expected.m_Profile.AddPoint(0.8, 0.0);
		expected.m_Profile.AddPoint(0.9, 0.0);
		expected.m_Profile.AddPoint(1.0, 0.0);
		expected.m_fTxAgl = 2.0;
		expected.m_fRxAgl = 302.0;
		expected.m_bHasL2 = true;
		expected.m_eL2Code = RICON_EL2ResultCode.OK;
		expected.m_aLayers.Insert(new RICON_LossLayer("fspl", 66.80144828801, RICON_TerrainDiffractionModel.MODEL_ID));
		expected.m_aLayers.Insert(new RICON_LossLayer("terrain_diffraction", 0.0, RICON_TerrainDiffractionModel.MODEL_ID));
		t.CheckCase("W20", true, query, config, source, expected);
	}

	protected static void Case20(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 400.0);
		query.m_vTxPos = Vector(0.0, 2.0, 0.0);
		query.m_vRxPos = Vector(5000.0, 2.0, 0.0);
		RICON_AnalyticHeightSource source;
		source = new RICON_AnalyticHeightSource();
		source.m_fBase = 0.0;
		source.m_fSlopeX = 0.0;
		source.m_fSlopeZ = 0.0;
		source.m_iFailAtCall = -1;
		source.m_iNanAtCall = -1;
		source.m_aHills.Insert(new RICON_TestHeightHill(2500.0, 0.0, 80.0, 300.0));
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = true;
		expected.m_eProviderReason = RICON_EReason.OK;
		expected.m_iHeightCalls = 51;
		expected.m_bModel = true;
		expected.m_eModelReason = RICON_EReason.OK;
		expected.m_Profile = new RICON_TerrainProfile();
		expected.m_Profile.AddPoint(0.0, 0.00000000000006659);
		expected.m_Profile.AddPoint(0.1, 0.00000000000101313);
		expected.m_Profile.AddPoint(0.2, 0.00000000001379297);
		expected.m_Profile.AddPoint(0.3, 0.00000000016803274);
		expected.m_Profile.AddPoint(0.4, 0.00000000183178788);
		expected.m_Profile.AddPoint(0.5, 0.00000001786905149);
		expected.m_Profile.AddPoint(0.6, 0.00000015598142288);
		expected.m_Profile.AddPoint(0.7, 0.00000121839837958);
		expected.m_Profile.AddPoint(0.8, 0.00000851629712886);
		expected.m_Profile.AddPoint(0.9, 0.00005326689175886);
		expected.m_Profile.AddPoint(1.0, 0.0002981322537662937);
		expected.m_Profile.AddPoint(1.1, 0.0014931575290816456);
		expected.m_Profile.AddPoint(1.2, 0.006691867778378202);
		expected.m_Profile.AddPoint(1.3, 0.026837010232200947);
		expected.m_Profile.AddPoint(1.4, 0.09630879958625649);
		expected.m_Profile.AddPoint(1.5, 0.3092736111578246);
		expected.m_Profile.AddPoint(1.6, 0.8887197230593845);
		expected.m_Profile.AddPoint(1.7, 2.2852400627640344);
		expected.m_Profile.AddPoint(1.8, 5.2582822893224375);
		expected.m_Profile.AddPoint(1.9, 10.826822658929016);
		expected.m_Profile.AddPoint(2.0, 19.948176702183698);
		expected.m_Profile.AddPoint(2.1, 32.888983240574994);
		expected.m_Profile.AddPoint(2.2, 48.52245277701067);
		expected.m_Profile.AddPoint(2.3, 64.05899223334464);
		expected.m_Profile.AddPoint(2.4, 75.67675751254123);
		expected.m_Profile.AddPoint(2.5, 80.0);
		expected.m_Profile.AddPoint(2.6, 75.67675751254123);
		expected.m_Profile.AddPoint(2.7, 64.05899223334464);
		expected.m_Profile.AddPoint(2.8, 48.5224527770106);
		expected.m_Profile.AddPoint(2.9, 32.888983240574994);
		expected.m_Profile.AddPoint(3.0, 19.948176702183698);
		expected.m_Profile.AddPoint(3.1, 10.826822658929016);
		expected.m_Profile.AddPoint(3.2, 5.2582822893224375);
		expected.m_Profile.AddPoint(3.3, 2.2852400627640304);
		expected.m_Profile.AddPoint(3.4, 0.8887197230593806);
		expected.m_Profile.AddPoint(3.5, 0.3092736111578246);
		expected.m_Profile.AddPoint(3.6, 0.09630879958625624);
		expected.m_Profile.AddPoint(3.7, 0.026837010232200947);
		expected.m_Profile.AddPoint(3.8, 0.006691867778378202);
		expected.m_Profile.AddPoint(3.9, 0.0014931575290816456);
		expected.m_Profile.AddPoint(4.0, 0.0002981322537662937);
		expected.m_Profile.AddPoint(4.1, 0.00005326689175886);
		expected.m_Profile.AddPoint(4.2, 0.00000851629712886);
		expected.m_Profile.AddPoint(4.3, 0.00000121839837958);
		expected.m_Profile.AddPoint(4.4, 0.00000015598142288);
		expected.m_Profile.AddPoint(4.5, 0.00000001786905149);
		expected.m_Profile.AddPoint(4.6, 0.00000000183178788);
		expected.m_Profile.AddPoint(4.7, 0.00000000016803274);
		expected.m_Profile.AddPoint(4.8, 0.00000000001379297);
		expected.m_Profile.AddPoint(4.9, 0.00000000000101313);
		expected.m_Profile.AddPoint(5.0, 0.00000000000006659);
		expected.m_fTxAgl = 1.9999999999999334;
		expected.m_fRxAgl = 1.9999999999999334;
		expected.m_bHasL2 = true;
		expected.m_eL2Code = RICON_EL2ResultCode.OK;
		expected.m_aLayers.Insert(new RICON_LossLayer("fspl", 98.468383135163, RICON_TerrainDiffractionModel.MODEL_ID));
		expected.m_aLayers.Insert(new RICON_LossLayer("terrain_diffraction", 33.939533034565684, RICON_TerrainDiffractionModel.MODEL_ID));
		t.CheckCase("W21", true, query, config, source, expected);
	}

	protected static void Case21(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		query.m_vTxPos = Vector(0.0, 2.0, 0.0);
		query.m_vRxPos = Vector(2000.0, 2.0, 0.0);
		RICON_AnalyticHeightSource source;
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = true;
		expected.m_eProviderReason = RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
		expected.m_iHeightCalls = 0;
		expected.m_bModel = true;
		expected.m_eModelReason = RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
		t.CheckCase("W22", true, query, config, source, expected);
	}

	protected static void Case22(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		query.m_vTxPos = Vector(0.0, 2.0, 0.0);
		query.m_vRxPos = Vector(2000.0, 2.0, 0.0);
		RICON_AnalyticHeightSource source;
		source = new RICON_AnalyticHeightSource();
		source.m_fBase = 0.0;
		source.m_fSlopeX = 0.0;
		source.m_fSlopeZ = 0.0;
		source.m_iFailAtCall = -1;
		source.m_iNanAtCall = -1;
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = true;
		expected.m_eProviderReason = RICON_EReason.OK;
		expected.m_iHeightCalls = 21;
		expected.m_Profile = new RICON_TerrainProfile();
		expected.m_Profile.AddPoint(0.0, 0.0);
		expected.m_Profile.AddPoint(0.1, 0.0);
		expected.m_Profile.AddPoint(0.2, 0.0);
		expected.m_Profile.AddPoint(0.3, 0.0);
		expected.m_Profile.AddPoint(0.4, 0.0);
		expected.m_Profile.AddPoint(0.5, 0.0);
		expected.m_Profile.AddPoint(0.6, 0.0);
		expected.m_Profile.AddPoint(0.7, 0.0);
		expected.m_Profile.AddPoint(0.8, 0.0);
		expected.m_Profile.AddPoint(0.9, 0.0);
		expected.m_Profile.AddPoint(1.0, 0.0);
		expected.m_Profile.AddPoint(1.1, 0.0);
		expected.m_Profile.AddPoint(1.2, 0.0);
		expected.m_Profile.AddPoint(1.3, 0.0);
		expected.m_Profile.AddPoint(1.4, 0.0);
		expected.m_Profile.AddPoint(1.5, 0.0);
		expected.m_Profile.AddPoint(1.6, 0.0);
		expected.m_Profile.AddPoint(1.7, 0.0);
		expected.m_Profile.AddPoint(1.8, 0.0);
		expected.m_Profile.AddPoint(1.9, 0.0);
		expected.m_Profile.AddPoint(2.0, 0.0);
		expected.m_fTxAgl = 2.0;
		expected.m_fRxAgl = 2.0;
		t.CheckCase("P01", false, query, config, source, expected);
	}

	protected static void Case23(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_LinkQuery query;
		RICON_AnalyticHeightSource source;
		source = new RICON_AnalyticHeightSource();
		source.m_fBase = 0.0;
		source.m_fSlopeX = 0.0;
		source.m_fSlopeZ = 0.0;
		source.m_iFailAtCall = -1;
		source.m_iNanAtCall = -1;
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = true;
		expected.m_eProviderReason = RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
		expected.m_iHeightCalls = 0;
		t.CheckCase("P02", false, query, config, source, expected);
	}

	protected static void Case24(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		query.m_vTxPos = Vector(0.0, 2.0, 0.0);
		query.m_vRxPos = Vector(2000.0, 2.0, 0.0);
		RICON_AnalyticHeightSource source;
		source = new RICON_AnalyticHeightSource();
		source.m_fBase = 0.0;
		source.m_fSlopeX = 0.0;
		source.m_fSlopeZ = 0.0;
		source.m_iFailAtCall = -1;
		source.m_iNanAtCall = -1;
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = true;
		expected.m_eProviderReason = RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
		expected.m_iHeightCalls = 0;
		t.CheckCase("P03", false, query, config, source, expected);
	}

	protected static void Case25(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 0.0, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		query.m_vTxPos = Vector(0.0, 2.0, 0.0);
		query.m_vRxPos = Vector(2000.0, 2.0, 0.0);
		RICON_AnalyticHeightSource source;
		source = new RICON_AnalyticHeightSource();
		source.m_fBase = 0.0;
		source.m_fSlopeX = 0.0;
		source.m_fSlopeZ = 0.0;
		source.m_iFailAtCall = -1;
		source.m_iNanAtCall = -1;
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = true;
		expected.m_eProviderReason = RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
		expected.m_iHeightCalls = 0;
		t.CheckCase("P04", false, query, config, source, expected);
	}

	protected static void Case26(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		query.m_vTxPos = Vector(0.0, 2.0, 0.0);
		query.m_vRxPos = Vector(2000.0, 2.0, 0.0);
		RICON_AnalyticHeightSource source;
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = true;
		expected.m_eProviderReason = RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
		expected.m_iHeightCalls = 0;
		t.CheckCase("P05", false, query, config, source, expected);
	}

	protected static void Case27(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		query.m_vTxPos = Vector(RICON_L0Run.Special(0), 2.0, 0.0);
		query.m_vRxPos = Vector(2000.0, 2.0, 0.0);
		RICON_AnalyticHeightSource source;
		source = new RICON_AnalyticHeightSource();
		source.m_fBase = 0.0;
		source.m_fSlopeX = 0.0;
		source.m_fSlopeZ = 0.0;
		source.m_iFailAtCall = -1;
		source.m_iNanAtCall = -1;
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = true;
		expected.m_eProviderReason = RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
		expected.m_iHeightCalls = 0;
		t.CheckCase("P06", false, query, config, source, expected);
	}

	protected static void Case28(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		query.m_vTxPos = Vector(RICON_L0Run.Special(1), 2.0, 0.0);
		query.m_vRxPos = Vector(2000.0, 2.0, 0.0);
		RICON_AnalyticHeightSource source;
		source = new RICON_AnalyticHeightSource();
		source.m_fBase = 0.0;
		source.m_fSlopeX = 0.0;
		source.m_fSlopeZ = 0.0;
		source.m_iFailAtCall = -1;
		source.m_iNanAtCall = -1;
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = true;
		expected.m_eProviderReason = RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
		expected.m_iHeightCalls = 0;
		t.CheckCase("P07", false, query, config, source, expected);
	}

	protected static void Case29(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		query.m_vTxPos = Vector(0.0, RICON_L0Run.Special(0), 0.0);
		query.m_vRxPos = Vector(2000.0, 2.0, 0.0);
		RICON_AnalyticHeightSource source;
		source = new RICON_AnalyticHeightSource();
		source.m_fBase = 0.0;
		source.m_fSlopeX = 0.0;
		source.m_fSlopeZ = 0.0;
		source.m_iFailAtCall = -1;
		source.m_iNanAtCall = -1;
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = true;
		expected.m_eProviderReason = RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
		expected.m_iHeightCalls = 0;
		t.CheckCase("P08", false, query, config, source, expected);
	}

	protected static void Case30(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		query.m_vTxPos = Vector(0.0, RICON_L0Run.Special(-1), 0.0);
		query.m_vRxPos = Vector(2000.0, 2.0, 0.0);
		RICON_AnalyticHeightSource source;
		source = new RICON_AnalyticHeightSource();
		source.m_fBase = 0.0;
		source.m_fSlopeX = 0.0;
		source.m_fSlopeZ = 0.0;
		source.m_iFailAtCall = -1;
		source.m_iNanAtCall = -1;
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = true;
		expected.m_eProviderReason = RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
		expected.m_iHeightCalls = 0;
		t.CheckCase("P09", false, query, config, source, expected);
	}

	protected static void Case31(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		query.m_vTxPos = Vector(0.0, 2.0, RICON_L0Run.Special(0));
		query.m_vRxPos = Vector(2000.0, 2.0, 0.0);
		RICON_AnalyticHeightSource source;
		source = new RICON_AnalyticHeightSource();
		source.m_fBase = 0.0;
		source.m_fSlopeX = 0.0;
		source.m_fSlopeZ = 0.0;
		source.m_iFailAtCall = -1;
		source.m_iNanAtCall = -1;
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = true;
		expected.m_eProviderReason = RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
		expected.m_iHeightCalls = 0;
		t.CheckCase("P10", false, query, config, source, expected);
	}

	protected static void Case32(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		query.m_vTxPos = Vector(0.0, 2.0, RICON_L0Run.Special(1));
		query.m_vRxPos = Vector(2000.0, 2.0, 0.0);
		RICON_AnalyticHeightSource source;
		source = new RICON_AnalyticHeightSource();
		source.m_fBase = 0.0;
		source.m_fSlopeX = 0.0;
		source.m_fSlopeZ = 0.0;
		source.m_iFailAtCall = -1;
		source.m_iNanAtCall = -1;
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = true;
		expected.m_eProviderReason = RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
		expected.m_iHeightCalls = 0;
		t.CheckCase("P11", false, query, config, source, expected);
	}

	protected static void Case33(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		query.m_vTxPos = Vector(0.0, 2.0, 0.0);
		query.m_vRxPos = Vector(RICON_L0Run.Special(0), 2.0, 0.0);
		RICON_AnalyticHeightSource source;
		source = new RICON_AnalyticHeightSource();
		source.m_fBase = 0.0;
		source.m_fSlopeX = 0.0;
		source.m_fSlopeZ = 0.0;
		source.m_iFailAtCall = -1;
		source.m_iNanAtCall = -1;
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = true;
		expected.m_eProviderReason = RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
		expected.m_iHeightCalls = 0;
		t.CheckCase("P12", false, query, config, source, expected);
	}

	protected static void Case34(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		query.m_vTxPos = Vector(0.0, 2.0, 0.0);
		query.m_vRxPos = Vector(RICON_L0Run.Special(1), 2.0, 0.0);
		RICON_AnalyticHeightSource source;
		source = new RICON_AnalyticHeightSource();
		source.m_fBase = 0.0;
		source.m_fSlopeX = 0.0;
		source.m_fSlopeZ = 0.0;
		source.m_iFailAtCall = -1;
		source.m_iNanAtCall = -1;
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = true;
		expected.m_eProviderReason = RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
		expected.m_iHeightCalls = 0;
		t.CheckCase("P13", false, query, config, source, expected);
	}

	protected static void Case35(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		query.m_vTxPos = Vector(0.0, 2.0, 0.0);
		query.m_vRxPos = Vector(2000.0, RICON_L0Run.Special(0), 0.0);
		RICON_AnalyticHeightSource source;
		source = new RICON_AnalyticHeightSource();
		source.m_fBase = 0.0;
		source.m_fSlopeX = 0.0;
		source.m_fSlopeZ = 0.0;
		source.m_iFailAtCall = -1;
		source.m_iNanAtCall = -1;
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = true;
		expected.m_eProviderReason = RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
		expected.m_iHeightCalls = 0;
		t.CheckCase("P14", false, query, config, source, expected);
	}

	protected static void Case36(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		query.m_vTxPos = Vector(0.0, 2.0, 0.0);
		query.m_vRxPos = Vector(2000.0, RICON_L0Run.Special(-1), 0.0);
		RICON_AnalyticHeightSource source;
		source = new RICON_AnalyticHeightSource();
		source.m_fBase = 0.0;
		source.m_fSlopeX = 0.0;
		source.m_fSlopeZ = 0.0;
		source.m_iFailAtCall = -1;
		source.m_iNanAtCall = -1;
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = true;
		expected.m_eProviderReason = RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
		expected.m_iHeightCalls = 0;
		t.CheckCase("P15", false, query, config, source, expected);
	}

	protected static void Case37(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		query.m_vTxPos = Vector(0.0, 2.0, 0.0);
		query.m_vRxPos = Vector(2000.0, 2.0, RICON_L0Run.Special(0));
		RICON_AnalyticHeightSource source;
		source = new RICON_AnalyticHeightSource();
		source.m_fBase = 0.0;
		source.m_fSlopeX = 0.0;
		source.m_fSlopeZ = 0.0;
		source.m_iFailAtCall = -1;
		source.m_iNanAtCall = -1;
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = true;
		expected.m_eProviderReason = RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
		expected.m_iHeightCalls = 0;
		t.CheckCase("P16", false, query, config, source, expected);
	}

	protected static void Case38(RICON_WorldProfileRun t)
	{
		RICON_TerrainDiffractionConfig config;
		config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_LinkQuery query;
		query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		query.m_vTxPos = Vector(0.0, 2.0, 0.0);
		query.m_vRxPos = Vector(2000.0, 2.0, RICON_L0Run.Special(1));
		RICON_AnalyticHeightSource source;
		source = new RICON_AnalyticHeightSource();
		source.m_fBase = 0.0;
		source.m_fSlopeX = 0.0;
		source.m_fSlopeZ = 0.0;
		source.m_iFailAtCall = -1;
		source.m_iNanAtCall = -1;
		RICON_WorldProfileObservation expected = new RICON_WorldProfileObservation();
		expected.m_bProviderCalled = true;
		expected.m_eProviderReason = RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
		expected.m_iHeightCalls = 0;
		t.CheckCase("P17", false, query, config, source, expected);
	}

	static void Run(RICON_WorldProfileRun t)
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
	}
}
