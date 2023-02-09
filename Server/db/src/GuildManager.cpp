#include "stdafx.h"
#include "GuildManager.h"
#include "Main.h"
#include "ClientManager.h"
#include "QID.h"
#include "Config.h"
#include <math.h>

#ifdef GUILD_WAR_COUNTER
#include "buffer_manager.h"
#endif

extern std::string g_stLocale;

const int GUILD_RANK_MAX_NUM = 20;

bool isEurope()
{
	do
	{
		if (g_stLocale.compare("germany") == 0) break;
		if (g_stLocale.compare("france") == 0) break;
		if (g_stLocale.compare("italy") == 0) break;
		if (g_stLocale.compare("spain") == 0) break;
		if (g_stLocale.compare("uk") == 0) break;
		if (g_stLocale.compare("turkey") == 0) break;
		if (g_stLocale.compare("poland") == 0) break;
		if (g_stLocale.compare("portugal") == 0) break;
		if (g_stLocale.compare("greek") == 0) break;

		return false;
	} while (false);

	return true;
}

DWORD GetGuildWarWaitStartDuration()
{
	// const int GUILD_WAR_WAIT_START_DURATION = 60;
	// const int GUILD_WAR_WAIT_START_DURATION = 5;

	if (isEurope() == true) return 60;
	else return 5;
}

DWORD GetGuildWarReserveSeconds()
{
	// const int GUILD_WAR_RESERVE_SECONDS = 180;
	// const int GUILD_WAR_RESERVE_SECONDS = 10;

	if (isEurope() == true) return 180;
	else return 10;
}

namespace
{
	struct FSendPeerWar
	{
		FSendPeerWar(BYTE bType, BYTE bWar, DWORD GID1, DWORD GID2
#ifdef __IMPROVED_GUILD_WAR__
			, int iMaxPlayer, int iMaxScore, DWORD flags, int custom_map_index
#endif
		)
		{
			if (number(0, 1))
				std::swap(GID1, GID2);

			memset(&p, 0, sizeof(TPacketGuildWar));

			p.bWar = bWar;
			p.bType = bType;
			p.dwGuildFrom = GID1;
			p.dwGuildTo = GID2;
#ifdef __IMPROVED_GUILD_WAR__
			p.iMaxPlayer = iMaxPlayer;
			p.iMaxScore = iMaxScore;
			p.flags = flags; 
			p.custom_map_index = custom_map_index; 
#endif
		}

		void operator() (CPeer* peer)
		{
			if (peer->GetChannel() == 0)
				return;

			peer->EncodeHeader(HEADER_DG_GUILD_WAR, 0, sizeof(TPacketGuildWar));
			peer->Encode(&p, sizeof(TPacketGuildWar));
		}

		TPacketGuildWar p;
	};

	struct FSendGuildWarScore
	{
		FSendGuildWarScore(DWORD guild_gain, DWORD dwOppGID, int iScore, int iBetScore)
		{
			pck.dwGuildGainPoint = guild_gain;
			pck.dwGuildOpponent = dwOppGID;
			pck.lScore = iScore;
			pck.lBetScore = iBetScore;
		}

		void operator() (CPeer* peer)
		{
			if (peer->GetChannel() == 0)
				return;

			peer->EncodeHeader(HEADER_DG_GUILD_WAR_SCORE, 0, sizeof(pck));
			peer->Encode(&pck, sizeof(pck));
		}

