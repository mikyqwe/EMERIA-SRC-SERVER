#include "stdafx.h"
#include "constants.h"
#include "utils.h"
#include "config.h"
#include "log.h"
#include "char.h"
#include "db.h"
#include "lzo_manager.h"
#include "desc_client.h"
#include "buffer_manager.h"
#include "char_manager.h"
#include "packet.h"
#include "war_map.h"
#include "questmanager.h"
#include "locale_service.h"
#include "guild_manager.h"
#include "MarkManager.h"
#include "p2p.h"
#include "../../common/service.h"


namespace
{

	struct FGuildNameSender
	{
		FGuildNameSender(DWORD id, const char* guild_name) : id(id), name(guild_name)
		{
			p.header = HEADER_GC_GUILD;
			p.subheader = GUILD_SUBHEADER_GC_GUILD_NAME;
			p.size = sizeof(p) + GUILD_NAME_MAX_LEN + sizeof(DWORD);
		}

		void operator()(LPCHARACTER ch)
		{
			LPDESC d = ch->GetDesc();

			if (d)
			{
				d->BufferedPacket(&p, sizeof(p));
				d->BufferedPacket(&id, sizeof(id));
				d->Packet(name, GUILD_NAME_MAX_LEN);
			}
		}

		DWORD		id;
		const char *	name;
		TPacketGCGuild	p;
	};
}

CGuildManager::CGuildManager()
{
#ifdef GUILD_WAR_COUNTER
	m_warStatisticsInfo.clear();
	m_warStatisticsData.clear();
#endif
}

CGuildManager::~CGuildManager()
{
#ifdef GUILD_WAR_COUNTER
	m_warStatisticsInfo.clear();
	m_warStatisticsData.clear();
#endif
	for( TGuildMap::const_iterator iter = m_mapGuild.begin() ; iter != m_mapGuild.end() ; ++iter )
	{
		M2_DELETE(iter->second);
	}

	m_mapGuild.clear();
//#ifdef GUILD_RANK_EFFECT
//	m_mapNameAll.clear();
//#endif
#ifdef ENABLE_GUILD_REQUEST
	m_mapNameGuild.clear();
	m_mapNameAll.clear();
	m_mapNameShinsoo.clear();
	m_mapNameChunjo.clear();
	m_mapNameJinno.clear();
#endif
}

int CGuildManager::GetDisbandDelay()
{
	return quest::CQuestManager::instance().GetEventFlag("guild_disband_delay") * (test_server ? 60 : 86400);
}

int CGuildManager::GetWithdrawDelay()
{
	return quest::CQuestManager::instance().GetEventFlag("guild_withdraw_delay") * (test_server ? 60 : 86400);
}

DWORD CGuildManager::CreateGuild(TGuildCreateParameter& gcp)
{
	if (!gcp.master)
		return 0;

	if (!check_name(gcp.name))
	{
		gcp.master->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드 이름이 적합하지 않습니다."));
		return 0;
	}

	// @fixme143 BEGIN
	static char	__guild_name[GUILD_NAME_MAX_LEN*2+1];
	DBManager::instance().EscapeString(__guild_name, sizeof(__guild_name), gcp.name, strnlen(gcp.name, sizeof(gcp.name)));
	if (strncmp(__guild_name, gcp.name, strnlen(gcp.name, sizeof(gcp.name))))
		return 0;
	// @fixme143 END

	std::auto_ptr<SQLMsg> pmsg(DBManager::instance().DirectQuery("SELECT COUNT(*) FROM guild%s WHERE name = '%s'",
				get_table_postfix(), __guild_name));

	if (pmsg->Get()->uiNumRows > 0)
	{
		MYSQL_ROW row = mysql_fetch_row(pmsg->Get()->pSQLResult);

		if (!(row[0] && row[0][0] == '0'))
		{
			gcp.master->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 이미 같은 이름의 길드가 있습니다."));
			return 0;
		}
	}
	else
	{
		gcp.master->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드를 생성할 수 없습니다."));
		return 0;
	}

	// new CGuild(gcp) queries guild tables and tell dbcache to notice other game servers.
	// other game server calls CGuildManager::LoadGuild to load guild.
	CGuild * pg = M2_NEW CGuild(gcp);
	m_mapGuild.insert(std::make_pair(pg->GetID(), pg));
#ifdef GUILD_RANK_EFFECT
	SortGuildCache();
#endif
	return pg->GetID();
}

void CGuildManager::Unlink(DWORD pid)
{
	m_map_pkGuildByPID.erase(pid);
}

CGuild * CGuildManager::GetLinkedGuild(DWORD pid)
{
	TGuildMap::iterator it = m_map_pkGuildByPID.find(pid);

	if (it == m_map_pkGuildByPID.end())
		return NULL;

	return it->second;
}

void CGuildManager::Link(DWORD pid, CGuild* guild)
{
	if (m_map_pkGuildByPID.find(pid) == m_map_pkGuildByPID.end())
		m_map_pkGuildByPID.insert(std::make_pair(pid,guild));
}

void CGuildManager::P2PLogoutMember(DWORD pid)
{
	TGuildMap::iterator it = m_map_pkGuildByPID.find(pid);

	if (it != m_map_pkGuildByPID.end())
	{
		it->second->P2PLogoutMember(pid);
	}
}

void CGuildManager::P2PLoginMember(DWORD pid)
{
	TGuildMap::iterator it = m_map_pkGuildByPID.find(pid);

	if (it != m_map_pkGuildByPID.end())
	{
		it->second->P2PLoginMember(pid);
	}
}

void CGuildManager::LoginMember(LPCHARACTER ch)
{
	TGuildMap::iterator it = m_map_pkGuildByPID.find(ch->GetPlayerID());

	if (it != m_map_pkGuildByPID.end())
	{
		it->second->LoginMember(ch);
	}
}


CGuild* CGuildManager::TouchGuild(DWORD guild_id)
{
	TGuildMap::iterator it = m_mapGuild.find(guild_id);

	if (it == m_mapGuild.end())
	{
		m_mapGuild.insert(std::make_pair(guild_id, M2_NEW CGuild(guild_id)));
		it = m_mapGuild.find(guild_id);

		CHARACTER_MANAGER::instance().for_each_pc(FGuildNameSender(guild_id, it->second->GetName()));
	}

	return it->second;
}

CGuild* CGuildManager::FindGuild(DWORD guild_id)
{
	TGuildMap::iterator it = m_mapGuild.find(guild_id);
	if (it == m_mapGuild.end())
	{
		return NULL;
	}
	return it->second;
}

CGuild*	CGuildManager::FindGuildByName(const std::string guild_name)
{
	itertype(m_mapGuild) it;
	for (it = m_mapGuild.begin(); it!=m_mapGuild.end(); ++it)
	{
		if (it->second->GetName()==guild_name)
			return it->second;
	}
	return NULL;
}

