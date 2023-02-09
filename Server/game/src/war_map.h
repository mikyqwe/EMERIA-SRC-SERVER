#ifndef __GUILD_WAR_MAP_MANAGER_H
#define __GUILD_WAR_MAP_MANAGER_H

#include "constants.h"
#include "guild.h"

#ifdef GUILD_WAR_COUNTER
#include <array>
#endif

enum EWarMapTypes
{
	WAR_MAP_TYPE_NORMAL,
	WAR_MAP_TYPE_FLAG,
};

typedef struct SWarMapInfo
{
	BYTE		bType;
	long		lMapIndex;
	PIXEL_POSITION	posStart[3];
#ifdef __IMPROVED_GUILD_WAR__
	int				iMaxPlayer;
	int				iMaxScore;
	DWORD			flags;
	int				custom_map_index;
#endif
} TWarMapInfo;

namespace warmap
{
	enum
	{
		WAR_FLAG_VNUM_START = 20035,
		WAR_FLAG_VNUM_END = 20037,

		WAR_FLAG_VNUM0 = 20035,
		WAR_FLAG_VNUM1 = 20036,
		WAR_FLAG_VNUM2 = 20037,

		WAR_FLAG_BASE_VNUM = 20038
	};

	inline bool IsWarFlag(DWORD dwVnum)
	{
		if (dwVnum >= WAR_FLAG_VNUM_START && dwVnum <= WAR_FLAG_VNUM_END)
			return true;

		return false;
	}

	inline bool IsWarFlagBase(DWORD dwVnum)
	{
		return dwVnum == WAR_FLAG_BASE_VNUM ? true : false;
	}
};

class CWarMap
{
	public:
		friend class CGuild;

		CWarMap(long lMapIndex, const TGuildWarInfo & r_info, TWarMapInfo * pkWarMapInfo, DWORD dwGuildID1, DWORD dwGuildID2
#ifdef __IMPROVED_GUILD_WAR__
			, int iMaxPlayer, int iMaxScore, DWORD flags, int custom_map_index
#endif
#ifdef GUILD_WAR_COUNTER
	, DWORD warDBID
#endif
		);
		~CWarMap();

		bool	GetTeamIndex(DWORD dwGuild, BYTE & bIdx);

		void	IncMember(LPCHARACTER ch);
		void	DecMember(LPCHARACTER ch);

		CGuild * GetGuild(BYTE bIdx);
		DWORD	GetGuildID(BYTE bIdx);
#ifdef __IMPROVED_GUILD_WAR__
		int		GetCurrentPlayer(BYTE bIdx);
		int		GetMaxPlayer(BYTE bIdx);
		int		GetCurrentScore(BYTE bIdx);
		int		GetMaxScore(BYTE bIdx);
		DWORD	GetWarFlags(BYTE bIdx);
		int		GetCustomMapIndex(BYTE bIdx);
#endif
		BYTE	GetType();
		long	GetMapIndex();
		DWORD	GetGuildOpponent(LPCHARACTER ch);

		DWORD	GetWinnerGuild();
		void	UsePotion(LPCHARACTER ch, LPITEM item);

		void	Draw();	// ���� ���º� ó��
		void	Timeout();
		void	CheckWarEnd();
		bool	SetEnded();
		void	ExitAll();

		void	SetBeginEvent(LPEVENT pkEv);
		void	SetTimeoutEvent(LPEVENT pkEv);
		void	SetEndEvent(LPEVENT pkEv);
		void	SetResetFlagEvent(LPEVENT pkEv);

		void	UpdateScore(DWORD g1, int score1, DWORD g2, int score2);
		bool	CheckScore();

		int	GetRewardGold(BYTE bWinnerIdx);

