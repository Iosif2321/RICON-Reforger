//! CT10.1: переносимое описание временного канала (REQ-channel-descriptor v0.1, корпус заморожен по LF SHA
//! 95208dd1…aa27). Глубокая копия CF-дескриптора, самодостаточный кадр RCFD/1 и перенос якоря без смены процесса.
//! Кодек проверяет структуру, а не происхождение; snapshot v1, producer/receiver/seam и Rpl не меняются.
//! Значение 0 enum — безопасный отказ по умолчанию. Ни один код не означает LOST.
enum RICON_EDescriptorCode
{
	INVALID_LIMITS = 0,
	INVALID_DESCRIPTOR = 1,
	INVALID_FRAME = 2,
	UNSUPPORTED_VERSION = 3,
	UNSUPPORTED_MODEL = 4,
	RESOURCE_LIMIT = 5,
	OUT_OF_MODEL_DOMAIN = 6,
	INVALID_ANCHOR = 7,
	OK = 8
}

//! При OK: новая глубокая копия descriptor; для Encode/Decode — байты кадра и их число. При отказе — только код.
class RICON_DescriptorResult
{
	RICON_EDescriptorCode m_eCode = RICON_EDescriptorCode.INVALID_LIMITS;
	ref RICON_FadingDescriptor m_Descriptor;
	ref array<int> m_aBytes;
	int m_iSizeBytes;
}

//! Последовательное чтение кадра; любая нехватка байтов поднимает m_bFailed, дальнейшие чтения возвращают 0.
class RICON_DescriptorReader
{
	protected array<int> m_aBytes;
	protected int m_iAt = 7;
	bool m_bFailed;

	void RICON_DescriptorReader(array<int> bytes)
	{
		m_aBytes = bytes;
	}

	bool AtEnd()
	{
		return m_iAt == m_aBytes.Count();
	}

	int U8()
	{
		if (m_bFailed || m_iAt >= m_aBytes.Count())
		{
			m_bFailed = true;
			return 0;
		}
		int value = m_aBytes[m_iAt];
		m_iAt++;
		return value;
	}

	//! Знаковый big-endian int32 без переполнения: (b0 − 256)·2^24 для старшего бита.
	int I32()
	{
		int b0 = U8();
		int b1 = U8();
		int b2 = U8();
		int b3 = U8();
		if (b0 >= 128)
			b0 = b0 - 256;
		return b0 * 16777216 + b1 * 65536 + b2 * 256 + b3;
	}

	bool F32(out float value)
	{
		int bits = I32();
		return !m_bFailed && RICON_DescriptorCodec.BitsToFloat(bits, value);
	}

	//! u8 длина 1..64 и столько же символов алфавита CF; иное — отказ кадра.
	string Key()
	{
		int length = U8();
		if (length < 1 || length > 64)
			m_bFailed = true;
		string key;
		for (int i = 0; i < length && !m_bFailed; i++)
		{
			string symbol = RICON_DescriptorCodec.KeySymbol(U8());
			if (symbol.IsEmpty())
				m_bFailed = true;
			key += symbol;
		}
		return key;
	}
}

class RICON_DescriptorCodec
{
	static const int VERSION = 1;
	static const int MIN_FRAME = 7;
	static const int MAX_FRAME = 5111;		//!< CD07: ключи по 64 байта и 64 составляющих
	static const int MAX_LIMIT = 65535;

	//! Символ ключа CF по ASCII-коду; пустая строка вне [A-Za-z0-9_.:-].
	static string KeySymbol(int code)
	{
		if (code >= 48 && code <= 57)
			return "0123456789".Substring(code - 48, 1);
		if (code >= 65 && code <= 90)
			return "ABCDEFGHIJKLMNOPQRSTUVWXYZ".Substring(code - 65, 1);
		if (code >= 97 && code <= 122)
			return "abcdefghijklmnopqrstuvwxyz".Substring(code - 97, 1);
		if (code == 95)
			return "_";
		if (code == 46)
			return ".";
		if (code == 58)
			return ":";
		if (code == 45)
			return "-";
		return "";
	}

