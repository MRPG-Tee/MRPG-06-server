/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "AccountData.h"

#include <game/server/entity_manager.h>
#include <game/server/gamecontext.h>

#include "../houses/house_data.h"
#include "../achievements/achievement_data.h"
#include "../guilds/guild_manager.h"
#include "../worlds/world_manager.h"

std::map < int, CAccountData > CAccountData::ms_aData;
std::map < int, CAccountTempData > CAccountTempData::ms_aPlayerTempData;

CGS* CAccountData::GS() const
{
	return m_pPlayer ? m_pPlayer->GS() : nullptr;
}

int CAccountData::GetGoldCapacity() const
{
	return DEFAULT_MAX_PLAYER_BAG_GOLD + m_pPlayer->GetTotalAttributeValue(AttributeIdentifier::GoldCapacity);
}

// Set the ID of the account
void CAccountData::Init(int ID, CPlayer* pPlayer, const char* pLogin, std::string Language, std::string LoginDate, ResultPtr pResult)
{
	// Check if the ID has already been set
	dbg_assert(m_ID <= 0 || !pResult, "Unique AccountID cannot change the value more than 1 time");

	// Get the server instance
	int ClientID = pPlayer->GetCID();
	IServer* pServer = Instance::Server();
	/*
		Initialize object
	*/
	m_ID = ID;
	m_pPlayer = pPlayer;
	str_copy(m_aLogin, pLogin, sizeof(m_aLogin));
	str_copy(m_aLastLogin, LoginDate.c_str(), sizeof(m_aLastLogin));

	// base data
	m_Level = pResult->getInt("Level");
	m_Exp = pResult->getInt("Exp");
	m_Upgrade = pResult->getInt("Upgrade");
	m_PrisonSeconds = pResult->getInt("PrisonSeconds");
	m_CrimeScore = pResult->getInt("CrimeScore");
	m_aHistoryWorld.push_front(pResult->getInt("WorldID"));
	m_ClassGroup = (ClassGroup)pResult->getInt("Class");
	m_Bank = pResult->getString("Bank").c_str();

	// achievements data
	InitAchievements(pResult->getString("Achievements").c_str());

	// time periods
	{
		m_Periods.m_DailyStamp = pResult->getInt64("DailyStamp");
		m_Periods.m_WeekStamp = pResult->getInt64("WeekStamp");
		m_Periods.m_MonthStamp = pResult->getInt64("MonthStamp");
	}

	// upgrades data
	for(const auto& [AttrbiteID, pAttribute] : CAttributeDescription::Data())
	{
		if(pAttribute->HasDatabaseField())
			m_aStats[AttrbiteID] = pResult->getInt(pAttribute->GetFieldName());
	}

	pServer->SetClientLanguage(ClientID, Language.c_str());
	pServer->SetClientScore(ClientID, m_Level);

	// Execute a database update query to update the "tw_accounts" table
	// Set the LoginDate to the current timestamp and LoginIP to the client address
	// The update query is executed on the row with the ID equal to the given UserID
	char aAddrStr[64];
	pServer->GetClientAddr(ClientID, aAddrStr, sizeof(aAddrStr));
	Database->Execute<DB::UPDATE>("tw_accounts", "LoginDate = CURRENT_TIMESTAMP, LoginIP = '%s', CountryISO = '%s' WHERE ID = '%d'", 
		aAddrStr, Instance::Server()->ClientCountryIsoCode(ClientID), ID);

	/*
		Initialize sub account data.
	*/
	ReinitializeHouse();
	ReinitializeGuild();
	m_BonusManager.Init(m_ClientID, m_pPlayer);
}

