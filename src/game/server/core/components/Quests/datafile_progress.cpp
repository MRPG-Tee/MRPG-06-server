/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/server/player.h>
#include "quest_data.h"

#include "datafile_progress.h"

/*
 * QuestDatafile
 */
void QuestDatafile::Create() const
{
	// check if the quest state is not ACCEPT or if the player does not exist
	if(!m_pQuest || m_pQuest->m_State != QuestState::ACCEPT || !m_pQuest->GetPlayer())
		return;

	// check if the "directories" does not exist
	if(!fs_is_dir("server_data/quest_tmp"))
	{
		fs_makedir("server_data");
		fs_makedir("server_data/quest_tmp");
	}

	// json structuring
	nlohmann::json JsonQuestData;
	JsonQuestData["current_step"] = m_pQuest->m_Step;
	m_pQuest->Info()->PreparePlayerSteps(m_pQuest->m_Step, m_pQuest->m_ClientID, &m_pQuest->m_vSteps);
	for(auto& Step : m_pQuest->m_vSteps)
	{
		nlohmann::json Append;
		Append["quest_bot_id"] = Step.m_Bot.m_ID;
		Append["state"] = false;

		for(auto& p : Step.m_Bot.m_vRequiredDefeat)
		{
			Step.m_aMobProgress[p.m_BotID].m_Count = 0;
			Append["defeat"].push_back({ { "id", p.m_BotID }, { "count", 0 }, { "complete", false } });
		}

		Step.m_aMoveToProgress.resize(Step.m_Bot.m_vRequiredMoveAction.size(), false);
		for(auto& p : Step.m_Bot.m_vRequiredMoveAction)
			Append["move_to"].push_back({ { "complete", false } });

		JsonQuestData["steps"].push_back(Append);
	}

	// Loop through each player step
	for(auto& Step : m_pQuest->m_vSteps)
	{
		int MoveToElementsSize = Step.m_Bot.m_vRequiredMoveAction.size();
		Step.m_aMoveToProgress.resize(MoveToElementsSize, false);
		Step.Update();
	}

	// save file
	std::string Data = JsonQuestData.dump();
	Tools::Files::saveFile(GetFilename().c_str(), Data.data(), (unsigned)Data.size());
}

void QuestDatafile::Load() const
{
	// only for accept state
	if(!m_pQuest || m_pQuest->m_State != QuestState::ACCEPT)
		return;

	// loading file is not open pereinitilized steps
	ByteArray RawData;
	if(!Tools::Files::loadFile(GetFilename().c_str(), &RawData))
	{
		Create();
		return;
	}

	// loading steps
	nlohmann::json JsonQuestData = nlohmann::json::parse((char*)RawData.data());
	m_pQuest->m_Step = JsonQuestData.value("current_step", 1);
	m_pQuest->Info()->PreparePlayerSteps(m_pQuest->m_Step, m_pQuest->m_ClientID, &m_pQuest->m_vSteps);

	// Check defferent size of steps
	if(JsonQuestData["steps"].size() != m_pQuest->m_vSteps.size())
	{
		dbg_msg(PRINT_QUEST_PREFIX, "Reinitialization... Player save file has a different size of steps!");
		Create();
		return;
	}

	// iterate through each element in the "steps" array of JsonQuestData
	int dequePos = 0;
	for(auto& Step : JsonQuestData["steps"])
	{
		auto& WorkedNode = m_pQuest->m_vSteps[dequePos++];
		WorkedNode.m_StepComplete = Step.value("state", false);
		if(WorkedNode.m_StepComplete)
			continue;

		// If "defeat" key exists in pStep
		if(Step.contains("defeat"))
		{
			// If the size of the "defeat" array in pStep is not equal to the size of the m_aMobProgress map of the corresponding player step
			if(Step["defeat"].size() != WorkedNode.m_Bot.m_vRequiredDefeat.size())
			{
				dbg_msg(PRINT_QUEST_PREFIX, "Reinitialization... Player save file has a defeat value, but it is not present in the data!");
				Create();
				return;
			}

			// Iterate through each element in the "defeat" array
			for(auto& p : Step["defeat"])
			{
				int ID = p.value("id", 0);
				WorkedNode.m_aMobProgress[ID].m_Count = p.value("count", 0);
				WorkedNode.m_aMobProgress[ID].m_Complete = p.value("complete", 0);
			}
		}

		// If "move_to" key exists in pStep
		if(Step.contains("move_to"))
		{
			// Check if the size of the "move_to" array of pStep is not equal to the size of the m_aPlayerSteps[SubBotID].m_Bot.m_RequiredMoveTo vector
			size_t TotalAction = WorkedNode.m_Bot.m_vRequiredMoveAction.size();
			if(Step["move_to"].size() != TotalAction)
			{
				dbg_msg(PRINT_QUEST_PREFIX, "Reinitialization... Player save file has a move_to value, but it is not present in the data!");
				Create();
				return;
			}

			// Initialize the size of the MoveToProgress array based on the number of required move-to elements
			WorkedNode.m_aMoveToProgress.resize(TotalAction, false);
			for(int p = 0; p < (int)Step["move_to"].size(); p++)
				WorkedNode.m_aMoveToProgress[p] = Step["move_to"][p].value("complete", false);
		}

		// Set ClientQuitting value of the corresponding player step to false
		WorkedNode.m_ClientQuitting = false;
	}

	// Update the steps of the bot
	for(auto& pStep : m_pQuest->m_vSteps)
	{
		// If the current step is not complete
		if(!pStep.m_StepComplete)
			pStep.Update();
	}

	// save file
	std::string Data = JsonQuestData.dump();
	Tools::Files::saveFile(GetFilename().c_str(), Data.data(), (unsigned)Data.size());
}

bool QuestDatafile::Save() const
{
	// Check if the current state of the quest is not "ACCEPT"
	if(!m_pQuest || m_pQuest->m_State != QuestState::ACCEPT)
		return false;

	// json structuring
	nlohmann::json JsonQuestData;
	JsonQuestData["current_step"] = m_pQuest->m_Step;
	for(auto& Step : m_pQuest->m_vSteps)
	{
		nlohmann::json Append;
		Append["quest_bot_id"] = Step.m_Bot.m_ID;
		Append["state"] = Step.m_StepComplete;
		for(auto& p : Step.m_aMobProgress)
			Append["defeat"].push_back({ { "id", p.first }, { "count", p.second.m_Count }, { "complete", p.second.m_Complete } });
		for(auto& p : Step.m_aMoveToProgress)
			Append["move_to"].push_back({ { "complete", p } });

		JsonQuestData["steps"].push_back(Append);
	}

	// replace file
	std::string Data = JsonQuestData.dump();
	const auto Result = Tools::Files::saveFile(GetFilename().c_str(), Data.data(), (unsigned)Data.size());
	return (Result == Tools::Files::Result::SUCCESSFUL);
}

void QuestDatafile::Delete() const
{
	if(!m_pQuest)
		return;

	m_pQuest->m_vSteps.clear();

	// Remove the temporary user quest data file
	Tools::Files::deleteFile(GetFilename().c_str());
	fs_remove(GetFilename().c_str());
}

std::string QuestDatafile::GetFilename() const
{
	const int QuestID = m_pQuest->GetID();
	const int AccountID = m_pQuest->GetPlayer()->Account()->GetID();
	return "server_data/quest_tmp/" + std::to_string(QuestID) + "-" + std::to_string(AccountID) + ".json";
}
