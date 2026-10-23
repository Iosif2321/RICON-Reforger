//! TEST_ONLY: корпус CT10.1, неизменность входа, отсутствие алиасов, контроль сравнения и проверки вне JSON.
class RICON_ChannelDescriptorRun
{
	int m_iCases;
	int m_iMatches;
	int m_iImmutable;
	int m_iControls;
	int m_iExtras;
	bool m_bOk = true;
	protected ref array<ref array<int>> m_aFrames = {};
	protected ref array<ref RICON_DescriptorResult> m_aFramed = {};
	protected ref RICON_FadingDescriptor m_Dynamic;

	protected void Require(bool condition, string id, string what)
	{
		m_bOk = RICON_TestSupport.Expect(condition, id, what) && m_bOk;
	}

	protected void Reject(bool accepted)
	{
		Require(!accepted, "descriptor controls", "искажённое ожидание отвергнуто");
		if (!accepted)
			m_iControls++;
	}

	protected static int Nibble(int c)
	{
		if (c >= 48 && c <= 57)
			return c - 48;
		return c - 87;
	}

	//! Байты из строчного hex генератора: длинный литерал массива упирается в лимит инструкций функции.
	array<int> Hex(string hex)
	{
		array<int> bytes = {};
		for (int i = 0; i + 1 < hex.Length(); i += 2)
		{
			bytes.Insert(Nibble(hex.ToAscii(i)) * 16 + Nibble(hex.ToAscii(i + 1)));
		}
		return bytes;
	}

	RICON_FadingEndpoint End(int deviceId, int transceiverIndex)
	{
		RICON_FadingEndpoint e = new RICON_FadingEndpoint();
		e.m_iDeviceId = deviceId;
		e.m_iTransceiverIndex = transceiverIndex;
		return e;
	}

	void Add(RICON_FadingDescriptor d, string id, float weight, int phaseQ, int dopplerQ)
	{
		RICON_FadingComponent c = new RICON_FadingComponent();
		c.m_sComponentId = id;
		c.m_fPowerWeight = weight;
		c.m_iPhaseQ = phaseQ;
		c.m_iDopplerQ = dopplerQ;
		d.m_aComponents.Insert(c);
	}

	protected static bool SameEnd(RICON_FadingEndpoint a, RICON_FadingEndpoint b)
	{
		return a && b && a.m_iDeviceId == b.m_iDeviceId && a.m_iTransceiverIndex == b.m_iTransceiverIndex;
	}

	//! Все поля строго; binary32 — равенством и равным битовым образом (различает +0 и прочие нули через кадр).
	static bool SameDescriptor(RICON_FadingDescriptor a, RICON_FadingDescriptor b)
	{
		if (!a || !b || a.m_sModelId != b.m_sModelId || a.m_sProfileId != b.m_sProfileId || a.m_iProfileVersion != b.m_iProfileVersion)
			return false;
		if (!SameEnd(a.m_A, b.m_A) || !SameEnd(a.m_B, b.m_B) || a.m_iEnvironmentRevision != b.m_iEnvironmentRevision)
			return false;
		if (a.m_fFrequencyMHz != b.m_fFrequencyMHz || a.m_fBandwidthHz != b.m_fBandwidthHz)
			return false;
		if (a.m_iAnchorMs != b.m_iAnchorMs || a.m_iValidFromMs != b.m_iValidFromMs || a.m_iValidUntilMs != b.m_iValidUntilMs)
			return false;
		if (a.m_bStaticChannel != b.m_bStaticChannel || a.m_bCoverageComplete != b.m_bCoverageComplete)
			return false;
		if (a.m_bResourceExhausted != b.m_bResourceExhausted || a.m_aComponents.Count() != b.m_aComponents.Count())
			return false;
		for (int i = 0; i < a.m_aComponents.Count(); i++)
		{
			RICON_FadingComponent x = a.m_aComponents[i];
			RICON_FadingComponent y = b.m_aComponents[i];
			if (!x || !y || x.m_sComponentId != y.m_sComponentId || x.m_fPowerWeight != y.m_fPowerWeight)
				return false;
			if (x.m_iPhaseQ != y.m_iPhaseQ || x.m_iDopplerQ != y.m_iDopplerQ)
				return false;
		}
		return true;
	}

