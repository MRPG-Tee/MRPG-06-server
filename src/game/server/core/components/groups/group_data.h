/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_CORE_COMPONENTS_GROUPS_GROUP_DATA
#define GAME_SERVER_CORE_COMPONENTS_GROUPS_GROUP_DATA

#define TW_GROUPS_TABLE "tw_groups"

using GroupIdentifier = int;
class GroupData : public MultiworldIdentifiableData< std::map< int, GroupData > >
{
	GroupIdentifier m_ID {};
	int m_LeaderUID {};
	int m_TeamColor {};
	ska::unordered_set<int> m_vAccountIds {};

public:
	GroupData() = default;

	static GroupData& CreateElement(GroupIdentifier ID) noexcept
	{
		m_pData[ID] = GroupData();
		m_pData[ID].m_ID = ID;
		return m_pData[ID];
	}

	// functions
	void Init(int LeaderUID, DBSet&& SetAccountIDs)
	{
		// initialize group list
		m_vAccountIds.reserve(MAX_GROUP_MEMBERS);
		for(auto& p : SetAccountIDs.GetDataItems())
		{
			if(int UID = str_toint(p.first.c_str()); UID > 0)
				m_vAccountIds.insert(UID);
		}

		// initialize variables
		m_LeaderUID = LeaderUID;
		m_pData[m_ID] = *this;
	}

	bool Add(int AccountID);
	bool Remove(int AccountID);
	void Disband();
	void ChangeOwner(int AccountID);
	void UpdateRandomColor();

	// getters
	int GetOnlineCount() const;
	bool HasAccountID(int AccountID) const { return m_vAccountIds.find(AccountID) != m_vAccountIds.end(); }
	bool IsFull() const { return (int)m_vAccountIds.size() >= (int)MAX_GROUP_MEMBERS; }
	int GetTeamColor() const { return m_TeamColor; }
	GroupIdentifier GetID() const { return m_ID; }
	int GetOwnerUID() const { return m_LeaderUID; }
	int IsLeader(int AccountID) const { return m_LeaderUID == AccountID; }
	const ska::unordered_set<int>& GetAccounts() const { return m_vAccountIds; }

private:
	void Save() const;
};

#endif