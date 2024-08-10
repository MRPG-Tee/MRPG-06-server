﻿#include "motd_menu.h"

#include <game/server/event_key_manager.h>
#include <game/server/gamecontext.h>

void MotdMenu::AddImpl(int extra, std::string_view command, const std::string& description)
{
	Point p;
	str_copy(p.m_aDesc, description.c_str(), sizeof(p.m_aDesc));
	str_utf8_fix_truncation(p.m_aDesc);
	p.m_Command = command;
	p.m_Extra = extra;
	m_Points.push_back(p);
	m_ScrollManager.SetMaxScrollPos(static_cast<int>(m_Points.size()));
}

void MotdMenu::Handle()
{
	// check is has points
	if(m_Points.empty())
		return;

	const auto pServer = Instance::Server();
	const auto pGS = (CGS*)Instance::GameServerPlayer(m_ClientID);
	const auto pChar = pGS->GetPlayerChar(m_ClientID);

	// check valid character
	if(!pChar)
		return;

	// initialize variables
	constexpr int lineSizeY = 20;
	constexpr int startLineY = -280;
	const int targetX = pChar->m_Core.m_Input.m_TargetX;
	const int targetY = pChar->m_Core.m_Input.m_TargetY;

	// handle scrolling with key events
	if(CEventKeyManager::IsKeyClicked(m_ClientID, KEY_EVENT_NEXT_WEAPON))
	{
		m_ScrollManager.ScrollUp();
		m_ResendMotdTick = pServer->Tick() + 5;
	}
	else if(CEventKeyManager::IsKeyClicked(m_ClientID, KEY_EVENT_PREV_WEAPON))
	{
		m_ScrollManager.ScrollDown();
		m_ResendMotdTick = pServer->Tick() + 5;
	}

	// prepare the buffer for the MOTD
	std::string buffer;
	int linePos = 0;

	if(m_ScrollManager.CanScrollUp())
	{
		buffer += "△ Scroll up △\n";
		linePos++;
	}

	auto addLineToBuffer = [&](int index, const std::string& line, bool isSelected)
	{
		buffer += (isSelected ? "✘ " : "➜ ") + std::to_string(index + 1) + ". " + line + "\n";
	};

	// add menu items to buffer
	for(int i = m_ScrollManager.GetScrollPos(); i < m_ScrollManager.GetEndScrollPos() && i < static_cast<int>(m_Points.size()); ++i, ++linePos)
	{
		const int checkYStart = startLineY + linePos * lineSizeY;
		const int checkYEnd = startLineY + (linePos + 1) * lineSizeY;
		const bool isSelected = (targetX > -196 && targetX < 196 && targetY >= checkYStart && targetY < checkYEnd);

		if(isSelected && CEventKeyManager::IsKeyClicked(m_ClientID, KEY_EVENT_FIRE))
		{
			const auto& command = m_Points[i].m_Command;
			const auto& extra = m_Points[i].m_Extra;

			if(pGS->OnClientMotdCommand(m_ClientID, command.c_str(), extra))
			{
				if(m_Flags & MTFLAG_CLOSE_LAST_MENU_ON_SELECT)
				{
					if(m_LastMenulist != NOPE)
					{
						pGS->SendMenuMotd(pChar->GetPlayer(), m_LastMenulist);
					}
					else
					{
						ClearMotd(pGS, pChar->GetPlayer());
					}
				}
				else if(m_Flags & MTFLAG_CLOSE_ON_SELECT)
				{
					ClearMotd(pGS, pChar->GetPlayer());
				}
				else
				{
					ScrollManager oldScrollData = m_ScrollManager;
					pGS->SendMenuMotd(pChar->GetPlayer(), m_Menulist);
					oldScrollData.SetMaxScrollPos((int)pChar->GetPlayer()->m_pMotdMenu->m_Points.size());
					pChar->GetPlayer()->m_pMotdMenu->m_ScrollManager = oldScrollData;
				}
				return;
			}
		}

		addLineToBuffer(i, m_Points[i].m_aDesc, isSelected);
	}

	// add close button if necessary
	if(m_Flags & MTFLAG_CLOSE_BUTTON)
	{
		const int closeIndex = static_cast<int>(m_Points.size());
		const int checkYStart = startLineY + closeIndex * lineSizeY;
		const int checkYEnd = startLineY + (closeIndex + 1) * lineSizeY;
		const bool isCloseSelected = (targetX > -196 && targetX < 196 && targetY >= checkYStart && targetY < checkYEnd);

		if(isCloseSelected && CEventKeyManager::IsKeyClicked(m_ClientID, KEY_EVENT_FIRE))
		{
			ClearMotd(pGS, pChar->GetPlayer());
			return;
		}

		addLineToBuffer(closeIndex, "Close", isCloseSelected);
	}

	if(m_ScrollManager.CanScrollDown())
	{
		buffer += "▽ Scroll down ▽\n";
	}

	buffer += "\n" + m_Description;

	// update buffer if it has changed
	if(m_LastBuffer != buffer)
	{
		m_LastBuffer = buffer;
		m_ResendMotdTick = pServer->Tick() + pServer->TickSpeed();
		pGS->SendMotd(m_ClientID, buffer.c_str());
	}
	else if(pServer->Tick() >= m_ResendMotdTick)
	{
		ScrollManager oldScrollData = m_ScrollManager;
		m_ResendMotdTick = pServer->Tick() + pServer->TickSpeed();
		pGS->SendMenuMotd(pChar->GetPlayer(), m_Menulist);
		oldScrollData.SetMaxScrollPos((int)pChar->GetPlayer()->m_pMotdMenu->m_Points.size());
		pChar->GetPlayer()->m_pMotdMenu->m_ScrollManager = oldScrollData;
	}
}

void MotdMenu::Send(int Menulist)
{
	const auto* pGS = (CGS*)Instance::GameServerPlayer(m_ClientID);
	if(auto* pPlayer = pGS->GetPlayer(m_ClientID))
	{
		m_Menulist = Menulist;
		if(pPlayer->m_pMotdMenu)
		{
			m_LastMenulist = (pPlayer->m_pMotdMenu->m_Menulist != Menulist) ? pPlayer->m_pMotdMenu->m_Menulist : pPlayer->m_pMotdMenu->m_LastMenulist;
		}
		pPlayer->m_pMotdMenu = std::make_unique<MotdMenu>(*this);
	}
}

void MotdMenu::ClearMotd(CGS* pGS, CPlayer* pPlayer)
{
	m_Points.clear();
	pGS->SendMotd(m_ClientID, "");
	if(pPlayer && pPlayer->m_pMotdMenu)
	{
		pPlayer->m_pMotdMenu.reset();
	}
}

