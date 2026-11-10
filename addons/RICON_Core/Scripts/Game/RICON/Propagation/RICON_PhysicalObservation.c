//! Observation qualification only. No descriptor, RF defaults, loss model, actuator or autorun.
//! Native/compile/runtime status is recorded separately; this source does not certify physical completeness.
class RICON_PhysicalObservationLimits
{
    int m_iMaxHeightSamples;
    int m_iMaxTraces;
    int m_iMaxSpanMs;

    bool IsValid()
    {
        return m_iMaxHeightSamples >= 3 && m_iMaxHeightSamples <= 100000
            && m_iMaxTraces >= 0 && m_iMaxTraces <= 1
            && m_iMaxSpanMs >= 0 && m_iMaxSpanMs <= 60000;
    }
}

class RICON_PhysicalEndpointRequest
{
    int m_iDeviceId;
    int m_iTransceiverIndex = -1;
    vector m_vAntennaPos;
    string m_sAntennaSourceId;
}

class RICON_PhysicalObservationRequest
{
    ref RICON_PhysicalEndpointRequest m_Tx;
    ref RICON_PhysicalEndpointRequest m_Rx;
    string m_sExpectedSourceId;
    int m_iExpectedSourceRevision;
    string m_sTimebaseId;
    bool m_bSampleTerrain;
    ref RICON_TerrainDiffractionConfig m_TerrainConfig;
    bool m_bSampleFirstContact;
    int m_iTraceFlags;
    int m_iTraceLayerMask;
}

class RICON_PhysicalObservationStamp
{
    string m_sSourceId;
    int m_iSourceRevision;
    string m_sTimebaseId;
    string m_sWorldIdentity;
    string m_sProvenance;
    float m_fRawWorldTimeMs;
}

class RICON_PhysicalEndpointWitness
{
    ref RICON_RadioObservation m_Observation;
    string m_sIdentityToken;
    string m_sAntennaSourceId;
    vector m_vOwnerOrigin;
    vector m_vOwnerAxis0;
    vector m_vOwnerAxis1;
    vector m_vOwnerAxis2;
    bool m_bHasBodyVelocity;
    vector m_vBodyVelocityAtPoint;
    string m_sBodyIdentityToken;
    string m_sMotionProvenance;
}

class RICON_PhysicalTraceWitness
{
    bool m_bHasContact;
    float m_fFraction;
    string m_sEntityIdentity;
    int m_iColliderIndex = -1;
    int m_iNodeIndex = -1;
    vector m_vNormal;
    string m_sMaterial;
}

//! Injected, read-only trusted source seam. These return values are not wire authority receipts.
//! ReadStamp must return a distinct stamp for each call. MeasureSpan owns its timestamp domain.
class RICON_PhysicalObservationSource
{
    RICON_PhysicalObservationStamp ReadStamp()
    {
        return null;
    }

    RICON_PhysicalEndpointWitness ReadEndpoint(RICON_PhysicalEndpointRequest endpoint)
    {
        return null;
    }

    RICON_HeightSource GetHeightSource()
    {
        return null;
    }

    RICON_PhysicalTraceWitness ReadFirstContact(vector tx, vector rx, int flags, int layerMask)
    {
        return null;
    }

    string MeasureSpan(RICON_PhysicalObservationStamp start, RICON_PhysicalObservationStamp end,
        int maxSpanMs, out float elapsedMs)
    {
        elapsedMs = 0;
        return "TIME_UNQUALIFIED";
    }
}

class RICON_PhysicalObservationFrame
{
    ref RICON_PhysicalObservationStamp m_Start;
    ref RICON_PhysicalObservationStamp m_End;
    ref RICON_PhysicalEndpointWitness m_Tx;
    ref RICON_PhysicalEndpointWitness m_Rx;
    ref RICON_TerrainProfile m_Terrain;
    float m_fTxHeightAglM;
    float m_fRxHeightAglM;
    ref RICON_PhysicalTraceWitness m_FirstContact;
    float m_fElapsedMs;
    ref array<int> m_aCoverage = {};
    ref array<string> m_aMissingReasons = {};
    string m_sClockQualification;
    bool m_bAtomicSnapshot;
    bool m_bCompletePhysicalPaths;
}

class RICON_PhysicalObservationResult
{
    string m_sCode = "INVALID_REQUEST";
    ref RICON_PhysicalObservationFrame m_Frame;
    int m_iStampReads;
    int m_iEndpointReads;
    int m_iHeightCalls;
    int m_iTraceCalls;
}

