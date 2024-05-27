﻿/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_CORE_UTILITIES_VOTE_WRAPPER_H
#define GAME_SERVER_CORE_UTILITIES_VOTE_WRAPPER_H

#include <game/voting.h>
#include "format.h"

class CGS;
class CPlayer;
class CVoteGroupHidden;

typedef void (*VoteOptionCallbackImpl)(CPlayer*, int, std::string, void*);
typedef struct { VoteOptionCallbackImpl m_Impl; void* m_pData; } VoteOptionCallback;

enum
{
	// depth list
	DEPTH_LVL1 = 0,
	DEPTH_LVL2 = 1,
	DEPTH_LVL3 = 2,
	DEPTH_LVL4 = 3,
	DEPTH_LVL5 = 4,
	DEPTH_LIST_STYLE_ROMAN = 1 << 6,
	DEPTH_LIST_STYLE_BOLD = 1 << 7,
	DEPTH_LIST_STYLE_CYRCLE = 1 << 8,

	// disabled
	VWF_DISABLED                   = 0, // regular title group

	// settings
	VWF_SEPARATE                       = 1 << 1, // ends the group with a line
	VWF_GROUP_NUMERAL              = 1 << 3, // numbers the title page
	VWF_ALIGN_TITLE                = 1 << 4, // example: ---  title  ---

	// styles
	VWF_STYLE_SIMPLE               = 1 << 5, // example: ╭ │ ╰
	VWF_STYLE_DOUBLE               = 1 << 6, // example: ╔ ═ ╚
	VWF_STYLE_STRICT               = 1 << 7, // example: ┌ │ └
	VWF_STYLE_STRICT_BOLD          = 1 << 8, // example: ┏ ┃ ┗

	// hidden
	VWF_OPEN                       = 1 << 9, // default open group
	VWF_CLOSED                     = 1 << 10, // default close group
	VWF_UNIQUE                     = 1 << 11, // default close group toggle unique groups

	// defined
	VWF_SEPARATE_OPEN                  = VWF_OPEN | VWF_SEPARATE, // default open group with line
	VWF_SEPARATE_CLOSED                = VWF_CLOSED | VWF_SEPARATE, // default close group with line
	VWF_SEPARATE_UNIQUE                = VWF_UNIQUE | VWF_SEPARATE, // default close group with line
};

class CVoteOption
{
public:
	char m_aDescription[VOTE_DESC_LENGTH] {};
	char m_aCommand[VOTE_CMD_LENGTH] {};
	int m_Depth {};
	int m_Extra1 { -1 };
	int m_Extra2 { -1 };
	bool m_Line { false };
	bool m_IsTitle { false };
	VoteOptionCallback m_Callback {};
};

class CVoteGroup
{
	friend class VoteWrapper;

	struct NumeralDepth
	{
		int m_Value {};
		int m_Style {};
	};
	std::map<int, NumeralDepth> m_vDepthNumeral {};
	std::deque<CVoteOption> m_vpVotelist {};
	bool m_NextMarkedListItem {};
	int m_CurrentDepth {};

	CGS* m_pGS {};
	CPlayer* m_pPlayer {};
	CGS* GS() const { return m_pGS; }

	bool m_TitleIsSet {};
	int m_GroupSize {};
	int m_HiddenID {};
	int m_Flags {};
	int m_ClientID {};

	CVoteGroup(int ClientID, int Flags);

	void SetNumeralDepthStyles(std::initializer_list<std::pair<int, int>> vNumeralFlags);

	int NextPos() const { return m_GroupSize + 1; }
	bool IsEmpty() const { return m_GroupSize <= 0; }
	bool IsTitleSet() const { return m_TitleIsSet; }
	bool IsHidden() const;

	void SetVoteTitleImpl(const char* pCmd, int Extra1, int Extra2, const char* pText);
	void AddVoteImpl(const char* pCmd, int Extra1, int Extra2, const char* pText);
	void SetLastVoteCallback(const VoteOptionCallbackImpl& CallbackImpl, void* pUser) { m_vpVotelist.back().m_Callback = { CallbackImpl, pUser }; }

