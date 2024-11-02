﻿/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "player.h"

#include "gamecontext.h"
#include "worldmodes/dungeon.h"

#include "core/components/Accounts/AccountManager.h"
#include "core/components/achievements/achievement_manager.h"
#include "core/components/Bots/BotManager.h"
#include "core/components/Dungeons/DungeonData.h"
#include "core/components/Eidolons/EidolonInfoData.h"
#include "core/components/guilds/guild_manager.h"
#include "core/components/quests/quest_manager.h"

#include "core/components/Inventory/ItemData.h"
#include "core/components/skills/skill_data.h"
#include "core/components/groups/group_data.h"
#include "core/components/worlds/world_data.h"

#include "core/tools/vote_optional.h"

MACRO_ALLOC_POOL_ID_IMPL(CPlayer, MAX_CLIENTS * ENGINE_MAX_WORLDS + MAX_CLIENTS)

IServer* CPlayer::Server() const { return m_pGS->Server(); };

CPlayer::CPlayer(CGS* pGS, int ClientID) : m_pGS(pGS), m_ClientID(ClientID)
{
	m_aPlayerTick[Die] = Server()->Tick();
	m_aPlayerTick[Respawn] = Server()->Tick() + Server()->TickSpeed();
	m_SnapHealthNicknameTick = 0;

	m_WantSpawn = true;
	m_PrevTuningParams = *pGS->Tuning();
	m_NextTuningParams = m_PrevTuningParams;
	m_Scenarios.Init(ClientID);
	m_Cooldown.Init(ClientID);
	m_VotesData.Init(m_pGS, this);
	m_Dialog.Init(this);

	// constructor only for players
	if(m_ClientID < MAX_PLAYERS)
	{
		m_MoodState = Mood::Normal;
		GS()->SendTuningParams(ClientID);

		m_Afk = false;
		m_pLastInput = new CNetObj_PlayerInput({ 0 });
		m_LastInputInit = false;
		m_LastPlaytime = 0;
	}
}

CPlayer::~CPlayer()
{
	// free data
	if(m_pCharacter)
	{
		delete m_pCharacter;
		m_pCharacter = nullptr;
	}
	delete m_pLastInput;
}

void CPlayer::GetFormatedName(char* aBuffer, int BufferSize)
{
	const auto isChatting = m_PlayerFlags & PLAYERFLAG_CHATTING;
	const auto isAuthed = IsAuthed();
	const auto currentTick = Server()->Tick();
	const auto tickSpeed = Server()->TickSpeed();

	// Player is not chatting and health nickname tick is valid
	if(!isChatting && currentTick < m_SnapHealthNicknameTick)
	{
		char aHealthProgressBuf[6];
		char aNicknameBuf[MAX_NAME_LENGTH];
		const int PercentHP = round_to_int(translate_to_percent(GetMaxHealth(), GetHealth()));

		str_format(aHealthProgressBuf, sizeof(aHealthProgressBuf), ":%d%%", clamp(PercentHP, 1, 100));
		str_utf8_truncate(aNicknameBuf, sizeof(aNicknameBuf), Server()->ClientName(m_ClientID), MAX_NAME_LENGTH - 1 - str_length(aHealthProgressBuf));
		str_format(aBuffer, BufferSize, "%s%s", aNicknameBuf, aHealthProgressBuf);
		return;
	}

	// Update nickname leveling tick if player is authenticated and the tick is a multiple of 10 seconds
	if(isAuthed && currentTick % (tickSpeed * 10) == 0)
	{
		m_aPlayerTick[RefreshNickLeveling] = currentTick + tickSpeed;
	}

	// Player is authenticated, nickname leveling tick is valid, and not chatting
	if(isAuthed && m_aPlayerTick[RefreshNickLeveling] > currentTick && !isChatting)
	{
		str_format(aBuffer, BufferSize, "Lv%d %.4s...", Account()->GetLevel(), Server()->ClientName(m_ClientID));
	}
	else
	{
		str_copy(aBuffer, Server()->ClientName(m_ClientID), BufferSize);
	}
}