//! Hard per-sample cap, also counts failed source calls. It never substitutes a height on failure.
class RICON_PhysicalBudgetHeightSource : RICON_HeightSource
{
    protected ref RICON_HeightSource m_Source;
    protected int m_iLimit;
    int m_iCalls;

    void RICON_PhysicalBudgetHeightSource(RICON_HeightSource source, int limit)
    {
        m_Source = source;
        m_iLimit = limit;
    }

    override bool TryGetHeight(float x, float z, out float heightM)
    {
        heightM = 0;
        if (!m_Source || m_iCalls >= m_iLimit)
        {
            return false;
        }
        m_iCalls++;
        return m_Source.TryGetHeight(x, z, heightM);
    }
}

class RICON_PhysicalObservationSampler
{
    static const int MAX_TEXT = 2048;

    protected static bool ValidEndpointRequest(RICON_PhysicalEndpointRequest endpoint)
    {
        return endpoint && endpoint.m_iDeviceId >= 1 && endpoint.m_iDeviceId < int.MAX
            && endpoint.m_iTransceiverIndex >= 0 && endpoint.m_iTransceiverIndex < int.MAX
            && RICON_Num.IsFiniteVector(endpoint.m_vAntennaPos)
            && RICON_PathObservationModel.ValidKey(endpoint.m_sAntennaSourceId);
    }

    protected static bool ValidRequest(RICON_PhysicalObservationRequest request)
    {
        if (!request || !ValidEndpointRequest(request.m_Tx) || !ValidEndpointRequest(request.m_Rx))
        {
            return false;
        }
        if (request.m_Tx.m_iDeviceId == request.m_Rx.m_iDeviceId
            && request.m_Tx.m_iTransceiverIndex == request.m_Rx.m_iTransceiverIndex)
        {
            return false;
        }
        if (request.m_Tx.m_vAntennaPos == request.m_Rx.m_vAntennaPos)
        {
            return false;
        }
        if (!RICON_Num.IsFinite(vector.Distance(request.m_Tx.m_vAntennaPos, request.m_Rx.m_vAntennaPos)))
        {
            return false;
        }
        if (!RICON_PathObservationModel.ValidKey(request.m_sExpectedSourceId)
            || !RICON_PathObservationModel.ValidKey(request.m_sTimebaseId) || request.m_iExpectedSourceRevision <= 0)
        {
            return false;
        }
        if (request.m_bSampleTerrain && (!request.m_TerrainConfig || !request.m_TerrainConfig.IsValid()))
        {
            return false;
        }
        if (request.m_bSampleFirstContact)
        {
            int allowed = TraceFlags.ENTS | TraceFlags.WORLD | TraceFlags.OCEAN;
            if (request.m_iTraceFlags == 0 || (request.m_iTraceFlags & ~allowed) != 0 || request.m_iTraceLayerMask == 0)
            {
                return false;
            }
        }
        return true;
    }

    protected static RICON_PhysicalEndpointRequest CopyEndpointRequest(RICON_PhysicalEndpointRequest endpoint)
    {
        RICON_PhysicalEndpointRequest copy = new RICON_PhysicalEndpointRequest();
        copy.m_iDeviceId = endpoint.m_iDeviceId;
        copy.m_iTransceiverIndex = endpoint.m_iTransceiverIndex;
        copy.m_vAntennaPos = endpoint.m_vAntennaPos;
        copy.m_sAntennaSourceId = endpoint.m_sAntennaSourceId;
        return copy;
    }

    protected static RICON_PhysicalObservationRequest CopyRequest(RICON_PhysicalObservationRequest request)
    {
        RICON_PhysicalObservationRequest copy = new RICON_PhysicalObservationRequest();
        copy.m_Tx = CopyEndpointRequest(request.m_Tx);
        copy.m_Rx = CopyEndpointRequest(request.m_Rx);
        copy.m_sExpectedSourceId = request.m_sExpectedSourceId;
        copy.m_iExpectedSourceRevision = request.m_iExpectedSourceRevision;
        copy.m_sTimebaseId = request.m_sTimebaseId;
        copy.m_bSampleTerrain = request.m_bSampleTerrain;
        if (copy.m_bSampleTerrain)
        {
            copy.m_TerrainConfig = request.m_TerrainConfig.CloneConfig();
        }
        copy.m_bSampleFirstContact = request.m_bSampleFirstContact;
        copy.m_iTraceFlags = request.m_iTraceFlags;
        copy.m_iTraceLayerMask = request.m_iTraceLayerMask;
        return copy;
    }

