#include "stdafx.h"
#include "war_map.h"
#include "sectree_manager.h"
#include "char.h"
#include "char_manager.h"
#include "affect.h"
#include "item.h"
#include "config.h"
#include "desc.h"
#include "desc_manager.h"
#include "guild_manager.h"
#include "buffer_manager.h"
#include "db.h"
#include "packet.h"
#include "locale_service.h"
#include <boost/algorithm/string/replace.hpp>
#ifdef GUILD_WAR_COUNTER
#include "desc_client.h"
#endif

EVENTINFO(war_map_info)
{
	int iStep;
	CWarMap * pWarMap;

	war_map_info()
	: iStep( 0 )
	, pWarMap( 0 )
	{
	}
};

EVENTFUNC(war_begin_event)
{
	war_map_info* info = dynamic_cast<war_map_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "war_begin_event> <Factor> Null pointer" );
		return 0;
	}

	CWarMap * pMap = info->pWarMap;
	pMap->CheckWarEnd();
	return PASSES_PER_SEC(10);
}

EVENTFUNC(war_end_event)
{
	war_map_info* info = dynamic_cast<war_map_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "war_end_event> <Factor> Null pointer" );
		return 0;
	}

	CWarMap * pMap = info->pWarMap;

	if (info->iStep == 0)
	{
		++info->iStep;
		pMap->ExitAll();
		return PASSES_PER_SEC(5);
	}
#ifdef GUILD_WAR_COUNTER
	else if (info->iStep == 1)
	{
		++info->iStep;
		pMap->SaveCounterData();
		return PASSES_PER_SEC(5);
	}
#endif
	else
	{
		pMap->SetEndEvent(NULL);
		CWarMapManager::instance().DestroyWarMap(pMap);
		return 0;
	}
}

EVENTFUNC(war_timeout_event)
{
	war_map_info* info = dynamic_cast<war_map_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "war_timeout_event> <Factor> Null pointer" );
		return 0;
	}

	CWarMap * pMap = info->pWarMap;
	pMap->Timeout();
	return 0;
}

void CWarMap::STeamData::Initialize()
{
	dwID = 0;
	pkGuild = NULL;
	iMemberCount = 0;
	iUsePotionPrice = 0;
	iScore = 0;
	pkChrFlag = NULL;
	pkChrFlagBase = NULL;

	set_pidJoiner.clear();
}

CWarMap::CWarMap(long lMapIndex, const TGuildWarInfo & r_info, TWarMapInfo * pkWarMapInfo, DWORD dwGuildID1, DWORD dwGuildID2
#ifdef __IMPROVED_GUILD_WAR__
	, int iMaxPlayer, int iMaxScore, DWORD flags, int custom_map_index
#endif
#ifdef GUILD_WAR_COUNTER
	,DWORD warDBID
#endif
)
{
#ifdef GUILD_WAR_COUNTER
	warID=warDBID;
#endif
	m_kMapInfo = *pkWarMapInfo;
	m_kMapInfo.lMapIndex = lMapIndex;

	memcpy(&m_WarInfo, &r_info, sizeof(TGuildWarInfo));

	m_TeamData[0].Initialize();
	m_TeamData[0].dwID = dwGuildID1;
	m_TeamData[0].pkGuild = CGuildManager::instance().TouchGuild(dwGuildID1);
#ifdef __IMPROVED_GUILD_WAR__
	m_TeamData[0].iMaxPlayer = iMaxPlayer;
	m_TeamData[0].iMaxScore = iMaxScore;
	m_TeamData[0].flags = flags;
	m_TeamData[0].custom_map_index = custom_map_index;
#endif

	m_TeamData[1].Initialize();
	m_TeamData[1].dwID = dwGuildID2;
	m_TeamData[1].pkGuild = CGuildManager::instance().TouchGuild(dwGuildID2);
#ifdef __IMPROVED_GUILD_WAR__
	m_TeamData[1].iMaxPlayer = iMaxPlayer;
	m_TeamData[1].iMaxScore = iMaxScore;
	m_TeamData[1].flags = flags;
	m_TeamData[1].custom_map_index = custom_map_index;
#endif
	m_iObserverCount = 0;

	war_map_info* info = AllocEventInfo<war_map_info>();
	info->pWarMap = this;

	SetBeginEvent(event_create(war_begin_event, info, PASSES_PER_SEC(60)));
	m_pkEndEvent = NULL;
	m_pkTimeoutEvent = NULL;
	m_pkResetFlagEvent = NULL;
	m_bTimeout = false;
	m_dwStartTime = get_dword_time();
	m_bEnded = false;

	if (GetType() == WAR_MAP_TYPE_FLAG)
	{
		AddFlagBase(0);
		AddFlagBase(1);
		AddFlag(0);
		AddFlag(1);
	}
}

CWarMap::~CWarMap()
{
	event_cancel(&m_pkBeginEvent);
	event_cancel(&m_pkEndEvent);
	event_cancel(&m_pkTimeoutEvent);
	event_cancel(&m_pkResetFlagEvent);

	sys_log(0, "WarMap::~WarMap : map index %d", GetMapIndex());

#ifdef GUILD_WAR_COUNTER
	for (itertype(m_set_pkChr) it = m_set_pkChr.begin(); it != m_set_pkChr.end(); ++it)
	{
		LPCHARACTER ch = CHARACTER_MANAGER::Instance().FindByPID(it->first);
		if (ch && ch->GetDesc())
		{
			sys_log(0, "WarMap::~WarMap : disconnecting %s", ch->GetName());
			DESC_MANAGER::instance().DestroyDesc(ch->GetDesc());
		}
	}
#else
	itertype(m_set_pkChr) it = m_set_pkChr.begin();
	while (it != m_set_pkChr.end())
	{
		LPCHARACTER ch = *(it++);
		if (ch->GetDesc())
		{
			sys_log(0, "WarMap::~WarMap : disconnecting %s", ch->GetName());
			DESC_MANAGER::instance().DestroyDesc(ch->GetDesc());
		}
	}
#endif
	m_set_pkChr.clear();
}

