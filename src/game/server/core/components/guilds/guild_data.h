/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_CORE_COMPONENTS_GUILDS_GUILD_DATA_H
#define GAME_SERVER_CORE_COMPONENTS_GUILDS_GUILD_DATA_H
#include "guild_house_data.h"
#include "guild_war_data.h"

#define TW_GUILDS_TABLE "tw_guilds"
#define TW_GUILDS_RANKS_TABLE "tw_guilds_ranks"
#define TW_GUILDS_HISTORY_TABLE "tw_guilds_history"
#define TW_GUILDS_INVITES_TABLE "tw_guilds_invites"

// Forward declaration and alias
class CGuildWarData;
using GuildIdentifier = int;
using GuildRankIdentifier = int;

// Enum for various guild limits and default values
enum GuildMisc
{
	GUILD_MAX_SLOTS = 20,                // Max number of members in a guild
	GUILD_RANKS_MAX_COUNT = 5,           // Max number of ranks in a guild
	GUILD_LOGS_MAX_COUNT = 50,           // Max number of log entries for a guild
	GUILD_NEW_UPGRADE_SLOTS = 2,         // New guilds start with 2 additional slots
	GUILD_NEW_UPGRADE_CHAIR = 1,         // New guilds start with 1 chair upgrade
	GUILD_RENT_DAYS_DEFAULT = 3,         // Default rent duration for a guild
	GUILD_HOUSE_MAX_RENT_DAYS = 30       // Max rent duration for a guild house
};

// Enum for guild rank rights levels
enum GuildRankRights
{
	GUILD_RANK_RIGHT_LEADER = -1,        // Highest access level for guild leaders
	GUILD_RANK_RIGHT_START = 0,          // Start of rank rights range
	GUILD_RANK_RIGHT_DEFAULT = 0,        // Default rights for new guild members
	GUILD_RANK_RIGHT_INVITE_KICK,        // Right to invite and kick members
	GUILD_RANK_RIGHT_UPGRADES_HOUSE,     // Right to upgrade guild house
	GUILD_RANK_RIGHT_FULL,               // Full access to all guild functions
	GUILD_RANK_RIGHT_END                 // End of rank rights range
};

// Enum for results of guild-related operations
enum class GuildResult : int
{
	BUY_HOUSE_ALREADY_HAVE,              // Guild already owns a house
	BUY_HOUSE_UNAVAILABLE,               // House is unavailable for purchase
	BUY_HOUSE_ALREADY_PURCHASED,         // House is already purchased
	BUY_HOUSE_NOT_ENOUGH_GOLD,           // Not enough gold to buy house
	SET_LEADER_PLAYER_ALREADY_LEADER,    // Player is already the guild leader
	SET_LEADER_NON_GUILD_PLAYER,         // Player is not a member of the guild
	SUCCESSFUL,                          // Operation successful

	RANK_ADD_LIMIT_HAS_REACHED,          // Cannot add more ranks, limit reached
	RANK_ADD_ALREADY_EXISTS,             // Rank already exists
	RANK_REMOVE_IS_DEFAULT,              // Cannot remove default rank
	RANK_REMOVE_DOES_NOT_EXIST,          // Rank does not exist
	RANK_RENAME_ALREADY_NAME_EXISTS,     // Rank name already exists
	RANK_WRONG_NUMBER_OF_CHAR_IN_NAME,   // Invalid rank name length
	RANK_SUCCESSFUL,                     // Rank operation successful

	MEMBER_JOIN_ALREADY_IN_GUILD,        // Member is already in the guild
	MEMBER_KICK_DOES_NOT_EXIST,          // Member does not exist
	MEMBER_KICK_IS_OWNER,                // Cannot kick guild leader
	MEMBER_REQUEST_ALREADY_SEND,         // Request to join has already been sent
	MEMBER_NO_AVAILABLE_SLOTS,           // No slots available in guild
	MEMBER_UNDEFINED_ERROR,              // Undefined error during member operation
	MEMBER_SUCCESSFUL                    // Member operation successful
};

// Enum for guild upgrade types
enum class GuildUpgrade : size_t
{
    AvailableSlots = 0,                   // Upgrade for additional member slots
    NumGuildUpgr,                         // Total number of guild upgrades

    ChairExperience = 1,                  // Upgrade for house chair experience
    NumGuildHouseUpgr                     // Total number of guild house upgrades
};