void CAccountData::InitAchievements(const std::string& Data)
{
	// initialize player base
	std::map<int, CAchievement*> m_apReferenceMap {};
	for(const auto& pAchievement : CAchievementInfo::Data())
	{
		const int AchievementID = pAchievement->GetID();
		m_apReferenceMap[AchievementID] = CAchievement::CreateElement(pAchievement, m_pPlayer->GetCID());
	}

	// initialize player achievements
	mystd::json::parse(Data, [&m_apReferenceMap, this](nlohmann::json& pJson)
	{
		for(auto& p : pJson)
		{
			int AchievementID = p.value("aid", -1);
			int Progress = p.value("progress", 0);
			bool Completed = p.value("completed", false);

			if(m_apReferenceMap.contains(AchievementID))
				m_apReferenceMap[AchievementID]->Init(Progress, Completed);
		}
		m_AchievementsData = std::move(pJson);
	});

	// clear reference map
	m_apReferenceMap.clear();
}

void CAccountData::UpdatePointer(CPlayer* pPlayer)
{
	dbg_assert(m_pPlayer != nullptr, "AccountManager pointer must always exist");

	m_pPlayer = pPlayer;
	m_ClientID = pPlayer->GetCID();

	// update class data
	m_pPlayer->GetClass()->Init(m_ClassGroup);
	m_pPlayer->GetClass()->SetClassSkin(m_TeeInfos, m_pPlayer->GetItem(itCustomizer)->IsEquipped());
}

// This function initializes the house data for the account
void CAccountData::ReinitializeHouse(bool SetNull)
{
	if(!SetNull)
	{
		// Iterate through all the group data objects
		for(auto pHouse : CHouse::Data())
		{
			// Check if the account ID of the group data object matches the account ID of the current account
			if(pHouse->GetAccountID() == m_ID)
			{
				m_pHouseData = pHouse;
				return;
			}
		}
	}

	// If no matching group data object is found, set the group data pointer of the account to nullptr
	m_pHouseData = nullptr;
}

void CAccountData::ReinitializeGuild(bool SetNull)
{
	if(!SetNull)
	{
		// Iterate through all the group data objects
		for(auto pGuild : CGuild::Data())
		{
			// Check if the account ID of the group data object matches the account ID of the current account
			auto& pMembers = pGuild->GetMembers()->GetContainer();
			if(pMembers.find(m_ID) != pMembers.end() && pMembers.at(m_ID) != nullptr)
			{
				m_pGuildData = pGuild;
				return;
			}
		}
	}

	// If no matching group data object is found, set the group data pointer of the account to nullptr
	m_pGuildData = nullptr;
}

CGuild::CMember* CAccountData::GetGuildMember() const
{
	return m_pGuildData ? m_pGuildData->GetMembers()->Get(m_ID) : nullptr;
}

bool CAccountData::IsClientSameGuild(int ClientID) const
{
	if(!m_pGuildData)
		return false;

	CPlayer* pPlayer = GS()->GetPlayer(ClientID, true);
	return pPlayer && pPlayer->Account()->GetGuild() && pPlayer->Account()->GetGuild()->GetID() == m_pGuildData->GetID();
}

bool CAccountData::IsSameGuild(int GuildID) const
{
	return m_pGuildData && m_pGuildData->GetID() == GuildID;
}

void CAccountData::IncreaseCrimeScore(int Score)
{
	if(!m_pPlayer || IsCrimeScoreMaxedOut())
		return;

	m_CrimeScore = minimum(m_CrimeScore + Score, 100);
	GS()->Chat(m_ClientID, "Your Crime Score has increased to {}%!", m_CrimeScore);

	if(m_CrimeScore >= 100)
		GS()->Chat(m_ClientID, "You have reached the maximum Crime Score and are now a wanted criminal! Be cautious, as law enforcers are actively searching for you.");

	GS()->Core()->SaveAccount(m_pPlayer, SAVE_SOCIAL_STATUS);
}