	static bool SameBytes(array<int> a, array<int> b)
	{
		if (!a || !b || a.Count() != b.Count())
			return false;
		for (int i = 0; i < a.Count(); i++)
		{
			if (a[i] != b[i])
				return false;
		}
		return true;
	}

	static bool SameResult(RICON_DescriptorResult a, RICON_DescriptorResult b)
	{
		if (!a || !b || a.m_eCode != b.m_eCode)
			return false;
		if (a.m_eCode != RICON_EDescriptorCode.OK)
			return !a.m_Descriptor && !a.m_aBytes && a.m_iSizeBytes == 0;
		if (!SameDescriptor(a.m_Descriptor, b.m_Descriptor))
			return false;
		if (!b.m_aBytes)
			return !a.m_aBytes && a.m_iSizeBytes == 0 && b.m_iSizeBytes == 0;
		return SameBytes(a.m_aBytes, b.m_aBytes) && a.m_iSizeBytes == b.m_iSizeBytes;
	}

	//! Результат не делит изменяемые данные с входом: новый descriptor, концы, массив и составляющие.
	protected static bool Detached(RICON_FadingDescriptor input, RICON_DescriptorResult r)
	{
		if (!r || r.m_eCode != RICON_EDescriptorCode.OK || !input)
			return true;
		RICON_FadingDescriptor out1 = r.m_Descriptor;
		if (out1 == input || out1.m_A == input.m_A || out1.m_B == input.m_B || out1.m_aComponents == input.m_aComponents)
			return false;
		foreach (RICON_FadingComponent c : out1.m_aComponents)
		{
			if (input.m_aComponents.Find(c) >= 0)
				return false;
		}
		return true;
	}

	protected static string Capture(RICON_FadingDescriptor d, notnull array<float> floats)
	{
		if (!d)
			return "null";
		string text = d.m_sModelId + "|" + d.m_sProfileId + "|" + d.m_iProfileVersion.ToString();
		text += "|" + d.m_A.m_iDeviceId.ToString() + "/" + d.m_A.m_iTransceiverIndex.ToString();
		text += "|" + d.m_B.m_iDeviceId.ToString() + "/" + d.m_B.m_iTransceiverIndex.ToString();
		text += "|" + d.m_iEnvironmentRevision.ToString() + "|" + d.m_iAnchorMs.ToString() + "|" + d.m_iValidFromMs.ToString();
		text += "|" + d.m_iValidUntilMs.ToString() + "|" + d.m_bStaticChannel.ToString() + "|" + d.m_bCoverageComplete.ToString();
		text += "|" + d.m_bResourceExhausted.ToString() + "|" + d.m_aComponents.Count().ToString();
		floats.Insert(d.m_fFrequencyMHz);
		floats.Insert(d.m_fBandwidthHz);
		foreach (RICON_FadingComponent c : d.m_aComponents)
		{
			text += "|" + c.m_sComponentId + "|" + c.m_iPhaseQ.ToString() + "|" + c.m_iDopplerQ.ToString();
			floats.Insert(c.m_fPowerWeight);
		}
		return text;
	}

	protected static string CaptureBytes(array<int> bytes)
	{
		if (!bytes)
			return "null";
		string text = bytes.Count().ToString();
		foreach (int b : bytes)
			text += "," + b.ToString();
		return text;
	}

	//! Порча результата первого вызова не должна влиять на второй; второй — новые объекты.
	protected static void Spoil(RICON_DescriptorResult r)
	{
		if (!r)
			return;
		r.m_iSizeBytes = -7;
		if (r.m_aBytes)
			r.m_aBytes.Insert(999);
		if (r.m_Descriptor)
		{
			r.m_Descriptor.m_iAnchorMs = -5;
			r.m_Descriptor.m_A.m_iDeviceId = -5;
			if (!r.m_Descriptor.m_aComponents.IsEmpty())
				r.m_Descriptor.m_aComponents[0].m_iPhaseQ = -5;
		}
	}

