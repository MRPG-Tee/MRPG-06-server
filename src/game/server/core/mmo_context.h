﻿/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_CORE_MMO_CONTEXT_H
#define GAME_SERVER_CORE_MMO_CONTEXT_H

#include "tools/path_finder_result.h"

// skin data
struct CTeeInfo
{
	char m_aSkinName[64];
	int m_UseCustomColor;
	int m_ColorBody;
	int m_ColorFeet;
};

// special sounds
enum ESpecialSound
{
	SOUND_NOPE = -1,
	SOUND_VOTE_SELECT = SOUND_MENU + 1,
	SOUND_ITEM_EQUIP,
	SOUND_ITEM_SELL_BUY,
	SOUND_USE_POTION,
};


// spawn types
enum ESpawnType
{
	SPAWN_HUMAN = 0,        // Spawn a human player
	SPAWN_BOT = 1,          // Spawn a bot player
	SPAWN_HUMAN_TREATMENT = 2,   // Spawn a human player in a safe location
	SPAWN_HUMAN_PRISON = 3, // Spawn a human prison
	SPAWN_NUM               // The total number of spawn types available
};

// bot types
enum EBotsType
{
	TYPE_BOT_MOB = 1,       // type for mob bots
	TYPE_BOT_QUEST = 2,     // type for quest bots
	TYPE_BOT_NPC = 3,       // type for NPC bots
	TYPE_BOT_FAKE = 4,      // type for fake bots
	TYPE_BOT_EIDOLON = 5,   // type for eidolon bots
	TYPE_BOT_QUEST_MOB = 6, // type for quest mob bots
};

// save types
enum ESaveType
{
	SAVE_ACCOUNT,           // Save Login Password Data
	SAVE_STATS,             // Save Stats Level Exp and other this type
	SAVE_SOCIAL,            // Save Social Data
	SAVE_PROFESSION,	    // Save Profession Data
	SAVE_UPGRADES,          // Save Upgrades Damage and other this type
	SAVE_POSITION,          // Save Position Player
	SAVE_LANGUAGE,          // Save Language Client
	SAVE_TIME_PERIODS,      // Save Time Periods
	SAVE_ACHIEVEMENTS,      // Save Achievements
};

// world day types
enum EDayType
{
	NIGHT_TYPE = 0,
	MORNING_TYPE = 1,
	DAY_TYPE = 2,
	EVENING_TYPE = 3
};

// skills
enum ESkill
{
	SKILL_HEART_TURRET = 1,	// health recovery turret
	SKILL_SLEEPY_GRAVITY = 2, // mobbing
	SKILL_CRAFT_DISCOUNT = 3, // discount on crafting
	SKILL_MASTER_WEAPON = 4, // automatic gunfire
	SKILL_BLESSING_GOD_WAR = 5, // refill ammunition
	SKILL_ATTACK_TELEPORT = 6, // ?knockout? teleport
	SKILL_CURE_I = 7, // health recovery cure
	SKILL_PROVOKE = 8, // provoke
	SKILL_ENERGY_SHIELD = 9, // energy shield
};

// account harvesting stats
enum EAccountHarvestingStats
{
	JOB_LEVEL = 0,
	JOB_EXPERIENCE = 1,
	JOB_UPGRADES = 2,
	NUM_JOB_ACCOUNTS_STATS
};

// player ticks
enum ETickState
{
	Die = 1,
	Respawn,
	LastSelfKill,
	LastEmote,
	LastChangeInfo,
	LastChangeTeam,
	LastChat,
	LastVote,
	LastDialog,
	LastRandomBox,
	HealPotionRecast,
	ManaPotionRecast,
	RefreshClanTitle,
	RefreshNickLeveling,
	NUM_TICK,
};

// time period
enum ETimePeriod
{
	DAILY_STAMP,
	WEEK_STAMP,
	MONTH_STAMP,
	NUM_STAMPS
};

// world types
enum class WorldType : int
{
	Default,
	Dungeon,
	Tutorial,
};

// drawboard events
enum class DrawboardToolEvent : int
{
	OnStart,
	OnPointAdd,
	OnPointErase,
	OnEnd,
};