	void Reformatting(dynamic_string& Buffer);

	void AddLineImpl();
	void AddEmptylineImpl();
	void AddBackpageImpl();
	void AddItemValueImpl(int ItemID);

};

#define FMT_LOCALIZE_STR(clientid, text, args) Tools::String::FormatLocalize(clientid, text, args).c_str()
class VoteWrapper : public MultiworldIdentifiableStaticData<std::map<int, std::deque<CVoteGroup*>>>
{
	CVoteGroup* m_pGroup {};

public:
	/* ====================================================
	 *  Constructors (default)
	 * ==================================================== */
	VoteWrapper(int ClientID)
	{
		dbg_assert(ClientID >= 0 && ClientID < MAX_CLIENTS, "Invalid ClientID");
		m_pGroup = new CVoteGroup(ClientID, VWF_DISABLED);
		m_pData[ClientID].push_back(m_pGroup);
	}
	template <typename T = int>
	VoteWrapper(int ClientID, T Flags)
	{
		dbg_assert(ClientID >= 0 && ClientID < MAX_CLIENTS, "Invalid ClientID");
		m_pGroup = new CVoteGroup(ClientID, Flags);
		m_pData[ClientID].push_back(m_pGroup);
	}

	/* ====================================================
	 *  Constructors (title)
	 * ==================================================== */
	template<typename ... Ts>
	VoteWrapper(int ClientID, const char* pTitle, const Ts&... args)
	{
		dbg_assert(ClientID >= 0 && ClientID < MAX_CLIENTS, "Invalid ClientID");

		m_pGroup = new CVoteGroup(ClientID, VWF_DISABLED);
		m_pGroup->SetVoteTitleImpl("null", NOPE, NOPE, FMT_LOCALIZE_STR(ClientID, pTitle, args...));
		m_pData[ClientID].push_back(m_pGroup);
	}

	/* ====================================================
	 *  Constructors (title, flags)
	 * ==================================================== */
	template<typename ... Ts>
	VoteWrapper(int ClientID, int Flags, const char* pTitle, const Ts&... args)
	{
		dbg_assert(ClientID >= 0 && ClientID < MAX_CLIENTS, "Invalid ClientID");

		m_pGroup = new CVoteGroup(ClientID, Flags);
		m_pGroup->SetVoteTitleImpl("null", NOPE, NOPE, FMT_LOCALIZE_STR(ClientID, pTitle, args...));
		m_pData[ClientID].push_back(m_pGroup);
	}

	/* ====================================================
	 *  SetTitle (title)
	 * ==================================================== */
	template<typename ... Ts>
	VoteWrapper& SetTitle(const char* pTitle, const Ts&... args) noexcept
	{
		m_pGroup->SetVoteTitleImpl("null", NOPE, NOPE, FMT_LOCALIZE_STR(m_pGroup->m_ClientID, pTitle, args...));
		return *this;
	}

	/* ====================================================
	 *  SetTitle (title, flags)
	 * ==================================================== */
	template<typename ... Ts>
	VoteWrapper& SetTitle(int Flags, const char* pTitle, const Ts&... args) noexcept
	{
		m_pGroup->m_Flags = Flags;
		m_pGroup->SetVoteTitleImpl("null", NOPE, NOPE, FMT_LOCALIZE_STR(m_pGroup->m_ClientID, pTitle, args...));
		return *this;
	}

	/* ====================================================
	 *  Group leveling
	 * ==================================================== */
	void ReinitNumeralDepthStyles(std::initializer_list<std::pair<int, int>> vNumeralFlags) const
	{
		dbg_assert(m_pGroup != nullptr, "For initilize depth, first needed initialize vote wrapper");
		m_pGroup->SetNumeralDepthStyles(std::move(vNumeralFlags));
	}

