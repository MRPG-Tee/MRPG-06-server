/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_CORE_COMPONENTS_GUILDS_GUILD_DATA_H
#define GAME_SERVER_CORE_COMPONENTS_GUILDS_GUILD_DATA_H
#include <game/server/core/tools/dbfield.h>

#include "guild_house_data.h"
#include "War/GuildWarData.h"

#define TW_GUILDS_TABLE "tw_guilds"
#define TW_GUILDS_RANKS_TABLE "tw_guilds_ranks"
#define TW_GUILDS_HISTORY_TABLE "tw_guilds_history"
#define TW_GUILDS_INVITES_TABLE "tw_guilds_invites"

// Forward declaration and alias
class CGuildWarData;
using GuildIdentifier = int;
using GuildRankIdentifier = int;

// This enum class represents the possible results of guild operations
enum class GUILD_RESULT : int
{
	BUY_HOUSE_ALREADY_HAVE,                         // The guild already owns a house and cannot buy another one
	BUY_HOUSE_UNAVAILABLE,                          // The house is not available for purchase
	BUY_HOUSE_ALREADY_PURCHASED,                    // The house has already been purchased by another player
	BUY_HOUSE_NOT_ENOUGH_GOLD,                      // The guild does not have enough gold to buy the house
	SET_LEADER_PLAYER_ALREADY_LEADER,               // The player is already the leader of the guild
	SET_LEADER_NON_GUILD_PLAYER,                    // The player is not a member of the guild
	SUCCESSFUL,                                     // The guild operation was successful

	RANK_ADD_LIMIT_HAS_REACHED,                     // Cannot add more ranks, limit has been reached
	RANK_ADD_ALREADY_EXISTS,                        // Rank with the same name already exists
	RANK_REMOVE_IS_DEFAULT,                         // Cannot remove default rank
	RANK_REMOVE_DOES_NOT_EXIST,                     // Rank to be removed does not exist
	RANK_RENAME_ALREADY_NAME_EXISTS,                // Cannot rename rank, name already exists
	RANK_WRONG_NUMBER_OF_CHAR_IN_NAME,              // Wrong number of characters in the name
	RANK_SUCCESSFUL,                                // Operation was successful

	MEMBER_JOIN_ALREADY_IN_GUILD,                   // Result when a member tries to join a guild they are already a part of
	MEMBER_KICK_DOES_NOT_EXIST,                     // Result when trying to kick a member who does not exist
	MEMBER_KICK_IS_OWNER,                           // Result when trying to kick the guild leader
	MEMBER_REQUEST_ALREADY_SEND,                    // Result when a member tries to send a join request to a guild they have already sent a request to
	MEMBER_NO_AVAILABLE_SLOTS,                      // Result when there are no available slots in the guild for new members
	MEMBER_UNDEFINED_ERROR,                         // Result when an undefined error occurs during the operation
	MEMBER_SUCCESSFUL                               // Result when the operation is successful
};

// Enum representing different flags for guild activity logging
enum GuildActivityLogFlags
{
	LOGFLAG_MEMBERS_CHANGES = 1 << 0,               // Flag for logging member changes
	LOGFLAG_HOUSE_MAIN_CHANGES = 1 << 1,            // Flag for logging house main changes
	LOGFLAG_HOUSE_DOORS_CHANGES = 1 << 2,           // Flag for logging house doors changes
	LOGFLAG_HOUSE_DECORATIONS_CHANGES = 1 << 3,     // Flag for logging house decorations changes
	LOGFLAG_UPGRADES_CHANGES = 1 << 4,              // Flag for logging upgrades changes
	LOGFLAG_RANKS_CHANGES = 1 << 5,                 // Flag for logging ranks changes
	LOGFLAG_BANK_CHANGES = 1 << 6,                  // Flag for logging bank changes
	LOGFLAG_GUILD_MAIN_CHANGES = 1 << 7,            // Flag for logging main guild information

	// Flag for logging all guild activities
	LOGFLAG_GUILD_FULL = LOGFLAG_MEMBERS_CHANGES | LOGFLAG_HOUSE_MAIN_CHANGES | LOGFLAG_HOUSE_DOORS_CHANGES | LOGFLAG_HOUSE_DECORATIONS_CHANGES
	| LOGFLAG_UPGRADES_CHANGES | LOGFLAG_RANKS_CHANGES | LOGFLAG_BANK_CHANGES | LOGFLAG_GUILD_MAIN_CHANGES,
};

class CGuildData : public MultiworldIdentifiableStaticData< std::deque < CGuildData* > >
{
	friend class CGuildWarHandler;
	friend class CGuildHouseData;

public:
	CGS* GS() const;

	/* -------------------------------------
	 * Bank impl
	 * ------------------------------------- */
	class CBank
	{
		CGS* GS() const;
		CGuildData* m_pGuild {};
		int m_Bank {};

	public:
		CBank(int Bank, CGuildData* pGuild) : m_pGuild(pGuild), m_Bank(Bank) {}

		const int& Get() const { return m_Bank; }                                           // Get value inside bank
		void Set(int Value);                                                                // Set value bank
		[[nodiscard]] bool Spend(int Value);                                                // Spend from bank return boolean
	};

