/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_COMPONENT_GUILD_MEMBERS_CONTROLLER_H
#define GAME_SERVER_COMPONENT_GUILD_MEMBERS_CONTROLLER_H

#include "GuildMemberData.h"

class CGuildData;
using CGuildMembersContainer = std::map<int, CGuildMemberData*>;

class CGuildMembersController
{
	CGS* GS() const;

	CGuildData* m_pGuild {};
	CGuildMembersContainer m_apMembers {};

public:
	CGuildMembersController(CGuildData* pGuild, std::string&& MembersData);
	~CGuildMembersController();

	CGuildMemberData* GetMember(int AccountID);
	CGuildMembersContainer& GetContainer() { return m_apMembers; }

	GUILD_MEMBER_RESULT Join(int AccountID);
	GUILD_MEMBER_RESULT Kick(int AccountID);
	void Save() const;

private:
	void Init(std::string&& MembersData);

};

#endif
