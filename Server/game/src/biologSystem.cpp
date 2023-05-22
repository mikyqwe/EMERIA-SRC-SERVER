#include "stdafx.h"
#include "char.h"
#include "db.h"
#include "desc_client.h"
#include "packet.h"
#include "biologSystem.h"
#include "item.h"
#include "config.h"
#include "utils.h"
CBiolog::CBiolog()
{
}

CBiolog::~CBiolog()
{
}


void CBiolog::BootBiologTable()
{
	if (g_bAuthServer)
		return;	
	std::unique_ptr<SQLMsg> pkMsg(DBManager::instance().DirectQuery(" \
			SELECT id, level, requestItem, secondsCoolDown, requestItemCount, applyType+0, applyValue, applyType1+0, applyValue1, applyType2+0, applyValue2, isSelectable, deliverPercent, rewardItem, rewardCount FROM biologTable"));

	auto pRes = pkMsg->Get();
	MYSQL_ROW row;

	TBiologTable table;
	uint8_t col;

	if (pRes->uiNumRows > 0)
	{
		while ((row = mysql_fetch_row(pRes->pSQLResult)))
		{
			memset(&table, 0, sizeof(table));
			col = 0;
		
			table.id = std::stoi(row[col++]);
			table.level = std::stoi(row[col++]);

			table.requestItem = std::stoi(row[col++]);
			table.secondsCoolDown = std::stoi(row[col++]);
			table.requestItemCount = std::stoi(row[col++]);

			for (auto i = 0; i < BIOLOG_MAX_VALUES; i++)
			{
				table.values[i].type = std::stoi(row[col++]);
				table.values[i].value = std::stoi(row[col++]);
			}

			table.isBonusSelectable = std::stoi(row[col++]);
			table.deliverPercent = std::stoi(row[col++]);

			table.rewardItem = std::stoi(row[col++]);
			table.rewardCount = std::stoi(row[col++]);

			++biologMaxLimit;

			m_map_BiologTable.insert(std::make_pair(table.id, table));
		}
	}
	else
	{
		sys_err("Failed to load biolog table, abort!");
		abort();
	}
}

bool CBiolog::GetBiologMap(uint8_t id, TBiologTable** pBiologInformation)
{
	auto itor = m_map_BiologTable.find(id);
	if (m_map_BiologTable.end() == itor)
		return false;

	*pBiologInformation = (TBiologTable*)&(itor->second);

	return true;
}

void CBiolog::OnConnect(LPCHARACTER pkChar)
{
	if (!pkChar)
		return;
	
	auto playerData = pkChar->getBiologData();
	uint8_t biologLevel = 0; uint8_t biologCount = 0; uint16_t biologItemVnum = 0;
	std::tie(biologLevel, biologCount, biologItemVnum) = playerData;


	if (biologLevel == 0)
	{
		biologLevel = 1;
		biologCount = 0;

		TBiologTable* info;
		const auto biologTableFirst = GetBiologMap(1, &info);
		if (biologTableFirst)
			biologItemVnum = info->requestItem;

		pkChar->SetBiologData(biologLevel, biologCount, biologItemVnum);
		SendBiologData(pkChar);

		return;
	}

	pkChar->SetBiologData(biologLevel, biologCount, biologItemVnum);

	SendBiologData(pkChar);
}
std::tuple<uint8_t, uint32_t> CBiolog::GetNextBiologInfo(DWORD nextLevel)
{
	TBiologTable* info;
	const auto& biologTable = GetBiologMap(nextLevel, &info);
	if (biologTable)
		return std::make_tuple(info->level, info->requestItem);

	return std::make_tuple(0, 0);

}

