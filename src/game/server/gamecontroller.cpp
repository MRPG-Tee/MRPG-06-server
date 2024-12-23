/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/mapitems.h>
#include "gamecontroller.h"

#include "gamecontext.h"

#include "entities/pickup.h"
#include "core/entities/logic/botwall.h"
#include "core/components/achievements/achievement_data.h"

#include "entities/character_bot.h"

/*
	Here you need to put it in order make more events
	For modes that each map can have one of them
*/

IGameController::IGameController(CGS* pGS)
{
	m_pGS = pGS;
	m_GameFlags = 0;
	m_pServer = m_pGS->Server();

	for(int i = 0; i < NUM_SPAWN; i++)
		m_aNumSpawnPoints[i] = 0;
}

void IGameController::OnCharacterDamage(CPlayer* pFrom, CPlayer* pTo, int Damage)
{
	// achievement total damage
	if(pFrom != pTo && !pFrom->IsBot())
	{
		pFrom->UpdateAchievement(AchievementType::TotalDamage, NOPE, Damage, PROGRESS_ACCUMULATE);
	}
}

void IGameController::OnCharacterDeath(CPlayer* pVictim, CPlayer* pKiller, int Weapon)
{
	GS()->EventListener()->Notify<IEventListener::Type::PlayerDeath>( pVictim, pKiller, Weapon);

	// achievement death
	if(!pVictim->IsBot())
	{
		pVictim->UpdateAchievement(AchievementType::Death, NOPE, 1, PROGRESS_ACCUMULATE);
	}

	if(pVictim != pKiller)
	{
		// achievement defeat mob & pve
		if(pVictim->IsBot() && !pKiller->IsBot())
		{
			const auto VictimBotID = dynamic_cast<CPlayerBot*>(pVictim)->GetBotID();
			pKiller->UpdateAchievement(AchievementType::DefeatMob, VictimBotID, 1, PROGRESS_ACCUMULATE);
			pKiller->UpdateAchievement(AchievementType::DefeatPVE, NOPE, 1, PROGRESS_ACCUMULATE);
		}

		// achievement defeat pvp
		if(!pVictim->IsBot() && !pKiller->IsBot())
		{
			pKiller->UpdateAchievement(AchievementType::DefeatPVP, NOPE, 1, PROGRESS_ACCUMULATE);
		}
	}

	// update last killed by weapon
	if(!pVictim->IsBot())
	{
		pVictim->TryRemoveEidolon();
		pVictim->GetTempData().m_LastKilledByWeapon = Weapon;

		// Clear all effects on the player
		if(Weapon != WEAPON_WORLD)
		{
			pVictim->m_Effects.RemoveAll();
			pVictim->UpdateTempData(0, 0);
		}
	}
}

bool IGameController::OnCharacterSpawn(CCharacter* pChr)
{
	GS()->EventListener()->Notify<IEventListener::Type::PlayerSpawn>(pChr->GetPlayer());

	// Health
	int StartHealth = pChr->GetPlayer()->GetMaxHealth();
	if(!GS()->IsWorldType(WorldType::Dungeon))
	{
		if(pChr->GetPlayer()->GetHealth() > 0)
			StartHealth = pChr->GetPlayer()->GetHealth();
		else
			StartHealth /= 2;
	}
	pChr->IncreaseHealth(StartHealth);

	// Mana
	if(pChr->GetPlayer()->GetMana() > 0)
	{
		const int StartMana = pChr->GetPlayer()->GetMana();
		pChr->IncreaseMana(StartMana);
	}

	// Weapons
	const int MaximumAmmo = 10 + pChr->GetPlayer()->GetTotalAttributeValue(AttributeIdentifier::Ammo);
	pChr->GiveWeapon(WEAPON_HAMMER, -1);
	for(int i = WEAPON_GUN; i < NUM_WEAPONS - 1; i++)
		pChr->GiveWeapon(i, MaximumAmmo);

	// eidolons
	pChr->GetPlayer()->TryCreateEidolon();
	return true;
}

