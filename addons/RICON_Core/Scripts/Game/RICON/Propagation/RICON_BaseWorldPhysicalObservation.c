//! Read-only public-API adapter. No registration, world launch, callbacks, RF synthesis or autorun.
//! Caller antenna coordinates and body-point velocity are witnesses, not a calibrated phase centre.
class RICON_PhysicalNativeEndpointBinding
{
    int m_iDeviceId;
    int m_iTransceiverIndex = -1;
    BaseRadioComponent m_Radio;
}

//! Private-to-source timestamp values never enter the public detached observation frame.
class RICON_BaseWorldPhysicalObservationStamp : RICON_PhysicalObservationStamp
{
    RICON_BaseWorldPhysicalObservationSource m_Issuer;
    BaseWorld m_World;
    WorldTimestamp m_Timestamp;
}

class RICON_BaseWorldPhysicalObservationSource : RICON_PhysicalObservationSource
{
    protected BaseWorld m_World;
    protected string m_sSourceId;
    protected int m_iSourceRevision;
    protected string m_sTimebaseId;
    protected ref RICON_PhysicalNativeEndpointBinding m_TxBinding;
    protected ref RICON_PhysicalNativeEndpointBinding m_RxBinding;
    protected ref RICON_BaseWorldHeightSource m_HeightSource;

    void RICON_BaseWorldPhysicalObservationSource(BaseWorld world, string sourceId, int sourceRevision,
        string timebaseId, RICON_PhysicalNativeEndpointBinding txBinding, RICON_PhysicalNativeEndpointBinding rxBinding)
    {
        m_World = world;
        m_sSourceId = sourceId;
        m_iSourceRevision = sourceRevision;
        m_sTimebaseId = timebaseId;
        m_TxBinding = CopyNativeBinding(txBinding);
        m_RxBinding = CopyNativeBinding(rxBinding);
        m_HeightSource = new RICON_BaseWorldHeightSource(world);
    }

    protected static RICON_PhysicalNativeEndpointBinding CopyNativeBinding(RICON_PhysicalNativeEndpointBinding binding)
    {
        if (!binding)
        {
            return null;
        }
        RICON_PhysicalNativeEndpointBinding copy = new RICON_PhysicalNativeEndpointBinding();
        copy.m_iDeviceId = binding.m_iDeviceId;
        copy.m_iTransceiverIndex = binding.m_iTransceiverIndex;
        copy.m_Radio = binding.m_Radio;
        return copy;
    }

    protected bool Ready()
    {
        return m_World && RICON_CommsManagerComponent.IsAuthority()
            && RICON_PathObservationModel.ValidKey(m_sSourceId) && m_iSourceRevision > 0
            && RICON_PathObservationModel.ValidKey(m_sTimebaseId)
            && ValidNativeBinding(m_TxBinding) && ValidNativeBinding(m_RxBinding)
            && (m_TxBinding.m_Radio != m_RxBinding.m_Radio
                || m_TxBinding.m_iTransceiverIndex != m_RxBinding.m_iTransceiverIndex);
    }

    protected static bool ValidNativeBinding(RICON_PhysicalNativeEndpointBinding binding)
    {
        return binding && binding.m_Radio && binding.m_iDeviceId >= 1 && binding.m_iDeviceId < int.MAX
            && binding.m_iTransceiverIndex >= 0 && binding.m_iTransceiverIndex < int.MAX;
    }

    protected RICON_PhysicalNativeEndpointBinding FindBinding(RICON_PhysicalEndpointRequest endpoint)
    {
        if (!endpoint || !m_TxBinding || !m_RxBinding)
        {
            return null;
        }
        if (m_TxBinding.m_iDeviceId == m_RxBinding.m_iDeviceId
            && m_TxBinding.m_iTransceiverIndex == m_RxBinding.m_iTransceiverIndex)
        {
            return null;
        }
        if (endpoint.m_iDeviceId == m_TxBinding.m_iDeviceId && endpoint.m_iTransceiverIndex == m_TxBinding.m_iTransceiverIndex)
        {
            return m_TxBinding;
        }
        if (endpoint.m_iDeviceId == m_RxBinding.m_iDeviceId && endpoint.m_iTransceiverIndex == m_RxBinding.m_iTransceiverIndex)
        {
            return m_RxBinding;
        }
        return null;
    }

    override RICON_PhysicalObservationStamp ReadStamp()
    {
        if (!Ready())
        {
            return null;
        }
        RICON_BaseWorldPhysicalObservationStamp stamp = new RICON_BaseWorldPhysicalObservationStamp();
        stamp.m_Issuer = this;
        stamp.m_World = m_World;
        stamp.m_Timestamp = m_World.GetTimestamp();
        stamp.m_sSourceId = m_sSourceId;
        // Registration revision only: not an observed environment revision and never a physical seed.
        stamp.m_iSourceRevision = m_iSourceRevision;
        stamp.m_sTimebaseId = m_sTimebaseId;
        stamp.m_sWorldIdentity = m_World.ToString();
        stamp.m_sProvenance = "ENGINE_GETTER";
        stamp.m_fRawWorldTimeMs = m_World.GetWorldTime();
        return stamp;
    }