void CAccountData::Imprison(int Seconds)
{
	if(!m_pPlayer)
		return;

	// kill character
	if(m_pPlayer->GetCharacter())
		m_pPlayer->GetCharacter()->Die(m_pPlayer->GetCID(), WEAPON_WORLD);

	vec2 SpawnPos;
	if(GS()->m_pController->CanSpawn(SPAWN_HUMAN_PRISON, &SpawnPos))
	{
		// Set the prison seconds and send a chat message to all players indicating that the player has been imprisoned
		m_PrisonSeconds = Seconds;
		GS()->Chat(-1, "{}, has been imprisoned for {} seconds.", Instance::Server()->ClientName(m_pPlayer->GetCID()), Seconds);
		GS()->Core()->SaveAccount(m_pPlayer, SAVE_SOCIAL_STATUS);
	}
}

void CAccountData::FreeFromPrison()
{
	if(!m_pPlayer)
		return;

	// kill character
	if(m_pPlayer->GetCharacter())
		m_pPlayer->GetCharacter()->Die(m_pPlayer->GetCID(), WEAPON_WORLD);

	m_PrisonSeconds = -1;
	GS()->Chat(-1, "{} has been released from prison.", Instance::Server()->ClientName(m_pPlayer->GetCID()));
	GS()->Core()->SaveAccount(m_pPlayer, SAVE_SOCIAL_STATUS);
}

int CAccountData::GetGold() const
{
	return m_pPlayer ? m_pPlayer->GetItem(itGold)->GetValue() : 0;
}

BigInt CAccountData::GetTotalGold() const
{
	return m_pPlayer ? m_Bank + m_pPlayer->GetItem(itGold)->GetValue() : 0;
}

void CAccountData::AddExperience(int Value)
{
	if(!m_pPlayer)
		return;

	// Increase the experience value
	m_BonusManager.ApplyBonuses(BONUS_TYPE_EXPERIENCE, &Value);
	m_Exp += Value;

	// Check if the experience is enough to level up
	while(m_Exp >= (int)computeExperience(m_Level))
	{
		// Reduce the experience and increase the level
		m_Exp -= (int)computeExperience(m_Level);
		m_Level++;
		m_Upgrade += 1;

		// increase skill points
		if(g_Config.m_SvSPEachLevel > 0)
		{
			auto pPlayerItemSP = m_pPlayer->GetItem(itSkillPoint);
			pPlayerItemSP->Add(g_Config.m_SvSPEachLevel);
			GS()->Chat(m_ClientID, "You have earned {} Skill Points! You now have {} SP!", g_Config.m_SvSPEachLevel, pPlayerItemSP->GetValue());
		}

		// effects
		if(CCharacter* pChar = m_pPlayer->GetCharacter())
		{
			GS()->CreateDeath(pChar->m_Core.m_Pos, m_ClientID);
			GS()->CreateSound(pChar->m_Core.m_Pos, 4);
			GS()->EntityManager()->Text(pChar->GetPos() + vec2(0, -40), 30, "level up");
		}

		GS()->Chat(m_ClientID, "Congratulations. You attain level {}!", m_Level);
		GS()->Core()->WorldManager()->NotifyUnlockedZonesByLeveling(m_pPlayer, m_ID);

		// post leveling
		if(m_Exp < (int)computeExperience(m_Level))
		{
			// Update votes, save stats, and save upgrades
			m_pPlayer->m_VotesData.UpdateVotesIf(MENU_MAIN);
			GS()->Core()->SaveAccount(m_pPlayer, SAVE_STATS);
			GS()->Core()->SaveAccount(m_pPlayer, SAVE_UPGRADES);
			m_pPlayer->UpdateAchievement(AchievementType::Leveling, NOPE, m_Level, PROGRESS_ABSOLUTE);
		}
	}

	// update the progress bar
	m_pPlayer->ProgressBar("Account", m_Level, m_Exp, (int)computeExperience(m_Level), Value);

	// randomly save the account stats
	if(rand() % 5 == 0)
	{
		GS()->Core()->SaveAccount(m_pPlayer, SAVE_STATS);
	}

	// add experience to the guild member
	if(HasGuild())
	{
		m_pGuildData->AddExperience(1);
	}
}

