//! TOOL_ONLY manual menu entry. Never starts a world or calls native sampling from CLI.
[WorkbenchPluginAttribute(name: "RICON: physical observation (one shot)", wbModules: {"WorldEditor"})]
class RICON_LivePhysicalObservationPlugin : WorkbenchPlugin
{
    protected static ref RICON_PhysicalWorldProbeLatch s_Attempt = new RICON_PhysicalWorldProbeLatch();
    protected static string s_RunId;

    protected static void PrintLines(array<string> lines)
    {
        if (!lines || lines.Count() > RICON_LivePhysicalObservationProbe.MAX_RECEIPT_LINES)
        {
            Print("[RICON_PHYS_WORLD] terminal=1 status=REFUSED code=INVALID_RESULT");
            return;
        }
        foreach (string line : lines)
        {
            Print(line);
        }
    }

    override void Run()
    {
        if (!s_Attempt.TakeAttempt())
        {
            PrintLines(RICON_LivePhysicalObservationProbe.Refusal(s_RunId, "ALREADY_ATTEMPTED"));
            return;
        }
        RICON_PhysicalWorldProbeRequest request = new RICON_PhysicalWorldProbeRequest();
        System.GetCLIParam("riconPhysicalObservation", request.m_sEnable);
        System.GetCLIParam("riconPhysicalRun", request.m_sRunId);
        System.GetCLIParam("riconPhysicalTxEntity", request.m_sTxOwnerName);
        System.GetCLIParam("riconPhysicalRxEntity", request.m_sRxOwnerName);
        string code = RICON_LivePhysicalObservationProbe.ValidateRequest(request);
        if (code != "OK")
        {
            PrintLines(RICON_LivePhysicalObservationProbe.Refusal("", code));
            return;
        }
        s_RunId = request.m_sRunId;
        WorldEditor editor = Workbench.GetModule(WorldEditor);
        WorldEditorAPI api;
        if (editor)
        {
            api = editor.GetApi();
        }
        ArmaReforgerScripted game = ArmaReforgerScripted.Cast(GetGame());
        BaseWorld world;
        if (game)
        {
            world = game.GetWorld();
        }
        bool currentPlay = api && api.IsGameMode();
        bool runtimeWorld = world && !world.IsEditMode();
        bool authority;
        if (currentPlay && runtimeWorld)
        {
            authority = RICON_CommsManagerComponent.IsAuthority();
        }
        code = RICON_LivePhysicalObservationProbe.AdmitContext(currentPlay, runtimeWorld, authority);
        if (code != "OK")
        {
            PrintLines(RICON_LivePhysicalObservationProbe.Refusal(s_RunId, code));
            return;
        }
        PrintLines(RICON_LivePhysicalObservationProbe.SampleNamedOwners(world, request));
    }

    override void RunCommandline()
    {
        s_Attempt.TakeAttempt();
        PrintLines(RICON_LivePhysicalObservationProbe.Refusal(s_RunId, "CLI_NATIVE_FORBIDDEN"));
    }
}
