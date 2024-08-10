/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_CHARACTER_H
#define GAME_SERVER_ENTITIES_CHARACTER_H
#include <game/server/entity.h>

#include "../core/tiles_handler.h"

class CMultipleOrbite;

class CCharacter : public CEntity
{
	MACRO_ALLOC_POOL_ID()

	class CPlayer *m_pPlayer;
	CTileHandler *m_pTilesHandler;

	int m_LastWeapon;
	int m_QueuedWeapon;
	int m_BleedingByClientID;

	// ninja
	struct
	{
		vec2 m_ActivationDir;
		int m_CurrentMoveTime;
		int m_OldVelAmount;
	} m_Ninja;

	// info for dead reckoning
	CCharacterCore m_SendCore; // core that we should send
	CCharacterCore m_ReckoningCore; // the dead reckoning core

	void HandleReload();
	void FireWeapon();
	void HandleWeaponSwitch();
	void DoWeaponSwitch();
	void HandleHookActions();
	bool InteractiveHammer(vec2 Direction, vec2 ProjStartPos);
	void HandleBuff(CTuningParams* TuningParams);
	void HandlePlayer();

	// return true if the world is closed
	bool CheckAllowedWorld() const;

protected:
	bool m_Alive;
	int m_Health;
	int m_Mana;
	int m_ReckoningTick; // tick that we are performing dead reckoning From

	int m_LastNoAmmoSound;
	int m_NumInputs;
	int m_TriggeredEvents;
	int m_LastAction;
	int m_ReloadTimer;

	int m_AttackTick;
	int m_EmoteType;
	int m_EmoteStop;
	vec2 m_SpawnPoint;
	CMultipleOrbite* m_pMultipleOrbite;

	// these are non-heldback inputs
	CNetObj_PlayerInput m_Input;
	CNetObj_PlayerInput m_LatestPrevInput;
	CNetObj_PlayerInput m_LatestInput;

	void HandleWeapons();
	void HandleNinja();
	void HandleTilesets();
	void HandleIndependentTuning();

	void SetSafe(int FlagsDisallow = CHARACTERFLAG_HAMMER_HIT_DISABLED | CHARACTERFLAG_COLLISION_DISABLED | CHARACTERFLAG_HOOK_HIT_DISABLED);
	void ResetSafe();

	bool StartConversation(CPlayer* pTarget) const;
	void HandleEventsDeath(int Killer, vec2 Force) const;

	void AutoUseHealingPotionIfNeeded() const;

public:
	// the player core for the physics
	CCharacterCore m_Core;

	// allow perm
	bool m_DamageDisabled;
	int m_AmmoRegen;
	bool m_SafeAreaForTick;
	vec2 m_OldPos;
	vec2 m_OlderPos;
	bool m_DoorHit;

	//character's size
	static constexpr int ms_PhysSize = 28;
	CCharacter(CGameWorld *pWorld);
	~CCharacter() override;

	CPlayer *GetPlayer() const { return m_pPlayer; }
	CTileHandler *GetTiles() const { return m_pTilesHandler; }

	void Tick() override;
	void TickDeferred() override;
	void Snap(int SnappingClient) override;
	void PostSnap() override;

	virtual bool Spawn(class CPlayer* pPlayer, vec2 Pos);
	virtual void GiveRandomEffects(int To);
	virtual bool TakeDamage(vec2 Force, int Dmg, int FromCID, int Weapon);
	virtual void Die(int Killer, int Weapon);
	virtual void HandleTuning();

	void OnPredictedInput(CNetObj_PlayerInput *pNewInput);
	void OnDirectInput(CNetObj_PlayerInput *pNewInput);
	void ResetHook();
	void ResetInput();
	bool IsGrounded() const;
	bool IsCollisionFlag(int Flag) const;
	CPlayer* GetHookedPlayer() const;

	bool IsAllowedPVP(int FromID) const;
	bool IsAlive() const { return m_Alive; }
	void SetEmote(int Emote, int Sec, bool StartEmoticion);
	void SetWeapon(int Weapon);
	bool IncreaseHealth(int Amount);
	bool IncreaseMana(int Amount);
	bool CheckFailMana(int Mana);
	int Mana() const { return m_Mana; }
	int Health() const { return m_Health; }

	void AddMultipleOrbite(int Amount, int Type, int Subtype);
	virtual bool GiveWeapon(int Weapon, int Ammo);
	bool RemoveWeapon(int Weapon);

	void ChangePosition(vec2 NewPos);
	void UpdateEquipingStats(int ItemID);
	void ResetDoorPos();

	vec2 GetMousePos() const { return m_Core.m_Pos + vec2(m_Core.m_Input.m_TargetX, m_Core.m_Input.m_TargetY); }
};

#endif
