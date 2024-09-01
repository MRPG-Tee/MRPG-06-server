﻿/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "quest_step_data.h"

#include <game/server/gamecontext.h>
#include <game/server/core/components/Inventory/InventoryManager.h>
#include "quest_manager.h"

#include <game/server/core/components/mails/mail_wrapper.h>
#include <game/server/core/entities/items/drop_quest_items.h>
#include <game/server/core/entities/tools/arrow_navigator.h>
#include <game/server/core/entities/tools/laser_orbite.h>

#include "entities/move_to.h"
#include "entities/path_finder.h"
#include "game/server/entity_manager.h"

void CQuestStepBase::UpdateBot() const
{
	CGS* pGS = (CGS*)Instance::GameServer(m_Bot.m_WorldID);

	// check it's if there's an active bot
	int BotClientID = -1;
	for(int i = MAX_PLAYERS; i < MAX_CLIENTS; i++)
	{
		CPlayer* pPlayer = pGS->GetPlayer(i);
		if(!pPlayer || pPlayer->GetBotType() != TYPE_BOT_QUEST || pPlayer->GetBotMobID() != m_Bot.m_ID)
			continue;

		BotClientID = i;
		break;
	}

	// seek if all players have an active bot
	const bool ActiveStepBot = IsActiveStep();
	if(ActiveStepBot && BotClientID <= -1)
	{
		dbg_msg(PRINT_QUEST_PREFIX, "The mob was not found, but the quest step remains active for players.");
		pGS->CreateBot(TYPE_BOT_QUEST, m_Bot.m_BotID, m_Bot.m_ID);
	}
	// if the bot is not active for more than one player
	if(!ActiveStepBot && BotClientID >= MAX_PLAYERS)
	{
		dbg_msg(PRINT_QUEST_PREFIX, "The mob was found, but the quest step is not active for players.");
		pGS->DestroyPlayer(BotClientID);
	}
}

bool CQuestStepBase::IsActiveStep() const
{
	CGS* pGS = (CGS*)Instance::GameServer(m_Bot.m_WorldID);
	const int QuestID = m_Bot.m_QuestID;
	const int QuestBotID = m_Bot.m_ID;

	for(int i = 0; i < MAX_PLAYERS; i++)
	{
		// check valid player
		CPlayer* pPlayer = pGS->GetPlayer(i);
		if(!pPlayer || !pPlayer->IsAuthed())
			continue;

		// invalid quest data
		if(!CQuestDescription::Data().contains(QuestID))
			continue;

		// skip is not accepted quest or not same quest step
		CPlayerQuest* pQuest = pPlayer->GetQuest(QuestID);
		if(pQuest->GetState() != QuestState::ACCEPT || pQuest->GetStepPos() != m_Bot.m_StepPos)
			continue;

		// skip some step actions
		CQuestStep* pStep = pQuest->GetStepByMob(QuestBotID);
		if(pStep->m_StepComplete || pStep->m_ClientQuitting)
			continue;

		return true;
	}
	return false;
}
// ##############################################################
// ################# PLAYER STEP STRUCTURE ######################
CGS* CQuestStep::GS() const { return (CGS*)Instance::GameServerPlayer(m_ClientID); }
CPlayer* CQuestStep::GetPlayer() const { return GS()->GetPlayer(m_ClientID); }

CQuestStep::~CQuestStep()
{
	m_ClientQuitting = true;

	// update bot and path navigator
	CQuestStepBase::UpdateBot();
	UpdatePathNavigator();

	// clear the move actions
	auto pGS = GS();
	for(auto& pEnt : m_vpEntitiesAction)
	{
		if(pEnt && (pEnt->IsMarkedForDestroy() || !pGS->m_World.ExistEntity(pEnt)))
			pEnt->MarkForDestroy();
	}
	m_vpEntitiesAction.clear();

	// clear the navigators
	for(auto& pEnt : m_apEntitiesNavigator)
	{
		if(pEnt && (pEnt->IsMarkedForDestroy() || !pGS->m_World.ExistEntity(pEnt)))
			pEnt->MarkForDestroy();
	}
	m_apEntitiesNavigator.clear();

	m_aMobProgress.clear();
	m_aMoveToProgress.clear();
}

int CQuestStep::GetNumberBlockedItem(int ItemID) const
{
	int Amount = 0;
	for(auto& p : m_Bot.m_vRequiredItems)
	{
		if(p.m_Item.GetID() == ItemID)
			Amount += p.m_Item.GetValue();
	}
	return Amount;
}