		TPacketGuildWarScore pck;
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
	while (!m_pqOnWar.empty())
	{
		if (!m_pqOnWar.top().second->bEnd)
			delete m_pqOnWar.top().second;

		m_pqOnWar.pop();
	}
}

TGuild & CGuildManager::TouchGuild(DWORD GID)
{
	itertype(m_map_kGuild) it = m_map_kGuild.find(GID);

	if (it != m_map_kGuild.end())
		return it->second;

	TGuild info;
	m_map_kGuild.insert(std::map<DWORD, TGuild>::value_type(GID, info));
	return m_map_kGuild[GID];
}

void CGuildManager::ParseResult(SQLResult * pRes)
{
	MYSQL_ROW row;

	while ((row = mysql_fetch_row(pRes->pSQLResult)))
	{
		DWORD GID = strtoul(row[0], NULL, 10);

		TGuild & r_info = TouchGuild(GID);

		strlcpy(r_info.szName, row[1], sizeof(r_info.szName));
		str_to_number(r_info.ladder_point, row[2]);
		str_to_number(r_info.win, row[3]);
		str_to_number(r_info.draw, row[4]);
		str_to_number(r_info.loss, row[5]);
		str_to_number(r_info.gold, row[6]);
		str_to_number(r_info.level, row[7]);

		sys_log(0,
				"GuildWar: %-24s ladder %-5d win %-3d draw %-3d loss %-3d",
				r_info.szName,
				r_info.ladder_point,
				r_info.win,
				r_info.draw,
				r_info.loss);
	}
}

void CGuildManager::Initialize()
{
	char szQuery[1024];

	snprintf(szQuery, sizeof(szQuery), "SELECT id, name, ladder_point, win, draw, loss, gold, level FROM guild%s", GetTablePostfix());

	std::auto_ptr<SQLMsg> pmsg(CDBManager::instance().DirectQuery(szQuery));

	if (pmsg->Get()->uiNumRows)
		ParseResult(pmsg->Get());

	char str[128 + 1];

	if (!CConfig::instance().GetValue("POLY_POWER", str, sizeof(str)))
		*str = '\0';

	if (!polyPower.Analyze(str))
		sys_err("cannot set power poly: %s", str);
	else
		sys_log(0, "POWER_POLY: %s", str);

	if (!CConfig::instance().GetValue("POLY_HANDICAP", str, sizeof(str)))
		*str = '\0';

	if (!polyHandicap.Analyze(str))
		sys_err("cannot set handicap poly: %s", str);
	else
		sys_log(0, "HANDICAP_POLY: %s", str);

	QueryRanking();
#ifdef GUILD_WAR_COUNTER
	BootReserveWar();
#endif
}

void CGuildManager::Load(DWORD dwGuildID)
{
	char szQuery[1024];
	snprintf(szQuery, sizeof(szQuery), "SELECT id, name, ladder_point, win, draw, loss, gold, level FROM guild%s WHERE id=%u", GetTablePostfix(), dwGuildID);
	std::auto_ptr<SQLMsg> pmsg(CDBManager::instance().DirectQuery(szQuery));

	if (pmsg->Get()->uiNumRows)
		ParseResult(pmsg->Get());
}

void CGuildManager::QueryRanking()
{
	char szQuery[256];
	snprintf(szQuery, sizeof(szQuery), "SELECT id,name,ladder_point FROM guild%s ORDER BY ladder_point DESC LIMIT 20", GetTablePostfix());

	CDBManager::instance().ReturnQuery(szQuery, QID_GUILD_RANKING, 0, 0);
}

int CGuildManager::GetRanking(DWORD dwGID)
{
	itertype(map_kLadderPointRankingByGID) it = map_kLadderPointRankingByGID.find(dwGID);

	if (it == map_kLadderPointRankingByGID.end())
		return GUILD_RANK_MAX_NUM;

	return MINMAX(0, it->second, GUILD_RANK_MAX_NUM);
}

void CGuildManager::ResultRanking(MYSQL_RES * pRes)
{
	if (!pRes)
		return;

	int iLastLadderPoint = -1;
	int iRank = 0;

	map_kLadderPointRankingByGID.clear();

	MYSQL_ROW row;

	while ((row = mysql_fetch_row(pRes)))
	{
		DWORD	dwGID = 0; str_to_number(dwGID, row[0]);
		int	iLadderPoint = 0; str_to_number(iLadderPoint, row[2]);

		if (iLadderPoint != iLastLadderPoint)
			++iRank;

		sys_log(0, "GUILD_RANK: %-24s %2d %d", row[1], iRank, iLadderPoint);

		map_kLadderPointRankingByGID.insert(std::make_pair(dwGID, iRank));
	}
}

void CGuildManager::Update()
{
	ProcessReserveWar(); 

	time_t now = CClientManager::instance().GetCurrentTime();

	if (!m_pqOnWar.empty())
	{
		// UNKNOWN_GUILD_MANAGE_UPDATE_LOG
		/*
		   sys_log(0, "GuildManager::Update size %d now %d top %d, %s(%u) vs %s(%u)",
		   m_WarMap.size(),
		   now,
		   m_pqOnWar.top().first,
		   m_map_kGuild[m_pqOnWar.top().second->GID[0]].szName,
		   m_pqOnWar.top().second->GID[0],
		   m_map_kGuild[m_pqOnWar.top().second->GID[1]].szName,
		   m_pqOnWar.top().second->GID[1]);
		   */
		// END_OF_UNKNOWN_GUILD_MANAGE_UPDATE_LOG

		while (!m_pqOnWar.empty() && (m_pqOnWar.top().first <= now || (m_pqOnWar.top().second && m_pqOnWar.top().second->bEnd)))
		{
			TGuildWarPQElement * e = m_pqOnWar.top().second;

			m_pqOnWar.pop();

			if (e)
			{
				if (!e->bEnd)
					WarEnd(e->GID[0], e->GID[1], false);

				delete e;
			}
		}
	}

	// GUILD_SKILL_COOLTIME_BUG_FIX
	while (!m_pqSkill.empty() && m_pqSkill.top().first <= now)
	{
		const TGuildSkillUsed& s = m_pqSkill.top().second;
		CClientManager::instance().SendGuildSkillUsable(s.GID, s.dwSkillVnum, true);
		m_pqSkill.pop();
	}
	// END_OF_GUILD_SKILL_COOLTIME_BUG_FIX

	while (!m_pqWaitStart.empty() && m_pqWaitStart.top().first <= now)
	{
		const TGuildWaitStartInfo & ws = m_pqWaitStart.top().second;
		m_pqWaitStart.pop();

#ifdef __IMPROVED_GUILD_WAR__
		StartWar(ws.bType, ws.GID[0], ws.GID[1], ws.pkReserve, ws.iMaxPlayer, ws.iMaxScore, ws.flags, ws.custom_map_index); // insert new element to m_WarMap and m_pqOnWar
#else
		StartWar(ws.bType, ws.GID[0], ws.GID[1], ws.pkReserve); // insert new element to m_WarMap and m_pqOnWar
#endif

		if (ws.lInitialScore)
		{
			UpdateScore(ws.GID[0], ws.GID[1], ws.lInitialScore, 0);
			UpdateScore(ws.GID[1], ws.GID[0], ws.lInitialScore, 0);
		}

		TPacketGuildWar p;

		p.bType		= ws.bType;
		p.bWar		= GUILD_WAR_ON_WAR;
		p.dwGuildFrom	= ws.GID[0];
		p.dwGuildTo	= ws.GID[1];
#ifdef __IMPROVED_GUILD_WAR__
		p.iMaxPlayer = ws.iMaxPlayer;
		p.iMaxScore = ws.iMaxScore;
		p.flags = ws.flags; 
		p.custom_map_index = ws.custom_map_index; 
#endif

		CClientManager::instance().ForwardPacket(HEADER_DG_GUILD_WAR, &p, sizeof(p));
		sys_log(0, "GuildWar: GUILD sending start of wait start war %d %d", ws.GID[0], ws.GID[1]);
	}
}

#define for_all(cont, it) for (__typeof((cont).begin()) it = (cont).begin(); it != (cont).end(); ++it)

void CGuildManager::OnSetup(CPeer* peer)
{
	for_all(m_WarMap, it_cont)
		for_all(it_cont->second, it)
		{
			DWORD g1 = it_cont->first;
			DWORD g2 = it->first;
			TGuildWarPQElement* p = it->second.pElement;

			if (!p || p->bEnd)
				continue;

#ifdef __IMPROVED_GUILD_WAR__
			FSendPeerWar(p->bType, GUILD_WAR_ON_WAR, g1, g2, p->iMaxPlayer, p->iMaxScore, p->flags, p->custom_map_index) (peer);
#else
			FSendPeerWar(p->bType, GUILD_WAR_ON_WAR, g1, g2) (peer);
#endif
			FSendGuildWarScore(p->GID[0], p->GID[1], p->iScore[0], p->iBetScore[0]);
			FSendGuildWarScore(p->GID[1], p->GID[0], p->iScore[1], p->iBetScore[1]);

		}

	for_all(m_DeclareMap, it)
	{
#ifdef __IMPROVED_GUILD_WAR__
		FSendPeerWar(it->bType, GUILD_WAR_SEND_DECLARE, it->dwGuildID[0], it->dwGuildID[1], it->iMaxPlayer, it->iMaxScore, it->flags, it->custom_map_index) (peer);
#else
		FSendPeerWar(it->bType, GUILD_WAR_SEND_DECLARE, it->dwGuildID[0], it->dwGuildID[1]) (peer);
#endif
	}

	for_all(m_map_kWarReserve, it)
	{
		it->second->OnSetup(peer);
	}
}

void CGuildManager::GuildWarWin(DWORD GID)
{
	itertype(m_map_kGuild) it = m_map_kGuild.find(GID);

	if (it == m_map_kGuild.end())
		return;

	++it->second.win;

	char buf[1024];
	snprintf(buf, sizeof(buf), "UPDATE guild%s SET win=%d WHERE id=%u", GetTablePostfix(), it->second.win, GID);
	CDBManager::instance().AsyncQuery(buf);
}

void CGuildManager::GuildWarLose(DWORD GID)
{
	itertype(m_map_kGuild) it = m_map_kGuild.find(GID);

	if (it == m_map_kGuild.end())
		return;

	++it->second.loss;

	char buf[1024];
	snprintf(buf, sizeof(buf), "UPDATE guild%s SET loss=%d WHERE id=%u", GetTablePostfix(), it->second.loss, GID);
	CDBManager::instance().AsyncQuery(buf);
}

void CGuildManager::GuildWarDraw(DWORD GID)
{
	itertype(m_map_kGuild) it = m_map_kGuild.find(GID);

	if (it == m_map_kGuild.end())
		return;

	++it->second.draw;

	char buf[1024];
	snprintf(buf, sizeof(buf), "UPDATE guild%s SET draw=%d WHERE id=%u", GetTablePostfix(), it->second.draw, GID);
	CDBManager::instance().AsyncQuery(buf);
}

bool CGuildManager::IsHalfWinLadderPoint(DWORD dwGuildWinner, DWORD dwGuildLoser)
{
	DWORD GID1 = dwGuildWinner;
	DWORD GID2 = dwGuildLoser;

	if (GID1 > GID2)
		std::swap(GID1, GID2);

	itertype(m_mapGuildWarEndTime[GID1]) it = m_mapGuildWarEndTime[GID1].find(GID2);

	if (it != m_mapGuildWarEndTime[GID1].end() &&
			it->second + GUILD_WAR_LADDER_HALF_PENALTY_TIME > CClientManager::instance().GetCurrentTime())
		return true;

	return false;
}

void CGuildManager::ProcessDraw(DWORD dwGuildID1, DWORD dwGuildID2)
{
	sys_log(0, "GuildWar: \tLa guerra tra %d e %d si e' conclusa in pareggio.", dwGuildID1, dwGuildID2);

	GuildWarDraw(dwGuildID1);
	GuildWarDraw(dwGuildID2);
	ChangeLadderPoint(dwGuildID1, 0);
	ChangeLadderPoint(dwGuildID2, 0);

	QueryRanking();
}

void CGuildManager::ProcessWinLose(DWORD dwGuildWinner, DWORD dwGuildLoser)
{
	GuildWarWin(dwGuildWinner);
	GuildWarLose(dwGuildLoser);
	sys_log(0, "GuildWar: \tWinner : %d Loser : %d", dwGuildWinner, dwGuildLoser);

#ifdef GUILD_RANK_EFFECT
	ChangeLadderPoint(dwGuildWinner, 3);
	ChangeLadderPoint(dwGuildLoser, -1);
#else
	int iPoint = GetLadderPoint(dwGuildLoser);
	int gain = (int)(iPoint * 0.05);
	int loss = (int)(iPoint * 0.07);

	if (IsHalfWinLadderPoint(dwGuildWinner, dwGuildLoser))
		gain /= 2;

	sys_log(0, "GuildWar: \tgain : %d loss : %d", gain, loss);
	ChangeLadderPoint(dwGuildWinner, gain);
	ChangeLadderPoint(dwGuildLoser, -loss);
#endif

	QueryRanking();
}

void CGuildManager::RemoveWar(DWORD GID1, DWORD GID2)
{
	sys_log(0, "GuildWar: RemoveWar(%d, %d)", GID1, GID2);

	if (GID1 > GID2)
		std::swap(GID2, GID1);

	itertype(m_WarMap[GID1]) it = m_WarMap[GID1].find(GID2);

	if (it == m_WarMap[GID1].end())
	{
		if (m_WarMap[GID1].empty())
			m_WarMap.erase(GID1);

		return;
	}

	if (it->second.pElement)
		it->second.pElement->bEnd = true;

	m_mapGuildWarEndTime[GID1][GID2] = CClientManager::instance().GetCurrentTime();

	m_WarMap[GID1].erase(it);

	if (m_WarMap[GID1].empty())
		m_WarMap.erase(GID1);
}

//

//
void CGuildManager::WarEnd(DWORD GID1, DWORD GID2, bool bForceDraw)
{
	if (GID1 > GID2)
		std::swap(GID2, GID1);

	sys_log(0, "GuildWar: WarEnd %d %d", GID1, GID2);

	itertype(m_WarMap[GID1]) itWarMap = m_WarMap[GID1].find(GID2);

	if (itWarMap == m_WarMap[GID1].end())
	{
		sys_err("GuildWar: war not exist or already ended. [1]");
		return;
	}

	TGuildWarInfo gwi = itWarMap->second;
	TGuildWarPQElement * pData = gwi.pElement;

	if (!pData || pData->bEnd)
	{
		sys_err("GuildWar: war not exist or already ended. [2]");
		return;
	}

	DWORD win_guild = pData->GID[0];
	DWORD lose_guild = pData->GID[1];

	bool bDraw = false;

	if (!bForceDraw) 
	{
		if (pData->iScore[0] > pData->iScore[1])
		{
			win_guild = pData->GID[0];
			lose_guild = pData->GID[1];
		}
		else if (pData->iScore[1] > pData->iScore[0])
		{
			win_guild = pData->GID[1];
			lose_guild = pData->GID[0];
		}
		else
			bDraw = true;
	}
	else 
		bDraw = true;

	if (bDraw)
		ProcessDraw(win_guild, lose_guild);
	else
		ProcessWinLose(win_guild, lose_guild);

	// DB 서버에서 자체적으로 끝낼 때도 있기 때문에 따로 패킷을 보내줘야 한다.
#ifdef __IMPROVED_GUILD_WAR__
	CClientManager::instance().for_each_peer(FSendPeerWar(0, GUILD_WAR_END, GID1, GID2, pData->iMaxPlayer, pData->iMaxScore, pData->flags, pData->custom_map_index));
#else
	CClientManager::instance().for_each_peer(FSendPeerWar(0, GUILD_WAR_END, GID1, GID2));
#endif

	RemoveWar(GID1, GID2);
}

//

//
void CGuildManager::RecvWarOver(DWORD dwGuildWinner, DWORD dwGuildLoser, bool bDraw, long lWarPrice
#ifdef __IMPROVED_GUILD_WAR__
	, int iMaxPlayer, int iMaxScore, DWORD flags, int custom_map_index
#endif
)
{
	sys_log(0, "GuildWar: RecvWarOver : winner %u vs %u draw? %d war_price %d", dwGuildWinner, dwGuildLoser, bDraw ? 1 : 0, lWarPrice);

	DWORD GID1 = dwGuildWinner;
	DWORD GID2 = dwGuildLoser;

	if (GID1 > GID2)
		std::swap(GID1, GID2);

	itertype(m_WarMap[GID1]) it = m_WarMap[GID1].find(GID2);

	if (it == m_WarMap[GID1].end())
		return;

	TGuildWarInfo & gw = it->second;

	// Award
	if (bDraw)
	{
		// give bet money / 2 to both guild
		DepositMoney(dwGuildWinner, lWarPrice / 2);
		DepositMoney(dwGuildLoser, lWarPrice / 2);
		ProcessDraw(dwGuildWinner, dwGuildLoser);
	}
	else
	{
		// give bet money to winner guild
		DepositMoney(dwGuildWinner, lWarPrice);
		ProcessWinLose(dwGuildWinner, dwGuildLoser);
	}

	if (gw.pkReserve)
	{
		if (bDraw || !gw.pElement)
			gw.pkReserve->Draw();
		else if (gw.pElement->bType == GUILD_WAR_TYPE_BATTLE)
			gw.pkReserve->End(gw.pElement->iBetScore[0], gw.pElement->iBetScore[1]);
		
#ifdef GUILD_WAR_COUNTER
		time_t now = time(0);
		struct tm tstruct;
		char buf[24];
		tstruct = *localtime(&now);
		strftime(buf, sizeof(buf), "%X-%d.%m.%Y", &tstruct);
		
		char query_buf[124];
		snprintf(query_buf, sizeof(query_buf), "UPDATE player.guild_war_reservation SET `time_str`='%s' WHERE id=%u", buf, gw.pkReserve->GetDataRef().dwID);
		delete CDBManager::instance().DirectQuery(query_buf);
#endif
	}

	RemoveWar(GID1, GID2);
}

void CGuildManager::RecvWarEnd(DWORD GID1, DWORD GID2
#ifdef __IMPROVED_GUILD_WAR__
	, int iMaxPlayer, int iMaxScore, DWORD flags, int custom_map_index
#endif
)
{
	sys_log(0, "GuildWar: RecvWarEnded : %u vs %u", GID1, GID2);
	WarEnd(GID1, GID2, true); 
}

void CGuildManager::StartWar(BYTE bType, DWORD GID1, DWORD GID2, CGuildWarReserve * pkReserve
#ifdef __IMPROVED_GUILD_WAR__
	, int iMaxPlayer, int iMaxScore, DWORD flags, int custom_map_index
#endif
)
{

#ifdef __IMPROVED_GUILD_WAR__
	sys_log(0, "GuildWar: StartWar(%d,%d,%d,%d,%d,%ld,%d)", bType, GID1, GID2, iMaxPlayer, iMaxScore, flags, custom_map_index);
#else
	sys_log(0, "GuildWar: StartWar(%d,%d,%d)", bType, GID1, GID2);
#endif

	if (GID1 > GID2)
		std::swap(GID1, GID2);

	TGuildWarInfo & gw = m_WarMap[GID1][GID2]; // map insert

	if (bType == GUILD_WAR_TYPE_FIELD)
		gw.tEndTime = CClientManager::instance().GetCurrentTime() + GUILD_WAR_DURATION;
	else
		gw.tEndTime = CClientManager::instance().GetCurrentTime() + 172800;

#ifdef __IMPROVED_GUILD_WAR__
	gw.pElement = new TGuildWarPQElement(bType, GID1, GID2, iMaxPlayer, iMaxScore, flags, custom_map_index);
#else
	gw.pElement = new TGuildWarPQElement(bType, GID1, GID2);
#endif
	gw.pkReserve = pkReserve;

	m_pqOnWar.push(std::make_pair(gw.tEndTime, gw.pElement));
}

void CGuildManager::UpdateScore(DWORD dwGainGID, DWORD dwOppGID, int iScoreDelta, int iBetScoreDelta)
{
	DWORD GID1 = dwGainGID;
	DWORD GID2 = dwOppGID;

	if (GID1 > GID2)
		std::swap(GID1, GID2);

	itertype(m_WarMap[GID1]) it = m_WarMap[GID1].find(GID2);

	if (it != m_WarMap[GID1].end())
	{
		TGuildWarPQElement * p = it->second.pElement;

		if (!p || p->bEnd)
		{
			sys_err("GuildWar: war not exist or already ended.");
			return;
		}

		int iNewScore = 0;
		int iNewBetScore = 0;

		if (p->GID[0] == dwGainGID)
		{
			p->iScore[0] += iScoreDelta;
			p->iBetScore[0] += iBetScoreDelta;

			iNewScore = p->iScore[0];
			iNewBetScore = p->iBetScore[0];
		}
		else
		{
			p->iScore[1] += iScoreDelta;
			p->iBetScore[1] += iBetScoreDelta;

			iNewScore = p->iScore[1];
			iNewBetScore = p->iBetScore[1];
		}

#ifdef __IMPROVED_GUILD_WAR__
		sys_log(0, "GuildWar: SendGuildWarScore guild %u wartype %u score_delta %d betscore_delta %d result %u, %u max score: %u",
				dwGainGID, p->bType, iScoreDelta, iBetScoreDelta, iNewScore, iNewBetScore, p->iMaxScore);
#else
		sys_log(0, "GuildWar: SendGuildWarScore guild %u wartype %u score_delta %d betscore_delta %d result %u, %u",
				dwGainGID, p->bType, iScoreDelta, iBetScoreDelta, iNewScore, iNewBetScore);
#endif

		CClientManager::instance().for_each_peer(FSendGuildWarScore(dwGainGID, dwOppGID, iNewScore, iNewBetScore));
	}
}

void CGuildManager::AddDeclare(BYTE bType, DWORD guild_from, DWORD guild_to
#ifdef __IMPROVED_GUILD_WAR__
	, int iMaxPlayer, int iMaxScore, DWORD flags, int custom_map_index
#endif
)
{
#ifdef __IMPROVED_GUILD_WAR__
	TGuildDeclareInfo di(bType, guild_from, guild_to, iMaxPlayer, iMaxScore, flags, custom_map_index);
#else
	TGuildDeclareInfo di(bType, guild_from, guild_to);
#endif

	if (m_DeclareMap.find(di) == m_DeclareMap.end())
		m_DeclareMap.insert(di);

#ifdef __IMPROVED_GUILD_WAR__
	sys_log(0, "GuildWar: AddDeclare(Type:%d,from:%d,to:%d,maxplayer:%d,maxscore:%d,info:%ld-%d)", bType, guild_from, guild_to, iMaxPlayer, iMaxScore, flags, custom_map_index);
#else
	sys_log(0, "GuildWar: AddDeclare(Type:%d,from:%d,to:%d)", bType, guild_from, guild_to);
#endif
}

void CGuildManager::RemoveDeclare(DWORD guild_from, DWORD guild_to
#ifdef __IMPROVED_GUILD_WAR__
	, int iMaxPlayer, int iMaxScore, DWORD flags, int custom_map_index
#endif
)
{
#ifdef __IMPROVED_GUILD_WAR__
	__typeof(m_DeclareMap.begin()) it = m_DeclareMap.find(TGuildDeclareInfo(0, guild_from, guild_to, iMaxPlayer, iMaxScore, flags, custom_map_index));
#else
	__typeof(m_DeclareMap.begin()) it = m_DeclareMap.find(TGuildDeclareInfo(0, guild_from, guild_to));
#endif

	if (it != m_DeclareMap.end())
		m_DeclareMap.erase(it);

#ifdef __IMPROVED_GUILD_WAR__
	it = m_DeclareMap.find(TGuildDeclareInfo(0, guild_to, guild_from, iMaxPlayer, iMaxScore, flags, custom_map_index));
#else
	it = m_DeclareMap.find(TGuildDeclareInfo(0,guild_to, guild_from));
#endif

	if (it != m_DeclareMap.end())
		m_DeclareMap.erase(it);

#ifdef __IMPROVED_GUILD_WAR__
	sys_log(0, "GuildWar: RemoveDeclare(from:%d,to:%d,maxplayer:%d,maxscore:%d,info:%ld-%d)", guild_from, guild_to, iMaxPlayer, iMaxScore, flags, custom_map_index);
#else
	sys_log(0, "GuildWar: RemoveDeclare(from:%d,to:%d)", guild_from, guild_to);
#endif
}

bool CGuildManager::TakeBetPrice(DWORD dwGuildTo, DWORD dwGuildFrom, long lWarPrice)
{
	itertype(m_map_kGuild) it_from = m_map_kGuild.find(dwGuildFrom);
	itertype(m_map_kGuild) it_to = m_map_kGuild.find(dwGuildTo);

	if (it_from == m_map_kGuild.end() || it_to == m_map_kGuild.end())
	{
		sys_log(0, "TakeBetPrice: guild not exist %u %u",
				dwGuildFrom, dwGuildTo);
		return false;
	}

	if (it_from->second.gold < lWarPrice || it_to->second.gold < lWarPrice)
	{
		sys_log(0, "TakeBetPrice: not enough gold %u %d to %u %d",
				dwGuildFrom, it_from->second.gold, dwGuildTo, it_to->second.gold);
		return false;
	}

	it_from->second.gold -= lWarPrice;
	it_to->second.gold -= lWarPrice;

	MoneyChange(dwGuildFrom, it_from->second.gold);
	MoneyChange(dwGuildTo, it_to->second.gold);
	return true;
}

bool CGuildManager::WaitStart(TPacketGuildWar * p)
{
	if (p->lWarPrice > 0)
		if (!TakeBetPrice(p->dwGuildFrom, p->dwGuildTo, p->lWarPrice))
			return false;

	DWORD dwCurTime = CClientManager::instance().GetCurrentTime();

#ifdef __IMPROVED_GUILD_WAR__
	TGuildWaitStartInfo info(p->bType, p->dwGuildFrom, p->dwGuildTo, p->lWarPrice, p->lInitialScore, NULL, p->iMaxPlayer, p->iMaxScore, p->flags, p->custom_map_index);
#else
	TGuildWaitStartInfo info(p->bType, p->dwGuildFrom, p->dwGuildTo, p->lWarPrice, p->lInitialScore, NULL);
#endif
	m_pqWaitStart.push(std::make_pair(dwCurTime + GetGuildWarWaitStartDuration(), info));

	sys_log(0,
			"GuildWar: WaitStart g1 %d g2 %d price %d start at %u"
#ifdef __IMPROVED_GUILD_WAR__
			"max player %d max score %d flags %ld custom map idx %d"
#endif
		,
			p->dwGuildFrom,
			p->dwGuildTo,
			p->lWarPrice,
			dwCurTime + GetGuildWarWaitStartDuration()
#ifdef __IMPROVED_GUILD_WAR__
			, p->iMaxPlayer,
			p->iMaxScore,
			p->flags,
			p->custom_map_index
#endif
	);

	return true;
}

int CGuildManager::GetLadderPoint(DWORD GID)
{
	itertype(m_map_kGuild) it = m_map_kGuild.find(GID);

	if (it == m_map_kGuild.end())
		return 0;

	return it->second.ladder_point;
}

void CGuildManager::ChangeLadderPoint(DWORD GID, int change)
{
	itertype(m_map_kGuild) it = m_map_kGuild.find(GID);

	if (it == m_map_kGuild.end())
		return;

	TGuild & r = it->second;

	r.ladder_point += change;

	if (r.ladder_point < 0)
		r.ladder_point = 0;

	char buf[1024];
	snprintf(buf, sizeof(buf), "UPDATE guild%s SET ladder_point=%d WHERE id=%u", GetTablePostfix(), r.ladder_point, GID);
	CDBManager::instance().AsyncQuery(buf);

	sys_log(0, "GuildManager::ChangeLadderPoint %u %d", GID, r.ladder_point);
	sys_log(0, "%s", buf);

	
	TPacketGuildLadder p;

	p.dwGuild = GID;
	p.lLadderPoint = r.ladder_point;
	p.lWin = r.win;
	p.lDraw = r.draw;
	p.lLoss = r.loss;

	CClientManager::instance().ForwardPacket(HEADER_DG_GUILD_LADDER, &p, sizeof(TPacketGuildLadder));
}

void CGuildManager::UseSkill(DWORD GID, DWORD dwSkillVnum, DWORD dwCooltime)
{
	// GUILD_SKILL_COOLTIME_BUG_FIX
	sys_log(0, "UseSkill(gid=%d, skill=%d) CoolTime(%d:%d)", GID, dwSkillVnum, dwCooltime, CClientManager::instance().GetCurrentTime() + dwCooltime);
	m_pqSkill.push(std::make_pair(CClientManager::instance().GetCurrentTime() + dwCooltime, TGuildSkillUsed(GID, dwSkillVnum)));
	// END_OF_GUILD_SKILL_COOLTIME_BUG_FIX
}

void CGuildManager::MoneyChange(DWORD dwGuild, DWORD dwGold)
{
	sys_log(0, "GuildManager::MoneyChange %d %d", dwGuild, dwGold);

	TPacketDGGuildMoneyChange p;
	p.dwGuild = dwGuild;
	p.iTotalGold = dwGold;
	CClientManager::instance().ForwardPacket(HEADER_DG_GUILD_MONEY_CHANGE, &p, sizeof(p));

	char buf[1024];
	snprintf(buf, sizeof(buf), "UPDATE guild%s SET gold=%u WHERE id = %u", GetTablePostfix(), dwGold, dwGuild);
	CDBManager::instance().AsyncQuery(buf);
}

void CGuildManager::DepositMoney(DWORD dwGuild, INT iGold)
{
	if (iGold <= 0)
		return;

	itertype(m_map_kGuild) it = m_map_kGuild.find(dwGuild);

	if (it == m_map_kGuild.end())
	{
		sys_err("No guild by id %u", dwGuild);
		return;
	}

	it->second.gold += iGold;
	sys_log(0, "GUILD: %u Deposit %u Total %d", dwGuild, iGold, it->second.gold);

	MoneyChange(dwGuild, it->second.gold);
}

void CGuildManager::WithdrawMoney(CPeer* peer, DWORD dwGuild, INT iGold)
{
	itertype(m_map_kGuild) it = m_map_kGuild.find(dwGuild);

	if (it == m_map_kGuild.end())
	{
		sys_err("No guild by id %u", dwGuild);
		return;
	}

	
	if (it->second.gold >= iGold)
	{
		it->second.gold -= iGold;
		sys_log(0, "GUILD: %u Withdraw %d Total %d", dwGuild, iGold, it->second.gold);

		TPacketDGGuildMoneyWithdraw p;
		p.dwGuild = dwGuild;
		p.iChangeGold = iGold;

		peer->EncodeHeader(HEADER_DG_GUILD_WITHDRAW_MONEY_GIVE, 0, sizeof(TPacketDGGuildMoneyWithdraw));
		peer->Encode(&p, sizeof(TPacketDGGuildMoneyWithdraw));
	}
}

void CGuildManager::WithdrawMoneyReply(DWORD dwGuild, BYTE bGiveSuccess, INT iGold)
{
	itertype(m_map_kGuild) it = m_map_kGuild.find(dwGuild);

	if (it == m_map_kGuild.end())
		return;

	sys_log(0, "GuildManager::WithdrawMoneyReply : guild %u success %d gold %d", dwGuild, bGiveSuccess, iGold);

	if (!bGiveSuccess)
		it->second.gold += iGold;
	else
		MoneyChange(dwGuild, it->second.gold);
}

//

//
const int c_aiScoreByLevel[GUILD_MAX_LEVEL+1] =
{
	500,	// level 0 = 500 probably error
	500,	// 1
	1000,
	2000,
	3000,
	4000,
	6000,
	8000,
	10000,
	12000,
	15000,	// 10
	18000,
	21000,
	24000,
	28000,
	32000,
	36000,
	40000,
	45000,
	50000,
	55000,
};

const int c_aiScoreByRanking[GUILD_RANK_MAX_NUM+1] =
{
	0,
	55000,	
	50000,
	45000,
	40000,
	36000,
	32000,
	28000,
	24000,
	21000,
	18000,	// 10À§
	15000,
	12000,
	10000,
	8000,
	6000,
	4000,
	3000,
	2000,
	1000,
	500		// 20À§
};


#ifdef GUILD_WAR_COUNTER
void CGuildManager::BootReserveWar()
{
	const char* c_apszQuery =
	{
		"SELECT id, guild1, guild2, UNIX_TIMESTAMP(time), type, warprice, initscore, bet_from, bet_to, power1, power2, handicap"
#ifdef __IMPROVED_GUILD_WAR__
		", max_player, max_score, war_flags, war_midx"
#endif
		", time_str, winner"
		" FROM guild_war_reservation"
	};

	char szQuery[120];
	snprintf(szQuery, sizeof(szQuery), "SELECT id FROM player.guild_war_reservation WHERE time < DATE_SUB(NOW(), INTERVAL 7 DAY)");
	std::unique_ptr<SQLMsg> pMsgDeletes(CDBManager::instance().DirectQuery(szQuery));
	if (pMsgDeletes->Get()->uiNumRows != 0)
	{
		MYSQL_ROW row_delete;
		while ((row_delete = mysql_fetch_row(pMsgDeletes->Get()->pSQLResult)))
		{
			DWORD warID;
			str_to_number(warID, row_delete[0]);
			snprintf(szQuery, sizeof(szQuery), "INSERT INTO player.guild_war_reservation_deleted SELECT * FROM player.guild_war_reservation WHERE id=%d", warID);
			delete CDBManager::instance().DirectQuery(szQuery);
			snprintf(szQuery, sizeof(szQuery), "DELETE FROM player.guild_war_reservation WHERE id=%d", warID);
			delete CDBManager::instance().DirectQuery(szQuery);
			snprintf(szQuery, sizeof(szQuery), "INSERT INTO player.guild_counter_ranking_deleted SELECT * FROM player.guild_counter_ranking WHERE warid=%d", warID);
			delete CDBManager::instance().DirectQuery(szQuery);
			snprintf(szQuery, sizeof(szQuery), "DELETE FROM player.guild_counter_ranking WHERE warid=%d", warID);
			delete CDBManager::instance().DirectQuery(szQuery);

			sys_log(0, "GuildWarStatistics: old war removing. id %d", warID);
		}
	}

	std::unique_ptr<SQLMsg> pmsg(CDBManager::instance().DirectQuery(c_apszQuery));
	if (pmsg->Get()->uiNumRows == 0)
		return;
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(pmsg->Get()->pSQLResult)))
	{
		int col = 0;
		TGuildWarReserve t;
		str_to_number(t.dwID, row[col++]);
		str_to_number(t.dwGuildFrom, row[col++]);
		str_to_number(t.dwGuildTo, row[col++]);
		str_to_number(t.dwTime, row[col++]);
		str_to_number(t.bType, row[col++]);
		str_to_number(t.lWarPrice, row[col++]);
		str_to_number(t.lInitialScore, row[col++]);
		str_to_number(t.dwBetFrom, row[col++]);
		str_to_number(t.dwBetTo, row[col++]);
		str_to_number(t.lPowerFrom, row[col++]);
		str_to_number(t.lPowerTo, row[col++]);
		str_to_number(t.lHandicap, row[col++]);
#ifdef __IMPROVED_GUILD_WAR__
		str_to_number(t.iMaxPlayer, row[col++]);
		str_to_number(t.iMaxScore, row[col++]);
		str_to_number(t.flags, row[col++]);
		str_to_number(t.custom_map_index, row[col++]);

#endif
		strlcpy(t.date, row[col++], sizeof(t.date));
		str_to_number(t.winner, row[col++]);
		LoadWarStatisticsInfo(t.dwID, t, false);
		t.bStarted = 0;
	}

}
#else
void CGuildManager::BootReserveWar()
{
	const char * c_apszQuery[2] =
	{
#ifdef __IMPROVED_GUILD_WAR__
		"SELECT id, guild1, guild2, UNIX_TIMESTAMP(time), type, warprice, initscore, bet_from, bet_to, power1, power2, handicap, max_player, max_score, war_flags, war_midx FROM guild_war_reservation WHERE started=1 AND winner=-1",
		"SELECT id, guild1, guild2, UNIX_TIMESTAMP(time), type, warprice, initscore, bet_from, bet_to, power1, power2, handicap, max_player, max_score, war_flags, war_midx FROM guild_war_reservation WHERE started=0"
#else
		"SELECT id, guild1, guild2, UNIX_TIMESTAMP(time), type, warprice, initscore, bet_from, bet_to, power1, power2, handicap FROM guild_war_reservation WHERE started=1 AND winner=-1",
		"SELECT id, guild1, guild2, UNIX_TIMESTAMP(time), type, warprice, initscore, bet_from, bet_to, power1, power2, handicap FROM guild_war_reservation WHERE started=0"
#endif
	};

	for (int i = 0; i < 2; ++i)
	{
		std::auto_ptr<SQLMsg> pmsg(CDBManager::instance().DirectQuery(c_apszQuery[i]));

		if (pmsg->Get()->uiNumRows == 0)
			continue;

		MYSQL_ROW row;

		while ((row = mysql_fetch_row(pmsg->Get()->pSQLResult)))
		{
			int col = 0;

			TGuildWarReserve t;

			str_to_number(t.dwID, row[col++]);
			str_to_number(t.dwGuildFrom, row[col++]);
			str_to_number(t.dwGuildTo, row[col++]);
			str_to_number(t.dwTime, row[col++]);
			str_to_number(t.bType, row[col++]);
			str_to_number(t.lWarPrice, row[col++]);
			str_to_number(t.lInitialScore, row[col++]);
			str_to_number(t.dwBetFrom, row[col++]);
			str_to_number(t.dwBetTo, row[col++]);
			str_to_number(t.lPowerFrom, row[col++]);
			str_to_number(t.lPowerTo, row[col++]);
			str_to_number(t.lHandicap, row[col++]);
#ifdef __IMPROVED_GUILD_WAR__
			str_to_number(t.iMaxPlayer, row[col++]);
			str_to_number(t.iMaxScore, row[col++]);
			str_to_number(t.flags, row[col++]);
			str_to_number(t.custom_map_index, row[col++]);
#endif
			t.bStarted = 0;

			CGuildWarReserve * pkReserve = new CGuildWarReserve(t);

			char buf[512];
#ifdef __IMPROVED_GUILD_WAR__
			snprintf(buf, sizeof(buf), "GuildWar: BootReserveWar : step %d id %u GID1 %u GID2 %u max player %d max score %d flags %d custommapidx %d", i, t.dwID, t.dwGuildFrom, t.dwGuildTo, t.iMaxPlayer, t.iMaxScore, t.flags, t.custom_map_index);
#else
			snprintf(buf, sizeof(buf), "GuildWar: BootReserveWar : step %d id %u GID1 %u GID2 %u", i, t.dwID, t.dwGuildFrom, t.dwGuildTo);
#endif
			// i == 0 이면 길드전 도중 DB가 튕긴 것이므로 무승부 처리한다.
			// 또는, 5분 이하 남은 예약 길드전도 무승부 처리한다. (각자의 배팅액을 돌려준다)
			//if (i == 0 || (int) t.dwTime - CClientManager::instance().GetCurrentTime() < 60 * 5)
			if (i == 0 || (int) t.dwTime - CClientManager::instance().GetCurrentTime() < 0)
			{
				if (i == 0)
					sys_log(0, "%s : DB was shutdowned while war is being.", buf);
				else
					sys_log(0, "%s : left time lower than 5 minutes, will be canceled", buf);

				pkReserve->Draw();
				delete pkReserve;
			}
			else
			{
				sys_log(0, "%s : OK", buf);
				m_map_kWarReserve.insert(std::make_pair(t.dwID, pkReserve));
			}
		}
	}
}
#endif

