/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "guild_door.h"

#include <engine/shared/config.h>
#include <game/server/gamecontext.h>

#include "game/server/core/components/guilds/guild_data.h"
#include "game/server/core/components/guilds/guild_house_data.h"

CEntityGuildDoor::CEntityGuildDoor(CGameWorld* pGameWorld, CGuildHouse* pHouse, std::string&& Name, vec2 Pos)
	: CEntity(pGameWorld, CGameWorld::ENTTYPE_PLAYER_HOUSE_DOOR, Pos), m_Name(std::move(Name)), m_pHouse(pHouse)
{
	GS()->Collision()->Wallline(32, vec2(0, -1), &m_Pos, &m_PosTo, false);
	m_PosControll = Pos;
	m_State = CLOSED;
	GS()->CreateLaserOrbite(this, 4, EntLaserOrbiteType::DEFAULT, 0.f, 16.f, LASERTYPE_DOOR);
	GameWorld()->InsertEntity(this);
}

void CEntityGuildDoor::Tick()
{
	for(CCharacter* pChar = (CCharacter*)GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER); pChar; pChar = (CCharacter*)pChar->TypeNext())
	{
		// Check if the distance between the position of the control and the mouse position of the character is less than 24.0f
		if(distance(m_PosControll, pChar->GetMousePos()) < 24.0f)
		{
			const int& ClientID = pChar->GetPlayer()->GetCID();
			CGuild* pCharGuild = pChar->GetPlayer()->Account()->GetGuild();
			if(pCharGuild && m_pHouse->GetGuild() && pCharGuild->GetID() == m_pHouse->GetGuild()->GetID() 
				&& pChar->GetPlayer()->Account()->GetGuildMember()->CheckAccess(GUILD_RIGHT_UPGRADES_HOUSE))
			{
				if(pChar->GetPlayer()->IsClickedKey(KEY_EVENT_FIRE_HAMMER))
				{
					// Check the state of the door
					if(m_State == OPENED)
						Close();
					else
						Open();
				}

				// Broadcast a game information message to the client
				GS()->Broadcast(ClientID, BroadcastPriority::GAME_INFORMATION, 10, "Use hammer 'fire.' To operate the door '{}'!", m_Name);
			}
			else
			{
				// Broadcast a game information message to the client
				GS()->Broadcast(ClientID, BroadcastPriority::GAME_INFORMATION, 10, "You do not have access to '{}' door!", m_Name);
			}
		}

		// Check if the door is closed
		if(m_State == CLOSED)
		{
			// Find the closest point on the line segment defined by m_Pos and m_PosTo to pChar's position
			vec2 IntersectPos;
			if(closest_point_on_line(m_Pos, m_PosTo, pChar->m_Core.m_Pos, IntersectPos))
			{
				// Check if the distance is within the door hit radius
				const float Distance = distance(IntersectPos, pChar->m_Core.m_Pos);
				if(Distance <= g_Config.m_SvDoorRadiusHit)
				{
					// Check if both the door's guild and pChar's guild exist and have the same ID
					CGuild* pCharGuild = pChar->GetPlayer()->Account()->GetGuild();
					if(pCharGuild && m_pHouse->GetGuild() && pCharGuild->GetID() == m_pHouse->GetGuild()->GetID())
					{
						continue;
					}

					// Set pChar's DoorHit flag to true
					pChar->m_DoorHit = true;
				}
			}
		}
	}
}

void CEntityGuildDoor::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient, true) || m_State == OPENED)
		return;

	if(GS()->GetClientVersion(SnappingClient) >= VERSION_DDNET_MULTI_LASER)
	{
		CNetObj_DDNetLaser* pObj = static_cast<CNetObj_DDNetLaser*>(Server()->SnapNewItem(NETOBJTYPE_DDNETLASER, GetID(), sizeof(CNetObj_DDNetLaser)));
		if(!pObj)
			return;

		pObj->m_ToX = int(m_Pos.x);
		pObj->m_ToY = int(m_Pos.y);
		pObj->m_FromX = int(m_PosTo.x);
		pObj->m_FromY = int(m_PosTo.y);
		pObj->m_StartTick = Server()->Tick() - 2;
		pObj->m_Owner = -1;
		pObj->m_Type = LASERTYPE_DOOR;
	}
	else
	{
		CNetObj_Laser* pObj = static_cast<CNetObj_Laser*>(Server()->SnapNewItem(NETOBJTYPE_LASER, GetID(), sizeof(CNetObj_Laser)));
		if(!pObj)
			return;

		pObj->m_X = int(m_Pos.x);
		pObj->m_Y = int(m_Pos.y);
		pObj->m_FromX = int(m_PosTo.x);
		pObj->m_FromY = int(m_PosTo.y);
		pObj->m_StartTick = Server()->Tick() - 2;
	}
}