bool CQuestStep::IsComplete()
{
	// check if all required items are gathered
	if(!m_Bot.m_vRequiredItems.empty())
	{
		for(auto& p : m_Bot.m_vRequiredItems)
		{
			if(GetPlayer()->GetItem(p.m_Item)->GetValue() < p.m_Item.GetValue())
				return false;
		}
	}

	// check if all required defeats are met
	if(!m_Bot.m_vRequiredDefeats.empty())
	{
		for(auto& [botID, requiredCount] : m_Bot.m_vRequiredDefeats)
		{
			if(m_aMobProgress[botID].m_Count < requiredCount)
				return false;
		}
	}

	// check if all move-to actions are completed
	if(GetCompletedMoveToCount() < (int)m_aMoveToProgress.size())
		return false;

	return true;
}

bool CQuestStep::Finish()
{
	// initialize variables
	CPlayer* pPlayer = GetPlayer();
	const int QuestID = m_Bot.m_QuestID;

	// check is competed quest
	if(!IsComplete())
		return false;

	// set flag to complete
	m_StepComplete = true;

	// save quest progress
	if(!pPlayer->GetQuest(QuestID)->m_Datafile.Save())
	{
		GS()->Chat(pPlayer->GetCID(), "A system error has occurred, contact administrator.");
		dbg_msg(PRINT_QUEST_PREFIX, "After completing the quest step, unable to save the file.");
		m_StepComplete = false;
		return false;
	}

	// apply post finish
	PostFinish();
	return true;
}

void CQuestStep::PostFinish()
{
	bool AntiDatabaseStress = false;
	CPlayer* pPlayer = GetPlayer();
	int ClientID = pPlayer->GetCID();
	ska::unordered_set<int> vInteractItemIds {};

	// required item's
	if(!m_Bot.m_vRequiredItems.empty())
	{
		for(auto& pRequired : m_Bot.m_vRequiredItems)
		{
			// show type element
			CPlayerItem* pPlayerItem = pPlayer->GetItem(pRequired.m_Item);

			if(pRequired.m_Type == QuestBotInfo::TaskRequiredItems::Type::SHOW)
			{
				GS()->Chat(pPlayer->GetCID(), "[Done] Show the {}x{} to the {}!", pPlayerItem->Info()->GetName(), pRequired.m_Item.GetValue(), m_Bot.GetName());
				continue;
			}

			// remove item
			vInteractItemIds.emplace(pPlayerItem->GetID());
			pPlayerItem->Remove(pRequired.m_Item.GetValue());
			GS()->Chat(pPlayer->GetCID(), "[Done] Give the {}x{} to the {}!", pPlayerItem->Info()->GetName(), pRequired.m_Item.GetValue(), m_Bot.GetName());
		}
	}

	// reward item's
	if(!m_Bot.m_RewardItems.empty())
	{
		for(auto& pRewardItem : m_Bot.m_RewardItems)
		{
			// no use same giving and receiving for it can use "show"
			dbg_assert(vInteractItemIds.find(pRewardItem.GetID()) != vInteractItemIds.end(), "the quest has (the same item of giving and receiving)");

			// check for enchant item
			CPlayerItem* pPlayerItem = pPlayer->GetItem(pRewardItem);

			// give item
			pPlayerItem->Add(pRewardItem.GetValue());
		}
	}

	// update bot status
	DataBotInfo::ms_aDataBot[m_Bot.m_BotID].m_aVisibleActive[ClientID] = false;
	pPlayer->GetQuest(m_Bot.m_QuestID)->Update();
	pPlayer->m_VotesData.UpdateVotesIf(MENU_JOURNAL_MAIN);
}

