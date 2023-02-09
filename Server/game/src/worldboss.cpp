#include "stdafx.h"
#ifdef __WORLD_BOSS_YUMA__
#include "utils.h"
#include "char.h"
#include "char_manager.h"
#include "config.h"
#include "worldboss.h"
#include "sectree_manager.h"
#include "mob_manager.h"

#include <boost/algorithm/string.hpp>
#include <fstream>

CWorldBossManager::CWorldBossManager()
{
}

CWorldBossManager::~CWorldBossManager()
{
}

bool CWorldBossManager::Initialize()
{
	return true;
}

void CWorldBossManager::Destroy()
{
	for (auto& pkBoss : m_listBosses)
	{
		if (pkBoss.ch != NULL)
			M2_DESTROY_CHARACTER(pkBoss.ch);
	}
	m_listBosses.clear();
}

void CWorldBossManager::DestroyBossesInMap(int iMapIndex)
{
	if (iMapIndex == 0)
		return;

	for (int i = m_listBosses.size() - 1; i >= 0; i--)
	{
		if (m_listBosses.at(i).map_index == iMapIndex)
		{
			LPCHARACTER pkChr = m_listBosses.at(i).ch;

			if (pkChr)
				M2_DESTROY_CHARACTER(pkChr);

			m_listBosses.erase(m_listBosses.begin() + i);
		}
	}
}

void CWorldBossManager::Idle(int iPulse)
{
	if (0 == (iPulse % PASSES_PER_SEC(1)))
	{
		for (auto& pkBoss : m_listBosses)
		{
			// only start timer when bosses are dead
			if (pkBoss.is_alive == false)
			{
				int actual_time_in_raw = get_global_time();

				// when boss can spawn
				if (actual_time_in_raw >= pkBoss.next_spawn)
				{
					LPCHARACTER ch = SpawnBoss(pkBoss);

					if (ch) 
					{
						pkBoss.is_alive = true;
						pkBoss.vid = ch->GetVID();
						pkBoss.ch = ch;
						pkBoss.next_spawn = CalculateNextSpawnTime(pkBoss.spawn_time);
					}
				}
				else
				{
					for (unsigned seconds_of_list : pkBoss.m_listSeconds) {
						if (actual_time_in_raw == (pkBoss.next_spawn - seconds_of_list)) {
							int seconds_left_to_spawn = pkBoss.next_spawn - actual_time_in_raw;
							BroadcastNewNotice("<Worldboss> name will spawn again in xx", pkBoss.name, seconds_left_to_spawn);
							break;
						}
					}
				}
			}
		}
	}
}

LPCHARACTER CWorldBossManager::SpawnBoss(BOSS pkBoss)
{
	if (pkBoss.map_index == 0)
		return NULL;

	LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(pkBoss.map_index);
	if (pkSectreeMap == NULL) {
		sys_err("SECTREE_MAP not found for #%ld", pkBoss.map_index);
		return NULL;
	}

	LPCHARACTER ch = CHARACTER_MANAGER::Instance().SpawnMob(pkBoss.vnum, pkBoss.map_index, pkBoss.x, pkBoss.y, pkBoss.z);
	if (!ch)
		return NULL;

	BroadcastNewNotice("<Worldboss> Attention! name has just appeared again", CMobManager::instance().Get(pkBoss.vnum)->m_table.szName);
	return ch;
}

void CWorldBossManager::OnDeathBoss(int iIndex)
{
	if (iIndex == -1)
		return;

	BOSS& pkBoss = m_listBosses[iIndex];

 	pkBoss.is_alive = false;
	pkBoss.ch = NULL;

	BroadcastNewNotice("<Worldboss> name has been defeated!", CMobManager::instance().Get(pkBoss.vnum)->m_table.szName);
}

// Returns the index of vector m_listBosses
int CWorldBossManager::IsBoss(DWORD dwVID)
{
	if (dwVID == 0)
		return -1;

	int iIndex = 0;
	for (const auto& pkBoss : m_listBosses)
	{
		if (pkBoss.ch)
		{
			if (pkBoss.vid == dwVID)
				return iIndex;
		}
		iIndex++;
	}
	return -1;
}