void CGuildManager::Initialize()
{
	sys_log(0, "Initializing Guild");

	if (g_bAuthServer)
	{
		sys_log(0, "	No need for auth server");
		return;
	}

	std::auto_ptr<SQLMsg> pmsg(DBManager::instance().DirectQuery("SELECT id FROM guild%s", get_table_postfix()));

	std::vector<DWORD> vecGuildID;
	vecGuildID.reserve(pmsg->Get()->uiNumRows);

	for (uint i = 0; i < pmsg->Get()->uiNumRows; i++)
	{
		MYSQL_ROW row = mysql_fetch_row(pmsg->Get()->pSQLResult);
		DWORD guild_id = strtoul(row[0], (char**) NULL, 10);
		LoadGuild(guild_id);

		vecGuildID.push_back(guild_id);
	}

	CGuildMarkManager & rkMarkMgr = CGuildMarkManager::instance();

	rkMarkMgr.SetMarkPathPrefix("mark");

	extern bool GuildMarkConvert(const std::vector<DWORD> & vecGuildID);
	GuildMarkConvert(vecGuildID);

	rkMarkMgr.LoadMarkIndex();
	rkMarkMgr.LoadMarkImages();
	rkMarkMgr.LoadSymbol(GUILD_SYMBOL_FILENAME);
}

void CGuildManager::LoadGuild(DWORD guild_id)
{
	TGuildMap::iterator it = m_mapGuild.find(guild_id);

	if (it == m_mapGuild.end())
	{
		m_mapGuild.insert(std::make_pair(guild_id, M2_NEW CGuild(guild_id)));
	}
	else
	{
		//it->second->Load(guild_id);
	}
}

void CGuildManager::DisbandGuild(DWORD guild_id)
{
	TGuildMap::iterator it = m_mapGuild.find(guild_id);

	if (it == m_mapGuild.end())
		return;
	it->second->Disband();

	M2_DELETE(it->second);
	m_mapGuild.erase(it);

	CGuildMarkManager::instance().DeleteMark(guild_id);
#ifdef GUILD_RANK_EFFECT
	SortGuildCache();
#endif
}

void CGuildManager::SkillRecharge()
{
	for (TGuildMap::iterator it = m_mapGuild.begin(); it!=m_mapGuild.end();++it)
	{
		it->second->SkillRecharge();
	}
}

int CGuildManager::GetRank(CGuild* g)
{
	int point = g->GetLadderPoint();
	int rank = 1;

	for (itertype(m_mapGuild) it = m_mapGuild.begin(); it != m_mapGuild.end();++it)
	{
		CGuild * pg = it->second;

		if (pg->GetLadderPoint() > point)
			rank++;
	}

	return rank;
}

struct FGuildCompare : public std::binary_function<CGuild*, CGuild*, bool>
{
	bool operator () (CGuild* g1, CGuild* g2) const
	{
		if (g1->GetLadderPoint() < g2->GetLadderPoint())
			return true;
		if (g1->GetLadderPoint() > g2->GetLadderPoint())
			return false;
		if (g1->GetGuildWarWinCount() < g2->GetGuildWarWinCount())
			return true;
		if (g1->GetGuildWarWinCount() > g2->GetGuildWarWinCount())
			return false;
		if (g1->GetGuildWarLossCount() < g2->GetGuildWarLossCount())
			return true;
		if (g1->GetGuildWarLossCount() > g2->GetGuildWarLossCount())
			return false;
		int c = strcmp(g1->GetName(), g2->GetName());
		if (c>0)
			return true;
		return false;
	}
};

void CGuildManager::GetHighRankString(DWORD dwMyGuild, char * buffer, size_t buflen)
{
	using namespace std;
	vector<CGuild*> v;

	for (itertype(m_mapGuild) it = m_mapGuild.begin(); it != m_mapGuild.end(); ++it)
	{
		if (it->second)
			v.push_back(it->second);
	}

	std::sort(v.begin(), v.end(), FGuildCompare());
	int n = v.size();
	int len = 0, len2;
	*buffer = '\0';

	for (int i = 0; i < 8; ++i)
	{
		if (n - i - 1 < 0)
			break;

		CGuild * g = v[n - i - 1];

		if (!g)
			continue;

		if (g->GetLadderPoint() <= 0)
			break;

		if (dwMyGuild == g->GetID())
		{
			len2 = snprintf(buffer + len, buflen - len, "[COLOR r;255|g;255|b;0]");

			if (len2 < 0 || len2 >= (int) buflen - len)
				len += (buflen - len) - 1;
			else
				len += len2;
		}

		if (i)
		{
			len2 = snprintf(buffer + len, buflen - len, "[ENTER]");

			if (len2 < 0 || len2 >= (int) buflen - len)
				len += (buflen - len) - 1;
			else
				len += len2;
		}

		len2 = snprintf(buffer + len, buflen - len, "%3d | %-12s | %-8d | %4d | %4d | %4d",
				GetRank(g),
				g->GetName(),
				g->GetLadderPoint(),
				g->GetGuildWarWinCount(),
				g->GetGuildWarDrawCount(),
				g->GetGuildWarLossCount());

		if (len2 < 0 || len2 >= (int) buflen - len)
			len += (buflen - len) - 1;
		else
			len += len2;

		if (g->GetID() == dwMyGuild)
		{
			len2 = snprintf(buffer + len, buflen - len, "[/COLOR]");

			if (len2 < 0 || len2 >= (int) buflen - len)
				len += (buflen - len) - 1;
			else
				len += len2;
		}
	}
}

void CGuildManager::GetAroundRankString(DWORD dwMyGuild, char * buffer, size_t buflen)
{
	using namespace std;
	vector<CGuild*> v;

	for (itertype(m_mapGuild) it = m_mapGuild.begin(); it != m_mapGuild.end(); ++it)
	{
		if (it->second)
			v.push_back(it->second);
	}

	std::sort(v.begin(), v.end(), FGuildCompare());
	int n = v.size();
	int idx;

	for (idx = 0; idx < (int) v.size(); ++idx)
		if (v[idx]->GetID() == dwMyGuild)
			break;

	int len = 0, len2;
	int count = 0;
	*buffer = '\0';

	for (int i = -3; i <= 3; ++i)
	{
		if (idx - i < 0)
			continue;

		if (idx - i >= n)
			continue;

		CGuild * g = v[idx - i];

		if (!g)
			continue;

		if (dwMyGuild == g->GetID())
		{
			len2 = snprintf(buffer + len, buflen - len, "[COLOR r;255|g;255|b;0]");

			if (len2 < 0 || len2 >= (int) buflen - len)
				len += (buflen - len) - 1;
			else
				len += len2;
		}

		if (count)
		{
			len2 = snprintf(buffer + len, buflen - len, "[ENTER]");

			if (len2 < 0 || len2 >= (int) buflen - len)
				len += (buflen - len) - 1;
			else
				len += len2;
		}

		len2 = snprintf(buffer + len, buflen - len, "%3d | %-12s | %-8d | %4d | %4d | %4d",
				GetRank(g),
				g->GetName(),
				g->GetLadderPoint(),
				g->GetGuildWarWinCount(),
				g->GetGuildWarDrawCount(),
				g->GetGuildWarLossCount());

		if (len2 < 0 || len2 >= (int) buflen - len)
			len += (buflen - len) - 1;
		else
			len += len2;

		++count;

		if (g->GetID() == dwMyGuild)
		{
			len2 = snprintf(buffer + len, buflen - len, "[/COLOR]");

			if (len2 < 0 || len2 >= (int) buflen - len)
				len += (buflen - len) - 1;
			else
				len += len2;
		}
	}
}

