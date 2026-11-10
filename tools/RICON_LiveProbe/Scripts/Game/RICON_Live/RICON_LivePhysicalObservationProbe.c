//! TOOL_ONLY, explicit one-shot public native observations. No hooks, physics synthesis or autorun.
//! Scene-unique names require an external fixture audit; a resolved instance does not prove uniqueness.
class RICON_PhysicalWorldProbeRequest
{
    string m_sEnable;
    string m_sRunId;
    string m_sTxOwnerName;
    string m_sRxOwnerName;
}

class RICON_PhysicalWorldProbeLatch
{
    protected bool m_bTaken;

    bool TakeAttempt()
    {
        if (m_bTaken)
        {
            return false;
        }
        m_bTaken = true;
        return true;
    }
}

class RICON_LivePhysicalObservationProbe
{
    static const int MAX_HEIGHT_SAMPLES = 8;
    static const int MAX_TRACES = 1;
    static const int MAX_SPAN_MS = 100;
    static const int MAX_RECEIPT_LINES = 12;

    static string ValidateRequest(RICON_PhysicalWorldProbeRequest request)
    {
        if (!request)
        {
            return "INVALID_REQUEST";
        }
        if (request.m_sEnable.Length() != 1 || request.m_sEnable != "1")
        {
            return "PROBE_DISABLED";
        }
        if (!RICON_PathObservationModel.ValidKey(request.m_sRunId))
        {
            return "INVALID_REQUEST";
        }
        if (!RICON_PathObservationModel.ValidKey(request.m_sTxOwnerName)
            || !RICON_PathObservationModel.ValidKey(request.m_sRxOwnerName))
        {
            return "INVALID_REQUEST";
        }
        if (request.m_sTxOwnerName == request.m_sRxOwnerName)
        {
            return "INVALID_REQUEST";
        }
        return "OK";
    }

    static string AdmitContext(bool currentPlay, bool runtimeWorld, bool authority)
    {
        if (!currentPlay || !runtimeWorld)
        {
            return "NOT_CURRENT_PLAY";
        }
        if (!authority)
        {
            return "AUTHORITY_REQUIRED";
        }
        return "OK";
    }

    static bool InDistanceDomain(vector tx, vector rx)
    {
        if (!RICON_Num.IsFiniteVector(tx) || !RICON_Num.IsFiniteVector(rx))
        {
            return false;
        }
        float dx = rx[0] - tx[0];
        float dz = rx[2] - tx[2];
        if (!RICON_Num.IsFinite(dx) || !RICON_Num.IsFinite(dz))
        {
            return false;
        }
        if (Math.AbsFloat(dx) > 100 || Math.AbsFloat(dz) > 100)
        {
            return false;
        }
        if (!RICON_Num.IsFinite(vector.Distance(tx, rx)))
        {
            return false;
        }
        float horizontalM = Math.Sqrt(dx * dx + dz * dz);
        return horizontalM >= 2 && horizontalM <= 100;
    }

    protected static string SafeText(string value, int limit = 128)
    {
        if (value.Length() > limit)
        {
            return RICON_LiveText.Esc(value.Substring(0, limit)) + "[TRUNCATED]";
        }
        return RICON_LiveText.Esc(value);
    }

    static array<string> Refusal(string runId, string code)
    {
        array<string> lines = {};
        lines.Insert("[RICON_PHYS_WORLD] terminal=1 status=REFUSED run=" + SafeText(runId, 64)
            + " code=" + SafeText(code) + " worldQualification=NONE");
        return lines;
    }

