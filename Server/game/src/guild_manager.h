#ifndef __GUILD_MANAGER_H
#define __GUILD_MANAGER_H

class CGuild;
struct TGuildCreateParameter;

#define GUILD_SYMBOL_FILENAME	"guild_symbol.tga"

class CGuildWarReserveForGame
{
	public:
		TGuildWarReserve				data;
		std::map<std::string, std::pair<DWORD, DWORD> > mapBet;
};

class CGuildManager : public singleton<CGuildManager>
{
	public:
		CGuildManager();
		virtual ~CGuildManager();

		DWORD		CreateGuild(TGuildCreateParameter& gcp);
		CGuild *	FindGuild(DWORD guild_id);
		CGuild *	FindGuildByName(const std::string guild_name);
		void		LoadGuild(DWORD guild_id);
		CGuild *	TouchGuild(DWORD guild_id);
		void		DisbandGuild(DWORD guild_id);

		void		Initialize();

		void		Link(DWORD pid, CGuild* guild);
		void		Unlink(DWORD pid);
		CGuild *	GetLinkedGuild(DWORD pid);

		void		LoginMember(LPCHARACTER ch);
		void		P2PLoginMember(DWORD pid);
		void		P2PLogoutMember(DWORD pid);

		void		SkillRecharge();

		void		ShowGuildWarList(LPCHARACTER ch);
		void		SendGuildWar(LPCHARACTER ch);

		void		RequestEndWar(DWORD guild_id1, DWORD guild_id2);
		void		RequestCancelWar(DWORD guild_id1, DWORD guild_id2);

#ifdef __IMPROVED_GUILD_WAR__
		void		RequestWarOver(DWORD dwGuild1, DWORD dwGuild2, DWORD dwGuildWinner, long lReward, int iMaxPlayer, int iMaxScore, DWORD flags, int custom_map_index);

		void		DeclareWar(DWORD guild_id1, DWORD guild_id2, BYTE bType, int iMaxPlayer, int iMaxScore, DWORD flags, int custom_map_index);
		void		RefuseWar(DWORD guild_id1, DWORD guild_id2, int iMaxPlayer, int iMaxScore, DWORD flags, int custom_map_index);
		void		StartWar(DWORD guild_id1, DWORD guild_id2, int iMaxPlayer, int iMaxScore, DWORD flags, int custom_map_index);
		void		WaitStartWar(DWORD guild_id1, DWORD guild_id2, int iMaxPlayer, int iMaxScore, DWORD flags, int custom_map_index
#ifdef GUILD_WAR_COUNTER
					, DWORD warID
#endif
		);
		void		WarOver(DWORD guild_id1, DWORD guild_id2, bool bDraw);
		void		CancelWar(DWORD guild_id1, DWORD guild_id2, int iMaxPlayer, int iMaxScore, DWORD flags, int custom_map_index);
		bool		EndWar(DWORD guild_id1, DWORD guild_id2);
		void		ReserveWar(DWORD dwGuild1, DWORD dwGuild2, BYTE bType, int iMaxPlayer, int iMaxScore, DWORD flags, int custom_map_index);
#else
		void		RequestWarOver(DWORD dwGuild1, DWORD dwGuild2, DWORD dwGuildWinner, long lReward);

		void		DeclareWar(DWORD guild_id1, DWORD guild_id2, BYTE bType);
		void		RefuseWar(DWORD guild_id1, DWORD guild_id2);
		void		StartWar(DWORD guild_id1, DWORD guild_id2);
		void		WaitStartWar(DWORD guild_id1, DWORD guild_id2);
		void		WarOver(DWORD guild_id1, DWORD guild_id2, bool bDraw);
		void		CancelWar(DWORD guild_id1, DWORD guild_id2);
		bool		EndWar(DWORD guild_id1, DWORD guild_id2);
		void		ReserveWar(DWORD dwGuild1, DWORD dwGuild2, BYTE bType);
#endif
#ifdef ENABLE_GUILD_ONLINE_LIST
	void		SendOnlineGuildData(LPCHARACTER ch);
	void		SendOnlineGuildRefresh(DWORD guildID, bool status);
#endif
		void            ReserveWarAdd(TGuildWarReserve * p);
#ifdef GUILD_WAR_COUNTER
		void		SetWarStatisticsInfo(const std::vector<TGuildWarReserve>& vec_data);
		void		SetWarStatisticsInfo(const TGuildWarReserve& m_reserverData);