	protected void Finish(string id, string op, RICON_FadingDescriptor input, array<int> inputBytes, bool matched,
		RICON_DescriptorResult first, RICON_DescriptorResult second, RICON_DescriptorResult expected, string before,
		array<float> beforeFloats, string beforeBytes)
	{
		m_iCases++;
		Require(matched, id, "код, descriptor, байты и размер совпадают с эталоном");
		if (matched)
			m_iMatches++;
		array<float> afterFloats = {};
		string after = Capture(input, afterFloats);
		bool immutable = after == before && RICON_ChannelCutoffRun.SameFloats(afterFloats, beforeFloats)
			&& CaptureBytes(inputBytes) == beforeBytes && second && second != first && SameResult(second, expected)
			&& Detached(input, second) && (!second.m_aBytes || second.m_aBytes != inputBytes);
		Require(immutable, id, "вход не изменён, результат новый и не делит данные с входом и прошлым вызовом");
		if (immutable)
			m_iImmutable++;
		if (id == "D01" && matched && immutable)
		{
			Controls(second, expected);
			Require(SameResult(second, expected), "descriptor controls", "после контролей исходный результат восстановлен");
		}
		if (second && second.m_eCode == RICON_EDescriptorCode.OK && second.m_aBytes)
		{
			m_aFrames.Insert(second.m_aBytes);
			m_aFramed.Insert(second);
		}
		if (id == "D42")
			m_Dynamic = input;
		PrintFormat("[RICON_DESCRIPTOR] %1 op=%2 code=%3 size=%4 match=%5 immutable=%6", id, op,
			typename.EnumToString(RICON_EDescriptorCode, second.m_eCode), second.m_iSizeBytes, matched, immutable);
	}

	//! Сравнение до порчи первого результата; второй вызов должен дать новый, неиспорченный объект.
	void CheckEncode(string id, RICON_FadingDescriptor d, int maxBytes, RICON_DescriptorResult expected)
	{
		array<float> floats = {};
		string before = Capture(d, floats);
		RICON_DescriptorResult first = RICON_DescriptorCodec.Encode(d, maxBytes);
		bool matched = SameResult(first, expected);
		Spoil(first);
		RICON_DescriptorResult second = RICON_DescriptorCodec.Encode(d, maxBytes);
		Finish(id, "ENCODE", d, null, matched, first, second, expected, before, floats, "null");
	}

	void CheckDecode(string id, array<int> bytes, int maxBytes, RICON_DescriptorResult expected)
	{
		string beforeBytes = CaptureBytes(bytes);
		RICON_DescriptorResult first = RICON_DescriptorCodec.Decode(bytes, maxBytes);
		bool matched = SameResult(first, expected);
		Spoil(first);
		RICON_DescriptorResult second = RICON_DescriptorCodec.Decode(bytes, maxBytes);
		array<float> none = {};
		Finish(id, "DECODE", null, bytes, matched, first, second, expected, "null", none, beforeBytes);
	}

	void CheckRebase(string id, RICON_FadingDescriptor d, int anchorMs, RICON_DescriptorResult expected)
	{
		array<float> floats = {};
		string before = Capture(d, floats);
		RICON_DescriptorResult first = RICON_DescriptorCodec.Rebase(d, anchorMs);
		bool matched = SameResult(first, expected);
		Spoil(first);
		RICON_DescriptorResult second = RICON_DescriptorCodec.Rebase(d, anchorMs);
		Finish(id, "REBASE", d, null, matched, first, second, expected, before, floats, "null");
	}

	//! Переворот младшего бита байта без побитовых операторов.
	static int Flip(int b)
	{
		if (b % 2 == 0)
			return b + 1;
		return b - 1;
	}

