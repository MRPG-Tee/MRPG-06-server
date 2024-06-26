/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_PLAYER_H
#define GAME_SERVER_PLAYER_H

#include "core/components/Accounts/AccountData.h"
#include "core/components/Inventory/ItemData.h"
#include "core/components/Quests/quest_data.h"
#include "core/components/skills/skill_data.h"

#include "entities/character.h"
#include "core/tools/cooldown.h"
#include "core/tools/vote_wrapper.h"
#include "class_data.h"

enum
{
	WEAPON_SELF = -2, // self die
	WEAPON_WORLD = -1, // swap world etc
};

enum StateSnapping
{
	STATE_SNAPPING_NONE = 0,
	STATE_SNAPPING_ONLY_CHARACTER,
	STATE_SNAPPING_FULL,
};

class CPlayer
{
	MACRO_ALLOC_POOL_ID()

	struct StructLatency
	{
		int m_Accum;
		int m_AccumMin;
		int m_AccumMax;
		int m_Avg;
		int m_Min;
		int m_Max;
	};

	struct StructLastAction
	{
		int m_TargetX;
		int m_TargetY;
	};

	int m_SnapHealthNicknameTick;

protected:
	IServer* Server() const;

	CGS* m_pGS;
	int m_ClientID;
	CCharacter* m_pCharacter;
	bool m_Afk;
	bool m_LastInputInit;
	int64_t m_LastPlaytime;
	CClassData m_Class {};

public:
	CGS* GS() const { return m_pGS; }
	CClassData* GetClass() { return &m_Class; }
	const CClassData* GetClass() const { return &m_Class; }

	vec2 m_ViewPos;
	int m_PlayerFlags;
	int m_aPlayerTick[NUM_TICK];
	char m_aRotateClanBuffer[128];
	Mood m_MoodState;
	CCooldown m_Cooldown {};
	CVotePlayerData m_VotesData;

	char m_aLastMsg[256];
	int m_TutorialStep;

	StructLatency m_Latency;
	StructLastAction m_LatestActivity;

	/* ==========================================================
		VAR AND OBJECTS PLAYER MMO
	========================================================== */
	CTuningParams m_PrevTuningParams;
	CTuningParams m_NextTuningParams;
	CPlayerDialog m_Dialog;

	bool m_WantSpawn;
	bool m_RequestChangeNickname;
	bool m_ActivedGroupColors;
	int m_TickActivedGroupColors;
	std::optional<int> m_EidolonCID;

	/* ==========================================================
		FUNCTIONS PLAYER ENGINE
	========================================================== */
public:
	CNetObj_PlayerInput* m_pLastInput;

	CPlayer(CGS* pGS, int ClientID);
	virtual ~CPlayer();

	virtual int GetTeam();
	virtual bool IsBot() const { return false; }
	virtual int GetBotID() const { return -1; }
	virtual int GetBotType() const { return -1; }
	virtual int GetBotMobID() const { return -1; }
	virtual	int GetPlayerWorldID() const;
	virtual CTeeInfo& GetTeeInfo() const;

	virtual int GetStartHealth() const;
	int GetStartMana() const;
	virtual	int GetHealth() const { return GetTempData().m_TempHealth; }
	virtual	int GetMana() const { return GetTempData().m_TempMana; }
	bool IsAfk() const { return m_Afk; }
	int64_t GetAfkTime() const;

	void FormatBroadcastBasicStats(char* pBuffer, int Size, const char* pAppendStr = "\0");

	virtual void HandleTuningParams();
	virtual int64_t GetMaskVisibleForClients() const { return -1; }
	virtual StateSnapping IsActiveForClient(int ClientID) const { return STATE_SNAPPING_FULL; }
	virtual std::optional<int> GetEquippedItemID(ItemFunctional EquipID, int SkipItemID = -1) const;
	virtual bool IsEquipped(ItemFunctional EquipID) const;
	virtual int GetAttributeSize(AttributeIdentifier ID) const;
	float GetAttributePercent(AttributeIdentifier ID) const;
	virtual void UpdateTempData(int Health, int Mana);

	virtual bool GiveEffect(const char* Potion, int Sec, float Chance = 100.0f);
	virtual bool IsActiveEffect(const char* Potion) const;
	virtual void ClearEffects();

	virtual void Tick();
	virtual void PostTick();
	virtual void Snap(int SnappingClient);
	virtual void FakeSnap();
	virtual bool IsActive() const { return true; }
	virtual void PrepareRespawnTick();

	void RefreshClanString();

	CPlayerBot* GetEidolon() const;
	void TryCreateEidolon();
	void TryRemoveEidolon();
	void UpdateAchievement(int Type, int Misc, int Value, int ProgressType);

private:
	virtual void GetFormatedName(char* aBuffer, int BufferSize);
	virtual void HandleEffects();
	virtual void TryRespawn();
	void HandleScoreboardColors();
	void HandlePrison();

public:
	int GetCID() const { return m_ClientID; }
	CCharacter* GetCharacter() const;
	void KillCharacter(int Weapon = WEAPON_WORLD);
	void OnDisconnect();
	void OnDirectInput(CNetObj_PlayerInput* pNewInput);
	void OnPredictedInput(CNetObj_PlayerInput* pNewInput) const;

	void ProgressBar(const char* Name, int MyLevel, int MyExp, int ExpNeed, int GivedExp) const;
	bool Upgrade(int Value, int* Upgrade, int* Useless, int Price, int MaximalUpgrade) const;
	const char* GetLanguage() const;
	bool IsAuthed() const;
	int GetStartTeam() const;
	bool ParseVoteOptionResult(int Vote);
	bool IsClickedKey(int KeyID) const;

	CPlayerItem* GetItem(const CItem& Item) { return GetItem(Item.GetID()); }
	virtual CPlayerItem* GetItem(ItemIdentifier ID);
	CSkill* GetSkill(SkillIdentifier ID);
	CPlayerQuest* GetQuest(QuestIdentifier ID) const;
	CAccountTempData& GetTempData() const { return CAccountTempData::ms_aPlayerTempData[m_ClientID]; }
	CAccountData* Account() const { return &CAccountData::ms_aData[m_ClientID]; }

	int GetTypeAttributesSize(AttributeGroup Type);
	int GetAttributesSize();

	void SetSnapHealthTick(int Sec);

	virtual Mood GetMoodState() const { return Mood::NORMAL; }
	void ChangeWorld(int WorldID);

	/* ==========================================================
	   VOTING OPTIONAL EVENT
	========================================================== */
private:
	void HandleVoteOptionals() const;
};

#endif