void CPlayer::Tick()
{
	if(!IsAuthed())
		return;

	// do latency stuff
	{
		int Latency = Server()->GetClientLatency(m_ClientID);
		if(Latency > 0)
		{
			m_Latency.m_Accum += Latency;
			m_Latency.m_AccumMax = maximum(m_Latency.m_AccumMax, Latency);
			m_Latency.m_AccumMin = minimum(m_Latency.m_AccumMin, Latency);
		}
		// each second
		if(Server()->Tick() % Server()->TickSpeed() == 0)
		{
			m_Latency.m_Avg = m_Latency.m_Accum / Server()->TickSpeed();
			m_Latency.m_Max = m_Latency.m_AccumMax;
			m_Latency.m_Min = m_Latency.m_AccumMin;
			m_Latency.m_Accum = 0;
			m_Latency.m_AccumMin = 1000;
			m_Latency.m_AccumMax = 0;
		}

		Server()->SetClientScore(m_ClientID, Account()->GetLevel());
	}

	if(m_pCharacter)
	{
		if(m_pCharacter->IsAlive())
		{
			m_ViewPos = m_pCharacter->GetPos();
		}
		else
		{
			delete m_pCharacter;
			m_pCharacter = nullptr;
		}
	}
	else if(m_WantSpawn && m_aPlayerTick[Respawn] + Server()->TickSpeed() * 3 <= Server()->Tick())
	{
		TryRespawn();
	}

	// update events
	m_FixedView.Tick(m_ViewPos);
	m_Scenarios.Tick();
	m_Cooldown.Tick();
	if(m_pMotdMenu)
	{
		m_pMotdMenu->Tick();
	}
	else
	{
		m_Dialog.Tick();
	}

	if(m_PlayerFlags & PLAYERFLAG_IN_MENU)
		m_VotesData.ApplyVoteUpdaterData();
}

void CPlayer::PostTick()
{
	// Check if the user is authenticated
	if(IsAuthed())
	{
		// update latency value
		if(Server()->ClientIngame(m_ClientID))
			GetTempData().m_TempPing = m_Latency.m_Min;

		// handlers
		HandleTuningParams();
		CVoteOptional::HandleVoteOptionals(m_ClientID);
		Account()->GetBonusManager().UpdateBonuses();
		Account()->GetPrisonManager().UpdatePrison();
		m_Effects.PostTick();
		m_Scenarios.PostTick();
	}

	// handlers
	HandleScoreboardColors();
}

void CPlayer::PrepareRespawnTick()
{
	m_aPlayerTick[Respawn] = Server()->Tick() + Server()->TickSpeed() / 2;
	m_WantSpawn = true;
}

CPlayerBot* CPlayer::GetEidolon() const
{
	if(!m_EidolonCID)
		return nullptr;

	return dynamic_cast<CPlayerBot*>(GS()->GetPlayer(m_EidolonCID.value()));
}

void CPlayer::TryCreateEidolon()
{
	if(IsBot() || !IsAuthed() || !GetCharacter() || m_EidolonCID.has_value())
		return;

	// check valid equppied item id
	const auto eidolonItemID = GetEquippedItemID(EquipEidolon);
	if(!eidolonItemID.has_value())
		return;

	// try to create eidolon
	if(const auto* pEidolonData = GS()->GetEidolonByItemID(eidolonItemID.value()))
	{
		if(int eidolonCID = GS()->CreateBot(TYPE_BOT_EIDOLON, pEidolonData->GetDataBotID(), m_ClientID); eidolonCID != -1)
		{
			if(auto* pEidolonPlayer = dynamic_cast<CPlayerBot*>(GS()->GetPlayer(eidolonCID)))
			{
				pEidolonPlayer->m_EidolonItemID = eidolonItemID.value();
				m_EidolonCID = eidolonCID;
			}
		}
	}
}

void CPlayer::TryRemoveEidolon()
{
	// try to remove eidolon
	if(m_EidolonCID)
	{
		GS()->DestroyPlayer(m_EidolonCID.value());
		m_EidolonCID.reset();
	}
}

void CPlayer::UpdateAchievement(AchievementType Type, int Criteria, int Progress, int ProgressType)
{
	GS()->Core()->AchievementManager()->UpdateAchievement(this, Type, Criteria, Progress, ProgressType);
}

