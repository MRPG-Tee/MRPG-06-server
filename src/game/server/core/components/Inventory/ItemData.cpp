/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "ItemData.h"

#include <game/server/gamecontext.h>
#include <game/server/entity_manager.h>
#include "game/server/core/components/Eidolons/EidolonManager.h"

CGS* CPlayerItem::GS() const
{
	return (CGS*)Server()->GameServerPlayer(m_ClientID);
}

CPlayer* CPlayerItem::GetPlayer() const
{
	return GS()->GetPlayer(m_ClientID);
}

inline int randomRangecount(int startrandom, int endrandom, int count)
{
	int result = 0;
	for(int i = 0; i < count; i++)
	{
		int random = startrandom + rand() % (endrandom - startrandom);
		result += random;
	}
	return result;
}

bool CPlayerItem::SetEnchant(int Enchant)
{
	if(m_Value < 1 || !GetPlayer() || !GetPlayer()->IsAuthed())
		return false;

	m_Enchant = Enchant;
	return Save();
}

bool CPlayerItem::SetSettings(int Settings)
{
	if(m_Value < 1 || !GetPlayer() || !GetPlayer()->IsAuthed())
		return false;

	m_Settings = Settings;
	return Save();
}

bool CPlayerItem::SetDurability(int Durability)
{
	if(m_Value < 1 || !GetPlayer() || !GetPlayer()->IsAuthed())
		return false;

	m_Durability = Durability;
	return Save();
}

bool CPlayerItem::SetValue(int Value)
{
	bool Changes = false;
	if(m_Value > Value)
		Changes = Remove((m_Value - Value));
	else if(m_Value < Value)
		Changes = Add((Value - m_Value), m_Settings, m_Enchant, false);
	return Changes;
}

bool CPlayerItem::Add(int Value, int StartSettings, int StartEnchant, bool Message)
{
	CPlayer* pPlayer = GetPlayer();
	if(Value < 1 || !pPlayer || !pPlayer->IsAuthed())
		return false;

	// check enchantable type
	const int ClientID = pPlayer->GetCID();
	if(Info()->IsEnchantable())
	{
		if(m_Value > 0)
		{
			GS()->Chat(ClientID, "This item cannot have more than 1 item");
			return false;
		}
		Value = 1;
	}

	if(!m_Value)
	{
		m_Enchant = StartEnchant;
		m_Settings = StartSettings;

		// run event got
		Info()->RunEvent(pPlayer, CItemDescription::OnEventGot);
	}

	m_Value += Value;

	// achievement for item
	pPlayer->UpdateAchievement(ACHIEVEMENT_RECEIVE_ITEM, m_ID, Value, PROGRESS_ADD);
	pPlayer->UpdateAchievement(ACHIEVEMENT_HAVE_ITEM, m_ID, m_Value, PROGRESS_SET);

	// check the empty slot if yes then put the item on
	if((Info()->IsType(ItemType::TYPE_EQUIP) && !pPlayer->GetEquippedItemID(Info()->GetFunctional()).has_value()) || Info()->IsType(ItemType::TYPE_MODULE))
	{
		if(!IsEquipped())
			Equip(false);

		GS()->Chat(ClientID, "Auto equip {} - {}", Info()->GetName(), GetStringAttributesInfo(pPlayer));
	}

	if(!Message || Info()->IsType(ItemType::TYPE_SETTINGS) || Info()->IsType(ItemType::TYPE_INVISIBLE))
		return Save();

	if(Info()->IsType(ItemType::TYPE_EQUIP) || Info()->IsType(ItemType::TYPE_MODULE))
	{
		GS()->Chat(-1, "{} got of the {}.", GS()->Server()->ClientName(ClientID), Info()->GetName());
		if(Info()->IsFunctional(EQUIP_EIDOLON))
		{
			std::pair EidolonSize = GS()->Core()->EidolonManager()->GetEidolonsSize(ClientID);
			GS()->Chat(-1, "{} has a collection {} out of {} eidolons.", GS()->Server()->ClientName(ClientID), EidolonSize.first, EidolonSize.second);
		}
	}
	else
	{
		GS()->Chat(ClientID, "You obtain an {}x{}({}).", Info()->GetName(), Value, m_Value);
	}

	return Save();
}