    static RICON_PhysicalObservationStamp CopyStampHeader(RICON_PhysicalObservationStamp stamp)
    {
        RICON_PhysicalObservationStamp copy = new RICON_PhysicalObservationStamp();
        copy.m_sSourceId = stamp.m_sSourceId;
        copy.m_iSourceRevision = stamp.m_iSourceRevision;
        copy.m_sTimebaseId = stamp.m_sTimebaseId;
        copy.m_sWorldIdentity = stamp.m_sWorldIdentity;
        copy.m_sProvenance = stamp.m_sProvenance;
        copy.m_fRawWorldTimeMs = stamp.m_fRawWorldTimeMs;
        return copy;
    }

    static RICON_RadioObservation CopyRadioObservation(RICON_RadioObservation observation)
    {
        RICON_RadioObservation copy = new RICON_RadioObservation();
        copy.m_iDeviceId = observation.m_iDeviceId;
        copy.m_iTransceiverIndex = observation.m_iTransceiverIndex;
        copy.m_bPowered = observation.m_bPowered;
        copy.m_iTuningValue = observation.m_iTuningValue;
        copy.m_sNetworkKey = observation.m_sNetworkKey;
        copy.m_vAntennaPos = observation.m_vAntennaPos;
        return copy;
    }

    static RICON_PhysicalEndpointWitness CopyEndpointWitness(RICON_PhysicalEndpointWitness endpoint)
    {
        RICON_PhysicalEndpointWitness copy = new RICON_PhysicalEndpointWitness();
        copy.m_Observation = CopyRadioObservation(endpoint.m_Observation);
        copy.m_sIdentityToken = endpoint.m_sIdentityToken;
        copy.m_sAntennaSourceId = endpoint.m_sAntennaSourceId;
        copy.m_vOwnerOrigin = endpoint.m_vOwnerOrigin;
        copy.m_vOwnerAxis0 = endpoint.m_vOwnerAxis0;
        copy.m_vOwnerAxis1 = endpoint.m_vOwnerAxis1;
        copy.m_vOwnerAxis2 = endpoint.m_vOwnerAxis2;
        copy.m_bHasBodyVelocity = endpoint.m_bHasBodyVelocity;
        copy.m_vBodyVelocityAtPoint = endpoint.m_vBodyVelocityAtPoint;
        copy.m_sBodyIdentityToken = endpoint.m_sBodyIdentityToken;
        copy.m_sMotionProvenance = endpoint.m_sMotionProvenance;
        return copy;
    }

    static RICON_PhysicalTraceWitness CopyTraceWitness(RICON_PhysicalTraceWitness trace)
    {
        RICON_PhysicalTraceWitness copy = new RICON_PhysicalTraceWitness();
        copy.m_bHasContact = trace.m_bHasContact;
        copy.m_fFraction = trace.m_fFraction;
        copy.m_sEntityIdentity = trace.m_sEntityIdentity;
        copy.m_iColliderIndex = trace.m_iColliderIndex;
        copy.m_iNodeIndex = trace.m_iNodeIndex;
        copy.m_vNormal = trace.m_vNormal;
        copy.m_sMaterial = trace.m_sMaterial;
        return copy;
    }

    protected static bool TextOverflow(RICON_PhysicalEndpointWitness endpoint)
    {
        if (!endpoint)
        {
            return false;
        }
        if (endpoint.m_sIdentityToken.Length() > MAX_TEXT || endpoint.m_sBodyIdentityToken.Length() > MAX_TEXT)
        {
            return true;
        }
        return endpoint.m_Observation && endpoint.m_Observation.m_sNetworkKey.Length() > MAX_TEXT;
    }

    protected static bool ValidStamp(RICON_PhysicalObservationStamp stamp)
    {
        return stamp && RICON_PathObservationModel.ValidKey(stamp.m_sSourceId) && stamp.m_iSourceRevision > 0
            && RICON_PathObservationModel.ValidKey(stamp.m_sTimebaseId)
            && !stamp.m_sWorldIdentity.IsEmpty() && stamp.m_sWorldIdentity.Length() <= MAX_TEXT
            && (stamp.m_sProvenance == "ENGINE_GETTER" || stamp.m_sProvenance == "TEST_ONLY")
            && RICON_Num.IsFinite(stamp.m_fRawWorldTimeMs) && stamp.m_fRawWorldTimeMs >= 0;
    }