void CPlayer::HandleScoreboardColors()
{
	if(m_TickActivatedGroupColour > Server()->Tick())
		return;

	bool ScoreboardActive = m_PlayerFlags & PLAYERFLAG_SCOREBOARD;
	if(ScoreboardActive != m_ActivatedGroupColour)
	{
		CMsgPacker Msg(NETMSGTYPE_SV_TEAMSSTATE);
		CMsgPacker MsgLegacy(NETMSGTYPE_SV_TEAMSSTATELEGACY);

		for(int i = 0; i < VANILLA_MAX_CLIENTS; ++i)
		{
			CPlayer* pPlayer = GS()->GetPlayer(i, true);
			int TeamColor = (ScoreboardActive && pPlayer && pPlayer->Account()->GetGroup()) ?
				pPlayer->Account()->GetGroup()->GetTeamColor() : 0;

			Msg.AddInt(TeamColor);
			MsgLegacy.AddInt(TeamColor);
		}

		Server()->SendMsg(&Msg, MSGFLAG_VITAL, m_ClientID);

		int ClientVersion = Server()->GetClientVersion(m_ClientID);
		if(VERSION_DDRACE < ClientVersion && ClientVersion < VERSION_DDNET_MSG_LEGACY)
			Server()->SendMsg(&MsgLegacy, MSGFLAG_VITAL, m_ClientID);

		m_ActivatedGroupColour = ScoreboardActive;
		m_TickActivatedGroupColour = Server()->Tick() + (Server()->TickSpeed() / 4);
	}
}

void CPlayer::HandleTuningParams()
{
	if(!(m_PrevTuningParams == m_NextTuningParams))
	{
		CMsgPacker Msg(NETMSGTYPE_SV_TUNEPARAMS);
		const int* pParams = reinterpret_cast<int*>(&m_NextTuningParams);
		for(unsigned i = 0; i < sizeof(m_NextTuningParams) / sizeof(int); i++)
		{
			Msg.AddInt(pParams[i]);
		}
		Server()->SendMsg(&Msg, MSGFLAG_VITAL, m_ClientID);
		m_PrevTuningParams = m_NextTuningParams;
	}

	m_NextTuningParams = *GS()->Tuning();
}

void CPlayer::Snap(int SnappingClient)
{
	// client info
	if(auto* pClientInfo = Server()->SnapNewItem<CNetObj_ClientInfo>(m_ClientID))
	{
		// prepare clan string
		if(m_aPlayerTick[RefreshClanTitle] < Server()->Tick())
		{
			const auto clanStringSize = str_utf8_fix_truncation(m_aRotateClanBuffer);
			std::rotate(m_aRotateClanBuffer, m_aRotateClanBuffer + str_utf8_forward(m_aRotateClanBuffer, 0), m_aRotateClanBuffer + clanStringSize);

			// set the next tick for refreshing the clan title
			m_aPlayerTick[RefreshClanTitle] = Server()->Tick() + (((m_aRotateClanBuffer[0] == '|') || (clanStringSize - 1 < 10)) ? Server()->TickSpeed() : (Server()->TickSpeed() / 8));

			// If the clan string size is less than 10
			if(clanStringSize < 10)
			{
				m_aPlayerTick[RefreshClanTitle] = Server()->Tick() + Server()->TickSpeed();
				RefreshClanString();
			}
		}

		char aNameBuf[MAX_NAME_LENGTH];
		GetFormatedName(aNameBuf, sizeof(aNameBuf));
		StrToInts(&pClientInfo->m_Name0, 4, aNameBuf);
		StrToInts(&pClientInfo->m_Clan0, 3, m_aRotateClanBuffer);
		pClientInfo->m_Country = Server()->ClientCountry(m_ClientID);
		StrToInts(&pClientInfo->m_Skin0, 6, GetTeeInfo().m_aSkinName);
		pClientInfo->m_UseCustomColor = GetTeeInfo().m_UseCustomColor;
		pClientInfo->m_ColorBody = GetTeeInfo().m_ColorBody;
		pClientInfo->m_ColorFeet = GetTeeInfo().m_ColorFeet;
	}

	// player info
	if(auto* pPlayerInfo = Server()->SnapNewItem<CNetObj_PlayerInfo>(m_ClientID))
	{
		const bool localClient = m_ClientID == SnappingClient;
		const bool isViewLocked = m_FixedView.GetCurrentView().has_value();

		pPlayerInfo->m_Local = localClient;
		pPlayerInfo->m_ClientId = m_ClientID;
		pPlayerInfo->m_Team = GetTeam();
		pPlayerInfo->m_Latency = (SnappingClient == -1 ? m_Latency.m_Min : GetTempData().m_TempPing);
		pPlayerInfo->m_Score = Account()->GetLevel();

		// ddnet player
		if(auto* pDDNetPlayer = Server()->SnapNewItem<CNetObj_DDNetPlayer>(m_ClientID))
		{
			pDDNetPlayer->m_AuthLevel = Server()->GetAuthedState(m_ClientID);
			pDDNetPlayer->m_Flags = isViewLocked ? EXPLAYERFLAG_SPEC : 0;
		}

		// spectator info
		if(localClient && (GetTeam() == TEAM_SPECTATORS || isViewLocked))
		{
			if(auto* pSpectatorInfo = Server()->SnapNewItem<CNetObj_SpectatorInfo>(m_ClientID))
			{
				pSpectatorInfo->m_X = m_ViewPos.x;
				pSpectatorInfo->m_Y = m_ViewPos.y;
				pSpectatorInfo->m_SpectatorId = (isViewLocked ? m_ClientID : -1);
				m_FixedView.Reset();
			}
		}
	}
}

