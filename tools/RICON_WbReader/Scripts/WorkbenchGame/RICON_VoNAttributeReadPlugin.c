//! Один fixed prefab: только metadata и числовые Get контейнера VoN, без мира и экземпляра персонажа.
[WorkbenchPluginAttribute(name: "RICON: числовые атрибуты VoN", wbModules: {"ScriptEditor"})]
class RICON_VoNAttributeReadPlugin : WorkbenchPlugin
{
	static const ResourceName PREFAB = "{26A9756790131354}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Rifleman.et";
	static const int MAX_COMPONENTS = 128;
	static const int MAX_FIELDS = 64;
	static const int MAX_LEVELS = 8;
	protected int m_iNumericRead;
	protected int m_iFieldsSeen;

	protected void Finish(string result, string reason, int code)
	{
		PrintFormat("[RICON_VONATTR] terminal=1 result=%1 reason=%2 fields=%3 numericRead=%4 scope=SOURCE_CONTAINER speakingRange=UNQUALIFIED playInstance=NOT_TESTED", result, reason, m_iFieldsSeen, m_iNumericRead);
		Workbench.Exit(code);
	}

	protected bool ReadFields(BaseContainer node, int level)
	{
		int count = node.GetNumVars();
		if (count <= 0 || count > MAX_FIELDS)
		{
			PrintFormat("[RICON_VONATTR] issue=FIELD_COUNT level=%1 count=%2", level, count);
			return false;
		}
		string resourceName = node.GetResourceName();
		if (resourceName.IsEmpty())
		{
			resourceName = "UNKNOWN";
		}
		PrintFormat("[RICON_VONATTR] container=1 level=%1 class=%2 source=%3 fields=%4", level, node.GetClassName(), resourceName, count);
		array<string> names = {};
		for (int index = 0; index < count; index++)
		{
			string name = node.GetVarName(index);
			if (name.IsEmpty() || name.Length() > 128 || name.Contains("\n") || name.Contains("\r") || name.Contains("\"") || names.Find(name) >= 0)
			{
				PrintFormat("[RICON_VONATTR] issue=FIELD_NAME_OR_DUPLICATE level=%1 index=%2", level, index);
				return false;
			}
			names.Insert(name);
			DataVarType fieldType = node.GetDataVarType(index);
			string typeName = typename.EnumToString(DataVarType, fieldType);
			if (typeName.IsEmpty())
			{
				PrintFormat("[RICON_VONATTR] issue=UNKNOWN_TYPE level=%1 index=%2", level, index);
				return false;
			}
			m_iFieldsSeen++;
			bool isDirect = node.IsVariableSetDirectly(name);
			bool isSet = node.IsVariableSet(name);
			if (fieldType == DataVarType.SCALAR)
			{
				float scalarValue;
				bool scalarRead = node.Get(name, scalarValue);
				if (!scalarRead || !(scalarValue <= float.MAX && scalarValue >= -float.MAX))
				{
					PrintFormat("[RICON_VONATTR] field=1 level=%1 name=\"%2\" type=%3 readOk=0 numeric=UNKNOWN", level, name, typeName);
					return false;
				}
				m_iNumericRead++;
				PrintFormat("[RICON_VONATTR] field=1 level=%1 name=\"%2\" type=%3 direct=%4 set=%5 readOk=1 numeric=%6", level, name, typeName, isDirect, isSet, scalarValue.ToString(-1, 9));
			}
			else if (fieldType == DataVarType.INTEGER)
			{
				int integerValue;
				bool integerRead = node.Get(name, integerValue);
				if (!integerRead)
				{
					PrintFormat("[RICON_VONATTR] field=1 level=%1 name=\"%2\" type=%3 readOk=0 numeric=UNKNOWN", level, name, typeName);
					return false;
				}
				m_iNumericRead++;
				PrintFormat("[RICON_VONATTR] field=1 level=%1 name=\"%2\" type=%3 direct=%4 set=%5 readOk=1 numeric=%6", level, name, typeName, isDirect, isSet, integerValue);
			}
			else
			{
				// Значения строк, ресурсов, ключей и объектов намеренно не читаются.
				PrintFormat("[RICON_VONATTR] field=1 level=%1 name=\"%2\" type=%3 direct=%4 set=%5 value=NOT_READ", level, name, typeName, isDirect, isSet);
			}
		}
		return true;
	}

	override void RunCommandline()
	{
		m_iNumericRead = 0;
		m_iFieldsSeen = 0;
		PrintFormat("[RICON_VONATTR] start=1 prefab=%1 noWorld=1 noSpawn=1", PREFAB);
		Resource resource = Resource.Load(PREFAB);
		if (!resource || !resource.IsValid() || !resource.GetResource())
		{
			Finish("INCOMPLETE", "RESOURCE_UNAVAILABLE", 2);
			return;
		}
		IEntitySource source = resource.GetResource().ToEntitySource();
		if (!source)
		{
			Finish("INCOMPLETE", "NO_ENTITY_SOURCE", 2);
			return;
		}
		int componentCount = source.GetComponentCount();
		PrintFormat("[RICON_VONATTR] entitySource=1 class=%1 components=%2", source.GetClassName(), componentCount);
		if (componentCount <= 0 || componentCount > MAX_COMPONENTS)
		{
			Finish("INCOMPLETE", "COMPONENT_COUNT", 2);
			return;
		}
		IEntityComponentSource selected;
		int matches = 0;
		for (int componentIndex = 0; componentIndex < componentCount; componentIndex++)
		{
			IEntityComponentSource component = source.GetComponent(componentIndex);
			if (!component)
			{
				Finish("INCOMPLETE", "NULL_COMPONENT", 2);
				return;
			}
			if (component.GetClassName() == "SCR_VoNComponent")
			{
				selected = component;
				matches++;
			}
		}
		if (matches != 1 || !selected)
		{
			PrintFormat("[RICON_VONATTR] matches=%1", matches);
			Finish("INCOMPLETE", "VON_COMPONENT_NOT_UNIQUE", 2);
			return;
		}
		BaseContainer node = selected;
		array<BaseContainer> visited = {};
		for (int level = 0; level < MAX_LEVELS && node; level++)
		{
			if (visited.Find(node) >= 0)
			{
				Finish("INCOMPLETE", "ANCESTOR_CYCLE", 2);
				return;
			}
			visited.Insert(node);
			if (!ReadFields(node, level))
			{
				Finish("INCOMPLETE", "NUMERIC_FIELDS_UNRESOLVED", 2);
				return;
			}
			node = node.GetAncestor();
		}
		if (node || m_iNumericRead == 0)
		{
			Finish("INCOMPLETE", "ANCESTOR_CAP_OR_NO_NUMERIC_FIELDS", 2);
			return;
		}
		Finish("NUMERIC_FIELDS_OBSERVED", "ENUMERATION_COMPLETE", 0);
	}
}
