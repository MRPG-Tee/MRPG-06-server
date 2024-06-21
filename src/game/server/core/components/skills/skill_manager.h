/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_COMPONENT_SKILL_CORE_H
#define GAME_SERVER_COMPONENT_SKILL_CORE_H
#include <game/server/core/mmo_component.h>

#include "skill_data.h"

class CSkillManager : public MmoComponent
{
	~CSkillManager() override
	{
		// free data
		mrpgstd::cleaning_free_container_data(CSkill::Data(), CSkillDescription::Data());
	}

	void OnInit() override;
	void OnPlayerLogin(CPlayer* pPlayer) override;
	void OnClientReset(int ClientID) override;
	bool OnCharacterTile(CCharacter* pChr) override;
	bool OnPlayerMenulist(CPlayer* pPlayer, int Menulist) override;
	bool OnPlayerVoteCommand(CPlayer* pPlayer, const char* pCmd, int Extra1, int Extra2, int ReasonNumber, const char* pReason) override;

	// vote list's menus
	void ShowSkillList(CPlayer* pPlayer, const char* pTitle, SkillType Type) const;
	void ShowSkill(CPlayer* pPlayer, SkillIdentifier ID) const;

public:
	// use skills by emoticion
	void UseSkillsByEmoticion(CPlayer* pPlayer, int EmoticionID);
};

#endif
