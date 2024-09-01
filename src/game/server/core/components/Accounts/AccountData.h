/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_COMPONENT_ACCOUNT_DATA_H
#define GAME_SERVER_COMPONENT_ACCOUNT_DATA_H

// TODO: fully rework structures
#include "bonus_manager.h"
#include <game/server/core/components/guilds/guild_data.h>
#include <game/server/core/components/auction/auction_data.h>

class CGS;
class CPlayer;
class CHouse;
class GroupData;
class CGuild;
class CGuildMemberData;

class CAccountData
{
	ska::unordered_set< int > m_aAetherLocation {};

	int m_ID {};
	int m_ClientID {};
	char m_aLogin[64] {};
	char m_aLastLogin[64] {};
	int m_DailyChairGolds {};
	int m_CrimeScore {};

	int m_Level {};
	int m_Exp {};
	CHouse* m_pHouseData{};
	std::weak_ptr<GroupData> m_pGroupData;
	CGuild* m_pGuildData{};
	ClassGroup m_ClassGroup {};
	nlohmann::json m_AchivementsData { };
	CBonusManager m_BonusManager;
	BigInt m_Bank {};

	CPlayer* m_pPlayer {};
	CGS* GS() const;
	CPlayer* GetPlayer() const { return m_pPlayer; }

public:
	CBonusManager& GetBonusManager() { return m_BonusManager; }
	const CBonusManager& GetBonusManager() const { return m_BonusManager; }

	/*
	 * Group functions: initialize or uniques from function
	 */
	void Init(int ID, CPlayer* pPlayer, const char* pLogin, std::string Language, std::string LoginDate, ResultPtr pResult); // Function to initialize
	void UpdatePointer(CPlayer* pPlayer);
	int GetID() const { return m_ID; } // Function to get the ID of an object

	/*
	 * Group functions: house system
	 */
	void ReinitializeHouse(bool SetNull = false); // This function re-initializes the house object
	CHouse* GetHouse() const { return m_pHouseData; } // Get the house data for the current object
	bool HasHouse() const { return m_pHouseData != nullptr; } // Check if the current object has house data

	/*
	 * Group functions: group system
	 */
	void SetGroup(const std::shared_ptr<GroupData>& pGroupPtr) { m_pGroupData = pGroupPtr; }
	GroupData* GetGroup() const { return m_pGroupData.lock().get(); }
	bool HasGroup() const { return !m_pGroupData.expired(); }

	/*
	 * Group functions: guild system
	 */
	void ReinitializeGuild(bool SetNull = false);
	CGuild* GetGuild() const { return m_pGuildData; }
	CGuild::CMember* GetGuildMember() const;
	bool HasGuild() const { return m_pGuildData != nullptr; }
	bool IsClientSameGuild(int ClientID) const;
	bool IsSameGuild(int GuildID) const;

	/*
	 * Group function: getters / setters
	 */
	int GetLevel() const { return m_Level; } // Returns the level of the player
	int GetExperience() const { return m_Exp; } // Returns the experience points of the player
	const char* GetLogin() const { return m_aLogin; } // Returns the login name of the player as a const char pointer
	const char* GetLastLoginDate() const { return m_aLastLogin; } // Returns the last login date of the player as a const char pointer
	int GetCurrentDailyChairGolds() const { return m_DailyChairGolds; } // Returns current daily chair golds
	int GetLimitDailyChairGolds() const; // Returns the daily limit of gold that a player can obtain from chairs

	void IncreaseCrimeScore(int Score);
	bool IsCrimeScoreMaxedOut() const { return m_CrimeScore >= 100; }
	int GetCrimeScore() const { return m_CrimeScore; }
	void ResetCrimeScore();

	bool IsInPrison() const { return m_PrisonSeconds > 0; }
	void Imprison(int Seconds);
	void FreeFromPrison();

	BigInt GetBank() const { return m_Bank; }
	int GetGold() const;
	BigInt GetTotalGold() const;

	void AddExperience(int Value); // Adds the specified value to the player's experience points
	void AddGold(int Value, bool ApplyBonuses = false); // Adds the specified value to the player's gold (currency)
	bool DepositGoldToBank(int Amount);
	bool WithdrawGoldFromBank(int Amount);
	bool SpendCurrency(int Price, int CurrencyItemID = 1); // Returns a boolean value indicating whether the currency was successfully spent or not.
	void ResetDailyChairGolds(); // Reset daily getting chair golds
	void HandleChair();

	// Achievements
	void InitAchievements(const std::string& Data);
	void SetAchieventProgress(int AchievementID, int Progress, bool Completed);
	nlohmann::json& GetAchievementsData() { return m_AchivementsData; }

	// Aethers
	bool IsUnlockedAether(int AetherID) const { return m_aAetherLocation.find(AetherID) != m_aAetherLocation.end(); }
	void AddAether(int AetherID) { m_aAetherLocation.insert(AetherID); }
	void RemoveAether(int AetherID) { m_aAetherLocation.erase(AetherID); }
	ska::unordered_set< int >& GetAethers() { return m_aAetherLocation; }

	struct TimePeriods
	{
		time_t m_DailyStamp { };
		time_t m_WeekStamp { };
		time_t m_MonthStamp { };
	};

	// main
	int m_GuildRank {};
	int m_PrisonSeconds {};
	TimePeriods m_Periods {};
	std::list< int > m_aHistoryWorld {};

	// upgrades
	int m_Upgrade {};
	std::map< AttributeIdentifier, int > m_aStats {};

	CTeeInfo m_TeeInfos {};
	int m_Team {};

	DBFieldContainer m_MiningData
	{
		{ DBField<int>(JOB_LEVEL, "Level", "Miner level") },
		{ DBField<int>(JOB_EXPERIENCE, "Exp", "Miner experience") },
		{ DBField<int>(JOB_UPGRADES, "Upgrade", "Miner upgrades") }
	};

	DBFieldContainer m_FarmingData
	{
		{ DBField<int>(JOB_LEVEL, "Level", "Farmer level") },
		{ DBField<int>(JOB_EXPERIENCE, "Exp", "Farmer experience") },
		{ DBField<int>(JOB_UPGRADES, "Upgrade", "Farmer upgrades") }
	};

	static std::map < int, CAccountData > ms_aData;
};

struct CAccountTempData
{
	int m_LastKilledByWeapon;
	int m_TempDecoractionID;
	int m_TempDecorationType;
	int m_TempID3;

	CAuctionSlot m_TempAuctionSlot;

	// temp rankname for guild rank settings
	char m_aRankGuildBuf[32];

	// temp for searching
	char m_aGuildSearchBuf[32];
	char m_aPlayerSearchBuf[32];

	// player stats
	int m_TempHealth;
	int m_TempMana;
	int m_TempPing;

	// dungeon
	int m_TempTimeDungeon;
	bool m_TempDungeonReady;

	void SetTeleportPosition(vec2 Position) { m_TempTeleportPos = Position; }
	vec2 GetTeleportPosition() const { return m_TempTeleportPos; }
	void ClearTeleportPosition() { m_TempTeleportPos = { -1, -1 }; }

	static std::map < int, CAccountTempData > ms_aPlayerTempData;

private:
	vec2 m_TempTeleportPos{};
};

#endif