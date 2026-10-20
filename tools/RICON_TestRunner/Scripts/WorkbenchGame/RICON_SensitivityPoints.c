//! Сгенерировано gen_sensitivity_points.py; TEST_ONLY, ожидания только копируются.
class RICON_SensitivityPoints
{
	static const string SOURCE_SHA = "8e80dabe4c9450aa76e4fed5b947072dd319b327f0838b829bbbc752fc136271";

	//! A01-p527-dry-30-V
	protected static void Anchor0(RICON_SensitivityProbePlugin t)
	{
		RICON_TerrainDiffractionConfig config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 3.762113290322435, 0.009829979534419586, RICON_ETerrainPolarization.VERTICAL, 0.1, 100000);
		RICON_L2Result expected = new RICON_L2Result();
		expected.m_fLossDb = 52.984249493341835;
		expected.m_fBullingtonActualDb = 12.497397915555577;
		expected.m_fBullingtonSmoothDb = 12.497397915555577;
		expected.m_fSphericalDb = 52.984249493341835;
		expected.m_eBranchActual = RICON_EL2Branch.LOS;
		expected.m_eBranchSmooth = RICON_EL2Branch.LOS;
		expected.m_eSphericalCase = RICON_EL2SphericalCase.INTERPOLATED;
		expected.m_eFirstTermAccuracy = RICON_EFirstTermAccuracy.NOT_MET;
		t.CheckPoint("A01-p527-dry-30-V", 10000.0, 30.0, 1.5, 0.0, 1.0, 0.0, config, expected, 101, 0.0);
	}

	//! A02-p527-dry-30-H
	protected static void Anchor1(RICON_SensitivityProbePlugin t)
	{
		RICON_TerrainDiffractionConfig config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 3.762113290322435, 0.009829979534419586, RICON_ETerrainPolarization.HORIZONTAL, 0.1, 100000);
		RICON_L2Result expected = new RICON_L2Result();
		expected.m_fLossDb = 71.38118219622037;
		expected.m_fBullingtonActualDb = 12.497397915555577;
		expected.m_fBullingtonSmoothDb = 12.497397915555577;
		expected.m_fSphericalDb = 71.38118219622037;
		expected.m_eBranchActual = RICON_EL2Branch.LOS;
		expected.m_eBranchSmooth = RICON_EL2Branch.LOS;
		expected.m_eSphericalCase = RICON_EL2SphericalCase.INTERPOLATED;
		expected.m_eFirstTermAccuracy = RICON_EFirstTermAccuracy.NOT_MET;
		t.CheckPoint("A02-p527-dry-30-H", 10000.0, 30.0, 1.5, 0.0, 1.0, 0.0, config, expected, 101, 0.0);
	}

	//! A03-fig24-B-30-V
	protected static void Anchor2(RICON_SensitivityProbePlugin t)
	{
		RICON_TerrainDiffractionConfig config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 30.0, 0.01, RICON_ETerrainPolarization.VERTICAL, 0.1, 100000);
		RICON_L2Result expected = new RICON_L2Result();
		expected.m_fLossDb = 40.48964259662578;
		expected.m_fBullingtonActualDb = 12.497397915555577;
		expected.m_fBullingtonSmoothDb = 12.497397915555577;
		expected.m_fSphericalDb = 40.48964259662578;
		expected.m_eBranchActual = RICON_EL2Branch.LOS;
		expected.m_eBranchSmooth = RICON_EL2Branch.LOS;
		expected.m_eSphericalCase = RICON_EL2SphericalCase.INTERPOLATED;
		expected.m_eFirstTermAccuracy = RICON_EFirstTermAccuracy.NOT_MET;
		t.CheckPoint("A03-fig24-B-30-V", 10000.0, 30.0, 1.5, 0.0, 1.0, 0.0, config, expected, 101, 0.0);
	}

	//! A04-fig24-E-30-V
	protected static void Anchor3(RICON_SensitivityProbePlugin t)
	{
		RICON_TerrainDiffractionConfig config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 3.0, 0.0001, RICON_ETerrainPolarization.VERTICAL, 0.1, 100000);
		RICON_L2Result expected = new RICON_L2Result();
		expected.m_fLossDb = 57.4396180841584;
		expected.m_fBullingtonActualDb = 12.497397915555577;
		expected.m_fBullingtonSmoothDb = 12.497397915555577;
		expected.m_fSphericalDb = 57.43961808415839;
		expected.m_eBranchActual = RICON_EL2Branch.LOS;
		expected.m_eBranchSmooth = RICON_EL2Branch.LOS;
		expected.m_eSphericalCase = RICON_EL2SphericalCase.INTERPOLATED;
		expected.m_eFirstTermAccuracy = RICON_EFirstTermAccuracy.NOT_MET;
		t.CheckPoint("A04-fig24-E-30-V", 10000.0, 30.0, 1.5, 0.0, 1.0, 0.0, config, expected, 101, 0.0);
	}

	//! A05-p527-clay-88-V
	protected static void Anchor4(RICON_SensitivityProbePlugin t)
	{
		RICON_TerrainDiffractionConfig config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 23.05313321227579, 0.01090984938323776, RICON_ETerrainPolarization.VERTICAL, 0.1, 100000);
		RICON_L2Result expected = new RICON_L2Result();
		expected.m_fLossDb = 52.44679689395747;
		expected.m_fBullingtonActualDb = 12.494850869212595;
		expected.m_fBullingtonSmoothDb = 12.494850869212595;
		expected.m_fSphericalDb = 52.44679689395747;
		expected.m_eBranchActual = RICON_EL2Branch.LOS;
		expected.m_eBranchSmooth = RICON_EL2Branch.LOS;
		expected.m_eSphericalCase = RICON_EL2SphericalCase.INTERPOLATED;
		expected.m_eFirstTermAccuracy = RICON_EFirstTermAccuracy.NOT_MET;
		t.CheckPoint("A05-p527-clay-88-V", 10000.0, 88.0, 1.5, 0.0, 1.0, 0.0, config, expected, 101, 0.0);
	}

	//! A06-p527-clay-88-H
	protected static void Anchor5(RICON_SensitivityProbePlugin t)
	{
		RICON_TerrainDiffractionConfig config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 23.05313321227579, 0.01090984938323776, RICON_ETerrainPolarization.HORIZONTAL, 0.1, 100000);
		RICON_L2Result expected = new RICON_L2Result();
		expected.m_fLossDb = 62.31199779563731;
		expected.m_fBullingtonActualDb = 12.494850869212595;
		expected.m_fBullingtonSmoothDb = 12.494850869212595;
		expected.m_fSphericalDb = 62.31199779563731;
		expected.m_eBranchActual = RICON_EL2Branch.LOS;
		expected.m_eBranchSmooth = RICON_EL2Branch.LOS;
		expected.m_eSphericalCase = RICON_EL2SphericalCase.INTERPOLATED;
		expected.m_eFirstTermAccuracy = RICON_EFirstTermAccuracy.NOT_MET;
		t.CheckPoint("A06-p527-clay-88-H", 10000.0, 88.0, 1.5, 0.0, 1.0, 0.0, config, expected, 101, 0.0);
	}

	//! A07-p527-mv30-400-V
	protected static void Anchor6(RICON_SensitivityProbePlugin t)
	{
		RICON_TerrainDiffractionConfig config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.070194268064137, 0.04298450891906116, RICON_ETerrainPolarization.VERTICAL, 0.1, 100000);
		RICON_L2Result expected = new RICON_L2Result();
		expected.m_fLossDb = 49.90337588373998;
		expected.m_fBullingtonActualDb = 12.4879209546304;
		expected.m_fBullingtonSmoothDb = 12.4879209546304;
		expected.m_fSphericalDb = 49.90337588373999;
		expected.m_eBranchActual = RICON_EL2Branch.LOS;
		expected.m_eBranchSmooth = RICON_EL2Branch.LOS;
		expected.m_eSphericalCase = RICON_EL2SphericalCase.INTERPOLATED;
		expected.m_eFirstTermAccuracy = RICON_EFirstTermAccuracy.NOT_MET;
		t.CheckPoint("A07-p527-mv30-400-V", 10000.0, 400.0, 1.5, 0.0, 1.0, 0.0, config, expected, 101, 0.0);
	}

	//! A08-matrix-50-H
	protected static void Anchor7(RICON_SensitivityProbePlugin t)
	{
		RICON_TerrainDiffractionConfig config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.005, RICON_ETerrainPolarization.HORIZONTAL, 0.1, 100000);
		RICON_L2Result expected = new RICON_L2Result();
		expected.m_fLossDb = 67.0583401059969;
		expected.m_fBullingtonActualDb = 12.496357981909313;
		expected.m_fBullingtonSmoothDb = 12.496357981909313;
		expected.m_fSphericalDb = 67.0583401059969;
		expected.m_eBranchActual = RICON_EL2Branch.LOS;
		expected.m_eBranchSmooth = RICON_EL2Branch.LOS;
		expected.m_eSphericalCase = RICON_EL2SphericalCase.INTERPOLATED;
		expected.m_eFirstTermAccuracy = RICON_EFirstTermAccuracy.NOT_MET;
		t.CheckPoint("A08-matrix-50-H", 10000.0, 50.0, 1.5, 0.0, 1.0, 0.0, config, expected, 101, 0.0);
	}

	//! A09-hw30-s100-on-node
	protected static void Anchor8(RICON_SensitivityProbePlugin t)
	{
		RICON_TerrainDiffractionConfig config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.005, RICON_ETerrainPolarization.VERTICAL, 0.1, 100000);
		RICON_L2Result expected = new RICON_L2Result();
		expected.m_fLossDb = 31.785291304308803;
		expected.m_fBullingtonActualDb = 20.295274307109473;
		expected.m_fBullingtonSmoothDb = 9.350358920063659;
		expected.m_fSphericalDb = 20.84037591726299;
		expected.m_eBranchActual = RICON_EL2Branch.TRANS;
		expected.m_eBranchSmooth = RICON_EL2Branch.LOS;
		expected.m_eSphericalCase = RICON_EL2SphericalCase.INTERPOLATED;
		expected.m_eFirstTermAccuracy = RICON_EFirstTermAccuracy.NOT_MET;
		t.CheckPoint("A09-hw30-s100-on-node", 10000.0, 30.0, 1.5, 2.0, 0.03, 60.0, config, expected, 101, 60.0);
	}

	//! A10-hw30-s100-missed
	protected static void Anchor9(RICON_SensitivityProbePlugin t)
	{
		RICON_TerrainDiffractionConfig config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.005, RICON_ETerrainPolarization.VERTICAL, 0.1, 100000);
		RICON_L2Result expected = new RICON_L2Result();
		expected.m_fLossDb = 46.229900886306694;
		expected.m_fBullingtonActualDb = 12.497397915555577;
		expected.m_fBullingtonSmoothDb = 12.497397915555577;
		expected.m_fSphericalDb = 46.22990088630669;
		expected.m_eBranchActual = RICON_EL2Branch.LOS;
		expected.m_eBranchSmooth = RICON_EL2Branch.LOS;
		expected.m_eSphericalCase = RICON_EL2SphericalCase.INTERPOLATED;
		expected.m_eFirstTermAccuracy = RICON_EFirstTermAccuracy.NOT_MET;
		t.CheckPoint("A10-hw30-s100-missed", 10000.0, 30.0, 1.5, 2.05, 0.03, 60.0, config, expected, 101, 0.0);
	}

	//! A11-hw30-s50-on-node
	protected static void Anchor10(RICON_SensitivityProbePlugin t)
	{
		RICON_TerrainDiffractionConfig config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.005, RICON_ETerrainPolarization.VERTICAL, 0.05, 100000);
		RICON_L2Result expected = new RICON_L2Result();
		expected.m_fLossDb = 31.800671159697938;
		expected.m_fBullingtonActualDb = 20.295274307109473;
		expected.m_fBullingtonSmoothDb = 9.348670525409798;
		expected.m_fSphericalDb = 20.854067377998263;
		expected.m_eBranchActual = RICON_EL2Branch.TRANS;
		expected.m_eBranchSmooth = RICON_EL2Branch.LOS;
		expected.m_eSphericalCase = RICON_EL2SphericalCase.INTERPOLATED;
		expected.m_eFirstTermAccuracy = RICON_EFirstTermAccuracy.NOT_MET;
		t.CheckPoint("A11-hw30-s50-on-node", 10000.0, 30.0, 1.5, 2.0, 0.03, 60.0, config, expected, 201, 60.0);
	}

	//! A12-hw30-s50-phase3
	protected static void Anchor11(RICON_SensitivityProbePlugin t)
	{
		RICON_TerrainDiffractionConfig config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.005, RICON_ETerrainPolarization.VERTICAL, 0.05, 100000);
		RICON_L2Result expected = new RICON_L2Result();
		expected.m_fLossDb = 40.03960486411805;
		expected.m_fBullingtonActualDb = 15.739950785550908;
		expected.m_fBullingtonSmoothDb = 11.36204533773913;
		expected.m_fSphericalDb = 35.66169941630627;
		expected.m_eBranchActual = RICON_EL2Branch.TRANS;
		expected.m_eBranchSmooth = RICON_EL2Branch.LOS;
		expected.m_eSphericalCase = RICON_EL2SphericalCase.INTERPOLATED;
		expected.m_eFirstTermAccuracy = RICON_EFirstTermAccuracy.NOT_MET;
		t.CheckPoint("A12-hw30-s50-phase3", 10000.0, 30.0, 1.5, 2.01875, 0.03, 60.0, config, expected, 201, 22.50000000000035);
	}

	//! A13-hw100-s100-400-h10
	protected static void Anchor12(RICON_SensitivityProbePlugin t)
	{
		RICON_TerrainDiffractionConfig config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.005, RICON_ETerrainPolarization.VERTICAL, 0.1, 100000);
		RICON_L2Result expected = new RICON_L2Result();
		expected.m_fLossDb = 23.55645035265345;
		expected.m_fBullingtonActualDb = 22.030600287848152;
		expected.m_fBullingtonSmoothDb = 3.686433340535658;
		expected.m_fSphericalDb = 5.212283405340957;
		expected.m_eBranchActual = RICON_EL2Branch.TRANS;
		expected.m_eBranchSmooth = RICON_EL2Branch.LOS;
		expected.m_eSphericalCase = RICON_EL2SphericalCase.INTERPOLATED;
		expected.m_eFirstTermAccuracy = RICON_EFirstTermAccuracy.MET;
		t.CheckPoint("A13-hw100-s100-400-h10", 10000.0, 400.0, 10.0, 2.05, 0.1, 60.0, config, expected, 101, 30.000000000000107);
	}

	//! A14-hw500-s100-worst
	protected static void Anchor13(RICON_SensitivityProbePlugin t)
	{
		RICON_TerrainDiffractionConfig config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.005, RICON_ETerrainPolarization.VERTICAL, 0.1, 100000);
		RICON_L2Result expected = new RICON_L2Result();
		expected.m_fLossDb = 32.610394442121134;
		expected.m_fBullingtonActualDb = 19.68696031660626;
		expected.m_fBullingtonSmoothDb = 9.74047918466272;
		expected.m_fSphericalDb = 22.663913310177595;
		expected.m_eBranchActual = RICON_EL2Branch.TRANS;
		expected.m_eBranchSmooth = RICON_EL2Branch.LOS;
		expected.m_eSphericalCase = RICON_EL2SphericalCase.INTERPOLATED;
		expected.m_eFirstTermAccuracy = RICON_EFirstTermAccuracy.NOT_MET;
		t.CheckPoint("A14-hw500-s100-worst", 10000.0, 30.0, 1.5, 2.05, 0.5, 60.0, config, expected, 101, 54.00000000000002);
	}

	//! A15-hw1900-s100-shape
	protected static void Anchor14(RICON_SensitivityProbePlugin t)
	{
		RICON_TerrainDiffractionConfig config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.005, RICON_ETerrainPolarization.VERTICAL, 0.1, 100000);
		RICON_L2Result expected = new RICON_L2Result();
		expected.m_fLossDb = 35.4071846736376;
		expected.m_fBullingtonActualDb = 20.295274307109473;
		expected.m_fBullingtonSmoothDb = 10.209030816361278;
		expected.m_fSphericalDb = 25.320941182889406;
		expected.m_eBranchActual = RICON_EL2Branch.TRANS;
		expected.m_eBranchSmooth = RICON_EL2Branch.LOS;
		expected.m_eSphericalCase = RICON_EL2SphericalCase.INTERPOLATED;
		expected.m_eFirstTermAccuracy = RICON_EFirstTermAccuracy.NOT_MET;
		t.CheckPoint("A15-hw1900-s100-shape", 10000.0, 30.0, 1.5, 2.0, 1.9, 60.0, config, expected, 101, 60.0);
	}

	//! A16-hw30-s2-reference
	protected static void Anchor15(RICON_SensitivityProbePlugin t)
	{
		RICON_TerrainDiffractionConfig config = RICON_L2Run.Config(6371.0, 1.3333333333333333, 15.0, 0.005, RICON_ETerrainPolarization.VERTICAL, 0.002, 100000);
		RICON_L2Result expected = new RICON_L2Result();
		expected.m_fLossDb = 31.808201225556015;
		expected.m_fBullingtonActualDb = 20.295274307109473;
		expected.m_fBullingtonSmoothDb = 9.348215243466075;
		expected.m_fSphericalDb = 20.861142161912618;
		expected.m_eBranchActual = RICON_EL2Branch.TRANS;
		expected.m_eBranchSmooth = RICON_EL2Branch.LOS;
		expected.m_eSphericalCase = RICON_EL2SphericalCase.INTERPOLATED;
		expected.m_eFirstTermAccuracy = RICON_EFirstTermAccuracy.NOT_MET;
		t.CheckPoint("A16-hw30-s2-reference", 10000.0, 30.0, 1.5, 2.0, 0.03, 60.0, config, expected, 5001, 60.0);
	}

	static void Run(RICON_SensitivityProbePlugin t)
	{
		Anchor0(t);
		Anchor1(t);
		Anchor2(t);
		Anchor3(t);
		Anchor4(t);
		Anchor5(t);
		Anchor6(t);
		Anchor7(t);
		Anchor8(t);
		Anchor9(t);
		Anchor10(t);
		Anchor11(t);
		Anchor12(t);
		Anchor13(t);
		Anchor14(t);
		Anchor15(t);
	}
}