int GetAverageGuildMemberLevel(DWORD dwGID)
{
	char szQuery[QUERY_MAX_LEN];

	snprintf(szQuery, sizeof(szQuery),
			"SELECT AVG(level) FROM guild_member%s, player%s AS p WHERE guild_id=%u AND guild_member%s.pid=p.id",
			GetTablePostfix(), GetTablePostfix(), dwGID, GetTablePostfix());

	std::auto_ptr<SQLMsg> msg(CDBManager::instance().DirectQuery(szQuery));

	MYSQL_ROW row;
	row = mysql_fetch_row(msg->Get()->pSQLResult);

	int nAverageLevel = 0; str_to_number(nAverageLevel, row[0]);
	return nAverageLevel;
}

int GetGuildMemberCount(DWORD dwGID)
{
	char szQuery[QUERY_MAX_LEN];

	snprintf(szQuery, sizeof(szQuery), "SELECT COUNT(*) FROM guild_member%s WHERE guild_id=%u", GetTablePostfix(), dwGID);

	std::auto_ptr<SQLMsg> msg(CDBManager::instance().DirectQuery(szQuery));

	MYSQL_ROW row;
	row = mysql_fetch_row(msg->Get()->pSQLResult);

	DWORD dwCount = 0; str_to_number(dwCount, row[0]);
	return dwCount;
}

