// Terrain diffraction model and pure calculator (REQ-L2-terrain v0.5, ITU-R P.526-16 section 4.5).
// Evidence level is recorded in docs/evidence/E-workbench-source.md. No world lookup or runtime provider lives here.

enum RICON_EL2ResultCode
{
	INVALID_CONFIG = 1,
	OUT_OF_DOMAIN_FREQUENCY = 2,
	INVALID_PROFILE = 3,
	PROFILE_STEP_TOO_LARGE = 4,
	INVALID_ANTENNA_HEIGHT = 5,
	NUMERIC_FAILURE = 6,
	OK = 7
}

enum RICON_ETerrainPolarization
{
	VERTICAL = 0,
	HORIZONTAL = 1
}

enum RICON_EL2Branch
{
	LOS = 0,
	TRANS = 1
}

enum RICON_EL2SphericalCase
{
	BEYOND = 0,
	CLEAR = 1,
	AH_NEGATIVE = 2,
	INTERPOLATED = 3
}

enum RICON_EFirstTermAccuracy
{
	MET = 0,
	NOT_MET = 1,
	NOT_EVALUATED = 2
}

enum RICON_ETerrainSamplingCode
{
	OK = 0,
	SKIP = 1,
	UNAVAILABLE = 2
}

class RICON_TerrainDiffractionConfig
{
	float m_fEarthRadiusKm;
	float m_fKFactor;
	float m_fPermittivity;
	float m_fConductivitySm;
	RICON_ETerrainPolarization m_ePolarization;
	float m_fMaxProfileStepKm;
	int m_iMaxProfilePoints;

	bool IsValid()
	{
		if (!RICON_Num.IsFinite(m_fEarthRadiusKm) || m_fEarthRadiusKm < 1000.0 || m_fEarthRadiusKm > 100000.0)
			return false;
		if (!RICON_Num.IsFinite(m_fKFactor) || m_fKFactor < 0.1 || m_fKFactor > 100.0)
			return false;
		if (!RICON_Num.IsFinite(m_fPermittivity) || m_fPermittivity <= 1.0 || m_fPermittivity > 1000.0)
			return false;
		if (!RICON_Num.IsFinite(m_fConductivitySm) || m_fConductivitySm < 0.0 || m_fConductivitySm > 1000.0)
			return false;
		if (m_ePolarization != RICON_ETerrainPolarization.VERTICAL
			&& m_ePolarization != RICON_ETerrainPolarization.HORIZONTAL)
			return false;
		if (!RICON_Num.IsFinite(m_fMaxProfileStepKm) || m_fMaxProfileStepKm < 0.002
			|| m_fMaxProfileStepKm > 10.0)
			return false;
		return m_iMaxProfilePoints >= 3 && m_iMaxProfilePoints <= 100000;
	}

	RICON_TerrainDiffractionConfig CloneConfig()
	{
		RICON_TerrainDiffractionConfig copy = new RICON_TerrainDiffractionConfig();
		copy.m_fEarthRadiusKm = m_fEarthRadiusKm;
		copy.m_fKFactor = m_fKFactor;
		copy.m_fPermittivity = m_fPermittivity;
		copy.m_fConductivitySm = m_fConductivitySm;
		copy.m_ePolarization = m_ePolarization;
		copy.m_fMaxProfileStepKm = m_fMaxProfileStepKm;
		copy.m_iMaxProfilePoints = m_iMaxProfilePoints;
		return copy;
	}
}

class RICON_TerrainProfile
{
	ref array<float> m_aDistanceKm = {};
	ref array<float> m_aHeightM = {};

	bool AddPoint(float distanceKm, float heightM)
	{
		m_aDistanceKm.Insert(distanceKm);
		m_aHeightM.Insert(heightM);
		return true;
	}

	int Count()
	{
		return m_aDistanceKm.Count();
	}

	RICON_TerrainProfile CloneProfile()
	{
		if (!m_aDistanceKm || !m_aHeightM || m_aDistanceKm.Count() != m_aHeightM.Count())
			return null;
		RICON_TerrainProfile copy = new RICON_TerrainProfile();
		for (int i = 0; i < m_aDistanceKm.Count(); i++)
			copy.AddPoint(m_aDistanceKm[i], m_aHeightM[i]);
		return copy;
	}
}

class RICON_L2Result
{
	RICON_EL2ResultCode m_eCode = RICON_EL2ResultCode.NUMERIC_FAILURE;
	float m_fLossDb;
	float m_fBullingtonActualDb;
	float m_fBullingtonSmoothDb;
	float m_fSphericalDb;
	RICON_EL2Branch m_eBranchActual;
	RICON_EL2Branch m_eBranchSmooth;
	RICON_EL2SphericalCase m_eSphericalCase;
	RICON_EFirstTermAccuracy m_eFirstTermAccuracy = RICON_EFirstTermAccuracy.NOT_EVALUATED;
}