void CPlayer::FakeSnap()
{
	constexpr int FakeID = VANILLA_MAX_CLIENTS - 1;

	// client info
	if(auto* pClientInfo = Server()->SnapNewItem<CNetObj_ClientInfo>(FakeID))
	{
		StrToInts(&pClientInfo->m_Name0, 4, " ");
		StrToInts(&pClientInfo->m_Clan0, 3, "");
		StrToInts(&pClientInfo->m_Skin0, 6, "default");
	}

	// player info
	if(auto* pPlayerInfo = Server()->SnapNewItem<CNetObj_PlayerInfo>(FakeID))
	{
		pPlayerInfo->m_Latency = m_Latency.m_Min;
		pPlayerInfo->m_Local = 1;
		pPlayerInfo->m_ClientId = FakeID;
		pPlayerInfo->m_Score = -9999;
		pPlayerInfo->m_Team = TEAM_SPECTATORS;
	}

	// spectator info
	if(auto* pSpectatorInfo = Server()->SnapNewItem<CNetObj_SpectatorInfo>(FakeID))
	{
		pSpectatorInfo->m_SpectatorId = -1;
		pSpectatorInfo->m_X = m_ViewPos.x;
		pSpectatorInfo->m_Y = m_ViewPos.y;
	}
}

void CPlayer::RefreshClanString()
{
	// is not authed send only clan
	if(!IsAuthed())
	{
		str_copy(m_aRotateClanBuffer, Server()->ClientClan(m_ClientID), sizeof(m_aRotateClanBuffer));
		return;
	}

	// location
	auto prepared = std::string(Server()->GetWorldName(GetCurrentWorldID()));

	// title
	if(const auto titleItemID = GetEquippedItemID(EquipTitle); titleItemID.has_value())
	{
		prepared += " | ";
		prepared += GetItem(titleItemID.value())->Info()->GetName();
	}

	// guild
	if(const auto* pGuild = Account()->GetGuild())
	{
		prepared += " | ";
		prepared += pGuild->GetName();
		prepared += " : ";
		prepared += Account()->GetGuildMember()->GetRank()->GetName();
	}

	// class
	char classBuffer[64];
	const char* professionName = GetProfessionName(Account()->GetClass().GetProfessionID());
	str_format(classBuffer, sizeof(classBuffer), "_%-*s_", 8 - str_length(professionName), professionName);
	prepared += " | ";
	prepared += classBuffer;

	// end format
	str_format(m_aRotateClanBuffer, sizeof(m_aRotateClanBuffer), "%s", prepared.c_str());
}

CCharacter* CPlayer::GetCharacter() const
{
	if(m_pCharacter && m_pCharacter->IsAlive())
		return m_pCharacter;
	return nullptr;
}

