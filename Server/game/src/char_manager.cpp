#include "stdafx.h"
#include "constants.h"
#include "utils.h"
#include "desc.h"
#include "char.h"
#include "char_manager.h"
#include "mob_manager.h"
#include "party.h"
#include "regen.h"
#include "p2p.h"
#include "dungeon.h"
#include "db.h"
#include "config.h"
#include "xmas_event.h"
#include "questmanager.h"
#include "questlua.h"
#include "locale_service.h"
#include "XTrapManager.h"
#include "skill.h"
#include "shutdown_manager.h"
#include "../../common/CommonDefines.h"
#if defined(__SHIP_DEFENSE__)
#	include "ShipDefense.h"
#endif
#ifndef __GNUC__
#include <boost/bind.hpp>
#endif

CHARACTER_MANAGER::CHARACTER_MANAGER() :
	m_iVIDCount(0),
	m_pkChrSelectedStone(NULL),
	m_bUsePendingDestroy(false)
{
	RegisterRaceNum(xmas::MOB_XMAS_FIRWORK_SELLER_VNUM);
	RegisterRaceNum(xmas::MOB_SANTA_VNUM);
	RegisterRaceNum(xmas::MOB_XMAS_TREE_VNUM);

	m_iMobItemRate = 100;
	m_iMobDamageRate = 100;
	m_iMobGoldAmountRate = 100;
	m_iMobGoldDropRate = 100;
	m_iMobExpRate = 100;

	m_iMobItemRatePremium = 100;
	m_iMobGoldAmountRatePremium = 100;
	m_iMobGoldDropRatePremium = 100;
	m_iMobExpRatePremium = 100;

	m_iUserDamageRate = 100;
	m_iUserDamageRatePremium = 100;
#ifdef ENABLE_EVENT_MANAGER
	m_eventData.clear();
#endif

#ifdef ENABLE_DECORUM
	memset(m_adwSeasonChampionDecorum, 0, sizeof(m_adwSeasonChampionDecorum));
#endif
}

CHARACTER_MANAGER::~CHARACTER_MANAGER()
{
	Destroy();
}

void CHARACTER_MANAGER::Destroy()
{
	itertype(m_map_pkChrByVID) it = m_map_pkChrByVID.begin();
	while (it != m_map_pkChrByVID.end()) {
		LPCHARACTER ch = it->second;
		M2_DESTROY_CHARACTER(ch); // m_map_pkChrByVID is changed here
#ifdef ENABLE_EVENT_MANAGER
		m_eventData.clear();
#endif
		it = m_map_pkChrByVID.begin();
	}
	
#ifdef ENABLE_SPECIAL_AFFECT
	m_map_pkSpecialAffectedChr.clear();
#endif
}

void CHARACTER_MANAGER::GracefulShutdown()
{
	NAME_MAP::iterator it = m_map_pkPCChr.begin();

	while (it != m_map_pkPCChr.end())
		(it++)->second->Disconnect("GracefulShutdown");
}

DWORD CHARACTER_MANAGER::AllocVID()
{
	++m_iVIDCount;
	return m_iVIDCount;
}

LPCHARACTER CHARACTER_MANAGER::CreateCharacter(const char * name, DWORD dwPID)
{
	DWORD dwVID = AllocVID();

#ifdef M2_USE_POOL
	LPCHARACTER ch = pool_.Construct();
#else
	LPCHARACTER ch = M2_NEW CHARACTER;
#endif
	ch->Create(name, dwVID, dwPID ? true : false);

	m_map_pkChrByVID.insert(std::make_pair(dwVID, ch));

	if (dwPID)
	{
		char szName[CHARACTER_NAME_MAX_LEN + 1];
		str_lower(name, szName, sizeof(szName));

		m_map_pkPCChr.insert(NAME_MAP::value_type(szName, ch));
		m_map_pkChrByPID.insert(std::make_pair(dwPID, ch));
	}

	return (ch);
}

#ifndef DEBUG_ALLOC
void CHARACTER_MANAGER::DestroyCharacter(LPCHARACTER ch)
#else
void CHARACTER_MANAGER::DestroyCharacter(LPCHARACTER ch, const char* file, size_t line)
#endif
{
	if (!ch)
		return;

	// <Factor> Check whether it has been already deleted or not.
	itertype(m_map_pkChrByVID) it = m_map_pkChrByVID.find(ch->GetVID());
	if (it == m_map_pkChrByVID.end()) {
		sys_err("[CHARACTER_MANAGER::DestroyCharacter] <Factor> %d not found", (long)(ch->GetVID()));
		return; // prevent duplicated destrunction
	}

	// 던전에 소속된 몬스터는 던전에서도 삭제하도록.
#ifdef NEW_PET_SYSTEM
	if (ch->IsNPC() && !ch->IsPet() && !ch->IsNewPet() && ch->GetRider() == NULL)
#else
	if (ch->IsNPC() && !ch->IsPet() && ch->GetRider() == NULL)
#endif
	{
		if (ch->GetDungeon())
		{
			ch->GetDungeon()->DeadCharacter(ch);
		}
	}

#if defined(__SHIP_DEFENSE__)
	// Delete monsters from the ship defense.
	if (ch->IsPC() == false)
		CShipDefenseManager::Instance().OnKill(ch);
#endif

	if (m_bUsePendingDestroy)
	{
		m_set_pkChrPendingDestroy.insert(ch);
		return;
	}

	m_map_pkChrByVID.erase(it);

	if (true == ch->IsPC())
	{
		char szName[CHARACTER_NAME_MAX_LEN + 1];

		str_lower(ch->GetName(), szName, sizeof(szName));

		NAME_MAP::iterator it = m_map_pkPCChr.find(szName);

		if (m_map_pkPCChr.end() != it)
			m_map_pkPCChr.erase(it);
	}

	if (0 != ch->GetPlayerID())
	{
		itertype(m_map_pkChrByPID) it = m_map_pkChrByPID.find(ch->GetPlayerID());

		if (m_map_pkChrByPID.end() != it)
		{
			m_map_pkChrByPID.erase(it);
		}
	}

	UnregisterRaceNumMap(ch);

	RemoveFromStateList(ch);

#ifdef M2_USE_POOL
	pool_.Destroy(ch);
#else
#ifndef DEBUG_ALLOC
	M2_DELETE(ch);
#else
	M2_DELETE_EX(ch, file, line);
#endif
#endif
}

LPCHARACTER CHARACTER_MANAGER::Find(DWORD dwVID)
{
	itertype(m_map_pkChrByVID) it = m_map_pkChrByVID.find(dwVID);

	if (m_map_pkChrByVID.end() == it)
		return NULL;

	// <Factor> Added sanity check
	LPCHARACTER found = it->second;
	if (found != NULL && dwVID != (DWORD)found->GetVID()) {
		sys_err("[CHARACTER_MANAGER::Find] <Factor> %u != %u", dwVID, (DWORD)found->GetVID());
		return NULL;
	}
	return found;
}

