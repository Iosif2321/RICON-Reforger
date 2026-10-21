//! Источник сырых высот для REQ-L2-world-profile. Базовый источник отказывает.
class RICON_HeightSource
{
	bool TryGetHeight(float x, float z, out float heightM)
	{
		heightM = 0;
		return false;
	}
}

//! Тонкая оболочка API; вызов с настоящим миром пока не проверен.
class RICON_BaseWorldHeightSource : RICON_HeightSource
{
	protected BaseWorld m_World;

	void RICON_BaseWorldHeightSource(BaseWorld world)
	{
		m_World = world;
	}

	override bool TryGetHeight(float x, float z, out float heightM)
	{
		heightM = 0;
		if (!m_World || !RICON_Num.IsFinite(x) || !RICON_Num.IsFinite(z))
			return false;
		float sampledHeight = m_World.GetSurfaceY(x, z);
		if (!RICON_Num.IsFinite(sampledHeight))
			return false;
		heightM = sampledHeight;
		return true;
	}
}

//! Геометрия профиля; источник можно исполнять отдельно от игрового мира.
//! shortcut: используется сырая высота источника без модели воды; морской режим требует отдельного контракта.
class RICON_WorldProfileProvider : RICON_TerrainProfileProvider
{
	protected ref RICON_HeightSource m_Source;

	void RICON_WorldProfileProvider(RICON_HeightSource source)
	{
		m_Source = source;
	}

	override RICON_EReason GetProfile(
		RICON_LinkQuery query,
		RICON_TerrainDiffractionConfig config,
		out RICON_TerrainProfile profile,
		out float transmitterHeightAglM,
		out float receiverHeightAglM)
	{
		profile = null;
		transmitterHeightAglM = 0;
		receiverHeightAglM = 0;
		if (!query || !config || !m_Source || !config.IsValid()
			|| !RICON_Num.IsFiniteVector(query.m_vTxPos) || !RICON_Num.IsFiniteVector(query.m_vRxPos))
			return RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;

		vector tx = query.m_vTxPos;
		vector rx = query.m_vRxPos;
		float dx = rx[0] - tx[0];
		float dz = rx[2] - tx[2];
		float horizontalDistanceM = Math.Sqrt(dx * dx + dz * dz);
		RICON_TerrainSamplingResult sampling = RICON_TerrainProfileSampling.BuildDistances(horizontalDistanceM, config);
		if (sampling.m_eCode != RICON_ETerrainSamplingCode.OK)
			return RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;

		RICON_TerrainProfile candidate = new RICON_TerrainProfile();
		int count = sampling.m_aDistanceKm.Count();
		for (int i = 0; i < count; i++)
		{
			float ratio = i / (count - 1.0);
			float x = tx[0] + dx * ratio;
			float z = tx[2] + dz * ratio;
			// Концы присваиваются явно: float32-вычитание может потерять младшие разряды RX.
			if (i == 0)
			{
				x = tx[0];
				z = tx[2];
			}
			else if (i == count - 1)
			{
				x = rx[0];
				z = rx[2];
			}
			float heightM;
			if (!RICON_Num.IsFinite(x) || !RICON_Num.IsFinite(z)
				|| !m_Source.TryGetHeight(x, z, heightM) || !RICON_Num.IsFinite(heightM))
				return RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
			candidate.AddPoint(sampling.m_aDistanceKm[i], heightM);
		}

		float txAgl = tx[1] - candidate.m_aHeightM[0];
		float rxAgl = rx[1] - candidate.m_aHeightM[count - 1];
		if (!RICON_Num.IsFinite(txAgl) || !RICON_Num.IsFinite(rxAgl))
			return RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
		// Домены конечных высот проверяет L2; частичный профиль наружу не выдаётся.
		profile = candidate;
		transmitterHeightAglM = txAgl;
		receiverHeightAglM = rxAgl;
		return RICON_EReason.OK;
	}
}