class RICON_TerrainSamplingResult
{
	RICON_ETerrainSamplingCode m_eCode = RICON_ETerrainSamplingCode.UNAVAILABLE;
	ref array<float> m_aDistanceKm = {};
}

//! Reference sampling rule only. A runtime provider supplies the corresponding ground heights later.
class RICON_TerrainProfileSampling
{
	static RICON_TerrainSamplingResult BuildDistances(float horizontalDistanceM, RICON_TerrainDiffractionConfig config)
	{
		RICON_TerrainSamplingResult result = new RICON_TerrainSamplingResult();
		if (!config || !config.IsValid() || !RICON_Num.IsFinite(horizontalDistanceM) || horizontalDistanceM < 0)
			return result;

		float distanceKm = horizontalDistanceM / 1000.0;
		if (distanceKm < 0.002)
		{
			result.m_eCode = RICON_ETerrainSamplingCode.SKIP;
			return result;
		}
		if (distanceKm > 500.0)
			return result;

		float intervalsFloat = Math.Ceil(distanceKm / config.m_fMaxProfileStepKm);
		if (!RICON_Num.IsFinite(intervalsFloat) || intervalsFloat > config.m_iMaxProfilePoints - 1)
			return result;
		int intervals = intervalsFloat;
		int pointCount = intervals + 1;
		if (pointCount < 3)
			pointCount = 3;
		if (pointCount > config.m_iMaxProfilePoints)
			return result;

		for (int i = 0; i < pointCount; i++)
			result.m_aDistanceKm.Insert(distanceKm * i / (pointCount - 1));
		result.m_eCode = RICON_ETerrainSamplingCode.OK;
		return result;
	}
}

//! Pure profile seam. The production BaseWorld provider is intentionally absent until runtime is authorized.
class RICON_TerrainProfileProvider
{
	RICON_EReason GetProfile(
		RICON_LinkQuery query,
		RICON_TerrainDiffractionConfig config,
		out RICON_TerrainProfile profile,
		out float transmitterHeightAglM,
		out float receiverHeightAglM)
	{
		profile = null;
		transmitterHeightAglM = 0;
		receiverHeightAglM = 0;
		return RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
	}
}

//! TEST_ONLY provider used by parity fixtures; it owns a clone and never reads the world.
class RICON_SyntheticTerrainProfileProvider : RICON_TerrainProfileProvider
{
	protected ref RICON_TerrainProfile m_Profile;
	protected float m_fTransmitterHeightAglM;
	protected float m_fReceiverHeightAglM;
	protected RICON_EReason m_eReason;

	void RICON_SyntheticTerrainProfileProvider(
		RICON_TerrainProfile profile,
		float transmitterHeightAglM,
		float receiverHeightAglM,
		RICON_EReason reason = RICON_EReason.OK)
	{
		if (profile)
			m_Profile = profile.CloneProfile();
		m_fTransmitterHeightAglM = transmitterHeightAglM;
		m_fReceiverHeightAglM = receiverHeightAglM;
		m_eReason = reason;
	}

	override RICON_EReason GetProfile(
		RICON_LinkQuery query,
		RICON_TerrainDiffractionConfig config,
		out RICON_TerrainProfile profile,
		out float transmitterHeightAglM,
		out float receiverHeightAglM)
	{
		profile = null;
		transmitterHeightAglM = m_fTransmitterHeightAglM;
		receiverHeightAglM = m_fReceiverHeightAglM;
		if (m_eReason != RICON_EReason.OK)
			return m_eReason;
		if (!m_Profile)
			return RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
		profile = m_Profile.CloneProfile();
		return RICON_EReason.OK;
	}
}

class RICON_L2BullingtonResult
{
	float m_fLossDb;
	RICON_EL2Branch m_eBranch;
}

class RICON_L2FieldResult
{
	float m_fRelativeDb;
	RICON_EFirstTermAccuracy m_eAccuracy;
}

class RICON_L2SphericalResult
{
	float m_fLossDb;
	RICON_EL2SphericalCase m_eCase;
	RICON_EFirstTermAccuracy m_eAccuracy;
}

//! Pure float32 implementation of the normative practical-unit equations in REQ-L2-terrain v0.5.
class RICON_TerrainDiffractionCalculator
{
	static const float SPEED_OF_LIGHT_MPS = 299792458.0;
	static const float STEP_ROUNDING_REL = 1e-6;

