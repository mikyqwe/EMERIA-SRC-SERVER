#ifndef __INC_METIN_II_GAME_PVP_H__
#define __INC_METIN_II_GAME_PVP_H__

class CHARACTER;

// CPVP에는 DWORD 아이디 두개를 받아서 m_dwCRC를 만들어서 가지고 있는다.
// CPVPManager에서 이렇게 만든 CRC를 통해 검색한다.
class CPVP
{
	public:
		friend class CPVPManager;
#ifdef ENABLE_DECORUM
		enum
		{
			BATTLE_FLAG_VNUM1 = 4990,
		};
#endif
		typedef struct _player
		{
			DWORD	dwPID;
			DWORD	dwVID;
			bool	bAgree;
			bool	bCanRevenge;

			_player() : dwPID(0), dwVID(0), bAgree(false), bCanRevenge(false)
			{
			}
		} TPlayer;

#ifdef ENABLE_DECORUM
		CPVP(DWORD dwID1, DWORD dwID2, bool bIsDecored = false);
#else
		CPVP(DWORD dwID1, DWORD dwID2);
#endif
		CPVP(CPVP & v);
		~CPVP();

		void	Win(DWORD dwPID); // dwPID가 이겼다!
		bool	CanRevenge(DWORD dwPID); // dwPID가 복수할 수 있어?
		bool	IsFight();
		bool	Agree(DWORD dwPID);

		void	SetVID(DWORD dwPID, DWORD dwVID);
		void	Packet(bool bDelete = false);
		
#ifdef ENABLE_DECORUM
		bool	IsDecored(){return m_bIsDecored;};
		void	WinDecored(DWORD dwPID);
		void	SpawnFlag();
#endif
#ifdef ENABLE_RENEWAL_PVP
		bool	pvpSetting[PVP_BET];
		long long	pvpBet;
#endif

		void	SetLastFightTime();
		DWORD	GetLastFightTime();

		DWORD 	GetCRC() { return m_dwCRC; }

	protected:
		TPlayer	m_players[2];
		DWORD	m_dwCRC;
		bool	m_bRevenge;

		DWORD   m_dwLastFightTime;
#ifdef ENABLE_DECORUM
		bool	m_bIsDecored;
		LPCHARACTER	m_pkFlag;
#endif		
};

class CPVPManager : public singleton<CPVPManager>
{
	typedef std::map<DWORD, TR1_NS::unordered_set<CPVP*> > CPVPSetMap;

	public:
	CPVPManager();
	virtual ~CPVPManager();

#ifdef ENABLE_NEWSTUFF
	bool			IsFighting(LPCHARACTER pkChr);
	bool			IsFighting(DWORD dwPID);
#endif

#ifdef ENABLE_RENEWAL_PVP
	void			RemoveCharactersPvP(LPCHARACTER pkChr, LPCHARACTER pkVictim);
	bool			HasPvP(LPCHARACTER pkChr, LPCHARACTER pkVictim);
#endif

	void			Insert(LPCHARACTER pkChr, LPCHARACTER pkVictim
#ifdef ENABLE_DECORUM
	, bool bIsDecored
#endif
#ifdef ENABLE_RENEWAL_PVP
	, bool* pvpSetting, long long pvpBet
#endif
	);

	bool			CanAttack(LPCHARACTER pkChr, LPCHARACTER pkVictim);
#ifdef ENABLE_RENEWAL_PVP
	bool			Dead(LPCHARACTER pkChr, LPCHARACTER pkKiller);
#else
	bool			Dead(LPCHARACTER pkChr, DWORD dwKillerPID);
#endif
	void			Connect(LPCHARACTER pkChr);
	void			Disconnect(LPCHARACTER pkChr);

	void			SendList(LPDESC d);
	void			Delete(CPVP * pkPVP);

	void			Process();

	public:
	CPVP *			Find(DWORD dwCRC);
	protected:
	void			ConnectEx(LPCHARACTER pkChr, bool bDisconnect);

	std::map<DWORD, CPVP *>	m_map_pkPVP;
	CPVPSetMap		m_map_pkPVPSetByID;
};

#endif
