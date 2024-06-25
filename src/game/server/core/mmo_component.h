/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_CORE_MMO_COMPONENT_H
#define GAME_SERVER_CORE_MMO_COMPONENT_H

#include <engine/server/sql_string_helpers.h>

// forward declarations
class CGS;
class IServer;
class CMmoController;
class CPlayer;
class CCharacter;
using namespace sqlstr;

class MmoComponent
{
public:
    class CStack
    {
    public:
        ~CStack()
        {
            for(auto* pComponent : m_vComponents)
            {
                delete pComponent;
            }
            m_vComponents.clear();
            m_vComponents.shrink_to_fit();
        }

        void add(MmoComponent* pComponent)
        {
            m_vComponents.push_back(pComponent);
        }

        std::vector< MmoComponent* > m_vComponents;
    };
    virtual ~MmoComponent() = default;

protected:
    friend CMmoController;
    CGS* m_GameServer;
	IServer* m_pServer;
	CMmoController* m_Core;

	CGS* GS() const { return m_GameServer; }
	IServer* Server() const { return m_pServer; }
	CMmoController* Core() const { return m_Core; }

private:
    /**
     * @brief Initializes the MMO component.
     */
    virtual void OnInit() {}

    /**
     * @brief Initializes the MMO component for a specific world.
     * 
     * @param pSqlQueryWhereWorld The location of the local world.
     */
    virtual void OnInitWorld(const char* pSqlQueryWhereWorld) {}

    /**
     * @brief Called when a time period occurs.
     * 
     * @param Period The time period that occurred.
     */
    virtual void OnTimePeriod(ETimePeriod Period) { return; }

    /**
     * @brief Called on each tick of the game.
     */
    virtual void OnTick() {}

    /**
     * @brief Called when a client is reset.
     * 
     * @param ClientID The ID of the client.
     */
    virtual void OnClientReset(int ClientID) {}

    /**
     * @brief Called when a client message is received.
     * 
     * @param MsgID The ID of the message.
     * @param pRawMsg A pointer to the raw message data.
     * @param ClientID The ID of the client.
     * 
     * @return True if the message was handled, false otherwise.
     */
    virtual bool OnClientMessage(int MsgID, void* pRawMsg, int ClientID) { return false; }

    /**
     * @brief Called when a player logs in.
     * 
     * @param pPlayer A pointer to the player object.
     */
    virtual void OnPlayerLogin(CPlayer* pPlayer) {}

    /**
     * @brief Called when a player selects a menu item.
     * 
     * @param pPlayer A pointer to the player object.
     * @param Menulist The selected menu item.
     * 
     * @return True if the menu item was handled, false otherwise.
     */
    virtual bool OnPlayerMenulist(CPlayer* pPlayer, int Menulist) { return false; }

    /**
     * @brief Called when a player issues a vote command.
     * 
     * @param pPlayer A pointer to the player object.
     * @param pCmd The vote command.
     * @param ExtraValue1 The ID of the vote.
     * @param ExtraValue2 The second ID of the vote.
     * @param ReasonNumber The get parameter.
     * @param pReason The get text parameter.
     * 
     * @return True if the vote command was handled, false otherwise.
     */
    virtual bool OnPlayerVoteCommand(CPlayer* pPlayer, const char* pCmd, const int ExtraValue1, const int ExtraValue2, int ReasonNumber, const char* pReason) { return false; }

    /**
     * @brief Called when a player experiences a time period.
     * 
     * @param pPlayer A pointer to the player object.
     * @param Period The time period that occurred.
     */
    virtual void OnPlayerTimePeriod(CPlayer* pPlayer, ETimePeriod Period) { return; }

    /**
     * @brief Called when a character tile is encountered.
     * 
     * @param pChr A pointer to the character object.
     * 
     * @return True if the character tile was handled, false otherwise.
     */
    virtual bool OnCharacterTile(CCharacter* pChr) { return false; }
};

#endif