void CWarMap::SetBeginEvent(LPEVENT pkEv)
{
	if (m_pkBeginEvent != NULL) {
		event_cancel(&m_pkBeginEvent);
	}
	if (pkEv != NULL) {
		m_pkBeginEvent = pkEv;
	}
}

void CWarMap::SetEndEvent(LPEVENT pkEv)
{
	if (m_pkEndEvent != NULL) {
		event_cancel(&m_pkEndEvent);
	}
	if (pkEv != NULL) {
		m_pkEndEvent = pkEv;
	}
}

void CWarMap::SetTimeoutEvent(LPEVENT pkEv)
{
	if (m_pkTimeoutEvent != NULL) {
		event_cancel(&m_pkTimeoutEvent);
	}
	if (pkEv != NULL) {
		m_pkTimeoutEvent = pkEv;
	}
}

void CWarMap::SetResetFlagEvent(LPEVENT pkEv)
{
	if (m_pkResetFlagEvent != NULL) {
		event_cancel(&m_pkResetFlagEvent);
	}
	if (pkEv != NULL) {
		m_pkResetFlagEvent = pkEv;
	}
}

bool CWarMap::GetTeamIndex(DWORD dwGuildID, BYTE & bIdx)
{
	if (m_TeamData[0].dwID == dwGuildID)
	{
		bIdx = 0;
		return true;
	}
	else if (m_TeamData[1].dwID == dwGuildID)
	{
		bIdx = 1;
		return true;
	}

	return false;
}

DWORD CWarMap::GetGuildID(BYTE bIdx)
{
	assert(bIdx < 2);
	return m_TeamData[bIdx].dwID;
}
#ifdef __IMPROVED_GUILD_WAR__
int CWarMap::GetCurrentPlayer(BYTE bIdx)
{
	assert(bIdx < 2);
	return m_TeamData[bIdx].iMemberCount;
}

int CWarMap::GetMaxPlayer(BYTE bIdx)
{
	assert(bIdx < 2);
	return m_TeamData[bIdx].iMaxPlayer;
}

int CWarMap::GetCurrentScore(BYTE bIdx)
{
	assert(bIdx < 2);
	return m_TeamData[bIdx].iScore;
}

int CWarMap::GetMaxScore(BYTE bIdx)
{
	assert(bIdx < 2);
	return m_TeamData[bIdx].iMaxScore;
}

DWORD CWarMap::GetWarFlags(BYTE bIdx)
{
	assert(bIdx < 2);
	return m_TeamData[bIdx].flags;
}

int CWarMap::GetCustomMapIndex(BYTE bIdx)
{
	assert(bIdx < 2);
	return m_TeamData[bIdx].custom_map_index;
}
#endif
CGuild * CWarMap::GetGuild(BYTE bIdx)
{
	return m_TeamData[bIdx].pkGuild;
}

long CWarMap::GetMapIndex()
{
	return m_kMapInfo.lMapIndex;
}

BYTE CWarMap::GetType()
{
	return m_kMapInfo.bType;
}

DWORD CWarMap::GetGuildOpponent(LPCHARACTER ch)
{
	if (ch->GetGuild())
	{
		DWORD gid = ch->GetGuild()->GetID();
		BYTE idx;

		if (GetTeamIndex(gid, idx))
			return m_TeamData[!idx].dwID;
	}
	return 0;
}

DWORD CWarMap::GetWinnerGuild()
{
	DWORD win_gid = 0;

	if (m_TeamData[1].iScore > m_TeamData[0].iScore)
	{
		win_gid = m_TeamData[1].dwID;
	}
	else if (m_TeamData[0].iScore > m_TeamData[1].iScore)
	{
		win_gid = m_TeamData[0].dwID;
	}

	return (win_gid);
}

void CWarMap::UsePotion(LPCHARACTER ch, LPITEM item)
{
	if (m_pkEndEvent)
		return;

	if (ch->IsObserverMode())
		return;

	if (!ch->GetGuild())
		return;

	if (!item->GetProto())
		return;

	int iPrice = item->GetProto()->dwGold;

	DWORD gid = ch->GetGuild()->GetID();

	if (gid == m_TeamData[0].dwID)
		m_TeamData[0].iUsePotionPrice += iPrice;
	else if (gid == m_TeamData[1].dwID)
		m_TeamData[1].iUsePotionPrice += iPrice;
}

int CWarMap::STeamData::GetAccumulatedJoinerCount()
{
	return set_pidJoiner.size();
}

int CWarMap::STeamData::GetCurJointerCount()
{
	return iMemberCount;
}

void CWarMap::STeamData::AppendMember(LPCHARACTER ch)
{
	set_pidJoiner.insert(ch->GetPlayerID());
	++iMemberCount;
}

void CWarMap::STeamData::RemoveMember(LPCHARACTER ch)
{
	// set_pidJoiner 는 누적 인원을 계산하기 때문에 제거하지 않는다
	--iMemberCount;
}

#ifndef GUILD_WAR_COUNTER
struct FSendUserCount
{
	char buf1[30];
	char buf2[128];

	FSendUserCount(DWORD g1, int g1_count, DWORD g2, int g2_count, int observer)
	{
		snprintf(buf1, sizeof(buf1), "ObserverCount %d", observer);
		snprintf(buf2, sizeof(buf2), "WarUC %u %d %u %d %d", g1, g1_count, g2, g2_count, observer);
	}

	void operator() (LPCHARACTER ch)
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, buf1);
		ch->ChatPacket(CHAT_TYPE_COMMAND, buf2);
	}
};
#endif