	VoteWrapper& MarkList() noexcept
	{
		m_pGroup->m_NextMarkedListItem = true;
		return *this;
	}

	VoteWrapper& BeginDepth() noexcept
	{
		m_pGroup->m_CurrentDepth++;
		return *this;
	}

	VoteWrapper& EndDepth() noexcept
	{
		m_pGroup->m_CurrentDepth--;
		return *this;
	}

	/* ====================================================
	 *  Tools
	 * ==================================================== */
	VoteWrapper& AddIfLine(bool Check) noexcept
	{
		if(CheckerAddVoteImpl(Check))
			m_pGroup->AddLineImpl();
		return *this;
	}

	VoteWrapper& AddLine()
	{
		return AddIfLine(true);
	}

	VoteWrapper& AddIfEmptyline(bool Check) noexcept
	{
		if(CheckerAddVoteImpl(Check))
			m_pGroup->AddEmptylineImpl();
		return *this;
	}

	VoteWrapper& AddEmptyline()
	{
		return AddIfEmptyline(true);
	}

	VoteWrapper& AddItemValue(int ItemID) noexcept
	{
		m_pGroup->AddItemValueImpl(ItemID);
		return *this;
	}

	int NextPos() const { return m_pGroup->NextPos(); }
	bool IsEmpty() const { return m_pGroup->IsEmpty(); }
	bool IsTittleSet() const { return m_pGroup->IsTitleSet(); }

	/* ====================================================
	 *  AddIf (text)
	 * ==================================================== */
	template<typename ... Ts>
	VoteWrapper& AddIf(bool Check, const char* pText, const Ts&... args)
	{
		if(CheckerAddVoteImpl(Check))
			m_pGroup->AddVoteImpl("null", NOPE, NOPE, FMT_LOCALIZE_STR(m_pGroup->m_ClientID, pText, args...));
		return *this;
	}

	/* ====================================================
	 *  Add (text)
	 * ==================================================== */
	template<typename ... Ts>
	VoteWrapper& Add(const char* pText, const Ts&... args)
	{
		return AddIf(true, pText, args...);
	}

	/* ====================================================
	 *  AddIfMenu (menuID)
	 * ==================================================== */
	template<typename ... Ts>
	VoteWrapper& AddIfMenu(bool Check, int MenuID, const char* pText, const Ts&... args)
	{
		if(CheckerAddVoteImpl(Check))
			m_pGroup->AddVoteImpl("MENU", MenuID, NOPE, FMT_LOCALIZE_STR(m_pGroup->m_ClientID, pText, args...));
		return *this;
	}

	/* ====================================================
	 *  AddMenu (menuID)
	 * ==================================================== */
	template<typename ... Ts>
	VoteWrapper& AddMenu(int MenuID, const char* pText, const Ts&... args)
	{
		return AddIfMenu(true, MenuID, pText, args...);
	}

	/* ====================================================
	 *  AddIfMenu (menuID, groupInteractID)
	 * ==================================================== */
	template<typename ... Ts>
	VoteWrapper& AddIfMenu(bool Check, int MenuID, int GroupID, const char* pText, const Ts&... args)
	{
		if(CheckerAddVoteImpl(Check))
			m_pGroup->AddVoteImpl("MENU", MenuID, GroupID, FMT_LOCALIZE_STR(m_pGroup->m_ClientID, pText, args...));
		return *this;
	}

	/* ====================================================
	 *  AddMenu (menuID, groupInteractID)
	 * ==================================================== */
	template<typename ... Ts>
	VoteWrapper& AddMenu(int MenuID, int GroupID, const char* pText, const Ts&... args)
	{
		return AddIfMenu(true, MenuID, GroupID, pText, args...);
	}

	/* ====================================================
	 *  AddIfOption (Cmd)
	 * ==================================================== */
	template<typename ... Ts>
	VoteWrapper& AddIfOption(bool Check, const char* pCmd, const char* pText, const Ts&... args)
	{
		if(CheckerAddVoteImpl(Check))
			m_pGroup->AddVoteImpl(pCmd, NOPE, NOPE, FMT_LOCALIZE_STR(m_pGroup->m_ClientID, pText, args...));
		return *this;
	}