void CPlayer::TryRespawn()
{
	vec2 SpawnPos;
	int SpawnType = Account()->GetPrisonManager().IsInPrison() ? SPAWN_HUMAN_PRISON : SPAWN_HUMAN;

	// Check if the last killed by weapon is not WEAPON_WORLD
	if(!Account()->GetPrisonManager().IsInPrison() && GetTempData().m_LastKilledByWeapon != WEAPON_WORLD)
	{
		int RespawnWorldID = GS()->GetWorldData()->GetRespawnWorld()->GetID();
		if(RespawnWorldID >= 0 && !GS()->IsPlayerInWorld(m_ClientID, RespawnWorldID))
		{
			ChangeWorld(RespawnWorldID);
			return;
		}
		SpawnType = SPAWN_HUMAN_TREATMENT;
	}

	// Check if the controller allows spawning of the given spawn type at the specified position
	if(GS()->m_pController->CanSpawn(SpawnType, &SpawnPos))
	{
		vec2 TeleportPosition = GetTempData().GetTeleportPosition();
		bool CanSelfCordSpawn = !is_negative_vec(TeleportPosition) && !GS()->Collision()->CheckPoint(TeleportPosition);

		// Use self-coordinated spawning if possible
		if(!GS()->IsWorldType(WorldType::Dungeon) && CanSelfCordSpawn)
		{
			SpawnPos = TeleportPosition;
		}

		// Create and spawn a new character
		int AllocMemoryCell = MAX_CLIENTS * GS()->GetWorldID() + m_ClientID;
		m_pCharacter = new(AllocMemoryCell) CCharacter(&GS()->m_World);
		m_pCharacter->Spawn(this, SpawnPos);
		GS()->CreatePlayerSpawn(SpawnPos);
		GetTempData().ClearTeleportPosition();
		m_WantSpawn = false;

		GS()->SendMenuMotd(this, MOTD_MENU_WIKI_INFO);
	}
}

void CPlayer::KillCharacter(int Weapon)
{
	if(m_pCharacter)
	{
		m_pCharacter->Die(m_ClientID, Weapon);
		delete m_pCharacter;
		m_pCharacter = nullptr;
	}
}

void CPlayer::OnDisconnect()
{
	KillCharacter();
}

void CPlayer::OnDirectInput(CNetObj_PlayerInput* pNewInput)
{
	// Update view position for spectators
	if(!m_pCharacter && GetTeam() == TEAM_SPECTATORS)
	{
		m_ViewPos = vec2(pNewInput->m_TargetX, pNewInput->m_TargetY);
	}

	// parse event keys
	Server()->Input()->ParseInputClickedKeys(m_ClientID , pNewInput, m_pLastInput);
	if(m_pCharacter)
	{
		const int ActiveWeapon = m_pCharacter->m_Core.m_ActiveWeapon;
		Server()->Input()->ProcessCharacterInput(m_ClientID, ActiveWeapon, pNewInput, m_pLastInput);
	}

	// Reset input when chatting
	if(pNewInput->m_PlayerFlags & PLAYERFLAG_CHATTING)
	{
		if(m_PlayerFlags & PLAYERFLAG_CHATTING)
			return;

		if(m_pCharacter)
			m_pCharacter->ResetInput();

		m_PlayerFlags = pNewInput->m_PlayerFlags;
		return;
	}

	m_PlayerFlags = pNewInput->m_PlayerFlags;

	if(m_pCharacter)
	{
		// Update AFK status
		if(g_Config.m_SvMaxAfkTime != 0)
			m_Afk = m_LastPlaytime < time_get() - time_freq() * g_Config.m_SvMaxAfkTime;

		m_pCharacter->OnDirectInput(pNewInput);
	}

	// Check for activity
	if(mem_comp(pNewInput, m_pLastInput, sizeof(CNetObj_PlayerInput)))
	{
		mem_copy(m_pLastInput, pNewInput, sizeof(CNetObj_PlayerInput));
		if(m_LastInputInit)
			m_LastPlaytime = time_get();

		m_LastInputInit = true;
	}
}

void CPlayer::OnPredictedInput(CNetObj_PlayerInput* pNewInput) const
{
	// skip the input if chat is active
	if((m_PlayerFlags & PLAYERFLAG_CHATTING) && (pNewInput->m_PlayerFlags & PLAYERFLAG_CHATTING))
		return;

	if(m_pCharacter)
		m_pCharacter->OnPredictedInput(pNewInput);
}

int CPlayer::GetTeam()
{
	return IsAuthed() ? TEAM_RED : TEAM_SPECTATORS;
}