	static RICON_L2Result Evaluate(
		RICON_TerrainProfile profile,
		float transmitterHeightAglM,
		float receiverHeightAglM,
		float frequencyMHz,
		RICON_TerrainDiffractionConfig config)
	{
		RICON_L2Result result = new RICON_L2Result();
		if (!config || !config.IsValid())
		{
			result.m_eCode = RICON_EL2ResultCode.INVALID_CONFIG;
			return result;
		}
		if (!RICON_Num.IsFinite(frequencyMHz) || frequencyMHz < 30.0 || frequencyMHz > 100000.0)
		{
			result.m_eCode = RICON_EL2ResultCode.OUT_OF_DOMAIN_FREQUENCY;
			return result;
		}

		RICON_EL2ResultCode profileCode = ValidateProfile(profile, config);
		if (profileCode != RICON_EL2ResultCode.OK)
		{
			result.m_eCode = profileCode;
			return result;
		}
		if (!IsInRange(transmitterHeightAglM, 0.01, 10000.0)
			|| !IsInRange(receiverHeightAglM, 0.01, 10000.0))
		{
			result.m_eCode = RICON_EL2ResultCode.INVALID_ANTENNA_HEIGHT;
			return result;
		}

		float wavelengthM = SPEED_OF_LIGHT_MPS / (frequencyMHz * 1000000.0);
		float effectiveEarthRadiusKm = config.m_fEarthRadiusKm * config.m_fKFactor;
		float curvature = 1.0 / effectiveEarthRadiusKm;
		int last = profile.Count() - 1;
		float transmitterMaslM = profile.m_aHeightM[0] + transmitterHeightAglM;
		float receiverMaslM = profile.m_aHeightM[last] + receiverHeightAglM;
		if (!AllFinite5(wavelengthM, effectiveEarthRadiusKm, curvature, transmitterMaslM, receiverMaslM))
			return result;

		RICON_L2BullingtonResult actual = Bullington(profile.m_aDistanceKm, profile.m_aHeightM,
			transmitterMaslM, receiverMaslM, curvature, wavelengthM);
		if (!actual)
			return result;

		float smoothGroundTx;
		float smoothGroundRx;
		if (!TrySmoothHeights(profile, transmitterMaslM, receiverMaslM, smoothGroundTx, smoothGroundRx))
			return result;
		float smoothTxM = transmitterMaslM - smoothGroundTx;
		float smoothRxM = receiverMaslM - smoothGroundRx;
		array<float> zeroHeights = {};
		for (int i = 0; i < profile.Count(); i++)
			zeroHeights.Insert(0);
		RICON_L2BullingtonResult smooth = Bullington(profile.m_aDistanceKm, zeroHeights,
			smoothTxM, smoothRxM, curvature, wavelengthM);
		if (!smooth)
			return result;

		RICON_L2SphericalResult spherical = Spherical(profile.m_aDistanceKm[last], smoothTxM, smoothRxM,
			wavelengthM, effectiveEarthRadiusKm, frequencyMHz, config);
		if (!spherical)
			return result;

		float lossDb = actual.m_fLossDb + Math.Max(spherical.m_fLossDb - smooth.m_fLossDb, 0);
		if (!AllFinite4(lossDb, actual.m_fLossDb, smooth.m_fLossDb, spherical.m_fLossDb) || lossDb < 0)
			return result;

		result.m_eCode = RICON_EL2ResultCode.OK;
		result.m_fLossDb = lossDb;
		result.m_fBullingtonActualDb = actual.m_fLossDb;
		result.m_fBullingtonSmoothDb = smooth.m_fLossDb;
		result.m_fSphericalDb = spherical.m_fLossDb;
		result.m_eBranchActual = actual.m_eBranch;
		result.m_eBranchSmooth = smooth.m_eBranch;
		result.m_eSphericalCase = spherical.m_eCase;
		result.m_eFirstTermAccuracy = spherical.m_eAccuracy;
		return result;
	}

