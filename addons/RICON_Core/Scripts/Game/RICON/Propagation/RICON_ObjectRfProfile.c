//! P3.1: определение RF-профиля объекта (REQ-object-rf-profile v0.1, корпус заморожен по LF SHA b0b93be3…e4c5).
//! Чистый resolver: явное назначение компонента или поверхности, затем проверенные правила prefab и класса от
//! конкретного узла к корню, затем подсказка. Не распознаёт объекты движка, не считает потерь, не задаёт слышимость.
//! Имя префаба, текстура и BallisticInfo сами не определяют радиосвойства. UNKNOWN — не воздух и не LOST.
enum RICON_EObjectProfileCode
{
	INVALID_INPUT = 0,
	UNRESOLVED_PROFILE = 1,
	UNCLASSIFIED = 2,
	RESOLVED = 3
}

//! Основание выбора (OP05).
enum RICON_EObjectProfileSource
{
	NONE = 0,
	COMPONENT = 1,
	MATERIAL = 2,
	PREFAB = 3,
	CLASS = 4,
	HINT = 5
}

//! Узел леса наследования: корень — m_bHasParent = false; пустой parentId при m_bHasParent = true — ошибка (R45).
class RICON_ProfileGraphNode
{
	string m_sId;
	bool m_bHasParent;
	string m_sParentId;
}

class RICON_ProfileRule
{
	string m_sNodeId;
	string m_sProfileId;
}

//! OP03: пустая строка — отсутствие сведений.
class RICON_ObjectProfileQuery
{
	string m_sComponentProfileId;
	string m_sSurfaceProfileId;
	string m_sPrefabId;
	string m_sClassId;
	string m_sBallisticHint;
}

class RICON_ObjectProfileInput
{
	ref RICON_ObjectProfileQuery m_Object;
	ref array<string> m_aProfiles;
	ref array<ref RICON_ProfileGraphNode> m_aPrefabParents;
	ref array<ref RICON_ProfileGraphNode> m_aClassParents;
	ref array<ref RICON_ProfileRule> m_aPrefabRules;
	ref array<ref RICON_ProfileRule> m_aClassRules;
}

//! При INVALID_INPUT остальные поля по умолчанию (UNKNOWN, "", NONE, "").
class RICON_ObjectProfileResult
{
	RICON_EObjectProfileCode m_eCode = RICON_EObjectProfileCode.INVALID_INPUT;
	RICON_EPathKnowledge m_eKnowledge = RICON_EPathKnowledge.UNKNOWN;
	string m_sMaterialId;
	RICON_EObjectProfileSource m_eSource = RICON_EObjectProfileSource.NONE;
	string m_sMatchedKey;
}

class RICON_ObjectRfProfileResolver
{
	protected static bool KeyOrEmpty(string s)
	{
		return s.IsEmpty() || RICON_PathObservationModel.ValidKey(s);
	}

	//! OP02: узлы с допустимыми ID без дублей; родитель существует, не равен узлу; циклов нет во всём графе.
	//! Проверка итеративная (состояния 1 — на текущем пути, 2 — проверен), без рекурсии по глубине.
	protected static bool BuildGraph(array<ref RICON_ProfileGraphNode> nodes, notnull map<string, string> parents)
	{
		if (!nodes)
			return false;
		map<string, bool> rooted = new map<string, bool>();
		foreach (RICON_ProfileGraphNode node : nodes)
		{
			if (!node || !RICON_PathObservationModel.ValidKey(node.m_sId) || parents.Contains(node.m_sId))
				return false;
			if (node.m_bHasParent && (!RICON_PathObservationModel.ValidKey(node.m_sParentId) || node.m_sParentId == node.m_sId))
				return false;
			parents.Set(node.m_sId, node.m_sParentId);
			rooted.Set(node.m_sId, !node.m_bHasParent);
		}
		foreach (RICON_ProfileGraphNode checkNode : nodes)
		{
			if (checkNode.m_bHasParent && !parents.Contains(checkNode.m_sParentId))
				return false;
		}
		map<string, int> state = new map<string, int>();
		foreach (RICON_ProfileGraphNode start : nodes)
		{
			array<string> path = {};
			string current = start.m_sId;
			bool walking = true;
			while (walking)
			{
				int seen = 0;
				if (state.Contains(current))
					seen = state.Get(current);
				if (seen == 2)
					break;
				if (seen == 1)
					return false;
				state.Set(current, 1);
				path.Insert(current);
				if (rooted.Get(current))
					walking = false;
				else
					current = parents.Get(current);
			}
			foreach (string done : path)
				state.Set(done, 2);
		}
		foreach (RICON_ProfileGraphNode rootNode : nodes)
		{
			if (!rootNode.m_bHasParent)
				parents.Set(rootNode.m_sId, "");
		}
		return true;
	}

	//! Правила: узел есть в графе, профиль есть в реестре, узел встречается один раз.
	protected static bool BuildRules(array<ref RICON_ProfileRule> rules, map<string, string> parents, map<string, bool> profiles,
		notnull map<string, string> out1)
	{
		if (!rules)
			return false;
		foreach (RICON_ProfileRule rule : rules)
		{
			if (!rule || !RICON_PathObservationModel.ValidKey(rule.m_sNodeId) || !parents.Contains(rule.m_sNodeId))
				return false;
			if (!RICON_PathObservationModel.ValidKey(rule.m_sProfileId) || !profiles.Contains(rule.m_sProfileId))
				return false;
			if (out1.Contains(rule.m_sNodeId))
				return false;
			out1.Set(rule.m_sNodeId, rule.m_sProfileId);
		}
		return true;
	}