	/* ====================================================
	 *  AddOption (Cmd)
	 * ==================================================== */
	template<typename ... Ts>
	VoteWrapper& AddOption(const char* pCmd, const char* pText, const Ts&... args)
	{
		return AddIfOption(true, pCmd, pText, args...);
	}


	/* ====================================================
	 *  AddIfOption (Cmd, Extra)
	 * ==================================================== */
	template<typename ... Ts>
	VoteWrapper& AddIfOption(bool Check, const char* pCmd, int Extra, const char* pText, const Ts&... args)
	{
		if(CheckerAddVoteImpl(Check))
			m_pGroup->AddVoteImpl(pCmd, Extra, NOPE, FMT_LOCALIZE_STR(m_pGroup->m_ClientID, pText, args...));
		return *this;
	}

	/* ====================================================
	 *  AddOption (Cmd, Extra)
	 * ==================================================== */
	template<typename ... Ts>
	VoteWrapper& AddOption(const char* pCmd, int Extra, const char* pText, const Ts&... args)
	{
		return AddIfOption(true, pCmd, Extra, pText, args...);
	}

	/* ====================================================
	 *  AddIfOption (Cmd, Extra1, Extra2)
	 * ==================================================== */
	template<typename ... Ts>
	VoteWrapper& AddIfOption(bool Check, const char* pCmd, int Extra1, int Extra2, const char* pText, const Ts&... args)
	{
		if(CheckerAddVoteImpl(Check))
			m_pGroup->AddVoteImpl(pCmd, Extra1, Extra2, FMT_LOCALIZE_STR(m_pGroup->m_ClientID, pText, args...));
		return *this;
	}

	/* ====================================================
	 *  AddOption (Cmd, Extra1, Extra2)
	 * ==================================================== */
	template<typename ... Ts>
	VoteWrapper& AddOption(const char* pCmd, int Extra1, int Extra2, const char* pText, const Ts&... args)
	{
		return AddIfOption(true, pCmd, Extra1, Extra2, pText, args...);
	}

	/* ====================================================
	 *  AddOptionCallback (Callback)
	 * ==================================================== */
	template<typename ... Ts>
	VoteWrapper& AddOptionCallback(void* pUser, const VoteOptionCallbackImpl& CallbackImpl, const char* pText, const Ts&... args)
	{
		m_pGroup->AddVoteImpl("CALLBACK_IMPL", NOPE, NOPE, FMT_LOCALIZE_STR(m_pGroup->m_ClientID, pText, args...));
		m_pGroup->SetLastVoteCallback(CallbackImpl, pUser);
		return *this;
	}

	/* ====================================================
	 *  AddOptionCallback (Callback, Extra1)
	 * ==================================================== */
	template<typename ... Ts>
	VoteWrapper& AddOptionCallback(void* pUser, const VoteOptionCallbackImpl& CallbackImpl, int Extra, const char* pText, const Ts&... args)
	{
		m_pGroup->AddVoteImpl("CALLBACK_IMPL", Extra, NOPE, FMT_LOCALIZE_STR(m_pGroup->m_ClientID, pText, args...));
		m_pGroup->SetLastVoteCallback(CallbackImpl, pUser);
		return *this;
	}

	/* ====================================================
	 *  AddOptionCallback (Callback, Extra1, Extra2)
	 * ==================================================== */
	template<typename ... Ts>
	VoteWrapper& AddOptionCallback(void* pUser, const VoteOptionCallbackImpl& CallbackImpl, int Extra1, int Extra2, const char* pText, const Ts&... args)
	{
		m_pGroup->AddVoteImpl("CALLBACK_IMPL", Extra1, Extra2, FMT_LOCALIZE_STR(m_pGroup->m_ClientID, pText, args...));
		m_pGroup->SetLastVoteCallback(CallbackImpl, pUser);
		return *this;
	}
	