	protected static RICON_EL2ResultCode ValidateProfile(
		RICON_TerrainProfile profile,
		RICON_TerrainDiffractionConfig config)
	{
		if (!profile || !profile.m_aDistanceKm || !profile.m_aHeightM)
			return RICON_EL2ResultCode.INVALID_PROFILE;
		int count = profile.m_aDistanceKm.Count();
		if (count < 3 || count > config.m_iMaxProfilePoints || profile.m_aHeightM.Count() != count)
			return RICON_EL2ResultCode.INVALID_PROFILE;
		if (profile.m_aDistanceKm[0] != 0)
			return RICON_EL2ResultCode.INVALID_PROFILE;
		float totalKm = profile.m_aDistanceKm[count - 1];
		if (!RICON_Num.IsFinite(totalKm) || totalKm <= 0 || totalKm > 500.0)
			return RICON_EL2ResultCode.INVALID_PROFILE;
		float minimumStepKm = 0.001 - STEP_ROUNDING_REL * (0.001 + totalKm);
		for (int i = 0; i < count; i++)
		{
			float distanceKm = profile.m_aDistanceKm[i];
			float heightM = profile.m_aHeightM[i];
			if (!RICON_Num.IsFinite(distanceKm) || !IsInRange(heightM, -1000.0, 10000.0))
				return RICON_EL2ResultCode.INVALID_PROFILE;
			if (i == 0)
				continue;
			float stepKm = distanceKm - profile.m_aDistanceKm[i - 1];
			if (!RICON_Num.IsFinite(stepKm) || stepKm < minimumStepKm)
				return RICON_EL2ResultCode.INVALID_PROFILE;
		}
		float maximumStepKm = config.m_fMaxProfileStepKm
			+ STEP_ROUNDING_REL * (config.m_fMaxProfileStepKm + totalKm);
		for (int j = 1; j < count; j++)
		{
			if (profile.m_aDistanceKm[j] - profile.m_aDistanceKm[j - 1] > maximumStepKm)
				return RICON_EL2ResultCode.PROFILE_STEP_TOO_LARGE;
		}
		return RICON_EL2ResultCode.OK;
	}

	protected static RICON_L2BullingtonResult Bullington(
		notnull array<float> distancesKm,
		notnull array<float> heightsM,
		float transmitterMaslM,
		float receiverMaslM,
		float curvaturePerKm,
		float wavelengthM)
	{
		int last = distancesKm.Count() - 1;
		float distanceKm = distancesKm[last];
		float highestTxSlope = -float.MAX;
		for (int i = 1; i < last; i++)
		{
			float bulgedHeightM = BulgedHeight(heightsM[i], distancesKm[i], distanceKm, curvaturePerKm);
			float slope = (bulgedHeightM - transmitterMaslM) / distancesKm[i];
			if (slope > highestTxSlope)
				highestTxSlope = slope;
		}
		float directSlope = (receiverMaslM - transmitterMaslM) / distanceKm;
		float uncorrectedLossDb;
		RICON_EL2Branch branch;

		if (highestTxSlope < directSlope)
		{
			if (!TryLosLoss(distancesKm, heightsM, transmitterMaslM, receiverMaslM, curvaturePerKm,
				wavelengthM, uncorrectedLossDb))
				return null;
			branch = RICON_EL2Branch.LOS;
		}
		else
		{
			float highestRxSlope = -float.MAX;
			for (int j = 1; j < last; j++)
			{
				float bulgedHeightM = BulgedHeight(heightsM[j], distancesKm[j], distanceKm, curvaturePerKm);
				float slope = (bulgedHeightM - receiverMaslM) / (distanceKm - distancesKm[j]);
				if (slope > highestRxSlope)
					highestRxSlope = slope;
			}
			float denominator = highestTxSlope + highestRxSlope;
			if (denominator == 0)
			{
				if (!TryLosLoss(distancesKm, heightsM, transmitterMaslM, receiverMaslM, curvaturePerKm,
					wavelengthM, uncorrectedLossDb))
					return null;
				branch = RICON_EL2Branch.LOS;
			}
			else
			{
				float bullingtonDistanceKm = (receiverMaslM - transmitterMaslM
					+ highestRxSlope * distanceKm) / denominator;
				if (!RICON_Num.IsFinite(bullingtonDistanceKm) || bullingtonDistanceKm <= 0
					|| bullingtonDistanceKm >= distanceKm)
					return null;
				float excessM = transmitterMaslM + highestTxSlope * bullingtonDistanceKm
					- (transmitterMaslM * (distanceKm - bullingtonDistanceKm)
					+ receiverMaslM * bullingtonDistanceKm) / distanceKm;
				float scale = Math.Sqrt(0.002 * distanceKm
					/ (wavelengthM * bullingtonDistanceKm * (distanceKm - bullingtonDistanceKm)));
				float nu = excessM * scale;
				if (!AllFinite3(excessM, scale, nu))
					return null;
				uncorrectedLossDb = KnifeEdgeLoss(nu);
				branch = RICON_EL2Branch.TRANS;
			}
		}

		float correction = (1.0 - Math.Pow(Math.E, -uncorrectedLossDb / 6.0))
			* (10.0 + 0.02 * distanceKm);
		float lossDb = uncorrectedLossDb + correction;
		if (!AllFinite3(uncorrectedLossDb, correction, lossDb))
			return null;
		RICON_L2BullingtonResult result = new RICON_L2BullingtonResult();
		result.m_fLossDb = lossDb;
		result.m_eBranch = branch;
		return result;
	}