/////////////////////////////////////////////////////////////////////
// Guild War
/////////////////////////////////////////////////////////////////////
void CGuildManager::RequestCancelWar(DWORD guild_id1, DWORD guild_id2)
{
	sys_log(0, "RequestCancelWar %d %d", guild_id1, guild_id2);

	TPacketGuildWar p;
	p.bWar = GUILD_WAR_CANCEL;
	p.dwGuildFrom = guild_id1;
	p.dwGuildTo = guild_id2;
	db_clientdesc->DBPacket(HEADER_GD_GUILD_WAR, 0, &p, sizeof(p));
}

void CGuildManager::RequestEndWar(DWORD guild_id1, DWORD guild_id2)
{
	sys_log(0, "RequestEndWar %d %d", guild_id1, guild_id2);

	TPacketGuildWar p;
	p.bWar = GUILD_WAR_END;
	p.dwGuildFrom = guild_id1;
	p.dwGuildTo = guild_id2;
	db_clientdesc->DBPacket(HEADER_GD_GUILD_WAR, 0, &p, sizeof(p));
}
void CGuildManager::RequestWarOver(DWORD dwGuild1, DWORD dwGuild2, DWORD dwGuildWinner, long lReward
#ifdef __IMPROVED_GUILD_WAR__
	, int iMaxPlayer, int iMaxScore, DWORD flags, int custom_map_index
#endif
)
{
	CGuild * g1 = TouchGuild(dwGuild1);
	CGuild * g2 = TouchGuild(dwGuild2);

	if (g1->GetGuildWarState(g2->GetID()) != GUILD_WAR_ON_WAR)
	{
		sys_log(0, "RequestWarOver : both guild was not in war %u %u", dwGuild1, dwGuild2);
		RequestEndWar(dwGuild1, dwGuild2);
		return;
	}

	TPacketGuildWar p;

	p.bWar = GUILD_WAR_OVER;
	// 길드전이 끝나도 보상은 없다.
	//p.lWarPrice = lReward;
	p.lWarPrice = 0;
	p.bType = dwGuildWinner == 0 ? 1 : 0; // bType == 1 means draw for this packet.

	if (dwGuildWinner == 0)
	{
		p.dwGuildFrom = dwGuild1;
		p.dwGuildTo = dwGuild2;
	}
	else
	{
		p.dwGuildFrom = dwGuildWinner;
		p.dwGuildTo = dwGuildWinner == dwGuild1 ? dwGuild2 : dwGuild1;
	}
#ifdef __IMPROVED_GUILD_WAR__
	p.iMaxPlayer = iMaxPlayer;
	p.iMaxScore = iMaxScore;
	p.flags = flags;
	p.custom_map_index = custom_map_index;
#endif
	db_clientdesc->DBPacket(HEADER_GD_GUILD_WAR, 0, &p, sizeof(p));
	sys_log(0, "RequestWarOver : winner %u loser %u draw %u betprice %d", p.dwGuildFrom, p.dwGuildTo, p.bType, p.lWarPrice);
}

void CGuildManager::DeclareWar(DWORD guild_id1, DWORD guild_id2, BYTE bType
#ifdef __IMPROVED_GUILD_WAR__
	, int iMaxPlayer, int iMaxScore, DWORD flags, int custom_map_index
#endif
)
{
	if (guild_id1 == guild_id2)
		return;

	CGuild * g1 = FindGuild(guild_id1);
	CGuild * g2 = FindGuild(guild_id2);

	if (!g1 || !g2)
		return;

#ifdef __IMPROVED_GUILD_WAR__
	if (g1->DeclareWar(guild_id2, bType, GUILD_WAR_SEND_DECLARE, iMaxPlayer, iMaxScore, flags, custom_map_index) &&
		g2->DeclareWar(guild_id1, bType, GUILD_WAR_RECV_DECLARE, iMaxPlayer, iMaxScore, flags, custom_map_index))
#else
	if (g1->DeclareWar(guild_id2, bType, GUILD_WAR_SEND_DECLARE) && g2->DeclareWar(guild_id1, bType, GUILD_WAR_RECV_DECLARE))
#endif
	{
		// @warme005
		{
			char buf[256];
			snprintf(buf, sizeof(buf), LC_TEXT("%s a declarat razboi contra %s!"), TouchGuild(guild_id1)->GetName(), TouchGuild(guild_id2)->GetName());
			SendNotice(buf);
		}
	}
}

void CGuildManager::RefuseWar(DWORD guild_id1, DWORD guild_id2
#ifdef __IMPROVED_GUILD_WAR__
	, int iMaxPlayer, int iMaxScore, DWORD flags, int custom_map_index
#endif
)
{
	CGuild * g1 = FindGuild(guild_id1);
	CGuild * g2 = FindGuild(guild_id2);

	if (g1 && g2)
	{
		if (g2->GetMasterCharacter())
			g2->GetMasterCharacter()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> %s 길드가 길드전을 거부하였습니다."), g1->GetName());

		char buf[256];
		snprintf(buf, sizeof(buf), LC_TEXT("Breasla %s a refuzat sa se lupte impotriva %s."), g1->GetName(), g2->GetName());
		SendNotice(buf);

	}

#ifdef __IMPROVED_GUILD_WAR__
	if (g1)
		g1->RefuseWar(guild_id2, iMaxPlayer, iMaxScore, flags, custom_map_index);

	if (g2 && g1)
		g2->RefuseWar(g1->GetID(), iMaxPlayer, iMaxScore, flags, custom_map_index);
#else
	if ( g1 != NULL )
		g1->RefuseWar(guild_id2);

	if ( g2 != NULL && g1 != NULL )
		g2->RefuseWar(g1->GetID());
#endif
}

void CGuildManager::WaitStartWar(DWORD guild_id1, DWORD guild_id2
#ifdef __IMPROVED_GUILD_WAR__
	, int iMaxPlayer, int iMaxScore, DWORD flags, int custom_map_index
#endif
#ifdef GUILD_WAR_COUNTER
	, DWORD warID
#endif
)
{
	CGuild * g1 = FindGuild(guild_id1);
	CGuild * g2 = FindGuild(guild_id2);

	if (!g1 || !g2)
	{
#ifdef __IMPROVED_GUILD_WAR__
		sys_log(0, "GuildWar: CGuildManager::WaitStartWar(%d,%d | %d,%d,%d,%d) - something wrong in arg. there is no guild like that.", guild_id1, guild_id2, iMaxPlayer, iMaxScore, flags, custom_map_index);
#else
		sys_log(0, "GuildWar: CGuildManager::WaitStartWar(%d,%d) - something wrong in arg. there is no guild like that.", guild_id1, guild_id2);
#endif
		return;
	}
#ifdef __IMPROVED_GUILD_WAR__
	if (g1->WaitStartWar(guild_id2, iMaxPlayer, iMaxScore, flags, custom_map_index
#ifdef GUILD_WAR_COUNTER
			, warID
#endif
	
	) ||
		g2->WaitStartWar(guild_id1, iMaxPlayer, iMaxScore, flags, custom_map_index
#ifdef GUILD_WAR_COUNTER
			, warID
#endif
		))
#else
	if (g1->WaitStartWar(guild_id2) || g2->WaitStartWar(guild_id1) )
#endif
	{
		char buf[256];
		snprintf(buf, sizeof(buf), LC_TEXT("Razboiul dintre %s si %s va incepe curand!"), g1->GetName(), g2->GetName());
		SendNotice(buf);
	}
}

