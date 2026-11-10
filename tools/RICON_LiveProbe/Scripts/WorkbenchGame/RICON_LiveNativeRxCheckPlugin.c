//! Workbench bridge only; synthetic test doubles execute inside the Game module before sealing.
[WorkbenchPluginAttribute(name: "RICON: Check native RX metadata", wbModules: {"ScriptEditor"})]
class RICON_LiveNativeRxCheckPlugin : WorkbenchPlugin
{
    override void RunCommandline()
    {
        RICON_NativeRxSelfChecks checks = new RICON_NativeRxSelfChecks();
        checks.Run();
        RICON_RadioReadinessSelfChecks readinessChecks = new RICON_RadioReadinessSelfChecks();
        readinessChecks.Run();
        RICON_PairPlacementSelfChecks placementChecks = new RICON_PairPlacementSelfChecks();
        placementChecks.Run();
        RICON_CaptureSeparatedSelfChecks separatedChecks = new RICON_CaptureSeparatedSelfChecks();
        separatedChecks.Run();
        RICON_BlindBObserveChecks observeChecks = new RICON_BlindBObserveChecks();
        observeChecks.Run();
        RICON_VoNSoundObserveChecks soundChecks = new RICON_VoNSoundObserveChecks();
        soundChecks.Run();
        RICON_NativeRadioPowerChecks powerChecks = new RICON_NativeRadioPowerChecks();
        powerChecks.Run();
        RICON_RadioRestoreObservationChecks restoreObserveChecks = new RICON_RadioRestoreObservationChecks();
        restoreObserveChecks.Run();
        RICON_CaptureBPowerOffChecks controlPowerChecks = new RICON_CaptureBPowerOffChecks();
        controlPowerChecks.Run();
        RICON_DirectNearChecks directNearChecks = new RICON_DirectNearChecks();
        directNearChecks.Run();
        RICON_DirectBPowerOffChecks directPowerChecks = new RICON_DirectBPowerOffChecks();
        directPowerChecks.Run();
        RICON_DirectPower10sChecks presetChecks = new RICON_DirectPower10sChecks();
        presetChecks.Run();
    }
}