LPCHARACTER CHARACTER_MANAGER::Find(const VID & vid)
{
	LPCHARACTER tch = Find((DWORD) vid);

	if (!tch || tch->GetVID() != vid)
		return NULL;

	return tch;
}

LPCHARACTER CHARACTER_MANAGER::FindByPID(DWORD dwPID)
{
	itertype(m_map_pkChrByPID) it = m_map_pkChrByPID.find(dwPID);

	if (m_map_pkChrByPID.end() == it)
		return NULL;

	// <Factor> Added sanity check
	LPCHARACTER found = it->second;
	if (found != NULL && dwPID != found->GetPlayerID()) {
		sys_err("[CHARACTER_MANAGER::FindByPID] <Factor> %u != %u", dwPID, found->GetPlayerID());
		return NULL;
	}
	return found;
}

LPCHARACTER CHARACTER_MANAGER::FindPC(const char * name)
{
	char szName[CHARACTER_NAME_MAX_LEN + 1];
	str_lower(name, szName, sizeof(szName));
	NAME_MAP::iterator it = m_map_pkPCChr.find(szName);

	if (it == m_map_pkPCChr.end())
		return NULL;

	// <Factor> Added sanity check
	LPCHARACTER found = it->second;
	if (found != NULL && strncasecmp(szName, found->GetName(), CHARACTER_NAME_MAX_LEN) != 0) {
		sys_err("[CHARACTER_MANAGER::FindPC] <Factor> %s != %s", name, found->GetName());
		return NULL;
	}
	return found;
}

LPCHARACTER CHARACTER_MANAGER::SpawnMobRandomPosition(DWORD dwVnum, long lMapIndex)
{
	// 왜구 스폰할지말지를 결정할 수 있게함
	{
		if (dwVnum == 5001 && !quest::CQuestManager::instance().GetEventFlag("japan_regen"))
		{
			sys_log(1, "WAEGU[5001] regen disabled.");
			return NULL;
		}
	}

	// 해태를 스폰할지 말지를 결정할 수 있게 함
	{
		if (dwVnum == 5002 && !quest::CQuestManager::instance().GetEventFlag("newyear_mob"))
		{
			sys_log(1, "HAETAE (new-year-mob) [5002] regen disabled.");
			return NULL;
		}
	}

	// 광복절 이벤트
	{
		if (dwVnum == 5004 && !quest::CQuestManager::instance().GetEventFlag("independence_day"))
		{
			sys_log(1, "INDEPENDECE DAY [5004] regen disabled.");
			return NULL;
		}
	}

	const CMob * pkMob = CMobManager::instance().Get(dwVnum);

	if (!pkMob)
	{
		sys_err("no mob data for vnum %u", dwVnum);
		return NULL;
	}

	if (!map_allow_find(lMapIndex))
	{
		sys_err("not allowed map %u", lMapIndex);
		return NULL;
	}

	LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(lMapIndex);
	if (pkSectreeMap == NULL) {
		return NULL;
	}

	int i;
	long x, y;
	for (i=0; i<2000; i++)
	{
		x = number(1, (pkSectreeMap->m_setting.iWidth / 100)  - 1) * 100 + pkSectreeMap->m_setting.iBaseX;
		y = number(1, (pkSectreeMap->m_setting.iHeight / 100) - 1) * 100 + pkSectreeMap->m_setting.iBaseY;
		//LPSECTREE tree = SECTREE_MANAGER::instance().Get(lMapIndex, x, y);
		LPSECTREE tree = pkSectreeMap->Find(x, y);

		if (!tree)
			continue;

		DWORD dwAttr = tree->GetAttribute(x, y);

		if (IS_SET(dwAttr, ATTR_BLOCK | ATTR_OBJECT))
			continue;

		if (IS_SET(dwAttr, ATTR_BANPK))
			continue;

		break;
	}

	if (i == 2000)
	{
		sys_err("cannot find valid location");
		return NULL;
	}

	LPSECTREE sectree = SECTREE_MANAGER::instance().Get(lMapIndex, x, y);

	if (!sectree)
	{
		sys_log(0, "SpawnMobRandomPosition: cannot create monster at non-exist sectree %d x %d (map %d)", x, y, lMapIndex);
		return NULL;
	}

	#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
	LPCHARACTER ch = CHARACTER_MANAGER::instance().CreateCharacter(pkMob->m_table.szLocaleName[0]);
	#else
	LPCHARACTER ch = CHARACTER_MANAGER::instance().CreateCharacter(pkMob->m_table.szLocaleName);
	#endif

	if (!ch)
	{
		sys_log(0, "SpawnMobRandomPosition: cannot create new character");
		return NULL;
	}

	ch->SetProto(pkMob);

	// if mob is npc with no empire assigned, assign to empire of map
	if (pkMob->m_table.bType == CHAR_TYPE_NPC)
		if (ch->GetEmpire() == 0)
			ch->SetEmpire(SECTREE_MANAGER::instance().GetEmpireFromMapIndex(lMapIndex));

	ch->SetRotation(number(0, 360));

	if (!ch->Show(lMapIndex, x, y, 0, false))
	{
		M2_DESTROY_CHARACTER(ch);
		sys_err(0, "SpawnMobRandomPosition: cannot show monster");
		return NULL;
	}

	char buf[512+1];
	long local_x = x - pkSectreeMap->m_setting.iBaseX;
	long local_y = y - pkSectreeMap->m_setting.iBaseY;
	snprintf(buf, sizeof(buf), "spawn %s[%d] random position at %ld %ld %ld %ld (time: %d)", ch->GetName(), dwVnum, x, y, local_x, local_y, get_global_time());

	if (test_server)
		SendNotice(buf);

	sys_log(0, buf);
	return (ch);
}