bool CGuildManager::ReserveWar(TPacketGuildWar * p)
{
	DWORD GID1 = p->dwGuildFrom;
	DWORD GID2 = p->dwGuildTo;

	if (GID1 > GID2)
		std::swap(GID1, GID2);

	if (p->lWarPrice > 0)
		if (!TakeBetPrice(GID1, GID2, p->lWarPrice))
			return false;

	TGuildWarReserve t;
	memset(&t, 0, sizeof(TGuildWarReserve));

	t.dwGuildFrom = GID1;
	t.dwGuildTo = GID2;
	t.dwTime = CClientManager::instance().GetCurrentTime() + GetGuildWarReserveSeconds();
	t.bType = p->bType;
	t.lWarPrice = p->lWarPrice;
	t.lInitialScore = p->lInitialScore;
#ifdef __IMPROVED_GUILD_WAR__
	t.iMaxPlayer = p->iMaxPlayer;
	t.iMaxScore = p->iMaxScore;
	t.flags = p->flags;
	t.custom_map_index = p->custom_map_index;
#endif
	int lvp, rkp, alv, mc;

	
	TGuild & k1 = TouchGuild(GID1);

	lvp = c_aiScoreByLevel[MIN(GUILD_MAX_LEVEL, k1.level)];
	rkp = c_aiScoreByRanking[GetRanking(GID1)];
	alv = GetAverageGuildMemberLevel(GID1);
	mc = GetGuildMemberCount(GID1);

	polyPower.SetVar("lvp", lvp);
	polyPower.SetVar("rkp", rkp);
	polyPower.SetVar("alv", alv);
	polyPower.SetVar("mc", mc);

	t.lPowerFrom = (long) polyPower.Eval();
	sys_log(0, "GuildWar: %u lvp %d rkp %d alv %d mc %d power %d", GID1, lvp, rkp, alv, mc, t.lPowerFrom);

	
	TGuild & k2 = TouchGuild(GID2);
#ifdef GUILD_WAR_COUNTER
	strlcpy(t.guild1_name, k1.szName, sizeof(t.guild1_name));
	strlcpy(t.guild2_name, k2.szName, sizeof(t.guild2_name));
#endif
	lvp = c_aiScoreByLevel[MIN(GUILD_MAX_LEVEL, k2.level)];
	rkp = c_aiScoreByRanking[GetRanking(GID2)];
	alv = GetAverageGuildMemberLevel(GID2);
	mc = GetGuildMemberCount(GID2);

	polyPower.SetVar("lvp", lvp);
	polyPower.SetVar("rkp", rkp);
	polyPower.SetVar("alv", alv);
	polyPower.SetVar("mc", mc);

	t.lPowerTo = (long) polyPower.Eval();
	sys_log(0, "GuildWar: %u lvp %d rkp %d alv %d mc %d power %d", GID2, lvp, rkp, alv, mc, t.lPowerTo);

	
	if (t.lPowerTo > t.lPowerFrom)
	{
		polyHandicap.SetVar("pA", t.lPowerTo);
		polyHandicap.SetVar("pB", t.lPowerFrom);
	}
	else
	{
		polyHandicap.SetVar("pA", t.lPowerFrom);
		polyHandicap.SetVar("pB", t.lPowerTo);
	}

	t.lHandicap = (long) polyHandicap.Eval();
	sys_log(0, "GuildWar: handicap %d", t.lHandicap);

	
	char szQuery[512];

#ifdef __IMPROVED_GUILD_WAR__
	snprintf(szQuery, sizeof(szQuery),
			"INSERT INTO guild_war_reservation (guild1, guild2, time, type, warprice, initscore, power1, power2, handicap, max_player, max_score, war_flags, war_midx) "
			"VALUES(%u, %u, DATE_ADD(NOW(), INTERVAL 180 SECOND), %u, %ld, %ld, %ld, %ld, %ld, %d, %d, %d, %d)",
			GID1, GID2, p->bType, p->lWarPrice, p->lInitialScore, t.lPowerFrom, t.lPowerTo, t.lHandicap, t.iMaxPlayer, t.iMaxScore, t.flags, t.custom_map_index);
#else
	snprintf(szQuery, sizeof(szQuery),
			"INSERT INTO guild_war_reservation (guild1, guild2, time, type, warprice, initscore, power1, power2, handicap) "
			"VALUES(%u, %u, DATE_ADD(NOW(), INTERVAL 180 SECOND), %u, %ld, %ld, %ld, %ld, %ld)",
			GID1, GID2, p->bType, p->lWarPrice, p->lInitialScore, t.lPowerFrom, t.lPowerTo, t.lHandicap);
#endif

	std::auto_ptr<SQLMsg> pmsg(CDBManager::instance().DirectQuery(szQuery));

	if (pmsg->Get()->uiAffectedRows == 0 || pmsg->Get()->uiInsertID == 0 || pmsg->Get()->uiAffectedRows == (uint32_t)-1)
	{
		sys_err("GuildWar: Cannot insert row");
		return false;
	}

	t.dwID = pmsg->Get()->uiInsertID;

	m_map_kWarReserve.insert(std::make_pair(t.dwID, new CGuildWarReserve(t)));

	CClientManager::instance().ForwardPacket(HEADER_DG_GUILD_WAR_RESERVE_ADD, &t, sizeof(TGuildWarReserve));
	return true;
}