bool IGameController::OnCharacterBotSpawn(CCharacterBotAI* pChr)
{
	auto* pPlayerBot = dynamic_cast<CPlayerBot*>(pChr->GetPlayer());

	const int MaxStartHP = pPlayerBot->GetTotalAttributeValue(AttributeIdentifier::HP);
	const int MaxStartMP = pPlayerBot->GetTotalAttributeValue(AttributeIdentifier::MP);
	pPlayerBot->InitBasicStats(MaxStartHP, MaxStartMP, MaxStartHP, MaxStartMP);

	pChr->IncreaseHealth(MaxStartHP);
	pChr->IncreaseMana(MaxStartMP);
	pChr->GiveWeapon(WEAPON_HAMMER, -1);

	if(pPlayerBot->IsEquipped(EquipGun))
		pChr->GiveWeapon(WEAPON_GUN, 10);
	if(pPlayerBot->IsEquipped(EquipShotgun))
		pChr->GiveWeapon(WEAPON_SHOTGUN, 10);
	if(pPlayerBot->IsEquipped(EquipGrenade))
		pChr->GiveWeapon(WEAPON_GRENADE, 10);
	if(pPlayerBot->IsEquipped(EquipLaser))
		pChr->GiveWeapon(WEAPON_LASER, 10);

	return true;
}

void IGameController::OnEntity(int Index, vec2 Pos, int Flags)
{
	int Type = -1;
	int SubType = 0;
	switch(Index)
	{
		case ENTITY_SPAWN:
		m_aaSpawnPoints[SPAWN_HUMAN][m_aNumSpawnPoints[SPAWN_HUMAN]++] = Pos;
		break;
		case ENTITY_SPAWN_MOBS:
		m_aaSpawnPoints[SPAWN_BOT][m_aNumSpawnPoints[SPAWN_BOT]++] = Pos;
		break;
		case ENTITY_SPAWN_SAFE:
		m_aaSpawnPoints[SPAWN_HUMAN_TREATMENT][m_aNumSpawnPoints[SPAWN_HUMAN_TREATMENT]++] = Pos;
		break;
		case ENTITY_SPAWN_PRISON:
		m_aaSpawnPoints[SPAWN_HUMAN_PRISON][m_aNumSpawnPoints[SPAWN_HUMAN_PRISON]++] = Pos;
		break;
		case ENTITY_ARMOR_1:
		Type = POWERUP_ARMOR;
		break;
		case ENTITY_HEALTH_1:
		Type = POWERUP_HEALTH;
		break;
		case ENTITY_PICKUP_SHOTGUN:
		Type = POWERUP_WEAPON;
		SubType = WEAPON_SHOTGUN;
		break;
		case ENTITY_PICKUP_GRENADE:
		Type = POWERUP_WEAPON;
		SubType = WEAPON_GRENADE;
		break;
		case ENTITY_PICKUP_LASER:
		Type = POWERUP_WEAPON;
		SubType = WEAPON_LASER;
		break;
		default: break;
	}

	if(Type != -1)
	{
		new CPickup(&GS()->m_World, Type, SubType, Pos);
	}

	if(Index == ENTITY_NPC_WALL)
	{
		vec2 Direction = GS()->Collision()->GetRotateDirByFlags(Flags);
		new CBotWall(&GS()->m_World, Pos, Direction, CBotWall::Flags::WALLLINEFLAG_FRIENDLY_BOT);
	}

	if(Index == ENTITY_MOB_WALL)
	{
		vec2 Direction = GS()->Collision()->GetRotateDirByFlags(Flags);
		new CBotWall(&GS()->m_World, Pos, Direction, CBotWall::Flags::WALLLINEFLAG_AGRESSED_BOT);
	}
}

void IGameController::OnPlayerConnect(CPlayer* pPlayer)
{
	const int ClientID = pPlayer->GetCID();
	if(Server()->ClientIngame(ClientID) && pPlayer->GetCurrentWorldID() == GS()->GetWorldID())
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "team_join player='%d:%s' team=%d", ClientID, Server()->ClientName(ClientID), pPlayer->GetTeam());
		GS()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
		UpdateGameInfo(ClientID);
	}
}