void CWarMap::UpdateUserCount()
{
#ifdef GUILD_WAR_COUNTER
	char buf1[30];
	char buf2[128];
	snprintf(buf1, sizeof(buf1), "ObserverCount %d", m_iObserverCount);
	snprintf(buf2, sizeof(buf2), "WarUC %u %d %u %d %d", m_TeamData[0].dwID, m_TeamData[0].GetCurJointerCount(), m_TeamData[1].dwID, m_TeamData[1].GetCurJointerCount(), m_iObserverCount);
	for (itertype(m_set_pkChr) it = m_set_pkChr.begin(); it != m_set_pkChr.end(); ++it)
	{
		LPCHARACTER ch = CHARACTER_MANAGER::Instance().FindByPID(it->first);
		if (ch)
		{
			ch->ChatPacket(CHAT_TYPE_COMMAND, buf1);
			ch->ChatPacket(CHAT_TYPE_COMMAND, buf2);
		}
	}
#else
	FSendUserCount f(
			m_TeamData[0].dwID,
			m_TeamData[0].GetAccumulatedJoinerCount(),
			m_TeamData[1].dwID,
			m_TeamData[1].GetAccumulatedJoinerCount(),
			m_iObserverCount);
	std::for_each(m_set_pkChr.begin(), m_set_pkChr.end(), f);
#endif
}

void CWarMap::IncMember(LPCHARACTER ch)
{
	if (!ch->IsPC())
		return;

	sys_log(0, "WarMap::IncMember");
	DWORD gid = 0;

	if (ch->GetGuild())
		gid = ch->GetGuild()->GetID();

	bool isWarMember = ch->GetQuestFlag("war.is_war_member") > 0 ? true : false;

	if (isWarMember && gid != m_TeamData[0].dwID && gid != m_TeamData[1].dwID)
	{
		ch->SetQuestFlag("war.is_war_member", 0);
		isWarMember = false;
	}

	if (isWarMember)
	{
		if (gid == m_TeamData[0].dwID)
		{
			m_TeamData[0].AppendMember(ch);
#ifdef GUILD_WAR_COUNTER
			RegisterStatics(ch);
#endif

		}
		else if (gid == m_TeamData[1].dwID)
		{
			m_TeamData[1].AppendMember(ch);
#ifdef GUILD_WAR_COUNTER
			RegisterStatics(ch);
#endif

		}

		event_cancel(&m_pkTimeoutEvent);

		sys_log(0, "WarMap +m %u(cur:%d, acc:%d) vs %u(cur:%d, acc:%d)",
				m_TeamData[0].dwID, m_TeamData[0].GetCurJointerCount(), m_TeamData[0].GetAccumulatedJoinerCount(),
				m_TeamData[1].dwID, m_TeamData[1].GetCurJointerCount(), m_TeamData[1].GetAccumulatedJoinerCount());
	}
	else
	{
		++m_iObserverCount;
		sys_log(0, "WarMap +o %d", m_iObserverCount);
		ch->SetObserverMode(true);
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Puoi partecipare alla guerra tra Gilde in modalita' spettatore."));
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Quando scegli il personaggio comparira' l'icona."));		
	}

	LPDESC d = ch->GetDesc();
	SendWarPacket(d);

#ifdef GUILD_WAR_COUNTER
	m_set_pkChr.emplace(ch->GetPlayerID(),false);
	std::vector<war_static_ptr> list;
	UpdateStatic(ch, GUILD_STATIC_LOAD, list);
	if (!isWarMember)
	{
		std::vector<war_static_ptr> list;
		UpdateStatic(ch, GUILD_STATIC_UPDATE_OBSERVER, list);
	}
#else
	m_set_pkChr.insert(ch);
	SendScorePacket(0, d);
	SendScorePacket(1, d);
	UpdateUserCount();
#endif
}

void CWarMap::DecMember(LPCHARACTER ch)
{
	if (!ch->IsPC())
		return;

	sys_log(0, "WarMap::DecMember");
	DWORD gid = 0;

	if (ch->GetGuild())
		gid = ch->GetGuild()->GetID();

	if (!ch->IsObserverMode())
	{
		if (gid == m_TeamData[0].dwID)
			m_TeamData[0].RemoveMember(ch);
		else if (gid == m_TeamData[1].dwID)
			m_TeamData[1].RemoveMember(ch);

		if (m_kMapInfo.bType == WAR_MAP_TYPE_FLAG)
		{
			CAffect * pkAff = ch->FindAffect(AFFECT_WAR_FLAG);

			if (pkAff)
			{
				BYTE idx;

				if (GetTeamIndex(pkAff->lApplyValue, idx))
					AddFlag(idx, ch->GetX(), ch->GetY());

				ch->RemoveAffect(AFFECT_WAR_FLAG);
			}
		}

		sys_log(0, "WarMap -m %u(cur:%d, acc:%d) vs %u(cur:%d, acc:%d)",
				m_TeamData[0].dwID, m_TeamData[0].GetCurJointerCount(), m_TeamData[0].GetAccumulatedJoinerCount(),
				m_TeamData[1].dwID, m_TeamData[1].GetCurJointerCount(), m_TeamData[1].GetAccumulatedJoinerCount());

		CheckWarEnd();
		ch->SetQuestFlag("war.is_war_member", 0);
	}
	else
	{
		--m_iObserverCount;

		sys_log(0, "WarMap -o %d", m_iObserverCount);
		ch->SetObserverMode(false);
#ifdef GUILD_WAR_COUNTER
		std::vector<war_static_ptr> m_list;
		UpdateStatic(NULL, GUILD_STATIC_UPDATE_OBSERVER, m_list);
#endif
	}
#ifdef GUILD_WAR_COUNTER
	itertype(war_static) it = war_static.find(ch->GetPlayerID());
	if (it != war_static.end())
	{
		it->second.online = false;
		std::vector<war_static_ptr> m_list;
		m_list.clear();
		m_list.emplace_back(it->second);
		UpdateStatic(NULL, GUILD_STATIC_UPDATE_ONLINE, m_list);
	}
	m_set_pkChr.erase(ch->GetPlayerID());
#else
	UpdateUserCount();
	m_set_pkChr.erase(ch);
#endif
}

#ifndef GUILD_WAR_COUNTER
struct FExitGuildWar
{
	void operator() (LPCHARACTER ch)
	{
		if (ch->IsPC())
		{
			ch->ExitToSavedLocation();
		}
	}
};
#endif

