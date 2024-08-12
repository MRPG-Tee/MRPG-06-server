#ifndef GAME_SERVER_CORE_COMPONENTS_ACCOUNTS_BONUS_MANAGER_H
#define GAME_SERVER_CORE_COMPONENTS_ACCOUNTS_BONUS_MANAGER_H

class CPlayer;

enum
{
	BONUS_TYPE_EXPERIENCE = 1,
	BONUS_TYPE_GOLD,
	BONUS_TYPE_HP,
	BONUS_TYPE_MP,
	BONUS_TYPE_DMG,
	END_BONUS_TYPE,
};

struct TemporaryBonus
{
	int Type{};
	float Amount{};
	time_t StartTime{};
	int Duration{};

	bool IsActive() const { return difftime(time(nullptr), StartTime) < Duration; }
	int RemainingTime() const { return maximum(0, Duration - static_cast<int>(difftime(time(nullptr), StartTime))); }
};

class CBonusManager
{
	int m_ClientID{};
	CPlayer* m_pPlayer{};
	std::vector<TemporaryBonus> m_vTemporaryBonuses{};

public:
	void Init(int ClientID, CPlayer* pPlayer)
	{
		m_ClientID = ClientID;
		m_pPlayer = pPlayer;
		LoadBonuses();
	}

	const char* GetStringBonusType(int bonusType) const;
	void SendInfoAboutActiveBonuses() const;
	void AddBonus(const TemporaryBonus& bonus);
	void UpdateBonuses();
	void ApplyBonuses(int bonusType, int* pValue, int* pBonusValue = nullptr) const;
	float GetTotalBonusPercentage(int bonusType) const;
	std::pair<int, std::string> GetBonusActivitiesString() const;
	std::vector<TemporaryBonus>& GetTemporaryBonuses() { return m_vTemporaryBonuses; }

private:
	void LoadBonuses();
	void SaveBonuses() const;
};

#endif