void IGameController::OnPlayerDisconnect(CPlayer* pPlayer)
{
	const int ClientID = pPlayer->GetCID();
	if(Server()->ClientIngame(ClientID) && pPlayer->GetCurrentWorldID() == GS()->GetWorldID())
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "leave player='%d:%s'", ClientID, Server()->ClientName(ClientID));
		GS()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);
		GS()->Core()->SaveAccount(pPlayer, SAVE_POSITION);
	}

	pPlayer->OnDisconnect();
}

void IGameController::OnPlayerInfoChange(CPlayer* pPlayer, int WorldID) {}

void IGameController::OnReset()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(CPlayer* pPlayer = GS()->GetPlayer(i))
			pPlayer->m_aPlayerTick[Respawn] = Server()->Tick() + Server()->TickSpeed() / 2;
	}
}

// general
void IGameController::Snap()
{
	// vanilla snap
	CNetObj_GameInfo* pGameInfoObj = (CNetObj_GameInfo*)Server()->SnapNewItem(NETOBJTYPE_GAMEINFO, 0, sizeof(CNetObj_GameInfo));
	if(!pGameInfoObj)
		return;

	pGameInfoObj->m_GameFlags = m_GameFlags;
	pGameInfoObj->m_GameStateFlags = 0;
	pGameInfoObj->m_RoundStartTick = Server()->GetOffsetGameTime();
	pGameInfoObj->m_WarmupTimer = 0;
	pGameInfoObj->m_RoundNum = 0;
	pGameInfoObj->m_RoundCurrent = 1;

	// ddnet snap
	CNetObj_GameInfoEx* pGameInfoEx = (CNetObj_GameInfoEx*)Server()->SnapNewItem(NETOBJTYPE_GAMEINFOEX, 0, sizeof(CNetObj_GameInfoEx));
	if(!pGameInfoEx)
		return;

	pGameInfoEx->m_Flags = GAMEINFOFLAG_GAMETYPE_PLUS | GAMEINFOFLAG_ALLOW_EYE_WHEEL | GAMEINFOFLAG_ALLOW_HOOK_COLL | GAMEINFOFLAG_ALLOW_ZOOM | GAMEINFOFLAG_PREDICT_VANILLA;
	pGameInfoEx->m_Flags2 = GAMEINFOFLAG2_GAMETYPE_CITY | GAMEINFOFLAG2_ALLOW_X_SKINS | GAMEINFOFLAG2_HUD_DDRACE | GAMEINFOFLAG2_HUD_HEALTH_ARMOR | GAMEINFOFLAG2_HUD_AMMO;
	pGameInfoEx->m_Version = GAMEINFO_CURVERSION;
}

void IGameController::Tick() { }

void IGameController::UpdateGameInfo(int ClientID)
{
	/*	CNetMsg_Sv_GameInfo GameInfoMsg;
		GameInfoMsg.m_GameFlags = m_GameFlags;
		GameInfoMsg.m_ScoreLimit = 0;
		GameInfoMsg.m_TimeLimit = 0;
		GameInfoMsg.m_MatchNum = 0;
		GameInfoMsg.m_MatchCurrent = 0;

		if(ClientID == -1)
		{
			for(int i = 0; i < MAX_PLAYERS; ++i)
			{
				if(!GS()->m_apPlayers[i] || !Server()->ClientIngame(i))
					continue;

				if((!GS()->IsMmoClient(i) && Server()->GetClientProtocolVersion(i) < MIN_RACE_CLIENTVERSION))
					GameInfoMsg.m_GameFlags &= ~GAMEFLAG_RACE;

				Server()->SendPackMsg(&GameInfoMsg, MSGFLAG_VITAL|MSGFLAG_NORECORD, i, Server()->GetClientWorldID(ClientID));
			}
		}
		else
		{
			if((!GS()->IsMmoClient(ClientID) && Server()->GetClientProtocolVersion(ClientID) < MIN_RACE_CLIENTVERSION))
				GameInfoMsg.m_GameFlags &= ~GAMEFLAG_RACE;

			Server()->SendPackMsg(&GameInfoMsg, MSGFLAG_VITAL|MSGFLAG_NORECORD, ClientID, Server()->GetClientWorldID(ClientID));
		}*/
}

