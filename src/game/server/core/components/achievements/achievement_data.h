/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_CORE_COMPONENTS_ACHIEVEMENT_DATA_H
#define GAME_SERVER_CORE_COMPONENTS_ACHIEVEMENT_DATA_H

#define TW_ACHIEVEMENTS "tw_achievements"
#define TW_ACCOUNTS_ACHIEVEMENTS "tw_accounts_achievements"

// forward declaration
class CGS;
class CPlayer;
class CAchievement;

enum AchievementProgressType
{
	PROGRESS_ADD = 0,
	PROGRESS_REMOVE,
	PROGRESS_SET,
};

// achievement types
enum AchievementType
{
	ACHIEVEMENT_DEFEAT_PVP = 0,
	ACHIEVEMENT_DEFEAT_PVE,
	ACHIEVEMENT_DEFEAT_MOB,
	ACHIEVEMENT_DEATH,
	ACHIEVEMENT_TOTAL_DAMAGE,
	ACHIEVEMENT_EQUIP,
	ACHIEVEMENT_RECEIVE_ITEM,
	ACHIEVEMENT_HAVE_ITEM,
	ACHIEVEMENT_CRAFT_ITEM,
	ACHIEVEMENT_UNLOCK_WORLD,
	ACHIEVEMENT_LEVELING,
};

// achievement information data
class CAchievementInfo : public MultiworldIdentifiableStaticData< std::deque<CAchievementInfo*> >
{
	int m_ID {};
	int m_Type {};
	std::string m_aName {};
	std::string m_aDescription {};
	int m_Misc {NOPE};
	int m_MiscRequired {};

public:
	explicit CAchievementInfo(int ID) : m_ID(ID) {}

	static CAchievementInfo* CreateElement(const int& ID)
	{
		auto pData = new CAchievementInfo(ID);
		pData->m_ID = ID;
		return m_pData.emplace_back(pData);
	}

	int GetID() const { return m_ID; }
	int GetMisc() const { return m_Misc; }
	int GetMiscRequired() const { return m_MiscRequired; }
	const char* GetName() const { return m_aName.c_str(); }
	const char* GetDescription() const { return m_aDescription.c_str(); }
	int GetType() const { return m_Type; }

	// initalize the Aether data
	void Init(const std::string& pName, const std::string& pDescription, int Type, const std::string& Data)
	{
		m_aName = pName;
		m_aDescription = pDescription;
		m_Type = Type;

		InitCriteriaJson(Data);
	}
	void InitCriteriaJson(const std::string& JsonData);

	// check if the achievement is completed
	bool CheckAchievement(int Value, const CAchievement* pAchievement) const;
};

// achievement data
class CAchievement : public MultiworldIdentifiableStaticData< std::map< int, ska::unordered_map<int, CAchievement* > > >
{
	friend class CAchievementInfo;
	friend class CAchievementManager;

	CGS* GS() const;
	CPlayer* GetPlayer() const;

	CAchievementInfo* m_pInfo {};
	int m_ClientID {};
	int m_Progress {};
	bool m_Completed {};

public:
	explicit CAchievement(CAchievementInfo* pInfo, int ClientID) : m_pInfo(pInfo), m_ClientID(ClientID) {}

	static CAchievement* CreateElement(CAchievementInfo* pInfo, int ClientID)
	{
		const auto pData = new CAchievement(pInfo, ClientID);
		return m_pData[ClientID][pInfo->GetID()] = pData;
	}

	// initalize the Aether data
	void Init(int Progress, bool Completed)
	{
		m_Progress = Progress;
		m_Completed = Completed;
	}

	CAchievementInfo* Info() const { return m_pInfo; }
	bool IsCompleted() const { return m_Completed; }

	bool UpdateProgress(int Misc, int Value, int ProgressType);
};


#endif