void CGuildManager::ProcessReserveWar()
{
	DWORD dwCurTime = CClientManager::instance().GetCurrentTime();

	itertype(m_map_kWarReserve) it = m_map_kWarReserve.begin();

	while (it != m_map_kWarReserve.end())
	{
		itertype(m_map_kWarReserve) it2 = it++;

		CGuildWarReserve * pk = it2->second;
		TGuildWarReserve & r = pk->GetDataRef();

		if (!r.bStarted && r.dwTime - 1800 <= dwCurTime) 
		{
			int iMin = (int) ceil((int)(r.dwTime - dwCurTime) / 60.0);

			TGuild & r_1 = m_map_kGuild[r.dwGuildFrom];
			TGuild & r_2 = m_map_kGuild[r.dwGuildTo];

#ifdef __IMPROVED_GUILD_WAR__
			sys_log(0, "GuildWar: started GID1 %u GID2 %u %d time %d min %d maxplayer %d maxscore %d flags %ld custommapidx %d", r.dwGuildFrom, r.dwGuildTo, r.bStarted, dwCurTime - r.dwTime, iMin, r.iMaxPlayer, r.iMaxScore, r.flags, r.custom_map_index);
#else
			sys_log(0, "GuildWar: started GID1 %u GID2 %u %d time %d min %d", r.dwGuildFrom, r.dwGuildTo, r.bStarted, dwCurTime - r.dwTime, iMin);
#endif

			if (iMin <= 0)
			{
				char szQuery[128];
				snprintf(szQuery, sizeof(szQuery), "UPDATE guild_war_reservation SET started=1 WHERE id=%u", r.dwID);
				CDBManager::instance().AsyncQuery(szQuery);

				CClientManager::instance().ForwardPacket(HEADER_DG_GUILD_WAR_RESERVE_DEL, &r.dwID, sizeof(DWORD));

				r.bStarted = true;

#ifdef __IMPROVED_GUILD_WAR__
				TGuildWaitStartInfo info(r.bType, r.dwGuildFrom, r.dwGuildTo, r.lWarPrice, r.lInitialScore, pk, r.iMaxPlayer, r.iMaxScore, r.flags, r.custom_map_index);
#else
				TGuildWaitStartInfo info(r.bType, r.dwGuildFrom, r.dwGuildTo, r.lWarPrice, r.lInitialScore, pk);
#endif

				m_pqWaitStart.push(std::make_pair(dwCurTime + GetGuildWarWaitStartDuration(), info));

				TPacketGuildWar pck;

				pck.bType = r.bType;
#ifdef GUILD_WAR_COUNTER
				pck.warID = r.dwID;
#endif
				pck.bWar = GUILD_WAR_WAIT_START;
				pck.dwGuildFrom = r.dwGuildFrom;
				pck.dwGuildTo = r.dwGuildTo;
				pck.lWarPrice = r.lWarPrice;
				pck.lInitialScore = r.lInitialScore;
#ifdef __IMPROVED_GUILD_WAR__
				pck.iMaxPlayer = r.iMaxPlayer;
				pck.iMaxScore = r.iMaxScore;
				pck.flags = r.flags;
				pck.custom_map_index = r.custom_map_index;
#endif
				CClientManager::instance().ForwardPacket(HEADER_DG_GUILD_WAR, &pck, sizeof(TPacketGuildWar));
				//m_map_kWarReserve.erase(it2);
			}
			else
			{
				if (iMin != pk->GetLastNoticeMin())
				{
					pk->SetLastNoticeMin(iMin);

					CClientManager::instance().SendNotice("The war between %s and %s will start in 1 minut!", r_1.szName, r_2.szName, iMin);
				}
			}
		}
	}
}