		void		SetWarStatisticsData(DWORD warID, const std::vector<war_static_ptr>& vec_data);
		
		void		SendWarStatisticsPacket(LPCHARACTER ch, BYTE subHeader);
		void		SendWarStatisticsData(LPCHARACTER ch, DWORD id);
#endif
		void            ReserveWarDelete(DWORD dwID);
		std::vector<CGuildWarReserveForGame *> & GetReserveWarRef();
		void		ReserveWarBet(TPacketGDGuildWarBet * p);
		bool		IsBet(DWORD dwID, const char * c_pszLogin);

		void		StopAllGuildWar();

		void		Kill(LPCHARACTER killer, LPCHARACTER victim);

		int		GetRank(CGuild* g);
		//void		GetHighRankString(DWORD dwMyGuild, char * buffer);
		//void		GetAroundRankString(DWORD dwMyGuild, char * buffer);
		void		GetHighRankString(DWORD dwMyGuild, char * buffer, size_t buflen);
		void		GetAroundRankString(DWORD dwMyGuild, char * buffer, size_t buflen);

		template <typename Func> void for_each_war(Func & f);

		int		GetDisbandDelay();
		int		GetWithdrawDelay();

		void		ChangeMaster(DWORD dwGID);
#ifdef ENABLE_GUILD_REQUEST
		void		SendData(LPCHARACTER ch, BYTE tabIndex, DWORD pageIndex, const char* guildName);
		void		SendRequest(LPCHARACTER ch, DWORD guildID, BYTE requestIndex);
#endif
#ifdef GUILD_RANK_EFFECT
		void		SortGuildCache();
		DWORD		GetMyGuildRank(CGuild* guild);
#endif
	private:
		typedef std::map<DWORD, CGuild*> TGuildMap;
		TGuildMap m_mapGuild;

		typedef std::set<std::pair<DWORD, DWORD> > TGuildWarContainer;
		TGuildWarContainer m_GuildWar;

		typedef std::map<std::pair<DWORD, DWORD>, DWORD> TGuildWarEndTimeContainer;
		TGuildWarEndTimeContainer m_GuildWarEndTime;

		TGuildMap				m_map_pkGuildByPID;

		std::map<DWORD, CGuildWarReserveForGame *>	m_map_kReserveWar;
		std::vector<CGuildWarReserveForGame *>		m_vec_kReserveWar;
#ifdef GUILD_WAR_COUNTER
		std::map<DWORD, TGuildWarReserve> m_warStatisticsInfo;
		std::map<DWORD, std::vector<war_static_ptr>> m_warStatisticsData;
#endif
#ifdef GUILD_RANK_EFFECT
		std::vector<CGuild*> m_mapNameAll;
#endif
#ifdef ENABLE_GUILD_REQUEST
		std::map<std::string, CGuild*> m_mapNameGuild;

		std::vector<CGuild*> m_mapNameShinsoo;
		std::vector<CGuild*> m_mapNameChunjo;
		std::vector<CGuild*> m_mapNameJinno;
#endif
		friend class CGuild;
};

template <typename Func> void CGuildManager::for_each_war(Func & f)
{
	for (itertype(m_GuildWar) it = m_GuildWar.begin(); it != m_GuildWar.end(); ++it)
	{
		f(it->first, it->second);
	}
}

extern void SendGuildWarScore(DWORD dwGuild, DWORD dwGuildOpp, int iDelta, int iBetScoreDelta = 0);

#endif