/* #########################################################################
	FUNCTIONS PLAYER HELPER
######################################################################### */
void CPlayer::ProgressBar(const char* pType, int Level, uint64_t Exp, uint64_t ExpNeeded, uint64_t GainedExp) const
{
	const auto ExpProgress = translate_to_percent(ExpNeeded, Exp);
	const auto GainedExpProgress = translate_to_percent(ExpNeeded, GainedExp);

	// send and format
	const auto ProgressBar = mystd::string::progressBar(100, static_cast<int>(ExpProgress), 10, ":", " ");
	const auto Result = fmt_default("Lv{lv} {type}[{bar}] {~.2f}%+{~.3f}%({})XP", Level, pType, ProgressBar, ExpProgress, GainedExpProgress, GainedExp);
	GS()->Broadcast(m_ClientID, BroadcastPriority::GameInformation, 100, Result.c_str());
}

/* #########################################################################
	FUNCTIONS PLAYER ACCOUNT
######################################################################### */
const char* CPlayer::GetLanguage() const
{
	return Server()->GetClientLanguage(m_ClientID);
}

void CPlayer::UpdateTempData(int Health, int Mana)
{
	auto& TempData = GetTempData();
	TempData.m_TempHealth = Health;
	TempData.m_TempMana = Mana;
}

bool CPlayer::IsAuthed() const
{
	const auto* pAccountManager = GS()->Core()->AccountManager();
	if(pAccountManager->IsActive(m_ClientID))
	{
		return Account()->GetID() > 0;
	}
	return false;
}

int CPlayer::GetMaxHealth() const
{
	auto DefaultHP = 10 + GetTotalAttributeValue(AttributeIdentifier::HP);
	DefaultHP += translate_to_percent_rest(DefaultHP, Account()->GetClass().GetExtraHP());
	Account()->GetBonusManager().ApplyBonuses(BONUS_TYPE_HP, &DefaultHP);
	return DefaultHP;
}

int CPlayer::GetMaxMana() const
{
	auto DefaultMP = 10 + GetTotalAttributeValue(AttributeIdentifier::MP);
	DefaultMP += translate_to_percent_rest(DefaultMP, Account()->GetClass().GetExtraMP());
	Account()->GetBonusManager().ApplyBonuses(BONUS_TYPE_MP, &DefaultMP);
	return DefaultMP;
}

int64_t CPlayer::GetAfkTime() const
{
	return m_Afk ? ((time_get() - m_LastPlaytime) / time_freq()) - g_Config.m_SvMaxAfkTime : 0;
}

void CPlayer::FormatBroadcastBasicStats(char* pBuffer, int Size, const char* pAppendStr) const
{
	if(!IsAuthed() || !m_pCharacter || m_PlayerFlags & PLAYERFLAG_IN_MENU)
		return;

	// information
	const int LevelPercent = round_to_int(translate_to_percent(computeExperience(Account()->GetLevel()), Account()->GetExperience()));
	const auto ProgressBar = mystd::string::progressBar(100, LevelPercent, 10, ":", " ");
	const auto MaxHP = GetMaxHealth();
	const auto MaxMP = GetMaxMana();
	const auto HP = m_pCharacter->Health();
	const auto MP = m_pCharacter->Mana();
	const auto Bank = Account()->GetBank();
	const auto Gold = Account()->GetGold();
	const auto GoldCapacity = Account()->GetGoldCapacity();
	const auto [BonusActivitiesLines, BonusActivitiesStr] = Account()->GetBonusManager().GetBonusActivitiesString();

	// result
	auto Result = fmt_localize(m_ClientID, "\n\n\n\n\nLv{}[{}]\nHP {}/{}\nMP {}/{}\nGold {} of {}\nBank {}",
		Account()->GetLevel(), ProgressBar, HP, MaxHP, MP, MaxMP, Gold, GoldCapacity, Bank);

	// recast heal info
	auto PotionRecastTime = m_aPlayerTick[HealPotionRecast] - Server()->Tick();
	if(PotionRecastTime > 0)
	{
		const auto Seconds = std::max(0, PotionRecastTime / Server()->TickSpeed());
		Result += "\n" + fmt_localize(m_ClientID, "Potion HP recast: {}", Seconds);
	}

	// recast mana info
	PotionRecastTime = m_aPlayerTick[ManaPotionRecast] - Server()->Tick();
	if(PotionRecastTime > 0)
	{
		const auto Seconds = std::max(0, PotionRecastTime / Server()->TickSpeed());
		Result += "\n" + fmt_localize(m_ClientID, "Potion MP recast: {}", Seconds);
	}

	if(!BonusActivitiesStr.empty())
	{
		Result += "\n" + BonusActivitiesLines;
	}

	constexpr int MaxLines = 20;
	const auto Lines = std::ranges::count(Result, '\n');
	if(Lines < MaxLines)
	{
		Result.append(MaxLines - Lines, '\n');
	}

	str_format(pBuffer, Size, "%s%-150s", Result.c_str(), pAppendStr);
}