	//! От узла к корню; первое правило. Корень помечен пустым родителем после BuildGraph.
	protected static string Walk(string start, map<string, string> parents, map<string, string> rules)
	{
		string current = start;
		while (!current.IsEmpty())
		{
			if (rules.Contains(current))
				return current;
			current = parents.Get(current);
		}
		return "";
	}

	protected static RICON_ObjectProfileResult Make(RICON_EObjectProfileCode code, RICON_EPathKnowledge knowledge,
		string materialId, RICON_EObjectProfileSource source, string matchedKey)
	{
		RICON_ObjectProfileResult r = new RICON_ObjectProfileResult();
		r.m_eCode = code;
		r.m_eKnowledge = knowledge;
		r.m_sMaterialId = materialId;
		r.m_eSource = source;
		r.m_sMatchedKey = matchedKey;
		return r;
	}

	static RICON_ObjectProfileResult Resolve(RICON_ObjectProfileInput input)
	{
		RICON_ObjectProfileResult invalid = new RICON_ObjectProfileResult();
		if (!input || !input.m_Object || !input.m_aProfiles)
			return invalid;
		map<string, bool> profiles = new map<string, bool>();
		foreach (string profile : input.m_aProfiles)
		{
			if (!RICON_PathObservationModel.ValidKey(profile) || profiles.Contains(profile))
				return invalid;
			profiles.Set(profile, true);
		}
		map<string, string> prefabParents = new map<string, string>();
		map<string, string> classParents = new map<string, string>();
		if (!BuildGraph(input.m_aPrefabParents, prefabParents) || !BuildGraph(input.m_aClassParents, classParents))
			return invalid;
		map<string, string> prefabRules = new map<string, string>();
		map<string, string> classRules = new map<string, string>();
		if (!BuildRules(input.m_aPrefabRules, prefabParents, profiles, prefabRules)
			|| !BuildRules(input.m_aClassRules, classParents, profiles, classRules))
			return invalid;
		RICON_ObjectProfileQuery o = input.m_Object;
		if (!KeyOrEmpty(o.m_sComponentProfileId) || !KeyOrEmpty(o.m_sSurfaceProfileId) || !KeyOrEmpty(o.m_sPrefabId)
			|| !KeyOrEmpty(o.m_sClassId) || !KeyOrEmpty(o.m_sBallisticHint))
			return invalid;
		if ((!o.m_sPrefabId.IsEmpty() && !prefabParents.Contains(o.m_sPrefabId))
			|| (!o.m_sClassId.IsEmpty() && !classParents.Contains(o.m_sClassId)))
			return invalid;
		// OP04: явные назначения, затем правила, затем подсказка; неизвестное выбранное назначение без fallback.
		if (!o.m_sComponentProfileId.IsEmpty())
		{
			if (profiles.Contains(o.m_sComponentProfileId))
				return Make(RICON_EObjectProfileCode.RESOLVED, RICON_EPathKnowledge.PROFILE, o.m_sComponentProfileId,
					RICON_EObjectProfileSource.COMPONENT, o.m_sComponentProfileId);
			return Make(RICON_EObjectProfileCode.UNRESOLVED_PROFILE, RICON_EPathKnowledge.UNKNOWN, "",
				RICON_EObjectProfileSource.COMPONENT, o.m_sComponentProfileId);
		}
		if (!o.m_sSurfaceProfileId.IsEmpty())
		{
			if (profiles.Contains(o.m_sSurfaceProfileId))
				return Make(RICON_EObjectProfileCode.RESOLVED, RICON_EPathKnowledge.PROFILE, o.m_sSurfaceProfileId,
					RICON_EObjectProfileSource.MATERIAL, o.m_sSurfaceProfileId);
			return Make(RICON_EObjectProfileCode.UNRESOLVED_PROFILE, RICON_EPathKnowledge.UNKNOWN, "",
				RICON_EObjectProfileSource.MATERIAL, o.m_sSurfaceProfileId);
		}
		if (!o.m_sPrefabId.IsEmpty())
		{
			string prefabHit = Walk(o.m_sPrefabId, prefabParents, prefabRules);
			if (!prefabHit.IsEmpty())
				return Make(RICON_EObjectProfileCode.RESOLVED, RICON_EPathKnowledge.RULE, prefabRules.Get(prefabHit),
					RICON_EObjectProfileSource.PREFAB, prefabHit);
		}
		if (!o.m_sClassId.IsEmpty())
		{
			string classHit = Walk(o.m_sClassId, classParents, classRules);
			if (!classHit.IsEmpty())
				return Make(RICON_EObjectProfileCode.RESOLVED, RICON_EPathKnowledge.RULE, classRules.Get(classHit),
					RICON_EObjectProfileSource.CLASS, classHit);
		}
		if (!o.m_sBallisticHint.IsEmpty())
			return Make(RICON_EObjectProfileCode.UNCLASSIFIED, RICON_EPathKnowledge.HINT_ONLY, "",
				RICON_EObjectProfileSource.HINT, o.m_sBallisticHint);
		return Make(RICON_EObjectProfileCode.UNCLASSIFIED, RICON_EPathKnowledge.UNKNOWN, "", RICON_EObjectProfileSource.NONE, "");
	}
}