bool CGuildManager::Bet(DWORD dwID, const char * c_pszLogin, DWORD dwGold, DWORD dwGuild)
{
	itertype(m_map_kWarReserve) it = m_map_kWarReserve.find(dwID);

	char szQuery[1024];

	if (it == m_map_kWarReserve.end())
	{
		sys_log(0, "WAR_RESERVE: Bet: cannot find reserve war by id %u", dwID);
		snprintf(szQuery, sizeof(szQuery), "INSERT INTO item_award (login, vnum, socket0, given_time) VALUES('%s', %d, %u, NOW())",
				c_pszLogin, ITEM_ELK_VNUM, dwGold);
		CDBManager::instance().AsyncQuery(szQuery);
		return false;
	}

	if (!it->second->Bet(c_pszLogin, dwGold, dwGuild))
	{
		sys_log(0, "WAR_RESERVE: Bet: cannot bet id %u, login %s, gold %u, guild %u", dwID, c_pszLogin, dwGold, dwGuild);
		snprintf(szQuery, sizeof(szQuery), "INSERT INTO item_award (login, vnum, socket0, given_time) VALUES('%s', %d, %u, NOW())",
				c_pszLogin, ITEM_ELK_VNUM, dwGold);
		CDBManager::instance().AsyncQuery(szQuery);
		return false;
	}

	return true;
}

void CGuildManager::CancelWar(DWORD GID1, DWORD GID2
#ifdef __IMPROVED_GUILD_WAR__
	, int iMaxPlayer, int iMaxScore, DWORD flags, int custom_map_index
#endif
)
{
#ifdef __IMPROVED_GUILD_WAR__
	RemoveDeclare(GID1, GID2, iMaxPlayer, iMaxScore, flags, custom_map_index);
#else
	RemoveDeclare(GID1, GID2);
#endif
	RemoveWar(GID1, GID2);
}