#ifdef OFFLINE_SHOP
LPCHARACTER CHARACTER_MANAGER::SpawnMob(DWORD dwVnum, long lMapIndex, long x, long y, long z, bool bSpawnMotion, int iRot, bool bShow, bool isOfflineShopNPC, DWORD real_owner, DWORD real_owner_aid)
#else
LPCHARACTER CHARACTER_MANAGER::SpawnMob(DWORD dwVnum, long lMapIndex, long x, long y, long z, bool bSpawnMotion, int iRot, bool bShow)
#endif
{
	const CMob * pkMob = CMobManager::instance().Get(dwVnum);
	if (!pkMob)
	{
		sys_err("SpawnMob: no mob data for vnum %u", dwVnum);
		return NULL;
	}

	if (!(pkMob->m_table.bType == CHAR_TYPE_NPC || pkMob->m_table.bType == CHAR_TYPE_PET  || pkMob->m_table.bType == CHAR_TYPE_WARP || pkMob->m_table.bType == CHAR_TYPE_PET || pkMob->m_table.bType == CHAR_TYPE_GOTO) || mining::IsVeinOfOre(dwVnum))
	{
		LPSECTREE tree = SECTREE_MANAGER::instance().Get(lMapIndex, x, y);

		if (!tree)
		{
			sys_log(0, "no sectree for spawn at %d %d mobvnum %d mapindex %d", x, y, dwVnum, lMapIndex);
			return NULL;
		}

		DWORD dwAttr = tree->GetAttribute(x, y);

		bool is_set = false;

		if ( mining::IsVeinOfOre (dwVnum) ) is_set = IS_SET(dwAttr, ATTR_BLOCK);
		else is_set = IS_SET(dwAttr, ATTR_BLOCK | ATTR_OBJECT);

		if ( is_set )
		{
			// SPAWN_BLOCK_LOG
			static bool s_isLog=quest::CQuestManager::instance().GetEventFlag("spawn_block_log");
			static DWORD s_nextTime=get_global_time()+10000;

			DWORD curTime=get_global_time();

			if (curTime>s_nextTime)
			{
				s_nextTime=curTime;
				s_isLog=quest::CQuestManager::instance().GetEventFlag("spawn_block_log");

			}

			if (s_isLog)
				sys_log(0, "SpawnMob: BLOCKED position for spawn %s %u at %d %d (attr %u)", pkMob->m_table.szName, dwVnum, x, y, dwAttr);
			// END_OF_SPAWN_BLOCK_LOG
			return NULL;
		}

		if (IS_SET(dwAttr, ATTR_BANPK))
		{
			sys_log(0, "SpawnMob: BAN_PK position for mob spawn %s %u at %d %d", pkMob->m_table.szName, dwVnum, x, y);
			return NULL;
		}
	}

	LPSECTREE sectree = SECTREE_MANAGER::instance().Get(lMapIndex, x, y);

	if (!sectree)
	{
		sys_log(0, "SpawnMob: cannot create monster at non-exist sectree %d x %d (map %d)", x, y, lMapIndex);
		return NULL;
	}

	#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
	LPCHARACTER ch = CHARACTER_MANAGER::instance().CreateCharacter(pkMob->m_table.szLocaleName[0]);
	#else
	LPCHARACTER ch = CHARACTER_MANAGER::instance().CreateCharacter(pkMob->m_table.szLocaleName);
	#endif

	if (!ch)
	{
		sys_log(0, "SpawnMob: cannot create new character");
		return NULL;
	}

	if (iRot == -1)
		iRot = number(0, 360);

	ch->SetProto(pkMob);

	// if mob is npc with no empire assigned, assign to empire of map
	if (pkMob->m_table.bType == CHAR_TYPE_NPC)
		if (ch->GetEmpire() == 0)
			ch->SetEmpire(SECTREE_MANAGER::instance().GetEmpireFromMapIndex(lMapIndex));

	ch->SetRotation(iRot);
#ifdef OFFLINE_SHOP
	if (isOfflineShopNPC)
	{
		ch->SetOfflineShopNPC(isOfflineShopNPC);
		ch->SetOfflineShopRealOwner(real_owner);
		ch->SetOfflineShopRealOwnerAccountID(real_owner_aid);
	}
#endif
	if (bShow && !ch->Show(lMapIndex, x, y, z, bSpawnMotion))
	{
		M2_DESTROY_CHARACTER(ch);
		sys_log(0, "SpawnMob: cannot show monster");
		return NULL;
	}

	return (ch);
}

LPCHARACTER CHARACTER_MANAGER::SpawnMobRange(DWORD dwVnum, long lMapIndex, int sx, int sy, int ex, int ey, bool bIsException, bool bSpawnMotion, bool bAggressive )
{
	const CMob * pkMob = CMobManager::instance().Get(dwVnum);

	if (!pkMob)
		return NULL;

	if (pkMob->m_table.bType == CHAR_TYPE_STONE
#ifdef ENABLE_DEFENSAWE_SHIP
	|| dwVnum == 6512
#endif
	)
		bSpawnMotion = true;

	int i = 16;

	while (i--)
	{
		int x = number(sx, ex);
		int y = number(sy, ey);
		/*
		   if (bIsException)
		   if (is_regen_exception(x, y))
		   continue;
		 */
		LPCHARACTER ch = SpawnMob(dwVnum, lMapIndex, x, y, 0, bSpawnMotion);

		if (ch)
		{
			sys_log(1, "MOB_SPAWN: %s(%d) %dx%d", ch->GetName(), (DWORD) ch->GetVID(), ch->GetX(), ch->GetY());
			if ( bAggressive )
				ch->SetAggressive();
			return (ch);
		}
	}

	return NULL;
}

void CHARACTER_MANAGER::SelectStone(LPCHARACTER pkChr)
{
	m_pkChrSelectedStone = pkChr;
}

bool CHARACTER_MANAGER::SpawnMoveGroup(DWORD dwVnum, long lMapIndex, int sx, int sy, int ex, int ey, int tx, int ty, LPREGEN pkRegen, bool bAggressive_)
{
	CMobGroup * pkGroup = CMobManager::Instance().GetGroup(dwVnum);

	if (!pkGroup)
	{
		sys_err("NOT_EXIST_GROUP_VNUM(%u) Map(%u) ", dwVnum, lMapIndex);
		return false;
	}

	LPCHARACTER pkChrMaster = NULL;
	LPPARTY pkParty = NULL;

	const std::vector<DWORD> & c_rdwMembers = pkGroup->GetMemberVector();

	bool bSpawnedByStone = false;
	bool bAggressive = bAggressive_;

	if (m_pkChrSelectedStone)
	{
		bSpawnedByStone = true;
		if (m_pkChrSelectedStone->GetDungeon())
			bAggressive = true;
	}

	for (DWORD i = 0; i < c_rdwMembers.size(); ++i)
	{
		LPCHARACTER tch = SpawnMobRange(c_rdwMembers[i], lMapIndex, sx, sy, ex, ey, true, bSpawnedByStone);

		if (!tch)
		{
			if (i == 0)	// 못만든 몬스터가 대장일 경우에는 그냥 실패
				return false;

			continue;
		}

		sx = tch->GetX() - number(300, 500);
		sy = tch->GetY() - number(300, 500);
		ex = tch->GetX() + number(300, 500);
		ey = tch->GetY() + number(300, 500);

		if (m_pkChrSelectedStone)
			tch->SetStone(m_pkChrSelectedStone);
		else if (pkParty)
		{
			pkParty->Join(tch->GetVID());
			pkParty->Link(tch);
		}
		else if (!pkChrMaster)
		{
			pkChrMaster = tch;
			pkChrMaster->SetRegen(pkRegen);

			pkParty = CPartyManager::instance().CreateParty(pkChrMaster);
		}
		if (bAggressive)
			tch->SetAggressive();

		if (tch->Goto(tx, ty))
			tch->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
	}

	return true;
}

bool CHARACTER_MANAGER::SpawnGroupGroup(DWORD dwVnum, long lMapIndex, int sx, int sy, int ex, int ey, LPREGEN pkRegen, bool bAggressive_, LPDUNGEON pDungeon)
{
	const DWORD dwGroupID = CMobManager::Instance().GetGroupFromGroupGroup(dwVnum);

	if( dwGroupID != 0 )
	{
		return SpawnGroup(dwGroupID, lMapIndex, sx, sy, ex, ey, pkRegen, bAggressive_, pDungeon);
	}
	else
	{
		sys_err( "NOT_EXIST_GROUP_GROUP_VNUM(%u) MAP(%ld)", dwVnum, lMapIndex );
		return false;
	}
}