    protected static bool MatchesStamp(RICON_PhysicalObservationRequest request, RICON_PhysicalObservationStamp stamp)
    {
        return request.m_sExpectedSourceId == stamp.m_sSourceId
            && request.m_iExpectedSourceRevision == stamp.m_iSourceRevision && request.m_sTimebaseId == stamp.m_sTimebaseId;
    }

    protected static bool SameStampContext(RICON_PhysicalObservationStamp left, RICON_PhysicalObservationStamp right)
    {
        return left.m_sSourceId == right.m_sSourceId && left.m_iSourceRevision == right.m_iSourceRevision
            && left.m_sTimebaseId == right.m_sTimebaseId && left.m_sWorldIdentity == right.m_sWorldIdentity
            && left.m_sProvenance == right.m_sProvenance;
    }

    protected static bool SameStampHeader(RICON_PhysicalObservationStamp left, RICON_PhysicalObservationStamp right)
    {
        return SameStampContext(left, right) && left.m_fRawWorldTimeMs == right.m_fRawWorldTimeMs;
    }

    protected static bool ValidWitness(RICON_PhysicalEndpointRequest request, RICON_PhysicalEndpointWitness endpoint)
    {
        if (!endpoint || !endpoint.m_Observation || endpoint.m_sIdentityToken.IsEmpty() || TextOverflow(endpoint))
        {
            return false;
        }
        RICON_RadioObservation observation = endpoint.m_Observation;
        if (!RICON_RadioQueryBuilder.IsValidObservation(observation)
            || observation.m_iDeviceId != request.m_iDeviceId || observation.m_iTransceiverIndex != request.m_iTransceiverIndex
            || observation.m_vAntennaPos != request.m_vAntennaPos || endpoint.m_sAntennaSourceId != request.m_sAntennaSourceId)
        {
            return false;
        }
        if (!RICON_Num.IsFiniteVector(endpoint.m_vOwnerOrigin) || !RICON_Num.IsFiniteVector(endpoint.m_vOwnerAxis0)
            || !RICON_Num.IsFiniteVector(endpoint.m_vOwnerAxis1) || !RICON_Num.IsFiniteVector(endpoint.m_vOwnerAxis2)
            || !RICON_Num.IsFiniteVector(endpoint.m_vBodyVelocityAtPoint))
        {
            return false;
        }
        if (endpoint.m_bHasBodyVelocity)
        {
            return !endpoint.m_sBodyIdentityToken.IsEmpty() && endpoint.m_sMotionProvenance == "BODY_AT_CALLER_POINT";
        }
        return endpoint.m_sBodyIdentityToken.IsEmpty() && endpoint.m_sMotionProvenance == "MOTION_UNAVAILABLE";
    }

    protected static bool SameWitness(RICON_PhysicalEndpointWitness left, RICON_PhysicalEndpointWitness right)
    {
        if (!left || !right || !left.m_Observation || !right.m_Observation)
        {
            return false;
        }
        RICON_RadioObservation a = left.m_Observation;
        RICON_RadioObservation b = right.m_Observation;
        if (a.m_iDeviceId != b.m_iDeviceId || a.m_iTransceiverIndex != b.m_iTransceiverIndex
            || a.m_bPowered != b.m_bPowered || a.m_iTuningValue != b.m_iTuningValue
            || a.m_sNetworkKey != b.m_sNetworkKey || a.m_vAntennaPos != b.m_vAntennaPos)
        {
            return false;
        }
        if (left.m_sIdentityToken != right.m_sIdentityToken || left.m_sAntennaSourceId != right.m_sAntennaSourceId)
        {
            return false;
        }
        if (left.m_vOwnerOrigin != right.m_vOwnerOrigin || left.m_vOwnerAxis0 != right.m_vOwnerAxis0)
        {
            return false;
        }
        if (left.m_vOwnerAxis1 != right.m_vOwnerAxis1 || left.m_vOwnerAxis2 != right.m_vOwnerAxis2)
        {
            return false;
        }
        if (left.m_bHasBodyVelocity != right.m_bHasBodyVelocity || left.m_vBodyVelocityAtPoint != right.m_vBodyVelocityAtPoint)
        {
            return false;
        }
        return left.m_sBodyIdentityToken == right.m_sBodyIdentityToken && left.m_sMotionProvenance == right.m_sMotionProvenance;
    }