/* #########################################################################
	FUNCTIONS PLAYER PARSING
######################################################################### */
bool CPlayer::ParseVoteOptionResult(int Vote)
{
	// check valid character
	if(!m_pCharacter)
	{
		GS()->Chat(m_ClientID, "Use it when you're not dead!");
		return true;
	}

	// execute is exist vote optional
	auto& voteOptions = CVoteOptional::Data()[m_ClientID];
	if(!voteOptions.empty())
	{
		CVoteOptional* pOptional = &voteOptions.front();
		pOptional->ExecuteVote(Vote == 1);
	}

	if(Vote == 1)
	{
		// dungeon change ready state
		if(GS()->IsWorldType(WorldType::Dungeon))
		{
			const int DungeonID = dynamic_cast<CGameControllerDungeon*>(GS()->m_pController)->GetDungeonID();
			if(!CDungeonData::ms_aDungeon[DungeonID].IsDungeonPlaying())
			{
				GetTempData().m_TempDungeonReady = !GetTempData().m_TempDungeonReady;
				GS()->Chat(m_ClientID, "You changed the ready mode to \"{}\"!", GetTempData().m_TempDungeonReady ? "ready" : "not ready");
			}
			return true;
		}
	}
	else
	{
		// continue dialog
		if(m_Dialog.IsActive())
		{
			if(m_aPlayerTick[LastDialog] && m_aPlayerTick[LastDialog] > GS()->Server()->Tick())
				return true;

			m_aPlayerTick[LastDialog] = GS()->Server()->Tick() + (GS()->Server()->TickSpeed() / 4);
			GS()->CreatePlayerSound(m_ClientID, SOUND_PICKUP_ARMOR);
			m_Dialog.Next();
			return true;
		}
	}
	return false;
}

CPlayerItem* CPlayer::GetItem(ItemIdentifier ID)
{
	const auto& itemsDescription = CItemDescription::Data();
	dbg_assert(itemsDescription.contains(ID), "invalid referring to the CPlayerItem");

	auto& playerItems = CPlayerItem::Data()[m_ClientID];
	if(!playerItems.contains(ID))
	{
		CPlayerItem(ID, m_ClientID).Init({}, {}, {}, {});
	}

	return &playerItems[ID];
}

CSkill* CPlayer::GetSkill(int SkillID) const
{
	const auto& skillsDescription = CSkillDescription::Data();
	dbg_assert(skillsDescription.contains(SkillID), "invalid referring to the CSkill");

	auto& playerSkills = CSkill::Data()[m_ClientID];
	const auto iter = std::ranges::find_if(playerSkills, [SkillID](const auto* pSkill)
	{
		return pSkill->GetID() == SkillID;
	});

	return (iter == playerSkills.end() ? CSkill::CreateElement(m_ClientID, SkillID) : *iter);
}

CPlayerQuest* CPlayer::GetQuest(QuestIdentifier ID) const
{
	const auto& questsDescription = CQuestDescription::Data();
	dbg_assert(questsDescription.contains(ID), "invalid referring to the CPlayerQuest");

	auto& questData = CPlayerQuest::Data()[m_ClientID];
	if(!questData.contains(ID))
	{
		CPlayerQuest::CreateElement(ID, m_ClientID);
	}

	return questData[ID];
}

std::optional<int> CPlayer::GetEquippedItemID(ItemFunctional EquipID, int SkipItemID) const
{
	const auto& playerItems = CPlayerItem::Data()[m_ClientID];
	for(const auto& [itemID, item] : playerItems)
	{
		if(itemID == SkipItemID)
			continue;

		if(!item.HasItem())
			continue;

		if(!item.IsEquipped())
			continue;

		if(!item.Info()->IsFunctional(EquipID))
			continue;

		return itemID;
	}

	return std::nullopt;
}