void CQuestStep::AppendDefeatProgress(int DefeatedBotID)
{
	// check default action
	CPlayer* pPlayer = GetPlayer();
	if(m_StepComplete || m_ClientQuitting || m_Bot.m_vRequiredDefeats.empty() || !pPlayer || !DataBotInfo::IsDataBotValid(DefeatedBotID))
		return;

	// check quest action
	CPlayerQuest* pQuest = pPlayer->GetQuest(m_Bot.m_QuestID);
	if(pQuest->GetState() != QuestState::ACCEPT || pQuest->GetStepPos() != m_Bot.m_StepPos)
		return;

	// check complecte mob
	for(auto& [DefeatBotID, DefeatCount] : m_Bot.m_vRequiredDefeats)
	{
		if(DefeatedBotID != DefeatBotID || m_aMobProgress[DefeatedBotID].m_Count >= DefeatCount)
			continue;

		m_aMobProgress[DefeatedBotID].m_Count++;
		if(m_aMobProgress[DefeatedBotID].m_Count >= DefeatCount)
		{
			m_aMobProgress[DefeatBotID].m_Complete = true;
			GS()->Chat(pPlayer->GetCID(), "[Done] Defeat the {}'s for the {}!", DataBotInfo::ms_aDataBot[DefeatedBotID].m_aNameBot, m_Bot.GetName());
		}

		pQuest->m_Datafile.Save();
		break;
	}
}

void CQuestStep::UpdatePathNavigator()
{
	// skip if the bot is without action
	if(!m_Bot.m_HasAction)
		return;

	CPlayer* pPlayer = GetPlayer();
	const bool Exists = m_pEntNavigator && GS()->m_World.ExistEntity(m_pEntNavigator);
	const bool DependLife = !m_StepComplete && !m_ClientQuitting && pPlayer && pPlayer->GetCharacter();

	if(!DependLife && Exists)
	{
		dbg_msg("test", "delete navigator");
		delete m_pEntNavigator;
		m_pEntNavigator = nullptr;
	}
	else if(DependLife && !Exists)
	{
		dbg_msg("test", "create navigator");
		m_pEntNavigator = new CEntityArrowNavigator(&GS()->m_World, m_ClientID, m_Bot.m_Position, m_Bot.m_WorldID);
	}
}

void CQuestStep::UpdateTaskMoveTo()
{
	// check default action
	CPlayer* pPlayer = GetPlayer();
	if(!m_TaskListReceived || m_StepComplete || m_ClientQuitting || !pPlayer || !pPlayer->GetCharacter())
		return;

	// check quest action
	CPlayerQuest* pQuest = pPlayer->GetQuest(m_Bot.m_QuestID);
	if(pQuest->GetState() != QuestState::ACCEPT || pQuest->GetStepPos() != m_Bot.m_StepPos)
		return;

	// check and mark required mob's
	for(auto& [DefeatBotID, DefeatCount] : m_Bot.m_vRequiredDefeats)
	{
		if(m_aMobProgress[DefeatBotID].m_Count >= DefeatCount)
			continue;

		if(const MobBotInfo* pMob = DataBotInfo::FindMobByBot(DefeatBotID))
		{
			UpdateEntityArrowNavigator(pMob->m_Position, pMob->m_WorldID, 400.f, &m_aMobProgress[DefeatBotID].m_Complete);
		}
	}

	// check and add entities
	if(!m_aMoveToProgress.empty())
	{
		const int CurrentStep = GetMoveToCurrentStepPos();
		for(int i = 0; i < (int)m_Bot.m_vRequiredMoveAction.size(); i++)
		{
			// skip completed and not current step's
			QuestBotInfo::TaskAction& pRequired = m_Bot.m_vRequiredMoveAction[i];
			if(CurrentStep != pRequired.m_Step || m_aMoveToProgress[i])
				continue;

			// Always creating navigator in other worlds 
			if(pRequired.m_WorldID != pPlayer->GetPlayerWorldID())
			{
				UpdateEntityArrowNavigator(pRequired.m_Position, pRequired.m_WorldID, 0.f, &m_aMoveToProgress[i]);
				continue;
			}

			// Add move to point questing mob
			CPlayerBot* pPlayerBot = nullptr;
			if(pRequired.IsHasDefeatMob())
			{
				for(int c = MAX_PLAYERS; c < MAX_CLIENTS; c++)
				{
					CPlayerBot* pPlBotSearch = dynamic_cast<CPlayerBot*>(GS()->GetPlayer(c));
					if(pPlBotSearch && pPlBotSearch->GetQuestBotMobInfo().m_QuestID == pQuest->GetID() &&
						pPlBotSearch->GetQuestBotMobInfo().m_QuestStep == m_Bot.m_StepPos &&
						pPlBotSearch->GetQuestBotMobInfo().m_MoveToStep == i)
					{
						pPlayerBot = pPlBotSearch;
						break;
					}
				}

				if(!pPlayerBot)
				{
					int MobClientID = GS()->CreateBot(TYPE_BOT_QUEST_MOB, pRequired.m_DefeatMobInfo.m_BotID, -1);
					pPlayerBot = dynamic_cast<CPlayerBot*>(GS()->GetPlayer(MobClientID));
					pPlayerBot->InitQuestBotMobInfo(
						{
							m_Bot.m_QuestID,
							m_Bot.m_StepPos,
							i,
							pRequired.m_DefeatMobInfo.m_AttributePower,
							pRequired.m_DefeatMobInfo.m_AttributeSpread,
							pRequired.m_DefeatMobInfo.m_WorldID,
							pRequired.m_Position
						});

					dbg_msg(PRINT_QUEST_PREFIX, "Creating a quest mob");
				}

				pPlayerBot->GetQuestBotMobInfo().m_ActiveForClient[pPlayer->GetCID()] = true;
				pPlayerBot->GetQuestBotMobInfo().m_CompleteClient[pPlayer->GetCID()] = false;
			}

			// Check if there is a move-to entity at the required position
			bool MarkNewItem;
			CEntityQuestAction* pEntMoveTo = UpdateEntityQuestAction(&MarkNewItem, pRequired, &m_aMoveToProgress[i], pPlayerBot);
			if(MarkNewItem)
			{
				// Check if the required task is navigation or defeating a mob
				if(!pRequired.m_Navigator || pRequired.m_Type == QuestBotInfo::TaskAction::Types::DEFEAT_MOB)
				{
					// Create orbital path and navigator for it
					float Radius;
					CEntityLaserOrbite* pEntOrbite;

					// If the required task is to defeat a mob, set a smaller radius for the orbit
					if(pRequired.m_Type == QuestBotInfo::TaskAction::Types::DEFEAT_MOB)
					{
						Radius = 400.f;
						GS()->EntityManager()->LaserOrbite(pEntOrbite, pEntMoveTo, (int)(Radius / 50.f),
							LaserOrbiteType::INSIDE_ORBITE, Radius, LASERTYPE_FREEZE, CmaskOne(pPlayer->GetCID()));
					}
					else
					{
						Radius = 400.f + random_float(2000.f);
						GS()->EntityManager()->LaserOrbite(pEntOrbite, (int)(Radius / 50.f),
							LaserOrbiteType::INSIDE_ORBITE_RANDOM, Radius, LASERTYPE_FREEZE, CmaskOne(pPlayer->GetCID()));
					}

					// Add navigator to the orbital path
					UpdateEntityArrowNavigator(pEntOrbite->GetPos(), pRequired.m_WorldID, Radius, &m_aMoveToProgress[i]);
					continue;
				}

				// Add navigator to the orbital path
				UpdateEntityArrowNavigator(pRequired.m_Position, pRequired.m_WorldID, 0.f, &m_aMoveToProgress[i]);
			}
		}
	}
}