    protected static bool ReceiptShape(RICON_PhysicalObservationFrame frame)
    {
        if (!frame || !frame.m_Start || !frame.m_End || !frame.m_Tx || !frame.m_Rx)
        {
            return false;
        }
        if (!frame.m_Tx.m_Observation || !frame.m_Rx.m_Observation)
        {
            return false;
        }
        if (!frame.m_aCoverage || frame.m_aCoverage.Count() != 5)
        {
            return false;
        }
        if (!frame.m_aMissingReasons || frame.m_aMissingReasons.Count() > 16)
        {
            return false;
        }
        if (frame.m_sClockQualification.IsEmpty() || frame.m_bAtomicSnapshot || frame.m_bCompletePhysicalPaths)
        {
            return false;
        }
        if (frame.m_Terrain)
        {
            if (!frame.m_Terrain.m_aDistanceKm || !frame.m_Terrain.m_aHeightM)
            {
                return false;
            }
            int count = frame.m_Terrain.m_aDistanceKm.Count();
            if (count < 1 || count > MAX_HEIGHT_SAMPLES || count != frame.m_Terrain.m_aHeightM.Count())
            {
                return false;
            }
        }
        return true;
    }

    //! Only explicitly selected scalar fields are formatted. Never dump or hash a radio DTO.
    static array<string> FormatReceipt(string runId, RICON_PhysicalObservationResult result)
    {
        if (!result)
        {
            return Refusal(runId, "INVALID_RESULT");
        }
        if (result.m_sCode != "OK")
        {
            return Refusal(runId, result.m_sCode);
        }
        RICON_PhysicalObservationFrame frame = result.m_Frame;
        if (!ReceiptShape(frame))
        {
            return Refusal(runId, "INVALID_RESULT");
        }
        array<string> lines = {};
        string prefix = "[RICON_PHYS_WORLD] run=" + SafeText(runId, 64);
        lines.Insert(prefix + " scope=OBSERVATION_ONLY sceneNameUniqueness=EXTERNAL_PRECONDITION"
            + " sceneAudit=NOT_VERIFIED_BY_PROBE antenna=CALLER_SUPPLIED_UNQUALIFIED rfCalibration=NONE atomic=0 fullPaths=0");
        string sourceLine = prefix + " sourceProvenance=" + SafeText(frame.m_Start.m_sProvenance);
        sourceLine += " sourceId=" + SafeText(frame.m_Start.m_sSourceId);
        sourceLine += " sourceRegistrationRevision=" + frame.m_Start.m_iSourceRevision.ToString();
        sourceLine += " worldLocal=" + SafeText(frame.m_Start.m_sWorldIdentity);
        sourceLine += " clockQualification=" + SafeText(frame.m_sClockQualification);
        lines.Insert(sourceLine);
        string timeLine = prefix + " rawStartMs=" + frame.m_Start.m_fRawWorldTimeMs.ToString();
        timeLine += " rawEndMs=" + frame.m_End.m_fRawWorldTimeMs.ToString();
        timeLine += " sourceSpanMs=" + frame.m_fElapsedMs.ToString();
        timeLine += " timebase=" + SafeText(frame.m_Start.m_sTimebaseId);
        lines.Insert(timeLine);
        string txLine = prefix + " txLocal=" + frame.m_Tx.m_Observation.m_iDeviceId.ToString();
        txLine += " trx=" + frame.m_Tx.m_Observation.m_iTransceiverIndex.ToString();
        txLine += " rawTuning=" + frame.m_Tx.m_Observation.m_iTuningValue.ToString();
        txLine += " powered=" + frame.m_Tx.m_Observation.m_bPowered.ToString(true);
        txLine += " callerPoint=" + SafeText(frame.m_Tx.m_Observation.m_vAntennaPos.ToString());
        txLine += " pointSource=" + SafeText(frame.m_Tx.m_sAntennaSourceId);
        txLine += " radioLocalWitness=" + SafeText(frame.m_Tx.m_sIdentityToken);
        lines.Insert(txLine);
        string rxLine = prefix + " rxLocal=" + frame.m_Rx.m_Observation.m_iDeviceId.ToString();
        rxLine += " trx=" + frame.m_Rx.m_Observation.m_iTransceiverIndex.ToString();
        rxLine += " rawTuning=" + frame.m_Rx.m_Observation.m_iTuningValue.ToString();
        rxLine += " powered=" + frame.m_Rx.m_Observation.m_bPowered.ToString(true);
        rxLine += " callerPoint=" + SafeText(frame.m_Rx.m_Observation.m_vAntennaPos.ToString());
        rxLine += " pointSource=" + SafeText(frame.m_Rx.m_sAntennaSourceId);
        rxLine += " radioLocalWitness=" + SafeText(frame.m_Rx.m_sIdentityToken);
        lines.Insert(rxLine);
        string callLine = prefix + " stampReads=" + result.m_iStampReads.ToString();
        callLine += " endpointReads=" + result.m_iEndpointReads.ToString();
        callLine += " heightCalls=" + result.m_iHeightCalls.ToString();
        callLine += " traceCalls=" + result.m_iTraceCalls.ToString();
        lines.Insert(callLine);
        string terrain = "NOT_OBSERVED";
        if (frame.m_Terrain)
        {
            terrain = "TEST_ONLY_SAMPLING_CONFIG";
            for (int point = 0; point < frame.m_Terrain.m_aDistanceKm.Count(); point++)
            {
                terrain += "|km:" + frame.m_Terrain.m_aDistanceKm[point].ToString()
                    + "/yM:" + frame.m_Terrain.m_aHeightM[point].ToString();
            }
            terrain += "|txAglM:" + frame.m_fTxHeightAglM.ToString() + "/rxAglM:" + frame.m_fRxHeightAglM.ToString();
        }
        lines.Insert(prefix + " terrain=" + terrain);
        string contact = "NOT_OBSERVED";
        if (frame.m_FirstContact)
        {
            contact = "known:" + frame.m_FirstContact.m_bHasContact.ToString(true);
            contact += "|fraction:" + frame.m_FirstContact.m_fFraction.ToString();
            contact += "|normal:" + SafeText(frame.m_FirstContact.m_vNormal.ToString());
            contact += "|entityLocal:" + SafeText(frame.m_FirstContact.m_sEntityIdentity);
            contact += "|materialHint:" + SafeText(frame.m_FirstContact.m_sMaterial);
        }
        lines.Insert(prefix + " firstContact=" + contact + " completeRays=0 thickness=UNKNOWN");
        string txVelocity = "UNAVAILABLE";
        string rxVelocity = "UNAVAILABLE";
        if (frame.m_Tx.m_bHasBodyVelocity)
        {
            txVelocity = SafeText(frame.m_Tx.m_vBodyVelocityAtPoint.ToString());
        }
        if (frame.m_Rx.m_bHasBodyVelocity)
        {
            rxVelocity = SafeText(frame.m_Rx.m_vBodyVelocityAtPoint.ToString());
        }
        string bodyLine = prefix + " txBodyKnown=" + frame.m_Tx.m_bHasBodyVelocity.ToString(true);
        bodyLine += " txBodyPointVelocity=" + txVelocity;
        bodyLine += " rxBodyKnown=" + frame.m_Rx.m_bHasBodyVelocity.ToString(true);
        bodyLine += " rxBodyPointVelocity=" + rxVelocity;
        bodyLine += " antennaMotion=UNVERIFIED";
        lines.Insert(bodyLine);
        string missing;
        foreach (string reason : frame.m_aMissingReasons)
        {
            missing += "|" + SafeText(reason, 64);
        }
        string coverage;
        foreach (int family : frame.m_aCoverage)
        {
            coverage += "|" + family.ToString();
        }
        lines.Insert(prefix + " familyCoverage=" + coverage + " missing=" + missing);
        lines.Insert(prefix + " terminal=1 status=OBSERVATION_OK code=OK physicalProfile=UNQUALIFIED");
        return lines;
    }

