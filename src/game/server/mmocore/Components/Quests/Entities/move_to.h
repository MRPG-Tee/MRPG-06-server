/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_MMOCORE_COMPONENTS_QUESTS_ENTITIES_MOVE_TO_H
#define GAME_SERVER_MMOCORE_COMPONENTS_QUESTS_ENTITIES_MOVE_TO_H

#include <game/server/entity.h>

class CEntityMoveTo : public CEntity
{
	int m_QuestID;
	int m_ClientID;
	bool* m_pComplete;
	std::deque < CEntityMoveTo* >* m_apCollection;
	const QuestBotInfo::TaskRequiredMoveTo* m_pTaskMoveTo;

public:
	class CPlayer* m_pPlayer;

	CEntityMoveTo(CGameWorld* pGameWorld, const QuestBotInfo::TaskRequiredMoveTo* pTaskMoveTo, int ClientID, int QuestID, bool *pComplete, std::deque < CEntityMoveTo* >* apCollection);

	void Destroy() override;
	void Tick() override;
	void Snap(int SnappingClient) override;

	bool PickItem() const;

	int GetClientID() const { return m_ClientID; }
	int GetQuestID() const { return m_QuestID; }
};

#endif