	/* ====================================================
	 *  Global functions
	 * ==================================================== */
	static void AddLine(int ClientID) noexcept
	{
		const auto pVoteGroup = new CVoteGroup(ClientID, VWF_DISABLED);
		pVoteGroup->AddLineImpl();
		m_pData[ClientID].push_back(pVoteGroup);
	}
	static void AddBackpage(int ClientID) noexcept
	{
		const auto pVoteGroup = new CVoteGroup(ClientID, VWF_DISABLED);
		pVoteGroup->AddBackpageImpl();
		m_pData[ClientID].push_back(pVoteGroup);
	}
	static void AddEmptyline(int ClientID) noexcept
	{
		const auto pVoteGroup = new CVoteGroup(ClientID, VWF_DISABLED);
		pVoteGroup->AddEmptylineImpl();
		m_pData[ClientID].push_back(pVoteGroup);
	}
	static void AddItemValue(int ClientID, int ItemID) noexcept
	{
		const auto pVoteGroup = new CVoteGroup(ClientID, VWF_DISABLED);
		pVoteGroup->AddItemValueImpl(ItemID);
		m_pData[ClientID].push_back(pVoteGroup);
	}
	static void RebuildVotes(int ClientID);
	static CVoteOption* GetOptionVoteByAction(int ClientID, const char* pActionName);

private:
	template <typename ... Args>
	bool CheckerAddVoteImpl(bool Checker) const
	{
		if(!Checker)
			m_pGroup->m_NextMarkedListItem = false;
		return Checker;
	}
};
#undef FMT_LOCALIZE_STR

class CVotePlayerData
{
	friend class CVoteGroup;
	friend class VoteWrapper;

	struct VoteGroupHidden
	{
		bool m_Value {};
		int m_Flag {};
	};

	CGS* m_pGS {};
	CPlayer* m_pPlayer {};
	int m_LastMenuID{};
	int m_CurrentMenuID { };
	int m_GroupID {};
	std::thread m_VoteUpdater {};
	enum class STATE_UPDATER { WAITING, RUNNING, DONE };
	std::atomic<STATE_UPDATER> m_VoteUpdaterStatus{ STATE_UPDATER::WAITING };
	ska::unordered_map<int, ska::unordered_map<int, VoteGroupHidden>> m_aHiddenGroup{};

	VoteGroupHidden* EmplaceHidden(int ID, int Type);
	VoteGroupHidden* GetHidden(int ID);
	void ResetHidden(int MenuID);
	void ResetHidden() { ResetHidden(m_CurrentMenuID); }
	static void ThreadVoteUpdater(CVotePlayerData* pData);

public:
	CVotePlayerData()
	{
		m_CurrentMenuID = MENU_MAIN;
	}

	~CVotePlayerData()
	{
		if(m_VoteUpdater.joinable())
			m_VoteUpdater.join();

		ClearVotes();
		m_pGS = nullptr;
		m_pPlayer = nullptr;
		m_aHiddenGroup.clear();
	}

	void Initilize(CGS* pGS, CPlayer* pPlayer)
	{
		m_pGS = pGS;
		m_pPlayer = pPlayer;
	}

	void ApplyVoteUpdaterData();
	void UpdateVotes(int MenuID);
	void UpdateVotesIf(int MenuID);
	void UpdateCurrentVotes() { UpdateVotes(m_CurrentMenuID); }
	void ClearVotes() const;

	void SetCurrentMenuID(int MenuID) { m_CurrentMenuID = MenuID; }
	int GetCurrentMenuID() const { return m_CurrentMenuID; }
	int GetGroupID() const { return m_GroupID; }

	void SetLastMenuID(int MenuID) { m_LastMenuID = MenuID; }
	int GetLastMenuID() const { return m_LastMenuID; }

	bool DefaultVoteCommands(const char* pCmd, int Extra1, int Extra2, int ReasonNumber, const char* pReason);
};

#endif