void CAccountData::AddGold(int Value, bool ToBank, bool ApplyBonuses)
{
	if(!m_pPlayer)
		return;

	// apply bonuses
	if(ApplyBonuses)
		m_BonusManager.ApplyBonuses(BONUS_TYPE_GOLD, &Value);

	// to bank
	if(ToBank)
	{
		m_Bank += Value;
		GS()->Core()->SaveAccount(m_pPlayer, SAVE_STATS);
		return;
	}

	// initialize variables
	CPlayerItem* pGoldItem = m_pPlayer->GetItem(itGold);
	const int CurrentGold = pGoldItem->GetValue();
	const int FreeSpace = GetGoldCapacity() - CurrentGold;

	// add golds
	if(Value > FreeSpace)
	{
		pGoldItem->Add(FreeSpace);
		m_Bank += (Value - FreeSpace);
		GS()->Core()->SaveAccount(m_pPlayer, SAVE_STATS);
	}
	else
	{
		pGoldItem->Add(Value);
	}
}

bool CAccountData::SpendCurrency(int Price, int CurrencyItemID)
{
	if(!m_pPlayer)
		return false;

	// check is free
	if(Price <= 0)
		return true;

	// initialize variables
	CPlayerItem* pCurrencyItem = m_pPlayer->GetItem(CurrencyItemID);
	const int PlayerCurrency = pCurrencyItem->GetValue();

	// gold with bank
	if(CurrencyItemID == itGold)
	{
		BigInt TotalCurrency = m_Bank + pCurrencyItem->GetValue();
		if(PlayerCurrency < Price)
		{
			GS()->Chat(m_ClientID, "Required {}, but you only have {} {} (including bank)!", Price, PlayerCurrency, pCurrencyItem->Info()->GetName());
			return false;
		}

		// first, spend currency from player's hand
		int RemainingPrice = Price;
		if(PlayerCurrency > 0)
		{
			int ToSpendFromHands = minimum(PlayerCurrency, RemainingPrice);
			pCurrencyItem->Remove(ToSpendFromHands);
			RemainingPrice -= ToSpendFromHands;
		}

		// second, spend the remaining currency from the bank
		if(RemainingPrice > 0)
		{
			m_Bank -= RemainingPrice;
			GS()->Core()->SaveAccount(m_pPlayer, SAVE_STATS);
		}

		return true;
	}

	// other currency
	if(PlayerCurrency < Price)
	{
		GS()->Chat(m_ClientID, "Required {}, but you only have {} {} (including bank)!", Price, PlayerCurrency, pCurrencyItem->Info()->GetName());
		return false;
	}

	return pCurrencyItem->Remove(Price);
}

bool CAccountData::DepositGoldToBank(int Amount)
{
	if(!m_pPlayer)
		return false;

	// initialize variables
	CPlayerItem* pItemGold = m_pPlayer->GetItem(itGold);
	int CurrentGold = pItemGold->GetValue();

	// check enough gold in inventory
	if(CurrentGold < Amount)
	{
		GS()->Chat(m_ClientID, "You don't have enough gold in your inventory. You only have {$} gold.", CurrentGold);
		return false;
	}

	// remove gold and add to bank
	if(pItemGold->Remove(Amount))
	{
		m_Bank += Amount;
		GS()->Core()->SaveAccount(m_pPlayer, SAVE_STATS);
		GS()->Chat(m_ClientID, "You have deposited {$} gold into your bank.", Amount);
		return true;
	}

	return false;
}

