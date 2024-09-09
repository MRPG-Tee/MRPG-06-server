/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMEWORLD_H
#define GAME_SERVER_GAMEWORLD_H

class CGS;
class CEntity;
class CEntityGroup;
class CCharacter;

struct FixedViewCam
{
	vec2 LockedAt {};
	void ViewLock(const vec2& position) { _Locked = true; LockedAt = position; }
	void ViewUnlock() { if(!_Locked) return; _Locked = false; }
	bool IsLocked() const { return _Locked; }

private:
	bool _Locked {};
};

/*
	Class: Game World
		Tracks all entities in the game. Propagates tick and
		snap calls to all entities.
*/
class CGameWorld
{
public:
	enum
	{
		ENTTYPE_PROJECTILE = 0,
		ENTTYPE_LASER,
		ENTTYPE_PICKUP,
		ENTTYPE_CHARACTER,
		ENTTYPE_RANDOM_BOX,
		ENTTYPE_WORLD_TEXT,
		ENTTYPE_BONUS_DROP,
		ENTTYPE_ITEM_DROP,
		ENTTYPE_QUEST_DROP,
		ENTTYPE_HERVESTING_ITEM,
		ENTTYPE_MULTIPLE_ORBITE,
		ENTTYPE_EYES,
		ENTTYPE_EYES_WALL,
		ENTTYPE_OTHER,

		// eidolon
		ENTTYPE_EIDOLON_ITEM,

		// quest
		ENTTYPE_MOVE_TO_POINT,
		ENTTYPE_PATH_NAVIGATOR,
		ENTTYPE_PATH_FINDER,

		// door's
		ENTTYPE_DUNGEON_DOOR,
		ENTTYPE_DUNGEON_PROGRESS_DOOR,
		ENTTYPE_GUILD_HOUSE_DOOR,
		ENTTYPE_PLAYER_HOUSE_DOOR,
		ENTTYPE_BOT_WALL,

		// skill's
		ENTYPE_HEALTH,
		ENTYPE_HEALTH_TURRET,
		ENTYPE_SLEEPY_GRAVITY,
		ENTYPE_ATTACK_TELEPORT,
		ENTTYPE_DRAW_BOARD,

		ENTYPE_LASER_ORBITE, // always end
		NUM_ENTTYPES
	};

private:
	CEntity *m_pNextTraverseEntity;
	CEntity *m_apFirstEntityTypes[NUM_ENTTYPES];
	ska::unordered_set<int> m_aMarkedBotsActive;
	ska::unordered_map<int, bool> m_aBotsActive;
	ska::flat_hash_set<CEntity*> m_apEntitiesCollection;

	CGS *m_pGS;
	IServer *m_pServer;

public:
	CGS *GS() const { return m_pGS; }
	IServer *Server() const { return m_pServer; }

	ska::unordered_set<std::shared_ptr<CEntityGroup>> m_EntityGroups;
	bool m_ResetRequested;
	bool m_Paused;
	CWorldCore m_Core;

	CGameWorld();
	~CGameWorld();

	bool ExistEntity(CEntity* pEnt) const;
	bool IsBotActive(int ClientID) { return m_aBotsActive[ClientID]; }

	void SetGameServer(CGS *pGS);
	void UpdatePlayerMaps();

	CEntity *FindFirst(int Type);
	int FindEntities(vec2 Pos, float Radius, CEntity **ppEnts, int Max, int Type);
	std::vector<CEntity*> FindEntities(vec2 Pos, float Radius, int Max, int Type);
	CEntity *ClosestEntity(vec2 Pos, float Radius, int Type, CEntity *pNotThis) const;
	CCharacter *IntersectCharacter(vec2 Pos0, vec2 Pos1, float Radius, vec2 &NewPos, CEntity *pNotThis = nullptr);
	bool IntersectClosestEntity(vec2 Pos, float Radius, int EnttypeID);
	bool IntersectClosestDoorEntity(vec2 Pos, float Radius);

	void InsertEntity(CEntity *pEntity);
	void RemoveEntity(CEntity *pEntity);
	void DestroyEntity(CEntity *pEntity);
	void Snap(int SnappingClient);
	void PostSnap();
	void Tick();

private:
	void RemoveEntities();
};

#endif
