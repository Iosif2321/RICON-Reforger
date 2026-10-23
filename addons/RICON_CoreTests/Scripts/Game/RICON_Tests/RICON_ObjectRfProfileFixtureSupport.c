//! TEST_ONLY: корпус P3.1, неизменность входа, контроль сравнения, порядок записей и длинная цепь наследования.
class RICON_ObjectRfProfileRun
{
	int m_iCases;
	int m_iMatches;
	int m_iImmutable;
	int m_iControls;
	int m_iExtras;
	bool m_bOk = true;
	protected ref array<ref RICON_ObjectProfileInput> m_aInputs = {};
	protected ref array<ref RICON_ObjectProfileResult> m_aExpected = {};
	protected ref RICON_ObjectProfileInput m_Base;

	protected void Require(bool condition, string id, string what)
	{
		m_bOk = RICON_TestSupport.Expect(condition, id, what) && m_bOk;
	}

	protected void Reject(bool accepted)
	{
		Require(!accepted, "object profile controls", "искажённое ожидание отвергнуто");
		if (!accepted)
			m_iControls++;
	}

	RICON_ObjectProfileInput NewInput()
	{
		RICON_ObjectProfileInput input = new RICON_ObjectProfileInput();
		input.m_Object = new RICON_ObjectProfileQuery();
		input.m_aProfiles = {};
		input.m_aPrefabParents = {};
		input.m_aClassParents = {};
		input.m_aPrefabRules = {};
		input.m_aClassRules = {};
		return input;
	}

	void AddNode(array<ref RICON_ProfileGraphNode> nodes, string id, bool hasParent, string parentId)
	{
		RICON_ProfileGraphNode node = new RICON_ProfileGraphNode();
		node.m_sId = id;
		node.m_bHasParent = hasParent;
		node.m_sParentId = parentId;
		nodes.Insert(node);
	}

	void AddRule(array<ref RICON_ProfileRule> rules, string nodeId, string profileId)
	{
		RICON_ProfileRule rule = new RICON_ProfileRule();
		rule.m_sNodeId = nodeId;
		rule.m_sProfileId = profileId;
		rules.Insert(rule);
	}

	static bool SameResult(RICON_ObjectProfileResult a, RICON_ObjectProfileResult b)
	{
		return a && b && a.m_eCode == b.m_eCode && a.m_eKnowledge == b.m_eKnowledge && a.m_sMaterialId == b.m_sMaterialId
			&& a.m_eSource == b.m_eSource && a.m_sMatchedKey == b.m_sMatchedKey;
	}

	protected static string Nodes(array<ref RICON_ProfileGraphNode> nodes)
	{
		if (!nodes)
			return "null";
		string text = nodes.Count().ToString();
		foreach (RICON_ProfileGraphNode n : nodes)
		{
			if (!n)
				text += "|null";
			else
				text += "|" + n.m_sId + ">" + n.m_bHasParent.ToString() + ":" + n.m_sParentId;
		}
		return text;
	}

	protected static string Rules(array<ref RICON_ProfileRule> rules)
	{
		if (!rules)
			return "null";
		string text = rules.Count().ToString();
		foreach (RICON_ProfileRule r : rules)
		{
			if (!r)
				text += "|null";
			else
				text += "|" + r.m_sNodeId + "=" + r.m_sProfileId;
		}
		return text;
	}

	static string Capture(RICON_ObjectProfileInput input)
	{
		if (!input)
			return "null";
		string text = "null";
		RICON_ObjectProfileQuery o = input.m_Object;
		if (o)
			text = o.m_sComponentProfileId + "|" + o.m_sSurfaceProfileId + "|" + o.m_sPrefabId + "|" + o.m_sClassId + "|" + o.m_sBallisticHint;
		text += "#";
		if (!input.m_aProfiles)
			text += "null";
		else
		{
			foreach (string p : input.m_aProfiles)
				text += p + ",";
		}
		text += "#" + Nodes(input.m_aPrefabParents) + "#" + Nodes(input.m_aClassParents);
		text += "#" + Rules(input.m_aPrefabRules) + "#" + Rules(input.m_aClassRules);
		return text;
	}

	protected void Controls(RICON_ObjectProfileResult actual, RICON_ObjectProfileResult expected)
	{
		Reject(SameResult(null, expected));
		actual.m_eCode = RICON_EObjectProfileCode.UNCLASSIFIED;
		Reject(SameResult(actual, expected));
		actual.m_eCode = expected.m_eCode;
		actual.m_eKnowledge = RICON_EPathKnowledge.RULE;
		Reject(SameResult(actual, expected));
		actual.m_eKnowledge = expected.m_eKnowledge;
		actual.m_sMaterialId = "rf.other";
		Reject(SameResult(actual, expected));
		actual.m_sMaterialId = expected.m_sMaterialId;
		actual.m_eSource = RICON_EObjectProfileSource.PREFAB;
		Reject(SameResult(actual, expected));
		actual.m_eSource = expected.m_eSource;
		actual.m_sMatchedKey = "other.key";
		Reject(SameResult(actual, expected));
		actual.m_sMatchedKey = expected.m_sMatchedKey;
	}