bool CAccountData::WithdrawGoldFromBank(int Amount)
{
	if(!m_pPlayer)
		return false;

	// initialize variables
	CPlayerItem* pItemGold = m_pPlayer->GetItem(itGold);
	int CurrentGold = pItemGold->GetValue();
	int AvailableSpace = GetGoldCapacity() - CurrentGold;

	// check bank amount
	if(m_Bank < Amount)
	{
		GS()->Chat(m_ClientID, "You only have {$} gold in your bank, but you tried to withdraw {}!", m_Bank, Amount);
		return false;
	}

	// calculate gold for withdraw
	int GoldToWithdraw = minimum(Amount, AvailableSpace);

	if(GoldToWithdraw > 0)
	{
		pItemGold->Add(GoldToWithdraw);
		m_Bank -= GoldToWithdraw;
		GS()->Core()->SaveAccount(m_pPlayer, SAVE_STATS);
		GS()->Chat(m_ClientID, "You have withdrawn {$} gold from your bank to your inventory.", GoldToWithdraw);
	}

	if(GoldToWithdraw < Amount)
	{
		GS()->Chat(m_ClientID, "Your inventory is full. You could only withdraw {$} gold.", GoldToWithdraw);
	}

	return true;
}

void CAccountData::ResetCrimeScore()
{
	if(!m_pPlayer)
		return;

	m_CrimeScore = 0;
	GS()->Core()->SaveAccount(m_pPlayer, SAVE_SOCIAL_STATUS);
}

void CAccountData::HandleChair(int Exp, int Gold)
{
	if(!m_pPlayer)
		return;

	IServer* pServer = Instance::Server();
	if(pServer->Tick() % pServer->TickSpeed() != 0)
		return;

	// initialize variables
	const int maxGoldCapacity = GetGoldCapacity();
	const bool isGoldBagFull = (GetGold() >= maxGoldCapacity);

	int expGain = maximum(Exp, (int)calculate_exp_gain(g_Config.m_SvChairExpFactor, m_Level, m_Level + Exp));
	int goldGain = isGoldBagFull ? 0 : maximum(Gold, (int)calculate_gold_gain(g_Config.m_SvChairGoldFactor, m_Level, m_Level + Gold));

	// total percent bonuses
	int totalPercentBonusGold = round_to_int(m_BonusManager.GetTotalBonusPercentage(BONUS_TYPE_GOLD));
	int totalPercentBonusExp = round_to_int(m_BonusManager.GetTotalBonusPercentage(BONUS_TYPE_EXPERIENCE));

	// add exp & gold
	AddExperience(expGain);
	if(!isGoldBagFull)
	{
		AddGold(goldGain, false, true);
	}

	// format
	std::string expStr = "+" + std::to_string(expGain);
	std::string goldStr = goldGain > 0 ? "+" + std::to_string(goldGain) : "Bag Full";

	// add bonus information
	if(totalPercentBonusExp > 0)
	{
		expStr += " (+" + std::to_string(totalPercentBonusExp) + "% bonus)";
	}
	if(totalPercentBonusGold > 0 && goldGain > 0)
	{
		goldStr += " (+" + std::to_string(totalPercentBonusGold) + "% bonus)";
	}

	// send broadcast
	GS()->Broadcast(m_pPlayer->GetCID(), BroadcastPriority::MAIN_INFORMATION, 250,
		"Gold {$} of {$} (Total: {$}) : {}\nExp {}/{} : {}",
		GetGold(), maxGoldCapacity, GetTotalGold(), goldStr.c_str(), m_Exp, computeExperience(m_Level), expStr.c_str());
}

void CAccountData::UpdateAchievementProgress(int AchievementID, int Progress, bool Completed)
{
	const auto it = std::ranges::find_if(m_AchievementsData, [AchievementID](const nlohmann::json& obj)
	{
		return obj.value("aid", -1) == AchievementID;
	});

	// update
	if(it != m_AchievementsData.end())
	{
		(*it)["progress"] = Progress;
		(*it)["completed"] = Completed;
	}
	else
	{
		nlohmann::json newAchievement;
		newAchievement["aid"] = AchievementID;
		newAchievement["progress"] = Progress;
		newAchievement["completed"] = Completed;
		m_AchievementsData.push_back(newAchievement);
	}
}