void CQuestStep::Update()
{
	UpdateBot();
	UpdatePathNavigator();
	UpdateTaskMoveTo();
}

void CQuestStep::CreateVarietyTypesRequiredItems()
{
	// check default action
	CPlayer* pPlayer = GetPlayer();
	if(m_StepComplete || m_ClientQuitting || m_Bot.m_vRequiredItems.empty() || !pPlayer || !pPlayer->GetCharacter())
		return;

	// check quest action
	CPlayerQuest* pQuest = pPlayer->GetQuest(m_Bot.m_QuestID);
	if(pQuest->GetState() != QuestState::ACCEPT || pQuest->GetStepPos() != m_Bot.m_StepPos)
		return;

	// create variety types
	const int ClientID = pPlayer->GetCID();
	for(auto& [RequiredItem, Type] : m_Bot.m_vRequiredItems)
	{
		// TYPE Drop and Pick up
		if(Type == QuestBotInfo::TaskRequiredItems::Type::PICKUP)
		{
			// check whether items are already available for pickup
			for(CDropQuestItem* pHh = (CDropQuestItem*)GS()->m_World.FindFirst(CGameWorld::ENTTYPE_QUEST_DROP); pHh; pHh = (CDropQuestItem*)pHh->TypeNext())
			{
				if(pHh->m_ClientID == ClientID && pHh->m_QuestID == m_Bot.m_QuestID && pHh->m_ItemID == RequiredItem.GetID() && pHh->m_Step == m_Bot.m_StepPos)
					return;
			}

			// create items
			const int Value = 3 + RequiredItem.GetValue();
			for(int i = 0; i < Value; i++)
			{
				vec2 Vel = vec2(random_float(-40.0f, 40.0f), random_float(-40.0f, 40.0f));
				float AngleForce = Vel.x * (0.15f + random_float(0.1f));
				new CDropQuestItem(&GS()->m_World, m_Bot.m_Position, Vel, AngleForce, RequiredItem.GetID(), RequiredItem.GetValue(), m_Bot.m_QuestID, m_Bot.m_StepPos, ClientID);
			}
		}

		// TODO: add new types
	}
}