void CWarMap::ExitAll()
{
#ifdef GUILD_WAR_COUNTER
	for (itertype(m_set_pkChr) it = m_set_pkChr.begin(); it != m_set_pkChr.end(); ++it)
	{
		LPCHARACTER ch = CHARACTER_MANAGER::Instance().FindByPID(it->first);
		if (ch)
			ch->ExitToSavedLocation();
	}
#else
	FExitGuildWar f;
	std::for_each(m_set_pkChr.begin(), m_set_pkChr.end(), f);
#endif
}

void CWarMap::CheckWarEnd()
{
	if (m_bEnded)
		return;

	if (m_TeamData[0].iMemberCount == 0 || m_TeamData[1].iMemberCount == 0)
	{
		if (m_bTimeout)
			return;

		if (m_pkTimeoutEvent)
			return;

		Notice(LC_TEXT("Non ci sono avversari."));
		Notice(LC_TEXT("Se non ci sono avversari la guerra tra Gilde viene automaticamente terminata."));

		sys_log(0, "CheckWarEnd: Timeout begin %u vs %u", m_TeamData[0].dwID, m_TeamData[1].dwID);

		war_map_info* info = AllocEventInfo<war_map_info>();
		info->pWarMap = this;

		SetTimeoutEvent(event_create(war_timeout_event, info, PASSES_PER_SEC(60)));
	}
	else
		CheckScore();
}

int CWarMap::GetRewardGold(BYTE bWinnerIdx)
{
	int iRewardGold = m_WarInfo.iWarPrice;
	iRewardGold += (m_TeamData[bWinnerIdx].iUsePotionPrice * m_WarInfo.iWinnerPotionRewardPctToWinner) / 100;
	iRewardGold += (m_TeamData[bWinnerIdx ? 0 : 1].iUsePotionPrice * m_WarInfo.iLoserPotionRewardPctToWinner) / 100;
	return iRewardGold;
}

void CWarMap::Draw()
{
	CGuildManager::instance().RequestWarOver(m_TeamData[0].dwID, m_TeamData[1].dwID, 0, 0
#ifdef __IMPROVED_GUILD_WAR__
		, 0, 0, 0, 0
#endif
	);
}

void CWarMap::Timeout()
{
	SetTimeoutEvent(NULL);

	if (m_bTimeout)
		return;

	if (m_bEnded)
		return;

	DWORD dwWinner = 0;
	DWORD dwLoser = 0;
	int iRewardGold = 0;

	if (get_dword_time() - m_dwStartTime < 60000 * 5)
	{
		Notice(LC_TEXT("Siccome la guerra tra Gilde e' terminata anticipatamente, il risultato e un pareggio."));
		dwWinner = 0;
		dwLoser = 0;
	}
	else
	{
		int iWinnerIdx = -1;

		if (m_TeamData[0].iMemberCount == 0)
			iWinnerIdx = 1;
		else if (m_TeamData[1].iMemberCount == 0)
			iWinnerIdx = 0;

		if (iWinnerIdx == -1)
		{
			dwWinner = GetWinnerGuild();

			if (dwWinner == m_TeamData[0].dwID)
			{
				iRewardGold = GetRewardGold(0);
				dwLoser = m_TeamData[1].dwID;
			}
			else if (dwWinner == m_TeamData[1].dwID)
			{
				iRewardGold = GetRewardGold(1);
				dwLoser = m_TeamData[0].dwID;
			}

			sys_err("WarMap: member count is not zero, guild1 %u %d guild2 %u %d, winner %u",
					m_TeamData[0].dwID, m_TeamData[0].iMemberCount,
					m_TeamData[1].dwID, m_TeamData[1].iMemberCount,
					dwWinner);
		}
		else
		{
			dwWinner = m_TeamData[iWinnerIdx].dwID;
			dwLoser = m_TeamData[iWinnerIdx == 0 ? 1 : 0].dwID;

			iRewardGold = GetRewardGold(iWinnerIdx);
		}
	}

	sys_log(0, "WarMap: Timeout %u %u winner %u loser %u reward %d map %d",
			m_TeamData[0].dwID, m_TeamData[1].dwID, dwWinner, dwLoser, iRewardGold, m_kMapInfo.lMapIndex);

	if (dwWinner)
		CGuildManager::instance().RequestWarOver(dwWinner, dwLoser, dwWinner, iRewardGold
#ifdef __IMPROVED_GUILD_WAR__
			, m_TeamData[0].iMaxPlayer, m_TeamData[0].iMaxScore, m_TeamData[0].flags, m_TeamData[0].custom_map_index
#endif
		);
	else
		CGuildManager::instance().RequestWarOver(m_TeamData[0].dwID, m_TeamData[1].dwID, dwWinner, iRewardGold
#ifdef __IMPROVED_GUILD_WAR__
			, m_TeamData[0].iMaxPlayer, m_TeamData[0].iMaxScore, m_TeamData[0].flags, m_TeamData[0].custom_map_index
#endif
		);

	m_bTimeout = true;
}

#ifndef GUILD_WAR_COUNTER
namespace
{
	struct FPacket
	{
		FPacket(const void * p, int size) : m_pvData(p), m_iSize(size)
		{
		}

		void operator () (LPCHARACTER ch)
		{
			ch->GetDesc()->Packet(m_pvData, m_iSize);
		}

		const void * m_pvData;
		int m_iSize;
	};

	struct FNotice
	{
		FNotice(const char * psz) : m_psz(psz)
		{
		}

		void operator() (LPCHARACTER ch)
		{
			ch->ChatPacket(CHAT_TYPE_NOTICE, "%s", m_psz);
		}

		const char * m_psz;
	};
};
#endif

void CWarMap::Notice(const char * psz)
{
#ifdef GUILD_WAR_COUNTER
	for (itertype(m_set_pkChr) it = m_set_pkChr.begin(); it != m_set_pkChr.end(); ++it)
	{
		LPCHARACTER ch = CHARACTER_MANAGER::Instance().FindByPID(it->first);
		if (ch)
			ch->ChatPacket(CHAT_TYPE_NOTICE, "%s", psz);
	}
#else
	FNotice f(psz);
	std::for_each(m_set_pkChr.begin(), m_set_pkChr.end(), f);
#endif
}

