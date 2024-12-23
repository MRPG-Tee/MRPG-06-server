#include "game/server/core/components/Bots/BotData.h"
#include "move_action.h"

#include <game/server/gamecontext.h>
#include "game/server/core/components/quests/quest_manager.h"

constexpr unsigned int s_Particles = 4;

CEntityQuestAction::CEntityQuestAction(CGameWorld* pGameWorld, int ClientID, int MoveToIndex, 
	const std::weak_ptr<CQuestStep>& pStep, bool AutoCompletesQuestStep, std::optional<int> optDefeatBotCID)
	: CEntity(pGameWorld, CGameWorld::ENTTYPE_MOVE_TO_POINT, {}, 32.f, ClientID), m_MoveToIndex(MoveToIndex)
{
	// Initialize base
	m_pStep = pStep;
	m_optDefeatBotCID = optDefeatBotCID;
	m_AutoCompletesQuestStep = AutoCompletesQuestStep;
	if(const auto* pTaskData = GetTaskMoveTo())
	{
		m_Pos = pTaskData->m_Position;
		m_Radius = (pTaskData->m_TypeFlags & QuestBotInfo::TaskAction::Types::DEFEAT_MOB) ? 400.f : 32.f;
	}
	GameWorld()->InsertEntity(this);

	// initialize snap ids
	m_IDs.set_size(s_Particles);
	for(int i = 0; i < m_IDs.size(); i++)
		m_IDs[i] = Server()->SnapNewID();
}

CEntityQuestAction::~CEntityQuestAction()
{
	// update player progress
	if(CPlayer* pPlayer = GetPlayer())
	{
		GS()->Core()->QuestManager()->Update(pPlayer);
	}

	// update defeat bot
	if(CPlayerBot* pDefeatBotPlayer = GetDefeatPlayerBot())
	{
		auto& QuestBotInfo = pDefeatBotPlayer->GetQuestBotMobInfo();
		QuestBotInfo.m_ActiveForClient[m_ClientID] = false;
		QuestBotInfo.m_CompleteClient[m_ClientID] = false;

		bool ClearDefeatMobPlayer = true;
		for(int i = 0; i < MAX_PLAYERS; ++i)
		{
			if(QuestBotInfo.m_ActiveForClient[i])
			{
				ClearDefeatMobPlayer = false;
				break;
			}
		}

		if(ClearDefeatMobPlayer)
		{
			GS()->DestroyPlayer(pDefeatBotPlayer->GetCID());
			dbg_msg(PRINT_QUEST_PREFIX, "Deleted questing mob");
		}
	}

	// free ids
	for(int i = 0; i < m_IDs.size(); i++)
		Server()->SnapFreeID(m_IDs[i]);
}

void CEntityQuestAction::Destroy()
{
	if(auto* pStep = GetQuestStep())
	{
		std::erase(pStep->m_vpEntitiesAction, shared_from_this());
	}
}

void CEntityQuestAction::Tick()
{
	CPlayer* pPlayer = GetPlayer();
	if(!pPlayer || !pPlayer->GetCharacter())
	{
		GameWorld()->DestroyEntity(this);
		return;
	}

	if(!GetPlayerQuest() || !GetQuestStep())
	{
		GameWorld()->DestroyEntity(this);
		return;
	}

	const auto* pTaskData = GetTaskMoveTo();
	if(!pTaskData)
	{
		GameWorld()->DestroyEntity(this);
		return;
	}

	const bool IsComplected = GetQuestStep()->m_aMoveActionProgress[m_MoveToIndex];
	if(IsComplected)
	{
		GameWorld()->DestroyEntity(this);
		return;
	}

	// check distance
	if(distance(pPlayer->GetCharacter()->GetPos(), m_Pos) > m_Radius)
		return;

	// handlers
	HandleTaskType(pTaskData);
	HandleBroadcastInformation(pTaskData);
}

bool CEntityQuestAction::PressedFire() const
{
	return Server()->Input()->IsKeyClicked(m_ClientID, KEY_EVENT_FIRE_HAMMER);
}

