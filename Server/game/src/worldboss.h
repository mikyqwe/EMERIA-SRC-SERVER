#pragma once

#ifdef __WORLD_BOSS_YUMA__
class CWorldBossManager : public singleton<CWorldBossManager>
{
	typedef struct Boss
	{
		LPCHARACTER ch;
		long map_index;
		int vnum;
		std::list<int> spawn_time;
		bool is_alive;
		long x, y;
		BYTE z;
		DWORD next_spawn;
		DWORD vid;
		const char* name;
		std::list<unsigned> m_listSeconds;
		Boss() :
			ch(NULL),
			map_index(0),
			vnum(0),
			spawn_time(0),
			is_alive(0),
			x(0),
			y(0),
			z(0),
			next_spawn(0),
			vid(0),
			name("Noname"),
			m_listSeconds({ 300, 600, 1200, 1800, 3600, 7200, 10800 })
		{}
	} BOSS;

	private:
		std::vector<Boss> m_listBosses;

	public:
		CWorldBossManager();
		virtual ~CWorldBossManager();

		bool								Initialize();

		void								Destroy();
		void								DestroyBossesInMap(int iMapIndex);

		void								Idle(int iPulse);
		void								GetWorldbossInfo(LPCHARACTER ch);

		bool								worldboss_regen_load(const char* filename, long lMapIndex, int base_x, int base_y);

		// Boss
		void                                OnDeathBoss(int iIndex);
		int									IsBoss(DWORD dwVID);
		LPCHARACTER							SpawnBoss(BOSS pkBoss);

	private:
		time_t								CalculateNextSpawnTime(std::list<int> spawnTimes);
};
#endif