	//! IEEE binary32 образ неотрицательного конечного числа точным масштабированием на 2^k (CD07).
	//! Ноль любого знака даёт +0 (CD03); отрицательные и неконечные значения сюда не попадают после CF.
	static int FloatToBits(float value)
	{
		if (value == 0)
			return 0;
		float x = value;
		int exponent = 0;
		while (x >= 2)
		{
			x = x * 0.5;
			exponent++;
		}
		while (x < 1 && exponent > -126)
		{
			x = x * 2;
			exponent--;
		}
		if (x < 1)
		{
			float subnormal = x * 8388608.0;
			int small = subnormal;
			return small;
		}
		float fraction = (x - 1) * 8388608.0;
		int mantissa = fraction;
		return ((exponent + 127) << 23) | mantissa;
	}

	//! Обратное к FloatToBits для любого образа; экспонента 255 (Inf/NaN) — false. Знак сохраняется.
	static bool BitsToFloat(int bits, out float value)
	{
		value = 0;
		int field = (bits >> 23) & 255;
		int mantissa = bits & 8388607;
		if (field == 255)
			return false;
		float magnitude = mantissa;
		int scale = -149;
		if (field != 0)
		{
			magnitude = 8388608 + mantissa;
			scale = field - 150;
		}
		while (scale > 0)
		{
			magnitude = magnitude * 2;
			scale--;
		}
		while (scale < 0)
		{
			magnitude = magnitude * 0.5;
			scale++;
		}
		value = magnitude;
		if (((bits >> 31) & 1) != 0)
			value = -magnitude;
		return true;
	}

	static void WriteI32(notnull array<int> frame, int value)
	{
		frame.Insert((value >> 24) & 255);
		frame.Insert((value >> 16) & 255);
		frame.Insert((value >> 8) & 255);
		frame.Insert(value & 255);
	}

	protected static void WriteKey(notnull array<int> frame, string key)
	{
		frame.Insert(key.Length());
		for (int i = 0; i < key.Length(); i++)
			frame.Insert(key.ToAscii(i));
	}

	//! CD07: 55 + len(modelId) + len(profileId) + Σ(13 + len(componentId)).
	static int FrameSize(notnull RICON_FadingDescriptor d)
	{
		int size = 55 + d.m_sModelId.Length() + d.m_sProfileId.Length();
		foreach (RICON_FadingComponent c : d.m_aComponents)
			size += 13 + c.m_sComponentId.Length();
		return size;
	}

	protected static RICON_FadingEndpoint CloneEndpoint(RICON_FadingEndpoint e)
	{
		RICON_FadingEndpoint copy = new RICON_FadingEndpoint();
		copy.m_iDeviceId = e.m_iDeviceId;
		copy.m_iTransceiverIndex = e.m_iTransceiverIndex;
		return copy;
	}

	//! Глубокая копия проверенного descriptor; нулевой вес становится +0 (CD03).
	static RICON_FadingDescriptor Clone(notnull RICON_FadingDescriptor d)
	{
		RICON_FadingDescriptor copy = new RICON_FadingDescriptor();
		copy.m_sModelId = d.m_sModelId;
		copy.m_sProfileId = d.m_sProfileId;
		copy.m_iProfileVersion = d.m_iProfileVersion;
		copy.m_A = CloneEndpoint(d.m_A);
		copy.m_B = CloneEndpoint(d.m_B);
		copy.m_fFrequencyMHz = d.m_fFrequencyMHz;
		copy.m_fBandwidthHz = d.m_fBandwidthHz;
		copy.m_iEnvironmentRevision = d.m_iEnvironmentRevision;
		copy.m_iAnchorMs = d.m_iAnchorMs;
		copy.m_iValidFromMs = d.m_iValidFromMs;
		copy.m_iValidUntilMs = d.m_iValidUntilMs;
		copy.m_bStaticChannel = d.m_bStaticChannel;
		copy.m_bCoverageComplete = d.m_bCoverageComplete;
		copy.m_bResourceExhausted = d.m_bResourceExhausted;
		foreach (RICON_FadingComponent c : d.m_aComponents)
		{
			RICON_FadingComponent part = new RICON_FadingComponent();
			part.m_sComponentId = c.m_sComponentId;
			part.m_fPowerWeight = c.m_fPowerWeight;
			if (c.m_fPowerWeight == 0)
				part.m_fPowerWeight = 0;
			part.m_iPhaseQ = c.m_iPhaseQ;
			part.m_iDopplerQ = c.m_iDopplerQ;
			copy.m_aComponents.Insert(part);
		}
		return copy;
	}

