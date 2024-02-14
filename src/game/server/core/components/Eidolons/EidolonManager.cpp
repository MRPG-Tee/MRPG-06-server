﻿/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "EidolonManager.h"

#include <game/server/gamecontext.h>

using namespace sqlstr;

#define TW_TABLE_EIDOLON_ENHANCEMENTS "tw_account_eidolon_enhancements"

bool CEidolonManager::OnHandleVoteCommands(CPlayer* pPlayer, const char* CMD, int VoteID, int VoteID2, int Get, const char* GetText)
{
	const int ClientID = pPlayer->GetCID();
	if(PPSTR(CMD, "EIDOLON_SELECT") == 0)
	{
		m_EidolonItemSelected[ClientID] = VoteID;

		//pPlayer->m_TempMenuValue = MENU_EIDOLON_COLLECTION_SELECTED;
		pPlayer->m_VotesData.UpdateVotes(MENU_EIDOLON_COLLECTION_SELECTED);
		return true;
	}

	return false;
}

bool CEidolonManager::OnHandleMenulist(CPlayer* pPlayer, int Menulist)
{
	const int ClientID = pPlayer->GetCID();

	if(Menulist == MENU_EIDOLON_COLLECTION)
	{
		pPlayer->m_VotesData.SetLastMenuID(MENU_MAIN);

		CVoteWrapper VInfo(ClientID, VWFLAG_SEPARATE_CLOSED, "Eidolon Collection Information");
		VInfo.Add("Here you can see your collection of eidolons.");
		VInfo.AddLine();

		std::pair EidolonSize = GetEidolonsSize(ClientID);
		CVoteWrapper VEidolon(ClientID, VWFLAG_UNIQUE | VWFLAG_STYLE_SIMPLE, "\u2727 My eidolons (own {INT} out of {INT}).", EidolonSize.first, EidolonSize.second);

		for(auto& pEidolon : CEidolonInfoData::Data())
		{
			CPlayerItem* pPlayerItem = pPlayer->GetItem(pEidolon.GetItemID());
			const char* pCollectedInfo = (pPlayerItem->HasItem() ? "✔" : "\0");
			const char* pUsedAtMoment = pPlayerItem->IsEquipped() ? Server()->Localization()->Localize(pPlayer->GetLanguage(), "[summoned by you]") : "\0";
			VEidolon.AddMenu(MENU_EIDOLON_COLLECTION_SELECTED, pEidolon.GetItemID(), "{STR} {STR} {STR}", pEidolon.GetDataBot()->m_aNameBot, pCollectedInfo, pUsedAtMoment);
		}

		CVoteWrapper::AddBackpage(ClientID);
		return true;
	}

	if(Menulist == MENU_EIDOLON_COLLECTION_SELECTED)
	{
		pPlayer->m_VotesData.SetLastMenuID(MENU_EIDOLON_COLLECTION);

		int EidolonID = pPlayer->m_VotesData.GetMenuTemporaryInteger();
		if(CEidolonInfoData* pEidolonInfo = GS()->GetEidolonByItemID(EidolonID))
		{
			char aAttributeBonus[128];
			CPlayerItem* pPlayerItem = pPlayer->GetItem(pEidolonInfo->GetItemID());
			pPlayerItem->StrFormatAttributes(pPlayer, aAttributeBonus, sizeof(aAttributeBonus));

			CVoteWrapper VDesc(ClientID, VWFLAG_SEPARATE_OPEN | VWFLAG_STYLE_SIMPLE, "Descriptions of eidolon ({STR})", pEidolonInfo->GetDataBot()->m_aNameBot);
			for(auto& Line : pEidolonInfo->GetLinesDescription())
				VDesc.Add(Line.c_str());
			VDesc.AddLine();
			VDesc.Add(aAttributeBonus);
			VDesc.AddLine();

			CVoteWrapper VEnchancement(ClientID, VWFLAG_SEPARATE_OPEN | VWFLAG_STYLE_SIMPLE, "Unlocking Enhancements");
			VEnchancement.Add("Available soon.");
			VEnchancement.AddLine();

			if(pPlayerItem->HasItem())
			{
				const char* pStateSummon = Server()->Localization()->Localize(pPlayer->GetLanguage(), pPlayerItem->IsEquipped() ? "Call off the summoned" : "Summon");
				CVoteWrapper(ClientID).AddOption("ISETTINGS", pEidolonInfo->GetItemID(), NOPE, "{STR} {STR}", pStateSummon, pEidolonInfo->GetDataBot()->m_aNameBot);
			}
			else
			{
				CVoteWrapper(ClientID).Add("To summon it, you must first get it");
			}
		}

		CVoteWrapper::AddBackpage(ClientID);
		return true;
	}

	return false;
}

std::pair<int, int> CEidolonManager::GetEidolonsSize(int ClientID) const
{
	int Collect = 0;
	int Max = static_cast<int>(CEidolonInfoData::Data().size());

	if(CPlayerItem::Data().find(ClientID) != CPlayerItem::Data().end())
	{
		for(auto& p : CPlayerItem::Data()[ClientID])
		{
			if(p.second.HasItem() && p.second.Info()->IsType(ItemType::TYPE_EQUIP) && p.second.Info()->IsFunctional(
				EQUIP_EIDOLON))
				Collect++;
		}
	}

	return std::make_pair(Collect, Max);
}