struct FSendWarList
{
	FSendWarList(BYTE subheader, DWORD guild_id1, DWORD guild_id2)
	{
		gid1 = guild_id1;
		gid2 = guild_id2;

		p.header	= HEADER_GC_GUILD;
		p.size		= sizeof(p) + sizeof(DWORD) * 2;
		p.subheader	= subheader;
	}

	void operator() (LPCHARACTER ch)
	{
		LPDESC d = ch->GetDesc();

		if (d)
		{
			d->BufferedPacket(&p, sizeof(p));
			d->BufferedPacket(&gid1, sizeof(DWORD));
			d->Packet(&gid2, sizeof(DWORD));
		}
	}

	DWORD gid1, gid2;
	TPacketGCGuild p;
};

void CGuildManager::StartWar(DWORD guild_id1, DWORD guild_id2
#ifdef __IMPROVED_GUILD_WAR__
	, int iMaxPlayer, int iMaxScore, DWORD flags, int custom_map_index
#endif
)
{
	CGuild * g1 = FindGuild(guild_id1);
	CGuild * g2 = FindGuild(guild_id2);

	if (!g1 || !g2)
		return;

	if (!g1->CheckStartWar(guild_id2) || !g2->CheckStartWar(guild_id1))
		return;
#ifdef __IMPROVED_GUILD_WAR__
	g1->StartWar(guild_id2, iMaxPlayer, iMaxScore, flags, custom_map_index);
	g2->StartWar(guild_id1, iMaxPlayer, iMaxScore, flags, custom_map_index);
#else
	g1->StartWar(guild_id2);
	g2->StartWar(guild_id1);
#endif

	char buf[256];
	snprintf(buf, sizeof(buf), LC_TEXT("%s si breasla %s inca se lupta!"), g1->GetName(), g2->GetName());
	SendNotice(buf);

	if (guild_id1 > guild_id2)
		std::swap(guild_id1, guild_id2);

	CHARACTER_MANAGER::instance().for_each_pc(FSendWarList(GUILD_SUBHEADER_GC_GUILD_WAR_LIST, guild_id1, guild_id2));
	m_GuildWar.insert(std::make_pair(guild_id1, guild_id2));
}

void SendGuildWarOverNotice(CGuild* g1, CGuild* g2, bool bDraw)
{
	if (g1 && g2)
	{
		char buf[256];

		if (bDraw)
		{
			snprintf(buf, sizeof(buf), LC_TEXT("Razboiul dintre %s si %s s-a terminat egalitate."), g1->GetName(), g2->GetName());
		}
		else
		{
			if ( g1->GetWarScoreAgainstTo( g2->GetID() ) > g2->GetWarScoreAgainstTo( g1->GetID() ) )
			{
				snprintf(buf, sizeof(buf), LC_TEXT("Breasla %s a castigat razboiul contra %s"), g1->GetName(), g2->GetName());
			}
			else
			{
				snprintf(buf, sizeof(buf), LC_TEXT("Breasla %s a castigat razboiul contra %s"), g2->GetName(), g1->GetName());
			}
		}

		SendNotice(buf);
	}
}

bool CGuildManager::EndWar(DWORD guild_id1, DWORD guild_id2)
{
	if (guild_id1 > guild_id2)
		std::swap(guild_id1, guild_id2);

	CGuild * g1 = FindGuild(guild_id1);
	CGuild * g2 = FindGuild(guild_id2);

	std::pair<DWORD, DWORD> k = std::make_pair(guild_id1, guild_id2);

	TGuildWarContainer::iterator it = m_GuildWar.find(k);

	if (m_GuildWar.end() == it)
	{
		sys_log(0, "EndWar(%d,%d) - EndWar request but guild is not in list", guild_id1, guild_id2);
		return false;
	}

	if ( g1 && g2 )
	{
	    if (g1->GetGuildWarType(guild_id2) == GUILD_WAR_TYPE_FIELD)
		{
			SendGuildWarOverNotice(g1, g2, g1->GetWarScoreAgainstTo(guild_id2) == g2->GetWarScoreAgainstTo(guild_id1));
		}
	}
	else
	{
	    return false;
	}

	if (g1)
		g1->EndWar(guild_id2);

	if (g2)
		g2->EndWar(guild_id1);

	m_GuildWarEndTime[k] = get_global_time();
	CHARACTER_MANAGER::instance().for_each_pc(FSendWarList(GUILD_SUBHEADER_GC_GUILD_WAR_END_LIST, guild_id1, guild_id2));
	m_GuildWar.erase(it);

	return true;
}

void CGuildManager::WarOver(DWORD guild_id1, DWORD guild_id2, bool bDraw)
{
	CGuild * g1 = FindGuild(guild_id1);
	CGuild * g2 = FindGuild(guild_id2);

	if (guild_id1 > guild_id2)
		std::swap(guild_id1, guild_id2);

	std::pair<DWORD, DWORD> k = std::make_pair(guild_id1, guild_id2);

	TGuildWarContainer::iterator it = m_GuildWar.find(k);

	if (m_GuildWar.end() == it)
		return;

	SendGuildWarOverNotice(g1, g2, bDraw);

	EndWar(guild_id1, guild_id2);
}

void CGuildManager::CancelWar(DWORD guild_id1, DWORD guild_id2
#ifdef __IMPROVED_GUILD_WAR__
	, int iMaxPlayer, int iMaxScore, DWORD flags, int custom_map_index
#endif
)
{
	if (!EndWar(guild_id1, guild_id2))
		return;

	CGuild * g1 = FindGuild(guild_id1);
	CGuild * g2 = FindGuild(guild_id2);

	if (g1)
	{
		LPCHARACTER master1 = g1->GetMasterCharacter();

		if (master1)
			master1->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드전이 취소 되었습니다."));
	}

	if (g2)
	{
		LPCHARACTER master2 = g2->GetMasterCharacter();

		if (master2)
			master2->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드전이 취소 되었습니다."));
	}

	if (g1 && g2)
	{
		char buf[256+1];
		snprintf(buf, sizeof(buf), LC_TEXT("%s 길드와 %s 길드 사이의 전쟁이 취소되었습니다."), g1->GetName(), g2->GetName());
		SendNotice(buf);
	}
}