	protected static bool TryLosLoss(
		notnull array<float> distancesKm,
		notnull array<float> heightsM,
		float transmitterMaslM,
		float receiverMaslM,
		float curvaturePerKm,
		float wavelengthM,
		out float lossDb)
	{
		lossDb = 0;
		int last = distancesKm.Count() - 1;
		float distanceKm = distancesKm[last];
		float maxNu = -float.MAX;
		for (int i = 1; i < last; i++)
		{
			float positionKm = distancesKm[i];
			float bulgedHeightM = BulgedHeight(heightsM[i], positionKm, distanceKm, curvaturePerKm);
			float lineHeightM = (transmitterMaslM * (distanceKm - positionKm)
				+ receiverMaslM * positionKm) / distanceKm;
			float scale = Math.Sqrt(0.002 * distanceKm
				/ (wavelengthM * positionKm * (distanceKm - positionKm)));
			float nu = (bulgedHeightM - lineHeightM) * scale;
			if (!AllFinite4(bulgedHeightM, lineHeightM, scale, nu))
				return false;
			if (nu > maxNu)
				maxNu = nu;
		}
		lossDb = KnifeEdgeLoss(maxNu);
		return RICON_Num.IsFinite(lossDb);
	}

	protected static float KnifeEdgeLoss(float nu)
	{
		if (nu <= -0.78)
			return 0;
		return 6.9 + 20.0 * Math.Log10(Math.Sqrt((nu - 0.1) * (nu - 0.1) + 1.0) + nu - 0.1);
	}

	protected static float BulgedHeight(float heightM, float positionKm, float distanceKm, float curvaturePerKm)
	{
		return heightM + 500.0 * curvaturePerKm * positionKm * (distanceKm - positionKm);
	}

	protected static bool TrySmoothHeights(
		RICON_TerrainProfile profile,
		float transmitterMaslM,
		float receiverMaslM,
		out float smoothGroundTxM,
		out float smoothGroundRxM)
	{
		smoothGroundTxM = 0;
		smoothGroundRxM = 0;
		int count = profile.Count();
		int last = count - 1;
		float distanceKm = profile.m_aDistanceKm[last];
		float v1 = 0;
		float v2 = 0;
		for (int i = 1; i < count; i++)
		{
			float previousDistanceKm = profile.m_aDistanceKm[i - 1];
			float currentDistanceKm = profile.m_aDistanceKm[i];
			float previousHeightM = profile.m_aHeightM[i - 1];
			float currentHeightM = profile.m_aHeightM[i];
			float stepKm = currentDistanceKm - previousDistanceKm;
			v1 += stepKm * (currentHeightM + previousHeightM);
			v2 += stepKm * (currentHeightM * (2.0 * currentDistanceKm + previousDistanceKm)
				+ previousHeightM * (currentDistanceKm + 2.0 * previousDistanceKm));
		}
		float initialTxM = (2.0 * v1 * distanceKm - v2) / (distanceKm * distanceKm);
		float initialRxM = (v2 - v1 * distanceKm) / (distanceKm * distanceKm);
		if (!AllFinite4(v1, v2, initialTxM, initialRxM))
			return false;

		float highestObstacleM = -float.MAX;
		float highestTxAngle = -float.MAX;
		float highestRxAngle = -float.MAX;
		for (int j = 1; j < last; j++)
		{
			float positionKm = profile.m_aDistanceKm[j];
			float rayHeightM = (transmitterMaslM * (distanceKm - positionKm)
				+ receiverMaslM * positionKm) / distanceKm;
			float obstacleM = profile.m_aHeightM[j] - rayHeightM;
			if (obstacleM > highestObstacleM)
				highestObstacleM = obstacleM;
			float txAngle = obstacleM / positionKm;
			float rxAngle = obstacleM / (distanceKm - positionKm);
			if (txAngle > highestTxAngle)
				highestTxAngle = txAngle;
			if (rxAngle > highestRxAngle)
				highestRxAngle = rxAngle;
		}

		float provisionalTxM = initialTxM;
		float provisionalRxM = initialRxM;
		if (highestObstacleM > 0)
		{
			float angleSum = highestTxAngle + highestRxAngle;
			if (!RICON_Num.IsFinite(angleSum) || angleSum <= 0)
				return false;
			provisionalTxM = initialTxM - highestObstacleM * highestTxAngle / angleSum;
			provisionalRxM = initialRxM - highestObstacleM * highestRxAngle / angleSum;
		}
		smoothGroundTxM = Math.Min(provisionalTxM, profile.m_aHeightM[0]);
		smoothGroundRxM = Math.Min(provisionalRxM, profile.m_aHeightM[last]);
		return AllFinite2(smoothGroundTxM, smoothGroundRxM);
	}