#ifdef GUILD_WAR_COUNTER
void CWarMap::Packet(const void* p, int size, bool broadcast)
{
	for (itertype(m_set_pkChr) it = m_set_pkChr.begin(); it != m_set_pkChr.end(); ++it)
	{
		if (broadcast && !it->second)
			continue;
		LPCHARACTER ch = CHARACTER_MANAGER::Instance().FindByPID(it->first);
		if (ch)
			ch->GetDesc()->Packet(p, size);
	}
}
#else
void CWarMap::Packet(const void * p, int size)
{
	FPacket f(p, size);
	std::for_each(m_set_pkChr.begin(), m_set_pkChr.end(), f);
}
#endif

void CWarMap::SendWarPacket(LPDESC d)
{
	TPacketGCGuild pack;
	TPacketGCGuildWar pack2;

	pack.header		= HEADER_GC_GUILD;
	pack.subheader	= GUILD_SUBHEADER_GC_WAR;
	pack.size		= sizeof(pack) + sizeof(pack2);

	pack2.dwGuildSelf	= m_TeamData[0].dwID;
	pack2.dwGuildOpp	= m_TeamData[1].dwID;
	pack2.bType		= CGuildManager::instance().TouchGuild(m_TeamData[0].dwID)->GetGuildWarType(m_TeamData[1].dwID);
	pack2.bWarState	= CGuildManager::instance().TouchGuild(m_TeamData[0].dwID)->GetGuildWarState(m_TeamData[1].dwID);
#ifdef __IMPROVED_GUILD_WAR__
	pack2.iMaxPlayer = m_TeamData[0].iMaxPlayer;
	pack2.iMaxScore = m_TeamData[0].iMaxScore;
	pack2.flags = m_TeamData[0].flags;
#endif
	d->BufferedPacket(&pack, sizeof(pack));
	d->Packet(&pack2, sizeof(pack2));
}

void CWarMap::SendScorePacket(BYTE bIdx, LPDESC d)
{
	TPacketGCGuild p;

	p.header = HEADER_GC_GUILD;
	p.subheader = GUILD_SUBHEADER_GC_WAR_SCORE;
	p.size = sizeof(p) + sizeof(DWORD) + sizeof(DWORD) + sizeof(long);

	TEMP_BUFFER buf;
	buf.write(&p, sizeof(p));
	buf.write(&m_TeamData[bIdx].dwID, sizeof(DWORD));
	buf.write(&m_TeamData[bIdx ? 0 : 1].dwID, sizeof(DWORD));
	buf.write(&m_TeamData[bIdx].iScore, sizeof(long));

	if (d)
		d->Packet(buf.read_peek(), buf.size());
	else
		Packet(buf.read_peek(), buf.size());
}

void CWarMap::UpdateScore(DWORD g1, int score1, DWORD g2, int score2)
{
	BYTE idx;

	if (GetTeamIndex(g1, idx))
	{
		if (m_TeamData[idx].iScore != score1)
		{
			m_TeamData[idx].iScore = score1;
#ifndef GUILD_WAR_COUNTER
			SendScorePacket(idx);
#endif
		}
	}

	if (GetTeamIndex(g2, idx))
	{
		if (m_TeamData[idx].iScore != score2)
		{
			m_TeamData[idx].iScore = score2;
#ifndef GUILD_WAR_COUNTER
			SendScorePacket(idx);
#endif
		}
	}

	CheckScore();
}

bool CWarMap::CheckScore()
{
	if (m_bEnded)
		return true;

	// 30초 이후 부터 확인한다.
	if (get_dword_time() - m_dwStartTime < 30000)
		return false;

	// 점수가 같으면 체크하지 않는다.
	if (m_TeamData[0].iScore == m_TeamData[1].iScore)
		return false;

#ifdef __IMPROVED_GUILD_WAR__
	int iEndScore = m_TeamData[0].iMaxScore;
#else
	int iEndScore = m_WarInfo.iEndScore;

	if (test_server) iEndScore /= 10;
#endif

	DWORD dwWinner;
	DWORD dwLoser;

	if (m_TeamData[0].iScore >= iEndScore)
	{
		dwWinner = m_TeamData[0].dwID;
		dwLoser = m_TeamData[1].dwID;
	}
	else if (m_TeamData[1].iScore >= iEndScore)
	{
		dwWinner = m_TeamData[1].dwID;
		dwLoser = m_TeamData[0].dwID;
	}
	else
		return false;

	int iRewardGold = 0;

	if (dwWinner == m_TeamData[0].dwID)
		iRewardGold = GetRewardGold(0);
	else if (dwWinner == m_TeamData[1].dwID)
		iRewardGold = GetRewardGold(1);

	sys_log(0, "WarMap::CheckScore end score %d guild1 %u score guild2 %d %u score %d winner %u reward %d",
			iEndScore,
			m_TeamData[0].dwID,
			m_TeamData[0].iScore,
			m_TeamData[1].dwID,
			m_TeamData[1].iScore,
			dwWinner,
			iRewardGold);


	CGuildManager::instance().RequestWarOver(dwWinner, dwLoser, dwWinner, iRewardGold
#ifdef __IMPROVED_GUILD_WAR__
		, m_TeamData[0].iMaxPlayer, m_TeamData[0].iMaxScore, m_TeamData[0].flags, m_TeamData[0].custom_map_index
#endif
	);
	return true;
}

bool CWarMap::SetEnded()
{
	sys_log(0, "WarMap::SetEnded %d", m_kMapInfo.lMapIndex);

	if (m_pkEndEvent)
		return false;

	if (m_TeamData[0].pkChrFlag)
	{
		M2_DESTROY_CHARACTER(m_TeamData[0].pkChrFlag);
		m_TeamData[0].pkChrFlag = NULL;
	}

	if (m_TeamData[0].pkChrFlagBase)
	{
		M2_DESTROY_CHARACTER(m_TeamData[0].pkChrFlagBase);
		m_TeamData[0].pkChrFlagBase = NULL;
	}

	if (m_TeamData[1].pkChrFlag)
	{
		M2_DESTROY_CHARACTER(m_TeamData[1].pkChrFlag);
		m_TeamData[1].pkChrFlag = NULL;
	}

	if (m_TeamData[1].pkChrFlagBase)
	{
		M2_DESTROY_CHARACTER(m_TeamData[1].pkChrFlagBase);
		m_TeamData[1].pkChrFlagBase = NULL;
	}

	event_cancel(&m_pkResetFlagEvent);
	m_bEnded = true;

	war_map_info* info = AllocEventInfo<war_map_info>();

	info->pWarMap = this;
	info->iStep = 0;
	SetEndEvent(event_create(war_end_event, info, PASSES_PER_SEC(10)));
	return true;
}