void CGuildManager::ReserveWar(DWORD dwGuild1, DWORD dwGuild2, BYTE bType
#ifdef __IMPROVED_GUILD_WAR__
	, int iMaxPlayer, int iMaxScore, DWORD flags, int custom_map_index
#endif
) // from DB
{
	sys_log(0, "GuildManager::ReserveWar %u %u", dwGuild1, dwGuild2);

	CGuild * g1 = FindGuild(dwGuild1);
	CGuild * g2 = FindGuild(dwGuild2);

	if (!g1 || !g2)
		return;
#ifdef __IMPROVED_GUILD_WAR__
	g1->ReserveWar(dwGuild2, bType, iMaxPlayer, iMaxScore, flags, custom_map_index);
	g2->ReserveWar(dwGuild1, bType, iMaxPlayer, iMaxScore, flags, custom_map_index); 
#else
	g1->ReserveWar(dwGuild2, bType);
	g2->ReserveWar(dwGuild1, bType);
#endif
}

void CGuildManager::ShowGuildWarList(LPCHARACTER ch)
{
	for (itertype(m_GuildWar) it = m_GuildWar.begin(); it != m_GuildWar.end(); ++it)
	{
		CGuild * A = TouchGuild(it->first);
		CGuild * B = TouchGuild(it->second);

		if (A && B)
		{
			ch->ChatPacket(CHAT_TYPE_NOTICE, "%s[%d] vs %s[%d] time %u sec.",
					A->GetName(), A->GetID(),
					B->GetName(), B->GetID(),
					get_global_time() - A->GetWarStartTime(B->GetID()));
		}
	}
}

void CGuildManager::SendGuildWar(LPCHARACTER ch)
{
	if (!ch->GetDesc())
		return;

	TEMP_BUFFER buf;
	TPacketGCGuild p;
	p.header= HEADER_GC_GUILD;
	p.subheader = GUILD_SUBHEADER_GC_GUILD_WAR_LIST;
	p.size = sizeof(p) + (sizeof(DWORD) * 2) * m_GuildWar.size();
	buf.write(&p, sizeof(p));

	itertype(m_GuildWar) it;

	for (it = m_GuildWar.begin(); it != m_GuildWar.end(); ++it)
	{
		buf.write(&it->first, sizeof(DWORD));
		buf.write(&it->second, sizeof(DWORD));
	}

	ch->GetDesc()->Packet(buf.read_peek(), buf.size());
}

void SendGuildWarScore(DWORD dwGuild, DWORD dwGuildOpp, int iDelta, int iBetScoreDelta)
{
	TPacketGuildWarScore p;

	p.dwGuildGainPoint = dwGuild;
	p.dwGuildOpponent = dwGuildOpp;
	p.lScore = iDelta;
	p.lBetScore = iBetScoreDelta;

	db_clientdesc->DBPacket(HEADER_GD_GUILD_WAR_SCORE, 0, &p, sizeof(TPacketGuildWarScore));
	sys_log(0, "SendGuildWarScore %u %u %d", dwGuild, dwGuildOpp, iDelta);
}

void CGuildManager::Kill(LPCHARACTER killer, LPCHARACTER victim)
{
	if (!killer->IsPC())
		return;

	if (!victim->IsPC())
		return;

	if (killer->GetWarMap())
	{
		killer->GetWarMap()->OnKill(killer, victim);
		return;
	}

	CGuild * gAttack = killer->GetGuild();
	CGuild * gDefend = victim->GetGuild();

	if (!gAttack || !gDefend)
		return;

	if (gAttack->GetGuildWarType(gDefend->GetID()) != GUILD_WAR_TYPE_FIELD)
		return;

	if (!gAttack->UnderWar(gDefend->GetID()))
		return;

	SendGuildWarScore(gAttack->GetID(), gDefend->GetID(), victim->GetLevel());
}

void CGuildManager::StopAllGuildWar()
{
	for (itertype(m_GuildWar) it = m_GuildWar.begin(); it != m_GuildWar.end(); ++it)
	{
		CGuild * g = CGuildManager::instance().TouchGuild(it->first);
		CGuild * pg = CGuildManager::instance().TouchGuild(it->second);
		g->EndWar(it->second);
		pg->EndWar(it->first);
	}

	m_GuildWar.clear();
}

void CGuildManager::ReserveWarAdd(TGuildWarReserve * p)
{
	itertype(m_map_kReserveWar) it = m_map_kReserveWar.find(p->dwID);

	CGuildWarReserveForGame * pkReserve;

	if (it != m_map_kReserveWar.end())
		pkReserve = it->second;
	else
	{
		pkReserve = M2_NEW CGuildWarReserveForGame;

		m_map_kReserveWar.insert(std::make_pair(p->dwID, pkReserve));
		m_vec_kReserveWar.push_back(pkReserve);
	}

	thecore_memcpy(&pkReserve->data, p, sizeof(TGuildWarReserve));

	sys_log(0, "ReserveWarAdd %u gid1 %u power %d gid2 %u power %d handicap %d",
			pkReserve->data.dwID, p->dwGuildFrom, p->lPowerFrom, p->dwGuildTo, p->lPowerTo, p->lHandicap);
}

void CGuildManager::ReserveWarBet(TPacketGDGuildWarBet * p)
{
	itertype(m_map_kReserveWar) it = m_map_kReserveWar.find(p->dwWarID);

	if (it == m_map_kReserveWar.end())
		return;

	it->second->mapBet.insert(std::make_pair(p->szLogin, std::make_pair(p->dwGuild, p->dwGold)));
}

bool CGuildManager::IsBet(DWORD dwID, const char * c_pszLogin)
{
	itertype(m_map_kReserveWar) it = m_map_kReserveWar.find(dwID);

	if (it == m_map_kReserveWar.end())
		return true;

	return it->second->mapBet.end() != it->second->mapBet.find(c_pszLogin);
}

void CGuildManager::ReserveWarDelete(DWORD dwID)
{
	sys_log(0, "ReserveWarDelete %u", dwID);
	itertype(m_map_kReserveWar) it = m_map_kReserveWar.find(dwID);

	if (it == m_map_kReserveWar.end())
		return;

	itertype(m_vec_kReserveWar) it_vec = m_vec_kReserveWar.begin();

	while (it_vec != m_vec_kReserveWar.end())
	{
		if (*it_vec == it->second)
		{
			it_vec = m_vec_kReserveWar.erase(it_vec);
			break;
		}
		else
			++it_vec;
	}

	M2_DELETE(it->second);
	m_map_kReserveWar.erase(it);
}

std::vector<CGuildWarReserveForGame *> & CGuildManager::GetReserveWarRef()
{
	return m_vec_kReserveWar;
}

//
// End of Guild War
//

void CGuildManager::ChangeMaster(DWORD dwGID)
{
	TGuildMap::iterator iter = m_mapGuild.find(dwGID);

	if ( iter != m_mapGuild.end() )
	{
		iter->second->Load(dwGID);
	}

	// 업데이트된 정보 보내주기
	DBManager::instance().FuncQuery(std::bind1st(std::mem_fun(&CGuild::SendGuildDataUpdateToAllMember), iter->second),
			"SELECT 1");

}