void CEntityQuestAction::Handler(const std::function<bool()>& pCallbackSuccesful)
{
	if(!pCallbackSuccesful())
		return;

	CPlayer* pPlayer = GetPlayer();
	const auto* pTaskData = GetTaskMoveTo();
	if(pTaskData->m_Cooldown > 0)
	{
		pPlayer->m_Cooldown.Start(pTaskData->m_Cooldown, pTaskData->m_TaskName, std::bind(&CEntityQuestAction::TryFinish, this));
	}
	else
	{
		TryFinish();
	}
}

void CEntityQuestAction::TryFinish()
{
	CPlayer* pPlayer = GetPlayer();
	CPlayerQuest* pQuest = GetPlayerQuest();
	CQuestStep* pQuestStep = GetQuestStep();
	const auto* pTaskData = GetTaskMoveTo();

	// required item
	if(pTaskData->m_TypeFlags & QuestBotInfo::TaskAction::Types::REQUIRED_ITEM && pTaskData->m_RequiredItem.IsValid())
	{
		ItemIdentifier ItemID = pTaskData->m_RequiredItem.GetID();
		int RequiredValue = pTaskData->m_RequiredItem.GetValue();
		if(!pPlayer->Account()->SpendCurrency(RequiredValue, ItemID))
			return;

		CPlayerItem* pPlayerItem = pPlayer->GetItem(ItemID);
		GS()->Chat(m_ClientID, "You've used on the point {} x{}", pPlayerItem->Info()->GetName(), RequiredValue);
	}

	// pickup item
	if(pTaskData->m_TypeFlags & QuestBotInfo::TaskAction::Types::PICKUP_ITEM && pTaskData->m_PickupItem.IsValid())
	{
		ItemIdentifier ItemID = pTaskData->m_PickupItem.GetID();
		int PickupValue = pTaskData->m_PickupItem.GetValue();
		CPlayerItem* pPlayerItem = pPlayer->GetItem(ItemID);

		pPlayerItem->Add(PickupValue);
		GS()->Chat(m_ClientID, "You've picked up {} x{}.", pPlayerItem->Info()->GetName(), PickupValue);
	}

	// Completion text
	if(!pTaskData->m_CompletionText.empty())
	{
		GS()->Chat(m_ClientID, pTaskData->m_CompletionText.c_str());
	}

	// Set the complete flag to true
	GetQuestStep()->m_aMoveActionProgress[m_MoveToIndex] = true;
	pQuest->Datafile().Save();

	// Create a death entity at the current position and destroy this entity
	GS()->CreateDeath(m_Pos, m_ClientID);
	GameWorld()->DestroyEntity(this);

	// Finish the quest step if AutoCompleteQuestStep is true
	if(m_AutoCompletesQuestStep)
	{
		const bool IsLastElement = (pQuestStep->GetCompletedMoveActionCount() == pQuestStep->GetMoveActionNum());
		if(IsLastElement && pQuestStep->IsComplete())
			pQuestStep->Finish();
	}
}

CPlayer* CEntityQuestAction::GetPlayer() const
{
	return GS()->GetPlayer(m_ClientID);
}

CPlayerBot* CEntityQuestAction::GetDefeatPlayerBot() const
{
	if(m_optDefeatBotCID.has_value())
		return dynamic_cast<CPlayerBot*>(GS()->GetPlayer(m_optDefeatBotCID.value()));
	return nullptr;
}

CPlayerQuest* CEntityQuestAction::GetPlayerQuest() const
{
	CPlayer* pPlayer = GetPlayer();
	if(!pPlayer)
		return nullptr;

	if(const auto pStep = m_pStep.lock())
	{
		return pPlayer->GetQuest(pStep->m_Bot.m_QuestID);
	}
	return nullptr;
}

CQuestStep* CEntityQuestAction::GetQuestStep() const
{
	if(const auto pStep = m_pStep.lock())
	{
		return pStep.get();
	}
	return nullptr;
}

QuestBotInfo::TaskAction* CEntityQuestAction::GetTaskMoveTo() const
{
	if(const auto pStep = m_pStep.lock())
	{
		return &pStep->m_Bot.m_vRequiredMoveAction[m_MoveToIndex];
	}
	return nullptr;
}

