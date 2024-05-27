/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_COMPONENT_ACCOUNT_MINER_CORE_H
#define GAME_SERVER_COMPONENT_ACCOUNT_MINER_CORE_H
#include <game/server/core/mmo_component.h>

class CItemDescription;

class CAccountMiningManager : public MmoComponent
{
	~CAccountMiningManager() override
	{
		ms_vmMiningPoints.clear();
	}

	struct MiningPoint
	{
		int m_ItemID;
		vec2 m_Position;
		int m_Distance;
		int m_WorldID;
	};
	static std::map < int, MiningPoint > ms_vmMiningPoints;

	void OnPlayerLogin(CPlayer* pPlayer) override;
	void OnInitWorld(const char* pWhereLocalWorld) override;
	bool OnPlayerVoteCommand(CPlayer* pPlayer, const char* pCmd, int Extra1, int Extra2, int ReasonNumber, const char* pReason) override;

public:
	CItemDescription* GetMiningItemInfoByPos(vec2 Pos) const;

	//void ShowMenu(CPlayer *pPlayer) const;
	void Process(CPlayer *pPlayer, int Exp) const;

	bool InsertItemsDetailVotes(CPlayer* pPlayer, int WorldID) const;
};

#endif