void CWorldBossManager::GetWorldbossInfo(LPCHARACTER ch)
{
	if (!ch)
		return;

	for (const auto& x : m_listBosses)
	{
		char buf[255];
		int seconds_left_to_spawn = x.next_spawn - get_global_time();

		if (x.is_alive == true)
		{
			snprintf(buf, sizeof(buf), LC_TEXT_LANGUAGE(ch->GetLanguage(), "<Worldboss> name is currently alive!"), CMobManager::instance().Get(x.vnum)->m_table.szName);
		}
		else
		{
			snprintf(buf, sizeof(buf), LC_TEXT_LANGUAGE(ch->GetLanguage(), "<Worldboss> name will spawn again in xx"), CMobManager::instance().Get(x.vnum)->m_table.szName,
				floor(seconds_left_to_spawn / 3600.0), floor(fmod(seconds_left_to_spawn, 3600.0) / 60.0),
				((seconds_left_to_spawn / 3600) != 0) ? "ore" : "minuti");
		}

		std::string notification_with_underscore = buf;
		std::replace(notification_with_underscore.begin(), notification_with_underscore.end(), ' ', '_');

		ch->ChatPacket(CHAT_TYPE_COMMAND, "SendWorldbossNotification %s", notification_with_underscore.c_str());
	}
}


// Returns the next valid spawn time from list
// in unix timestamp format
time_t CWorldBossManager::CalculateNextSpawnTime(std::list<int> spawnTimes)
{
	struct tm y2k;
	double time_difference = 0;
	time_t start_time = 0, spawn_time = 0;

	int day = 0;
	bool stop = false;

	while (stop != true)
	{
		for (auto const& i : spawnTimes)
		{
			y2k = { 0 };
			start_time = get_global_time();

			y2k = *localtime(&start_time);
			y2k.tm_mday = y2k.tm_mday + day;
			y2k.tm_hour = i;
			y2k.tm_min = 0;	y2k.tm_sec = 0;

			time_difference = difftime(mktime(&y2k), start_time);

			if (time_difference > 0.0) {
				spawn_time = start_time + time_difference;
				stop = true;
				break;
			}
		}

		if (!stop)
			day++;
	}

	if (spawn_time == 0)
		sys_err("Couldn't calculate next spawnTime. (spawn_time=%lld, start_time=%lld, time_difference=%d", spawn_time, start_time, time_difference);

	return spawn_time;
}

bool CWorldBossManager::worldboss_regen_load(const char* filename, long lMapIndex, int base_x, int base_y)
{
	std::ifstream ifs_file(filename);

	std::vector<std::string> temp_words;
	std::string temp_line;

	// Save each line in vector
	while (std::getline(ifs_file, temp_line))
	{
		if (temp_line.empty())
			continue;

		int index = 0, failure = 0;
		int x, y;

		BOSS temp_boss;

		boost::split(temp_words, temp_line, boost::is_any_of("\t"));
		for (const auto& type : temp_words)
		{
			if (index == 0) {
				if (type != "b")
				{
					failure = 1;
					sys_err("Couldn't find type in worldboss.txt");
					break;
				}
			}
			else if (index == 1) // x
				str_to_number(x, temp_words[index].c_str());
			else if (index == 2) // y
				str_to_number(y, temp_words[index].c_str());
			else if (index == 3) // z
				str_to_number(temp_boss.z, temp_words[index].c_str());
			else if (index == 4) // vnum
				str_to_number(temp_boss.vnum, temp_words[index].c_str());
			else // time
			{
				int time;
				std::string temp_time;

				for (unsigned i = 0; i < temp_words[index].length(); i++)
				{
					switch (temp_words[index][i])
					{
						case 't':
							time = 0;
							str_to_number(time, temp_time.c_str());

							temp_boss.spawn_time.push_back(time);
							temp_time.clear();
							break;

						default:
							if (temp_words[index][i] >= '0' && temp_words[index][i] <= '9')
								temp_time += temp_words[index][i];
					}
				}
				temp_time.clear();
			}
			index++;
		}

		if (failure == 1)
			continue;

		temp_boss.spawn_time.sort();
		temp_boss.next_spawn = CalculateNextSpawnTime(temp_boss.spawn_time);
		if (temp_boss.next_spawn == 0)
			continue;

		x = (base_x / 100) + x;
		y = (base_y / 100) + y;

		temp_boss.x = x * 100;
		temp_boss.y = y * 100;

		temp_boss.map_index = lMapIndex;
		temp_boss.is_alive = false;

		LPCHARACTER ch = SpawnBoss(temp_boss);

		if (ch) {
			temp_boss.name = ch->GetName();
			temp_boss.vid = ch->GetVID();
			M2_DESTROY_CHARACTER(ch);
			temp_boss.ch = NULL;
		}

		m_listBosses.push_back(temp_boss);
	}
	return true;
}
#endif