bool CPlayerItem::Remove(int Value)
{
	Value = minimum(Value, m_Value);
	if(Value <= 0 || !GetPlayer())
		return false;

	// unequip if this is the last item
	if(m_Value <= Value && IsEquipped())
	{
		Equip(false);

		// run event lost
		Info()->RunEvent(GetPlayer(), CItemDescription::OnEventLost);
	}

	m_Value -= Value;
	return Save();
}

bool CPlayerItem::Equip(bool SaveItem)
{
	CPlayer* pPlayer = GetPlayer();
	if(m_Value < 1 || !pPlayer || !pPlayer->IsAuthed())
		return false;

	m_Settings ^= true;

	if(Info()->IsType(ItemType::TYPE_EQUIP))
	{
		const ItemFunctional EquipID = Info()->GetFunctional();
		auto ItemID = pPlayer->GetEquippedItemID(EquipID, m_ID);
		while(ItemID.has_value())
		{
			CPlayerItem* pPlayerItem = pPlayer->GetItem(ItemID.value());
			pPlayerItem->SetSettings(0);
			ItemID = pPlayer->GetEquippedItemID(EquipID, m_ID);
		}
	}

	if(pPlayer->GetCharacter())
		pPlayer->GetCharacter()->UpdateEquipingStats(m_ID);

	// achievement for item
	pPlayer->UpdateAchievement(ACHIEVEMENT_EQUIP, m_ID, m_Settings, PROGRESS_SET);

	// run event equip and unequip
	Info()->RunEvent(pPlayer, m_Settings ? CItemDescription::OnEventEquip : CItemDescription::OnEventUnequip);
	GS()->MarkUpdatedBroadcast(m_ClientID);
	return SaveItem ? Save() : true;
}

