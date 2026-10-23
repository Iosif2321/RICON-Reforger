//! Сгенерировано gen_channel_descriptor_fixture.py; ожидания только копируются; TEST_ONLY.
//! Host-only (недостижимы в типизированном Enforce): D17 — bool вместо бюджета, D18 — дробный бюджет, D19 — bool вместо веса, D20 — лишнее поле epoch, D21 — пропущенное поле, D31 — bool вместо байта, D47 — bool вместо якоря, D54 — bool и 999 в списке байтов.
class RICON_ChannelDescriptorFixtures
{
	static const string SOURCE_SHA = "95208dd198cee4680763e6813cb66374832ef241ea1b5590e20799d834f2aa27";
	static const int CASES = 46;
	static const int HOST_ONLY = 8;

	//! Корневой goldenHex корпуса: вручную размеченный кадр CD07.
	static array<int> Golden()
	{
		array<int> golden = {82, 67, 70, 68, 1, 0, 102, 19, 70, 73, 78, 73, 84, 69, 95, 77, 85, 76, 84, 73, 80, 65, 84, 72, 95, 86, 49, 9, 84, 69, 83, 84, 95, 79, 78, 76, 89, 0, 0, 0, 1, 0, 0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 20, 0, 0, 0, 0, 67, 22, 0, 0, 70, 195, 80, 0, 0, 0, 0, 1, 0, 0, 3, 232, 0, 0, 3, 232, 0, 0, 19, 136, 3, 1, 6, 100, 105, 114, 101, 99, 116, 63, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		return golden;
	}

	protected static void Case0(RICON_ChannelDescriptorRun t)
	{
		RICON_FadingDescriptor d = new RICON_FadingDescriptor();
		d.m_sModelId = "FINITE_MULTIPATH_V1";
		d.m_sProfileId = "TEST_ONLY";
		d.m_A = t.End(10, 0);
		d.m_B = t.End(20, 0);
		d.m_iProfileVersion = 1;
		d.m_iEnvironmentRevision = 1;
		d.m_iAnchorMs = 1000;
		d.m_iValidFromMs = 1000;
		d.m_iValidUntilMs = 5000;
		d.m_bStaticChannel = true;
		d.m_bCoverageComplete = true;
		d.m_bResourceExhausted = false;
		d.m_fFrequencyMHz = 150.0;
		d.m_fBandwidthHz = 25000.0;
		t.Add(d, "direct", 1.0, 0, 0);
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.OK;
		RICON_FadingDescriptor expectedDescriptor = new RICON_FadingDescriptor();
		expectedDescriptor.m_sModelId = "FINITE_MULTIPATH_V1";
		expectedDescriptor.m_sProfileId = "TEST_ONLY";
		expectedDescriptor.m_A = t.End(10, 0);
		expectedDescriptor.m_B = t.End(20, 0);
		expectedDescriptor.m_iProfileVersion = 1;
		expectedDescriptor.m_iEnvironmentRevision = 1;
		expectedDescriptor.m_iAnchorMs = 1000;
		expectedDescriptor.m_iValidFromMs = 1000;
		expectedDescriptor.m_iValidUntilMs = 5000;
		expectedDescriptor.m_bStaticChannel = true;
		expectedDescriptor.m_bCoverageComplete = true;
		expectedDescriptor.m_bResourceExhausted = false;
		expectedDescriptor.m_fFrequencyMHz = 150.0;
		expectedDescriptor.m_fBandwidthHz = 25000.0;
		t.Add(expectedDescriptor, "direct", 1.0, 0, 0);
		expected.m_Descriptor = expectedDescriptor;
		array<int> expectedBytes = t.Hex("524346440100661346494e4954455f4d554c5449504154485f563109544553545f4f4e4c59000000010000000a0000000000000014000000004316000046c3500000000001000003e8000003e8000013880301066469726563743f8000000000000000000000");
		expected.m_aBytes = expectedBytes;
		expected.m_iSizeBytes = 102;
		t.CheckEncode("D01", d, 5120, expected);
	}

	protected static void Case1(RICON_ChannelDescriptorRun t)
	{
		array<int> bytes = t.Hex("524346440100661346494e4954455f4d554c5449504154485f563109544553545f4f4e4c59000000010000000a0000000000000014000000004316000046c3500000000001000003e8000003e8000013880301066469726563743f8000000000000000000000");
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.OK;
		RICON_FadingDescriptor expectedDescriptor = new RICON_FadingDescriptor();
		expectedDescriptor.m_sModelId = "FINITE_MULTIPATH_V1";
		expectedDescriptor.m_sProfileId = "TEST_ONLY";
		expectedDescriptor.m_A = t.End(10, 0);
		expectedDescriptor.m_B = t.End(20, 0);
		expectedDescriptor.m_iProfileVersion = 1;
		expectedDescriptor.m_iEnvironmentRevision = 1;
		expectedDescriptor.m_iAnchorMs = 1000;
		expectedDescriptor.m_iValidFromMs = 1000;
		expectedDescriptor.m_iValidUntilMs = 5000;
		expectedDescriptor.m_bStaticChannel = true;
		expectedDescriptor.m_bCoverageComplete = true;
		expectedDescriptor.m_bResourceExhausted = false;
		expectedDescriptor.m_fFrequencyMHz = 150.0;
		expectedDescriptor.m_fBandwidthHz = 25000.0;
		t.Add(expectedDescriptor, "direct", 1.0, 0, 0);
		expected.m_Descriptor = expectedDescriptor;
		array<int> expectedBytes = t.Hex("524346440100661346494e4954455f4d554c5449504154485f563109544553545f4f4e4c59000000010000000a0000000000000014000000004316000046c3500000000001000003e8000003e8000013880301066469726563743f8000000000000000000000");
		expected.m_aBytes = expectedBytes;
		expected.m_iSizeBytes = 102;
		t.CheckDecode("D02", bytes, 5120, expected);
	}

	protected static void Case2(RICON_ChannelDescriptorRun t)
	{
		RICON_FadingDescriptor d = new RICON_FadingDescriptor();
		d.m_sModelId = "FINITE_MULTIPATH_V1";
		d.m_sProfileId = "TEST_ONLY";
		d.m_A = t.End(10, 0);
		d.m_B = t.End(20, 0);
		d.m_iProfileVersion = 1;
		d.m_iEnvironmentRevision = 1;
		d.m_iAnchorMs = 1000;
		d.m_iValidFromMs = 1000;
		d.m_iValidUntilMs = 5000;
		d.m_bStaticChannel = true;
		d.m_bCoverageComplete = true;
		d.m_bResourceExhausted = false;
		d.m_fFrequencyMHz = 150.0;
		d.m_fBandwidthHz = 25000.0;
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.OK;
		RICON_FadingDescriptor expectedDescriptor = new RICON_FadingDescriptor();
		expectedDescriptor.m_sModelId = "FINITE_MULTIPATH_V1";
		expectedDescriptor.m_sProfileId = "TEST_ONLY";
		expectedDescriptor.m_A = t.End(10, 0);
		expectedDescriptor.m_B = t.End(20, 0);
		expectedDescriptor.m_iProfileVersion = 1;
		expectedDescriptor.m_iEnvironmentRevision = 1;
		expectedDescriptor.m_iAnchorMs = 1000;
		expectedDescriptor.m_iValidFromMs = 1000;
		expectedDescriptor.m_iValidUntilMs = 5000;
		expectedDescriptor.m_bStaticChannel = true;
		expectedDescriptor.m_bCoverageComplete = true;
		expectedDescriptor.m_bResourceExhausted = false;
		expectedDescriptor.m_fFrequencyMHz = 150.0;
		expectedDescriptor.m_fBandwidthHz = 25000.0;
		expected.m_Descriptor = expectedDescriptor;
		array<int> expectedBytes = t.Hex("524346440100531346494e4954455f4d554c5449504154485f563109544553545f4f4e4c59000000010000000a0000000000000014000000004316000046c3500000000001000003e8000003e8000013880300");
		expected.m_aBytes = expectedBytes;
		expected.m_iSizeBytes = 83;
		t.CheckEncode("D03", d, 5120, expected);
	}

	protected static void Case3(RICON_ChannelDescriptorRun t)
	{
		RICON_FadingDescriptor d = new RICON_FadingDescriptor();
		d.m_sModelId = "FINITE_MULTIPATH_V1";
		d.m_sProfileId = "TEST_ONLY";
		d.m_A = t.End(10, 0);
		d.m_B = t.End(20, 0);
		d.m_iProfileVersion = 1;
		d.m_iEnvironmentRevision = 1;
		d.m_iAnchorMs = 1000;
		d.m_iValidFromMs = 1000;
		d.m_iValidUntilMs = 5000;
		d.m_bStaticChannel = true;
		d.m_bCoverageComplete = true;
		d.m_bResourceExhausted = false;
		d.m_fFrequencyMHz = 150.0;
		d.m_fBandwidthHz = 25000.0;
		t.Add(d, "direct", 0.0, 0, 0);
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.OK;
		RICON_FadingDescriptor expectedDescriptor = new RICON_FadingDescriptor();
		expectedDescriptor.m_sModelId = "FINITE_MULTIPATH_V1";
		expectedDescriptor.m_sProfileId = "TEST_ONLY";
		expectedDescriptor.m_A = t.End(10, 0);
		expectedDescriptor.m_B = t.End(20, 0);
		expectedDescriptor.m_iProfileVersion = 1;
		expectedDescriptor.m_iEnvironmentRevision = 1;
		expectedDescriptor.m_iAnchorMs = 1000;
		expectedDescriptor.m_iValidFromMs = 1000;
		expectedDescriptor.m_iValidUntilMs = 5000;
		expectedDescriptor.m_bStaticChannel = true;
		expectedDescriptor.m_bCoverageComplete = true;
		expectedDescriptor.m_bResourceExhausted = false;
		expectedDescriptor.m_fFrequencyMHz = 150.0;
		expectedDescriptor.m_fBandwidthHz = 25000.0;
		t.Add(expectedDescriptor, "direct", 0.0, 0, 0);
		expected.m_Descriptor = expectedDescriptor;
		array<int> expectedBytes = t.Hex("524346440100661346494e4954455f4d554c5449504154485f563109544553545f4f4e4c59000000010000000a0000000000000014000000004316000046c3500000000001000003e8000003e800001388030106646972656374000000000000000000000000");
		expected.m_aBytes = expectedBytes;
		expected.m_iSizeBytes = 102;
		t.CheckEncode("D04", d, 5120, expected);
	}

	protected static void Case4(RICON_ChannelDescriptorRun t)
	{
		RICON_FadingDescriptor d = new RICON_FadingDescriptor();
		d.m_sModelId = "FINITE_MULTIPATH_V1";
		d.m_sProfileId = "TEST_ONLY";
		d.m_A = t.End(10, 0);
		d.m_B = t.End(20, 0);
		d.m_iProfileVersion = 1;
		d.m_iEnvironmentRevision = 1;
		d.m_iAnchorMs = 1000;
		d.m_iValidFromMs = 1000;
		d.m_iValidUntilMs = 5000;
		d.m_bStaticChannel = true;
		d.m_bCoverageComplete = false;
		d.m_bResourceExhausted = false;
		d.m_fFrequencyMHz = 150.0;
		d.m_fBandwidthHz = 25000.0;
		t.Add(d, "direct", 1.0, 0, 0);
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.OK;
		RICON_FadingDescriptor expectedDescriptor = new RICON_FadingDescriptor();
		expectedDescriptor.m_sModelId = "FINITE_MULTIPATH_V1";
		expectedDescriptor.m_sProfileId = "TEST_ONLY";
		expectedDescriptor.m_A = t.End(10, 0);
		expectedDescriptor.m_B = t.End(20, 0);
		expectedDescriptor.m_iProfileVersion = 1;
		expectedDescriptor.m_iEnvironmentRevision = 1;
		expectedDescriptor.m_iAnchorMs = 1000;
		expectedDescriptor.m_iValidFromMs = 1000;
		expectedDescriptor.m_iValidUntilMs = 5000;
		expectedDescriptor.m_bStaticChannel = true;
		expectedDescriptor.m_bCoverageComplete = false;
		expectedDescriptor.m_bResourceExhausted = false;
		expectedDescriptor.m_fFrequencyMHz = 150.0;
		expectedDescriptor.m_fBandwidthHz = 25000.0;
		t.Add(expectedDescriptor, "direct", 1.0, 0, 0);
		expected.m_Descriptor = expectedDescriptor;
		array<int> expectedBytes = t.Hex("524346440100661346494e4954455f4d554c5449504154485f563109544553545f4f4e4c59000000010000000a0000000000000014000000004316000046c3500000000001000003e8000003e8000013880101066469726563743f8000000000000000000000");
		expected.m_aBytes = expectedBytes;
		expected.m_iSizeBytes = 102;
		t.CheckEncode("D05", d, 5120, expected);
	}

	protected static void Case5(RICON_ChannelDescriptorRun t)
	{
		RICON_FadingDescriptor d = new RICON_FadingDescriptor();
		d.m_sModelId = "FINITE_MULTIPATH_V1";
		d.m_sProfileId = "TEST_ONLY";
		d.m_A = t.End(10, 0);
		d.m_B = t.End(20, 0);
		d.m_iProfileVersion = 1;
		d.m_iEnvironmentRevision = 1;
		d.m_iAnchorMs = 1000;
		d.m_iValidFromMs = 1000;
		d.m_iValidUntilMs = 5000;
		d.m_bStaticChannel = true;
		d.m_bCoverageComplete = true;
		d.m_bResourceExhausted = true;
		d.m_fFrequencyMHz = 150.0;
		d.m_fBandwidthHz = 25000.0;
		t.Add(d, "direct", 1.0, 0, 0);
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.OK;
		RICON_FadingDescriptor expectedDescriptor = new RICON_FadingDescriptor();
		expectedDescriptor.m_sModelId = "FINITE_MULTIPATH_V1";
		expectedDescriptor.m_sProfileId = "TEST_ONLY";
		expectedDescriptor.m_A = t.End(10, 0);
		expectedDescriptor.m_B = t.End(20, 0);
		expectedDescriptor.m_iProfileVersion = 1;
		expectedDescriptor.m_iEnvironmentRevision = 1;
		expectedDescriptor.m_iAnchorMs = 1000;
		expectedDescriptor.m_iValidFromMs = 1000;
		expectedDescriptor.m_iValidUntilMs = 5000;
		expectedDescriptor.m_bStaticChannel = true;
		expectedDescriptor.m_bCoverageComplete = true;
		expectedDescriptor.m_bResourceExhausted = true;
		expectedDescriptor.m_fFrequencyMHz = 150.0;
		expectedDescriptor.m_fBandwidthHz = 25000.0;
		t.Add(expectedDescriptor, "direct", 1.0, 0, 0);
		expected.m_Descriptor = expectedDescriptor;
		array<int> expectedBytes = t.Hex("524346440100661346494e4954455f4d554c5449504154485f563109544553545f4f4e4c59000000010000000a0000000000000014000000004316000046c3500000000001000003e8000003e8000013880701066469726563743f8000000000000000000000");
		expected.m_aBytes = expectedBytes;
		expected.m_iSizeBytes = 102;
		t.CheckEncode("D06", d, 5120, expected);
	}

	protected static void Case6(RICON_ChannelDescriptorRun t)
	{
		RICON_FadingDescriptor d = new RICON_FadingDescriptor();
		d.m_sModelId = "FINITE_MULTIPATH_V1";
		d.m_sProfileId = "TEST_ONLY";
		d.m_A = t.End(10, 0);
		d.m_B = t.End(20, 0);
		d.m_iProfileVersion = 1;
		d.m_iEnvironmentRevision = 1;
		d.m_iAnchorMs = 1000;
		d.m_iValidFromMs = 1000;
		d.m_iValidUntilMs = 5000;
		d.m_bStaticChannel = true;
		d.m_bCoverageComplete = true;
		d.m_bResourceExhausted = false;
		d.m_fFrequencyMHz = 150.0;
		d.m_fBandwidthHz = 25000.0;
		t.Add(d, "direct", 0.000000000000000000000000000000000000000000001401298464324817, 0, 0);
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.OK;
		RICON_FadingDescriptor expectedDescriptor = new RICON_FadingDescriptor();
		expectedDescriptor.m_sModelId = "FINITE_MULTIPATH_V1";
		expectedDescriptor.m_sProfileId = "TEST_ONLY";
		expectedDescriptor.m_A = t.End(10, 0);
		expectedDescriptor.m_B = t.End(20, 0);
		expectedDescriptor.m_iProfileVersion = 1;
		expectedDescriptor.m_iEnvironmentRevision = 1;
		expectedDescriptor.m_iAnchorMs = 1000;
		expectedDescriptor.m_iValidFromMs = 1000;
		expectedDescriptor.m_iValidUntilMs = 5000;
		expectedDescriptor.m_bStaticChannel = true;
		expectedDescriptor.m_bCoverageComplete = true;
		expectedDescriptor.m_bResourceExhausted = false;
		expectedDescriptor.m_fFrequencyMHz = 150.0;
		expectedDescriptor.m_fBandwidthHz = 25000.0;
		t.Add(expectedDescriptor, "direct", 0.000000000000000000000000000000000000000000001401298464324817, 0, 0);
		expected.m_Descriptor = expectedDescriptor;
		array<int> expectedBytes = t.Hex("524346440100661346494e4954455f4d554c5449504154485f563109544553545f4f4e4c59000000010000000a0000000000000014000000004316000046c3500000000001000003e8000003e800001388030106646972656374000000010000000000000000");
		expected.m_aBytes = expectedBytes;
		expected.m_iSizeBytes = 102;
		t.CheckEncode("D07", d, 5120, expected);
	}

	protected static void Case7(RICON_ChannelDescriptorRun t)
	{
		RICON_FadingDescriptor d = new RICON_FadingDescriptor();
		d.m_sModelId = "FINITE_MULTIPATH_V1";
		d.m_sProfileId = "TEST_ONLY";
		d.m_A = t.End(10, 0);
		d.m_B = t.End(20, 0);
		d.m_iProfileVersion = 1;
		d.m_iEnvironmentRevision = 1;
		d.m_iAnchorMs = 1000;
		d.m_iValidFromMs = 1000;
		d.m_iValidUntilMs = 5000;
		d.m_bStaticChannel = true;
		d.m_bCoverageComplete = true;
		d.m_bResourceExhausted = false;
		d.m_fFrequencyMHz = 340282346638528859811704183484516925440.0;
		d.m_fBandwidthHz = 0.000000000000000000000000000000000000000000001401298464324817;
		t.Add(d, "direct", 1.0, 0, 0);
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.OK;
		RICON_FadingDescriptor expectedDescriptor = new RICON_FadingDescriptor();
		expectedDescriptor.m_sModelId = "FINITE_MULTIPATH_V1";
		expectedDescriptor.m_sProfileId = "TEST_ONLY";
		expectedDescriptor.m_A = t.End(10, 0);
		expectedDescriptor.m_B = t.End(20, 0);
		expectedDescriptor.m_iProfileVersion = 1;
		expectedDescriptor.m_iEnvironmentRevision = 1;
		expectedDescriptor.m_iAnchorMs = 1000;
		expectedDescriptor.m_iValidFromMs = 1000;
		expectedDescriptor.m_iValidUntilMs = 5000;
		expectedDescriptor.m_bStaticChannel = true;
		expectedDescriptor.m_bCoverageComplete = true;
		expectedDescriptor.m_bResourceExhausted = false;
		expectedDescriptor.m_fFrequencyMHz = 340282346638528859811704183484516925440.0;
		expectedDescriptor.m_fBandwidthHz = 0.000000000000000000000000000000000000000000001401298464324817;
		t.Add(expectedDescriptor, "direct", 1.0, 0, 0);
		expected.m_Descriptor = expectedDescriptor;
		array<int> expectedBytes = t.Hex("524346440100661346494e4954455f4d554c5449504154485f563109544553545f4f4e4c59000000010000000a0000000000000014000000007f7fffff0000000100000001000003e8000003e8000013880301066469726563743f8000000000000000000000");
		expected.m_aBytes = expectedBytes;
		expected.m_iSizeBytes = 102;
		t.CheckEncode("D08", d, 5120, expected);
	}

	protected static void Case8(RICON_ChannelDescriptorRun t)
	{
		RICON_FadingDescriptor d = new RICON_FadingDescriptor();
		d.m_sModelId = "FINITE_MULTIPATH_V1";
		d.m_sProfileId = "TEST_ONLY";
		d.m_A = t.End(10, 0);
		d.m_B = t.End(20, 0);
		d.m_iProfileVersion = 1;
		d.m_iEnvironmentRevision = 1;
		d.m_iAnchorMs = 1000;
		d.m_iValidFromMs = 1000;
		d.m_iValidUntilMs = 5000;
		d.m_bStaticChannel = true;
		d.m_bCoverageComplete = true;
		d.m_bResourceExhausted = false;
		d.m_fFrequencyMHz = 150.0;
		d.m_fBandwidthHz = 25000.0;
		t.Add(d, "direct", -0.0, 0, 0);
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.OK;
		RICON_FadingDescriptor expectedDescriptor = new RICON_FadingDescriptor();
		expectedDescriptor.m_sModelId = "FINITE_MULTIPATH_V1";
		expectedDescriptor.m_sProfileId = "TEST_ONLY";
		expectedDescriptor.m_A = t.End(10, 0);
		expectedDescriptor.m_B = t.End(20, 0);
		expectedDescriptor.m_iProfileVersion = 1;
		expectedDescriptor.m_iEnvironmentRevision = 1;
		expectedDescriptor.m_iAnchorMs = 1000;
		expectedDescriptor.m_iValidFromMs = 1000;
		expectedDescriptor.m_iValidUntilMs = 5000;
		expectedDescriptor.m_bStaticChannel = true;
		expectedDescriptor.m_bCoverageComplete = true;
		expectedDescriptor.m_bResourceExhausted = false;
		expectedDescriptor.m_fFrequencyMHz = 150.0;
		expectedDescriptor.m_fBandwidthHz = 25000.0;
		t.Add(expectedDescriptor, "direct", 0.0, 0, 0);
		expected.m_Descriptor = expectedDescriptor;
		array<int> expectedBytes = t.Hex("524346440100661346494e4954455f4d554c5449504154485f563109544553545f4f4e4c59000000010000000a0000000000000014000000004316000046c3500000000001000003e8000003e800001388030106646972656374000000000000000000000000");
		expected.m_aBytes = expectedBytes;
		expected.m_iSizeBytes = 102;
		t.CheckEncode("D09", d, 5120, expected);
	}

	protected static void Case9(RICON_ChannelDescriptorRun t)
	{
		RICON_FadingDescriptor d = new RICON_FadingDescriptor();
		d.m_sModelId = "FINITE_MULTIPATH_V1";
		d.m_sProfileId = "TEST_ONLY";
		d.m_A = t.End(20, 0);
		d.m_B = t.End(10, 0);
		d.m_iProfileVersion = 1;
		d.m_iEnvironmentRevision = 1;
		d.m_iAnchorMs = 1000;
		d.m_iValidFromMs = 1000;
		d.m_iValidUntilMs = 5000;
		d.m_bStaticChannel = true;
		d.m_bCoverageComplete = true;
		d.m_bResourceExhausted = false;
		d.m_fFrequencyMHz = 150.0;
		d.m_fBandwidthHz = 25000.0;
		t.Add(d, "direct", 1.0, 0, 0);
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.INVALID_DESCRIPTOR;
		t.CheckEncode("D10", d, 5120, expected);
	}

	protected static void Case10(RICON_ChannelDescriptorRun t)
	{
		RICON_FadingDescriptor d = new RICON_FadingDescriptor();
		d.m_sModelId = "OTHER_MODEL";
		d.m_sProfileId = "TEST_ONLY";
		d.m_A = t.End(10, 0);
		d.m_B = t.End(20, 0);
		d.m_iProfileVersion = 1;
		d.m_iEnvironmentRevision = 1;
		d.m_iAnchorMs = 1000;
		d.m_iValidFromMs = 1000;
		d.m_iValidUntilMs = 5000;
		d.m_bStaticChannel = true;
		d.m_bCoverageComplete = true;
		d.m_bResourceExhausted = false;
		d.m_fFrequencyMHz = 150.0;
		d.m_fBandwidthHz = 25000.0;
		t.Add(d, "direct", 1.0, 0, 0);
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.UNSUPPORTED_MODEL;
		t.CheckEncode("D11", d, 5120, expected);
	}

	protected static void Case11(RICON_ChannelDescriptorRun t)
	{
		RICON_FadingDescriptor d = new RICON_FadingDescriptor();
		d.m_sModelId = "FINITE_MULTIPATH_V1";
		d.m_sProfileId = "TEST_ONLY";
		d.m_A = t.End(10, 0);
		d.m_B = t.End(20, 0);
		d.m_iProfileVersion = 1;
		d.m_iEnvironmentRevision = 1;
		d.m_iAnchorMs = 1000;
		d.m_iValidFromMs = 1000;
		d.m_iValidUntilMs = 5000;
		d.m_bStaticChannel = true;
		d.m_bCoverageComplete = true;
		d.m_bResourceExhausted = false;
		d.m_fFrequencyMHz = 150.0;
		d.m_fBandwidthHz = 25000.0;
		t.Add(d, "c00", 1.0, 0, 0);
		t.Add(d, "c01", 1.0, 0, 0);
		t.Add(d, "c02", 1.0, 0, 0);
		t.Add(d, "c03", 1.0, 0, 0);
		t.Add(d, "c04", 1.0, 0, 0);
		t.Add(d, "c05", 1.0, 0, 0);
		t.Add(d, "c06", 1.0, 0, 0);
		t.Add(d, "c07", 1.0, 0, 0);
		t.Add(d, "c08", 1.0, 0, 0);
		t.Add(d, "c09", 1.0, 0, 0);
		t.Add(d, "c10", 1.0, 0, 0);
		t.Add(d, "c11", 1.0, 0, 0);
		t.Add(d, "c12", 1.0, 0, 0);
		t.Add(d, "c13", 1.0, 0, 0);
		t.Add(d, "c14", 1.0, 0, 0);
		t.Add(d, "c15", 1.0, 0, 0);
		t.Add(d, "c16", 1.0, 0, 0);
		t.Add(d, "c17", 1.0, 0, 0);
		t.Add(d, "c18", 1.0, 0, 0);
		t.Add(d, "c19", 1.0, 0, 0);
		t.Add(d, "c20", 1.0, 0, 0);
		t.Add(d, "c21", 1.0, 0, 0);
		t.Add(d, "c22", 1.0, 0, 0);
		t.Add(d, "c23", 1.0, 0, 0);
		t.Add(d, "c24", 1.0, 0, 0);
		t.Add(d, "c25", 1.0, 0, 0);
		t.Add(d, "c26", 1.0, 0, 0);
		t.Add(d, "c27", 1.0, 0, 0);
		t.Add(d, "c28", 1.0, 0, 0);
		t.Add(d, "c29", 1.0, 0, 0);
		t.Add(d, "c30", 1.0, 0, 0);
		t.Add(d, "c31", 1.0, 0, 0);
		t.Add(d, "c32", 1.0, 0, 0);
		t.Add(d, "c33", 1.0, 0, 0);
		t.Add(d, "c34", 1.0, 0, 0);
		t.Add(d, "c35", 1.0, 0, 0);
		t.Add(d, "c36", 1.0, 0, 0);
		t.Add(d, "c37", 1.0, 0, 0);
		t.Add(d, "c38", 1.0, 0, 0);
		t.Add(d, "c39", 1.0, 0, 0);
		t.Add(d, "c40", 1.0, 0, 0);
		t.Add(d, "c41", 1.0, 0, 0);
		t.Add(d, "c42", 1.0, 0, 0);
		t.Add(d, "c43", 1.0, 0, 0);
		t.Add(d, "c44", 1.0, 0, 0);
		t.Add(d, "c45", 1.0, 0, 0);
		t.Add(d, "c46", 1.0, 0, 0);
		t.Add(d, "c47", 1.0, 0, 0);
		t.Add(d, "c48", 1.0, 0, 0);
		t.Add(d, "c49", 1.0, 0, 0);
		t.Add(d, "c50", 1.0, 0, 0);
		t.Add(d, "c51", 1.0, 0, 0);
		t.Add(d, "c52", 1.0, 0, 0);
		t.Add(d, "c53", 1.0, 0, 0);
		t.Add(d, "c54", 1.0, 0, 0);
		t.Add(d, "c55", 1.0, 0, 0);
		t.Add(d, "c56", 1.0, 0, 0);
		t.Add(d, "c57", 1.0, 0, 0);
		t.Add(d, "c58", 1.0, 0, 0);
		t.Add(d, "c59", 1.0, 0, 0);
		t.Add(d, "c60", 1.0, 0, 0);
		t.Add(d, "c61", 1.0, 0, 0);
		t.Add(d, "c62", 1.0, 0, 0);
		t.Add(d, "c63", 1.0, 0, 0);
		t.Add(d, "c64", 1.0, 0, 0);
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.INVALID_DESCRIPTOR;
		t.CheckEncode("D12", d, 5120, expected);
	}

	protected static void Case12(RICON_ChannelDescriptorRun t)
	{
		RICON_FadingDescriptor d = new RICON_FadingDescriptor();
		d.m_sModelId = "FINITE_MULTIPATH_V1";
		d.m_sProfileId = "pppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppp";
		d.m_A = t.End(10, 0);
		d.m_B = t.End(20, 0);
		d.m_iProfileVersion = 1;
		d.m_iEnvironmentRevision = 1;
		d.m_iAnchorMs = 1000;
		d.m_iValidFromMs = 1000;
		d.m_iValidUntilMs = 5000;
		d.m_bStaticChannel = true;
		d.m_bCoverageComplete = true;
		d.m_bResourceExhausted = false;
		d.m_fFrequencyMHz = 150.0;
		d.m_fBandwidthHz = 25000.0;
		t.Add(d, "c00xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 0, 0);
		t.Add(d, "c01xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 1, 0);
		t.Add(d, "c02xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 2, 0);
		t.Add(d, "c03xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 3, 0);
		t.Add(d, "c04xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 4, 0);
		t.Add(d, "c05xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 5, 0);
		t.Add(d, "c06xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 6, 0);
		t.Add(d, "c07xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 7, 0);
		t.Add(d, "c08xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 8, 0);
		t.Add(d, "c09xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 9, 0);
		t.Add(d, "c10xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 10, 0);
		t.Add(d, "c11xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 11, 0);
		t.Add(d, "c12xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 12, 0);
		t.Add(d, "c13xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 13, 0);
		t.Add(d, "c14xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 14, 0);
		t.Add(d, "c15xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 15, 0);
		t.Add(d, "c16xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 16, 0);
		t.Add(d, "c17xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 17, 0);
		t.Add(d, "c18xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 18, 0);
		t.Add(d, "c19xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 19, 0);
		t.Add(d, "c20xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 20, 0);
		t.Add(d, "c21xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 21, 0);
		t.Add(d, "c22xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 22, 0);
		t.Add(d, "c23xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 23, 0);
		t.Add(d, "c24xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 24, 0);
		t.Add(d, "c25xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 25, 0);
		t.Add(d, "c26xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 26, 0);
		t.Add(d, "c27xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 27, 0);
		t.Add(d, "c28xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 28, 0);
		t.Add(d, "c29xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 29, 0);
		t.Add(d, "c30xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 30, 0);
		t.Add(d, "c31xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 31, 0);
		t.Add(d, "c32xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 32, 0);
		t.Add(d, "c33xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 33, 0);
		t.Add(d, "c34xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 34, 0);
		t.Add(d, "c35xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 35, 0);
		t.Add(d, "c36xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 36, 0);
		t.Add(d, "c37xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 37, 0);
		t.Add(d, "c38xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 38, 0);
		t.Add(d, "c39xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 39, 0);
		t.Add(d, "c40xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 40, 0);
		t.Add(d, "c41xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 41, 0);
		t.Add(d, "c42xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 42, 0);
		t.Add(d, "c43xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 43, 0);
		t.Add(d, "c44xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 44, 0);
		t.Add(d, "c45xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 45, 0);
		t.Add(d, "c46xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 46, 0);
		t.Add(d, "c47xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 47, 0);
		t.Add(d, "c48xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 48, 0);
		t.Add(d, "c49xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 49, 0);
		t.Add(d, "c50xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 50, 0);
		t.Add(d, "c51xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 51, 0);
		t.Add(d, "c52xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 52, 0);
		t.Add(d, "c53xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 53, 0);
		t.Add(d, "c54xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 54, 0);
		t.Add(d, "c55xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 55, 0);
		t.Add(d, "c56xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 56, 0);
		t.Add(d, "c57xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 57, 0);
		t.Add(d, "c58xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 58, 0);
		t.Add(d, "c59xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 59, 0);
		t.Add(d, "c60xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 60, 0);
		t.Add(d, "c61xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 61, 0);
		t.Add(d, "c62xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 62, 0);
		t.Add(d, "c63xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 63, 0);
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.OK;
		RICON_FadingDescriptor expectedDescriptor = new RICON_FadingDescriptor();
		expectedDescriptor.m_sModelId = "FINITE_MULTIPATH_V1";
		expectedDescriptor.m_sProfileId = "pppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppp";
		expectedDescriptor.m_A = t.End(10, 0);
		expectedDescriptor.m_B = t.End(20, 0);
		expectedDescriptor.m_iProfileVersion = 1;
		expectedDescriptor.m_iEnvironmentRevision = 1;
		expectedDescriptor.m_iAnchorMs = 1000;
		expectedDescriptor.m_iValidFromMs = 1000;
		expectedDescriptor.m_iValidUntilMs = 5000;
		expectedDescriptor.m_bStaticChannel = true;
		expectedDescriptor.m_bCoverageComplete = true;
		expectedDescriptor.m_bResourceExhausted = false;
		expectedDescriptor.m_fFrequencyMHz = 150.0;
		expectedDescriptor.m_fBandwidthHz = 25000.0;
		t.Add(expectedDescriptor, "c00xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 0, 0);
		t.Add(expectedDescriptor, "c01xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 1, 0);
		t.Add(expectedDescriptor, "c02xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 2, 0);
		t.Add(expectedDescriptor, "c03xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 3, 0);
		t.Add(expectedDescriptor, "c04xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 4, 0);
		t.Add(expectedDescriptor, "c05xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 5, 0);
		t.Add(expectedDescriptor, "c06xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 6, 0);
		t.Add(expectedDescriptor, "c07xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 7, 0);
		t.Add(expectedDescriptor, "c08xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 8, 0);
		t.Add(expectedDescriptor, "c09xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 9, 0);
		t.Add(expectedDescriptor, "c10xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 10, 0);
		t.Add(expectedDescriptor, "c11xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 11, 0);
		t.Add(expectedDescriptor, "c12xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 12, 0);
		t.Add(expectedDescriptor, "c13xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 13, 0);
		t.Add(expectedDescriptor, "c14xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 14, 0);
		t.Add(expectedDescriptor, "c15xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 15, 0);
		t.Add(expectedDescriptor, "c16xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 16, 0);
		t.Add(expectedDescriptor, "c17xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 17, 0);
		t.Add(expectedDescriptor, "c18xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 18, 0);
		t.Add(expectedDescriptor, "c19xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 19, 0);
		t.Add(expectedDescriptor, "c20xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 20, 0);
		t.Add(expectedDescriptor, "c21xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 21, 0);
		t.Add(expectedDescriptor, "c22xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 22, 0);
		t.Add(expectedDescriptor, "c23xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 23, 0);
		t.Add(expectedDescriptor, "c24xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 24, 0);
		t.Add(expectedDescriptor, "c25xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 25, 0);
		t.Add(expectedDescriptor, "c26xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 26, 0);
		t.Add(expectedDescriptor, "c27xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 27, 0);
		t.Add(expectedDescriptor, "c28xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 28, 0);
		t.Add(expectedDescriptor, "c29xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 29, 0);
		t.Add(expectedDescriptor, "c30xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 30, 0);
		t.Add(expectedDescriptor, "c31xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 31, 0);
		t.Add(expectedDescriptor, "c32xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 32, 0);
		t.Add(expectedDescriptor, "c33xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 33, 0);
		t.Add(expectedDescriptor, "c34xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 34, 0);
		t.Add(expectedDescriptor, "c35xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 35, 0);
		t.Add(expectedDescriptor, "c36xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 36, 0);
		t.Add(expectedDescriptor, "c37xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 37, 0);
		t.Add(expectedDescriptor, "c38xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 38, 0);
		t.Add(expectedDescriptor, "c39xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 39, 0);
		t.Add(expectedDescriptor, "c40xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 40, 0);
		t.Add(expectedDescriptor, "c41xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 41, 0);
		t.Add(expectedDescriptor, "c42xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 42, 0);
		t.Add(expectedDescriptor, "c43xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 43, 0);
		t.Add(expectedDescriptor, "c44xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 44, 0);
		t.Add(expectedDescriptor, "c45xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 45, 0);
		t.Add(expectedDescriptor, "c46xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 46, 0);
		t.Add(expectedDescriptor, "c47xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 47, 0);
		t.Add(expectedDescriptor, "c48xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 48, 0);
		t.Add(expectedDescriptor, "c49xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 49, 0);
		t.Add(expectedDescriptor, "c50xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 50, 0);
		t.Add(expectedDescriptor, "c51xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 51, 0);
		t.Add(expectedDescriptor, "c52xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 52, 0);
		t.Add(expectedDescriptor, "c53xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 53, 0);
		t.Add(expectedDescriptor, "c54xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 54, 0);
		t.Add(expectedDescriptor, "c55xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 55, 0);
		t.Add(expectedDescriptor, "c56xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 56, 0);
		t.Add(expectedDescriptor, "c57xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 57, 0);
		t.Add(expectedDescriptor, "c58xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 58, 0);
		t.Add(expectedDescriptor, "c59xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 59, 0);
		t.Add(expectedDescriptor, "c60xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 60, 0);
		t.Add(expectedDescriptor, "c61xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 61, 0);
		t.Add(expectedDescriptor, "c62xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 62, 0);
		t.Add(expectedDescriptor, "c63xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 63, 0);
		expected.m_Descriptor = expectedDescriptor;
		array<int> expectedBytes = t.Hex("524346440113ca1346494e4954455f4d554c5449504154485f56314070707070707070707070707070707070707070707070707070707070707070707070707070707070707070707070707070707070707070707070707070707070000000010000000a0000000000000014000000004316000046c3500000000001000003e8000003e800001388034040633030787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000000000000040633031787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000010000000040633032787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000020000000040633033787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000030000000040633034787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000040000000040633035787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000050000000040633036787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000060000000040633037787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000070000000040633038787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000080000000040633039787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000090000000040633130787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f8000000000000a0000000040633131787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f8000000000000b0000000040633132787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f8000000000000c0000000040633133787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f8000000000000d0000000040633134787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f8000000000000e0000000040633135787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f8000000000000f0000000040633136787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000100000000040633137787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000110000000040633138787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000120000000040633139787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000130000000040633230787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000140000000040633231787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000150000000040633232787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000160000000040633233787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000170000000040633234787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000180000000040633235787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000190000000040633236787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f8000000000001a0000000040633237787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f8000000000001b0000000040633238787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f8000000000001c0000000040633239787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f8000000000001d0000000040633330787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f8000000000001e0000000040633331787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f8000000000001f0000000040633332787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000200000000040633333787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000210000000040633334787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000220000000040633335787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000230000000040633336787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000240000000040633337787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000250000000040633338787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000260000000040633339787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000270000000040633430787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000280000000040633431787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000290000000040633432787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f8000000000002a0000000040633433787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f8000000000002b0000000040633434787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f8000000000002c0000000040633435787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f8000000000002d0000000040633436787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f8000000000002e0000000040633437787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f8000000000002f0000000040633438787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000300000000040633439787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000310000000040633530787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000320000000040633531787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000330000000040633532787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000340000000040633533787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000350000000040633534787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000360000000040633535787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000370000000040633536787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000380000000040633537787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f800000000000390000000040633538787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f8000000000003a0000000040633539787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f8000000000003b0000000040633630787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f8000000000003c0000000040633631787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f8000000000003d0000000040633632787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f8000000000003e0000000040633633787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878783f8000000000003f00000000");
		expected.m_aBytes = expectedBytes;
		expected.m_iSizeBytes = 5066;
		t.CheckEncode("D13", d, 5066, expected);
	}

	protected static void Case13(RICON_ChannelDescriptorRun t)
	{
		RICON_FadingDescriptor d = new RICON_FadingDescriptor();
		d.m_sModelId = "FINITE_MULTIPATH_V1";
		d.m_sProfileId = "pppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppp";
		d.m_A = t.End(10, 0);
		d.m_B = t.End(20, 0);
		d.m_iProfileVersion = 1;
		d.m_iEnvironmentRevision = 1;
		d.m_iAnchorMs = 1000;
		d.m_iValidFromMs = 1000;
		d.m_iValidUntilMs = 5000;
		d.m_bStaticChannel = true;
		d.m_bCoverageComplete = true;
		d.m_bResourceExhausted = false;
		d.m_fFrequencyMHz = 150.0;
		d.m_fBandwidthHz = 25000.0;
		t.Add(d, "c00xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 0, 0);
		t.Add(d, "c01xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 1, 0);
		t.Add(d, "c02xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 2, 0);
		t.Add(d, "c03xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 3, 0);
		t.Add(d, "c04xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 4, 0);
		t.Add(d, "c05xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 5, 0);
		t.Add(d, "c06xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 6, 0);
		t.Add(d, "c07xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 7, 0);
		t.Add(d, "c08xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 8, 0);
		t.Add(d, "c09xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 9, 0);
		t.Add(d, "c10xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 10, 0);
		t.Add(d, "c11xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 11, 0);
		t.Add(d, "c12xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 12, 0);
		t.Add(d, "c13xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 13, 0);
		t.Add(d, "c14xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 14, 0);
		t.Add(d, "c15xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 15, 0);
		t.Add(d, "c16xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 16, 0);
		t.Add(d, "c17xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 17, 0);
		t.Add(d, "c18xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 18, 0);
		t.Add(d, "c19xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 19, 0);
		t.Add(d, "c20xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 20, 0);
		t.Add(d, "c21xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 21, 0);
		t.Add(d, "c22xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 22, 0);
		t.Add(d, "c23xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 23, 0);
		t.Add(d, "c24xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 24, 0);
		t.Add(d, "c25xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 25, 0);
		t.Add(d, "c26xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 26, 0);
		t.Add(d, "c27xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 27, 0);
		t.Add(d, "c28xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 28, 0);
		t.Add(d, "c29xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 29, 0);
		t.Add(d, "c30xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 30, 0);
		t.Add(d, "c31xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 31, 0);
		t.Add(d, "c32xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 32, 0);
		t.Add(d, "c33xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 33, 0);
		t.Add(d, "c34xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 34, 0);
		t.Add(d, "c35xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 35, 0);
		t.Add(d, "c36xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 36, 0);
		t.Add(d, "c37xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 37, 0);
		t.Add(d, "c38xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 38, 0);
		t.Add(d, "c39xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 39, 0);
		t.Add(d, "c40xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 40, 0);
		t.Add(d, "c41xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 41, 0);
		t.Add(d, "c42xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 42, 0);
		t.Add(d, "c43xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 43, 0);
		t.Add(d, "c44xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 44, 0);
		t.Add(d, "c45xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 45, 0);
		t.Add(d, "c46xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 46, 0);
		t.Add(d, "c47xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 47, 0);
		t.Add(d, "c48xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 48, 0);
		t.Add(d, "c49xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 49, 0);
		t.Add(d, "c50xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 50, 0);
		t.Add(d, "c51xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 51, 0);
		t.Add(d, "c52xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 52, 0);
		t.Add(d, "c53xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 53, 0);
		t.Add(d, "c54xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 54, 0);
		t.Add(d, "c55xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 55, 0);
		t.Add(d, "c56xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 56, 0);
		t.Add(d, "c57xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 57, 0);
		t.Add(d, "c58xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 58, 0);
		t.Add(d, "c59xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 59, 0);
		t.Add(d, "c60xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 60, 0);
		t.Add(d, "c61xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 61, 0);
		t.Add(d, "c62xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 62, 0);
		t.Add(d, "c63xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 1.0, 63, 0);
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.RESOURCE_LIMIT;
		t.CheckEncode("D14", d, 5065, expected);
	}

	protected static void Case14(RICON_ChannelDescriptorRun t)
	{
		RICON_FadingDescriptor d = new RICON_FadingDescriptor();
		d.m_sModelId = "FINITE_MULTIPATH_V1";
		d.m_sProfileId = "TEST_ONLY";
		d.m_A = t.End(10, 0);
		d.m_B = t.End(20, 0);
		d.m_iProfileVersion = 1;
		d.m_iEnvironmentRevision = 1;
		d.m_iAnchorMs = 1000;
		d.m_iValidFromMs = 1000;
		d.m_iValidUntilMs = 5000;
		d.m_bStaticChannel = true;
		d.m_bCoverageComplete = true;
		d.m_bResourceExhausted = false;
		d.m_fFrequencyMHz = 150.0;
		d.m_fBandwidthHz = 25000.0;
		t.Add(d, "direct", 1.0, 0, 0);
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.INVALID_LIMITS;
		t.CheckEncode("D15", d, 0, expected);
	}

	protected static void Case15(RICON_ChannelDescriptorRun t)
	{
		RICON_FadingDescriptor d = new RICON_FadingDescriptor();
		d.m_sModelId = "FINITE_MULTIPATH_V1";
		d.m_sProfileId = "TEST_ONLY";
		d.m_A = t.End(10, 0);
		d.m_B = t.End(20, 0);
		d.m_iProfileVersion = 1;
		d.m_iEnvironmentRevision = 1;
		d.m_iAnchorMs = 1000;
		d.m_iValidFromMs = 1000;
		d.m_iValidUntilMs = 5000;
		d.m_bStaticChannel = true;
		d.m_bCoverageComplete = true;
		d.m_bResourceExhausted = false;
		d.m_fFrequencyMHz = 150.0;
		d.m_fBandwidthHz = 25000.0;
		t.Add(d, "direct", 1.0, 0, 0);
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.INVALID_LIMITS;
		t.CheckEncode("D16", d, 65536, expected);
	}

	protected static void Case16(RICON_ChannelDescriptorRun t)
	{
		RICON_FadingDescriptor d = new RICON_FadingDescriptor();
		d.m_sModelId = "FINITE_MULTIPATH_V1";
		d.m_sProfileId = "TEST_ONLY";
		d.m_A = t.End(10, 0);
		d.m_B = t.End(20, 0);
		d.m_iProfileVersion = 1;
		d.m_iEnvironmentRevision = 1;
		d.m_iAnchorMs = 1000;
		d.m_iValidFromMs = 1000;
		d.m_iValidUntilMs = 5000;
		d.m_bStaticChannel = true;
		d.m_bCoverageComplete = true;
		d.m_bResourceExhausted = false;
		d.m_fFrequencyMHz = 150.0;
		d.m_fBandwidthHz = 25000.0;
		t.Add(d, "direct", 1.0, 0, 1);
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.INVALID_DESCRIPTOR;
		t.CheckEncode("D22", d, 5120, expected);
	}

	protected static void Case17(RICON_ChannelDescriptorRun t)
	{
		RICON_FadingDescriptor d = new RICON_FadingDescriptor();
		d.m_sModelId = "FINITE_MULTIPATH_V1";
		d.m_sProfileId = "TEST_ONLY";
		d.m_A = t.End(10, 0);
		d.m_B = t.End(20, 0);
		d.m_iProfileVersion = 1;
		d.m_iEnvironmentRevision = 1;
		d.m_iAnchorMs = 1000;
		d.m_iValidFromMs = 1000;
		d.m_iValidUntilMs = 5000;
		d.m_bStaticChannel = true;
		d.m_bCoverageComplete = true;
		d.m_bResourceExhausted = false;
		d.m_fFrequencyMHz = 150.0;
		d.m_fBandwidthHz = 25000.0;
		t.Add(d, "b", 1.0, 0, 0);
		t.Add(d, "a", 1.0, 0, 0);
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.INVALID_DESCRIPTOR;
		t.CheckEncode("D23", d, 5120, expected);
	}

	protected static void Case18(RICON_ChannelDescriptorRun t)
	{
		array<int> bytes = t.Hex("004346440100661346494e4954455f4d554c5449504154485f563109544553545f4f4e4c59000000010000000a0000000000000014000000004316000046c3500000000001000003e8000003e8000013880301066469726563743f8000000000000000000000");
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.INVALID_FRAME;
		t.CheckDecode("D24", bytes, 5120, expected);
	}

	protected static void Case19(RICON_ChannelDescriptorRun t)
	{
		array<int> bytes = t.Hex("524346440200661346494e4954455f4d554c5449504154485f563109544553545f4f4e4c59000000010000000a0000000000000014000000004316000046c3500000000001000003e8000003e8000013880301066469726563743f8000000000000000000000");
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.UNSUPPORTED_VERSION;
		t.CheckDecode("D25", bytes, 5120, expected);
	}

	protected static void Case20(RICON_ChannelDescriptorRun t)
	{
		array<int> bytes = t.Hex("524346440100661346494e4954455f4d554c5449504154485f563109544553545f4f4e4c59000000010000000a0000000000000014000000004316000046c3500000000001000003e8000003e8000013880301066469726563743f80000000000000000000");
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.INVALID_FRAME;
		t.CheckDecode("D26", bytes, 5120, expected);
	}

	protected static void Case21(RICON_ChannelDescriptorRun t)
	{
		array<int> bytes = t.Hex("524346440100651346494e4954455f4d554c5449504154485f563109544553545f4f4e4c59000000010000000a0000000000000014000000004316000046c3500000000001000003e8000003e8000013880301066469726563743f80000000000000000000");
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.INVALID_FRAME;
		t.CheckDecode("D27", bytes, 5120, expected);
	}

	protected static void Case22(RICON_ChannelDescriptorRun t)
	{
		array<int> bytes = t.Hex("524346440100671346494e4954455f4d554c5449504154485f563109544553545f4f4e4c59000000010000000a0000000000000014000000004316000046c3500000000001000003e8000003e8000013880301066469726563743f800000000000000000000000");
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.INVALID_FRAME;
		t.CheckDecode("D28", bytes, 5120, expected);
	}

	protected static void Case23(RICON_ChannelDescriptorRun t)
	{
		array<int> bytes = t.Hex("524346440100661346494e4954455f4d554c5449504154485f563109544553545f4f4e4c59000000010000000a0000000000000014000000004316000046c3500000000001000003e8000003e8000013880301066469726563743f8000000000000000000000");
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.RESOURCE_LIMIT;
		t.CheckDecode("D29", bytes, 101, expected);
	}

	protected static void Case24(RICON_ChannelDescriptorRun t)
	{
		array<int> bytes = {82, 67, 70, 68, 1, 0, 102, 19, 256, 73, 78, 73, 84, 69, 95, 77, 85, 76, 84, 73, 80, 65, 84, 72, 95, 86, 49, 9, 84, 69, 83, 84, 95, 79, 78, 76, 89, 0, 0, 0, 1, 0, 0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 20, 0, 0, 0, 0, 67, 22, 0, 0, 70, 195, 80, 0, 0, 0, 0, 1, 0, 0, 3, 232, 0, 0, 3, 232, 0, 0, 19, 136, 3, 1, 6, 100, 105, 114, 101, 99, 116, 63, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.INVALID_FRAME;
		t.CheckDecode("D30", bytes, 5120, expected);
	}

	protected static void Case25(RICON_ChannelDescriptorRun t)
	{
		array<int> bytes = t.Hex("524346440100660046494e4954455f4d554c5449504154485f563109544553545f4f4e4c59000000010000000a0000000000000014000000004316000046c3500000000001000003e8000003e8000013880301066469726563743f8000000000000000000000");
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.INVALID_FRAME;
		t.CheckDecode("D32", bytes, 5120, expected);
	}

	protected static void Case26(RICON_ChannelDescriptorRun t)
	{
		array<int> bytes = t.Hex("5243464401006613ff494e4954455f4d554c5449504154485f563109544553545f4f4e4c59000000010000000a0000000000000014000000004316000046c3500000000001000003e8000003e8000013880301066469726563743f8000000000000000000000");
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.INVALID_FRAME;
		t.CheckDecode("D33", bytes, 5120, expected);
	}

	protected static void Case27(RICON_ChannelDescriptorRun t)
	{
		array<int> bytes = t.Hex("524346440100661346494e4954455f4d554c5449504154485f563109544553545f4f4e4c59000000010000000a0000000000000014000000004316000046c3500000000001000003e8000003e8000013880b01066469726563743f8000000000000000000000");
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.INVALID_FRAME;
		t.CheckDecode("D34", bytes, 5120, expected);
	}

	protected static void Case28(RICON_ChannelDescriptorRun t)
	{
		array<int> bytes = t.Hex("524346440100661346494e4954455f4d554c5449504154485f563109544553545f4f4e4c59000000010000000a0000000000000014000000004316000046c3500000000001000003e8000003e8000013880341066469726563743f8000000000000000000000");
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.INVALID_FRAME;
		t.CheckDecode("D35", bytes, 5120, expected);
	}

	protected static void Case29(RICON_ChannelDescriptorRun t)
	{
		array<int> bytes = t.Hex("524346440100661346494e4954455f4d554c5449504154485f563109544553545f4f4e4c59000000010000000a0000000000000014000000004316000046c3500000000001000003e8000003e8000013880301066469726563747fc000000000000000000000");
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.INVALID_FRAME;
		t.CheckDecode("D36", bytes, 5120, expected);
	}

	protected static void Case30(RICON_ChannelDescriptorRun t)
	{
		array<int> bytes = t.Hex("524346440100661346494e4954455f4d554c5449504154485f563109544553545f4f4e4c59000000010000000a0000000000000014000000007f80000046c3500000000001000003e8000003e8000013880301066469726563743f8000000000000000000000");
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.INVALID_FRAME;
		t.CheckDecode("D37", bytes, 5120, expected);
	}

	protected static void Case31(RICON_ChannelDescriptorRun t)
	{
		array<int> bytes = t.Hex("524346440100661346494e4954455f4d554c5449504154485f563109544553545f4f4e4c59000000010000000a0000000000000014000000004316000046c3500000000001000003e8000003e800001388030106646972656374800000000000000000000000");
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.INVALID_FRAME;
		t.CheckDecode("D38", bytes, 5120, expected);
	}

	protected static void Case32(RICON_ChannelDescriptorRun t)
	{
		array<int> bytes = t.Hex("524346440100661358494e4954455f4d554c5449504154485f563109544553545f4f4e4c59000000010000000a0000000000000014000000004316000046c3500000000001000003e8000003e8000013880301066469726563743f8000000000000000000000");
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.UNSUPPORTED_MODEL;
		t.CheckDecode("D39", bytes, 5120, expected);
	}

	protected static void Case33(RICON_ChannelDescriptorRun t)
	{
		RICON_FadingDescriptor d = new RICON_FadingDescriptor();
		d.m_sModelId = "FINITE_MULTIPATH_V1";
		d.m_sProfileId = "TEST_ONLY";
		d.m_A = t.End(10, 0);
		d.m_B = t.End(20, 0);
		d.m_iProfileVersion = 1;
		d.m_iEnvironmentRevision = 1;
		d.m_iAnchorMs = 1000;
		d.m_iValidFromMs = 1200;
		d.m_iValidUntilMs = 5000;
		d.m_bStaticChannel = false;
		d.m_bCoverageComplete = true;
		d.m_bResourceExhausted = false;
		d.m_fFrequencyMHz = 150.0;
		d.m_fBandwidthHz = 25000.0;
		t.Add(d, "a", 1.0, 123, (-2147483647 - 1));
		t.Add(d, "b", 0.25, 268435456, -12345);
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.OK;
		RICON_FadingDescriptor expectedDescriptor = new RICON_FadingDescriptor();
		expectedDescriptor.m_sModelId = "FINITE_MULTIPATH_V1";
		expectedDescriptor.m_sProfileId = "TEST_ONLY";
		expectedDescriptor.m_A = t.End(10, 0);
		expectedDescriptor.m_B = t.End(20, 0);
		expectedDescriptor.m_iProfileVersion = 1;
		expectedDescriptor.m_iEnvironmentRevision = 1;
		expectedDescriptor.m_iAnchorMs = 1000;
		expectedDescriptor.m_iValidFromMs = 1200;
		expectedDescriptor.m_iValidUntilMs = 5000;
		expectedDescriptor.m_bStaticChannel = false;
		expectedDescriptor.m_bCoverageComplete = true;
		expectedDescriptor.m_bResourceExhausted = false;
		expectedDescriptor.m_fFrequencyMHz = 150.0;
		expectedDescriptor.m_fBandwidthHz = 25000.0;
		t.Add(expectedDescriptor, "a", 1.0, 123, (-2147483647 - 1));
		t.Add(expectedDescriptor, "b", 0.25, 268435456, -12345);
		expected.m_Descriptor = expectedDescriptor;
		array<int> expectedBytes = t.Hex("5243464401006f1346494e4954455f4d554c5449504154485f563109544553545f4f4e4c59000000010000000a0000000000000014000000004316000046c3500000000001000003e8000004b000001388020201613f8000000000007b8000000001623e80000010000000ffffcfc7");
		expected.m_aBytes = expectedBytes;
		expected.m_iSizeBytes = 111;
		t.CheckEncode("D40", d, 5120, expected);
	}

	protected static void Case34(RICON_ChannelDescriptorRun t)
	{
		array<int> bytes = t.Hex("5243464401006f1346494e4954455f4d554c5449504154485f563109544553545f4f4e4c59000000010000000a0000000000000014000000004316000046c3500000000001000003e8000004b000001388020201613f8000000000007b8000000001623e80000010000000ffffcfc7");
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.OK;
		RICON_FadingDescriptor expectedDescriptor = new RICON_FadingDescriptor();
		expectedDescriptor.m_sModelId = "FINITE_MULTIPATH_V1";
		expectedDescriptor.m_sProfileId = "TEST_ONLY";
		expectedDescriptor.m_A = t.End(10, 0);
		expectedDescriptor.m_B = t.End(20, 0);
		expectedDescriptor.m_iProfileVersion = 1;
		expectedDescriptor.m_iEnvironmentRevision = 1;
		expectedDescriptor.m_iAnchorMs = 1000;
		expectedDescriptor.m_iValidFromMs = 1200;
		expectedDescriptor.m_iValidUntilMs = 5000;
		expectedDescriptor.m_bStaticChannel = false;
		expectedDescriptor.m_bCoverageComplete = true;
		expectedDescriptor.m_bResourceExhausted = false;
		expectedDescriptor.m_fFrequencyMHz = 150.0;
		expectedDescriptor.m_fBandwidthHz = 25000.0;
		t.Add(expectedDescriptor, "a", 1.0, 123, (-2147483647 - 1));
		t.Add(expectedDescriptor, "b", 0.25, 268435456, -12345);
		expected.m_Descriptor = expectedDescriptor;
		array<int> expectedBytes = t.Hex("5243464401006f1346494e4954455f4d554c5449504154485f563109544553545f4f4e4c59000000010000000a0000000000000014000000004316000046c3500000000001000003e8000004b000001388020201613f8000000000007b8000000001623e80000010000000ffffcfc7");
		expected.m_aBytes = expectedBytes;
		expected.m_iSizeBytes = 111;
		t.CheckDecode("D41", bytes, 5120, expected);
	}

	protected static void Case35(RICON_ChannelDescriptorRun t)
	{
		RICON_FadingDescriptor d = new RICON_FadingDescriptor();
		d.m_sModelId = "FINITE_MULTIPATH_V1";
		d.m_sProfileId = "TEST_ONLY";
		d.m_A = t.End(10, 0);
		d.m_B = t.End(20, 0);
		d.m_iProfileVersion = 1;
		d.m_iEnvironmentRevision = 1;
		d.m_iAnchorMs = 1000;
		d.m_iValidFromMs = 1200;
		d.m_iValidUntilMs = 5000;
		d.m_bStaticChannel = false;
		d.m_bCoverageComplete = true;
		d.m_bResourceExhausted = false;
		d.m_fFrequencyMHz = 150.0;
		d.m_fBandwidthHz = 25000.0;
		t.Add(d, "a", 1.0, 123, 11117);
		t.Add(d, "b", 0.25, 268435456, -12345);
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.OK;
		RICON_FadingDescriptor expectedDescriptor = new RICON_FadingDescriptor();
		expectedDescriptor.m_sModelId = "FINITE_MULTIPATH_V1";
		expectedDescriptor.m_sProfileId = "TEST_ONLY";
		expectedDescriptor.m_A = t.End(10, 0);
		expectedDescriptor.m_B = t.End(20, 0);
		expectedDescriptor.m_iProfileVersion = 1;
		expectedDescriptor.m_iEnvironmentRevision = 1;
		expectedDescriptor.m_iAnchorMs = 1200;
		expectedDescriptor.m_iValidFromMs = 1200;
		expectedDescriptor.m_iValidUntilMs = 5000;
		expectedDescriptor.m_bStaticChannel = false;
		expectedDescriptor.m_bCoverageComplete = true;
		expectedDescriptor.m_bResourceExhausted = false;
		expectedDescriptor.m_fFrequencyMHz = 150.0;
		expectedDescriptor.m_fBandwidthHz = 25000.0;
		t.Add(expectedDescriptor, "a", 1.0, 75916475, 11117);
		t.Add(expectedDescriptor, "b", 0.25, 1020660928, -12345);
		expected.m_Descriptor = expectedDescriptor;
		t.CheckRebase("D42", d, 1200, expected);
	}

	protected static void Case36(RICON_ChannelDescriptorRun t)
	{
		RICON_FadingDescriptor d = new RICON_FadingDescriptor();
		d.m_sModelId = "FINITE_MULTIPATH_V1";
		d.m_sProfileId = "TEST_ONLY";
		d.m_A = t.End(10, 0);
		d.m_B = t.End(20, 0);
		d.m_iProfileVersion = 1;
		d.m_iEnvironmentRevision = 1;
		d.m_iAnchorMs = 1000;
		d.m_iValidFromMs = 1200;
		d.m_iValidUntilMs = 5000;
		d.m_bStaticChannel = false;
		d.m_bCoverageComplete = true;
		d.m_bResourceExhausted = false;
		d.m_fFrequencyMHz = 150.0;
		d.m_fBandwidthHz = 25000.0;
		t.Add(d, "a", 1.0, 123, 11117);
		t.Add(d, "b", 0.25, 268435456, -12345);
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.OK;
		RICON_FadingDescriptor expectedDescriptor = new RICON_FadingDescriptor();
		expectedDescriptor.m_sModelId = "FINITE_MULTIPATH_V1";
		expectedDescriptor.m_sProfileId = "TEST_ONLY";
		expectedDescriptor.m_A = t.End(10, 0);
		expectedDescriptor.m_B = t.End(20, 0);
		expectedDescriptor.m_iProfileVersion = 1;
		expectedDescriptor.m_iEnvironmentRevision = 1;
		expectedDescriptor.m_iAnchorMs = 1234;
		expectedDescriptor.m_iValidFromMs = 1234;
		expectedDescriptor.m_iValidUntilMs = 5000;
		expectedDescriptor.m_bStaticChannel = false;
		expectedDescriptor.m_bCoverageComplete = true;
		expectedDescriptor.m_bResourceExhausted = false;
		expectedDescriptor.m_fFrequencyMHz = 150.0;
		expectedDescriptor.m_fBandwidthHz = 25000.0;
		t.Add(expectedDescriptor, "a", 1.0, 453894475, 11117);
		t.Add(expectedDescriptor, "b", 0.25, 600930928, -12345);
		expected.m_Descriptor = expectedDescriptor;
		t.CheckRebase("D43", d, 1234, expected);
	}

	protected static void Case37(RICON_ChannelDescriptorRun t)
	{
		RICON_FadingDescriptor d = new RICON_FadingDescriptor();
		d.m_sModelId = "FINITE_MULTIPATH_V1";
		d.m_sProfileId = "TEST_ONLY";
		d.m_A = t.End(10, 0);
		d.m_B = t.End(20, 0);
		d.m_iProfileVersion = 1;
		d.m_iEnvironmentRevision = 1;
		d.m_iAnchorMs = 1000;
		d.m_iValidFromMs = 1200;
		d.m_iValidUntilMs = 5000;
		d.m_bStaticChannel = false;
		d.m_bCoverageComplete = true;
		d.m_bResourceExhausted = false;
		d.m_fFrequencyMHz = 150.0;
		d.m_fBandwidthHz = 25000.0;
		t.Add(d, "a", 1.0, 123, 11117);
		t.Add(d, "b", 0.25, 268435456, -12345);
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.OK;
		RICON_FadingDescriptor expectedDescriptor = new RICON_FadingDescriptor();
		expectedDescriptor.m_sModelId = "FINITE_MULTIPATH_V1";
		expectedDescriptor.m_sProfileId = "TEST_ONLY";
		expectedDescriptor.m_A = t.End(10, 0);
		expectedDescriptor.m_B = t.End(20, 0);
		expectedDescriptor.m_iProfileVersion = 1;
		expectedDescriptor.m_iEnvironmentRevision = 1;
		expectedDescriptor.m_iAnchorMs = 4999;
		expectedDescriptor.m_iValidFromMs = 4999;
		expectedDescriptor.m_iValidUntilMs = 5000;
		expectedDescriptor.m_bStaticChannel = false;
		expectedDescriptor.m_bCoverageComplete = true;
		expectedDescriptor.m_bResourceExhausted = false;
		expectedDescriptor.m_fFrequencyMHz = 150.0;
		expectedDescriptor.m_fBandwidthHz = 25000.0;
		t.Add(expectedDescriptor, "a", 1.0, 433468339, 11117);
		t.Add(expectedDescriptor, "b", 0.25, 292904360, -12345);
		expected.m_Descriptor = expectedDescriptor;
		t.CheckRebase("D44", d, 4999, expected);
	}

	protected static void Case38(RICON_ChannelDescriptorRun t)
	{
		RICON_FadingDescriptor d = new RICON_FadingDescriptor();
		d.m_sModelId = "FINITE_MULTIPATH_V1";
		d.m_sProfileId = "TEST_ONLY";
		d.m_A = t.End(10, 0);
		d.m_B = t.End(20, 0);
		d.m_iProfileVersion = 1;
		d.m_iEnvironmentRevision = 1;
		d.m_iAnchorMs = 1000;
		d.m_iValidFromMs = 1200;
		d.m_iValidUntilMs = 5000;
		d.m_bStaticChannel = false;
		d.m_bCoverageComplete = true;
		d.m_bResourceExhausted = false;
		d.m_fFrequencyMHz = 150.0;
		d.m_fBandwidthHz = 25000.0;
		t.Add(d, "a", 1.0, 123, 11117);
		t.Add(d, "b", 0.25, 268435456, -12345);
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.INVALID_ANCHOR;
		t.CheckRebase("D45", d, 1199, expected);
	}

	protected static void Case39(RICON_ChannelDescriptorRun t)
	{
		RICON_FadingDescriptor d = new RICON_FadingDescriptor();
		d.m_sModelId = "FINITE_MULTIPATH_V1";
		d.m_sProfileId = "TEST_ONLY";
		d.m_A = t.End(10, 0);
		d.m_B = t.End(20, 0);
		d.m_iProfileVersion = 1;
		d.m_iEnvironmentRevision = 1;
		d.m_iAnchorMs = 1000;
		d.m_iValidFromMs = 1200;
		d.m_iValidUntilMs = 5000;
		d.m_bStaticChannel = false;
		d.m_bCoverageComplete = true;
		d.m_bResourceExhausted = false;
		d.m_fFrequencyMHz = 150.0;
		d.m_fBandwidthHz = 25000.0;
		t.Add(d, "a", 1.0, 123, 11117);
		t.Add(d, "b", 0.25, 268435456, -12345);
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.INVALID_ANCHOR;
		t.CheckRebase("D46", d, 5000, expected);
	}

	protected static void Case40(RICON_ChannelDescriptorRun t)
	{
		RICON_FadingDescriptor d = new RICON_FadingDescriptor();
		d.m_sModelId = "FINITE_MULTIPATH_V1";
		d.m_sProfileId = "TEST_ONLY";
		d.m_A = t.End(10, 0);
		d.m_B = t.End(20, 0);
		d.m_iProfileVersion = 1;
		d.m_iEnvironmentRevision = 1;
		d.m_iAnchorMs = 1000;
		d.m_iValidFromMs = 1200;
		d.m_iValidUntilMs = 5000;
		d.m_bStaticChannel = false;
		d.m_bCoverageComplete = true;
		d.m_bResourceExhausted = false;
		d.m_fFrequencyMHz = 150.0;
		d.m_fBandwidthHz = 25000.0;
		t.Add(d, "a", 1.0, 123, (-2147483647 - 1));
		t.Add(d, "b", 0.25, 268435456, -12345);
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.OUT_OF_MODEL_DOMAIN;
		t.CheckRebase("D48", d, -1, expected);
	}

	protected static void Case41(RICON_ChannelDescriptorRun t)
	{
		RICON_FadingDescriptor d = new RICON_FadingDescriptor();
		d.m_sModelId = "FINITE_MULTIPATH_V1";
		d.m_sProfileId = "TEST_ONLY";
		d.m_A = t.End(10, 0);
		d.m_B = t.End(20, 0);
		d.m_iProfileVersion = 1;
		d.m_iEnvironmentRevision = 1;
		d.m_iAnchorMs = 0;
		d.m_iValidFromMs = 0;
		d.m_iValidUntilMs = 2147483647;
		d.m_bStaticChannel = false;
		d.m_bCoverageComplete = true;
		d.m_bResourceExhausted = false;
		d.m_fFrequencyMHz = 150.0;
		d.m_fBandwidthHz = 25000.0;
		t.Add(d, "a", 1.0, 123, 11117);
		t.Add(d, "b", 0.25, 268435456, -12345);
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.OK;
		RICON_FadingDescriptor expectedDescriptor = new RICON_FadingDescriptor();
		expectedDescriptor.m_sModelId = "FINITE_MULTIPATH_V1";
		expectedDescriptor.m_sProfileId = "TEST_ONLY";
		expectedDescriptor.m_A = t.End(10, 0);
		expectedDescriptor.m_B = t.End(20, 0);
		expectedDescriptor.m_iProfileVersion = 1;
		expectedDescriptor.m_iEnvironmentRevision = 1;
		expectedDescriptor.m_iAnchorMs = 2147483646;
		expectedDescriptor.m_iValidFromMs = 2147483646;
		expectedDescriptor.m_iValidUntilMs = 2147483647;
		expectedDescriptor.m_bStaticChannel = false;
		expectedDescriptor.m_bCoverageComplete = true;
		expectedDescriptor.m_bResourceExhausted = false;
		expectedDescriptor.m_fFrequencyMHz = 150.0;
		expectedDescriptor.m_fBandwidthHz = 25000.0;
		t.Add(expectedDescriptor, "a", 1.0, 1051507947, 11117);
		t.Add(expectedDescriptor, "b", 0.25, 293125456, -12345);
		expected.m_Descriptor = expectedDescriptor;
		t.CheckRebase("D49", d, 2147483646, expected);
	}

	protected static void Case42(RICON_ChannelDescriptorRun t)
	{
		RICON_FadingDescriptor d = new RICON_FadingDescriptor();
		d.m_sModelId = "FINITE_MULTIPATH_V1";
		d.m_sProfileId = "TEST_ONLY";
		d.m_A = t.End(10, 0);
		d.m_B = t.End(20, 0);
		d.m_iProfileVersion = 1;
		d.m_iEnvironmentRevision = 1;
		d.m_iAnchorMs = 1000;
		d.m_iValidFromMs = 1000;
		d.m_iValidUntilMs = 5000;
		d.m_bStaticChannel = true;
		d.m_bCoverageComplete = true;
		d.m_bResourceExhausted = false;
		d.m_fFrequencyMHz = 150.0;
		d.m_fBandwidthHz = 25000.0;
		t.Add(d, "direct", 1.0, 0, 0);
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.OK;
		RICON_FadingDescriptor expectedDescriptor = new RICON_FadingDescriptor();
		expectedDescriptor.m_sModelId = "FINITE_MULTIPATH_V1";
		expectedDescriptor.m_sProfileId = "TEST_ONLY";
		expectedDescriptor.m_A = t.End(10, 0);
		expectedDescriptor.m_B = t.End(20, 0);
		expectedDescriptor.m_iProfileVersion = 1;
		expectedDescriptor.m_iEnvironmentRevision = 1;
		expectedDescriptor.m_iAnchorMs = 4000;
		expectedDescriptor.m_iValidFromMs = 4000;
		expectedDescriptor.m_iValidUntilMs = 5000;
		expectedDescriptor.m_bStaticChannel = true;
		expectedDescriptor.m_bCoverageComplete = true;
		expectedDescriptor.m_bResourceExhausted = false;
		expectedDescriptor.m_fFrequencyMHz = 150.0;
		expectedDescriptor.m_fBandwidthHz = 25000.0;
		t.Add(expectedDescriptor, "direct", 1.0, 0, 0);
		expected.m_Descriptor = expectedDescriptor;
		t.CheckRebase("D50", d, 4000, expected);
	}

	protected static void Case43(RICON_ChannelDescriptorRun t)
	{
		RICON_FadingDescriptor d;
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.INVALID_DESCRIPTOR;
		t.CheckEncode("D51", d, 5120, expected);
	}

	protected static void Case44(RICON_ChannelDescriptorRun t)
	{
		array<int> bytes;
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.INVALID_FRAME;
		t.CheckDecode("D52", bytes, 5120, expected);
	}

	protected static void Case45(RICON_ChannelDescriptorRun t)
	{
		array<int> bytes = t.Hex("5243");
		RICON_DescriptorResult expected = new RICON_DescriptorResult();
		expected.m_eCode = RICON_EDescriptorCode.INVALID_FRAME;
		t.CheckDecode("D53", bytes, 5120, expected);
	}

	static void Run(RICON_ChannelDescriptorRun t)
	{
		Case0(t);
		Case1(t);
		Case2(t);
		Case3(t);
		Case4(t);
		Case5(t);
		Case6(t);
		Case7(t);
		Case8(t);
		Case9(t);
		Case10(t);
		Case11(t);
		Case12(t);
		Case13(t);
		Case14(t);
		Case15(t);
		Case16(t);
		Case17(t);
		Case18(t);
		Case19(t);
		Case20(t);
		Case21(t);
		Case22(t);
		Case23(t);
		Case24(t);
		Case25(t);
		Case26(t);
		Case27(t);
		Case28(t);
		Case29(t);
		Case30(t);
		Case31(t);
		Case32(t);
		Case33(t);
		Case34(t);
		Case35(t);
		Case36(t);
		Case37(t);
		Case38(t);
		Case39(t);
		Case40(t);
		Case41(t);
		Case42(t);
		Case43(t);
		Case44(t);
		Case45(t);
	}
}