	protected void Controls(RICON_DescriptorResult actual, RICON_DescriptorResult expected)
	{
		Reject(SameResult(null, expected));
		actual.m_eCode = RICON_EDescriptorCode.RESOURCE_LIMIT;
		Reject(SameResult(actual, expected));
		actual.m_eCode = RICON_EDescriptorCode.OK;
		RICON_FadingComponent c = actual.m_Descriptor.m_aComponents[0];
		c.m_iPhaseQ = c.m_iPhaseQ + 1;
		Reject(SameResult(actual, expected));
		c.m_iPhaseQ = c.m_iPhaseQ - 1;
		float weight = c.m_fPowerWeight;
		c.m_fPowerWeight = 0.5;
		Reject(SameResult(actual, expected));
		c.m_fPowerWeight = weight;
		int last = actual.m_aBytes.Count() - 1;
		actual.m_aBytes[last] = Flip(actual.m_aBytes[last]);
		Reject(SameResult(actual, expected));
		actual.m_aBytes[last] = Flip(actual.m_aBytes[last]);
		actual.m_aBytes.Insert(0);
		Reject(SameResult(actual, expected));
		actual.m_aBytes.Remove(actual.m_aBytes.Count() - 1);
		actual.m_iSizeBytes = actual.m_iSizeBytes + 1;
		Reject(SameResult(actual, expected));
		actual.m_iSizeBytes = actual.m_iSizeBytes - 1;
		actual.m_Descriptor.m_iValidUntilMs = actual.m_Descriptor.m_iValidUntilMs + 1;
		Reject(SameResult(actual, expected));
		actual.m_Descriptor.m_iValidUntilMs = actual.m_Descriptor.m_iValidUntilMs - 1;
	}

	protected static bool Bits(float value, int expected)
	{
		float back;
		return RICON_DescriptorCodec.FloatToBits(value) == expected && RICON_DescriptorCodec.BitsToFloat(expected, back) && back == value;
	}

	protected static bool Int32Trip(int value)
	{
		array<int> frame = {0, 0, 0, 0, 0, 0, 0};
		RICON_DescriptorCodec.WriteI32(frame, value);
		RICON_DescriptorReader reader = new RICON_DescriptorReader(frame);
		return frame.Count() == 11 && reader.I32() == value && reader.AtEnd() && !reader.m_bFailed;
	}

	//! Битовые образы binary32 и int32 на краях и корневой goldenHex корпуса.
	void CheckArithmetic()
	{
		bool ok = Bits(1.0, 1065353216) && Bits(150.0, 1125515264) && Bits(0.1, 1036831949);
		ok = ok && Bits(340282346638528859811704183484516925440.0, 2139095039);
		ok = ok && Bits(0.000000000000000000000000000000000000000000001401298464324817, 1);
		ok = ok && Bits(0.000000000000000000000000000000000000011754943508222875, 8388608);
		ok = ok && RICON_DescriptorCodec.FloatToBits(0) == 0;
		float special;
		ok = ok && !RICON_DescriptorCodec.BitsToFloat(2139095040, special) && !RICON_DescriptorCodec.BitsToFloat(2143289344, special);
		float negative;
		ok = ok && RICON_DescriptorCodec.BitsToFloat(-1082130432, negative) && negative == -1;
		ok = ok && Int32Trip(0) && Int32Trip(-1) && Int32Trip(2147483647) && Int32Trip(-2147483647 - 1) && Int32Trip(-12345);
		array<int> golden = RICON_ChannelDescriptorFixtures.Golden();
		RICON_DescriptorResult decoded = RICON_DescriptorCodec.Decode(golden, 65535);
		ok = ok && decoded.m_eCode == RICON_EDescriptorCode.OK && SameBytes(decoded.m_aBytes, golden);
		ok = ok && SameBytes(RICON_DescriptorCodec.Encode(decoded.m_Descriptor, 65535).m_aBytes, golden);
		Require(ok, "descriptor arithmetic", "binary32/int32 на краях, NaN/Inf отвергнуты, goldenHex декодируется и пакуется обратно");
		if (ok)
			m_iExtras++;
	}