bool CPlayerItem::Use(int Value)
{
	Value = Info()->IsFunctional(FUNCTION_ONE_USED) ? 1 : minimum(Value, m_Value);
	if(Value <= 0 || !GetPlayer() || !GetPlayer()->IsAuthed())
		return false;

	const int ClientID = GetPlayer()->GetCID();

	// potion mana regen
	if(m_ID == itPotionManaRegen && Remove(Value))
	{
		GetPlayer()->GiveEffect("RegenMana", 15);
		GS()->Chat(ClientID, "You used {}x{}", Info()->GetName(), Value);
		return true;
	}
	// ticket discount craft
	if(m_ID == itTicketDiscountCraft)
	{
		GS()->Chat(ClientID, "This item can only be used (Auto Use, and then craft).");
		return true;
	}
	// survial capsule experience
	if(m_ID == itCapsuleSurvivalExperience && Remove(Value))
	{
		int Getting = randomRangecount(10, 50, Value);
		GS()->Chat(-1, "{} used {}x{} and got {} survival experience.", GS()->Server()->ClientName(ClientID), Info()->GetName(), Value, Getting);
		GetPlayer()->Account()->AddExperience(Getting);
		return true;
	}
	// little bag gold
	if(m_ID == itLittleBagGold && Remove(Value))
	{
		int Getting = randomRangecount(10, 50, Value);
		GS()->Chat(-1, "{} used {}x{} and got {} gold.", GS()->Server()->ClientName(ClientID), Info()->GetName(), Value, Getting);
		GetPlayer()->Account()->AddGold(Getting);
		return true;
	}
	// ticket reset for class stats
	if(m_ID == itTicketResetClassStats && Remove(Value))
	{
		int BackUpgrades = 0;
		for(const auto& [ID, pAttribute] : CAttributeDescription::Data())
		{
			if(pAttribute->HasDatabaseField() && GetPlayer()->Account()->m_aStats[ID] > 0)
			{
				// skip weapon spreading
				if(pAttribute->IsGroup(AttributeGroup::Weapon))
					continue;

				BackUpgrades += GetPlayer()->Account()->m_aStats[ID] * pAttribute->GetUpgradePrice();
				GetPlayer()->Account()->m_aStats[ID] = 0;
			}
		}

		GS()->Chat(-1, "{} used {} returned {} upgrades.", GS()->Server()->ClientName(ClientID), Info()->GetName(), BackUpgrades);
		GetPlayer()->Account()->m_Upgrade += BackUpgrades;
		GS()->Core()->SaveAccount(GetPlayer(), SAVE_UPGRADES);
		return true;
	}
	// ticket reset for weapons stats
	if(m_ID == itTicketResetWeaponStats && Remove(Value))
	{
		int BackUpgrades = 0;
		for(const auto& [ID, pAttribute] : CAttributeDescription::Data())
		{
			if(pAttribute->HasDatabaseField() && GetPlayer()->Account()->m_aStats[ID] > 0)
			{
				// skip all stats allow only weapons
				if(pAttribute->GetGroup() != AttributeGroup::Weapon)
					continue;

				int UpgradeValue = GetPlayer()->Account()->m_aStats[ID];
				if(ID == AttributeIdentifier::SpreadShotgun)
					UpgradeValue = GetPlayer()->Account()->m_aStats[ID] - 3;
				else if(ID == AttributeIdentifier::SpreadGrenade || ID == AttributeIdentifier::SpreadRifle)
					UpgradeValue = GetPlayer()->Account()->m_aStats[ID] - 1;

				if(UpgradeValue <= 0)
					continue;

				BackUpgrades += UpgradeValue * pAttribute->GetUpgradePrice();
				GetPlayer()->Account()->m_aStats[ID] -= UpgradeValue;
			}
		}

		GS()->Chat(-1, "{} used {} returned {} upgrades.", GS()->Server()->ClientName(ClientID), Info()->GetName(), BackUpgrades);
		GetPlayer()->Account()->m_Upgrade += BackUpgrades;
		GS()->Core()->SaveAccount(GetPlayer(), SAVE_UPGRADES);
		return true;
	}

	// potion health regen
	if(const PotionTools::Heal* pHeal = PotionTools::Heal::getHealInfo(m_ID); pHeal)
	{
		if(GetPlayer()->m_aPlayerTick[PotionRecast] >= Server()->Tick())
			return true;

		if(Remove(Value))
		{
			int PotionTime = pHeal->getTime();
			GetPlayer()->GiveEffect(pHeal->getEffect(), PotionTime);
			GetPlayer()->m_aPlayerTick[PotionRecast] = Server()->Tick() + ((PotionTime + POTION_RECAST_APPEND_TIME) * Server()->TickSpeed());

			GS()->Chat(ClientID, "You used {}x{}", Info()->GetName(), Value);
			GS()->EntityManager()->Text(GetPlayer()->m_ViewPos + vec2(0, -140.0f), 70, pHeal->getEffect());
		}
		return true;
	}

	// or if it random box
	if(Info()->GetRandomBox())
	{
		Info()->GetRandomBox()->Start(GetPlayer(), 5, this, Value);
		return true;
	}

	return true;
}

bool CPlayerItem::Drop(int Value)
{
	Value = minimum(Value, m_Value);
	if(Value <= 0 || !GetPlayer() || !GetPlayer()->IsAuthed() || !GetPlayer()->GetCharacter())
		return false;

	CCharacter* m_pCharacter = GetPlayer()->GetCharacter();
	vec2 Force = vec2(m_pCharacter->m_Core.m_Input.m_TargetX, m_pCharacter->m_Core.m_Input.m_TargetY);
	if(length(Force) > 8.0f)
		Force = normalize(Force) * 8.0f;

	CPlayerItem DropItem = *this;
	if(Remove(Value))
	{
		DropItem.m_Value = Value;
		GS()->EntityManager()->DropItem(m_pCharacter->m_Core.m_Pos, -1, static_cast<CItem>(DropItem), Force);
		return true;
	}
	return false;
}