#ifdef GUILD_WAR_COUNTER
void CGuildManager::SendWarStatisticsPacket(LPCHARACTER ch, BYTE subHeader)
{
	TPacketGCGuildStatic p;
	p.header = HEDAER_GC_GUILD_WAR;
	p.size = sizeof(p);
	if (subHeader == GUILD_STATIC_EVENT)
	{
		p.sub_index = GUILD_STATIC_EVENT;
		ch->GetDesc()->Packet(&p, sizeof(p));
	}
	else if (subHeader == GUILD_STATIC_INFO)
	{
		if (m_warStatisticsInfo.size())
		{
			p.sub_index = GUILD_STATIC_INFO;
			std::vector<TGuildWarReserve> m_vec;
			m_vec.clear();
			for (itertype(m_warStatisticsInfo) it = m_warStatisticsInfo.begin(); it != m_warStatisticsInfo.end(); ++it)
			{
				m_vec.emplace_back(it->second);
				if (m_vec.size() >= 20)
				{
					TEMP_BUFFER buf;
					p.size = sizeof(TPacketGCGuildStatic) + (sizeof(TGuildWarReserve) * m_vec.size());
					p.packet_size = m_vec.size();
					buf.write(&p, sizeof(p));
					buf.write(&m_vec[0], (sizeof(TGuildWarReserve) * m_vec.size()));
					ch->GetDesc()->Packet(buf.read_peek(), buf.size());
					m_vec.clear();
				}
			}
			if (m_vec.size())
			{
				TEMP_BUFFER buf;
				p.size = sizeof(TPacketGCGuildStatic) + (sizeof(TGuildWarReserve) * m_vec.size());
				p.packet_size = m_vec.size();
				buf.write(&p, sizeof(p));
				buf.write(&m_vec[0], (sizeof(TGuildWarReserve) * m_vec.size()));
				ch->GetDesc()->Packet(buf.read_peek(), buf.size());
				m_vec.clear();
			}
		}
		CGuildManager::Instance().SendWarStatisticsPacket(ch, GUILD_STATIC_EVENT);
	}
}
void CGuildManager::SendWarStatisticsData(LPCHARACTER ch, DWORD id)
{
	itertype(m_warStatisticsData) it = m_warStatisticsData.find(id);
	if (it == m_warStatisticsData.end())
	{
		BYTE sub_index = SUB_GUILDWAR_LOADDATA;
		db_clientdesc->DBPacketHeader(HEADER_GD_GUILD_COUNTER, ch->GetDesc()->GetHandle(), sizeof(BYTE) + sizeof(DWORD));
		db_clientdesc->Packet(&sub_index, sizeof(BYTE));
		db_clientdesc->Packet(&id, sizeof(DWORD));
		return;
	}

	TPacketGCGuildStatic p;
	p.header = HEDAER_GC_GUILD_WAR;
	p.sub_index = GUILD_STATIC_DATA;
	std::vector<war_static_ptr> m_vec;
	m_vec.clear();
	for (DWORD i = 0; i < it->second.size(); ++i)
	{
		m_vec.emplace_back(it->second[i]);
		if (m_vec.size() >= 20)
		{
			TEMP_BUFFER buf;
			p.size = sizeof(TPacketGCGuildStatic) + sizeof(DWORD) + (sizeof(war_static_ptr) * m_vec.size());
			p.packet_size = m_vec.size();
			buf.write(&p, sizeof(p));
			buf.write(&id, sizeof(id));
			buf.write(m_vec.data(), (sizeof(war_static_ptr) * m_vec.size()));
			ch->GetDesc()->Packet(buf.read_peek(), buf.size());
			m_vec.clear();
		}
	}

	if (m_vec.size())
	{
		TEMP_BUFFER buf;
		p.size = sizeof(TPacketGCGuildStatic) + sizeof(DWORD) + (sizeof(war_static_ptr) * m_vec.size());
		p.packet_size = m_vec.size();
		buf.write(&p, sizeof(p));
		buf.write(&id, sizeof(id));
		buf.write(m_vec.data(), (sizeof(war_static_ptr) * m_vec.size()));
		ch->GetDesc()->Packet(buf.read_peek(), buf.size());
		m_vec.clear();
	}		
	SendWarStatisticsPacket(ch, GUILD_STATIC_EVENT);
}

void CGuildManager::SetWarStatisticsData(DWORD warID, const std::vector<war_static_ptr>& vec_data)
{
	itertype(m_warStatisticsData) it = m_warStatisticsData.find(warID);
	if (it == m_warStatisticsData.end())
		m_warStatisticsData.emplace(warID, vec_data);
	else
	{
		it->second.clear();
		it->second.resize(vec_data.size());
		thecore_memcpy(&it->second[0], &vec_data[0], sizeof(war_static_ptr) * vec_data.size());
	}
	sys_log(0, "SetWarStatisticsData adding warID: %d", warID);
}

void CGuildManager::SetWarStatisticsInfo(const std::vector<TGuildWarReserve>& vec_data)
{
	for (DWORD i = 0; i < vec_data.size(); ++i)
	{
		itertype(m_warStatisticsInfo) it = m_warStatisticsInfo.find(vec_data[i].dwID);
		if (it != m_warStatisticsInfo.end())
			thecore_memcpy(&it->second, &vec_data[i], sizeof(it->second));
		else
			m_warStatisticsInfo.emplace(vec_data[i].dwID, vec_data[i]);
		sys_log(0, "SetWarStatisticsInfo adding warID: %d", vec_data[i].dwID);
	}
}
void CGuildManager::SetWarStatisticsInfo(const TGuildWarReserve& m_reserverData)
{
	itertype(m_warStatisticsInfo) it = m_warStatisticsInfo.find(m_reserverData.dwID);
	if (it != m_warStatisticsInfo.end())
		thecore_memcpy(&it->second, &m_reserverData, sizeof(it->second));
	else
		m_warStatisticsInfo.emplace(m_reserverData.dwID, m_reserverData);
	sys_log(0, "SetWarStatisticsInfo adding warID: %d", m_reserverData.dwID);
}
#endif