// Flags for tracking guild activity logs
enum GuildActivityLogFlags
{
	LOGFLAG_MEMBERS_CHANGES = 1 << 0,    // Log member changes
	LOGFLAG_HOUSE_MAIN_CHANGES = 1 << 1, // Log main house changes
	LOGFLAG_HOUSE_DOORS_CHANGES = 1 << 2,// Log house doors changes
	LOGFLAG_HOUSE_DECORATIONS_CHANGES = 1 << 3,// Log house decoration changes
	LOGFLAG_UPGRADES_CHANGES = 1 << 4,   // Log upgrade changes
	LOGFLAG_RANKS_CHANGES = 1 << 5,      // Log rank changes
	LOGFLAG_BANK_CHANGES = 1 << 6,       // Log bank changes
	LOGFLAG_GUILD_MAIN_CHANGES = 1 << 7, // Log main guild info changes

	LOGFLAG_GUILD_FULL = LOGFLAG_MEMBERS_CHANGES | LOGFLAG_HOUSE_MAIN_CHANGES
	| LOGFLAG_HOUSE_DOORS_CHANGES | LOGFLAG_HOUSE_DECORATIONS_CHANGES
	| LOGFLAG_UPGRADES_CHANGES | LOGFLAG_RANKS_CHANGES | LOGFLAG_BANK_CHANGES
	| LOGFLAG_GUILD_MAIN_CHANGES         // Log all guild activities
};

class CGuild : public MultiworldIdentifiableData< std::deque < CGuild* > >
{
	friend class CGuildWarHandler;
	friend class CGuildHouse;

public:
	CGS* GS() const;

	/* -------------------------------------
	 * Bank impl
	 * ------------------------------------- */
	class CBank
	{
		CGS* GS() const;
		CGuild* m_pGuild {};
		BigInt m_Value {};

	public:
		CBank(const BigInt& Value, CGuild* pGuild)
			: m_pGuild(pGuild), m_Value(Value) {}

		// Get current bank balance
		const BigInt& Get() const
		{
			return m_Value;
		}

		// Add to bank balance
		void Add(const BigInt& Value);

		// Spend from bank balance
		[[nodiscard]] bool Spend(const BigInt& Value);
	};

	/* -------------------------------------
	 * Logger impl
	 * ------------------------------------- */
	struct LogData
	{
		std::string m_Text {};
		std::string m_Time {};
	};

	using LogContainer = std::deque<LogData>;
	class CLogEntry
	{
		CGuild* m_pGuild {};
		int64_t m_Logflag {};
		LogContainer m_aLogs {};

	public:
		CLogEntry() = delete;
		CLogEntry(CGuild* pGuild, int64_t Logflag);

		// Set activity flag
		void SetActivityFlag(int64_t Flag);

		// Check if flag is set
		bool IsActivityFlagSet(int64_t Flag) const;

		// Get log container
		const LogContainer& GetContainer() const
		{
			return m_aLogs;
		}

		// Add log message
		void Add(int64_t LogFlag, const char* pBuffer, ...);

	private:
		void InitLogs();
	};

	/* -------------------------------------
	 * Ranks impl
	 * ------------------------------------- */
	class CRank
	{
		CGS* GS() const;
		GuildRankIdentifier m_ID {};
		std::string m_Rank {};
		GuildRankRights m_Rights {};
		CGuild* m_pGuild {};

	public:
		CRank() = delete;
		CRank(GuildRankIdentifier RID, std::string&& Rank, GuildRankRights Rights, CGuild* pGuild);

		// Get rank ID
		GuildRankIdentifier GetID() const
		{
			return m_ID;
		}

		// Get rank name
		const char* GetName() const
		{
			return m_Rank.c_str();
		}

		// Get name of specific rank right
		const char* GetRightsName(GuildRankRights Right) const;

		// Get name of current rank rights
		const char* GetRightsName() const
		{
			return GetRightsName(m_Rights);
		}

		// Rename the rank
		[[nodiscard]] GuildResult Rename(std::string NewRank);

		// Set rank rights
		void SetRights(GuildRankRights Rights);

		// Get current rank rights
		const GuildRankRights& GetRights() const { return m_Rights; }
	};

	using RankContainer = std::deque<class CRank*>;
	class CRanksManager
	{
		CGS* GS() const;
		CRank* m_pDefaultRank {};
		std::deque<class CRank*> m_aRanks {};
		CGuild* m_pGuild {};