LPCHARACTER CHARACTER_MANAGER::SpawnGroup(DWORD dwVnum, long lMapIndex, int sx, int sy, int ex, int ey, LPREGEN pkRegen, bool bAggressive_, LPDUNGEON pDungeon)
{
	CMobGroup * pkGroup = CMobManager::Instance().GetGroup(dwVnum);

	if (!pkGroup)
	{
		sys_err("NOT_EXIST_GROUP_VNUM(%u) Map(%u) ", dwVnum, lMapIndex);
		return NULL;
	}

	LPCHARACTER pkChrMaster = NULL;
	LPPARTY pkParty = NULL;

	const std::vector<DWORD> & c_rdwMembers = pkGroup->GetMemberVector();

	bool bSpawnedByStone = false;
	bool bAggressive = bAggressive_;

	if (m_pkChrSelectedStone)
	{
		bSpawnedByStone = true;

		if (m_pkChrSelectedStone->GetDungeon())
			bAggressive = true;
	}

	LPCHARACTER chLeader = NULL;

	for (DWORD i = 0; i < c_rdwMembers.size(); ++i)
	{
		LPCHARACTER tch = SpawnMobRange(c_rdwMembers[i], lMapIndex, sx, sy, ex, ey, true, bSpawnedByStone);

		if (!tch)
		{
			if (i == 0)	// 못만든 몬스터가 대장일 경우에는 그냥 실패
				return NULL;

			continue;
		}

		if (i == 0)
			chLeader = tch;

		tch->SetDungeon(pDungeon);

		sx = tch->GetX() - number(300, 500);
		sy = tch->GetY() - number(300, 500);
		ex = tch->GetX() + number(300, 500);
		ey = tch->GetY() + number(300, 500);

		if (m_pkChrSelectedStone)
			tch->SetStone(m_pkChrSelectedStone);
		else if (pkParty)
		{
			pkParty->Join(tch->GetVID());
			pkParty->Link(tch);
		}
		else if (!pkChrMaster)
		{
			pkChrMaster = tch;
			pkChrMaster->SetRegen(pkRegen);

			pkParty = CPartyManager::instance().CreateParty(pkChrMaster);
		}

		if (bAggressive)
			tch->SetAggressive();
	}

	return chLeader;
}

struct FuncUpdateAndResetChatCounter
{
	void operator () (LPCHARACTER ch)
	{
		ch->ResetChatCounter();
		ch->CFSM::Update();
	}
};

void CHARACTER_MANAGER::Update(int iPulse)
{
	using namespace std;
#if defined(__GNUC__) && !defined(__clang__) && !defined(CXX11_ENABLED)
	using namespace __gnu_cxx;
#endif

	BeginPendingDestroy();

	// PC 캐릭터 업데이트
	{
		if (!m_map_pkPCChr.empty())
		{
			// 컨테이너 복사
			CHARACTER_VECTOR v;
			v.reserve(m_map_pkPCChr.size());
			for(auto& iter : m_map_pkPCChr)
				v.emplace_back(iter.second);

			if (0 == (iPulse % PASSES_PER_SEC(5)))
			{
				FuncUpdateAndResetChatCounter f;
				for_each(v.begin(), v.end(), f);
			}
			else
			{
				//for_each(v.begin(), v.end(), mem_fun(&CFSM::Update));
				for_each(v.begin(), v.end(), bind2nd(mem_fun(&CHARACTER::UpdateCharacter), iPulse));
			}
		}

//		for_each_pc(bind2nd(mem_fun(&CHARACTER::UpdateCharacter), iPulse));
	}

	// 몬스터 업데이트
	{
		if (!m_set_pkChrState.empty())
		{
			CHARACTER_VECTOR v;
			v.reserve(m_set_pkChrState.size());
#if defined(__GNUC__) && !defined(__clang__) && !defined(CXX11_ENABLED)
			transform(m_set_pkChrState.begin(), m_set_pkChrState.end(), back_inserter(v), identity<CHARACTER_SET::value_type>());
#else
			v.insert(v.end(), m_set_pkChrState.begin(), m_set_pkChrState.end());
#endif
			for_each(v.begin(), v.end(), bind2nd(mem_fun(&CHARACTER::UpdateStateMachine), iPulse));
		}
	}

	// 산타 따로 업데이트
	{
		CharacterVectorInteractor i;

		if (CHARACTER_MANAGER::instance().GetCharactersByRaceNum(xmas::MOB_SANTA_VNUM, i))
		{
			for_each(i.begin(), i.end(),
					bind2nd(mem_fun(&CHARACTER::UpdateStateMachine), iPulse));
		}
	}

	// 1시간에 한번씩 몹 사냥 개수 기록
	if (0 == (iPulse % PASSES_PER_SEC(3600)))
	{
		for (itertype(m_map_dwMobKillCount) it = m_map_dwMobKillCount.begin(); it != m_map_dwMobKillCount.end(); ++it)
			DBManager::instance().SendMoneyLog(MONEY_LOG_MONSTER_KILL, it->first, it->second);

		m_map_dwMobKillCount.clear();
	}

	// 테스트 서버에서는 60초마다 캐릭터 개수를 센다
	if (test_server && 0 == (iPulse % PASSES_PER_SEC(60)))
		sys_log(0, "CHARACTER COUNT vid %zu pid %zu", m_map_pkChrByVID.size(), m_map_pkChrByPID.size());

	// 지연된 DestroyCharacter 하기
	FlushPendingDestroy();

	// ShutdownManager Update
	CShutdownManager::Instance().Update();
}

#ifdef ENABLE_DECORUM
void CHARACTER_MANAGER::SetSeasonChampionDecorum(BYTE bIndex, DWORD dwPID)
{
	if (bIndex >= 3)
		return;
	
	m_adwSeasonChampionDecorum[bIndex] = dwPID;
}

int CHARACTER_MANAGER::IsSeasonChampionDecorum(DWORD dwPID)
{
	for (int i = 0; i < 3; i++)
	{
		if (m_adwSeasonChampionDecorum[i] == dwPID)
			return i;
	}
	
	return -1;
}
#endif

void CHARACTER_MANAGER::ProcessDelayedSave()
{
	CHARACTER_SET::iterator it = m_set_pkChrForDelayedSave.begin();

	while (it != m_set_pkChrForDelayedSave.end())
	{
		LPCHARACTER pkChr = *it++;
		pkChr->SaveReal();
	}

	m_set_pkChrForDelayedSave.clear();
}

bool CHARACTER_MANAGER::AddToStateList(LPCHARACTER ch)
{
	assert(ch != NULL);

	CHARACTER_SET::iterator it = m_set_pkChrState.find(ch);

	if (it == m_set_pkChrState.end())
	{
		m_set_pkChrState.insert(ch);
		return true;
	}

	return false;
}

