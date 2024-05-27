/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_CORE_COMPONENTS_AETHERNET_MANAGER_H
#define GAME_SERVER_CORE_COMPONENTS_AETHERNET_MANAGER_H
#include <game/server/core/mmo_component.h>

#include "aether_data.h"

class CAethernetManager : public MmoComponent
{
	inline static ska::unordered_map<int, std::deque<CAetherData*>> s_vpAetherSortedList {};

	~CAethernetManager() override
	{
		for(const auto& pAether : CAetherData::Data())
			delete pAether;
		CAetherData::Data().clear();
		CAetherData::Data().shrink_to_fit();
		s_vpAetherSortedList.clear();
	};

	void OnInit() override;
	void OnPlayerLogin(CPlayer* pPlayer) override;
	bool OnCharacterTile(CCharacter* pChr) override;
	bool OnPlayerMenulist(CPlayer* pPlayer, int Menulist) override;
	bool OnPlayerVoteCommand(CPlayer* pPlayer, const char* pCmd, int Extra1, int Extra2, int ReasonNumber, const char* pReason) override;

	// vote list's menus
	void ShowMenu(CCharacter* pChar) const;

	// unlock location by position
	void UnlockLocationByPos(CPlayer* pPlayer, vec2 Pos) const;

	// get aether by id
	CAetherData* GetAetherByID(int AetherID) const;

	// get aether by position
	CAetherData* GetAetherByPos(vec2 Pos) const;
};

#endif