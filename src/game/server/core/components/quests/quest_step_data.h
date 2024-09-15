/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_COMPONENT_QUEST_STEP_DATA_INFO_H
#define GAME_SERVER_COMPONENT_QUEST_STEP_DATA_INFO_H

#include <game/server/core/components/Bots/BotData.h>

class CGS;
class CPlayer;


/*
 * Quest step
 */
class CQuestStepBase
{
public:
	QuestBotInfo m_Bot{};
	virtual void UpdateBot() const;

private:
	bool IsActiveStep() const;
};

// ##############################################################
// ################# PLAYER STEP STRUCTURE ######################
class CEntityArrowNavigator;
class CQuestStep : public CQuestStepBase, public std::enable_shared_from_this<CQuestStep>
{
	class CGS* GS() const;
	class CPlayer* GetPlayer() const;

	CEntityArrowNavigator* m_pEntNavigator{};

	struct MobProgressStatus
	{
		int m_Count;
		bool m_Complete;
	};

public:
	CQuestStep(int ClientID, const QuestBotInfo& Bot) : m_ClientID(ClientID)
	{
		m_Bot = Bot;
		m_aMobProgress.clear();
		m_aMoveToProgress.clear();
	}
	~CQuestStep();

	std::unordered_map < int /*BotID*/, MobProgressStatus/*MobProgressStatus*/ > m_aMobProgress { };
	std::deque < bool /* State */ > m_aMoveToProgress { };

	int m_ClientID {};
	bool m_StepComplete{};
	bool m_ClientQuitting{};
	bool m_TaskListReceived{};

	int GetNumberBlockedItem(int ItemID) const;
	bool IsComplete();
	bool Finish();
	void PostFinish();

	void AppendDefeatProgress(int DefeatedBotID);
	void CreateVarietyTypesRequiredItems();
	void FormatStringTasks(char* aBufQuestTask, int Size);

	void UpdatePathNavigator();
	void UpdateTaskMoveTo();
	void Update();

	int GetMoveToNum() const;
	int GetMoveToCurrentStepPos() const;
	int GetCompletedMoveToCount();

	// steps path finder tools
	std::deque < std::shared_ptr<class CEntityQuestAction> > m_vpEntitiesAction {};
	std::deque < class CEntityPathArrow* > m_apEntitiesNavigator {};

	void UpdateEntityQuestAction(int MoveToIndex, std::optional<int> OptDefeatBotCID = std::nullopt);
	CEntityPathArrow* UpdateEntityArrowNavigator(vec2 Position, int WorldID, float AreaClipped, bool* pComplete);
};

#endif