void CHARACTER_MANAGER::RemoveFromStateList(LPCHARACTER ch)
{
	CHARACTER_SET::iterator it = m_set_pkChrState.find(ch);

	if (it != m_set_pkChrState.end())
	{
		//sys_log(0, "RemoveFromStateList %p", ch);
		m_set_pkChrState.erase(it);
	}
}

void CHARACTER_MANAGER::DelayedSave(LPCHARACTER ch)
{
	m_set_pkChrForDelayedSave.insert(ch);
}

bool CHARACTER_MANAGER::FlushDelayedSave(LPCHARACTER ch)
{
	CHARACTER_SET::iterator it = m_set_pkChrForDelayedSave.find(ch);

	if (it == m_set_pkChrForDelayedSave.end())
		return false;

	m_set_pkChrForDelayedSave.erase(it);
	ch->SaveReal();
	return true;
}

void CHARACTER_MANAGER::RegisterForMonsterLog(LPCHARACTER ch)
{
	m_set_pkChrMonsterLog.insert(ch);
}

void CHARACTER_MANAGER::UnregisterForMonsterLog(LPCHARACTER ch)
{
	m_set_pkChrMonsterLog.erase(ch);
}

void CHARACTER_MANAGER::PacketMonsterLog(LPCHARACTER ch, const void* buf, int size)
{
	itertype(m_set_pkChrMonsterLog) it;

	for (it = m_set_pkChrMonsterLog.begin(); it!=m_set_pkChrMonsterLog.end();++it)
	{
		LPCHARACTER c = *it;

		if (ch && DISTANCE_APPROX(c->GetX()-ch->GetX(), c->GetY()-ch->GetY())>6000)
			continue;

		LPDESC d = c->GetDesc();

		if (d)
			d->Packet(buf, size);
	}
}

void CHARACTER_MANAGER::KillLog(DWORD dwVnum)
{
	const DWORD SEND_LIMIT = 10000;

	itertype(m_map_dwMobKillCount) it = m_map_dwMobKillCount.find(dwVnum);

	if (it == m_map_dwMobKillCount.end())
		m_map_dwMobKillCount.insert(std::make_pair(dwVnum, 1));
	else
	{
		++it->second;

		if (it->second > SEND_LIMIT)
		{
			DBManager::instance().SendMoneyLog(MONEY_LOG_MONSTER_KILL, it->first, it->second);
			m_map_dwMobKillCount.erase(it);
		}
	}
}

void CHARACTER_MANAGER::RegisterRaceNum(DWORD dwVnum)
{
	m_set_dwRegisteredRaceNum.insert(dwVnum);
}

void CHARACTER_MANAGER::RegisterRaceNumMap(LPCHARACTER ch)
{
	DWORD dwVnum = ch->GetRaceNum();

	if (m_set_dwRegisteredRaceNum.find(dwVnum) != m_set_dwRegisteredRaceNum.end()) // 등록된 번호 이면
	{
		sys_log(0, "RegisterRaceNumMap %s %u", ch->GetName(), dwVnum);
		m_map_pkChrByRaceNum[dwVnum].insert(ch);
	}
}

void CHARACTER_MANAGER::UnregisterRaceNumMap(LPCHARACTER ch)
{
	DWORD dwVnum = ch->GetRaceNum();

	itertype(m_map_pkChrByRaceNum) it = m_map_pkChrByRaceNum.find(dwVnum);

	if (it != m_map_pkChrByRaceNum.end())
		it->second.erase(ch);
}

bool CHARACTER_MANAGER::GetCharactersByRaceNum(DWORD dwRaceNum, CharacterVectorInteractor & i)
{
	std::map<DWORD, CHARACTER_SET>::iterator it = m_map_pkChrByRaceNum.find(dwRaceNum);

	if (it == m_map_pkChrByRaceNum.end())
		return false;

	// 컨테이너 복사
	i = it->second;
	return true;
}

#define FIND_JOB_WARRIOR_0	(1 << 3)
#define FIND_JOB_WARRIOR_1	(1 << 4)
#define FIND_JOB_WARRIOR_2	(1 << 5)
#define FIND_JOB_WARRIOR	(FIND_JOB_WARRIOR_0 | FIND_JOB_WARRIOR_1 | FIND_JOB_WARRIOR_2)
#define FIND_JOB_ASSASSIN_0	(1 << 6)
#define FIND_JOB_ASSASSIN_1	(1 << 7)
#define FIND_JOB_ASSASSIN_2	(1 << 8)
#define FIND_JOB_ASSASSIN	(FIND_JOB_ASSASSIN_0 | FIND_JOB_ASSASSIN_1 | FIND_JOB_ASSASSIN_2)
#define FIND_JOB_SURA_0		(1 << 9)
#define FIND_JOB_SURA_1		(1 << 10)
#define FIND_JOB_SURA_2		(1 << 11)
#define FIND_JOB_SURA		(FIND_JOB_SURA_0 | FIND_JOB_SURA_1 | FIND_JOB_SURA_2)
#define FIND_JOB_SHAMAN_0	(1 << 12)
#define FIND_JOB_SHAMAN_1	(1 << 13)
#define FIND_JOB_SHAMAN_2	(1 << 14)
#define FIND_JOB_SHAMAN		(FIND_JOB_SHAMAN_0 | FIND_JOB_SHAMAN_1 | FIND_JOB_SHAMAN_2)
#ifdef ENABLE_WOLFMAN_CHARACTER
#define FIND_JOB_WOLFMAN_0	(1 << 15)
#define FIND_JOB_WOLFMAN_1	(1 << 16)
#define FIND_JOB_WOLFMAN_2	(1 << 17)
#define FIND_JOB_WOLFMAN		(FIND_JOB_WOLFMAN_0 | FIND_JOB_WOLFMAN_1 | FIND_JOB_WOLFMAN_2)
#endif

//
// (job+1)*3+(skill_group)
//
LPCHARACTER CHARACTER_MANAGER::FindSpecifyPC(unsigned int uiJobFlag, long lMapIndex, LPCHARACTER except, int iMinLevel, int iMaxLevel)
{
	LPCHARACTER chFind = NULL;
	itertype(m_map_pkChrByPID) it;
	int n = 0;

	for (it = m_map_pkChrByPID.begin(); it != m_map_pkChrByPID.end(); ++it)
	{
		LPCHARACTER ch = it->second;

		if (ch == except)
			continue;

		if (ch->GetLevel() < iMinLevel)
			continue;

		if (ch->GetLevel() > iMaxLevel)
			continue;

		if (ch->GetMapIndex() != lMapIndex)
			continue;

		if (uiJobFlag)
		{
			unsigned int uiChrJob = (1 << ((ch->GetJob() + 1) * 3 + ch->GetSkillGroup()));

			if (!IS_SET(uiJobFlag, uiChrJob))
				continue;
		}

		if (!chFind || number(1, ++n) == 1)
			chFind = ch;
	}

	return chFind;
}