// laser orbite types
enum class LaserOrbiteType : unsigned char
{
	Default,
	MoveLeft,
	MoveRight,
	InsideOrbite,
	InsideOrbiteRandom,
};

// professions
enum class ProfessionIdentifier : int
{
	None = -1,
	Tank,
	Dps,
	Healer,
	Miner,
	Farmer,
	NUM_PROFESSIONS,
};

constexpr const char* GetProfessionName(ProfessionIdentifier profID) noexcept
{
	switch(profID)
	{
		case ProfessionIdentifier::Tank:   return "Tank";
		case ProfessionIdentifier::Dps:    return "Dps";
		case ProfessionIdentifier::Healer: return "Healer";
		case ProfessionIdentifier::Miner:  return "Miner";
		case ProfessionIdentifier::Farmer: return "Farmer";
		default:                  return "None";
	}
}

// mood type
enum class Mood : short
{
	Normal = 0,
	Angry,
	Agressed,
	Friendly,
	Quest,
	Tank,
};

constexpr const char* GetMoodName(Mood mood) noexcept
{
	switch(mood)
	{
		case Mood::Normal:    return "Normal";
		case Mood::Angry:     return "Angry";
		case Mood::Agressed:  return "Agressed";
		case Mood::Friendly:  return "Friendly";
		case Mood::Quest:     return "Quest";
		case Mood::Tank:      return "Tank";
		default:              return "Unknown";
	}
}

// toplist types
enum class ToplistType : int
{
	GuildLeveling,
	GuildWealthy,
	PlayerRankPoints,
	PlayerWealthy,
	NUM_TOPLIST_TYPES
};

// item types
enum class ItemType : short
{
	Invisible = 0,
	Usable,
	CraftingMaterial,
	Module,
	Other,
	Setting,
	Equipment,
	Decoration,
	Potion,
	NUM_TYPES,
};

constexpr const char* GetTypeItemName(ItemType type) noexcept
{
	switch(type)
	{
		case ItemType::Invisible:        return "Invisible";
		case ItemType::Usable:           return "Usable";
		case ItemType::CraftingMaterial: return "Crafting material";
		case ItemType::Module:           return "Module";
		case ItemType::Other:            return "Other";
		case ItemType::Setting:          return "Setting";
		case ItemType::Equipment:        return "Equipment";
		case ItemType::Decoration:       return "Decoration";
		case ItemType::Potion:           return "Potion";
		default:                         return "Unknown";
	}

}

// item functional
enum ItemFunctional : short
{
	// equipped items
	EquipHammer = 0,
	EquipGun,
	EquipShotgun,
	EquipGrenade,
	EquipLaser,
	EquipPickaxe,
	EquipRake,
	EquipArmor,
	EquipEidolon,
	EquipPotionHeal,
	EquipPotionMana,
	EquipTitle,
	NUM_EQUIPPED,

	// functional categories
	UseSingle = NUM_EQUIPPED,
	UseMultiple,
	Setting,
	ResourceHarvestable,
	ResourceMineable,
	NUM_FUNCTIONS
};

constexpr const char* GetFunctionalItemName(ItemFunctional functional) noexcept
{
	switch(functional)
	{
		case EquipHammer:         return "Hammer";
		case EquipGun:            return "Gun";
		case EquipShotgun:        return "Shotgun";
		case EquipGrenade:        return "Grenade";
		case EquipLaser:          return "Laser";
		case EquipPickaxe:        return "Pickaxe";
		case EquipRake:           return "Rake";
		case EquipArmor:          return "Armor";
		case EquipEidolon:        return "Eidolon";
		case EquipPotionHeal:     return "Potion Heal";
		case EquipPotionMana:     return "Potion Mana";
		case EquipTitle:          return "Title";
		case UseSingle:           return "Use Single";
		case UseMultiple:         return "Use Multiple";
		case Setting:             return "Setting";
		case ResourceHarvestable: return "Resource Harvestable";
		case ResourceMineable:    return "Resource Mineable";
		default:                  return "Unknown";
	}

}

