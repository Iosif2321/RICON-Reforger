//! V1 route body for trusted control. The RPC carries the control sequence and profile key separately.
class RICON_MediaWireControlV1
{
	static const int ROUTE_FIELD_COUNT = 26;
	protected static const int FIRST_SEQUENCE = 24;
	protected static const int FIRST_SAMPLE_INDEX = 25;

	static array<int> EncodeRoute(RICON_MediaEgressRoute route, int firstSequence, int firstSampleIndex)
	{
		if (!route || !route.IsValid() || !ValidCutover(firstSequence, firstSampleIndex)) return null;
		RICON_MediaStreamBinding source = route.m_Source;
		array<int> fields = {};
		fields.Reserve(ROUTE_FIELD_COUNT);
		fields.Insert(source.m_iSessionEpoch);
		fields.Insert(source.m_iPlayerId);
		fields.Insert(source.m_iPlayerGeneration);
		fields.Insert(source.m_iConnectionId);
		fields.Insert(source.m_iEntityId);
		fields.Insert(source.m_iLeaseId);
		fields.Insert(source.m_iDeviceId);
		fields.Insert(source.m_iNetworkTypeId);
		fields.Insert(source.m_iChannelId);
		fields.Insert(source.m_iTxTransceiverIndex);
		fields.Insert(source.m_iStreamId);
		fields.Insert(source.m_iStreamGeneration);
		fields.Insert(source.m_iCaptureEpoch);
		fields.Insert(source.m_iFormatEpoch);
		fields.Insert(route.m_iListenerPlayerId);
		fields.Insert(route.m_iListenerConnectionId);
		fields.Insert(route.m_iListenerGeneration);
		fields.Insert(route.m_iListenerEntityId);
		fields.Insert(route.m_iRxDeviceId);
		fields.Insert(route.m_iRxTransceiverIndex);
		fields.Insert(route.m_iRxGeneration);
		fields.Insert(route.m_iRouteId);
		fields.Insert(route.m_iRouteGeneration);
		fields.Insert(route.m_iPolicyRevision);
		fields.Insert(firstSequence);
		fields.Insert(firstSampleIndex);
		return fields;
	}

	static RICON_MediaEgressRoute DecodeRoute(array<int> fields, string profileId, out int firstSequence, out int firstSampleIndex)
	{
		firstSequence = 0;
		firstSampleIndex = 0;
		if (!fields || fields.Count() != ROUTE_FIELD_COUNT || !RICON_TemporalSnapshots.ValidKey(profileId)) return null;
		if (!ValidCutover(fields[FIRST_SEQUENCE], fields[FIRST_SAMPLE_INDEX])) return null;

		RICON_MediaEgressRoute route = new RICON_MediaEgressRoute();
		route.m_Source = new RICON_MediaStreamBinding();
		RICON_MediaStreamBinding source = route.m_Source;
		source.m_iSessionEpoch = fields[0];
		source.m_iPlayerId = fields[1];
		source.m_iPlayerGeneration = fields[2];
		source.m_iConnectionId = fields[3];
		source.m_iEntityId = fields[4];
		source.m_iLeaseId = fields[5];
		source.m_iDeviceId = fields[6];
		source.m_iNetworkTypeId = fields[7];
		source.m_iChannelId = fields[8];
		source.m_iTxTransceiverIndex = fields[9];
		source.m_iStreamId = fields[10];
		source.m_iStreamGeneration = fields[11];
		source.m_iCaptureEpoch = fields[12];
		source.m_iFormatEpoch = fields[13];
		source.m_sProfileId = profileId;
		route.m_iListenerPlayerId = fields[14];
		route.m_iListenerConnectionId = fields[15];
		route.m_iListenerGeneration = fields[16];
		route.m_iListenerEntityId = fields[17];
		route.m_iRxDeviceId = fields[18];
		route.m_iRxTransceiverIndex = fields[19];
		route.m_iRxGeneration = fields[20];
		route.m_iRouteId = fields[21];
		route.m_iRouteGeneration = fields[22];
		route.m_iPolicyRevision = fields[23];
		if (!route.IsValid()) return null;

		firstSequence = fields[FIRST_SEQUENCE];
		firstSampleIndex = fields[FIRST_SAMPLE_INDEX];
		return route;
	}

	protected static bool ValidCutover(int firstSequence, int firstSampleIndex)
	{
		return firstSequence >= 0 && firstSequence < int.MAX && firstSampleIndex >= 0 && firstSampleIndex < int.MAX;
	}
}