void CQuestStep::FormatStringTasks(char* aBufQuestTask, int Size)
{
	CPlayer* pPlayer = GetPlayer();
	if(!pPlayer)
		return;

	std::string strBuffer {};
	const char* pLang = pPlayer->GetLanguage();

	// show required bots
	if(!m_Bot.m_vRequiredDefeats.empty())
	{
		strBuffer += "\n\n" + fmt_localize(m_ClientID, "- \u270E Slay enemies:");
		for(auto& p : m_Bot.m_vRequiredDefeats)
		{
			const char* pCompletePrefix = (m_aMobProgress[p.m_BotID].m_Count >= p.m_RequiredCount ? "\u2611" : "\u2610");
			strBuffer += "\n" + fmt_localize(m_ClientID, "{} Defeat {} ({}/{})",
				pCompletePrefix, DataBotInfo::ms_aDataBot[p.m_BotID].m_aNameBot, m_aMobProgress[p.m_BotID].m_Count, p.m_RequiredCount);
		}
	}

	// show required items
	if(!m_Bot.m_vRequiredItems.empty())
	{
		strBuffer += "\n\n" + fmt_localize(m_ClientID, "- \u270E Retrieve an item's:");
		for(auto& pRequied : m_Bot.m_vRequiredItems)
		{
			CPlayerItem* pPlayerItem = pPlayer->GetItem(pRequied.m_Item);
			const char* pCompletePrefix = (pPlayerItem->GetValue() >= pRequied.m_Item.GetValue() ? "\u2611" : "\u2610");
			const char* pInteractiveType = pRequied.m_Type == QuestBotInfo::TaskRequiredItems::Type::SHOW ? "Show a" : "Require a";
			strBuffer += "\n" + fmt_localize(m_ClientID, "{} {} {} ({}/{}).",
				pCompletePrefix, pInteractiveType, pPlayerItem->Info()->GetName(), pPlayerItem->GetValue(), pRequied.m_Item.GetValue());
		}
	}

	// show move to
	if(!m_Bot.m_vRequiredMoveAction.empty())
	{
		strBuffer += "\n\n" + fmt_localize(m_ClientID, "- \u270E Trigger some action's:");

		// Create an unordered map called m_Order with key type int and value type unordered_map<string, pair<int, int>> for special order task's
		std::map<int /* step */, ska::unordered_map<std::string /* task name */, std::pair<int /* complected */, int /* count */>>> m_Order;
		for(int i = 0; i < (int)m_Bot.m_vRequiredMoveAction.size(); i++)
		{
			// If TaskMapID is empty, assign it the value "Demands a bit of action"
			std::string TaskMapID = m_Bot.m_vRequiredMoveAction[i].m_TaskName;

			// If m_aMoveToProgress[i] is true, increment the first value of the pair in m_Order[Step][TaskMapID]
			int Step = m_Bot.m_vRequiredMoveAction[i].m_Step;
			if(m_aMoveToProgress[i])
				m_Order[Step][TaskMapID].first++;

			// Increment the second value of the pair in m_Order[Step][TaskMapID]
			m_Order[Step][TaskMapID].second++;
		}

		// Loop through each element in m_Order
		for(auto& [Step, TaskMap] : m_Order)
		{
			// Loop through each element in TaskMap
			for(auto& [Name, StepCount] : TaskMap)
			{
				// Append a newline character to Buffer
				strBuffer += "\n";

				// Check for one task
				const int& TaskNum = StepCount.second;
				const int& TaskCompleted = StepCount.first;
				const char* pCompletePrefix = (TaskCompleted >= TaskNum ? "\u2611" : "\u2610");
				if(TaskNum == 1)
				{
					strBuffer += fmt_localize(m_ClientID, "{}. {} {}.", Step, pCompletePrefix, Name.c_str());
					continue;
				}

				// Multi task
				strBuffer += fmt_localize(m_ClientID, "{}. {} {} ({}/{}).", Step, pCompletePrefix, Name.c_str(), TaskCompleted, TaskNum);
			}
		}
	}

	// show reward items
	if(!m_Bot.m_RewardItems.empty())
	{
		strBuffer += "\n\n" + fmt_localize(m_ClientID, "- \u270E Reward for completing a task:");
		for(auto& p : m_Bot.m_RewardItems)
			strBuffer += "\n" + fmt_localize(m_ClientID, "Obtain a {} ({}).", p.Info()->GetName(), p.GetValue());
	}

	// Copy the contents of the buffer `Buffer` into the character array `aBufQuestTask`,
	str_copy(aBufQuestTask, strBuffer.c_str(), Size);
}

