/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_COMPONENT_GUILD_MEMBER_DATA_H
#define GAME_SERVER_COMPONENT_GUILD_MEMBER_DATA_H

#include "../Ranks/GuildRankData.h"

// Forward declaration and alias
class CGuildData;
class CGuildRankData;

class CGuildMemberData
{
	CGS* GS() const;

	CGuildData* m_pGuild {};
	CGuildRankData* m_pRank {};
	int m_AccountID {};
	int m_Deposit {};

public:
	CGuildMemberData(CGuildData* pGuild, int AccountID, CGuildRankData* pRank, int Deposit = 0);
	~CGuildMemberData();

	int GetAccountID() const { return m_AccountID; } // Get the account ID of the guild member
	int GetDeposit() const { return m_Deposit; } // Get the amount of gold deposited by the guild member
	void SetDeposit(int Deposit) { m_Deposit = Deposit; } // Set the amount of gold deposited by the guild member

	CGuildRankData* GetRank() const { return m_pRank; } // Get the rank of the guild member
	[[nodiscard]] bool SetRank(GuildRankIdentifier RankID); // Set the rank of the guild member using the rank ID
	[[nodiscard]] bool SetRank(CGuildRankData* pRank); // Set the rank of the guild member using a rank object

	[[nodiscard]] bool DepositInBank(int Golds); // Deposit gold in the guild bank
	[[nodiscard]] bool WithdrawFromBank(int Golds); // Withdraw gold from the guild bank
	
	[[nodiscard]] bool CheckAccess(GuildRankAccess RequiredAccess) const; // Check if a member has the required access level
};

#endif