bool CPlayer::IsEquipped(ItemFunctional EquipID) const
{
	const auto& optItemID = GetEquippedItemID(EquipID, -1);
	return optItemID.has_value();
}

int CPlayer::GetTotalAttributeValue(AttributeIdentifier ID) const
{
	// initialize variables
	const auto* pAtt = GS()->GetAttributeInfo(ID);

	// check if the player is in a dungeon and the attribute has a low improvement cost
	if(GS()->IsWorldType(WorldType::Dungeon))
	{
		const auto* pDungeon = dynamic_cast<const CGameControllerDungeon*>(GS()->m_pController);
		if(pAtt->GetUpgradePrice() < 4 && CDungeonData::ms_aDungeon[pDungeon->GetDungeonID()].IsDungeonPlaying())
		{
			return pDungeon->GetAttributeDungeonSyncByClass(Account()->GetClass().GetProfessionID(), ID);
		}
	}

	// counting attributes from equipped items
	int totalValue = 0;
	for(const auto& ItemData : CPlayerItem::Data()[m_ClientID])
	{
		// required repair
		if(ItemData.second.GetDurability() <= 0)
			continue;

		// if is equipped and enchantable add attribute
		if(ItemData.second.IsEquipped() && ItemData.second.Info()->IsEnchantable() && ItemData.second.Info()->GetInfoEnchantStats(ID))
		{
			totalValue += ItemData.second.GetEnchantStats(ID);
		}
	}

	// add attribute value from player's improvements
	for(const auto& Profession : Account()->GetProfessions())
	{
		totalValue += Profession.GetAttributeValue(ID);
	}

	return totalValue;
}

float CPlayer::GetAttributeChance(AttributeIdentifier ID) const
{
	// use a lambda to calculate chance
	int attributeValue = GetTotalAttributeValue(ID);
	auto calculateChance = [attributeValue](float base, float multiplier, float max)
	{
		return std::min(base + static_cast<float>(attributeValue) * multiplier, max);
	};

	// chance
	switch(ID)
	{
		case AttributeIdentifier::Vampirism:
		case AttributeIdentifier::Crit:
			return calculateChance(8.0f, 0.0015f, 30.0f);

		case AttributeIdentifier::Lucky:
			return calculateChance(5.0f, 0.0015f, 20.0f);

		default:
			return 0.f;
	}
}

int CPlayer::GetTotalAttributesInGroup(AttributeGroup Type) const
{
	int totalSize = 0;

	// iterate over all attributes by group and sum their values
	for(const auto& [ID, pAttribute] : CAttributeDescription::Data())
	{
		if(pAttribute->IsGroup(Type))
		{
			totalSize += GetTotalAttributeValue(ID);
		}
	}
	return totalSize;
}

int CPlayer::GetTotalAttributes() const
{
	int totalSize = 0;

	// iterate over all attributes and sum their values
	for(const auto& attributeID : CAttributeDescription::Data() | std::views::keys)
	{
		totalSize += GetTotalAttributeValue(attributeID);
	}

	return totalSize;
}

void CPlayer::SetSnapHealthTick(int Sec)
{
	m_SnapHealthNicknameTick = Server()->Tick() + (Server()->TickSpeed() * Sec);
}

void CPlayer::ChangeWorld(int WorldID, std::optional<vec2> newWorldPosition) const
{
	// reset dungeon temporary data
	auto& tempData = GetTempData();
	tempData.m_TempDungeonReady = false;
	tempData.m_TempTimeDungeon = 0;

	// if new position is provided, set the teleport position
	if(newWorldPosition.has_value())
	{
		tempData.SetTeleportPosition(newWorldPosition.value());
	}

	// change the player's world
	Account()->m_aHistoryWorld.push_front(WorldID);
	Server()->ChangeWorld(m_ClientID, WorldID);
}

int CPlayer::GetCurrentWorldID() const
{
	return Server()->GetClientWorldID(m_ClientID);
}

CTeeInfo& CPlayer::GetTeeInfo() const
{
	return Account()->m_TeeInfos;
}