bool CPlayerItem::Save()
{
	if(GetPlayer() && GetPlayer()->IsAuthed())
	{
		int UserID = GetPlayer()->Account()->GetID();
		const auto pResCheck = Database->Prepare<DB::SELECT>("ItemID, UserID", "tw_accounts_items", "WHERE ItemID = '%d' AND UserID = '%d'", m_ID, UserID);
		pResCheck->AtExecute([this, UserID](ResultPtr pRes)
		{
			// check database value
			if(pRes->next())
			{
				// remove item
				if(!m_Value)
				{
					Database->Execute<DB::REMOVE>("tw_accounts_items", "WHERE ItemID = '%d' AND UserID = '%d'", m_ID, UserID);
					return;
				}

				// update an item
				Database->Execute<DB::UPDATE>("tw_accounts_items", "Value = '%d', Settings = '%d', Enchant = '%d', Durability = '%d' WHERE UserID = '%d' AND ItemID = '%d'",
					m_Value, m_Settings, m_Enchant, m_Durability, GetPlayer()->Account()->GetID(), m_ID);
				return;
			}

			// insert item
			if(m_Value)
			{
				m_Durability = 100;
				Database->Execute<DB::INSERT>("tw_accounts_items", "(ItemID, UserID, Value, Settings, Enchant) VALUES ('%d', '%d', '%d', '%d', '%d')", m_ID, UserID, m_Value, m_Settings, m_Enchant);
			}
		});
		return true;
	}
	return false;
}

// helper functions
CItem CItem::FromJSON(const nlohmann::json& json)
{
	try
	{
		ItemIdentifier ID = json.value("id", 0);
		int Value = json.value("value", 1);
		int Enchant = json.value("enchant", 0);
		int Durability = json.value("durability", 0);

		CItem Item(ID, Value, Enchant, Durability);
		return Item;
	}
	catch (nlohmann::json::exception& s)
	{
		dbg_msg("CItem(FromJSON)", "%s (json %s)", json.dump().c_str(), s.what());
	}

	return {};
}

CItemsContainer CItem::FromArrayJSON(const nlohmann::json& json, const char* pField)
{
	CItemsContainer Container;
	try
	{
		if(json.find(pField) != json.end())
		{
			for(auto& p : json[pField])
			{
				ItemIdentifier ID = p.value("id", 0);
				int Value = p.value("value", 0);
				int Enchant = p.value("enchant", 0);
				int Durability = p.value("durability", 0);

				CItem Item(ID, Value, Enchant, Durability);
				Container.push_back(Item);
			}
		}
	}
	catch (nlohmann::json::exception& s)
	{
		dbg_msg("CItem(FromArrayJson)", "(json %s)", s.what());
	}

	return Container;
}

void CItem::ToJSON(CItem& Item, nlohmann::json& json)
{
	try
	{
		json["id"] = Item.GetID();
		json["value"] = Item.GetValue();
		json["enchant"] = Item.GetEnchant();
		json["durability"] = Item.GetDurability();
	}
	catch(nlohmann::json::exception& s)
	{
		dbg_msg("CItem(ToJSON)", "%s (json %s)", json.dump().c_str(), s.what());
	}
}

void CItem::ToArrayJSON(CItemsContainer& vItems, nlohmann::json& json, const char* pField)
{
	try
	{
		for(auto& p : vItems)
		{
			nlohmann::json jsItem {};
			jsItem["id"] = p.GetID();
			jsItem["value"] = p.GetValue();
			jsItem["enchant"] = p.GetEnchant();
			jsItem["durability"] = p.GetDurability();
			json[pField].push_back(jsItem);
		}
	}
	catch (nlohmann::json::exception& s)
	{
		dbg_msg("CItem(ToArrayJson)", "(json %s)", s.what());
	}
}