		bool	GetGuildIndex(DWORD dwGuild, int& iIndex);

#ifdef GUILD_WAR_COUNTER
		void	SendKillNotice(LPCHARACTER killer, LPCHARACTER victim, long damage = 0);
		void	UpdateStatic(LPCHARACTER ch, BYTE sub_index, std::vector<war_static_ptr> m_list);
		void	RegisterStatics(LPCHARACTER ch);
		void	UpdateSpy(DWORD pid);
		void	Packet(const void* pv, int size, bool broadcast = false);
		void	SaveCounterData();
#else
		void	Packet(const void* pv, int size);
#endif
		void	Notice(const char * psz);
		void	SendWarPacket(LPDESC d);
		void	SendScorePacket(BYTE bIdx, LPDESC d = NULL);

		void	OnKill(LPCHARACTER killer, LPCHARACTER ch);

		void	AddFlag(BYTE bIdx, DWORD x=0, DWORD y=0);
		void	AddFlagBase(BYTE bIdx, DWORD x=0, DWORD y=0);
		void	RemoveFlag(BYTE bIdx);
		bool	IsFlagOnBase(BYTE bIdx);
		void	ResetFlag();

	private:
		void	UpdateUserCount();

	private:
		TWarMapInfo	m_kMapInfo;
		bool		m_bEnded;

		LPEVENT m_pkBeginEvent;
		LPEVENT m_pkTimeoutEvent;
		LPEVENT m_pkEndEvent;
		LPEVENT	m_pkResetFlagEvent;

		typedef struct STeamData
		{
			DWORD	dwID;
			CGuild * pkGuild;
			int		iMemberCount;
			int		iUsePotionPrice;
			int		iScore;
			LPCHARACTER pkChrFlag;
			LPCHARACTER pkChrFlagBase;
#ifdef __IMPROVED_GUILD_WAR__
			int iMaxPlayer;
			int iMaxScore;
			DWORD flags;
			int	custom_map_index;
#endif
			std::set<DWORD> set_pidJoiner;

			void Initialize();

			int GetAccumulatedJoinerCount(); // ������ ������ ��
			int GetCurJointerCount(); // ���� ������ ��

			void AppendMember(LPCHARACTER ch);
			void RemoveMember(LPCHARACTER ch);
		} TeamData;

		TeamData	m_TeamData[2];
#ifdef GUILD_WAR_COUNTER
		DWORD warID;
		std::map<DWORD, war_static_ptr> war_static;
#endif

		int		m_iObserverCount;
		DWORD		m_dwStartTime;
		BYTE		m_bTimeout;

		TGuildWarInfo	m_WarInfo;
#ifdef GUILD_WAR_COUNTER
		std::map<DWORD, bool>	m_set_pkChr;
#else
		CHARACTER_SET m_set_pkChr;
#endif
};

class CWarMapManager : public singleton<CWarMapManager>
{
	public:
		CWarMapManager();
		virtual ~CWarMapManager();

		bool		LoadWarMapInfo(const char * c_pszFileName);
		bool		IsWarMap(long lMapIndex);
		TWarMapInfo *	GetWarMapInfo(long lMapIndex);
		bool		GetStartPosition(long lMapIndex, BYTE bIdx, PIXEL_POSITION & pos);

		template <typename Func> Func for_each(Func f);
		long		CreateWarMap(const TGuildWarInfo & r_WarInfo, DWORD dwGuildID1, DWORD dwGuildID2
#ifdef __IMPROVED_GUILD_WAR__
			, int iMaxPlayer, int iMaxScore, DWORD flags, int custom_map_index
#endif
#ifdef GUILD_WAR_COUNTER
			, DWORD warID
#endif
		);
		void		DestroyWarMap(CWarMap* pMap);
		CWarMap *	Find(long lMapIndex);
		int		CountWarMap() { return m_mapWarMap.size(); }

		void		OnShutdown();

	private:
		std::map<long, TWarMapInfo *> m_map_kWarMapInfo;
		std::map<long, CWarMap *> m_mapWarMap;
};

template <typename Func> Func CWarMapManager::for_each(Func f)
{
	for (itertype(m_mapWarMap) it = m_mapWarMap.begin(); it != m_mapWarMap.end(); ++it)
		f(it->second);

	return f;
}

#endif