int CQuestStep::GetMoveToNum() const
{
	return (int)m_aMoveToProgress.size();
}

int CQuestStep::GetMoveToCurrentStepPos() const
{
	for(int i = 0; i < (int)m_Bot.m_vRequiredMoveAction.size(); i++)
	{
		if(m_aMoveToProgress[i])
			continue;

		return m_Bot.m_vRequiredMoveAction[i].m_Step;
	}

	return 1;
}

// This function returns the count of completed move steps in a player quest
int CQuestStep::GetCompletedMoveToCount()
{
	// Using std::count_if to count the number of elements in m_aMoveToProgress that satisfy the condition
	// The condition is a lambda function that checks if the element is true
	return (int)std::count_if(m_aMoveToProgress.begin(), m_aMoveToProgress.end(), [](const bool State){return State == true; });
}

CEntityQuestAction* CQuestStep::UpdateEntityQuestAction(bool* pMarkIsNewItem, const QuestBotInfo::TaskAction& TaskMoveTo, bool* pComplete, CPlayerBot* pDefeatMobPlayer)
{
	// erase the move to if it does not exist
	m_vpEntitiesAction.erase(std::remove_if(m_vpEntitiesAction.begin(), m_vpEntitiesAction.end(),
		[pGS = GS()](CEntityQuestAction* p) { return (p && p->IsMarkedForDestroy()) || !pGS->m_World.ExistEntity(p); }), m_vpEntitiesAction.end());

	// find the bot navigator by the position
	const auto iter = std::find_if(m_vpEntitiesAction.begin(), m_vpEntitiesAction.end(),
		[Position = TaskMoveTo.m_Position](CEntityQuestAction* p) { return p && p->GetPos() == Position; });

	// create a new move to item if it does not exist
	*pMarkIsNewItem = false;
	CEntityQuestAction* pEntQuestAction = (iter != m_vpEntitiesAction.end()) ? *iter : nullptr;
	if(pComplete && !(*pComplete) && !pEntQuestAction)
	{
		*pMarkIsNewItem = true;
		pEntQuestAction = new CEntityQuestAction(&GS()->m_World, TaskMoveTo, GetPlayer()->GetCID(), m_Bot.m_QuestID, pComplete, m_Bot.IsAutoCompletesQuestStep(), pDefeatMobPlayer);
		m_vpEntitiesAction.emplace_back(pEntQuestAction);
	}

	return pEntQuestAction;
}

CEntityPathArrow* CQuestStep::UpdateEntityArrowNavigator(vec2 Position, int WorldID, float AreaClipped, bool* pComplete)
{
	// erase the bot navigator if it does not exist
	m_apEntitiesNavigator.erase(std::remove_if(m_apEntitiesNavigator.begin(), m_apEntitiesNavigator.end(),
		[pGS = GS()](CEntityPathArrow* p) { return (p && p->IsMarkedForDestroy())|| !pGS->m_World.ExistEntity(p); }), m_apEntitiesNavigator.end());

	// find the bot navigator by the position
	const auto iter = std::find_if(m_apEntitiesNavigator.begin(), m_apEntitiesNavigator.end(),
		[&Position](CEntityPathArrow* p) { return p && p->GetPosTo() == Position; });

	// create a new arrow navigator if it does not exist
	CEntityPathArrow* pEntArrow = (iter != m_apEntitiesNavigator.end()) ? *iter : nullptr;
	if(pComplete && !(*pComplete) && !pEntArrow)
	{
		pEntArrow = new CEntityPathArrow(&GS()->m_World, Position, WorldID, GetPlayer()->GetCID(), AreaClipped, pComplete);
		m_apEntitiesNavigator.emplace_back(pEntArrow);
	}
	return pEntArrow;
}