void CWarMap::OnKill(LPCHARACTER killer, LPCHARACTER ch)
{
	if (m_bEnded)
		return;

	DWORD dwKillerGuild = 0;
	DWORD dwDeadGuild = 0;

	if (killer->GetGuild())
		dwKillerGuild = killer->GetGuild()->GetID();

	if (ch->GetGuild())
		dwDeadGuild = ch->GetGuild()->GetID();

	BYTE idx;

	sys_log(0, "WarMap::OnKill %u %u", dwKillerGuild, dwDeadGuild);

	if (!GetTeamIndex(dwKillerGuild, idx))
		return;

	if (!GetTeamIndex(dwDeadGuild, idx))
		return;

	switch (m_kMapInfo.bType)
	{
		case WAR_MAP_TYPE_NORMAL:
			SendGuildWarScore(dwKillerGuild, dwDeadGuild, 1, ch->GetLevel());
#ifdef GUILD_WAR_COUNTER
			SendKillNotice(killer,ch);
#endif
			break;

		case WAR_MAP_TYPE_FLAG:
			{
				CAffect * pkAff = ch->FindAffect(AFFECT_WAR_FLAG);

				if (pkAff)
				{
					if (GetTeamIndex(pkAff->lApplyValue, idx))
						AddFlag(idx, ch->GetX(), ch->GetY());

					ch->RemoveAffect(AFFECT_WAR_FLAG);
				}
			}
			break;

		default:
			sys_err("unknown war map type %u index %d", m_kMapInfo.bType, m_kMapInfo.lMapIndex);
			break;
	}
}

void CWarMap::AddFlagBase(BYTE bIdx, DWORD x, DWORD y)
{
	if (m_bEnded)
		return;

	assert(bIdx < 2);

	TeamData & r = m_TeamData[bIdx];

	if (r.pkChrFlagBase)
		return;

	if (x == 0)
	{
		x = m_kMapInfo.posStart[bIdx].x;
		y = m_kMapInfo.posStart[bIdx].y;
	}

	r.pkChrFlagBase = CHARACTER_MANAGER::instance().SpawnMob(warmap::WAR_FLAG_BASE_VNUM, m_kMapInfo.lMapIndex, x, y, 0);
	sys_log(0, "WarMap::AddFlagBase %u %p id %u", bIdx, get_pointer(r.pkChrFlagBase), r.dwID);

	r.pkChrFlagBase->SetPoint(POINT_STAT, r.dwID);
	r.pkChrFlagBase->SetWarMap(this);
}

void CWarMap::AddFlag(BYTE bIdx, DWORD x, DWORD y)
{
	if (m_bEnded)
		return;

	assert(bIdx < 2);

	TeamData & r = m_TeamData[bIdx];

	if (r.pkChrFlag)
		return;

	if (x == 0)
	{
		x = m_kMapInfo.posStart[bIdx].x;
		y = m_kMapInfo.posStart[bIdx].y;
	}

	r.pkChrFlag = CHARACTER_MANAGER::instance().SpawnMob(bIdx == 0 ? warmap::WAR_FLAG_VNUM0 : warmap::WAR_FLAG_VNUM1, m_kMapInfo.lMapIndex, x, y, 0);
	sys_log(0, "WarMap::AddFlag %u %p id %u", bIdx, get_pointer(r.pkChrFlag), r.dwID);

	r.pkChrFlag->SetPoint(POINT_STAT, r.dwID);
	r.pkChrFlag->SetWarMap(this);
}

void CWarMap::RemoveFlag(BYTE bIdx)
{
	assert(bIdx < 2);

	TeamData & r = m_TeamData[bIdx];

	if (!r.pkChrFlag)
		return;

	sys_log(0, "WarMap::RemoveFlag %u %p", bIdx, get_pointer(r.pkChrFlag));

	r.pkChrFlag->Dead(NULL, true);
	r.pkChrFlag = NULL;
}

bool CWarMap::IsFlagOnBase(BYTE bIdx)
{
	assert(bIdx < 2);

	TeamData & r = m_TeamData[bIdx];

	if (!r.pkChrFlag)
		return false;

	const PIXEL_POSITION & pos = r.pkChrFlag->GetXYZ();

	if (pos.x == m_kMapInfo.posStart[bIdx].x && pos.y == m_kMapInfo.posStart[bIdx].y)
		return true;

	return false;
}

EVENTFUNC(war_reset_flag_event)
{
	war_map_info* info = dynamic_cast<war_map_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "war_reset_flag_event> <Factor> Null pointer" );
		return 0;
	}

	CWarMap * pMap = info->pWarMap;

	pMap->AddFlag(0);
	pMap->AddFlag(1);

	pMap->SetResetFlagEvent(NULL);
	return 0;
}

#ifndef GUILD_WAR_COUNTER
struct FRemoveFlagAffect
{
	void operator() (LPCHARACTER ch)
	{
		if (ch->FindAffect(AFFECT_WAR_FLAG))
			ch->RemoveAffect(AFFECT_WAR_FLAG);
	}
};
#endif