	protected static RICON_L2SphericalResult Spherical(
		float distanceKm,
		float transmitterHeightM,
		float receiverHeightM,
		float wavelengthM,
		float effectiveEarthRadiusKm,
		float frequencyMHz,
		RICON_TerrainDiffractionConfig config)
	{
		float distanceM = distanceKm * 1000.0;
		float earthRadiusM = effectiveEarthRadiusKm * 1000.0;
		float lineOfSightM = Math.Sqrt(2.0 * earthRadiusM)
			* (Math.Sqrt(transmitterHeightM) + Math.Sqrt(receiverHeightM));
		if (!AllFinite3(distanceM, earthRadiusM, lineOfSightM))
			return null;

		RICON_L2SphericalResult result = new RICON_L2SphericalResult();
		if (distanceM >= lineOfSightM)
		{
			RICON_L2FieldResult field = FieldRelative(distanceKm, transmitterHeightM, receiverHeightM,
				effectiveEarthRadiusKm, frequencyMHz, config);
			if (!field)
				return null;
			result.m_fLossDb = -field.m_fRelativeDb;
			result.m_eCase = RICON_EL2SphericalCase.BEYOND;
			result.m_eAccuracy = field.m_eAccuracy;
			return result;
		}

		float m = distanceM * distanceM
			/ (4.0 * earthRadiusM * (transmitterHeightM + receiverHeightM));
		float c = (transmitterHeightM - receiverHeightM) / (transmitterHeightM + receiverHeightM);
		float acosArgument = 1.5 * c * Math.Sqrt(3.0 * m / Math.Pow(m + 1.0, 3.0));
		acosArgument = Math.Clamp(acosArgument, -1.0, 1.0);
		float b = 2.0 * Math.Sqrt((m + 1.0) / (3.0 * m))
			* Math.Cos(Math.PI / 3.0 + Math.Acos(acosArgument) / 3.0);
		float d1M = distanceM * 0.5 * (1.0 + b);
		float d2M = distanceM - d1M;
		float clearanceM = ((transmitterHeightM - d1M * d1M / (2.0 * earthRadiusM)) * d2M
			+ (receiverHeightM - d2M * d2M / (2.0 * earthRadiusM)) * d1M) / distanceM;
		float requiredM = 0.552 * Math.Sqrt(d1M * d2M * wavelengthM / distanceM);
		if (!AllFinite6(m, c, acosArgument, b, d1M, d2M) || !AllFinite2(clearanceM, requiredM))
			return null;
		if (clearanceM > requiredM)
		{
			result.m_fLossDb = 0;
			result.m_eCase = RICON_EL2SphericalCase.CLEAR;
			result.m_eAccuracy = RICON_EFirstTermAccuracy.NOT_EVALUATED;
			return result;
		}

		float modifiedEarthRadiusM = 0.5 * Math.Pow(distanceM
			/ (Math.Sqrt(transmitterHeightM) + Math.Sqrt(receiverHeightM)), 2.0);
		RICON_L2FieldResult modifiedField = FieldRelative(distanceKm, transmitterHeightM, receiverHeightM,
			modifiedEarthRadiusM / 1000.0, frequencyMHz, config);
		if (!modifiedField)
			return null;
		float ahDb = -modifiedField.m_fRelativeDb;
		if (ahDb < 0)
		{
			result.m_fLossDb = 0;
			result.m_eCase = RICON_EL2SphericalCase.AH_NEGATIVE;
			result.m_eAccuracy = modifiedField.m_eAccuracy;
			return result;
		}
		result.m_fLossDb = (1.0 - clearanceM / requiredM) * ahDb;
		result.m_eCase = RICON_EL2SphericalCase.INTERPOLATED;
		result.m_eAccuracy = modifiedField.m_eAccuracy;
		if (!RICON_Num.IsFinite(result.m_fLossDb))
			return null;
		return result;
	}