void CBiolog::SendBiologData(LPCHARACTER pkChar)
{
	if (pkChar && pkChar->GetDesc())
	{			
		auto playerData = pkChar->getBiologData();
		uint8_t biologLevel = 0; uint8_t biologCount = 0; uint16_t biologItemVnum = 0;
		std::tie(biologLevel, biologCount, biologItemVnum) = playerData;

		TPacketGCBiolog packBiolog;
		TBiologTable* info;

		const auto& biologTable = GetBiologMap(biologLevel, &info);
		if (biologTable && biologLevel >= 1)
		{
			memset(&packBiolog, 0, sizeof(packBiolog));

			packBiolog.bHeader = HEADER_GC_BIOLOG;
			packBiolog.biologLevel = biologLevel;

			packBiolog.requestItem = info->requestItem;

			packBiolog.actualCount = biologCount;
			packBiolog.requestItemCount = info->requestItemCount;

			for (auto i = 0; i < BIOLOG_MAX_VALUES; i++)
			{
				packBiolog.values[i].type = info->values[i].type;
				packBiolog.values[i].value = info->values[i].value;
			}

			packBiolog.secondsCoolDown = pkChar->GetQuestFlag("biolog.cooldown");
			packBiolog.biologRealCooldown = info->secondsCoolDown;

			packBiolog.isBonusSelectable = info->isBonusSelectable;

			packBiolog.rewardItem = info->rewardItem;
			packBiolog.rewardCount = info->rewardCount;

			pkChar->GetDesc()->Packet(&packBiolog, sizeof(TPacketGCBiolog));	
		}
		else
		{
			memset(&packBiolog, 0, sizeof(packBiolog));

			packBiolog.bHeader = HEADER_GC_BIOLOG;
			packBiolog.biologLevel = 99;

			packBiolog.requestItem = 0;

			packBiolog.actualCount = 0;
			packBiolog.requestItemCount = 0;

			for (auto i = 0; i < BIOLOG_MAX_VALUES; i++)
			{
				packBiolog.values[i].type = 0;
				packBiolog.values[i].value = 0;
			}

			packBiolog.secondsCoolDown = 0;
			packBiolog.biologRealCooldown = 0;

			packBiolog.isBonusSelectable = 0;

			packBiolog.rewardItem = 0;
			packBiolog.rewardCount = 0;



			pkChar->GetDesc()->Packet(&packBiolog, sizeof(TPacketGCBiolog));				
		}

	}
}
void CBiolog::AddBonusToChar(LPCHARACTER pkChar, int16_t bonusOption)
{
	auto biologData = pkChar->getBiologData();
	uint8_t biologLevel = 0; uint8_t biologCount = 0; uint16_t biologItemVnum = 0;
	std::tie(biologLevel, biologCount, biologItemVnum) = biologData;
	
	std::set<int16_t> bonusIndex = {-1, 0, 1, 2};
	if (!bonusIndex.count(bonusOption))
		return;

	TBiologTable* info;
	const auto biologTable = GetBiologMap(biologLevel, &info);
	if (biologTable)
	{
		if (info->isBonusSelectable)
			pkChar->AddAffect(AFFECT_BIOLOG_START + biologLevel, aApplyInfo[info->values[bonusOption].type].bPointType, info->values[bonusOption].value, 0, 60 * 60 * 24 * 365, 0, false);
		else
			for (auto i = 0; i < 3; i++)
				if (info->values[i].type > 0)
					pkChar->AddAffect(AFFECT_BIOLOG_START + biologLevel, aApplyInfo[info->values[i].type].bPointType, info->values[i].value, 0, 60 * 60 * 24 * 365, 0, false);
	}
}
bool CBiolog::DeliverItem(LPCHARACTER pkChar, int16_t option, bool morePercentage, bool resetDeliver)
{
	if (!pkChar)
	{
		sys_err("char nullptr biolog");
		return false;
	}

	if (!pkChar->CanHandleItem())
	{
		pkChar->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("CLOSE_ALL_WINDOWS_ERROR"));
		return false;
	}

	auto biologTime = pkChar->GetQuestFlag("biolog.cooldown");
	if (biologTime && get_global_time() < biologTime)
	{
		if (resetDeliver)
		{
			if (pkChar->CountSpecifyItem())
		}
		pkChar->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("YOU NEED TO WAIT!"));
		return false;
	}

	auto biologData = pkChar->getBiologData();
	uint8_t biologLevel = 0; uint8_t biologCount = 0; uint16_t biologItemVnum = 0;
	std::tie(biologLevel, biologCount, biologItemVnum) = biologData;

	if (biologLevel == biologMaxLimit)
	{
		pkChar->ChatPacket(CHAT_TYPE_INFO, "MAX_BIOLOG_REACH");
		return false;
	}

	TBiologTable* info;
	const auto& biologTable = GetBiologMap(biologLevel, &info);
	if (biologTable)
	{
		if (pkChar->GetLevel() < info->level)
		{
			pkChar->ChatPacket(CHAT_TYPE_INFO, "NEED_BIG_LEVEL");
			return false;
		}		

		if (pkChar->CountSpecifyItem(info->requestItem) < 1)
		{
			pkChar->ChatPacket(CHAT_TYPE_INFO, "CANNOT_FIND_ANY_ITEM");
			return false;
		}

		if (biologCount == info->requestItemCount && info->isBonusSelectable)
		{
			std::set<uint8_t> bonusIndex = {0, 1, 2};
			if (!bonusIndex.count(option))
			{
				pkChar->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("PLEASE_SELECT_BONUS_FROM_INTERFACE"));
				return false;
			}
			
			auto biologData = GetNextBiologInfo(biologLevel + 1);

			auto nextItem = std::get<TUPLE_REQUEST_ITEM>(biologData);

			AddBonusToChar(pkChar, option);

			++biologLevel;
			biologCount = 0;
			biologItemVnum = nextItem;				

			pkChar->SetBiologData(biologLevel, biologCount, biologItemVnum);
			pkChar->ChatPacket(CHAT_TYPE_INFO, "LEVEL_UP_BIO");
			pkChar->AutoGiveItem(info->rewardItem, info->rewardCount);


			SendBiologData(pkChar);
			
			return false;				
		}
		
		bool succesPercentage = false;
		auto succesNumber = number(1, 100);

		if (morePercentage)
		{
			succesNumber = succesNumber + 10;
			if (succesNumber > 100)
				succesNumber = 100;
		}

		if (succesNumber <= info->deliverPercent)
			succesPercentage = true;

		if (succesPercentage)
		{
			if (biologCount + 1 == info->requestItemCount && !info->isBonusSelectable)
			{
				auto biologData = GetNextBiologInfo(biologLevel + 1);

				auto nextLevel = std::get<TUPLE_LEVEL>(biologData);
				auto nextItem = std::get<TUPLE_REQUEST_ITEM>(biologData);

				AddBonusToChar(pkChar, option);

				++biologLevel;
				biologCount = 0;
				biologItemVnum = nextItem;				

				pkChar->SetBiologData(biologLevel, biologCount, biologItemVnum);
				pkChar->ChatPacket(CHAT_TYPE_INFO, "LEVEL_UP_BIO");


				SendBiologData(pkChar);

				pkChar->AutoGiveItem(info->rewardItem, info->rewardCount);

			}
			else
			{
				++biologCount;
				SendBiologData(pkChar);
			}
			pkChar->SetBiologData(biologLevel, biologCount, biologItemVnum);		
		}
		else
		{
			pkChar->ChatPacket(CHAT_TYPE_INFO, "BIOLOG_FAILED");
		}

		bool canReset = biologCount == info->requestItemCount ? false : true;
		if (canReset && !resetDeliver)
			pkChar->SetQuestFlag("biolog.cooldown", get_global_time() + info->secondsCoolDown);
		pkChar->RemoveSpecifyItem(info->requestItem, 1);
		SendBiologData(pkChar);
	}

	return true;
}

void CBiolog::ResetBiolog(LPCHARACTER pkChar)
{
	if (!pkChar)
		return;

	auto biologTime = pkChar->GetQuestFlag("biolog.cooldown");
	if (get_global_time() > biologTime)
	{
		pkChar->ChatPacket(CHAT_TYPE_INFO, "NO MORE TIME REMANING");
		return;
	}

	if (pkChar->CountSpecifyItem(BIOLOG_RESET_ITEM) == 0)
	{
		pkChar->ChatPacket(CHAT_TYPE_INFO, "CANNOT_FIND_RESET_ITEM");
		return;
	}

	pkChar->RemoveSpecifyItem(BIOLOG_RESET_ITEM, 1);
	pkChar->SetQuestFlag("biolog.cooldown", 0);
	SendBiologData(pkChar);
}