	protected static array<int> Write(RICON_FadingDescriptor d)
	{
		array<int> frame = {82, 67, 70, 68, VERSION};
		int size = FrameSize(d);
		frame.Insert((size >> 8) & 255);
		frame.Insert(size & 255);
		WriteKey(frame, d.m_sModelId);
		WriteKey(frame, d.m_sProfileId);
		WriteI32(frame, d.m_iProfileVersion);
		WriteI32(frame, d.m_A.m_iDeviceId);
		WriteI32(frame, d.m_A.m_iTransceiverIndex);
		WriteI32(frame, d.m_B.m_iDeviceId);
		WriteI32(frame, d.m_B.m_iTransceiverIndex);
		WriteI32(frame, FloatToBits(d.m_fFrequencyMHz));
		WriteI32(frame, FloatToBits(d.m_fBandwidthHz));
		WriteI32(frame, d.m_iEnvironmentRevision);
		WriteI32(frame, d.m_iAnchorMs);
		WriteI32(frame, d.m_iValidFromMs);
		WriteI32(frame, d.m_iValidUntilMs);
		int flags = 0;
		if (d.m_bStaticChannel)
			flags = flags | 1;
		if (d.m_bCoverageComplete)
			flags = flags | 2;
		if (d.m_bResourceExhausted)
			flags = flags | 4;
		frame.Insert(flags);
		frame.Insert(d.m_aComponents.Count());
		foreach (RICON_FadingComponent c : d.m_aComponents)
		{
			WriteKey(frame, c.m_sComponentId);
			WriteI32(frame, FloatToBits(c.m_fPowerWeight));
			WriteI32(frame, c.m_iPhaseQ);
			WriteI32(frame, c.m_iDopplerQ);
		}
		return frame;
	}

	protected static RICON_DescriptorResult Make(RICON_EDescriptorCode code)
	{
		RICON_DescriptorResult r = new RICON_DescriptorResult();
		r.m_eCode = code;
		return r;
	}

	protected static RICON_DescriptorResult Framed(RICON_FadingDescriptor copy, array<int> bytes)
	{
		RICON_DescriptorResult r = Make(RICON_EDescriptorCode.OK);
		r.m_Descriptor = copy;
		r.m_aBytes = bytes;
		r.m_iSizeBytes = bytes.Count();
		return r;
	}

	//! CD04: INVALID_LIMITS → INVALID_DESCRIPTOR → UNSUPPORTED_MODEL → RESOURCE_LIMIT → OK, без частичных байтов.
	static RICON_DescriptorResult Encode(RICON_FadingDescriptor d, int maxBytes)
	{
		if (maxBytes < 1 || maxBytes > MAX_LIMIT)
			return Make(RICON_EDescriptorCode.INVALID_LIMITS);
		if (!RICON_FiniteMultipath.ValidDescriptor(d))
			return Make(RICON_EDescriptorCode.INVALID_DESCRIPTOR);
		if (d.m_sModelId != RICON_FiniteMultipath.MODEL_ID)
			return Make(RICON_EDescriptorCode.UNSUPPORTED_MODEL);
		if (FrameSize(d) > maxBytes)
			return Make(RICON_EDescriptorCode.RESOURCE_LIMIT);
		RICON_FadingDescriptor copy = Clone(d);
		return Framed(copy, Write(copy));
	}