    protected static bool ValidTrace(RICON_PhysicalTraceWitness trace)
    {
        if (!trace || !RICON_Num.IsFinite(trace.m_fFraction) || trace.m_fFraction < 0 || trace.m_fFraction > 1
            || !RICON_Num.IsFiniteVector(trace.m_vNormal) || trace.m_iColliderIndex < -1 || trace.m_iNodeIndex < -1)
        {
            return false;
        }
        return trace.m_bHasContact || (trace.m_fFraction == 1 && trace.m_sEntityIdentity.IsEmpty());
    }

    protected static string WitnessCode(RICON_PhysicalEndpointRequest request, RICON_PhysicalEndpointWitness endpoint)
    {
        if (!endpoint)
        {
            return "ENDPOINT_UNAVAILABLE";
        }
        if (TextOverflow(endpoint))
        {
            return "RESOURCE_LIMIT";
        }
        if (!ValidWitness(request, endpoint))
        {
            return "INVALID_SOURCE_RESULT";
        }
        return "OK";
    }

    static RICON_PhysicalObservationResult Sample(RICON_PhysicalObservationRequest request,
        RICON_PhysicalObservationSource source, RICON_PhysicalObservationLimits limits)
    {
        RICON_PhysicalObservationResult result = new RICON_PhysicalObservationResult();
        result.m_sCode = "INVALID_LIMITS";
        if (!limits || !limits.IsValid())
        {
            return result;
        }
        int heightLimit = limits.m_iMaxHeightSamples;
        int traceLimit = limits.m_iMaxTraces;
        int spanLimit = limits.m_iMaxSpanMs;
        result.m_sCode = "INVALID_REQUEST";
        if (!ValidRequest(request))
        {
            return result;
        }
        ref RICON_PhysicalObservationRequest input = CopyRequest(request);
        ref RICON_TerrainDiffractionConfig terrainConfig;
        if (input.m_bSampleFirstContact && traceLimit == 0)
        {
            result.m_sCode = "RESOURCE_LIMIT";
            return result;
        }
        if (input.m_bSampleTerrain)
        {
            terrainConfig = input.m_TerrainConfig.CloneConfig();
            float dx = input.m_Rx.m_vAntennaPos[0] - input.m_Tx.m_vAntennaPos[0];
            float dz = input.m_Rx.m_vAntennaPos[2] - input.m_Tx.m_vAntennaPos[2];
            float distanceM = Math.Sqrt(dx * dx + dz * dz);
            if (!RICON_Num.IsFinite(distanceM) || distanceM > 500000)
            {
                result.m_sCode = "TERRAIN_UNAVAILABLE";
                return result;
            }
            if (terrainConfig.m_iMaxProfilePoints > heightLimit)
            {
                terrainConfig.m_iMaxProfilePoints = heightLimit;
            }
            ref RICON_TerrainSamplingResult sampling = RICON_TerrainProfileSampling.BuildDistances(distanceM, terrainConfig);
            if (sampling.m_eCode != RICON_ETerrainSamplingCode.OK)
            {
                result.m_sCode = "RESOURCE_LIMIT";
                if (sampling.m_eCode == RICON_ETerrainSamplingCode.SKIP)
                {
                    result.m_sCode = "TERRAIN_UNAVAILABLE";
                }
                return result;
            }
        }
        result.m_sCode = "SOURCE_UNAVAILABLE";
        if (!source)
        {
            return result;
        }
        result.m_iStampReads++;
        ref RICON_PhysicalObservationStamp start = source.ReadStamp();
        if (!start)
        {
            return result;
        }
        result.m_sCode = "INVALID_SOURCE_RESULT";
        if (!ValidStamp(start))
        {
            return result;
        }
        result.m_sCode = "CONTEXT_MISMATCH";
        if (!MatchesStamp(input, start))
        {
            return result;
        }
        ref RICON_PhysicalObservationStamp startHeader = CopyStampHeader(start);
        result.m_iEndpointReads++;
        ref RICON_PhysicalEndpointWitness txRaw = source.ReadEndpoint(CopyEndpointRequest(input.m_Tx));
        result.m_sCode = WitnessCode(input.m_Tx, txRaw);
        if (result.m_sCode != "OK")
        {
            return result;
        }
        ref RICON_PhysicalEndpointWitness tx = CopyEndpointWitness(txRaw);
        result.m_iEndpointReads++;
        ref RICON_PhysicalEndpointWitness rxRaw = source.ReadEndpoint(CopyEndpointRequest(input.m_Rx));
        result.m_sCode = WitnessCode(input.m_Rx, rxRaw);
        if (result.m_sCode != "OK")
        {
            return result;
        }
        ref RICON_PhysicalEndpointWitness rx = CopyEndpointWitness(rxRaw);
        ref RICON_TerrainProfile terrain;
        float txAgl;
        float rxAgl;
        if (input.m_bSampleTerrain)
        {
            ref RICON_HeightSource heights = source.GetHeightSource();
            result.m_sCode = "TERRAIN_UNAVAILABLE";
            if (!heights)
            {
                return result;
            }
            ref RICON_PhysicalBudgetHeightSource budgetSource = new RICON_PhysicalBudgetHeightSource(heights, heightLimit);
            ref RICON_WorldProfileProvider provider = new RICON_WorldProfileProvider(budgetSource);
            RICON_LinkQuery terrainQuery = new RICON_LinkQuery();
            terrainQuery.m_vTxPos = input.m_Tx.m_vAntennaPos;
            terrainQuery.m_vRxPos = input.m_Rx.m_vAntennaPos;
            RICON_EReason terrainReason = provider.GetProfile(terrainQuery, terrainConfig, terrain, txAgl, rxAgl);
            result.m_iHeightCalls = budgetSource.m_iCalls;
            if (terrainReason != RICON_EReason.OK || !terrain)
            {
                return result;
            }
        }
        ref RICON_PhysicalTraceWitness trace;
        if (input.m_bSampleFirstContact)
        {
            result.m_iTraceCalls++;
            ref RICON_PhysicalTraceWitness traceRaw = source.ReadFirstContact(input.m_Tx.m_vAntennaPos,
                input.m_Rx.m_vAntennaPos, input.m_iTraceFlags, input.m_iTraceLayerMask);
            result.m_sCode = "TRACE_UNAVAILABLE";
            if (!traceRaw)
            {
                return result;
            }
            result.m_sCode = "RESOURCE_LIMIT";
            if (traceRaw.m_sEntityIdentity.Length() > MAX_TEXT || traceRaw.m_sMaterial.Length() > MAX_TEXT)
            {
                return result;
            }
            result.m_sCode = "INVALID_SOURCE_RESULT";
            if (!ValidTrace(traceRaw))
            {
                return result;
            }
            trace = CopyTraceWitness(traceRaw);
        }
        result.m_iEndpointReads++;
        ref RICON_PhysicalEndpointWitness txAfter = source.ReadEndpoint(CopyEndpointRequest(input.m_Tx));
        result.m_sCode = WitnessCode(input.m_Tx, txAfter);
        if (result.m_sCode != "OK")
        {
            return result;
        }
        result.m_sCode = "SOURCE_CHANGED_DURING_SAMPLE";
        if (!SameWitness(tx, txAfter))
        {
            return result;
        }
        result.m_iEndpointReads++;
        ref RICON_PhysicalEndpointWitness rxAfter = source.ReadEndpoint(CopyEndpointRequest(input.m_Rx));
        result.m_sCode = WitnessCode(input.m_Rx, rxAfter);
        if (result.m_sCode != "OK")
        {
            return result;
        }
        result.m_sCode = "SOURCE_CHANGED_DURING_SAMPLE";
        if (!SameWitness(rx, rxAfter))
        {
            return result;
        }
        result.m_iStampReads++;
        ref RICON_PhysicalObservationStamp end = source.ReadStamp();
        result.m_sCode = "SOURCE_UNAVAILABLE";
        if (!end)
        {
            return result;
        }
        result.m_sCode = "INVALID_SOURCE_RESULT";
        if (end == start || !ValidStamp(end))
        {
            return result;
        }
        ref RICON_PhysicalObservationStamp endHeader = CopyStampHeader(end);
        result.m_sCode = "SOURCE_CHANGED_DURING_SAMPLE";
        if (!SameStampHeader(startHeader, start) || !SameStampContext(startHeader, endHeader)
            || !SameWitness(tx, txAfter) || !SameWitness(rx, rxAfter))
        {
            return result;
        }
        float elapsedMs;
        result.m_sCode = source.MeasureSpan(start, end, spanLimit, elapsedMs);
        if (result.m_sCode != "OK")
        {
            if (result.m_sCode != "TIME_UNQUALIFIED" && result.m_sCode != "CLOCK_ROLLBACK" && result.m_sCode != "SPAN_EXCEEDED")
            {
                result.m_sCode = "INVALID_SOURCE_RESULT";
            }
            return result;
        }
        result.m_sCode = "INVALID_SOURCE_RESULT";
        if (!RICON_Num.IsFinite(elapsedMs) || elapsedMs < 0 || elapsedMs > spanLimit)
        {
            return result;
        }
        result.m_sCode = "SOURCE_CHANGED_DURING_SAMPLE";
        if (!SameStampHeader(startHeader, start) || !SameStampHeader(endHeader, end)
            || !SameWitness(tx, txAfter) || !SameWitness(rx, rxAfter))
        {
            return result;
        }
        RICON_PhysicalObservationFrame frame = new RICON_PhysicalObservationFrame();
        frame.m_Start = startHeader;
        frame.m_End = endHeader;
        frame.m_Tx = CopyEndpointWitness(tx);
        frame.m_Rx = CopyEndpointWitness(rx);
        if (terrain)
        {
            frame.m_Terrain = terrain.CloneProfile();
        }
        frame.m_fTxHeightAglM = txAgl;
        frame.m_fRxHeightAglM = rxAgl;
        if (trace)
        {
            frame.m_FirstContact = CopyTraceWitness(trace);
        }
        frame.m_fElapsedMs = elapsedMs;
        for (int family = 0; family < 5; family++)
        {
            frame.m_aCoverage.Insert(RICON_EPathCoverage.NOT_OBSERVED);
        }
        if (terrain)
        {
            frame.m_aCoverage[RICON_EPathFamily.TERRAIN] = RICON_EPathCoverage.COMPLETE;
        }
        if (trace)
        {
            if ((input.m_iTraceFlags & TraceFlags.WORLD) != 0 && !terrain)
            {
                frame.m_aCoverage[RICON_EPathFamily.TERRAIN] = RICON_EPathCoverage.PARTIAL;
            }
            if ((input.m_iTraceFlags & TraceFlags.OCEAN) != 0)
            {
                frame.m_aCoverage[RICON_EPathFamily.WATER] = RICON_EPathCoverage.PARTIAL;
            }
            if ((input.m_iTraceFlags & TraceFlags.ENTS) != 0)
            {
                frame.m_aCoverage[RICON_EPathFamily.STRUCTURE] = RICON_EPathCoverage.PARTIAL;
                frame.m_aCoverage[RICON_EPathFamily.VEGETATION] = RICON_EPathCoverage.PARTIAL;
                frame.m_aCoverage[RICON_EPathFamily.VEHICLE] = RICON_EPathCoverage.PARTIAL;
            }
        }
        frame.m_aMissingReasons.Insert("ANTENNA_POSITION_CALLER_SUPPLIED");
        frame.m_aMissingReasons.Insert("ANTENNA_MOTION_UNVERIFIED");
        if (!tx.m_bHasBodyVelocity)
        {
            frame.m_aMissingReasons.Insert("TX_BODY_VELOCITY_UNAVAILABLE");
        }
        if (!rx.m_bHasBodyVelocity)
        {
            frame.m_aMissingReasons.Insert("RX_BODY_VELOCITY_UNAVAILABLE");
        }
        frame.m_aMissingReasons.Insert("FULL_PATH_SET_NOT_OBSERVED");
        frame.m_aMissingReasons.Insert("RF_PROFILE_NOT_SAMPLED");
        frame.m_aMissingReasons.Insert("CLOCK_MAPPING_NOT_QUALIFIED");
        frame.m_sClockQualification = "WORLD_TIMESTAMP_DOMAIN_OBSERVED_PHYSICAL_CLOCK_UNQUALIFIED";
        if (startHeader.m_sProvenance == "TEST_ONLY")
        {
            frame.m_sClockQualification = "TEST_ONLY_SPAN_OBSERVED_PHYSICAL_CLOCK_UNQUALIFIED";
        }
        result.m_Frame = frame;
        result.m_sCode = "OK";
        return result;
    }
}