enum class QuestState : int
{
	NoAccepted = 0,
	Accepted,
	Finished,
};

constexpr const char* GetQustStateName(QuestState state) noexcept
{
	switch(state)
	{
		case QuestState::NoAccepted: return "No accepted";
		case QuestState::Accepted:   return "Accepted";
		case QuestState::Finished:   return "Finished";
		default:                     return "Unknown";
	}
}

// npc functions
enum EFunctionsNPC
{
	FUNCTION_NPC_NURSE,
	FUNCTION_NPC_GIVE_QUEST,
	FUNCTION_NPC_GUARDIAN,
};

// menu list
enum EMenuList
{
	// Main menu
	MENU_MAIN = 1,
	MENU_ACCOUNT_INFO,
	MENU_EQUIPMENT,
	MENU_INVENTORY,
	MENU_UPGRADES,
	MENU_MODULES,

	// Settings
	MENU_SETTINGS,
	MENU_SETTINGS_ACCOUNT,
	MENU_SETTINGS_LANGUAGE,
	MENU_SETTINGS_TITLE,

	// Mailbox
	MENU_MAILBOX,
	MENU_MAILBOX_SELECT,

	// Guild-related
	MENU_GUILD,
	MENU_GUILD_DISBAND,
	MENU_GUILD_UPGRADES,
	MENU_GUILD_MEMBER_LIST,
	MENU_GUILD_MEMBER_SELECT,
	MENU_GUILD_RANK_LIST,
	MENU_GUILD_RANK_SELECT,
	MENU_GUILD_INVITATIONS,
	MENU_GUILD_LOGS,
	MENU_GUILD_WARS,
	MENU_GUILD_FINDER,
	MENU_GUILD_FINDER_SELECT,

	MENU_GUILD_SELL_HOUSE,
	MENU_GUILD_BUY_HOUSE,
	MENU_GUILD_HOUSE_FARMZONE_LIST,
	MENU_GUILD_HOUSE_FARMZONE_SELECT,
	MENU_GUILD_HOUSE_DOOR_LIST,

	// Achievements-related
	MENU_ACHIEVEMENTS,
	MENU_ACHIEVEMENTS_SELECT,

	// House-related
	MENU_HOUSE,
	MENU_HOUSE_SELL,
	MENU_HOUSE_DOOR_LIST,
	MENU_HOUSE_DOOR_ACCESS,
	MENU_HOUSE_BUY,
	MENU_HOUSE_FARMZONE_LIST,
	MENU_HOUSE_FARMZONE_SELECT,

	// Warehouse
	MENU_WAREHOUSE,
	MENU_WAREHOUSE_ITEM_SELECT,

	// Eidolon Collection menus
	MENU_EIDOLON,
	MENU_EIDOLON_SELECT,

	// Journal menus
	MENU_JOURNAL_MAIN,
	MENU_JOURNAL_QUEST_DETAILS,

	// Skills
	MENU_SKILL_LIST,
	MENU_SKILL_SELECT,

	// Quest menus
	MENU_BOARD,
	MENU_BOARD_QUEST_SELECT,

	// Aethernet menus
	MENU_AETHERNET_LIST,

	// Auction
	MENU_AUCTION_LIST,
	MENU_AUCTION_SLOT_SELECT,
	MENU_AUCTION_CREATE_SLOT,

	// Group menus
	MENU_GROUP,

	// Grinding
	MENU_GUIDE,
	MENU_GUIDE_SELECT,

	// Crafting
	MENU_CRAFTING_LIST,
	MENU_CRAFTING_SELECT,

	// Other menus
	MENU_LEADERBOARD,
	MENU_DUNGEONS,

	// motd menus
	MOTD_MENU_BANK_MANAGER,
	MOTD_MENU_BONUSES_INFO,
	MOTD_MENU_WANTED_INFO,

	MOTD_MENU_WIKI_INFO,
	MOTD_MENU_WIKI_SELECT,
};

// player scenarios
enum
{
	SCENARIO_UNIVERSAL = 1,
	SCENARIO_TUTORIAL,
	SCENARIO_EIDOLON,
};