int CHARACTER_MANAGER::GetMobItemRate(LPCHARACTER ch)
{
	//PREVENT_TOXICATION_FOR_CHINA
	if (g_bChinaIntoxicationCheck)
	{
		if ( ch->IsOverTime( OT_3HOUR ) )
		{
			if (ch && ch->GetPremiumRemainSeconds(PREMIUM_ITEM) > 0)
				return m_iMobItemRatePremium/2;
			return m_iMobItemRate/2;
		}
		else if ( ch->IsOverTime( OT_5HOUR ) )
		{
			return 0;
		}
	}
	//END_PREVENT_TOXICATION_FOR_CHINA
	if (ch && ch->GetPremiumRemainSeconds(PREMIUM_ITEM) > 0)
		return m_iMobItemRatePremium;
	return m_iMobItemRate;
}

int CHARACTER_MANAGER::GetMobDamageRate(LPCHARACTER ch)
{
	return m_iMobDamageRate;
}

int CHARACTER_MANAGER::GetMobGoldAmountRate(LPCHARACTER ch)
{
	if ( !ch )
		return m_iMobGoldAmountRate;

	//PREVENT_TOXICATION_FOR_CHINA
	if (g_bChinaIntoxicationCheck)
	{
		if ( ch->IsOverTime( OT_3HOUR ) )
		{
			if (ch && ch->GetPremiumRemainSeconds(PREMIUM_GOLD) > 0)
				return m_iMobGoldAmountRatePremium/2;
			return m_iMobGoldAmountRate/2;
		}
		else if ( ch->IsOverTime( OT_5HOUR ) )
		{
			return 0;
		}
	}
	//END_PREVENT_TOXICATION_FOR_CHINA
	if (ch && ch->GetPremiumRemainSeconds(PREMIUM_GOLD) > 0)
		return m_iMobGoldAmountRatePremium;
	return m_iMobGoldAmountRate;
}

int CHARACTER_MANAGER::GetMobGoldDropRate(LPCHARACTER ch)
{
	if ( !ch )
		return m_iMobGoldDropRate;

	//PREVENT_TOXICATION_FOR_CHINA
	if (g_bChinaIntoxicationCheck)
	{
		if ( ch->IsOverTime( OT_3HOUR ) )
		{
			if (ch && ch->GetPremiumRemainSeconds(PREMIUM_GOLD) > 0)
				return m_iMobGoldDropRatePremium/2;
			return m_iMobGoldDropRate/2;
		}
		else if ( ch->IsOverTime( OT_5HOUR ) )
		{
			return 0;
		}
	}
	//END_PREVENT_TOXICATION_FOR_CHINA
	if (ch && ch->GetPremiumRemainSeconds(PREMIUM_GOLD) > 0)
		return m_iMobGoldDropRatePremium;
	return m_iMobGoldDropRate;
}

int CHARACTER_MANAGER::GetMobExpRate(LPCHARACTER ch)
{
	if ( !ch )
		return m_iMobExpRate;

	//PREVENT_TOXICATION_FOR_CHINA
	if (g_bChinaIntoxicationCheck)
	{
		if ( ch->IsOverTime( OT_3HOUR ) )
		{
			if (ch && ch->GetPremiumRemainSeconds(PREMIUM_EXP) > 0)
				return m_iMobExpRatePremium/2;
			return m_iMobExpRate/2;
		}
		else if ( ch->IsOverTime( OT_5HOUR ) )
		{
			return 0;
		}
	}
	//END_PREVENT_TOXICATION_FOR_CHINA
	if (ch && ch->GetPremiumRemainSeconds(PREMIUM_EXP) > 0)
		return m_iMobExpRatePremium;
	return m_iMobExpRate;
}

int	CHARACTER_MANAGER::GetUserDamageRate(LPCHARACTER ch)
{
	if (!ch)
		return m_iUserDamageRate;

	if (ch && ch->GetPremiumRemainSeconds(PREMIUM_EXP) > 0)
		return m_iUserDamageRatePremium;

	return m_iUserDamageRate;
}

void CHARACTER_MANAGER::SendScriptToMap(long lMapIndex, const std::string & s)
{
	LPSECTREE_MAP pSecMap = SECTREE_MANAGER::instance().GetMap(lMapIndex);

	if (NULL == pSecMap)
		return;

	struct packet_script p;

	p.header = HEADER_GC_SCRIPT;
	p.skin = 1;
	p.src_size = s.size();

	quest::FSendPacket f;
	p.size = p.src_size + sizeof(struct packet_script);
	f.buf.write(&p, sizeof(struct packet_script));
	f.buf.write(&s[0], s.size());

	pSecMap->for_each(f);
}

bool CHARACTER_MANAGER::BeginPendingDestroy()
{
	// Begin 이 후에 Begin을 또 하는 경우에 Flush 하지 않는 기능 지원을 위해
	// 이미 시작되어있으면 false 리턴 처리
	if (m_bUsePendingDestroy)
		return false;

	m_bUsePendingDestroy = true;
	return true;
}

void CHARACTER_MANAGER::FlushPendingDestroy()
{

	using namespace std;

	m_bUsePendingDestroy = false; // 플래그를 먼저 설정해야 실제 Destroy 처리가 됨

	if (!m_set_pkChrPendingDestroy.empty())
	{
		sys_log(0, "FlushPendingDestroy size %d", m_set_pkChrPendingDestroy.size());

		CHARACTER_SET::iterator it = m_set_pkChrPendingDestroy.begin(),
			end = m_set_pkChrPendingDestroy.end();
		for ( ; it != end; ++it) {
			M2_DESTROY_CHARACTER(*it);
		}

		m_set_pkChrPendingDestroy.clear();
	}
}

#ifdef SYSTEM_PDA
const DWORD* CHARACTER_MANAGER::GetUsableSkillList(BYTE bJob, BYTE bSkillGroup) const
{
	if (bSkillGroup == 0)
		return NULL;

	static const DWORD SkillList[JOB_MAX_NUM][SKILL_GROUP_MAX_NUM][CHARACTER_SKILL_COUNT] =
	{
		{ {	1,	2,	3,	4,	5,	6	}, {	16,	17,	18,	19,	20,	21	} },
		{ {	31,	32,	33,	34,	35,	36	}, {	46,	47,	48,	49,	50,	51	} },
		{ {	61,	62,	63,	64,	65,	66	}, {	76,	77,	78,	79,	80,	81	} },
		{ {	91,	92,	93,	94,	95,	96	}, {	106,107,108,109,110,111	} },
		#ifdef ENABLE_WOLFMAN_CHARACTER
		{ {	170,	171,	172,	173,	174,	175	} }, // WOLF
		#endif
	};

	return SkillList[bJob][bSkillGroup - 1];
}
#endif

CharacterVectorInteractor::CharacterVectorInteractor(const CHARACTER_SET & r)
{
	using namespace std;
#if defined(__GNUC__) && !defined(__clang__) && !defined(CXX11_ENABLED)
	using namespace __gnu_cxx;
#endif

	reserve(r.size());
#if defined(__GNUC__) && !defined(__clang__) && !defined(CXX11_ENABLED)
	transform(r.begin(), r.end(), back_inserter(*this), identity<CHARACTER_SET::value_type>());
#else
	insert(end(), r.begin(), r.end());
#endif
	

	if (CHARACTER_MANAGER::instance().BeginPendingDestroy())
		m_bMyBegin = true;
}