	protected static RICON_L2FieldResult FieldRelative(
		float distanceKm,
		float transmitterHeightM,
		float receiverHeightM,
		float earthRadiusKm,
		float frequencyMHz,
		RICON_TerrainDiffractionConfig config)
	{
		float k = SurfaceAdmittance(earthRadiusKm, frequencyMHz, config);
		if (!RICON_Num.IsFinite(k) || k <= 0)
			return null;
		float k2 = k * k;
		float k4 = k2 * k2;
		float beta = (1.0 + 1.6 * k2 + 0.67 * k4) / (1.0 + 4.5 * k2 + 1.53 * k4);
		float x = 2.188 * beta * Math.Pow(frequencyMHz, 1.0 / 3.0)
			* Math.Pow(earthRadiusKm, -2.0 / 3.0) * distanceKm;
		if (!AllFinite4(k2, k4, beta, x) || x <= 0)
			return null;
		float distanceTerm;
		if (x >= 1.6)
			distanceTerm = 11.0 + 10.0 * Math.Log10(x) - 17.6 * x;
		else
			distanceTerm = -20.0 * Math.Log10(x) - 5.6488 * Math.Pow(x, 1.425);
		float floor = 2.0 + 20.0 * Math.Log10(k);
		float txGain;
		float rxGain;
		float txDelta;
		float rxDelta;
		if (!TryHeightGain(transmitterHeightM, earthRadiusKm, frequencyMHz, beta, floor, txGain, txDelta)
			|| !TryHeightGain(receiverHeightM, earthRadiusKm, frequencyMHz, beta, floor, rxGain, rxDelta))
			return null;
		float xLimit = 1.096 - 1.280 * (1.0 - beta);
		float relativeDb = distanceTerm + txGain + rxGain;
		if (!AllFinite4(distanceTerm, floor, txGain, rxGain) || !AllFinite4(txDelta, rxDelta, xLimit, relativeDb))
			return null;
		RICON_L2FieldResult result = new RICON_L2FieldResult();
		result.m_fRelativeDb = relativeDb;
		result.m_eAccuracy = RICON_EFirstTermAccuracy.NOT_MET;
		if (x - txDelta - rxDelta > xLimit)
			result.m_eAccuracy = RICON_EFirstTermAccuracy.MET;
		return result;
	}

	protected static float SurfaceAdmittance(
		float earthRadiusKm,
		float frequencyMHz,
		RICON_TerrainDiffractionConfig config)
	{
		float conductiveTerm = 18000.0 * config.m_fConductivitySm / frequencyMHz;
		float horizontal = 0.36 * Math.Pow(earthRadiusKm * frequencyMHz, -1.0 / 3.0)
			* Math.Pow((config.m_fPermittivity - 1.0) * (config.m_fPermittivity - 1.0)
			+ conductiveTerm * conductiveTerm, -0.25);
		if (config.m_ePolarization == RICON_ETerrainPolarization.HORIZONTAL)
			return horizontal;
		return horizontal * Math.Sqrt(config.m_fPermittivity * config.m_fPermittivity
			+ conductiveTerm * conductiveTerm);
	}

	protected static bool TryHeightGain(
		float heightM,
		float earthRadiusKm,
		float frequencyMHz,
		float beta,
		float floor,
		out float gain,
		out float weightedDelta)
	{
		gain = 0;
		weightedDelta = 0;
		float y = 9.575e-3 * beta * Math.Pow(frequencyMHz, 2.0 / 3.0)
			* Math.Pow(earthRadiusKm, -1.0 / 3.0) * heightM;
		float b = beta * y;
		if (!AllFinite2(y, b) || b <= 0)
			return false;
		if (b > 2.0)
			gain = 17.6 * Math.Sqrt(b - 1.1) - 5.0 * Math.Log10(b - 1.1) - 8.0;
		else
			gain = 20.0 * Math.Log10(b + 0.1 * b * b * b);
		gain = Math.Max(gain, floor);

		float d0 = 0.5 * (1.0 + StableTanh((0.5 * Math.Log10(b) - 0.255) / 0.3));
		float dInf = 0.5 * (1.0 + StableTanh((0.5 * Math.Log10(b) + 0.255) / 0.25));
		float delta = d0 + 1.779 * (1.0 - beta) * (dInf - d0);
		weightedDelta = Math.Sqrt(b) * delta;
		return AllFinite5(gain, d0, dInf, delta, weightedDelta);
	}

	protected static float StableTanh(float value)
	{
		if (value >= 10.0)
			return 1.0;
		if (value <= -10.0)
			return -1.0;
		float exp2x = Math.Pow(Math.E, 2.0 * value);
		return (exp2x - 1.0) / (exp2x + 1.0);
	}

	protected static bool IsInRange(float value, float minimum, float maximum)
	{
		return RICON_Num.IsFinite(value) && value >= minimum && value <= maximum;
	}

	protected static bool AllFinite2(float a, float b)
	{
		return RICON_Num.IsFinite(a) && RICON_Num.IsFinite(b);
	}

	protected static bool AllFinite3(float a, float b, float c)
	{
		return RICON_Num.IsFinite(a) && RICON_Num.IsFinite(b) && RICON_Num.IsFinite(c);
	}

	protected static bool AllFinite4(float a, float b, float c, float d)
	{
		return RICON_Num.IsFinite(a) && RICON_Num.IsFinite(b)
			&& RICON_Num.IsFinite(c) && RICON_Num.IsFinite(d);
	}

	protected static bool AllFinite5(float a, float b, float c, float d, float e)
	{
		return RICON_Num.IsFinite(a) && RICON_Num.IsFinite(b) && RICON_Num.IsFinite(c)
			&& RICON_Num.IsFinite(d) && RICON_Num.IsFinite(e);
	}

