/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "quest_manager.h"

#include <game/server/entity_manager.h>
#include <game/server/gamecontext.h>
#include <game/server/core/components/Dungeons/DungeonManager.h>

void CQuestReward::ApplyReward(CPlayer* pPlayer) const
{
	pPlayer->Account()->AddExperience(m_Experience);
	pPlayer->Account()->AddGold(m_Gold);
}

void CQuestDescription::PreparePlayerSteps(int StepPos, int ClientID, std::deque<CQuestStep>* pElem)
{
	// clear old steps
	if(!(*pElem).empty())
		(*pElem).clear();

	// prepare new steps
	for(const auto& Step : m_vSteps[StepPos])
	{
		CQuestStep Base;
		Base.m_ClientID = ClientID;
		Base.m_Bot = Step.m_Bot;
		Base.m_StepComplete = false;
		Base.m_ClientQuitting = false;
		Base.m_aMobProgress.clear();
		(*pElem).push_back(std::move(Base));
	}
}

CGS* CPlayerQuest::GS() const { return dynamic_cast<CGS*>(Instance::GameServerPlayer(m_ClientID)); }
CPlayer* CPlayerQuest::GetPlayer() const { return GS()->GetPlayer(m_ClientID); }
CQuestDescription* CPlayerQuest::Info() const { return CQuestDescription::Data()[m_ID]; }

CPlayerQuest::~CPlayerQuest()
{
	m_vSteps.clear();
}

bool CPlayerQuest::HasUnfinishedSteps() const
{
	return std::ranges::any_of(m_vSteps, [](const CQuestStep& p) { return !p.m_StepComplete && p.m_Bot.m_HasAction; });
}

bool CPlayerQuest::Accept()
{
	// check valid player and quest state
	CPlayer* pPlayer = GetPlayer();
	if(m_State != QuestState::NO_ACCEPT || !pPlayer)
		return false;

	// initialize
	m_State = QuestState::ACCEPT;
	m_Step = 1;
	m_Datafile.Create();
	Database->Execute<DB::INSERT>("tw_accounts_quests", "(QuestID, UserID, Type) VALUES ('%d', '%d', '%d')", m_ID, GetPlayer()->Account()->GetID(), m_State);

	// handle repeatable quest
	int ClientID = GetPlayer()->GetCID();
	if(Info()->HasFlag(QUEST_FLAG_TYPE_REPEATABLE))
	{
		GS()->Chat(ClientID, "Repeatable quest: '{}' accepted!", Info()->GetName());
	}
	// handle daily quest
	else if(Info()->HasFlag(QUEST_FLAG_TYPE_DAILY))
	{
		GS()->Chat(ClientID, "Daily quest: '{}' accepted!", Info()->GetName());
	}
	// handle weekly quest
	else if(Info()->HasFlag(QUEST_FLAG_TYPE_WEEKLY))
	{
		GS()->Chat(ClientID, "Weekly quest: '{}' accepted!", Info()->GetName());
	}
	// handle main quest
	else if(Info()->HasFlag(QUEST_FLAG_TYPE_MAIN))
	{
		GS()->Chat(ClientID, "Main quest: '{}' accepted!", Info()->GetName());
	}
	// handle other quest
	else
	{
		GS()->Chat(ClientID, "Side quest: '{}' accepted!", Info()->GetName());
	}

	// accepted effects
	GS()->Broadcast(ClientID, BroadcastPriority::TITLE_INFORMATION, 100, "Quest Accepted");
	GS()->CreatePlayerSound(ClientID, SOUND_CTF_GRAB_EN);
	return true;
}

void CPlayerQuest::Refuse()
{
	// check valid player and quest state
	CPlayer* pPlayer = GetPlayer();
	if(m_State != QuestState::ACCEPT || !pPlayer)
		return;

	// refuse quest
	Database->Execute<DB::REMOVE>("tw_accounts_quests", "WHERE QuestID = '%d' AND UserID = '%d'", m_ID, GetPlayer()->Account()->GetID());
	Reset();
}

void CPlayerQuest::Reset()
{
	m_Step = 0;
	m_vSteps.clear();
	m_State = QuestState::NO_ACCEPT;
	m_Datafile.Delete();
}

void CPlayerQuest::UpdateStepPosition()
{
	// check player valid and unfinished steps
	CPlayer* pPlayer = GetPlayer();
	if(!pPlayer || HasUnfinishedSteps())
		return;

	// update
	m_Step++;
	Info()->PreparePlayerSteps(m_Step, m_ClientID, &m_vSteps);
	if(!m_vSteps.empty())
	{
		m_Datafile.Create();
		Update();
		return;
	}

	// apply reward for player
	Info()->Reward().ApplyReward(pPlayer);

	// completion effects
	GS()->Broadcast(m_ClientID, BroadcastPriority::TITLE_INFORMATION, 100, "Quest Complete");
	GS()->EntityManager()->Text(pPlayer->m_ViewPos + vec2(0, -70), 30, "QUEST COMPLETE");
	GS()->CreatePlayerSound(m_ClientID, SOUND_CTF_CAPTURE);

	// handle repeatable type quest
	if(Info()->HasFlag(QUEST_FLAG_TYPE_REPEATABLE))
	{
		GS()->Chat(-1, "{} completed repeatable quest \"{}\".", GS()->Server()->ClientName(m_ClientID), Info()->GetName());
		Refuse();
		return;
	}

	// handle type daily quest
	if(Info()->HasFlag(QUEST_FLAG_TYPE_DAILY))
	{
		pPlayer->GetItem(itAlliedSeals)->Add(g_Config.m_SvDailyQuestAlliedSealsReward);
		GS()->Chat(-1, "{} completed daily quest \"{}\".", GS()->Server()->ClientName(m_ClientID), Info()->GetName());
	}
	// handle type weekly quest
	else if(Info()->HasFlag(QUEST_FLAG_TYPE_WEEKLY))
	{
		GS()->Chat(-1, "{} completed weekly quest \"{}\".", GS()->Server()->ClientName(m_ClientID), Info()->GetName());
	}
	// handle type main quest
	else if(Info()->HasFlag(QUEST_FLAG_TYPE_MAIN))
	{
		GS()->Chat(-1, "{} completed main quest \"{}\".", GS()->Server()->ClientName(m_ClientID), Info()->GetName());
	}
	// handle other type quest
	else
	{
		GS()->Chat(-1, "{} completed side quest \"{}\".", GS()->Server()->ClientName(m_ClientID), Info()->GetName());
	}

	// update quest state in database
	m_State = QuestState::FINISHED;
	Database->Execute<DB::UPDATE>("tw_accounts_quests", "Type = '%d' WHERE QuestID = '%d' AND UserID = '%d'", m_State, m_ID, pPlayer->Account()->GetID());
	m_Datafile.Delete();

	// save player stats and accept next story quest
	GS()->Core()->SaveAccount(pPlayer, SAVE_STATS);
	GS()->Core()->QuestManager()->TryAcceptNextQuestChain(pPlayer, m_ID);
	GS()->Core()->DungeonManager()->NotifyUnlockedDungeonsByQuest(pPlayer, m_ID);
}

void CPlayerQuest::Update()
{
	for(auto& pStep : m_vSteps)
		pStep.Update();

	UpdateStepPosition();
}

CQuestStep* CPlayerQuest::GetStepByMob(int MobID)
{
	auto iter = std::find_if(m_vSteps.begin(), m_vSteps.end(), [MobID](const CQuestStep& Step) { return Step.m_Bot.m_ID == MobID; });
	return iter != m_vSteps.end() ? &(*iter) : nullptr;
}