CharacterVectorInteractor::~CharacterVectorInteractor()
{
	if (m_bMyBegin)
		CHARACTER_MANAGER::instance().FlushPendingDestroy();
}


#ifdef ENABLE_SPECIAL_AFFECT
void CHARACTER_MANAGER::AddSpecialAffect(DWORD pid, DWORD affect, DWORD aff)
{
	if (pid != 0)
	{
		LPCHARACTER ch = FindByPID(pid);
		
		if (!ch)
		{
			sys_err("AddSpecialAffect: Cannot find pid %u", pid);
			return;
		}
		
		ch->AddAffect(affect, POINT_NONE, 0, aff, INFINITE_AFFECT_DURATION, 0, true);
		m_map_pkSpecialAffectedChr[affect] = pid;		
	}
}

void CHARACTER_MANAGER::HandleSpecialAffects(DWORD* aPids)
{
	if (!aPids)
		return;
	
	std::map<DWORD, DWORD>::iterator it = m_map_pkSpecialAffectedChr.begin(), end = m_map_pkSpecialAffectedChr.end();
	
	for (; it != end; it++)
	{
		CHARACTER* pCh = FindByPID(it->second);
		if (pCh)
			pCh->RemoveAffect(it->first);		
	}
	
	m_map_pkSpecialAffectedChr.clear();
	
	AddSpecialAffect(aPids[0], AFFECT_SPECIAL_AFFECT_PLAYER_1ST, AFF_NONE);
	AddSpecialAffect(aPids[1], AFFECT_SPECIAL_AFFECT_PLAYER_2ND, AFF_NONE);
	AddSpecialAffect(aPids[2], AFFECT_SPECIAL_AFFECT_PLAYER_3RD, AFF_NONE);
}

void CHARACTER_MANAGER::AddSpecialAffect(LPCHARACTER ch)
{
	std::map<DWORD, DWORD>::iterator it = m_map_pkSpecialAffectedChr.begin(), end = m_map_pkSpecialAffectedChr.end();
	
	for (; it != end; it++)
	{
		if (it->second == ch->GetPlayerID())
		{
			ch->AddAffect(it->first, POINT_NONE, 0, /*(AFFECT_SPECIAL_AFFECT_PLAYER_3RD - it->first) + AFF_SPECIAL_AFFECT_PLAYER_1ST*/ AFF_NONE, INFINITE_AFFECT_DURATION, 0, true);
		}
	}	
}

#endif

#ifdef ENABLE_EVENT_MANAGER
#include "item_manager.h"
#include "item.h"
#include "desc_client.h"
#include "desc_manager.h"
void CHARACTER_MANAGER::ClearEventData()
{
	m_eventData.clear();
}
/*
BONUS_EVENT = 1,////DONE
DOUBLE_BOSS_LOOT_EVENT = 2,//DONE
DOUBLE_METIN_LOOT_EVENT = 3,//DONE
DOUBLE_MISSION_BOOK_EVENT = 4,//DONE
DUNGEON_COOLDOWN_EVENT = 5,//DONE
DUNGEON_TICKET_LOOT_EVENT = 6,//DONE
EMPIRE_WAR_EVENT = 7,
MOONLIGHT_EVENT = 8,//DONE
TOURNAMENT_EVENT = 9,
WHELL_OF_FORTUNE_EVENT = 10,
HALLOWEEN_EVENT = 11,
NPC_SEARCH_EVENT = 12,
*/