    override string MeasureSpan(RICON_PhysicalObservationStamp start, RICON_PhysicalObservationStamp end,
        int maxSpanMs, out float elapsedMs)
    {
        elapsedMs = 0;
        RICON_BaseWorldPhysicalObservationStamp first = RICON_BaseWorldPhysicalObservationStamp.Cast(start);
        RICON_BaseWorldPhysicalObservationStamp last = RICON_BaseWorldPhysicalObservationStamp.Cast(end);
        if (!Ready() || !first || !last || first == last || maxSpanMs < 0 || maxSpanMs > 60000)
        {
            return "TIME_UNQUALIFIED";
        }
        if (first.m_Issuer != this || last.m_Issuer != this || first.m_World != m_World || last.m_World != m_World)
        {
            return "TIME_UNQUALIFIED";
        }
        if (last.m_Timestamp.Compare(first.m_Timestamp) < 0)
        {
            return "CLOCK_ROLLBACK";
        }
        // Exact comparison in the issuing world's opaque timestamp domain; no absolute float subtraction.
        // This is an observation-domain budget, not physical clock accuracy or client/server mapping.
        WorldTimestamp deadline = first.m_Timestamp.PlusMilliseconds(maxSpanMs);
        if (last.m_Timestamp.Compare(deadline) > 0)
        {
            return "SPAN_EXCEEDED";
        }
        float measured = last.m_Timestamp.DiffMilliseconds(first.m_Timestamp);
        if (!RICON_Num.IsFinite(measured) || measured < 0 || measured > 60000)
        {
            return "TIME_UNQUALIFIED";
        }
        elapsedMs = measured;
        return "OK";
    }

    override RICON_PhysicalEndpointWitness ReadEndpoint(RICON_PhysicalEndpointRequest endpoint)
    {
        if (!Ready())
        {
            return null;
        }
        RICON_PhysicalNativeEndpointBinding binding = FindBinding(endpoint);
        if (!binding || !binding.m_Radio)
        {
            return null;
        }
        BaseRadioComponent radio = binding.m_Radio;
        IEntity owner = radio.GetOwner();
        if (!owner || owner.GetWorld() != m_World || binding.m_iTransceiverIndex < 0
            || binding.m_iTransceiverIndex >= radio.TransceiversCount())
        {
            return null;
        }
        BaseTransceiver transceiver = radio.GetTransceiver(binding.m_iTransceiverIndex);
        if (!transceiver || transceiver.GetRadio() != radio)
        {
            return null;
        }
        RICON_RadioObservation observation;
        if (!RICON_BaseRadioObservationReader.TryRead(radio, endpoint.m_iDeviceId, endpoint.m_iTransceiverIndex,
            endpoint.m_vAntennaPos, observation))
        {
            return null;
        }
        vector transform[4];
        owner.GetWorldTransform(transform);
        RICON_PhysicalEndpointWitness witness = new RICON_PhysicalEndpointWitness();
        witness.m_Observation = observation;
        witness.m_sIdentityToken = owner.ToString() + "|" + radio.ToString() + "|" + transceiver.ToString();
        witness.m_sAntennaSourceId = endpoint.m_sAntennaSourceId;
        witness.m_vOwnerAxis0 = transform[0];
        witness.m_vOwnerAxis1 = transform[1];
        witness.m_vOwnerAxis2 = transform[2];
        witness.m_vOwnerOrigin = transform[3];
        witness.m_sMotionProvenance = "MOTION_UNAVAILABLE";
        Physics body = owner.GetPhysics();
        if (body && body.IsDynamic())
        {
            witness.m_bHasBodyVelocity = true;
            witness.m_vBodyVelocityAtPoint = body.GetVelocityAt(endpoint.m_vAntennaPos);
            witness.m_sBodyIdentityToken = body.ToString();
            witness.m_sMotionProvenance = "BODY_AT_CALLER_POINT";
        }
        if (radio.GetOwner() != owner || owner.GetWorld() != m_World
            || radio.GetTransceiver(binding.m_iTransceiverIndex) != transceiver || transceiver.GetRadio() != radio)
        {
            return null;
        }
        return witness;
    }

    override RICON_HeightSource GetHeightSource()
    {
        if (!Ready())
        {
            return null;
        }
        return m_HeightSource;
    }

    override RICON_PhysicalTraceWitness ReadFirstContact(vector tx, vector rx, int flags, int layerMask)
    {
        int allowed = TraceFlags.ENTS | TraceFlags.WORLD | TraceFlags.OCEAN;
        if (!Ready() || !RICON_Num.IsFiniteVector(tx) || !RICON_Num.IsFiniteVector(rx)
            || tx == rx || flags == 0 || (flags & ~allowed) != 0 || layerMask == 0)
        {
            return null;
        }
        TraceParam trace = new TraceParam();
        trace.Start = tx;
        trace.End = rx;
        trace.Flags = flags;
        trace.LayerMask = layerMask;
        float fraction = m_World.TraceMove(trace);
        RICON_PhysicalTraceWitness witness = new RICON_PhysicalTraceWitness();
        witness.m_fFraction = fraction;
        witness.m_bHasContact = fraction < 1 || trace.TraceEnt != null;
        if (trace.TraceEnt)
        {
            witness.m_sEntityIdentity = trace.TraceEnt.ToString();
        }
        witness.m_iColliderIndex = trace.ColliderIndex;
        witness.m_iNodeIndex = trace.NodeIndex;
        witness.m_vNormal = trace.TraceNorm;
        witness.m_sMaterial = trace.TraceMaterial;
        return witness;
    }
}