	void CheckCase(string id, RICON_ObjectProfileInput input, RICON_ObjectProfileResult expected)
	{
		m_iCases++;
		string before = Capture(input);
		RICON_ObjectProfileResult first = RICON_ObjectRfProfileResolver.Resolve(input);
		bool matched = SameResult(first, expected);
		Require(matched, id, "код, knowledge, materialId, source и matchedKey совпадают с эталоном");
		if (matched)
			m_iMatches++;
		if (first)
			first.m_sMaterialId = "spoiled";
		RICON_ObjectProfileResult second = RICON_ObjectRfProfileResolver.Resolve(input);
		bool immutable = Capture(input) == before && second && second != first && SameResult(second, expected);
		Require(immutable, id, "вход не изменён, повторный результат новый");
		if (immutable)
			m_iImmutable++;
		if (id == "R01" && matched && immutable)
		{
			Controls(second, expected);
			Require(SameResult(second, expected), "object profile controls", "после контролей исходный результат восстановлен");
			m_Base = input;
		}
		if (input)
		{
			m_aInputs.Insert(input);
			m_aExpected.Insert(expected);
		}
		PrintFormat("[RICON_OBJPROFILE] %1 code=%2 knowledge=%3 material=%4 source=%5 matched=%6 match=%7 immutable=%8", id,
			typename.EnumToString(RICON_EObjectProfileCode, second.m_eCode), typename.EnumToString(RICON_EPathKnowledge, second.m_eKnowledge),
			second.m_sMaterialId, typename.EnumToString(RICON_EObjectProfileSource, second.m_eSource), second.m_sMatchedKey, matched, immutable);
	}

	protected static void ReverseNodes(array<ref RICON_ProfileGraphNode> nodes)
	{
		if (!nodes)
			return;
		int n = nodes.Count();
		for (int i = 0; i < n / 2; i++)
		{
			nodes.SwapItems(i, n - 1 - i);
		}
	}

	protected static void ReverseRules(array<ref RICON_ProfileRule> rules)
	{
		if (!rules)
			return;
		int n = rules.Count();
		for (int i = 0; i < n / 2; i++)
		{
			rules.SwapItems(i, n - 1 - i);
		}
	}

	protected static void ReverseProfiles(array<string> profiles)
	{
		if (!profiles)
			return;
		int n = profiles.Count();
		for (int i = 0; i < n / 2; i++)
		{
			profiles.SwapItems(i, n - 1 - i);
		}
	}

	//! OP04: порядок записей графов, правил и реестра не влияет на результат; входы восстанавливаются.
	void CheckOrder()
	{
		bool ok = !m_aInputs.IsEmpty();
		for (int k = 0; k < m_aInputs.Count(); k++)
		{
			RICON_ObjectProfileInput input = m_aInputs[k];
			string before = Capture(input);
			ReverseNodes(input.m_aPrefabParents);
			ReverseNodes(input.m_aClassParents);
			ReverseRules(input.m_aPrefabRules);
			ReverseRules(input.m_aClassRules);
			ReverseProfiles(input.m_aProfiles);
			ok = ok && SameResult(RICON_ObjectRfProfileResolver.Resolve(input), m_aExpected[k]);
			ReverseNodes(input.m_aPrefabParents);
			ReverseNodes(input.m_aClassParents);
			ReverseRules(input.m_aPrefabRules);
			ReverseRules(input.m_aClassRules);
			ReverseProfiles(input.m_aProfiles);
			ok = ok && Capture(input) == before;
		}
		Require(ok, "object profile order", "обратный порядок графов, правил и реестра не меняет ни одного результата");
		if (ok)
			m_iExtras++;
	}

	//! Цепь 5000 узлов разрешается итеративно; цикл и висячий родитель в неиспользуемой ветви отвергаются при
	//! известном профиле компонента (OP02: глобальная проверка).
	void CheckGraphs()
	{
		RICON_ObjectProfileInput deep = NewInput();
		deep.m_aProfiles.Insert("deep.profile");
		for (int i = 0; i < 5000; i++)
		{
			int next1 = i + 1;
			AddNode(deep.m_aPrefabParents, "n" + i.ToString(), i < 4999, "n" + next1.ToString());
		}
		AddRule(deep.m_aPrefabRules, "n4999", "deep.profile");
		deep.m_Object.m_sPrefabId = "n0";
		RICON_ObjectProfileResult r = RICON_ObjectRfProfileResolver.Resolve(deep);
		bool ok = r.m_eCode == RICON_EObjectProfileCode.RESOLVED && r.m_eSource == RICON_EObjectProfileSource.PREFAB
			&& r.m_sMatchedKey == "n4999" && r.m_sMaterialId == "deep.profile";
		ok = ok && m_Base != null;
		if (m_Base)
		{
			AddNode(m_Base.m_aPrefabParents, "zz.a", true, "zz.b");
			AddNode(m_Base.m_aPrefabParents, "zz.b", true, "zz.a");
			ok = ok && RICON_ObjectRfProfileResolver.Resolve(m_Base).m_eCode == RICON_EObjectProfileCode.INVALID_INPUT;
			m_Base.m_aPrefabParents.Remove(m_Base.m_aPrefabParents.Count() - 1);
			m_Base.m_aPrefabParents.Remove(m_Base.m_aPrefabParents.Count() - 1);
			AddNode(m_Base.m_aClassParents, "zz.c", true, "zz.missing");
			ok = ok && RICON_ObjectRfProfileResolver.Resolve(m_Base).m_eCode == RICON_EObjectProfileCode.INVALID_INPUT;
			m_Base.m_aClassParents.Remove(m_Base.m_aClassParents.Count() - 1);
			m_Base.m_aPrefabParents.Insert(null);
			ok = ok && RICON_ObjectRfProfileResolver.Resolve(m_Base).m_eCode == RICON_EObjectProfileCode.INVALID_INPUT;
			m_Base.m_aPrefabParents.Remove(m_Base.m_aPrefabParents.Count() - 1);
			ok = ok && RICON_ObjectRfProfileResolver.Resolve(m_Base).m_eCode == RICON_EObjectProfileCode.RESOLVED;
		}
		Require(ok, "object profile graphs", "цепь 5000 узлов, цикл/висячий родитель/null-узел в неиспользуемой ветви");
		if (ok)
			m_iExtras++;
	}
}
