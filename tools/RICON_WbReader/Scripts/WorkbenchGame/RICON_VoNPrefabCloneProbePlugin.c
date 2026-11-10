//! TOOL_ONLY: detached source-container probe; no save, spawn or audio.
[WorkbenchPluginAttribute(name: "RICON: VoN prefab clone probe", wbModules: {"ScriptEditor"})]
class RICON_VoNPrefabCloneProbePlugin : WorkbenchPlugin
{
	static const ResourceName PREFAB = "{26A9756790131354}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Rifleman.et";
	static const ResourceName STOCK_ACP = "{B4C3941EC8B2AD20}Sounds/VON/von.acp";
	static const ResourceName OWN_ACP = "{63C2132413FE4337}Sounds/RICON_VoNProbe/RICON_VoNProbe.acp";
	static const int MAX_COMPONENTS = 128;

	protected void Finish(string result, string reason, int code)
	{
		PrintFormat("[RICON_VONCLONE] terminal=1 result=%1 reason=%2 code=%3 scope=SOURCE_CONTAINER nativeVoiceQualified=0", result, reason, code);
		Workbench.Exit(code);
	}

	protected BaseContainer SelectVoN(IEntitySource entity, array<BaseContainer> containers, string stage)
	{
		int count = entity.GetComponentCount();
		if (count <= 0 || count > MAX_COMPONENTS)
		{
			PrintFormat("[RICON_VONCLONE] stage=%1 componentCount=%2", stage, count);
			Finish("STOP", "COMPONENT_COUNT", 2);
			return null;
		}
		containers.Insert(entity);
		BaseContainer selected;
		int matches = 0;
		for (int index = 0; index < count; index++)
		{
			IEntityComponentSource component = entity.GetComponent(index);
			if (!component || containers.Find(component) >= 0)
			{
				PrintFormat("[RICON_VONCLONE] stage=%1 componentIndex=%2", stage, index);
				Finish("STOP", "NULL_OR_DUPLICATE_COMPONENT", 2);
				return null;
			}
			containers.Insert(component);
			if (component.GetClassName() == "SCR_VoNComponent")
			{
				selected = component;
				matches++;
			}
		}
		PrintFormat("[RICON_VONCLONE] stage=%1 components=%2 vonMatches=%3", stage, count, matches);
		if (matches != 1 || !selected)
		{
			Finish("STOP", "VON_COMPONENT_NOT_UNIQUE", 2);
			return null;
		}
		return selected;
	}

	protected void InspectCloneDeferred()
	{
		Sleep(2000);
		Print("[RICON_VONCLONE] phase=DEFERRED_INSPECT_BEGIN mode=clone-inspect");
		Resource originalHolder = Resource.Load(PREFAB);
		if (!originalHolder || !originalHolder.IsValid() || !originalHolder.GetResource())
		{
			Finish("STOP", "ORIGINAL_RESOURCE_INVALID", 2);
			return;
		}
		BaseResourceObject originalObject = originalHolder.GetResource();
		IEntitySource originalSource = originalObject.ToEntitySource();
		if (!originalSource)
		{
			Finish("STOP", "ORIGINAL_NOT_ENTITY_SOURCE", 2);
			return;
		}
		array<BaseContainer> originalContainers = {};
		BaseContainer originalVoN = SelectVoN(originalSource, originalContainers, "original");
		if (!originalVoN)
			return;
		ResourceName originalBefore;
		if (!originalVoN.Get("Filename", originalBefore) || originalBefore != STOCK_ACP)
		{
			Finish("STOP", "ORIGINAL_FILENAME_NOT_EXPECTED_STOCK", 2);
			return;
		}

		Resource cloneHolder = BaseContainerTools.CloneContainer(originalSource);
		if (!cloneHolder || !cloneHolder.IsValid() || !cloneHolder.GetResource())
		{
			Finish("STOP", "CLONE_RESOURCE_INVALID", 3);
			return;
		}
		BaseResourceObject cloneObject = cloneHolder.GetResource();
		IEntitySource cloneSource = cloneObject.ToEntitySource();
		if (!cloneSource)
		{
			Finish("STOP", "CLONE_NOT_ENTITY_SOURCE", 3);
			return;
		}
		if (cloneHolder == originalHolder || cloneObject == originalObject || originalContainers.Find(cloneSource) >= 0)
		{
			Finish("STOP", "SHARED_CLONE_ROOT", 3);
			return;
		}
		if (cloneSource.GetClassName() != originalSource.GetClassName())
		{
			Finish("STOP", "CLONE_ENTITY_CLASS_CHANGED", 3);
			return;
		}
		array<BaseContainer> cloneContainers = {};
		BaseContainer cloneVoN = SelectVoN(cloneSource, cloneContainers, "clone");
		if (!cloneVoN)
			return;
		if (cloneContainers.Count() != originalContainers.Count())
		{
			Finish("STOP", "CLONE_COMPONENT_COUNT_CHANGED", 3);
			return;
		}
		foreach (BaseContainer container : cloneContainers)
		{
			if (originalContainers.Find(container) >= 0)
			{
				Finish("STOP", "SHARED_COMPONENT_CONTAINER", 3);
				return;
			}
		}
		ResourceName cloneBefore;
		if (!cloneVoN.Get("Filename", cloneBefore) || cloneBefore != originalBefore)
		{
			Finish("STOP", "CLONE_FILENAME_BEFORE_MISMATCH", 3);
			return;
		}

		Print("[RICON_VONCLONE] phase=SET_BEGIN rootsDistinct=1 componentContainersDistinct=1 field=Filename detachedIdentityGuards=1");
		bool setReturned = cloneVoN.Set("Filename", OWN_ACP);
		ResourceName cloneAfter;
		ResourceName originalAfter;
		bool cloneRead = cloneVoN.Get("Filename", cloneAfter);
		bool originalRead = originalVoN.Get("Filename", originalAfter);
		bool originalUnchanged = originalRead && originalAfter == originalBefore;
		PrintFormat("[RICON_VONCLONE] phase=READBACK setReturned=%1 cloneRead=%2 cloneOwn=%3 originalRead=%4 originalUnchanged=%5", setReturned, cloneRead, cloneAfter == OWN_ACP, originalRead, originalUnchanged);
		if (!originalUnchanged)
		{
			Finish("STOP", "ORIGINAL_CHANGED_OR_UNREADABLE", 4);
			return;
		}
		if (!setReturned || !cloneRead || cloneAfter != OWN_ACP)
		{
			Finish("STOP", "CLONE_FILENAME_WRITE_OR_READBACK_FAILED", 4);
			return;
		}
		Finish("OBSERVED", "DETACHED_CLONE_FILENAME_READBACK_ONLY", 0);
	}

	override void RunCommandline()
	{
		string mode;
		if (!System.GetCLIParam("riconVoNPrefabMode", mode) || (mode != "compile-only" && mode != "clone-inspect"))
		{
			Finish("STOP", "REFUSED_MODE", 5);
			return;
		}
		if (mode == "compile-only")
		{
			Finish("COMPILE_ONLY_REACHED", "RESOURCE_OPERATIONS_NOT_REQUESTED", 0);
			return;
		}
		Print("[RICON_VONCLONE] phase=DEFERRED_SCHEDULED mode=clone-inspect delayMs=2000");
		thread InspectCloneDeferred();
	}
}