void CEntityQuestAction::Snap(int SnappingClient)
{
	if(m_ClientID != SnappingClient)
		return;

	for(int i = 0; i < m_IDs.size(); i++)
	{
		vec2 randomRangePos = random_range_pos(m_Pos, m_Radius);
		GS()->SnapProjectile(SnappingClient, m_IDs[i], randomRangePos, { }, Server()->Tick() - 3, WEAPON_HAMMER, m_ClientID);
	}
}

void CEntityQuestAction::HandleTaskType(const QuestBotInfo::TaskAction* pTaskData)
{
	const unsigned TypeFlags = pTaskData->m_TypeFlags;

	// defeat type flag
	if(TypeFlags & QuestBotInfo::TaskAction::Types::DEFEAT_MOB)
	{
		Handler([this]
		{
			CPlayerBot* pDefeatMobPlayer = GetDefeatPlayerBot();
			if(!pDefeatMobPlayer)
				return true;
			return pDefeatMobPlayer && pDefeatMobPlayer->GetQuestBotMobInfo().m_CompleteClient[m_ClientID];
		});
	}
	// move only type flag
	else if(TypeFlags & QuestBotInfo::TaskAction::Types::MOVE_ONLY)
	{
		Handler([this] { return true; });
	}
	// interactive flag
	else if(TypeFlags & QuestBotInfo::TaskAction::Types::INTERACTIVE)
	{
		if(Server()->Tick() % (Server()->TickSpeed() / 3) == 0)
		{
			GS()->CreateHammerHit(pTaskData->m_Interaction.m_Position, CmaskOne(m_ClientID));
		}
		Handler([this, pTaskData]
		{
			return PressedFire() && distance(GetPlayer()->GetCharacter()->GetMousePos(), pTaskData->m_Interaction.m_Position) < 48.f;
		});
	}
	// pickup or required item
	else if(TypeFlags & (QuestBotInfo::TaskAction::PICKUP_ITEM | QuestBotInfo::TaskAction::REQUIRED_ITEM))
	{
		Handler([this] { return PressedFire(); });
	}
}

void CEntityQuestAction::HandleBroadcastInformation(const QuestBotInfo::TaskAction* pTaskData) const
{
	CPlayer* pPlayer = GetPlayer();
	const auto& pPickupItem = pTaskData->m_PickupItem;
	const auto& pRequireItem = pTaskData->m_RequiredItem;
	const auto Type = pTaskData->m_TypeFlags;

	// skip defeat mob
	if(Type & QuestBotInfo::TaskAction::Types::DEFEAT_MOB)
		return;

	// formating
	std::string strBuffer;
	if(pRequireItem.IsValid())
	{
		CPlayerItem* pPlayerItem = pPlayer->GetItem(pRequireItem.GetID());
		strBuffer += fmt_localize(m_ClientID, "- Required: {} ({} | {})\n",
			pPlayerItem->Info()->GetName(), pRequireItem.GetValue(), pPlayerItem->GetValue());
	}

	if(pPickupItem.IsValid())
	{
		CPlayerItem* pPlayerItem = pPlayer->GetItem(pPickupItem.GetID());
		strBuffer += fmt_localize(m_ClientID, "- Pick up: {} ({} | {})\n",
			pPlayerItem->Info()->GetName(), pPickupItem.GetValue(), pPlayerItem->GetValue());
	}

	// send broadcast
	if(Type & QuestBotInfo::TaskAction::Types::INTERACTIVE)
	{
		GS()->Broadcast(m_ClientID, BroadcastPriority::MainInformation, 10, "Click the highlighted area with the hammer to interact.\n{}", strBuffer.c_str());
	}
	else if(Type & (QuestBotInfo::TaskAction::Types::PICKUP_ITEM | QuestBotInfo::TaskAction::Types::REQUIRED_ITEM))
	{
		GS()->Broadcast(m_ClientID, BroadcastPriority::MainInformation, 10, "Press 'Fire' with the hammer to interact.\n{}", strBuffer.c_str());
	}

}