void CHARACTER_MANAGER::CheckBonusEvent(LPCHARACTER ch)
{
	const TEventManagerData* eventPtr = CheckEventIsActive(BONUS_EVENT);
	if (eventPtr)
	{
		if (eventPtr->empireFlag != 0)
			if (eventPtr->empireFlag != ch->GetEmpire())
				return;
		ch->ApplyPoint(eventPtr->value[0], eventPtr->value[1]);
	}
}
TEventManagerData* CHARACTER_MANAGER::CheckEventIsActive(BYTE eventIndex)
{
	time_t cur_Time = time(NULL);
	struct tm vKey = *localtime(&cur_Time);

	if (m_eventData.size())
	{
		for (auto it = m_eventData.begin(); it != m_eventData.end(); ++it)
		{
			if (it->second.size())
			{
				for (BYTE j = 0; j < it->second.size(); ++j)
				{
					TEventManagerData& eventData = it->second[j];
					if (eventData.eventIndex == eventIndex)
					{
						if (eventData.channelFlag != 0)
							if (eventData.channelFlag != g_bChannel)
								continue;
						if (cur_Time > eventData.startRealTime && cur_Time < eventData.endRealTime)
							return &eventData;
					}
				}
			}
		}
	}
	return NULL;
}
void CHARACTER_MANAGER::CheckEventForDrop(LPCHARACTER pkChr, LPCHARACTER pkKiller, std::vector<LPITEM>& vec_item)
{
	TEventManagerData* eventPtr = NULL;

	if (pkChr->GetMobRank() >= MOB_RANK_BOSS)
	{
		eventPtr = CheckEventIsActive(DOUBLE_BOSS_LOOT_EVENT);
		if (eventPtr)
		{
			int prob = number(1, 100);
			int success_prob = eventPtr->value[3];
			if (prob < success_prob)
			{
				std::vector<LPITEM> m_cache;
				LPITEM item = NULL;
				for (DWORD j = 0; j < vec_item.size(); ++j)
				{
					item = ITEM_MANAGER::Instance().CreateItem(vec_item[j]->GetVnum(), vec_item[j]->GetCount(), 0, true);
					if (item) m_cache.emplace_back(item);
				}
				for (DWORD j = 0; j < m_cache.size(); ++j)
					vec_item.emplace_back(m_cache[j]);
				m_cache.clear();
			}
		}
	}

	eventPtr = CheckEventIsActive(DOUBLE_METIN_LOOT_EVENT);
	if (eventPtr)
	{
		if (pkChr->IsStone())
		{
			int prob = number(1, 100);
			int success_prob = eventPtr->value[3];
			if (prob < success_prob)
			{
				std::vector<LPITEM> m_cache;
				LPITEM item = NULL;
				for (DWORD j = 0; j < vec_item.size(); ++j)
				{
					item = ITEM_MANAGER::Instance().CreateItem(vec_item[j]->GetVnum(), vec_item[j]->GetCount(), 0, true);
					if (item) m_cache.emplace_back(item);
				}
				for (DWORD j = 0; j < m_cache.size(); ++j)
					vec_item.emplace_back(m_cache[j]);
				m_cache.clear();
			}
		}
	}

	eventPtr = CheckEventIsActive(DOUBLE_MISSION_BOOK_EVENT);
	if (eventPtr)
	{
		int prob = number(1, 100);
		int success_prob = eventPtr->value[3];
		if (prob < success_prob)
		{
			std::vector<LPITEM> m_cache;
			LPITEM item = NULL;
			std::vector<DWORD> m_missionbook = {
				50300,
				469,
			};
			for (DWORD j = 0; j < vec_item.size(); ++j)
			{
				LPITEM checkItem = vec_item[j];
				auto it = std::find(m_missionbook.begin(), m_missionbook.end(), checkItem->GetVnum());
				if (it != m_missionbook.end())
				{

					item = ITEM_MANAGER::Instance().CreateItem(checkItem->GetVnum(), checkItem->GetCount(), 0, true);
					if (item) m_cache.emplace_back(item);
				}
			}
			for (DWORD j = 0; j < m_cache.size(); ++j)
				vec_item.emplace_back(m_cache[j]);
			m_cache.clear();
		}
	}

	eventPtr = CheckEventIsActive(DUNGEON_TICKET_LOOT_EVENT);
	if (eventPtr)
	{
		int prob = number(1, 100);
		int success_prob = eventPtr->value[3];
		if (prob < success_prob)
		{
			std::vector<LPITEM> m_cache;
			LPITEM item = NULL;
			std::vector<DWORD> m_missionbook = {
				50300,
				469,
			};
			for (DWORD j = 0; j < vec_item.size(); ++j)
			{
				LPITEM checkItem = vec_item[j];
				auto it = std::find(m_missionbook.begin(), m_missionbook.end(), checkItem->GetVnum());
				if (it != m_missionbook.end())
				{
					item = ITEM_MANAGER::Instance().CreateItem(checkItem->GetVnum(), checkItem->GetCount(), 0, true);
					if (item) m_cache.emplace_back(item);
				}
			}
			for (DWORD j = 0; j < m_cache.size(); ++j)
				vec_item.emplace_back(m_cache[j]);
			m_cache.clear();
		}
	}

	eventPtr = CheckEventIsActive(MOONLIGHT_EVENT);
	if (eventPtr)
	{
		int prob = number(1, 100);
		int success_prob = eventPtr->value[3];
		if (prob < success_prob)
		{
			DWORD itemVnum = 50011;
			LPITEM item = ITEM_MANAGER::Instance().CreateItem(itemVnum, 1, 0, true);
			if (item) vec_item.emplace_back(item);
		}
	}
}
void CHARACTER_MANAGER::SendDataPlayer(LPCHARACTER ch)
{
	time_t cur_Time = time(NULL);
	struct tm vKey = *localtime(&cur_Time);

	TPacketGCEventManager p;
	p.header = HEADER_GC_EVENT_MANAGER;	
	p.size = sizeof(p);
	TEMP_BUFFER buf;
	BYTE dayCount = m_eventData.size();
	buf.write(&dayCount, sizeof(BYTE));
	p.size += sizeof(BYTE);

	if (m_eventData.size())
	{
		for (auto it = m_eventData.begin(); it != m_eventData.end(); ++it) {
			p.size += sizeof(BYTE) + sizeof(BYTE) + (sizeof(TEventManagerDataClient) * it->second.size());

			BYTE dayIndex = it->first;
			buf.write(&dayIndex, sizeof(BYTE));
			BYTE dayEventCount = it->second.size();
			buf.write(&dayEventCount, sizeof(BYTE));

			std::vector<TEventManagerDataClient> m_vec;
			for (BYTE j = 0; j < it->second.size(); ++j)
			{
				const TEventManagerData& eventData= it->second[j];
				TEventManagerDataClient p;
				p.eventIndex = eventData.eventIndex;
				p.startRealTime = eventData.startRealTime-get_global_time();
				p.endRealTime = eventData.endRealTime-get_global_time();
				p.isAlreadyStart = CheckEventIsActive(eventData.eventIndex);
				strlcpy(p.endText, eventData.endText, sizeof(p.endText));
				strlcpy(p.startText, eventData.startText, sizeof(p.startText));
				p.empireFlag = eventData.empireFlag;
				p.channelFlag = eventData.channelFlag;
				thecore_memcpy(&p.value, &eventData.value, sizeof(p.value));
				m_vec.emplace_back(p);
			}
			buf.write(m_vec.data(), sizeof(TEventManagerDataClient) * dayEventCount);
		}
	}
	ch->GetDesc()->BufferedPacket(&p, sizeof(p));
	ch->GetDesc()->Packet(buf.read_peek(), buf.size());
}
void CHARACTER_MANAGER::SetEventStatus(const TEventManagerData& m_data, bool eventStatus)
{
	// eventStatus 1 - ACTIVE
	// eventStatus 0 - DEACTIVE
	std::map<BYTE, std::pair<std::string, std::string>> m_eventText = {
		{BONUS_EVENT,{"761","762"}},
		{DOUBLE_BOSS_LOOT_EVENT,{"763","764"}},
		{DOUBLE_METIN_LOOT_EVENT,{"765","766"}},
		{DOUBLE_MISSION_BOOK_EVENT,{"767","768"}},
		{DUNGEON_COOLDOWN_EVENT,{"769","770"}},
		{DUNGEON_TICKET_LOOT_EVENT,{"771","772"}},
		{MOONLIGHT_EVENT,{"773","774"}},
	};

	auto it = m_eventText.find(m_data.eventIndex);
	if (it != m_eventText.end())
	{
		if (m_data.channelFlag == 0)
		{
			if (eventStatus)
				SendNotice(it->second.first.c_str());
			else
				SendNotice(it->second.second.c_str());
		}
		else
		{
			if (eventStatus)
				BroadcastNotice(it->second.first.c_str());
			else
				BroadcastNotice(it->second.second.c_str());
		}
	}

	// CLEAR BONUS
	if (m_data.eventIndex == BONUS_EVENT)
	{
		const DESC_MANAGER::DESC_SET& c_ref_set = DESC_MANAGER::instance().GetClientSet();
		if (eventStatus)
		{
			for (auto it = c_ref_set.begin(); it != c_ref_set.end(); ++it)
			{
				LPCHARACTER ch = (*it)->GetCharacter();
				if (!ch)
					continue;

				if (m_data.empireFlag != 0)
					if (m_data.empireFlag != ch->GetEmpire())
						continue;

				ch->ComputePoints();
			}
		}
		else
		{
			for (auto it = c_ref_set.begin(); it != c_ref_set.end(); ++it)
			{
				LPCHARACTER ch = (*it)->GetCharacter();
				if (!ch)
					continue;
				if (m_data.empireFlag != 0)
					if (m_data.empireFlag != ch->GetEmpire())
						continue;
				long value = m_data.value[1];
				ch->ApplyPoint(m_data.value[0], -value);
				ch->ComputePoints();
			}
		}
	}
}
void CHARACTER_MANAGER::SetEventData(BYTE dayIndex, const std::vector<TEventManagerData>& m_data)
{
	auto it = m_eventData.find(dayIndex);
	if (it != m_eventData.end())
	{
		it->second.clear();
		for (DWORD j = 0; j < m_data.size(); ++j)
			it->second.emplace_back(m_data[j]);
	}
	else
	{
		m_eventData.emplace(dayIndex, m_data);
	}
}
#endif
