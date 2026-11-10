//! TOOL_ONLY: source prefab registration/inspection; no world or audio execution.
[WorkbenchPluginAttribute(name: "RICON: inherited VoN prefab probe", wbModules: {"ScriptEditor"})]
class RICON_VoNInheritedPrefabProbePlugin : WorkbenchPlugin
{
	static const string OWN_PATH = "$RICON_LiveProbe:Prefabs/RICON_VoNProbeCharacter.et";
	static const string OWN_ABSOLUTE = "C:/Users/Ded/Documents/Projects/ARMA 3 Scripts/RICON-Reforger/tools/RICON_LiveProbe/Prefabs/RICON_VoNProbeCharacter.et";
	static const string OWN_RESOURCE_PATH = "Prefabs/RICON_VoNProbeCharacter.et";
	static const ResourceName ORIGINAL = "{26A9756790131354}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Rifleman.et";
	static const ResourceName STOCK_ACP = "{B4C3941EC8B2AD20}Sounds/VON/von.acp";
	static const ResourceName OWN_ACP = "{63C2132413FE4337}Sounds/RICON_VoNProbe/RICON_VoNProbe.acp";
	static const ResourceName ORIGINAL_GUID_ONLY = "{26A9756790131354}";
	static const ResourceName OWN_GUID_ONLY = "{0DE75817F6D87DD2}";
	static const ResourceName OWN_FULL = "{0DE75817F6D87DD2}Prefabs/RICON_VoNProbeCharacter.et";
	static const int MAX_COMPONENTS = 128;

	protected void Finish(string result, string reason, int code)
	{
		PrintFormat("[RICON_VONINHERIT] terminal=1 result=%1 reason=%2 code=%3 scope=SOURCE_CONTAINER nativeVoiceQualified=0", result, reason, code);
		Workbench.Exit(code);
	}

	protected BaseContainer SelectVoN(Resource holder, string stage, bool capabilityControl = false)
	{
		string failure = "STOP";
		int failureCode = 2;
		if (capabilityControl)
		{
			failure = "STOP_CAPABILITY";
			failureCode = 6;
		}
		if (!holder || !holder.IsValid() || !holder.GetResource())
		{
			Finish(failure, stage + "_RESOURCE_INVALID", failureCode);
			return null;
		}
		IEntitySource source = holder.GetResource().ToEntitySource();
		if (!source)
		{
			Finish(failure, stage + "_NOT_ENTITY_SOURCE", failureCode);
			return null;
		}
		int count = source.GetComponentCount();
		if (count <= 0 || count > MAX_COMPONENTS)
		{
			Finish(failure, stage + "_COMPONENT_COUNT", failureCode);
			return null;
		}
		BaseContainer selected;
		int matches = 0;
		for (int index = 0; index < count; index++)
		{
			IEntityComponentSource component = source.GetComponent(index);
			if (!component)
			{
				Finish(failure, stage + "_NULL_COMPONENT", failureCode);
				return null;
			}
			if (component.GetClassName() == "SCR_VoNComponent")
			{
				selected = component;
				matches++;
			}
		}
		PrintFormat("[RICON_VONINHERIT] stage=%1 components=%2 vonMatches=%3", stage, count, matches);
		if (matches != 1 || !selected)
		{
			Finish(failure, stage + "_VON_NOT_UNIQUE", failureCode);
			return null;
		}
		return selected;
	}