// primary
enum
{
	/*
		Basic kernel server settings
		This is where the most basic server settings are stored
	*/
	MAX_GROUP_MEMBERS = 4,					// maximum number of players in a group
	MAX_HOUSE_DOOR_INVITED_PLAYERS = 3,		// maximum player what can have access for house door
	MAX_DECORATIONS_PER_HOUSE = 20,			// maximum decorations for houses
	MAIL_MAX_CAPACITY = 10,					// maximum number of emails what is displayed
	MAX_ATTRIBUTES_FOR_ITEM = 2,			// maximum number of stats per item
	POTION_RECAST_APPEND_TIME = 15,			// recast append time for potion in seconds
	DEFAULT_MAX_PLAYER_BAG_GOLD = 5000,		// player gold limit
	MIN_SKINCHANGE_CLIENTVERSION = 0x0703,	// minimum client version for skin change
	MIN_RACE_CLIENTVERSION = 0x0704,		// minimum client version for race type
	MAX_DROPPED_FROM_MOBS = 5,              // maximum number of items dropped from mobs

	// items
	NOPE = -1,
	itCapsuleSurvivalExperience = 16,	// Gives 10-50 experience
	itLittleBagGold = 17,				// Gives 10-50 gold
	itTicketResetClassStats = 21,		// Ticket to reset the statistics of class upgrades
	itTicketResetWeaponStats = 23,		// Ticket to reset the statistics cartridge upgrade
	itTicketDiscountCraft = 24,			// Discount ticket for crafting
	itRandomHomeDecoration = 26,		// Random home decor
	itRandomRelicsBox = 58,				// Random Relics box

	// potions
	itPotionManaRegen = 14,				// Mana regeneration potion
	itTinyHealthPotion = 15,			// Tiny health potion

	// eidolons
	itEidolonOtohime = 57,				// Eidolon
	itEidolonMerrilee = 59,				// Eidolon
	itEidolonDryad = 80,				// Eidolon
	itEidolonPigQueen = 88,				// Eidolon

	// currency
	itGold = 1,							// Money ordinary currency
	itMaterial = 7,						// Scraping material
	itProduct = 8,						// Scraping material
	itSkillPoint = 9,					// Skillpoint
	itAchievementPoint = 10,			// Achievement point
	itAlliedSeals = 30,					// Allied seals
	itTicketGuild = 95,					// Ticket for the creation of the guild

	// modules
	itExplosiveGun = 19,				// Explosion for gun
	itExplosiveShotgun = 20,			// Explosion for shotgun
	itPoisonHook = 64,					// Poison hook
	itExplodeHook = 65,					// Explode hook
	itSpiderHook = 66,					// Spider hook
	itCustomizer = 96,                  // Curomizer for personal skins
	itDamageEqualizer = 97,				// Module for dissable self dmg

	// weapons
	itHammer = 2,						// Equipment Hammers
	itHammerLamp = 99,					// Equipment Lamp hammer
	itHammerBlast = 102,				// Equipment Blast hammer

	itGun = 3,							// Equipment Gun

	itShotgun = 4,						// Equipment Shotgun

	itGrenade = 5,						// Equipment Grenade
	itPizdamet = 100,					// Equipment Pizdamet

	itLaser = 6,						// Equipment Rifle
	itRifleWallPusher = 101,			// Equipment Rifle Plazma wall
	itRifleMagneticPulse = 103,			// Equpment Magnetic pulse rifle

	// decoration items
	itPickupHealth = 18,				// Pickup heart
	itPickupMana = 13,					// Pickup mana
	itPickupShotgun = 11,				// Pickup shotgun
	itPickupGrenade = 12,				// Pickup grenade
	itPickupLaser = 94,				    // Pickup laser

	// settings items
	itShowEquipmentDescription = 25,	// Description setting
	itShowCriticalDamage = 34,			// Critical damage setting
	itShowQuestStarNavigator = 93,		// Show quest path when idle
	itShowDetailGainMessages = 98,      // Show detail gain messages
};