	//! CD05: лимит и размер до обхода элементов, затем заголовок, версия, тело, модель и каноничность.
	static RICON_DescriptorResult Decode(array<int> bytes, int maxBytes)
	{
		if (maxBytes < 1 || maxBytes > MAX_LIMIT)
			return Make(RICON_EDescriptorCode.INVALID_LIMITS);
		if (!bytes)
			return Make(RICON_EDescriptorCode.INVALID_FRAME);
		int count = bytes.Count();
		if (count > maxBytes)
			return Make(RICON_EDescriptorCode.RESOURCE_LIMIT);
		if (count < MIN_FRAME || count > MAX_FRAME)
			return Make(RICON_EDescriptorCode.INVALID_FRAME);
		foreach (int b : bytes)
		{
			if (b < 0 || b > 255)
				return Make(RICON_EDescriptorCode.INVALID_FRAME);
		}
		if (bytes[0] != 82 || bytes[1] != 67 || bytes[2] != 70 || bytes[3] != 68 || bytes[5] * 256 + bytes[6] != count)
			return Make(RICON_EDescriptorCode.INVALID_FRAME);
		if (bytes[4] != VERSION)
			return Make(RICON_EDescriptorCode.UNSUPPORTED_VERSION);
		RICON_DescriptorReader reader = new RICON_DescriptorReader(bytes);
		RICON_FadingDescriptor d = new RICON_FadingDescriptor();
		d.m_sModelId = reader.Key();
		d.m_sProfileId = reader.Key();
		d.m_iProfileVersion = reader.I32();
		d.m_A = new RICON_FadingEndpoint();
		d.m_A.m_iDeviceId = reader.I32();
		d.m_A.m_iTransceiverIndex = reader.I32();
		d.m_B = new RICON_FadingEndpoint();
		d.m_B.m_iDeviceId = reader.I32();
		d.m_B.m_iTransceiverIndex = reader.I32();
		float frequency;
		float bandwidth;
		bool numbers = reader.F32(frequency);
		numbers = reader.F32(bandwidth) && numbers;
		d.m_fFrequencyMHz = frequency;
		d.m_fBandwidthHz = bandwidth;
		d.m_iEnvironmentRevision = reader.I32();
		d.m_iAnchorMs = reader.I32();
		d.m_iValidFromMs = reader.I32();
		d.m_iValidUntilMs = reader.I32();
		int flags = reader.U8();
		d.m_bStaticChannel = (flags & 1) != 0;
		d.m_bCoverageComplete = (flags & 2) != 0;
		d.m_bResourceExhausted = (flags & 4) != 0;
		int components = reader.U8();
		if ((flags & 248) != 0 || components > RICON_FiniteMultipath.MAX_COMPONENTS)
			return Make(RICON_EDescriptorCode.INVALID_FRAME);
		for (int i = 0; i < components && !reader.m_bFailed; i++)
		{
			RICON_FadingComponent c = new RICON_FadingComponent();
			c.m_sComponentId = reader.Key();
			float weight;
			numbers = reader.F32(weight) && numbers;
			c.m_fPowerWeight = weight;
			c.m_iPhaseQ = reader.I32();
			c.m_iDopplerQ = reader.I32();
			d.m_aComponents.Insert(c);
		}
		if (reader.m_bFailed || !numbers || !reader.AtEnd() || !RICON_FiniteMultipath.ValidDescriptor(d))
			return Make(RICON_EDescriptorCode.INVALID_FRAME);
		if (d.m_sModelId != RICON_FiniteMultipath.MODEL_ID)
			return Make(RICON_EDescriptorCode.UNSUPPORTED_MODEL);
		RICON_FadingDescriptor copy = Clone(d);
		array<int> again = Write(copy);
		if (again.Count() != count)
			return Make(RICON_EDescriptorCode.INVALID_FRAME);
		for (int j = 0; j < count; j++)
		{
			if (again[j] != bytes[j])
				return Make(RICON_EDescriptorCode.INVALID_FRAME);
		}
		return Framed(copy, again);
	}

	//! CD06: тот же процесс с новым якорем; validUntil не продлевается.
	static RICON_DescriptorResult Rebase(RICON_FadingDescriptor d, int anchorMs)
	{
		if (!RICON_FiniteMultipath.ValidDescriptor(d))
			return Make(RICON_EDescriptorCode.INVALID_DESCRIPTOR);
		if (d.m_sModelId != RICON_FiniteMultipath.MODEL_ID)
			return Make(RICON_EDescriptorCode.UNSUPPORTED_MODEL);
		foreach (RICON_FadingComponent domain : d.m_aComponents)
		{
			if (domain.m_iDopplerQ > RICON_FiniteMultipath.QUARTER || domain.m_iDopplerQ < -RICON_FiniteMultipath.QUARTER)
				return Make(RICON_EDescriptorCode.OUT_OF_MODEL_DOMAIN);
		}
		if (anchorMs < d.m_iValidFromMs || anchorMs >= d.m_iValidUntilMs)
			return Make(RICON_EDescriptorCode.INVALID_ANCHOR);
		RICON_FadingDescriptor copy = Clone(d);
		int deltaMs = anchorMs - d.m_iAnchorMs;
		int tQ = RICON_FiniteMultipath.MulMod(deltaMs % RICON_FiniteMultipath.PHASE_M, 1000);
		foreach (RICON_FadingComponent c : copy.m_aComponents)
			c.m_iPhaseQ = RICON_FiniteMultipath.PhaseAt(c.m_iPhaseQ, c.m_iDopplerQ, tQ);
		copy.m_iAnchorMs = anchorMs;
		copy.m_iValidFromMs = anchorMs;
		RICON_DescriptorResult r = Make(RICON_EDescriptorCode.OK);
		r.m_Descriptor = copy;
		return r;
	}
}