	protected static bool AllFinite6(float a, float b, float c, float d, float e, float f)
	{
		return RICON_Num.IsFinite(a) && RICON_Num.IsFinite(b) && RICON_Num.IsFinite(c)
			&& RICON_Num.IsFinite(d) && RICON_Num.IsFinite(e) && RICON_Num.IsFinite(f);
	}
}

class RICON_TerrainDiffractionModel : RICON_PropagationModel
{
	static const string MODEL_ID = "terrain_diffraction_p526_16";
	static const string FSPL_LAYER_ID = "fspl";
	static const string TERRAIN_LAYER_ID = "terrain_diffraction";

	protected ref RICON_TerrainDiffractionConfig m_Config;
	protected ref RICON_TerrainProfileProvider m_ProfileProvider;

	void RICON_TerrainDiffractionModel(
		RICON_TerrainDiffractionConfig config,
		RICON_TerrainProfileProvider profileProvider)
	{
		if (config)
			m_Config = config.CloneConfig();
		m_ProfileProvider = profileProvider;
	}

	override string GetModelId()
	{
		return MODEL_ID;
	}

	override bool IsConfigValid()
	{
		return m_Config && m_Config.IsValid() && m_ProfileProvider != null;
	}

	override bool IsApplicable(float frequencyMHz)
	{
		return RICON_Num.IsFinite(frequencyMHz) && frequencyMHz >= 30.0 && frequencyMHz <= 100000.0;
	}

	override RICON_EReason AppendLosses(
		RICON_LinkQuery query,
		float distanceM,
		notnull array<ref RICON_LossLayer> layers)
	{
		if (!query || !IsConfigValid())
			return RICON_EReason.CATALOG_VIOLATION;
		if (!IsApplicable(query.m_fFrequencyMHz))
			return RICON_EReason.OUT_OF_MODEL_DOMAIN;

		float dx = query.m_vRxPos[0] - query.m_vTxPos[0];
		float dz = query.m_vRxPos[2] - query.m_vTxPos[2];
		float horizontalDistanceM = Math.Sqrt(dx * dx + dz * dz);
		float fsplDb = RICON_FreeSpaceModel.LossDb(distanceM, query.m_fFrequencyMHz);
		if (!RICON_Num.IsFinite(horizontalDistanceM) || !RICON_Num.IsFinite(fsplDb))
			return RICON_EReason.NUMERIC_FAILURE;
		if (horizontalDistanceM < 2.0)
		{
			layers.Insert(new RICON_LossLayer(FSPL_LAYER_ID, fsplDb, MODEL_ID));
			layers.Insert(new RICON_LossLayer(TERRAIN_LAYER_ID, 0, MODEL_ID));
			return RICON_EReason.OK;
		}

		RICON_TerrainProfile profile;
		float transmitterHeightAglM;
		float receiverHeightAglM;
		RICON_EReason providerReason = m_ProfileProvider.GetProfile(query, m_Config,
			profile, transmitterHeightAglM, receiverHeightAglM);
		if (providerReason != RICON_EReason.OK)
			return providerReason;

		RICON_L2Result result = RICON_TerrainDiffractionCalculator.Evaluate(profile,
			transmitterHeightAglM, receiverHeightAglM, query.m_fFrequencyMHz, m_Config);
		RICON_EReason mappedReason = MapResult(result);
		if (mappedReason != RICON_EReason.OK)
			return mappedReason;

		layers.Insert(new RICON_LossLayer(FSPL_LAYER_ID, fsplDb, MODEL_ID));
		layers.Insert(new RICON_LossLayer(TERRAIN_LAYER_ID, result.m_fLossDb, MODEL_ID));
		return RICON_EReason.OK;
	}

	protected static RICON_EReason MapResult(RICON_L2Result result)
	{
		if (!result)
			return RICON_EReason.NUMERIC_FAILURE;
		switch (result.m_eCode)
		{
			case RICON_EL2ResultCode.INVALID_CONFIG:
				return RICON_EReason.CATALOG_VIOLATION;
			case RICON_EL2ResultCode.OUT_OF_DOMAIN_FREQUENCY:
				return RICON_EReason.OUT_OF_MODEL_DOMAIN;
			case RICON_EL2ResultCode.INVALID_PROFILE:
			case RICON_EL2ResultCode.PROFILE_STEP_TOO_LARGE:
			case RICON_EL2ResultCode.INVALID_ANTENNA_HEIGHT:
				return RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
			case RICON_EL2ResultCode.OK:
				return RICON_EReason.OK;
		}
		return RICON_EReason.NUMERIC_FAILURE;
	}
}