bool IGameController::CanSpawn(int SpawnType, vec2* pOutPos, std::pair<vec2, float> LimiterSpread) const
{
	if(SpawnType < SPAWN_HUMAN || SpawnType >= NUM_SPAWN)
		return false;

	CSpawnEval Eval;
	EvaluateSpawnType(&Eval, SpawnType, LimiterSpread);

	*pOutPos = Eval.m_Pos;
	return Eval.m_Got;
}

float IGameController::EvaluateSpawnPos(CSpawnEval* pEval, vec2 Pos) const
{
	float Score = 0.0f;
	for(const CCharacter* pC = dynamic_cast<CCharacter*>(GS()->m_World.FindFirst(CGameWorld::ENTTYPE_CHARACTER)); pC; pC = (CCharacter*)pC->TypeNext())
	{
		// team mates are not as dangerous as enemies
		float Scoremod = 1.0f;
		if(pEval->m_FriendlyTeam != -1 && pC->GetPlayer()->GetTeam() == pEval->m_FriendlyTeam)
			Scoremod = 0.5f;

		const float d = distance(Pos, pC->GetPos());
		Score += Scoremod * (d == 0.f ? 1000000000.0f : 1.0f / d);
	}

	return Score;
}

void IGameController::EvaluateSpawnType(CSpawnEval* pEval, int SpawnType, std::pair<vec2, float> LimiterSpread) const
{
	// get spawn point
	for(int i = 0; i < m_aNumSpawnPoints[SpawnType]; i++)
	{
		// check if the position is occupado
		CCharacter* aEnts[MAX_CLIENTS];
		int Num = GS()->m_World.FindEntities(m_aaSpawnPoints[SpawnType][i], 64, (CEntity**)aEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
		vec2 Positions[5] = { vec2(0.0f, 0.0f), vec2(-32.0f, 0.0f), vec2(0.0f, -32.0f), vec2(32.0f, 0.0f), vec2(0.0f, 32.0f) };
		int Result = -1;

		if(LimiterSpread.second >= 1.f && distance(LimiterSpread.first, m_aaSpawnPoints[SpawnType][i]) > LimiterSpread.second)
			continue;

		for(int Index = 0; Index < 5 && Result == -1; ++Index)
		{
			Result = Index;
			for(int c = 0; c < Num; ++c)
			{
				if(
					GS()->Collision()->CheckPoint(m_aaSpawnPoints[SpawnType][i] + Positions[Index]) ||
					distance(aEnts[c]->GetPos(), m_aaSpawnPoints[SpawnType][i] + Positions[Index]) <= aEnts[c]->GetRadius())
				{
					Result = -1;
					break;
				}
			}
		}
		if(Result == -1)
			continue; // try next spawn point

		const vec2 P = m_aaSpawnPoints[SpawnType][i] + Positions[Result];
		const float S = EvaluateSpawnPos(pEval, P);
		if(!pEval->m_Got || pEval->m_Score > S)
		{
			pEval->m_Got = true;
			pEval->m_Score = S;
			pEval->m_Pos = P;
		}
	}
}

void IGameController::DoTeamChange(CPlayer* pPlayer)
{
	const int ClientID = pPlayer->GetCID();
	const int Team = pPlayer->GetTeam();

	pPlayer->GetTempData().m_LastKilledByWeapon = WEAPON_WORLD;

	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "team_join player='%d:%s' m_Team=%d", ClientID, Server()->ClientName(ClientID), Team);
	GS()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
	OnPlayerInfoChange(pPlayer, GS()->GetWorldID());
	Server()->ExpireServerInfo();
}