	/* -------------------------------------
	 * Logger impl
	 * ------------------------------------- */
	struct LogData
	{
		std::string m_Text {}; // The log message
		std::string m_Time {}; // The time when the log was added
	};

	using LogContainer = std::deque<LogData>;
	class CLogEntry
	{
		CGuildData* m_pGuild {};
		int64_t m_Logflag {};
		LogContainer m_aLogs {};

	public:
		CLogEntry() = delete;
		CLogEntry(CGuildData* pGuild, int64_t Logflag);

		void SetActivityFlag(int64_t Flag);                                                 // Set activity flag
		bool IsActivityFlagSet(int64_t Flag) const;                                         // Check activity flag
		const LogContainer& GetContainer() const { return m_aLogs; }                        // Get the guild history logs
		void Add(int64_t LogFlag, const char* pBuffer, ...);                                // Add a log message to the guild history

	private:
		void InitLogs();
	};

	/* -------------------------------------
	 * Ranks impl
	 * ------------------------------------- */
	using GuildRankIdentifier = int;
	class CRank
	{
		CGS* GS() const;
		GuildRankIdentifier m_ID {};
		std::string m_Rank {};
		GuildRankAccess m_Access {};
		CGuildData* m_pGuild {};

	public:
		CRank() = delete;
		CRank(GuildRankIdentifier RID, std::string&& Rank, GuildRankAccess Access, CGuildData* pGuild);

		GuildRankIdentifier GetID() const { return m_ID; }                                  // Get the unique identifier of the guild rank
		const char* GetName() const { return m_Rank.c_str(); }                              // Get the name of the guild rank
		const char* GetAccessName() const;                                                  // Get the name of the access level of the guild rank
		[[nodiscard]] GUILD_RESULT Rename(std::string NewRank);                             // Change the name of the guild rank
		void ChangeAccess();                                                                // Change the access level of the guild rank
		void SetAccess(GuildRankAccess Access);                                             // Set the access level of the guild rank
		const GuildRankAccess& GetAccess() const { return m_Access; }                       // Get the rank access
	};

	using RankContainer = std::deque<class CRank*>;
	class CRanksManager
	{
		CGS* GS() const;
		CRank* m_pDefaultRank {};
		std::deque<class CRank*> m_aRanks {};
		CGuildData* m_pGuild {};

	public:
		CRanksManager() = delete;
		CRanksManager(CGuildData* pGuild, GuildRankIdentifier DefaultID);
		~CRanksManager();
		
		std::deque<class CRank*>& GetContainer() { return m_aRanks; }                       // Function to get the container of guild ranks
		[[nodiscard]] GUILD_RESULT Add(const std::string& Rank);                            // Function to add a new guild rank
		[[nodiscard]] GUILD_RESULT Remove(const std::string& Rank);                         // Function to remove an existing guild rank
		CRank* Get(const std::string& Rank) const;                                          // Function to get a guild rank by its name
		CRank* Get(GuildRankIdentifier ID) const;                                           // Function to get a guild rank by its id
		CRank* GetDefaultRank() const { return m_pDefaultRank; }                            // Function get default rank
		void UpdateDefaultRank();                                                           // Function to initialize the default guild rank

	private:
		void Init(GuildRankIdentifier DefaultID);                                           // Function to initialize the guild ranks
	};

	/* -------------------------------------
	 * Members impl
	 * ------------------------------------- */
	class CRequestsManager;
	class CMember
	{
		CGS* GS() const;

		CGuildData* m_pGuild {};
		CRank* m_pRank {};
		int m_AccountID {};
		int m_Deposit {};

	public:
		CMember(CGuildData* pGuild, int AccountID, CRank* pRank, int Deposit = 0);
		~CMember();

		int GetAccountID() const { return m_AccountID; }                                    // Get the account ID of the guild member
		int GetDeposit() const { return m_Deposit; }                                        // Get the amount of gold deposited by the guild member
		void SetDeposit(int Deposit) { m_Deposit = Deposit; }                               // Set the amount of gold deposited by the guild member
		CRank* GetRank() const { return m_pRank; }                                          // Get the rank of the guild member
		[[nodiscard]] bool SetRank(GuildRankIdentifier RankID);                             // Set the rank of the guild member using the rank ID
		[[nodiscard]] bool SetRank(CRank* pRank);                                           // Set the rank of the guild member using a rank object
		[[nodiscard]] bool DepositInBank(int Golds);                                        // Deposit gold in the guild bank
		[[nodiscard]] bool WithdrawFromBank(int Golds);                                     // Withdraw gold from the guild bank
		[[nodiscard]] bool CheckAccess(GuildRankAccess RequiredAccess) const;               // Check if a member has the required access level
	};

	using MembersContainer = std::map<int, CMember*>;
	class CMembersManager
	{
		CGS* GS() const;
		CGuildData* m_pGuild {};
		CRequestsManager* m_pRequests {};
		MembersContainer m_apMembers {};

	public:
		CMembersManager(CGuildData* pGuild, std::string&& MembersData);
		~CMembersManager();
		