#ifdef ENABLE_GUILD_ONLINE_LIST
#include "desc_client.h"
#include "desc_manager.h"
void CGuildManager::SendOnlineGuildData(LPCHARACTER ch)
{
	ch->SetProtectTime("guildlist", 1);

	TEMP_BUFFER buf;
	
	struct GuildListItem {
		DWORD id;
		char guildName[GUILD_NAME_MAX_LEN + 1];
		bool masterOnline;
	};

	TPacketGCGuildStatic p;
	p.header = HEDAER_GC_GUILD_WAR;
	p.sub_index = GUILD_ONLINE_LIST;
	//char guildName[GUILD_NAME_MAX_LEN + 1];
	//p.size = sizeof(p)+sizeof(DWORD)+(m_mapGuild.size()*(sizeof(DWORD) + sizeof(guildName) + sizeof(bool)));
	p.size = sizeof(p)+sizeof(DWORD)+(sizeof(GuildListItem)*m_mapGuild.size());

	DWORD guildCount = m_mapGuild.size();
	buf.write(&p, sizeof(p));
	buf.write(&guildCount, sizeof(DWORD));
	if(guildCount > 0)
	{
		for (auto it = m_mapGuild.begin(); it != m_mapGuild.end(); ++it)
		{
			auto guild = it->second;
			bool isOnline = false;
			LPCHARACTER master = CHARACTER_MANAGER::Instance().FindByPID(guild->GetMasterPID());
			auto masterP2P = P2P_MANAGER::Instance().FindByPID(guild->GetMasterPID());
			if (master || masterP2P)
				isOnline = true;
			
#ifdef ENABLE_GENERAL_IN_GUILD
			if(!isOnline)
			{
				DWORD generalPID = guild->GetGeneralMember();
				LPCHARACTER general = CHARACTER_MANAGER::Instance().FindByPID(generalPID);
				auto generalP2P = P2P_MANAGER::Instance().FindByPID(generalPID);
				if (general || generalP2P)
					isOnline = true;
			}
#endif
			//strlcpy(guildName, guild->GetName(), sizeof(guildName));
			//buf.write(&it->first, sizeof(DWORD));
			//buf.write(&guildName, sizeof(guildName));
			//buf.write(&isOnline, sizeof(bool));
			
			struct GuildListItem guildItem;
			strlcpy(guildItem.guildName, guild->GetName(), sizeof(guildItem.guildName));
			guildItem.id = it->first;
			guildItem.masterOnline = isOnline;
			buf.write(&guildItem, sizeof(guildItem));
		}
	}
	ch->GetDesc()->Packet(buf.read_peek(), buf.size());
}
void CGuildManager::SendOnlineGuildRefresh(DWORD guildID, bool status)
{
	auto it = m_mapGuild.find(guildID);
	if (it == m_mapGuild.end())
		return;
	auto guild = it->second;
	//char guildName[GUILD_NAME_MAX_LEN + 1];
	//strlcpy(guildName, guild->GetName(), sizeof(guildName));

	struct GuildListItem {
		DWORD id;
		char guildName[GUILD_NAME_MAX_LEN + 1];
		bool masterOnline;
	};

	TEMP_BUFFER buf;
	TPacketGCGuildStatic p;
	p.header = HEDAER_GC_GUILD_WAR;
	p.sub_index = GUILD_ONLINE_UPDATE;
	//p.size = sizeof(p) + sizeof(DWORD) + sizeof(guildName) + sizeof(bool);
	p.size = sizeof(p) + sizeof(GuildListItem);
	buf.write(&p, sizeof(p));
	
	struct GuildListItem guildItem;
	strlcpy(guildItem.guildName, guild->GetName(), sizeof(guildItem.guildName));
	guildItem.id = it->first;
	guildItem.masterOnline = status;
	buf.write(&guildItem, sizeof(guildItem));
			
	//buf.write(&it->first, sizeof(DWORD));
	//buf.write(&guildName, sizeof(guildName));
	//buf.write(&status, sizeof(bool));
	
	const DESC_MANAGER::DESC_SET& c_ref_set = DESC_MANAGER::instance().GetClientSet();
	for (auto it = c_ref_set.begin(); it != c_ref_set.end(); ++it)
	{
		auto desc = *it;
		LPCHARACTER ch = desc->GetCharacter();
		if (ch)
		{
			if (ch->GetProtectTime("guildlist") == 1)
				desc->Packet(buf.read_peek(), buf.size());
		}

	}
}
#endif

#ifdef GUILD_RANK_EFFECT
DWORD CGuildManager::GetMyGuildRank(CGuild* guild)
{
	DWORD rankIndex = 0;
	if (m_mapNameAll.size())
	{
		for (auto it = m_mapNameAll.begin(); it != m_mapNameAll.end(); ++it)
		{
			rankIndex += 1;
			auto rankGuild = *it;
			if (rankGuild == guild)
			{
				if (rankGuild->GetLadderPoint() == 0)
					rankIndex = 0;
				break;
			}
				
		}
	}
	return rankIndex;
}
#endif