	//! Round trip каждого OK-кадра и переворот бита в байтах 0, 4, 6 и последнем.
	void CheckFrames()
	{
		bool ok = !m_aFrames.IsEmpty();
		int flips;
		for (int i = 0; i < m_aFrames.Count(); i++)
		{
			array<int> frame = m_aFrames[i];
			RICON_DescriptorResult back = RICON_DescriptorCodec.Decode(frame, 65535);
			ok = ok && back.m_eCode == RICON_EDescriptorCode.OK && SameDescriptor(back.m_Descriptor, m_aFramed[i].m_Descriptor)
				&& SameBytes(back.m_aBytes, frame);
			array<int> positions = {0, 4, 6, frame.Count() - 1};
			foreach (int position : positions)
			{
				array<int> bad = {};
				bad.Copy(frame);
				bad[position] = Flip(bad[position]);
				RICON_DescriptorResult broken = RICON_DescriptorCodec.Decode(bad, 65535);
				ok = ok && (broken.m_eCode != RICON_EDescriptorCode.OK || !SameDescriptor(broken.m_Descriptor, back.m_Descriptor));
				flips++;
			}
		}
		Require(ok, "descriptor frames", "round trip всех OK-кадров, переворот бита не даёт прежний descriptor");
		if (ok)
			m_iExtras++;
		PrintFormat("[RICON_DESCRIPTOR_FRAMES] frames=%1 flips=%2", m_aFrames.Count(), flips);
	}

	protected static RICON_FadingQuery Query(RICON_FadingDescriptor d, int nowMs, int subUs)
	{
		RICON_FadingQuery q = new RICON_FadingQuery();
		q.m_Tx = d.m_A;
		q.m_Rx = d.m_B;
		q.m_fFrequencyMHz = d.m_fFrequencyMHz;
		q.m_fBandwidthHz = d.m_fBandwidthHz;
		q.m_sProfileId = d.m_sProfileId;
		q.m_iProfileVersion = d.m_iProfileVersion;
		q.m_iEnvironmentRevision = d.m_iEnvironmentRevision;
		q.m_iNowMs = nowMs;
		q.m_iSubUs = subUs;
		return q;
	}

	//! CD06: композиция Rebase и непрерывность ядра P1b для старого и нового descriptor (D42).
	void CheckRebaseProperties()
	{
		bool ok = m_Dynamic != null;
		if (ok)
		{
			int low = m_Dynamic.m_iValidFromMs;
			int high = m_Dynamic.m_iValidUntilMs - 1;
			int middle = (low + high) / 2;
			array<int> firsts = {low, low, middle, high};
			array<int> seconds = {high, middle, high, high};
			for (int i = 0; i < firsts.Count(); i++)
			{
				RICON_DescriptorResult once = RICON_DescriptorCodec.Rebase(m_Dynamic, seconds[i]);
				RICON_DescriptorResult step = RICON_DescriptorCodec.Rebase(m_Dynamic, firsts[i]);
				RICON_DescriptorResult twice = RICON_DescriptorCodec.Rebase(step.m_Descriptor, seconds[i]);
				ok = ok && SameResult(twice, once);
				// Порча размера у OK Rebase (поля нет, норма 0) должна ловиться сравнением (codex #3662).
				twice.m_iSizeBytes = 99999;
				ok = ok && !SameResult(twice, once);
				twice.m_iSizeBytes = 0;
				array<int> times = {seconds[i], high};
				foreach (int t : times)
				{
					array<int> subs = {0, 1, 999};
					foreach (int sub : subs)
					{
						RICON_FadingResult a = RICON_FiniteMultipath.Evaluate(m_Dynamic, Query(m_Dynamic, t, sub));
						RICON_FadingResult b = RICON_FiniteMultipath.Evaluate(once.m_Descriptor, Query(once.m_Descriptor, t, sub));
						ok = ok && RICON_ChannelFadingRun.SameResult(a, b);
					}
				}
			}
		}
		Require(ok, "descriptor rebase", "Rebase композиционен и сохраняет фазы и коэффициент P1b");
		if (ok)
			m_iExtras++;
	}
}