	protected void InspectDeferred()
	{
		Sleep(2000);
		ResourceManager manager = Workbench.GetModule(ResourceManager);
		string absolute;
		if (!manager || !Workbench.GetAbsolutePath(OWN_PATH, absolute, true))
		{
			Finish("STOP", "NO_MANAGER_OR_OWN_PATH", 2);
			return;
		}
		absolute.Replace("\\", "/");
		if (absolute != OWN_ABSOLUTE)
		{
			Finish("STOP", "OWN_ABSOLUTE_PATH_MISMATCH", 2);
			return;
		}
		Resource originalHolder = Resource.Load(ORIGINAL);
		BaseContainer originalVoN = SelectVoN(originalHolder, "ORIGINAL");
		if (!originalVoN)
			return;
		ResourceName originalBefore;
		if (!originalVoN.Get("Filename", originalBefore) || originalBefore != STOCK_ACP)
		{
			Finish("STOP", "ORIGINAL_FILENAME_NOT_STOCK", 2);
			return;
		}
		PrintFormat("[RICON_VONINHERIT] phase=REGISTER_BEGIN path=%1 buildRuntimeResource=1 originalFilename=%2", absolute, originalBefore);
		bool registered = manager.RegisterResourceFile(absolute, true);
		PrintFormat("[RICON_VONINHERIT] phase=REGISTER_RETURN returned=%1", registered);
		if (!registered)
		{
			Finish("STOP", "REGISTER_RETURNED_FALSE", 3);
			return;
		}
		MetaFile meta = manager.GetMetaFile(absolute);
		if (!meta)
		{
			Finish("STOP", "META_NOT_AVAILABLE", 3);
			return;
		}
		ResourceName identity = meta.GetResourceID();
		PrintFormat("[RICON_VONINHERIT] phase=NATIVE_RESOURCE_ID identity=%1", identity);
		if (identity.IsEmpty() || !identity.IsExternal() || identity.GetPath() != OWN_RESOURCE_PATH)
		{
			Finish("STOP", "RESOURCE_ID_OR_PATH_MISMATCH", 3);
			return;
		}
		Resource childHolder = Resource.Load(identity);
		BaseContainer childVoN = SelectVoN(childHolder, "CHILD");
		if (!childVoN)
			return;
		ResourceName childFilename;
		ResourceName originalAfter;
		bool childRead = childVoN.Get("Filename", childFilename);
		bool originalRead = originalVoN.Get("Filename", originalAfter);
		bool originalUnchanged = originalRead && originalAfter == originalBefore;
		PrintFormat("[RICON_VONINHERIT] phase=READBACK childRead=%1 childFilename=%2 originalRead=%3 originalFilename=%4 originalUnchanged=%5", childRead, childFilename, originalRead, originalAfter, originalUnchanged);
		if (!originalUnchanged)
		{
			Finish("STOP", "ORIGINAL_CHANGED_OR_UNREADABLE", 4);
			return;
		}
		if (!childRead || childFilename != OWN_ACP)
		{
			Finish("STOP", "CHILD_FILENAME_NOT_OWN_ACP", 4);
			return;
		}
		Finish("OBSERVED", "INHERITED_SOURCE_SELECTION_ONLY", 0);
	}

	protected void InspectExistingDeferred()
	{
		Sleep(2000);
		Print("[RICON_VONINHERIT] phase=EXISTING_BEGIN guidOnlySyntax=HYPOTHESIS stockPositiveControlRequired=1");
		Resource originalHolder = Resource.Load(ORIGINAL_GUID_ONLY);
		if (!originalHolder || !originalHolder.IsValid() || !originalHolder.GetResource())
		{
			Finish("STOP_CAPABILITY", "STOCK_GUID_ONLY_LOAD_FAILED_OWN_NOT_TESTED", 6);
			return;
		}
		PrintFormat("[RICON_VONINHERIT] stage=STOCK_GUID_ONLY requested=%1 loadedResourceName=%2", ORIGINAL_GUID_ONLY, originalHolder.GetResource().GetResourceName());
		BaseContainer originalVoN = SelectVoN(originalHolder, "STOCK_GUID_ONLY", true);
		if (!originalVoN)
			return;
		ResourceName originalBefore;
		if (!originalVoN.Get("Filename", originalBefore) || originalBefore != STOCK_ACP)
		{
			Finish("STOP_CAPABILITY", "STOCK_GUID_ONLY_FILENAME_CONTROL_FAILED_OWN_NOT_TESTED", 6);
			return;
		}
		PrintFormat("[RICON_VONINHERIT] phase=STOCK_GUID_ONLY_CONTROL_OBSERVED originalFilename=%1", originalBefore);
		Resource childHolder = Resource.Load(OWN_GUID_ONLY);
		BaseContainer childVoN = SelectVoN(childHolder, "OWN_GUID_ONLY");
		if (!childVoN)
			return;
		PrintFormat("[RICON_VONINHERIT] stage=OWN_GUID_ONLY requested=%1 loadedResourceName=%2", OWN_GUID_ONLY, childHolder.GetResource().GetResourceName());
		ResourceName childFilename;
		ResourceName originalAfter;
		bool childRead = childVoN.Get("Filename", childFilename);
		bool originalRead = originalVoN.Get("Filename", originalAfter);
		bool originalUnchanged = originalRead && originalAfter == originalBefore;
		PrintFormat("[RICON_VONINHERIT] phase=EXISTING_READBACK childRead=%1 childFilename=%2 originalRead=%3 originalFilename=%4 originalUnchanged=%5", childRead, childFilename, originalRead, originalAfter, originalUnchanged);
		if (!originalUnchanged)
		{
			Finish("STOP", "ORIGINAL_CHANGED_OR_UNREADABLE", 4);
			return;
		}
		if (!childRead || childFilename != OWN_ACP)
		{
			Finish("STOP", "EXISTING_CHILD_FILENAME_NOT_OWN_ACP", 4);
			return;
		}
		Finish("OBSERVED", "EXISTING_GUID_ONLY_SOURCE_SELECTION_ONLY", 0);
	}