void CWarMap::ResetFlag()
{
	if (m_kMapInfo.bType != WAR_MAP_TYPE_FLAG)
		return;

	if (m_pkResetFlagEvent)
		return;

	if (m_bEnded)
		return;

#ifdef GUILD_WAR_COUNTER
	for (itertype(m_set_pkChr) it = m_set_pkChr.begin(); it != m_set_pkChr.end(); ++it)
	{
		LPCHARACTER ch = CHARACTER_MANAGER::Instance().FindByPID(it->first);
		if (ch)
		{
			if (ch->FindAffect(AFFECT_WAR_FLAG))
				ch->RemoveAffect(AFFECT_WAR_FLAG);
		}
	}
#else
	FRemoveFlagAffect f;
	std::for_each(m_set_pkChr.begin(), m_set_pkChr.end(), f);
#endif

	RemoveFlag(0);
	RemoveFlag(1);

	war_map_info* info = AllocEventInfo<war_map_info>();

	info->pWarMap = this;
	info->iStep = 0;
	SetResetFlagEvent(event_create(war_reset_flag_event, info, PASSES_PER_SEC(10)));
}

/////////////////////////////////////////////////////////////////////////////////
// WarMapManager
/////////////////////////////////////////////////////////////////////////////////
CWarMapManager::CWarMapManager()
{
}

CWarMapManager::~CWarMapManager()
{
	for( std::map<long, TWarMapInfo *>::const_iterator iter = m_map_kWarMapInfo.begin() ; iter != m_map_kWarMapInfo.end() ; ++iter )
	{
		M2_DELETE(iter->second);
	}

	m_map_kWarMapInfo.clear();
}

bool CWarMapManager::LoadWarMapInfo(const char * c_pszFileName)
{
	TWarMapInfo * k;

	k = M2_NEW TWarMapInfo;
	k->bType = WAR_MAP_TYPE_NORMAL;

	k->lMapIndex = 110;
	k->posStart[0].x = 175 * 100 + 32000;
	k->posStart[0].y = 240 * 100 + 0;
	k->posStart[1].x = 326 * 100 + 32000;
	k->posStart[1].y = 239 * 100 + 0;
	k->posStart[2].x = 141 * 100 + 32000;
	k->posStart[2].y = 117 * 100 + 0;

	m_map_kWarMapInfo.insert(std::make_pair(k->lMapIndex, k));

	k = M2_NEW TWarMapInfo;
	k->bType = WAR_MAP_TYPE_FLAG;

	k->lMapIndex = 111;
	k->posStart[0].x = 68 * 100 + 57600;
	k->posStart[0].y = 69 * 100 + 0;
	k->posStart[1].x = 171 * 100 + 57600;
	k->posStart[1].y = 182 * 100 + 0;
	k->posStart[2].x = 122 * 100 + 57600;
	k->posStart[2].y = 131 * 100 + 0;

	m_map_kWarMapInfo.insert(std::make_pair(k->lMapIndex, k));
	return true;
}

bool CWarMapManager::IsWarMap(long lMapIndex)
{
	return GetWarMapInfo(lMapIndex) ? true : false;
}

bool CWarMapManager::GetStartPosition(long lMapIndex, BYTE bIdx, PIXEL_POSITION & pos)
{
	assert(bIdx < 3);

	TWarMapInfo * pi = GetWarMapInfo(lMapIndex);

	if (!pi)
	{
		sys_log(0, "GetStartPosition FAILED [%d] WarMapInfoSize(%d)", lMapIndex, m_map_kWarMapInfo.size());

		itertype(m_map_kWarMapInfo) it;
		for (it	= m_map_kWarMapInfo.begin(); it != m_map_kWarMapInfo.end(); ++it)
		{
			PIXEL_POSITION& cur=it->second->posStart[bIdx];
			sys_log(0, "WarMap[%d]=Pos(%d, %d)", it->first, cur.x, cur.y);
		}
		return false;
	}

	pos = pi->posStart[bIdx];
	return true;
}

long CWarMapManager::CreateWarMap(const TGuildWarInfo& guildWarInfo, DWORD dwGuildID1, DWORD dwGuildID2
#ifdef __IMPROVED_GUILD_WAR__
	, int iMaxPlayer, int iMaxScore, DWORD flags, int custom_map_index
#endif
#ifdef GUILD_WAR_COUNTER
	, DWORD warID
#endif
)
{
	TWarMapInfo * pkInfo = GetWarMapInfo(guildWarInfo.lMapIndex);
	if (!pkInfo)
	{
		sys_err("GuildWar.CreateWarMap.NOT_FOUND_MAPINFO[%d]", guildWarInfo.lMapIndex);
		return 0;
	}

	DWORD lMapIndex = SECTREE_MANAGER::instance().CreatePrivateMap(guildWarInfo.lMapIndex);

	if (lMapIndex)
	{
		m_mapWarMap.insert(std::make_pair(lMapIndex, M2_NEW CWarMap(lMapIndex, guildWarInfo, pkInfo, dwGuildID1, dwGuildID2
#ifdef __IMPROVED_GUILD_WAR__
			, iMaxPlayer, iMaxScore, flags, custom_map_index
#endif
#ifdef GUILD_WAR_COUNTER
			, warID
#endif
		)));
	}

	return lMapIndex;
}

TWarMapInfo * CWarMapManager::GetWarMapInfo(long lMapIndex)
{
	if (lMapIndex >= 10000)
		lMapIndex /= 10000;

	itertype(m_map_kWarMapInfo) it = m_map_kWarMapInfo.find(lMapIndex);

	if (m_map_kWarMapInfo.end() == it)
		return NULL;

	return it->second;
}

void CWarMapManager::DestroyWarMap(CWarMap* pMap)
{
	long mapIdx = pMap->GetMapIndex();

	sys_log(0, "WarMap::DestroyWarMap : %d", mapIdx);

	m_mapWarMap.erase(pMap->GetMapIndex());
	M2_DELETE(pMap);

	SECTREE_MANAGER::instance().DestroyPrivateMap(mapIdx);
}

CWarMap * CWarMapManager::Find(long lMapIndex)
{
	itertype(m_mapWarMap) it = m_mapWarMap.find(lMapIndex);

	if (it == m_mapWarMap.end())
		return NULL;

	return it->second;
}

void CWarMapManager::OnShutdown()
{
	itertype(m_mapWarMap) it = m_mapWarMap.begin();

	while (it != m_mapWarMap.end())
		(it++)->second->Draw();
}