	public:
		CRanksManager() = delete;
		CRanksManager(CGuild* pGuild, GuildRankIdentifier DefaultID);
		~CRanksManager();
		
		// Get container of guild ranks
		std::deque<class CRank*>& GetContainer()
		{
			return m_aRanks;
		}

		// Add a new guild rank
		[[nodiscard]] GuildResult Add(const std::string& Rank);

		// Remove a guild rank
		[[nodiscard]] GuildResult Remove(const std::string& Rank);

		// Get guild rank by name
		CRank* Get(const std::string& Rank) const;

		// Get guild rank by ID
		CRank* Get(GuildRankIdentifier ID) const;

		// Get default guild rank
		CRank* GetDefaultRank() const
		{
			return m_pDefaultRank;
		}

		// Update the default guild rank
		void UpdateDefaultRank();

	private:
		void Init(GuildRankIdentifier DefaultID);
	};

	/* -------------------------------------
	 * Members impl
	 * ------------------------------------- */
	class CRequestsManager;
	class CMember
	{
		CGS* GS() const;

		CGuild* m_pGuild {};
		CRank* m_pRank {};
		int m_AccountID {};
		BigInt m_Deposit {};

	public:
		CMember(CGuild* pGuild, int AccountID, CRank* pRank, BigInt Deposit = 0);
		~CMember();

		// Check if member is online
		bool IsOnline() const;

		// Get member's account ID
		int GetAccountID() const
		{
			return m_AccountID;
		}

		// Get member's deposit
		BigInt GetDeposit() const
		{
			return m_Deposit;
		}

		// Set member's deposit
		void SetDeposit(const BigInt& Deposit)
		{
			m_Deposit = Deposit;
		}

		// Get member's rank
		CRank* GetRank() const
		{
			return m_pRank;
		}

		// Set member's rank by ID
		[[nodiscard]] bool SetRank(GuildRankIdentifier RankID);

		// Set member's rank by rank pointer
		[[nodiscard]] bool SetRank(CRank* pRank);

		// Deposit money to bank
		[[nodiscard]] bool DepositInBank(int Value);

		// Withdraw money from bank
		[[nodiscard]] bool WithdrawFromBank(int Value);

		// Check if member has required access
		[[nodiscard]] bool CheckAccess(GuildRankRights RequiredAccess) const;
	};

	using MembersContainer = std::map<int, CMember*>;
	class CMembersManager
	{
		CGS* GS() const;
		CGuild* m_pGuild {};
		CRequestsManager* m_pRequests {};
		MembersContainer m_apMembers {};

	public:
		CMembersManager(CGuild* pGuild, const std::string& JsonMembers);
		~CMembersManager();

		// Pointer to the request manager for join requests
		CRequestsManager* GetRequests() const
		{
			return m_pRequests;
		}

		// Get a guild member by Account ID
		CMember* Get(int AccountID);

		// Get the guild members container
		MembersContainer& GetContainer()
		{
			return m_apMembers;
		}

		// Add a member to the guild by Account ID
		[[nodiscard]] GuildResult Join(int AccountID);

		// Remove a member from the guild by Account ID
		[[nodiscard]] GuildResult Kick(int AccountID);

		// Reset all deposits to zero
		void ResetDeposits();

		// Get the current number of used and total slots
		std::pair<int, int> GetCurrentSlots() const;

		// Check if there are free slots available
		bool HasFreeSlots() const;

		// Get the number of online members
		int GetOnlineCount() const;

		// Save the guild members data
		void Save() const;

	private:
		void Init(const std::string& JsonMembers);
	};

	/* -------------------------------------
	 * Request member impl
	 * ------------------------------------- */
	class RequestData
	{
		int m_FromUID;

	public:
		RequestData() = delete;
		RequestData(int FromUID) noexcept : m_FromUID(FromUID) {}

		// Getter method for retrieving the FromUID member variable
		int GetFromUID() const noexcept { return m_FromUID; }                                
	};

	using RequestsContainer = std::vector < RequestData* >;
	class CRequestsManager
	{
		CGS* GS() const;
		CGuild* m_pGuild {};
		RequestsContainer m_aRequestsJoin {};

	public:
		CRequestsManager() = delete;
		CRequestsManager(CGuild* pGuild);
		~CRequestsManager();

		// Getter for the join requests container
		const RequestsContainer& GetContainer() const
		{
			return m_aRequestsJoin;
		}