	//! Installed ResavePlugin serialization, fixed to one owned resource.
	protected void ResaveOwnDeferred()
	{
		Sleep(2000);
		ResourceManager manager = Workbench.GetModule(ResourceManager);
		string absolute;
		if (!manager || !Workbench.GetAbsolutePath(OWN_PATH, absolute, true))
		{
			Finish("STOP", "RESAVE_NO_MANAGER_OR_OWN_PATH", 2);
			return;
		}
		absolute.Replace("\\", "/");
		if (absolute != OWN_ABSOLUTE)
		{
			Finish("STOP", "RESAVE_OWN_ABSOLUTE_PATH_MISMATCH", 2);
			return;
		}
		MetaFile beforeMeta = manager.GetMetaFile(absolute);
		if (!beforeMeta || beforeMeta.GetResourceID() != OWN_FULL)
		{
			Finish("STOP", "RESAVE_META_ID_MISMATCH", 3);
			return;
		}
		Resource originalHolder = Resource.Load(ORIGINAL_GUID_ONLY);
		BaseContainer originalVoN = SelectVoN(originalHolder, "RESAVE_STOCK_BEFORE", true);
		if (!originalVoN)
			return;
		ResourceName originalBefore;
		if (!originalVoN.Get("Filename", originalBefore) || originalBefore != STOCK_ACP)
		{
			Finish("STOP_CAPABILITY", "RESAVE_STOCK_FILENAME_CONTROL_FAILED", 6);
			return;
		}
		Resource childHolder = BaseContainerTools.LoadContainer(OWN_FULL);
		if (!childHolder || !childHolder.IsValid() || !childHolder.GetResource() || childHolder.GetResource().GetResourceName() != OWN_FULL)
		{
			Finish("STOP", "RESAVE_OWN_RESOURCE_NAME_OR_LOAD_FAILED", 3);
			return;
		}
		BaseContainer childVoN = SelectVoN(childHolder, "RESAVE_OWN_BEFORE");
		if (!childVoN)
			return;
		ResourceName childBefore;
		if (!childVoN.Get("Filename", childBefore) || childBefore != OWN_ACP)
		{
			Finish("STOP", "RESAVE_CHILD_FILENAME_NOT_OWN_ACP", 4);
			return;
		}
		BaseContainer cont = childHolder.GetResource().ToBaseContainer();
		if (!cont)
		{
			Finish("STOP", "RESAVE_OWN_CONTAINER_NULL", 3);
			return;
		}
		bool saved = BaseContainerTools.SaveContainer(cont, OWN_FULL);
		PrintFormat("[RICON_VONINHERIT] phase=NATIVE_SAVE_RETURN resource=%1 saved=%2 scope=FILE_DIFF_AND_REPLICATION_UNQUALIFIED", OWN_FULL, saved);
		if (!saved)
		{
			Finish("STOP", "RESAVE_NATIVE_SAVE_RETURNED_FALSE", 7);
			return;
		}
		MetaFile afterMeta = manager.GetMetaFile(absolute);
		Resource originalAfterHolder = Resource.Load(ORIGINAL_GUID_ONLY);
		BaseContainer originalAfterVoN = SelectVoN(originalAfterHolder, "RESAVE_STOCK_AFTER", true);
		if (!originalAfterVoN)
			return;
		ResourceName originalAfter;
		ResourceName childAfter;
		if (!afterMeta || afterMeta.GetResourceID() != OWN_FULL || !originalAfterVoN.Get("Filename", originalAfter) || originalAfter != STOCK_ACP || !childVoN.Get("Filename", childAfter) || childAfter != OWN_ACP)
		{
			Finish("STOP", "RESAVE_POST_SAVE_ID_OR_FILENAME_MISMATCH", 4);
			return;
		}
		Finish("NATIVE_SAVE_RETURN_ONLY", "FILE_DIFF_AND_REPLICATION_UNQUALIFIED", 0);
	}

	override void RunCommandline()
	{
		string mode;
		if (!System.GetCLIParam("riconVoNInheritedPrefabMode", mode) || (mode != "compile-only" && mode != "prefab-register-inspect" && mode != "prefab-inspect-existing" && mode != "prefab-resave-own"))
		{
			Finish("STOP", "REFUSED_MODE", 5);
			return;
		}
		if (mode == "compile-only")
		{
			Finish("COMPILE_ONLY_REACHED", "RESOURCE_OPERATIONS_NOT_REQUESTED", 0);
			return;
		}
		if (mode == "prefab-resave-own")
		{
			Print("[RICON_VONINHERIT] phase=DEFERRED_SCHEDULED mode=prefab-resave-own delayMs=2000");
			thread ResaveOwnDeferred();
			return;
		}
		if (mode == "prefab-inspect-existing")
		{
			Print("[RICON_VONINHERIT] phase=DEFERRED_SCHEDULED mode=prefab-inspect-existing delayMs=2000");
			thread InspectExistingDeferred();
			return;
		}
		Print("[RICON_VONINHERIT] phase=DEFERRED_SCHEDULED mode=prefab-register-inspect delayMs=2000");
		thread InspectDeferred();
	}
}