bool CGuildManager::ChangeMaster(DWORD dwGID, DWORD dwFrom, DWORD dwTo)
{
	itertype(m_map_kGuild) iter = m_map_kGuild.find(dwGID);

	if (iter == m_map_kGuild.end())
		return false;

	char szQuery[1024];

	snprintf(szQuery, sizeof(szQuery), "UPDATE guild%s SET master=%u WHERE id=%u", GetTablePostfix(), dwTo, dwGID);
	delete CDBManager::instance().DirectQuery(szQuery);

	snprintf(szQuery, sizeof(szQuery), "UPDATE guild_member%s SET grade=1 WHERE pid=%u", GetTablePostfix(), dwTo);
	delete CDBManager::instance().DirectQuery(szQuery);

	snprintf(szQuery, sizeof(szQuery), "UPDATE guild_member%s SET grade=15 WHERE pid=%u", GetTablePostfix(), dwFrom);
	delete CDBManager::instance().DirectQuery(szQuery);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Guild War Reserve Class
//////////////////////////////////////////////////////////////////////////////////////////
CGuildWarReserve::CGuildWarReserve(const TGuildWarReserve & rTable)
{
	thecore_memcpy(&m_data, &rTable, sizeof(TGuildWarReserve));
	m_iLastNoticeMin = -1;

	Initialize();
}

void CGuildWarReserve::Initialize()
{
	char szQuery[256];
	snprintf(szQuery, sizeof(szQuery), "SELECT login, guild, gold FROM guild_war_bet WHERE war_id=%u", m_data.dwID);

	std::auto_ptr<SQLMsg> msgbet(CDBManager::instance().DirectQuery(szQuery));

	if (msgbet->Get()->uiNumRows)
	{
		MYSQL_RES * res = msgbet->Get()->pSQLResult;
		MYSQL_ROW row;

		char szLogin[LOGIN_MAX_LEN+1];
		DWORD dwGuild;
		DWORD dwGold;

		while ((row = mysql_fetch_row(res)))
		{
			dwGuild = dwGold = 0;
			strlcpy(szLogin, row[0], sizeof(szLogin));
			str_to_number(dwGuild, row[1]);
			str_to_number(dwGold, row[2]);

			mapBet.insert(std::make_pair(szLogin, std::make_pair(dwGuild, dwGold)));
		}
	}
}

void CGuildWarReserve::OnSetup(CPeer * peer)
{
	if (m_data.bStarted) 
		return;

#ifdef __IMPROVED_GUILD_WAR__
	FSendPeerWar(m_data.bType, GUILD_WAR_RESERVE, m_data.dwGuildFrom, m_data.dwGuildTo, m_data.iMaxPlayer, m_data.iMaxScore, m_data.flags, m_data.custom_map_index) (peer);
#else
	FSendPeerWar(m_data.bType, GUILD_WAR_RESERVE, m_data.dwGuildFrom, m_data.dwGuildTo) (peer);
#endif

	peer->EncodeHeader(HEADER_DG_GUILD_WAR_RESERVE_ADD, 0, sizeof(TGuildWarReserve));
	peer->Encode(&m_data, sizeof(TGuildWarReserve));

	TPacketGDGuildWarBet pckBet;
	pckBet.dwWarID = m_data.dwID;

	itertype(mapBet) it = mapBet.begin();

	while (it != mapBet.end())
	{
		strlcpy(pckBet.szLogin, it->first.c_str(), sizeof(pckBet.szLogin));
		pckBet.dwGuild = it->second.first;
		pckBet.dwGold = it->second.second;

		peer->EncodeHeader(HEADER_DG_GUILD_WAR_BET, 0, sizeof(TPacketGDGuildWarBet));
		peer->Encode(&pckBet, sizeof(TPacketGDGuildWarBet));

		++it;
	}
}

bool CGuildWarReserve::Bet(const char * pszLogin, DWORD dwGold, DWORD dwGuild)
{
	char szQuery[1024];

	if (m_data.dwGuildFrom != dwGuild && m_data.dwGuildTo != dwGuild)
	{
		sys_log(0, "GuildWarReserve::Bet: invalid guild id");
		return false;
	}

	if (m_data.bStarted)
	{
		sys_log(0, "GuildWarReserve::Bet: war is already started");
		return false;
	}

	if (mapBet.find(pszLogin) != mapBet.end())
	{
		sys_log(0, "GuildWarReserve::Bet: failed. already bet");
		return false;
	}

	snprintf(szQuery, sizeof(szQuery),
			"INSERT INTO guild_war_bet (war_id, login, gold, guild) VALUES(%u, '%s', %u, %u)",
			m_data.dwID, pszLogin, dwGold, dwGuild);

	std::auto_ptr<SQLMsg> pmsg(CDBManager::instance().DirectQuery(szQuery));

	if (pmsg->Get()->uiAffectedRows == 0 || pmsg->Get()->uiAffectedRows == (uint32_t)-1)
	{
		sys_log(0, "GuildWarReserve::Bet: failed. cannot insert row to guild_war_bet table");
		return false;
	}

	if (m_data.dwGuildFrom == dwGuild)
		m_data.dwBetFrom += dwGold;
	else
		m_data.dwBetTo += dwGold;

	CClientManager::instance().ForwardPacket(HEADER_DG_GUILD_WAR_RESERVE_ADD, &m_data, sizeof(TGuildWarReserve));

	snprintf(szQuery, sizeof(szQuery), "UPDATE guild_war_reservation SET bet_from=%u,bet_to=%u WHERE id=%u",
			m_data.dwBetFrom, m_data.dwBetTo, m_data.dwID);

	CDBManager::instance().AsyncQuery(szQuery);

	sys_log(0, "GuildWarReserve::Bet: success. %s %u war_id %u bet %u : %u", pszLogin, dwGuild, m_data.dwID, m_data.dwBetFrom, m_data.dwBetTo);
	mapBet.insert(std::make_pair(pszLogin, std::make_pair(dwGuild, dwGold)));

	TPacketGDGuildWarBet pckBet;
	pckBet.dwWarID = m_data.dwID;
	strlcpy(pckBet.szLogin, pszLogin, sizeof(pckBet.szLogin));
	pckBet.dwGuild = dwGuild;
	pckBet.dwGold = dwGold;

	CClientManager::instance().ForwardPacket(HEADER_DG_GUILD_WAR_BET, &pckBet, sizeof(TPacketGDGuildWarBet));
	return true;
}

//


//
void CGuildWarReserve::Draw()
{
	char szQuery[1024];

	snprintf(szQuery, sizeof(szQuery), "UPDATE guild_war_reservation SET started=1,winner=0 WHERE id=%u", m_data.dwID);
	CDBManager::instance().AsyncQuery(szQuery);

	if (mapBet.empty())
		return;

	sys_log(0, "WAR_REWARD: Draw. war_id %u", m_data.dwID);

	itertype(mapBet) it = mapBet.begin();

	while (1)
	{
		int iLen = 0;
		int iRow = 0;

		iLen += snprintf(szQuery, sizeof(szQuery) - iLen, "INSERT INTO item_award (login, vnum, socket0, given_time) VALUES");

		while (it != mapBet.end())
		{
			if (iRow == 0)
				iLen += snprintf(szQuery + iLen, sizeof(szQuery) - iLen, "('%s', %d, %u, NOW())",
						it->first.c_str(), ITEM_ELK_VNUM, it->second.second);
			else
				iLen += snprintf(szQuery + iLen, sizeof(szQuery) - iLen, ",('%s', %d, %u, NOW())",
						it->first.c_str(), ITEM_ELK_VNUM, it->second.second);

			it++;

			if (iLen > 384)
				break;

			++iRow;
		}

		if (iRow > 0)
		{
			sys_log(0, "WAR_REWARD: QUERY: %s", szQuery);
			CDBManager::instance().AsyncQuery(szQuery);
		}

		if (it == mapBet.end())
			break;
	}
}

void CGuildWarReserve::End(int iScoreFrom, int iScoreTo)
{
	DWORD dwWinner;

	sys_log(0, "WAR_REWARD: End: From %u %d To %u %d", m_data.dwGuildFrom, iScoreFrom, m_data.dwGuildTo, iScoreTo);

	if (m_data.lPowerFrom > m_data.lPowerTo)
	{
		if (m_data.lHandicap > iScoreFrom - iScoreTo)
		{
			sys_log(0, "WAR_REWARD: End: failed to overcome handicap, From is strong but To won");
			dwWinner = m_data.dwGuildTo;
		}
		else
		{
			sys_log(0, "WAR_REWARD: End: success to overcome handicap, From win!");
			dwWinner = m_data.dwGuildFrom;
		}
	}
	else
	{
		if (m_data.lHandicap > iScoreTo - iScoreFrom)
		{
			sys_log(0, "WAR_REWARD: End: failed to overcome handicap, To is strong but From won");
			dwWinner = m_data.dwGuildFrom;
		}
		else
		{
			sys_log(0, "WAR_REWARD: End: success to overcome handicap, To win!");
			dwWinner = m_data.dwGuildTo;
		}
	}

	char szQuery[1024];
	snprintf(szQuery, sizeof(szQuery), "UPDATE guild_war_reservation SET started=1,winner=%u,result1=%d,result2=%d WHERE id=%u",
			dwWinner, iScoreFrom, iScoreTo, m_data.dwID);
	CDBManager::instance().AsyncQuery(szQuery);

	if (mapBet.empty())
		return;

	DWORD dwTotalBet = m_data.dwBetFrom + m_data.dwBetTo;
	DWORD dwWinnerBet = 0;

	if (dwWinner == m_data.dwGuildFrom)
		dwWinnerBet = m_data.dwBetFrom;
	else if (dwWinner == m_data.dwGuildTo)
		dwWinnerBet = m_data.dwBetTo;
	else
	{
		sys_err("WAR_REWARD: fatal error, winner does not exist!");
		return;
	}

	if (dwWinnerBet == 0)
	{
		sys_err("WAR_REWARD: total bet money on winner is zero");
		return;
	}

	sys_log(0, "WAR_REWARD: End: Total bet: %u, Winner bet: %u", dwTotalBet, dwWinnerBet);

	itertype(mapBet) it = mapBet.begin();

	while (1)
	{
		int iLen = 0;
		int iRow = 0;

		iLen += snprintf(szQuery, sizeof(szQuery) - iLen, "INSERT INTO item_award (login, vnum, socket0, given_time) VALUES");

		while (it != mapBet.end())
		{
			if (it->second.first != dwWinner)
			{
				++it;
				continue;
			}

			double ratio = (double) it->second.second / dwWinnerBet;

			
			sys_log(0, "WAR_REWARD: %s %u ratio %f", it->first.c_str(), it->second.second, ratio);

			DWORD dwGold = (DWORD) (dwTotalBet * ratio * 0.9);

			if (iRow == 0)
				iLen += snprintf(szQuery + iLen, sizeof(szQuery) - iLen, "('%s', %d, %u, NOW())",
						it->first.c_str(), ITEM_ELK_VNUM, dwGold);
			else
				iLen += snprintf(szQuery + iLen, sizeof(szQuery) - iLen, ",('%s', %d, %u, NOW())",
						it->first.c_str(), ITEM_ELK_VNUM, dwGold);

			++it;

			if (iLen > 384)
				break;

			++iRow;
		}

		if (iRow > 0)
		{
			sys_log(0, "WAR_REWARD: query: %s", szQuery);
			CDBManager::instance().AsyncQuery(szQuery);
		}

		if (it == mapBet.end())
			break;
	}
}


#ifdef GUILD_WAR_COUNTER
void CGuildManager::LoadWarStatisticsInfo(DWORD warID, TGuildWarReserve& t, bool isP2P)
{
	sys_err("Load %d type %d",warID,t.bType);

	if (t.bType != 1)
		return;

	//if(!isP2P)
	{
		TGuild& k1 = TouchGuild(t.dwGuildFrom);
		strlcpy(t.guild1_name, k1.szName, sizeof(t.guild1_name));
		TGuild& k2 = TouchGuild(t.dwGuildTo);
		strlcpy(t.guild2_name, k2.szName, sizeof(t.guild2_name));
	}

	itertype(m_warStatisticsInfo) it = m_warStatisticsInfo.find(warID);
	if (it == m_warStatisticsInfo.end())
		m_warStatisticsInfo.emplace(warID, t);

	if (isP2P)
	{
		BYTE sub_index = SUB_GUILDWAR_LOADWAR;
		TEMP_BUFFER buf;
		buf.write(&sub_index, sizeof(BYTE));
		buf.write(&t, sizeof(TGuildWarReserve));
		CClientManager::instance().ForwardPacket(HEADER_DG_GUILD_COUNTER, buf.read_peek(), buf.size());
	}
}
bool CGuildManager::LoadWarStatisticsData(DWORD warID)
{
	itertype(m_warStatisticsInfo) itInfo = m_warStatisticsInfo.find(warID);
	if (itInfo == m_warStatisticsInfo.end())
		return false;
	itertype(m_warStatisticsData) it = m_warStatisticsData.find(warID);
	if (it != m_warStatisticsData.end())
		return true;

	char szQuery[152];
	snprintf(szQuery, sizeof(szQuery), "SELECT empire,name,level,race,killed,dead,skill_dmg,is_leader,guild_id,pid,spy,online FROM player.guild_counter_ranking WHERE warid = %d", warID);
	std::unique_ptr<SQLMsg> pmsg(CDBManager::instance().DirectQuery(szQuery));
	MYSQL_ROW row;
	if (pmsg && pmsg->Get()->uiNumRows != 0)
	{
		std::vector<war_static_ptr> m_data;
		m_data.clear();
		while (NULL != (row = mysql_fetch_row(pmsg->Get()->pSQLResult)))
		{
			war_static_ptr _ptr;
			str_to_number(_ptr.empire, row[0]);
			strlcpy(_ptr.name, row[1],sizeof(_ptr.name));
			str_to_number(_ptr.level, row[2]);
			str_to_number(_ptr.race, row[3]);
			str_to_number(_ptr.kill, row[4]);
			str_to_number(_ptr.dead, row[5]);
			str_to_number(_ptr.skill_dmg, row[6]);
			str_to_number(_ptr.is_leader, row[7]);
			str_to_number(_ptr.guild_id, row[8]);
			str_to_number(_ptr.pid, row[9]);
			str_to_number(_ptr.spy, row[10]);
			str_to_number(_ptr.online, row[11]);
			m_data.emplace_back(_ptr);
		}
		if (!m_data.size())
			return false;
		m_warStatisticsData.emplace(warID, m_data);
	}
	else
		return false;
	return true;
}
void CGuildManager::GetWarStatisticsInfo(std::vector<TGuildWarReserve>& p_vec)
{
	for (itertype(m_warStatisticsInfo) it = m_warStatisticsInfo.begin(); it != m_warStatisticsInfo.end(); ++it)
		p_vec.push_back(it->second);
}
void CGuildManager::WarStatistics(CPeer* peer, DWORD handle, const char* data)
{
	const BYTE sub_index = *(BYTE*)data;
	data += sizeof(BYTE);
	if (sub_index == SUB_GUILDWAR_LOADDATA)
	{
		if (!peer)
			return;
		const DWORD warID = *(DWORD*)data;
		data += sizeof(DWORD);
		if (LoadWarStatisticsData(warID))
		{
			itertype(m_warStatisticsData) it = m_warStatisticsData.find(warID);
			if (it != m_warStatisticsData.end())
			{
				int packetSize = it->second.size();
				BYTE subIndex = SUB_GUILDWAR_LOADDATA;
				peer->EncodeHeader(HEADER_DG_GUILD_COUNTER, handle, sizeof(BYTE) + sizeof(DWORD) + sizeof(int) + (sizeof(war_static_ptr) * packetSize));
				peer->Encode(&subIndex, sizeof(BYTE));
				peer->Encode(&warID, sizeof(DWORD));
				peer->Encode(&packetSize, sizeof(int));
				if(packetSize > 0)
					peer->Encode(it->second.data(), sizeof(war_static_ptr) * packetSize);
			}
		}
	}
	else if (sub_index == SUB_GUILDWAR_LOADWAR)
	{
		const DWORD warID = *(DWORD*)data;
		data += sizeof(DWORD);

		itertype(m_warStatisticsInfo) it = m_warStatisticsInfo.find(warID);
		if (it == m_warStatisticsInfo.end())
		{
			char buf[512];
			snprintf(buf, sizeof(buf),
				"SELECT guild1, guild2, UNIX_TIMESTAMP(time), type, warprice, initscore, bet_from, bet_to, power1, power2, handicap"
#ifdef __IMPROVED_GUILD_WAR__
				", max_player, max_score, war_flags, war_midx"
#endif
				", time_str, winner FROM guild_war_reservation WHERE id = %d", warID);

			std::unique_ptr<SQLMsg> pmsg(CDBManager::instance().DirectQuery(buf));
			if (pmsg->Get()->uiNumRows == 0)
				return;

			MYSQL_ROW row;
			TGuildWarReserve t;
			while ((row = mysql_fetch_row(pmsg->Get()->pSQLResult)))
			{
				int col = 0;
				t.dwID =  warID;
				str_to_number(t.dwGuildFrom, row[col++]);
				str_to_number(t.dwGuildTo, row[col++]);
				str_to_number(t.dwTime, row[col++]);
				str_to_number(t.bType, row[col++]);
				str_to_number(t.lWarPrice, row[col++]);
				str_to_number(t.lInitialScore, row[col++]);
				str_to_number(t.dwBetFrom, row[col++]);
				str_to_number(t.dwBetTo, row[col++]);
				str_to_number(t.lPowerFrom, row[col++]);
				str_to_number(t.lPowerTo, row[col++]);
				str_to_number(t.lHandicap, row[col++]);
#ifdef __IMPROVED_GUILD_WAR__
				str_to_number(t.iMaxPlayer, row[col++]);
				str_to_number(t.iMaxScore, row[col++]);
				str_to_number(t.flags, row[col++]);
				str_to_number(t.custom_map_index, row[col++]);
#endif
				strlcpy(t.date, row[col++], sizeof(t.date));
				str_to_number(t.winner, row[col++]);
			}
			LoadWarStatisticsInfo(warID, t, true);
			m_warStatisticsInfo.emplace(warID, t);
		}

		const int packet_size = *(int*)data;
		data += sizeof(int);
		//std::vector<war_static_ptr> m_data;
		//m_data.clear();
		char szQuery[512];
		for (int i = 0; i < packet_size; ++i)
		{
			const war_static_ptr& statistics = *(war_static_ptr*)data;
			snprintf(szQuery, sizeof(szQuery), "REPLACE INTO player.guild_counter_ranking (warid,empire,name,level,race,killed,dead,skill_dmg,is_leader,guild_id,pid,spy,online) VALUES (%u,%d,'%s',%d,%d,%d,%d,%ld,%d,%d,%d,%d,%d)",
				warID, statistics.empire, statistics.name, statistics.level, statistics.race, statistics.kill, statistics.dead, statistics.skill_dmg, statistics.is_leader ? 1 : 0, statistics.guild_id, statistics.pid, statistics.spy ? 1 : 0, statistics.online ? 1 : 0);
			std::unique_ptr<SQLMsg> pMsg(CDBManager::instance().DirectQuery(szQuery));
			//m_data.emplace_back(statistics);
			data += sizeof(war_static_ptr);
		}

		//auto itData = m_warStatisticsData.find(warID);
		//if (itData == m_warStatisticsData.end())
		//	m_warStatisticsData.emplace(warID, m_data);
		//else
		//	sys_err("GuildWarCounter: %d this index war data already insert!", warID);
	}
}
#endif