		CRequestsManager* GetRequests() const { return m_pRequests; }                        // Returns the pointer to the controller requests to join
		CMember* Get(int AccountID);                                                         // Get a guild member by account ID
		MembersContainer& GetContainer() { return m_apMembers; }                             // Get the guild members container
		[[nodiscard]] GUILD_RESULT Join(int AccountID);                                      // Join a guild by account ID
		[[nodiscard]] GUILD_RESULT Kick(int AccountID);                                      // Kick a guild member by account ID
		void ResetDeposits();                                                                // This function is used to reset all deposits to zero.
		std::pair<int, int> GetCurrentSlots() const;                                         // This function returns the current number of slots being used and the total number of slots
		bool HasFreeSlots() const;                                                           // This function checks if there are any free slots available
		int GetOnlinePlayersCount() const;                                                   // This function is used to get online players.
		void Save() const;                                                                   // Save the guild members data

	private:
		void Init(std::string&& MembersData);                                                // Initialize the guild members controller
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

		int GetFromUID() const noexcept { return m_FromUID; }                                // Getter method for retrieving the FromUID member variable
	};

	using RequestsContainer = std::vector < RequestData* >;
	class CRequestsManager
	{
		CGS* GS() const;
		CGuildData* m_pGuild {};
		RequestsContainer m_aRequestsJoin {};

	public:
		CRequestsManager() = delete;
		CRequestsManager(CGuildData* pGuild);
		~CRequestsManager();

		const RequestsContainer& GetContainer() const { return m_aRequestsJoin; }            // Getter for the join requests container
		[[nodiscard]] GUILD_RESULT Request(int FromUID);                                     // Method for requesting to join the guild
		[[nodiscard]] GUILD_RESULT Accept(int UserID, const CMember* pFromMember = nullptr); // Method for accepting a join request
		void Deny(int UserID, const CMember* pFromMember = nullptr);                         // Method for denying a join request

	private:
		void Init();                                                                         // Private method for initializing the controller
	};

private:
	GuildIdentifier m_ID {};
	std::string m_Name {};
	int m_LeaderUID {};
	int m_Level {};
	int m_Experience {};
	int m_Score {};

	DBFieldContainer m_UpgradesData
	{
		DBField<int> { UPGRADE_AVAILABLE_SLOTS, "AvailableSlots", "Available slots", DEFAULT_GUILD_AVAILABLE_SLOTS },
		DBField<int> { UPGRADE_CHAIR_EXPERIENCE, "ChairExperience", "Chair experience", DEFAULT_GUILD_CHAIR },
	};

	CBank* m_pBank {};
	CLogEntry* m_pLogger {};
	CGuildWarData* m_pWar {};
	CRanksManager* m_pRanks {};
	CMembersManager* m_pMembers {};
	CGuildHouseData* m_pHouse {};

public:
	enum
	{
		UPGRADE_AVAILABLE_SLOTS = 0,
		UPGRADE_CHAIR_EXPERIENCE = 1,
		NUM_GUILD_UPGRADES,
	};

	CGuildData() = default;
	~CGuildData();

	static CGuildData* CreateElement(const GuildIdentifier& ID)
	{
		auto pData = new CGuildData;
		pData->m_ID = ID;
		return m_pData.emplace_back(pData);
	}

	void Init(const std::string& Name, std::string&& MembersData, GuildRankIdentifier DefaultRankID, int Level, int Experience, int Score, int LeaderUID, int Bank, int64_t Logflag, ResultPtr* pRes)
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
		m_pMembers = new CMembersManager(this, std::move(MembersData));
		m_pRanks->UpdateDefaultRank();
	}

	// getters
	GuildIdentifier GetID() const { return m_ID; }
	CBank* GetBank() const { return m_pBank; }
	CLogEntry* GetLogger() const { return m_pLogger; }
	CRanksManager* GetRanks() const { return m_pRanks; }
	CGuildHouseData* GetHouse() const { return m_pHouse; }
	CMembersManager* GetMembers() const { return m_pMembers; }
	DBField<int>* GetUpgrades(int Type) { return &m_UpgradesData(Type, 0); }
	const char* GetName() const { return m_Name.c_str(); }
	int GetLeaderUID() const { return m_LeaderUID; }
	int GetLevel() const { return m_Level; }
	int GetExperience() const { return m_Experience; }
	int GetScore() const { return m_Score; }
	bool HasHouse() const { return m_pHouse != nullptr; }
	int GetUpgradePrice(int Type);

	// functions
	void AddExperience(int Experience);
	[[nodiscard]] bool Upgrade(int Type);
	[[nodiscard]] GUILD_RESULT SetNewLeader(int AccountID);
	[[nodiscard]] GUILD_RESULT BuyHouse(int HouseID);
	[[nodiscard]] bool SellHouse();
	void TimePeriodEvent(TIME_PERIOD Period);

	// war
	bool StartWar(CGuildData* pTargetGuild);
	CGuildWarData* GetWar() const { return m_pWar; };

	// global functions
	static bool IsAccountMemberGuild(int AccountID);
};

#endif
