//! Exact server-local routing tuple. Encryption separates control channels, not physical RF energy.
class RICON_GameRadioControlChannel
{
	protected int m_iRawTune, m_iChannelId;
	protected string m_sKey;
	void RICON_GameRadioControlChannel(int rawTune, string key, int channelId)
	{
		m_iRawTune = rawTune; m_sKey = key; m_iChannelId = channelId;
	}
	bool Matches(int rawTune, string key) { return m_iRawTune == rawTune && m_sKey == key; }
	int ChannelId() { return m_iChannelId; }
}

//! Trusted server policy only. Raw tuning has no declared physical unit here; keys never leave this registry.
class RICON_GameRadioControlPolicy
{
	protected int m_iEpoch, m_iNetworkId, m_iMaxChannels, m_iChannelHighWater;
	protected string m_sIdentity;
	protected ref array<ref RICON_GameRadioControlChannel> m_Channels = {};
	protected ref map<int, ref array<ref RICON_GameRadioControlChannel>> m_ByTune = new map<int, ref array<ref RICON_GameRadioControlChannel>>();

	static bool IsValidIdentity(string identity)
	{
		if (!UUID.IsUUID(identity)) return false;
		UUID domain = identity;
		return !domain.IsNull() && domain.GetVersion() == 4;
	}

	bool Configure(int sessionEpoch, UUID publicDomain, int networkId, int maxChannels)
	{
		string identity = publicDomain;
		if (sessionEpoch <= 0 || sessionEpoch >= int.MAX || !IsValidIdentity(identity) || networkId <= 0 || maxChannels <= 0) return false;
		identity.ToLower();
		if (m_iEpoch)
			return m_iEpoch == sessionEpoch && m_sIdentity == identity && m_iNetworkId == networkId && m_iMaxChannels == maxChannels;
		m_iEpoch = sessionEpoch; m_sIdentity = identity; m_iNetworkId = networkId; m_iMaxChannels = maxChannels;
		return true;
	}

	string Identity() { return m_sIdentity; }
	int Revision()
	{
		if (m_iEpoch > 0)
		{
			return 1;
		}
		return 0;
	}
	int NetworkId() { return m_iNetworkId; }
	int ChannelCount() { return m_Channels.Count(); }

	bool Matches(int sessionEpoch, string identity, int revision, int networkId)
	{
		return m_iEpoch > 0 && sessionEpoch == m_iEpoch && identity == m_sIdentity && revision == 1 && networkId == m_iNetworkId;
	}

	int ReadChannel(int expectedEpoch, int nativeRawTune, string exactKey)
	{
		if (!m_iEpoch || expectedEpoch != m_iEpoch || nativeRawTune <= 0) return 0;
		array<ref RICON_GameRadioControlChannel> bucket = m_ByTune.Get(nativeRawTune);
		if (!bucket) return 0;
		foreach (RICON_GameRadioControlChannel channel : bucket)
			if (channel.Matches(nativeRawTune, exactKey)) return channel.ChannelId();
		return 0;
	}

	int Resolve(int expectedEpoch, int nativeRawTune, string exactKey)
	{
		array<int> tunes = {nativeRawTune};
		array<string> keys = {exactKey};
		array<int> channels = {};
		if (ResolveBatch(expectedEpoch, tunes, keys, channels) != "OK") return 0;
		return channels[0];
	}

	//! Preflight the complete trusted snapshot before allocating any new channel identities.
	string ResolveBatch(int expectedEpoch, array<int> nativeRawTunes, array<string> exactKeys, array<int> channelIds)
	{
		if (!channelIds) return "INVALID_TUNE";
		if (channelIds == nativeRawTunes) return "INVALID_TUNE";
		channelIds.Clear();
		if (!m_iEpoch || expectedEpoch != m_iEpoch) return "INVALID_SCOPE";
		if (!nativeRawTunes || !exactKeys || nativeRawTunes.Count() != exactKeys.Count()) return "INVALID_TUNE";
		array<int> newTunes = {};
		array<string> newKeys = {};
		for (int index = 0; index < nativeRawTunes.Count(); index++)
		{
			int tune = nativeRawTunes[index];
			if (tune <= 0) return "INVALID_TUNE";
			if (ReadChannel(expectedEpoch, tune, exactKeys[index]) > 0) continue;
			bool duplicate = false;
			for (int previous = 0; previous < newTunes.Count(); previous++)
				if (newTunes[previous] == tune && newKeys[previous] == exactKeys[index]) { duplicate = true; break; }
			if (!duplicate) { newTunes.Insert(tune); newKeys.Insert(exactKeys[index]); }
		}
		int count = newTunes.Count();
		if (count > m_iMaxChannels - m_Channels.Count() || m_iChannelHighWater > int.MAX - count) return "RESOURCE_LIMIT";
		for (int added = 0; added < count; added++)
		{
			m_iChannelHighWater++;
			RICON_GameRadioControlChannel channel = new RICON_GameRadioControlChannel(newTunes[added], newKeys[added], m_iChannelHighWater);
			array<ref RICON_GameRadioControlChannel> bucket = m_ByTune.Get(newTunes[added]);
			if (!bucket) { bucket = {}; m_ByTune.Set(newTunes[added], bucket); }
			m_Channels.Insert(channel); bucket.Insert(channel);
		}
		for (int row = 0; row < nativeRawTunes.Count(); row++)
			channelIds.Insert(ReadChannel(expectedEpoch, nativeRawTunes[row], exactKeys[row]));
		return "OK";
	}
}