		// Method for requesting to join the guild
		[[nodiscard]] GuildResult Request(int FromUID);

		// Method for accepting a join request
		[[nodiscard]] GuildResult Accept(int UserID, const CMember* pFromMember = nullptr);

		// Method for denying a join request
		void Deny(int UserID, const CMember* pFromMember = nullptr);

	private:
		void Init();
	};

private:
	GuildIdentifier m_ID {};
	std::string m_Name {};
	int m_LeaderUID {};
	int m_Level {};
	uint64_t m_Experience {};
	int m_Score {};

	DBFieldContainer m_UpgradesData
	{
		DBField<int>((int)GuildUpgrade::AvailableSlots, "AvailableSlots", "Available slots", GUILD_NEW_UPGRADE_SLOTS),
		DBField<int>((int)GuildUpgrade::ChairExperience, "ChairExperience", "Chair experience", GUILD_NEW_UPGRADE_CHAIR),
	};

	CBank* m_pBank {};
	CLogEntry* m_pLogger {};
	CRanksManager* m_pRanks {};
	CMembersManager* m_pMembers {};
	CGuildWarData* m_pWar {};
	CGuildHouse* m_pHouse {};

public:
	CGuild() = default;
	~CGuild();

	static CGuild* CreateElement(const GuildIdentifier& ID)
	{
		auto pData = new CGuild;
		pData->m_ID = ID;
		return m_pData.emplace_back(pData);
	}

	void Init(const std::string& Name, const std::string& JsonMembers, GuildRankIdentifier DefaultRankID, int Level, 
		uint64_t Experience, int Score, int LeaderUID, const BigInt& Bank, int64_t Logflag, ResultPtr* pRes)
	{
		m_Name = Name;
		m_LeaderUID = LeaderUID;
		m_Level = Level;
		m_Experience = Experience;
		m_Score = Score;
		m_UpgradesData.initFields(pRes);

		// components init
		m_pLogger = new CLogEntry(this, Logflag);
		m_pBank = new CBank(Bank, this);
		m_pRanks = new CRanksManager(this, DefaultRankID);
		m_pMembers = new CMembersManager(this, JsonMembers);
		m_pRanks->UpdateDefaultRank();
	}

	// Get guild ID
	GuildIdentifier GetID() const
	{
		return m_ID;
	}

	// Get guild bank
	CBank* GetBank() const
	{
		return m_pBank;
	}

	// Get guild log entry
	CLogEntry* GetLogger() const
	{
		return m_pLogger;
	}

	// Get guild ranks manager
	CRanksManager* GetRanks() const
	{
		return m_pRanks;
	}

	// Get guild house
	CGuildHouse* GetHouse() const
	{
		return m_pHouse;
	}

	// Get members manager
	CMembersManager* GetMembers() const
	{
		return m_pMembers;
	}

	// Get specific guild upgrade
	DBFieldContainer& GetUpgrades()
	{
		return m_UpgradesData;
	}

	// Get guild name
	const char* GetName() const
	{
		return m_Name.c_str();
	}

	// Get guild leader's account ID
	int GetLeaderUID() const
	{
		return m_LeaderUID;
	}

	// Get guild level
	int GetLevel() const
	{
		return m_Level;
	}

	// Get guild experience points
	uint64_t GetExperience() const
	{
		return m_Experience;
	}

	// Get guild score
	int GetScore() const
	{
		return m_Score;
	}

	// Check if guild has a house
	bool HasHouse() const
	{
		return m_pHouse != nullptr;
	}

	// Get upgrade price
	int GetUpgradePrice(GuildUpgrade Type);

	// Add experience to guild
	void AddExperience(uint64_t Experience);

	// Upgrade guild
	[[nodiscard]] bool Upgrade(GuildUpgrade Type);

	// Set a new guild leader
	[[nodiscard]] GuildResult SetLeader(int AccountID);

	// Buy a guild house
	[[nodiscard]] GuildResult BuyHouse(int HouseID);

	// Sell the guild house
	void SellHouse();

	// Handle time-based events
	void HandleTimePeriod(ETimePeriod Period);

	// Start a war with another guild
	bool StartWar(CGuild* pTargetGuild);

	// Get current guild war data
	CGuildWarData* GetWar() const
	{
		return m_pWar;
	}

	// Check if an account is a member of the guild
	static bool IsAccountMemberGuild(int AccountID);
};

#endif