#ifdef ENABLE_GUILD_REQUEST
#include <cmath>
#include "p2p.h"
bool sortGuild(const CGuild* a, const CGuild* b)
{
	return (a->GetLevel() > b->GetLevel() && a->GetLadderPoint() > b->GetLadderPoint());
}
void CGuildManager::SortGuildCache()
{
	m_mapNameGuild.clear();
	m_mapNameAll.clear();
	m_mapNameShinsoo.clear();
	m_mapNameChunjo.clear();
	m_mapNameJinno.clear();
	if (m_mapGuild.size())
	{
		for (auto it = m_mapGuild.begin(); it != m_mapGuild.end(); ++it)
		{
			CGuild* guild = it->second;
			m_mapNameGuild.emplace(guild->GetName(), guild);
			m_mapNameAll.emplace_back(guild);
		}
	}
	std::sort(m_mapNameAll.begin(), m_mapNameAll.end(), sortGuild);
	if (m_mapNameAll.size())
	{
		for (DWORD j = 0; j < m_mapNameAll.size(); ++j)
		{
			CGuild* guild = m_mapNameAll[j];
			guild->SetIndex(j + 1);
			if (guild->GetEmpire() == 1)
				m_mapNameShinsoo.emplace_back(guild);
			else if (guild->GetEmpire() == 2)
				m_mapNameChunjo.emplace_back(guild);
			else if (guild->GetEmpire() == 3)
				m_mapNameJinno.emplace_back(guild);
		}
	}
}
void CGuildManager::SendRequest(LPCHARACTER ch, DWORD guildID, BYTE requestIndex)
{
	// 0 - PLAYER SEND REQUEST
	// 1 - PLAYER REMOVE REQUEST
	// 2 - MASTER ACCEPT REQUEST
	// 3 - MASTER DONT ACCEPT REQUEST
	

	if (requestIndex == 0)
	{
		if (ch->GetGuild() != NULL)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("757"));
			return;
		}

		auto it = m_mapGuild.find(guildID);
		if (it != m_mapGuild.end())
		{
			CGuild* guild = it->second;
			if (guild->IsHaveRequest(ch->GetPlayerID()))
				return;
			else if (ch->GetEmpire() != guild->GetEmpire())
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("756"), guild->GetName());
				return;
			}


			TGuild_request p;
			p.pid = ch->GetPlayerID();
			strlcpy(p.name, ch->GetName(), sizeof(p.name));
			p.level = ch->GetLevel();
			p.race = ch->GetRaceNum();
			p.skillIndex = ch->GetSkillGroup();

			guild->SaveRequestData(p, true);
		}
	}
	else if (requestIndex == 1)
	{
		auto it = m_mapGuild.find(guildID);
		if (it != m_mapGuild.end())
		{
			CGuild* guild = it->second;
			if (!guild->IsHaveRequest(ch->GetPlayerID()))
				return;
			guild->RemoveRequestData(ch->GetPlayerID(), true);
		}
	}
	else if (requestIndex == 2)
	{
		CGuild* guild = ch->GetGuild();
		if (!guild)
			return;
		else if (!guild->IsHaveRequest(guildID))
			return;

		bool isOkey = false;
		auto itGuild = guild->GetMember(ch->GetPlayerID());
		if (ch == guild->GetMasterCharacter())
			isOkey = true;
		else if (itGuild && guild->HasGradeAuth(itGuild->grade, GUILD_AUTH_ADD_MEMBER))
			isOkey = true;

		if (!isOkey)
			return;
		

		LPCHARACTER tch = CHARACTER_MANAGER::Instance().FindByPID(guildID);
		if (!tch)
		{
			auto p2ptch = P2P_MANAGER::Instance().FindByPID(guildID);
			if (p2ptch == NULL)
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("755"));
			else
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("754"), p2ptch->bChannel);
		}
		else
		{
			if (tch->GetGuild() != NULL)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("753"), tch->GetName());
				return;
			}
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("752"));
			guild->Invite(ch, tch);
		}
	}
	else if (requestIndex == 3)
	{
		CGuild* guild = ch->GetGuild();
		if (guild)
		{
			if (!guild->IsHaveRequest(guildID))
				return;
			bool isOkey = false;
			auto itGuild = guild->GetMember(ch->GetPlayerID());
			if (ch == guild->GetMasterCharacter())
				isOkey = true;
			else if(itGuild && guild->HasGradeAuth(itGuild->grade, GUILD_AUTH_ADD_MEMBER))
				isOkey = true;
			if(isOkey)
				guild->RemoveRequestData(guildID, true, true);
		}
	}
}
void CGuildManager::SendData(LPCHARACTER ch, BYTE tabIndex, DWORD pageIndex, const char* guildName)
{
	const std::vector<CGuild*> m_cache[] = { m_mapNameAll, m_mapNameShinsoo, m_mapNameChunjo, m_mapNameJinno };
	if (tabIndex >= _countof(m_cache)+1) //+1 for request window..
		return;

	ch->SetProtectTime("request.tabIndex", tabIndex);
	ch->SetProtectTime("request.pageIndex", pageIndex);

	TPacketGCGuildRequest p;
	p.header = HEADER_GC_GUILD_REQUEST;
	p.size = sizeof(p);
	int pageCount = 1;
	TEMP_BUFFER buf;
	if (tabIndex == 4)
	{
		std::vector<TGuild_request> m_data;
		m_data.clear();
		CGuild* guild = ch->GetGuild();
		if(guild != NULL)
		{
			bool isOkey = false;
			auto itGuild = guild->GetMember(ch->GetPlayerID());
			if(guild->GetMasterCharacter() == ch)
				isOkey = true;
			else if(itGuild && guild->HasGradeAuth(itGuild->grade, GUILD_AUTH_ADD_MEMBER))
				isOkey = true;
			if (guild && isOkey)
			{
				int maxIteminGui = 8;
				int mapSize = guild->m_request.size();
				pageCount = int(ceil(float(mapSize) / float(maxIteminGui)));
				if (mapSize)
				{
					if (pageIndex > pageCount)
						return;
					int start = (pageIndex - 1) * maxIteminGui;
					if (start < 0)
						start = 0;
					int end = ((pageIndex - 1) * maxIteminGui) + maxIteminGui;
					if (end < 0)
						end = 0;
					for (int x = start; x <= end; ++x)
					{
						if (x >= mapSize)
							break;
						auto it = guild->m_request.begin();
						std::advance(it, x);
						TGuild_request p;
						p.index = x+1;
						p.level = it->level;
						p.pid = it->pid;
						p.race = it->race;
						p.skillIndex = it->skillIndex;
						strlcpy(p.name, it->name, sizeof(p.name));
						m_data.emplace_back(p);
					}
				}
			}
		}
	

		
		int dataSize = m_data.size();
		p.size += sizeof(TGuild_request) * dataSize;
		p.sub_index = SUB_REQUEST_REQUEST;
		buf.write(&p, sizeof(TPacketGCGuildRequest));
		buf.write(&tabIndex, sizeof(BYTE));
		buf.write(&pageIndex, sizeof(DWORD));
		buf.write(&pageCount, sizeof(int));
		buf.write(&dataSize, sizeof(int));
		if (dataSize)
			buf.write(m_data.data(), sizeof(TGuild_request) * dataSize);		
	}
	else
	{
		std::vector<TGuildRequest> m_data;
		m_data.clear();
		if (guildName != NULL)
		{
			auto it = m_mapNameGuild.find(guildName);
			if (it != m_mapNameGuild.end())
			{
				auto guild = it->second;
				TGuildRequest g;
				strlcpy(g.name, guild->GetName(), sizeof(g.name));
				g.index = guild->GetIndex();
				g.g_id = guild->GetID();
				g.level = guild->GetLevel();
				g.ladder_point = guild->GetLadderPoint();
				g.member[0] = guild->GetMemberCount();
				g.member[1] = guild->GetMaxMemberCount();
				g.isRequest = guild->IsHaveRequest(ch->GetPlayerID());
				m_data.emplace_back(g);
			}
		}
		else
		{
			int maxIteminGui = 8;
			int mapSize = m_cache[tabIndex].size();
			pageCount = int(ceil(float(mapSize) / float(maxIteminGui)));
			if (mapSize)
			{
				if (pageIndex > pageCount)
					return;
				int start = (pageIndex - 1) * maxIteminGui;
				if (start < 0)
					start = 0;
				int end = ((pageIndex - 1) * maxIteminGui) + maxIteminGui;
				if (end < 0)
					end = 0;
				for (int x = start; x <= end; ++x)
				{
					if (x >= mapSize)
						break;
					auto it = m_cache[tabIndex].begin();
					std::advance(it, x);
					auto guild = *it;
					TGuildRequest g;
					strlcpy(g.name, guild->GetName(), sizeof(g.name));
					g.index = guild->GetIndex();
					g.g_id = guild->GetID();
					g.level = guild->GetLevel();
					g.ladder_point = guild->GetLadderPoint();
					g.member[0] = guild->GetMemberCount();
					g.member[1] = guild->GetMaxMemberCount();
					g.isRequest = guild->IsHaveRequest(ch->GetPlayerID());
					m_data.emplace_back(g);
				}
			}
		}

		int dataSize = m_data.size();
		p.size += sizeof(TGuildRequest) * dataSize;
		
		if (guildName != NULL)
		{
			p.sub_index = SUB_REQUEST_NAME;
			buf.write(&p, sizeof(TPacketGCGuildRequest));
			buf.write(&tabIndex, sizeof(BYTE));
			buf.write(&dataSize, sizeof(int));
			if (dataSize)
				buf.write(m_data.data(), sizeof(TGuildRequest) * dataSize);
		}
		else
		{
			p.sub_index = SUB_REQUEST_PAGEINDEX;
			buf.write(&p, sizeof(TPacketGCGuildRequest));
			buf.write(&tabIndex, sizeof(BYTE));
			buf.write(&pageIndex, sizeof(DWORD));
			buf.write(&pageCount, sizeof(int));
			buf.write(&dataSize, sizeof(int));
			if (dataSize)
				buf.write(m_data.data(), sizeof(TGuildRequest) * dataSize);
		}
	}
	ch->GetDesc()->Packet(buf.read_peek(), buf.size());
}
#endif

#ifdef GUILD_RANK_EFFECT
bool sortGuild(const CGuild* a, const CGuild* b)
{
	return (a->GetLadderPoint() > b->GetLadderPoint());
}
void CGuildManager::SortGuildCache()
{
	m_mapNameAll.clear();
	if (m_mapGuild.size())
	{
		for (auto it = m_mapGuild.begin(); it != m_mapGuild.end(); ++it)
			m_mapNameAll.emplace_back(it->second);
	}
	std::sort(m_mapNameAll.begin(), m_mapNameAll.end(), sortGuild);
}

#endif