    protected static RICON_PhysicalWorldProbeRequest CopyRequest(RICON_PhysicalWorldProbeRequest request)
    {
        RICON_PhysicalWorldProbeRequest copy = new RICON_PhysicalWorldProbeRequest();
        copy.m_sEnable = request.m_sEnable;
        copy.m_sRunId = request.m_sRunId;
        copy.m_sTxOwnerName = request.m_sTxOwnerName;
        copy.m_sRxOwnerName = request.m_sRxOwnerName;
        return copy;
    }

    protected static bool StillBound(BaseWorld world, string name, IEntity owner,
        BaseRadioComponent radio, BaseTransceiver transceiver, vector point)
    {
        if (!world || !owner || !radio || !transceiver)
        {
            return false;
        }
        if (owner.GetWorld() != world || world.FindEntityByName(name) != owner)
        {
            return false;
        }
        if (radio.GetOwner() != owner || owner.FindComponent(BaseRadioComponent) != radio)
        {
            return false;
        }
        if (radio.TransceiversCount() < 1 || radio.GetTransceiver(0) != transceiver || transceiver.GetRadio() != radio)
        {
            return false;
        }
        vector transform[4];
        owner.GetWorldTransform(transform);
        return transform[3] == point;
    }

    static array<string> SampleNamedOwners(BaseWorld world, RICON_PhysicalWorldProbeRequest request)
    {
        string code = ValidateRequest(request);
        if (code != "OK")
        {
            return Refusal("", code);
        }
        ref RICON_PhysicalWorldProbeRequest input = CopyRequest(request);
        ChimeraGame game = GetGame();
        if (!game || !world || game.GetWorld() != world)
        {
            return Refusal(input.m_sRunId, "NOT_CURRENT_PLAY");
        }
        if (world.IsEditMode())
        {
            return Refusal(input.m_sRunId, "NOT_CURRENT_PLAY");
        }
        if (!RICON_CommsManagerComponent.IsAuthority())
        {
            return Refusal(input.m_sRunId, "AUTHORITY_REQUIRED");
        }
        IEntity txOwner = world.FindEntityByName(input.m_sTxOwnerName);
        IEntity rxOwner = world.FindEntityByName(input.m_sRxOwnerName);
        if (!txOwner || !rxOwner || txOwner == rxOwner)
        {
            return Refusal(input.m_sRunId, "OWNER_UNAVAILABLE");
        }
        if (txOwner.GetWorld() != world || rxOwner.GetWorld() != world)
        {
            return Refusal(input.m_sRunId, "OWNER_UNAVAILABLE");
        }
        BaseRadioComponent txRadio = BaseRadioComponent.Cast(txOwner.FindComponent(BaseRadioComponent));
        BaseRadioComponent rxRadio = BaseRadioComponent.Cast(rxOwner.FindComponent(BaseRadioComponent));
        if (!txRadio || !rxRadio || txRadio == rxRadio)
        {
            return Refusal(input.m_sRunId, "RADIO_UNAVAILABLE");
        }
        if (txRadio.TransceiversCount() < 1 || rxRadio.TransceiversCount() < 1)
        {
            return Refusal(input.m_sRunId, "TRANSCEIVER_UNAVAILABLE");
        }
        BaseTransceiver txTransceiver = txRadio.GetTransceiver(0);
        BaseTransceiver rxTransceiver = rxRadio.GetTransceiver(0);
        if (!txTransceiver || !rxTransceiver)
        {
            return Refusal(input.m_sRunId, "TRANSCEIVER_UNAVAILABLE");
        }
        vector txTransform[4];
        vector rxTransform[4];
        txOwner.GetWorldTransform(txTransform);
        rxOwner.GetWorldTransform(rxTransform);
        vector txPoint = txTransform[3];
        vector rxPoint = rxTransform[3];
        if (!InDistanceDomain(txPoint, rxPoint))
        {
            return Refusal(input.m_sRunId, "DISTANCE_OUT_OF_PROBE_DOMAIN");
        }
        RICON_PhysicalNativeEndpointBinding txBinding = new RICON_PhysicalNativeEndpointBinding();
        txBinding.m_iDeviceId = 101;
        txBinding.m_iTransceiverIndex = 0;
        txBinding.m_Radio = txRadio;
        RICON_PhysicalNativeEndpointBinding rxBinding = new RICON_PhysicalNativeEndpointBinding();
        rxBinding.m_iDeviceId = 202;
        rxBinding.m_iTransceiverIndex = 0;
        rxBinding.m_Radio = rxRadio;
        RICON_PhysicalObservationRequest query = new RICON_PhysicalObservationRequest();
        query.m_Tx = new RICON_PhysicalEndpointRequest();
        query.m_Tx.m_iDeviceId = 101;
        query.m_Tx.m_iTransceiverIndex = 0;
        query.m_Tx.m_vAntennaPos = txPoint;
        query.m_Tx.m_sAntennaSourceId = "CALLER_ENTITY_ORIGIN";
        query.m_Rx = new RICON_PhysicalEndpointRequest();
        query.m_Rx.m_iDeviceId = 202;
        query.m_Rx.m_iTransceiverIndex = 0;
        query.m_Rx.m_vAntennaPos = rxPoint;
        query.m_Rx.m_sAntennaSourceId = "CALLER_ENTITY_ORIGIN";
        query.m_sExpectedSourceId = input.m_sRunId;
        query.m_iExpectedSourceRevision = 1;
        query.m_sTimebaseId = "WORLD_OBSERVATION_DOMAIN";
        query.m_bSampleTerrain = true;
        query.m_bSampleFirstContact = true;
        query.m_iTraceFlags = TraceFlags.ENTS | TraceFlags.WORLD | TraceFlags.OCEAN;
        query.m_iTraceLayerMask = -1;
        query.m_TerrainConfig = new RICON_TerrainDiffractionConfig();
        query.m_TerrainConfig.m_fEarthRadiusKm = 6371;
        query.m_TerrainConfig.m_fKFactor = 4.0 / 3.0;
        query.m_TerrainConfig.m_fPermittivity = 15;
        query.m_TerrainConfig.m_fConductivitySm = 0.005;
        query.m_TerrainConfig.m_ePolarization = RICON_ETerrainPolarization.VERTICAL;
        query.m_TerrainConfig.m_fMaxProfileStepKm = 0.025;
        query.m_TerrainConfig.m_iMaxProfilePoints = MAX_HEIGHT_SAMPLES;
        RICON_PhysicalObservationLimits limits = new RICON_PhysicalObservationLimits();
        limits.m_iMaxHeightSamples = MAX_HEIGHT_SAMPLES;
        limits.m_iMaxTraces = MAX_TRACES;
        limits.m_iMaxSpanMs = MAX_SPAN_MS;
        if (!StillBound(world, input.m_sTxOwnerName, txOwner, txRadio, txTransceiver, txPoint)
            || !StillBound(world, input.m_sRxOwnerName, rxOwner, rxRadio, rxTransceiver, rxPoint))
        {
            return Refusal(input.m_sRunId, "OWNER_CHANGED");
        }
        if (!game || game != GetGame() || game.GetWorld() != world)
        {
            return Refusal(input.m_sRunId, "OWNER_CHANGED");
        }
        if (world.IsEditMode() || !RICON_CommsManagerComponent.IsAuthority())
        {
            return Refusal(input.m_sRunId, "OWNER_CHANGED");
        }
        ref RICON_BaseWorldPhysicalObservationSource source = new RICON_BaseWorldPhysicalObservationSource(
            world, input.m_sRunId, 1, "WORLD_OBSERVATION_DOMAIN", txBinding, rxBinding);
        ref RICON_PhysicalObservationResult observed = RICON_PhysicalObservationSampler.Sample(query, source, limits);
        if (!game || game != GetGame() || game.GetWorld() != world)
        {
            return Refusal(input.m_sRunId, "OWNER_CHANGED");
        }
        if (world.IsEditMode() || !RICON_CommsManagerComponent.IsAuthority())
        {
            return Refusal(input.m_sRunId, "OWNER_CHANGED");
        }
        if (!StillBound(world, input.m_sTxOwnerName, txOwner, txRadio, txTransceiver, txPoint)
            || !StillBound(world, input.m_sRxOwnerName, rxOwner, rxRadio, rxTransceiver, rxPoint))
        {
            return Refusal(input.m_sRunId, "OWNER_CHANGED");
        }
        if (observed && observed.m_sCode == "OK" && observed.m_Frame)
        {
            if (observed.m_Frame.m_Tx.m_vOwnerOrigin != txPoint || observed.m_Frame.m_Rx.m_vOwnerOrigin != rxPoint)
            {
                return Refusal(input.m_sRunId, "OWNER_CHANGED");
            }
        }
        return FormatReceipt(input.m_sRunId, observed);
    }
}