// broadcast
enum class BroadcastPriority
{
	Lower,
	GameBasicStats,
	GameInformation,
	GamePriority,
	GameWarning,
	MainInformation,
	TitleInformation,
	VeryImportant,
};

// attribute groups
enum class AttributeGroup : int
{
	Tank,
	Healer,
	Dps,
	Weapon,
	Hardtype,
	Job,
	Other,
};

// Attribute context
enum class AttributeIdentifier : int
{
	DMG = 1,                     // Attribute identifier for damage
	AttackSPD = 2,               // Attribute identifier for attack speed
	CritDMG = 3,                 // Attribute identifier for critical damage
	Crit = 4,                    // Attribute identifier for critical chance
	HP = 5,                      // Attribute identifier for health points
	Lucky = 6,                   // Attribute identifier for luck
	MP = 7,                      // Attribute identifier for mana points
	Vampirism = 8,               // Attribute identifier for vampirism
	AmmoRegen = 9,               // Attribute identifier for ammo regeneration
	Ammo = 10,                   // Attribute identifier for ammo
	Efficiency = 11,             // Attribute identifier for efficiency
	Extraction = 12,             // Attribute identifier for extraction
	HammerDMG = 13,              // Attribute identifier for hammer damage
	GunDMG = 14,                 // Attribute identifier for gun damage
	ShotgunDMG = 15,             // Attribute identifier for shotgun damage
	GrenadeDMG = 16,             // Attribute identifier for grenade damage
	RifleDMG = 17,               // Attribute identifier for rifle damage
	LuckyDropItem = 18,          // Attribute identifier for lucky drop item
	EidolonPWR = 19,             // Attribute identifier for eidolon power
	GoldCapacity = 20,           // Attribute identifier for gold capacity
	ATTRIBUTES_NUM,              // The number of total attributes
};


/*
 * Enum default naming conventions
 */
constexpr const char* GetEmoteNameById(int emoteId) noexcept
{
	switch(emoteId)
	{
		case EMOTE_PAIN:         return "Pain";
		case EMOTE_HAPPY:        return "Happy";
		case EMOTE_SURPRISE:     return "Surprise";
		case EMOTE_ANGRY:        return "Angry";
		case EMOTE_BLINK:        return "Blink";
		default:                 return "Normal";
	}
}

constexpr const char* GetEmoticonNameById(int emoticonId) noexcept
{
	switch(emoticonId)
	{
		case EMOTICON_OOP:          return "Emoticon Ooop";
		case EMOTICON_EXCLAMATION:  return "Emoticon Exclamation";
		case EMOTICON_HEARTS:       return "Emoticon Hearts";
		case EMOTICON_DROP:         return "Emoticon Drop";
		case EMOTICON_DOTDOT:       return "Emoticon ...";
		case EMOTICON_MUSIC:        return "Emoticon Music";
		case EMOTICON_SORRY:        return "Emoticon Sorry";
		case EMOTICON_GHOST:        return "Emoticon Ghost";
		case EMOTICON_SUSHI:        return "Emoticon Sushi";
		case EMOTICON_SPLATTEE:     return "Emoticon Splatee";
		case EMOTICON_DEVILTEE:     return "Emoticon Deviltee";
		case EMOTICON_ZOMG:         return "Emoticon Zomg";
		case EMOTICON_ZZZ:          return "Emoticon Zzz";
		case EMOTICON_WTF:          return "Emoticon Wtf";
		case EMOTICON_EYES:         return "Emoticon Eyes";
		case EMOTICON_QUESTION:     return "Emoticon Question";
		default:                    return "Not selected";
	}
}

/*
 * MultiworldIdentifiableData
 */
class IServer;
namespace detail
{
	class _MultiworldIdentifiableData
	{
		inline static IServer* m_pServer {};

	public:
		IServer* Server() const { return m_pServer; }
		static void Init(IServer* pServer) { m_pServer = pServer; }
	};
}

template < typename T >
class MultiworldIdentifiableData : public detail::_MultiworldIdentifiableData
{
protected:
	static inline T m_pData {};

public:
	static T& Data() { return m_pData; }
};

#endif