#ifdef GUILD_WAR_COUNTER
void CWarMap::SaveCounterData()
{
	if (war_static.size())
	{
		std::vector<war_static_ptr> send_data;
		for (itertype(war_static) it = war_static.begin(); it != war_static.end(); ++it)
			send_data.emplace_back(it->second);
		BYTE sub_index = SUB_GUILDWAR_LOADWAR;
		int vecSize = send_data.size();
		db_clientdesc->DBPacketHeader(HEADER_GD_GUILD_COUNTER, 0,  sizeof(BYTE) + sizeof(DWORD) + sizeof(int) + (send_data.size() * sizeof(war_static_ptr)));
		db_clientdesc->Packet(&sub_index, sizeof(BYTE));
		db_clientdesc->Packet(&warID, sizeof(DWORD));
		db_clientdesc->Packet(&vecSize, sizeof(int));
		if(vecSize)
			db_clientdesc->Packet(send_data.data(), sizeof(war_static_ptr)*send_data.size());
	}
	war_static.clear();
}
void CWarMap::UpdateStatic(LPCHARACTER ch, BYTE sub_index, std::vector<war_static_ptr> m_list)
{
	TPacketGCGuildStatic p;
	p.header = HEDAER_GC_GUILD_WAR;
	p.sub_index = sub_index;
	if (sub_index == GUILD_STATIC_LOAD)
	{
		if (!ch)
			return;
		itertype(m_set_pkChr) it = m_set_pkChr.find(ch->GetPlayerID());
		if (it == m_set_pkChr.end())
			return;
		if (it->second)
			return;

		it->second = true;
		if (war_static.size())
		{
			p.packet_size = war_static.size();
			p.size = sizeof(p) + (sizeof(war_static_ptr) * war_static.size());
			std::vector<war_static_ptr> m_data;
			m_data.clear();
			for (itertype(war_static) it = war_static.begin(); it != war_static.end(); ++it)
				m_data.emplace_back(it->second);
			ch->GetDesc()->BufferedPacket(&p, sizeof(p));
			ch->GetDesc()->Packet(m_data.data(), sizeof(war_static_ptr)* p.packet_size);
		}
		else
		{
			p.sub_index = 99;
			p.size = sizeof(p);
			ch->GetDesc()->Packet(&p, sizeof(p));
		}
	}
	else if (GUILD_STATIC_UPDATE_OBSERVER == sub_index)
	{
		p.size = sizeof(p) + sizeof(m_iObserverCount);
		TEMP_BUFFER buf;
		buf.write(&p, sizeof(p));
		buf.write(&m_iObserverCount, sizeof(m_iObserverCount));
		Packet(buf.read_peek(), buf.size(), true);
	}
	else
	{
		for (itertype(m_list) it = m_list.begin(); it != m_list.end(); ++it)
			sys_err("name %s level %d empire %d race %d", it->name, it->level, it->empire, it->race);

		p.size = sizeof(p) + (sizeof(war_static_ptr) * m_list.size());
		p.packet_size = m_list.size();
		TEMP_BUFFER buf;
		buf.write(&p, sizeof(p));
		buf.write(m_list.data(), sizeof(war_static_ptr) * p.packet_size);
		Packet(buf.read_peek(), buf.size(), true);
	}
}

void CWarMap::RegisterStatics(LPCHARACTER ch)
{
	itertype(war_static) it = war_static.find(ch->GetPlayerID());
	if (it == war_static.end())
	{
		war_static_ptr ptr;
		memset(&ptr, 0, sizeof(ptr));
		ptr.empire = ch->GetEmpire();
		strlcpy(ptr.name, ch->GetName(), sizeof(ptr.name));
		ptr.pid = ch->GetPlayerID();
		ptr.level = ch->GetLevel();
		ptr.race = ch->GetRaceNum();
		ptr.guild_id = ch->GetGuild()->GetID();
		ptr.is_leader = ch->GetPlayerID() == ch->GetGuild()->GetMasterPID();
		ptr.online = true;
		war_static.emplace(ch->GetPlayerID(), ptr);

		std::vector<war_static_ptr> m_list;
		m_list.clear();
		m_list.emplace_back(ptr);
		UpdateStatic(NULL, GUILD_STATIC_ADD_MEMBER, m_list);
	}
	else
	{
		if (!it->second.online)
		{
			it->second.online = true;
			std::vector<war_static_ptr> m_list;
			m_list.clear();
			m_list.emplace_back(it->second);
			UpdateStatic(NULL, GUILD_STATIC_UPDATE_ONLINE, m_list);
		}
	}
}

void CWarMap::UpdateSpy(DWORD pid)
{
	itertype(war_static) spy_it = war_static.find(pid);
	if (spy_it != war_static.end())
	{
		spy_it->second.spy = true;
		std::vector<war_static_ptr> m_list;
		m_list.clear();
		m_list.emplace_back(spy_it->second);
		UpdateStatic(NULL, GUILD_STATIC_SPY, m_list);
	}
}
void CWarMap::SendKillNotice(LPCHARACTER killer, LPCHARACTER victim, long damage)
{
	if (m_bEnded)
		return;
	//DWORD team_ID = killer->GetProtectTime("guild_team");
	//if (team_ID >= 1 && team_ID <= 2)
	{
		std::vector<war_static_ptr> m_list;
		m_list.clear();
		if (killer)
		{
			itertype(war_static) killer_it = war_static.find(killer->GetPlayerID());
			if (killer_it != war_static.end())
			{
				if (damage > 0)
				{
					killer_it->second.skill_dmg += damage;
					m_list.emplace_back(killer_it->second);
					UpdateStatic(NULL, GUILD_STATIC_DMG, m_list);
					return;
				}
				else
				{
					killer_it->second.kill += 1;
					m_list.emplace_back(killer_it->second);
				}
			}
		}

		if (victim)
		{
			itertype(war_static) victim_it = war_static.find(victim->GetPlayerID());
			if (victim_it != war_static.end())
			{
				victim_it->second.dead += 1;
				m_list.emplace_back(victim_it->second);
			}
		}
		UpdateStatic(NULL, GUILD_STATIC_KILL_DEAD, m_list);
	}
}


#endif

