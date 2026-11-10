//! TOOL_ONLY: own-resource identity/open and deferred read only; no Save/rebuild/audio.
[WorkbenchPluginAttribute(name: "RICON: own VoN resource registration", wbModules: {"ScriptEditor"})]
class RICON_VoNAssetPreparePlugin : WorkbenchPlugin
{
	protected void ReadDeferred()
	{
		Sleep(2000);
		ResourceManager manager = Workbench.GetModule(ResourceManager);
		string absolute;
		if (!manager || !Workbench.GetAbsolutePath("$RICON_LiveProbe:Sounds/RICON_VoNProbe/RICON_VoNProbe.sig", absolute, true)) { Print("[RICON_ASSET_PREP] done: READ_PATH_REFUSED"); Workbench.Exit(10); return; }
		absolute.Replace("\\", "/");
		if (!absolute.Contains("/tools/RICON_LiveProbe/Sounds/RICON_VoNProbe/")) { Print("[RICON_ASSET_PREP] done: READ_OWNER_REFUSED"); Workbench.Exit(10); return; }
		MetaFile meta = manager.GetMetaFile(absolute);
		if (!meta) { Print("[RICON_ASSET_PREP] done: READ_META_MISSING"); Workbench.Exit(10); return; }
		ResourceName identity = meta.GetResourceID();
		if (identity != "{181E1B71C9410BEF}Sounds/RICON_VoNProbe/RICON_VoNProbe.sig") { Print("[RICON_ASSET_PREP] done: READ_IDENTITY_REFUSED"); Workbench.Exit(10); return; }
		PrintFormat("[RICON_ASSET_PREP] readBegin=%1 playback=NOT_REQUESTED", identity);
		Resource holder = Resource.Load(identity);
		if (!holder || !holder.IsValid()) { Print("[RICON_ASSET_PREP] done: RESOURCE_READ_INVALID"); Workbench.Exit(10); return; }
		BaseResourceObject object = holder.GetResource();
		if (!object) { Print("[RICON_ASSET_PREP] done: RESOURCE_READ_NO_OBJECT"); Workbench.Exit(10); return; }
		PrintFormat("[RICON_ASSET_PREP] loadedResource=%1", object.GetResourceName());
		BaseContainer container = object.ToBaseContainer();
		if (!container) { Print("[RICON_ASSET_PREP] done: RESOURCE_LOADED_NON_CONTAINER_PROGRAM_UNKNOWN"); Workbench.Exit(0); return; }
		string type = container.GetClassName();
		PrintFormat("[RICON_ASSET_PREP] loadedContainerClass=%1 representation=DATA_OR_CACHE", type);
		BaseContainer compiled;
		if (type == "AudioSignalResClass") compiled = container.GetObject("compiled");
		else if (type == "IOPCompiledClass") compiled = container;
		if (!compiled || compiled.GetClassName() != "IOPCompiledClass") { Print("[RICON_ASSET_PREP] done: RESOURCE_LOADED_PROGRAM_NOT_EXPOSED"); Workbench.Exit(0); return; }
		int processed = -1;
		bool p = compiled.Get("processed", processed);
		BaseContainerList inputs = compiled.GetObjectArray("ins");
		BaseContainerList operations = compiled.GetObjectArray("ops");
		BaseContainerList outputs = compiled.GetObjectArray("outs");
		if (!p || !inputs || !operations || !outputs) { Print("[RICON_ASSET_PREP] done: RESOURCE_LOADED_FIELDS_NOT_READABLE"); Workbench.Exit(10); return; }
		PrintFormat("[RICON_ASSET_PREP] loadedDescriptor processed=%1 ins=%2 ops=%3 outs=%4 representation=DATA_OR_CACHE nativeVoiceQualified=0", processed, inputs.Count(), operations.Count(), outputs.Count());
		Print("[RICON_ASSET_PREP] done: READ_ONLY_DESCRIPTOR_OBSERVED_NOT_AUDIO_PASS");
		Workbench.Exit(0);
	}
	protected void OpenDeferred()
	{
		Sleep(2000);
		ResourceManager manager = Workbench.GetModule(ResourceManager);
		AudioEditor editor = Workbench.GetModule(AudioEditor);
		string absolute;
		if (!manager || !editor || !Workbench.GetAbsolutePath("$RICON_LiveProbe:Sounds/RICON_VoNProbe/RICON_VoNProbe.sig", absolute, true)) { Print("[RICON_ASSET_PREP] done: DEFERRED_OPEN_REFUSED"); Workbench.Exit(9); return; }
		MetaFile meta = manager.GetMetaFile(absolute);
		if (!meta) { Print("[RICON_ASSET_PREP] done: DEFERRED_NO_META"); Workbench.Exit(9); return; }
		ResourceName identity = meta.GetResourceID();
		bool opened = editor.SetOpenedResource(identity);
		Sleep(2000);
		PrintFormat("[RICON_ASSET_PREP] deferredOwnOpen=%1 resource=%2 saveRequested=0 playback=NOT_REQUESTED", opened, identity);
	}
	protected bool Check(string path)
	{
		ResourceManager manager = Workbench.GetModule(ResourceManager);
		string absolute;
		if (!manager || !Workbench.GetAbsolutePath(path, absolute, true)) return false;
		absolute.Replace("\\", "/");
		if (!absolute.Contains("/tools/RICON_LiveProbe/Sounds/RICON_VoNProbe/")) return false;
		MetaFile meta = manager.GetMetaFile(absolute);
		if (!meta) return false;
		ResourceName identity = meta.GetResourceID();
		PrintFormat("[RICON_ASSET_PREP] registeredIdentity=%1 path=%2 scope=REGISTRATION_ONLY", identity, path);
		return !identity.IsEmpty();
	}

	override void RunCommandline()
	{
		string mode;
		if (!System.GetCLIParam("riconVoNAssetMode", mode) || (mode != "register-own" && mode != "deferred-own-open" && mode != "deferred-resource-read"))
		{
			Print("[RICON_ASSET_PREP] done: REFUSED_MODE");
			Workbench.Exit(4);
			return;
		}
		if (mode == "deferred-resource-read")
		{
			thread ReadDeferred();
			return;
		}
		if (mode == "deferred-own-open")
		{
			bool moduleOpened = Workbench.OpenModule(AudioEditor);
			PrintFormat("[RICON_ASSET_PREP] deferredModuleOpened=%1", moduleOpened);
			thread OpenDeferred();
			return;
		}
		bool sig = Check("$RICON_LiveProbe:Sounds/RICON_VoNProbe/RICON_VoNProbe.sig");
		bool acp = Check("$RICON_LiveProbe:Sounds/RICON_VoNProbe/RICON_VoNProbe.acp");
		PrintFormat("[RICON_ASSET_PREP] terminal registrationSig=%1 registrationAcp=%2 graphQualified=0 playback=NOT_REQUESTED play=NOT_REQUESTED", sig, acp);
		if (sig && acp) { Print("[RICON_ASSET_PREP] done: REGISTRATION_CONFIRMED_ONLY"); Workbench.Exit(0); }
		else { Print("[RICON_ASSET_PREP] done: REGISTRATION_INCOMPLETE"); Workbench.Exit(6); }
	}
}
